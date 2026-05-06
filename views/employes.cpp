#include "employes.h"

#include <QMetaType>
#include <QRegularExpression>
#include <QCryptographicHash>
#include <QStringList>
#include <QSettings>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QEventLoop>
#include <QTimer>
#include <QSslConfiguration>
#include <QSslSocket>

static bool isValidCin(const QString& cin)
{
    static const QRegularExpression re("^[0-9]{8}$");
    return re.match(cin.trimmed()).hasMatch();
}

static bool isValidEmail(const QString& email)
{
    static const QRegularExpression re("^[A-Z0-9._%+-]+@[A-Z0-9.-]+\\.[A-Z]{2,}$",
                                       QRegularExpression::CaseInsensitiveOption);
    return re.match(email.trimmed()).hasMatch();
}

static bool isStrongPassword(const QString& password)
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

static QString hashPasswordSha256(const QString& password)
{
    const QByteArray digest = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);
    return QString::fromLatin1(digest.toHex());
}

static bool cinExists(const QString& cin, int excludeEmployeeId = 0)
{
    QSqlQuery q;
    if (excludeEmployeeId > 0) {
        q.prepare("SELECT COUNT(1) FROM \"Employés\" WHERE \"CIN\" = :cin AND \"employee_id\" <> :id");
        q.bindValue(":id", excludeEmployeeId);
    } else {
        q.prepare("SELECT COUNT(1) FROM \"Employés\" WHERE \"CIN\" = :cin");
    }
    q.bindValue(":cin", cin.trimmed());
    if (!q.exec() || !q.next()) return false;
    return q.value(0).toInt() > 0;
}

static bool emailExists(const QString& email, int excludeEmployeeId = 0)
{
    QSqlQuery q;
    if (excludeEmployeeId > 0) {
        q.prepare("SELECT COUNT(1) FROM \"Employés\" WHERE LOWER(\"EMAIL\") = LOWER(:email) AND \"employee_id\" <> :id");
        q.bindValue(":id", excludeEmployeeId);
    } else {
        q.prepare("SELECT COUNT(1) FROM \"Employés\" WHERE LOWER(\"EMAIL\") = LOWER(:email)");
    }
    q.bindValue(":email", email.trimmed());
    if (!q.exec() || !q.next()) return false;
    return q.value(0).toInt() > 0;
}

static bool jsonValueToBool(const QJsonValue& v, bool* out)
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

static bool readApiBool(const QJsonObject& obj, const QString& key, bool* out)
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

static QString emailValidationApiKey()
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

static QString emailValidationApiUrl()
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

static bool verifyEmailWithApi(const QString& email, QString* reason)
{
    const QString apiKey = emailValidationApiKey();
    if (apiKey.isEmpty()) {
        // Keep behavior unchanged when API key is not configured.
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
        if (reason) *reason = "Verification e-mail expiree (timeout API).";
        reply->deleteLater();
        return false;
    }
    timer.stop();

    if (reply->error() != QNetworkReply::NoError) {
        if (reason) *reason = "Impossible de verifier l'adresse e-mail pour le moment.";
        reply->deleteLater();
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
    reply->deleteLater();

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (reason) *reason = "Reponse API e-mail invalide.";
        return false;
    }

    const QJsonObject root = doc.object();
    if (root.contains("error") && !root.value("error").isNull()) {
        if (reason) *reason = "Service de verification e-mail indisponible.";
        return false;
    }

    const QString deliverability = root.value("deliverability").toString().trimmed().toLower();
    if (!deliverability.isEmpty()) {
        if (deliverability.contains("undeliver") || deliverability.contains("invalid")
            || deliverability.contains("risky") || deliverability.contains("unknown")) {
            if (reason) *reason = "L'adresse e-mail semble invalide ou non delivrable.";
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
            if (reason) *reason = "Le serveur e-mail n'a pas valide cette adresse.";
            return false;
        }
    }

    if (anySignal) return true;

    if (reason) *reason = "La validation e-mail n'a pas pu confirmer cette adresse.";
    return false;
}

static void ensureEmployesExtraColumns()
{
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

    QSqlQuery q;
    const QStringList ddl = {
        "ALTER TABLE \"Employés\" ADD (\"QUALIFICATION\" VARCHAR2(120))",
        "ALTER TABLE \"Employés\" ADD (\"TEMPS_TRAVAIL\" VARCHAR2(30))",
        "ALTER TABLE \"Employés\" ADD (\"LABORATOIRE\" VARCHAR2(120))",
        "ALTER TABLE \"Employés\" ADD (\"PROJET_AFFECTE\" VARCHAR2(150))",
        "ALTER TABLE \"Employés\" ADD (\"NB_PUBLICATIONS\" NUMBER DEFAULT 0)"
    };

    for (const QString& stmt : ddl) {
        q.exec(stmt);
    }
}

