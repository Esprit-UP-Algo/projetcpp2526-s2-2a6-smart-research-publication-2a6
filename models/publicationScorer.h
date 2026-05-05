#ifndef PUBLICATIONSCORER_H
#define PUBLICATIONSCORER_H

#include "publication.h"
#include <QString>
#include <QStringList>
#include <QJsonObject>

/**
 * @class PublicationScorer
 * @brief AI-powered scoring system for publication health assessment
 * 
 * Provides explainable health scores based on:
 * - Completeness (core fields)
 * - Citation count and impact factor
 * - Recency (publication year)
 * - Project linkage
 * - Duplication risk detection
 */
class PublicationScorer
{
public:
    struct ScoreBreakdown
    {
        double completenessScore;      // 0-100
        double citationScore;          // 0-100
        double impactFactorScore;      // 0-100
        double recencyScore;           // 0-100
        double projectLinkageScore;    // 0-100
        double duplicationRiskScore;   // 0-100 (inverse: high = low risk)
        double totalHealthScore;       // 0-100 overall
    };

    struct NextAction
    {
        QString action;                // Action text
        QString reason;                // Why this action is needed
        int priority;                  // 1-5, where 1 is highest
    };

    /**
     * @brief Calculate health score for a publication
     * @param publication The publication to score
     * @return ScoreBreakdown with all component scores
     */
    static ScoreBreakdown calculateScores(const Publication& publication);

    /**
     * @brief Get next best actions for a publication
     * @param publication The publication to analyze
     * @return List of NextAction items sorted by priority
     */
    static QList<NextAction> getNextActions(const Publication& publication);

    /**
     * @brief Get a simple health status label
     * @param healthScore The overall health score (0-100)
     * @return Status string: "Poor", "Fair", "Good", or "Excellent"
     */
    static QString getHealthStatus(double healthScore);

    /**
     * @brief Get human-readable explanation of the score
     * @param breakdown The score breakdown
     * @return Formatted explanation string
     */
    static QString getExplanation(const ScoreBreakdown& breakdown);

    /**
     * @brief Serialize scores to JSON for API/display
     * @param publication The publication
     * @return QJsonObject with scores and actions
     */
    static QJsonObject toJson(const Publication& publication);

private:
    // Scoring component methods
    static double scoreCompleteness(const Publication& publication);
    static double scoreCitations(const Publication& publication);
    static double scoreImpactFactor(const Publication& publication);
    static double scoreRecency(const Publication& publication);
    static double scoreProjectLinkage(const Publication& publication);
    static double scoreDuplicationRisk(const Publication& publication);

    // Helper methods
    static bool isValidDoi(const QString& doi);
    static QStringList parseCitationEntries(const QString& citations);
    static int countValidCitations(const QString& citations);
    static bool isValidCitationEntry(const QString& citation);
    static bool containsCitationYear(const QString& citation);
    static bool containsCitationDoi(const QString& citation);
    static int getCurrentYear();
    static QString detectDuplicationRisk(const Publication& publication);
};

#endif // PUBLICATIONSCORER_H
