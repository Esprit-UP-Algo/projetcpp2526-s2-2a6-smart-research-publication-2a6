#include "crudexperience.h"
#include <QRegularExpression>

namespace {

QString statusKey(const QString& value)
{
    const QString s = value.toLower();
    if (s.contains("cours"))  return "en_cours";
    if (s.contains("conclu")) return "concluante";
    if (s.contains("ussie"))  return "reussie";
    if (s.contains("chou"))   return "echouee";
    if (s.contains("rchiv"))  return "archivee";
    return QString();
}

QStringList allowedExpStatusesFromDb()
{
    QSqlQuery q;
    q.prepare("SELECT SEARCH_CONDITION_VC FROM USER_CONSTRAINTS WHERE CONSTRAINT_NAME = 'CK_EXP_STAT'");
    if (!q.exec() || !q.next()) {
        return {"En cours", "Concluante", "Réussie", "Échouée", "Archivée"};
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
        return {"En cours", "Concluante", "Réussie", "Échouée", "Archivée"};
    }
    return values;
}

const QStringList& allowedExpStatuses()
{
    static QStringList cached;
    static bool loaded = false;
    if (!loaded) {
        cached = allowedExpStatusesFromDb();
        loaded = true;
    }
    return cached;
}

QString toDbStatus(const QString& uiStatus)
{
    if (uiStatus.isEmpty()) return uiStatus;

    const QStringList values = allowedExpStatuses();
    if (values.contains(uiStatus)) return uiStatus;

    const QString wanted = statusKey(uiStatus);
    if (!wanted.isEmpty()) {
        for (const QString& v : values) {
            if (statusKey(v) == wanted) return v;
        }
    }

    if (values.contains("En cours")) return "En cours";
    if (!values.isEmpty()) return values.first();
    return uiStatus;
}

QString toUiStatus(const QString& dbStatus)
{
    if (dbStatus.isEmpty()) return dbStatus;

    if (dbStatus == "En cours" || dbStatus == "Concluante" || dbStatus == "Réussie"
        || dbStatus == "Échouée" || dbStatus == "Archivée") {
        return dbStatus;
    }

    const QString key = statusKey(dbStatus);
    if (key == "en_cours")   return "En cours";
    if (key == "concluante") return "Concluante";
    if (key == "reussie")    return "Réussie";
    if (key == "echouee")    return "Échouée";
    if (key == "archivee")   return "Archivée";

    return dbStatus;
}

QString normalizeEquipAvailability(const QString& value)
{
    const QString t = value.trimmed().toLower();
    if (t.contains("non")) return "Non disponible";
    if (t.contains("dispon")) return "Disponible";
    return QString();
}

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
    return hasColumn;
}

QString inferEquipAvailabilityFromEquipement(QString* error)
{
    QSqlQuery q;
    q.prepare(
        "SELECT COUNT(*) "
        "FROM \"Équipement\" "
        "WHERE LOWER(\"statut\") LIKE '%actif%' "
        "   OR LOWER(\"statut\") LIKE '%dispon%' "
        "   OR LOWER(\"statut\") LIKE '%op%rationnel%'");

    if (!q.exec() || !q.next()) {
        if (error) *error = q.lastError().text();
        return QString();
    }

    return q.value(0).toInt() > 0 ? "Disponible" : "Non disponible";
}

QString inferEquipAvailabilityForExperience(int experienceId, QString* error)
{
    if (experienceId <= 0 || !equipementHasIdExpColumn()) {
        return QString();
    }

    QSqlQuery q;
    q.prepare(
        "SELECT "
        "  SUM(CASE WHEN LOWER(\"statut\") LIKE '%actif%' "
        "            OR LOWER(\"statut\") LIKE '%dispon%' "
        "            OR LOWER(\"statut\") LIKE '%op%rationnel%' "
        "      THEN 1 ELSE 0 END) AS cnt_dispo, "
        "  COUNT(*) AS cnt_total "
        "FROM \"Équipement\" "
        "WHERE \"Id_exp\" = :id");
    q.bindValue(":id", experienceId);

    if (!q.exec() || !q.next()) {
        if (error) *error = q.lastError().text();
        return QString();
    }

    const int cntDispo = q.value(0).toInt();
    const int cntTotal = q.value(1).toInt();
    if (cntTotal <= 0) return QString();
    return cntDispo > 0 ? QString("Disponible") : QString("Non disponible");
}

}

bool ExperienceCrud::loadProjects(QList<ProjectItem>& out, QString* error)
{
    out.clear();
    QSqlQuery q;
    q.prepare("SELECT \"Id_projet\", \"nom_du_projet\" FROM \"projet\" ORDER BY \"Id_projet\"");
    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }
    while (q.next()) {
        ProjectItem item;
        item.id   = q.value(0).toInt();
        item.name = q.value(1).toString();
        out.push_back(item);
    }
    return true;
}

