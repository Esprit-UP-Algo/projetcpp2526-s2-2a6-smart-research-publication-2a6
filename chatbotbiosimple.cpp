#include "chatbotbiosimple.h"

#include <QPainter>
#include <QPainterPath>
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
#include <QUrl>
#include <QHttpMultiPart>
#include <QIODevice>
#include <QByteArray>
#include <QDataStream>
#include <QLocale>
#include <QTimer>

#include "apiconfig.h"

static const QString SYSTEM_PROMPT =
    "Tu es un assistant intelligent intégré dans l'application SmartVision, "
    "un système complet de gestion de laboratoire de recherche scientifique.\n\n"
    "Tu connais parfaitement tous les modules de l'application :\n\n"

    "=== MODULE BIOSAMPLE (Échantillons biologiques) ===\n"
    "- Champs : Référence (unique), Type (ADN/ARN/Protéine/Cellule/Tissu/Organisme), "
    "Organisme source, Emplacement (format Cong:xx/Etag:xx), Température (°C), "
    "Quantité restante (µg), Date de collecte, Date d'expiration, Niveau de dangerosité (BSL-1/2/3).\n"
    "- Niveaux BSL : BSL-1 risque minimal, BSL-2 risque modéré, BSL-3 risque élevé.\n"
    "- Températures standards : -80°C (ARN/virus), -20°C (ADN/protéines), 4°C (anticorps).\n"
    "- Statuts : OK (>30j avant expiration), Bientôt expiré (<30j), Expiré, Haut risque (BSL-3).\n\n"

    "=== MODULE CONGÉLATEUR (Gestion du stockage) ===\n"
    "- Gestion des congélateurs et étagères du laboratoire.\n"
    "- Permet de localiser les échantillons par congélateur et étagère.\n"
    "- Format d'emplacement : Cong:C01/Etag:A3.\n"
    "- Filtres IA pour rechercher des emplacements disponibles.\n\n"

    "=== MODULE EXPÉRIENCE (Expériences scientifiques) ===\n"
    "- Champs : ID, Titre, Hypothèse, Date de début, Date de fin, Statut, Projet associé.\n"
    "- Statuts possibles : En cours, Terminée, Suspendue, Planifiée.\n"
    "- Lien avec les projets et les équipements utilisés.\n\n"

    "=== MODULE EMPLOYÉS (Gestion du personnel) ===\n"
    "- Champs : CIN (identifiant), Nom, Prénom, Rôle, Spécialisation, Qualification, "
    "Temps de travail, Laboratoire.\n"
    "- Rôles : Chercheur, Technicien, Responsable, Stagiaire, etc.\n"
    "- CRUD complet : ajouter, modifier, supprimer, rechercher par CIN/nom/rôle/spécialisation.\n\n"

    "=== MODULE ÉQUIPEMENT (Matériel de laboratoire) ===\n"
    "- Champs : Nom, Fabricant, Numéro de modèle, Date d'achat, Date dernière maintenance, "
    "Date prochaine maintenance, Statut, Localisation, Date limite de calibration, Expérience liée.\n"
    "- Statuts : Opérationnel, En maintenance, Hors service, En calibration.\n"
    "- Alertes de maintenance et calibration selon les dates.\n\n"

    "=== MODULE PROJETS (Gestion de projets de recherche) ===\n"
    "- Champs : Nom du projet, Domaine de recherche, Date début, Date fin, Budget (DT), "
    "Statut, Source de financement, Numéro d'approbation éthique, Nombre de publications.\n"
    "- Statuts : En cours, Terminé, Suspendu, Planifié.\n"
    "- Les projets regroupent les expériences et les échantillons.\n\n"

    "=== MODULE PUBLICATION (Publications scientifiques) ===\n"
    "- Champs : Titre, Journal, Année, DOI, Statut, Résumé (abstract), Projet lié, Employé auteur.\n"
    "- Statuts : Soumis, En révision, Accepté, Publié, Rejeté.\n"
    "- Lien avec les projets de recherche et les employés chercheurs.\n\n"

    "=== MODULE AUTHENTIFICATION ===\n"
    "- Connexion sécurisée avec captcha.\n"
    "- Gestion des comptes utilisateurs du laboratoire.\n\n"

    "Réponds toujours en français, de façon concise et utile. "
    "Tu peux aider avec toutes les fonctionnalités de SmartVision : "
    "CRUD, recherches, statistiques, conseils de gestion de laboratoire, "
    "interprétation des données, et bonnes pratiques scientifiques.";

