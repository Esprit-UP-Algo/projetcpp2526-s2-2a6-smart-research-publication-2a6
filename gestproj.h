#ifndef GESTPROJ_H
#define GESTPROJ_H

#include <QString>
#include <QList>
#include <QDate>
#include <QMap>
#include <QWidget>
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
    QString sourceDeFinancement;   // plain text, single funding source
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

    // Validation — returns empty string if valid, error message otherwise
    QString validateProjet(const ProjetRecord& in, bool isUpdate = false);

    // ── Statistique : Répartition des projets par domaine de recherche ──
    // Queries the DB and returns { domaine -> count }
    QMap<QString,int> loadDomaineStats(QString* error = nullptr);

    // Builds and shows the horizontal bar-chart dialog (self-contained)
    static void showDomaineChart(QWidget* parent = nullptr);

    // ── Statistique : Distribution des budgets par projet (actifs) ──
    // Queries the DB and returns { nomProjet -> budget } for active projects
    QMap<QString,double> loadBudgetStats(QString* error = nullptr);

    // Builds and shows the vertical bar-chart dialog for budget distribution
    static void showBudgetChart(QWidget* parent = nullptr);

    // ── Statistique : Distribution du budget par domaine de recherche ──
    // Queries the DB and returns { domaine -> totalBudget }
    QMap<QString,double> loadDomaineBudgetStats(QString* error = nullptr);

    // Builds and shows the horizontal bar-chart dialog for budget per domaine
    static void showDomaineBudgetChart(QWidget* parent = nullptr);

    // ── Statistique : Répartition des projets par statut (Pie Chart) ──
    // Queries the DB and returns { statut -> count }
    QMap<QString,int> loadStatutStats(QString* error = nullptr);

    // Builds and shows the pie-chart dialog (self-contained)
    static void showStatutChart(QWidget* parent = nullptr);
};

#endif // GESTPROJ_H
