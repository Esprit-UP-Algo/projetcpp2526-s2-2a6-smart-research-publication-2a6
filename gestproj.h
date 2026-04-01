#ifndef GESTPROJ_H
#define GESTPROJ_H

#include <QString>
#include <QList>
#include <QDate>
#include <QSqlQuery>
#include <QSqlError>

struct ProjetRecord {
    int     idProjet = 0;
    QString nomDuProjet;
    QString domaineDeRecherche;
    QDate   dateDeDebut;
    QDate   dateDeFin;
    double  budget = 0.0;
    QString statut;
    QString sourceDeFinancement;
    QString numeroDApprobationEthique;
    int     nombreDePublications = 0;
};

class GestProjCrud
{
public:
    bool loadProjets(QList<ProjetRecord>& out,
                     QString* error = nullptr,
                     const QString& nom = QString(),
                     const QString& domaine = QString(),
                     const QString& statut = QString());
    bool fetchProjet(int idProjet, ProjetRecord& out, QString* error = nullptr);
    bool deleteProjet(int idProjet, QString* error = nullptr);
    int  nextProjetId(QString* error = nullptr);
    bool insertProjet(const ProjetRecord& in, QString* error = nullptr);
    bool updateProjet(const ProjetRecord& in, QString* error = nullptr);
};

#endif // GESTPROJ_H
