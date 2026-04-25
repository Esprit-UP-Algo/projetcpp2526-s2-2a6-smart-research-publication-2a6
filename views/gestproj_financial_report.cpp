// ─────────────────────────────────────────────────────────────────────────────
//  gestproj_financial_report.cpp
//  RAPPORT FINANCIER TRIMESTRIEL — SmartVision (Excel .xlsx export)
//
//  ZERO external dependencies — no Python, no openpyxl, no Qt-private APIs.
//  Uses only Qt Core / Qt Sql / Qt Widgets / Qt Network (already in project)
//  + a self-contained CRC32 / Deflate-less ZIP writer that produces a valid
//  .xlsx (Open XML) file understood by Excel, LibreOffice, WPS, etc.
//
//  Only change in integration.cpp: replace the entire lambda body of the
//  btnRapportFinancier clicked-connect with a single call:
//       GestProjCrud::generateFinancialReport(0, 0, this);
// ─────────────────────────────────────────────────────────────────────────────

#include "gestproj.h"

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QDate>
#include <QDateTime>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QTextStream>
#include <QDesktopServices>
#include <QUrl>
#include <QFrame>
#include <QFileInfo>
#include <QProgressDialog>
#include <QCoreApplication>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSslConfiguration>
#include <QSslSocket>
#include <cmath>
#include <cstring>

// ─── API constants (re-used from rest of project) ────────────────────────────
#ifndef GROQ_API_KEY
#  define GROQ_API_KEY   ""
#endif
#ifndef GROQ_API_URL
#  define GROQ_API_URL   "https://api.groq.com/openai/v1/chat/completions"
#endif
#ifndef GROQ_API_MODEL
#  define GROQ_API_MODEL "llama3-8b-8192"
#endif

// ═════════════════════════════════════════════════════════════════════════════
//  SECTION A — DATA COLLECTION  (unchanged)
// ═════════════════════════════════════════════════════════════════════════════

struct ProjFinData {
    QString nom;
    QString domaine;
    QString statut;
    QString sourceFinancement;
    QString ethique;
    double  budgetAlloue     = 0.0;
    double  budgetConsomme   = 0.0;
    double  budgetRestant    = 0.0;
    double  pctConsomme      = 0.0;
    int     nbEmployes       = 0;
    int     nbExperiences    = 0;
    int     nbBioSamples     = 0;
    int     nbPublications   = 0;
    double  coutEmployes     = 0.0;
    double  coutExperiences  = 0.0;
    double  coutBioSamples   = 0.0;
    double  coutPublications = 0.0;
    double  coutSousTotal    = 0.0;
    double  coutOverhead     = 0.0;
    double  coutTotal        = 0.0;
};

static QList<ProjFinData> loadAllProjFinData()
{
    const QString expTable = QString::fromUtf8("Exp\xc3\xa9rience");

    GestProjCrud crud;
    QList<ProjetRecord> recs;
    crud.loadProjets(recs);

    QList<ProjFinData> result;
    for (const ProjetRecord& p : recs) {
        ProjFinData d;
        d.nom               = p.nomDuProjet;
        d.domaine           = p.domaineDeRecherche;
        d.statut            = p.statut;
        d.sourceFinancement = p.sourceDeFinancement;
        d.ethique           = p.numeroDApprobationEthique;
        d.budgetAlloue      = p.budget;
        d.nbPublications    = p.nombreDePublications;

        { QSqlQuery q;
            q.prepare("SELECT COUNT(*) FROM \"Associer\" WHERE \"Id_projet\"=:id");
            q.bindValue(":id", p.idProjet);
            if (q.exec() && q.next()) d.nbEmployes = q.value(0).toInt(); }

        { QSqlQuery q;
            q.prepare(QString("SELECT COUNT(*) FROM \"%1\" WHERE \"Id_projet\"=:id").arg(expTable));
            q.bindValue(":id", p.idProjet);
            if (q.exec() && q.next()) d.nbExperiences = q.value(0).toInt(); }

        { QSqlQuery q;
            q.prepare("SELECT COUNT(*) FROM \"BioSample\" WHERE \"Id_projet\"=:id");
            q.bindValue(":id", p.idProjet);
            if (q.exec() && q.next()) d.nbBioSamples = q.value(0).toInt(); }

        d.coutEmployes     = d.nbEmployes     * 2500.0;
        d.coutExperiences  = d.nbExperiences  * 500.0;
        d.coutBioSamples   = d.nbBioSamples   * 150.0;
        d.coutPublications = d.nbPublications * 200.0;
        d.coutSousTotal    = d.coutEmployes + d.coutExperiences
                          + d.coutBioSamples + d.coutPublications;
        d.coutOverhead     = d.coutSousTotal * 0.15;
        d.coutTotal        = d.coutSousTotal + d.coutOverhead;
        d.budgetConsomme   = d.coutTotal;
        d.budgetRestant    = d.budgetAlloue - d.budgetConsomme;
        d.pctConsomme      = (d.budgetAlloue > 0)
                            ? (d.budgetConsomme / d.budgetAlloue * 100.0) : 0.0;
        result.append(d);
    }
    return result;
}

// ═════════════════════════════════════════════════════════════════════════════
//  SECTION B — AI ANALYSIS (GROQ)  (unchanged)
// ═════════════════════════════════════════════════════════════════════════════

struct ObsEntry { QString titre; QString detail; };

static QList<ObsEntry> callAIForObservations(const QList<ProjFinData>& data,
                                             int quarter, int year,
                                             QWidget* parent)
{
    int nActifs = 0, nTermines = 0, nAlerts = 0;
    double totalBudget = 0, totalConsomme = 0;
    for (const auto& d : data) {
        totalBudget   += d.budgetAlloue;
        totalConsomme += d.budgetConsomme;
        QString s = d.statut.trimmed().toLower();
        if (s == "en cours" || s.contains("planifi")) ++nActifs;
        if (s.contains("termin"))                      ++nTermines;
        if (d.pctConsomme > 100.0)                     ++nAlerts;
    }

    QString prompt = QString(
                         "Tu es un analyste financier pour SmartVision, laboratoire de recherche biologique. "
                         "Voici les données financières pour Q%1 %2:\n"
                         "- Projets totaux: %3\n- Projets actifs: %4\n- Projets terminés: %5\n"
                         "- Budget total alloué: %6 TND\n- Budget total consommé estimé: %7 TND\n"
                         "- Projets en dépassement budgétaire: %8\n\n"
                         "Génère exactement 5 observations analytiques au format STRICT:\nTITRE|DETAIL\n"
                         "Une observation par ligne. Chaque détail: 1-2 phrases courtes, précises, professionnelles. "
                         "Couvre: santé budgétaire, risques, recommandations, points positifs, tendances. "
                         "Réponds UNIQUEMENT avec les 5 lignes au format TITRE|DETAIL, rien d'autre."
                         ).arg(quarter).arg(year)
                         .arg(data.size()).arg(nActifs).arg(nTermines)
                         .arg(totalBudget, 0, 'f', 0).arg(totalConsomme, 0, 'f', 0).arg(nAlerts);

    QNetworkAccessManager* net = new QNetworkAccessManager(parent);
    QJsonObject body;
    body["model"]       = QString(GROQ_API_MODEL);
    body["max_tokens"]  = 400;
    body["temperature"] = 0.3;
    body["messages"] = QJsonArray{
        QJsonObject{{"role","system"},
                    {"content","Tu es un analyste financier expert en recherche scientifique. "
                                "Réponds UNIQUEMENT au format demandé: TITRE|DETAIL"}},
        QJsonObject{{"role","user"},{"content",prompt}}
    };

    QUrl url(QString(GROQ_API_URL));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", ("Bearer " + QString(GROQ_API_KEY)).toUtf8());
    QSslConfiguration ssl = QSslConfiguration::defaultConfiguration();
    ssl.setPeerVerifyMode(QSslSocket::VerifyNone);
    req.setSslConfiguration(ssl);

    QNetworkReply* reply = net->post(req, QJsonDocument(body).toJson());
    reply->ignoreSslErrors();
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QList<ObsEntry> obs;
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray raw = reply->readAll();
        QJsonObject root = QJsonDocument::fromJson(raw).object();
        QString text = root["choices"].toArray().first()
                           .toObject()["message"].toObject()["content"]
                           .toString().trimmed();
        for (const QString& line : text.split('\n', Qt::SkipEmptyParts)) {
            QStringList parts = line.split('|');
            if (parts.size() >= 2) {
                obs.append({ parts[0].trimmed(), parts[1].trimmed() });
                if (obs.size() == 5) break;
            }
        }
    }
    reply->deleteLater();
    net->deleteLater();

    while (obs.size() < 5) {
        int i = obs.size() + 1;
        obs.append({ QString("Observation %1").arg(i),
                    QString::fromUtf8("Donn\xc3\xa9" "es insuffisantes pour g\xc3\xa9n\xc3\xa9rer "
                                      "une analyse IA. V\xc3\xa9rifiez la connexion internet.") });
    }
    return obs;
}

// ═════════════════════════════════════════════════════════════════════════════
//  SECTION C — NATIVE XLSX GENERATION (pure Qt/C++, no Python)
// ═════════════════════════════════════════════════════════════════════════════
//
//  An .xlsx file is a ZIP archive containing XML files.
//  We build each XML part as a QByteArray, then write a "stored" (no
//  compression) ZIP using only Qt's QFile + manual ZIP local-file-header
//  construction.  The result is a fully valid .xlsx.
//
//  ZIP spec used: PKWARE APPNOTE 6.3 — stored (method 0), no data descriptors.
// ─────────────────────────────────────────────────────────────────────────────

