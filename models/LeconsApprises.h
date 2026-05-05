#ifndef LECONSAPPRISES_H
#define LECONSAPPRISES_H

// ─────────────────────────────────────────────────────────────
//  LeconsApprises.h
//  Feature: Leçons Apprises — Gestion Projet (SmartVision)
//
//  READ-ONLY feature.
//  Analyzes the current project + all similar completed projects
//  in the same domaine_de_recherche, then generates:
//    - Avertissements  (red)
//    - Points Positifs (green)
//    - Recommandations (blue)
//    - Taux de succès global
//  Plus an AI enrichment step (GROQ API) that searches for
//  real-world research projects in the same domain.
//  Export to .txt/.doc or .pdf is built-in.
// ─────────────────────────────────────────────────────────────

#include <QString>
#include <QStringList>
#include <QWidget>

// ─────────────────────────────────────────────────────────────
//  Result structure returned by LeconsApprises::analyze()
// ─────────────────────────────────────────────────────────────
struct LeconsApprisesResult
{
    QString     domaine;
    int         nombreProjetsSimilaires = 0;

    QStringList avertissements;     // ⚠  red   — problems detected in similar projects
    QStringList pointsPositifs;     // ✔  green — things that went well
    QStringList recommandations;    // →  blue  — actionable suggestions

    QString     tauxSuccesGlobal;   // formatted summary string
    bool        donneesSuffisantes = false;
    QString     messageVide;        // filled when donneesSuffisantes == false
};

// ─────────────────────────────────────────────────────────────
//  Main class — all methods are static, no state
// ─────────────────────────────────────────────────────────────
class LeconsApprises
{
public:
    // Core analysis: queries DB, runs 8 analyses, returns result struct
    static LeconsApprisesResult analyze(int idProjetCourant);

    // Self-contained dialog:
    //   1. Project picker list
    //   2. Full result display (warnings / positives / recommendations)
    //   3. AI enrichment section (GROQ API — real-world similar projects)
    //   4. Export buttons (PDF and TXT/DOC)
    static void showDialog(QWidget* parent = nullptr);
};

#endif // LECONSAPPRISES_H
