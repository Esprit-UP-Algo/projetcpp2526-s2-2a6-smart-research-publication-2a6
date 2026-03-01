#include "signupserver.h"

#include <QHostAddress>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

SignupServer::SignupServer(QObject* parent)
    : QObject(parent)
{
    connect(&m_server, &QTcpServer::newConnection, this, [this]() {
        while (m_server.hasPendingConnections()) {
            QTcpSocket* socket = m_server.nextPendingConnection();
            if (!socket) {
                continue;
            }

            auto* buffer = new QByteArray;
            connect(socket, &QTcpSocket::readyRead, socket, [this, socket, buffer]() {
                buffer->append(socket->readAll());

                const int headerEnd = buffer->indexOf("\r\n\r\n");
                if (headerEnd < 0) {
                    return;
                }

                int contentLength = 0;
                const QList<QByteArray> lines = buffer->left(headerEnd).split('\n');
                for (QByteArray line : lines) {
                    line = line.trimmed();
                    if (line.toLower().startsWith("content-length:")) {
                        contentLength = line.mid(QByteArray("content-length:").size()).trimmed().toInt();
                        break;
                    }
                }

                const int bodySize = buffer->size() - (headerEnd + 4);
                if (bodySize < contentLength) {
                    return;
                }

                handleHttpRequest(*buffer, socket);
            });

            connect(socket, &QTcpSocket::disconnected, socket, [buffer, socket]() {
                delete buffer;
                socket->deleteLater();
            });
        }
    });
}

bool SignupServer::start(quint16 preferredPort)
{
    if (m_server.isListening()) {
        return true;
    }

    if (m_server.listen(QHostAddress::LocalHost, preferredPort)) {
        return true;
    }

    return m_server.listen(QHostAddress::LocalHost, 0);
}

QUrl SignupServer::signupUrl() const
{
    return QUrl(QString("http://127.0.0.1:%1/signup").arg(m_server.serverPort()));
}

QByteArray SignupServer::signupHtml() const
{
    static const char* html = R"HTML(
<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>SmartVision - Créer un compte</title>
    <style>
        :root {
            --topbar: #12443B;
            --primary: #0A5F58;
            --bg: #A3CAD3;
            --beige: #C6B29A;
            --text: #64533A;
        }

        * { box-sizing: border-box; }

        body {
            margin: 0;
            min-height: 100vh;
            font-family: Inter, Segoe UI, Arial, sans-serif;
            color: var(--text);
            background: radial-gradient(circle at 15% 20%, rgba(198,178,154,0.32), transparent 40%),
                        radial-gradient(circle at 85% 30%, rgba(18,68,59,0.24), transparent 45%),
                        linear-gradient(180deg, rgba(163,202,211,0.95), rgba(163,202,211,1));
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 24px;
        }

        .card {
            width: 100%;
            max-width: 700px;
            background: rgba(246,248,247,0.9);
            border: 1px solid rgba(0,0,0,0.1);
            border-radius: 18px;
            box-shadow: 0 14px 36px rgba(0,0,0,0.18);
            padding: 28px;
        }

        h1 {
            margin: 0;
            color: var(--topbar);
            font-size: 30px;
        }

        p {
            margin: 10px 0 20px;
            color: rgba(100,83,58,0.86);
        }

        .grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 12px;
        }

        .full { grid-column: 1 / -1; }

        label {
            display: block;
            font-size: 12px;
            margin-bottom: 6px;
            color: rgba(100,83,58,0.86);
            font-weight: 600;
        }

        input, select {
            width: 100%;
            border-radius: 10px;
            border: 1px solid rgba(18,68,59,0.25);
            background: rgba(255,255,255,0.92);
            padding: 10px 12px;
            font-size: 14px;
            color: var(--text);
            outline: none;
        }

        input:focus, select:focus {
            border-color: var(--primary);
            box-shadow: 0 0 0 2px rgba(10,95,88,0.12);
        }

        button {
            width: 100%;
            border: none;
            border-radius: 12px;
            height: 46px;
            margin-top: 16px;
            color: #fff;
            background: var(--primary);
            font-size: 15px;
            font-weight: 700;
            cursor: pointer;
        }

        button:hover { background: var(--topbar); }

        .status {
            margin-top: 14px;
            padding: 10px 12px;
            border-radius: 10px;
            display: none;
            font-size: 13px;
        }

        .ok {
            display: block;
            background: rgba(10,95,88,0.15);
            border: 1px solid rgba(10,95,88,0.35);
            color: #0b4f4a;
        }

        .err {
            display: block;
            background: rgba(177,74,74,0.15);
            border: 1px solid rgba(177,74,74,0.35);
            color: #7a1f2f;
        }

        @media (max-width: 760px) {
            .grid { grid-template-columns: 1fr; }
        }
    </style>
