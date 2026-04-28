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

    // One-time migration: clear PROJET_AFFECTE in Employés when the referenced project no longer exists
    bool clearStaleProjetAffecte(QString* error = nullptr);

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

    // ── Analyse budgétaire complète (3 onglets) ──────────────────────────
    // Onglet 1 : Vue comparative  — bar chart horizontal par projet
    // Onglet 2 : Ventilation interne — donut chart par domaine de recherche
    // Onglet 3 : Prévu vs Réel    — grouped bar chart (budget alloué vs dépensé estimé)
    static void showAnalyseBudgetaire(QWidget* parent = nullptr);

    // ── Statistique : Répartition des projets par statut (Pie Chart) ──
    // Queries the DB and returns { statut -> count }
    QMap<QString,int> loadStatutStats(QString* error = nullptr);

    // Builds and shows the pie-chart dialog (self-contained)
    static void showStatutChart(QWidget* parent = nullptr);

    // ── Santé du Projet (Radar Chart) ────────────────────────────────
    // Prices (in dinars) used to compute the "budget spent" axis
    static constexpr double PRICE_EXPERIENCE  = 500.0;  // cost per experience used
    static constexpr double PRICE_ECHANTILLON = 150.0;  // cost per biosample used
    static constexpr double PRICE_EQUIPEMENT  = 300.0;  // cost per equipement used

    // Maximum publications considered "perfect" for the impact axis
    static constexpr int    MAX_PUBS_REFERENCE = 10;

    struct ProjetSante {
        int idProjet        = 0;
        QString nomProjet;

        // Individual axis scores (0–100)
        double scorebudget      = 0.0;  // Avancement budgétaire
        double scoreDelais      = 0.0;  // Respect des délais
        double scoreEthique     = 0.0;  // Approbation éthique
        double scoreImpact      = 0.0;  // Impact scientifique
        double scoreEquipe      = 0.0;  // Disponibilité de l'équipe

        double scoreGlobal      = 0.0;  // Weighted average (0–100)

        QString errorMsg;               // Non-empty if DB query failed
    };

    // Compute health score for a given project ID
    ProjetSante computeProjetSante(int idProjet, QString* error = nullptr);

    // Build and show the radar dialog for a given project ID
    static void showSanteRadar(int idProjet, QWidget* parent = nullptr);

    // ── Statistique : Évolution du projet dans le temps (Line Chart) ──
    // Shows a project-picker then plots cumulative Expériences, BioSamples,
    // Publications over the project's month timeline.
    static void showEvolutionChart(QWidget* parent = nullptr);

    // ── Métier Avancé : Milestone Tracker ────────────────────────────
    static void showMilestoneTracker(QWidget* parent = nullptr);

    // ── Métier Avancé : Estimation Réaliste ──────────────────────────
    static void showEstimationRealiste(QWidget* parent = nullptr);

    // ── Métier Avancé : Collaborateurs Suggérés ───────────────────────
    static void showCollaborateursSuggeres(QWidget* parent = nullptr);

    // ── Métier Avancé : Spécialisations Manquantes ────────────────────
    static void showSpecialisationsManquantes(QWidget* parent = nullptr);

    // ── Métier Avancé : Risques Probables ───────────────────
    static void showRisquesProbables(QWidget* parent = nullptr);
    // ── Métier Avancé : Analyse Intelligente
    // Reads project statistics and produces a textual analytical summary
    static void showAnalyseIntelligente(QWidget* parent = nullptr);

    // ── Rapport Financier Trimestriel (Excel Export) ──────────
    // Generates a comprehensive quarterly financial report in .xlsx format
    // Quarter: 1-4, Year: e.g. 2025
    static void generateFinancialReport(int quarter, int year, QWidget* parent = nullptr);
};

#endif // GESTPROJ_H
