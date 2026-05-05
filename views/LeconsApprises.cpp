// ─────────────────────────────────────────────────────────────
//  LeconsApprises.cpp
//  Feature: Leçons Apprises — Gestion Projet (SmartVision)
//
//  Depends on:
//    - gestproj.h  (ProjetRecord, GestProjCrud)
//    - Oracle DB already connected (QSqlQuery works out of the box)
//    - GROQ_API_KEY / GROQ_API_URL / GROQ_API_MODEL already
//      defined by the project (same constants used in gestproj.cpp)
//    - Qt modules: Core, Sql, Network, Widgets, PrintSupport
//
//  To add to your .pro:
//    SOURCES += LeconsApprises.cpp
//    HEADERS += LeconsApprises.h
//    QT      += printsupport
// ─────────────────────────────────────────────────────────────

#include "LeconsApprises.h"
#include "gestproj.h"

// Qt — Widgets
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QListWidget>
#include <QListWidgetItem>
#include <QFrame>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>

// Qt — Network
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QSslError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// Qt — SQL
#include <QSqlQuery>
#include <QSqlError>

// Qt — Misc
#include <QDate>
#include <QRegularExpression>
#include <QMap>
#include <QFont>

// Qt — Print / PDF
#include <QPrinter>
#include <QTextDocument>
#include <QPageSize>
#include <QPageLayout>
#include <QMarginsF>

// STL
#include <algorithm>
#include <cmath>

// ── API constants — pulled from the shared project header ────
#include "chatbotbiosimple.h"
#ifndef GROQ_API_URL
#  define GROQ_API_URL   "https://api.groq.com/openai/v1/chat/completions"
#endif
#ifndef GROQ_API_KEY
#  define GROQ_API_KEY   "gsk_placeholder_replace_with_real_key"
#endif
#ifndef GROQ_API_MODEL
#  define GROQ_API_MODEL "llama3-8b-8192"
#endif

// ═════════════════════════════════════════════════════════════
//  SECTION 1 — ANALYSIS ENGINE
// ═════════════════════════════════════════════════════════════

