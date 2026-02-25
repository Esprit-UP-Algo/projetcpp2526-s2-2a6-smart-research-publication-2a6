#include "crudebiosimple.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QVariant>
#include <QTableWidget>
#include <QTableWidgetItem>

CrudeBioSimple::CrudeBioSimple(){}

// ──────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────
static QVariant dateOrNull(const QDate& d)
{
    return d.isValid() ? QVariant(d) : QVariant(QVariant::Date); // NULL DATE
}

static void logErr(const char* tag, const QSqlQuery& q)
{
    QSqlError e = q.lastError();
    qDebug() << tag << e.text();
    qDebug() << "DB:"     << e.databaseText();
    qDebug() << "Driver:" << e.driverText();
    qDebug() << "Query:"  << q.lastQuery();
}

// ──────────────────────────────────────────────
// Status badge
// ──────────────────────────────────────────────
BioStatus CrudeBioSimple::computeStatus(const QDate& expDate, const QString& danger)
{
    if (danger == "BSL-3") return BioStatus::HighRisk;

    QDate today = QDate::currentDate();
    if (expDate.isValid() && expDate < today)              return BioStatus::Expired;
    if (expDate.isValid() && expDate <= today.addDays(30)) return BioStatus::Soon;

    return BioStatus::Ok;
}

// ──────────────────────────────────────────────
// ADD (ID_ECHANTILLON auto via trigger/seq)
// ──────────────────────────────────────────────
bool CrudeBioSimple::add(const BioSample& s)
{
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isValid() || !db.isOpen()) {
        qDebug() << "[ADD] DB not open:" << db.lastError().text();
        return false;
    }

    // Auto-seed: ensure project 1 exists (biosimple module dev — no project module yet)
    // Pure SQL conditional INSERT — works via ODBC without PL/SQL
    QSqlQuery seedQ(db);
    seedQ.exec(
        "INSERT INTO Projet (Id_projet, nom_du_projet) "
        "SELECT 1, 'Projet par defaut' FROM DUAL "
        "WHERE NOT EXISTS (SELECT 1 FROM Projet WHERE Id_projet = 1)"
    );

    // Oracle: no sequence/trigger, generate ID manually
    QSqlQuery idQ(db);
    if (!idQ.exec("SELECT NVL(MAX(ID_ECHANTILLON),0)+1 FROM BIOSAMPLE") || !idQ.next()) {
        m_lastError = idQ.lastError().text();
        logErr("[ADD] Cannot get next ID", idQ);
        return false;
    }
    int nextId = idQ.value(0).toInt();
    qDebug() << "[ADD] next ID_ECHANTILLON =" << nextId << " ID_PROJET =" << s.idProjet;

    QSqlQuery q(db);
    // Use positional ? parameters — more reliable with Qt ODBC driver
    q.prepare(
        "INSERT INTO BIOSAMPLE "
        "(ID_ECHANTILLON, REFERENCE_ECHANTILLON, TYPE_ECHANTILLON, ORGANISME_SOURCE, "
        " EMPLACEMENT_DE_STOCKAGE, TEMPERATURE_DE_STOCKAGE, QUANTITE_RESTANTE, "
        " DATE_DE_COLLECTE, DATE_EXPIRATION, NIVEAU_DE_DANGEROSITE, ID_PROJET) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
        );

    q.addBindValue(nextId);
    q.addBindValue(s.reference);
    q.addBindValue(s.type);
    q.addBindValue(s.organisme);
    q.addBindValue(s.emplacement);
    q.addBindValue(s.temperature.toDouble());
    q.addBindValue(s.quantite);
    q.addBindValue(dateOrNull(s.dateCollecte));
    q.addBindValue(dateOrNull(s.dateExpiration));
    q.addBindValue(s.niveauDanger);
    q.addBindValue(s.idProjet);

    if (!q.exec()) {
        m_lastError = q.lastError().text();
        logErr("[ADD ERROR]", q);
        return false;
    }
    return true;
}