// ── CRC-32 (needed by ZIP) ────────────────────────────────────────────────────
static const quint32 CRC32_TABLE[256] = {
    0x00000000,0x77073096,0xEE0E612C,0x990951BA,0x076DC419,0x706AF48F,0xE963A535,
    0x9E6495A3,0x0EDB8832,0x79DCB8A4,0xE0D5E91B,0x97D2D988,0x09B64C2B,0x7EB17CBF,
    0xE7B82D09,0x90BF1CBF,0x1DB71064,0x6AB020F2,0xF3B97148,0x84BE41DE,0x1ADAD47D,
    0x6DDDE4EB,0xF4D4B551,0x83D385C7,0x136C9856,0x646BA8C0,0xFD62F97A,0x8A65C9EC,
    0x14015C4F,0x63066CD9,0xFA0F3D63,0x8D080DF5,0x3B6E20C8,0x4C69105E,0xD56041E4,
    0xA2677172,0x3C03E4D1,0x4B04D447,0xD20D85FD,0xA50AB56B,0x35B5A8FA,0x42B2986C,
    0xDBBBC9D6,0xACBCF940,0x32D86CE3,0x45DF5C75,0xDCD60DCF,0xABD13D59,0x26D930AC,
    0x51DE003A,0xC8D75180,0xBFD06116,0x21B4F927,0x56B3C9B1,0xCFBA9C09,0xB8BDA50F,
    0x2802B89E,0x5F058808,0xC60CD9B2,0xB10BE924,0x2F6F7C87,0x58684C11,0xC1611DAB,
    0xB6662D3D,0x76DC4190,0x01DB7106,0x98D220BC,0xEFD5102A,0x71B18589,0x06B6B51F,
    0x9FBFE4A5,0xE8B8D433,0x7807C9A2,0x0F00F934,0x9609A88E,0xE10E9818,0x7F6F2B63,
    0x086D3D2D,0x91646C97,0xE6635C01,0x6B6B51F4,0x1C6C6162,0x856530D8,0xF262004E,
    0x6C0695ED,0x1B01A57B,0x8208F4C1,0xF50FC457,0x65B0D9C6,0x12B7E950,0x8BBEB8EA,
    0xFCB9887C,0x62DD1D7F,0x15DA2D49,0x8CD37CF3,0xFBD44C65,0x4DB26158,0x3AB551CE,
    0xA3BC0074,0xD4BB30E2,0x4ADFA541,0x3DD895D7,0xA4D1C46D,0xD3D6F4FB,0x4369E96A,
    0x346ED9FC,0xAD678846,0xDA60B8D0,0x44042D73,0x33031DE5,0xAA0A4C5F,0xDD0D7CC9,
    0x5005713C,0x270241AA,0xBE0B1010,0xC90C2086,0x5768B525,0x206F85B3,0xB966D409,
    0xCE61E49F,0x5EDEF90E,0x29D9C998,0xB0D09822,0xC7D7A8B4,0x59B33D17,0x2EB40D81,
    0xB7BD5C3B,0xC0BA6CAD,0xEDB88320,0x9ABFB3B6,0x03B6E20C,0x74B1D29A,0xEAD54739,
    0x9DD277AF,0x04DB2615,0x73DC1683,0xE3630B12,0x94643B84,0x0D6D6A3E,0x7A6A5AA8,
    0xE40ECF0B,0x9309FF9D,0x0A00AE27,0x7D079EB1,0xF00F9344,0x8708A3D2,0x1E01F268,
    0x6906C2FE,0xF762575D,0x806567CB,0x196C3671,0x6E6B06E7,0xFED41B76,0x89D32BE0,
    0x10DA7A5A,0x67DD4ACC,0xF9B9DF6F,0x8EBEEFF9,0x17B7BE43,0x60B08ED5,0xD6D6A3E8,
    0xA1D1937E,0x38D8C2C4,0x4FDFF252,0xD1BB67F1,0xA6BC5767,0x3FB506DD,0x48B2364B,
    0xD80D2BDA,0xAF0A1B4C,0x36034AF6,0x41047A60,0xDF60EFC3,0xA8670955,0x316658EF,
    0x466882A9,0xB40BBE37,0xC30C8EA1,0x5A05DF1B,0x2D02EF8D
};

static quint32 crc32Calc(const QByteArray& data) {
    quint32 crc = 0xFFFFFFFF;
    for (unsigned char c : data)
        crc = CRC32_TABLE[(crc ^ c) & 0xFF] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFF;
}

// ── Minimal ZIP writer ────────────────────────────────────────────────────────
struct ZipEntry {
    QString     name;
    QByteArray  data;
    quint32     crc   = 0;
    quint32     localHeaderOffset = 0;
};

static void le16(QByteArray& b, quint16 v) {
    b.append((char)(v & 0xFF));
    b.append((char)((v >> 8) & 0xFF));
}
static void le32(QByteArray& b, quint32 v) {
    b.append((char)(v & 0xFF));
    b.append((char)((v >> 8) & 0xFF));
    b.append((char)((v >> 16) & 0xFF));
    b.append((char)((v >> 24) & 0xFF));
}

static bool writeZip(const QString& path, const QList<ZipEntry>& entries)
{
    QByteArray archive;

    // Local file headers + data
    QList<ZipEntry> ents = entries;
    for (ZipEntry& e : ents) {
        e.crc = crc32Calc(e.data);
        e.localHeaderOffset = (quint32)archive.size();

        QByteArray nameBytes = e.name.toUtf8();
        // Local file header signature
        archive.append("\x50\x4B\x03\x04", 4);
        le16(archive, 20);      // version needed: 2.0
        le16(archive, 0x0800);  // flags: UTF-8
        le16(archive, 0);       // compression: stored
        le16(archive, 0);       // mod time
        le16(archive, 0);       // mod date
        le32(archive, e.crc);
        le32(archive, (quint32)e.data.size()); // compressed size
        le32(archive, (quint32)e.data.size()); // uncompressed size
        le16(archive, (quint16)nameBytes.size());
        le16(archive, 0);       // extra field length
        archive.append(nameBytes);
        archive.append(e.data);
    }

    // Central directory
    quint32 cdOffset = (quint32)archive.size();
    for (const ZipEntry& e : ents) {
        QByteArray nameBytes = e.name.toUtf8();
        archive.append("\x50\x4B\x01\x02", 4);
        le16(archive, 20);      // version made by
        le16(archive, 20);      // version needed
        le16(archive, 0x0800);  // flags: UTF-8
        le16(archive, 0);       // method: stored
        le16(archive, 0);       // mod time
        le16(archive, 0);       // mod date
        le32(archive, e.crc);
        le32(archive, (quint32)e.data.size());
        le32(archive, (quint32)e.data.size());
        le16(archive, (quint16)nameBytes.size());
        le16(archive, 0);       // extra
        le16(archive, 0);       // comment
        le16(archive, 0);       // disk number start
        le16(archive, 0);       // internal attr
        le32(archive, 0);       // external attr
        le32(archive, e.localHeaderOffset);
        archive.append(nameBytes);
    }
    quint32 cdSize = (quint32)archive.size() - cdOffset;

    // End of central directory record
    archive.append("\x50\x4B\x05\x06", 4);
    le16(archive, 0);  // disk number
    le16(archive, 0);  // disk with cd
    le16(archive, (quint16)ents.size());
    le16(archive, (quint16)ents.size());
    le32(archive, cdSize);
    le32(archive, cdOffset);
    le16(archive, 0);  // comment length

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(archive);
    f.close();
    return true;
}

// ── XML helpers ───────────────────────────────────────────────────────────────
static QString xmlEsc(const QString& s)
{
    QString r = s;
    r.replace("&","&amp;");
    r.replace("<","&lt;");
    r.replace(">","&gt;");
    r.replace("\"","&quot;");
    r.replace("'","&apos;");
    return r;
}

static QString fmtNum(double v) { return QString::number(v, 'f', 2); }

// ── Shared strings table ──────────────────────────────────────────────────────
// We collect every string in insertion order and reference by index.
// This keeps xl/sharedStrings.xml consistent with cell v elements.
struct SST {
    QList<QString> strings;
    QMap<QString,int> index;
    int add(const QString& s) {
        auto it = index.find(s);
        if (it != index.end()) return it.value();
        int i = strings.size();
        strings.append(s);
        index.insert(s, i);
        return i;
    }
};

// ── Style indices (fixed mapping defined in styles.xml below) ─────────────────
// We define a fixed set of cell styles in the styles.xml.
// Index:
//   0 = normal text (left, no fill, 10pt, Calibri)
//   1 = header dark navy (bold, white text, dark navy fill, 10pt, center)
//   2 = header teal (bold, white text, teal fill, 10pt, center)
//   3 = header grey (bold, white text, grey fill, 10pt, center)
//   4 = header red (bold, white text, red fill, 10pt, center)
//   5 = data alt row (normal, teal-light fill)
//   6 = data plain row (normal, no fill)
//   7 = data total row (bold, blue-light fill)
//   8 = data warn row (normal, warn-red-bg fill)
//   9 = title (bold, white text, dark-navy fill, 16pt, left)
//  10 = sub-title (italic, grey text, dark-navy fill, 9pt, left)
//  11 = meta key (bold, white text, dark-navy fill, 9pt, center)
//  12 = meta value (normal, light-bg fill, 9pt, center)
//  13 = kpi label (bold, white text, mid-blue fill, 9pt, center, wrap)
//  14 = kpi value-number (bold, white text, mid-blue fill, 13pt, center)
//  15 = kpi value-number teal (bold, white text, teal fill, 13pt, center)
//  16 = section divider (bold, white text, section-blue fill, 10pt, left)
//  17 = data alt row — number TND format  (fmt id 164)
//  18 = data plain row — number TND format
//  19 = data total row — number TND format
//  20 = data alt row — number PCT format  (fmt id 165)
//  21 = data plain row — number PCT format
//  22 = data total row — number PCT format
//  23 = data alt row — number INT format  (fmt id 166)
//  24 = data plain row — number INT format
//  25 = data total row — number INT format
//  26 = warn bg — number TND
//  27 = alert yellow bg — text
//  28 = alert yellow bg — number TND
//  29 = alert blue-light bg — text
//  30 = footer italic (italic, grey, 8pt, center)
//  31 = obs alt row — text left (wrap)
//  32 = obs plain row — text left (wrap)
//  33 = kpi value-number dark1 (bold, white, 13pt, center) — 4 extra KPI cols
//  34 = kpi label dark1 (bold, white, mid-blue2 fill, 9pt, center, wrap)
//  35 = kpi value-number purple (bold, white, purple fill, 13pt, center)
//  36 = kpi label purple (bold, white, purple fill, 9pt, center, wrap)
//  37 = data alt row — number, no format (plain number)
//  38 = data plain row — number, no format

