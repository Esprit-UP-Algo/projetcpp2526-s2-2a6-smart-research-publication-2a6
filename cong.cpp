#include "cong.h"

#include <QPainter>
#include <QPainterPath>
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
#include "apiconfig.h"


static QString parseCong(const QString& emp) {
    int s = emp.indexOf("Cong:") + 5, e = emp.indexOf("/", s);
    return (s >= 5 && e > s) ? emp.mid(s, e - s) : QString();
}
static QString parseEtag(const QString& emp) {
    int s = emp.indexOf("Etag:") + 5;
    return s >= 5 ? emp.mid(s) : QString();
}
static int etageNum(const QString& etage) {
    // Extract trailing digit sequence: handles "A4", "E4", "4", "A10", etc.
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

// ═══════════════════════════════════════════════════════════════
// AiBubble — floating draggable AI response window
// ═══════════════════════════════════════════════════════════════
AiBubble::AiBubble(QWidget* parent)
    : QFrame(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
{
    setFixedSize(380, 330);
    setAttribute(Qt::WA_DeleteOnClose, false);

    // ── Video background via QVideoSink (no black bars, overlay works correctly) ──
    m_videoBg = new QLabel(this);
    m_videoBg->setGeometry(0, 0, 380, 330);
    m_videoBg->setScaledContents(true);   // fills entire area, no black bars

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
    m_player->play();

    // ── Dark semi-transparent overlay ──────────────────────────
    m_overlay = new QWidget(this);
    m_overlay->setGeometry(0, 0, 380, 330);
    m_overlay->setStyleSheet("background: rgba(6, 14, 28, 0.76); border-radius: 14px;");

    auto* vl = new QVBoxLayout(m_overlay);
    vl->setContentsMargins(12, 10, 12, 10);
    vl->setSpacing(7);

    // Title bar (drag zone)
    auto* titleRow = new QHBoxLayout;
    titleRow->setContentsMargins(0, 0, 0, 0);
    auto* titleIco = new QLabel("🤖");
    titleIco->setStyleSheet("font-size:16px; background:transparent; border:none;");
    auto* titleLbl = new QLabel("Assistant IA Biologie");
    titleLbl->setStyleSheet(
        "color: rgba(0,232,168,0.95); font-weight:900; font-size:13px;"
        "background:transparent; border:none;");
    auto* closeBtn = new QPushButton("✕");
    closeBtn->setFixedSize(24, 24);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(
        "QPushButton{ background:rgba(220,55,48,0.85); color:white; border:none;"
        " border-radius:12px; font-weight:900; font-size:11px; }"
        "QPushButton:hover{ background:rgba(255,75,65,1.0); }");
    connect(closeBtn, &QPushButton::clicked, this, &AiBubble::hideResponse);
    titleRow->addWidget(titleIco);
    titleRow->addWidget(titleLbl, 1);
    titleRow->addWidget(closeBtn);
    vl->addLayout(titleRow);

    // Separator
    auto* sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("border: 1px solid rgba(0,232,168,0.28); background:transparent;");
    vl->addWidget(sep);

    // Status label (loading indicator)
    m_statusLbl = new QLabel("⏳ Analyse en cours…");
    m_statusLbl->setStyleSheet(
        "color:rgba(0,200,140,0.80); font-size:11px; background:transparent; border:none;");
    vl->addWidget(m_statusLbl);

    // Response browser (HTML with clickable links)
    m_textBrowser = new QTextBrowser(m_overlay);
    m_textBrowser->setOpenExternalLinks(true);
    m_textBrowser->setOpenLinks(true);
    m_textBrowser->setStyleSheet(
        "QTextBrowser{ background:transparent; color:rgba(235,245,255,0.93);"
        " font-size:12px; border:none; selection-background-color:rgba(0,200,140,0.35); }"
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
    m_statusLbl->hide();
    m_textBrowser->setHtml(html);
    m_textBrowser->show();
    if (!isVisible()) {
        // Position near the parent center-right on first show
        if (parentWidget()) {
            QRect pr = parentWidget()->geometry();
            move(pr.right() - 400, pr.center().y() - 165);
        }
    }
    show();
    raise();
    activateWindow();
}

void AiBubble::hideResponse() {
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
    setMaximumWidth(460);                                         // keep portrait
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    m_data.resize(N_SHELVES, QVector<Slot>(N_SLOTS));

    m_doorAnim = new QPropertyAnimation(this, "doorOpen", this);
    m_doorAnim->setDuration(700);
    m_doorAnim->setEasingCurve(QEasingCurve::InOutCubic);

    // Blinking colon for the clock display
    m_clockTimer = new QTimer(this);
    m_clockTimer->setInterval(500);
    connect(m_clockTimer, &QTimer::timeout, this, [this]() {
        m_colonVisible = !m_colonVisible;
        if (m_doorOpen < 0.95f) update(); // repaint only when door visible
    });
    m_clockTimer->start();

    // Application logo for the door
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

void FreezerWidget::setData(const QVector<QVector<Slot>>& shelves) {
    m_data = shelves; m_selShelf = m_selSlot = -1; update();
}
void FreezerWidget::selectSlot(int shelf, int slot) {
    m_selShelf = shelf; m_selSlot = slot; update();
}
void FreezerWidget::clearSelection() { selectSlot(-1, -1); }

// ── Geometry for realistic 3-D freezer ───────────────────────────
// Perspective: moderate depth, view slightly from upper-left
static const float RDX = 52.0f;   // depth → right
static const float RDY = -34.0f;  // depth → up
static const float R_LABEL_W  = 62.0f;
static const float R_SLOT_GAP = 3.0f;
static const float R_SHELF_TH = 8.0f;   // shelf thickness (front edge height)

struct RGeo {
    float cL,cR,cT,cB;       // outer cabinet front face
    float iL,iR,iT,iB;       // inner cavity (opening)
    float shH;                // shelf height
    float sX1,sX2,sW;        // slots area
};
static RGeo makeRGeo(float W, float H) {
    RGeo g;
    g.cL = 14;          g.cR = W - RDX - 14;
    g.cT = -RDY + 10;   g.cB = H - 66;   // raised to leave room for control strip below
    g.iL = g.cL + 18;   g.iR = g.cR - 12;
    g.iT = g.cT + 22;   g.iB = g.cB - 16;
    g.shH  = (g.iB - g.iT) / float(FreezerWidget::N_SHELVES);
    g.sX1  = g.iL + R_LABEL_W + 5;
    g.sX2  = g.iR - 5;
    float avail = g.sX2 - g.sX1 - R_SLOT_GAP * (FreezerWidget::N_SLOTS - 1);
    g.sW   = avail / float(FreezerWidget::N_SLOTS);
    return g;
}

// ── Coordinate helpers ─────────────────────────────────────────────
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

// ── Pin ────────────────────────────────────────────────────────────
void FreezerWidget::drawPin(QPainter& p, QRectF sr) const {
    float cx = sr.center().x(), tipY = sr.top() - 2.0f;
    float r  = 12.0f, cyF = tipY - r * 2.2f;
    QPainterPath path;
    path.moveTo(cx, tipY);
    path.arcTo(QRectF(cx-r, cyF-r, r*2, r*2), 210.0f, 300.0f);
    path.lineTo(cx, tipY);
    // Shadow
    p.setBrush(QColor(0,0,0,55)); p.setPen(Qt::NoPen);
    p.drawPath(path.translated(2, 3));
    // Body
    QRadialGradient rg(cx - r*0.3f, cyF - r*0.3f, r*1.5f);
    rg.setColorAt(0, QColor(255, 80, 80));
    rg.setColorAt(0.6, QColor(210, 40, 40));
    rg.setColorAt(1, QColor(150, 15, 15));
    p.setBrush(rg);
    p.setPen(QPen(QColor(120, 10, 10), 1.2));
    p.drawPath(path);
    // Highlight dot
    float hr = r * 0.38f;
    p.setBrush(QColor(255,255,255,200)); p.setPen(Qt::NoPen);
    p.drawEllipse(QRectF(cx-hr, cyF-hr, hr*2, hr*2));
}

// ═══════════════════════════════════════════════════════════════════
// Realistic 3-D paintEvent
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

    // ── 1. OUTER CABINET (white modern style) ──────────────────────

    // Right side face — light gray shadow
    {
        QLinearGradient rg(g.cR, g.cT, g.cR + RDX, g.cB);
        rg.setColorAt(0, QColor(195, 200, 206));
        rg.setColorAt(1, QColor(170, 176, 182));
        p.setBrush(rg); p.setPen(Qt::NoPen);
        p.drawPolygon(quad({g.cR, g.cT}, PT(g.cR, g.cT),
                            PT(g.cR, g.cB), {g.cR, g.cB}));
    }

    // Top face — bright lit white
    {
        QLinearGradient tg(g.cL, g.cT, g.cR + RDX, g.cT + RDY);
        tg.setColorAt(0,   QColor(252, 253, 255));
        tg.setColorAt(0.5, QColor(238, 241, 244));
        tg.setColorAt(1,   QColor(215, 220, 226));
        p.setBrush(tg); p.setPen(Qt::NoPen);
        p.drawPolygon(quad({g.cL, g.cT}, {g.cR, g.cT},
                            PT(g.cR, g.cT), PT(g.cL, g.cT)));
    }

    // Front face — white/light gray body
    {
        QLinearGradient fg(g.cL, g.cT, g.cR, g.cB);
        fg.setColorAt(0.0, QColor(248, 250, 252));
        fg.setColorAt(0.5, QColor(240, 242, 245));
        fg.setColorAt(1.0, QColor(225, 228, 232));
        p.setBrush(fg);
        p.setPen(QPen(QColor(185, 190, 196), 1.8));
        p.drawRoundedRect(QRectF(g.cL, g.cT, g.cR - g.cL, g.cB - g.cT), 12, 12);
    }

    // ── 2. CABINET TOP SENSOR STRIP ────────────────────────────────
    {
        // Subtle top inner rim
        p.setBrush(QColor(210, 215, 220));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(QRectF(g.cL + 8, g.cT + 4, g.cR - g.cL - 16, 4), 2, 2);
    }

    // ── 3. INNER CAVITY (bright glass-like interior) ───────────────

    // Back wall — light blue-gray
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

    // Left inner wall
    {
        QLinearGradient lw(g.iL, 0, g.iL + RDX * 0.88f, 0);
        lw.setColorAt(0, QColor(195, 212, 228));
        lw.setColorAt(1, QColor(218, 232, 244));
        p.setBrush(lw); p.setPen(Qt::NoPen);
        p.drawPolygon(quad({g.iL, g.iT}, PT(g.iL, g.iT, 0.88f),
                            PT(g.iL, g.iB, 0.88f), {g.iL, g.iB}));
    }

    // Top inner ceiling
    {
        QLinearGradient cw(0, g.iT, 0, g.iT + 18);
        cw.setColorAt(0, QColor(245, 250, 255, 235));
        cw.setColorAt(1, QColor(225, 238, 250, 160));
        p.setBrush(cw); p.setPen(Qt::NoPen);
        p.drawPolygon(quad({g.iL, g.iT}, {g.iR, g.iT},
                            PT(g.iR, g.iT, 0.88f), PT(g.iL, g.iT, 0.88f)));
    }

    // Interior front face (visible when door open) — glass-like light blue
    {
        QLinearGradient ig(g.iL, g.iT, g.iL, g.iB);
        ig.setColorAt(0.0, QColor(240, 246, 252));
        ig.setColorAt(0.5, QColor(232, 240, 250));
        ig.setColorAt(1.0, QColor(215, 228, 242));
        p.setBrush(ig); p.setPen(Qt::NoPen);
        p.drawRect(QRectF(g.iL, g.iT, g.iR - g.iL, g.iB - g.iT));
    }

    // Door frame gasket — white/light border
    {
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(QColor(185, 192, 200), 3.0));
        p.drawRoundedRect(QRectF(g.iL - 2, g.iT - 2, g.iR - g.iL + 4, g.iB - g.iT + 4), 5, 5);
        p.setPen(QPen(QColor(0, 0, 0, 25), 1.2));
        p.drawRoundedRect(QRectF(g.iL + 2, g.iT + 2, g.iR - g.iL - 4, g.iB - g.iT - 4), 3, 3);
    }

    // ── 4. SHELVES & REALISTIC SAMPLE BOXES ────────────────────────

    for (int row = 0; row < N_SHELVES; ++row) {
        float sy1 = g.iT + row * g.shH;
        float sfY = sy1 + g.shH - R_SHELF_TH;

        // Shelf top surface (white/light gray parallelogram)
        {
            float z = 0.7f;
            QLinearGradient stg(g.iL, sfY, g.iL, sfY + 4);
            stg.setColorAt(0, QColor(250, 252, 254));
            stg.setColorAt(1, QColor(228, 238, 246));
            p.setBrush(stg); p.setPen(Qt::NoPen);
            p.drawPolygon(quad({g.iL + 2, sfY}, {g.iR - 2, sfY},
                                PT(g.iR-2, sfY, z), PT(g.iL+2, sfY, z)));
        }

        // Shelf front edge (white beveled bar)
        {
            QLinearGradient eg(g.iL, sfY, g.iL, sfY + R_SHELF_TH);
            eg.setColorAt(0, QColor(245, 249, 252));
            eg.setColorAt(0.5, QColor(225, 236, 244));
            eg.setColorAt(1, QColor(195, 210, 222));
            p.setBrush(eg);
            p.setPen(QPen(QColor(190, 205, 218), 0.8));
            p.drawRect(QRectF(g.iL + 2, sfY, g.iR - g.iL - 4, R_SHELF_TH));
        }

        // Shelf label — white card with teal border
        {
            float ly = sy1 + (g.shH - 22.0f) / 2.0f;
            QRectF lr(g.iL + 3, ly, R_LABEL_W - 3, 22);
            p.setBrush(QColor(255, 255, 255, 235));
            p.setPen(QPen(QColor(10, 95, 88), 1.8));
            p.drawRoundedRect(lr, 6, 6);
            // Colored left strip
            QRectF strip(lr.left(), lr.top(), 5, lr.height());
            p.setBrush(QColor(10, 95, 88));
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(strip, 3, 3);
            p.setPen(QColor(10, 70, 64));
            p.setFont(QFont("Arial", 9, QFont::Bold));
            p.drawText(QRectF(lr.left() + 8, lr.top(), lr.width() - 8, lr.height()),
                       Qt::AlignVCenter | Qt::AlignLeft, rowToLabel(row));
        }

        // ── Realistic sample boxes (white lab containers) ────────
        for (int col = 0; col < N_SLOTS; ++col) {
            QRectF sr = slotRectF(row, col);
            bool sel = (row == m_selShelf && col == m_selSlot);
            bool occ = (row < m_data.size() && col < m_data[row].size())
                        && m_data[row][col].occupied;
            QString danger = occ ? m_data[row][col].danger : QString();

            if (occ || sel) {
                // Determine accent color by danger level
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

                // Right side of box (slightly darker)
                {
                    QColor sideC = frontBase.darker(118);
                    p.setBrush(sideC); p.setPen(Qt::NoPen);
                    p.drawPolygon(quad({sr.right(), sr.top()},
                                       PT(sr.right(), sr.top(), z),
                                       PT(sr.right(), sr.bottom(), z),
                                       {sr.right(), sr.bottom()}));
                }

                // Top face of box (lightest)
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

                // Front face of box
                {
                    QLinearGradient fg(sr.left(), sr.top(), sr.left(), sr.bottom());
                    fg.setColorAt(0, frontBase);
                    fg.setColorAt(1, frontBase.darker(112));
                    p.setBrush(fg);
                    p.setPen(QPen(QColor(200,215,225), 0.8));
                    p.drawRoundedRect(sr, 2, 2);
                }

                // Lid groove line (horizontal crease)
                {
                    float lidY = sr.top() + sr.height() * 0.28f;
                    p.setPen(QPen(accent.lighter(170), 0.9));
                    p.drawLine(QPointF(sr.left()+2, lidY), QPointF(sr.right()-2, lidY));
                    p.setPen(QPen(QColor(255,255,255,180), 0.5));
                    p.drawLine(QPointF(sr.left()+2, lidY+1), QPointF(sr.right()-2, lidY+1));
                }

                // Colored label tape on lid
                {
                    QRectF tape(sr.left() + 2, sr.top() + 3, sr.width() - 4, 4);
                    p.setBrush(accent);
                    p.setPen(Qt::NoPen);
                    p.drawRoundedRect(tape, 1, 1);
                }

                // Selection glow
                if (sel) {
                    p.setBrush(Qt::NoBrush);
                    p.setPen(QPen(QColor(52, 130, 218, 180), 2));
                    p.drawRoundedRect(sr.adjusted(-1,-1,1,1), 3, 3);
                }

            } else {
                // Empty slot — subtle outlined recess
                p.setBrush(QColor(238, 248, 254, 60));
                p.setPen(QPen(QColor(190, 210, 225, 120), 0.7, Qt::DashLine));
                p.drawRoundedRect(sr, 2, 2);
            }
        }
    }

    // ── 5. ANIMATED DOOR ─────────────────────────────────────────
    {
        float dL    = g.iL - 2;
        float dT    = g.iT - 2;
        float dB    = g.iB + 2;
        float dH    = dB - dT;
        float fullW = g.iR - g.iL + 4;
        float t     = m_doorOpen;

        // ── Swung-open door panel (right side) ───────────────────
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
            // Inner border
            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(QColor(3, 40, 37, ia), 1.2));
            p.drawRoundedRect(QRectF(dL2 + 4, dT + 8, dW2 - 10, dH - 16), 4, 4);
            // Gasket rubber strip
            p.setBrush(QColor(5, 58, 53, ia));
            p.setPen(Qt::NoPen);
            p.drawRoundedRect(QRectF(dL2 + 3, dT + 16, 5, dH - 32), 2, 2);
            // Texture lines
            p.setPen(QPen(QColor(255, 255, 255, int(22 * alpha)), 0.7));
            for (float fy = dT + 30; fy < dB - 18; fy += 18)
                p.drawLine(QPointF(dL2 + 10, fy), QPointF(dL2 + dW2 - 5, fy));

            // Fermer button on swung panel
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

        // ── Door front face (shrinks as door opens) ───────────────
        float frontW = fullW * (1.0f - t);
        if (frontW > 1.0f) {
            // Main teal gradient
            QLinearGradient dg(dL, dT, dL + frontW, dT);
            dg.setColorAt(0.0f, QColor(10, 110, 102));
            dg.setColorAt(0.45f, QColor(18, 132, 122));
            dg.setColorAt(1.0f, QColor(7, 84, 76));
            p.setBrush(dg);
            p.setPen(QPen(QColor(4, 52, 48), 2.2));
            p.drawRoundedRect(QRectF(dL, dT, frontW, dH), 8, 8);

            // Dark green outer border inset
            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(QColor(3, 42, 39, 200), 1.8));
            p.drawRoundedRect(QRectF(dL + 5, dT + 5, frontW - 10, dH - 10), 6, 6);

            float curY = dT + 10;

            // ── Digital clock + temperature (top of door) ─────────
            if (frontW > 62) {
                float cw = qMin(frontW - 18, 135.0f), ch = 32.0f;
                float cx = dL + (frontW - cw) / 2.0f;
                // Black LCD panel
                p.setBrush(QColor(5, 4, 3));
                p.setPen(QPen(QColor(55, 38, 16), 1.2));
                p.drawRoundedRect(QRectF(cx, curY, cw, ch), 5, 5);
                // Status LED dot (green)
                QRadialGradient led(cx + 7, curY + ch / 2, 4.0f);
                led.setColorAt(0, m_colonVisible ? QColor(120, 255, 150) : QColor(40, 130, 60));
                led.setColorAt(1, m_colonVisible ? QColor(0,  200,  80) : QColor(10, 70, 30));
                p.setBrush(led); p.setPen(Qt::NoPen);
                p.drawEllipse(QRectF(cx + 4, curY + ch / 2 - 4, 7, 7));
                // Time (orange, left portion)
                QString timeStr = QTime::currentTime().toString(
                    m_colonVisible ? "hh:mm" : "hh mm");
                p.setPen(QColor(255, 115, 22));
                p.setFont(QFont("Courier New", 12, QFont::Bold));
                p.drawText(QRectF(cx + 14, curY + 1, cw * 0.58f, ch - 2),
                           Qt::AlignVCenter | Qt::AlignHCenter, timeStr);
                // Temperature (teal, right portion)
                p.setPen(QColor(0, 232, 168));
                p.setFont(QFont("Courier New", 9, QFont::Bold));
                p.drawText(QRectF(cx + cw * 0.58f, curY, cw * 0.42f, ch),
                           Qt::AlignCenter, "-80°C");
                curY += ch + 7;
            }

            // ── App logo (center of door) ──────────────────────────
            if (!m_logoPixmap.isNull() && frontW > 72) {
                float avail  = dB - curY - 40;   // space above Ouvrir button
                float lh     = qMax(qMin(avail * 0.60f, 56.0f), 18.0f);
                float aspect = float(m_logoPixmap.width()) / float(m_logoPixmap.height());
                float lw     = qMin(lh * aspect, frontW - 24.0f);
                lh = lw / aspect;
                float lx = dL + (frontW - lw) / 2.0f;
                float ly = curY + (avail - lh) / 2.0f;
                if (ly < curY) ly = curY;
                p.drawPixmap(QRectF(lx, ly, lw, lh), m_logoPixmap,
                             QRectF(m_logoPixmap.rect()));
                // Freezer name below logo
                if (frontW > 70 && !m_freezerName.isEmpty()) {
                    p.setPen(QColor(255, 255, 255, 210));
                    p.setFont(QFont("Arial", 9, QFont::Bold));
                    p.drawText(QRectF(dL + 8, ly + lh + 2, frontW - 16, 18),
                               Qt::AlignCenter, m_freezerName);
                }
            } else if (frontW > 50 && !m_freezerName.isEmpty()) {
                // No logo — just show name
                p.setPen(Qt::white);
                p.setFont(QFont("Arial", 10, QFont::Bold));
                p.drawText(QRectF(dL + 8, curY, frontW - 16, dB - curY - 38),
                           Qt::AlignCenter, m_freezerName);
            }

            // ── Ouvrir button (bottom of door face) ───────────────
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

            // ── Door handle ────────────────────────────────────────
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

            // ── Subtle texture lines ───────────────────────────────
            p.setPen(QPen(QColor(255, 255, 255, 14), 0.7));
            for (float rx = dL + 20; rx < dL + frontW - 17; rx += 24)
                p.drawLine(QPointF(rx, dT + 14), QPointF(rx, dB - 14));

            // ── Side edge depth ────────────────────────────────────
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

    // ── 6. PIN (only visible when door is open) ────────────────────
    if (m_selShelf >= 0 && m_selSlot >= 0 && m_doorOpen > 0.5f)
        drawPin(p, slotRectF(m_selShelf, m_selSlot));

    // ── 7. CONTROL STRIP (below the cabinet) ──────────────────────
    {
        float stripT = g.cB + 8;
        float stripB = H - 8;
        float stripH = stripB - stripT;
        float stripL = g.cL;
        float stripR = g.cR;
        float stripW = stripR - stripL;

        // Main body — dark metallic gradient
        QLinearGradient bg(stripL, stripT, stripL, stripB);
        bg.setColorAt(0.0f, QColor(82,  88,  98));
        bg.setColorAt(0.3f, QColor(68,  74,  84));
        bg.setColorAt(1.0f, QColor(52,  56,  64));
        p.setBrush(bg);
        p.setPen(QPen(QColor(38, 42, 50), 1.5));
        p.drawRoundedRect(QRectF(stripL, stripT, stripW, stripH), 7, 7);

        // Inner highlight line at top
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(QColor(130, 138, 150, 90), 1.0));
        p.drawLine(QPointF(stripL + 10, stripT + 3), QPointF(stripR - 10, stripT + 3));

        float midY = stripT + stripH / 2.0f;

        // ── LCD chip (left) ────────────────────────────────────────
        float chipX = stripL + 10, chipY = stripT + (stripH - 18) / 2.0f;
        QLinearGradient chipG(chipX, chipY, chipX + 26, chipY + 18);
        chipG.setColorAt(0, QColor(45, 50, 58));
        chipG.setColorAt(1, QColor(30, 34, 40));
        p.setBrush(chipG);
        p.setPen(QPen(QColor(22, 26, 32), 1.0));
        p.drawRoundedRect(QRectF(chipX, chipY, 26, 18), 3, 3);
        // Chip pins (top + bottom)
        p.setPen(QPen(QColor(145, 155, 168), 0.8));
        for (int pi = 0; pi < 4; ++pi) {
            float px = chipX + 4 + pi * 6;
            p.drawLine(QPointF(px, chipY),     QPointF(px, chipY - 3));
            p.drawLine(QPointF(px, chipY + 18), QPointF(px, chipY + 21));
        }
        // Chip label
        p.setPen(QColor(0, 200, 120, 180));
        p.setFont(QFont("Courier New", 5, QFont::Bold));
        p.drawText(QRectF(chipX + 1, chipY + 1, 24, 16), Qt::AlignCenter, "MCU\n01");

        // Power LED
        float ledX = chipX + 32, ledY = midY - 4;
        QRadialGradient ledG(ledX + 4, ledY + 4, 5);
        ledG.setColorAt(0, m_colonVisible ? QColor(80, 255, 140) : QColor(20, 160, 70));
        ledG.setColorAt(1, m_colonVisible ? QColor(0,  200,  90) : QColor(8,  90,  36));
        p.setBrush(ledG); p.setPen(Qt::NoPen);
        p.drawEllipse(QRectF(ledX, ledY, 8, 8));
        // LED lens glint
        p.setBrush(QColor(255, 255, 255, 110));
        p.drawEllipse(QRectF(ledX + 1.5f, ledY + 1.0f, 3, 3));

        // ── CONTROL label (center) ─────────────────────────────────
        p.setPen(QColor(185, 195, 210));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(QRectF(stripL + stripW * 0.38f, stripT, stripW * 0.25f, stripH),
                   Qt::AlignCenter, "CONTROL");

        // ── Ventilation dots grid (right) ─────────────────────────
        float gx = stripR - 56, gy = stripT + (stripH - 20) / 2.0f;
        for (int di = 0; di < 3; ++di)
            for (int dj = 0; dj < 6; ++dj) {
                float dotX = gx + dj * 8.0f, dotY = gy + di * 8.0f;
                // Shadow
                p.setBrush(QColor(35, 39, 46)); p.setPen(Qt::NoPen);
                p.drawEllipse(QRectF(dotX + 1, dotY + 1, 4, 4));
                // Dot
                p.setBrush(QColor(105, 115, 128));
                p.drawEllipse(QRectF(dotX, dotY, 4, 4));
            }

        // Bottom edge highlight
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(QColor(255, 255, 255, 18), 0.8));
        p.drawLine(QPointF(stripL + 10, stripB - 3), QPointF(stripR - 10, stripB - 3));
    }

}

