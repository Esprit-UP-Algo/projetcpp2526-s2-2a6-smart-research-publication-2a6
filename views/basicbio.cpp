#include "basicbio.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

// ── Emplacement parsing ───────────────────────────────────────
QString BasicBio::parseCong(const QString& emp)
{
    int s = emp.indexOf("Cong:") + 5;
    int e = emp.indexOf("/", s);
    return (s >= 5 && e > s) ? emp.mid(s, e - s) : QString();
}

QString BasicBio::parseEtag(const QString& emp)
{
    int s = emp.indexOf("Etag:") + 5;
    return (s >= 5) ? emp.mid(s) : QString();
}

// ── loadCongelateurs ─────────────────────────────────────────
QStringList BasicBio::loadCongelateurs()
{
    QStringList result;
    QSqlQuery q;
    q.prepare(
        "SELECT DISTINCT \"Emplacement_de_stockage\" "
        "FROM \"BioSample\" "
        "WHERE \"Emplacement_de_stockage\" LIKE 'Cong:%' "
        "ORDER BY \"Emplacement_de_stockage\"");
    if (!q.exec()) {
        qDebug() << "[BasicBio::loadCongelateurs]" << q.lastError().text();
        return result;
    }
    QStringList seen;
    while (q.next()) {
        QString c = parseCong(q.value(0).toString());
        if (!c.isEmpty() && !seen.contains(c)) {
            seen.append(c);
            result.append(c);
        }
    }
    return result;
}

// ── loadEtageres ─────────────────────────────────────────────
QStringList BasicBio::loadEtageres(const QString& cong)
{
    QStringList result;
    QSqlQuery q;
    q.prepare(
        "SELECT DISTINCT \"Emplacement_de_stockage\" "
        "FROM \"BioSample\" "
        "WHERE \"Emplacement_de_stockage\" LIKE :pat "
        "ORDER BY \"Emplacement_de_stockage\"");
    q.bindValue(":pat", QString("Cong:%1/%").arg(cong));
    if (!q.exec()) {
        qDebug() << "[BasicBio::loadEtageres]" << q.lastError().text();
        return result;
    }
    QStringList seen;
    while (q.next()) {
        QString emp = q.value(0).toString();
        if (parseCong(emp) != cong) continue;
        QString e = parseEtag(emp);
        if (!e.isEmpty() && !seen.contains(e)) {
            seen.append(e);
            result.append(e);
        }
    }
    return result;
}

// ── loadSamples ──────────────────────────────────────────────
QList<BasicBioInfo> BasicBio::loadSamples(const QString& cong, const QString& etagere)
{
    QList<BasicBioInfo> result;

    QSqlQuery q;
    q.prepare(
        "SELECT b.\"Reference_de_léchantillon\", "
        "       b.\"Type_déchantillon\", "
        "       b.\"Organisme_source\", "
        "       p.\"nom_du_projet\", "
        "       b.\"Niveau_de_dangerosité\", "
        "       b.\"Quantité_restante\", "
        "       b.\"Emplacement_de_stockage\", "
        "       b.\"Température_de_stockage\" "
        "FROM \"BioSample\" b "
        "LEFT JOIN \"projet\" p ON b.\"Id_projet\" = p.\"Id_projet\" "
        "WHERE b.\"Emplacement_de_stockage\" LIKE :pat "
        "ORDER BY b.\"Reference_de_léchantillon\"");
    q.bindValue(":pat", QString("Cong:%1/%").arg(cong));

    if (!q.exec()) {
        qDebug() << "[BasicBio::loadSamples]" << q.lastError().text();
        return result;
    }

    while (q.next()) {
        QString emp = q.value(6).toString();  // Emplacement_de_stockage
        if (parseCong(emp) != cong) continue;
        QString etag = parseEtag(emp);
        if (!etagere.isEmpty() && etag != etagere) continue;

        BasicBioInfo info;
        info.reference   = q.value(0).toString();
        info.type        = q.value(1).toString();
        info.organisme   = q.value(2).toString();
        info.projet      = q.value(3).toString();
        info.bslLevel    = q.value(4).toString();
        info.quantite    = q.value(5).toInt();  // Quantité_restante
        info.congelateur = cong;
        info.etagere     = etag;
        info.temperature = q.value(7).toString();
        result.append(info);
    }

    return result;
}
