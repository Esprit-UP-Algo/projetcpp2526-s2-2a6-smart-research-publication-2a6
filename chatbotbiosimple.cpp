#include "chatbotbiosimple.h"

#include <QPainter>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QMouseEvent>
#include <QScrollBar>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSslConfiguration>
#include <QRegularExpression>
#include <cmath>

// ── API config ─────────────────────────────────────────────────
static const QString API_KEY = "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";

static const QString API_URL  = "https://api.groq.com/openai/v1/chat/completions";
static const QString MODEL    = "llama-3.1-8b-instant";

static const QString SYSTEM_PROMPT =
    "Tu es un assistant intelligent intégré dans l'application SmartVision, "
    "module BioSimple — un système de gestion d'échantillons biologiques.\n\n"
    "Tu connais parfaitement :\n"
    "- La table BIOSAMPLE avec les colonnes : REFERENCE_ECHANTILLON (unique), "
    "TYPE_ECHANTILLON (DNA/RNA/Protéine), ORGANISME_SOURCE, EMPLACEMENT_DE_STOCKAGE "
    "(format Cong:xx/Etag:xx), TEMPERATURE_DE_STOCKAGE (°C), QUANTITE_RESTANTE (µg), "
    "DATE_DE_COLLECTE, DATE_EXPIRATION, NIVEAU_DE_DANGEROSITE (BSL-1/BSL-2/BSL-3).\n"
    "- Les niveaux BSL : BSL-1 risque minimal, BSL-2 risque modéré, BSL-3 risque élevé.\n"
    "- Les températures : -80°C pour RNA/virus, -20°C pour DNA/protéines, "
    "2-8°C pour anticorps, 20-25°C pour réactifs stables.\n"
    "- Le statut d'expiration : OK (>30j), Bientôt expiré (<30j), Expiré (date dépassée), "
    "Haut risque (BSL-3).\n"
    "- Les opérations CRUD : Ajouter, Modifier, Supprimer, Afficher des échantillons.\n"
    "- L'unité de quantité : µg (microgrammes).\n"
    "- Le format d'emplacement : Cong:C01/Etag:A3 (congélateur/étagère).\n\n"
    "Réponds toujours en français, de façon concise et utile. "
    "Si la question ne concerne pas le module BioSimple, "
    "réponds poliment que tu es spécialisé dans ce module uniquement.";

// ─────────────────────────────────────────────────────────────────
//  Bubble widget
// ─────────────────────────────────────────────────────────────────
static QWidget* makeBubble(const QString& text, bool isUser, bool richText = false)
{
    QWidget* row = new QWidget;
    QHBoxLayout* hl = new QHBoxLayout(row);
    hl->setContentsMargins(8, 2, 8, 2);
    hl->setSpacing(8);

    if (!isUser) {
        QLabel* av = new QLabel("🤖");
        av->setFixedSize(32, 32);
        av->setStyleSheet(
            "background: qlineargradient(x1:0,y1:0,x2:1,y2:1,"
            " stop:0 #3b82f6, stop:1 #8b5cf6);"
            "border-radius:16px; font-size:16px;"
        );
        av->setAlignment(Qt::AlignCenter);
        hl->addWidget(av, 0, Qt::AlignBottom);
    }

    QLabel* lbl = new QLabel;
    lbl->setTextFormat(richText ? Qt::RichText : Qt::PlainText);
    lbl->setText(text);
    lbl->setWordWrap(true);
    lbl->setMaximumWidth(280);
    lbl->setTextInteractionFlags(Qt::TextSelectableByMouse);

    if (isUser) {
        lbl->setStyleSheet(
            "background: qlineargradient(x1:0,y1:0,x2:1,y2:1,"
            " stop:0 #7c3aed, stop:1 #4f46e5);"
            "color:white; border-radius:16px 4px 16px 16px;"
            "padding:10px 14px; font-size:13px;"
        );
        hl->addStretch(1);
        hl->addWidget(lbl);
        QLabel* av = new QLabel("👤");
        av->setFixedSize(32, 32);
        av->setStyleSheet(
            "background:rgba(255,255,255,0.25); border-radius:16px; font-size:16px;"
        );
        av->setAlignment(Qt::AlignCenter);
        hl->addWidget(av, 0, Qt::AlignBottom);
    } else {
        lbl->setStyleSheet(
            "background:rgba(255,255,255,0.92);"
            "color:#1e1b4b; border-radius:4px 16px 16px 16px;"
            "padding:10px 14px; font-size:13px;"
        );
        hl->addWidget(lbl);
        hl->addStretch(1);
    }

    return row;
}

