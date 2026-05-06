#ifndef FLOATINGCHATBTN_H
#define FLOATINGCHATBTN_H

#include <QWidget>
#include <QPoint>
#include <QPixmap>
#include <QMediaPlayer>
#include <QVideoSink>
#include <QVideoFrame>

class QEvent;
class QShowEvent;

// Bouton flottant draggable qui ouvre ChatBotBioSimple au clic.
// Icône animée par la vidéo :/new/prefix1/iconechatbot.mp4
class FloatingChatBtn : public QWidget
{
    Q_OBJECT
public:
    explicit FloatingChatBtn(QWidget* parent = nullptr);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void paintEvent(QPaintEvent*)        override;
    void mousePressEvent(QMouseEvent*)   override;
    void mouseMoveEvent(QMouseEvent*)    override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void enterEvent(QEnterEvent*)        override;
    void leaveEvent(QEvent*)             override;
    void showEvent(QShowEvent* event)    override;

private:
    void positionFloatingButton(bool forceCenter = false);
    void openChatbot();

    // Vidéo icône
    QMediaPlayer* m_iconPlayer;
    QVideoSink*   m_iconSink;
    QVideoFrame   m_iconFrame;
    QPixmap       m_fallbackIcon;

    QPoint m_dragStart;
    QPoint m_dragOffset;
    bool   m_dragging = false;
    bool   m_hovered  = false;
    bool   m_userMoved = false;
};

#endif // FLOATINGCHATBTN_H
