#include "cong.h"

#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QSslError>
#include <QSslConfiguration>
#include <QTime>
#include <QDateTime>
#include <QTextToSpeech>
#include <QSerialPortInfo>
#include <QDebug>
#include "apiconfig.h"

// ─── Emplacement helpers ──────────────────────────────────────
static QString parseCong(const QString& emp) {
    int s = emp.indexOf("Cong:") + 5, e = emp.indexOf("/", s);
    return (s >= 5 && e > s) ? emp.mid(s, e - s) : QString();
}
static QString parseEtag(const QString& emp) {
    int s = emp.indexOf("Etag:") + 5;
    return s >= 5 ? emp.mid(s) : QString();
}
static int etageNum(const QString& etage) {
    QString s = etage.trimmed();
    QString digits;
    for (int i = s.length() - 1; i >= 0; --i) {
        if (s[i].isDigit()) digits.prepend(s[i]);
        else break;
    }
    if (!digits.isEmpty()) {
        int n = digits.toInt();
        if (n > 0) return n;
    }
    return 1;
}
static int etageToRow(const QString& etage) {
    return qBound(0, FreezerWidget::N_SHELVES - etageNum(etage), FreezerWidget::N_SHELVES - 1);
}
static QString rowToLabel(int row) {
    return QString("Étage %1").arg(FreezerWidget::N_SHELVES - row);
}

static int badgeFrigoIdByUid(const QString& uid)
{
    if (uid.trimmed().isEmpty()) return -1;

    const QString cleanUid  = uid.trimmed().toUpper();
    const QString strippedUid = QString(cleanUid).remove(':').remove(' ');

    // Match exact UID (Arduino format "AB:12:CD:34")
    QSqlQuery q;
    q.prepare("SELECT \"FRIGO_ID\" FROM \"BADGES\" WHERE UPPER(\"UID\") = :uid");
    q.bindValue(":uid", cleanUid);
    if (q.exec() && q.next() && !q.value(0).isNull())
        return q.value(0).toInt();

    // Match UID without colons (DB format "AB12CD34")
    QSqlQuery q2;
    q2.prepare("SELECT \"FRIGO_ID\" FROM \"BADGES\" "
               "WHERE UPPER(REPLACE(REPLACE(\"UID\",':',''),' ','')) = :uid");
    q2.bindValue(":uid", strippedUid);
    if (q2.exec() && q2.next() && !q2.value(0).isNull())
        return q2.value(0).toInt();

    // Legacy column fallback
    QSqlQuery q3;
    q3.prepare("SELECT \"FRIGO_ID\" FROM \"BADGES\" "
               "WHERE UPPER(REPLACE(REPLACE(\"ADDRESS\",':',''),' ','')) = :uid");
    q3.bindValue(":uid", strippedUid);
    if (q3.exec() && q3.next() && !q3.value(0).isNull())
        return q3.value(0).toInt();

    return -1;
}

static QString congDoorLabel(int doorNum)
{
    return doorNum > 0
        ? QString("C%1").arg(doorNum, 2, 10, QChar('0'))
        : QString("porte inconnue");
}

static bool isArduinoCongDoor(int doorNum)
{
    return doorNum == 1 || doorNum == 2;
}

static int congNumberFromName(const QString& name)
{
    const QString s = name.trimmed();
    QString digits;
    for (int i = s.length() - 1; i >= 0; --i) {
        if (s[i].isDigit())
            digits.prepend(s[i]);
        else if (!digits.isEmpty())
            break;
    }

    bool ok = false;
    const int value = digits.toInt(&ok);
    return (ok && value > 0) ? value : -1;
}

static QString congTempBlockedMessage(int doorNum, double temp, double threshold)
{
    return QString("⛔  Impossible d'ouvrir %1 : température %2 °C dépasse le seuil %3 °C. Attendez quelques minutes.")
        .arg(congDoorLabel(doorNum))
        .arg(temp, 0, 'f', 1)
        .arg(threshold, 0, 'f', 1);
}

static double congThresholdById(int frigoId, double fallback = 32.0)
{
    if (frigoId <= 0) return fallback;

    QSqlQuery q;
    q.prepare("SELECT \"TEMP_THRESHOLD\" FROM \"FRIGO\" WHERE \"ID\" = :id");
    q.bindValue(":id", frigoId);
    if (q.exec() && q.next() && !q.value(0).isNull()) {
        bool ok = false;
        const double threshold = q.value(0).toDouble(&ok);
        if (ok && threshold > 0.0)
            return threshold;
    }

    QSqlQuery legacy;
    legacy.prepare("SELECT \"THRESHOLD\" FROM \"FRIGO\" WHERE \"ID\" = :id");
    legacy.bindValue(":id", frigoId);
    if (legacy.exec() && legacy.next() && !legacy.value(0).isNull()) {
        bool ok = false;
        const double threshold = legacy.value(0).toDouble(&ok);
        if (ok && threshold > 0.0)
            return threshold;
    }
    return fallback;
}

static void updateCongFrigoStatus(int frigoId, const QString& status)
{
    if (frigoId <= 0) return;

    QSqlQuery q;
    q.prepare("UPDATE \"FRIGO\" SET \"STATUS\" = :status WHERE \"ID\" = :id");
    q.bindValue(":status", status);
    q.bindValue(":id", frigoId);
    if (!q.exec())
        qWarning() << "[DB] update FRIGO.STATUS:" << q.lastError().text();
}

// ═══════════════════════════════════════════════════════════════
// AiBubble — floating draggable AI response window
// ═══════════════════════════════════════════════════════════════
AiBubble::~AiBubble()
{
    if (m_tts) {
        disconnect(m_tts, nullptr, this, nullptr);
        if (m_tts->state() == QTextToSpeech::Speaking ||
            m_tts->state() == QTextToSpeech::Synthesizing)
            m_tts->stop();
    }
}

void AiBubble::resetSpeechUi()
{
    if (!m_speakBtn) return;
    m_speakBtn->setChecked(false);
    m_speakBtn->setText("🔊");
    m_speakBtn->setToolTip("Lire le texte");
}

void AiBubble::requestSpeechStop()
{
    resetSpeechUi();
    if (!m_tts) return;
    if (m_speechStopRequested) return;
    if (m_tts->state() != QTextToSpeech::Speaking) {
        m_speechStopRequested = false;
        return;
    }

    m_speechStopRequested = true;
    QTimer::singleShot(0, this, [this](){
        if (!m_tts) return;
        if (m_tts->state() == QTextToSpeech::Speaking)
            m_tts->stop();
    });
}

AiBubble::AiBubble(QWidget* parent)
    : QFrame(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
{
    setFixedSize(520, 420);
    setAttribute(Qt::WA_DeleteOnClose,        false);
    setAttribute(Qt::WA_QuitOnClose,          false);
    setAttribute(Qt::WA_ShowWithoutActivating, true);

    m_videoBg = new QLabel(this);
    m_videoBg->setGeometry(0, 0, width(), height());
    m_videoBg->setScaledContents(true);
    m_videoBg->setStyleSheet("background:rgba(6,14,28,0.94); border-radius:14px;");

    m_player = new QMediaPlayer(this);
    auto* sink = new QVideoSink(this);
    m_player->setVideoSink(sink);
    m_player->setSource(QUrl("qrc:/new/prefix1/msgbio.mp4"));
    m_player->setLoops(QMediaPlayer::Infinite);
    connect(sink, &QVideoSink::videoFrameChanged, this, [this](const QVideoFrame& frame) {
        QImage img = frame.toImage();
        if (!img.isNull())
            m_videoBg->setPixmap(QPixmap::fromImage(img));
    });

    m_overlay = new QWidget(this);
    m_overlay->setGeometry(rect());
    m_overlay->setStyleSheet("background: rgba(6, 14, 28, 0.76); border-radius: 14px;");

    auto* vl = new QVBoxLayout(m_overlay);
    vl->setContentsMargins(12, 10, 12, 10);
    vl->setSpacing(7);

    auto* titleRow = new QHBoxLayout;
    titleRow->setContentsMargins(0, 0, 0, 0);
    auto* titleIco = new QLabel("🤖");
    titleIco->setStyleSheet("font-size:16px; background:transparent; border:none;");
    auto* titleLbl = new QLabel("Assistant IA Biologie");
    titleLbl->setMinimumWidth(210);
    titleLbl->setStyleSheet(
        "color: rgba(0,232,168,0.95); font-weight:900; font-size:14px;"
        "background:transparent; border:none;");

    m_tts = new QTextToSpeech(this);
    m_tts->setLocale(QLocale(QLocale::French));

    m_speakBtn = new QPushButton("🔊");
    m_speakBtn->setFixedSize(28, 28);
    m_speakBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_speakBtn->setCursor(Qt::PointingHandCursor);
    m_speakBtn->setToolTip("Lire le texte");
    m_speakBtn->setStyleSheet(
        "QPushButton{ background:rgba(0,232,168,0.25); color:white; border:none;"
        " border-radius:14px; font-size:12px; padding:0px;"
        " min-width:28px; max-width:28px; min-height:28px; max-height:28px; }"
        "QPushButton:hover{ background:rgba(0,232,168,0.55); }"
        "QPushButton:checked{ background:rgba(0,180,120,0.75); }");
    m_speakBtn->setCheckable(true);

    connect(m_speakBtn, &QPushButton::clicked, this, [this](bool checked){
        if (!m_tts) return;
        if (checked) {
            const QString plain = m_textBrowser->toPlainText().trimmed();
            if (!plain.isEmpty()) {
                m_speechStopRequested = false;
                m_speakBtn->setText("🔇");
                m_speakBtn->setToolTip("Arrêter la lecture");
                m_tts->say(plain);
            } else {
                m_speakBtn->setChecked(false);
            }
        } else {
            requestSpeechStop();
        }
    });

    connect(m_tts, &QTextToSpeech::stateChanged, this,
            [this](QTextToSpeech::State state){
        if (!m_speakBtn) return;
        if (state == QTextToSpeech::Ready || state == QTextToSpeech::Error) {
            m_speechStopRequested = false;
            resetSpeechUi();
        }
    }, Qt::QueuedConnection);

    auto* closeBtn = new QPushButton("✕");
    closeBtn->setFixedSize(28, 28);
    closeBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(
        "QPushButton{ background:rgba(220,55,48,0.85); color:white; border:none;"
        " border-radius:14px; font-weight:900; font-size:12px; padding:0px;"
        " min-width:28px; max-width:28px; min-height:28px; max-height:28px; }"
        "QPushButton:hover{ background:rgba(255,75,65,1.0); }");
    connect(closeBtn, &QPushButton::clicked, this, [this](){
        resetSpeechUi();
        if (m_tts && !m_speechStopRequested) {
            m_speechStopRequested = true;
            if (m_tts->state() == QTextToSpeech::Speaking ||
                m_tts->state() == QTextToSpeech::Synthesizing)
                m_tts->stop();
            else
                m_speechStopRequested = false;
        }
        setVisible(false);
    });
    titleRow->addWidget(titleIco);
    titleRow->addWidget(titleLbl, 1);
    titleRow->addWidget(m_speakBtn);
    titleRow->addWidget(closeBtn);
    vl->addLayout(titleRow);

    auto* sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("border: 1px solid rgba(0,232,168,0.28); background:transparent;");
    vl->addWidget(sep);

    m_statusLbl = new QLabel("⏳ Analyse en cours…");
    m_statusLbl->setStyleSheet(
        "color:rgba(0,200,140,0.80); font-size:11px; background:transparent; border:none;");
    vl->addWidget(m_statusLbl);

    m_textBrowser = new QTextBrowser(m_overlay);
    m_textBrowser->setOpenExternalLinks(true);
    m_textBrowser->setOpenLinks(true);
    m_textBrowser->setStyleSheet(
        "QTextBrowser{ background:transparent; color:rgba(235,245,255,0.93);"
        " font-size:13px; border:none; selection-background-color:rgba(0,200,140,0.35); }"
        "QScrollBar:vertical{ background:transparent; width:5px; margin:0; }"
        "QScrollBar::handle:vertical{ background:rgba(0,232,168,0.40); border-radius:2px; }"
        "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{ height:0; }");
    m_textBrowser->document()->setDefaultStyleSheet(
        "a { color: #00e8a8; text-decoration: underline; }"
        "b { color: #7dd3fc; }"
        "h3 { color: #fbbf24; margin:2px 0; }");
    vl->addWidget(m_textBrowser, 1);

    hide();
}

void AiBubble::resizeEvent(QResizeEvent* e) {
    QFrame::resizeEvent(e);
    m_videoBg->setGeometry(rect());
    m_overlay->setGeometry(rect());
}

void AiBubble::showResponse(const QString& html) {
    requestSpeechStop();
    if (m_player && m_player->playbackState() != QMediaPlayer::PlayingState)
        m_player->play();
    m_statusLbl->hide();
    m_textBrowser->setHtml(html);
    m_textBrowser->show();
    if (!isVisible()) {
        if (parentWidget()) {
            QRect pr = parentWidget()->geometry();
            move(pr.right() - 400, pr.center().y() - 165);
        }
    }
    show();
    raise();
}

void AiBubble::hideResponse() {
    requestSpeechStop();
    if (m_player)
        m_player->pause();
    hide();
}

void AiBubble::closeEvent(QCloseEvent* event)
{
    event->ignore();
    requestSpeechStop();
    if (m_player)
        m_player->pause();
    hide();
}

void AiBubble::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragPos  = e->globalPosition().toPoint() - frameGeometry().topLeft();
        e->accept();
    } else {
        QFrame::mousePressEvent(e);
    }
}
void AiBubble::mouseMoveEvent(QMouseEvent* e) {
    if (m_dragging && (e->buttons() & Qt::LeftButton)) {
        move(e->globalPosition().toPoint() - m_dragPos);
        e->accept();
    } else {
        QFrame::mouseMoveEvent(e);
    }
}
void AiBubble::mouseReleaseEvent(QMouseEvent* e) {
    m_dragging = false;
    QFrame::mouseReleaseEvent(e);
}

