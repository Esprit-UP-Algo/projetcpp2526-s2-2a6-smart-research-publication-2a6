#include "emailsender.h"
#include <QCoreApplication>
#include <QByteArray>
#include <QDateTime>
#include <QFile>
#include <QSslConfiguration>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>

namespace {

QString encodeMimeHeaderUtf8(const QString& text)
{
    if (text.isEmpty()) return QString();
    return "=?UTF-8?B?" + QString::fromLatin1(text.toUtf8().toBase64()) + "?=";
}

QByteArray buildHtmlMessage(const QString& senderName,
                            const QString& senderEmail,
                            const QString& recipientEmail,
                            const QString& subject,
                            const QString& htmlBody)
{
    QByteArray msg;
    msg += "From: " + encodeMimeHeaderUtf8(senderName).toUtf8() + " <" + senderEmail.toUtf8() + ">\r\n";
    msg += "To: <" + recipientEmail.toUtf8() + ">\r\n";
    msg += "Subject: " + encodeMimeHeaderUtf8(subject).toUtf8() + "\r\n";
    msg += "MIME-Version: 1.0\r\n";
    msg += "Content-Type: text/html; charset=UTF-8\r\n";
    msg += "Content-Transfer-Encoding: 8bit\r\n";
    msg += "\r\n";
    msg += htmlBody.toUtf8();
    msg += "\r\n.\r\n";
    return msg;
}

QString smtpLogPath()
{
    const QString currentPath = QDir::current().filePath("smtp_mail.log");
    const QFileInfo currentInfo(currentPath);
    if (currentInfo.dir().exists()) return currentPath;
    return QCoreApplication::applicationDirPath() + "/smtp_mail.log";
}

void appendSmtpLog(const QString& message)
{
    QFile file(smtpLogPath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        return;

    QTextStream out(&file);
    out << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")
        << " " << message << "\n";
}

QString maskSmtpIdentity(const QString& value)
{
    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty()) return QString("(vide)");
    const int atPos = trimmed.indexOf('@');
    if (atPos <= 1) return QString("***%1").arg(trimmed.mid(atPos));
    return trimmed.left(2) + "***" + trimmed.mid(atPos);
}

} // namespace

// ── singleton ─────────────────────────────────────────────────────────────────
EmailSender* EmailSender::s_instance = nullptr;

EmailSender* EmailSender::instance()
{
    if (!s_instance)
        s_instance = new EmailSender(QCoreApplication::instance());
    return s_instance;
}

EmailSender::EmailSender(QObject* parent) : QObject(parent)
{
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    m_timer->setInterval(15000);   // 15 s timeout per SMTP step
    connect(m_timer, &QTimer::timeout, this, &EmailSender::onTimeout);
}

// ── configure ─────────────────────────────────────────────────────────────────
void EmailSender::configure(const QString& smtpHost,
                             int            smtpPort,
                             const QString& username,
                             const QString& password,
                             const QString& senderDisplayName)
{
    m_smtpHost   = smtpHost;
    m_smtpPort   = smtpPort;
    m_username   = username;
    m_password   = password;
    m_senderName = senderDisplayName;
    appendSmtpLog(QString("[SMTP] configure host=%1 port=%2 user=%3 configured=%4")
                      .arg(m_smtpHost,
                           QString::number(m_smtpPort),
                           maskSmtpIdentity(m_username),
                           isConfigured() ? "yes" : "no"));
}

// ── public send ───────────────────────────────────────────────────────────────
void EmailSender::send(const QString& to,
                       const QString& subject,
                       const QString& htmlBody)
{
    if (!isConfigured()) {
        emit failed(to, "EmailSender not configured (call configure() first).");
        return;
    }
    const QString trimmedTo = to.trimmed();
    if (trimmedTo.isEmpty()) {
        emit failed(to, "Recipient email is empty.");
        return;
    }

    m_queue.enqueue(PendingEmail{trimmedTo, subject, htmlBody});
    if (m_state == Idle)
        startNextSend();
}

