#ifndef SIMPLE_H
#define SIMPLE_H

#include <QList>
#include <QMap>
#include <QPair>
#include <QString>
#include "crudexperience.h"

enum class ExpSortKey { None = 0, DateDebut, DateFin, Status, Disponibilite };

class ExperienceSorter
{
public:
    static QList<ExperienceRecord> sort(const QList<ExperienceRecord>& recs, ExpSortKey key);
};

class ExperienceAnalytics
{
public:
    static QMap<QString, int> countByStatus(const QList<ExperienceRecord>& recs);
    static QList<QPair<int, QString>> countByMonth(const QList<ExperienceRecord>& recs);
};

#endif // SIMPLE_H