bool ExperienceCrud::loadExperiences(QList<ExperienceRecord>& out, QString* error)
{
    out.clear();
    QSqlQuery q;
    q.prepare("SELECT \"Id_exp\", \"Titre\", \"Hypothese\", \"Date_Debut\", \"Date_fin\", \"Status\", "
              "\"Disponibilite_Equipement\", \"Resultat\", \"Type_Experience\" "
              "FROM \"Expérience\" ORDER BY \"Id_exp\" DESC");
    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }
    while (q.next()) {
        ExperienceRecord rec;
        rec.id        = q.value(0).toInt();
        rec.titre     = q.value(1).toString();
        rec.hypothese = q.value(2).toString();
        rec.dateDebut = q.value(3).toDate();
        rec.dateFin   = q.value(4).toDate();
        rec.status    = toUiStatus(q.value(5).toString());
        rec.disponibiliteEquipement = normalizeEquipAvailability(q.value(6).toString());
        rec.resultat = q.value(7).toString();
        rec.typeExperience = q.value(8).toString();
        const QString linkedAvail = inferEquipAvailabilityForExperience(rec.id, nullptr);
        if (!linkedAvail.isEmpty()) {
            rec.disponibiliteEquipement = linkedAvail;
        } else if (rec.disponibiliteEquipement.isEmpty()) {
            rec.disponibiliteEquipement = inferEquipAvailabilityFromEquipement(nullptr);
        }
        out.push_back(rec);
    }
    return true;
}

bool ExperienceCrud::fetchExperience(int id, ExperienceRecord& out, QString* error)
{
    QSqlQuery q;
    q.prepare("SELECT \"Titre\", \"Hypothese\", \"Date_Debut\", \"Date_fin\", \"Status\", \"Id_projet\", "
              "\"Disponibilite_Equipement\", \"Resultat\", \"Type_Experience\" "
              "FROM \"Expérience\" WHERE \"Id_exp\" = :id");
    q.bindValue(":id", id);
    if (!q.exec() || !q.next()) {
        if (error) *error = q.lastError().text();
        return false;
    }
    out.id        = id;
    out.titre     = q.value(0).toString();
    out.hypothese = q.value(1).toString();
    out.dateDebut = q.value(2).toDate();
    out.dateFin   = q.value(3).toDate();
    out.status    = toUiStatus(q.value(4).toString());
    out.projetId  = q.value(5);
    out.disponibiliteEquipement = normalizeEquipAvailability(q.value(6).toString());
    out.resultat = q.value(7).toString();
    out.typeExperience = q.value(8).toString();
    const QString linkedAvail = inferEquipAvailabilityForExperience(id, nullptr);
    if (!linkedAvail.isEmpty()) {
        out.disponibiliteEquipement = linkedAvail;
    } else if (out.disponibiliteEquipement.isEmpty()) {
        out.disponibiliteEquipement = inferEquipAvailabilityFromEquipement(nullptr);
    }
    return true;
}

bool ExperienceCrud::deleteExperience(int id, QString* error)
{
    // Supprimer d'abord les liens équipement (FK_EQUIPEMENT_EXP)
    QSqlQuery qEq;
    qEq.prepare("DELETE FROM \"Équipement\" WHERE \"Id_exp\" = :id");
    qEq.bindValue(":id", id);
    if (!qEq.exec()) {
        if (error) *error = qEq.lastError().text();
        return false;
    }

    QSqlQuery q;
    q.prepare("DELETE FROM \"Expérience\" WHERE \"Id_exp\" = :id");
    q.bindValue(":id", id);
    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }
    return true;
}

int ExperienceCrud::nextExperienceId(QString* error)
{
    QSqlQuery q;
    if (!q.exec("SELECT NVL(MAX(\"Id_exp\"),0)+1 FROM \"Expérience\"") || !q.next()) {
        if (error) *error = q.lastError().text();
        return -1;
    }
    return q.value(0).toInt();
}

