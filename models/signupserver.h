#ifndef SIGNUPSERVER_H
#define SIGNUPSERVER_H

#include <functional>
#include <QObject>
#include <QTcpServer>
#include <QUrl>

class QNetworkAccessManager;
class QTcpSocket;

class SignupServer : public QObject
{
public:
    explicit SignupServer(QObject* parent = nullptr);
    bool start(quint16 preferredPort = 8088);
    QUrl signupUrl() const;

    // ── Google OAuth natif (sans PHP) ──────────────────────────────
    void    setGoogleCredentials(const QString& clientId, const QString& clientSecret);
    QString googleCallbackUrl() const;   // http://127.0.0.1:<port>/google-callback
    void setFaceLoginSucceededHandler(std::function<void(const QString&)> handler);
    void setGoogleLoginSucceededHandler(std::function<void(const QString&)> handler);

private:
    QByteArray signupHtml() const;
    void handleHttpRequest(const QByteArray& requestData, QTcpSocket* socket);
    void sendHttpResponse(QTcpSocket* socket, int statusCode, const QByteArray& body, const QByteArray& contentType);
    void sendJson(QTcpSocket* socket, int statusCode, const QString& message, bool ok);

    // Exchange the OAuth authorization code for tokens via Google's token endpoint
    void exchangeGoogleCode(const QString& code);

    QTcpServer             m_server;
    QNetworkAccessManager* m_net            = nullptr;
    QString                m_googleClientId;
    QString                m_googleClientSecret;
    std::function<void(const QString&)> m_onFaceLoginSucceeded;
    std::function<void(const QString&)> m_onGoogleLoginSucceeded;
};

#endif // SIGNUPSERVER_H