LeconsApprisesResult LeconsApprises::analyze(int idProjetCourant)
{
    LeconsApprisesResult result;

    // ── Table names with special characters ──────────────────
    const QString expTable = QString::fromUtf8("Exp\xc3\xa9rience");
    const QString empTable = QString::fromUtf8("Employ\xc3\xa9s");
    const QString termSt   = QString::fromUtf8("termin\xc3\xa9");
    const QString termSt2  = QString::fromUtf8("termin\xc3\xa9" "e");

    // ══════════════════════════════════════════════════════════
    //  QUERY 1 — Load current project
    // ══════════════════════════════════════════════════════════
    ProjetRecord current;
    {
        GestProjCrud crud;
        QString err;
        if (!crud.fetchProjet(idProjetCourant, current, &err)) {
            result.messageVide =
                QString("Impossible de charger le projet (id=%1): %2")
                    .arg(idProjetCourant).arg(err);
            return result;
        }
    }
    result.domaine = current.domaineDeRecherche.trimmed();

    // ══════════════════════════════════════════════════════════
    //  QUERY 2 — Find similar completed projects
    // ══════════════════════════════════════════════════════════
    QList<ProjetRecord> similarProjects;
    {
        GestProjCrud crud;
        QString err;
        QList<ProjetRecord> all;
        crud.loadProjets(all, &err);

        for (const ProjetRecord& p : all) {
            if (p.idProjet == idProjetCourant) continue;
            if (p.domaineDeRecherche.trimmed().compare(
                    current.domaineDeRecherche.trimmed(),
                    Qt::CaseInsensitive) != 0)
                continue;

            // Accept "Terminé" in any casing / accent variant
            QString st = p.statut.trimmed().toLower();
            // Strip accents for comparison
            st.replace(QString::fromUtf8("\xc3\xa9"), "e"); // é → e
            if (st == "termine" || st == "terminé")
                similarProjects.append(p);
        }
    }

    result.nombreProjetsSimilaires = similarProjects.size();

    if (similarProjects.isEmpty()) {
        result.donneesSuffisantes = false;
        result.messageVide = QString(
                                 "Aucun projet similaire terminé trouvé dans le domaine \"%1\".\n\n"
                                 "Les leçons apprises seront disponibles une fois que des projets "
                                 "similaires auront été complétés.\n\n"
                                 "En attendant, vous pouvez utiliser la recherche IA ci-dessous "
                                 "pour obtenir des informations sur des projets mondiaux dans ce domaine.")
                                 .arg(result.domaine);
        result.donneesSuffisantes = false;
        return result;
    }

    // ══════════════════════════════════════════════════════════
    //  QUERY 3 — Per-project statistics
    // ══════════════════════════════════════════════════════════
    struct SimilarStats {
        int         idProjet;
        double      budget;
        QDate       dateDebut;
        QDate       dateFin;
        QString     ethique;
        int         teamCount      = 0;
        int         totalExp       = 0;
        int         completedExp   = 0;
        int         pubCount       = 0;
        int         bioCount       = 0;
        QStringList specializations;
    };

    QVector<SimilarStats> stats;
    stats.reserve(similarProjects.size());

    for (const ProjetRecord& sp : similarProjects) {
        SimilarStats s;
        s.idProjet  = sp.idProjet;
        s.budget    = sp.budget;
        s.dateDebut = sp.dateDeDebut;
        s.dateFin   = sp.dateDeFin;
        s.ethique   = sp.numeroDApprobationEthique.trimmed();

        // Team count
        {
            QSqlQuery q;
            q.prepare(R"(SELECT COUNT(*) FROM "Associer" WHERE "Id_projet"=:pid)");
            q.bindValue(":pid", s.idProjet);
            if (q.exec() && q.next()) s.teamCount = q.value(0).toInt();
        }

        // Experience counts (total and completed)
        {
            QSqlQuery q;
            q.prepare(QString(
                          R"(SELECT COUNT(*) FROM "%1" WHERE "Id_projet"=:pid)")
                          .arg(expTable));
            q.bindValue(":pid", s.idProjet);
            if (q.exec() && q.next()) s.totalExp = q.value(0).toInt();

            QSqlQuery q2;
            q2.prepare(QString(
                           R"(SELECT COUNT(*) FROM "%1")"
                           R"( WHERE "Id_projet"=:pid)"
                           R"( AND (LOWER("Status")=:t1 OR LOWER("Status")=:t2)"
                           R"(  OR LOWER("Status") LIKE 'reussi%')"
                           R"(  OR LOWER("Status") LIKE 'archiv%'))")
                           .arg(expTable));
            q2.bindValue(":pid", s.idProjet);
            q2.bindValue(":t1",  termSt);
            q2.bindValue(":t2",  termSt2);
            if (q2.exec() && q2.next()) s.completedExp = q2.value(0).toInt();
        }

        // Publication count — via "Id_projet" on Publication table
        {
            QSqlQuery q;
            q.prepare(
                R"(SELECT COUNT(*) FROM "Publication" WHERE "Id_projet"=:pid)");
            q.bindValue(":pid", s.idProjet);
            if (q.exec() && q.next())
                s.pubCount = q.value(0).toInt();
            else
                s.pubCount = sp.nombreDePublications; // fallback to projet field
        }

        // BioSample count
        {
            QSqlQuery q;
            q.prepare(
                R"(SELECT COUNT(*) FROM "BioSample" WHERE "Id_projet"=:pid)");
            q.bindValue(":pid", s.idProjet);
            if (q.exec() && q.next()) s.bioCount = q.value(0).toInt();
        }

        // Distinct specializations of assigned employees
        {
            QSqlQuery q;
            q.prepare(QString(
                          R"(SELECT DISTINCT NVL(TRIM(emp."specialization"),''))"
                          R"( FROM "%1" emp)"
                          R"( JOIN "Associer" a ON a."employee_id"=emp."employee_id")"
                          R"( WHERE a."Id_projet"=:pid)"
                          R"( AND NVL(TRIM(emp."specialization"),'')!='')")
                          .arg(empTable));
            q.bindValue(":pid", s.idProjet);
            if (q.exec())
                while (q.next())
                    s.specializations.append(q.value(0).toString().trimmed());
        }

        stats.append(s);
    }

    const int n = stats.size();

    // ── Current project supplementary data ───────────────────
    int currentTeamSize = 0;
    {
        QSqlQuery q;
        q.prepare(R"(SELECT COUNT(*) FROM "Associer" WHERE "Id_projet"=:pid)");
        q.bindValue(":pid", idProjetCourant);
        if (q.exec() && q.next()) currentTeamSize = q.value(0).toInt();
    }

    QStringList currentSpecs;
    {
        QSqlQuery q;
        q.prepare(QString(
                      R"(SELECT NVL(TRIM(emp."specialization"),''))"
                      R"( FROM "%1" emp)"
                      R"( JOIN "Associer" a ON a."employee_id"=emp."employee_id")"
                      R"( WHERE a."Id_projet"=:pid)"
                      R"( AND NVL(TRIM(emp."specialization"),'')!='')")
                      .arg(empTable));
        q.bindValue(":pid", idProjetCourant);
        if (q.exec())
            while (q.next())
                currentSpecs.append(
                    q.value(0).toString().trimmed().toLower());
    }

    // ══════════════════════════════════════════════════════════
    //  ANALYSIS 1 — Budget patterns
    // ══════════════════════════════════════════════════════════
    {
        double sumBudget    = 0;
        double minBudget    = stats[0].budget;
        double maxBudget    = stats[0].budget;
        int    noBudgetCount = 0;

        for (const SimilarStats& s : stats) {
            sumBudget += s.budget;
            minBudget  = qMin(minBudget, s.budget);
            maxBudget  = qMax(maxBudget, s.budget);
            if (s.budget <= 0) ++noBudgetCount;
        }
        double avgBudget   = sumBudget / n;
        double riskRate    = (double)noBudgetCount / n * 100.0;

        if (riskRate > 30.0) {
            result.avertissements << QString(
                                         "AVERTISSEMENT BUDGET: %1% des projets similaires avaient "
                                         "un budget non défini ou nul, ce qui est un facteur de risque financier.\n"
                                         "Budget moyen constaté dans ce domaine: %2 TND "
                                         "(Min: %3 — Max: %4 TND).\n"
                                         "Recommandation: Définir un budget réaliste et prévoir "
                                         "une marge de sécurité d'au moins 20%.")
                                         .arg((int)riskRate)
                                         .arg(avgBudget, 0, 'f', 0)
                                         .arg(minBudget, 0, 'f', 0)
                                         .arg(maxBudget, 0, 'f', 0);
        } else {
            result.pointsPositifs << QString(
                                         "Budget: Les projets similaires dans ce domaine ont bien "
                                         "défini et respecté leur budget.\n"
                                         "Moyenne observée: %1 TND (Min: %2 — Max: %3 TND).")
                                         .arg(avgBudget, 0, 'f', 0)
                                         .arg(minBudget, 0, 'f', 0)
                                         .arg(maxBudget, 0, 'f', 0);
        }

        // Warn if current project budget is far below domain average
        if (avgBudget > 0 && current.budget > 0 &&
            current.budget < avgBudget * 0.60)
        {
            result.avertissements << QString(
                                         "AVERTISSEMENT BUDGET ACTUEL: Le budget de votre projet "
                                         "(%1 TND) est inférieur à 60% de la moyenne du domaine (%2 TND).\n"
                                         "Recommandation: Revoir le budget alloué ou réduire le périmètre.")
                                         .arg(current.budget, 0, 'f', 0)
                                         .arg(avgBudget, 0, 'f', 0);
        }
    }

    // ══════════════════════════════════════════════════════════
    //  ANALYSIS 2 — Duration patterns
    // ══════════════════════════════════════════════════════════
    {
        double sumDuration = 0;
        int    validCount  = 0;

        for (const SimilarStats& s : stats) {
            if (s.dateDebut.isValid() && s.dateFin.isValid() &&
                s.dateFin > s.dateDebut)
            {
                sumDuration += s.dateDebut.daysTo(s.dateFin) / 30.4375;
                ++validCount;
            }
        }

        if (validCount > 0) {
            double avgDuration = sumDuration / validCount;
            double currentDur  = 0;
            if (current.dateDeDebut.isValid() && current.dateDeFin.isValid())
                currentDur = current.dateDeDebut.daysTo(current.dateDeFin) / 30.4375;

            if (currentDur > 0 && currentDur < avgDuration * 0.70) {
                result.avertissements << QString(
                                             "AVERTISSEMENT DÉLAI: La durée planifiée de votre projet "
                                             "(%1 mois) est nettement inférieure à la durée moyenne "
                                             "des projets similaires (%2 mois).\n"
                                             "Recommandation: Réviser le calendrier et ajouter "
                                             "un buffer de 20% à la durée planifiée.")
                                             .arg((int)std::round(currentDur))
                                             .arg((int)std::round(avgDuration));
            } else {
                result.pointsPositifs << QString(
                                             "Durée: Les projets similaires dans ce domaine se "
                                             "terminent en moyenne en %1 mois.\n"
                                             "Votre durée planifiée est cohérente avec les données historiques.")
                                             .arg((int)std::round(avgDuration));
            }
        }
    }

    // ══════════════════════════════════════════════════════════
    //  ANALYSIS 3 — Ethics approval patterns
    // ══════════════════════════════════════════════════════════
    {
        int noEthicsCount = 0;
        for (const SimilarStats& s : stats)
            if (s.ethique.isEmpty()) ++noEthicsCount;

        double noEthicsRate = (double)noEthicsCount / n * 100.0;

        if (noEthicsRate > 30.0) {
            result.avertissements << QString(
                                         "POINT D'ATTENTION ÉTHIQUE: %1% des projets similaires "
                                         "ont démarré sans numéro d'approbation éthique.\n"
                                         "Cela peut bloquer les expériences en cours de route "
                                         "et retarder la publication des résultats.\n"
                                         "Recommandation: Soumettre le dossier éthique avant "
                                         "le démarrage officiel du projet.")
                                         .arg((int)noEthicsRate);
        } else {
            result.pointsPositifs << QString(
                                         "Éthique: %1% des projets similaires avaient leur "
                                         "approbation éthique en ordre avant démarrage.\n"
                                         "Bonne pratique bien respectée dans ce domaine.")
                                         .arg((int)(100.0 - noEthicsRate));
        }

        // Warn if current project has no ethics number
        if (current.numeroDApprobationEthique.trimmed().isEmpty()) {
            result.recommandations << QString(
                "ACTION REQUISE — ÉTHIQUE: Votre projet n'a pas encore "
                "de numéro d'approbation éthique.\n"
                "Soumettre le dossier au comité éthique dès que possible "
                "pour éviter des blocages ultérieurs.");
        }
    }

    // ══════════════════════════════════════════════════════════
    //  ANALYSIS 4 — Team size patterns
    // ══════════════════════════════════════════════════════════
    {
        double sumTeam = 0;
        for (const SimilarStats& s : stats) sumTeam += s.teamCount;
        double avgTeamSize = sumTeam / n;

        if (currentTeamSize < (int)std::round(avgTeamSize * 0.7)) {
            result.avertissements << QString(
                                         "AVERTISSEMENT ÉQUIPE: Les projets similaires avaient "
                                         "en moyenne %1 membre(s) d'équipe.\n"
                                         "Votre projet en a actuellement %2.\n"
                                         "Recommandation: Envisager d'ajouter des collaborateurs "
                                         "pour atteindre une taille d'équipe adéquate.")
                                         .arg((int)std::round(avgTeamSize))
                                         .arg(currentTeamSize);
        } else {
            result.pointsPositifs << QString(
                                         "Équipe: Votre équipe (%1 personne(s)) est bien "
                                         "dimensionnée par rapport aux projets similaires "
                                         "(moyenne: %2 personne(s)).")
                                         .arg(currentTeamSize)
                                         .arg((int)std::round(avgTeamSize));
        }
    }

    // ══════════════════════════════════════════════════════════
    //  ANALYSIS 5 — Specialization gaps
    // ══════════════════════════════════════════════════════════
    {
        QMap<QString, int> specFreq;
        for (const SimilarStats& s : stats)
            for (const QString& sp : s.specializations)
                specFreq[sp.toLower()]++;

        // Sort by frequency descending
        QList<QPair<QString, int>> specList;
        for (auto it = specFreq.begin(); it != specFreq.end(); ++it)
            specList.append({it.key(), it.value()});
        std::sort(specList.begin(), specList.end(),
                  [](const QPair<QString,int>& a, const QPair<QString,int>& b){
                      return a.second > b.second; });

        // Top 3 most frequent specializations
        QStringList top3;
        for (int i = 0; i < qMin(3, (int)specList.size()); ++i)
            top3 << specList[i].first;

        // Check which ones are missing in current project
        QStringList missing;
        for (const QString& req : top3) {
            bool found = false;
            for (const QString& cur : currentSpecs)
                if (cur.contains(req.left(5)) || req.contains(cur.left(5)))
                { found = true; break; }
            if (!found) missing << req;
        }

        if (!missing.isEmpty()) {
            result.recommandations << QString(
                                          "COMPÉTENCES FRÉQUENTES DANS CE DOMAINE: %1\n"
                                          "Compétences non couvertes dans votre équipe actuelle: %2\n"
                                          "Recommandation: Rechercher des collaborateurs "
                                          "avec ces profils pour maximiser les chances de succès.")
                                          .arg(top3.join(", "))
                                          .arg(missing.join(", "));
        } else if (!top3.isEmpty()) {
            result.pointsPositifs << QString(
                                         "Compétences: Votre équipe couvre les spécialisations "
                                         "les plus fréquentes dans ce domaine (%1).")
                                         .arg(top3.join(", "));
        }
    }

    // ══════════════════════════════════════════════════════════
    //  ANALYSIS 6 — Experience completion rate
    // ══════════════════════════════════════════════════════════
    {
        double sumExpCount = 0;
        double sumCompRate = 0;
        int    withExp     = 0;

        for (const SimilarStats& s : stats) {
            sumExpCount += s.totalExp;
            if (s.totalExp > 0) {
                sumCompRate += (double)s.completedExp / s.totalExp * 100.0;
                ++withExp;
            }
        }
        double avgExpCount = sumExpCount / n;
        double avgCompRate = (withExp > 0) ? sumCompRate / withExp : 100.0;

        if (avgCompRate < 80.0) {
            result.avertissements << QString(
                                         "AVERTISSEMENT EXPÉRIENCES: Dans les projets similaires, "
                                         "en moyenne seulement %1% des expériences prévues "
                                         "ont été complétées.\n"
                                         "Nombre moyen d'expériences par projet: %2.\n"
                                         "Recommandation: Ne pas surcharger le plan d'expériences. "
                                         "Prioriser la qualité sur la quantité.")
                                         .arg((int)std::round(avgCompRate))
                                         .arg((int)std::round(avgExpCount));
        } else {
            result.pointsPositifs << QString(
                                         "Expériences: Les projets similaires complètent "
                                         "en moyenne %1% de leurs expériences planifiées "
                                         "(%2 expériences par projet).")
                                         .arg((int)std::round(avgCompRate))
                                         .arg((int)std::round(avgExpCount));
        }
    }

    // ══════════════════════════════════════════════════════════
    //  ANALYSIS 7 — Publication output
    // ══════════════════════════════════════════════════════════
    {
        double sumPubs      = 0;
        int    zeroPubCount = 0;

        for (const SimilarStats& s : stats) {
            sumPubs += s.pubCount;
            if (s.pubCount == 0) ++zeroPubCount;
        }
        double avgPubs      = sumPubs / n;
        double zeroPubRate  = (double)zeroPubCount / n * 100.0;

        if (zeroPubRate > 40.0) {
            result.avertissements << QString(
                                         "AVERTISSEMENT PUBLICATIONS: %1% des projets similaires "
                                         "se sont terminés sans aucune publication scientifique.\n"
                                         "Recommandation: Intégrer un objectif de publication "
                                         "dès le début et assigner un responsable de rédaction.")
                                         .arg((int)std::round(zeroPubRate));
        } else {
            result.pointsPositifs << QString(
                                         "Publications: Les projets similaires dans ce domaine "
                                         "produisent en moyenne %1 publication(s) par projet.\n"
                                         "C'est un indicateur de bonne productivité scientifique.")
                                         .arg(avgPubs, 0, 'f', 1);
        }
    }

    // ══════════════════════════════════════════════════════════
    //  ANALYSIS 8 — Overall success rate
    //  Success = budget défini + éthique présente + ≥1 publication
    // ══════════════════════════════════════════════════════════
    {
        int successCount = 0;
        for (const SimilarStats& s : stats) {
            bool budgetOk  = (s.budget > 0);
            bool ethiqueOk = !s.ethique.isEmpty();
            bool pubOk     = (s.pubCount >= 1);
            if (budgetOk && ethiqueOk && pubOk) ++successCount;
        }
        double successRate = (double)successCount / n * 100.0;

        result.tauxSuccesGlobal = QString(
                                      "TAUX DE SUCCÈS GLOBAL dans ce domaine: %1%\n"
                                      "Basé sur %2 projet(s) similaire(s) terminé(s).\n"
                                      "Critères: budget défini + approbation éthique + au moins 1 publication.")
                                      .arg((int)std::round(successRate))
                                      .arg(n);

        if (successRate >= 70.0) {
            result.recommandations << QString(
                                          "Ce domaine affiche un bon taux de succès global (%1%). "
                                          "Respectez les bonnes pratiques observées: "
                                          "budget bien défini, éthique en ordre, objectif de publication.")
                                          .arg((int)std::round(successRate));
        } else {
            result.recommandations << QString(
                                          "Le taux de succès dans ce domaine est de %1%.\n"
                                          "Portez une attention particulière aux critères souvent "
                                          "manquants dans les projets échoués: budget, éthique, publications.")
                                          .arg((int)std::round(successRate));
        }
    }

    result.donneesSuffisantes = true;
    return result;
}