// ═══════════════════════════════════════════════════════════════
// FreezerWidget
// ═══════════════════════════════════════════════════════════════
FreezerWidget::FreezerWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(320, 500);
    setMaximumWidth(460);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    m_data.resize(N_SHELVES, QVector<Slot>(N_SLOTS));

    m_doorAnim = new QPropertyAnimation(this, "doorOpen", this);
    m_doorAnim->setDuration(700);
    m_doorAnim->setEasingCurve(QEasingCurve::InOutCubic);

    m_clockTimer = new QTimer(this);
    m_clockTimer->setInterval(500);
    connect(m_clockTimer, &QTimer::timeout, this, [this]() {
        m_colonVisible = !m_colonVisible;
        if (m_doorOpen < 0.95f) update();
    });
    m_clockTimer->start();

    m_logoPixmap = QPixmap(":/image/smartvision.png");
}

void FreezerWidget::openDoor() {
    m_doorAnim->stop();
    m_doorAnim->setStartValue(m_doorOpen);
    m_doorAnim->setEndValue(1.0f);
    m_doorAnim->start();
}

void FreezerWidget::closeDoor() {
    m_doorAnim->stop();
    m_doorAnim->setStartValue(m_doorOpen);
    m_doorAnim->setEndValue(0.0f);
    m_doorAnim->start();
}

// Called by SERVO Arduino events or manual open/close
void FreezerWidget::setDoorState(bool open, bool animate) {
    if (animate) {
        if (open) openDoor(); else closeDoor();
    } else {
        m_doorAnim->stop();
        m_doorOpen = open ? 1.0f : 0.0f;
        update();
    }
}

// Called by TEMP Arduino events to show live temperature on door LCD
void FreezerWidget::setCurrentTemperature(double celsius) {
    m_currentTemp = celsius;
    m_hasTemp     = true;
    if (m_doorOpen < 1.0f) update();
}

void FreezerWidget::setData(const QVector<QVector<Slot>>& shelves) {
    m_data = shelves; m_selShelf = m_selSlot = -1; update();
}
void FreezerWidget::selectSlot(int shelf, int slot) {
    m_selShelf = shelf; m_selSlot = slot; update();
}
void FreezerWidget::clearSelection() { selectSlot(-1, -1); }

// ── Geometry ─────────────────────────────────────────────────────
static const float RDX = 52.0f;
static const float RDY = -34.0f;
static const float R_LABEL_W  = 62.0f;
static const float R_SLOT_GAP = 3.0f;
static const float R_SHELF_TH = 8.0f;

struct RGeo {
    float cL,cR,cT,cB;
    float iL,iR,iT,iB;
    float shH;
    float sX1,sX2,sW;
};
static RGeo makeRGeo(float W, float H) {
    RGeo g;
    g.cL = 14;          g.cR = W - RDX - 14;
    g.cT = -RDY + 10;   g.cB = H - 66;
    g.iL = g.cL + 18;   g.iR = g.cR - 12;
    g.iT = g.cT + 22;   g.iB = g.cB - 16;
    g.shH  = (g.iB - g.iT) / float(FreezerWidget::N_SHELVES);
    g.sX1  = g.iL + R_LABEL_W + 5;
    g.sX2  = g.iR - 5;
    float avail = g.sX2 - g.sX1 - R_SLOT_GAP * (FreezerWidget::N_SLOTS - 1);
    g.sW   = avail / float(FreezerWidget::N_SLOTS);
    return g;
}

QRectF FreezerWidget::bodyRect() const {
    RGeo g = makeRGeo(width(), height());
    return QRectF(g.cL, g.cT, g.cR - g.cL, g.cB - g.cT);
}
QRectF FreezerWidget::innerRect() const {
    RGeo g = makeRGeo(width(), height());
    return QRectF(g.iL, g.iT, g.iR - g.iL, g.iB - g.iT);
}
QRectF FreezerWidget::shelfBand(int row) const {
    RGeo g = makeRGeo(width(), height());
    return QRectF(g.iL, g.iT + row * g.shH, g.iR - g.iL, g.shH);
}
QRectF FreezerWidget::slotRectF(int row, int col) const {
    RGeo g = makeRGeo(width(), height());
    float sy1   = g.iT + row * g.shH;
    float sfY   = sy1 + g.shH - R_SHELF_TH;
    float slotH = sfY - sy1 - 5;
    float slotX = g.sX1 + col * (g.sW + R_SLOT_GAP);
    return QRectF(slotX, sy1 + 3, g.sW, slotH);
}

void FreezerWidget::drawPin(QPainter& p, QRectF sr) const {
    float cx = sr.center().x(), tipY = sr.top() - 2.0f;
    float r  = 12.0f, cyF = tipY - r * 2.2f;
    QPainterPath path;
    path.moveTo(cx, tipY);
    path.arcTo(QRectF(cx-r, cyF-r, r*2, r*2), 210.0f, 300.0f);
    path.lineTo(cx, tipY);
    p.setBrush(QColor(0,0,0,55)); p.setPen(Qt::NoPen);
    p.drawPath(path.translated(2, 3));
    QRadialGradient rg(cx - r*0.3f, cyF - r*0.3f, r*1.5f);
    rg.setColorAt(0, QColor(255, 80, 80));
    rg.setColorAt(0.6, QColor(210, 40, 40));
    rg.setColorAt(1, QColor(150, 15, 15));
    p.setBrush(rg);
    p.setPen(QPen(QColor(120, 10, 10), 1.2));
    p.drawPath(path);
    float hr = r * 0.38f;
    p.setBrush(QColor(255,255,255,200)); p.setPen(Qt::NoPen);
    p.drawEllipse(QRectF(cx-hr, cyF-hr, hr*2, hr*2));
}

