#include "floatingchatbtn.h"
#include "chatbotbiosimple.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QEnterEvent>
#include <QEvent>
#include <QShowEvent>
#include <QUrl>

static const int BTN_SIZE = 72;
static const int BTN_MARGIN = 24;

FloatingChatBtn::FloatingChatBtn(QWidget* parent)
    : QWidget(parent)
{
    setFixedSize(BTN_SIZE, BTN_SIZE);
    setAttribute(Qt::WA_TranslucentBackground);
    setCursor(Qt::PointingHandCursor);
    setToolTip("Ouvrir le chatbot IA");
    m_fallbackIcon.load(":/image/chatia.png");

    if (parent)
        parent->installEventFilter(this);

    // ── Vidéo icône (sans son) ────────────────────────────────────
    m_iconPlayer = new QMediaPlayer(this);
    m_iconSink   = new QVideoSink(this);
    m_iconPlayer->setVideoSink(m_iconSink);
    m_iconPlayer->setSource(QUrl("qrc:/new/prefix1/iconechatbot.mp4"));
    // Pas de QAudioOutput → pas de son

    connect(m_iconSink, &QVideoSink::videoFrameChanged,
            this, [=](const QVideoFrame& frame) {
        m_iconFrame = frame;
        update();
    });
    connect(m_iconPlayer, &QMediaPlayer::mediaStatusChanged,
            this, [=](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia)
            m_iconPlayer->play();   // boucle infinie
    });
    m_iconPlayer->play();

    positionFloatingButton(true);
    raise();
}

bool FloatingChatBtn::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == parentWidget() && event) {
        switch (event->type()) {
        case QEvent::Resize:
        case QEvent::Move:
        case QEvent::Show:
        case QEvent::WindowStateChange:
        case QEvent::ZOrderChange:
            positionFloatingButton(false);
            raise();
            break;
        default:
            break;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void FloatingChatBtn::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    positionFloatingButton(false);
    raise();
}

void FloatingChatBtn::positionFloatingButton(bool forceCenter)
{
    QWidget* parent = parentWidget();
    if (!parent)
        return;

    const QRect area = parent->rect();
    const int minX = BTN_MARGIN;
    const int maxX = qMax(minX, area.width() - width() - BTN_MARGIN);
    const int minY = BTN_MARGIN;
    const int maxY = qMax(minY, area.height() - height() - BTN_MARGIN);

    QPoint target = pos();
    if (forceCenter || !m_userMoved) {
        target.setX(maxX);
        target.setY(qMax(minY, (area.height() - height()) / 2));
    } else {
        target.setX(qBound(minX, target.x(), maxX));
        target.setY(qBound(minY, target.y(), maxY));
    }

    move(target);
}

void FloatingChatBtn::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int S = BTN_SIZE;

    // Ombre portée
    p.setBrush(QColor(0, 0, 0, 50));
    p.setPen(Qt::NoPen);
    p.drawEllipse(5, 8, S - 8, S - 8);

    if (m_iconFrame.isValid()) {
        // ── Icône vidéo clippée en cercle ──────────────────────
        QImage img = m_iconFrame.toImage();
        if (!img.isNull()) {
            QPainterPath circle;
            circle.addEllipse(2, 2, S - 6, S - 6);

            // Halo hover
            if (m_hovered) {
                p.setBrush(QColor(255, 255, 255, 40));
                p.setPen(QPen(QColor(255, 255, 255, 130), 3));
                p.drawEllipse(1, 1, S - 4, S - 4);
            }

            p.setClipPath(circle);
            p.drawImage(QRect(2, 2, S - 6, S - 6), img);
            p.setClipping(false);

            // Bordure blanche subtile
            p.setPen(QPen(QColor(255, 255, 255, 100), 2));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(2, 2, S - 6, S - 6);
        }
    } else {
        // ── Fallback (avant chargement vidéo) ──────────────────
        QColor bg = m_hovered ? QColor(8, 160, 100) : QColor(10, 140, 88);
        p.setBrush(bg);
        p.setPen(QPen(QColor(255, 255, 255, 90), 2.5));
        p.drawEllipse(2, 2, S - 6, S - 6);

        if (!m_fallbackIcon.isNull()) {
            QPainterPath circle;
            circle.addEllipse(6, 6, S - 12, S - 12);
            p.setClipPath(circle);
            p.drawPixmap(QRect(6, 6, S - 12, S - 12), m_fallbackIcon);
            p.setClipping(false);
            p.setPen(QPen(QColor(255, 255, 255, 125), 2));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(4, 4, S - 8, S - 8);
        } else {
            // Bulle de dialogue
            p.setPen(Qt::NoPen);
            p.setBrush(Qt::white);
            QPainterPath bubble;
            bubble.addRoundedRect(S * 0.22f, S * 0.22f, S * 0.56f, S * 0.40f, 6, 6);
            p.drawPath(bubble);
        }
    }
}

void FloatingChatBtn::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) {
        m_dragStart  = e->globalPosition().toPoint();
        m_dragOffset = e->globalPosition().toPoint() - mapToGlobal(QPoint(0, 0));
        m_dragging   = false;
    }
}

void FloatingChatBtn::mouseMoveEvent(QMouseEvent* e)
{
    if (!(e->buttons() & Qt::LeftButton)) return;

    QPoint delta = e->globalPosition().toPoint() - m_dragStart;
    if (!m_dragging && delta.manhattanLength() > 6)
        m_dragging = true;

    if (m_dragging) {
        QPoint newPos = e->globalPosition().toPoint() - m_dragOffset;
        if (parentWidget())
            newPos = parentWidget()->mapFromGlobal(newPos);

        if (parentWidget()) {
            const int maxX = qMax(BTN_MARGIN, parentWidget()->width()  - width()  - BTN_MARGIN);
            const int maxY = qMax(BTN_MARGIN, parentWidget()->height() - height() - BTN_MARGIN);
            newPos.setX(qBound(BTN_MARGIN, newPos.x(), maxX));
            newPos.setY(qBound(BTN_MARGIN, newPos.y(), maxY));
        }
        m_userMoved = true;
        move(newPos);
    }
}

void FloatingChatBtn::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton && !m_dragging) {
        openChatbot();
    }
    m_dragging = false;
}

void FloatingChatBtn::enterEvent(QEnterEvent*)
{
    m_hovered = true;
    update();
}

void FloatingChatBtn::leaveEvent(QEvent*)
{
    m_hovered = false;
    update();
}

void FloatingChatBtn::openChatbot()
{
    QWidget* hostWindow = window();
    ChatBotBioSimple* bot = new ChatBotBioSimple(hostWindow);
    if (hostWindow) {
        const QRect hostRect = hostWindow->geometry();
        bot->move(hostRect.center().x() - bot->width() / 2,
                  hostRect.center().y() - bot->height() / 2);
    }
    bot->exec();
}