static QByteArray le32(quint32 v)
{
    QByteArray b(4, '\0');
    b[0] = char(v & 0xFF);
    b[1] = char((v >> 8) & 0xFF);
    b[2] = char((v >> 16) & 0xFF);
    b[3] = char((v >> 24) & 0xFF);
    return b;
}

static QByteArray le16(quint16 v)
{
    QByteArray b(2, '\0');
    b[0] = char(v & 0xFF);
    b[1] = char((v >> 8) & 0xFF);
    return b;
}

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

    QLabel* titleLbl = new QLabel("Assistant SmartVision");
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
        "QScrollArea{ background:transparent; border:none; }"
        "QScrollBar:vertical{ width:8px; background:transparent; margin:0; }"
        "QScrollBar::handle:vertical{ background:rgba(255,255,255,0.55); border-radius:4px; min-height:20px; }"
        "QScrollBar::handle:vertical:hover{ background:rgba(255,255,255,0.80); }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical{ height:0; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical{ background:transparent; }"
    );
    m_scroll->viewport()->setStyleSheet("background:transparent;");
    m_scroll->setAttribute(Qt::WA_TranslucentBackground);
    m_scroll->viewport()->setAttribute(Qt::WA_TranslucentBackground);
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

    m_micBtn = new QPushButton("🎤");
    m_micBtn->setFixedSize(38, 38);
    m_micBtn->setCursor(Qt::PointingHandCursor);
    m_micBtn->setCheckable(true);
    m_micBtn->setToolTip("Activer/Désactiver le micro (Ctrl+M)");
    m_micBtn->setStyleSheet(
        "QPushButton{ background:rgba(59,130,246,0.16); border-radius:19px;"
        " color:#1d4ed8; font-size:15px; border:none; }"
        "QPushButton:hover{ background:rgba(59,130,246,0.30); }"
        "QPushButton:checked{ background:rgba(239,68,68,0.20); color:#dc2626; }"
        "QPushButton:disabled{ background:rgba(200,200,200,0.5); color:#999; }"
    );

    inputL->addWidget(m_input, 1);
    inputL->addWidget(m_micBtn);
    inputL->addWidget(m_clearBtn);
    inputL->addWidget(m_sendBtn);
    cardL->addWidget(inputBar);

    connect(m_sendBtn,  &QPushButton::clicked,    this, &ChatBotBioSimple::sendMessage);
    connect(m_clearBtn, &QPushButton::clicked,    this, &ChatBotBioSimple::clearConversation);
    connect(m_input,    &QLineEdit::returnPressed, this, &ChatBotBioSimple::sendMessage);
    connect(m_micBtn,   &QPushButton::clicked,    this, &ChatBotBioSimple::toggleMicro);
    QShortcut* micShortcut = new QShortcut(QKeySequence("Ctrl+M"), this);
    connect(micShortcut, &QShortcut::activated, this, &ChatBotBioSimple::toggleMicro);

    // TTS désactivé — les réponses restent uniquement en texte
    m_tts = nullptr;

    // ── Vidéo de fond (sans son) ──────────────────────────────────
    m_bgPlayer = new QMediaPlayer(this);
    m_bgSink   = new QVideoSink(this);
    m_bgPlayer->setVideoSink(m_bgSink);
    m_bgPlayer->setSource(QUrl("qrc:/new/prefix1/backchatbot.mp4"));
    // Pas de QAudioOutput → pas de son
    connect(m_bgSink, &QVideoSink::videoFrameChanged,
            this, [=](const QVideoFrame& frame) {
        m_bgFrame = frame;
        update();
    });
    connect(m_bgPlayer, &QMediaPlayer::mediaStatusChanged,
            this, [=](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia)
            m_bgPlayer->play();   // lecture en boucle
    });
    m_bgPlayer->play();

    // ── Welcome ──
    addMessage("Bonjour ! 👋 Je suis l'assistant SmartVision.\n"
               "Propulsé par Groq · Llama — posez-moi n'importe quelle\n"
               "question sur BioSample, Congélateur, Expériences,\n"
               "Employés, Équipements, Projets ou Publications !", false);
}