bool EmployesCrud::loadEmployes(QList<EmployeRecord>& out,
                                QString* error,
                                const QString& cin,
                                const QString& nom,
                                const QString& prenom,
                                const QString& role,
                                const QString& specialization)
{
    out.clear();
    ensureEmployesExtraColumns();

    QSqlQuery q;
    q.prepare(
        "SELECT \"employee_id\", \"CIN\", \"nom\", \"prenom\", \"EMAIL\", \"ROLE\", \"specialization\", "
        "       NVL(\"QUALIFICATION\", ''), NVL(\"TEMPS_TRAVAIL\", ''), NVL(\"LABORATOIRE\", ''), "
        "       NVL(\"PROJET_AFFECTE\", ''), NVL(\"NB_PUBLICATIONS\", 0) "
        "FROM \"Employés\" "
        "WHERE (:cin IS NULL OR :cin = '' OR LOWER(\"CIN\") LIKE '%' || LOWER(:cin) || '%') "
        "  AND (:nom IS NULL OR :nom = '' OR LOWER(\"nom\") LIKE '%' || LOWER(:nom) || '%') "
        "  AND (:prenom IS NULL OR :prenom = '' OR LOWER(\"prenom\") LIKE '%' || LOWER(:prenom) || '%') "
        "  AND (:role IS NULL OR :role = '' OR LOWER(\"ROLE\") LIKE '%' || LOWER(:role) || '%') "
        "  AND (:spec IS NULL OR :spec = '' OR LOWER(\"specialization\") LIKE '%' || LOWER(:spec) || '%') "
        "ORDER BY \"nom\", \"prenom\", \"employee_id\"");

    q.bindValue(":cin", cin);
    q.bindValue(":nom", nom);
    q.bindValue(":prenom", prenom);
    q.bindValue(":role", role);
    q.bindValue(":spec", specialization);

    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }

    while (q.next()) {
        EmployeRecord rec;
        rec.employeeId     = q.value(0).toInt();
        rec.cin            = q.value(1).toString();
        rec.nom            = q.value(2).toString();
        rec.prenom         = q.value(3).toString();
        rec.email          = q.value(4).toString();
        rec.role           = q.value(5).toString();
        rec.specialization = q.value(6).toString();
        rec.qualification  = q.value(7).toString();
        rec.tempsTravail   = q.value(8).toString();
        rec.laboratoire    = q.value(9).toString();
        rec.projetAffecte  = q.value(10).toString();
        rec.nbPublications = q.value(11).toInt();
        rec.password.clear();
        out.push_back(rec);
    }

    return true;
}

bool EmployesCrud::fetchEmploye(int employeeId, EmployeRecord& out, QString* error)
{
    ensureEmployesExtraColumns();
    QSqlQuery q;
    q.prepare("SELECT \"CIN\", \"nom\", \"prenom\", \"EMAIL\", \"ROLE\", \"specialization\", "
              "       NVL(\"QUALIFICATION\", ''), NVL(\"TEMPS_TRAVAIL\", ''), NVL(\"LABORATOIRE\", ''), "
              "       NVL(\"PROJET_AFFECTE\", ''), NVL(\"NB_PUBLICATIONS\", 0) "
              "FROM \"Employés\" WHERE \"employee_id\" = :id");
    q.bindValue(":id", employeeId);

    if (!q.exec() || !q.next()) {
        if (error) *error = q.lastError().text();
        return false;
    }

    out.employeeId     = employeeId;
    out.cin            = q.value(0).toString();
    out.nom            = q.value(1).toString();
    out.prenom         = q.value(2).toString();
    out.email          = q.value(3).toString();
    out.role           = q.value(4).toString();
    out.specialization = q.value(5).toString();
    out.qualification  = q.value(6).toString();
    out.tempsTravail   = q.value(7).toString();
    out.laboratoire    = q.value(8).toString();
    out.projetAffecte  = q.value(9).toString();
    out.nbPublications = q.value(10).toInt();
    out.password.clear();

    return true;
}

bool EmployesCrud::deleteEmploye(int employeeId, QString* error)
{
    // Supprimer d'abord les lignes enfants dans Associer (FK_ASSOCIER_EMP)
    QSqlQuery qAssoc;
    qAssoc.prepare("DELETE FROM \"Associer\" WHERE \"employee_id\" = :id");
    qAssoc.bindValue(":id", employeeId);
    if (!qAssoc.exec()) {
        if (error) *error = qAssoc.lastError().text();
        return false;
    }

    QSqlQuery q;
    q.prepare("DELETE FROM \"Employés\" WHERE \"employee_id\" = :id");
    q.bindValue(":id", employeeId);
    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }

    return true;
}