void EmailSender::startNextSend()
{
    if (m_state != Idle || m_queue.isEmpty())
        return;

    const PendingEmail next = m_queue.dequeue();
    m_pendingTo      = next.to;
    m_pendingSubject = next.subject;
    m_pendingBody    = next.htmlBody;
    m_currentTo      = next.to;

    // Create a fresh socket each time
    if (m_sock) {
        m_sock->deleteLater();
        m_sock = nullptr;
    }
    m_sock = new QSslSocket(this);

    connect(m_sock, &QSslSocket::connected,
            this,   &EmailSender::onConnected);
    connect(m_sock, &QSslSocket::readyRead,
            this,   &EmailSender::onReadyRead);
    connect(m_sock,
            QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors),
            this, &EmailSender::onSslErrors);
    connect(m_sock,
            QOverload<QAbstractSocket::SocketError>::of(&QSslSocket::errorOccurred),
            this, &EmailSender::onSocketError);

    m_state = Connecting;
    m_timer->start();
    appendSmtpLog(QString("[SMTP] connecting host=%1 port=%2 to=%3")
                      .arg(m_smtpHost,
                           QString::number(m_smtpPort),
                           maskSmtpIdentity(m_currentTo)));
    if (m_smtpPort == 465) {
        m_sock->connectToHostEncrypted(m_smtpHost, static_cast<quint16>(m_smtpPort));
    } else {
        m_sock->connectToHost(m_smtpHost, static_cast<quint16>(m_smtpPort));
    }
}

// ── slot: TCP connected ───────────────────────────────────────────────────────
void EmailSender::onConnected()
{
    m_timer->start();          // reset timeout
    appendSmtpLog(QString("[SMTP] connected host=%1 port=%2 mode=%3")
                      .arg(m_smtpHost,
                           QString::number(m_smtpPort),
                           m_smtpPort == 465 ? "ssl" : "starttls"));
    m_state = WaitGreeting;
    // Server sends greeting automatically; wait in onReadyRead
}

// ── slot: data available ──────────────────────────────────────────────────────
void EmailSender::onReadyRead()
{
    m_timer->start();   // reset timeout on every received line
    while (m_sock && m_sock->canReadLine()) {
        const QString line = QString::fromUtf8(m_sock->readLine()).trimmed();
        qDebug() << "[SMTP <]" << line;
        appendSmtpLog(QString("[SMTP <] %1").arg(line));
        processLine(line);
    }
}

// ── state machine ─────────────────────────────────────────────────────────────
void EmailSender::processLine(const QString& line)
{
    // Extract 3-digit SMTP code
    const int code = line.left(3).toInt();
    const QString smtpError = line.isEmpty() ? QString("SMTP session failed.") : QString("SMTP error: %1").arg(line);

    switch (m_state) {

    case WaitGreeting:
        if (code == 220) {
            m_state = (m_smtpPort == 465) ? WaitEhlo2 : WaitEhlo1;
            sendLine("EHLO smartvision.local");
        } else {
            cleanup(false, smtpError);
        }
        break;

    case WaitEhlo1:
        // Server may send multi-line EHLO response; wait for final line (no dash after code)
        if (line.length() > 3 && line[3] == '-') break;   // continuation
        if (code == 250) {
            m_state = WaitStartTls;
            sendLine("STARTTLS");
        } else {
            cleanup(false, smtpError);
        }
        break;

    case WaitStartTls:
        if (code == 220) {
            // Upgrade to TLS
            QSslConfiguration cfg = QSslConfiguration::defaultConfiguration();
            cfg.setProtocol(QSsl::TlsV1_2OrLater);
            m_sock->setSslConfiguration(cfg);
            m_sock->ignoreSslErrors();          // accept self-signed for simplicity
            connect(m_sock, &QSslSocket::encrypted, this, [this](){
                appendSmtpLog("[SMTP] TLS handshake completed");
                sendLine("EHLO smartvision.local");
            });
            // After TLS handshake, socket emits encrypted() — but we can continue
            // because Qt buffers; send EHLO again once encrypted
            m_state = WaitEhlo2;
            m_sock->startClientEncryption();
        } else {
            cleanup(false, smtpError);
        }
        break;

    case WaitEhlo2:
        if (line.length() > 3 && line[3] == '-') break;   // continuation
        if (code == 250) {
            m_state = WaitAuthLogin;
            sendLine("AUTH LOGIN");
        } else {
            cleanup(false, smtpError);
        }
        break;

    case WaitAuthLogin:
        if (code == 334) {
            m_state = WaitUsername;
            sendLine(m_username.toUtf8().toBase64());
        } else {
            cleanup(false, smtpError);
        }
        break;

    case WaitUsername:
        if (code == 334) {
            m_state = WaitPassword;
            sendLine(m_password.toUtf8().toBase64());
        } else {
            cleanup(false, smtpError);
        }
        break;

    case WaitPassword:
        if (code == 235) {
            m_state = WaitMailFrom;
            sendLine(QString("MAIL FROM:<%1>").arg(m_username));
        } else {
            cleanup(false, smtpError);
        }
        break;

    case WaitMailFrom:
        if (code == 250) {
            m_state = WaitRcptTo;
            sendLine(QString("RCPT TO:<%1>").arg(m_pendingTo));
        } else {
            cleanup(false, smtpError);
        }
        break;

    case WaitRcptTo:
        if (code == 250) {
            m_state = WaitData;
            sendLine("DATA");
        } else {
            cleanup(false, smtpError);
        }
        break;

    case WaitData:
        if (code == 354) {
            m_state = WaitBody;
            const QByteArray msg = buildHtmlMessage(
                m_senderName,
                m_username,
                m_pendingTo,
                m_pendingSubject,
                m_pendingBody);
            if (m_sock) m_sock->write(msg);
        } else {
            cleanup(false, smtpError);
        }
        break;

    case WaitBody:
        if (code == 250) {
            m_state = WaitQuit;
            sendLine("QUIT");
        } else {
            cleanup(false, smtpError);
        }
        break;

    case WaitQuit:
        cleanup(code == 221 || code == 250, smtpError);
        break;

    default:
        break;
    }
}

