#ifndef BASICBIO_H
#define BASICBIO_H

#include <QString>
#include <QStringList>
#include <QList>

// ── Full info for one BioSample (with project name resolved) ──
struct BasicBioInfo {
    QString reference;
    QString type;
    QString organisme;    // "Organisme_source"
    QString projet;       // resolved from "projet"."nom_du_projet"
    QString bslLevel;     // "Niveau_de_dangerosité"
    int     quantite = 0;
    QString congelateur;  // parsed from emplacement ("Cong:XX/Etag:YY")
    QString etagere;      // parsed from emplacement
    QString temperature;
};

// ── Static helpers for the Localisation & Stockage page ──
class BasicBio
{
public:
    // Returns distinct congélateur names present in BioSample
    static QStringList loadCongelateurs();

    // Returns distinct étagères for a given congélateur
    static QStringList loadEtageres(const QString& cong);

    // Returns all samples for a congélateur (and optionally a specific étagère)
    static QList<BasicBioInfo> loadSamples(const QString& cong,
                                           const QString& etagere = QString());

private:
    static QString parseCong(const QString& emp);
    static QString parseEtag(const QString& emp);
};

#endif // BASICBIO_H