// ── Build styles.xml ──────────────────────────────────────────────────────────
static QByteArray buildStyles()
{
    // We define the full OOXML styles document with:
    //   - custom number formats (164=TND, 165=PCT, 166=INT)
    //   - fonts (10 entries)
    //   - fills (colours used)
    //   - borders (thin grey border for all data cells)
    //   - cellXfs (the indexed styles 0..38)
    const char* xml = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<styleSheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main">
  <numFmts count="3">
    <numFmt numFmtId="164" formatCode="#,##0.00 &quot;TND&quot;;(#,##0.00 &quot;TND&quot;);&quot;-&quot;"/>
    <numFmt numFmtId="165" formatCode="0.0%"/>
    <numFmt numFmtId="166" formatCode="#,##0;&quot;-&quot;"/>
  </numFmts>
  <fonts count="12">
    <font><sz val="10"/><name val="Calibri"/></font>
    <font><b/><sz val="10"/><color rgb="FFFFFFFF"/><name val="Calibri"/></font>
    <font><i/><sz val="9"/><color rgb="FFAAAAAA"/><name val="Calibri"/></font>
    <font><b/><sz val="16"/><color rgb="FFFFFFFF"/><name val="Calibri"/></font>
    <font><b/><sz val="9"/><color rgb="FFFFFFFF"/><name val="Calibri"/></font>
    <font><sz val="9"/><name val="Calibri"/></font>
    <font><b/><sz val="13"/><color rgb="FFFFFFFF"/><name val="Calibri"/></font>
    <font><b/><sz val="10"/><name val="Calibri"/></font>
    <font><i/><sz val="8"/><color rgb="FF888888"/><name val="Calibri"/></font>
    <font><sz val="9"/><color rgb="FFFFFFFF"/><name val="Calibri"/></font>
    <font><b/><sz val="9"/><name val="Calibri"/></font>
    <font><b/><sz val="14"/><color rgb="FFFFFFFF"/><name val="Calibri"/></font>
  </fonts>
  <fills count="20">
    <fill><patternFill patternType="none"/></fill>
    <fill><patternFill patternType="gray125"/></fill>
    <fill><patternFill patternType="solid"><fgColor rgb="FF162534"/></patternFill></fill>
    <fill><patternFill patternType="solid"><fgColor rgb="FF1F3A55"/></patternFill></fill>
    <fill><patternFill patternType="solid"><fgColor rgb="FF0A5F58"/></patternFill></fill>
    <fill><patternFill patternType="solid"><fgColor rgb="FF4A4A4A"/></patternFill></fill>
    <fill><patternFill patternType="solid"><fgColor rgb="FFC0392B"/></patternFill></fill>
    <fill><patternFill patternType="solid"><fgColor rgb="FFE8F4F2"/></patternFill></fill>
    <fill><patternFill patternType="solid"><fgColor rgb="FFF2F2F2"/></patternFill></fill>
    <fill><patternFill patternType="solid"><fgColor rgb="FFD9E8F5"/></patternFill></fill>
    <fill><patternFill patternType="solid"><fgColor rgb="FFFDECEA"/></patternFill></fill>
    <fill><patternFill patternType="solid"><fgColor rgb="FFFFF3CD"/></patternFill></fill>
    <fill><patternFill patternType="solid"><fgColor rgb="FFD6EAF8"/></patternFill></fill>
    <fill><patternFill patternType="solid"><fgColor rgb="FF2E6DA4"/></patternFill></fill>
    <fill><patternFill patternType="solid"><fgColor rgb="FF1A6B82"/></patternFill></fill>
    <fill><patternFill patternType="solid"><fgColor rgb="FF1E5F74"/></patternFill></fill>
    <fill><patternFill patternType="solid"><fgColor rgb="FF4A235A"/></patternFill></fill>
    <fill><patternFill patternType="solid"><fgColor rgb="FF2E4057"/></patternFill></fill>
    <fill><patternFill patternType="solid"><fgColor rgb="FFD5F5E3"/></patternFill></fill>
    <fill><patternFill patternType="solid"><fgColor rgb="FFFFF9C4"/></patternFill></fill>
  </fills>
  <borders count="2">
    <border><left/><right/><top/><bottom/><diagonal/></border>
    <border>
      <left style="thin"><color rgb="FFBFBFBF"/></left>
      <right style="thin"><color rgb="FFBFBFBF"/></right>
      <top style="thin"><color rgb="FFBFBFBF"/></top>
      <bottom style="thin"><color rgb="FFBFBFBF"/></bottom>
      <diagonal/>
    </border>
  </borders>
  <cellStyleXfs count="1"><xf numFmtId="0" fontId="0" fillId="0" borderId="0"/></cellStyleXfs>
  <cellXfs count="39">
    <!-- 0: normal -->
    <xf numFmtId="0" fontId="0" fillId="0" borderId="1" xfId="0"><alignment horizontal="left" vertical="center"/></xf>
    <!-- 1: header dark navy bold white -->
    <xf numFmtId="0" fontId="1" fillId="2" borderId="1" xfId="0"><alignment horizontal="center" vertical="center" wrapText="1"/></xf>
    <!-- 2: header teal bold white -->
    <xf numFmtId="0" fontId="1" fillId="4" borderId="1" xfId="0"><alignment horizontal="center" vertical="center" wrapText="1"/></xf>
    <!-- 3: header grey bold white -->
    <xf numFmtId="0" fontId="1" fillId="5" borderId="1" xfId="0"><alignment horizontal="center" vertical="center" wrapText="1"/></xf>
    <!-- 4: header red bold white -->
    <xf numFmtId="0" fontId="1" fillId="6" borderId="1" xfId="0"><alignment horizontal="center" vertical="center" wrapText="1"/></xf>
    <!-- 5: data alt row text -->
    <xf numFmtId="0" fontId="5" fillId="7" borderId="1" xfId="0"><alignment horizontal="left" vertical="center" wrapText="1"/></xf>
    <!-- 6: data plain row text -->
    <xf numFmtId="0" fontId="5" fillId="8" borderId="1" xfId="0"><alignment horizontal="left" vertical="center" wrapText="1"/></xf>
    <!-- 7: total row bold -->
    <xf numFmtId="0" fontId="7" fillId="9" borderId="1" xfId="0"><alignment horizontal="center" vertical="center"/></xf>
    <!-- 8: warn row text -->
    <xf numFmtId="0" fontId="5" fillId="10" borderId="1" xfId="0"><alignment horizontal="center" vertical="center" wrapText="1"/></xf>
    <!-- 9: title 16pt bold white navy -->
    <xf numFmtId="0" fontId="3" fillId="2" borderId="1" xfId="0"><alignment horizontal="left" vertical="center"/></xf>
    <!-- 10: subtitle italic grey navy -->
    <xf numFmtId="0" fontId="2" fillId="2" borderId="1" xfId="0"><alignment horizontal="left" vertical="center"/></xf>
    <!-- 11: meta key bold white navy center -->
    <xf numFmtId="0" fontId="4" fillId="2" borderId="1" xfId="0"><alignment horizontal="center" vertical="center"/></xf>
    <!-- 12: meta value 9pt light-bg center -->
    <xf numFmtId="0" fontId="5" fillId="8" borderId="1" xfId="0"><alignment horizontal="center" vertical="center"/></xf>
    <!-- 13: kpi label bold white mid-blue wrap -->
    <xf numFmtId="0" fontId="4" fillId="3" borderId="1" xfId="0"><alignment horizontal="center" vertical="center" wrapText="1"/></xf>
    <!-- 14: kpi value bold white mid-blue 13pt -->
    <xf numFmtId="0" fontId="6" fillId="3" borderId="1" xfId="0"><alignment horizontal="center" vertical="center"/></xf>
    <!-- 15: kpi value teal 13pt -->
    <xf numFmtId="0" fontId="6" fillId="4" borderId="1" xfId="0"><alignment horizontal="center" vertical="center"/></xf>
    <!-- 16: section divider bold white section-blue left -->
    <xf numFmtId="0" fontId="1" fillId="13" borderId="1" xfId="0"><alignment horizontal="left" vertical="center"/></xf>
    <!-- 17: alt row TND -->
    <xf numFmtId="164" fontId="5" fillId="7" borderId="1" xfId="0"><alignment horizontal="center" vertical="center"/></xf>
    <!-- 18: plain row TND -->
    <xf numFmtId="164" fontId="5" fillId="8" borderId="1" xfId="0"><alignment horizontal="center" vertical="center"/></xf>
    <!-- 19: total row TND -->
    <xf numFmtId="164" fontId="7" fillId="9" borderId="1" xfId="0"><alignment horizontal="center" vertical="center"/></xf>
    <!-- 20: alt row PCT -->
    <xf numFmtId="165" fontId="5" fillId="7" borderId="1" xfId="0"><alignment horizontal="center" vertical="center"/></xf>
    <!-- 21: plain row PCT -->
    <xf numFmtId="165" fontId="5" fillId="8" borderId="1" xfId="0"><alignment horizontal="center" vertical="center"/></xf>
    <!-- 22: total row PCT -->
    <xf numFmtId="165" fontId="7" fillId="9" borderId="1" xfId="0"><alignment horizontal="center" vertical="center"/></xf>
    <!-- 23: alt row INT -->
    <xf numFmtId="166" fontId="5" fillId="7" borderId="1" xfId="0"><alignment horizontal="center" vertical="center"/></xf>
    <!-- 24: plain row INT -->
    <xf numFmtId="166" fontId="5" fillId="8" borderId="1" xfId="0"><alignment horizontal="center" vertical="center"/></xf>
    <!-- 25: total row INT -->
    <xf numFmtId="166" fontId="7" fillId="9" borderId="1" xfId="0"><alignment horizontal="center" vertical="center"/></xf>
    <!-- 26: warn bg TND -->
    <xf numFmtId="164" fontId="5" fillId="10" borderId="1" xfId="0"><alignment horizontal="center" vertical="center"/></xf>
    <!-- 27: alert yellow text -->
    <xf numFmtId="0" fontId="5" fillId="11" borderId="1" xfId="0"><alignment horizontal="center" vertical="center" wrapText="1"/></xf>
    <!-- 28: alert yellow TND -->
    <xf numFmtId="164" fontId="5" fillId="11" borderId="1" xfId="0"><alignment horizontal="center" vertical="center"/></xf>
    <!-- 29: alert blue-light text -->
    <xf numFmtId="0" fontId="5" fillId="12" borderId="1" xfId="0"><alignment horizontal="center" vertical="center" wrapText="1"/></xf>
    <!-- 30: footer italic grey center -->
    <xf numFmtId="0" fontId="8" fillId="0" borderId="0" xfId="0"><alignment horizontal="center" vertical="center"/></xf>
    <!-- 31: obs alt row wrap -->
    <xf numFmtId="0" fontId="5" fillId="7" borderId="1" xfId="0"><alignment horizontal="left" vertical="center" wrapText="1"/></xf>
    <!-- 32: obs plain row wrap -->
    <xf numFmtId="0" fontId="5" fillId="8" borderId="1" xfId="0"><alignment horizontal="left" vertical="center" wrapText="1"/></xf>
    <!-- 33: kpi value dark navy 13pt -->
    <xf numFmtId="0" fontId="6" fillId="2" borderId="1" xfId="0"><alignment horizontal="center" vertical="center"/></xf>
    <!-- 34: kpi label teal-dark wrap -->
    <xf numFmtId="0" fontId="4" fillId="14" borderId="1" xfId="0"><alignment horizontal="center" vertical="center" wrapText="1"/></xf>
    <!-- 35: kpi value purple 13pt -->
    <xf numFmtId="0" fontId="6" fillId="16" borderId="1" xfId="0"><alignment horizontal="center" vertical="center"/></xf>
    <!-- 36: kpi label purple wrap -->
    <xf numFmtId="0" fontId="4" fillId="16" borderId="1" xfId="0"><alignment horizontal="center" vertical="center" wrapText="1"/></xf>
    <!-- 37: alt row plain number -->
    <xf numFmtId="0" fontId="5" fillId="7" borderId="1" xfId="0"><alignment horizontal="center" vertical="center"/></xf>
    <!-- 38: plain row plain number -->
    <xf numFmtId="0" fontId="5" fillId="8" borderId="1" xfId="0"><alignment horizontal="center" vertical="center"/></xf>
  </cellXfs>
</styleSheet>)";
    return QByteArray(xml);
}

