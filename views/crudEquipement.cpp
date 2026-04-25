#include "crudEquipement.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QRegularExpression>

namespace {

bool equipementHasIdExpColumn()
{
    static bool resolved = false;
    static bool hasColumn = false;
    if (resolved) return hasColumn;

    QSqlQuery q;
    q.prepare(
        "SELECT COUNT(*) "
        "FROM USER_TAB_COLUMNS "
        "WHERE TABLE_NAME = :tbl "
        "  AND COLUMN_NAME = 'Id_exp'");
    q.bindValue(":tbl", QString::fromUtf8("Équipement"));

    if (q.exec() && q.next()) {
        hasColumn = q.value(0).toInt() > 0;
    } else {
        hasColumn = false;
    }
    resolved = true;

    // If the column exists, ensure it allows NULL (remove NOT NULL constraint if present)
    if (hasColumn) {
        QSqlQuery nullable;
        nullable.prepare(
            "SELECT NULLABLE FROM USER_TAB_COLUMNS "
            "WHERE TABLE_NAME = :tbl AND COLUMN_NAME = 'Id_exp'");
        nullable.bindValue(":tbl", QString::fromUtf8("Équipement"));
        if (nullable.exec() && nullable.next()) {
            const QString isNullable = nullable.value(0).toString().trimmed();
            if (isNullable == "N") {
                QSqlQuery alter;
                alter.exec(QString("ALTER TABLE \"%1\" MODIFY (\"Id_exp\" NULL)")
                               .arg(QString::fromUtf8("Équipement")));
            }
        }
    }

    return hasColumn;
}

bool experienceExists(int expId, QString* error)
{
    QSqlQuery q;
    q.prepare("SELECT COUNT(*) FROM \"Expérience\" WHERE \"Id_exp\" = :id");
    q.bindValue(":id", expId);
    if (!q.exec() || !q.next()) {
        if (error) *error = q.lastError().text();
        return false;
    }
    return q.value(0).toInt() > 0;
}

QString statusKey(const QString& value)
{
    const QString s = value.toLower();
    if (s.contains("actif") || s.contains("dispon") || s.contains("usage") || s.contains("mainten")) return "actif";
    if (s.contains("hors") || s.contains("service")) return "hors_service";
    if (s.contains("rchiv") || s.contains("archive")) return "archive";
    return QString();
}

QStringList allowedEqStatusesFromDb()
{
    QSqlQuery q;
    q.prepare("SELECT SEARCH_CONDITION_VC FROM USER_CONSTRAINTS WHERE CONSTRAINT_NAME = 'CK_EQ_STAT'");
    if (!q.exec() || !q.next()) {
        return {"Actif", "Hors service", "Archivé"};
    }

    const QString condition = q.value(0).toString();
    QRegularExpression re("'([^']*)'");
    auto it = re.globalMatch(condition);

    QStringList values;
    while (it.hasNext()) {
        auto m = it.next();
        values.push_back(m.captured(1));
    }

    if (values.isEmpty()) {
        return {"Actif", "Hors service", "Archivé"};
    }
    return values;
}

const QStringList& allowedEqStatuses()
{
    static QStringList cached;
    static bool loaded = false;
    if (!loaded) {
        cached = allowedEqStatusesFromDb();
        loaded = true;
    }
    return cached;
}

QString toDbStatus(const QString& uiStatus)
{
    if (uiStatus.isEmpty()) return "Actif";

    const QStringList values = allowedEqStatuses();
    if (values.contains(uiStatus)) return uiStatus;

    const QString wanted = statusKey(uiStatus);
    if (!wanted.isEmpty()) {
        for (const QString& v : values) {
            if (statusKey(v) == wanted) return v;
        }
    }

    if (values.contains("Actif")) return "Actif";

    for (const QString& v : values) {
        if (statusKey(v) == "actif") return v;
    }

    if (!values.isEmpty()) return values.first();

    return uiStatus;
}

QString toUiStatus(const QString& dbStatus)
{
    if (dbStatus.isEmpty()) return dbStatus;

    if (dbStatus == "Actif" || dbStatus == "Hors service" || dbStatus == "Archivé") {
        return dbStatus;
    }

    const QString key = statusKey(dbStatus);
    if (key == "actif") return "Actif";
    if (key == "hors_service") return "Hors service";
    if (key == "archive") return "Archivé";

    return dbStatus;
}

bool isValidEquipmentText(const QString& value)
{
    static const QRegularExpression re(QStringLiteral("^[\\p{L}\\p{N}\\s'\\-_.()/]+$"));
    return re.match(value.trimmed()).hasMatch();
}

}