int EmployesCrud::nextEmployeId(QString* error)
{
    QSqlQuery q;
    if (!q.exec("SELECT NVL(MAX(\"employee_id\"),0)+1 FROM \"Employés\"") || !q.next()) {
        if (error) *error = q.lastError().text();
        return -1;
    }

    return q.value(0).toInt();
}

bool EmployesCrud::insertEmploye(const EmployeRecord& in, QString* error)
{
    ensureEmployesExtraColumns();
    if (in.cin.trimmed().isEmpty()) {
        if (error) *error = "CIN est obligatoire.";
        return false;
    }
    if (in.nom.trimmed().isEmpty()) {
        if (error) *error = "NOM est obligatoire.";
        return false;
    }
    if (in.prenom.trimmed().isEmpty()) {
        if (error) *error = "PRENOM est obligatoire.";
        return false;
    }
    if (!isValidCin(in.cin)) {
        if (error) *error = "CIN invalide. Il doit contenir exactement 8 chiffres.";
        return false;
    }
    if (in.email.trimmed().isEmpty()) {
        if (error) *error = "EMAIL est obligatoire.";
        return false;
    }
    if (!isValidEmail(in.email)) {
        if (error) *error = "EMAIL invalide.";
        return false;
    }
    QString emailReason;
    if (!verifyEmailWithApi(in.email, &emailReason)) {
        if (error) *error = emailReason.isEmpty() ? "Adresse e-mail non valide." : emailReason;
        return false;
    }
    if (cinExists(in.cin)) {
        if (error) *error = "Ce CIN existe déjà.";
        return false;
    }
    if (emailExists(in.email)) {
        if (error) *error = "Cet EMAIL existe déjà.";
        return false;
    }

    const QString password = in.password.trimmed();
    if (!isStrongPassword(password)) {
        if (error) *error = "Mot de passe faible (min 8, majuscule, minuscule, chiffre, caractère spécial).";
        return false;
    }

    const QString inputRole = in.role.trimmed();
    QString role;
    if (inputRole.isEmpty() || inputRole.compare("Chercheur", Qt::CaseInsensitive) == 0) {
        role = QStringLiteral("Chercheur");
    } else if (inputRole.compare("Technicien", Qt::CaseInsensitive) == 0) {
        role = QStringLiteral("Technicien");
    } else if (inputRole.compare("Responsable", Qt::CaseInsensitive) == 0) {
        role = QStringLiteral("Responsable");
    } else if (inputRole.compare("RH", Qt::CaseInsensitive) == 0) {
        role = QStringLiteral("RH");
    } else {
        if (error) *error = "ROLE invalide. Valeurs autorisées: Chercheur, Technicien, Responsable, RH.";
        return false;
    }

    int employeeId = in.employeeId;
    if (employeeId <= 0) {
        employeeId = nextEmployeId(error);
        if (employeeId <= 0) return false;
    }

    QSqlQuery q;
    q.prepare("INSERT INTO \"Employés\" "
              "(\"employee_id\", \"CIN\", \"nom\", \"prenom\", \"EMAIL\", \"USER_PASSWORD\", \"ROLE\", \"specialization\", "
              " \"QUALIFICATION\", \"TEMPS_TRAVAIL\", \"LABORATOIRE\", \"PROJET_AFFECTE\", \"NB_PUBLICATIONS\", \"ACTIVE\") "
              "VALUES (:id, :cin, :nom, :prenom, :email, :pwd, :role, :spec, :qualif, :temps, :lab, :proj, :pubs, 'O')");

    auto nullStr = QVariant(QMetaType::fromType<QString>());

    q.bindValue(":id", employeeId);
    q.bindValue(":cin", in.cin.trimmed());
    q.bindValue(":nom", in.nom.trimmed());
    q.bindValue(":prenom", in.prenom.trimmed());
    q.bindValue(":email", in.email.trimmed());
    q.bindValue(":pwd", hashPasswordSha256(password));
    q.bindValue(":role", role);
    q.bindValue(":spec", in.specialization.trimmed().isEmpty() ? nullStr : QVariant(in.specialization.trimmed()));
    q.bindValue(":qualif", in.qualification.trimmed().isEmpty() ? nullStr : QVariant(in.qualification.trimmed()));
    q.bindValue(":temps", in.tempsTravail.trimmed().isEmpty() ? nullStr : QVariant(in.tempsTravail.trimmed()));
    q.bindValue(":lab", in.laboratoire.trimmed().isEmpty() ? nullStr : QVariant(in.laboratoire.trimmed()));
    q.bindValue(":proj", in.projetAffecte.trimmed().isEmpty() || in.projetAffecte.trimmed() == "-" ? nullStr : QVariant(in.projetAffecte.trimmed()));
    q.bindValue(":pubs", in.nbPublications < 0 ? 0 : in.nbPublications);

    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }

    return true;
}

