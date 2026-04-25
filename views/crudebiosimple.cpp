#include "crudebiosimple.h"
#include "saisiebio.h"

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
// ADD
// ──────────────────────────────────────────────
bool CrudeBioSimple::add(const BioSample& s)
{
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isValid() || !db.isOpen()) {
        qDebug() << "[ADD] DB not open:" << db.lastError().text();
        return false;
    }

    const QString normalizedType   = SaisieBio::normalizeSampleType(s.type);
    const QString normalizedDanger = SaisieBio::normalizeDangerLevel(s.niveauDanger);
    if (normalizedType.isEmpty()) {
        m_lastError = "Type invalide. Valeurs autorisées: ADN, ARN, Protéine, Cellule, Tissu, Organisme.";
        return false;
    }
    if (normalizedDanger.isEmpty()) {
        m_lastError = "Niveau de danger invalide. Valeurs autorisées: BSL-1, BSL-2, BSL-3.";
        return false;
    }

    QSqlQuery idQ(db);
    QSqlQuery seedQ(db);
    seedQ.exec(
        "INSERT INTO \"projet\" (\"Id_projet\", \"nom_du_projet\") "
        "SELECT 1, 'Projet par defaut' FROM DUAL "
        "WHERE NOT EXISTS (SELECT 1 FROM \"projet\" WHERE \"Id_projet\" = 1)"
    );

    if (!idQ.exec("SELECT NVL(MAX(\"ID_de_léchantillon\"),0)+1 FROM \"BioSample\"") || !idQ.next()) {
        m_lastError = idQ.lastError().text();
        logErr("[ADD] Cannot get next ID", idQ);
        return false;
    }
    int nextId = idQ.value(0).toInt();
    qDebug() << "[ADD] next ID_de_léchantillon =" << nextId;

    QSqlQuery q(db);
    q.prepare(
        "INSERT INTO \"BioSample\" "
        "(\"ID_de_léchantillon\", \"Reference_de_léchantillon\", \"Type_déchantillon\", \"Organisme_source\", "
        " \"Emplacement_de_stockage\", \"Température_de_stockage\", \"Quantité_restante\", "
        " \"Date_de_collecte\", \"Date_dexpiration\", \"Niveau_de_dangerosité\", \"Id_projet\") "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
        );

    q.addBindValue(nextId);
    q.addBindValue(s.reference);
    q.addBindValue(normalizedType);
    q.addBindValue(s.organisme);
    q.addBindValue(s.emplacement);
    q.addBindValue(SaisieBio::normalizeStorageTemperature(s.temperature));
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
bool CrudeBioSimple::update(const BioSample& s, const QString& oldRef)
{
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isValid() || !db.isOpen()) {
        qDebug() << "[UPDATE] DB not open:" << db.lastError().text();
        return false;
    }

    const QString normalizedType   = SaisieBio::normalizeSampleType(s.type);
    const QString normalizedDanger = SaisieBio::normalizeDangerLevel(s.niveauDanger);
    if (normalizedType.isEmpty()) {
        m_lastError = "Type invalide. Valeurs autorisées: ADN, ARN, Protéine, Cellule, Tissu, Organisme.";
        return false;
    }
    if (normalizedDanger.isEmpty()) {
        m_lastError = "Niveau de danger invalide. Valeurs autorisées: BSL-1, BSL-2, BSL-3.";
        return false;
    }

    // Si oldRef non fourni, on cible la référence actuelle (rétro-compatibilité)
    const QString whereRef = oldRef.isEmpty() ? s.reference : oldRef;

    QSqlQuery q(db);
    q.prepare(
        "UPDATE \"BioSample\" SET "
        " \"Reference_de_léchantillon\" = ?, "
        " \"Type_déchantillon\"         = ?, "
        " \"Organisme_source\"          = ?, "
        " \"Emplacement_de_stockage\"   = ?, "
        " \"Température_de_stockage\"   = ?, "
        " \"Quantité_restante\"         = ?, "
        " \"Date_de_collecte\"          = ?, "
        " \"Date_dexpiration\"          = ?, "
        " \"Niveau_de_dangerosité\"     = ? "
        "WHERE \"Reference_de_léchantillon\" = ?"
        );

    q.addBindValue(s.reference);
    q.addBindValue(normalizedType);
    q.addBindValue(s.organisme);
    q.addBindValue(s.emplacement);
    q.addBindValue(SaisieBio::normalizeStorageTemperature(s.temperature));
    q.addBindValue(s.quantite);
    q.addBindValue(dateOrNull(s.dateCollecte));
    q.addBindValue(dateOrNull(s.dateExpiration));
    q.addBindValue(normalizedDanger);
    q.addBindValue(whereRef);

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
    q.prepare("DELETE FROM \"BioSample\" WHERE \"Reference_de_léchantillon\" = :ref");
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
        "SELECT \"Reference_de_léchantillon\", \"Type_déchantillon\", \"Organisme_source\", "
        "       \"Emplacement_de_stockage\", \"Température_de_stockage\", \"Quantité_restante\", "
        "       \"Date_de_collecte\", \"Date_dexpiration\", \"Niveau_de_dangerosité\", \"Id_projet\" "
        "FROM \"BioSample\" WHERE \"Reference_de_léchantillon\" = :ref"
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
        s.idProjet       = q.value(9).toInt();
    }

    return s;
}