// ═════════════════════════════════════════════════════════════
//  SECTION 2 — UI HELPERS (file-local)
// ═════════════════════════════════════════════════════════════

static QLabel* makeSectionHeader(const QString& text, const QString& color)
{
    QLabel* l = new QLabel(text);
    l->setStyleSheet(QString(
                         "color:%1; font-size:11px; font-weight:900;"
                         " background:transparent; padding:2px 0;").arg(color));
    return l;
}

static QFrame* makeResultCard(const QString& text,
                              const QString& prefix,
                              const QString& borderColor,
                              const QString& bgColor)
{
    QFrame* card = new QFrame;
    card->setStyleSheet(QString(
                            "QFrame{ background:%1;"
                            " border:1px solid rgba(255,255,255,0.08);"
                            " border-left:4px solid %2;"
                            " border-radius:8px; }")
                            .arg(bgColor, borderColor));

    QHBoxLayout* hl = new QHBoxLayout(card);
    hl->setContentsMargins(12, 10, 12, 10);
    hl->setSpacing(10);

    QLabel* icon = new QLabel(prefix);
    icon->setStyleSheet(QString(
                            "color:%1; font-size:15px; background:transparent;"
                            " min-width:18px;").arg(borderColor));
    icon->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    hl->addWidget(icon);

    QLabel* lbl = new QLabel(text);
    lbl->setWordWrap(true);
    lbl->setStyleSheet(
        "color:rgba(255,255,255,0.88);"
        " font-size:11px; background:transparent;"
        " line-height:1.4;");
    lbl->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    hl->addWidget(lbl, 1);

    return card;
}

