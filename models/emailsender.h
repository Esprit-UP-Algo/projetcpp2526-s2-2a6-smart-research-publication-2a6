#ifndef EMAILSENDER_H
#define EMAILSENDER_H

#include <QObject>
#include <QQueue>
#include <QString>
#include <QStringList>
#include <QSslSocket>
#include <QTimer>

// ─────────────────────────────────────────────────────────────────────────────
//  EmailSender  –  async SMTP/STARTTLS email sender (Gmail-compatible)
//
//  Usage:
//      EmailSender::instance()->configure("smtp.gmail.com", 587,
//                                         "app@gmail.com", "app-password");
//      EmailSender::instance()->send("dest@example.com",
//                                    "Subject", "<b>HTML body</b>");
//
//  Connect signals  sent() / failed()  to react to results.
// ─────────────────────────────────────────────────────────────────────────────
class EmailSender : public QObject
{
    Q_OBJECT
public:
    static EmailSender* instance();

    // Call once at startup (or after login) to set SMTP credentials
    void configure(const QString& smtpHost,
                   int            smtpPort,
                   const QString& username,
                   const QString& password,
                   const QString& senderDisplayName = "SmartVision BioSimple");

    bool isConfigured() const
    {
        return !m_smtpHost.isEmpty()
               && m_smtpPort > 0
               && !m_username.isEmpty()
               && !m_password.isEmpty();
    }

    // Enqueue an email; delivers asynchronously
    void send(const QString& to,
              const QString& subject,
              const QString& htmlBody);

signals:
    void sent(const QString& to);
    void failed(const QString& to, const QString& reason);

private slots:
    void onConnected();
    void onReadyRead();
    void onSslErrors(const QList<QSslError>& errors);
    void onSocketError(QAbstractSocket::SocketError err);
    void onTimeout();

private:
    struct PendingEmail {
        QString to;
        QString subject;
        QString htmlBody;
    };

    explicit EmailSender(QObject* parent = nullptr);

    void startNextSend();
    void sendLine(const QString& cmd);
    void processLine(const QString& line);
    void cleanup(bool success, const QString& reason = QString());

    // ── state machine ──────────────────────────────────────────────────────
    enum State {
        Idle,
        Connecting,
        WaitGreeting,
        WaitEhlo1,
        WaitStartTls,
        WaitEhlo2,
        WaitAuthLogin,
        WaitUsername,
        WaitPassword,
        WaitMailFrom,
        WaitRcptTo,
        WaitData,
        WaitBody,
        WaitQuit,
        Done
    };
    State m_state = Idle;

    // ── pending message ────────────────────────────────────────────────────
    QString m_pendingTo;
    QString m_pendingSubject;
    QString m_pendingBody;

    // ── config ─────────────────────────────────────────────────────────────
    QString m_smtpHost;
    int     m_smtpPort   = 587;
    QString m_username;
    QString m_password;
    QString m_senderName;

    // ── runtime ────────────────────────────────────────────────────────────
    QSslSocket* m_sock    = nullptr;
    QTimer*     m_timer   = nullptr;
    QString     m_currentTo;
    QQueue<PendingEmail> m_queue;

    static EmailSender* s_instance;
};

#endif // EMAILSENDER_H