</head>
<body>
    <div class="card">
        <h1>Créer un compte</h1>
        <p>Inscription SmartVision (données employé + accès applicatif).</p>

        <form id="signupForm">
            <div class="grid">
                <div>
                    <label for="cin">CIN</label>
                    <input id="cin" required maxlength="20" />
                </div>
                <div>
                    <label for="role">Rôle</label>
                    <select id="role" required>
                        <option value="Chercheur">Chercheur</option>
                        <option value="Technicien">Technicien</option>
                        <option value="Responsable">Responsable</option>
                    </select>
                </div>

                <div>
                    <label for="nom">Nom</label>
                    <input id="nom" required maxlength="80" />
                </div>
                <div>
                    <label for="prenom">Prénom</label>
                    <input id="prenom" required maxlength="80" />
                </div>

                <div class="full">
                    <label for="specialisation">Spécialisation</label>
                    <input id="specialisation" maxlength="120" />
                </div>

                <div class="full">
                    <label for="email">Adresse e-mail</label>
                    <input id="email" type="email" required maxlength="120" />
                </div>
                <div class="full">
                    <label for="password">Mot de passe</label>
                    <input id="password" type="password" required maxlength="255" />
                </div>
            </div>

            <button type="submit">Créer le compte</button>
            <div id="status" class="status"></div>
        </form>
    </div>

<script>
const form = document.getElementById('signupForm');
const statusBox = document.getElementById('status');

function setStatus(ok, text) {
    statusBox.className = 'status ' + (ok ? 'ok' : 'err');
    statusBox.textContent = text;
}

form.addEventListener('submit', async (e) => {
    e.preventDefault();

    const payload = {
        cin: document.getElementById('cin').value.trim(),
        nom: document.getElementById('nom').value.trim(),
        prenom: document.getElementById('prenom').value.trim(),
        role: document.getElementById('role').value,
        specialisation: document.getElementById('specialisation').value.trim(),
        email: document.getElementById('email').value.trim(),
        password: document.getElementById('password').value
    };

    try {
        const res = await fetch('/api/signup', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(payload)
        });
        const data = await res.json();

        if (!res.ok || !data.ok) {
            setStatus(false, data.message || 'Échec de création du compte.');
            return;
        }

        setStatus(true, data.message || 'Compte créé avec succès.');
        form.reset();
    } catch (err) {
        setStatus(false, 'Le service d\'inscription local est indisponible.');
    }
});
</script>
</body>
</html>
)HTML";

    return QByteArray(html);
}

void SignupServer::sendHttpResponse(QTcpSocket* socket, int statusCode, const QByteArray& body, const QByteArray& contentType)
{
    QByteArray statusText = "OK";
    if (statusCode == 201) statusText = "Created";
    if (statusCode == 400) statusText = "Bad Request";
    if (statusCode == 404) statusText = "Not Found";
    if (statusCode == 405) statusText = "Method Not Allowed";
    if (statusCode == 500) statusText = "Internal Server Error";

    QByteArray response;
    response += "HTTP/1.1 " + QByteArray::number(statusCode) + " " + statusText + "\r\n";
    response += "Content-Type: " + contentType + "; charset=utf-8\r\n";
    response += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    response += "Connection: close\r\n";
    response += "Access-Control-Allow-Origin: *\r\n";
    response += "\r\n";
    response += body;

    socket->write(response);
    socket->disconnectFromHost();
}

void SignupServer::sendJson(QTcpSocket* socket, int statusCode, const QString& message, bool ok)
{
    QJsonObject obj;
    obj["ok"] = ok;
    obj["message"] = message;

    sendHttpResponse(socket,
                     statusCode,
                     QJsonDocument(obj).toJson(QJsonDocument::Compact),
                     "application/json");
}

