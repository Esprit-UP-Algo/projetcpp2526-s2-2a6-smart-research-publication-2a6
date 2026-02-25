#ifndef CRUDEBIOSIMPLE_H
#define CRUDEBIOSIMPLE_H

#include <QString>
#include <QDate>
#include <QMap>
#include <QVector>
#include <QPair>
#include <QList>

class QTableWidget;

// ── Sample status (integer values must stay in sync with ExpireStatus in integration.cpp) ──
enum class BioStatus { Ok = 0, Soon = 1, Expired = 2, HighRisk = 3 };

// ── One record of the biosample table ──
struct BioSample {
    QString reference;       // REFERENCE_ECHANTILLON
    QString type;            // TYPE_ECHANTILLON
    QString organisme;       // ORGANISME_SOURCE
    QString emplacement;     // EMPLACEMENT_DE_STOCKAGE
    QString temperature;     // TEMPERATURE_DE_STOCKAGE
    int     quantite = 0;    // QUANTITE_RESTANTE
    QDate   dateCollecte;    // DATE_DE_COLLECTE
    QDate   dateExpiration;  // DATE_EXPIRATION
    QString niveauDanger;    // NIVEAU_DE_DANGEROSITE
    int     idProjet = 0;    // ID_PROJET (optional FK)
    bool isEmpty() const { return reference.isEmpty(); }
};

// ── CRUD class for the biosample table ──
class CrudeBioSimple
{
public:
    CrudeBioSimple();

    // ── CRUD ──────────────────────────────────────────────
    bool      add   (const BioSample& s);
    bool      update(const BioSample& s);
    bool      remove(const QString& reference);
    BioSample get   (const QString& reference);

    // ── Load all rows into a QTableWidget ─────────────────
    void loadAll(QTableWidget* table);

    // ── Statistics ────────────────────────────────────────
    QMap<QString, int>            countByType();
    QVector<QPair<int, QString>>  countByMonth();
    int                           totalCount();

    // ── Last error (for UI display) ───────────────────────
    QString lastError() const { return m_lastError; }

private:
    static BioStatus computeStatus(const QDate& expDate, const QString& danger);
    mutable QString m_lastError;
};

#endif // CRUDEBIOSIMPLE_H
