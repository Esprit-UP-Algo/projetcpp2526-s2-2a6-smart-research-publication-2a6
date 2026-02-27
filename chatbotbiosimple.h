#ifndef CHATBOTBIOSIMPLE_H
#define CHATBOTBIOSIMPLE_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QWidget>
#include <QFrame>
#include <QScrollBar>
#include <QNetworkAccessManager>
#include <QNetworkReply>

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
    void animateBg();
    void onApiReply(QNetworkReply* reply);

private:
    void    addMessage(const QString& text, bool isUser, bool richText = false);
    void    addTypingIndicator();
    void    removeTypingIndicator();
    void    callOpenAI(const QString& userMessage);
    QString formatResponse(const QString& text);   // **bold** + bullet lists → HTML

    QWidget*     m_msgContainer;
    QVBoxLayout* m_msgLayout;
    QScrollArea* m_scroll;
    QLineEdit*   m_input;
    QPushButton* m_sendBtn;
    QPushButton* m_clearBtn;

    QNetworkAccessManager* m_net;
    QWidget*               m_typingWidget = nullptr;

    // Conversation history for context
    QList<QPair<QString,QString>> m_history; // role, content
    QString m_lastUserMsg;

    // Background animation
    QTimer* m_bgTimer;
    float   m_bgAngle = 0.0f;

    // Drag
    QPoint m_dragPos;
    bool   m_dragging = false;
};

#endif // CHATBOTBIOSIMPLE_H