// ──────────────────────────────────────────────
// UPDATE (by reference)
// ──────────────────────────────────────────────
bool CrudeBioSimple::update(const BioSample& s)
{
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isValid() || !db.isOpen()) {
        qDebug() << "[UPDATE] DB not open:" << db.lastError().text();
        return false;
    }

    QSqlQuery q(db);
    q.prepare(
        "UPDATE BIOSAMPLE SET "
        " TYPE_ECHANTILLON        = ?, "
        " ORGANISME_SOURCE        = ?, "
        " EMPLACEMENT_DE_STOCKAGE = ?, "
        " TEMPERATURE_DE_STOCKAGE = ?, "
        " QUANTITE_RESTANTE       = ?, "
        " DATE_DE_COLLECTE        = ?, "
        " DATE_EXPIRATION         = ?, "
        " NIVEAU_DE_DANGEROSITE   = ?, "
        " ID_PROJET               = ? "
        "WHERE REFERENCE_ECHANTILLON = ?"
        );

    q.addBindValue(s.type);
    q.addBindValue(s.organisme);
    q.addBindValue(s.emplacement);
    q.addBindValue(s.temperature.toDouble());
    q.addBindValue(s.quantite);
    q.addBindValue(dateOrNull(s.dateCollecte));
    q.addBindValue(dateOrNull(s.dateExpiration));
    q.addBindValue(s.niveauDanger);
    q.addBindValue(s.idProjet);
    q.addBindValue(s.reference);

    if (!q.exec()) {
        m_lastError = q.lastError().text();
        logErr("[UPDATE ERROR]", q);
        return false;
    }
    return true;
}

// ──────────────────────────────────────────────
// DELETE
// ──────────────────────────────────────────────
bool CrudeBioSimple::remove(const QString& reference)
{
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isValid() || !db.isOpen()) {
        qDebug() << "[DELETE] DB not open:" << db.lastError().text();
        return false;
    }

    QSqlQuery q(db);
    q.prepare("DELETE FROM BIOSAMPLE WHERE REFERENCE_ECHANTILLON = :ref");
    q.bindValue(":ref", reference);

    if (!q.exec()) {
        logErr("[DELETE ERROR]", q);
        return false;
    }
    return true;
}

// ──────────────────────────────────────────────
// GET ONE
// ──────────────────────────────────────────────
BioSample CrudeBioSimple::get(const QString& reference)
{
    BioSample s;

    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isValid() || !db.isOpen()) {
        qDebug() << "[GET] DB not open:" << db.lastError().text();
        return s;
    }

    QSqlQuery q(db);
    q.prepare(
        "SELECT REFERENCE_ECHANTILLON, TYPE_ECHANTILLON, ORGANISME_SOURCE, "
        "       EMPLACEMENT_DE_STOCKAGE, TEMPERATURE_DE_STOCKAGE, QUANTITE_RESTANTE, "
        "       DATE_DE_COLLECTE, DATE_EXPIRATION, NIVEAU_DE_DANGEROSITE, ID_PROJET "
        "FROM BIOSAMPLE WHERE REFERENCE_ECHANTILLON = :ref"
        );
    q.bindValue(":ref", reference);

    if (!q.exec()) {
        logErr("[GET ERROR]", q);
        return s;
    }

    if (q.next()) {
        s.reference      = q.value(0).toString();
        s.type           = q.value(1).toString();
        s.organisme      = q.value(2).toString();
        s.emplacement    = q.value(3).toString();
        s.temperature    = q.value(4).toString();
        s.quantite       = q.value(5).toInt();
        s.dateCollecte   = q.value(6).toDate();
        s.dateExpiration = q.value(7).toDate();
        s.niveauDanger   = q.value(8).toString();
    }

    return s;
}