// ═══════════════════════════════════════════════════════════════════
// paintEvent — 3-D freezer rendering (design inchangé)
// ═══════════════════════════════════════════════════════════════════
void FreezerWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    float W = width(), H = height();
    RGeo g = makeRGeo(W, H);

    auto PT = [&](float x, float y, float z = 1.0f) -> QPointF {
        return QPointF(x + RDX*z, y + RDY*z);
    };
    auto quad = [](QPointF a, QPointF b, QPointF c, QPointF d) {
        return QPolygonF(QVector<QPointF>{a,b,c,d});
    };

    // Cabinet right side
    {
        QLinearGradient rg(g.cR, g.cT, g.cR + RDX, g.cB);
        rg.setColorAt(0, QColor(195, 200, 206));
        rg.setColorAt(1, QColor(170, 176, 182));
        p.setBrush(rg); p.setPen(Qt::NoPen);
        p.drawPolygon(quad({g.cR, g.cT}, PT(g.cR, g.cT),
                           PT(g.cR, g.cB), {g.cR, g.cB}));
    }
    // Cabinet top
    {
        QLinearGradient tg(g.cL, g.cT, g.cR + RDX, g.cT + RDY);
        tg.setColorAt(0,   QColor(252, 253, 255));
        tg.setColorAt(0.5, QColor(238, 241, 244));
        tg.setColorAt(1,   QColor(215, 220, 226));
        p.setBrush(tg); p.setPen(Qt::NoPen);
        p.drawPolygon(quad({g.cL, g.cT}, {g.cR, g.cT},
                           PT(g.cR, g.cT), PT(g.cL, g.cT)));
    }
    // Cabinet front
    {
        QLinearGradient fg(g.cL, g.cT, g.cR, g.cB);
        fg.setColorAt(0.0, QColor(248, 250, 252));
        fg.setColorAt(0.5, QColor(240, 242, 245));
        fg.setColorAt(1.0, QColor(225, 228, 232));
        p.setBrush(fg);
        p.setPen(QPen(QColor(185, 190, 196), 1.8));
        p.drawRoundedRect(QRectF(g.cL, g.cT, g.cR - g.cL, g.cB - g.cT), 12, 12);
    }
    // Top sensor strip
    {
        p.setBrush(QColor(210, 215, 220)); p.setPen(Qt::NoPen);
        p.drawRoundedRect(QRectF(g.cL + 8, g.cT + 4, g.cR - g.cL - 16, 4), 2, 2);
    }
    // Inner cavity back
    {
        float z = 0.88f;
        QLinearGradient bw(PT(g.iL, g.iT, z).x(), 0, PT(g.iR, g.iT, z).x(), 0);
        bw.setColorAt(0,   QColor(215, 228, 240));
        bw.setColorAt(0.5, QColor(228, 238, 248));
        bw.setColorAt(1,   QColor(205, 220, 235));
        p.setBrush(bw); p.setPen(Qt::NoPen);
        p.drawPolygon(quad(PT(g.iL, g.iT, z), PT(g.iR, g.iT, z),
                           PT(g.iR, g.iB, z), PT(g.iL, g.iB, z)));
    }
    // Inner left wall
    {
        QLinearGradient lw(g.iL, 0, g.iL + RDX * 0.88f, 0);
        lw.setColorAt(0, QColor(195, 212, 228));
        lw.setColorAt(1, QColor(218, 232, 244));
        p.setBrush(lw); p.setPen(Qt::NoPen);
        p.drawPolygon(quad({g.iL, g.iT}, PT(g.iL, g.iT, 0.88f),
                           PT(g.iL, g.iB, 0.88f), {g.iL, g.iB}));
    }
    // Inner ceiling
    {
        QLinearGradient cw(0, g.iT, 0, g.iT + 18);
        cw.setColorAt(0, QColor(245, 250, 255, 235));
        cw.setColorAt(1, QColor(225, 238, 250, 160));
        p.setBrush(cw); p.setPen(Qt::NoPen);
        p.drawPolygon(quad({g.iL, g.iT}, {g.iR, g.iT},
                           PT(g.iR, g.iT, 0.88f), PT(g.iL, g.iT, 0.88f)));
    }
    // Interior front face
    {
        QLinearGradient ig(g.iL, g.iT, g.iL, g.iB);
        ig.setColorAt(0.0, QColor(240, 246, 252));
        ig.setColorAt(0.5, QColor(232, 240, 250));
        ig.setColorAt(1.0, QColor(215, 228, 242));
        p.setBrush(ig); p.setPen(Qt::NoPen);
        p.drawRect(QRectF(g.iL, g.iT, g.iR - g.iL, g.iB - g.iT));
    }
    // Door gasket
    {
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(QColor(185, 192, 200), 3.0));
        p.drawRoundedRect(QRectF(g.iL - 2, g.iT - 2, g.iR - g.iL + 4, g.iB - g.iT + 4), 5, 5);
        p.setPen(QPen(QColor(0, 0, 0, 25), 1.2));
        p.drawRoundedRect(QRectF(g.iL + 2, g.iT + 2, g.iR - g.iL - 4, g.iB - g.iT - 4), 3, 3);
    }

    // Shelves & sample boxes
    for (int row = 0; row < N_SHELVES; ++row) {
        float sy1 = g.iT + row * g.shH;
        float sfY = sy1 + g.shH - R_SHELF_TH;
        {
            float z = 0.7f;
            QLinearGradient stg(g.iL, sfY, g.iL, sfY + 4);
            stg.setColorAt(0, QColor(250, 252, 254));
            stg.setColorAt(1, QColor(228, 238, 246));
            p.setBrush(stg); p.setPen(Qt::NoPen);
            p.drawPolygon(quad({g.iL + 2, sfY}, {g.iR - 2, sfY},
                               PT(g.iR-2, sfY, z), PT(g.iL+2, sfY, z)));
        }
        {
            QLinearGradient eg(g.iL, sfY, g.iL, sfY + R_SHELF_TH);
            eg.setColorAt(0, QColor(245, 249, 252));
            eg.setColorAt(0.5, QColor(225, 236, 244));
            eg.setColorAt(1, QColor(195, 210, 222));
            p.setBrush(eg);
            p.setPen(QPen(QColor(190, 205, 218), 0.8));
            p.drawRect(QRectF(g.iL + 2, sfY, g.iR - g.iL - 4, R_SHELF_TH));
        }
        {
            float ly = sy1 + (g.shH - 22.0f) / 2.0f;
            QRectF lr(g.iL + 3, ly, R_LABEL_W - 3, 22);
            p.setBrush(QColor(255, 255, 255, 235));
            p.setPen(QPen(QColor(10, 95, 88), 1.8));
            p.drawRoundedRect(lr, 6, 6);
            QRectF strip(lr.left(), lr.top(), 5, lr.height());
            p.setBrush(QColor(10, 95, 88));
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(strip, 3, 3);
            p.setPen(QColor(10, 70, 64));
            p.setFont(QFont("Arial", 9, QFont::Bold));
            p.drawText(QRectF(lr.left() + 8, lr.top(), lr.width() - 8, lr.height()),
                       Qt::AlignVCenter | Qt::AlignLeft, rowToLabel(row));
        }
        for (int col = 0; col < N_SLOTS; ++col) {
            QRectF sr = slotRectF(row, col);
            bool sel = (row == m_selShelf && col == m_selSlot);
            bool occ = (row < m_data.size() && col < m_data[row].size())
                       && m_data[row][col].occupied;
            QString danger = occ ? m_data[row][col].danger : QString();

            if (occ || sel) {
                QColor accent, frontBase, topBase;
                if (sel) {
                    accent    = QColor(52, 130, 218);
                    frontBase = QColor(232, 244, 255);
                    topBase   = QColor(248, 252, 255);
                } else if (danger == "BSL-3") {
                    accent    = QColor(195, 45, 45);
                    frontBase = QColor(255, 238, 238);
                    topBase   = QColor(255, 248, 248);
                } else if (danger == "BSL-2") {
                    accent    = QColor(200, 118, 30);
                    frontBase = QColor(255, 245, 228);
                    topBase   = QColor(255, 252, 242);
                } else {
                    accent    = QColor(10, 95, 88);
                    frontBase = QColor(242, 250, 248);
                    topBase   = QColor(250, 254, 253);
                }
                float z = 0.28f;
                {
                    QColor sideC = frontBase.darker(118);
                    p.setBrush(sideC); p.setPen(Qt::NoPen);
                    p.drawPolygon(quad({sr.right(), sr.top()},
                                       PT(sr.right(), sr.top(), z),
                                       PT(sr.right(), sr.bottom(), z),
                                       {sr.right(), sr.bottom()}));
                }
                {
                    QLinearGradient tg(sr.left(), sr.top(), sr.right(), sr.top() + 6);
                    tg.setColorAt(0, topBase);
                    tg.setColorAt(1, topBase.darker(108));
                    p.setBrush(tg); p.setPen(QPen(QColor(210,225,235), 0.5));
                    p.drawPolygon(quad({sr.left(),  sr.top()},
                                       {sr.right(), sr.top()},
                                       PT(sr.right(), sr.top(), z),
                                       PT(sr.left(),  sr.top(), z)));
                }
                {
                    QLinearGradient fg(sr.left(), sr.top(), sr.left(), sr.bottom());
                    fg.setColorAt(0, frontBase);
                    fg.setColorAt(1, frontBase.darker(112));
                    p.setBrush(fg);
                    p.setPen(QPen(QColor(200,215,225), 0.8));
                    p.drawRoundedRect(sr, 2, 2);
                }
                {
                    float lidY = sr.top() + sr.height() * 0.28f;
                    p.setPen(QPen(accent.lighter(170), 0.9));
                    p.drawLine(QPointF(sr.left()+2, lidY), QPointF(sr.right()-2, lidY));
                    p.setPen(QPen(QColor(255,255,255,180), 0.5));
                    p.drawLine(QPointF(sr.left()+2, lidY+1), QPointF(sr.right()-2, lidY+1));
                }
                {
                    QRectF tape(sr.left() + 2, sr.top() + 3, sr.width() - 4, 4);
                    p.setBrush(accent); p.setPen(Qt::NoPen);
                    p.drawRoundedRect(tape, 1, 1);
                }
                if (sel) {
                    p.setBrush(Qt::NoBrush);
                    p.setPen(QPen(QColor(52, 130, 218, 180), 2));
                    p.drawRoundedRect(sr.adjusted(-1,-1,1,1), 3, 3);
                }
            } else {
                p.setBrush(QColor(238, 248, 254, 60));
                p.setPen(QPen(QColor(190, 210, 225, 120), 0.7, Qt::DashLine));
                p.drawRoundedRect(sr, 2, 2);
            }
        }
    }

    // Animated door
    {
        float dL    = g.iL - 2;
        float dT    = g.iT - 2;
        float dB    = g.iB + 2;
        float dH    = dB - dT;
        float fullW = g.iR - g.iL + 4;
        float t     = m_doorOpen;

        if (t > 0.75f) {
            float alpha = qMin(1.0f, (t - 0.75f) / 0.25f);
            int   ia    = int(alpha * 255);
            float dW2   = 62.0f;
            float dL2   = g.cR + 2;
            QLinearGradient og(dL2, dT, dL2 + dW2, dT);
            og.setColorAt(0, QColor(10, 108, 100, ia));
            og.setColorAt(1, QColor(7,  80,  74,  int(ia * 0.88f)));
            p.setBrush(og);
            p.setPen(QPen(QColor(4, 52, 48, ia), 1.8));
            p.drawRoundedRect(QRectF(dL2, dT + 4, dW2 - 2, dH - 8), 6, 6);
            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(QColor(3, 40, 37, ia), 1.2));
            p.drawRoundedRect(QRectF(dL2 + 4, dT + 8, dW2 - 10, dH - 16), 4, 4);
            p.setBrush(QColor(5, 58, 53, ia)); p.setPen(Qt::NoPen);
            p.drawRoundedRect(QRectF(dL2 + 3, dT + 16, 5, dH - 32), 2, 2);
            p.setPen(QPen(QColor(255, 255, 255, int(22 * alpha)), 0.7));
            for (float fy = dT + 30; fy < dB - 18; fy += 18)
                p.drawLine(QPointF(dL2 + 10, fy), QPointF(dL2 + dW2 - 5, fy));

            if (alpha > 0.80f) {
                float btnW = 52, btnH = 22;
                float btnX = dL2 + (dW2 - btnW) / 2;
                float btnY = dT + dH / 2 - btnH / 2;
                m_doorCloseBtnRect = QRectF(btnX, btnY, btnW, btnH);
                QLinearGradient bg(btnX, btnY, btnX, btnY + btnH);
                bg.setColorAt(0, QColor(225, 62, 52, ia));
                bg.setColorAt(1, QColor(178, 32, 26, ia));
                p.setBrush(bg);
                p.setPen(QPen(QColor(140, 18, 14, ia), 1.2));
                p.drawRoundedRect(m_doorCloseBtnRect, 6, 6);
                p.setPen(QColor(255, 255, 255, ia));
                p.setFont(QFont("Arial", 8, QFont::Bold));
                p.drawText(m_doorCloseBtnRect, Qt::AlignCenter, "Fermer");
            } else {
                m_doorCloseBtnRect = QRectF();
            }
        } else {
            m_doorCloseBtnRect = QRectF();
        }

        float frontW = fullW * (1.0f - t);
        if (frontW > 1.0f) {
            QLinearGradient dg(dL, dT, dL + frontW, dT);
            dg.setColorAt(0.0f, QColor(10, 110, 102));
            dg.setColorAt(0.45f, QColor(18, 132, 122));
            dg.setColorAt(1.0f, QColor(7, 84, 76));
            p.setBrush(dg);
            p.setPen(QPen(QColor(4, 52, 48), 2.2));
            p.drawRoundedRect(QRectF(dL, dT, frontW, dH), 8, 8);

            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(QColor(3, 42, 39, 200), 1.8));
            p.drawRoundedRect(QRectF(dL + 5, dT + 5, frontW - 10, dH - 10), 6, 6);

            float curY = dT + 10;

            // LCD: horloge + température live Arduino
            if (frontW > 62) {
                float cw = qMin(frontW - 18, 135.0f), ch = 32.0f;
                float cx = dL + (frontW - cw) / 2.0f;
                p.setBrush(QColor(5, 4, 3));
                p.setPen(QPen(QColor(55, 38, 16), 1.2));
                p.drawRoundedRect(QRectF(cx, curY, cw, ch), 5, 5);
                QRadialGradient led(cx + 7, curY + ch / 2, 4.0f);
                led.setColorAt(0, m_colonVisible ? QColor(120, 255, 150) : QColor(40, 130, 60));
                led.setColorAt(1, m_colonVisible ? QColor(0,  200,  80) : QColor(10, 70, 30));
                p.setBrush(led); p.setPen(Qt::NoPen);
                p.drawEllipse(QRectF(cx + 4, curY + ch / 2 - 4, 7, 7));
                QString timeStr = QTime::currentTime().toString(
                    m_colonVisible ? "hh:mm" : "hh mm");
                p.setPen(QColor(255, 115, 22));
                p.setFont(QFont("Courier New", 12, QFont::Bold));
                p.drawText(QRectF(cx + 14, curY + 1, cw * 0.58f, ch - 2),
                           Qt::AlignVCenter | Qt::AlignHCenter, timeStr);

                // Température live depuis Arduino (DHT11) — couleur selon valeur
                QString tempStr = m_hasTemp
                    ? QString::number(m_currentTemp, 'f', 1) + "°C"
                    : "--°C";
                QColor tempColor(0, 232, 168);
                if (m_hasTemp) {
                    if (m_currentTemp >= 35.0)      tempColor = QColor(255, 90, 90);
                    else if (m_currentTemp >= 25.0) tempColor = QColor(255, 180, 60);
                }
                p.setPen(tempColor);
                p.setFont(QFont("Courier New", 10, QFont::Bold));
                p.drawText(QRectF(cx + cw * 0.58f, curY, cw * 0.42f, ch),
                           Qt::AlignCenter, tempStr);
                curY += ch + 7;
            }

            // Logo & name
            if (!m_logoPixmap.isNull() && frontW > 72) {
                float avail  = dB - curY - 40;
                float lh     = qMax(qMin(avail * 0.60f, 56.0f), 18.0f);
                float aspect = float(m_logoPixmap.width()) / float(m_logoPixmap.height());
                float lw     = qMin(lh * aspect, frontW - 24.0f);
                lh = lw / aspect;
                float lx = dL + (frontW - lw) / 2.0f;
                float ly = curY + (avail - lh) / 2.0f;
                if (ly < curY) ly = curY;
                p.drawPixmap(QRectF(lx, ly, lw, lh), m_logoPixmap, QRectF(m_logoPixmap.rect()));
                if (frontW > 70 && !m_freezerName.isEmpty()) {
                    p.setPen(QColor(255, 255, 255, 210));
                    p.setFont(QFont("Arial", 9, QFont::Bold));
                    p.drawText(QRectF(dL + 8, ly + lh + 2, frontW - 16, 18),
                               Qt::AlignCenter, m_freezerName);
                }
            } else if (frontW > 50 && !m_freezerName.isEmpty()) {
                p.setPen(Qt::white);
                p.setFont(QFont("Arial", 10, QFont::Bold));
                p.drawText(QRectF(dL + 8, curY, frontW - 16, dB - curY - 38),
                           Qt::AlignCenter, m_freezerName);
            }

            // Ouvrir button
            if (frontW > 65 && t < 0.28f) {
                float btnW = qMin(frontW * 0.62f, 85.0f), btnH = 24.0f;
                float btnX = dL + (frontW - btnW) / 2.0f;
                float btnY = dB - btnH - 10;
                m_doorOpenBtnRect = QRectF(btnX, btnY, btnW, btnH);
                QLinearGradient bg(btnX, btnY, btnX, btnY + btnH);
                bg.setColorAt(0, QColor(44, 188, 108));
                bg.setColorAt(1, QColor(30, 148, 80));
                p.setBrush(bg);
                p.setPen(QPen(QColor(18, 110, 56), 1.5));
                p.drawRoundedRect(m_doorOpenBtnRect, 7, 7);
                p.setPen(Qt::white);
                p.setFont(QFont("Arial", 9, QFont::Bold));
                p.drawText(m_doorOpenBtnRect, Qt::AlignCenter, "Ouvrir");
            } else {
                m_doorOpenBtnRect = QRectF();
            }

            // Handle
            if (frontW > 24) {
                float hx  = dL + frontW - 15;
                float hy1 = dT + dH * 0.34f;
                float hy2 = dT + dH * 0.66f;
                QLinearGradient hg(hx, hy1, hx + 10, hy1);
                hg.setColorAt(0.0f, QColor(165, 175, 185));
                hg.setColorAt(0.5f, QColor(218, 228, 236));
                hg.setColorAt(1.0f, QColor(145, 155, 165));
                p.setBrush(hg);
                p.setPen(QPen(QColor(120, 132, 142), 1.0));
                p.drawRoundedRect(QRectF(hx, hy1, 10, hy2 - hy1), 5, 5);
                p.setBrush(Qt::NoBrush);
                p.setPen(QPen(QColor(0, 0, 0, 35), 1.2));
                p.drawRoundedRect(QRectF(hx + 1, hy1 + 1, 10, hy2 - hy1), 5, 5);
            }
            p.setPen(QPen(QColor(255, 255, 255, 14), 0.7));
            for (float rx = dL + 20; rx < dL + frontW - 17; rx += 24)
                p.drawLine(QPointF(rx, dT + 14), QPointF(rx, dB - 14));
            if (t > 0.02f) {
                float sideW = qMin(fullW * t * 0.22f, 18.0f);
                float edgeX = dL + frontW;
                QLinearGradient sg(edgeX, dT, edgeX + sideW, dT);
                sg.setColorAt(0.0f, QColor(4, 52, 48, 235));
                sg.setColorAt(1.0f, QColor(3, 38, 35, 0));
                p.setBrush(sg); p.setPen(Qt::NoPen);
                p.drawRect(QRectF(edgeX, dT + 2, sideW, dH - 4));
            }
        }
    }

    // Pin
    if (m_selShelf >= 0 && m_selSlot >= 0 && m_doorOpen > 0.5f)
        drawPin(p, slotRectF(m_selShelf, m_selSlot));

    // Control strip
    {
        float stripT = g.cB + 8;
        float stripB = H - 8;
        float stripH = stripB - stripT;
        float stripL = g.cL;
        float stripR = g.cR;
        float stripW = stripR - stripL;

        QLinearGradient bg(stripL, stripT, stripL, stripB);
        bg.setColorAt(0.0f, QColor(82,  88,  98));
        bg.setColorAt(0.3f, QColor(68,  74,  84));
        bg.setColorAt(1.0f, QColor(52,  56,  64));
        p.setBrush(bg);
        p.setPen(QPen(QColor(38, 42, 50), 1.5));
        p.drawRoundedRect(QRectF(stripL, stripT, stripW, stripH), 7, 7);

        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(QColor(130, 138, 150, 90), 1.0));
        p.drawLine(QPointF(stripL + 10, stripT + 3), QPointF(stripR - 10, stripT + 3));

        float midY = stripT + stripH / 2.0f;

        float chipX = stripL + 10, chipY = stripT + (stripH - 18) / 2.0f;
        QLinearGradient chipG(chipX, chipY, chipX + 26, chipY + 18);
        chipG.setColorAt(0, QColor(45, 50, 58));
        chipG.setColorAt(1, QColor(30, 34, 40));
        p.setBrush(chipG);
        p.setPen(QPen(QColor(22, 26, 32), 1.0));
        p.drawRoundedRect(QRectF(chipX, chipY, 26, 18), 3, 3);
        p.setPen(QPen(QColor(145, 155, 168), 0.8));
        for (int pi = 0; pi < 4; ++pi) {
            float px = chipX + 4 + pi * 6;
            p.drawLine(QPointF(px, chipY),      QPointF(px, chipY - 3));
            p.drawLine(QPointF(px, chipY + 18), QPointF(px, chipY + 21));
        }
        p.setPen(QColor(0, 200, 120, 180));
        p.setFont(QFont("Courier New", 5, QFont::Bold));
        p.drawText(QRectF(chipX + 1, chipY + 1, 24, 16), Qt::AlignCenter, "MCU\n01");

        float ledX = chipX + 32, ledY = midY - 4;
        QRadialGradient ledG(ledX + 4, ledY + 4, 5);
        ledG.setColorAt(0, m_colonVisible ? QColor(80, 255, 140) : QColor(20, 160, 70));
        ledG.setColorAt(1, m_colonVisible ? QColor(0,  200,  90) : QColor(8,  90,  36));
        p.setBrush(ledG); p.setPen(Qt::NoPen);
        p.drawEllipse(QRectF(ledX, ledY, 8, 8));
        p.setBrush(QColor(255, 255, 255, 110));
        p.drawEllipse(QRectF(ledX + 1.5f, ledY + 1.0f, 3, 3));

        p.setPen(QColor(185, 195, 210));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(QRectF(stripL + stripW * 0.38f, stripT, stripW * 0.25f, stripH),
                   Qt::AlignCenter, "CONTROL");

        float gx = stripR - 56, gy = stripT + (stripH - 20) / 2.0f;
        for (int di = 0; di < 3; ++di)
            for (int dj = 0; dj < 6; ++dj) {
                float dotX = gx + dj * 8.0f, dotY = gy + di * 8.0f;
                p.setBrush(QColor(35, 39, 46)); p.setPen(Qt::NoPen);
                p.drawEllipse(QRectF(dotX + 1, dotY + 1, 4, 4));
                p.setBrush(QColor(105, 115, 128));
                p.drawEllipse(QRectF(dotX, dotY, 4, 4));
            }
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(QColor(255, 255, 255, 18), 0.8));
        p.drawLine(QPointF(stripL + 10, stripB - 3), QPointF(stripR - 10, stripB - 3));
    }
}