// ── Cell builder helpers ──────────────────────────────────────────────────────
// A "cell" in OOXML is:  <c r="A1" s="styleIdx" [t="s"|"n"]><v>...</v></c>
// We accumulate rows into a QString (sheet XML is large, QString is fine).

struct XlCell {
    QString ref;   // e.g. "B2"
    int     style = 0;
    enum Type { STRING, NUMBER } type = STRING;
    int     sIdx  = 0;    // shared string index (type==STRING)
    double  num   = 0.0;  // (type==NUMBER)
};

static QString colLetter(int col) {
    // col is 1-based
    QString r;
    while (col > 0) {
        int rem = (col - 1) % 26;
        r.prepend(QChar('A' + rem));
        col = (col - 1) / 26;
    }
    return r;
}

static QString cellRef(int row, int col) {
    return colLetter(col) + QString::number(row);
}

// ── Worksheet builder ─────────────────────────────────────────────────────────
// We produce the sheet XML incrementally.  Each "row" is a list of cells.
// We write sheetData row by row; merges and colWidths are collected separately.

struct SheetBuilder {
    QString xmlHeader; // XML declaration + worksheet open + sheetViews (set by begin())
    QString xml;       // accumulated sheetData row XML only (rows appended here)
    QStringList merges;
    SST* sst;
    QList<QPair<int,double>> colWidths; // col (1-based) -> width

    SheetBuilder(SST* s) : sst(s) {}

    // Start the XML (call before first addRow)
    void begin(const QString& sheetName, bool freezeRow = 0) {
        Q_UNUSED(sheetName);
        xml.clear();
        xmlHeader  = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
        xmlHeader += "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" "
                     "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">\n";
        // sheetViews
        xmlHeader += "<sheetViews><sheetView workbookViewId=\"0\" showGridLines=\"0\"";
        if (freezeRow > 0)
            xmlHeader += QString(" topLeftCell=\"B%1\"").arg(freezeRow + 1);
        xmlHeader += ">";
        if (freezeRow > 0) {
            xmlHeader += QString("<pane ySplit=\"%1\" topLeftCell=\"A%2\" activePane=\"bottomLeft\" state=\"frozen\"/>")
            .arg(freezeRow).arg(freezeRow + 1);
        }
        xmlHeader += "</sheetView></sheetViews>\n";
        // column widths placeholder — filled in finish()
    }

    void addMerge(int r1, int c1, int r2, int c2) {
        merges.append(QString("%1:%2").arg(cellRef(r1,c1)).arg(cellRef(r2,c2)));
    }

    // Add a single merged cell (string)
    void strCell(int row, int col, const QString& text, int style) {
        int si = sst->add(text);
        xml += QString("<row r=\"%1\"><c r=\"%2\" s=\"%3\" t=\"s\"><v>%4</v></c></row>\n")
                   .arg(row).arg(cellRef(row,col)).arg(style).arg(si);
    }
    // For full rows (multiple cells):
    void beginRow(int row) {
        xml += QString("<row r=\"%1\">").arg(row);
    }
    void endRow() { xml += "</row>\n"; }

    void addStrCell(int row, int col, const QString& text, int style) {
        int si = sst->add(text);
        xml += QString("<c r=\"%1\" s=\"%2\" t=\"s\"><v>%3</v></c>")
                   .arg(cellRef(row,col)).arg(style).arg(si);
    }
    void addNumCell(int row, int col, double val, int style) {
        xml += QString("<c r=\"%1\" s=\"%2\"><v>%3</v></c>")
        .arg(cellRef(row,col)).arg(style).arg(fmtNum(val));
    }
    void addEmptyCell(int row, int col, int style) {
        xml += QString("<c r=\"%1\" s=\"%2\"/>").arg(cellRef(row,col)).arg(style);
    }

    QByteArray finish() {
        // Column widths
        QString cols = "<cols>";
        for (const auto& [c, w] : colWidths) {
            cols += QString("<col min=\"%1\" max=\"%1\" width=\"%2\" customWidth=\"1\"/>")
            .arg(c).arg(w);
        }
        cols += "</cols>\n";

        // Build sheetData section — xml contains only row data, not the header
        QString sd = "<sheetData>\n" + xml + "</sheetData>\n";

        // Merge cells
        QString mc;
        if (!merges.isEmpty()) {
            mc = "<mergeCells count=\"" + QString::number(merges.size()) + "\">\n";
            for (const QString& m : merges)
                mc += "<mergeCell ref=\"" + m + "\"/>\n";
            mc += "</mergeCells>\n";
        }

        // Assemble: header (declaration + worksheet + sheetViews) + cols + sheetData + merges + close
        QString full = xmlHeader + cols + sd + mc
                       + "<pageMargins left=\"0.7\" right=\"0.7\" top=\"0.75\" bottom=\"0.75\" "
                         "header=\"0.3\" footer=\"0.3\"/>\n"
                       + "</worksheet>";

        return full.toUtf8();
    }
};

// ── Helper: pick row text/number style based on alternating row ───────────────
// isAlt: even data rows (0-indexed)
static int rowStyleText(bool isAlt)  { return isAlt ? 5 : 6; }
static int rowStyleTND (bool isAlt)  { return isAlt ? 17: 18; }
static int rowStylePCT (bool isAlt)  { return isAlt ? 20: 21; }
static int rowStyleINT (bool isAlt)  { return isAlt ? 23: 24; }

