#ifndef CHATBOTBIOSIMPLE_H
#define CHATBOTBIOSIMPLE_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QWidget>
#include <QFrame>
#include <QScrollBar>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMediaPlayer>
#include <QVideoSink>
#include <QVideoFrame>

class ChatBotBioSimple : public QDialog
{
    Q_OBJECT
public:
    explicit ChatBotBioSimple(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
    void sendMessage();
    void clearConversation();
    void onApiReply(QNetworkReply* reply);

private:
    void    addMessage(const QString& text, bool isUser, bool richText = false);
    void    addTypingIndicator();
    void    removeTypingIndicator();
    void    callOpenAI(const QString& userMessage);
    QString formatResponse(const QString& text);

    QWidget*     m_msgContainer;
    QVBoxLayout* m_msgLayout;
    QScrollArea* m_scroll;
    QLineEdit*   m_input;
    QPushButton* m_sendBtn;
    QPushButton* m_clearBtn;

    QNetworkAccessManager* m_net;
    QWidget*               m_typingWidget = nullptr;

    QList<QPair<QString,QString>> m_history;
    QString m_lastUserMsg;

    // Vidéo de fond
    QMediaPlayer* m_bgPlayer;
    QVideoSink*   m_bgSink;
    QVideoFrame   m_bgFrame;

    // Drag
    QPoint m_dragPos;
    bool   m_dragging = false;
};

#endif // CHATBOTBIOSIMPLE_H
