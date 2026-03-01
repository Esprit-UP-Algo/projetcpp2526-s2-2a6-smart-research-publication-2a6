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

}

bool ExperienceCrud::loadProjects(QList<ProjectItem>& out, QString* error)
{
    out.clear();
    QSqlQuery q;
    q.prepare("SELECT ID_PROJET, NOM_DU_PROJET FROM PROJET ORDER BY ID_PROJET");
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
    q.prepare("SELECT ID_EXP, TITRE, HYPOTHESE, DATE_DEBUT, DATE_FIN, STATUS "
              "FROM EXPERIENCE ORDER BY ID_EXP DESC");
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
        out.push_back(rec);
    }
    return true;
}

bool ExperienceCrud::fetchExperience(int id, ExperienceRecord& out, QString* error)
{
    QSqlQuery q;
    q.prepare("SELECT TITRE, HYPOTHESE, DATE_DEBUT, DATE_FIN, STATUS, ID_PROJET "
              "FROM EXPERIENCE WHERE ID_EXP = :id");
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
    return true;
}

bool ExperienceCrud::deleteExperience(int id, QString* error)
{
    QSqlQuery q;
    q.prepare("DELETE FROM EXPERIENCE WHERE ID_EXP = :id");
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
    if (!q.exec("SELECT NVL(MAX(ID_EXP),0)+1 FROM EXPERIENCE") || !q.next()) {
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
    q.prepare("INSERT INTO EXPERIENCE "
              "(ID_EXP, TITRE, HYPOTHESE, DATE_DEBUT, DATE_FIN, STATUS, ID_PROJET) "
              "VALUES (:id, :t, :h, TO_DATE(:d,'YYYY-MM-DD'), TO_DATE(:df,'YYYY-MM-DD'), :s, :p)");
    auto nullInt  = QVariant(QMetaType::fromType<int>());
    auto nullStr  = QVariant(QMetaType::fromType<QString>());
    const QString dbStatus = toDbStatus(in.status);
    q.bindValue(":id", id);
    q.bindValue(":t",  in.titre.isEmpty()     ? nullStr : QVariant(in.titre));
    q.bindValue(":h",  in.hypothese.isEmpty() ? nullStr : QVariant(in.hypothese));
    q.bindValue(":d",  in.dateDebut.isValid() ? QVariant(in.dateDebut.toString("yyyy-MM-dd")) : nullStr);
    q.bindValue(":df", in.dateFin.isValid()   ? QVariant(in.dateFin.toString("yyyy-MM-dd"))   : nullStr);
    q.bindValue(":s",  dbStatus.isEmpty()      ? nullStr : QVariant(dbStatus));
    q.bindValue(":p",  (in.projetId.isNull() || !in.projetId.isValid()) ? nullInt : QVariant(in.projetId.toInt()));
    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }
    return true;
}

bool ExperienceCrud::updateExperience(const ExperienceRecord& in, QString* error)
{
    QSqlQuery q;
    q.prepare("UPDATE EXPERIENCE "
              "SET TITRE = :t, HYPOTHESE = :h, "
              "    DATE_DEBUT = TO_DATE(:d,'YYYY-MM-DD'), DATE_FIN = TO_DATE(:df,'YYYY-MM-DD'), "
              "    STATUS = :s, ID_PROJET = :p "
              "WHERE ID_EXP = :id");
    auto nullInt  = QVariant(QMetaType::fromType<int>());
    auto nullStr  = QVariant(QMetaType::fromType<QString>());
    const QString dbStatus = toDbStatus(in.status);
    q.bindValue(":t",  in.titre.isEmpty()     ? nullStr : QVariant(in.titre));
    q.bindValue(":h",  in.hypothese.isEmpty() ? nullStr : QVariant(in.hypothese));
    q.bindValue(":d",  in.dateDebut.isValid() ? QVariant(in.dateDebut.toString("yyyy-MM-dd")) : nullStr);
    q.bindValue(":df", in.dateFin.isValid()   ? QVariant(in.dateFin.toString("yyyy-MM-dd"))   : nullStr);
    q.bindValue(":s",  dbStatus.isEmpty()      ? nullStr : QVariant(dbStatus));
    q.bindValue(":p",  (in.projetId.isNull() || !in.projetId.isValid()) ? nullInt : QVariant(in.projetId.toInt()));
    q.bindValue(":id", in.id);
    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }
    return true;
}