// ═════════════════════════════════════════════════════════════════════════════
//  Build Sheet 1 — Résumé Trimestriel
// ═════════════════════════════════════════════════════════════════════════════
static QByteArray buildSheet1(SST& sst,
                              const QList<ProjFinData>& data,
                              const QList<ObsEntry>& obs,
                              int quarter, int year)
{
    static const QMap<int,QString> MONTH_NAMES = {
        {1,"Janvier"},{2,QString::fromUtf8("F\xc3\xa9vrier")},{3,"Mars"},{4,"Avril"},
        {5,"Mai"},{6,"Juin"},{7,"Juillet"},{8,QString::fromUtf8("Ao\xc3\xbbt")},
        {9,"Septembre"},{10,"Octobre"},{11,"Novembre"},{12,QString::fromUtf8("D\xc3\xa9" "cembre")}
    };
    int m1 = (quarter - 1) * 3 + 1;
    QString period = MONTH_NAMES[m1] + QString::fromUtf8(" \xe2\x80\x93 ") + MONTH_NAMES[m1+2]
                     + " " + QString::number(year);
    QString quarterLabel = QString("Q%1 %2").arg(quarter).arg(year);
    QString dateGen = QDate::currentDate().toString("dd/MM/yyyy");

    // Compute summary
    int nActifs = 0, nTermines = 0;
    double totalAlloue = 0, totalConsomme = 0, totalRestant = 0;
    for (const auto& d : data) {
        totalAlloue   += d.budgetAlloue;
        totalConsomme += d.budgetConsomme;
        totalRestant  += d.budgetRestant;
        QString s = d.statut.trimmed().toLower();
        if (s == "en cours" || s.contains("planifi")) ++nActifs;
        if (s.contains("termin")) ++nTermines;
    }
    double tauxGlobal = (totalAlloue > 0) ? (totalConsomme / totalAlloue) : 0.0;

    const QStringList STATUTS = {
        "En cours", QString::fromUtf8("Planifi\xc3\xa9"),
        QString::fromUtf8("Termin\xc3\xa9"),
        "Suspendu", QString::fromUtf8("Annul\xc3\xa9"),
        "En retard","Critique"
    };
    QMap<QString,int>    statutCount;
    QMap<QString,double> statutAlloue, statutConsomme, statutRestant;
    for (const QString& s : STATUTS) {
        statutCount[s] = 0;
        statutAlloue[s] = statutConsomme[s] = statutRestant[s] = 0.0;
    }
    for (const auto& d : data) {
        QString s = d.statut.trimmed();
        if (!statutCount.contains(s)) s = "Suspendu";
        statutCount[s]++;
        statutAlloue[s]   += d.budgetAlloue;
        statutConsomme[s] += d.budgetConsomme;
        statutRestant[s]  += d.budgetRestant;
    }

    SheetBuilder sh(&sst);
    sh.begin(QString::fromUtf8("R\xc3\xa9sum\xc3\xa9 Trimestriel"), 5);

    // Column widths (1-indexed)
    sh.colWidths = {{1,2},{2,38},{3,20},{4,20},{5,20},{6,20},{7,20},{8,20},{9,2}};

    // Row 1 — spacer (empty)
    sh.xml += "<row r=\"1\" ht=\"6\" customHeight=\"1\"/>\n";

    // Row 2 — Title  (merge B2:H2)
    sh.addMerge(2,2, 2,8);
    sh.beginRow(2); sh.xml += " ht=\"32\" customHeight=\"1\">";
    sh.addStrCell(2,2,
                  QString::fromUtf8("RAPPORT FINANCIER TRIMESTRIEL \xe2\x80\x94 SmartVision"), 9);
    sh.endRow();

    // Row 3 — subtitle
    sh.addMerge(3,2, 3,8);
    sh.beginRow(3); sh.xml += " ht=\"18\" customHeight=\"1\">";
    sh.addStrCell(3,2,"Bio-Laboratory Research Management System", 10);
    sh.endRow();

    // Row 4 — spacer
    sh.xml += "<row r=\"4\" ht=\"8\" customHeight=\"1\"/>\n";

    // Rows 5-8 — meta keys (cols E-F) / values (cols G-H)
    const QStringList metaKeys   = {"DATE DE PREPARATION", "TRIMESTRE",
                                  QString::fromUtf8("P\xc3\x89RIODE"), "LABORATOIRE"};
    const QStringList metaValues = {dateGen, quarterLabel, period, "SmartVision"};
    for (int i = 0; i < 4; ++i) {
        int r = 5 + i;
        sh.addMerge(r,5, r,6);
        sh.addMerge(r,7, r,8);
        sh.beginRow(r); sh.xml += " ht=\"17\" customHeight=\"1\">";
        sh.addStrCell(r,5, metaKeys[i],   11);
        sh.addStrCell(r,7, metaValues[i], 12);
        sh.endRow();
    }
    sh.xml += "<row r=\"9\" ht=\"10\" customHeight=\"1\"/>\n";

    // Rows 10-11 — KPI strip (6 KPIs across cols B-G)
    // Labels row 10, values row 11
    struct KPI { QString label; double val; int lblStyle; int valStyle; };
    const QList<KPI> kpis = {
        { QString::fromUtf8("Projets\nActifs"),         (double)nActifs,   13, 14 },
        { QString::fromUtf8("Budget\nTotal (TND)"),     totalAlloue,        13, 15 },
        { QString::fromUtf8("Budget\nConsomm\xc3\xa9 (TND)"), totalConsomme, 34, 14 },
        { QString::fromUtf8("Budget\nRestant (TND)"),   totalRestant,       34, 14 },
        { QString::fromUtf8("Taux de\nConsommation"),   tauxGlobal * 100.0, 36, 35 },
        { QString::fromUtf8("Projets\nTermin\xc3\xa9s"),(double)nTermines,  36, 35 }
    };
    sh.beginRow(10); sh.xml += " ht=\"44\" customHeight=\"1\">";
    for (int i = 0; i < 6; ++i)
        sh.addStrCell(10, 2+i, kpis[i].label, kpis[i].lblStyle);
    sh.endRow();
    sh.beginRow(11); sh.xml += " ht=\"26\" customHeight=\"1\">";
    for (int i = 0; i < 6; ++i) {
        // For taux use PCT style (valStyle 35 but need PCT numFmt — use style 22 total PCT)
        // We'll just store as plain number and let style handle it
        // KPI 4 (taux) → style 22 (total PCT); others use their valStyle
        int vs = kpis[i].valStyle;
        if (i == 4) vs = 22; // PCT total style
        sh.addNumCell(11, 2+i, kpis[i].val / (i==4 ? 100.0 : 1.0), vs);
    }
    sh.endRow();
    sh.xml += "<row r=\"12\" ht=\"10\" customHeight=\"1\"/>\n";

    // Row 13 — Section header "RÉPARTITION DES BUDGETS PAR STATUT"
    sh.addMerge(13,2, 13,8);
    sh.beginRow(13); sh.xml += " ht=\"20\" customHeight=\"1\">";
    sh.addStrCell(13,2,
                  QString::fromUtf8("R\xc3\x89PARTITION DES BUDGETS PAR STATUT"), 3);
    sh.endRow();

    // Row 14 — column headers
    const QStringList hdrStat = {
        "Statut", "Nb Projets",
        QString::fromUtf8("Budget Allou\xc3\xa9 (TND)"),
        QString::fromUtf8("Budget Consomm\xc3\xa9 (TND)"),
        "Budget Restant (TND)",
        QString::fromUtf8("% du Portfolio")
    };
    sh.beginRow(14); sh.xml += " ht=\"24\" customHeight=\"1\">";
    for (int i = 0; i < 6; ++i)
        sh.addStrCell(14, 2+i, hdrStat[i], 16);
    sh.endRow();

    // Data rows 15..
    for (int si = 0; si < STATUTS.size(); ++si) {
        int r = 15 + si;
        const QString& s = STATUTS[si];
        bool alt = (si % 2 == 0);
        double pct = (totalAlloue > 0) ? (statutAlloue[s] / totalAlloue) : 0.0;
        sh.beginRow(r); sh.xml += " ht=\"17\" customHeight=\"1\">";
        sh.addStrCell(r, 2, s,              rowStyleText(alt));
        sh.addNumCell(r, 3, statutCount[s], rowStyleINT(alt));
        sh.addNumCell(r, 4, statutAlloue[s],   rowStyleTND(alt));
        sh.addNumCell(r, 5, statutConsomme[s], rowStyleTND(alt));
        sh.addNumCell(r, 6, statutRestant[s],  rowStyleTND(alt));
        sh.addNumCell(r, 7, pct,               rowStylePCT(alt));
        sh.endRow();
    }

    // Total row
    int totR = 15 + STATUTS.size();
    sh.beginRow(totR); sh.xml += " ht=\"18\" customHeight=\"1\">";
    sh.addStrCell(totR, 2, "TOTAL", 7);
    sh.addNumCell(totR, 3, (double)data.size(), 25);
    sh.addNumCell(totR, 4, totalAlloue,   19);
    sh.addNumCell(totR, 5, totalConsomme, 19);
    sh.addNumCell(totR, 6, totalRestant,  19);
    sh.addNumCell(totR, 7, tauxGlobal,    22);
    sh.endRow();

    // AI Analysis section
    int aiStart = totR + 2;
    sh.addMerge(aiStart, 2, aiStart, 8);
    sh.beginRow(aiStart); sh.xml += " ht=\"20\" customHeight=\"1\">";
    sh.addStrCell(aiStart, 2,
                  QString::fromUtf8("ANALYSE INTELLIGENTE \xe2\x80\x94 OBSERVATIONS"), 2);
    sh.endRow();

    int aiHdr = aiStart + 1;
    sh.addMerge(aiHdr, 2, aiHdr, 3);
    sh.addMerge(aiHdr, 4, aiHdr, 8);
    sh.beginRow(aiHdr); sh.xml += " ht=\"18\" customHeight=\"1\">";
    sh.addStrCell(aiHdr, 2, "Observation", 16);
    sh.addStrCell(aiHdr, 4, QString::fromUtf8("D\xc3\xa9tail analytique"), 16);
    sh.endRow();

    for (int i = 0; i < obs.size() && i < 5; ++i) {
        int ro = aiHdr + 1 + i;
        bool alt = (i % 2 == 0);
        sh.addMerge(ro, 2, ro, 3);
        sh.addMerge(ro, 4, ro, 8);
        sh.beginRow(ro); sh.xml += " ht=\"30\" customHeight=\"1\">";
        sh.addStrCell(ro, 2, obs[i].titre,  alt ? 31 : 32);
        sh.addStrCell(ro, 4, obs[i].detail, alt ? 31 : 32);
        sh.endRow();
    }

    // Footer
    int footR = aiHdr + 7;
    sh.addMerge(footR, 2, footR, 8);
    sh.beginRow(footR); sh.xml += " ht=\"16\" customHeight=\"1\">";
    sh.addStrCell(footR, 2,
                  QString::fromUtf8("FIN DU RAPPORT \xe2\x80\x94 SmartVision Bio-Laboratory "
                                    "Research Management System \xe2\x80\x94 Confidentiel"), 30);
    sh.endRow();

    return sh.finish();
}

