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

bool ensurePublicationTableExists(QString* errorMessage)
{
    QSqlQuery existsQuery;
    if (!existsQuery.prepare("SELECT COUNT(*) FROM USER_TABLES WHERE TABLE_NAME = 'PUBLICATION'")) {
        setError(errorMessage, existsQuery);
        return false;
    }

    if (!existsQuery.exec()) {
        setError(errorMessage, existsQuery);
        return false;
    }

    bool tableExists = false;
    if (existsQuery.next()) {
        tableExists = existsQuery.value(0).toInt() > 0;
    }

    if (tableExists) {
        return true;
    }

    QSqlQuery createQuery;
    const QString ddl =
        "CREATE TABLE Publication ("
        "id NUMBER PRIMARY KEY, "
        "titre VARCHAR2(300) NOT NULL, "
        "journal VARCHAR2(200), "
        "annee NUMBER(4), "
        "DOI VARCHAR2(100) UNIQUE, "
        "status VARCHAR2(50), "
        "abstract CLOB, "
        "Id_projet NUMBER, "
        "employee_id NUMBER"
        ")";

    if (!createQuery.exec(ddl)) {
        setError(errorMessage, createQuery);
        return false;
    }

    return true;
}

void setError(QString* errorMessage, const QSqlQuery& query)
{
    if (errorMessage) {
        *errorMessage = formatSqlError(query.lastError(), query.lastQuery());
    }
}

}

Publication::Publication()
    : m_id(0)
    , m_annee(0)
    , m_idProjet(0)
    , m_employeeId(0)
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
                         int employeeId)
    : m_id(id)
    , m_titre(titre)
    , m_journal(journal)
    , m_annee(annee)
    , m_doi(doi)
    , m_status(status)
    , m_abstractText(abstractText)
    , m_idProjet(idProjet)
    , m_employeeId(employeeId)
{
}

int Publication::id() const
{
    return m_id;
}

QString Publication::titre() const
{
    return m_titre;
}

QString Publication::journal() const
{
    return m_journal;
}

int Publication::annee() const
{
    return m_annee;
}

QString Publication::doi() const
{
    return m_doi;
}

QString Publication::status() const
{
    return m_status;
}

QString Publication::abstractText() const
{
    return m_abstractText;
}

int Publication::idProjet() const
{
    return m_idProjet;
}

int Publication::employeeId() const
{
    return m_employeeId;
}

void Publication::setId(int id)
{
    m_id = id;
}

void Publication::setTitre(const QString& titre)
{
    m_titre = titre;
}

void Publication::setJournal(const QString& journal)
{
    m_journal = journal;
}

void Publication::setAnnee(int annee)
{
    m_annee = annee;
}

void Publication::setDoi(const QString& doi)
{
    m_doi = doi;
}

void Publication::setStatus(const QString& status)
{
    m_status = status;
}

void Publication::setAbstractText(const QString& abstractText)
{
    m_abstractText = abstractText;
}

void Publication::setIdProjet(int idProjet)
{
    m_idProjet = idProjet;
}

void Publication::setEmployeeId(int employeeId)
{
    m_employeeId = employeeId;
}

bool Publication::create(QString* errorMessage) const
{
    if (!ensureDbConnection(errorMessage)) {
        return false;
    }
    if (!ensurePublicationTableExists(errorMessage)) {
        return false;
    }

    int generatedId = m_id;
    if (generatedId <= 0) {
        QSqlQuery idQuery;
        if (!idQuery.exec("SELECT NVL(MAX(id), 0) + 1 FROM Publication")) {
            setError(errorMessage, idQuery);
            return false;
        }
        if (idQuery.next()) {
            generatedId = idQuery.value(0).toInt();
        } else {
            generatedId = 1;
        }
    }

    QSqlQuery query;
    if (!query.prepare("INSERT INTO Publication (id, titre, journal, annee, DOI, status, abstract, Id_projet, employee_id) "
                       "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)")) {
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
    query.addBindValue(m_idProjet > 0 ? m_idProjet : 1);
    query.addBindValue(m_employeeId > 0 ? m_employeeId : 1);

    if (!query.exec()) {
        setError(errorMessage, query);
        return false;
    }

    return true;
}

bool Publication::update(QString* errorMessage) const
{
    if (!ensureDbConnection(errorMessage)) {
        return false;
    }
    if (!ensurePublicationTableExists(errorMessage)) {
        return false;
    }

    QSqlQuery query;
    if (!query.prepare("UPDATE Publication "
                       "SET titre = ?, "
                       "    journal = ?, "
                       "    annee = ?, "
                       "    DOI = ?, "
                       "    status = ?, "
                       "    abstract = ?, "
                       "    Id_projet = ?, "
                       "    employee_id = ? "
                       "WHERE id = ?")) {
        setError(errorMessage, query);
        return false;
    }

    query.addBindValue(m_titre);
    query.addBindValue(m_journal);
    query.addBindValue(m_annee);
    query.addBindValue(m_doi);
    query.addBindValue(m_status);
    query.addBindValue(m_abstractText);
    query.addBindValue(m_idProjet > 0 ? m_idProjet : 1);
    query.addBindValue(m_employeeId > 0 ? m_employeeId : 1);
    query.addBindValue(m_id);

    if (!query.exec()) {
        setError(errorMessage, query);
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
    if (!ensurePublicationTableExists(errorMessage)) {
        return false;
    }

    QSqlQuery query;
    if (!query.prepare("DELETE FROM Publication WHERE id = ?")) {
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
    if (!ensurePublicationTableExists(errorMessage)) {
        return false;
    }

    QSqlQuery query;
    if (!query.prepare("SELECT id, titre, journal, annee, DOI, status, abstract, Id_projet, employee_id "
                       "FROM Publication "
                       "WHERE id = ?")) {
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
    outPublication.setIdProjet(query.value(7).toInt());
    outPublication.setEmployeeId(query.value(8).toInt());

    return true;
}

QSqlQueryModel* Publication::readAll(QObject* parent, QString* errorMessage)
{
    if (!ensureDbConnection(errorMessage)) {
        return nullptr;
    }
    if (!ensurePublicationTableExists(errorMessage)) {
        return nullptr;
    }

    QSqlQueryModel* model = new QSqlQueryModel(parent);
    model->setQuery("SELECT id, titre, journal, annee, DOI, status, abstract, Id_projet, employee_id "
                    "FROM Publication "
                    "ORDER BY annee DESC, id DESC");

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
    model->setHeaderData(3, Qt::Horizontal, "Annee");
    model->setHeaderData(4, Qt::Horizontal, "DOI");
    model->setHeaderData(5, Qt::Horizontal, "Status");
    model->setHeaderData(6, Qt::Horizontal, "Abstract");
    model->setHeaderData(7, Qt::Horizontal, "Id_projet");
    model->setHeaderData(8, Qt::Horizontal, "Employee_id");

    return model;
}
