#include "publicationScorer.h"
#include <QDateTime>
#include <QJsonArray>
#include <QRegularExpression>
#include <algorithm>
#include <cmath>

PublicationScorer::ScoreBreakdown PublicationScorer::calculateScores(const Publication& publication)
{
    ScoreBreakdown breakdown;

    // Calculate individual component scores (0-100)
    breakdown.completenessScore = scoreCompleteness(publication);
    breakdown.citationScore = scoreCitations(publication);
    breakdown.impactFactorScore = scoreImpactFactor(publication);
    breakdown.recencyScore = scoreRecency(publication);
    breakdown.projectLinkageScore = scoreProjectLinkage(publication);
    breakdown.duplicationRiskScore = scoreDuplicationRisk(publication);

    // Calculate weighted total (all components are equally weighted for now)
    breakdown.totalHealthScore = 
        (breakdown.completenessScore * 0.25 +
         breakdown.citationScore * 0.20 +
         breakdown.impactFactorScore * 0.15 +
         breakdown.recencyScore * 0.15 +
         breakdown.projectLinkageScore * 0.15 +
         breakdown.duplicationRiskScore * 0.10);

    // Ensure score is within 0-100 range
    breakdown.totalHealthScore = std::min(100.0, std::max(0.0, breakdown.totalHealthScore));

    return breakdown;
}

double PublicationScorer::scoreCompleteness(const Publication& publication)
{
    int completedFields = 0;
    int totalFields = 0;

    // Check core fields
    totalFields += 6; // titre, journal, doi, abstractText, status, annee
    if (!publication.titre().trimmed().isEmpty()) completedFields++;
    if (!publication.journal().trimmed().isEmpty()) completedFields++;
    if (!publication.doi().trimmed().isEmpty()) completedFields++;
    if (!publication.abstractText().trimmed().isEmpty()) completedFields++;
    if (!publication.status().trimmed().isEmpty()) completedFields++;
    if (publication.annee() > 1900) completedFields++; // Reasonable year range

    return (completedFields / static_cast<double>(totalFields)) * 100.0;
}

double PublicationScorer::scoreCitations(const Publication& publication)
{
    const QString citations = publication.citations().trimmed();
    if (citations.isEmpty()) {
        return 0.0;
    }

    const int validCitations = countValidCitations(citations);
    if (validCitations <= 0) {
        return 0.0;
    }

    const double baseScore = qMin(100.0, (validCitations / 50.0) * 100.0);
    const double doiBonus = containsCitationDoi(citations) ? 10.0 : 0.0;
    const double yearBonus = validCitations >= 2 ? 5.0 : 0.0;
    const double qualityFactor = qMax(0.8, qMin(1.0, validCitations / 10.0));

    return qMin(100.0, (baseScore * qualityFactor) + doiBonus + yearBonus);
}

double PublicationScorer::scoreImpactFactor(const Publication& publication)
{
    // Score based on impact factor
    // Scale: 0 = 0, 10+ = 100
    double impactFactor = publication.impactFactor();
    
    if (impactFactor <= 0) return 20.0; // Slight credit for published even without IF
    if (impactFactor >= 10) return 100.0;
    
    // Logarithmic scale (impact factors often follow this pattern)
    return (impactFactor / 10.0) * 100.0;
}

double PublicationScorer::scoreRecency(const Publication& publication)
{
    // Score based on how recent the publication is
    // Current year = 100, 5+ years old = 50, 10+ years old = 0
    int currentYear = getCurrentYear();
    int publicationYear = publication.annee();
    
    if (publicationYear <= 0) return 0.0; // Invalid year
    
    int yearsDiff = currentYear - publicationYear;
    
    if (yearsDiff <= 0) return 100.0;
    if (yearsDiff >= 10) return 0.0;
    
    // Linear decay over 10 years
    return std::max(0.0, 100.0 - (yearsDiff * 10.0));
}

double PublicationScorer::scoreProjectLinkage(const Publication& publication)
{
    // Score based on whether the publication is linked to a project
    if (publication.idProjet() > 0) {
        return 100.0; // Fully linked
    }
    
    if (publication.employeeId() > 0) {
        return 50.0; // Partially linked (at least has an author)
    }
    
    return 0.0; // No linkage
}