// ═════════════════════════════════════════════════════════════════════════════
//  Build Sheet 2 — Détail Projets
// ═════════════════════════════════════════════════════════════════════════════
static QByteArray buildSheet2(SST& sst, const QList<ProjFinData>& data)
{
    double totalAlloue=0, totalConsomme=0, totalRestant=0, tauxGlobal=0;
    int totalEmp=0, totalExp=0, totalBio=0, totalPub=0;
    for (const auto& d : data) {
        totalAlloue += d.budgetAlloue; totalConsomme += d.budgetConsomme;
        totalRestant += d.budgetRestant;
        totalEmp += d.nbEmployes; totalExp += d.nbExperiences;
        totalBio += d.nbBioSamples; totalPub += d.nbPublications;
    }
    if (totalAlloue > 0) tauxGlobal = totalConsomme / totalAlloue;

    SheetBuilder sh(&sst);
    sh.begin(QString::fromUtf8("D\xc3\xa9tail Projets"), 5);
    sh.colWidths = {{1,2},{2,36},{3,24},{4,16},{5,16},{6,16},{7,16},
                    {8,10},{9,10},{10,10},{11,10},{12,22},{13,22},{14,2}};

    sh.xml += "<row r=\"1\" ht=\"6\" customHeight=\"1\"/>\n";

    sh.addMerge(2,2,2,13);
    sh.beginRow(2); sh.xml += " ht=\"28\" customHeight=\"1\">";
    sh.addStrCell(2,2,
                  QString::fromUtf8("D\xc3\x89TAIL FINANCIER PAR PROJET"), 9);
    sh.endRow();

    sh.addMerge(3,2,3,13);
    sh.beginRow(3); sh.xml += " ht=\"16\" customHeight=\"1\">";
    sh.addStrCell(3,2,
                  QString::fromUtf8("Co\xc3\xbbt = (Employ\xc3\xa9s\xc3\x97" "2 500 + "
                                    "Exp\xc3\xa9riences\xc3\x97" "500 + "
                                    "BioSamples\xc3\x97" "150 + Publications\xc3\x97" "200) \xc3\x97 1.15"), 10);
    sh.endRow();

    sh.xml += "<row r=\"4\" ht=\"8\" customHeight=\"1\"/>\n";

    const QStringList h2 = {
        "Nom du Projet", "Domaine de Recherche", "Statut",
        QString::fromUtf8("Budget Allou\xc3\xa9 (TND)"),
        QString::fromUtf8("Budget Consomm\xc3\xa9 (TND)"),
        "Budget Restant (TND)",
        QString::fromUtf8("% Consomm\xc3\xa9"),
        QString::fromUtf8("Nb Employ\xc3\xa9s"),
        QString::fromUtf8("Nb Exp\xc3\xa9riences"),
        "Nb BioSamples", "Nb Publications",
        "Source de Financement",
        QString::fromUtf8("N\xc2\xb0 \xc3\x89thique")
    };
    sh.beginRow(5); sh.xml += " ht=\"40\" customHeight=\"1\">";
    for (int i = 0; i < h2.size(); ++i)
        sh.addStrCell(5, 2+i, h2[i], 1);
    sh.endRow();

    for (int pi = 0; pi < data.size(); ++pi) {
        const auto& d = data[pi];
        int row = 6 + pi;
        bool alt = (pi % 2 == 0);
        QString src = d.sourceFinancement.trimmed().isEmpty() ?
                          QString::fromUtf8("\xe2\x80\x94") : d.sourceFinancement;
        QString eth = d.ethique.trimmed().isEmpty() ?
                          QString::fromUtf8("\xe2\x80\x94") : d.ethique;

        sh.beginRow(row); sh.xml += " ht=\"17\" customHeight=\"1\">";
        sh.addStrCell(row, 2,  d.nom,     rowStyleText(alt));
        sh.addStrCell(row, 3,  d.domaine, rowStyleText(alt));
        sh.addStrCell(row, 4,  d.statut,  rowStyleText(alt));
        sh.addNumCell(row, 5,  d.budgetAlloue,     rowStyleTND(alt));
        sh.addNumCell(row, 6,  d.budgetConsomme,   rowStyleTND(alt));
        sh.addNumCell(row, 7,  d.budgetRestant,    rowStyleTND(alt));
        sh.addNumCell(row, 8,  d.pctConsomme/100.0,rowStylePCT(alt));
        sh.addNumCell(row, 9,  d.nbEmployes,       rowStyleINT(alt));
        sh.addNumCell(row, 10, d.nbExperiences,    rowStyleINT(alt));
        sh.addNumCell(row, 11, d.nbBioSamples,     rowStyleINT(alt));
        sh.addNumCell(row, 12, d.nbPublications,   rowStyleINT(alt));
        sh.addStrCell(row, 13, src, rowStyleText(alt));
        sh.addStrCell(row, 14, eth, rowStyleText(alt));
        sh.endRow();
    }

    int tr = 6 + data.size();
    sh.beginRow(tr); sh.xml += " ht=\"20\" customHeight=\"1\">";
    sh.addStrCell(tr, 2, "TOTAL", 7);
    sh.addEmptyCell(tr, 3, 7); sh.addEmptyCell(tr, 4, 7);
    sh.addNumCell(tr, 5,  totalAlloue,   19);
    sh.addNumCell(tr, 6,  totalConsomme, 19);
    sh.addNumCell(tr, 7,  totalRestant,  19);
    sh.addNumCell(tr, 8,  tauxGlobal,    22);
    sh.addNumCell(tr, 9,  totalEmp,  25);
    sh.addNumCell(tr, 10, totalExp,  25);
    sh.addNumCell(tr, 11, totalBio,  25);
    sh.addNumCell(tr, 12, totalPub,  25);
    sh.endRow();

    return sh.finish();
}

// ═════════════════════════════════════════════════════════════════════════════
//  Build Sheet 3 — Répartition Dépenses
// ═════════════════════════════════════════════════════════════════════════════
static QByteArray buildSheet3(SST& sst, const QList<ProjFinData>& data)
{
    SheetBuilder sh(&sst);
    sh.begin(QString::fromUtf8("R\xc3\xa9partition D\xc3\xa9penses"), 5);
    sh.colWidths = {{1,2},{2,36},{3,24},{4,16},{5,16},{6,16},{7,16},{8,16},{9,16},{10,2}};

    sh.xml += "<row r=\"1\" ht=\"6\" customHeight=\"1\"/>\n";

    sh.addMerge(2,2,2,9);
    sh.beginRow(2); sh.xml += " ht=\"28\" customHeight=\"1\">";
    sh.addStrCell(2,2,
                  QString::fromUtf8("R\xc3\x89PARTITION DES D\xc3\x89PENSES PAR PROJET"), 9);
    sh.endRow();

    sh.addMerge(3,2,3,9);
    sh.beginRow(3); sh.xml += " ht=\"16\" customHeight=\"1\">";
    sh.addStrCell(3,2,
                  QString::fromUtf8("Employ\xc3\xa9s\xc3\x97" "2 500 TND | "
                                    "Exp\xc3\xa9riences\xc3\x97" "500 | "
                                    "BioSamples\xc3\x97" "150 | Publications\xc3\x97" "200 | Overhead +15%"), 10);
    sh.endRow();

    sh.xml += "<row r=\"4\" ht=\"8\" customHeight=\"1\"/>\n";

    const QStringList h3 = {
        "Nom du Projet", "Domaine",
        QString::fromUtf8("Co\xc3\xbbt Employ\xc3\xa9s (TND)"),
        QString::fromUtf8("Co\xc3\xbbt Exp\xc3\xa9riences (TND)"),
        "Cout BioSamples (TND)", "Cout Publications (TND)",
        "Sous-Total (TND)", "Overhead 15% (TND)", "TOTAL (TND)"
    };
    sh.beginRow(5); sh.xml += " ht=\"40\" customHeight=\"1\">";
    for (int i = 0; i < h3.size(); ++i)
        sh.addStrCell(5, 2+i, h3[i], 1);
    sh.endRow();

    double totEmp=0,totExp=0,totBio=0,totPub=0,totSub=0,totOvh=0,totTot=0;
    for (int pi = 0; pi < data.size(); ++pi) {
        const auto& d = data[pi];
        int row = 6 + pi;
        bool alt = (pi % 2 == 0);
        sh.beginRow(row); sh.xml += " ht=\"17\" customHeight=\"1\">";
        sh.addStrCell(row, 2, d.nom,     rowStyleText(alt));
        sh.addStrCell(row, 3, d.domaine, rowStyleText(alt));
        sh.addNumCell(row, 4, d.coutEmployes,    rowStyleTND(alt));
        sh.addNumCell(row, 5, d.coutExperiences, rowStyleTND(alt));
        sh.addNumCell(row, 6, d.coutBioSamples,  rowStyleTND(alt));
        sh.addNumCell(row, 7, d.coutPublications,rowStyleTND(alt));
        sh.addNumCell(row, 8, d.coutSousTotal,   rowStyleTND(alt));
        sh.addNumCell(row, 9, d.coutOverhead,    rowStyleTND(alt));
        sh.addNumCell(row, 10, d.coutTotal,      rowStyleTND(alt));
        sh.endRow();
        totEmp+=d.coutEmployes; totExp+=d.coutExperiences;
        totBio+=d.coutBioSamples; totPub+=d.coutPublications;
        totSub+=d.coutSousTotal; totOvh+=d.coutOverhead; totTot+=d.coutTotal;
    }

    int tr = 6 + data.size();
    sh.beginRow(tr); sh.xml += " ht=\"20\" customHeight=\"1\">";
    sh.addStrCell(tr, 2, "TOTAL", 7);
    sh.addEmptyCell(tr, 3, 7);
    sh.addNumCell(tr, 4,  totEmp, 19);
    sh.addNumCell(tr, 5,  totExp, 19);
    sh.addNumCell(tr, 6,  totBio, 19);
    sh.addNumCell(tr, 7,  totPub, 19);
    sh.addNumCell(tr, 8,  totSub, 19);
    sh.addNumCell(tr, 9,  totOvh, 19);
    sh.addNumCell(tr, 10, totTot, 19);
    sh.endRow();

    return sh.finish();
}