// ─────────────────────────────────────────────────────────────────
//  Typing indicator  (animated dots)
// ─────────────────────────────────────────────────────────────────
static QWidget* makeTypingBubble()
{
    QWidget* row = new QWidget;
    row->setObjectName("typingRow");
    QHBoxLayout* hl = new QHBoxLayout(row);
    hl->setContentsMargins(8, 2, 8, 2);
    hl->setSpacing(8);

    QLabel* av = new QLabel("🤖");
    av->setFixedSize(32, 32);
    av->setStyleSheet(
        "background: qlineargradient(x1:0,y1:0,x2:1,y2:1,"
        " stop:0 #3b82f6, stop:1 #8b5cf6);"
        "border-radius:16px; font-size:16px;"
    );
    av->setAlignment(Qt::AlignCenter);

    QLabel* dots = new QLabel("● ● ●");
    dots->setStyleSheet(
        "background:rgba(255,255,255,0.92); color:#7c3aed;"
        "border-radius:4px 16px 16px 16px; padding:10px 14px; font-size:14px;"
    );

    // Pulse animation on opacity
    QGraphicsOpacityEffect* eff = new QGraphicsOpacityEffect(dots);
    dots->setGraphicsEffect(eff);
    QPropertyAnimation* anim = new QPropertyAnimation(eff, "opacity", dots);
    anim->setDuration(800);
    anim->setStartValue(0.3);
    anim->setEndValue(1.0);
    anim->setLoopCount(-1);
    anim->setEasingCurve(QEasingCurve::InOutSine);
    anim->start();

    hl->addWidget(av, 0, Qt::AlignBottom);
    hl->addWidget(dots);
    hl->addStretch(1);

    return row;
}

// ─────────────────────────────────────────────────────────────────
//  Format bot response: **bold** → <b>, "- item" → <ul><li>
// ─────────────────────────────────────────────────────────────────
QString ChatBotBioSimple::formatResponse(const QString& text)
{
    // Escape HTML entities first, then apply markdown-like formatting
    QStringList lines = text.split('\n');
    QString result;
    bool inList = false;

    auto applyBold = [](QString s) -> QString {
        static QRegularExpression re("\\*\\*(.+?)\\*\\*");
        s.replace(re, "<b>\\1</b>");
        return s;
    };

    for (const QString& rawLine : lines) {
        QString line    = rawLine.toHtmlEscaped();
        QString trimmed = line.trimmed();

        if (trimmed.isEmpty()) {
            if (inList) { result += "</ul>"; inList = false; }
            result += "<br/>";
            continue;
        }

        // Bullet list: lines starting with - or •
        if (trimmed.startsWith("- ") || trimmed.startsWith("• ") ||
            trimmed.startsWith("* ")) {
            QString item = trimmed.mid(2);
            item = applyBold(item);
            if (!inList) {
                result += "<ul style='margin:4px 0 4px 18px; padding:0;'>";
                inList = true;
            }
            result += "<li>" + item + "</li>";
        } else {
            if (inList) { result += "</ul>"; inList = false; }
            result += applyBold(trimmed) + "<br/>";
        }
    }

    if (inList) result += "</ul>";
    return result;
}