bool EquipementCrud::loadResponsables(QList<ResponsableItem>& out, QString* error)
{
    out.clear();
    QSqlQuery q;
    q.prepare("SELECT \"employee_id\", \"nom\", \"prenom\" FROM \"Employés\" ORDER BY \"nom\", \"prenom\"");
    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }

    while (q.next()) {
        ResponsableItem item;
        item.id = q.value(0).toInt();
        item.fullName = q.value(1).toString() + " " + q.value(2).toString();
        out.push_back(item);
    }
    return true;
}

bool EquipementCrud::loadEquipements(QList<EquipementRecord>& out,
                                     QString* error,
                                     const QString& fabricant,
                                     const QString& nom,
                                     const QString& statut,
                                     const QString& localisation)
{
    out.clear();

    const bool hasIdExp = equipementHasIdExpColumn();

    QSqlQuery q;
    const QString sql = QString(
        "SELECT \"equipement_id\", \"nom_equipement\", \"fabricant\", \"numéro_de_modèle\", "
        "       \"date_d_achat\", \"date_dernière_maintenance\", \"date_prochaine_maintenance\", "
        "       \"statut\", \"localisation\", \"date_limite_calibration\", %1 "
        "FROM \"Équipement\" "
        "WHERE (:fab IS NULL OR :fab = '' OR "
        "       LOWER(\"nom_equipement\") LIKE '%' || LOWER(:fab) || '%' OR "
        "       LOWER(\"fabricant\") LIKE '%' || LOWER(:fab) || '%' OR "
        "       LOWER(\"numéro_de_modèle\") LIKE '%' || LOWER(:fab) || '%') "
        "  AND (:nom IS NULL OR :nom = '' OR LOWER(\"nom_equipement\") LIKE '%' || LOWER(:nom) || '%') "
        "  AND (:stat IS NULL OR :stat = '' OR \"statut\" = :stat) "
        "  AND (:loc IS NULL OR :loc = '' OR LOWER(\"localisation\") LIKE '%' || LOWER(:loc) || '%') "
        "ORDER BY CASE WHEN LOWER(\"statut\") LIKE '%actif%' THEN 1 "
        "              WHEN LOWER(\"statut\") LIKE '%hors%' OR LOWER(\"statut\") LIKE '%service%' THEN 2 "
        "              WHEN LOWER(\"statut\") LIKE '%archive%' OR LOWER(\"statut\") LIKE '%rchiv%' THEN 3 "
        "              ELSE 9 END, "
        "         \"date_prochaine_maintenance\" ASC NULLS LAST, \"nom_equipement\"")
        .arg(hasIdExp ? "\"Id_exp\"" : "NULL AS \"Id_exp\"");
    q.prepare(sql);

    const QString dbStatusFilter = toDbStatus(statut);
    q.bindValue(":fab", fabricant);
    q.bindValue(":nom", nom);
    q.bindValue(":stat", statut.trimmed().isEmpty() ? QString() : dbStatusFilter);
    q.bindValue(":loc", localisation);

    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }

    while (q.next()) {
        EquipementRecord rec;
        rec.id                       = q.value(0).toInt();
        rec.nomEquipement            = q.value(1).toString();
        rec.fabricant                = q.value(2).toString();
        rec.numeroModele             = q.value(3).toString();
        rec.dateAchat                = q.value(4).toDate();
        rec.dateDerniereMaintenance  = q.value(5).toDate();
        rec.dateProchaineMaintenance = q.value(6).toDate();
        rec.statut                   = toUiStatus(q.value(7).toString());
        rec.localisation             = q.value(8).toString();
        rec.dateLimiteCalibration    = q.value(9).toDate();
        rec.idExp                    = q.value(10);
        out.push_back(rec);
    }
    return true;
}