// ═════════════════════════════════════════════════════════════════════════════
//  Build Sheet 4 — Alertes Budgétaires
// ═════════════════════════════════════════════════════════════════════════════
static QByteArray buildSheet4(SST& sst, const QList<ProjFinData>& data)
{
    SheetBuilder sh(&sst);
    sh.begin(QString::fromUtf8("Alertes Budg\xc3\xa9taires"), 0);
    sh.colWidths = {{1,2},{2,32},{3,22},{4,24},{5,18},{6,18},{7,42},{8,2}};

    sh.xml += "<row r=\"1\" ht=\"6\" customHeight=\"1\"/>\n";

    sh.addMerge(2,2,2,7);
    sh.beginRow(2); sh.xml += " ht=\"28\" customHeight=\"1\">";
    sh.addStrCell(2,2,
                  QString::fromUtf8("ALERTES BUDG\xc3\x89TAIRES"), 4);
    sh.endRow();

    sh.addMerge(3,2,3,7);
    sh.beginRow(3); sh.xml += " ht=\"16\" customHeight=\"1\">";
    sh.addStrCell(3,2,
                  QString::fromUtf8("Projets n\xc3\xa9" "cessitant une attention financi\xc3\xa8re imm\xc3\xa9" "diate"), 4);
    sh.endRow();

    sh.xml += "<row r=\"4\" ht=\"8\" customHeight=\"1\"/>\n";

    const QStringList h4 = {
        "Nom du Projet", "Domaine",
        QString::fromUtf8("Type d\xe2\x80\x99" "Alerte"),
        QString::fromUtf8("Valeur Actuelle (TND)"),
        "Seuil (TND)", "Recommandation"
    };
    sh.beginRow(5); sh.xml += " ht=\"36\" customHeight=\"1\">";
    for (int i = 0; i < h4.size(); ++i)
        sh.addStrCell(5, 2+i, h4[i], 3);
    sh.endRow();

    struct AlertDef { QString type; double val; double seuil; QString reco; int textStyle; int numStyle; };

    int alertRow = 6;
    for (const auto& d : data) {
        QList<AlertDef> alerts;

        if (d.budgetAlloue <= 0)
            alerts.append({
                           QString::fromUtf8("Budget non d\xc3\xa9" "fini"), 0.0, 0.0,
                           QString::fromUtf8("D\xc3\xa9" "finir un budget allou\xc3\xa9 avant de d\xc3\xa9marrer."),
                           6, 18 });
        else if (d.budgetConsomme > d.budgetAlloue)
            alerts.append({
                           QString::fromUtf8("Budget d\xc3\xa9pass\xc3\xa9"), d.budgetConsomme, d.budgetAlloue,
                           QString::fromUtf8("R\xc3\xa9" "duire le p\xc3\xa9rim\xc3\xa8tre ou demander un avenant budg\xc3\xa9taire."),
                           8, 26 });
        else if (d.pctConsomme > 85.0)
            alerts.append({
                           QString::fromUtf8("Risque d\xc3\xa9passement (>85%)"), d.budgetConsomme, d.budgetAlloue * 0.85,
                           QString::fromUtf8("Surveiller les d\xc3\xa9penses et anticiper un d\xc3\xa9passement."),
                           27, 28 });

        if (d.sourceFinancement.trimmed().isEmpty())
            alerts.append({
                           "Source financement manquante", 0.0, 0.0,
                           QString::fromUtf8("Renseigner la source de financement dans la fiche projet."),
                           29, 18 });
        if (d.ethique.trimmed().isEmpty())
            alerts.append({
                           QString::fromUtf8("Approbation \xc3\xa9thique manquante"), 0.0, 0.0,
                           QString::fromUtf8("Soumettre le dossier au comit\xc3\xa9 \xc3\xa9thique imm\xc3\xa9" "diatement."),
                           29, 18 });

        for (const AlertDef& a : alerts) {
            sh.beginRow(alertRow); sh.xml += " ht=\"22\" customHeight=\"1\">";
            sh.addStrCell(alertRow, 2, d.nom,     a.textStyle);
            sh.addStrCell(alertRow, 3, d.domaine, a.textStyle);
            sh.addStrCell(alertRow, 4, a.type,    a.textStyle);
            sh.addNumCell(alertRow, 5, a.val,     a.numStyle);
            sh.addNumCell(alertRow, 6, a.seuil,   a.numStyle);
            sh.addStrCell(alertRow, 7, a.reco,    a.textStyle);
            sh.endRow();
            ++alertRow;
        }
    }

    if (alertRow == 6) {
        sh.addMerge(6,2,6,7);
        sh.beginRow(6); sh.xml += " ht=\"22\" customHeight=\"1\">";
        sh.addStrCell(6,2,
                      QString::fromUtf8("Aucune alerte budg\xc3\xa9taire d\xc3\xa9tect\xc3\xa9" "e."), 2);
        sh.endRow();
        alertRow = 7;
    }

    // Legend
    int legR = alertRow + 2;
    sh.addMerge(legR,2,legR,7);
    sh.beginRow(legR); sh.xml += " ht=\"17\" customHeight=\"1\">";
    sh.addStrCell(legR, 2, QString::fromUtf8("L\xc3\x89GENDE DES ALERTES"), 7);
    sh.endRow(); ++legR;

    const QList<QPair<QString,int>> legendItems = {
        { QString::fromUtf8("\xe2\x97\x8f Budget d\xc3\xa9pass\xc3\xa9 (consomm\xc3\xa9 > allou\xc3\xa9)"), 8 },
        { QString::fromUtf8("\xe2\x97\x8f Risque d\xc3\xa9passement (consomm\xc3\xa9 > 85% allou\xc3\xa9)"), 27 },
        { QString::fromUtf8("\xe2\x97\x8f Budget non d\xc3\xa9" "fini (0 TND)"), 6 },
        { QString::fromUtf8("\xe2\x97\x8f Source de financement manquante"), 29 },
        { QString::fromUtf8("\xe2\x97\x8f Approbation \xc3\xa9thique manquante"), 29 }
    };
    for (const auto& [lbl, sty] : legendItems) {
        sh.addMerge(legR,2,legR,7);
        sh.beginRow(legR); sh.xml += " ht=\"16\" customHeight=\"1\">";
        sh.addStrCell(legR, 2, lbl, sty);
        sh.endRow(); ++legR;
    }

    return sh.finish();
}

// ═════════════════════════════════════════════════════════════════════════════
//  Assemble all XML parts into a .xlsx ZIP
// ═════════════════════════════════════════════════════════════════════════════
static bool generateXlsx(const QString& outputPath,
                         int quarter, int year,
                         const QList<ProjFinData>& data,
                         const QList<ObsEntry>& obs)
{
    // ── 1. Build shared strings & sheets ─────────────────────────────────────
    SST sst;

    QByteArray sheet1 = buildSheet1(sst, data, obs, quarter, year);
    QByteArray sheet2 = buildSheet2(sst, data);
    QByteArray sheet3 = buildSheet3(sst, data);
    QByteArray sheet4 = buildSheet4(sst, data);

    // ── 2. Shared strings XML ─────────────────────────────────────────────────
    QString sstXml = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
                     "<sst xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" "
                     "count=\"" + QString::number(sst.strings.size()) + "\" "
                                                             "uniqueCount=\"" + QString::number(sst.strings.size()) + "\">\n";
    for (const QString& s : sst.strings) {
        sstXml += "<si><t xml:space=\"preserve\">" + xmlEsc(s) + "</t></si>\n";
    }
    sstXml += "</sst>";

    // ── 3. Workbook XML ───────────────────────────────────────────────────────
    const char* workbook = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<workbook xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main"
          xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <sheets>
    <sheet name="R&#233;sum&#233; Trimestriel"  sheetId="1" r:id="rId1"/>
    <sheet name="D&#233;tail Projets"           sheetId="2" r:id="rId2"/>
    <sheet name="R&#233;partition D&#233;penses" sheetId="3" r:id="rId3"/>
    <sheet name="Alertes Budg&#233;taires"      sheetId="4" r:id="rId4"/>
  </sheets>
</workbook>)";

    // ── 4. Relationships ──────────────────────────────────────────────────────
    const char* wbRels = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet" Target="worksheets/sheet1.xml"/>
  <Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet" Target="worksheets/sheet2.xml"/>
  <Relationship Id="rId3" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet" Target="worksheets/sheet3.xml"/>
  <Relationship Id="rId4" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet" Target="worksheets/sheet4.xml"/>
  <Relationship Id="rId5" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/sharedStrings" Target="sharedStrings.xml"/>
  <Relationship Id="rId6" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles" Target="styles.xml"/>
</Relationships>)";

    const char* rootRels = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="xl/workbook.xml"/>
</Relationships>)";

    const char* contentTypes = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml"  ContentType="application/xml"/>
  <Override PartName="/xl/workbook.xml"            ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml"/>
  <Override PartName="/xl/worksheets/sheet1.xml"   ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml"/>
  <Override PartName="/xl/worksheets/sheet2.xml"   ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml"/>
  <Override PartName="/xl/worksheets/sheet3.xml"   ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml"/>
  <Override PartName="/xl/worksheets/sheet4.xml"   ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml"/>
  <Override PartName="/xl/sharedStrings.xml"        ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.sharedStrings+xml"/>
  <Override PartName="/xl/styles.xml"               ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml"/>
</Types>)";

    // ── 5. Pack into ZIP ──────────────────────────────────────────────────────
    QList<ZipEntry> entries;
    auto addEntry = [&](const QString& name, const QByteArray& data2) {
        ZipEntry e;
        e.name = name;
        e.data = data2;
        entries.append(e);
    };

    addEntry("[Content_Types].xml",       QByteArray(contentTypes));
    addEntry("_rels/.rels",               QByteArray(rootRels));
    addEntry("xl/workbook.xml",           QByteArray(workbook));
    addEntry("xl/_rels/workbook.xml.rels",QByteArray(wbRels));
    addEntry("xl/styles.xml",             buildStyles());
    addEntry("xl/sharedStrings.xml",      sstXml.toUtf8());
    addEntry("xl/worksheets/sheet1.xml",  sheet1);
    addEntry("xl/worksheets/sheet2.xml",  sheet2);
    addEntry("xl/worksheets/sheet3.xml",  sheet3);
    addEntry("xl/worksheets/sheet4.xml",  sheet4);

    return writeZip(outputPath, entries);
}

// ═════════════════════════════════════════════════════════════════════════════
//  SECTION D — MAIN DIALOG  (unchanged from original)
// ═════════════════════════════════════════════════════════════════════════════