void FreezerWidget::mousePressEvent(QMouseEvent* e) {
    QPointF pos = e->position();

    // Boutons Ouvrir/Fermer → signaux vers CongelateurDialog (Arduino + DB + animation)
    if (m_doorOpenBtnRect.isValid() && m_doorOpenBtnRect.contains(pos)) {
        emit openRequested(); return;
    }
    if (m_doorCloseBtnRect.isValid() && m_doorCloseBtnRect.contains(pos)) {
        emit closeRequested(); return;
    }

    if (m_doorOpen < 0.85f) return;
    for (int r = 0; r < N_SHELVES; ++r)
        for (int c = 0; c < N_SLOTS; ++c)
            if (slotRectF(r,c).contains(pos))
                if (r < m_data.size() && c < m_data[r].size() && m_data[r][c].occupied)
                { emit slotClicked(r, c); return; }
}

// ═══════════════════════════════════════════════════════════════
// CongelateurDialog
// ═══════════════════════════════════════════════════════════════
static QWidget* card(QWidget* parent = nullptr) {
    auto* f = new QFrame(parent);
    f->setStyleSheet("QFrame{ background:rgba(255,255,255,0.62);"
                     "border:1px solid rgba(10,95,88,0.20); border-radius:14px; }");
    return f;
}
static QLabel* sectionTitle(const QString& t) {
    auto* l = new QLabel(t);
    l->setStyleSheet("font-weight:900; font-size:12px; color:rgba(10,95,88,0.90); padding:2px 0;");
    return l;
}
static void detailRow(QVBoxLayout* vl, const QString& lbl, QLabel*& out) {
    auto* row = new QWidget; auto* hl = new QHBoxLayout(row);
    hl->setContentsMargins(0,1,0,1); hl->setSpacing(6);
    auto* key = new QLabel(lbl + " :");
    key->setStyleSheet("color:rgba(10,95,88,0.70); font-size:11px; font-weight:700; min-width:72px;");
    out = new QLabel("—");
    out->setStyleSheet("color:rgba(0,0,0,0.75); font-size:11px; font-weight:900;");
    out->setWordWrap(true);
    hl->addWidget(key); hl->addWidget(out, 1);
    vl->addWidget(row);
}

CongelateurDialog::CongelateurDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle("❄  AI Congélateur — Localisation des Échantillons");
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint | Qt::WindowMaximizeButtonHint);
    resize(1200, 720);

    m_bgPixmap.load(":/image/cong.png");
    setAttribute(Qt::WA_OpaquePaintEvent, false);

    setStyleSheet(
        "QDialog{ background: transparent; }"
        "QScrollBar:vertical{ background:transparent; width:6px; }"
        "QScrollBar::handle:vertical{ background:rgba(10,95,88,0.40); border-radius:3px; }"
        "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{ height:0; }");

    m_net  = new QNetworkAccessManager(this);
    m_crud = new CrudeBioSimple;
    connect(m_net, &QNetworkAccessManager::finished, this, &CongelateurDialog::onAiReply);
    setupTables();

    auto* rootL = new QVBoxLayout(this);
    rootL->setContentsMargins(14,14,14,14); rootL->setSpacing(10);

    // Header
    auto* hdrCard = card(); auto* hdrL = new QHBoxLayout(hdrCard);
    hdrL->setContentsMargins(16,10,16,10);
    auto* hdrTitle = new QLabel("❄  AI Congélateur");
    hdrTitle->setStyleSheet("font-size:18px; font-weight:900; color:rgba(10,95,88,0.95);"
                            "border:none; background:transparent;");
    hdrL->addWidget(hdrTitle); hdrL->addStretch(1);
    rootL->addWidget(hdrCard);

    m_aiInput = new QLineEdit;
    m_aiInput->setPlaceholderText("🔍  Posez une question sur un échantillon…");
    m_aiInput->setStyleSheet("QLineEdit{ background:rgba(255,255,255,0.90);"
                             "color:rgba(0,0,0,0.92);"
                             "placeholder-text-color:rgba(0,0,0,0.48);"
                             "selection-background-color:rgba(10,95,88,0.25);"
                             "border:1.5px solid rgba(10,95,88,0.30); border-radius:8px;"
                             "padding:7px 12px; font-size:11px; }");
    m_aiBtn = new QPushButton("Envoyer");
    m_aiBtn->setCursor(Qt::PointingHandCursor);
    m_aiBtn->setStyleSheet(
        "QPushButton{ background:qlineargradient(x1:0,y1:0,x2:1,y2:1,"
        "    stop:0 rgba(10,95,88,0.92),stop:1 rgba(18,68,59,0.92));"
        "    color:white; font-weight:900; border:none; border-radius:8px; padding:7px 14px; }"
        "QPushButton:hover{ background:rgba(14,115,106,0.95); }"
        "QPushButton:disabled{ background:rgba(10,95,88,0.35); }");

    auto* bodyL = new QHBoxLayout; bodyL->setSpacing(12);

    // Left panel
    auto* leftCard = card(); leftCard->setFixedWidth(240);
    auto* leftVL = new QVBoxLayout(leftCard);
    leftVL->setContentsMargins(10,12,10,12); leftVL->setSpacing(8);
    leftVL->addWidget(sectionTitle("Congélateurs"));
    m_freezerList = new QListWidget; m_freezerList->setMaximumHeight(160);
    m_freezerList->setStyleSheet(
        "QListWidget{ border:none; background:transparent; }"
        "QListWidget::item{ padding:8px 10px; border-radius:8px; font-weight:700;"
        "    color:rgba(0,0,0,0.65); font-size:12px; }"
        "QListWidget::item:selected{ background:rgba(10,95,88,0.18); color:rgba(10,95,88,0.95); }"
        "QListWidget::item:hover{ background:rgba(10,95,88,0.08); }");
    leftVL->addWidget(m_freezerList);
    m_searchEdit   = nullptr;
    m_searchFilter = nullptr;
    auto* sep2 = new QFrame; sep2->setFrameShape(QFrame::HLine);
    sep2->setStyleSheet("border-top:1px solid rgba(10,95,88,0.12);");
    leftVL->addWidget(sep2);
    leftVL->addWidget(sectionTitle("Échantillons"));
    m_sampleList = new QListWidget;
    m_sampleList->setStyleSheet(
        "QListWidget{ border:none; background:transparent; }"
        "QListWidget::item{ padding:6px 8px; border-radius:7px; font-size:10px; color:rgba(0,0,0,0.65); }"
        "QListWidget::item:selected{ background:rgba(10,95,88,0.18); color:rgba(10,95,88,0.90); font-weight:700; }"
        "QListWidget::item:hover{ background:rgba(10,95,88,0.08); }");
    leftVL->addWidget(m_sampleList, 1);
    bodyL->addWidget(leftCard);

    // Center
    auto* centerCard = card(); auto* centerVL = new QVBoxLayout(centerCard);
    centerVL->setContentsMargins(14,14,14,10); centerVL->setSpacing(8);
    auto* centerTitle = sectionTitle("Localisation de l'Échantillon");
    centerTitle->setStyleSheet("font-weight:900; font-size:13px; color:rgba(10,95,88,0.90);"
                               "border:none; background:transparent;");
    centerVL->addWidget(centerTitle);

    auto* freezerRow = new QHBoxLayout;
    freezerRow->setContentsMargins(0,0,0,0);
    m_freezerWidget = new FreezerWidget;
    freezerRow->addStretch(1);
    freezerRow->addWidget(m_freezerWidget);
    freezerRow->addStretch(1);
    centerVL->addLayout(freezerRow, 1);

    m_statusBar = new QLabel("Sélectionnez un congélateur, puis cliquez sur un échantillon.");
    m_statusBar->setAlignment(Qt::AlignCenter); m_statusBar->setFixedHeight(34);
    m_statusBar->setStyleSheet(
        "background:qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "    stop:0 rgba(10,95,88,0.85),stop:1 rgba(18,68,59,0.85));"
        "color:white; font-weight:900; font-size:12px; border-radius:8px; padding:0 14px;");
    centerVL->addWidget(m_statusBar);
    bodyL->addWidget(centerCard, 1);

    // Right
    auto* rightCard = card(); rightCard->setFixedWidth(260);
    auto* rightVL = new QVBoxLayout(rightCard);
    rightVL->setContentsMargins(14,12,14,12); rightVL->setSpacing(6);
    m_pinIcon = new QLabel("📍"); m_pinIcon->setAlignment(Qt::AlignCenter);
    m_pinIcon->setStyleSheet("font-size:28px; border:none; background:transparent;");
    m_pinIcon->hide(); rightVL->addWidget(m_pinIcon);
    rightVL->addWidget(sectionTitle("Détails de l'Échantillon"));
    detailRow(rightVL, "ID",         m_detId);
    detailRow(rightVL, "Type",       m_detType);
    detailRow(rightVL, "Organisme",  m_detOrg);
    detailRow(rightVL, "Étage",      m_detEtage);
    detailRow(rightVL, "Slot",       m_detSlot);
    detailRow(rightVL, "Temp.",      m_detTemp);
    detailRow(rightVL, "Danger",     m_detDanger);
    auto* sep3 = new QFrame; sep3->setFrameShape(QFrame::HLine);
    sep3->setStyleSheet("border-top:1px solid rgba(10,95,88,0.15); margin:4px 0; background:transparent;");
    rightVL->addWidget(sep3);
    rightVL->addWidget(sectionTitle("Informations supplémentaires"));
    detailRow(rightVL, "Stockage",   m_detDateCol);
    detailRow(rightVL, "Expiration", m_detDateExp);
    auto* sep4 = new QFrame; sep4->setFrameShape(QFrame::HLine);
    sep4->setStyleSheet("border-top:1px solid rgba(10,95,88,0.15); margin:4px 0; background:transparent;");
    rightVL->addWidget(sep4);
    rightVL->addWidget(sectionTitle("Assistant IA"));
    rightVL->addWidget(m_aiInput);
    auto* aiRow = new QHBoxLayout; aiRow->setSpacing(6);
    aiRow->addWidget(m_aiBtn, 1);
    rightVL->addLayout(aiRow);
    m_aiResp = new QLabel("Posez une question — la réponse s'affiche dans\nune fenêtre flottante.");
    m_aiResp->setWordWrap(true); m_aiResp->setAlignment(Qt::AlignCenter);
    m_aiResp->setStyleSheet("color:rgba(10,95,88,0.65); font-size:10px; padding:6px;"
                            "background:rgba(10,95,88,0.06); border-radius:8px; font-style:italic;");
    rightVL->addWidget(m_aiResp);
    rightVL->addStretch(1);
    bodyL->addWidget(rightCard);
    rootL->addLayout(bodyL, 1);

    // ── Wiring ────────────────────────────────────────────────
    connect(m_freezerList,  &QListWidget::itemClicked,  this, &CongelateurDialog::onFreezerClicked);
    connect(m_freezerWidget,&FreezerWidget::slotClicked,this, &CongelateurDialog::onSlotClicked);
    connect(m_sampleList,   &QListWidget::itemClicked,  this, &CongelateurDialog::onSampleListClicked);
    connect(m_aiBtn,        &QPushButton::clicked,      this, &CongelateurDialog::onAiSearch);
    connect(m_aiInput,      &QLineEdit::returnPressed,  this, &CongelateurDialog::onAiSearch);

    // Boutons Ouvrir/Fermer sur la porte → routés via DB + Arduino
    connect(m_freezerWidget, &FreezerWidget::openRequested,
            this, &CongelateurDialog::onManualOpenClicked);
    connect(m_freezerWidget, &FreezerWidget::closeRequested,
            this, &CongelateurDialog::onManualCloseClicked);

    loadFreezers();
    QTimer::singleShot(300, this, [this]{ setupArduino(); });
}