void SignupServer::handleHttpRequest(const QByteArray& requestData, QTcpSocket* socket)
{
    const int headerEnd = requestData.indexOf("\r\n\r\n");
    if (headerEnd < 0) {
        sendHttpResponse(socket, 400, "Invalid HTTP request", "text/plain");
        return;
    }

    const QByteArray head = requestData.left(headerEnd);
    const QByteArray body = requestData.mid(headerEnd + 4);
    const QList<QByteArray> headLines = head.split('\n');
    if (headLines.isEmpty()) {
        sendHttpResponse(socket, 400, "Invalid HTTP request", "text/plain");
        return;
    }

    const QList<QByteArray> requestLine = headLines.first().trimmed().split(' ');
    if (requestLine.size() < 2) {
        sendHttpResponse(socket, 400, "Invalid HTTP request line", "text/plain");
        return;
    }

    const QByteArray method = requestLine.at(0).trimmed().toUpper();
    const QByteArray path = requestLine.at(1).trimmed();

    if (method == "GET" && (path == "/" || path == "/signup")) {
        sendHttpResponse(socket, 200, signupHtml(), "text/html");
        return;
    }

    if (method != "POST" || path != "/api/signup") {
        sendHttpResponse(socket, 404, "Not found", "text/plain");
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        sendJson(socket, 400, "Payload JSON invalide.", false);
        return;
    }

    const QJsonObject obj = doc.object();
    const QString cin = obj.value("cin").toString().trimmed();
    const QString nom = obj.value("nom").toString().trimmed();
    const QString prenom = obj.value("prenom").toString().trimmed();
    const QString role = obj.value("role").toString().trimmed();
    const QString specialisation = obj.value("specialisation").toString().trimmed();
    const QString email = obj.value("email").toString().trimmed();
    const QString password = obj.value("password").toString();

    if (cin.isEmpty() || nom.isEmpty() || prenom.isEmpty() || role.isEmpty() || email.isEmpty() || password.isEmpty()) {
        sendJson(socket, 400, "Tous les champs obligatoires doivent être remplis.", false);
        return;
    }

    if (role != "Chercheur" && role != "Technicien" && role != "Responsable") {
        sendJson(socket, 400, "Rôle invalide.", false);
        return;
    }

    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        sendJson(socket, 500, "La connexion à la base de données est indisponible.", false);
        return;
    }

    if (!db.transaction()) {
        sendJson(socket, 500, "Impossible de démarrer la transaction SQL.", false);
        return;
    }

    // Generate next EMPLOYEE_ID
    QSqlQuery idQ(db);
    if (!idQ.exec("SELECT NVL(MAX(EMPLOYEE_ID),0)+1 FROM EMPLOYES") || !idQ.next()) {
        db.rollback();
        sendJson(socket, 500, "Impossible de générer l'ID employé : " + idQ.lastError().text(), false);
        return;
    }
    int nextEmpId = idQ.value(0).toInt();

    QSqlQuery query(db);
    query.prepare("INSERT INTO EMPLOYES (EMPLOYEE_ID, CIN, NOM, PRENOM, ROLE, SPECIALIZATION) "
                  "VALUES (?, ?, ?, ?, ?, ?)");
    query.addBindValue(nextEmpId);
    query.addBindValue(cin);
    query.addBindValue(nom);
    query.addBindValue(prenom);
    query.addBindValue(role);
    query.addBindValue(specialisation.isEmpty() ? QVariant(QMetaType::fromType<QString>()) : QVariant(specialisation));

    if (!query.exec()) {
        db.rollback();
        sendJson(socket, 400, "Insertion employé échouée : " + query.lastError().text(), false);
        return;
    }

    query.prepare("INSERT INTO APP_USERS (EMAIL, USER_PASSWORD, FULL_NAME, ROLE, ACTIVE) "
                  "VALUES (?, ?, ?, ?, 'O')");
    query.addBindValue(email);
    query.addBindValue(password);
    query.addBindValue(nom + " " + prenom);
    query.addBindValue(role);

    if (!query.exec()) {
        db.rollback();
        sendJson(socket, 400, "Création du compte applicatif échouée : " + query.lastError().text(), false);
        return;
    }

    if (!db.commit()) {
        db.rollback();
        sendJson(socket, 500, "Commit SQL échoué.", false);
        return;
    }

    sendJson(socket, 201, "Compte créé avec succès. Vous pouvez vous connecter dans l'application.", true);
}
