#include "publication.h"
#include "connection.h"

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QVariant>
#include <QStringList>

namespace {

QString sanitizeForDisplay(const QString& input)
{
    QString output;
    output.reserve(input.size());
    for (QChar ch : input) {
        if (ch == '\n' || ch == '\r' || ch == '\t' || ch.isPrint()) {
            output.append(ch);
        } else {
            output.append('?');
        }
    }
    return output;
}

QString sqlErrorTypeToString(QSqlError::ErrorType type)
{
    switch (type) {
    case QSqlError::NoError: return "NoError";
    case QSqlError::ConnectionError: return "ConnectionError";
    case QSqlError::StatementError: return "StatementError";
    case QSqlError::TransactionError: return "TransactionError";
    case QSqlError::UnknownError:
    default:
        return "UnknownError";
    }
}

QString formatSqlError(const QSqlError& error, const QString& sql = QString())
{
    QStringList lines;
    lines << "Type: " + sqlErrorTypeToString(error.type());
    lines << "Native code: " + sanitizeForDisplay(error.nativeErrorCode());
    lines << "Driver text: " + sanitizeForDisplay(error.driverText());
    lines << "Database text: " + sanitizeForDisplay(error.databaseText());
    if (!sql.trimmed().isEmpty()) {
        lines << "SQL: " + sanitizeForDisplay(sql);
    }
    return lines.join("\n");
}

bool publicationColumnExists(const QString& columnName)
{
    QSqlQuery query;
    if (!query.prepare(
            "SELECT COUNT(*) "
            "FROM USER_TAB_COLS "
            "WHERE UPPER(TABLE_NAME) = UPPER('Publication') "
            "  AND UPPER(COLUMN_NAME) = UPPER(?)")) {
        return false;
    }
    query.addBindValue(columnName);
    if (!query.exec() || !query.next()) {
        return false;
    }
    return query.value(0).toInt() > 0;
}

bool publicationTableExists(const QString& tableName)
{
    QSqlQuery query;
    if (!query.prepare(
            "SELECT COUNT(*) FROM USER_TABLES WHERE UPPER(TABLE_NAME) = UPPER(?)")) {
        return false;
    }
    query.addBindValue(tableName);
    if (!query.exec() || !query.next()) {
        return false;
    }
    return query.value(0).toInt() > 0;
}

QString publicationLinkTableIdentifier()
{
    if (publicationTableExists("Ecrire")) {
        return "\"Ecrire\"";
    }

    QSqlQuery query;
    if (!query.exec(
            "SELECT table_name "
            "FROM ( "
            "    SELECT table_name "
            "    FROM USER_TAB_COLS "
            "    GROUP BY table_name "
            "    HAVING SUM(CASE WHEN UPPER(column_name) = UPPER('employee_id') THEN 1 ELSE 0 END) > 0 "
            "       AND SUM(CASE WHEN UPPER(column_name) = UPPER('id_publication') THEN 1 ELSE 0 END) > 0 "
            "    ORDER BY table_name "
            ") "
            "WHERE ROWNUM = 1")) {
        return QString();
    }

    if (!query.next()) {
        return QString();
    }

    const QString tableName = query.value(0).toString().trimmed();
    if (tableName.isEmpty()) {
        return QString();
    }

    return QString("\"") + tableName + QString("\"");
}

QString publicationYearColumnIdentifier()
{
    QSqlQuery query;
    if (query.prepare(
            "SELECT COLUMN_NAME "
            "FROM USER_TAB_COLS "
            "WHERE UPPER(TABLE_NAME) = UPPER('Publication') "
            "  AND COLUMN_ID = 4")) {
        if (query.exec() && query.next()) {
            const QString col = query.value(0).toString().trimmed();
            if (!col.isEmpty()) {
                return QString("\"") + col + QString("\"");
            }
        }
    }

    // Fallback for older schemas.
    return "\"année\"";
}

void setError(QString* errorMessage, const QSqlQuery& query);

bool ensureDbConnection(QString* errorMessage)
{
    QSqlDatabase db = QSqlDatabase::database();
    if (db.isValid() && db.isOpen()) {
        return true;
    }

    if (Connection::instance()->createConnect()) {
        return true;
    }

    if (errorMessage) {
        *errorMessage = formatSqlError(QSqlDatabase::database().lastError());
    }
    return false;
}

void setError(QString* errorMessage, const QSqlQuery& query)
{
    if (errorMessage) {
        *errorMessage = formatSqlError(query.lastError(), query.lastQuery());
    }
}

bool replacePublicationEmployeeLink(int publicationId, int employeeId, QString* errorMessage)
{
    const QString linkTable = publicationLinkTableIdentifier();
    if (linkTable.isEmpty()) {
        // Some environments may not have the association table. Do not block publication CRUD.
        return true;
    }

    QSqlQuery deleteQuery;
    if (!deleteQuery.prepare(QString("DELETE FROM %1 WHERE \"id_publication\" = ?").arg(linkTable))) {
        setError(errorMessage, deleteQuery);
        return false;
    }
    deleteQuery.addBindValue(publicationId);

    if (!deleteQuery.exec()) {
        setError(errorMessage, deleteQuery);
        return false;
    }

    if (employeeId <= 0) {
        return true;
    }

    QSqlQuery insertQuery;
    if (!insertQuery.prepare(QString("INSERT INTO %1 (\"employee_id\", \"id_publication\") VALUES (?, ?)").arg(linkTable)) ) {
        setError(errorMessage, insertQuery);
        return false;
    }
    insertQuery.addBindValue(employeeId);
    insertQuery.addBindValue(publicationId);

    if (!insertQuery.exec()) {
        setError(errorMessage, insertQuery);
        return false;
    }

    return true;
}

}