double PublicationScorer::scoreDuplicationRisk(const Publication& publication)
{
    // Score based on duplication risk detection
    // Higher score = lower risk
    
    QString riskFactors = detectDuplicationRisk(publication);
    
    if (riskFactors.isEmpty()) {
        return 100.0; // No risk detected
    }
    
    // Count detected risk factors
    int riskCount = riskFactors.split("|").size();
    
    if (riskCount >= 3) return 0.0;   // High risk
    if (riskCount == 2) return 40.0;  // Medium risk
    if (riskCount == 1) return 70.0;  // Low risk
    
    return 100.0; // No risk
}

QString PublicationScorer::detectDuplicationRisk(const Publication& publication)
{
    QStringList risks;
    
    // Check for suspiciously short fields (common in duplicates/spam)
    if (publication.titre().length() < 5) {
        risks.append("short_title");
    }
    
    if (publication.abstractText().length() < 20 && !publication.abstractText().isEmpty()) {
        risks.append("very_short_abstract");
    }
    
    // Check for suspicious DOI format
    if (!publication.doi().isEmpty() && !isValidDoi(publication.doi())) {
        risks.append("invalid_doi_format");
    }
    
    // Citation quality risk
    const QString citations = publication.citations().trimmed();
    if (!citations.isEmpty() && countValidCitations(citations) == 0) {
        risks.append("invalid_citations");
    }
    
    // Check for missing metadata
    if (publication.status().isEmpty() && publication.journal().isEmpty()) {
        risks.append("missing_metadata");
    }
    
    return risks.join("|");
}

bool PublicationScorer::isValidDoi(const QString& doi)
{
    // Basic DOI validation: should start with 10. and contain /
    if (doi.isEmpty()) return false;
    
    QRegularExpression doiRegex("^10\\.\\d{4,}/");
    return doiRegex.match(doi).hasMatch();
}

QStringList PublicationScorer::parseCitationEntries(const QString& citations)
{
    QStringList entries = citations
        .split(QRegularExpression("[;\n\r]+"), Qt::SkipEmptyParts);

    for (QString& entry : entries) {
        entry = entry.trimmed();
        if (entry.endsWith('.')) {
            entry.chop(1);
        }
    }
    return entries;
}

int PublicationScorer::countValidCitations(const QString& citations)
{
    int validCount = 0;
    for (const QString& entry : parseCitationEntries(citations)) {
        if (isValidCitationEntry(entry)) {
            validCount++;
        }
    }
    return validCount;
}

bool PublicationScorer::isValidCitationEntry(const QString& citation)
{
    const QString trimmed = citation.trimmed();
    if (trimmed.isEmpty() || trimmed.length() < 10) {
        return false;
    }

    if (!containsCitationYear(trimmed)) {
        return false;
    }

    if (trimmed.contains(QRegularExpression("\\bet al\\.?\\b", QRegularExpression::CaseInsensitiveOption))) {
        return true;
    }

    if (trimmed.contains(QRegularExpression("\\b[A-Z][a-z]+\\s+[A-Z][a-z]+\\b"))) {
        return true;
    }

    return containsCitationDoi(trimmed);
}

bool PublicationScorer::containsCitationYear(const QString& citation)
{
    QRegularExpression yearRegex("\\b(19|20)\\d{2}\\b");
    return yearRegex.match(citation).hasMatch();
}

bool PublicationScorer::containsCitationDoi(const QString& citation)
{
    QRegularExpression doiRefRegex("(10\\.\\d{4,9}/[^\\s]+)|doi\\s*:\\s*10\\.\\d{4,9}/[^\\s]+", QRegularExpression::CaseInsensitiveOption);
    return doiRefRegex.match(citation).hasMatch();
}