// ──────────────────────────────────────────────
// LOAD ALL
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
            "SELECT \"Reference_de_léchantillon\", \"Type_déchantillon\", \"Organisme_source\", "
            "       \"Emplacement_de_stockage\", \"Température_de_stockage\", \"Quantité_restante\", "
            "       \"Date_de_collecte\", \"Date_dexpiration\", \"Niveau_de_dangerosité\" "
            "FROM \"BioSample\" ORDER BY \"Reference_de_léchantillon\""
            )) {
        logErr("[LOAD ERROR]", q);
        return;
    }

    auto mk = [](const QString& t, Qt::Alignment align = Qt::AlignLeft | Qt::AlignVCenter) {
        auto* it = new QTableWidgetItem(t);
        it->setTextAlignment(align);
        return it;
    };

    int row = 0;
    while (q.next()) {
        table->insertRow(row);

        QString ref     = q.value(0).toString();
        QString type    = q.value(1).toString();
        QString org     = q.value(2).toString();
        QString emp     = q.value(3).toString();
        QString qtyStr  = q.value(5).toString();
        // Format temperature as integer °C (Oracle NUMBER → avoid scientific notation)
        QString tempStr;
        if (!q.value(4).isNull()) {
            bool ok = false;
            double tv = q.value(4).toDouble(&ok);
            if (!ok) {
                // Oracle ODBC may return locale string with comma e.g. "-3,0E+001"
                QString s = q.value(4).toString().replace(',', '.');
                tv = s.toDouble(&ok);
            }
            if (ok) tempStr = QString::number(qRound(tv)) + "°C";
        }
        QDate   dc      = q.value(6).toDate();
        QDate   de      = q.value(7).toDate();
        QString danger  = q.value(8).toString();

        auto* refItem = mk(ref);
        refItem->setData(Qt::UserRole, ref);
        table->setItem(row, 0, refItem);

        table->setItem(row, 1, mk(emp));
        table->setItem(row, 2, mk(type));
        table->setItem(row, 3, mk(org));
        table->setItem(row, 4, mk(tempStr));
        table->setItem(row, 5, mk(qtyStr + " µg", Qt::AlignRight | Qt::AlignVCenter));
        table->setItem(row, 6, mk(dc.isValid() ? dc.toString("dd/MM/yyyy") : ""));
        table->setItem(row, 7, mk(de.isValid() ? de.toString("dd/MM/yyyy") : ""));

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

// Oracle NUMBER peut revenir en notation scientifique ("-2,0E+001").
// Cette fonction convertit proprement en "–20°C".
static QString fmtTemp(const QVariant& v)
{
    if (v.isNull()) return "Inconnu";
    bool ok = false;
    double t = v.toDouble(&ok);
    if (!ok) {
        QString s = v.toString().replace(',', '.');
        t = s.toDouble(&ok);
    }
    if (ok) return QString::number(qRound(t)) + "°C";
    return v.toString(); // fallback texte brut
}

// Macro interne : récupère la connexion active ou retourne valeur par défaut.
#define STATS_DB \
    QSqlDatabase db = QSqlDatabase::database(); \
    if (!db.isValid() || !db.isOpen()) { qDebug() << "[STATS] DB not open"; return {}; }

QMap<QString, int> CrudeBioSimple::countByType()
{
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isValid() || !db.isOpen()) return {};
    QMap<QString, int> result;
    QSqlQuery q(db);
    if (q.exec("SELECT \"Type_déchantillon\", COUNT(*) "
               "FROM \"BioSample\" GROUP BY \"Type_déchantillon\""))
        while (q.next())
            result[q.value(0).toString()] = q.value(1).toInt();
    else qDebug() << "[countByType]" << q.lastError().text();
    return result;
}