// ─────────────────────────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────────────────────────
ChatBotBioSimple::ChatBotBioSimple(QWidget* parent)
    : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(420, 580);

    m_net = new QNetworkAccessManager(this);
    connect(m_net, &QNetworkAccessManager::finished, this, &ChatBotBioSimple::onApiReply);

    // ── Root layout ──
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    QWidget* card = new QWidget;
    card->setObjectName("chatCard");
    card->setStyleSheet("#chatCard{ background:transparent; border-radius:20px; }");
    QVBoxLayout* cardL = new QVBoxLayout(card);
    cardL->setContentsMargins(0, 0, 0, 0);
    cardL->setSpacing(0);
    root->addWidget(card);

    // ── Header ──
    QWidget* header = new QWidget;
    header->setFixedHeight(66);
    header->setStyleSheet("background:transparent;");
    QHBoxLayout* headerL = new QHBoxLayout(header);
    headerL->setContentsMargins(16, 0, 16, 0);
    headerL->setSpacing(10);

    QLabel* robotIcon = new QLabel("🤖");
    robotIcon->setFixedSize(40, 40);
    robotIcon->setAlignment(Qt::AlignCenter);
    robotIcon->setStyleSheet(
        "background:rgba(255,255,255,0.20); border-radius:20px; font-size:20px;"
    );

    QLabel* titleLbl = new QLabel("Chatbot  BioSimple");
    titleLbl->setStyleSheet(
        "color:white; font-size:16px; font-weight:800; background:transparent;"
    );

    QLabel* modelLbl = new QLabel("Llama-3.1 · Groq");
    modelLbl->setStyleSheet(
        "color:rgba(255,255,255,0.65); font-size:10px; background:transparent;"
    );

    QVBoxLayout* titleCol = new QVBoxLayout;
    titleCol->setSpacing(0);
    titleCol->addWidget(titleLbl);
    titleCol->addWidget(modelLbl);

    QPushButton* closeBtn = new QPushButton("✕");
    closeBtn->setFixedSize(32, 32);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(
        "QPushButton{ background:rgba(255,255,255,0.20); border-radius:16px;"
        " color:white; font-size:14px; font-weight:700; border:none; }"
        "QPushButton:hover{ background:rgba(255,80,80,0.70); }"
    );

    headerL->addWidget(robotIcon);
    headerL->addLayout(titleCol);
    headerL->addStretch(1);
    headerL->addWidget(closeBtn);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);
    cardL->addWidget(header);

    // ── Messages area ──
    m_scroll = new QScrollArea;
    m_scroll->setWidgetResizable(true);
    m_scroll->setStyleSheet(
        "QScrollArea{ background:rgba(245,243,255,0.88); border:none; }"
        "QScrollBar:vertical{ width:4px; background:transparent; }"
        "QScrollBar::handle:vertical{ background:rgba(109,40,217,0.35); border-radius:2px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical{ height:0; }"
    );
    m_msgContainer = new QWidget;
    m_msgContainer->setStyleSheet("background:transparent;");
    m_msgLayout = new QVBoxLayout(m_msgContainer);
    m_msgLayout->setContentsMargins(4, 10, 4, 10);
    m_msgLayout->setSpacing(6);
    m_msgLayout->addStretch(1);
    m_scroll->setWidget(m_msgContainer);
    cardL->addWidget(m_scroll, 1);

    // Auto-scroll to bottom whenever content grows
    connect(m_scroll->verticalScrollBar(), &QScrollBar::rangeChanged,
            m_scroll->verticalScrollBar(), [this](int, int max){
        m_scroll->verticalScrollBar()->setValue(max);
    });

    // ── Input bar ──
    QWidget* inputBar = new QWidget;
    inputBar->setFixedHeight(60);
    inputBar->setStyleSheet(
        "background:rgba(255,255,255,0.92);"
        "border-top:1px solid rgba(109,40,217,0.15);"
    );
    QHBoxLayout* inputL = new QHBoxLayout(inputBar);
    inputL->setContentsMargins(12, 8, 12, 8);
    inputL->setSpacing(8);

    m_input = new QLineEdit;
    m_input->setPlaceholderText("Écrivez ici...");
    m_input->setStyleSheet(
        "QLineEdit{ border:none; background:transparent; font-size:13px;"
        " color:#1e1b4b; padding:4px 0; }"
    );

    m_clearBtn = new QPushButton("Clear");
    m_clearBtn->setFixedSize(52, 34);
    m_clearBtn->setCursor(Qt::PointingHandCursor);
    m_clearBtn->setStyleSheet(
        "QPushButton{ background:rgba(109,40,217,0.15); border-radius:10px;"
        " color:#5b21b6; font-size:11px; font-weight:700; border:none; }"
        "QPushButton:hover{ background:rgba(109,40,217,0.30); }"
    );

    m_sendBtn = new QPushButton("➤");
    m_sendBtn->setFixedSize(38, 38);
    m_sendBtn->setCursor(Qt::PointingHandCursor);
    m_sendBtn->setStyleSheet(
        "QPushButton{ background: qlineargradient(x1:0,y1:0,x2:1,y2:1,"
        " stop:0 #f59e0b, stop:1 #ef4444);"
        " border-radius:19px; color:white; font-size:16px; border:none; }"
        "QPushButton:hover{ background: qlineargradient(x1:0,y1:0,x2:1,y2:1,"
        " stop:0 #fbbf24, stop:1 #f87171); }"
        "QPushButton:disabled{ background:rgba(200,200,200,0.5); }"
    );

    inputL->addWidget(m_input, 1);
    inputL->addWidget(m_clearBtn);
    inputL->addWidget(m_sendBtn);
    cardL->addWidget(inputBar);

    connect(m_sendBtn,  &QPushButton::clicked,    this, &ChatBotBioSimple::sendMessage);
    connect(m_clearBtn, &QPushButton::clicked,    this, &ChatBotBioSimple::clearConversation);
    connect(m_input,    &QLineEdit::returnPressed, this, &ChatBotBioSimple::sendMessage);

    // ── Background animation ──
    m_bgTimer = new QTimer(this);
    connect(m_bgTimer, &QTimer::timeout, this, &ChatBotBioSimple::animateBg);
    m_bgTimer->start(30);

    // ── Welcome ──
    addMessage("Bonjour ! 👋 Je suis votre assistant BioSimple.\n"
               "Propulsé par GPT-4.1-mini — posez-moi n'importe quelle\n"
               "question sur vos échantillons biologiques !", false);
}