// ─────────────────────────────────────────────────────────────────
//  DB — création des tables si elles n'existent pas encore
// ─────────────────────────────────────────────────────────────────
void CongelateurDialog::setupTables()
{
    // Oracle: ignorer ORA-00955 "name is already used by an existing object"
    auto tryCreate = [](const QString& sql) {
        QSqlQuery q;
        if (!q.exec(sql)) {
            const QString err = q.lastError().databaseText();
            if (!err.contains("00955") && !err.contains("already used"))
                qWarning() << "[DB] setupTables:" << err;
        }
    };

    tryCreate(
        "CREATE TABLE \"CONG_ACCESS_LOG\" ("
        "  \"ID\"          NUMBER        NOT NULL PRIMARY KEY,"
        "  \"BADGE_ID\"    NUMBER,"
        "  \"FRIGO_ID\"    NUMBER,"
        "  \"ACCESS_TIME\" TIMESTAMP     DEFAULT SYSTIMESTAMP,"
        "  \"ACTION\"      VARCHAR2(50)"
        ")"
    );

    tryCreate(
        "CREATE TABLE \"ALERTS\" ("
        "  \"ID\"          NUMBER        NOT NULL PRIMARY KEY,"
        "  \"VALEUR\"      NUMBER(10,2),"
        "  \"BADGE_ID\"    NUMBER,"
        "  \"ALERT_TIME\"  TIMESTAMP     DEFAULT SYSTIMESTAMP"
        ")"
    );

    tryCreate(
        "CREATE TABLE \"BADGES\" ("
        "  \"ID\"       NUMBER       NOT NULL PRIMARY KEY,"
        "  \"UID\"      VARCHAR2(50) NOT NULL,"
        "  \"FRIGO_ID\" NUMBER"
        ")"
    );

    tryCreate(
        "CREATE TABLE \"FRIGO\" ("
        "  \"ID\"             NUMBER       NOT NULL PRIMARY KEY,"
        "  \"NOM\"            VARCHAR2(80) NOT NULL,"
        "  \"STATUS\"         VARCHAR2(20) DEFAULT 'CLOSED',"
        "  \"DOOR_NUM\"       NUMBER,"
        "  \"TEMP_THRESHOLD\" NUMBER(10,2) DEFAULT 32"
        ")"
    );

    qDebug() << "[DB] setupTables: vérification des tables terminée";
}

// ─────────────────────────────────────────────────────────────────
//  ARDUINO — connexion automatique
// ─────────────────────────────────────────────────────────────────
void CongelateurDialog::setupArduino()
{
    if (m_arduino && m_arduino->isOpen())
        return;

    if (!m_arduino)
        m_arduino = new QSerialPort(this);

    QStringList candidatePorts;
    const QString envPort = qEnvironmentVariable("SMARTVISION_ARDUINO_PORT").trimmed();
    if (!envPort.isEmpty())
        candidatePorts << envPort;
    candidatePorts << "COM14";

    for (const QSerialPortInfo& info : QSerialPortInfo::availablePorts()) {
        const QString desc = info.description().toUpper();
        const QString manufacturer = info.manufacturer().toUpper();
        const bool likelyArduino =
            desc.contains("ARDUINO") || desc.contains("CH340") ||
            desc.contains("CP210")   || desc.contains("USB SERIAL") ||
            desc.contains("USB-SERIAL") || manufacturer.contains("ARDUINO") ||
            manufacturer.contains("CH340");

        if (likelyArduino)
            candidatePorts.prepend(info.portName());
        else
            candidatePorts.append(info.portName());
    }

    candidatePorts.removeDuplicates();

    auto configurePort = [this](const QString& portName) {
        m_arduino->setPortName(portName);
        m_arduino->setBaudRate(QSerialPort::Baud9600);
        m_arduino->setDataBits(QSerialPort::Data8);
        m_arduino->setParity(QSerialPort::NoParity);
        m_arduino->setStopBits(QSerialPort::OneStop);
        m_arduino->setFlowControl(QSerialPort::NoFlowControl);
    };

    for (const QString& portName : candidatePorts) {
        configurePort(portName);

        if (!m_arduino->open(QSerialPort::ReadWrite)) {
            qWarning() << "[Arduino] Échec ouverture" << portName
                       << "—" << m_arduino->errorString();
            continue;
        }

        m_arduino->setDataTerminalReady(true);
        m_arduino->setRequestToSend(false);
        m_arduino->clear();

        connect(m_arduino, &QSerialPort::readyRead,
                this, &CongelateurDialog::onSerialReadyRead,
                Qt::UniqueConnection);

        qDebug() << "[Arduino] Connecté sur" << portName << "à 9600 baud";
        if (m_statusBar)
            m_statusBar->setText("✅  Arduino connecté sur " + portName + " — attente température...");
        return;
    }

    qWarning() << "[Arduino] Aucun port Arduino utilisable. Ports testés:"
               << candidatePorts.join(", ");
    qWarning() << "[Arduino] Définissez SMARTVISION_ARDUINO_PORT=COMx pour forcer un port.";
    if (m_statusBar)
        m_statusBar->setText("❌  Arduino non détecté — vérifiez le câble USB et fermez l'IDE Arduino (Serial Monitor)");
}

// ─────────────────────────────────────────────────────────────────
//  ARDUINO — lecture série ligne par ligne
// ─────────────────────────────────────────────────────────────────
void CongelateurDialog::onSerialReadyRead()
{
    m_serialBuffer += m_arduino->readAll();
    while (true) {
        int nl = m_serialBuffer.indexOf('\n');
        if (nl < 0) break;
        const QString line = QString::fromUtf8(m_serialBuffer.left(nl)).trimmed();
        m_serialBuffer.remove(0, nl + 1);
        if (!line.isEmpty()) {
            qDebug() << "[Arduino RX]" << line;
            parseSerialLine(line);
        }
    }
}

// ─────────────────────────────────────────────────────────────────
//  ARDUINO — parsing du protocole
//  Formats reçus :
//    TEMP;DOOR=1;TEMP=22.5;HUM=45.2;THR=25.0;OK=1
//    RFID;UID=AB12CD34
//    SERVO;DOOR=1;STATUS=OPEN
//    SERVO;DOOR=1;STATUS=CLOSED
//    ACCESS;DOOR=1;STATUS=GRANTED;UID=AB12CD34
//    ACCESS;DOOR=1;STATUS=BLOCKED_TEMP;TEMP=28.5;UID=AB12CD34
//    ALERT;DOOR=1;TYPE=TEMP_HIGH;TEMP=28.5
//    SYSTEM;...   ERROR;...
// ─────────────────────────────────────────────────────────────────
void CongelateurDialog::parseSerialLine(const QString& line)
{
    const QStringList parts = line.split(';');
    if (parts.isEmpty()) return;
    const QString type = parts[0].trimmed().toUpper();

    // Construire map KEY→VALUE à partir des tokens suivants
    QMap<QString,QString> kv;
    for (int i = 1; i < parts.size(); ++i) {
        const int eq = parts[i].indexOf('=');
        if (eq > 0)
            kv[parts[i].left(eq).trimmed().toUpper()] = parts[i].mid(eq + 1).trimmed();
    }

    if (type == "TEMP") {
        const int    doorNum = kv.value("DOOR", "1").toInt();
        const double temp    = kv.value("VALUE", kv.value("TEMP", "0")).toDouble();
        const double hum     = kv.value("HUM",  "0").toDouble();
        const double thr     = kv.value("THRESHOLD", kv.value("THR", "32.0")).toDouble();
        bool ok = true;
        if (kv.contains("OK")) {
            ok = (kv.value("OK") != "0");
        } else if (kv.contains("STATUS")) {
            const QString status = kv.value("STATUS").toUpper();
            ok = (status == "OK" || status == "NORMAL");
        } else {
            ok = (temp < thr);
        }
        handleTempUpdate(doorNum, temp, hum, thr, ok);

    } else if (type == "SERVO") {
        const int     doorNum = kv.value("DOOR", "1").toInt();
        const QString status  = kv.value("STATUS");
        handleServoStatus(doorNum, status);

    } else if (type == "ACCESS_AUTHORIZED") {
        const int doorNum = kv.value("DOOR", "1").toInt();
        handleAccessAuthorized(doorNum);

    } else if (type == "ACCESS_DENIED") {
        const int     doorNum = kv.value("DOOR", "1").toInt();
        const QString reason  = kv.value("REASON", "DENIED");
        const double  temp    = kv.value("TEMP",
                                  kv.value("VALUE",
                                           QString::number(m_currentTemps.value(doorNum, 0.0)))).toDouble();
        handleAccessDenied(doorNum, reason, temp);

    } else if (type == "ACCESS") {
        int doorNum = kv.contains("DOOR") ? kv.value("DOOR").toInt() : 0;
        const QString status  = kv.value("STATUS").toUpper();
        const QString uid     = kv.value("UID");
        if (doorNum <= 0 && !uid.isEmpty())
            doorNum = badgeFrigoIdByUid(uid);
        if (!uid.isEmpty() && doorNum > 0)
            m_pendingScanUid[doorNum] = uid;
        if (!uid.isEmpty())
            m_pendingScanUid[0] = uid;
        if (uid.isEmpty() && doorNum > 0) {
            m_pendingScanUid.remove(doorNum);
            m_pendingScanUid.remove(0);
        }
        if (status == "GRANTED") {
            handleAccessAuthorized(doorNum);
        } else if (status.startsWith("BLOCKED") || status == "DENIED") {
            const double temp = kv.value("TEMP",
                                  kv.value("VALUE",
                                           QString::number(m_currentTemps.value(doorNum, 0.0)))).toDouble();
            handleAccessDenied(doorNum, status, temp);
        }

    } else if (type == "RFID") {
        // Supporte RFID;UID=xxx  ET  RFID;xxx  (UID brut sans clé)
        QString uid = kv.value("UID");
        if (uid.isEmpty() && parts.size() > 1 && !parts[1].contains('='))
            uid = parts[1].trimmed();
        if (!uid.isEmpty())
            handleRfidScan(uid);

    } else if (type == "ALERT") {
        const int     doorNum = kv.value("DOOR", "1").toInt();
        const QString atype   = kv.value("TYPE").toUpper();
        const double  temp    = kv.value("TEMP",
                                  kv.value("VALUE",
                                           QString::number(m_currentTemps.value(doorNum, 0.0)))).toDouble();
        if (atype == "TEMP_HIGH") {
            const int frigoId = frigoIdByDoorNum(doorNum);
            const QString uid = m_pendingScanUid.value(doorNum, m_pendingScanUid.value(0));
            const int badgeId = badgeIdByUid(uid);
            const QDateTime now = QDateTime::currentDateTime();
            const QDateTime last = m_lastAlertTime.value(doorNum);
            if (!last.isValid() || last.secsTo(now) >= 300) {
                m_lastAlertTime[doorNum] = now;
                insertAlert(frigoId, temp, badgeId);
            }
            updateCongFrigoStatus(frigoId, "CLOSED");
            if (doorNum == currentDisplayedDoorNum())
                m_freezerWidget->setDoorState(false, true);
            m_statusBar->setText(congTempBlockedMessage(
                doorNum, temp, congThresholdById(frigoId, 32.0)));
        }

    } else if (type == "SYSTEM") {
        qDebug() << "[Arduino SYSTEM]" << line;

    } else if (type == "ERROR") {
        qWarning() << "[Arduino ERROR]" << line;
    }
}