bool EmployesCrud::updateEmploye(const EmployeRecord& in, QString* error)
{
    ensureEmployesExtraColumns();
    if (in.employeeId <= 0) {
        if (error) *error = "EMPLOYEE_ID invalide.";
        return false;
    }
    if (in.cin.trimmed().isEmpty()) {
        if (error) *error = "CIN est obligatoire.";
        return false;
    }
    if (in.nom.trimmed().isEmpty()) {
        if (error) *error = "NOM est obligatoire.";
        return false;
    }
    if (in.prenom.trimmed().isEmpty()) {
        if (error) *error = "PRENOM est obligatoire.";
        return false;
    }
    if (!isValidCin(in.cin)) {
        if (error) *error = "CIN invalide. Il doit contenir exactement 8 chiffres.";
        return false;
    }
    if (in.email.trimmed().isEmpty()) {
        if (error) *error = "EMAIL est obligatoire.";
        return false;
    }
    if (!isValidEmail(in.email)) {
        if (error) *error = "EMAIL invalide.";
        return false;
    }
    if (cinExists(in.cin, in.employeeId)) {
        if (error) *error = "Ce CIN existe déjà.";
        return false;
    }
    if (emailExists(in.email, in.employeeId)) {
        if (error) *error = "Cet EMAIL existe déjà.";
        return false;
    }

    const QString inputRole = in.role.trimmed();
    QString role;
    if (inputRole.isEmpty() || inputRole.compare("Chercheur", Qt::CaseInsensitive) == 0) {
        role = QStringLiteral("Chercheur");
    } else if (inputRole.compare("Technicien", Qt::CaseInsensitive) == 0) {
        role = QStringLiteral("Technicien");
    } else if (inputRole.compare("Responsable", Qt::CaseInsensitive) == 0) {
        role = QStringLiteral("Responsable");
    } else if (inputRole.compare("RH", Qt::CaseInsensitive) == 0) {
        role = QStringLiteral("RH");
    } else {
        if (error) *error = "ROLE invalide. Valeurs autorisées: Chercheur, Technicien, Responsable, RH.";
        return false;
    }

    QSqlQuery q;
    q.prepare("UPDATE \"Employés\" "
              "SET \"CIN\" = :cin, \"nom\" = :nom, \"prenom\" = :prenom, "
              "    \"EMAIL\" = :email, \"ROLE\" = :role, \"specialization\" = :spec, "
              "    \"QUALIFICATION\" = :qualif, \"TEMPS_TRAVAIL\" = :temps, \"LABORATOIRE\" = :lab, "
              "    \"PROJET_AFFECTE\" = :proj, \"NB_PUBLICATIONS\" = :pubs "
              "WHERE \"employee_id\" = :id");

    auto nullStr = QVariant(QMetaType::fromType<QString>());

    q.bindValue(":cin", in.cin.trimmed());
    q.bindValue(":nom", in.nom.trimmed());
    q.bindValue(":prenom", in.prenom.trimmed());
    q.bindValue(":email", in.email.trimmed());
    q.bindValue(":role", role);
    q.bindValue(":spec", in.specialization.trimmed().isEmpty() ? nullStr : QVariant(in.specialization.trimmed()));
    q.bindValue(":qualif", in.qualification.trimmed().isEmpty() ? nullStr : QVariant(in.qualification.trimmed()));
    q.bindValue(":temps", in.tempsTravail.trimmed().isEmpty() ? nullStr : QVariant(in.tempsTravail.trimmed()));
    q.bindValue(":lab", in.laboratoire.trimmed().isEmpty() ? nullStr : QVariant(in.laboratoire.trimmed()));
    q.bindValue(":proj", in.projetAffecte.trimmed().isEmpty() || in.projetAffecte.trimmed() == "-" ? nullStr : QVariant(in.projetAffecte.trimmed()));
    q.bindValue(":pubs", in.nbPublications < 0 ? 0 : in.nbPublications);
    q.bindValue(":id", in.employeeId);

    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }

    return true;
}