// ─────────────────────────────────────────────────────────────────
//  Background animation
// ─────────────────────────────────────────────────────────────────
void ChatBotBioSimple::animateBg()
{
    m_bgAngle += 0.8f;
    if (m_bgAngle >= 360.0f) m_bgAngle -= 360.0f;
    update();
}

void ChatBotBioSimple::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    float a = m_bgAngle * 3.14159f / 180.0f;

    int r1 = 30  + int(25 * std::sin(a));
    int g1 = 20  + int(10 * std::cos(a * 0.7f));
    int b1 = 160 + int(60 * std::sin(a * 0.5f));
    int r2 = 90  + int(50 * std::cos(a * 0.8f));
    int g2 = 0   + int(20 * std::sin(a * 1.2f));
    int b2 = 210 + int(45 * std::cos(a * 0.6f));
    int r3 = 140 + int(60 * std::sin(a * 0.4f));
    int g3 = 30  + int(20 * std::cos(a * 0.9f));
    int b3 = 180 + int(50 * std::sin(a * 1.1f));

    QLinearGradient grad(0, 0, width(), height());
    grad.setColorAt(0.0, QColor(r1, g1, b1));
    grad.setColorAt(0.5, QColor(r2, g2, b2));
    grad.setColorAt(1.0, QColor(r3, g3, b3));

    p.setPen(Qt::NoPen);
    p.setBrush(grad);
    p.drawRoundedRect(rect(), 20, 20);

    // Floating blobs
    auto blob = [&](float cx, float cy, float radius, QColor c) {
        QRadialGradient rg(cx, cy, radius);
        rg.setColorAt(0, c);
        rg.setColorAt(1, QColor(c.red(), c.green(), c.blue(), 0));
        p.fillRect(rect(), rg);
    };

    blob(width() * 0.15f + 30 * std::cos(a * 0.3f),
         height() * 0.12f + 20 * std::sin(a * 0.5f),
         80, QColor(255, 255, 255, 30));

    blob(width() * 0.80f + 40 * std::sin(a * 0.4f),
         height() * 0.08f + 25 * std::cos(a * 0.6f),
         70, QColor(200, 100, 255, 35));

    blob(width() * 0.50f + 35 * std::cos(a * 0.7f),
         height() * 0.04f + 15 * std::sin(a * 0.9f),
         55, QColor(100, 200, 255, 25));

    // Messages area
    p.fillRect(QRect(0, 66, width(), height() - 66 - 60),
               QColor(245, 243, 255, 220));

    // Input bar
    p.fillRect(QRect(0, height() - 60, width(), 60),
               QColor(255, 255, 255, 235));
}

