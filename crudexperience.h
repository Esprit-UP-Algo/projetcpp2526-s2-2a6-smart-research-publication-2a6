#ifndef CRUDEXPERIENCE_H
#define CRUDEXPERIENCE_H

#include <QString>
#include <QDate>
#include <QVariant>
#include <QList>
#include <QSqlQuery>
#include <QSqlError>

struct ProjectItem {
    int     id   = 0;
    QString name;
};

struct ExperienceRecord {
    int      id        = 0;
    QString  titre;
    QString  hypothese;
    QDate    dateDebut;
    QDate    dateFin;
    QString  status;
    QVariant projetId;   // can be NULL / int
};

class ExperienceCrud
{
public:
    bool loadProjects   (QList<ProjectItem>& out,      QString* error = nullptr);
    bool loadExperiences(QList<ExperienceRecord>& out,  QString* error = nullptr);
    bool fetchExperience(int id, ExperienceRecord& out, QString* error = nullptr);
    bool deleteExperience(int id,                       QString* error = nullptr);
    int  nextExperienceId(                              QString* error = nullptr);
    bool insertExperience(const ExperienceRecord& in,   QString* error = nullptr);
    bool updateExperience(const ExperienceRecord& in,   QString* error = nullptr);
};

#endif // CRUDEXPERIENCE_H