// ═════════════════════════════════════════════════════════════
//  SECTION 3 — EXPORT HELPERS (file-local)
// ═════════════════════════════════════════════════════════════

static QString buildReportText(const LeconsApprisesResult& res,
                               const QString& projNom)
{
    QString text;
    text += "========================================================\n";
    text += "LEÇONS APPRISES — SmartVision\n";
    text += "========================================================\n";
    text += QString("Projet : %1\n").arg(projNom);
    text += QString("Domaine: %1\n").arg(res.domaine);
    text += QString("Basé sur %1 projet(s) similaire(s) terminé(s)\n")
                .arg(res.nombreProjetsSimilaires);
    text += QString("Généré le: %1\n")
                .arg(QDate::currentDate().toString("dd/MM/yyyy"));
    text += "========================================================\n\n";

    if (!res.donneesSuffisantes) {
        text += res.messageVide + "\n";
        return text;
    }

    if (!res.avertissements.isEmpty()) {
        text += "── AVERTISSEMENTS ─────────────────────────────────────\n\n";
        for (const QString& s : res.avertissements)
            text += "⚠  " + s + "\n\n";
    }
    if (!res.pointsPositifs.isEmpty()) {
        text += "── CE QUI A BIEN FONCTIONNÉ ────────────────────────────\n\n";
        for (const QString& s : res.pointsPositifs)
            text += "✔  " + s + "\n\n";
    }
    if (!res.recommandations.isEmpty()) {
        text += "── RECOMMANDATIONS ─────────────────────────────────────\n\n";
        for (const QString& s : res.recommandations)
            text += "→  " + s + "\n\n";
    }
    text += "── TAUX DE SUCCÈS GLOBAL ───────────────────────────────\n\n";
    text += res.tauxSuccesGlobal + "\n";
    return text;
}

static QString buildReportHtml(const LeconsApprisesResult& res,
                               const QString& projNom)
{
    auto escape = [](const QString& s) -> QString {
        return s.toHtmlEscaped().replace("\n", "<br>");
    };

    QString html;
    html += "<html><body style='font-family:Arial,sans-serif;"
            "font-size:10pt;color:#1a1a1a;background:white;"
            "margin:0;padding:0;'>";

    // Title block
    html += "<div style='background:#1a4470;color:white;padding:16px 20px;"
            "border-radius:4px;margin-bottom:16px;'>";
    html += "<h2 style='margin:0 0 4px 0;font-size:16pt;'>Leçons Apprises</h2>";
    html += QString("<p style='margin:0;font-size:10pt;opacity:0.85;'>"
                    "Domaine: <b>%1</b></p>").arg(escape(res.domaine));
    html += "</div>";

    // Meta info
    html += "<table width='100%' style='margin-bottom:14px;"
            "font-size:9pt;color:#555;'><tr>";
    html += QString("<td><b>Projet:</b> %1</td>").arg(escape(projNom));
    html += QString("<td><b>Projets analysés:</b> %1</td>")
                .arg(res.nombreProjetsSimilaires);
    html += QString("<td><b>Date:</b> %1</td>")
                .arg(QDate::currentDate().toString("dd/MM/yyyy"));
    html += "</tr></table>";

    if (!res.donneesSuffisantes) {
        html += "<div style='background:#e8f4fb;border-left:4px solid #2A649B;"
                "padding:12px;border-radius:4px;'>";
        html += "<p style='margin:0;color:#2A649B;'>" +
                escape(res.messageVide) + "</p></div>";
        html += "</body></html>";
        return html;
    }

    auto htmlSection = [&](const QString& title, const QString& color,
                           const QStringList& items,
                           const QString& prefix,
                           const QString& bgColor) -> QString
    {
        if (items.isEmpty()) return QString();
        QString h;
        h += QString("<h3 style='color:%1;margin:18px 0 8px 0;"
                     "font-size:11pt;border-bottom:1px solid %1;"
                     "padding-bottom:4px;'>%2</h3>").arg(color, title);
        for (const QString& s : items) {
            h += QString("<div style='background:%1;border-left:4px solid %2;"
                         "padding:10px 12px;margin-bottom:8px;"
                         "border-radius:3px;font-size:9.5pt;'>"
                         "<span style='color:%2;font-weight:bold;'>%3&nbsp;</span>"
                         "%4</div>")
                     .arg(bgColor, color, prefix, escape(s));
        }
        return h;
    };

    html += htmlSection("⚠  Avertissements", "#CC3344",
                        res.avertissements, "⚠",  "#fff5f5");
    html += htmlSection("✔  Ce qui a bien fonctionné", "#2E7A5C",
                        res.pointsPositifs, "✔",  "#f0faf6");
    html += htmlSection("→  Recommandations", "#1A4470",
                        res.recommandations, "→", "#f0f5fb");

    // Success rate
    html += "<div style='background:#f5f5f5;border:1px solid #ddd;"
            "padding:12px;border-radius:4px;margin-top:18px;'>";
    html += "<h3 style='margin:0 0 6px 0;font-size:10pt;color:#444;'>"
            "Taux de succès global</h3>";
    html += "<p style='margin:0;font-size:9.5pt;color:#333;'>" +
            escape(res.tauxSuccesGlobal) + "</p>";
    html += "</div>";

    // Footer
    html += "<p style='margin-top:24px;font-size:8pt;color:#999;"
            "text-align:center;'>"
            "Rapport généré par SmartVision — Gestion Projet</p>";
    html += "</body></html>";
    return html;
}