// ─────────────────────────────────────────────────────────────────
//  paintEvent — fond vidéo + zones semi-transparentes
// ─────────────────────────────────────────────────────────────────
void ChatBotBioSimple::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Clip arrondi
    QPainterPath clip;
    clip.addRoundedRect(rect(), 20, 20);
    p.setClipPath(clip);

    // Fond vidéo
    if (m_bgFrame.isValid()) {
        QImage img = m_bgFrame.toImage();
        if (!img.isNull())
            p.drawImage(rect(), img);
    } else {
        // Fallback tant que la vidéo n'est pas encore chargée
        p.fillPath(clip, QColor(30, 20, 120));
    }

    // Zone messages — voile très léger pour lisibilité des bulles
    p.fillRect(QRect(0, 66, width(), height() - 66 - 60),
               QColor(0, 0, 0, 55));

    // Barre de saisie
    p.fillRect(QRect(0, height() - 60, width(), 60),
               QColor(255, 255, 255, 210));
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

    QTimer::singleShot(50, this, [this](){
        m_scroll->verticalScrollBar()->setValue(m_scroll->verticalScrollBar()->maximum());
    });
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
    m_micBtn->setEnabled(false);

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
    body["model"]       = GROQ_API_MODEL;
    body["messages"]    = messages;
    body["temperature"] = 0.7;
    body["max_tokens"]  = 400;

    QJsonDocument doc(body);

    QUrl endpoint(GROQ_API_URL);
    QNetworkRequest req(endpoint);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", ("Bearer " + GROQ_API_KEY).toUtf8());

    // Ignore SSL peer verification (works without OpenSSL DLLs)
    QSslConfiguration ssl = QSslConfiguration::defaultConfiguration();
    ssl.setPeerVerifyMode(QSslSocket::VerifyNone);
    req.setSslConfiguration(ssl);

    QNetworkReply* rpl = m_net->post(req, doc.toJson());
    rpl->setProperty("requestType", "chat");
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
    // Read everything BEFORE deleteLater
    QByteArray data   = reply->readAll();
    bool netError     = (reply->error() != QNetworkReply::NoError);
    QString errStr    = reply->errorString();   // save before deleteLater
    const QString requestType = reply->property("requestType").toString();
    reply->deleteLater();

    if (requestType == "stt") {
        m_input->setEnabled(true);
        m_sendBtn->setEnabled(true);
        m_micBtn->setEnabled(true);
        m_input->setFocus();

        QJsonDocument sttDoc = QJsonDocument::fromJson(data);
        if (netError) {
            addMessage("Erreur STT : " + errStr, false);
            return;
        }
        if (!sttDoc.isObject()) {
            addMessage("Erreur STT : réponse invalide.", false);
            return;
        }

        const QJsonObject sttObj = sttDoc.object();
        if (sttObj.contains("error")) {
            const QString apiErr = sttObj["error"].toObject().value("message").toString();
            addMessage("Erreur STT : " + (apiErr.isEmpty() ? QString("transcription indisponible") : apiErr), false);
            return;
        }

        const QString transcribed = sttObj.value("text").toString().trimmed();
        if (transcribed.isEmpty()) {
            addMessage("Je n'ai pas bien entendu. Réessaie avec le micro.", false);
            return;
        }

        m_input->setText(transcribed);
        sendMessage();
        return;
    }

    removeTypingIndicator();
    m_input->setEnabled(true);
    m_sendBtn->setEnabled(true);
    m_micBtn->setEnabled(true);
    m_input->setFocus();

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
    addMessage("Bonjour ! 👋 Je suis l'assistant SmartVision.<br/>"
               "Propulsé par <b>Groq · Llama</b> — posez-moi n'importe quelle<br/>"
               "question sur BioSample, Congélateur, Expériences,<br/>"
               "Employés, Équipements, Projets ou Publications !", false, true);
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

void ChatBotBioSimple::keyPressEvent(QKeyEvent* event)
{
    // Prevent Enter/Return from closing the dialog (QDialog default behavior)
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        sendMessage();
        return;
    }
    // Escape closes the chatbot
    if (event->key() == Qt::Key_Escape) {
        close();
        return;
    }
    // All other keys pass normally (but NOT to QDialog to avoid accept/reject)
    QWidget::keyPressEvent(event);
}

void ChatBotBioSimple::toggleMicro()
{
    if (m_isRecording) {
        stopMicroCaptureAndTranscribe();
    } else {
        startMicroCapture();
    }
}