// ─────────────────────────────────────────────────────────────────
//  ARDUINO — handlers
// ─────────────────────────────────────────────────────────────────

// Mise à jour température + humidité depuis DHT
void CongelateurDialog::handleTempUpdate(int doorNum, double temp,
                                         double hum, double thr, bool ok)
{
    m_currentTemps[doorNum]      = temp;
    m_currentHumidities[doorNum] = hum;

    const int frigoId = frigoIdByDoorNum(doorNum);
    const int displayedDoor = currentDisplayedDoorNum();
    const double threshold = (thr > 0.0) ? thr : congThresholdById(frigoId, 32.0);
    const bool tempHigh = !ok || temp >= threshold;

    // Afficher la température si : porte correspond au frigo affiché,
    // aucun frigo sélectionné, ou aucun congélateur dans la DB (fallback).
    if (doorNum == displayedDoor || displayedDoor <= 0 || m_currentCong.isEmpty())
        m_freezerWidget->setCurrentTemperature(temp);

    if (m_statusBar && (doorNum == displayedDoor || displayedDoor <= 0 || m_currentCong.isEmpty())) {
        if (tempHigh) {
            m_freezerWidget->setDoorState(false, true);
            m_statusBar->setText(congTempBlockedMessage(doorNum, temp, threshold));
        } else {
            m_statusBar->setText(QString("🌡  C%1 : %2 °C  |  Humidité : %3%")
                                 .arg(doorNum, 2, 10, QChar('0'))
                                 .arg(temp, 0, 'f', 1)
                                 .arg(hum, 0, 'f', 0));
        }
    }

    // Alerte température avec débounce de 5 minutes par porte
    if (tempHigh) {
        cancelAutoCloseTimer(doorNum);
        m_doorOpen[doorNum] = false;
        updateCongFrigoStatus(frigoId, "CLOSED");
        if (doorNum == currentDisplayedDoorNum())
            m_freezerWidget->setDoorState(false, true);
        sendArduinoCommand(QString("CLOSE;DOOR=%1").arg(doorNum));

        const QDateTime now  = QDateTime::currentDateTime();
        const QDateTime last = m_lastAlertTime.value(doorNum);
        if (!last.isValid() || last.secsTo(now) >= 300) {
            m_lastAlertTime[doorNum] = now;
            insertAlert(frigoId, temp, 0);
        }
    }
}

// État physique de la porte (retour SERVO;DOOR=N;STATUS=OPEN/CLOSED depuis Arduino)
void CongelateurDialog::handleServoStatus(int doorNum, const QString& status)
{
    const bool open     = (status.toUpper() == "OPEN" || status.toUpper() == "OUVERT");
    const int  frigoId  = frigoIdByDoorNum(doorNum);

    updateCongFrigoStatus(frigoId, open ? "OPEN" : "CLOSED");
    m_doorOpen[doorNum] = open;   // toujours synchroniser l'état mémoire
    if (open)
        startAutoCloseTimer(doorNum);
    else
        cancelAutoCloseTimer(doorNum);

    if (doorNum == currentDisplayedDoorNum())
        m_freezerWidget->setDoorState(open, true);

    const double temp      = m_currentTemps.value(doorNum, 0.0);
    const double threshold = congThresholdById(frigoId, 32.0);
    if (!open && m_currentTemps.contains(doorNum) && temp >= threshold) {
        m_statusBar->setText(congTempBlockedMessage(doorNum, temp, threshold));
    } else {
        m_statusBar->setText(
            QString("%1 %2").arg(congDoorLabel(doorNum))
                            .arg(open ? ": OUVERTE" : ": FERMÉE"));
    }

    qDebug() << "[SERVO] doorNum=" << doorNum << (open ? "OPEN" : "CLOSED");
}

// Scan RFID reçu depuis Arduino. Qt vérifie Oracle puis commande le servo.
void CongelateurDialog::handleRfidScan(const QString& uid)
{
    const QString cleanUid = uid.trimmed().toUpper();
    if (cleanUid.isEmpty())
        return;

    qDebug() << "[RFID] UID reçu:" << cleanUid;

    const int badgeId = badgeIdByUid(cleanUid);
    const int frigoId = badgeFrigoIdByUid(cleanUid);
    const int doorNum = (frigoId > 0) ? doorNumByFrigoId(frigoId) : -1;

    qDebug() << "[RFID] badgeId=" << badgeId
             << "frigoId=" << frigoId << "doorNum=" << doorNum;

    // Badge inconnu ou frigo non configuré → refus
    if (badgeId <= 0 || frigoId <= 0 || !isArduinoCongDoor(doorNum)) {
        const int fallbackFrigo = (frigoId > 0) ? frigoId : currentDisplayedFrigoId();
        logAccess(badgeId > 0 ? badgeId : 0, fallbackFrigo, "ACCESS_DENIED");
        m_statusBar->setText(QString("⛔  Badge non autorisé : %1").arg(cleanUid));
        qWarning() << "[RFID] Badge refusé — non trouvé en base ou frigo invalide";
        return;
    }

    // Mémoriser l'UID pour les handlers asynchrones (ACCESS_AUTHORIZED, etc.)
    m_pendingScanUid[doorNum] = cleanUid;
    m_pendingScanUid[0]       = cleanUid;

    // Vérification température avant toute action
    const double threshold = congThresholdById(frigoId, 32.0);
    const bool   hasTemp   = m_currentTemps.contains(doorNum);
    const double temp      = m_currentTemps.value(doorNum, 0.0);

    if (hasTemp && temp >= threshold) {
        insertAlert(frigoId, temp, badgeId);
        logAccess(badgeId, frigoId, "DENIED_TMP");
        updateCongFrigoStatus(frigoId, "CLOSED");
        cancelAutoCloseTimer(doorNum);
        m_doorOpen[doorNum] = false;
        sendArduinoCommand(QString("CLOSE;DOOR=%1").arg(doorNum));
        if (doorNum == currentDisplayedDoorNum())
            m_freezerWidget->setDoorState(false, true);
        m_statusBar->setText(congTempBlockedMessage(doorNum, temp, threshold));
        m_pendingScanUid.remove(doorNum);
        m_pendingScanUid.remove(0);
        return;
    }

    // Toggle ouverture / fermeture basé sur l'état mémoire (fiable, pas de roundtrip DB)
    const bool isOpen = m_doorOpen.value(doorNum, false);
    qDebug() << "[RFID] doorNum=" << doorNum << "isOpen=" << isOpen;

    if (isOpen) {
        // Porte ouverte → fermer
        logAccess(badgeId, frigoId, "CLOSE");
        updateCongFrigoStatus(frigoId, "CLOSED");
        cancelAutoCloseTimer(doorNum);
        m_doorOpen[doorNum] = false;
        sendArduinoCommand(QString("CLOSE;DOOR=%1").arg(doorNum));
        if (doorNum == currentDisplayedDoorNum())
            m_freezerWidget->setDoorState(false, true);
        m_statusBar->setText(QString("🔒  Badge accepté — fermeture %1").arg(congDoorLabel(doorNum)));
        m_pendingScanUid.remove(doorNum);
        m_pendingScanUid.remove(0);
    } else {
        // Porte fermée → demander ouverture (Arduino confirmera avec ACCESS_AUTHORIZED)
        sendArduinoCommand(QString("THRESHOLD;DOOR=%1;VALUE=%2")
                               .arg(doorNum).arg(threshold, 0, 'f', 1));
        sendArduinoCommand(QString("OPEN;DOOR=%1").arg(doorNum));
        m_statusBar->setText(QString("🔓  Badge accepté — ouverture %1 en cours...")
                                 .arg(congDoorLabel(doorNum)));
    }
}

// Accès RFID autorisé — Arduino a physiquement ouvert le servo
void CongelateurDialog::handleAccessAuthorized(int doorNum)
{
    const QString uid     = m_pendingScanUid.value(doorNum, m_pendingScanUid.value(0));
    const int     badgeId = badgeIdByUid(uid);
    const int     frigoId = frigoIdByDoorNum(doorNum);

    logAccess(badgeId, frigoId, "OPEN");
    updateCongFrigoStatus(frigoId, "OPEN");
    m_doorOpen[doorNum] = true;   // état mémoire : porte ouverte
    startAutoCloseTimer(doorNum);

    if (doorNum == currentDisplayedDoorNum())
        m_freezerWidget->setDoorState(true, true);

    m_statusBar->setText(
        QString("✅  Accès autorisé — %1 ouverte").arg(congDoorLabel(doorNum)));
    m_pendingScanUid.remove(doorNum);
    m_pendingScanUid.remove(0);

    qDebug() << "[RFID] handleAccessAuthorized doorNum=" << doorNum
             << "badge=" << badgeId << "frigo=" << frigoId;
}

// Accès RFID refusé
void CongelateurDialog::handleAccessDenied(int doorNum,
                                            const QString& reason,
    double temp)
{
    QString msg;
    const int frigoId = frigoIdByDoorNum(doorNum);
    const QString doorLabel = congDoorLabel(doorNum);
    const QString uid     = m_pendingScanUid.value(doorNum, m_pendingScanUid.value(0));
    const int     badgeId = badgeIdByUid(uid);

    if (reason.contains("TEMP") || reason.contains("BLOCKED")) {
        const double threshold = congThresholdById(frigoId, 32.0);
        msg = congTempBlockedMessage(doorNum, temp, threshold);
        insertAlert(frigoId, temp, badgeId);
        logAccess(badgeId, frigoId, "DENIED_TMP");
        updateCongFrigoStatus(frigoId, "CLOSED");
        cancelAutoCloseTimer(doorNum);
        m_doorOpen[doorNum] = false;
        if (doorNum == currentDisplayedDoorNum())
            m_freezerWidget->setDoorState(false, true);
    } else {
        msg = QString("⛔  Accès refusé — Badge non autorisé — %1").arg(doorLabel);
        logAccess(badgeId, frigoId, "ACCESS_DENIED");
        cancelAutoCloseTimer(doorNum);
        m_doorOpen[doorNum] = false;
    }
    m_pendingScanUid.remove(doorNum);
    m_pendingScanUid.remove(0);
    m_statusBar->setText(msg);
}

// ─────────────────────────────────────────────────────────────────
//  DB — insertion alerte température
// ─────────────────────────────────────────────────────────────────
void CongelateurDialog::insertAlert(int frigoId, double value, int badgeId)
{
    QSqlQuery q;
    Q_UNUSED(frigoId);
    if (badgeId > 0) {
        q.prepare(
            "INSERT INTO \"ALERTS\" (\"ID\", \"VALEUR\", \"BADGE_ID\", \"ALERT_TIME\") "
            "VALUES ((SELECT NVL(MAX(\"ID\"), 0) + 1 FROM \"ALERTS\"), :val, :bid, SYSTIMESTAMP)");
        q.bindValue(":bid", badgeId);
    } else {
        q.prepare(
            "INSERT INTO \"ALERTS\" (\"ID\", \"VALEUR\", \"BADGE_ID\", \"ALERT_TIME\") "
            "VALUES ((SELECT NVL(MAX(\"ID\"), 0) + 1 FROM \"ALERTS\"), :val, NULL, SYSTIMESTAMP)");
    }
    q.bindValue(":val", value);
    if (!q.exec()) {
        qWarning() << "[DB] insertAlert:" << q.lastError().text();
        if (m_statusBar)
            m_statusBar->setText("⚠️  Alerte température non enregistrée dans la base.");
    } else {
        qDebug() << "[DB] ALERTS insert OK" << "badge" << badgeId << "value" << value;
    }
}

