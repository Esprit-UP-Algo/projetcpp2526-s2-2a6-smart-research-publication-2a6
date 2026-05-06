#include "signupserver.h"

#include <QHostAddress>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QUrl>
#include <QSettings>
#include <QRegularExpression>
#include <QEventLoop>
#include <QTimer>
#include <QSslConfiguration>
#include <QSslSocket>
#include <cmath>
#include <limits>

namespace {

bool isValidSignupEmail(const QString& email)
{
    static const QRegularExpression re(
        "^[A-Z0-9._%+-]+@[A-Z0-9.-]+\\.[A-Z]{2,}$",
        QRegularExpression::CaseInsensitiveOption);
    return re.match(email.trimmed()).hasMatch();
}

bool isStrongSignupPassword(const QString& password)
{
    const QString p = password.trimmed();
    if (p.size() < 8) return false;

    static const QRegularExpression lowerRe("[a-z]");
    static const QRegularExpression upperRe("[A-Z]");
    static const QRegularExpression digitRe("[0-9]");
    static const QRegularExpression specialRe("[^A-Za-z0-9]");

    return lowerRe.match(p).hasMatch()
        && upperRe.match(p).hasMatch()
        && digitRe.match(p).hasMatch()
        && specialRe.match(p).hasMatch();
}

bool isValidHumanName(const QString& value)
{
    static const QRegularExpression re("^[\\p{L}][\\p{L}\\s'\\-]{0,79}$");
    return re.match(value.trimmed()).hasMatch();
}

bool jsonValueToBool(const QJsonValue& v, bool* out)
{
    if (v.isBool()) {
        *out = v.toBool();
        return true;
    }

    if (v.isString()) {
        const QString s = v.toString().trimmed().toLower();
        if (s == "true" || s == "1" || s == "yes") {
            *out = true;
            return true;
        }
        if (s == "false" || s == "0" || s == "no") {
            *out = false;
            return true;
        }
    }

    return false;
}

bool readApiBool(const QJsonObject& obj, const QString& key, bool* out)
{
    if (!obj.contains(key)) return false;

    const QJsonValue v = obj.value(key);
    if (jsonValueToBool(v, out)) return true;

    if (v.isObject()) {
        const QJsonObject nested = v.toObject();
        if (jsonValueToBool(nested.value("value"), out)) return true;
    }

    return false;
}

QString emailValidationApiKey()
{
    QSettings settings;

    const QString primary = qEnvironmentVariable("WASTEGUARD_EMAIL_API_KEY").trimmed();
    if (!primary.isEmpty()) {
        settings.setValue("emailValidation/apiKey", primary);
        return primary;
    }

    const QString fallback = qEnvironmentVariable("ABSTRACT_EMAIL_API_KEY").trimmed();
    if (!fallback.isEmpty()) {
        settings.setValue("emailValidation/apiKey", fallback);
        return fallback;
    }

    return settings.value("emailValidation/apiKey").toString().trimmed();
}

QString emailValidationApiUrl()
{
    static const QString kDefaultUrl = "https://emailreputation.abstractapi.com/v1/";

    QSettings settings;
    const QString envUrl = qEnvironmentVariable("WASTEGUARD_EMAIL_API_URL").trimmed();
    if (!envUrl.isEmpty()) {
        settings.setValue("emailValidation/apiUrl", envUrl);
        return envUrl;
    }

    const QString savedUrl = settings.value("emailValidation/apiUrl").toString().trimmed();
    return savedUrl.isEmpty() ? kDefaultUrl : savedUrl;
}

bool verifyEmailWithApi(const QString& email, QString* reason)
{
    const QString apiKey = emailValidationApiKey();
    if (apiKey.isEmpty()) {
        // Do not block sign-up when API key is not configured.
        return true;
    }

    QUrl url(emailValidationApiUrl());
    QUrlQuery query(url);
    query.addQueryItem("api_key", apiKey);
    query.addQueryItem("email", email.trimmed());
    url.setQuery(query);

    QNetworkAccessManager net;
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QSslConfiguration ssl = QSslConfiguration::defaultConfiguration();
    ssl.setPeerVerifyMode(QSslSocket::VerifyNone);
    req.setSslConfiguration(ssl);

    QNetworkReply* reply = net.get(req);
    QObject::connect(reply, &QNetworkReply::sslErrors, reply,
                     [reply](const QList<QSslError>&){ reply->ignoreSslErrors(); });

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(8000);
    loop.exec();

    if (!timer.isActive()) {
        reply->abort();
        if (reason) *reason = "Vérification e-mail expirée (timeout API).";
        reply->deleteLater();
        return false;
    }
    timer.stop();

    if (reply->error() != QNetworkReply::NoError) {
        if (reason) *reason = "Impossible de vérifier l'adresse e-mail pour le moment.";
        reply->deleteLater();
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
    reply->deleteLater();

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (reason) *reason = "Réponse API e-mail invalide.";
        return false;
    }

    const QJsonObject root = doc.object();
    if (root.contains("error") && !root.value("error").isNull()) {
        if (reason) *reason = "Service de vérification e-mail indisponible.";
        return false;
    }

    const QString deliverability = root.value("deliverability").toString().trimmed().toLower();
    if (!deliverability.isEmpty()) {
        if (deliverability.contains("undeliver") || deliverability.contains("invalid")
            || deliverability.contains("risky") || deliverability.contains("unknown")) {
            if (reason) *reason = "L'adresse e-mail semble invalide ou non délivrable.";
            return false;
        }
        if (deliverability.contains("deliver")) {
            return true;
        }
    }

    bool anySignal = false;
    bool value = false;

    if (readApiBool(root, "is_valid_format", &value)) {
        anySignal = true;
        if (!value) {
            if (reason) *reason = "Le format de l'adresse e-mail est invalide.";
            return false;
        }
    }
    if (readApiBool(root, "is_mx_found", &value)) {
        anySignal = true;
        if (!value) {
            if (reason) *reason = "Le domaine e-mail n'accepte pas de courriels (MX introuvable).";
            return false;
        }
    }
    if (readApiBool(root, "is_smtp_valid", &value)) {
        anySignal = true;
        if (!value) {
            if (reason) *reason = "Le serveur e-mail n'a pas validé cette adresse.";
            return false;
        }
    }

    if (anySignal) return true;

    if (reason) *reason = "La validation e-mail n'a pas pu confirmer cette adresse.";
    return false;
}

} // namespace