void FreezerWidget::mousePressEvent(QMouseEvent* e) {
    QPointF pos = e->position();

    // Door "Ouvrir" button (on closed door face)
    if (m_doorOpenBtnRect.isValid() && m_doorOpenBtnRect.contains(pos)) {
        openDoor(); return;
    }
    // Door "Fermer" button (on swung-open panel)
    if (m_doorCloseBtnRect.isValid() && m_doorCloseBtnRect.contains(pos)) {
        closeDoor(); return;
    }

    // Sample clicks only when door is sufficiently open
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
    f->setStyleSheet("QFrame{ background:rgba(255,255,255,0.82);"
                     "border:1px solid rgba(10,95,88,0.15); border-radius:14px; }");
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
    setStyleSheet(
        "QDialog{ background:qlineargradient(x1:0,y1:0,x2:1,y2:1,"
        "    stop:0 #e4f2ef, stop:1 #cde8e3); }"
        "QScrollBar:vertical{ background:transparent; width:6px; }"
        "QScrollBar::handle:vertical{ background:rgba(10,95,88,0.30); border-radius:3px; }"
        "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{ height:0; }");

    m_net  = new QNetworkAccessManager(this);
    m_crud = new CrudeBioSimple;
    connect(m_net, &QNetworkAccessManager::finished, this, &CongelateurDialog::onAiReply);

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
    // AI widgets created here, added to right panel below
    m_aiInput = new QLineEdit;
    m_aiInput->setPlaceholderText("🔍  Posez une question sur un échantillon…");
    m_aiInput->setStyleSheet("QLineEdit{ background:rgba(255,255,255,0.90);"
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

    // Body
    auto* bodyL = new QHBoxLayout; bodyL->setSpacing(12);

    // Left
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
    // search widgets removed from UI
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

    // Center — portrait freezer, horizontally centered
    auto* centerCard = card(); auto* centerVL = new QVBoxLayout(centerCard);
    centerVL->setContentsMargins(14,14,14,10); centerVL->setSpacing(8);
    auto* centerTitle = sectionTitle("Localisation de l'Échantillon");
    centerTitle->setStyleSheet("font-weight:900; font-size:13px; color:rgba(10,95,88,0.90);"
                               "border:none; background:transparent;");
    centerVL->addWidget(centerTitle);

    // Horizontal row to center the portrait freezer widget
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

    // Floating AI bubble
    m_aiBubble = new AiBubble(this);

    connect(m_freezerList,  &QListWidget::itemClicked,  this, &CongelateurDialog::onFreezerClicked);
    connect(m_freezerWidget,&FreezerWidget::slotClicked,this, &CongelateurDialog::onSlotClicked);
    connect(m_sampleList,   &QListWidget::itemClicked,  this, &CongelateurDialog::onSampleListClicked);
    connect(m_aiBtn,        &QPushButton::clicked,      this, &CongelateurDialog::onAiSearch);
    connect(m_aiInput,      &QLineEdit::returnPressed,  this, &CongelateurDialog::onAiSearch);

    loadFreezers();
}

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
    m_freezerWidget->closeDoor();   // Always start closed when switching freezers
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
    m_aiBtn->setEnabled(false);
    m_aiResp->setText("⏳ Analyse en cours…");
    // Show bubble immediately in loading state
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

    // Show in floating bubble (answer already HTML from prompt)
    m_aiBubble->showResponse(answer);
    m_aiResp->setText("✅ Réponse reçue — voir la fenêtre flottante.");

    // Auto-highlight matching sample in freezer
    for (int r = 0; r < m_slotData.size(); ++r)
        for (int c = 0; c < m_slotData[r].size(); ++c)
            if (!m_slotData[r][c].reference.isEmpty()
                && answer.contains(m_slotData[r][c].reference, Qt::CaseInsensitive))
            { onSlotClicked(r, c); return; }
}
