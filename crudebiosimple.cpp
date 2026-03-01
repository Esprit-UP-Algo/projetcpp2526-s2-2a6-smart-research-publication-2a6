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
    return d.isValid() ? QVariant(d) : QVariant(QMetaType::fromType<QDate>());
}

static QVariant normalizeStorageTemperature(const QString& raw)
{
    const QString t = raw.trimmed();
    if (t.isEmpty() || t == "-" || t == "--") return QVariant(QMetaType::fromType<double>());
    const QString lower = t.toLower();
    if (lower.contains("-80")) return QVariant(-80.0);
    if (lower.contains("-20")) return QVariant(-20.0);
    if (lower.contains("+4") || lower == "4" || lower.contains("4c")) return QVariant(4.0);
    if (lower.contains("amb")) return QVariant(QMetaType::fromType<double>());
    // try direct numeric conversion
    bool ok = false;
    double v = t.toDouble(&ok);
    if (ok) return QVariant(v);
    return QVariant(QMetaType::fromType<double>());
}

static QString normalizeSampleType(const QString& raw)
{
    const QString t = raw.trimmed();
    if (t.isEmpty()) return QString();

    const QString lower = t.toLower();
    if (lower == "adn" || lower == "dna") return "ADN";
    if (lower == "arn" || lower == "rna") return "ARN";
    if (lower.contains("prot")) return "Protéine";
    if (lower.contains("cell")) return "Cellule";
    if (lower.contains("tiss")) return "Tissu";
    if (lower.contains("organ")) return "Organisme";

    return QString();
}

static QString normalizeDangerLevel(const QString& raw)
{
    const QString t = raw.trimmed();
    if (t.isEmpty()) return "BSL-1";

    if (t == "BSL-1" || t == "BSL-2" || t == "BSL-3") return t;
    return QString();
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

    const QString normalizedType = normalizeSampleType(s.type);
    const QString normalizedDanger = normalizeDangerLevel(s.niveauDanger);
    if (normalizedType.isEmpty()) {
        m_lastError = "Type invalide. Valeurs autorisées: ADN, ARN, Protéine, Cellule, Tissu, Organisme.";
        return false;
    }
    if (normalizedDanger.isEmpty()) {
        m_lastError = "Niveau de danger invalide. Valeurs autorisées: BSL-1, BSL-2, BSL-3.";
        return false;
    }

    // Oracle: generate ID manually if trigger is not used by this module
    QSqlQuery idQ(db);
    // Auto-seed project 1 if needed (BIOSAMPLE.ID_PROJET is NOT NULL)
    QSqlQuery seedQ(db);
    seedQ.exec(
        "INSERT INTO PROJET (ID_PROJET, NOM_DU_PROJET) "
        "SELECT 1, 'Projet par defaut' FROM DUAL "
        "WHERE NOT EXISTS (SELECT 1 FROM PROJET WHERE ID_PROJET = 1)"
    );

    if (!idQ.exec("SELECT NVL(MAX(ID_ECHANTILLON),0)+1 FROM BIOSAMPLE") || !idQ.next()) {
        m_lastError = idQ.lastError().text();
        logErr("[ADD] Cannot get next ID", idQ);
        return false;
    }
    int nextId = idQ.value(0).toInt();
    qDebug() << "[ADD] next ID_ECHANTILLON =" << nextId;

    QSqlQuery q(db);
    q.prepare(
        "INSERT INTO BIOSAMPLE "
        "(ID_ECHANTILLON, REFERENCE_ECHANTILLON, TYPE_ECHANTILLON, ORGANISME_SOURCE, "
        " EMPLACEMENT_DE_STOCKAGE, TEMPERATURE_DE_STOCKAGE, QUANTITE_RESTANTE, "
        " DATE_DE_COLLECTE, DATE_EXPIRATION, NIVEAU_DE_DANGEROSITE, ID_PROJET) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
        );

    q.addBindValue(nextId);
    q.addBindValue(s.reference);
    q.addBindValue(normalizedType);
    q.addBindValue(s.organisme);
    q.addBindValue(s.emplacement);
    q.addBindValue(normalizeStorageTemperature(s.temperature));
    q.addBindValue(s.quantite);
    q.addBindValue(dateOrNull(s.dateCollecte));
    q.addBindValue(dateOrNull(s.dateExpiration));
    q.addBindValue(normalizedDanger);
    q.addBindValue(s.idProjet > 0 ? s.idProjet : 1);

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

    const QString normalizedType = normalizeSampleType(s.type);
    const QString normalizedDanger = normalizeDangerLevel(s.niveauDanger);
    if (normalizedType.isEmpty()) {
        m_lastError = "Type invalide. Valeurs autorisées: ADN, ARN, Protéine, Cellule, Tissu, Organisme.";
        return false;
    }
    if (normalizedDanger.isEmpty()) {
        m_lastError = "Niveau de danger invalide. Valeurs autorisées: BSL-1, BSL-2, BSL-3.";
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
        " NIVEAU_DE_DANGEROSITE   = ? "
        "WHERE REFERENCE_ECHANTILLON = ?"
        );

    q.addBindValue(normalizedType);
    q.addBindValue(s.organisme);
    q.addBindValue(s.emplacement);
    q.addBindValue(normalizeStorageTemperature(s.temperature));
    q.addBindValue(s.quantite);
    q.addBindValue(dateOrNull(s.dateCollecte));
    q.addBindValue(dateOrNull(s.dateExpiration));
    q.addBindValue(normalizedDanger);
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
        "       DATE_DE_COLLECTE, DATE_EXPIRATION, NIVEAU_DE_DANGEROSITE "
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