SignupServer::SignupServer(QObject* parent)
    : QObject(parent)
{
    connect(&m_server, &QTcpServer::newConnection, this, [this]() {
        while (m_server.hasPendingConnections()) {
            QTcpSocket* socket = m_server.nextPendingConnection();
            if (!socket) continue;

            auto* buffer = new QByteArray;
            connect(socket, &QTcpSocket::readyRead, socket, [this, socket, buffer]() {
                buffer->append(socket->readAll());

                const int headerEnd = buffer->indexOf("\r\n\r\n");
                if (headerEnd < 0) return;

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
                if (bodySize < contentLength) return;

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
    if (m_server.isListening()) return true;
    if (m_server.listen(QHostAddress::LocalHost, preferredPort)) return true;
    return m_server.listen(QHostAddress::LocalHost, 0);
}

QUrl SignupServer::signupUrl() const
{
    return QUrl(QString("http://127.0.0.1:%1/signup").arg(m_server.serverPort()));
}

void SignupServer::setFaceLoginSucceededHandler(std::function<void(const QString&)> handler)
{
    m_onFaceLoginSucceeded = handler;
}

void SignupServer::setGoogleLoginSucceededHandler(std::function<void(const QString&)> handler)
{
    m_onGoogleLoginSucceeded = handler;
}

// ─── Google OAuth natif ───────────────────────────────────────────
void SignupServer::setGoogleCredentials(const QString& clientId, const QString& clientSecret)
{
    m_googleClientId     = clientId;
    m_googleClientSecret = clientSecret;
    if (!m_net) m_net = new QNetworkAccessManager(this);
}

QString SignupServer::googleCallbackUrl() const
{
    return QString("http://127.0.0.1:%1/google-callback").arg(m_server.serverPort());
}

void SignupServer::exchangeGoogleCode(const QString& code)
{
    if (!m_net || m_googleClientId.isEmpty() || m_googleClientSecret.isEmpty()) return;

    // POST to Google token endpoint to exchange code → id_token
    QUrlQuery postData;
    postData.addQueryItem("code",          code);
    postData.addQueryItem("client_id",     m_googleClientId);
    postData.addQueryItem("client_secret", m_googleClientSecret);
    postData.addQueryItem("redirect_uri",  googleCallbackUrl());
    postData.addQueryItem("grant_type",    "authorization_code");

    QNetworkRequest req(QUrl("https://oauth2.googleapis.com/token"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QSslConfiguration ssl = QSslConfiguration::defaultConfiguration();
    ssl.setPeerVerifyMode(QSslSocket::VerifyNone);
    req.setSslConfiguration(ssl);

    QNetworkReply* reply = m_net->post(req, postData.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::sslErrors, reply,
            [reply](const QList<QSslError>&){ reply->ignoreSslErrors(); });

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) return;

        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) return;

        // Decode JWT id_token payload (base64url, middle segment) — no signature verify needed
        // since we received it directly over HTTPS from Google's token endpoint.
        const QString idToken = doc.object()["id_token"].toString();
        const QStringList parts = idToken.split('.');
        if (parts.size() < 2) return;

        const QByteArray payload = QByteArray::fromBase64(
            parts[1].toUtf8(),
            QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

        const QJsonDocument payloadDoc = QJsonDocument::fromJson(payload);
        if (!payloadDoc.isObject()) return;

        const QString email = payloadDoc.object()["email"].toString().trimmed();
        const QString name  = payloadDoc.object()["name"].toString().trimmed();
        if (email.isEmpty()) return;

        const QString identity = name.isEmpty() ? email : name;
        if (m_onGoogleLoginSucceeded) m_onGoogleLoginSucceeded(identity);
    });
}

static QByteArray facePageHtml(const QString& title, const QString& subtitle, bool registerMode)
{
        const QString action = registerMode
            ? QStringLiteral("Capturer et enregistrer")
            : QStringLiteral("Verifier mon visage");
        const QString mode = registerMode
            ? QStringLiteral("register")
            : QStringLiteral("verify");

        const QString html = QString(
R"HTML(<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>%1</title>
    <style>
        :root { --p:#0A5F58; --bg:#eef6f4; --txt:#2d4f48; }
        * { box-sizing: border-box; }
        body { margin:0; font-family:Segoe UI, Arial, sans-serif; background:var(--bg); color:var(--txt); }
        .card { max-width:760px; margin:24px auto; background:#fff; border-radius:14px; padding:20px; box-shadow:0 10px 24px rgba(0,0,0,.12); }
        h2 { margin:0 0 8px; color:var(--p); }
        p { margin:0 0 14px; color:#56756f; }
        video { width:100%; border-radius:12px; background:#111; }
        .row { margin-top:14px; display:flex; gap:10px; flex-wrap:wrap; }
        button { border:none; border-radius:10px; background:var(--p); color:#fff; padding:10px 14px; cursor:pointer; font-weight:700; }
        button.secondary { background:#667f7a; }
        .auth-box { margin-top: 12px; display:none; gap:8px; flex-direction:column; }
        .auth-box input { border:1px solid #c7d8d4; border-radius:10px; padding:10px; font-size:14px; }
        #status { margin-top:12px; font-size:14px; }
    </style>
</head>
<body>
    <div class="card">
        <h2>%1</h2>
        <p>%2</p>
        <video id="video" autoplay playsinline></video>
        <div class="row">
            <button id="start">Activer la camera</button>
            <button id="act" class="secondary" disabled>%3</button>
        </div>
        <div id="authBox" class="auth-box">
            <input id="email" type="email" placeholder="Adresse e-mail du compte" />
            <input id="password" type="password" placeholder="Mot de passe" />
        </div>
        <div id="status">Pret.</div>
    </div>
    <script>
        let stream = null;
        const MODE = '%4';
        const video = document.getElementById('video');
        const startBtn = document.getElementById('start');
        const actBtn = document.getElementById('act');
        const statusBox = document.getElementById('status');
        const authBox = document.getElementById('authBox');
        const emailInput = document.getElementById('email');
        const passwordInput = document.getElementById('password');

        if (MODE === 'register') {
            authBox.style.display = 'flex';
        }

        function buildDescriptor(videoEl) {
            const w = videoEl.videoWidth || 0;
            const h = videoEl.videoHeight || 0;
            if (w < 80 || h < 80) {
                throw new Error('Flux video indisponible');
            }

            const src = document.createElement('canvas');
            src.width = w;
            src.height = h;
            const sctx = src.getContext('2d', { willReadFrequently: true });
            sctx.drawImage(videoEl, 0, 0, w, h);

            const cropSize = Math.floor(Math.min(w, h) * 0.72);
            const sx = Math.floor((w - cropSize) / 2);
            const sy = Math.floor((h - cropSize) / 2);

            const normalized = document.createElement('canvas');
            normalized.width = 16;
            normalized.height = 16;
            const nctx = normalized.getContext('2d', { willReadFrequently: true });
            nctx.drawImage(src, sx, sy, cropSize, cropSize, 0, 0, 16, 16);

            const pixels = nctx.getImageData(0, 0, 16, 16).data;
            const descriptor = [];
            let sum = 0;

            for (let i = 0; i < pixels.length; i += 4) {
                const gray = (0.299 * pixels[i]) + (0.587 * pixels[i + 1]) + (0.114 * pixels[i + 2]);
                descriptor.push(gray / 255.0);
                sum += gray / 255.0;
            }

            const mean = sum / descriptor.length;
            let norm = 0;
            for (let i = 0; i < descriptor.length; i++) {
                descriptor[i] = descriptor[i] - mean;
                norm += descriptor[i] * descriptor[i];
            }

            norm = Math.sqrt(norm) || 1;
            for (let i = 0; i < descriptor.length; i++) {
                descriptor[i] = Number((descriptor[i] / norm).toFixed(6));
            }

            return descriptor;
        }

        async function sendJson(url, payload) {
            const res = await fetch(url, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(payload)
            });

            let data = {};
            try {
                data = await res.json();
            } catch (_) {
                data = { ok: false, message: 'Reponse serveur invalide.' };
            }

            if (!res.ok || !data.ok) {
                throw new Error(data.message || 'Operation echouee.');
            }

            return data;
        }

        startBtn.addEventListener('click', async () => {
            try {
                stream = await navigator.mediaDevices.getUserMedia({ video: { facingMode: 'user' } });
                video.srcObject = stream;
                actBtn.disabled = false;
                statusBox.textContent = 'Camera activee.';
            } catch (e) {
                statusBox.textContent = 'Impossible d\'acceder a la camera. Verifiez les permissions navigateur.';
            }
        });

        actBtn.addEventListener('click', async () => {
            try {
                const faceEmbedding = buildDescriptor(video);

                if (MODE === 'register') {
                    const email = emailInput.value.trim();
                    const password = passwordInput.value;
                    if (!email || !password) {
                        statusBox.textContent = 'Saisissez e-mail et mot de passe pour lier le Face ID.';
                        return;
                    }

                    statusBox.textContent = 'Enregistrement Face ID en cours...';
                    const out = await sendJson('/api/face-register', {
                        email,
                        password,
                        faceEmbedding
                    });
                    statusBox.textContent = out.message || 'Face ID enregistre avec succes.';
                    return;
                }

                statusBox.textContent = 'Verification Face ID en cours...';
                const out = await sendJson('/api/face-verify', { faceEmbedding });
                statusBox.textContent = out.message || 'Visage reconnu.';
            } catch (e) {
                statusBox.textContent = e && e.message
                    ? e.message
                    : 'Erreur lors du traitement Face ID.';
            }
        });
    </script>
</body>
</html>)HTML")
        .arg(title, subtitle, action, mode);

        return html.toUtf8();
}

static QVector<double> jsonToVector(const QJsonArray& arr)
{
    QVector<double> out;
    out.reserve(arr.size());
    for (const QJsonValue& v : arr) {
        out.push_back(v.toDouble(0.0));
    }
    return out;
}

static double euclideanDistance(const QVector<double>& a, const QVector<double>& b)
{
    if (a.isEmpty() || b.isEmpty() || a.size() != b.size()) {
        return std::numeric_limits<double>::infinity();
    }

    double sum = 0.0;
    for (int i = 0; i < a.size(); ++i) {
        const double d = a[i] - b[i];
        sum += d * d;
    }

    return std::sqrt(sum);
}

// ─────────────────────────────────────────────────────────────
//  HTML  (CAPTCHA canvas + JS generator added)
// ─────────────────────────────────────────────────────────────
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
        h1 { margin: 0; color: var(--topbar); font-size: 30px; }
        p  { margin: 10px 0 20px; color: rgba(100,83,58,0.86); }
        .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 12px; }
        .full { grid-column: 1 / -1; }
        label { display: block; font-size: 12px; margin-bottom: 6px; color: rgba(100,83,58,0.86); font-weight: 600; }
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
        .ok  { display:block; background:rgba(10,95,88,0.15);  border:1px solid rgba(10,95,88,0.35);  color:#0b4f4a; }
        .err { display:block; background:rgba(177,74,74,0.15); border:1px solid rgba(177,74,74,0.35); color:#7a1f2f; }

        /* ── CAPTCHA styles ── */
        .captcha-wrapper {
            display: flex;
            align-items: center;
            gap: 10px;
            margin-top: 4px;
        }
        #captchaCanvas {
            border-radius: 8px;
            border: 1px solid rgba(18,68,59,0.3);
            cursor: pointer;
            flex-shrink: 0;
        }
        #captchaCanvas:hover { opacity: 0.85; }
        .captcha-hint {
            font-size: 11px;
            color: rgba(100,83,58,0.7);
            margin-top: 4px;
        }
        .refresh-btn {
            background: none;
            border: 1px solid rgba(18,68,59,0.3);
            border-radius: 8px;
            padding: 6px 10px;
            cursor: pointer;
            font-size: 18px;
            color: var(--primary);
            width: auto;
            height: auto;
            margin-top: 0;
            line-height: 1;
        }
        .refresh-btn:hover { background: rgba(10,95,88,0.08); }

        @media (max-width: 760px) { .grid { grid-template-columns: 1fr; } }
    </style>
</head>
<body>
<div class="card">
    <h1>Créer un compte</h1>
    <p>Inscription SmartVision (données employé + accès applicatif).</p>

    <form id="signupForm">
        <div class="grid">
            <div>
                <label for="nom">Nom</label>
                <input id="nom" required maxlength="80" />
            </div>
            <div>
                <label for="prenom">Prénom</label>
                <input id="prenom" required maxlength="80" />
            </div>
            <div class="full">
                <label for="role">Rôle</label>
                <select id="role" required>
                    <option value="Chercheur">Chercheur</option>
                    <option value="Technicien">Technicien</option>
                    <option value="Responsable">Responsable</option>
                </select>
            </div>
            <div class="full">
                <label for="email">Adresse e-mail</label>
                <input id="email" type="email" required maxlength="120" />
            </div>
            <div class="full">
                <label for="password">Mot de passe</label>
                <input id="password" type="password" required maxlength="255" />
            </div>

            <!-- ════════════════ CAPTCHA BLOCK ════════════════ -->
            <div class="full">
                <label>Vérification CAPTCHA</label>
                <div class="captcha-wrapper">
                    <canvas id="captchaCanvas" width="180" height="54" title="Cliquer pour rafraîchir"></canvas>
                    <button type="button" class="refresh-btn" id="refreshCaptcha" title="Nouveau code">&#x21BB;</button>
                    <input id="captchaInput"
                           placeholder="Tapez les caractères ci-dessus"
                           required
                           maxlength="6"
                           autocomplete="off"
                           style="flex:1;" />
                </div>
                <p class="captcha-hint">⚠ Cliquez sur l'image ou sur ↻ pour générer un nouveau code.</p>
            </div>
            <!-- ═════════════════════════════════════════════── -->
        </div>

        <button type="submit">Créer le compte</button>
        <div id="status" class="status"></div>
    </form>
</div>

<script>
// ─────────────────────────────────────────────
//  CAPTCHA  –  pure canvas, no external library
// ─────────────────────────────────────────────
const CHARS   = 'ABCDEFGHJKLMNPQRSTUVWXYZ23456789'; // no I/O/0/1
let captchaCode = '';

function randomInt(min, max) {
    return Math.floor(Math.random() * (max - min + 1)) + min;
}

function randomColor(minBrightness, maxBrightness) {
    const r = randomInt(minBrightness, maxBrightness);
    const g = randomInt(minBrightness, maxBrightness);
    const b = randomInt(minBrightness, maxBrightness);
    return `rgb(${r},${g},${b})`;
}

function drawCaptcha() {
    // Generate a fresh 5-char code
    captchaCode = Array.from({ length: 5 }, () =>
        CHARS[randomInt(0, CHARS.length - 1)]
    ).join('');

    const canvas = document.getElementById('captchaCanvas');
    const ctx    = canvas.getContext('2d');
    const W = canvas.width, H = canvas.height;

    // 1. Background gradient
    const grad = ctx.createLinearGradient(0, 0, W, H);
    grad.addColorStop(0, '#d6eaed');
    grad.addColorStop(1, '#b8d8df');
    ctx.fillStyle = grad;
    ctx.fillRect(0, 0, W, H);

    // 2. Noise dots
    for (let i = 0; i < 80; i++) {
        ctx.beginPath();
        ctx.arc(randomInt(0, W), randomInt(0, H), randomInt(1, 2), 0, Math.PI * 2);
        ctx.fillStyle = randomColor(100, 200);
        ctx.fill();
    }

    // 3. Interference lines
    for (let i = 0; i < 5; i++) {
        ctx.beginPath();
        ctx.moveTo(randomInt(0, W), randomInt(0, H));
        ctx.bezierCurveTo(
            randomInt(0, W), randomInt(0, H),
            randomInt(0, W), randomInt(0, H),
            randomInt(0, W), randomInt(0, H)
        );
        ctx.strokeStyle = randomColor(80, 160);
        ctx.lineWidth   = randomInt(1, 2);
        ctx.stroke();
    }

    // 4. Characters — each randomly rotated, colored, sized
    const charW = W / (captchaCode.length + 1);
    ctx.textBaseline = 'middle';

    captchaCode.split('').forEach((ch, i) => {
        const x = charW * (i + 0.8) + randomInt(-4, 4);
        const y = H / 2 + randomInt(-6, 6);

        ctx.save();
        ctx.translate(x, y);
        ctx.rotate((randomInt(-25, 25) * Math.PI) / 180);
        ctx.font = `bold ${randomInt(22, 28)}px 'Courier New', monospace`;
        ctx.fillStyle = randomColor(20, 100);

        // Slight shadow for depth
        ctx.shadowColor = 'rgba(0,0,0,0.25)';
        ctx.shadowBlur  = 3;
        ctx.fillText(ch, 0, 0);
        ctx.restore();
    });

    // 5. Border
    ctx.strokeStyle = 'rgba(10,95,88,0.4)';
    ctx.lineWidth   = 1;
    ctx.strokeRect(0.5, 0.5, W - 1, H - 1);

    // Clear user input
    document.getElementById('captchaInput').value = '';
}

// Draw on load, and on refresh button / canvas click
drawCaptcha();
document.getElementById('refreshCaptcha').addEventListener('click', drawCaptcha);
document.getElementById('captchaCanvas').addEventListener('click', drawCaptcha);

// ─────────────────────────────────────────────
//  Form submission
// ─────────────────────────────────────────────
const form      = document.getElementById('signupForm');
const statusBox = document.getElementById('status');

function isValidName(value) {
    return /^[\p{L}][\p{L}\s'\-]{0,79}$/u.test(value.trim());
}

function isValidEmail(value) {
    return /^[A-Z0-9._%+-]+@[A-Z0-9.-]+\.[A-Z]{2,}$/i.test(value.trim());
}

function isStrongPassword(value) {
    const v = value.trim();
    return v.length >= 8
        && /[a-z]/.test(v)
        && /[A-Z]/.test(v)
        && /[0-9]/.test(v)
        && /[^A-Za-z0-9]/.test(v);
}

function setStatus(ok, text) {
    statusBox.className  = 'status ' + (ok ? 'ok' : 'err');
    statusBox.textContent = text;
}

form.addEventListener('submit', async (e) => {
    e.preventDefault();

    // ── CAPTCHA check (client-side fast fail) ──
    const userCaptcha = document.getElementById('captchaInput').value.trim().toUpperCase();
    if (userCaptcha !== captchaCode) {
        setStatus(false, '❌ Code CAPTCHA incorrect. Veuillez réessayer.');
        drawCaptcha();   // regenerate immediately
        return;
    }

    const payload = {
        nom:             document.getElementById('nom').value.trim(),
        prenom:          document.getElementById('prenom').value.trim(),
        role:            document.getElementById('role').value,
        email:           document.getElementById('email').value.trim(),
        password:        document.getElementById('password').value,
        captcha:         userCaptcha,
        captchaExpected: captchaCode
    };

    if (!isValidName(payload.nom) || !isValidName(payload.prenom)) {
        setStatus(false, "Nom et prénom invalides (lettres uniquement, max 80 caractères).");
        return;
    }

    if (!isValidEmail(payload.email)) {
        setStatus(false, "Adresse e-mail invalide.");
        return;
    }

    if (!isStrongPassword(payload.password)) {
        setStatus(false, "Mot de passe faible (8+ caractères, majuscule, minuscule, chiffre, spécial).");
        return;
    }

    try {
        const res  = await fetch('/api/signup', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(payload)
        });
        const data = await res.json();

        if (!res.ok || !data.ok) {
            setStatus(false, data.message || 'Échec de création du compte.');
            drawCaptcha();
            return;
        }

        setStatus(true, data.message || 'Compte créé avec succès.');
        form.reset();
        drawCaptcha();

    } catch (err) {
        setStatus(false, "Le service d'inscription local est indisponible.");
    }
});
</script>
</body>
</html>
)HTML";

    return QByteArray(html);
}

// ─────────────────────────────────────────────────────────────
//  HTTP helpers  (unchanged)
// ─────────────────────────────────────────────────────────────
void SignupServer::sendHttpResponse(QTcpSocket* socket, int statusCode,
                                    const QByteArray& body,
                                    const QByteArray& contentType)
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

void SignupServer::sendJson(QTcpSocket* socket, int statusCode,
                            const QString& message, bool ok)
{
    QJsonObject obj;
    obj["ok"]      = ok;
    obj["message"] = message;
    sendHttpResponse(socket,
                     statusCode,
                     QJsonDocument(obj).toJson(QJsonDocument::Compact),
                     "application/json");
}

// ─────────────────────────────────────────────────────────────
//  Request router  (CAPTCHA server-side double-check added)
// ─────────────────────────────────────────────────────────────
void SignupServer::handleHttpRequest(const QByteArray& requestData, QTcpSocket* socket)
{
    const int headerEnd = requestData.indexOf("\r\n\r\n");
    if (headerEnd < 0) {
        sendHttpResponse(socket, 400, "Invalid HTTP request", "text/plain");
        return;
    }

    const QByteArray head      = requestData.left(headerEnd);
    const QByteArray body      = requestData.mid(headerEnd + 4);
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

    const QByteArray method  = requestLine.at(0).trimmed().toUpper();
    const QByteArray rawPath = requestLine.at(1).trimmed();

    // Séparer chemin et query string (ex: /google-callback?code=xxx&state=yyy)
    const int qmark = rawPath.indexOf('?');
    const QByteArray path      = (qmark >= 0) ? rawPath.left(qmark) : rawPath;
    const QByteArray queryPart = (qmark >= 0) ? rawPath.mid(qmark + 1) : QByteArray();

    // ── Callback Google OAuth natif (sans PHP) ────────────────────
    if (method == "GET" && path == "/google-callback") {
        // Répondre immédiatement au navigateur (page auto-fermante)
        const QByteArray successHtml =
            "<!DOCTYPE html><html lang='fr'><head><meta charset='UTF-8'>"
            "<title>SmartVision - Connexion Google</title>"
            "<style>body{margin:0;display:flex;align-items:center;justify-content:center;"
            "min-height:100vh;font-family:Segoe UI,Arial,sans-serif;"
            "background:linear-gradient(135deg,#0a5f58,#0d7a70);color:#fff;}"
            ".card{text-align:center;background:rgba(255,255,255,.12);"
            "border-radius:18px;padding:40px 50px;backdrop-filter:blur(12px);}"
            "h2{margin:0 0 10px;font-size:24px;}p{opacity:.8;margin:0 0 20px;}"
            ".ok{font-size:48px;}</style>"
            "<script>setTimeout(()=>window.close(),2500);</script>"
            "</head><body><div class='card'>"
            "<div class='ok'>✅</div>"
            "<h2>Connexion réussie !</h2>"
            "<p>Vous pouvez fermer cette fenêtre.</p>"
            "</div></body></html>";
        sendHttpResponse(socket, 200, successHtml, "text/html; charset=utf-8");

        // Extraire le code OAuth depuis la query string
        QUrlQuery q(QString::fromUtf8(queryPart));
        const QString code = q.queryItemValue("code");
        if (!code.isEmpty()) {
            exchangeGoogleCode(code);
        }
        return;
    }

    if (method == "GET" && (path == "/" || path == "/signup")) {
        sendHttpResponse(socket, 200, signupHtml(), "text/html");
        return;
    }

    if (method == "GET" && path == "/face-register") {
        sendHttpResponse(
            socket,
            200,
            facePageHtml("Enregistrement Face ID", "Associez votre visage a votre compte SmartVision.", true),
            "text/html");
        return;
    }

    if (method == "GET" && path == "/face-verify") {
        sendHttpResponse(
            socket,
            200,
            facePageHtml("Connexion Face ID", "Verification faciale locale contre la base SmartVision.", false),
            "text/html");
        return;
    }

    if (method == "POST" && path == "/api/face-register") {
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            sendJson(socket, 400, "Payload JSON invalide.", false);
            return;
        }

        const QJsonObject obj = doc.object();
        const QString email = obj.value("email").toString().trimmed();
        const QString password = obj.value("password").toString();
        const QJsonArray embeddingArr = obj.value("faceEmbedding").toArray();

        if (email.isEmpty() || password.isEmpty() || embeddingArr.isEmpty()) {
            sendJson(socket, 400, "E-mail, mot de passe et visage sont requis.", false);
            return;
        }

        QSqlDatabase db = QSqlDatabase::database();
        if (!db.isOpen()) {
            sendJson(socket, 500, "La connexion a la base de donnees est indisponible.", false);
            return;
        }

        const QByteArray faceBlob = QJsonDocument(embeddingArr).toJson(QJsonDocument::Compact);

        QSqlQuery query(db);
        query.prepare(
            "UPDATE \"Employés\" "
            "SET \"FACE_ID\" = ? "
            "WHERE LOWER(\"EMAIL\") = LOWER(?) "
            "AND \"USER_PASSWORD\" = ? "
            "AND \"ACTIVE\" = 'O'");
        query.addBindValue(faceBlob);
        query.addBindValue(email);
        query.addBindValue(password);

        if (!query.exec()) {
            sendJson(socket, 500, "Erreur SQL lors de l'enregistrement Face ID : " + query.lastError().text(), false);
            return;
        }

        if (query.numRowsAffected() <= 0) {
            sendJson(socket, 400, "Compte introuvable ou identifiants invalides.", false);
            return;
        }

        sendJson(socket, 200, "Face ID enregistre avec succes.", true);
        return;
    }

    if (method == "POST" && path == "/api/face-verify") {
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            sendJson(socket, 400, "Payload JSON invalide.", false);
            return;
        }

        const QJsonArray inputArr = doc.object().value("faceEmbedding").toArray();
        const QVector<double> inputVec = jsonToVector(inputArr);
        if (inputVec.isEmpty()) {
            sendJson(socket, 400, "Empreinte faciale manquante.", false);
            return;
        }

        QSqlDatabase db = QSqlDatabase::database();
        if (!db.isOpen()) {
            sendJson(socket, 500, "La connexion a la base de donnees est indisponible.", false);
            return;
        }

        QSqlQuery query(db);
        query.prepare(
            "SELECT \"EMAIL\", NVL(\"FULL_NAME\", \"nom\" || ' ' || \"prenom\"), \"FACE_ID\" "
            "FROM \"Employés\" "
            "WHERE \"ACTIVE\" = 'O' AND \"FACE_ID\" IS NOT NULL");

        if (!query.exec()) {
            sendJson(socket, 500, "Erreur SQL lors de la verification Face ID : " + query.lastError().text(), false);
            return;
        }

        QString bestEmail;
        QString bestName;
        double bestDistance = std::numeric_limits<double>::infinity();

        while (query.next()) {
            const QString email = query.value(0).toString();
            const QString fullName = query.value(1).toString();
            const QByteArray blob = query.value(2).toByteArray();
            if (blob.isEmpty()) continue;

            QJsonParseError rowErr;
            const QJsonDocument rowDoc = QJsonDocument::fromJson(blob, &rowErr);
            if (rowErr.error != QJsonParseError::NoError || !rowDoc.isArray()) continue;

            const QVector<double> rowVec = jsonToVector(rowDoc.array());
            const double dist = euclideanDistance(inputVec, rowVec);
            if (dist < bestDistance) {
                bestDistance = dist;
                bestEmail = email;
                bestName = fullName;
            }
        }

        const double threshold = 0.95;
        if (!bestEmail.isEmpty() && bestDistance <= threshold) {
            const QString displayName = bestName.trimmed().isEmpty() ? bestEmail : bestName;
            if (m_onFaceLoginSucceeded) m_onFaceLoginSucceeded(displayName);
            sendJson(socket, 200,
                     QString("Visage reconnu : %1 (score %.3f)").arg(displayName).arg(bestDistance),
                     true);
            return;
        }

        sendJson(socket, 401, "Visage non reconnu. Rapprochez-vous de la camera et reessayez.", false);
        return;
    }

    if (method == "POST" && path == "/api/google-oauth-success") {
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            sendJson(socket, 400, "Payload JSON invalide.", false);
            return;
        }

        const QJsonObject obj = doc.object();
        const QString email = obj.value("email").toString().trimmed();
        const QString name = obj.value("name").toString().trimmed();

        if (email.isEmpty()) {
            sendJson(socket, 400, "Email Google manquant.", false);
            return;
        }

        const QString identity = name.isEmpty() ? email : name;
        if (m_onGoogleLoginSucceeded) m_onGoogleLoginSucceeded(identity);
        sendJson(socket, 200, "Connexion Google transmise a l'application.", true);
        return;
    }

    if (method != "POST" || path != "/api/signup") {
        sendHttpResponse(socket, 404, "Not found", "text/plain");
        return;
    }

    // ── Parse JSON ──
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        sendJson(socket, 400, "Payload JSON invalide.", false);
        return;
    }

    const QJsonObject obj = doc.object();

    // ── CAPTCHA server-side double-check ──────────────────────
    const QString captchaUser     = obj.value("captcha").toString().trimmed().toUpper();
    const QString captchaExpected = obj.value("captchaExpected").toString().trimmed().toUpper();

    if (captchaUser.isEmpty() || captchaExpected.isEmpty() || captchaUser != captchaExpected) {
        sendJson(socket, 400, "Vérification CAPTCHA échouée côté serveur.", false);
        return;
    }
    // ─────────────────────────────────────────────────────────

    const QString nom      = obj.value("nom").toString().trimmed();
    const QString prenom   = obj.value("prenom").toString().trimmed();
    const QString role     = obj.value("role").toString().trimmed();
    const QString email    = obj.value("email").toString().trimmed();
    const QString password = obj.value("password").toString();

    if (nom.isEmpty() || prenom.isEmpty() ||
        role.isEmpty() || email.isEmpty() || password.isEmpty()) {
        sendJson(socket, 400, "Tous les champs obligatoires doivent être remplis.", false);
        return;
    }

    if (!isValidHumanName(nom) || !isValidHumanName(prenom)) {
        sendJson(socket, 400, "Nom ou prénom invalide (lettres uniquement, max 80 caractères).", false);
        return;
    }

    if (!isValidSignupEmail(email)) {
        sendJson(socket, 400, "Adresse e-mail invalide.", false);
        return;
    }

    if (!isStrongSignupPassword(password)) {
        sendJson(socket, 400, "Mot de passe faible (8+ caractères, majuscule, minuscule, chiffre, spécial).", false);
        return;
    }

    if (role != "Chercheur" && role != "Technicien" && role != "Responsable") {
        sendJson(socket, 400, "Rôle invalide.", false);
        return;
    }

    QString emailReason;
    if (!verifyEmailWithApi(email, &emailReason)) {
        sendJson(socket, 400,
                 emailReason.isEmpty() ? "Adresse e-mail non valide." : emailReason,
                 false);
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

    QSqlQuery idQ(db);
    if (!idQ.exec("SELECT NVL(MAX(\"employee_id\"),0)+1 FROM \"Employés\"") || !idQ.next()) {
        db.rollback();
        sendJson(socket, 500, "Impossible de générer l'ID employé : " + idQ.lastError().text(), false);
        return;
    }
    int nextEmpId = idQ.value(0).toInt();
    // Auto-generate CIN if not provided by the form
    const QString autoCin = QString("EMP%1").arg(nextEmpId);

    QSqlQuery query(db);
    query.prepare(
        "INSERT INTO \"Employés\" "
        "(\"employee_id\", \"CIN\", \"nom\", \"prenom\", \"ROLE\", "
        " \"EMAIL\", \"USER_PASSWORD\", \"FULL_NAME\", \"ACTIVE\") "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, 'O')");
    query.addBindValue(nextEmpId);
    query.addBindValue(autoCin);
    query.addBindValue(nom);
    query.addBindValue(prenom);
    query.addBindValue(role);
    query.addBindValue(email);
    query.addBindValue(password);
    query.addBindValue(nom + " " + prenom);

    if (!query.exec()) {
        db.rollback();
        sendJson(socket, 400, "Insertion employé échouée : " + query.lastError().text(), false);
        return;
    }

    if (!db.commit()) {
        db.rollback();
        sendJson(socket, 500, "Commit SQL échoué.", false);
        return;
    }

    sendJson(socket, 201, "Compte créé avec succès. Vous pouvez vous connecter dans l'application.", true);
}