Publication::Publication()
    : m_id(0)
    , m_annee(0)
    , m_idProjet(0)
    , m_employeeId(0)
    , m_impactFactor(0.0)
    , m_citations(QString())
{
}

Publication::Publication(int id,
                         const QString& titre,
                         const QString& journal,
                         int annee,
                         const QString& doi,
                         const QString& status,
                         const QString& abstractText,
                         int idProjet,
                         int employeeId,
                         double impactFactor,
                         const QString& citations)
    : m_id(id)
    , m_titre(titre)
    , m_journal(journal)
    , m_annee(annee)
    , m_doi(doi)
    , m_status(status)
    , m_abstractText(abstractText)
    , m_idProjet(idProjet)
    , m_employeeId(employeeId)
    , m_impactFactor(impactFactor)
    , m_citations(citations)
{
}

int Publication::id() const { return m_id; }
QString Publication::titre() const { return m_titre; }
QString Publication::journal() const { return m_journal; }
int Publication::annee() const { return m_annee; }
QString Publication::doi() const { return m_doi; }
QString Publication::status() const { return m_status; }
QString Publication::abstractText() const { return m_abstractText; }
int Publication::idProjet() const { return m_idProjet; }
int Publication::employeeId() const { return m_employeeId; }
double Publication::impactFactor() const { return m_impactFactor; }
QString Publication::citations() const { return m_citations; }

void Publication::setId(int id) { m_id = id; }
void Publication::setTitre(const QString& titre) { m_titre = titre; }
void Publication::setJournal(const QString& journal) { m_journal = journal; }
void Publication::setAnnee(int annee) { m_annee = annee; }
void Publication::setDoi(const QString& doi) { m_doi = doi; }
void Publication::setStatus(const QString& status) { m_status = status; }
void Publication::setAbstractText(const QString& abstractText) { m_abstractText = abstractText; }
void Publication::setIdProjet(int idProjet) { m_idProjet = idProjet; }
void Publication::setEmployeeId(int employeeId) { m_employeeId = employeeId; }
void Publication::setImpactFactor(double impactFactor) { m_impactFactor = impactFactor; }
void Publication::setCitations(const QString& citations) { m_citations = citations; }

bool Publication::create(QString* errorMessage) const
{
    if (!ensureDbConnection(errorMessage)) {
        return false;
    }

    int generatedId = m_id;
    if (generatedId <= 0) {
        QSqlQuery idQuery;
        if (!idQuery.exec("SELECT NVL(MAX(\"id_publication\"), 0) + 1 FROM \"Publication\"")) {
            setError(errorMessage, idQuery);
            return false;
        }
        if (idQuery.next()) {
            generatedId = idQuery.value(0).toInt();
        } else {
            generatedId = 1;
        }
    }

    const QString yearCol = publicationYearColumnIdentifier();
    const bool hasImpactFactor = publicationColumnExists("impact_factor");
    const bool hasCitations = publicationColumnExists("citations");

    QString sql =
        "INSERT INTO \"Publication\" (\"id_publication\", \"titre\", \"journal\", " + yearCol + ", \"DOI\", \"status\", \"abstract\"";
    if (hasImpactFactor) {
        sql += ", \"impact_factor\"";
    }
    if (hasCitations) {
        sql += ", \"citations\"";
    }
    sql += ") VALUES (?, ?, ?, ?, ?, ?, ?";
    if (hasImpactFactor) {
        sql += ", ?";
    }
    if (hasCitations) {
        sql += ", ?";
    }
    sql += ")";

    QSqlQuery query;
    if (!query.prepare(sql)) {
        setError(errorMessage, query);
        return false;
    }

    query.addBindValue(generatedId);
    query.addBindValue(m_titre);
    query.addBindValue(m_journal);
    query.addBindValue(m_annee);
    query.addBindValue(m_doi);
    query.addBindValue(m_status);
    query.addBindValue(m_abstractText);
    if (hasImpactFactor) {
        query.addBindValue(m_impactFactor);
    }
    if (hasCitations) {
        query.addBindValue(m_citations);
    }

    if (!query.exec()) {
        setError(errorMessage, query);
        return false;
    }

    if (!replacePublicationEmployeeLink(generatedId, m_employeeId, errorMessage)) {
        return false;
    }

    return true;
}

