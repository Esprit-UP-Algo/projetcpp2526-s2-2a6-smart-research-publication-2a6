#include "../models/TracabiliteManager.h"
#include <fstream>
#include <iomanip>
#include <chrono>
#include <ctime>

namespace {

const QString LOG_FILE_NAME = "tracabilite_smartvision.txt";
static QString s_currentUserFullName;
static QString s_currentUserEmail;

QString valueOrDash(const QString& value)
{
    const QString trimmed = value.trimmed();
    return trimmed.isEmpty() ? QString("-") : trimmed;
}

QString formatDate(const QDate& date)
{
    return date.isValid() ? date.toString("dd/MM/yyyy") : QString("-");
}

QString formatBudget(double budget)
{
    return QString("%1 TND").arg(QString::number(budget, 'f', 2));
}

QString formatInteger(int value)
{
    return QString::number(value);
}

QString currentDateTime()
{
    using namespace std::chrono;
    const auto now = system_clock::now();
    const std::time_t t = system_clock::to_time_t(now);
    std::tm localTm;
#ifdef _WIN32
    localtime_s(&localTm, &t);
#else
    localtime_r(&t, &localTm);
#endif
    char buffer[64] = {};
    if (std::strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", &localTm) == 0) {
        return QString("-");
    }
    return QString::fromUtf8(buffer);
}

void writeLine(std::ofstream& out, const QString& label, const QString& value)
{
    out << label.toStdString();
    out << " : ";
    out << value.toStdString();
    out << '\n';
}

void writeHeader(std::ofstream& out,
                 const QString& action,
                 const QString& fullName,
                 const QString& email)
{
    out << std::string(50, '=') << '\n';
    writeLine(out, "ACTION", action);
    writeLine(out, "DATE & HEURE", currentDateTime());
    writeLine(out, "UTILISATEUR", valueOrDash(fullName));
    writeLine(out, "EMAIL", valueOrDash(email));
}

QString normalizeForTable(const QString& value)
{
    const QString text = value.trimmed();
    return text.isEmpty() ? QString("-") : text;
}

void writeProjectFields(std::ofstream& out, const ProjetRecord& p)
{
    writeLine(out, "Nom du projet", valueOrDash(p.nomDuProjet));
    writeLine(out, "Domaine", valueOrDash(p.domaineDeRecherche));
    writeLine(out, "Statut", valueOrDash(p.statut));
    writeLine(out, "Date de debut", formatDate(p.dateDeDebut));
    writeLine(out, "Date de fin", formatDate(p.dateDeFin));
    writeLine(out, "Budget", formatBudget(p.budget));
    writeLine(out, "Source financement", valueOrDash(p.sourceDeFinancement));
    writeLine(out, "N approbation ethique", valueOrDash(p.numeroDApprobationEthique));
    writeLine(out, "Nb publications", formatInteger(p.nombreDePublications));
}

void writeComparisonRow(std::ofstream& out,
                        const QString& label,
                        const QString& oldValue,
                        const QString& newValue)
{
    out << std::left << std::setw(26) << label.toStdString();
    out << std::left << std::setw(21) << oldValue.toStdString();
    out << "->  " << newValue.toStdString() << '\n';
}

} // namespace

void TracabiliteManager::logAjoutProjet(const ProjetRecord& p)
{
    std::ofstream out(LOG_FILE_NAME.toStdString(), std::ios::app);
    if (!out.is_open()) return;

    writeHeader(out, "AJOUT PROJET", s_currentUserFullName, s_currentUserEmail);
    out << std::string(50, '-') << '\n';
    writeProjectFields(out, p);
    out << std::string(50, '=') << '\n' << '\n';
}

void TracabiliteManager::logModificationProjet(const ProjetRecord& ancien,
                                                const ProjetRecord& nouveau)
{
    std::ofstream out(LOG_FILE_NAME.toStdString(), std::ios::app);
    if (!out.is_open()) return;

    writeHeader(out, "MODIFICATION PROJET", s_currentUserFullName, s_currentUserEmail);
    out << std::string(50, '-') << '\n';
    writeLine(out, "nom projet", valueOrDash(ancien.nomDuProjet));
    out << std::string(50, '-') << '\n';
    out << std::left << std::setw(26) << "CHAMP";
    out << std::left << std::setw(21) << "VALEUR INITIALE";
    out << "VALEUR FINALE" << '\n';
    out << std::string(50, '-') << '\n';

    bool hasChanges = false;
    const auto addRow = [&](const QString& label,
                            const QString& before,
                            const QString& after) {
        const QString b = normalizeForTable(before);
        const QString a = normalizeForTable(after);
        if (b != a) {
            writeComparisonRow(out, label, b, a);
            hasChanges = true;
        }
    };

    addRow("Nom du projet", ancien.nomDuProjet, nouveau.nomDuProjet);
    addRow("Domaine", ancien.domaineDeRecherche, nouveau.domaineDeRecherche);
    addRow("Statut", ancien.statut, nouveau.statut);
    addRow("Date de debut", formatDate(ancien.dateDeDebut), formatDate(nouveau.dateDeDebut));
    addRow("Date de fin", formatDate(ancien.dateDeFin), formatDate(nouveau.dateDeFin));
    addRow("Budget", formatBudget(ancien.budget), formatBudget(nouveau.budget));
    addRow("Source financement", ancien.sourceDeFinancement, nouveau.sourceDeFinancement);
    addRow("N approbation ethique", ancien.numeroDApprobationEthique, nouveau.numeroDApprobationEthique);
    addRow("Nb publications",
           formatInteger(ancien.nombreDePublications),
           formatInteger(nouveau.nombreDePublications));

    if (!hasChanges) {
        writeLine(out, "Remarque", "Aucun champ modifié.");
    }

    out << std::string(50, '=') << '\n' << '\n';
}

void TracabiliteManager::logSuppressionProjet(const ProjetRecord& p)
{
    std::ofstream out(LOG_FILE_NAME.toStdString(), std::ios::app);
    if (!out.is_open()) return;

    writeHeader(out, "SUPPRESSION PROJET", s_currentUserFullName, s_currentUserEmail);
    out << std::string(50, '-') << '\n';
    out << "ATTENTION: Ce projet a ete definitivement supprime" << '\n';
    out << std::string(50, '-') << '\n';
    out << '\n';
    writeProjectFields(out, p);
    out << std::string(50, '=') << '\n' << '\n';
}

void TracabiliteManager::logConnexion(const QString& fullName,
                                       const QString& email,
                                       const QString& role)
{
    std::ofstream out(LOG_FILE_NAME.toStdString(), std::ios::app);
    if (!out.is_open()) return;

    writeHeader(out, "CONNEXION", fullName, email);
    out << std::string(50, '-') << '\n';
    writeLine(out, "Role", valueOrDash(role));
    out << std::string(50, '=') << '\n' << '\n';
}

void TracabiliteManager::logDeconnexion(const QString& fullName,
                                         const QString& email)
{
    std::ofstream out(LOG_FILE_NAME.toStdString(), std::ios::app);
    if (!out.is_open()) return;

    writeHeader(out, "DECONNEXION", fullName, email);
    out << std::string(50, '-') << '\n';
    out << std::string(50, '=') << '\n' << '\n';
}

void TracabiliteManager::setUserContext(const QString& fullName, const QString& email)
{
    s_currentUserFullName = fullName.trimmed();
    s_currentUserEmail = email.trimmed();
}

void TracabiliteManager::clearUserContext()
{
    s_currentUserFullName.clear();
    s_currentUserEmail.clear();
}