void GestProjCrud::generateFinancialReport(int /*quarter*/, int /*year*/, QWidget* parent)
{
    QDialog* dlg = new QDialog(parent,
                               Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dlg->setWindowTitle(QString::fromUtf8("Rapport Financier Trimestriel"));
    dlg->setMinimumSize(480, 360);
    dlg->setAttribute(Qt::WA_DeleteOnClose);

    dlg->setStyleSheet(R"(
        QDialog    { background-color: #1B2B3A; }
        QLabel     { color: #D0E8FF; font-size: 12px; }
        QLabel#hdr { color: #4FC3A1; font-size: 15px; font-weight: 900; }
        QLabel#sub { color: #7FA8C8; font-size: 11px; font-style: italic; }
        QComboBox  { background-color: #243447; color: #D0E8FF;
                     border: 1px solid #3A5068; border-radius: 5px;
                     padding: 5px 10px; font-size: 12px; }
        QComboBox::drop-down { border: none; width: 20px; }
        QComboBox QAbstractItemView { background-color: #243447; color: #D0E8FF;
                                      selection-background-color: #0A5F58; }
        QFrame#div { color: #3A5068; }
    )");

    QVBoxLayout* mainL = new QVBoxLayout(dlg);
    mainL->setContentsMargins(24, 20, 24, 20);
    mainL->setSpacing(14);

    QLabel* hdr = new QLabel(QString::fromUtf8(
        "G\xc3\xa9n\xc3\xa9rer un rapport financier trimestriel"));
    hdr->setObjectName("hdr");
    mainL->addWidget(hdr);

    QLabel* sub = new QLabel(QString::fromUtf8(
        "\xe2\x9c\xa8  Export Excel professionnel (.xlsx) \xe2\x80\x94 100% natif Qt"));
    sub->setObjectName("sub");
    mainL->addWidget(sub);

    QFrame* div = new QFrame; div->setObjectName("div");
    div->setFrameShape(QFrame::HLine); div->setFrameShadow(QFrame::Sunken);
    mainL->addWidget(div);

    // Quarter
    QHBoxLayout* qL = new QHBoxLayout; qL->setSpacing(12);
    QLabel* qLbl = new QLabel("Trimestre :"); qLbl->setFixedWidth(80);
    QComboBox* qCombo = new QComboBox;
    qCombo->addItem("Q1 (Jan-Mar)", 1);
    qCombo->addItem("Q2 (Avr-Jun)", 2);
    qCombo->addItem("Q3 (Jul-Sep)", 3);
    qCombo->addItem("Q4 (Oct-Dec)", 4);
    qCombo->setCurrentIndex((QDate::currentDate().month() - 1) / 3);
    qL->addWidget(qLbl); qL->addWidget(qCombo, 1);
    mainL->addLayout(qL);

    // Year
    QHBoxLayout* yL = new QHBoxLayout; yL->setSpacing(12);
    QLabel* yLbl = new QLabel(QString::fromUtf8("Ann\xc3\xa9" "e :")); yLbl->setFixedWidth(80);
    QComboBox* yCombo = new QComboBox;
    int cy = QDate::currentDate().year();
    for (int y = cy - 2; y <= cy + 1; ++y)
        yCombo->addItem(QString::number(y), y);
    yCombo->setCurrentIndex(2);
    yL->addWidget(yLbl); yL->addWidget(yCombo, 1);
    mainL->addLayout(yL);

    mainL->addSpacing(6);

    QLabel* info = new QLabel(QString::fromUtf8(
        "Le rapport Excel g\xc3\xa9n\xc3\xa9r\xc3\xa9 contiendra :\n"
        "  \xe2\x80\xa2 R\xc3\xa9sum\xc3\xa9 trimestriel (KPIs, r\xc3\xa9partition par statut)\n"
        "  \xe2\x80\xa2 D\xc3\xa9tail financier complet par projet\n"
        "  \xe2\x80\xa2 R\xc3\xa9partition des d\xc3\xa9penses (employ\xc3\xa9s, exp., biosamples)\n"
        "  \xe2\x80\xa2 Alertes budg\xc3\xa9taires automatiques\n"
        "  \xe2\x80\xa2 Analyse intelligente IA (5 observations)\n"
        "  \xe2\x80\xa2 \xe2\x9c\x94 Aucun Python requis \xe2\x80\x94 g\xc3\xa9n\xc3\xa9ration 100% native"));
    info->setWordWrap(true);
    info->setStyleSheet(
        "color:#7FA8C8; font-size:11px; padding:10px;"
        "background-color:#243447; border-radius:6px; border:1px solid #3A5068;");
    mainL->addWidget(info);
    mainL->addStretch(1);

    // Buttons
    QHBoxLayout* bL = new QHBoxLayout; bL->setSpacing(10); bL->addStretch(1);

    QPushButton* cancelBtn = new QPushButton("Annuler");
    cancelBtn->setFixedWidth(90);
    cancelBtn->setStyleSheet(
        "QPushButton{background:#3A5068;color:#78a0c5;border:1px solid #3A5068;"
        "border-radius:6px;padding:7px;font-size:12px;}"
        "QPushButton:hover{background:#2E4460;}");
    QObject::connect(cancelBtn, &QPushButton::clicked, dlg, &QDialog::reject);
    bL->addWidget(cancelBtn);

    QPushButton* genBtn = new QPushButton(
        QString::fromUtf8("G\xc3\xa9n\xc3\xa9rer Rapport (.xlsx)"));
    genBtn->setFixedWidth(200);
    genBtn->setStyleSheet(
        "QPushButton{background:#0A5F58;color:white;border:none;"
        "border-radius:6px;padding:8px;font-weight:600;font-size:12px;}"
        "QPushButton:hover{background:#0D7A71;}"
        "QPushButton:pressed{background:#074F49;}");
    bL->addWidget(genBtn);
    mainL->addLayout(bL);

    QObject::connect(genBtn, &QPushButton::clicked, dlg, [=]() {
        int selQ = qCombo->currentData().toInt();
        int selY = yCombo->currentData().toInt();

        QString fileName = QString("Rapport_Financier_Q%1_%2.xlsx").arg(selQ).arg(selY);
        QString filePath = QFileDialog::getSaveFileName(
            dlg,
            QString::fromUtf8("Enregistrer le rapport financier"),
            QDir::homePath() + "/" + fileName,
            "Excel (*.xlsx);;All Files (*)");
        if (filePath.isEmpty()) return;
        if (!filePath.endsWith(".xlsx", Qt::CaseInsensitive))
            filePath += ".xlsx";

        QProgressDialog* prog = new QProgressDialog(
            QString::fromUtf8(
                "G\xc3\xa9n\xc3\xa9ration du rapport en cours...\n"
                "Collecte des donn\xc3\xa9" "es et analyse IA..."),
            QString(), 0, 0, dlg);
        prog->setWindowTitle(QString::fromUtf8("SmartVision \xe2\x80\x94 Rapport"));
        prog->setWindowModality(Qt::WindowModal);
        prog->setMinimumDuration(0);
        prog->setStyleSheet(
            "QProgressDialog{background-color:#1B2B3A;}"
            "QLabel{color:#D0E8FF;font-size:12px;}"
            "QPushButton{display:none;}");
        prog->show();
        QCoreApplication::processEvents();

        // 1. Load data
        QList<ProjFinData> finData = loadAllProjFinData();

        // 2. AI observations
        QList<ObsEntry> aiObs = callAIForObservations(finData, selQ, selY, dlg);

        // 3. Generate xlsx natively (no Python)
        bool ok = generateXlsx(filePath, selQ, selY, finData, aiObs);

        prog->close();
        prog->deleteLater();

        if (!ok) {
            QMessageBox::critical(dlg, "Erreur",
                                  QString::fromUtf8(
                                      "Impossible d'\xc3\xa9" "crire le fichier Excel.\n\n"
                                      "V\xc3\xa9rifiez que le chemin de destination est accessible\n"
                                      "et que vous avez les droits en \xc3\xa9" "criture."));
            return;
        }

        // Success
        QDialog* okDlg = new QDialog(dlg,
                                     Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
        okDlg->setWindowTitle(QString::fromUtf8("Succ\xc3\xa8s"));
        okDlg->setFixedSize(420, 200);
        okDlg->setAttribute(Qt::WA_DeleteOnClose);
        okDlg->setStyleSheet(
            "QDialog{background-color:#1B2B3A;}"
            "QLabel{color:#D0E8FF;font-size:12px;}"
            "QLabel#t{color:#4FC3A1;font-size:14px;font-weight:700;}"
            "QLabel#f{color:#7FA8C8;font-size:11px;}"
            "QPushButton{background:#0A5F58;color:#FFFFFF;border:none;"
            "border-radius:6px;padding:7px 24px;font-weight:600;font-size:12px;}"
            "QPushButton:hover{background:#0D7A71;}");
        QVBoxLayout* sl = new QVBoxLayout(okDlg);
        sl->setContentsMargins(24,20,24,18); sl->setSpacing(8);
        QLabel* tl = new QLabel(QString::fromUtf8(
            "\xe2\x9c\x94  Rapport g\xc3\xa9n\xc3\xa9r\xc3\xa9 avec succ\xc3\xa8s !"));
        tl->setObjectName("t"); sl->addWidget(tl);
        QLabel* fl2 = new QLabel(QFileInfo(filePath).fileName());
        fl2->setObjectName("f"); fl2->setWordWrap(true); sl->addWidget(fl2);
        QLabel* al = new QLabel(QString::fromUtf8(
            "\xe2\x9c\xa8  4 feuilles Excel \xe2\x80\x94 Analyse IA int\xc3\xa9" "gr\xc3\xa9" "e "
            "\xe2\x80\x94 100% natif Qt"));
        al->setObjectName("f"); sl->addWidget(al);
        sl->addStretch(1);
        QHBoxLayout* sbl = new QHBoxLayout; sbl->addStretch(1);
        QPushButton* okBtn2 = new QPushButton("OK");
        okBtn2->setFixedWidth(80);
        QObject::connect(okBtn2, &QPushButton::clicked, okDlg, &QDialog::accept);
        sbl->addWidget(okBtn2); sl->addLayout(sbl);
        okDlg->exec();

        QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
        dlg->accept();
    });

    dlg->exec();
}