// ─────────────────────────────────────────────────────────────────
//  Add message bubble
// ─────────────────────────────────────────────────────────────────
void ChatBotBioSimple::addMessage(const QString& text, bool isUser, bool richText)
{
    QWidget* bubble = makeBubble(text, isUser, richText);

    QGraphicsOpacityEffect* eff = new QGraphicsOpacityEffect(bubble);
    bubble->setGraphicsEffect(eff);
    QPropertyAnimation* anim = new QPropertyAnimation(eff, "opacity");
    anim->setDuration(300);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);

    int pos = m_msgLayout->count() - 1;
    if (pos < 0) pos = 0;
    m_msgLayout->insertWidget(pos, bubble);
    anim->start(QAbstractAnimation::DeleteWhenStopped);

}

// ─────────────────────────────────────────────────────────────────
//  Typing indicator
// ─────────────────────────────────────────────────────────────────
void ChatBotBioSimple::addTypingIndicator()
{
    m_typingWidget = makeTypingBubble();
    int pos = m_msgLayout->count() - 1;
    if (pos < 0) pos = 0;
    m_msgLayout->insertWidget(pos, m_typingWidget);
}

void ChatBotBioSimple::removeTypingIndicator()
{
    if (m_typingWidget) {
        m_msgLayout->removeWidget(m_typingWidget);
        delete m_typingWidget;
        m_typingWidget = nullptr;
    }
}

// ─────────────────────────────────────────────────────────────────
//  Send message → OpenAI
// ─────────────────────────────────────────────────────────────────
void ChatBotBioSimple::sendMessage()
{
    QString text = m_input->text().trimmed();
    if (text.isEmpty()) return;
    m_input->clear();
    m_input->setEnabled(false);
    m_sendBtn->setEnabled(false);

    addMessage(text, true);
    m_history.append({"user", text});
    m_lastUserMsg = text;

    addTypingIndicator();
    callOpenAI(text);
}