QVector<QPair<int, QString>> CrudeBioSimple::countByMonth()
{
    static const QStringList months =
        {"Jan","Fév","Mar","Avr","Mai","Juin","Juil","Août","Sep","Oct","Nov","Déc"};

    QVector<QPair<int, QString>> result;
    result.reserve(12);
    for (int i = 0; i < 12; ++i) result.append({0, months[i]});

    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isValid() || !db.isOpen()) return result;

    QSqlQuery q(db);
    if (q.exec(
        "SELECT EXTRACT(MONTH FROM \"Date_de_collecte\"), COUNT(*) "
        "FROM \"BioSample\" "
        "WHERE \"Date_de_collecte\" IS NOT NULL "
        "GROUP BY EXTRACT(MONTH FROM \"Date_de_collecte\") "
        "ORDER BY EXTRACT(MONTH FROM \"Date_de_collecte\")"))
    {
        while (q.next()) {
            int m = q.value(0).toInt();
            if (m >= 1 && m <= 12) result[m - 1].first = q.value(1).toInt();
        }
    } else qDebug() << "[countByMonth]" << q.lastError().text();
    return result;
}

int CrudeBioSimple::totalCount()
{
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isValid() || !db.isOpen()) return 0;
    QSqlQuery q(db);
    if (q.exec("SELECT COUNT(*) FROM \"BioSample\"") && q.next())
        return q.value(0).toInt();
    qDebug() << "[totalCount]" << q.lastError().text();
    return 0;
}

QMap<QString, int> CrudeBioSimple::countByTemperature()
{
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isValid() || !db.isOpen()) return {};
    QMap<QString, int> result;
    QSqlQuery q(db);
    if (q.exec("SELECT \"Température_de_stockage\", COUNT(*) "
               "FROM \"BioSample\" "
               "GROUP BY \"Température_de_stockage\" "
               "ORDER BY COUNT(*) DESC"))
        while (q.next())
            result[fmtTemp(q.value(0))] += q.value(1).toInt();
    else qDebug() << "[countByTemperature]" << q.lastError().text();
    return result;
}

QVector<QPair<QString, int>> CrudeBioSimple::topByQuantity(int n)
{
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isValid() || !db.isOpen()) return {};
    QVector<QPair<QString, int>> result;
    QSqlQuery q(db);
    q.prepare(
        "SELECT \"Reference_de_léchantillon\", \"Quantité_restante\" "
        "FROM (SELECT \"Reference_de_léchantillon\", \"Quantité_restante\" "
        "      FROM \"BioSample\" ORDER BY \"Quantité_restante\" DESC) "
        "WHERE ROWNUM <= :n");
    q.bindValue(":n", n);
    if (q.exec())
        while (q.next())
            result.append({q.value(0).toString(), q.value(1).toInt()});
    else qDebug() << "[topByQuantity]" << q.lastError().text();
    return result;
}

QString CrudeBioSimple::topTemperature()
{
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isValid() || !db.isOpen()) return "-";
    QSqlQuery q(db);
    if (q.exec("SELECT \"Température_de_stockage\" "
               "FROM (SELECT \"Température_de_stockage\", COUNT(*) AS cnt "
               "      FROM \"BioSample\" "
               "      GROUP BY \"Température_de_stockage\" ORDER BY cnt DESC) "
               "WHERE ROWNUM = 1") && q.next())
        return fmtTemp(q.value(0));
    qDebug() << "[topTemperature]" << q.lastError().text();
    return "-";
}

QString CrudeBioSimple::topReference()
{
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isValid() || !db.isOpen()) return "-";
    QSqlQuery q(db);
    if (q.exec("SELECT \"Reference_de_léchantillon\" "
               "FROM (SELECT \"Reference_de_léchantillon\", \"Quantité_restante\" "
               "      FROM \"BioSample\" ORDER BY \"Quantité_restante\" DESC) "
               "WHERE ROWNUM = 1") && q.next())
        return q.value(0).toString();
    qDebug() << "[topReference]" << q.lastError().text();
    return "-";
}

int CrudeBioSimple::countThisMonth()
{
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isValid() || !db.isOpen()) return 0;
    QSqlQuery q(db);
    if (q.exec("SELECT COUNT(*) FROM \"BioSample\" "
               "WHERE EXTRACT(MONTH FROM \"Date_de_collecte\") = EXTRACT(MONTH FROM SYSDATE) "
               "  AND EXTRACT(YEAR  FROM \"Date_de_collecte\") = EXTRACT(YEAR  FROM SYSDATE)")
        && q.next())
        return q.value(0).toInt();
    qDebug() << "[countThisMonth]" << q.lastError().text();
    return 0;
}

int CrudeBioSimple::countDistinctTypes()
{
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isValid() || !db.isOpen()) return 0;
    QSqlQuery q(db);
    if (q.exec("SELECT COUNT(DISTINCT \"Type_déchantillon\") FROM \"BioSample\"") && q.next())
        return q.value(0).toInt();
    qDebug() << "[countDistinctTypes]" << q.lastError().text();
    return 0;
}

#undef STATS_DB
