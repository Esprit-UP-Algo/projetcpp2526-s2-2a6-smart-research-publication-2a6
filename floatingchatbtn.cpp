#include "floatingchatbtn.h"
#include "chatbotbiosimple.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QEnterEvent>
#include <QUrl>

static const int BTN_SIZE = 72;

FloatingChatBtn::FloatingChatBtn(QWidget* parent)
    : QWidget(parent)
{
    setFixedSize(BTN_SIZE, BTN_SIZE);
    setAttribute(Qt::WA_TranslucentBackground);
    setCursor(Qt::PointingHandCursor);
    setToolTip("Ouvrir le chatbot IA");

    // Position initiale : coin inférieur droit avec marge
    if (parent)
        move(parent->width() - BTN_SIZE - 24, parent->height() - BTN_SIZE - 24);

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

    raise();
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

        // Bulle de dialogue
        p.setPen(Qt::NoPen);
        p.setBrush(Qt::white);
        QPainterPath bubble;
        bubble.addRoundedRect(S * 0.22f, S * 0.22f, S * 0.56f, S * 0.40f, 6, 6);
        p.drawPath(bubble);
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
            newPos.setX(qBound(0, newPos.x(), parentWidget()->width()  - width()));
            newPos.setY(qBound(0, newPos.y(), parentWidget()->height() - height()));
        }
        move(newPos);
    }
}

void FloatingChatBtn::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton && !m_dragging) {
        ChatBotBioSimple* bot = new ChatBotBioSimple(window());
        QPoint center = window()->geometry().center();
        bot->move(center.x() - bot->width() / 2,
                  center.y() - bot->height() / 2);
        bot->exec();
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