bool ExperienceCrud::insertExperience(const ExperienceRecord& in, QString* error)
{
    int id = in.id;
    if (id <= 0) {
        id = nextExperienceId(error);
        if (id <= 0) return false;
    }

    QSqlQuery q;
    q.prepare("INSERT INTO \"Expérience\" "
              "(\"Id_exp\", \"Titre\", \"Hypothese\", \"Date_Debut\", \"Date_fin\", \"Status\", \"Id_projet\", "
              " \"Disponibilite_Equipement\", \"Resultat\", \"Type_Experience\") "
              "VALUES (:id, :t, :h, TO_DATE(:d,'YYYY-MM-DD'), TO_DATE(:df,'YYYY-MM-DD'), :s, :p, :de, :r, :te)");
    auto nullInt  = QVariant(QMetaType::fromType<int>());
    auto nullStr  = QVariant(QMetaType::fromType<QString>());
    const QString dbStatus = toDbStatus(in.status);
    QString eqAvail = inferEquipAvailabilityForExperience(id, nullptr);
    if (eqAvail.isEmpty()) eqAvail = normalizeEquipAvailability(in.disponibiliteEquipement);
    if (eqAvail.isEmpty()) {
        eqAvail = inferEquipAvailabilityFromEquipement(nullptr);
    }
    q.bindValue(":id", id);
    q.bindValue(":t",  in.titre.isEmpty()     ? nullStr : QVariant(in.titre));
    q.bindValue(":h",  in.hypothese.isEmpty() ? nullStr : QVariant(in.hypothese));
    q.bindValue(":d",  in.dateDebut.isValid() ? QVariant(in.dateDebut.toString("yyyy-MM-dd")) : nullStr);
    q.bindValue(":df", in.dateFin.isValid()   ? QVariant(in.dateFin.toString("yyyy-MM-dd"))   : nullStr);
    q.bindValue(":s",  dbStatus.isEmpty()      ? nullStr : QVariant(dbStatus));
    q.bindValue(":p",  (in.projetId.isNull() || !in.projetId.isValid()) ? nullInt : QVariant(in.projetId.toInt()));
    q.bindValue(":de", eqAvail.isEmpty() ? nullStr : QVariant(eqAvail));
    q.bindValue(":r",  in.resultat.isEmpty() ? nullStr : QVariant(in.resultat));
    q.bindValue(":te", in.typeExperience.isEmpty() ? nullStr : QVariant(in.typeExperience));
    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }
    return true;
}

bool ExperienceCrud::updateExperience(const ExperienceRecord& in, QString* error)
{
    QSqlQuery q;
    q.prepare("UPDATE \"Expérience\" "
              "SET \"Titre\" = :t, \"Hypothese\" = :h, "
              "    \"Date_Debut\" = TO_DATE(:d,'YYYY-MM-DD'), \"Date_fin\" = TO_DATE(:df,'YYYY-MM-DD'), "
              "    \"Status\" = :s, \"Id_projet\" = :p, \"Disponibilite_Equipement\" = :de, "
              "    \"Resultat\" = :r, \"Type_Experience\" = :te "
              "WHERE \"Id_exp\" = :id");
    auto nullInt  = QVariant(QMetaType::fromType<int>());
    auto nullStr  = QVariant(QMetaType::fromType<QString>());
    const QString dbStatus = toDbStatus(in.status);
    QString eqAvail = inferEquipAvailabilityForExperience(in.id, nullptr);
    if (eqAvail.isEmpty()) eqAvail = normalizeEquipAvailability(in.disponibiliteEquipement);
    if (eqAvail.isEmpty()) {
        eqAvail = inferEquipAvailabilityFromEquipement(nullptr);
    }
    q.bindValue(":t",  in.titre.isEmpty()     ? nullStr : QVariant(in.titre));
    q.bindValue(":h",  in.hypothese.isEmpty() ? nullStr : QVariant(in.hypothese));
    q.bindValue(":d",  in.dateDebut.isValid() ? QVariant(in.dateDebut.toString("yyyy-MM-dd")) : nullStr);
    q.bindValue(":df", in.dateFin.isValid()   ? QVariant(in.dateFin.toString("yyyy-MM-dd"))   : nullStr);
    q.bindValue(":s",  dbStatus.isEmpty()      ? nullStr : QVariant(dbStatus));
    q.bindValue(":p",  (in.projetId.isNull() || !in.projetId.isValid()) ? nullInt : QVariant(in.projetId.toInt()));
    q.bindValue(":de", eqAvail.isEmpty() ? nullStr : QVariant(eqAvail));
    q.bindValue(":r",  in.resultat.isEmpty() ? nullStr : QVariant(in.resultat));
    q.bindValue(":te", in.typeExperience.isEmpty() ? nullStr : QVariant(in.typeExperience));
    q.bindValue(":id", in.id);
    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }
    return true;
}

QString ExperienceCrud::suggestedEquipAvailability(QString* error)
{
    QString err;
    const QString v = inferEquipAvailabilityFromEquipement(&err);
    if (!err.isEmpty() && error) {
        *error = err;
    }
    return v.isEmpty() ? QString("Disponible") : v;
}