// ─────────────────────────────────────────────────────────────────
//  DB — journal des accès
// ─────────────────────────────────────────────────────────────────
void CongelateurDialog::logAccess(int badgeId, int frigoId, const QString& action)
{
    int resolvedFrigoId = frigoId;
    if (resolvedFrigoId <= 0 && badgeId > 0) {
        QSqlQuery f;
        f.prepare("SELECT \"FRIGO_ID\" FROM \"BADGES\" WHERE \"ID\" = :id");
        f.bindValue(":id", badgeId);
        if (f.exec() && f.next() && !f.value(0).isNull())
            resolvedFrigoId = f.value(0).toInt();
    }
    if (resolvedFrigoId <= 0)
        resolvedFrigoId = currentDisplayedFrigoId();

    QSqlQuery q;
    const QString badgeExpr = (badgeId > 0) ? ":bid" : "NULL";
    const QString frigoExpr = (resolvedFrigoId > 0) ? ":fid" : "NULL";
    q.prepare(QString(
        "INSERT INTO \"CONG_ACCESS_LOG\" (\"ID\", \"BADGE_ID\", \"FRIGO_ID\", \"ACCESS_TIME\", \"ACTION\") "
        "VALUES ((SELECT NVL(MAX(\"ID\"), 0) + 1 FROM \"CONG_ACCESS_LOG\"), %1, %2, SYSTIMESTAMP, :act)")
        .arg(badgeExpr, frigoExpr));
    if (badgeId > 0)
        q.bindValue(":bid", badgeId);
    if (resolvedFrigoId > 0)
        q.bindValue(":fid", resolvedFrigoId);
    q.bindValue(":act", action);
    if (!q.exec()) {
        qWarning() << "[DB] logAccess:" << q.lastError().text();
        if (m_statusBar)
            m_statusBar->setText("⚠️  Journal d'accès non enregistré dans la base.");
    } else {
        qDebug() << "[DB] CONG_ACCESS_LOG insert OK"
                 << "badge" << badgeId
                 << "frigo" << resolvedFrigoId
                 << "action" << action;
    }
}

void CongelateurDialog::logAccess(int badgeId, const QString& action)
{
    logAccess(badgeId, currentDisplayedFrigoId(), action);
}

// ─────────────────────────────────────────────────────────────────
//  ARDUINO — envoi de commande
//  Format : "OPEN;DOOR=1\n"  ou  "CLOSE;DOOR=1\n"
// ─────────────────────────────────────────────────────────────────
void CongelateurDialog::sendArduinoCommand(const QString& cmd)
{
    if (!m_arduino || !m_arduino->isOpen()) {
        qWarning() << "[Arduino TX] Port série fermé, commande ignorée:" << cmd;
        if (m_statusBar)
            m_statusBar->setText("❌  Arduino non connecté — impossible d'envoyer la commande.");
        return;
    }

    const QByteArray payload = (cmd + "\n").toUtf8();
    const qint64 written = m_arduino->write(payload);
    if (written != payload.size() || !m_arduino->waitForBytesWritten(500)) {
        qWarning() << "[Arduino TX] Échec envoi:" << cmd << m_arduino->errorString();
        if (m_statusBar)
            m_statusBar->setText("❌  Commande Arduino non envoyée.");
        return;
    }
    qDebug().noquote() << "[Arduino TX]" << cmd;
}

void CongelateurDialog::startAutoCloseTimer(int doorNum)
{
    if (!isArduinoCongDoor(doorNum))
        return;

    cancelAutoCloseTimer(doorNum);

    auto* timer = new QTimer(this);
    timer->setSingleShot(true);
    timer->setInterval(10000);
    connect(timer, &QTimer::timeout, this, [this, doorNum, timer]() {
        m_autoCloseTimers.remove(doorNum);
        timer->deleteLater();

        if (!m_doorOpen.value(doorNum, false))
            return;

        const int frigoId = frigoIdByDoorNum(doorNum);
        m_doorOpen[doorNum] = false;
        updateCongFrigoStatus(frigoId, "CLOSED");
        sendArduinoCommand(QString("CLOSE;DOOR=%1").arg(doorNum));
        logAccess(0, frigoId, "AUTO_CLOSE");

        if (doorNum == currentDisplayedDoorNum())
            m_freezerWidget->setDoorState(false, true);

        if (m_statusBar)
            m_statusBar->setText(QString("⏱  Fermeture automatique après 10 secondes — %1")
                                 .arg(congDoorLabel(doorNum)));
    });

    m_autoCloseTimers[doorNum] = timer;
    timer->start();
}

void CongelateurDialog::cancelAutoCloseTimer(int doorNum)
{
    QTimer* timer = m_autoCloseTimers.take(doorNum);
    if (!timer)
        return;

    timer->stop();
    timer->deleteLater();
}

// ─────────────────────────────────────────────────────────────────
//  Boutons manuels Ouvrir / Fermer
// ─────────────────────────────────────────────────────────────────
void CongelateurDialog::onManualOpenClicked()
{
    const int fid = currentDisplayedFrigoId();
    const int doorNum = currentDisplayedDoorNum();
    if (fid <= 0 || !isArduinoCongDoor(doorNum)) {
        m_statusBar->setText("⚠️  Aucun congélateur sélectionné.");
        return;
    }

    const double threshold = congThresholdById(fid, 32.0);
    sendArduinoCommand(QString("THRESHOLD;DOOR=%1;VALUE=%2")
                           .arg(doorNum)
                           .arg(threshold, 0, 'f', 1));
    if (m_currentTemps.contains(doorNum) && m_currentTemps.value(doorNum) >= threshold) {
        handleAccessDenied(doorNum, "TEMP_HIGH", m_currentTemps.value(doorNum));
        sendArduinoCommand(QString("CLOSE;DOOR=%1").arg(doorNum));
        return;
    }

    // Mise à jour visuelle immédiate; SERVO Arduino confirme l'état physique ensuite.
    m_pendingScanUid.remove(doorNum);
    m_pendingScanUid.remove(0);
    m_freezerWidget->setDoorState(true, true);
    updateCongFrigoStatus(fid, "OPEN");
    m_doorOpen[doorNum] = true;
    startAutoCloseTimer(doorNum);
    sendArduinoCommand(QString("OPEN;DOOR=%1").arg(doorNum));
    logAccess(0, fid, "MANUAL_OPEN");
    m_statusBar->setText(QString("🔓  Ouverture manuelle — %1").arg(congDoorLabel(doorNum)));
}

void CongelateurDialog::onManualCloseClicked()
{
    const int fid = currentDisplayedFrigoId();
    const int doorNum = currentDisplayedDoorNum();
    if (fid <= 0 || !isArduinoCongDoor(doorNum)) {
        m_statusBar->setText("⚠️  Aucun congélateur sélectionné.");
        return;
    }

    m_freezerWidget->setDoorState(false, true);
    m_pendingScanUid.remove(doorNum);
    m_pendingScanUid.remove(0);
    updateCongFrigoStatus(fid, "CLOSED");
    cancelAutoCloseTimer(doorNum);
    m_doorOpen[doorNum] = false;
    sendArduinoCommand(QString("CLOSE;DOOR=%1").arg(doorNum));
    logAccess(0, fid, "MANUAL_CLOSE");
    m_statusBar->setText(QString("🔒  Fermeture manuelle — %1").arg(congDoorLabel(doorNum)));
}

// ─────────────────────────────────────────────────────────────────
//  Helpers DB
// ─────────────────────────────────────────────────────────────────
int CongelateurDialog::currentDisplayedFrigoId() const
{
    return frigoIdByName(m_currentCong);
}

int CongelateurDialog::currentDisplayedDoorNum() const
{
    const int frigoId = currentDisplayedFrigoId();
    const int doorFromDb = doorNumByFrigoId(frigoId);
    if (isArduinoCongDoor(doorFromDb))
        return doorFromDb;

    const int doorFromName = congNumberFromName(m_currentCong);
    if (isArduinoCongDoor(doorFromName))
        return doorFromName;

    return -1;
}

// Cherche l'ID du frigo dans la table FRIGO par son nom.
// Fallback : extrait le numéro terminal du nom (ex. "Congélateur A2" → 2).
int CongelateurDialog::frigoIdByName(const QString& name) const
{
    if (!name.isEmpty()) {
        QSqlQuery q;
        q.prepare("SELECT \"ID\" FROM \"FRIGO\" WHERE TRIM(UPPER(\"NOM\")) = TRIM(UPPER(:n))");
        q.bindValue(":n", name);
        if (q.exec() && q.next())
            return q.value(0).toInt();

        QSqlQuery legacy;
        legacy.prepare("SELECT \"ID\" FROM \"FRIGO\" WHERE TRIM(UPPER(\"NAME\")) = TRIM(UPPER(:n))");
        legacy.bindValue(":n", name);
        if (legacy.exec() && legacy.next())
            return legacy.value(0).toInt();
    }
    const int n = congNumberFromName(name);
    return (n > 0) ? n : 1;
}

int CongelateurDialog::frigoIdByDoorNum(int doorNum) const
{
    if (!isArduinoCongDoor(doorNum))
        return -1;

    QSqlQuery q;
    q.prepare("SELECT \"ID\" FROM \"FRIGO\" WHERE \"DOOR_NUM\" = :door");
    q.bindValue(":door", doorNum);
    if (q.exec() && q.next())
        return q.value(0).toInt();

    QSqlQuery byId;
    byId.prepare("SELECT \"ID\" FROM \"FRIGO\" WHERE \"ID\" = :id");
    byId.bindValue(":id", doorNum);
    if (byId.exec() && byId.next())
        return byId.value(0).toInt();

    return doorNum;
}

int CongelateurDialog::doorNumByFrigoId(int frigoId) const
{
    if (frigoId <= 0)
        return -1;

    QSqlQuery q;
    q.prepare("SELECT \"DOOR_NUM\" FROM \"FRIGO\" WHERE \"ID\" = :id");
    q.bindValue(":id", frigoId);
    if (q.exec() && q.next() && !q.value(0).isNull()) {
        const int door = q.value(0).toInt();
        if (isArduinoCongDoor(door))
            return door;
    }

    QSqlQuery nameQ;
    nameQ.prepare("SELECT \"NOM\" FROM \"FRIGO\" WHERE \"ID\" = :id");
    nameQ.bindValue(":id", frigoId);
    if (nameQ.exec() && nameQ.next()) {
        const int door = congNumberFromName(nameQ.value(0).toString());
        if (isArduinoCongDoor(door))
            return door;
    }

    return isArduinoCongDoor(frigoId) ? frigoId : -1;
}

bool CongelateurDialog::frigoStatusIsOpen(int frigoId) const
{
    if (frigoId <= 0)
        return false;

    QSqlQuery q;
    q.prepare("SELECT \"STATUS\" FROM \"FRIGO\" WHERE \"ID\" = :id");
    q.bindValue(":id", frigoId);
    if (q.exec() && q.next()) {
        const QString status = q.value(0).toString().trimmed().toUpper();
        return status == "OPEN" || status == "OUVERT" || status == "OUVERTE";
    }
    return false;
}

// Résout un UID RFID en ID de badge (table BADGES)
// Supporte UID avec colons "AB:12:CD:34" ET sans colons "AB12CD34"
int CongelateurDialog::badgeIdByUid(const QString& uid) const
{
    if (uid.isEmpty()) return 0;
    const QString clean    = uid.trimmed().toUpper();
    const QString stripped = QString(clean).remove(':').remove(' ');

    QSqlQuery q;
    q.prepare("SELECT \"ID\" FROM \"BADGES\" WHERE UPPER(\"UID\") = :uid");
    q.bindValue(":uid", clean);
    if (q.exec() && q.next())
        return q.value(0).toInt();

    QSqlQuery q2;
    q2.prepare("SELECT \"ID\" FROM \"BADGES\" "
               "WHERE UPPER(REPLACE(REPLACE(\"UID\",':',''),' ','')) = :uid");
    q2.bindValue(":uid", stripped);
    if (q2.exec() && q2.next())
        return q2.value(0).toInt();

    QSqlQuery q3;
    q3.prepare("SELECT \"ID\" FROM \"BADGES\" "
               "WHERE UPPER(REPLACE(REPLACE(\"ADDRESS\",':',''),' ','')) = :uid");
    q3.bindValue(":uid", stripped);
    if (q3.exec() && q3.next())
        return q3.value(0).toInt();

    return 0;
}

// ─────────────────────────────────────────────────────────────────
//  Reste du dialog (inchangé)
// ─────────────────────────────────────────────────────────────────
void CongelateurDialog::refresh() {
    QString prev = m_currentCong; loadFreezers();
    if (!prev.isEmpty()) loadFreezer(prev);
}