QList<PublicationScorer::NextAction> PublicationScorer::getNextActions(const Publication& publication)
{
    QList<NextAction> actions;

    // Priority 1: Critical missing fields
    if (publication.abstractText().trimmed().isEmpty()) {
        actions.append({
            "Add abstract",
            "Abstracts improve discoverability and impact assessment",
            1
        });
    }

    if (publication.doi().trimmed().isEmpty()) {
        actions.append({
            "Verify or add DOI",
            "DOI significantly improves citation tracking and impact",
            1
        });
    }

    // Priority 2: Enhance metadata
    if (publication.employeeId() <= 0) {
        actions.append({
            "Link an employee",
            "Linking employees enables better tracking and team metrics",
            2
        });
    }

    if (publication.status() != "Publiée" && !publication.status().isEmpty()) {
        actions.append({
            QString("Update status to 'Publiée'"),
            "Marking as published improves visibility and statistics",
            2
        });
    }

    // Priority 3: Optimize impact tracking
    if (publication.citations().trimmed().isEmpty()) {
        actions.append({
            "Check for citations",
            "Verifying citations helps assess publication impact",
            3
        });
    } else if (countValidCitations(publication.citations()) == 0) {
        actions.append({
            "Review citation format",
            "Citations should look like real scientific references with author names and year",
            3
        });
    }

    // Priority 4: Linkage enhancement
    if (publication.idProjet() <= 0 && publication.employeeId() > 0) {
        actions.append({
            "Link to a project",
            "Project linkage connects your publication to research initiatives",
            4
        });
    }

    // Priority 5: Quality improvements
    if (publication.journal().trimmed().isEmpty()) {
        actions.append({
            "Verify journal name",
            "Journal information is important for impact assessment",
            5
        });
    }

    // Sort by priority
    std::sort(actions.begin(), actions.end(), [](const NextAction& a, const NextAction& b) {
        return a.priority < b.priority;
    });

    return actions;
}

QString PublicationScorer::getHealthStatus(double healthScore)
{
    if (healthScore >= 80) return "Excellent";
    if (healthScore >= 60) return "Good";
    if (healthScore >= 40) return "Fair";
    return "Poor";
}

QString PublicationScorer::getExplanation(const ScoreBreakdown& breakdown)
{
    QString explanation;
    explanation += QString("Publication Health Assessment\n");
    explanation += QString("=============================\n\n");
    
    explanation += QString("Overall Health Score: %1/100 (%2)\n\n")
        .arg(static_cast<int>(breakdown.totalHealthScore))
        .arg(getHealthStatus(breakdown.totalHealthScore));
    
    explanation += QString("Component Breakdown:\n");
    explanation += QString("- Completeness:      %1%\n").arg(static_cast<int>(breakdown.completenessScore));
    explanation += QString("- Citations:         %1%\n").arg(static_cast<int>(breakdown.citationScore));
    explanation += QString("- Impact Factor:     %1%\n").arg(static_cast<int>(breakdown.impactFactorScore));
    explanation += QString("- Recency:           %1%\n").arg(static_cast<int>(breakdown.recencyScore));
    explanation += QString("- Project Linkage:   %1%\n").arg(static_cast<int>(breakdown.projectLinkageScore));
    explanation += QString("- Duplication Risk:  %1%\n").arg(static_cast<int>(breakdown.duplicationRiskScore));
    
    return explanation;
}

QJsonObject PublicationScorer::toJson(const Publication& publication)
{
    ScoreBreakdown scores = calculateScores(publication);
    QList<NextAction> actions = getNextActions(publication);
    
    QJsonObject json;
    
    // Add score breakdown
    QJsonObject scoreBreakdown;
    scoreBreakdown["completeness"] = scores.completenessScore;
    scoreBreakdown["citations"] = scores.citationScore;
    scoreBreakdown["impact_factor"] = scores.impactFactorScore;
    scoreBreakdown["recency"] = scores.recencyScore;
    scoreBreakdown["project_linkage"] = scores.projectLinkageScore;
    scoreBreakdown["duplication_risk"] = scores.duplicationRiskScore;
    scoreBreakdown["total"] = scores.totalHealthScore;
    
    json["scores"] = scoreBreakdown;
    json["health_status"] = getHealthStatus(scores.totalHealthScore);
    
    // Add next actions
    QJsonArray actionsArray;
    for (const auto& action : actions) {
        QJsonObject actionObj;
        actionObj["action"] = action.action;
        actionObj["reason"] = action.reason;
        actionObj["priority"] = action.priority;
        actionsArray.append(actionObj);
    }
    json["next_actions"] = actionsArray;
    
    // Add explanation
    json["explanation"] = getExplanation(scores);
    
    return json;
}

int PublicationScorer::getCurrentYear()
{
    return QDateTime::currentDateTime().date().year();
}