bool EquipementCrud::fetchEquipement(int id, EquipementRecord& out, QString* error)
{
    const bool hasIdExp = equipementHasIdExpColumn();

    QSqlQuery q;
    const QString sql = QString(
        "SELECT \"nom_equipement\", \"fabricant\", \"numéro_de_modèle\", "
        "       \"date_d_achat\", \"date_dernière_maintenance\", \"date_prochaine_maintenance\", "
        "       \"statut\", \"localisation\", \"date_limite_calibration\", %1 "
        "FROM \"Équipement\" WHERE \"equipement_id\" = :id")
        .arg(hasIdExp ? "\"Id_exp\"" : "NULL AS \"Id_exp\"");
    q.prepare(sql);
    q.bindValue(":id", id);

    if (!q.exec() || !q.next()) {
        if (error) *error = q.lastError().text();
        return false;
    }

    out.id                       = id;
    out.nomEquipement            = q.value(0).toString();
    out.fabricant                = q.value(1).toString();
    out.numeroModele             = q.value(2).toString();
    out.dateAchat                = q.value(3).toDate();
    out.dateDerniereMaintenance  = q.value(4).toDate();
    out.dateProchaineMaintenance = q.value(5).toDate();
    out.statut                   = toUiStatus(q.value(6).toString());
    out.localisation             = q.value(7).toString();
    out.dateLimiteCalibration    = q.value(8).toDate();
    out.idExp                    = q.value(9);
    return true;
}

bool EquipementCrud::deleteEquipement(int id, QString* error)
{
    QSqlQuery q;
    q.prepare("DELETE FROM \"Équipement\" WHERE \"equipement_id\" = :id");
    q.bindValue(":id", id);
    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }
    return true;
}

int EquipementCrud::nextEquipementId(QString* error)
{
    QSqlQuery q;
    if (!q.exec("SELECT NVL(MAX(\"equipement_id\"),0)+1 FROM \"Équipement\"") || !q.next()) {
        if (error) *error = q.lastError().text();
        return -1;
    }
    return q.value(0).toInt();
}