bool Publication::update(QString* errorMessage) const
{
    if (!ensureDbConnection(errorMessage)) {
        return false;
    }

    const QString yearCol = publicationYearColumnIdentifier();
    const bool hasImpactFactor = publicationColumnExists("impact_factor");
    const bool hasCitations = publicationColumnExists("citations");

    QString sql =
        "UPDATE \"Publication\" "
        "SET \"titre\" = ?, "
        "    \"journal\" = ?, "
        "    " + yearCol + " = ?, "
        "    \"DOI\" = ?, "
        "    \"status\" = ?, "
        "    \"abstract\" = ?";
    if (hasImpactFactor) {
        sql += ", \"impact_factor\" = ?";
    }
    if (hasCitations) {
        sql += ", \"citations\" = ?";
    }
    sql += " WHERE \"id_publication\" = ?";

    QSqlQuery query;
    if (!query.prepare(sql)) {
        setError(errorMessage, query);
        return false;
    }

    query.addBindValue(m_titre);
    query.addBindValue(m_journal);
    query.addBindValue(m_annee);
    query.addBindValue(m_doi);
    query.addBindValue(m_status);
    query.addBindValue(m_abstractText);
    if (hasImpactFactor) {
        query.addBindValue(m_impactFactor);
    }
    if (hasCitations) {
        query.addBindValue(m_citations);
    }
    query.addBindValue(m_id);

    if (!query.exec()) {
        setError(errorMessage, query);
        return false;
    }

    if (!replacePublicationEmployeeLink(m_id, m_employeeId, errorMessage)) {
        return false;
    }

    if (query.numRowsAffected() <= 0) {
        if (errorMessage) {
            *errorMessage = "Aucune publication mise a jour (id introuvable).";
        }
        return false;
    }

    return true;
}

bool Publication::remove(int id, QString* errorMessage)
{
    if (!ensureDbConnection(errorMessage)) {
        return false;
    }

    const QString linkTable = publicationLinkTableIdentifier();
    if (!linkTable.isEmpty()) {
        QSqlQuery deleteLinkQuery;
        if (!deleteLinkQuery.prepare(QString("DELETE FROM %1 WHERE \"id_publication\" = ?").arg(linkTable))) {
            setError(errorMessage, deleteLinkQuery);
            return false;
        }
        deleteLinkQuery.addBindValue(id);
        if (!deleteLinkQuery.exec()) {
            setError(errorMessage, deleteLinkQuery);
            return false;
        }
    }

    QSqlQuery query;
    if (!query.prepare("DELETE FROM \"Publication\" WHERE \"id_publication\" = ?")) {
        setError(errorMessage, query);
        return false;
    }
    query.addBindValue(id);

    if (!query.exec()) {
        setError(errorMessage, query);
        return false;
    }

    if (query.numRowsAffected() <= 0) {
        if (errorMessage) {
            *errorMessage = "Aucune publication supprimee (id introuvable).";
        }
        return false;
    }

    return true;
}

