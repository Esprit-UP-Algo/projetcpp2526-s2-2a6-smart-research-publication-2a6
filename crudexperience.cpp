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

    if (uiStatus == "Réussie" || uiStatus == "Reussie") {
        return "Concluante";
    }

    const QStringList values = allowedExpStatuses();
    if (values.contains(uiStatus)) return uiStatus;

    const QString wanted = statusKey(uiStatus);
    if (!wanted.isEmpty()) {
        for (const QString& v : values) {
            if (statusKey(v) == wanted) return v;
        }
    }

    if (uiStatus == "Réussie" && values.contains("Concluante")) {
        return "Concluante";
    }

    if (values.contains("En cours")) {
        return "En cours";
    }

    for (const QString& v : values) {
        if (statusKey(v) == "en_cours") return v;
    }

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
    q.prepare("SELECT PROJET_ID, NOM_PROJET FROM PROJETS ORDER BY PROJET_ID");
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
    q.prepare("SELECT EXPERIENCE_ID, TITRE, HYPOTHESE, DATE_DEBUT, DATE_FIN, STATUT "
              "FROM EXPERIENCES ORDER BY EXPERIENCE_ID DESC");
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
    q.prepare("SELECT TITRE, HYPOTHESE, DATE_DEBUT, DATE_FIN, STATUT, PROJET_ID "
              "FROM EXPERIENCES WHERE EXPERIENCE_ID = :id");
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
    q.prepare("DELETE FROM EXPERIENCES WHERE EXPERIENCE_ID = :id");
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
    if (!q.exec("SELECT NVL(MAX(EXPERIENCE_ID),0)+1 FROM EXPERIENCES") || !q.next()) {
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
    q.prepare("INSERT INTO EXPERIENCES "
              "(EXPERIENCE_ID, TITRE, HYPOTHESE, DATE_DEBUT, DATE_FIN, STATUT, PROJET_ID) "
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
    q.prepare("UPDATE EXPERIENCES "
              "SET TITRE = :t, HYPOTHESE = :h, "
              "    DATE_DEBUT = TO_DATE(:d,'YYYY-MM-DD'), DATE_FIN = TO_DATE(:df,'YYYY-MM-DD'), "
              "    STATUT = :s, PROJET_ID = :p "
              "WHERE EXPERIENCE_ID = :id");
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