// ═════════════════════════════════════════════════════════════
//  SECTION 4 — MAIN DIALOG
// ═════════════════════════════════════════════════════════════

void LeconsApprises::showDialog(QWidget* parent)
{
    // ── 1. Project picker ─────────────────────────────────────
    QDialog* picker = new QDialog(parent,
                                  Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    picker->setWindowTitle("Leçons Apprises — Choisir un projet");
    picker->setMinimumSize(520, 420);
    picker->setAttribute(Qt::WA_DeleteOnClose);
    picker->setStyleSheet("QDialog{ background:#1C2A35; }");

    QVBoxLayout* pl = new QVBoxLayout(picker);
    pl->setContentsMargins(20, 18, 20, 16);
    pl->setSpacing(12);

    // Header
    QLabel* plTitle = new QLabel("  Sélectionnez le projet à analyser");
    plTitle->setStyleSheet(
        "color:#518195; font-size:13px; font-weight:900;"
        " background:rgba(81,129,149,0.15);"
        " border-radius:10px; padding:7px 14px;");
    pl->addWidget(plTitle);

    QLabel* plSub = new QLabel(
        "L'analyse compare votre projet avec tous les projets terminés "
        "du même domaine de recherche.");
    plSub->setWordWrap(true);
    plSub->setStyleSheet(
        "color:rgba(255,255,255,0.50); font-size:10px;"
        " background:transparent; padding-left:2px;");
    pl->addWidget(plSub);

    // Project list
    QListWidget* projList = new QListWidget;
    projList->setStyleSheet(
        "QListWidget{ background:rgba(255,255,255,0.08);"
        " border-radius:10px;"
        " border:1px solid rgba(255,255,255,0.15); }"
        "QListWidget::item{ padding:9px 14px;"
        " color:rgba(255,255,255,0.85);"
        " font-weight:600; font-size:11px; }"
        "QListWidget::item:selected{ background:#518195;"
        " color:white; border-radius:6px; }"
        "QListWidget::item:hover:!selected{"
        " background:rgba(255,255,255,0.08); }");

    GestProjCrud crud;
    QList<ProjetRecord> allProjs;
    QString perr;
    crud.loadProjets(allProjs, &perr);

    for (const ProjetRecord& pr : allProjs) {
        QListWidgetItem* item = new QListWidgetItem(
            QString("%1  [%2]  —  %3")
                .arg(pr.nomDuProjet)
                .arg(pr.domaineDeRecherche.isEmpty()
                         ? "domaine non défini" : pr.domaineDeRecherche)
                .arg(pr.statut));
        item->setData(Qt::UserRole, pr.idProjet);
        item->setData(Qt::UserRole + 1, pr.nomDuProjet);
        projList->addItem(item);
    }
    pl->addWidget(projList, 1);

    // Buttons
    QHBoxLayout* pbl = new QHBoxLayout;
    pbl->setSpacing(10);

    QPushButton* cancelBtn = new QPushButton("Annuler");
    cancelBtn->setFixedHeight(36);
    cancelBtn->setStyleSheet(
        "QPushButton{ background:rgba(255,255,255,0.12);"
        " color:rgba(255,255,255,0.80);"
        " border-radius:8px; font-weight:700;"
        " font-size:12px; padding:0 16px; }"
        "QPushButton:hover{ background:rgba(255,255,255,0.20); }");

    QPushButton* goBtn = new QPushButton("  Analyser les leçons");
    goBtn->setFixedHeight(36);
    goBtn->setEnabled(false);
    goBtn->setStyleSheet(
        "QPushButton{ background:#518195; color:white;"
        " border-radius:8px; font-weight:800;"
        " font-size:12px; padding:0 18px; }"
        "QPushButton:hover{ background:#3d6475; }"
        "QPushButton:disabled{ background:rgba(81,129,149,0.30);"
        " color:rgba(255,255,255,0.40); }");

    pbl->addWidget(cancelBtn);
    pbl->addStretch(1);
    pbl->addWidget(goBtn);
    pl->addLayout(pbl);

    QObject::connect(cancelBtn, &QPushButton::clicked,
                     picker, &QDialog::reject);
    QObject::connect(projList, &QListWidget::itemSelectionChanged, picker,
                     [=](){ goBtn->setEnabled(projList->currentItem() != nullptr); });
    QObject::connect(projList, &QListWidget::itemDoubleClicked,
                     goBtn,    &QPushButton::click);

    // ── 2. On "Analyser" click — build result dialog ──────────
    QObject::connect(goBtn, &QPushButton::clicked, picker, [=](){

        QListWidgetItem* sel = projList->currentItem();
        if (!sel) return;

        const int     projId  = sel->data(Qt::UserRole).toInt();
        const QString projNom = sel->data(Qt::UserRole + 1).toString();
        picker->hide();

        // Run analysis
        LeconsApprisesResult res = LeconsApprises::analyze(projId);

        // ── Result dialog ─────────────────────────────────────
        QDialog* dlg = new QDialog(parent,
                                   Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
        dlg->setWindowTitle(
            QString("Leçons Apprises — %1").arg(projNom));
        dlg->setMinimumSize(880, 720);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->setStyleSheet("QDialog{ background:#1C2A35; }");

        QVBoxLayout* mainL = new QVBoxLayout(dlg);
        mainL->setContentsMargins(22, 18, 22, 16);
        mainL->setSpacing(12);

        // ── Header ────────────────────────────────────────────
        QFrame* hdr = new QFrame;
        hdr->setStyleSheet(
            "QFrame{ background:qlineargradient("
            "x1:0,y1:0,x2:1,y2:0,"
            "stop:0 #1a4470,stop:1 #518195);"
            " border-radius:12px; }");
        QVBoxLayout* hL = new QVBoxLayout(hdr);
        hL->setContentsMargins(18, 14, 18, 14);
        hL->setSpacing(4);

        QLabel* hTitle = new QLabel(
            QString("Leçons Apprises — Domaine: %1").arg(res.domaine));
        hTitle->setStyleSheet(
            "color:white; font-size:15px; font-weight:900;"
            " background:transparent;");

        QLabel* hSub = new QLabel(
            QString("Basé sur %1 projet(s) similaire(s) terminé(s) "
                    "dans la base de données")
                .arg(res.nombreProjetsSimilaires));
        hSub->setStyleSheet(
            "color:rgba(255,255,255,0.75); font-size:11px;"
            " background:transparent;");

        hL->addWidget(hTitle);
        hL->addWidget(hSub);
        mainL->addWidget(hdr);

        // ── KPI strip ─────────────────────────────────────────
        QWidget* kpiRow = new QWidget;
        kpiRow->setStyleSheet("background:transparent;");
        QHBoxLayout* kpiL = new QHBoxLayout(kpiRow);
        kpiL->setContentsMargins(0, 0, 0, 0);
        kpiL->setSpacing(8);

        auto makeKpi = [](const QString& val, const QString& lbl,
                          const QString& bg,  const QString& fg) -> QFrame*
        {
            QFrame* pill = new QFrame;
            pill->setStyleSheet(QString(
                                    "QFrame{ background:%1; border-radius:8px; border:none; }")
                                    .arg(bg));
            QVBoxLayout* pL = new QVBoxLayout(pill);
            pL->setContentsMargins(12, 8, 12, 8);
            pL->setSpacing(2);
            QLabel* vLbl = new QLabel(val);
            vLbl->setStyleSheet(QString(
                                    "color:%1; font-size:20px; font-weight:900;"
                                    " background:transparent;").arg(fg));
            vLbl->setAlignment(Qt::AlignCenter);
            QLabel* lLbl = new QLabel(lbl);
            lLbl->setStyleSheet(
                "color:rgba(255,255,255,0.50); font-size:8px;"
                " font-weight:700; background:transparent;");
            lLbl->setAlignment(Qt::AlignCenter);
            pL->addWidget(vLbl);
            pL->addWidget(lLbl);
            return pill;
        };

        kpiL->addWidget(makeKpi(
                            QString::number(res.nombreProjetsSimilaires),
                            "Projets analysés",
                            "rgba(42,100,155,0.18)", "#a8d4e0"), 1);
        kpiL->addWidget(makeKpi(
                            QString::number(res.avertissements.size()),
                            "Avertissements",
                            "rgba(139,47,60,0.20)", "#CF4F5E"), 1);
        kpiL->addWidget(makeKpi(
                            QString::number(res.pointsPositifs.size()),
                            "Points positifs",
                            "rgba(46,139,124,0.18)", "#2E8B7C"), 1);
        kpiL->addWidget(makeKpi(
                            QString::number(res.recommandations.size()),
                            "Recommandations",
                            "rgba(26,68,112,0.25)", "#6ba8d4"), 1);
        mainL->addWidget(kpiRow);

        // ── Scroll area ───────────────────────────────────────
        QScrollArea* scroll = new QScrollArea;
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setStyleSheet(
            "QScrollArea{ background:transparent; border:none; }"
            "QScrollBar:vertical{ width:7px; background:transparent; }"
            "QScrollBar::handle:vertical{"
            " background:rgba(81,129,149,0.45);"
            " border-radius:4px; }");

        QWidget* scrollW = new QWidget;
        scrollW->setStyleSheet("background:transparent;");
        QVBoxLayout* scrollL = new QVBoxLayout(scrollW);
        scrollL->setContentsMargins(0, 0, 4, 0);
        scrollL->setSpacing(8);

        // ── Content: no-data or full results ─────────────────
        if (!res.donneesSuffisantes) {
            QFrame* infoCard = new QFrame;
            infoCard->setStyleSheet(
                "QFrame{ background:rgba(42,100,155,0.12);"
                " border:2px solid rgba(42,100,155,0.35);"
                " border-radius:12px; }");
            QVBoxLayout* infoL = new QVBoxLayout(infoCard);
            infoL->setContentsMargins(20, 20, 20, 20);
            infoL->setSpacing(10);

            QLabel* infoIcon = new QLabel("ℹ");
            infoIcon->setAlignment(Qt::AlignCenter);
            infoIcon->setStyleSheet(
                "color:#518195; font-size:32px; background:transparent;");

            QLabel* infoTxt = new QLabel(res.messageVide);
            infoTxt->setWordWrap(true);
            infoTxt->setAlignment(Qt::AlignCenter);
            infoTxt->setStyleSheet(
                "color:rgba(255,255,255,0.75); font-size:12px;"
                " background:transparent; line-height:1.5;");

            infoL->addWidget(infoIcon);
            infoL->addWidget(infoTxt);
            scrollL->addWidget(infoCard);

        } else {
            // AVERTISSEMENTS (red)
            if (!res.avertissements.isEmpty()) {
                scrollL->addWidget(
                    makeSectionHeader("⚠  AVERTISSEMENTS", "#CF4F5E"));
                for (const QString& s : res.avertissements)
                    scrollL->addWidget(makeResultCard(s, "⚠",
                                                      "#CF4F5E", "rgba(139,47,60,0.10)"));
            }

            // CE QUI A BIEN FONCTIONNÉ (green)
            if (!res.pointsPositifs.isEmpty()) {
                scrollL->addWidget(
                    makeSectionHeader("✔  CE QUI A BIEN FONCTIONNÉ",
                                      "#2E8B7C"));
                for (const QString& s : res.pointsPositifs)
                    scrollL->addWidget(makeResultCard(s, "✔",
                                                      "#2E8B7C", "rgba(46,139,124,0.10)"));
            }

            // RECOMMANDATIONS (blue)
            if (!res.recommandations.isEmpty()) {
                scrollL->addWidget(
                    makeSectionHeader("→  RECOMMANDATIONS", "#6ba8d4"));
                for (const QString& s : res.recommandations)
                    scrollL->addWidget(makeResultCard(s, "→",
                                                      "#6ba8d4", "rgba(42,100,155,0.10)"));
            }

            // TAUX DE SUCCÈS
            QFrame* rateCard = new QFrame;
            rateCard->setStyleSheet(
                "QFrame{ background:rgba(255,255,255,0.05);"
                " border:1px solid rgba(255,255,255,0.12);"
                " border-radius:10px; }");
            QVBoxLayout* rL = new QVBoxLayout(rateCard);
            rL->setContentsMargins(16, 12, 16, 12);
            rL->setSpacing(4);

            QLabel* rHdr = new QLabel("TAUX DE SUCCÈS GLOBAL");
            rHdr->setStyleSheet(
                "color:rgba(255,255,255,0.40); font-size:9px;"
                " font-weight:900; background:transparent;");

            QLabel* rTxt = new QLabel(res.tauxSuccesGlobal);
            rTxt->setWordWrap(true);
            rTxt->setStyleSheet(
                "color:rgba(255,255,255,0.85); font-size:11px;"
                " background:transparent;");

            rL->addWidget(rHdr);
            rL->addWidget(rTxt);
            scrollL->addWidget(rateCard);
        }

        // ══════════════════════════════════════════════════════
        //  AI ENRICHMENT SECTION
        // ══════════════════════════════════════════════════════
        QFrame* aiCard = new QFrame;
        aiCard->setStyleSheet(
            "QFrame{ background:rgba(181,103,44,0.08);"
            " border:1px solid rgba(181,103,44,0.28);"
            " border-radius:10px; }");
        QVBoxLayout* aiL = new QVBoxLayout(aiCard);
        aiL->setContentsMargins(16, 12, 16, 12);
        aiL->setSpacing(8);

        QLabel* aiHdr = new QLabel(
            "★  RECHERCHE IA — Projets similaires dans le monde");
        aiHdr->setStyleSheet(
            "color:#D4762A; font-size:11px; font-weight:900;"
            " background:transparent;");
        aiL->addWidget(aiHdr);

        QLabel* aiDesc = new QLabel(
            "L'IA recherche des projets de recherche internationaux "
            "dans ce domaine pour enrichir les leçons apprises "
            "avec des expériences mondiales.");
        aiDesc->setWordWrap(true);
        aiDesc->setStyleSheet(
            "color:rgba(255,255,255,0.50); font-size:10px;"
            " background:transparent;");
        aiL->addWidget(aiDesc);

        QLabel* aiStatus = new QLabel(
            "Appuyez sur 'Lancer la recherche IA' pour démarrer...");
        aiStatus->setWordWrap(true);
        aiStatus->setStyleSheet(
            "color:rgba(255,255,255,0.40); font-size:10px;"
            " background:transparent;");
        aiL->addWidget(aiStatus);

        // Container for AI results
        QWidget* aiResultsW = new QWidget;
        aiResultsW->setStyleSheet("background:transparent;");
        QVBoxLayout* aiResultsL = new QVBoxLayout(aiResultsW);
        aiResultsL->setContentsMargins(0, 4, 0, 0);
        aiResultsL->setSpacing(6);
        aiL->addWidget(aiResultsW);

        // AI buttons
        QHBoxLayout* aiBtns = new QHBoxLayout;
        aiBtns->setSpacing(8);

        QPushButton* launchAiBtn = new QPushButton("★  Lancer la recherche IA");
        launchAiBtn->setFixedHeight(30);
        launchAiBtn->setStyleSheet(
            "QPushButton{ background:#D4762A; color:white;"
            " border-radius:6px; font-weight:800;"
            " font-size:10px; padding:0 14px; }"
            "QPushButton:hover{ background:#B5672C; }"
            "QPushButton:disabled{ background:rgba(212,118,42,0.25);"
            " color:rgba(255,255,255,0.35); }");

        QPushButton* retryAiBtn = new QPushButton("↺  Réessayer");
        retryAiBtn->setFixedHeight(30);
        retryAiBtn->setVisible(false);
        retryAiBtn->setStyleSheet(
            "QPushButton{ background:rgba(255,255,255,0.10);"
            " color:rgba(255,255,255,0.75);"
            " border-radius:6px; font-weight:700;"
            " font-size:10px; padding:0 12px; }"
            "QPushButton:hover{ background:rgba(255,255,255,0.18); }");

        aiBtns->addWidget(launchAiBtn);
        aiBtns->addWidget(retryAiBtn);
        aiBtns->addStretch(1);
        aiL->addLayout(aiBtns);
        scrollL->addWidget(aiCard);

        scrollL->addStretch(1);
        scroll->setWidget(scrollW);
        mainL->addWidget(scroll, 1);

        // ── AI Lambda ─────────────────────────────────────────
        QString domaineForAI = res.domaine;

        auto launchAI = [=]() mutable
        {
            // Clear old results
            while (QLayoutItem* it = aiResultsL->takeAt(0)) {
                delete it->widget();
                delete it;
            }
            launchAiBtn->setEnabled(false);
            retryAiBtn->setVisible(false);
            aiStatus->setVisible(true);
            aiStatus->setText("  Recherche IA en cours...");
            aiStatus->setStyleSheet(
                "color:#D4762A; font-size:10px; font-weight:700;"
                " background:rgba(212,118,42,0.08);"
                " border-radius:5px; padding:5px 8px;");

            QNetworkAccessManager* net = new QNetworkAccessManager(dlg);

            QString domForPrompt = domaineForAI.isEmpty()
                                       ? "recherche biomedicale" : domaineForAI;

            QString prompt =
                "Tu es un expert mondial en recherche scientifique. "
                "Pour le domaine de recherche scientifique: \"" +
                domForPrompt + "\", "
                               "cite exactement 4 projets de recherche réels et reconnus "
                               "menés dans le monde ces 15 dernières années. "
                               "Pour chaque projet: nom exact, institution/pays, "
                               "durée approximative, et une leçon clé ou résultat marquant. "
                               "FORMAT STRICT - exactement 4 lignes:\n"
                               "NOM | INSTITUTION | DURÉE | LEÇON_CLÉ\n"
                               "Réponds UNIQUEMENT avec les 4 lignes au format, rien d'autre.";

            QJsonObject body;
            body["model"]       = QString(GROQ_API_MODEL);
            body["max_tokens"]  = 600;
            body["temperature"] = 0.3;
            body["messages"]    = QJsonArray{
                QJsonObject{
                            {"role", "system"},
                            {"content",
                             "Tu es un expert en recherche scientifique mondiale. "
                             "Réponds uniquement au format: "
                             "NOM | INSTITUTION | DURÉE | LEÇON_CLÉ"}},
                QJsonObject{{"role", "user"}, {"content", prompt}}
            };

            QUrl url(QString(GROQ_API_URL));
            QNetworkRequest req(url);
            req.setHeader(QNetworkRequest::ContentTypeHeader,
                          "application/json");
            req.setRawHeader("Authorization",
                             ("Bearer " + QString(GROQ_API_KEY)).toUtf8());
            QSslConfiguration ssl = QSslConfiguration::defaultConfiguration();
            ssl.setPeerVerifyMode(QSslSocket::VerifyNone);
            req.setSslConfiguration(ssl);

            QNetworkReply* reply =
                net->post(req, QJsonDocument(body).toJson());

            QObject::connect(reply, &QNetworkReply::sslErrors, reply,
                             [reply](const QList<QSslError>&){
                                 reply->ignoreSslErrors(); });

            QObject::connect(reply, &QNetworkReply::finished, dlg,
                             [=]() mutable {
                                 reply->deleteLater();
                                 net->deleteLater();
                                 launchAiBtn->setEnabled(true);

                                 QByteArray data  = reply->readAll();
                                 QJsonObject root = QJsonDocument::fromJson(data).object();

                                 bool hasError =
                                     root.contains("error") ||
                                     reply->error() != QNetworkReply::NoError;

                                 if (hasError) {
                                     aiStatus->setVisible(true);
                                     aiStatus->setText(
                                         "  IA indisponible — vérifiez votre connexion.");
                                     aiStatus->setStyleSheet(
                                         "color:#CF4F5E; font-size:10px; font-weight:700;"
                                         " background:rgba(139,47,60,0.10);"
                                         " border-radius:5px; padding:5px 8px;");
                                     retryAiBtn->setVisible(true);
                                     return;
                                 }

                                 aiStatus->setVisible(false);

                                 QString raw = root["choices"].toArray().first()
                                                   .toObject()["message"].toObject()["content"]
                                                   .toString().trimmed();

                                 if (raw.isEmpty()) {
                                     aiStatus->setVisible(true);
                                     aiStatus->setText(
                                         "  Aucun résultat retourné par l'IA.");
                                     retryAiBtn->setVisible(true);
                                     return;
                                 }

                                 // Parse NOM | INSTITUTION | DURÉE | LEÇON_CLÉ
                                 QStringList lines = raw.split('\n', Qt::SkipEmptyParts);
                                 int added = 0;

                                 for (const QString& line : lines) {
                                     if (line.trimmed().startsWith("NOM")) continue;
                                     QStringList parts = line.split('|');
                                     if (parts.isEmpty()) continue;

                                     QString pName  = parts[0].trimmed();
                                     QString pInst  = parts.size()>=2
                                                         ? parts[1].trimmed() : "";
                                     QString pDur   = parts.size()>=3
                                                        ? parts[2].trimmed() : "";
                                     QString pLecon = parts.size()>=4
                                                          ? parts[3].trimmed() : "";
                                     if (pName.isEmpty()) continue;

                                     QFrame* rCard = new QFrame;
                                     rCard->setStyleSheet(
                                         "QFrame{ background:rgba(181,103,44,0.10);"
                                         " border:1px solid rgba(181,103,44,0.22);"
                                         " border-radius:7px; }");
                                     QVBoxLayout* rL2 = new QVBoxLayout(rCard);
                                     rL2->setContentsMargins(12, 8, 12, 8);
                                     rL2->setSpacing(3);

                                     QLabel* rTitle = new QLabel(
                                         QString("★  %1").arg(pName));
                                     rTitle->setWordWrap(true);
                                     rTitle->setStyleSheet(
                                         "color:#D4762A; font-size:11px;"
                                         " font-weight:900; background:transparent;");
                                     rL2->addWidget(rTitle);

                                     if (!pInst.isEmpty() || !pDur.isEmpty()) {
                                         QLabel* rMeta = new QLabel(
                                             QString("%1%2%3")
                                                 .arg(pInst)
                                                 .arg((!pInst.isEmpty() && !pDur.isEmpty())
                                                          ? "  |  " : "")
                                                 .arg(pDur));
                                         rMeta->setStyleSheet(
                                             "color:rgba(255,255,255,0.40);"
                                             " font-size:9px; background:transparent;");
                                         rL2->addWidget(rMeta);
                                     }

                                     if (!pLecon.isEmpty()) {
                                         QLabel* rLecon = new QLabel(pLecon);
                                         rLecon->setWordWrap(true);
                                         rLecon->setStyleSheet(
                                             "color:rgba(255,255,255,0.78);"
                                             " font-size:10px; background:transparent;");
                                         rL2->addWidget(rLecon);
                                     }

                                     aiResultsL->addWidget(rCard);
                                     ++added;
                                 }

                                 if (added == 0) {
                                     aiStatus->setVisible(true);
                                     aiStatus->setText(
                                         "  Format de réponse inattendu. Réessayez.");
                                     retryAiBtn->setVisible(true);
                                 }
                             });
        };

        QObject::connect(launchAiBtn, &QPushButton::clicked, dlg, launchAI);
        QObject::connect(retryAiBtn,  &QPushButton::clicked, dlg, launchAI);

        // ══════════════════════════════════════════════════════
        //  FOOTER — Export buttons + Close
        // ══════════════════════════════════════════════════════
        QFrame* foot = new QFrame;
        foot->setStyleSheet(
            "QFrame{ background:rgba(255,255,255,0.04);"
            " border-radius:8px; }");
        QHBoxLayout* fl = new QHBoxLayout(foot);
        fl->setContentsMargins(12, 8, 12, 8);
        fl->setSpacing(8);

        // Export TXT / DOC
        QPushButton* exportTxtBtn = new QPushButton("  Exporter (.txt / .doc)");
        exportTxtBtn->setFixedHeight(32);
        exportTxtBtn->setStyleSheet(
            "QPushButton{ background:#2A649B; color:white;"
            " border-radius:7px; font-weight:700;"
            " font-size:11px; padding:0 14px; }"
            "QPushButton:hover{ background:#1A4470; }");

        // Export PDF
        QPushButton* exportPdfBtn = new QPushButton("  Exporter (.pdf)");
        exportPdfBtn->setFixedHeight(32);
        exportPdfBtn->setStyleSheet(
            "QPushButton{ background:#8B2F3C; color:white;"
            " border-radius:7px; font-weight:700;"
            " font-size:11px; padding:0 14px; }"
            "QPushButton:hover{ background:#6A2030; }");

        QPushButton* closeBtn = new QPushButton("Fermer");
        closeBtn->setFixedHeight(32);
        closeBtn->setStyleSheet(
            "QPushButton{ background:rgba(255,255,255,0.10);"
            " color:rgba(255,255,255,0.80);"
            " border-radius:7px; font-weight:700;"
            " font-size:11px; padding:0 14px; }"
            "QPushButton:hover{ background:rgba(255,255,255,0.18); }");

        fl->addWidget(exportTxtBtn);
        fl->addWidget(exportPdfBtn);
        fl->addStretch(1);
        fl->addWidget(closeBtn);
        mainL->addWidget(foot);

        QObject::connect(closeBtn, &QPushButton::clicked,
                         dlg, &QDialog::accept);

        // ── Export TXT / DOC ──────────────────────────────────
        QObject::connect(exportTxtBtn, &QPushButton::clicked, dlg,
                         [=]()
                         {
                             QString safeName = projNom;
                             safeName.replace(
                                 QRegularExpression(R"([<>:"/\\|?*\s])"), "_");

                             QString fileName = QFileDialog::getSaveFileName(
                                 dlg,
                                 "Exporter les Leçons Apprises",
                                 QString("lecons_apprises_%1.txt").arg(safeName),
                                 "Document texte (*.txt);;"
                                 "Word document (*.doc);;"
                                 "Tous les fichiers (*)");

                             if (fileName.isEmpty()) return;

                             QFile f(fileName);
                             if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                                 QDialog* errDlg = new QDialog(dlg);
                                 errDlg->setWindowTitle("Erreur d'export");
                                 errDlg->setStyleSheet("QDialog{ background:#1C2A35; }");
                                 QVBoxLayout* el = new QVBoxLayout(errDlg);
                                 QLabel* el2 = new QLabel(
                                     "Impossible d'écrire le fichier:\n" + fileName);
                                 el2->setStyleSheet(
                                     "color:rgba(255,255,255,0.80); font-size:11px;");
                                 el->addWidget(el2);
                                 QPushButton* eb = new QPushButton("OK");
                                 eb->setStyleSheet(
                                     "QPushButton{ background:#518195; color:white;"
                                     " border-radius:6px; padding:4px 16px; }");
                                 QObject::connect(eb, &QPushButton::clicked,
                                                  errDlg, &QDialog::accept);
                                 el->addWidget(eb, 0, Qt::AlignRight);
                                 errDlg->exec();
                                 return;
                             }
                             QTextStream out(&f);
                             out.setEncoding(QStringConverter::Utf8);
                             out << buildReportText(res, projNom);
                             f.close();
                         });

        // ── Export PDF ────────────────────────────────────────
        QObject::connect(exportPdfBtn, &QPushButton::clicked, dlg,
                         [=]()
                         {
                             QString safeName = projNom;
                             safeName.replace(
                                 QRegularExpression(R"([<>:"/\\|?*\s])"), "_");

                             QString fileName = QFileDialog::getSaveFileName(
                                 dlg,
                                 "Exporter les Leçons Apprises en PDF",
                                 QString("lecons_apprises_%1.pdf").arg(safeName),
                                 "PDF (*.pdf);;Tous les fichiers (*)");

                             if (fileName.isEmpty()) return;
                             if (!fileName.endsWith(".pdf", Qt::CaseInsensitive))
                                 fileName += ".pdf";

                             QPrinter printer(QPrinter::HighResolution);
                             printer.setOutputFormat(QPrinter::PdfFormat);
                             printer.setOutputFileName(fileName);
                             printer.setPageSize(QPageSize(QPageSize::A4));
                             printer.setPageMargins(
                                 QMarginsF(18, 18, 18, 18),
                                 QPageLayout::Millimeter);

                             QTextDocument doc;
                             doc.setDefaultFont(QFont("Arial", 10));
                             doc.setHtml(buildReportHtml(res, projNom));
                             doc.print(&printer);
                         });

        picker->close();
        dlg->exec();
    });

    picker->exec();
}