void CongelateurDialog::loadFreezers() {
    m_freezerList->clear();
    QSqlQuery q;
    q.prepare("SELECT DISTINCT \"Emplacement_de_stockage\" FROM \"BioSample\" "
              "WHERE \"Emplacement_de_stockage\" LIKE 'Cong:%'");
    if (!q.exec()) return;
    QStringList seen;
    while (q.next()) {
        QString c = parseCong(q.value(0).toString());
        if (!c.isEmpty() && !seen.contains(c)) {
            seen.append(c);
            auto* item = new QListWidgetItem("❄  " + c);
            item->setData(Qt::UserRole, c);
            m_freezerList->addItem(item);
        }
    }
    if (m_freezerList->count() == 0) {
        auto* item = new QListWidgetItem("Aucun congélateur");
        item->setFlags(Qt::NoItemFlags); m_freezerList->addItem(item);
    } else {
        m_freezerList->setCurrentRow(0);
        loadFreezer(m_freezerList->item(0)->data(Qt::UserRole).toString());
    }
}

void CongelateurDialog::loadFreezer(const QString& cong)
{
    m_currentCong = cong;
    m_freezerWidget->setFreezerName(cong);
    m_freezerWidget->closeDoor();
    m_slotData.clear();
    m_slotData.resize(FreezerWidget::N_SHELVES, QVector<SlotInfo>(FreezerWidget::N_SLOTS));
    QVector<int> nextSlot(FreezerWidget::N_SHELVES, 0);

    QSqlQuery q;
    q.prepare(
        "SELECT \"Reference_de_léchantillon\", \"Type_déchantillon\", \"Organisme_source\","
        "       \"Emplacement_de_stockage\", \"Température_de_stockage\","
        "       \"Quantité_restante\", \"Niveau_de_dangerosité\","
        "       \"Date_de_collecte\", \"Date_dexpiration\" "
        "FROM \"BioSample\" WHERE \"Emplacement_de_stockage\" LIKE :pat");
    q.bindValue(":pat", QString("Cong:%1/%").arg(cong));

    QStringList ctxParts;
    if (q.exec()) {
        while (q.next()) {
            QString emp = q.value(3).toString();
            if (parseCong(emp) != cong) continue;
            int row = etageToRow(parseEtag(emp));
            int col = nextSlot[row];
            if (col >= FreezerWidget::N_SLOTS) continue;
            nextSlot[row]++;
            SlotInfo si;
            si.reference      = q.value(0).toString();
            si.type           = q.value(1).toString();
            si.organisme      = q.value(2).toString();
            si.emplacement    = emp;
            si.temperature    = q.value(4).toString();
            si.quantite       = q.value(5).toInt();
            si.danger         = q.value(6).toString();
            si.dateCollecte   = q.value(7).toDate().toString("dd/MM/yyyy");
            si.dateExpiration = q.value(8).toDate().toString("dd/MM/yyyy");
            si.etage          = parseEtag(emp);
            m_slotData[row][col] = si;
            ctxParts << QString("Réf:%1 Type:%2 Étage:%3 Slot:%4")
                        .arg(si.reference, si.type, si.etage).arg(col+1);
        }
    }
    m_lastContext = ctxParts.join(" | ");

    QVector<QVector<FreezerWidget::Slot>> fwData(FreezerWidget::N_SHELVES,
        QVector<FreezerWidget::Slot>(FreezerWidget::N_SLOTS));
    for (int r = 0; r < FreezerWidget::N_SHELVES; ++r)
        for (int c = 0; c < FreezerWidget::N_SLOTS; ++c) {
            fwData[r][c].occupied  = !m_slotData[r][c].reference.isEmpty();
            fwData[r][c].reference = m_slotData[r][c].reference;
            fwData[r][c].danger    = m_slotData[r][c].danger;
        }
    m_freezerWidget->setData(fwData);
    clearDetails();
    m_statusBar->setText(QString("Congélateur  %1  —  %2 échantillon(s)")
                         .arg(cong).arg(ctxParts.size()));
    rebuildSampleList();

    // Restaurer la température live si déjà connue pour cette porte
    const int doorNum = currentDisplayedDoorNum();
    if (m_currentTemps.contains(doorNum))
        m_freezerWidget->setCurrentTemperature(m_currentTemps[doorNum]);
}

void CongelateurDialog::rebuildSampleList(const QString& filter) {
    m_sampleList->clear();
    QString ft = m_searchFilter ? m_searchFilter->currentText() : "Nom / ID";
    for (int r = 0; r < FreezerWidget::N_SHELVES; ++r)
        for (int c = 0; c < FreezerWidget::N_SLOTS; ++c) {
            const SlotInfo& si = m_slotData[r][c];
            if (si.reference.isEmpty()) continue;
            if (!filter.isEmpty()) {
                QString hay = (ft=="Type") ? si.type : (ft=="Danger") ? si.danger : si.reference;
                if (!hay.contains(filter, Qt::CaseInsensitive)) continue;
            }
            auto* item = new QListWidgetItem(si.reference + "\n" + si.type + "  |  " + rowToLabel(r));
            item->setData(Qt::UserRole, QPoint(r, c));
            m_sampleList->addItem(item);
        }
}

void CongelateurDialog::showDetails(int shelf, int slot) {
    const SlotInfo& si = m_slotData[shelf][slot];
    if (si.reference.isEmpty()) { clearDetails(); return; }
    m_pinIcon->show();
    m_detId      ->setText(si.reference);
    m_detType    ->setText(si.type.isEmpty()        ? "—" : si.type);
    m_detOrg     ->setText(si.organisme.isEmpty()   ? "—" : si.organisme);
    m_detEtage   ->setText(rowToLabel(shelf));
    m_detSlot    ->setText(QString("Slot %1  /  Rack %2").arg(slot+1).arg(slot/4+1));
    m_detTemp    ->setText(si.temperature.isEmpty() ? "—" : si.temperature + " °C");
    m_detDanger  ->setText(si.danger.isEmpty()      ? "—" : si.danger);
    m_detDateCol ->setText(si.dateCollecte.isEmpty()   ? "—" : si.dateCollecte);
    m_detDateExp ->setText(si.dateExpiration.isEmpty() ? "—" : si.dateExpiration);
    m_statusBar->setText(rowToLabel(shelf) + "  |  Slot " + QString::number(slot+1) + "  |  " + si.reference);
    for (int i = 0; i < m_sampleList->count(); ++i) {
        QPoint p = m_sampleList->item(i)->data(Qt::UserRole).toPoint();
        if (p.x()==shelf && p.y()==slot) { m_sampleList->setCurrentRow(i); break; }
    }
}

void CongelateurDialog::clearDetails() {
    if (m_pinIcon)    m_pinIcon->hide();
    if (m_detId)      m_detId      ->setText("—");
    if (m_detType)    m_detType    ->setText("—");
    if (m_detOrg)     m_detOrg     ->setText("—");
    if (m_detEtage)   m_detEtage   ->setText("—");
    if (m_detSlot)    m_detSlot    ->setText("—");
    if (m_detTemp)    m_detTemp    ->setText("—");
    if (m_detDanger)  m_detDanger  ->setText("—");
    if (m_detDateCol) m_detDateCol ->setText("—");
    if (m_detDateExp) m_detDateExp ->setText("—");
    if (m_aiResp)     m_aiResp->setText("Posez une question ci-dessus\npour localiser un échantillon.");
}

void CongelateurDialog::onFreezerClicked(QListWidgetItem* item) {
    loadFreezer(item->data(Qt::UserRole).toString());
}
void CongelateurDialog::onSlotClicked(int shelf, int slot) {
    m_freezerWidget->selectSlot(shelf, slot); showDetails(shelf, slot);
}
void CongelateurDialog::onSampleListClicked(QListWidgetItem* item) {
    QPoint p = item->data(Qt::UserRole).toPoint(); onSlotClicked(p.x(), p.y());
}
void CongelateurDialog::onSearchChanged(const QString& text) {
    rebuildSampleList(text.trimmed());
}
void CongelateurDialog::onAiSearch() {
    QString q = m_aiInput->text().trimmed();
    if (q.isEmpty()) return;
    if (!m_aiBubble)
        m_aiBubble = new AiBubble(this);
    m_aiBtn->setEnabled(false);
    m_aiResp->setText("⏳ Analyse en cours…");
    m_aiBubble->showResponse(
        "<div style='text-align:center; color:rgba(0,232,168,0.85); padding-top:40px;'>"
        "<b style='font-size:22px;'>⏳</b><br><br>"
        "<span style='font-size:12px;'>Analyse de votre question…</span></div>");
    callGroq(q, m_lastContext);
}
void CongelateurDialog::callGroq(const QString& userMsg, const QString& context) {
    QString system = QString(
        "Tu es un expert en biologie et en gestion d'échantillons biologiques intégré dans SmartVision.\n"
        "Congélateur actuel : %1. Échantillons stockés : %2.\n\n"
        "Réponds à TOUTES les questions de biologie (ADN, ARN, protéines, cellules, organismes, maladies, "
        "techniques de laboratoire, etc.) ainsi qu'aux questions de localisation d'échantillons.\n\n"
        "Format de réponse OBLIGATOIRE en HTML :\n"
        "- Utilise <b>gras</b> pour les termes scientifiques clés\n"
        "- Paragraphes séparés par <br>\n"
        "- Termine TOUJOURS avec une section :<br>"
        "<b>🔗 Pour aller plus loin :</b><br>"
        "<a href='https://www.ncbi.nlm.nih.gov'>NCBI</a> | "
        "<a href='https://pubmed.ncbi.nlm.nih.gov/?term='>PubMed</a> | "
        "<a href='https://www.uniprot.org'>UniProt</a> | "
        "<a href='https://www.genome.gov'>Genome.gov</a><br>"
        "Ajoute aussi un lien PubMed spécifique au sujet si pertinent.\n"
        "Réponds en français, clair et précis (3-5 phrases max avant les liens)."
    ).arg(m_currentCong.isEmpty() ? "non sélectionné" : m_currentCong,
          context.isEmpty()       ? "aucun"            : context);
    QJsonArray msgs = {
        QJsonObject{{"role","system"},{"content",system}},
        QJsonObject{{"role","user"},  {"content",userMsg}}
    };
    QJsonObject body{{"model",GROQ_API_MODEL},{"messages",msgs},{"max_tokens",300},{"temperature",0.3}};
    QUrl reqUrl(GROQ_API_URL); QNetworkRequest req(reqUrl);
    req.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    req.setRawHeader("Authorization",("Bearer "+GROQ_API_KEY).toUtf8());
    QSslConfiguration ssl = QSslConfiguration::defaultConfiguration();
    ssl.setPeerVerifyMode(QSslSocket::VerifyNone);
    req.setSslConfiguration(ssl);
    QNetworkReply* rpl = m_net->post(req, QJsonDocument(body).toJson());
    connect(rpl,&QNetworkReply::sslErrors,rpl,[rpl](const QList<QSslError>&){rpl->ignoreSslErrors();});
}
void CongelateurDialog::onAiReply(QNetworkReply* reply) {
    if (!m_aiBubble)
        m_aiBubble = new AiBubble(this);
    m_aiBtn->setEnabled(true);
    QByteArray data = reply->readAll();
    bool netErr = (reply->error() != QNetworkReply::NoError);
    QString errStr = reply->errorString();
    reply->deleteLater();

    if (netErr) {
        m_aiBubble->showResponse(
            "<span style='color:#f87171;'>❌ Erreur réseau : " + errStr.toHtmlEscaped() + "</span>");
        m_aiResp->setText("❌ Erreur réseau.");
        return;
    }
    QJsonObject root = QJsonDocument::fromJson(data).object();
    if (root.contains("error")) {
        QString msg = root["error"].toObject()["message"].toString();
        m_aiBubble->showResponse(
            "<span style='color:#f87171;'>❌ Erreur API : " + msg.toHtmlEscaped() + "</span>");
        m_aiResp->setText("❌ Erreur API.");
        return;
    }
    QString answer = root["choices"].toArray().first()
                         .toObject()["message"].toObject()["content"].toString().trimmed();
    m_aiBubble->showResponse(answer);
    m_aiResp->setText("✅ Réponse reçue — voir la fenêtre flottante.");
    for (int r = 0; r < m_slotData.size(); ++r)
        for (int c = 0; c < m_slotData[r].size(); ++c)
            if (!m_slotData[r][c].reference.isEmpty()
                && answer.contains(m_slotData[r][c].reference, Qt::CaseInsensitive))
            { onSlotClicked(r, c); return; }
}

void CongelateurDialog::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    if (!m_bgPixmap.isNull()) {
        p.drawPixmap(rect(),
                     m_bgPixmap.scaled(size(),
                                       Qt::KeepAspectRatioByExpanding,
                                       Qt::SmoothTransformation));
    } else {
        QLinearGradient grad(rect().topLeft(), rect().bottomRight());
        grad.setColorAt(0, QColor("#e4f2ef"));
        grad.setColorAt(1, QColor("#cde8e3"));
        p.fillRect(rect(), grad);
    }
    p.fillRect(rect(), QColor(0, 0, 0, 30));
}