bool EquipementCrud::insertEquipement(const EquipementRecord& in, QString* error)
{
    const QString nomEq = in.nomEquipement.trimmed();
    const QString fabEq = in.fabricant.trimmed();

    if (nomEq.isEmpty()) {
        if (error) *error = "nom_equipement est obligatoire.";
        return false;
    }

    if (!isValidEquipmentText(nomEq)) {
        if (error) *error = "nom_equipement contient des caractères non autorisés.";
        return false;
    }

    if (!fabEq.isEmpty() && !isValidEquipmentText(fabEq)) {
        if (error) *error = "fabricant contient des caractères non autorisés.";
        return false;
    }

    if (in.dateAchat.isValid() && in.dateProchaineMaintenance.isValid()
        && in.dateAchat >= in.dateProchaineMaintenance) {
        if (error) *error = "date_d_achat doit être inférieure à date_prochaine_maintenance.";
        return false;
    }

    int id = in.id;
    if (id <= 0) {
        id = nextEquipementId(error);
        if (id <= 0) return false;
    }

    const bool hasIdExp = equipementHasIdExpColumn();
    if (hasIdExp && !in.idExp.isNull() && in.idExp.isValid() && in.idExp.toInt() > 0) {
        QString checkErr;
        if (!experienceExists(in.idExp.toInt(), &checkErr)) {
            if (error) {
                *error = checkErr.isEmpty()
                    ? QString("Id_exp %1 n'existe pas dans Expérience.").arg(in.idExp.toInt())
                    : checkErr;
            }
            return false;
        }
    }

    QSqlQuery q;
    const QString sql = hasIdExp
        ? QString(
        "INSERT INTO \"Équipement\" "
        "(\"equipement_id\", \"nom_equipement\", \"fabricant\", \"numéro_de_modèle\", \"date_d_achat\", "
        " \"date_dernière_maintenance\", \"date_prochaine_maintenance\", \"statut\", "
        " \"localisation\", \"date_limite_calibration\", \"Id_exp\") "
        "VALUES (:id, :nom, :fab, :mod, TO_DATE(:da,'YYYY-MM-DD'), "
        "        TO_DATE(:ddm,'YYYY-MM-DD'), TO_DATE(:dpm,'YYYY-MM-DD'), :stat, "
        "        :loc, TO_DATE(:dlc,'YYYY-MM-DD'), :idexp)")
        : QString(
        "INSERT INTO \"Équipement\" "
        "(\"equipement_id\", \"nom_equipement\", \"fabricant\", \"numéro_de_modèle\", \"date_d_achat\", "
        " \"date_dernière_maintenance\", \"date_prochaine_maintenance\", \"statut\", "
        " \"localisation\", \"date_limite_calibration\") "
        "VALUES (:id, :nom, :fab, :mod, TO_DATE(:da,'YYYY-MM-DD'), "
        "        TO_DATE(:ddm,'YYYY-MM-DD'), TO_DATE(:dpm,'YYYY-MM-DD'), :stat, "
        "        :loc, TO_DATE(:dlc,'YYYY-MM-DD'))");
    q.prepare(sql);

    auto nullStr = QVariant(QMetaType::fromType<QString>());
    const QString dbStatus = toDbStatus(in.statut);

    q.bindValue(":id", id);
    q.bindValue(":nom", nomEq);
    q.bindValue(":fab", fabEq.isEmpty() ? nullStr : QVariant(fabEq));
    q.bindValue(":mod", in.numeroModele.isEmpty() ? nullStr : QVariant(in.numeroModele));
    q.bindValue(":da",  in.dateAchat.isValid() ? QVariant(in.dateAchat.toString("yyyy-MM-dd")) : nullStr);
    q.bindValue(":ddm", in.dateDerniereMaintenance.isValid() ? QVariant(in.dateDerniereMaintenance.toString("yyyy-MM-dd")) : nullStr);
    q.bindValue(":dpm", in.dateProchaineMaintenance.isValid() ? QVariant(in.dateProchaineMaintenance.toString("yyyy-MM-dd")) : nullStr);
    q.bindValue(":stat", dbStatus.isEmpty() ? QVariant("Actif") : QVariant(dbStatus));
    q.bindValue(":loc", in.localisation.isEmpty() ? nullStr : QVariant(in.localisation));
    q.bindValue(":dlc", in.dateLimiteCalibration.isValid() ? QVariant(in.dateLimiteCalibration.toString("yyyy-MM-dd")) : nullStr);
    if (hasIdExp) {
        if (in.idExp.isValid() && in.idExp.toInt() > 0) {
            q.bindValue(":idexp", in.idExp.toInt());
        } else {
            q.bindValue(":idexp", QVariant(QMetaType::fromType<int>()));
        }
    }

    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }
    return true;
}

