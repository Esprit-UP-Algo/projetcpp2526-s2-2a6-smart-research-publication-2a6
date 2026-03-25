#include "equipement.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QRegularExpression>

namespace {

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

    QSqlQuery q;
    q.prepare(
        "SELECT \"equipement_id\", \"nom_equipement\", \"fabricant\", \"numéro_de_modèle\", "
        "       \"date_d_achat\", \"date_dernière_maintenance\", \"date_prochaine_maintenance\", "
        "       \"statut\", \"localisation\", \"date_limite_calibration\", \"Id_exp\" "
        "FROM \"Équipement\" "
        "WHERE (:fab IS NULL OR :fab = '' OR LOWER(\"fabricant\") LIKE '%' || LOWER(:fab) || '%') "
        "  AND (:nom IS NULL OR :nom = '' OR LOWER(\"nom_equipement\") LIKE '%' || LOWER(:nom) || '%') "
        "  AND (:stat IS NULL OR :stat = '' OR \"statut\" = :stat) "
        "  AND (:loc IS NULL OR :loc = '' OR LOWER(\"localisation\") LIKE '%' || LOWER(:loc) || '%') "
        "ORDER BY CASE WHEN LOWER(\"statut\") LIKE '%actif%' THEN 1 "
        "              WHEN LOWER(\"statut\") LIKE '%hors%' OR LOWER(\"statut\") LIKE '%service%' THEN 2 "
        "              WHEN LOWER(\"statut\") LIKE '%archive%' OR LOWER(\"statut\") LIKE '%rchiv%' THEN 3 "
        "              ELSE 9 END, "
        "         \"date_prochaine_maintenance\" ASC NULLS LAST, \"nom_equipement\"");

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
    QSqlQuery q;
    q.prepare(
        "SELECT \"nom_equipement\", \"fabricant\", \"numéro_de_modèle\", "
        "       \"date_d_achat\", \"date_dernière_maintenance\", \"date_prochaine_maintenance\", "
        "       \"statut\", \"localisation\", \"date_limite_calibration\", \"Id_exp\" "
        "FROM \"Équipement\" WHERE \"equipement_id\" = :id");
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
    if (in.nomEquipement.trimmed().isEmpty()) {
        if (error) *error = "nom_equipement est obligatoire.";
        return false;
    }

    int id = in.id;
    if (id <= 0) {
        id = nextEquipementId(error);
        if (id <= 0) return false;
    }

    QSqlQuery q;
    q.prepare(
        "INSERT INTO \"Équipement\" "
        "(\"equipement_id\", \"nom_equipement\", \"fabricant\", \"numéro_de_modèle\", \"date_d_achat\", "
        " \"date_dernière_maintenance\", \"date_prochaine_maintenance\", \"statut\", "
        " \"localisation\", \"date_limite_calibration\", \"Id_exp\") "
        "VALUES (:id, :nom, :fab, :mod, TO_DATE(:da,'YYYY-MM-DD'), "
        "        TO_DATE(:ddm,'YYYY-MM-DD'), TO_DATE(:dpm,'YYYY-MM-DD'), :stat, "
        "        :loc, TO_DATE(:dlc,'YYYY-MM-DD'), :idexp)");

    auto nullStr = QVariant(QMetaType::fromType<QString>());
    const QString dbStatus = toDbStatus(in.statut);

    q.bindValue(":id", id);
    q.bindValue(":nom", in.nomEquipement.trimmed());
    q.bindValue(":fab", in.fabricant.isEmpty() ? nullStr : QVariant(in.fabricant));
    q.bindValue(":mod", in.numeroModele.isEmpty() ? nullStr : QVariant(in.numeroModele));
    q.bindValue(":da",  in.dateAchat.isValid() ? QVariant(in.dateAchat.toString("yyyy-MM-dd")) : nullStr);
    q.bindValue(":ddm", in.dateDerniereMaintenance.isValid() ? QVariant(in.dateDerniereMaintenance.toString("yyyy-MM-dd")) : nullStr);
    q.bindValue(":dpm", in.dateProchaineMaintenance.isValid() ? QVariant(in.dateProchaineMaintenance.toString("yyyy-MM-dd")) : nullStr);
    q.bindValue(":stat", dbStatus.isEmpty() ? QVariant("Actif") : QVariant(dbStatus));
    q.bindValue(":loc", in.localisation.isEmpty() ? nullStr : QVariant(in.localisation));
    q.bindValue(":dlc", in.dateLimiteCalibration.isValid() ? QVariant(in.dateLimiteCalibration.toString("yyyy-MM-dd")) : nullStr);
    q.bindValue(":idexp", (in.idExp.isNull() || !in.idExp.isValid()) ? QVariant(QMetaType::fromType<int>()) : QVariant(in.idExp.toInt()));

    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }
    return true;
}

bool EquipementCrud::updateEquipement(const EquipementRecord& in, QString* error)
{
    if (in.id <= 0) {
        if (error) *error = "equipement_id invalide.";
        return false;
    }
    if (in.nomEquipement.trimmed().isEmpty()) {
        if (error) *error = "nom_equipement est obligatoire.";
        return false;
    }

    QSqlQuery q;
    q.prepare(
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
        "WHERE \"equipement_id\" = :id");

    auto nullStr = QVariant(QMetaType::fromType<QString>());
    const QString dbStatus = toDbStatus(in.statut);

    q.bindValue(":nom", in.nomEquipement.trimmed());
    q.bindValue(":fab", in.fabricant.isEmpty() ? nullStr : QVariant(in.fabricant));
    q.bindValue(":mod", in.numeroModele.isEmpty() ? nullStr : QVariant(in.numeroModele));
    q.bindValue(":da",  in.dateAchat.isValid() ? QVariant(in.dateAchat.toString("yyyy-MM-dd")) : nullStr);
    q.bindValue(":ddm", in.dateDerniereMaintenance.isValid() ? QVariant(in.dateDerniereMaintenance.toString("yyyy-MM-dd")) : nullStr);
    q.bindValue(":dpm", in.dateProchaineMaintenance.isValid() ? QVariant(in.dateProchaineMaintenance.toString("yyyy-MM-dd")) : nullStr);
    q.bindValue(":stat", dbStatus.isEmpty() ? QVariant("Actif") : QVariant(dbStatus));
    q.bindValue(":loc", in.localisation.isEmpty() ? nullStr : QVariant(in.localisation));
    q.bindValue(":dlc", in.dateLimiteCalibration.isValid() ? QVariant(in.dateLimiteCalibration.toString("yyyy-MM-dd")) : nullStr);
    q.bindValue(":idexp", (in.idExp.isNull() || !in.idExp.isValid()) ? QVariant(QMetaType::fromType<int>()) : QVariant(in.idExp.toInt()));
    q.bindValue(":id", in.id);

    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }
    return true;
}
