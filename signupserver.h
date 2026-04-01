#ifndef SIGNUPSERVER_H
#define SIGNUPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QUrl>

class SignupServer : public QObject
{
    Q_OBJECT
public:
    explicit SignupServer(QObject* parent = nullptr);
    bool start(quint16 preferredPort = 8088);
    QUrl signupUrl() const;

private:
    QByteArray signupHtml() const;
    void handleHttpRequest(const QByteArray& requestData, QTcpSocket* socket);
    void sendHttpResponse(QTcpSocket* socket, int statusCode, const QByteArray& body, const QByteArray& contentType);
    void sendJson(QTcpSocket* socket, int statusCode, const QString& message, bool ok);

    QTcpServer m_server;
};

#endif // SIGNUPSERVER_H