// ─────────────────────────────────────────────────────────────────
//  HTTP call to OpenAI
// ─────────────────────────────────────────────────────────────────
void ChatBotBioSimple::callOpenAI(const QString& /*userMessage*/)
{
    // Build messages array
    QJsonArray messages;

    // System prompt
    QJsonObject sys;
    sys["role"]    = "system";
    sys["content"] = SYSTEM_PROMPT;
    messages.append(sys);

    // History (last 10 exchanges max for token economy)
    int start = qMax(0, m_history.size() - 20);
    for (int i = start; i < m_history.size(); ++i) {
        QJsonObject msg;
        msg["role"]    = m_history[i].first;
        msg["content"] = m_history[i].second;
        messages.append(msg);
    }

    QJsonObject body;
    body["model"]       = MODEL;
    body["messages"]    = messages;
    body["temperature"] = 0.7;
    body["max_tokens"]  = 400;

    QJsonDocument doc(body);

    QUrl endpoint(API_URL);
    QNetworkRequest req(endpoint);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", ("Bearer " + API_KEY).toUtf8());

    // Ignore SSL peer verification (works without OpenSSL DLLs)
    QSslConfiguration ssl = QSslConfiguration::defaultConfiguration();
    ssl.setPeerVerifyMode(QSslSocket::VerifyNone);
    req.setSslConfiguration(ssl);

    QNetworkReply* rpl = m_net->post(req, doc.toJson());
    // Ignore SSL errors on this specific reply
    connect(rpl, &QNetworkReply::sslErrors, rpl, [rpl](const QList<QSslError>&){
        rpl->ignoreSslErrors();
    });
}

// ─────────────────────────────────────────────────────────────────
//  Handle API response
// ─────────────────────────────────────────────────────────────────
void ChatBotBioSimple::onApiReply(QNetworkReply* reply)
{
    removeTypingIndicator();
    m_input->setEnabled(true);
    m_sendBtn->setEnabled(true);
    m_input->setFocus();

    // Read everything BEFORE deleteLater
    QByteArray data   = reply->readAll();
    bool netError     = (reply->error() != QNetworkReply::NoError);
    QString errStr    = reply->errorString();   // save before deleteLater
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(data);

    // Always check JSON body first (contains real error msg even on HTTP errors)
    if (doc.isObject() && doc.object().contains("error")) {
        QString errMsg = doc.object()["error"].toObject()["message"].toString();
        if (errMsg.isEmpty()) errMsg = data;
        addMessage("Erreur API : " + errMsg, false);
        return;
    }

    if (netError) {
        addMessage("Erreur réseau : " + errStr, false);
        return;
    }

    if (!doc.isObject()) {
        addMessage("Erreur : réponse invalide de l'API.", false);
        return;
    }

    QJsonObject obj = doc.object();

    QJsonArray choices = obj["choices"].toArray();
    if (choices.isEmpty()) {
        addMessage("❌ Aucune réponse reçue.", false);
        return;
    }

    QString content = choices[0].toObject()["message"].toObject()["content"].toString().trimmed();
    if (content.isEmpty()) {
        addMessage("❌ Réponse vide.", false);
        return;
    }

    // Save assistant response in history (raw text)
    m_history.append({"assistant", content});

    // Display with HTML formatting (bold + bullet lists)
    addMessage(formatResponse(content), false, true);
}

// ─────────────────────────────────────────────────────────────────
//  Clear conversation
// ─────────────────────────────────────────────────────────────────
void ChatBotBioSimple::clearConversation()
{
    // Remove all message widgets (keep trailing stretch at index count-1)
    while (m_msgLayout->count() > 1) {
        QLayoutItem* item = m_msgLayout->takeAt(0);
        if (item->widget()) delete item->widget();
        delete item;
    }
    m_history.clear();
    m_lastUserMsg.clear();

    // Re-show welcome message
    addMessage("Bonjour ! 👋 Je suis votre assistant BioSimple.<br/>"
               "Propulsé par GPT-4.1-mini — posez-moi n'importe quelle<br/>"
               "question sur vos échantillons biologiques !", false, true);
}

// ─────────────────────────────────────────────────────────────────
//  Drag
// ─────────────────────────────────────────────────────────────────
void ChatBotBioSimple::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton && e->pos().y() < 66) {
        m_dragging = true;
        m_dragPos  = e->globalPosition().toPoint() - frameGeometry().topLeft();
    }
}
void ChatBotBioSimple::mouseMoveEvent(QMouseEvent* e)
{
    if (m_dragging)
        move(e->globalPosition().toPoint() - m_dragPos);
}
void ChatBotBioSimple::mouseReleaseEvent(QMouseEvent*)
{
    m_dragging = false;
}

