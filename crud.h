#ifndef CRUD_H
#define CRUD_H

#include <QDate>
#include <QList>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

struct ProjectItem {
    int id = -1;
    QString name;
};

struct ExperienceRecord {
    int id = -1;
    QString titre;
    QString hypothese;
    QDate dateDebut;
    QDate dateFin;
    QString status;
    QVariant projetId;
};

class ExperienceCrud
{
public:
    static bool loadProjects(QList<ProjectItem>& out, QString* error = nullptr);
    static bool loadExperiences(QList<ExperienceRecord>& out, QString* error = nullptr);
    static bool fetchExperience(int id, ExperienceRecord& out, QString* error = nullptr);
    static bool deleteExperience(int id, QString* error = nullptr);
    static int nextExperienceId(QString* error = nullptr);
    static bool insertExperience(const ExperienceRecord& in, QString* error = nullptr);
    static bool updateExperience(const ExperienceRecord& in, QString* error = nullptr);
};

#endif
