#include "simple.h"

#include <algorithm>

namespace {

QString statusKey(const QString& value)
{
    const QString s = value.toLower();
    if (s.contains("cours"))   return "en_cours";
    if (s.contains("suspend")) return "suspendue";
    if (s.contains("chou"))    return "suspendue";
    if (s.contains("conclu"))  return "terminee";
    if (s.contains("reuss"))   return "terminee";
    if (s.contains("réuss"))   return "terminee";
    if (s.contains("term"))    return "terminee";
    if (s.contains("termin"))  return "terminee";
    if (s.contains("attente")) return "en_attente";
    if (s.contains("archiv"))  return "en_attente";
    return QString();
}

int statusRank(const QString& status)
{
    const QString key = statusKey(status);
    if (key == "en_cours")   return 0;
    if (key == "suspendue")  return 1;
    if (key == "terminee")   return 2;
    if (key == "en_attente") return 3;
    return 4;
}

QDate safeDate(const QDate& d)
{
    if (d.isValid()) return d;
    return QDate(9999, 12, 31);
}

QString normalizedStatusLabel(const QString& status)
{
    const QString key = statusKey(status);
    if (key == "en_cours")   return "En cours";
    if (key == "suspendue")  return "Suspendue";
    if (key == "terminee")   return "Terminée";
    if (key == "en_attente") return "En attente";
    return "En attente";
}

int disponibiliteRank(const QString& disponibilite)
{
    const QString d = disponibilite.trimmed().toLower();
    if (d.contains("non")) return 1;
    if (d.contains("dispon")) return 0;
    return 2;
}

}

QList<ExperienceRecord> ExperienceSorter::sort(const QList<ExperienceRecord>& recs, ExpSortKey key)
{
    if (key == ExpSortKey::None) return recs;

    QList<ExperienceRecord> out = recs;

    std::stable_sort(out.begin(), out.end(), [=](const ExperienceRecord& a, const ExperienceRecord& b){
        if (key == ExpSortKey::Status) {
            const int ra = statusRank(a.status);
            const int rb = statusRank(b.status);
            if (ra != rb) return ra < rb;
            return safeDate(a.dateDebut) > safeDate(b.dateDebut);
        }

        if (key == ExpSortKey::Disponibilite) {
            const int ra = disponibiliteRank(a.disponibiliteEquipement);
            const int rb = disponibiliteRank(b.disponibiliteEquipement);
            if (ra != rb) return ra < rb;
            return safeDate(a.dateDebut) > safeDate(b.dateDebut);
        }

        if (key == ExpSortKey::DateFin) {
            const QDate da = safeDate(a.dateFin);
            const QDate db = safeDate(b.dateFin);
            if (da != db) return da < db;
            return safeDate(a.dateDebut) < safeDate(b.dateDebut);
        }

        const QDate da = safeDate(a.dateDebut);
        const QDate db = safeDate(b.dateDebut);
        if (da != db) return da > db;
        return safeDate(a.dateFin) > safeDate(b.dateFin);
    });

    return out;
}

QMap<QString, int> ExperienceAnalytics::countByStatus(const QList<ExperienceRecord>& recs)
{
    QMap<QString, int> counts;
    counts["En cours"] = 0;
    counts["Suspendue"] = 0;
    counts["Terminée"] = 0;
    counts["En attente"] = 0;

    for (const ExperienceRecord& rec : recs) {
        const QString label = normalizedStatusLabel(rec.status);
        counts[label] = counts.value(label) + 1;
    }

    return counts;
}

QList<QPair<int, QString>> ExperienceAnalytics::countByMonth(const QList<ExperienceRecord>& recs)
{
    static const QStringList months = {
        "Jan", "Fév", "Mar", "Avr", "Mai", "Juin",
        "Juil", "Août", "Sep", "Oct", "Nov", "Déc"
    };

    int counts[12] = {0};
    for (const ExperienceRecord& rec : recs) {
        if (!rec.dateDebut.isValid()) continue;
        const int m = rec.dateDebut.month();
        if (m >= 1 && m <= 12) counts[m - 1] += 1;
    }

    QList<QPair<int, QString>> out;
    for (int i = 0; i < 12; ++i) {
        out.push_back({counts[i], months[i]});
    }
    return out;
}
