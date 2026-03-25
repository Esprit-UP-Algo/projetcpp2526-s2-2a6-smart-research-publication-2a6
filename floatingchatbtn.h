#ifndef FLOATINGCHATBTN_H
#define FLOATINGCHATBTN_H

#include <QWidget>
#include <QPoint>
#include <QMediaPlayer>
#include <QVideoSink>
#include <QVideoFrame>

// Bouton flottant draggable qui ouvre ChatBotBioSimple au clic.
// Icône animée par la vidéo :/new/prefix1/iconechatbot.mp4
class FloatingChatBtn : public QWidget
{
    Q_OBJECT
public:
    explicit FloatingChatBtn(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent*)        override;
    void mousePressEvent(QMouseEvent*)   override;
    void mouseMoveEvent(QMouseEvent*)    override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void enterEvent(QEnterEvent*)        override;
    void leaveEvent(QEvent*)             override;

private:
    // Vidéo icône
    QMediaPlayer* m_iconPlayer;
    QVideoSink*   m_iconSink;
    QVideoFrame   m_iconFrame;

    QPoint m_dragStart;
    QPoint m_dragOffset;
    bool   m_dragging = false;
    bool   m_hovered  = false;
};

#endif // FLOATINGCHATBTN_H