void ChatBotBioSimple::startMicroCapture()
{
    if (m_isRecording) return;

    QAudioFormat format;
    format.setSampleRate(16000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    m_audioSource = new QAudioSource(format, this);
    m_audioBuffer = new QBuffer(this);
    m_audioBuffer->open(QIODevice::WriteOnly);
    m_audioSource->start(m_audioBuffer);

    m_isRecording = true;
    m_micBtn->setChecked(true);
    m_input->setPlaceholderText("Écoute en cours... (Ctrl+M pour arrêter)");
}

void ChatBotBioSimple::stopMicroCaptureAndTranscribe()
{
    if (!m_isRecording) return;

    m_isRecording = false;
    m_micBtn->setChecked(false);
    m_input->setPlaceholderText("Écrivez ici...");

    QByteArray pcmData;
    if (m_audioSource) m_audioSource->stop();
    if (m_audioBuffer) {
        pcmData = m_audioBuffer->data();
        m_audioBuffer->close();
    }
    if (m_audioSource) {
        m_audioSource->deleteLater();
        m_audioSource = nullptr;
    }
    if (m_audioBuffer) {
        m_audioBuffer->deleteLater();
        m_audioBuffer = nullptr;
    }

    if (pcmData.size() < 2048) {
        addMessage("Audio trop court. Maintiens le micro un peu plus longtemps.", false);
        return;
    }

    m_input->setEnabled(false);
    m_sendBtn->setEnabled(false);
    m_micBtn->setEnabled(false);
    callSpeechToText(pcmData);
}

QByteArray ChatBotBioSimple::buildWavFromPcm16(const QByteArray& pcmData, int sampleRate, int channels) const
{
    const quint32 dataSize = quint32(pcmData.size());
    const quint16 bitsPerSample = 16;
    const quint32 byteRate = quint32(sampleRate * channels * (bitsPerSample / 8));
    const quint16 blockAlign = quint16(channels * (bitsPerSample / 8));

    QByteArray wav;
    wav.reserve(int(44 + dataSize));
    wav.append("RIFF", 4);
    wav.append(le32(36 + dataSize));
    wav.append("WAVE", 4);
    wav.append("fmt ", 4);
    wav.append(le32(16));
    wav.append(le16(1));
    wav.append(le16(quint16(channels)));
    wav.append(le32(quint32(sampleRate)));
    wav.append(le32(byteRate));
    wav.append(le16(blockAlign));
    wav.append(le16(bitsPerSample));
    wav.append("data", 4);
    wav.append(le32(dataSize));
    wav.append(pcmData);
    return wav;
}

void ChatBotBioSimple::callSpeechToText(const QByteArray& pcmData)
{
    const QByteArray wavData = buildWavFromPcm16(pcmData, 16000, 1);

    QUrl endpoint(GROQ_STT_API_URL);
    QNetworkRequest req(endpoint);
    req.setRawHeader("Authorization", ("Bearer " + GROQ_API_KEY).toUtf8());

    QSslConfiguration ssl = QSslConfiguration::defaultConfiguration();
    ssl.setPeerVerifyMode(QSslSocket::VerifyNone);
    req.setSslConfiguration(ssl);

    QHttpMultiPart* mp = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    auto addPart = [&](const QString& name, const QByteArray& val) {
        QHttpPart p;
        p.setHeader(QNetworkRequest::ContentDispositionHeader,
                    QString("form-data; name=\"%1\"").arg(name));
        p.setBody(val);
        mp->append(p);
    };

    addPart("model",           GROQ_STT_API_MODEL.toUtf8());
    addPart("language",        "fr");
    addPart("response_format", "json");
    addPart("temperature",     "0");
    // Hint de vocabulaire : aide Whisper à reconnaître les termes métier
    addPart("prompt",
            "BioSample, congélateur, expérience, équipement, projet, publication, employé, "
            "biosample, laboratoire, échantillon, réfrigérateur, statistiques, modifier, "
            "supprimer, ajouter, rechercher, exporter, PDF, SmartVision.");

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("audio/wav"));
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant("form-data; name=\"file\"; filename=\"speech.wav\""));
    filePart.setBody(wavData);
    mp->append(filePart);

    QNetworkReply* rpl = m_net->post(req, mp);
    mp->setParent(rpl);
    rpl->setProperty("requestType", "stt");
    connect(rpl, &QNetworkReply::sslErrors, rpl, [rpl](const QList<QSslError>&){
        rpl->ignoreSslErrors();
    });
}

void ChatBotBioSimple::speakAssistantText(const QString& text)
{
    if (!m_tts) return;
    if (m_tts->state() == QTextToSpeech::Speaking)
        m_tts->stop();

    // Strip markdown formatting before speaking
    QString clean = text;
    clean.replace(QRegularExpression("\\*\\*([^*]+)\\*\\*"), "\\1");
    clean.replace(QRegularExpression("\\*([^*]+)\\*"),       "\\1");
    clean.replace(QRegularExpression("#+\\s*"),              "");
    clean.replace(QRegularExpression("[`]{1,3}[^`]*[`]{1,3}"), "");
    clean.replace(QRegularExpression("[•\\-]\\s+"),          "");
    clean.replace(QRegularExpression("\\[([^\\]]+)\\]\\([^)]*\\)"), "\\1");
    clean = clean.simplified().trimmed();
    if (!clean.isEmpty())
        m_tts->say(clean);
}