bool Publication::readById(int id, Publication& outPublication, QString* errorMessage)
{
    if (!ensureDbConnection(errorMessage)) {
        return false;
    }

    const QString yearCol = publicationYearColumnIdentifier();
    const bool hasImpactFactor = publicationColumnExists("impact_factor");
    const bool hasCitations = publicationColumnExists("citations");

    QString sql =
        "SELECT \"id_publication\", \"titre\", \"journal\", " + yearCol + ", \"DOI\", \"status\", \"abstract\"";
    if (hasImpactFactor) {
        sql += ", NVL(\"impact_factor\", 0)";
    }
    if (hasCitations) {
        sql += ", NVL(\"citations\", '')";
    }
    sql += " FROM \"Publication\" WHERE \"id_publication\" = ?";

    QSqlQuery query;
    if (!query.prepare(sql)) {
        setError(errorMessage, query);
        return false;
    }
    query.addBindValue(id);

    if (!query.exec()) {
        setError(errorMessage, query);
        return false;
    }

    if (!query.next()) {
        if (errorMessage) {
            *errorMessage = "Publication introuvable.";
        }
        return false;
    }

    outPublication.setId(query.value(0).toInt());
    outPublication.setTitre(query.value(1).toString());
    outPublication.setJournal(query.value(2).toString());
    outPublication.setAnnee(query.value(3).toInt());
    outPublication.setDoi(query.value(4).toString());
    outPublication.setStatus(query.value(5).toString());
    outPublication.setAbstractText(query.value(6).toString());
    int metricCol = 7;
    outPublication.setImpactFactor(hasImpactFactor ? query.value(metricCol++).toDouble() : 0.0);
    outPublication.setCitations(hasCitations ? query.value(metricCol).toString() : QString());

    QSqlQuery employeeQuery;
    const QString linkTable = publicationLinkTableIdentifier();
    if (!linkTable.isEmpty() && employeeQuery.prepare(
            QString("SELECT \"employee_id\" FROM %1 WHERE \"id_publication\" = ? ORDER BY \"employee_id\"").arg(linkTable))) {
        employeeQuery.addBindValue(id);
        if (employeeQuery.exec() && employeeQuery.next()) {
            outPublication.setEmployeeId(employeeQuery.value(0).toInt());
        } else {
            outPublication.setEmployeeId(0);
        }
    } else {
        outPublication.setEmployeeId(0);
    }

    return true;
}

QSqlQueryModel* Publication::readAll(QObject* parent, QString* errorMessage)
{
    if (!ensureDbConnection(errorMessage)) {
        return nullptr;
    }

    const QString yearCol = publicationYearColumnIdentifier();
    const bool hasImpactFactor = publicationColumnExists("impact_factor");
    const bool hasCitations = publicationColumnExists("citations");
    const QString linkTable = publicationLinkTableIdentifier();

    const QString impactExpr = hasImpactFactor
        ? "NVL(p.\"impact_factor\", 0)"
        : "0";
    const QString citationExpr = hasCitations
        ? "NVL(p.\"citations\", '')"
        : "''";

    QString employeeExpr = "'Aucun employé'";
    if (!linkTable.isEmpty()) {
        employeeExpr =
            "NVL((SELECT LISTAGG(NVL(NULLIF(TRIM(e.\"FULL_NAME\"), ''), TRIM(e.\"prenom\" || ' ' || e.\"nom\")), ', ') "
            "      WITHIN GROUP (ORDER BY e.\"nom\", e.\"prenom\", e.\"employee_id\") "
            "      FROM " + linkTable + " ec "
            "      JOIN \"Employés\" e ON e.\"employee_id\" = ec.\"employee_id\" "
            "      WHERE ec.\"id_publication\" = p.\"id_publication\"), 'Aucun employé')";
    }

    const QString sql =
        "SELECT p.\"id_publication\", p.\"titre\", p.\"journal\", p." + yearCol + ", p.\"DOI\", p.\"status\", "
        "       " + employeeExpr + " AS \"employe\", "
        "       NVL(p.\"abstract\", '') AS \"mots_cles\", "
        "       " + impactExpr + " AS \"impact_factor\", "
        "       " + citationExpr + " AS \"citations\" "
        "FROM \"Publication\" p "
        "ORDER BY p." + yearCol + " DESC, p.\"id_publication\" DESC";

    QSqlQueryModel* model = new QSqlQueryModel(parent);
    model->setQuery(sql);

    if (model->lastError().isValid()) {
        if (errorMessage) {
            *errorMessage = formatSqlError(model->lastError(), model->query().lastQuery());
        }
        delete model;
        return nullptr;
    }

    model->setHeaderData(0, Qt::Horizontal, "ID");
    model->setHeaderData(1, Qt::Horizontal, "Titre");
    model->setHeaderData(2, Qt::Horizontal, "Journal");
    model->setHeaderData(3, Qt::Horizontal, "Année");
    model->setHeaderData(4, Qt::Horizontal, "DOI");
    model->setHeaderData(5, Qt::Horizontal, "Status");
    model->setHeaderData(6, Qt::Horizontal, "Employé");
    model->setHeaderData(7, Qt::Horizontal, "Mots-clés");
    model->setHeaderData(8, Qt::Horizontal, "Impact Factor");
    model->setHeaderData(9, Qt::Horizontal, "Citations");

    return model;
}