// ──────────────────────────────────────────────
// LOAD ALL (affichage propre double->QString)
// ──────────────────────────────────────────────
void CrudeBioSimple::loadAll(QTableWidget* table)
{
    if (!table) return;
    table->setRowCount(0);

    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isValid() || !db.isOpen()) {
        qDebug() << "[LOAD] DB not open:" << db.lastError().text();
        return;
    }

    QSqlQuery q(db);
    if (!q.exec(
            "SELECT REFERENCE_ECHANTILLON, TYPE_ECHANTILLON, ORGANISME_SOURCE, "
            "       EMPLACEMENT_DE_STOCKAGE, TEMPERATURE_DE_STOCKAGE, QUANTITE_RESTANTE, "
            "       DATE_DE_COLLECTE, DATE_EXPIRATION, NIVEAU_DE_DANGEROSITE "
            "FROM BIOSAMPLE ORDER BY REFERENCE_ECHANTILLON"
            )) {
        logErr("[LOAD ERROR]", q);
        return;
    }

    auto mk = [](const QString& t, int align = Qt::AlignLeft | Qt::AlignVCenter) {
        auto* it = new QTableWidgetItem(t);
        it->setTextAlignment(align);
        return it;
    };

    int row = 0;
    while (q.next()) {
        table->insertRow(row);

        QString ref = q.value(0).toString();
        QString type = q.value(1).toString();
        QString org  = q.value(2).toString();
        QString emp  = q.value(3).toString();

        QString tempStr = q.value(4).toString();
        QString qtyStr  = q.value(5).toString();

        QDate dc = q.value(6).toDate();
        QDate de = q.value(7).toDate();
        QString danger = q.value(8).toString();

        // Col 0 – Référence (UserRole=reference for CRUD)
        auto* refItem = mk(ref);
        refItem->setData(Qt::UserRole, ref);
        table->setItem(row, 0, refItem);

        // Col 1+ (shifted)
        table->setItem(row, 1, mk(emp));
        table->setItem(row, 2, mk(type));
        table->setItem(row, 3, mk(org));
        table->setItem(row, 4, mk(tempStr.isEmpty() ? "" : tempStr + " °C"));
        table->setItem(row, 5, mk(qtyStr + " µg", Qt::AlignRight | Qt::AlignVCenter));

        table->setItem(row, 6, mk(dc.isValid() ? dc.toString("dd/MM/yyyy") : ""));
        table->setItem(row, 7, mk(de.isValid() ? de.toString("dd/MM/yyyy") : ""));

        // Badge col 8
        BioStatus st = computeStatus(de, danger);
        auto* badge = new QTableWidgetItem;
        badge->setData(Qt::UserRole, static_cast<int>(st));
        table->setItem(row, 8, badge);

        table->setItem(row, 9, mk(danger));
        table->setRowHeight(row, 46);
        ++row;
    }
}

// ──────────────────────────────────────────────
// STATS
// ──────────────────────────────────────────────
QMap<QString, int> CrudeBioSimple::countByType()
{
    QMap<QString, int> result;
    QSqlQuery q("SELECT TYPE_ECHANTILLON, COUNT(*) FROM BIOSAMPLE GROUP BY TYPE_ECHANTILLON");
    while (q.next())
        result[q.value(0).toString()] = q.value(1).toInt();
    return result;
}

QVector<QPair<int, QString>> CrudeBioSimple::countByMonth()
{
    static const QStringList months =
        {"Jan","Fév","Mar","Avr","Mai","Juin","Juil","Août","Sep","Oct","Nov","Déc"};

    QVector<QPair<int, QString>> result;
    result.reserve(12);
    for (int i = 0; i < 12; ++i)
        result.append({0, months[i]});

    QSqlQuery q(
        "SELECT EXTRACT(MONTH FROM DATE_DE_COLLECTE), COUNT(*) "
        "FROM BIOSAMPLE "
        "WHERE DATE_DE_COLLECTE IS NOT NULL "
        "GROUP BY EXTRACT(MONTH FROM DATE_DE_COLLECTE) "
        "ORDER BY EXTRACT(MONTH FROM DATE_DE_COLLECTE)"
        );

    while (q.next()) {
        int m = q.value(0).toInt();
        if (m >= 1 && m <= 12)
            result[m - 1].first = q.value(1).toInt();
    }
    return result;
}

int CrudeBioSimple::totalCount()
{
    QSqlQuery q("SELECT COUNT(*) FROM BIOSAMPLE");
    if (q.exec() && q.next()) return q.value(0).toInt();
    return 0;
}