bool EquipementCrud::updateEquipement(const EquipementRecord& in, QString* error)
{
    const QString nomEq = in.nomEquipement.trimmed();
    const QString fabEq = in.fabricant.trimmed();

    if (in.id <= 0) {
        if (error) *error = "equipement_id invalide.";
        return false;
    }
    if (nomEq.isEmpty()) {
        if (error) *error = "nom_equipement est obligatoire.";
        return false;
    }

    if (!isValidEquipmentText(nomEq)) {
        if (error) *error = "nom_equipement contient des caractères non autorisés.";
        return false;
    }

    if (!fabEq.isEmpty() && !isValidEquipmentText(fabEq)) {
        if (error) *error = "fabricant contient des caractères non autorisés.";
        return false;
    }

    if (in.dateAchat.isValid() && in.dateProchaineMaintenance.isValid()
        && in.dateAchat >= in.dateProchaineMaintenance) {
        if (error) *error = "date_d_achat doit être inférieure à date_prochaine_maintenance.";
        return false;
    }

    const bool hasIdExp = equipementHasIdExpColumn();
    if (hasIdExp && !in.idExp.isNull() && in.idExp.isValid() && in.idExp.toInt() > 0) {
        QString checkErr;
        if (!experienceExists(in.idExp.toInt(), &checkErr)) {
            if (error) {
                *error = checkErr.isEmpty()
                    ? QString("Id_exp %1 n'existe pas dans Expérience.").arg(in.idExp.toInt())
                    : checkErr;
            }
            return false;
        }
    }

    QSqlQuery q;
    const QString sql = hasIdExp
        ? QString(
        "UPDATE \"Équipement\" "
        "SET \"nom_equipement\" = :nom, "
        "    \"fabricant\" = :fab, "
        "    \"numéro_de_modèle\" = :mod, "
        "    \"date_d_achat\" = TO_DATE(:da,'YYYY-MM-DD'), "
        "    \"date_dernière_maintenance\" = TO_DATE(:ddm,'YYYY-MM-DD'), "
        "    \"date_prochaine_maintenance\" = TO_DATE(:dpm,'YYYY-MM-DD'), "
        "    \"statut\" = :stat, "
        "    \"localisation\" = :loc, "
        "    \"date_limite_calibration\" = TO_DATE(:dlc,'YYYY-MM-DD'), "
        "    \"Id_exp\" = :idexp "
        "WHERE \"equipement_id\" = :id")
        : QString(
        "UPDATE \"Équipement\" "
        "SET \"nom_equipement\" = :nom, "
        "    \"fabricant\" = :fab, "
        "    \"numéro_de_modèle\" = :mod, "
        "    \"date_d_achat\" = TO_DATE(:da,'YYYY-MM-DD'), "
        "    \"date_dernière_maintenance\" = TO_DATE(:ddm,'YYYY-MM-DD'), "
        "    \"date_prochaine_maintenance\" = TO_DATE(:dpm,'YYYY-MM-DD'), "
        "    \"statut\" = :stat, "
        "    \"localisation\" = :loc, "
        "    \"date_limite_calibration\" = TO_DATE(:dlc,'YYYY-MM-DD') "
        "WHERE \"equipement_id\" = :id");
    q.prepare(sql);

    auto nullStr = QVariant(QMetaType::fromType<QString>());
    const QString dbStatus = toDbStatus(in.statut);

    q.bindValue(":nom", nomEq);
    q.bindValue(":fab", fabEq.isEmpty() ? nullStr : QVariant(fabEq));
    q.bindValue(":mod", in.numeroModele.isEmpty() ? nullStr : QVariant(in.numeroModele));
    q.bindValue(":da",  in.dateAchat.isValid() ? QVariant(in.dateAchat.toString("yyyy-MM-dd")) : nullStr);
    q.bindValue(":ddm", in.dateDerniereMaintenance.isValid() ? QVariant(in.dateDerniereMaintenance.toString("yyyy-MM-dd")) : nullStr);
    q.bindValue(":dpm", in.dateProchaineMaintenance.isValid() ? QVariant(in.dateProchaineMaintenance.toString("yyyy-MM-dd")) : nullStr);
    q.bindValue(":stat", dbStatus.isEmpty() ? QVariant("Actif") : QVariant(dbStatus));
    q.bindValue(":loc", in.localisation.isEmpty() ? nullStr : QVariant(in.localisation));
    q.bindValue(":dlc", in.dateLimiteCalibration.isValid() ? QVariant(in.dateLimiteCalibration.toString("yyyy-MM-dd")) : nullStr);
    if (hasIdExp) {
        if (in.idExp.isValid() && in.idExp.toInt() > 0) {
            q.bindValue(":idexp", in.idExp.toInt());
        } else {
            q.bindValue(":idexp", QVariant(QMetaType::fromType<int>()));
        }
    }
    q.bindValue(":id", in.id);

    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }
    return true;
}