// ── helpers ───────────────────────────────────────────────────────────────────
void EmailSender::sendLine(const QString& cmd)
{
    qDebug() << "[SMTP >]" << cmd;
    appendSmtpLog(QString("[SMTP >] %1").arg(cmd));
    if (m_sock) m_sock->write((cmd + "\r\n").toUtf8());
}

void EmailSender::cleanup(bool success, const QString& reason)
{
    m_timer->stop();
    m_state = Idle;
    const QString to = m_currentTo;
    m_currentTo.clear();
    m_pendingTo.clear();
    m_pendingSubject.clear();
    m_pendingBody.clear();
    if (m_sock) {
        m_sock->disconnectFromHost();
        m_sock->deleteLater();
        m_sock = nullptr;
    }
    if (success)
        emit sent(to);
    else
        emit failed(to, reason.isEmpty() ? QString("SMTP session failed.") : reason);

    appendSmtpLog(QString("[SMTP] %1 to=%2 reason=%3")
                      .arg(success ? "success" : "failure",
                           maskSmtpIdentity(to),
                           success ? QString("-") : reason));

    if (!m_queue.isEmpty())
        QMetaObject::invokeMethod(this, [this]() { startNextSend(); }, Qt::QueuedConnection);
}

// ── slot: SSL errors (ignored for self-signed) ────────────────────────────────
void EmailSender::onSslErrors(const QList<QSslError>& errors)
{
    for (const QSslError& e : errors)
        qWarning() << "[SMTP SSL]" << e.errorString();
    for (const QSslError& e : errors)
        appendSmtpLog(QString("[SMTP SSL] %1").arg(e.errorString()));
    if (m_sock) m_sock->ignoreSslErrors();
}

// ── slot: TCP error ───────────────────────────────────────────────────────────
void EmailSender::onSocketError(QAbstractSocket::SocketError)
{
    const QString err = m_sock ? m_sock->errorString() : "unknown error";
    qWarning() << "[SMTP] socket error:" << err;
    appendSmtpLog(QString("[SMTP] socket error=%1").arg(err));
    cleanup(false, err);
}

// ── slot: timeout ─────────────────────────────────────────────────────────────
void EmailSender::onTimeout()
{
    qWarning() << "[SMTP] timeout in state" << m_state;
    appendSmtpLog(QString("[SMTP] timeout state=%1").arg(m_state));
    cleanup(false, QString("SMTP timeout in state %1").arg(m_state));
}
