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
    q.prepare("SELECT EMPLOYE_ID, NOM, PRENOM FROM EMPLOYES ORDER BY NOM, PRENOM");
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
        "SELECT EQUIPEMENT_ID, NOM_EQUIPEMENT, FABRICANT, NUMERO_MODELE, "
        "       DATE_ACHAT, DATE_DERNIERE_MAINTENANCE, DATE_PROCHAINE_MAINTENANCE, "
        "       STATUT, LOCALISATION, DATE_LIMITE_CALIBRATION, RESPONSABLE_ID "
        "FROM EQUIPEMENTS "
        "WHERE (:fab IS NULL OR :fab = '' OR LOWER(FABRICANT) LIKE '%' || LOWER(:fab) || '%') "
        "  AND (:nom IS NULL OR :nom = '' OR LOWER(NOM_EQUIPEMENT) LIKE '%' || LOWER(:nom) || '%') "
        "  AND (:stat IS NULL OR :stat = '' OR STATUT = :stat) "
        "  AND (:loc IS NULL OR :loc = '' OR LOWER(LOCALISATION) LIKE '%' || LOWER(:loc) || '%') "
        "ORDER BY CASE WHEN LOWER(STATUT) LIKE '%actif%' THEN 1 "
        "              WHEN LOWER(STATUT) LIKE '%hors%' OR LOWER(STATUT) LIKE '%service%' THEN 2 "
        "              WHEN LOWER(STATUT) LIKE '%archive%' OR LOWER(STATUT) LIKE '%rchiv%' THEN 3 "
        "              ELSE 9 END, "
        "         DATE_PROCHAINE_MAINTENANCE ASC NULLS LAST, NOM_EQUIPEMENT");

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
        rec.responsableId            = q.value(10);
        out.push_back(rec);
    }
    return true;
}

bool EquipementCrud::fetchEquipement(int id, EquipementRecord& out, QString* error)
{
    QSqlQuery q;
    q.prepare(
        "SELECT NOM_EQUIPEMENT, FABRICANT, NUMERO_MODELE, "
        "       DATE_ACHAT, DATE_DERNIERE_MAINTENANCE, DATE_PROCHAINE_MAINTENANCE, "
        "       STATUT, LOCALISATION, DATE_LIMITE_CALIBRATION, RESPONSABLE_ID "
        "FROM EQUIPEMENTS WHERE EQUIPEMENT_ID = :id");
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
    out.responsableId            = q.value(9);
    return true;
}

bool EquipementCrud::deleteEquipement(int id, QString* error)
{
    QSqlQuery q;
    q.prepare("DELETE FROM EQUIPEMENTS WHERE EQUIPEMENT_ID = :id");
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
    if (!q.exec("SELECT NVL(MAX(EQUIPEMENT_ID),0)+1 FROM EQUIPEMENTS") || !q.next()) {
        if (error) *error = q.lastError().text();
        return -1;
    }
    return q.value(0).toInt();
}

bool EquipementCrud::insertEquipement(const EquipementRecord& in, QString* error)
{
    if (in.nomEquipement.trimmed().isEmpty()) {
        if (error) *error = "NOM_EQUIPEMENT est obligatoire.";
        return false;
    }

    int id = in.id;
    if (id <= 0) {
        id = nextEquipementId(error);
        if (id <= 0) return false;
    }

    QSqlQuery q;
    q.prepare(
        "INSERT INTO EQUIPEMENTS "
        "(EQUIPEMENT_ID, NOM_EQUIPEMENT, FABRICANT, NUMERO_MODELE, DATE_ACHAT, "
        " DATE_DERNIERE_MAINTENANCE, DATE_PROCHAINE_MAINTENANCE, STATUT, "
        " LOCALISATION, DATE_LIMITE_CALIBRATION, RESPONSABLE_ID) "
        "VALUES (:id, :nom, :fab, :mod, TO_DATE(:da,'YYYY-MM-DD'), "
        "        TO_DATE(:ddm,'YYYY-MM-DD'), TO_DATE(:dpm,'YYYY-MM-DD'), :stat, "
        "        :loc, TO_DATE(:dlc,'YYYY-MM-DD'), :resp)");

    auto nullInt = QVariant(QMetaType::fromType<int>());
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
    q.bindValue(":resp", (in.responsableId.isNull() || !in.responsableId.isValid()) ? nullInt : QVariant(in.responsableId.toInt()));

    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }
    return true;
}

bool EquipementCrud::updateEquipement(const EquipementRecord& in, QString* error)
{
    if (in.id <= 0) {
        if (error) *error = "EQUIPEMENT_ID invalide.";
        return false;
    }
    if (in.nomEquipement.trimmed().isEmpty()) {
        if (error) *error = "NOM_EQUIPEMENT est obligatoire.";
        return false;
    }

    QSqlQuery q;
    q.prepare(
        "UPDATE EQUIPEMENTS "
        "SET NOM_EQUIPEMENT = :nom, "
        "    FABRICANT = :fab, "
        "    NUMERO_MODELE = :mod, "
        "    DATE_ACHAT = TO_DATE(:da,'YYYY-MM-DD'), "
        "    DATE_DERNIERE_MAINTENANCE = TO_DATE(:ddm,'YYYY-MM-DD'), "
        "    DATE_PROCHAINE_MAINTENANCE = TO_DATE(:dpm,'YYYY-MM-DD'), "
        "    STATUT = :stat, "
        "    LOCALISATION = :loc, "
        "    DATE_LIMITE_CALIBRATION = TO_DATE(:dlc,'YYYY-MM-DD'), "
        "    RESPONSABLE_ID = :resp "
        "WHERE EQUIPEMENT_ID = :id");

    auto nullInt = QVariant(QMetaType::fromType<int>());
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
    q.bindValue(":resp", (in.responsableId.isNull() || !in.responsableId.isValid()) ? nullInt : QVariant(in.responsableId.toInt()));
    q.bindValue(":id", in.id);

    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }
    return true;
}
