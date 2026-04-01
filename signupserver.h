#ifndef SIGNUPSERVER_H
#define SIGNUPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class SignupServer : public QObject
{
    Q_OBJECT
public:
    explicit SignupServer(QObject* parent = nullptr);
    bool start(quint16 preferredPort = 8088);
    QUrl signupUrl() const;

    // ── Google OAuth natif (sans PHP) ──────────────────────────────
    void    setGoogleCredentials(const QString& clientId, const QString& clientSecret);
    QString googleCallbackUrl() const;   // http://127.0.0.1:<port>/google-callback

signals:
    void faceLoginSucceeded(const QString& identity);
    void googleLoginSucceeded(const QString& identity);

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
};

#endif // SIGNUPSERVER_H
