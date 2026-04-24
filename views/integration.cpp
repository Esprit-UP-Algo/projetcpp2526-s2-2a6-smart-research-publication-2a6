// ===================== biosimple.cpp (UN SEUL FICHIER - BioSimple + Gestion Projet - 3 Widgets) =====================
#include "integration.h"
#include "crudebiosimple.h"
#include "publication.h"
#include "publicationScorer.h"
#include "chatbotbiosimple.h"
#include "crudexperience.h"
#include "simple.h"
#include "crudEquipement.h"
#include "employes.h"
#include "gestproj.h"
#include "../models/TracabiliteManager.h"
#include "cong.h"
#include "basicbio.h"
#include "pdfbiosample.h"
#include "pdfequipement.h"
#include "pdfExp.h"
#include "pdfemploye.h"
#include "floatingchatbtn.h"
#include "voicecommande.h"
#include "signupserver.h"
#include "captchawidget.h"
#include "emailsender.h"
#include "LeconsApprises.h"
#include <QTextEdit>
#include <QTextCharFormat>
#include <QCalendarWidget>

#include <QPainterPath>
#include <QDialog>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

#include <QApplication>
#include <QGuiApplication>
#include <QMainWindow>
#include <QScreen>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QLabel>
#include <QToolButton>
#include <QPushButton>
#include <QMenu>
#include <QAction>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QGraphicsDropShadowEffect>
#include <QStyle>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QDate>
#include <QDateTime>
#include <QStackedWidget>
#include <QDateEdit>
#include <QSpinBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QTreeWidget>
#include <QListWidget>
#include <QLinearGradient>
#include <QScrollArea>
#include <QMap>
#include <QtMath>
#include <QResizeEvent>
#include <QPixmap>
#include <QFont>
#include <QColor>
#include <QDesktopServices>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "apiconfig.h"
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QGraphicsColorizeEffect>
#include <QEasingCurve>
#include <QStackedLayout>
#include <QUrl>
#include <QUrlQuery>
#include <QSettings>
#include <QTcpSocket>
#include <QThread>
#include <QProcess>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QBuffer>

#include <QPrinter>
#include <QTextDocument>
#include <QIntValidator>
#include <QRegularExpressionValidator>
#include <QRegularExpression>
#include <QCompleter>
#include <QFileDialog>
#include <QTextStream>
#include <QVector>
#include <QSet>
#include <cmath>
#include <algorithm>
#include <functional>

// ===================== COULEURS =====================
static const QString C_TOPBAR     = "#12443B";
static const QString C_PRIMARY    = "#0A5F58";
static const QString C_BG         = "#A3CAD3";
static const QString C_BEIGE      = "#C6B29A";
static const QString C_TEXT_DARK  = "#64533A";
static const QString C_TABLE_HDR  = "#AFC6C3";
static const QString C_ROW_ODD    = "#F2F0EB";
static const QString C_ROW_EVEN   = "#ECE9E2";
static const QString C_BORDER     = "rgba(0,0,0,0.10)";
static const QString C_PANEL_BG   = "rgba(246,248,247,0.86)";
static const QString C_PANEL_IN   = "rgba(244,246,245,0.78)";
static const QString C_PANEL_BR   = "rgba(0,0,0,0.10)";

static const QColor  W_GREEN      = QColor("#2E6F63");
static const QColor  W_ORANGE     = QColor("#B5672C");
static const QColor  W_RED        = QColor("#8B2F3C");
static const QColor  W_GRAY       = QColor("#7A8B8A");

// ── Fetch {email, displayName} for every active employee ─────────────────────
Q_DECL_UNUSED static QList<QPair<QString,QString>> getAllUserEmails()
{
    QList<QPair<QString,QString>> result;
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isValid() || !db.isOpen()) return result;
    QSqlQuery q(db);
    if (!q.exec("SELECT \"EMAIL\", NVL(\"FULL_NAME\", \"nom\" || ' ' || \"prenom\") "
                "FROM \"Employés\" "
                "WHERE NVL(\"ACTIVE\",'O') = 'O' "
                "  AND \"EMAIL\" IS NOT NULL "
                "  AND TRIM(\"EMAIL\") IS NOT NULL")) {
        qWarning() << "[getAllUserEmails]" << q.lastError().text();
        return result;
    }
    while (q.next()) {
        const QString em   = q.value(0).toString().trimmed();
        const QString name = q.value(1).toString().trimmed();
        if (!em.isEmpty())
            result.append(qMakePair(em, name.isEmpty() ? em : name));
    }
    return result;
}

// ===================== SMART ASSIGNMENT ENGINE =====================
struct SmartEmpSuggestion {
    int employeeId  = 0;
    QString fullName;
    QString role;
    QString specialization;
    int activeProjects = 0;
    int matchPercent   = 0;
    QString explanation;
};

static int clampScore(int v) { return std::max(0, std::min(100, v)); }

static bool loadSmartProjectSuggestions(int projetId,
                                        const QString& domaineProjet,
                                        const QString& requiredRole,
                                        QVector<SmartEmpSuggestion>& out,
                                        QString* error = nullptr,
                                        int maxCapacity = 3)
{
    out.clear();
    if (projetId <= 0) { if (error) *error = "Projet invalide."; return false; }

    QSqlQuery q;
    q.prepare(
        "SELECT e.\"employee_id\", "
        "       NVL(e.\"FULL_NAME\", TRIM(e.\"prenom\" || ' ' || e.\"nom\")) AS full_name, "
        "       NVL(e.\"ROLE\", 'Chercheur') AS role_name, "
        "       NVL(e.\"specialization\", '') AS specialization, "
        "       (SELECT COUNT(*) FROM \"Associer\" a2 WHERE a2.\"employee_id\" = e.\"employee_id\") AS active_projects, "
        "       NVL(e.\"NB_PUBLICATIONS\", 0) AS pubs_count, "
        "       NVL(e.\"QUALIFICATION\", '') AS qualification, "
        "       NVL(e.\"TEMPS_TRAVAIL\", '') AS temps_travail "
        "FROM \"Employés\" e "
        "WHERE e.\"ACTIVE\" = 'O' "
        "  AND LOWER(NVL(e.\"ROLE\", 'chercheur')) IN ('chercheur','technicien') "
        "  AND NOT EXISTS ( "
        "      SELECT 1 FROM \"Associer\" ax "
        "      WHERE ax.\"employee_id\" = e.\"employee_id\" AND ax.\"Id_projet\" = :pid "
        "  ) "
        "ORDER BY e.\"nom\", e.\"prenom\", e.\"employee_id\""
    );
    q.bindValue(":pid", projetId);
    if (!q.exec()) { if (error) *error = q.lastError().text(); return false; }

    const QString domNorm     = domaineProjet.trimmed().toLower();
    const QString reqRoleNorm = requiredRole.trimmed().toLower();

    while (q.next()) {
        SmartEmpSuggestion s;
        s.employeeId    = q.value(0).toInt();
        s.fullName      = q.value(1).toString().trimmed();
        s.role          = q.value(2).toString().trimmed();
        s.specialization= q.value(3).toString().trimmed();
        s.activeProjects= q.value(4).toInt();
        const int pubsCount  = q.value(5).toInt();
        const QString qualif = q.value(6).toString().trimmed();
        const QString wMode  = q.value(7).toString().trimmed().toLower();
        if (s.fullName.isEmpty()) s.fullName = QString("Employé #%1").arg(s.employeeId);

        if (s.activeProjects >= maxCapacity) continue;

        const QString specNorm = s.specialization.toLower();
        const QString roleNorm = s.role.toLower();

        int specScore = 40;
        if (!domNorm.isEmpty() && !specNorm.isEmpty()) {
            if (specNorm == domNorm) specScore = 100;
            else if (specNorm.contains(domNorm) || domNorm.contains(specNorm)) specScore = 70;
            else specScore = 25;
        }
        int workloadScore = clampScore(100 - (s.activeProjects * 100 / std::max(1, maxCapacity)));
        int roleMatch = 50;
        if (reqRoleNorm.isEmpty() || reqRoleNorm == "tous") {
            if (roleNorm == "chercheur")  roleMatch = 85;
            else if (roleNorm == "technicien") roleMatch = 75;
            else roleMatch = 0;
        } else {
            roleMatch = (roleNorm == reqRoleNorm) ? 100 : 0;
        }
        int expScore  = clampScore(std::min(10, std::max(0, pubsCount)) * 10);
        int profScore = 0;
        if (!qualif.isEmpty())    profScore += 60;
        if (!specNorm.isEmpty())  profScore += 25;
        if (wMode == "plein")     profScore += 15;
        else if (wMode == "partiel") profScore += 8;
        profScore = clampScore(profScore);

        s.matchPercent = clampScore(
            (specScore * 28 + workloadScore * 30 + roleMatch * 20 + expScore * 17 + profScore * 5) / 100);
        s.explanation = QString("Spec %1% | Charge %2% | Rôle %3% | Exp %4%")
                            .arg(specScore).arg(workloadScore).arg(roleMatch).arg(expScore);
        out.push_back(s);
    }

    std::sort(out.begin(), out.end(), [](const SmartEmpSuggestion& a, const SmartEmpSuggestion& b){
        if (a.matchPercent != b.matchPercent) return a.matchPercent > b.matchPercent;
        if (a.activeProjects != b.activeProjects) return a.activeProjects < b.activeProjects;
        return a.fullName.toLower() < b.fullName.toLower();
    });
    return true;
}

// ===================== SMART ASSIGNMENT ENGINE — EXPÉRIENCE =====================
struct SmartExpSuggestion {
    int     employeeId   = 0;
    QString fullName;
    QString role;
    QString specialization;
    int     activeProjects = 0;   // total workload (projects + experiences via project link)
    int     matchPercent   = 0;
    QString explanation;
};

// Scoring: spécialisation 40% | charge 35% | rôle 25%
static bool loadSmartExpSuggestions(int expId,
                                    const QString& typeExperience,
                                    const QString& requiredRole,
                                    QVector<SmartExpSuggestion>& out,
                                    QString* error = nullptr)
{
    out.clear();
    if (expId <= 0) { if (error) *error = "Expérience invalide."; return false; }

    // All active employees not already linked to this experience's project
    QSqlQuery q;
    q.prepare(
        "SELECT e.\"employee_id\", "
        "       NVL(e.\"FULL_NAME\", TRIM(e.\"prenom\" || ' ' || e.\"nom\")) AS full_name, "
        "       NVL(e.\"ROLE\", 'Chercheur') AS role_name, "
        "       NVL(e.\"specialization\", '') AS specialization, "
        "       (SELECT COUNT(*) FROM \"Associer\" a2 WHERE a2.\"employee_id\" = e.\"employee_id\") AS workload, "
        "       NVL(e.\"TEMPS_TRAVAIL\", '') AS temps_travail, "
        "       NVL(e.\"QUALIFICATION\", '') AS qualification "
        "FROM \"Employés\" e "
        "WHERE e.\"ACTIVE\" = 'O' "
        "ORDER BY e.\"nom\", e.\"prenom\""
    );
    if (!q.exec()) { if (error) *error = q.lastError().text(); return false; }

    const QString typeNorm    = typeExperience.trimmed().toLower();
    const QString reqRoleNorm = requiredRole.trimmed().toLower();
    const int maxWorkload     = 5;

    while (q.next()) {
        SmartExpSuggestion s;
        s.employeeId    = q.value(0).toInt();
        s.fullName      = q.value(1).toString().trimmed();
        s.role          = q.value(2).toString().trimmed();
        s.specialization= q.value(3).toString().trimmed();
        s.activeProjects= q.value(4).toInt();
        const QString wMode  = q.value(5).toString().trimmed().toLower();
        const QString qualif = q.value(6).toString().trimmed();
        if (s.fullName.isEmpty()) s.fullName = QString("Employé #%1").arg(s.employeeId);

        // Skip overloaded employees
        if (s.activeProjects >= maxWorkload) continue;

        const QString specNorm = s.specialization.toLower();
        const QString roleNorm = s.role.toLower();

        // ① Spécialisation vs Type_Experience (40%)
        int specScore = 40;
        if (!typeNorm.isEmpty() && !specNorm.isEmpty()) {
            if (specNorm == typeNorm)                                    specScore = 100;
            else if (specNorm.contains(typeNorm)||typeNorm.contains(specNorm)) specScore = 70;
            else                                                         specScore = 20;
        }

        // ② Charge de travail (35%) — moins de projets = mieux classé
        int workScore = clampScore(100 - (s.activeProjects * 100 / std::max(1, maxWorkload)));
        if (wMode == "plein")        workScore = clampScore(workScore - 10);
        else if (wMode == "partiel") workScore = clampScore(workScore + 10);

        // ③ Rôle (25%)
        int roleScore = 50;
        if (reqRoleNorm.isEmpty() || reqRoleNorm == "tous") {
            if (roleNorm == "chercheur")    roleScore = 90;
            else if (roleNorm == "technicien")  roleScore = 75;
            else if (roleNorm == "responsable") roleScore = 70;
        } else {
            roleScore = (roleNorm == reqRoleNorm) ? 100 : 20;
        }

        s.matchPercent = clampScore(
            (specScore * 40 + workScore * 35 + roleScore * 25) / 100);
        s.explanation = QString("Spec %1% · Charge %2% · Rôle %3%")
                            .arg(specScore).arg(workScore).arg(roleScore);
        out.push_back(s);
    }

    std::sort(out.begin(), out.end(), [](const SmartExpSuggestion& a, const SmartExpSuggestion& b){
        if (a.matchPercent != b.matchPercent) return a.matchPercent > b.matchPercent;
        return a.activeProjects < b.activeProjects;
    });
    return true;
}

static bool g_darkThemeEnabled = false;
static std::function<void(bool)> g_applyThemeFn;
static QList<QPushButton*> g_themeButtons;

static QIcon themeToggleIcon(bool dark)
{
    QPixmap px(20, 20);
    px.fill(Qt::transparent);

    QPainter p(&px);
    p.setRenderHint(QPainter::Antialiasing, true);

    if (!dark) {
        p.setPen(QPen(QColor("#F5A623"), 1.8, Qt::SolidLine, Qt::RoundCap));
        const QPointF c(10, 10);
        for (int i = 0; i < 8; ++i) {
            const double a = i * (M_PI / 4.0);
            QPointF a1(c.x() + 5.8 * std::cos(a), c.y() + 5.8 * std::sin(a));
            QPointF a2(c.x() + 8.6 * std::cos(a), c.y() + 8.6 * std::sin(a));
            p.drawLine(a1, a2);
        }
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#F9C74F"));
        p.drawEllipse(QPointF(10, 10), 4.4, 4.4);
    } else {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#E9EEF7"));
        p.drawEllipse(QPointF(10, 10), 6.2, 6.2);
        p.setBrush(QColor("#5C6A7B"));
        p.drawEllipse(QPointF(12.5, 8.5), 5.6, 5.6);
    }

    return QIcon(px);
}

static void syncThemeToggleButtons()
{
    for (QPushButton* btn : g_themeButtons) {
        if (!btn) continue;
        btn->setText(QString());
        btn->setIcon(themeToggleIcon(g_darkThemeEnabled));
        btn->setIconSize(QSize(20, 20));
        btn->setChecked(g_darkThemeEnabled);
        btn->setToolTip(g_darkThemeEnabled ? "Passer en mode clair" : "Passer en mode sombre");
    }
}

static QString findTraceLogPath()
{
    const QString candidate1 = QDir::current().filePath("tracabilite_smartvision.txt");
    if (QFile::exists(candidate1)) return candidate1;

    const QString candidate2 = QCoreApplication::applicationDirPath() + "/tracabilite_smartvision.txt";
    if (QFile::exists(candidate2)) return candidate2;

    return QString();
}

static bool copyTraceLogFile(const QString& destination, QString* outError)
{
    const QString sourcePath = findTraceLogPath();
    if (sourcePath.isEmpty()) {
        if (outError) *outError = "Fichier de traçabilité introuvable.";
        return false;
    }

    QFile source(sourcePath);
    if (!source.open(QIODevice::ReadOnly)) {
        if (outError) *outError = "Impossible d'ouvrir le fichier source.";
        return false;
    }

    QFile dest(destination);
    if (!dest.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (outError) *outError = "Impossible d'écrire le fichier de destination.";
        return false;
    }

    const qint64 bytes = dest.write(source.readAll());
    if (bytes < 0) {
        if (outError) *outError = "Erreur lors de la copie du fichier.";
        return false;
    }

    return true;
}

static QString smtpLogPath()
{
    const QString currentPath = QDir::current().filePath("smtp_mail.log");
    const QFileInfo currentInfo(currentPath);
    if (currentInfo.dir().exists()) return currentPath;
    return QCoreApplication::applicationDirPath() + "/smtp_mail.log";
}

static void appendSmtpLog(const QString& message)
{
    QFile file(smtpLogPath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        return;

    QTextStream out(&file);
    out << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss")
        << " " << message << "\n";
}

static QString maskSmtpIdentity(const QString& value)
{
    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty()) return QString("(vide)");
    const int atPos = trimmed.indexOf('@');
    if (atPos <= 1) return QString("***%1").arg(trimmed.mid(atPos));
    return trimmed.left(2) + "***" + trimmed.mid(atPos);
}

static QString normalizeSmtpUiReason(const QString& reason)
{
    const QString r = reason.trimmed();
    if (r.isEmpty()) return "Erreur SMTP inconnue.";

    const bool authRejected = r.contains("535", Qt::CaseInsensitive)
                           || r.contains("username and password not accepted", Qt::CaseInsensitive)
                           || r.contains("application-specific password", Qt::CaseInsensitive);
    if (authRejected) {
        return "Authentification SMTP refusée (Gmail). Utilisez un mot de passe d'application Google (16 caractères), pas le mot de passe normal.";
    }
    return r;
}

static bool isPlaceholderSmtpValue(const QString& value)
{
    const QString v = value.trimmed().toLower();
    return v.isEmpty()
           || v.contains("votre.adresse.gmail")
           || v.contains("app-password")
           || v.contains("example.com")
           || v == "xxxx xxxx xxxx xxxx"
           || v == "xxxxxxxxxxxxxxxx";
}

static bool configureEmailSenderFromLocalConfig(QString* smtpConfigError)
{
    QString smtpHost   = QString::fromLocal8Bit(qgetenv("SMTP_HOST")).trimmed();
    QString smtpPortSt = QString::fromLocal8Bit(qgetenv("SMTP_PORT")).trimmed();
    QString smtpUser   = QString::fromLocal8Bit(qgetenv("SMTP_USERNAME")).trimmed();
    QString smtpPass   = QString::fromLocal8Bit(qgetenv("SMTP_PASSWORD")).trimmed();
    QString senderName = QString::fromLocal8Bit(qgetenv("SMTP_SENDER_NAME")).trimmed();
    QString configSource = "environment";
    QString foundIniPath;
    bool iniPresent = false;

    QStringList searchDirs;
    QSet<QString> seenDirs;
    auto addSearchDir = [&](const QString& dirPath) {
        if (dirPath.trimmed().isEmpty()) return;
        const QString clean = QDir(dirPath).absolutePath();
        if (seenDirs.contains(clean)) return;
        seenDirs.insert(clean);
        searchDirs << clean;
    };

    const QString appDirPath = QCoreApplication::applicationDirPath();
    const QString curDirPath = QDir::currentPath();
    addSearchDir(appDirPath);
    addSearchDir(curDirPath);

    // Also inspect parent folders because the executable often runs from build/.../debug
    // while smtp_mail.ini is kept at the project root.
    {
        QDir d(appDirPath);
        for (int i = 0; i < 12; ++i) {
            if (!d.cdUp()) break;
            addSearchDir(d.absolutePath());
        }
    }
    {
        QDir d(curDirPath);
        for (int i = 0; i < 12; ++i) {
            if (!d.cdUp()) break;
            addSearchDir(d.absolutePath());
        }
    }

    for (const QString& dir : searchDirs) {
        const QString ini = QDir(dir).filePath("smtp_mail.ini");
        if (!QFileInfo::exists(ini)) continue;
        iniPresent = true;
        foundIniPath = ini;
        QSettings cfg(ini, QSettings::IniFormat);
        if (smtpHost.isEmpty())   smtpHost   = cfg.value("smtp/host", "smtp.gmail.com").toString().trimmed();
        if (smtpPortSt.isEmpty()) smtpPortSt = cfg.value("smtp/port", 587).toString().trimmed();
        if (smtpUser.isEmpty())   smtpUser   = cfg.value("smtp/username").toString().trimmed();
        if (smtpPass.isEmpty())   smtpPass   = cfg.value("smtp/password").toString().trimmed();
        if (senderName.isEmpty()) senderName = cfg.value("smtp/sender_name", "SmartVision BioSimple").toString().trimmed();
        configSource = QString("ini:%1").arg(ini);
        break;
    }

    if (!iniPresent) {
        appendSmtpLog(QString("[CONFIG] smtp_mail.ini not found searched=%1")
                          .arg(searchDirs.join(";")));
    }

    if (smtpHost.isEmpty()) smtpHost = "smtp.gmail.com";
    if (smtpPortSt.isEmpty()) smtpPortSt = "587";
    if (senderName.isEmpty()) senderName = "SmartVision BioSimple";

    smtpUser = smtpUser.trimmed();
    smtpPass = smtpPass.trimmed();

    // Gmail app-passwords are often pasted with spaces or quotes.
    if (smtpHost.contains("gmail", Qt::CaseInsensitive)) {
        smtpUser.remove('"');
        smtpPass.remove('"');
        smtpPass.remove(' ');
    }

    bool portOk = false;
    int smtpPort = smtpPortSt.toInt(&portOk);
    if (!portOk || smtpPort <= 0 || smtpPort > 65535) {
        if (smtpConfigError) {
            *smtpConfigError = QString("port SMTP invalide : \"%1\"")
                                   .arg(smtpPortSt.isEmpty() ? "vide" : smtpPortSt);
        }
        EmailSender::instance()->configure(QString(), 0, QString(), QString(), QString());
        appendSmtpLog(QString("[CONFIG] invalid port source=%1 host=%2 port=%3 user=%4")
                          .arg(configSource, smtpHost, smtpPortSt, maskSmtpIdentity(smtpUser)));
        return false;
    }

    if (smtpHost.trimmed().isEmpty()) {
        if (smtpConfigError) *smtpConfigError = "serveur SMTP manquant";
        EmailSender::instance()->configure(QString(), 0, QString(), QString(), QString());
        appendSmtpLog(QString("[CONFIG] missing host source=%1 user=%2")
                          .arg(configSource, maskSmtpIdentity(smtpUser)));
        return false;
    }

    if (isPlaceholderSmtpValue(smtpUser) || isPlaceholderSmtpValue(smtpPass)) {
        if (smtpConfigError) {
            if (!iniPresent && QString::fromLocal8Bit(qgetenv("SMTP_USERNAME")).trimmed().isEmpty()) {
                *smtpConfigError = "fichier smtp_mail.ini introuvable et variables SMTP_* absentes";
            } else if (iniPresent && !foundIniPath.isEmpty()) {
                *smtpConfigError = QString("identifiants SMTP manquants ou placeholders dans %1")
                                       .arg(QFileInfo(foundIniPath).fileName());
            } else {
                *smtpConfigError = "identifiants SMTP manquants dans les variables d'environnement";
            }
        }
        EmailSender::instance()->configure(QString(), 0, QString(), QString(), QString());
        appendSmtpLog(QString("[CONFIG] missing credentials source=%1 ini_present=%2 host=%3 port=%4 user=%5")
                          .arg(configSource,
                               iniPresent ? "yes" : "no",
                               smtpHost,
                               QString::number(smtpPort),
                               maskSmtpIdentity(smtpUser)));
        return false;
    }

    EmailSender::instance()->configure(smtpHost, smtpPort, smtpUser, smtpPass, senderName);
    const bool configured = EmailSender::instance()->isConfigured();
    if (smtpConfigError) {
        *smtpConfigError = configured
                           ? QString()
                           : QString("configuration SMTP incomplète (%1:%2)").arg(smtpHost).arg(smtpPort);
    }
    appendSmtpLog(QString("[CONFIG] loaded source=%1 host=%2 port=%3 user=%4 configured=%5")
                      .arg(configSource,
                           smtpHost,
                           QString::number(smtpPort),
                           maskSmtpIdentity(smtpUser),
                           configured ? "yes" : "no"));
    return configured;
}

// ── Animated toast notification ─────────────────────────────────────────────
static void showToast(QWidget* parent, const QString& msg, bool success = true)
{
    QWidget* toast = new QWidget(parent,
        Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    toast->setAttribute(Qt::WA_TranslucentBackground);
    toast->setAttribute(Qt::WA_DeleteOnClose);

    // ── Card container ──
    QWidget* card = new QWidget(toast);
    card->setObjectName("toastCard");
    card->setStyleSheet(QString(
        "QWidget#toastCard{"
        " background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "   stop:0 %1, stop:1 %2);"
        " border-radius: 14px;"
        " border: 1.5px solid rgba(255,255,255,0.18);"
        "}").arg(success ? "#0A5F58" : "#7B1D2A",
                 success ? "#12443B" : "#5C1020"));

    QHBoxLayout* lay = new QHBoxLayout(card);
    lay->setContentsMargins(18, 14, 22, 14);
    lay->setSpacing(14);

    // Icon circle
    QLabel* icon = new QLabel(success ? "✓" : "✕");
    icon->setFixedSize(38, 38);
    icon->setAlignment(Qt::AlignCenter);
    icon->setStyleSheet(QString(
        "color:white; font-size:17px; font-weight:900;"
        "background: rgba(255,255,255,0.20);"
        "border-radius:19px; border:1.5px solid rgba(255,255,255,0.35);"));

    // Text
    QLabel* lbl = new QLabel(msg);
    lbl->setWordWrap(true);
    lbl->setMaximumWidth(340);
    lbl->setStyleSheet(
        "color:white; font-size:13px; font-weight:600;"
        "background:transparent; border:none;");

    lay->addWidget(icon);
    lay->addWidget(lbl);

    card->adjustSize();

    // Outer toast is same size as card
    QVBoxLayout* outerL = new QVBoxLayout(toast);
    outerL->setContentsMargins(0,0,0,0);
    outerL->addWidget(card);
    toast->adjustSize();

    // Position: centred, starts 40px above final position
    QRect pr  = parent->geometry();
    int   tx  = pr.left() + (pr.width() - toast->width()) / 2;
    int   tyEnd = pr.top() + 88;
    int   tyStart = tyEnd - 40;

    toast->move(tx, tyStart);
    toast->show();
    toast->raise();

    // ── Slide-down + fade-in ──
    QGraphicsOpacityEffect* eff = new QGraphicsOpacityEffect(toast);
    toast->setGraphicsEffect(eff);
    eff->setOpacity(0.0);

    QPropertyAnimation* fadeIn = new QPropertyAnimation(eff, "opacity", toast);
    fadeIn->setDuration(280);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::OutCubic);
    fadeIn->start(QAbstractAnimation::DeleteWhenStopped);

    QPropertyAnimation* slideIn = new QPropertyAnimation(toast, "pos", toast);
    slideIn->setDuration(280);
    slideIn->setStartValue(QPoint(tx, tyStart));
    slideIn->setEndValue(QPoint(tx, tyEnd));
    slideIn->setEasingCurve(QEasingCurve::OutCubic);
    slideIn->start(QAbstractAnimation::DeleteWhenStopped);

    // ── Auto-close: slide-up + fade-out after 2.5 s ──
    QTimer::singleShot(2500, toast, [toast, eff, tx, tyEnd]{
        QPropertyAnimation* fadeOut = new QPropertyAnimation(eff, "opacity", toast);
        fadeOut->setDuration(400);
        fadeOut->setStartValue(1.0);
        fadeOut->setEndValue(0.0);
        fadeOut->setEasingCurve(QEasingCurve::InCubic);
        QObject::connect(fadeOut, &QPropertyAnimation::finished, toast, &QWidget::close);
        fadeOut->start(QAbstractAnimation::DeleteWhenStopped);

        QPropertyAnimation* slideOut = new QPropertyAnimation(toast, "pos", toast);
        slideOut->setDuration(400);
        slideOut->setStartValue(QPoint(tx, tyEnd));
        slideOut->setEndValue(QPoint(tx, tyEnd - 30));
        slideOut->setEasingCurve(QEasingCurve::InCubic);
        slideOut->start(QAbstractAnimation::DeleteWhenStopped);
    });
}

// ===================== STACK INDEX =====================
// Login page
static const int LOGIN = 0;

// BioSimple (5 pages)
static const int BIO_LIST  = 1;
static const int BIO_FORM  = 2;
static const int BIO_LOC   = 3;
static const int BIO_RACK  = 4;
static const int BIO_STATS = 5;

// Gestion Projet (3 widgets/pages)
static const int PROJ_LIST  = 6; // Widget 1
static const int PROJ_FORM  = 7; // Widget 2
static const int PROJ_STATS = 8; // Widget 3

// Expériences / Protocoles (3 widgets/pages)
static const int EXP_LIST  = 9;  // Widget 1
static const int EXP_FORM  = 10;  // Widget 2
static const int EXP_STATS = 11; // Widget 3

// Publications (3 pages)
static const int PUB_LIST    = 12;  // Page 1 : Liste / Gestion
static const int PUB_FORM    = 13;  // Page 2 : Ajouter / Modifier
static const int PUB_STATS   = 14;  // Page 3 : Statistiques

// Équipements (4 pages)
static const int EQUIP_LIST    = 15; // Page 1 : Liste / Gestion
static const int EQUIP_FORM    = 16; // Page 2 : Ajouter / Modifier
static const int EQUIP_LOC     = 17; // Page 3 : Localisation
static const int EQUIP_DETAILS = 18; // Page 4 : Détails

// Employés (5 pages)
static const int EMP_LIST    = 19; // Page 1 : Liste / Gestion
static const int EMP_FORM    = 20; // Page 2 : Créer / Modifier
static const int EMP_AFF     = 21; // Page 3 : Affectation Intelligente — Projet
static const int EMP_AFF_EXP = 22; // Page 3b: Affectation Intelligente — Expérience
static const int EMP_AVAIL   = 23; // Page 4 : Disponibilités
static const int EMP_STATS   = 24; // Page 5 : Statistiques

// Détails (pages additionnelles)
static const int PUB_DETAILS  = 25;
static const int EXP_DETAILS  = 26;
static const int PROJ_DETAILS = 27;
static const int PROJ_ADVANCED = 28;

// ===================== UI responsive margin =====================
// Returns adaptive margins based on window width.
static int uiMargin(QWidget* w)
{
    int W = w->width();
    if (W < 1100) return 6;
    if (W < 1400) return 10;
    return 14;
}


// ===================== Helpers =====================
// Creates a rounded card container with a soft drop shadow.
static QFrame* makeCard(QWidget* parent=nullptr)
{
    QFrame* card = new QFrame(parent);
    card->setObjectName("card");
    card->setFrameShape(QFrame::NoFrame);
    card->setStyleSheet("#card { background: rgba(238,242,241,0.78); border: 1px solid rgba(0,0,0,0.10); border-radius: 12px; }");

    auto* shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(25);
    shadow->setOffset(0, 8);
    shadow->setColor(QColor(0,0,0,45));
    card->setGraphicsEffect(shadow);
    return card;
}

// Creates a light inset panel used for grouped inputs.
static QFrame* softBox(QWidget* parent=nullptr)
{
    QFrame* f = new QFrame(parent);
    f->setStyleSheet(QString("QFrame{ background:%1; border: 1px solid %2; border-radius: 12px; }")
                         .arg(C_PANEL_IN, C_PANEL_BR));
    return f;
}

// Reuses a soft shadow to reinforce the glass-card look on translucent panels.
static void applyGlassShadow(QWidget* widget,
                             const QColor& color = QColor(0, 0, 0, 60),
                             qreal blurRadius = 34.0,
                             const QPointF& offset = QPointF(0.0, 10.0))
{
    if (!widget) return;

    auto* shadow = qobject_cast<QGraphicsDropShadowEffect*>(widget->graphicsEffect());
    if (!shadow) {
        shadow = new QGraphicsDropShadowEffect(widget);
        widget->setGraphicsEffect(shadow);
    }

    shadow->setBlurRadius(blurRadius);
    shadow->setOffset(offset);
    shadow->setColor(color);
}

// Builds a top-bar icon button with a standard Qt icon.
static QToolButton* topIconBtn(QStyle* st, QStyle::StandardPixmap sp, const QString& tooltip)
{
    QToolButton* b = new QToolButton;
    b->setAutoRaise(true);
    b->setIcon(st->standardIcon(sp));
    b->setToolTip(tooltip);
    b->setCursor(Qt::PointingHandCursor);
    b->setStyleSheet(R"(
        QToolButton { color: white; padding: 6px; border-radius: 10px; }
        QToolButton:hover { background: rgba(255,255,255,0.10); }
    )");
    return b;
}

// Creates a styled action button with optional icon and enabled state.
static QPushButton* actionBtn(const QString& text, const QString& bg, const QString& fg, const QIcon& icon, bool enabled=true)
{
    QPushButton* b = new QPushButton(icon, "  " + text);
    b->setEnabled(enabled);
    b->setCursor(Qt::PointingHandCursor);
    b->setStyleSheet(QString(R"(
        QPushButton{
            background:%1; color:%2;
            border:1px solid rgba(0,0,0,0.12);
            border-radius:10px;
            padding:10px 18px;
            font-weight:800;
        }
        QPushButton:disabled{
            background: rgba(200,200,200,0.55);
            color: rgba(90,90,90,0.55);
        }
        QPushButton:hover{ background: rgba(255,255,255,0.70); }
    )").arg(bg, fg));
    return b;
}

// Small square button used for compact action shortcuts.
static QToolButton* tinySquareBtn(const QIcon& icon)
{
    QToolButton* b = new QToolButton;
    b->setIcon(icon);
    b->setCursor(Qt::PointingHandCursor);
    b->setStyleSheet(R"(
        QToolButton{
            background: rgba(255,255,255,0.55);
            border: 1px solid rgba(0,0,0,0.12);
            border-radius: 10px;
            padding: 10px;
        }
        QToolButton:hover{ background: rgba(255,255,255,0.75); }
    )");
    return b;
}

// ===================== BUTTON COLOR ANIMATOR (global event filter) =====================
// On hover: smoothly tints the button with a bright accent color.
// On leave: removes the tint smoothly.
class ButtonAnimator : public QObject {
public:
    explicit ButtonAnimator(QObject* parent = nullptr) : QObject(parent) {}

    bool eventFilter(QObject* obj, QEvent* ev) override {
        auto* btn = qobject_cast<QPushButton*>(obj);
        if (!btn) return false;

        if (ev->type() == QEvent::Enter) {
            // Kill any running animation on this button
            if (btn->graphicsEffect()) btn->setGraphicsEffect(nullptr);
            auto* eff = new QGraphicsColorizeEffect(btn);
            eff->setColor(QColor(72, 210, 190));  // bright teal accent
            eff->setStrength(0.0);
            btn->setGraphicsEffect(eff);
            auto* a = new QPropertyAnimation(eff, "strength", btn);
            a->setDuration(220);
            a->setStartValue(0.0);
            a->setEndValue(0.50);
            a->setEasingCurve(QEasingCurve::OutCubic);
            a->start(QAbstractAnimation::DeleteWhenStopped);
        }
        else if (ev->type() == QEvent::Leave) {
            if (auto* eff = qobject_cast<QGraphicsColorizeEffect*>(btn->graphicsEffect())) {
                auto* a = new QPropertyAnimation(eff, "strength", btn);
                a->setDuration(200);
                a->setStartValue(eff->strength());
                a->setEndValue(0.0);
                a->setEasingCurve(QEasingCurve::InCubic);
                QObject::connect(a, &QPropertyAnimation::finished, btn,
                    [btn]{ btn->setGraphicsEffect(nullptr); });
                a->start(QAbstractAnimation::DeleteWhenStopped);
            }
        }
        return false;
    }
};

// ===================== ANIMATED SYRINGE BACKGROUND =====================
class SyringeBackground : public QWidget {
public:
    explicit SyringeBackground(QWidget* parent = nullptr) : QWidget(parent) {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAutoFillBackground(false);
        QTimer* t = new QTimer(this);
        QObject::connect(t, &QTimer::timeout, this, [this]{ m_angle += 0.35; update(); });
        t->start(16); // ~60 fps
    }

protected:
    void paintEvent(QPaintEvent*) override {
        if (width() <= 0 || height() <= 0) return;
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        struct S { double xR, yR, size, off, spd, op; };
        static const S items[] = {
            { 0.05, 0.12, 100,   0,  0.28, 0.08 },
            { 0.22, 0.70,  75,  50,  0.18, 0.06 },
            { 0.48, 0.08, 120,  95,  0.22, 0.07 },
            { 0.72, 0.60,  85, 140,  0.32, 0.08 },
            { 0.90, 0.22, 110, 185,  0.14, 0.06 },
            { 0.58, 0.85,  70, 230,  0.38, 0.07 },
            { 0.13, 0.48,  90, 275,  0.20, 0.06 },
            { 0.38, 0.38, 100, 320,  0.28, 0.07 },
            { 0.82, 0.78,  80,  65,  0.24, 0.06 },
            { 0.33, 0.18, 115, 155,  0.16, 0.07 },
            { 0.65, 0.30,  88, 200,  0.30, 0.06 },
            { 0.10, 0.88,  95, 260,  0.20, 0.08 },
        };

        for (const auto& s : items)
            drawSyringe(p, s.xR * width(), s.yR * height(),
                        s.size, s.off + m_angle * s.spd, s.op);
    }

private:
    double m_angle = 0.0;

    void drawSyringe(QPainter& p, double cx, double cy,
                     double sz, double rotDeg, double opacity) {
        p.save();
        p.translate(cx, cy);
        p.rotate(rotDeg);
        p.setOpacity(opacity);

        const double bW = sz * 0.22;  // barrel width
        const double bL = sz * 0.65;  // barrel length
        const double nL = sz * 0.32;  // needle length
        const double hW = sz * 0.07;  // hub width

        // --- barrel ---
        p.setPen(QPen(QColor(10, 95, 88), 1.5));
        p.setBrush(QColor(200, 230, 238, 210));
        p.drawRoundedRect(QRectF(-bL/2, -bW/2, bL, bW), bW/3, bW/3);

        // --- liquid fill (blue) ---
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(90, 160, 215, 170));
        p.drawRoundedRect(QRectF(-bL/2 + 3, -bW/2 + 3, bL * 0.50, bW - 6), 2, 2);

        // --- needle hub ---
        p.setPen(QPen(QColor(120, 150, 162), 1));
        p.setBrush(QColor(175, 200, 208));
        p.drawRect(QRectF(bL/2, -bW/3, hW, bW * 0.67));

        // --- needle (tapered) ---
        QPainterPath nd;
        nd.moveTo(bL/2 + hW,        -sz * 0.026);
        nd.lineTo(bL/2 + hW + nL,    0);
        nd.lineTo(bL/2 + hW,         sz * 0.026);
        nd.closeSubpath();
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(170, 215, 225));
        p.drawPath(nd);

        // --- plunger rod ---
        p.setPen(QPen(QColor(185, 55, 55), 1));
        p.setBrush(QColor(195, 70, 70));
        p.drawRect(QRectF(-bL/2 - sz*0.12, -bW/10, sz*0.12, bW/5));

        // --- plunger head ---
        p.drawRoundedRect(QRectF(-bL/2 - sz*0.145, -bW/2 + 1, sz*0.055, bW - 2), 3, 3);

        // --- graduation marks ---
        p.setPen(QPen(QColor(10, 95, 88, 110), 1.0));
        for (int i = 1; i <= 4; ++i) {
            double x = -bL/2 + bL * i / 5.0;
            p.drawLine(QPointF(x, -bW/2 + 1), QPointF(x, -bW/2 + bW * 0.40));
        }

        p.restore();
    }
};

// ===================== LOGO TRÈS GRAND CENTRÉ =====================
// ===================== CENTERED LOGO CARD =====================
// Builds the centered logo panel with the app name.
static QFrame* makeBigLogoPanel()
{
    // Carte verte arrondie
    QFrame* box = new QFrame;
    box->setObjectName("logoCard");
    box->setFixedSize(360, 180);

    box->setStyleSheet(
        "QFrame#logoCard {"
        " background-color: #12443B;"   // vert foncé
        " border-radius: 28px;"         // coins très arrondis
        "}"
        );

    // Layout vertical centré
    QVBoxLayout* v = new QVBoxLayout(box);
    v->setContentsMargins(20, 15, 20, 15);
    v->setSpacing(8);
    v->setAlignment(Qt::AlignCenter);

    // ===== LOGO PLUS GRAND =====
    QLabel* logo = new QLabel;
    logo->setAlignment(Qt::AlignCenter);

    QPixmap px;
    if (!px.load(":/image/smartvision.png")) {
        if (!px.load(":/smartvision.png")) {
            px.load("resources/smartvision.png");
        }
    }

    if (!px.isNull())
    {
        logo->setPixmap(
            px.scaled(150, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation)
            );
    }
    else
    {
        logo->setText("LOGO");
        logo->setStyleSheet("color:#bbb; font-size:18px;");
    }

    // --- Logo pulse animation (breathing opacity) ---
    QGraphicsOpacityEffect* logoEffect = new QGraphicsOpacityEffect(logo);
    logo->setGraphicsEffect(logoEffect);
    // Logo pulse: 2 cycles then stops (stays at full opacity)
    QPropertyAnimation* logoPulse = new QPropertyAnimation(logoEffect, "opacity", box);
    logoPulse->setDuration(1800);
    logoPulse->setStartValue(0.55);
    logoPulse->setEndValue(1.0);
    logoPulse->setEasingCurve(QEasingCurve::SineCurve);
    logoPulse->setLoopCount(2);
    QObject::connect(logoPulse, &QPropertyAnimation::finished, logo, [logoEffect](){
        logoEffect->setOpacity(1.0);
    });
    logoPulse->start();

    // --- Logo rotation: one full spin (360°) then fixes in place ---
    if (!px.isNull()) {
        double* angle = new double(0.0);
        QTimer* spinTimer = new QTimer(box);
        QObject::connect(spinTimer, &QTimer::timeout, logo, [logo, px, angle, spinTimer](){
            *angle += 1.2;
            if (*angle >= 360.0) {
                // Snap back to upright and stop
                logo->setPixmap(px.scaled(150, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                spinTimer->stop();
                delete angle;
                return;
            }
            QTransform tr;
            tr.rotate(*angle);
            logo->setPixmap(px.scaled(150, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation)
                                .transformed(tr, Qt::SmoothTransformation));
        });
        spinTimer->start(16); // 60 fps, completes in ~5 s
    }

    v->addWidget(logo, 0, Qt::AlignCenter);

    // ===== TEXTE SmartVision CENTRÉ =====
    QLabel* title = new QLabel("SmartVision");
    title->setAlignment(Qt::AlignCenter);

    QFont ft;
    ft.setPointSize(16);
    ft.setBold(true);
    title->setFont(ft);

    title->setStyleSheet("color: #C6B29A;"); // beige

    // --- Title color animation (beige <-> white) ---
    QGraphicsOpacityEffect* titleEffect = new QGraphicsOpacityEffect(title);
    title->setGraphicsEffect(titleEffect);
    // Title pulse: 2 cycles then stays visible
    QPropertyAnimation* titlePulse = new QPropertyAnimation(titleEffect, "opacity", box);
    titlePulse->setDuration(2400);
    titlePulse->setStartValue(0.3);
    titlePulse->setEndValue(1.0);
    titlePulse->setEasingCurve(QEasingCurve::SineCurve);
    titlePulse->setLoopCount(2);
    QObject::connect(titlePulse, &QPropertyAnimation::finished, title, [titleEffect](){
        titleEffect->setOpacity(1.0);
    });
    titlePulse->start();

    v->addWidget(title, 0, Qt::AlignCenter);

    return box;
}


// ===================== MODULES BAR =====================
enum class ModuleTab {
    Employee = 0,
    Publication,
    BioSimple,
    Equipement,
    ExperiencesProtocoles,
    GestionProjet
};

// Creates a pill-style module selector button.
static QPushButton* modulePill(const QString& text, bool selected)
{
    QPushButton* b = new QPushButton(text);
    b->setCursor(Qt::PointingHandCursor);
    b->setCheckable(true);
    b->setChecked(selected);

    b->setStyleSheet(QString(R"(
        QPushButton{
            background: rgba(255,255,255,0.70);
            border: 1px solid rgba(0,0,0,0.12);
            border-radius: 18px;
            padding: 8px 16px;
            font-weight: 900;
            color: rgba(0,0,0,0.60);
        }
        QPushButton:hover{
            background: rgba(255,255,255,0.82);
        }
        QPushButton:checked{
            background: rgba(10,95,88,0.75);
            border: 1px solid rgba(0,0,0,0.18);
            color: rgba(255,255,255,0.92);
        }
    )"));
    return b;
}

struct ModulesBar {
    QFrame* bar = nullptr;
    QPushButton* bEmployee = nullptr;
    QPushButton* bPublication = nullptr;
    QPushButton* bBioSimple = nullptr;
    QPushButton* bEquipement = nullptr;
    QPushButton* bExp = nullptr;
    QPushButton* bProjet = nullptr;
    QPushButton* bTheme = nullptr;
    QPushButton* bVoice = nullptr;
    QPushButton* bTrace = nullptr;
    QPushButton* bLogout = nullptr;
};

static QVector<ModulesBar>& registeredModuleBars()
{
    static QVector<ModulesBar> bars;
    return bars;
}

static void setModuleAccess(QPushButton* button, bool allowed)
{
    if (!button) return;
    button->setVisible(allowed);
    button->setEnabled(allowed);
    if (!allowed) button->setChecked(false);
}

static bool isModuleAllowedForRole(const QString& role, const QString& module)
{
    if (module.isEmpty()) return true;
    if (module == "congelateur") return true;

    if (role == "Responsable") return true;
    if (module == "employee")    return role == "RH";
    if (module == "publication") return role == "Chercheur";
    if (module == "experience")  return role == "Chercheur";
    if (module == "projet")      return role == "Chercheur";
    if (module == "biosample")   return role == "Technicien";
    if (module == "equipement")  return role == "Technicien";

    return false;
}

static void applyRoleToModulesBar(const ModulesBar& bar, const QString& role)
{
    setModuleAccess(bar.bEmployee,    isModuleAllowedForRole(role, "employee"));
    setModuleAccess(bar.bPublication, isModuleAllowedForRole(role, "publication"));
    setModuleAccess(bar.bBioSimple,   isModuleAllowedForRole(role, "biosample"));
    setModuleAccess(bar.bEquipement,  isModuleAllowedForRole(role, "equipement"));
    setModuleAccess(bar.bExp,         isModuleAllowedForRole(role, "experience"));
    setModuleAccess(bar.bProjet,      isModuleAllowedForRole(role, "projet"));
}

static void clearModuleSelection(const ModulesBar& bar)
{
    if (bar.bEmployee)    bar.bEmployee->setChecked(false);
    if (bar.bPublication) bar.bPublication->setChecked(false);
    if (bar.bBioSimple)   bar.bBioSimple->setChecked(false);
    if (bar.bEquipement)  bar.bEquipement->setChecked(false);
    if (bar.bExp)         bar.bExp->setChecked(false);
    if (bar.bProjet)      bar.bProjet->setChecked(false);
}

static void syncModuleSelection(ModuleTab activeTab)
{
    for (const ModulesBar& bar : registeredModuleBars()) {
        if (bar.bEmployee)    bar.bEmployee->setChecked(activeTab == ModuleTab::Employee && !bar.bEmployee->isHidden());
        if (bar.bPublication) bar.bPublication->setChecked(activeTab == ModuleTab::Publication && !bar.bPublication->isHidden());
        if (bar.bBioSimple)   bar.bBioSimple->setChecked(activeTab == ModuleTab::BioSimple && !bar.bBioSimple->isHidden());
        if (bar.bEquipement)  bar.bEquipement->setChecked(activeTab == ModuleTab::Equipement && !bar.bEquipement->isHidden());
        if (bar.bExp)         bar.bExp->setChecked(activeTab == ModuleTab::ExperiencesProtocoles && !bar.bExp->isHidden());
        if (bar.bProjet)      bar.bProjet->setChecked(activeTab == ModuleTab::GestionProjet && !bar.bProjet->isHidden());
    }
}

static void clearAllModuleSelections()
{
    for (const ModulesBar& bar : registeredModuleBars())
        clearModuleSelection(bar);
}

static bool moduleTabForPageIndex(int idx, ModuleTab* outTab)
{
    if (!outTab) return false;

    if (idx == BIO_LIST || (idx > BIO_LIST && idx < PROJ_LIST)) {
        *outTab = ModuleTab::BioSimple;
        return true;
    }
    if (idx == PROJ_LIST || (idx > PROJ_LIST && idx < EXP_LIST)) {
        *outTab = ModuleTab::GestionProjet;
        return true;
    }
    if (idx == EXP_LIST || (idx > EXP_LIST && idx < PUB_LIST)) {
        *outTab = ModuleTab::ExperiencesProtocoles;
        return true;
    }
    if (idx == PUB_LIST || (idx > PUB_LIST && idx < EQUIP_LIST)) {
        *outTab = ModuleTab::Publication;
        return true;
    }
    if (idx == EQUIP_LIST || (idx > EQUIP_LIST && idx < EMP_LIST)) {
        *outTab = ModuleTab::Equipement;
        return true;
    }
    if (idx == PROJ_ADVANCED) {
        *outTab = ModuleTab::GestionProjet;
        return true;
    }
    if (idx >= EMP_LIST) {
        *outTab = ModuleTab::Employee;
        return true;
    }

    return false;
}

static int rootPageForModule(ModuleTab tab)
{
    switch (tab) {
    case ModuleTab::Employee:               return EMP_LIST;
    case ModuleTab::Publication:            return PUB_LIST;
    case ModuleTab::BioSimple:              return BIO_LIST;
    case ModuleTab::Equipement:             return EQUIP_LIST;
    case ModuleTab::ExperiencesProtocoles:  return EXP_LIST;
    case ModuleTab::GestionProjet:          return PROJ_LIST;
    }
    return LOGIN;
}

static QString windowTitleForModule(ModuleTab tab)
{
    switch (tab) {
    case ModuleTab::Employee:              return "Employés";
    case ModuleTab::Publication:           return "Publications";
    case ModuleTab::BioSimple:             return "Échantillons";
    case ModuleTab::Equipement:            return "Équipements";
    case ModuleTab::ExperiencesProtocoles: return "Expériences";
    case ModuleTab::GestionProjet:         return "Projet";
    }
    return "SmartVision";
}

static bool firstVisibleModuleTab(const ModulesBar& bar, ModuleTab* outTab)
{
    if (!outTab) return false;

    if (bar.bEmployee && !bar.bEmployee->isHidden()) {
        *outTab = ModuleTab::Employee;
        return true;
    }
    if (bar.bPublication && !bar.bPublication->isHidden()) {
        *outTab = ModuleTab::Publication;
        return true;
    }
    if (bar.bBioSimple && !bar.bBioSimple->isHidden()) {
        *outTab = ModuleTab::BioSimple;
        return true;
    }
    if (bar.bEquipement && !bar.bEquipement->isHidden()) {
        *outTab = ModuleTab::Equipement;
        return true;
    }
    if (bar.bExp && !bar.bExp->isHidden()) {
        *outTab = ModuleTab::ExperiencesProtocoles;
        return true;
    }
    if (bar.bProjet && !bar.bProjet->isHidden()) {
        *outTab = ModuleTab::GestionProjet;
        return true;
    }

    return false;
}

// Builds the horizontal module bar and returns its buttons.
static ModulesBar makeModulesBar(ModuleTab selected, QWidget* parent=nullptr)
{
    ModulesBar out;

    out.bar = new QFrame(parent);
    out.bar->setFixedHeight(52);
    out.bar->setStyleSheet("background: rgba(255,255,255,0.18); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");

    QHBoxLayout* h = new QHBoxLayout(out.bar);
    h->setContentsMargins(14, 8, 14, 8);
    h->setSpacing(10);

    out.bEmployee    = modulePill("Employee",        selected == ModuleTab::Employee);
    out.bPublication = modulePill("Publication",     selected == ModuleTab::Publication);
    out.bBioSimple   = modulePill("BioSample",       selected == ModuleTab::BioSimple);
    out.bEquipement  = modulePill("Équipement",      selected == ModuleTab::Equipement);
    out.bExp         = modulePill("Expériences", selected == ModuleTab::ExperiencesProtocoles);
    out.bProjet      = modulePill("Projet",  selected == ModuleTab::GestionProjet);

    h->addWidget(out.bEmployee);
    h->addWidget(out.bPublication);
    h->addWidget(out.bBioSimple);
    h->addWidget(out.bEquipement);
    h->addWidget(out.bExp);
    h->addWidget(out.bProjet);
    h->addStretch(1);

    out.bTheme = new QPushButton;
    out.bTheme->setCursor(Qt::PointingHandCursor);
    out.bTheme->setCheckable(true);
    out.bTheme->setChecked(g_darkThemeEnabled);
    out.bTheme->setStyleSheet(QString(R"(
        QPushButton{
            background: rgba(255,255,255,0.76);
            color: rgba(0,0,0,0.80);
            border: 1px solid rgba(0,0,0,0.18);
            border-radius: 16px;
            padding: 0px;
            font-weight: 800;
            min-width: 32px;
            max-width: 32px;
            min-height: 32px;
            max-height: 32px;
        }
        QPushButton:hover{ background: rgba(255,255,255,0.90); }
        QPushButton:checked{
            background: rgba(10,95,88,0.78);
            color: rgba(255,255,255,0.95);
        }
    )"));
    out.bTheme->setIcon(themeToggleIcon(g_darkThemeEnabled));
    out.bTheme->setIconSize(QSize(20, 20));
    g_themeButtons.push_back(out.bTheme);
    h->addWidget(out.bTheme);

    out.bVoice = new QPushButton("🎙");
    out.bVoice->setCursor(Qt::PointingHandCursor);
    out.bVoice->setCheckable(true);
    out.bVoice->setToolTip("Commandes Vocales");
    out.bVoice->setStyleSheet(R"(
        QPushButton{
            background: rgba(99,102,241,0.25);
            color: white;
            border: 1px solid rgba(99,102,241,0.40);
            border-radius: 16px;
            font-size: 15px;
            min-width: 32px; max-width: 32px;
            min-height: 32px; max-height: 32px;
        }
        QPushButton:hover{ background: rgba(99,102,241,0.50); }
        QPushButton:checked{ background: rgba(99,102,241,0.80); border-color: rgba(99,102,241,0.90); }
    )");
    h->addWidget(out.bVoice);

    out.bTrace = new QPushButton("Tracabilité");
    out.bTrace->setCursor(Qt::PointingHandCursor);
    out.bTrace->setToolTip("Exporter le fichier de traçabilité");
    out.bTrace->setStyleSheet(QString(R"(
        QPushButton{
            background: rgba(255,255,255,0.82);
            color: rgba(0,0,0,0.78);
            border: 1px solid rgba(0,0,0,0.18);
            border-radius: 16px;
            padding: 0px 12px;
            font-weight: 700;
            font-size: 12px;
            min-height: 32px;
            max-height: 32px;
        }
        QPushButton:hover{ background: rgba(255,255,255,0.98); }
    )"));
    out.bTrace->hide();

    out.bLogout = new QPushButton("Déconnexion");
    out.bLogout->setCursor(Qt::PointingHandCursor);
    out.bLogout->setStyleSheet(QString(R"(
        QPushButton{
            background: rgba(139, 47, 60, 0.80);
            color: white;
            border: 1px solid rgba(0,0,0,0.18);
            border-radius: 10px;
            padding: 8px 16px;
            font-weight: 800;
            font-size: 12px;
        }
        QPushButton:hover{ background: rgba(139, 47, 60, 0.95); }
    )"));
    out.bLogout->setFixedSize(120, 34);
    out.bLogout->setVisible(true);
    h->addWidget(out.bLogout);

    registeredModuleBars().push_back(out);
    return out;
}

// ===================== TOPBAR (sans logo) =====================
// Builds the top bar with title and window controls.
static QFrame* makeTopBarNoLogo(QStyle* st, const QString& titleText, QWidget* parent=nullptr)
{
    QFrame* top = new QFrame(parent);
    top->setFixedHeight(46);
    top->setStyleSheet(QString("background:%1; border-radius: 18px;").arg(C_TOPBAR));

    QHBoxLayout* L = new QHBoxLayout(top);
    L->setContentsMargins(16,8,16,8);
    L->setSpacing(8);

    QLabel* title = new QLabel(titleText);
    title->setObjectName("topBarTitle");
    QFont f = title->font(); f.setPointSize(14); f.setBold(true);
    title->setFont(f);
    title->setStyleSheet("color: rgba(255,255,255,0.90);");

    L->addStretch(1);
    L->addWidget(title, 0, Qt::AlignCenter);
    L->addStretch(1);

    QWidget* icons = new QWidget;
    QHBoxLayout* icL = new QHBoxLayout(icons);
    icL->setContentsMargins(0,0,0,0);
    icL->setSpacing(4);

    QToolButton* bHome = topIconBtn(st, QStyle::SP_DirHomeIcon, "Accueil");
    QToolButton* bInfo = topIconBtn(st, QStyle::SP_MessageBoxInformation, "Informations");
    QToolButton* bClose= topIconBtn(st, QStyle::SP_TitleBarCloseButton, "Fermer");

    QObject::connect(bClose, &QToolButton::clicked, top, [=](){
        QWidget* w = top->window();
        if (w) w->close();
    });

    icL->addWidget(bHome);
    icL->addWidget(bInfo);
    icL->addWidget(bClose);

    L->addWidget(icons, 0, Qt::AlignRight);
    return top;
}

// ===================== HEADER BLOCK (logo + modules + topbar) =====================
// Assembles logo, module bar, and topbar into a header block.
static QWidget* makeHeaderBlock(QStyle* st,
                                const QString& pageTitle,
                                ModuleTab selectedModule,
                                ModulesBar* outBar=nullptr)
{
    QWidget* wrap = new QWidget;
    QHBoxLayout* H = new QHBoxLayout(wrap);
    H->setContentsMargins(0,0,0,0);
    H->setSpacing(14);

    QFrame* logo = makeBigLogoPanel();

    QWidget* right = new QWidget;
    QVBoxLayout* R = new QVBoxLayout(right);
    R->setContentsMargins(0,0,0,0);
    R->setSpacing(10);

    ModulesBar bar = makeModulesBar(selectedModule);
    if (outBar) *outBar = bar;

    QFrame* topbar  = makeTopBarNoLogo(st, pageTitle);

    R->addWidget(bar.bar);
    R->addWidget(topbar);

    H->addWidget(logo, 0, Qt::AlignTop);
    H->addWidget(right, 1);

    return wrap;
}

// ===================== Connexion modules (BioSimple / Gestion Projet) =====================
// Wires module buttons to switch the stacked pages and sync global bar.
static ModulesBar* g_globalBar = nullptr;  // Global reference to modules bar
static VoiceCommand* g_voiceCmd = nullptr; // Global reference to voice command widget

static void connectModulesSwitch(MainWindow* self, QStackedWidget* stack, ModulesBar mb)
{
    if (!g_globalBar || !self || !stack) return;

    // Modules activés
    QObject::connect(mb.bBioSimple, &QPushButton::clicked, self, [=](){
        syncModuleSelection(ModuleTab::BioSimple);
        self->setWindowTitle("Échantillons");
        stack->setCurrentIndex(BIO_LIST);
    });

    QObject::connect(mb.bProjet, &QPushButton::clicked, self, [=](){
        syncModuleSelection(ModuleTab::GestionProjet);
        self->setWindowTitle("Projet");
        stack->setCurrentIndex(PROJ_LIST);
    });

    QObject::connect(mb.bExp, &QPushButton::clicked, self, [=](){
        syncModuleSelection(ModuleTab::ExperiencesProtocoles);
        self->setWindowTitle("Expériences");
        stack->setCurrentIndex(EXP_LIST);
    });

    QObject::connect(mb.bEmployee, &QPushButton::clicked, self, [=](){
        syncModuleSelection(ModuleTab::Employee);
        self->setWindowTitle("Employés");
        if (EMP_LIST >= 0 && EMP_LIST < stack->count()) {
            stack->setCurrentIndex(EMP_LIST);
        }
    });

    QObject::connect(mb.bPublication, &QPushButton::clicked, self, [=](){
        syncModuleSelection(ModuleTab::Publication);
        self->setWindowTitle("Publications");
        stack->setCurrentIndex(PUB_LIST);
    });

    QObject::connect(mb.bEquipement, &QPushButton::clicked, self, [=](){
        syncModuleSelection(ModuleTab::Equipement);
        self->setWindowTitle("Équipements");
        stack->setCurrentIndex(EQUIP_LIST);
    });

    QObject::connect(mb.bLogout, &QPushButton::clicked, self, [=](){
        self->setWindowTitle("SmartVision - Connexion");
        stack->setCurrentIndex(LOGIN);
    });

    if (mb.bVoice) {
        QObject::connect(mb.bVoice, &QPushButton::clicked, self, [=](bool checked){
            if (g_voiceCmd) {
                if (checked) { g_voiceCmd->show(); g_voiceCmd->raise(); }
                else          g_voiceCmd->hide();
            }
        });
    }

    if (mb.bTrace) {
        QObject::connect(mb.bTrace, &QPushButton::clicked, self, [=](){
            const QString fileName = QFileDialog::getSaveFileName(
                self,
                "Exporter le fichier de tracabilité",
                "tracabilite_smartvision.txt",
                "Text Files (*.txt);;All Files (*)");
            if (fileName.isEmpty()) return;

            QString target = fileName;
            if (!target.endsWith(".txt", Qt::CaseInsensitive)) target += ".txt";

            QString error;
            if (!copyTraceLogFile(target, &error)) {
                showToast(self, "Échec de l'export : " + error, false);
                return;
            }

            showToast(self, "Fichier de tracabilité enregistré : " + target, true);
        });
    }

    QObject::connect(mb.bTheme, &QPushButton::clicked, self, [=](){
        g_darkThemeEnabled = !g_darkThemeEnabled;
        if (g_applyThemeFn) g_applyThemeFn(g_darkThemeEnabled);
        syncThemeToggleButtons();
    });
}


// ===================== Widget1 badge delegate (BioSimple) =====================
enum class ExpireStatus { Ok=0, Soon=1, Expired=2, Bsl=3 };

// Maps status to label text shown in the badge.
static QString statusText(ExpireStatus s)
{
    switch (s) {
    case ExpireStatus::Ok:      return "OK";
    case ExpireStatus::Soon:    return "Bientôt\nexpiré";
    case ExpireStatus::Expired: return "Expiré";
    case ExpireStatus::Bsl:     return "BSL";
    }
    return "OK";
}

// Maps status to badge background color.
static QColor statusColor(ExpireStatus s)
{
    switch (s) {
    case ExpireStatus::Ok:      return QColor("#2E6F63");
    case ExpireStatus::Soon:    return QColor("#B5672C");
    case ExpireStatus::Expired: return QColor("#8B2F3C");
    case ExpireStatus::Bsl:     return QColor("#2E6F63");
    }
    return QColor("#2E6F63");
}

// Custom delegate to render the status pill in the table.
class BadgeDelegate : public QStyledItemDelegate
{
public:
    explicit BadgeDelegate(QObject* parent=nullptr) : QStyledItemDelegate(parent) {}

    void paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const override
    {
        QVariant v = idx.data(Qt::UserRole);
        ExpireStatus st = ExpireStatus::Ok;
        if (v.isValid()) st = static_cast<ExpireStatus>(v.toInt());

        QStyledItemDelegate::paint(p, opt, idx);

        p->save();
        p->setRenderHint(QPainter::Antialiasing, true);

        QRect r = opt.rect.adjusted(8, 6, -8, -6);
        int h = qMin(r.height(), 28);
        int w = qMin(r.width(), 140);
        QRect pill(r.left() + (r.width() - w)/2, r.top() + (r.height()-h)/2, w, h);

        QColor bg = statusColor(st);
        p->setPen(Qt::NoPen);
        p->setBrush(bg);
        p->drawRoundedRect(pill, 14, 14);

        QRect iconCircle(pill.left()+10, pill.top()+6, 16, 16);
        p->setBrush(QColor(255,255,255,35));
        p->drawEllipse(iconCircle);

        p->setPen(QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        if (st == ExpireStatus::Ok) {
            QPoint a(iconCircle.left()+4,  iconCircle.top()+9);
            QPoint b(iconCircle.left()+7,  iconCircle.top()+12);
            QPoint c(iconCircle.left()+13, iconCircle.top()+5);
            p->drawLine(a,b); p->drawLine(b,c);
        } else if (st == ExpireStatus::Soon) {
            QPainterPath path;
            path.moveTo(iconCircle.center().x(), iconCircle.top()+2);
            path.lineTo(iconCircle.left()+2, iconCircle.bottom()-2);
            path.lineTo(iconCircle.right()-2, iconCircle.bottom()-2);
            path.closeSubpath();
            p->setPen(QPen(Qt::white, 1.8));
            p->drawPath(path);
        } else if (st == ExpireStatus::Expired) {
            p->drawLine(QPoint(iconCircle.center().x(), iconCircle.top()+4),
                        QPoint(iconCircle.center().x(), iconCircle.bottom()-5));
            p->drawPoint(QPoint(iconCircle.center().x(), iconCircle.bottom()-3));
        } else {
            QRect lock(iconCircle.left()+4, iconCircle.top()+7, 8, 7);
            p->setPen(QPen(Qt::white, 1.8));
            p->drawRoundedRect(lock, 2, 2);
            p->drawArc(QRect(iconCircle.left()+4, iconCircle.top()+3, 8, 8), 0*16, 180*16);
        }

        p->setPen(Qt::white);
        QFont f = opt.font; f.setBold(true); f.setPointSizeF(f.pointSizeF()-0.5);
        p->setFont(f);
        QRect textRect = pill.adjusted(34, 4, -10, -4);
        p->drawText(textRect, Qt::AlignVCenter|Qt::AlignLeft, statusText(st));

        p->restore();
    }
};

// ===================== Équipement status badge delegate =====================
enum class EquipmentStatus { Available=0, InUse=1, UnderMaintenance=2, OutOfOrder=3 };

static QString equipmentStatusText(EquipmentStatus s)
{
    switch (s) {
    case EquipmentStatus::Available:        return "Disponible";
    case EquipmentStatus::InUse:            return "En usage";
    case EquipmentStatus::UnderMaintenance: return "Maintenance";
    case EquipmentStatus::OutOfOrder:       return "Hors service";
    }
    return "Disponible";
}

static QColor equipmentStatusColor(EquipmentStatus s)
{
    switch (s) {
    case EquipmentStatus::Available:        return QColor("#2E6F63");
    case EquipmentStatus::InUse:            return QColor("#B5672C");
    case EquipmentStatus::UnderMaintenance: return QColor("#7A8B8A");
    case EquipmentStatus::OutOfOrder:       return QColor("#8B2F3C");
    }
    return QColor("#2E6F63");
}

class StatusBadgeDelegate : public QStyledItemDelegate
{
public:
    explicit StatusBadgeDelegate(QObject* parent=nullptr) : QStyledItemDelegate(parent) {}

    void paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const override
    {
        QVariant v = idx.data(Qt::UserRole);
        EquipmentStatus st = EquipmentStatus::Available;
        if (v.isValid()) st = static_cast<EquipmentStatus>(v.toInt());

        QStyledItemDelegate::paint(p, opt, idx);

        p->save();
        p->setRenderHint(QPainter::Antialiasing, true);

        QRect r = opt.rect.adjusted(8, 6, -8, -6);
        int h = qMin(r.height(), 28);
        int w = qMin(r.width(), 130);
        QRect pill(r.left() + (r.width() - w)/2, r.top() + (r.height()-h)/2, w, h);

        QColor bg = equipmentStatusColor(st);
        p->setPen(Qt::NoPen);
        p->setBrush(bg);
        p->drawRoundedRect(pill, 14, 14);

        QRect iconCircle(pill.left()+10, pill.top()+6, 16, 16);
        p->setBrush(QColor(255,255,255,35));
        p->drawEllipse(iconCircle);

        p->setPen(QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        if (st == EquipmentStatus::Available) {
            QPoint a(iconCircle.left()+4,  iconCircle.top()+9);
            QPoint b(iconCircle.left()+7,  iconCircle.top()+12);
            QPoint c(iconCircle.left()+13, iconCircle.top()+5);
            p->drawLine(a,b); p->drawLine(b,c);
        } else if (st == EquipmentStatus::InUse) {
            p->drawEllipse(iconCircle.adjusted(4,4,-4,-4));
        } else if (st == EquipmentStatus::UnderMaintenance) {
            p->drawLine(QPoint(iconCircle.center().x()-5, iconCircle.center().y()),
                        QPoint(iconCircle.center().x()+5, iconCircle.center().y()));
        } else if (st == EquipmentStatus::OutOfOrder) {
            p->drawLine(QPoint(iconCircle.left()+4, iconCircle.top()+4),
                        QPoint(iconCircle.right()-4, iconCircle.bottom()-4));
            p->drawLine(QPoint(iconCircle.right()-4, iconCircle.top()+4),
                        QPoint(iconCircle.left()+4, iconCircle.bottom()-4));
        }

        p->setPen(Qt::white);
        QFont f = opt.font; f.setBold(true); f.setPointSizeF(f.pointSizeF()-0.5);
        p->setFont(f);
        QRect textRect = pill.adjusted(34, 4, -10, -4);
        p->drawText(textRect, Qt::AlignVCenter|Qt::AlignLeft, equipmentStatusText(st));

        p->restore();
    }
};

// ===================== Employés badge delegate =====================
enum class FTStatus { FullTime=0, PartTime=1, Contract=2, OnLeave=3 };

static QString empStatusText(FTStatus s)
{
    switch (s) {
    case FTStatus::FullTime: return "Plein";
    case FTStatus::PartTime: return "Partiel";
    case FTStatus::Contract: return "Contrat";
    case FTStatus::OnLeave:  return "Absence";
    }
    return "Plein";
}

static QColor empStatusColor(FTStatus s)
{
    switch (s) {
    case FTStatus::FullTime: return QColor("#2E6F63");
    case FTStatus::PartTime: return QColor("#B5672C");
    case FTStatus::Contract: return QColor("#7A8B8A");
    case FTStatus::OnLeave:  return QColor("#8B2F3C");
    }
    return QColor("#2E6F63");
}

class EmployeeBadgeDelegate : public QStyledItemDelegate
{
public:
    explicit EmployeeBadgeDelegate(QObject* parent=nullptr) : QStyledItemDelegate(parent) {}

    void paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const override
    {
        QVariant v = idx.data(Qt::UserRole);
        FTStatus st = FTStatus::FullTime;
        if (v.isValid()) st = static_cast<FTStatus>(v.toInt());

        QStyledItemDelegate::paint(p, opt, idx);

        p->save();
        p->setRenderHint(QPainter::Antialiasing, true);

        QRect r = opt.rect.adjusted(8, 6, -8, -6);
        int h = qMin(r.height(), 28);
        int w = qMin(r.width(), 120);
        QRect pill(r.left() + (r.width() - w)/2, r.top() + (r.height()-h)/2, w, h);

        QColor bg = empStatusColor(st);
        p->setPen(Qt::NoPen);
        p->setBrush(bg);
        p->drawRoundedRect(pill, 14, 14);

        QRect iconCircle(pill.left()+10, pill.top()+6, 16, 16);
        p->setBrush(QColor(255,255,255,35));
        p->drawEllipse(iconCircle);

        p->setPen(QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        if (st == FTStatus::FullTime) {
            QPoint a(iconCircle.left()+4,  iconCircle.top()+9);
            QPoint b(iconCircle.left()+7,  iconCircle.top()+12);
            QPoint c(iconCircle.left()+13, iconCircle.top()+5);
            p->drawLine(a,b); p->drawLine(b,c);
        } else if (st == FTStatus::PartTime) {
            QPainterPath path;
            path.moveTo(iconCircle.center().x(), iconCircle.top()+2);
            path.lineTo(iconCircle.left()+2, iconCircle.bottom()-2);
            path.lineTo(iconCircle.right()-2, iconCircle.bottom()-2);
            path.closeSubpath();
            p->setPen(QPen(Qt::white, 1.8));
            p->drawPath(path);
        } else if (st == FTStatus::Contract) {
            p->drawLine(QPoint(iconCircle.center().x(), iconCircle.top()+4),
                        QPoint(iconCircle.center().x(), iconCircle.bottom()-5));
            p->drawPoint(QPoint(iconCircle.center().x(), iconCircle.bottom()-3));
        } else {
            QRect lock(iconCircle.left()+4, iconCircle.top()+7, 8, 7);
            p->setPen(QPen(Qt::white, 1.8));
            p->drawRoundedRect(lock, 2, 2);
            p->drawArc(QRect(iconCircle.left()+4, iconCircle.top()+3, 8, 8), 0*16, 180*16);
        }

        p->setPen(Qt::white);
        QFont f = opt.font; f.setBold(true); f.setPointSizeF(f.pointSizeF()-0.5);
        p->setFont(f);
        QRect textRect = pill.adjusted(34, 4, -10, -4);
        p->drawText(textRect, Qt::AlignVCenter|Qt::AlignLeft, empStatusText(st));

        p->restore();
    }
};

// ===================== Projet status badge delegate =====================
enum class ProjStatus { EnCours=0, EnRetard=1, Critique=2, Suspendu=3, Termine=4, Planifie=5, Annule=6 };

static QString projStatusText(ProjStatus s)
{
    switch (s) {
    case ProjStatus::EnCours:  return "En cours";
    case ProjStatus::EnRetard: return "En retard";
    case ProjStatus::Critique: return "Critique";
    case ProjStatus::Suspendu: return "Suspendu";
    case ProjStatus::Termine:  return "Terminé";
    case ProjStatus::Planifie: return "Planifié";
    case ProjStatus::Annule:   return "Annulé";
    }
    return "En cours";
}

static QColor projStatusColor(ProjStatus s)
{
    switch (s) {
    case ProjStatus::EnCours:  return QColor("#416e66");
    case ProjStatus::EnRetard: return QColor("#ae7040");
    case ProjStatus::Critique: return QColor("#8B2F3C");
    case ProjStatus::Suspendu: return QColor("#7A8B8A");
    case ProjStatus::Termine:  return QColor("#367e71");
    case ProjStatus::Planifie: return QColor("#518195");
    case ProjStatus::Annule:   return QColor("#84091a");
    }
    return QColor("#547e76");
}

static ProjStatus projStatusFromText(const QString& value)
{
    const QString v = value.trimmed().toLower();
    if (v == "en retard")  return ProjStatus::EnRetard;
    if (v == "critique")   return ProjStatus::Critique;
    if (v == "suspendu")   return ProjStatus::Suspendu;
    if (v == "terminé" || v == "termine") return ProjStatus::Termine;
    if (v == "planifié" || v == "planifie") return ProjStatus::Planifie;
    if (v == "annulé"   || v == "annule")   return ProjStatus::Annule;
    return ProjStatus::EnCours;
}

class ProjetBadgeDelegate : public QStyledItemDelegate
{
public:
    explicit ProjetBadgeDelegate(QObject* parent=nullptr) : QStyledItemDelegate(parent) {}

    void paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const override
    {
        QVariant v = idx.data(Qt::UserRole);
        ProjStatus st = ProjStatus::EnCours;
        if (v.isValid()) st = static_cast<ProjStatus>(v.toInt());

        QStyledItemDelegate::paint(p, opt, idx);

        p->save();
        p->setRenderHint(QPainter::Antialiasing, true);

        QRect r = opt.rect.adjusted(8, 6, -8, -6);
        int h = qMin(r.height(), 28);
        int w = qMin(r.width(), 120);
        QRect pill(r.left() + (r.width() - w)/2, r.top() + (r.height()-h)/2, w, h);

        QColor bg = projStatusColor(st);
        p->setPen(Qt::NoPen);
        p->setBrush(bg);
        p->drawRoundedRect(pill, 14, 14);

        QRect iconCircle(pill.left()+10, pill.top()+6, 16, 16);
        p->setBrush(QColor(255,255,255,35));
        p->drawEllipse(iconCircle);

        p->setPen(QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        if (st == ProjStatus::EnCours) {
            QPoint a(iconCircle.left()+4,  iconCircle.top()+9);
            QPoint b(iconCircle.left()+7,  iconCircle.top()+12);
            QPoint c(iconCircle.left()+13, iconCircle.top()+5);
            p->drawLine(a,b); p->drawLine(b,c);
        } else if (st == ProjStatus::EnRetard) {
            QPainterPath path;
            path.moveTo(iconCircle.center().x(), iconCircle.top()+2);
            path.lineTo(iconCircle.left()+2, iconCircle.bottom()-2);
            path.lineTo(iconCircle.right()-2, iconCircle.bottom()-2);
            path.closeSubpath();
            p->setPen(QPen(Qt::white, 1.8));
            p->drawPath(path);
        } else if (st == ProjStatus::Critique) {
            p->drawLine(QPoint(iconCircle.center().x(), iconCircle.top()+4),
                        QPoint(iconCircle.center().x(), iconCircle.bottom()-5));
            p->drawPoint(QPoint(iconCircle.center().x(), iconCircle.bottom()-3));
        } else if (st == ProjStatus::Termine) {
            QPoint a(iconCircle.left()+4,  iconCircle.top()+9);
            QPoint b(iconCircle.left()+7,  iconCircle.top()+12);
            QPoint c(iconCircle.left()+13, iconCircle.top()+5);
            p->drawLine(a,b); p->drawLine(b,c);
        } else if (st == ProjStatus::Planifie) {
            // Clock icon: circle with hands
            p->drawEllipse(iconCircle.adjusted(3,3,-3,-3));
            p->drawLine(iconCircle.center(), QPoint(iconCircle.center().x(), iconCircle.top()+4));
            p->drawLine(iconCircle.center(), QPoint(iconCircle.right()-4, iconCircle.center().y()));
        } else if (st == ProjStatus::Annule) {
            p->drawLine(QPoint(iconCircle.left()+4, iconCircle.top()+4),
                        QPoint(iconCircle.right()-4, iconCircle.bottom()-4));
            p->drawLine(QPoint(iconCircle.right()-4, iconCircle.top()+4),
                        QPoint(iconCircle.left()+4, iconCircle.bottom()-4));
        } else {
            QRect lock(iconCircle.left()+4, iconCircle.top()+7, 8, 7);
            p->setPen(QPen(Qt::white, 1.8));
            p->drawRoundedRect(lock, 2, 2);
            p->drawArc(QRect(iconCircle.left()+4, iconCircle.top()+3, 8, 8), 0*16, 180*16);
        }

        p->setPen(Qt::white);
        QFont f = opt.font; f.setBold(true); f.setPointSizeF(f.pointSizeF()-0.5);
        p->setFont(f);
        QRect textRect = pill.adjusted(34, 4, -10, -4);
        p->drawText(textRect, Qt::AlignVCenter|Qt::AlignLeft, projStatusText(st));

        p->restore();
    }
};

// ===================== Expériences status badge delegate =====================
enum class ExpStatus { EnCours=0, Termine=1, EnAttente=2, Suspendue=3 };

static QString expStatusText(ExpStatus s)
{
    switch (s) {
    case ExpStatus::EnCours:   return "En cours";
    case ExpStatus::Termine:   return "Terminé";
    case ExpStatus::EnAttente: return "En attente";
    case ExpStatus::Suspendue: return "Suspendue";
    }
    return "En cours";
}

static QColor expStatusColor(ExpStatus s)
{
    switch (s) {
    case ExpStatus::EnCours:   return QColor("#2E6F63");
    case ExpStatus::Termine:   return QColor("#3A7CA5");
    case ExpStatus::EnAttente: return QColor("#B5672C");
    case ExpStatus::Suspendue: return QColor("#8B2F3C");
    }
    return QColor("#2E6F63");
}

class ExpBadgeDelegate : public QStyledItemDelegate
{
public:
    explicit ExpBadgeDelegate(QObject* parent=nullptr) : QStyledItemDelegate(parent) {}

    void paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const override
    {
        QVariant v = idx.data(Qt::UserRole);
        ExpStatus st = ExpStatus::EnCours;
        if (v.isValid()) st = static_cast<ExpStatus>(v.toInt());

        QStyledItemDelegate::paint(p, opt, idx);

        p->save();
        p->setRenderHint(QPainter::Antialiasing, true);

        QRect r = opt.rect.adjusted(8, 6, -8, -6);
        int h = qMin(r.height(), 28);
        int w = qMin(r.width(), 120);
        QRect pill(r.left() + (r.width() - w)/2, r.top() + (r.height()-h)/2, w, h);

        QColor bg = expStatusColor(st);
        p->setPen(Qt::NoPen);
        p->setBrush(bg);
        p->drawRoundedRect(pill, 14, 14);

        QRect iconCircle(pill.left()+10, pill.top()+6, 16, 16);
        p->setBrush(QColor(255,255,255,35));
        p->drawEllipse(iconCircle);

        p->setPen(QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        if (st == ExpStatus::EnCours) {
            p->drawEllipse(iconCircle.adjusted(4,4,-4,-4));
        } else if (st == ExpStatus::Termine) {
            QPoint a(iconCircle.left()+4,  iconCircle.top()+9);
            QPoint b(iconCircle.left()+7,  iconCircle.top()+12);
            QPoint c(iconCircle.left()+13, iconCircle.top()+5);
            p->drawLine(a,b); p->drawLine(b,c);
        } else if (st == ExpStatus::EnAttente) {
            QPainterPath path;
            path.moveTo(iconCircle.center().x(), iconCircle.top()+2);
            path.lineTo(iconCircle.left()+2, iconCircle.bottom()-2);
            path.lineTo(iconCircle.right()-2, iconCircle.bottom()-2);
            path.closeSubpath();
            p->setPen(QPen(Qt::white, 1.8));
            p->drawPath(path);
        } else {
            p->drawLine(QPoint(iconCircle.left()+4, iconCircle.top()+4),
                        QPoint(iconCircle.right()-4, iconCircle.bottom()-4));
            p->drawLine(QPoint(iconCircle.right()-4, iconCircle.top()+4),
                        QPoint(iconCircle.left()+4, iconCircle.bottom()-4));
        }

        p->setPen(Qt::white);
        QFont f = opt.font; f.setBold(true); f.setPointSizeF(f.pointSizeF()-0.5);
        p->setFont(f);
        QRect textRect = pill.adjusted(34, 4, -10, -4);
        p->drawText(textRect, Qt::AlignVCenter|Qt::AlignLeft, expStatusText(st));

        p->restore();
    }
};

// ===================== Widget3 (BioSimple) gradient row =====================
// A list row widget with gradient background and right-side pill.
class GradientRowWidget : public QWidget
{
public:
    GradientRowWidget(QStyle* st,
                      const QString& name,
                      const QString& rightText,
                      const QColor& baseColor,
                      QStyle::StandardPixmap leftIconSp,
                      bool warningTriangle=false,
                      QWidget* parent=nullptr)
        : QWidget(parent),
        m_style(st),
        m_name(name),
        m_right(rightText),
        m_base(baseColor),
        m_leftIconSp(leftIconSp),
        m_warning(warningTriangle)
    {
        setFixedHeight(38);
        QHBoxLayout* l = new QHBoxLayout(this);
        l->setContentsMargins(10, 6, 10, 6);
        l->setSpacing(10);

        QFrame* iconBox = new QFrame(this);
        iconBox->setFixedSize(26, 26);
        iconBox->setStyleSheet("background: rgba(255,255,255,0.18); border-radius: 6px; border:1px solid rgba(255,255,255,0.25);");
        QHBoxLayout* ib = new QHBoxLayout(iconBox);
        ib->setContentsMargins(0,0,0,0);

        QLabel* ic = new QLabel(iconBox);
        ic->setPixmap(m_style->standardIcon(m_leftIconSp).pixmap(16,16));
        ic->setAlignment(Qt::AlignCenter);
        ib->addWidget(ic);

        QLabel* nameLbl = new QLabel(m_name, this);
        nameLbl->setStyleSheet("color: rgba(255,255,255,0.92); font-weight: 900;");

        QLabel* warnLbl = new QLabel(this);
        if (m_warning) {
            warnLbl->setPixmap(m_style->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(16,16));
            warnLbl->setFixedWidth(18);
        } else {
            warnLbl->setFixedWidth(0);
        }

        QLabel* pill = new QLabel(this);
        pill->setText(m_right);
        pill->setAlignment(Qt::AlignCenter);
        pill->setFixedHeight(26);
        pill->setMinimumWidth(58);
        QColor pillC = m_base.lighter(120);
        pill->setStyleSheet(QString("QLabel{ background:%1; color: rgba(255,255,255,0.92); border-radius: 13px; padding: 2px 10px; font-weight: 900; }")
                                .arg(pillC.name()));

        l->addWidget(iconBox);
        l->addWidget(nameLbl);
        l->addStretch(1);
        if (m_warning) l->addWidget(warnLbl);
        l->addWidget(pill);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        QRect r = rect().adjusted(1, 1, -1, -1);

        QColor left = m_base.darker(115);
        QColor mid  = m_base;
        QColor right= m_base.lighter(135);

        QLinearGradient g(r.topLeft(), r.topRight());
        g.setColorAt(0.0, left);
        g.setColorAt(0.55, mid);
        g.setColorAt(1.0, right);

        QLinearGradient shine(r.topLeft(), r.bottomLeft());
        shine.setColorAt(0.0, QColor(255,255,255,35));
        shine.setColorAt(0.45, QColor(255,255,255,10));
        shine.setColorAt(1.0, QColor(0,0,0,12));

        QPainterPath path;
        path.addRoundedRect(r, 10, 10);

        p.fillPath(path, g);
        p.fillPath(path, shine);

        p.setPen(QPen(QColor(0,0,0,35), 1));
        p.drawPath(path);
    }

private:
    QStyle* m_style;
    QString m_name, m_right;
    QColor m_base;
    QStyle::StandardPixmap m_leftIconSp;
    bool m_warning;
};

// Creates a compact information block for temperature and quantity.
static QFrame* w3TempQtyBlock(QStyle* st, const QString& temp, const QString& qty)
{
    QFrame* box = new QFrame;
    box->setStyleSheet(QString("QFrame{ background: rgba(255,255,255,0.70); border:1px solid %1; border-radius: 12px; }").arg(C_PANEL_BR));
    QVBoxLayout* v = new QVBoxLayout(box);
    v->setContentsMargins(12,10,12,10);
    v->setSpacing(8);

    auto line = [&](QStyle::StandardPixmap sp, const QString& t){
        QWidget* row = new QWidget;
        QHBoxLayout* h = new QHBoxLayout(row);
        h->setContentsMargins(0,0,0,0);
        h->setSpacing(10);
        QLabel* ic = new QLabel;
        ic->setPixmap(st->standardIcon(sp).pixmap(18,18));
        QLabel* lab = new QLabel(t);
        lab->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
        h->addWidget(ic);
        h->addWidget(lab);
        h->addStretch(1);
        return row;
    };

    v->addWidget(line(QStyle::SP_BrowserStop, QString("Température : %1").arg(temp)));
    v->addWidget(line(QStyle::SP_ArrowUp,    QString("Quantité : %1").arg(qty)));
    return box;
}

// ===================== STATISTIQUES (Graphiques) =====================
// Simple donut chart widget for stats dashboards.
class DonutChart : public QWidget {
public:
    struct Slice { double value; QColor color; QString label; };
    explicit DonutChart(QWidget* parent=nullptr) : QWidget(parent) { setMinimumHeight(180); }
    void setData(const QList<Slice>& s) { m_slices = s; update(); }
    void setDark(bool d) { m_dark = d; update(); }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        QRect r = rect().adjusted(8, 8, -8, -8);
        int d = qMin(r.width(), r.height());
        QRect pie(r.center().x() - d/2, r.center().y() - d/2, d, d);

        double total = 0;
        for (auto& s : m_slices) total += s.value;
        if (total <= 0) {
            p.setPen(m_dark ? QColor(140,160,170,120) : QColor(0,0,0,60));
            QFont f = font(); f.setPointSize(10); f.setBold(true);
            p.setFont(f);
            p.drawText(rect(), Qt::AlignCenter, "Aucune donnée");
            return;
        }

        int thickness = (int)(d * 0.27);
        QRect inner = pie.adjusted(thickness, thickness, -thickness, -thickness);

        // Draw slices
        double start = 90.0 * 16;
        for (auto& s : m_slices) {
            double span = -(s.value / total) * 360.0 * 16;
            p.setPen(Qt::NoPen);
            p.setBrush(s.color);
            p.drawPie(pie, (int)start, (int)span);
            start += span;
        }

        // Thin separator rings for polish
        p.setPen(QPen(m_dark ? QColor(20,30,42,180) : QColor(255,255,255,160), 2));
        p.setBrush(Qt::NoBrush);
        start = 90.0 * 16;
        for (auto& s : m_slices) {
            double span = -(s.value / total) * 360.0 * 16;
            double rad = qDegreesToRadians(start / 16.0);
            QPointF c = pie.center();
            double r1 = d / 2.0, r2 = d / 2.0 - thickness;
            p.drawLine(QPointF(c.x() + r2*std::cos(rad), c.y() - r2*std::sin(rad)),
                       QPointF(c.x() + r1*std::cos(rad), c.y() - r1*std::sin(rad)));
            start += span;
        }

        // Percentage labels on arc
        start = 90.0 * 16;
        for (auto& s : m_slices) {
            double span = -(s.value / total) * 360.0 * 16;
            int pct = (int)std::round((s.value / total) * 100.0);
            if (pct >= 6) {
                double midDeg = (start + span / 2.0) / 16.0;
                double rad = qDegreesToRadians(midDeg);
                QPointF c = pie.center();
                double rr = d * 0.37;
                QPointF pos(c.x() + rr * std::cos(rad), c.y() - rr * std::sin(rad));
                p.setPen(QColor(255,255,255,230));
                QFont f = font(); f.setBold(true); f.setPointSize(8);
                p.setFont(f);
                p.drawText(QRectF(pos.x()-18, pos.y()-10, 36, 20),
                           Qt::AlignCenter, QString("%1%").arg(pct));
            }
            start += span;
        }

        // Center hole
        p.setPen(Qt::NoPen);
        p.setBrush(m_dark ? QColor(25,35,45,255) : QColor(246,248,247,255));
        p.drawEllipse(inner);

        // Center: total count
        QFont f = font(); f.setBold(true); f.setPointSize(14);
        p.setFont(f);
        p.setPen(m_dark ? QColor(220,235,242) : QColor(0,0,0,160));
        QRect topHalf = inner.adjusted(0, inner.height()/5, 0, -inner.height()/2);
        p.drawText(topHalf, Qt::AlignCenter, QString::number((int)total));

        QFont sf = font(); sf.setPointSize(8); sf.setBold(false);
        p.setFont(sf);
        p.setPen(m_dark ? QColor(130,155,170) : QColor(0,0,0,90));
        QRect botHalf = inner.adjusted(0, inner.height()/2, 0, -inner.height()/6);
        p.drawText(botHalf, Qt::AlignCenter, "total");
    }

private:
    QList<Slice> m_slices;
    bool m_dark = false;
};

// Simple bar chart widget for stats dashboards.
class BarChart : public QWidget {
public:
    struct Bar { double value; QString label; };
    explicit BarChart(QWidget* parent=nullptr) : QWidget(parent) { setMinimumHeight(180); }
    void setData(const QList<Bar>& b) { m_bars = b; update(); }
    void setDark(bool d) { m_dark = d; update(); }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        QRect r = rect().adjusted(12,12,-12,-12);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255,255,255,60));
        p.drawRoundedRect(r, 12, 12);

        if (m_bars.isEmpty()) return;

        double maxV = 0;
        for (auto &b : m_bars) maxV = std::max(maxV, b.value);
        if (maxV <= 0) maxV = 1;

        int leftPad = 40;
        int bottomPad = 30;
        QRect plot = r.adjusted(leftPad, 18, -10, -bottomPad);

        const QColor textClr  = m_dark ? QColor(180,200,210)  : QColor(0,0,0,140);
        const QColor gridClr  = m_dark ? QColor(255,255,255,18) : QColor(0,0,0,22);
        const QColor bar1Clr  = m_dark ? QColor("#2DD4BF") : W_GREEN.lighter(130);
        const QColor bar2Clr  = m_dark ? QColor("#0A9488") : W_GREEN.darker(120);

        p.setPen(textClr);
        QFont t = font(); t.setBold(true); t.setPointSize(9);
        p.setFont(t);
        p.drawText(QRect(r.left()+10, r.top()-2, r.width()-20, 16),
                   Qt::AlignLeft|Qt::AlignVCenter, "Nombre");

        QFont f = font(); f.setPointSize(8); f.setBold(true);
        p.setFont(f);

        int tickCount = 4;
        // For small ranges, use integer steps to avoid duplicate rounded labels (e.g. 2,2,1,1,0).
        if (maxV <= 4.0) {
            tickCount = qMax(1, static_cast<int>(std::round(maxV)));
        }
        for (int i=0;i<=tickCount;i++){
            double frac = (double)i / (double)tickCount;
            double val = maxV * (1.0 - frac);
            int y = (int)(plot.top() + frac * plot.height());
            p.setPen(textClr);
            p.drawText(QRect(r.left(), y-8, leftPad-6, 16),
                       Qt::AlignRight|Qt::AlignVCenter, QString::number((int)std::round(val)));
            p.setPen(gridClr);
            p.drawLine(plot.left(), y, plot.right(), y);
        }

        int n = m_bars.size();
        int gap = 8;
        int bw = (plot.width() - gap*(n-1)) / n;
        if (bw < 6) bw = 6;

        for (int i=0; i<n; ++i) {
            double v = m_bars[i].value;
            int h = (int)((v / maxV) * plot.height());
            QRect bar(plot.left() + i*(bw+gap), plot.bottom()-h, bw, h);

            QLinearGradient g(bar.topLeft(), bar.bottomLeft());
            g.setColorAt(0, bar1Clr);
            g.setColorAt(1, bar2Clr);

            p.setPen(Qt::NoPen);
            p.setBrush(g);
            p.drawRoundedRect(bar, 6, 6);

            // Value on top of bar
            if (h > 18 && v > 0) {
                p.setPen(QColor(255,255,255,200));
                QFont vf = font(); vf.setPointSize(7); vf.setBold(true);
                p.setFont(vf);
                p.drawText(QRect(bar.left(), bar.top()-1, bar.width(), 14),
                           Qt::AlignCenter, QString::number((int)v));
            }

            p.setPen(textClr);
            p.setFont(f);
            p.drawText(QRect(bar.left(), plot.bottom()+6, bar.width(), 18),
                       Qt::AlignCenter, m_bars[i].label);
        }
    }

private:
    QList<Bar> m_bars;
    bool m_dark = false;
};

// Horizontal bar chart widget — used for "Quantité restante par référence".
class HorizontalBarChart : public QWidget {
public:
    struct Bar { double value; QString label; };
    explicit HorizontalBarChart(QWidget* parent = nullptr) : QWidget(parent) { setMinimumHeight(200); }
    void setData(const QList<Bar>& b) { m_bars = b; update(); }
    void setDark(bool d) { m_dark = d; update(); }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const QColor textClr = m_dark ? QColor(200,220,230) : QColor(0,0,0,150);
        const QColor gridClr = m_dark ? QColor(255,255,255,14) : QColor(0,0,0,18);
        const QColor bar1    = m_dark ? QColor("#2DD4BF")       : QColor("#0A5F58");
        const QColor bar2    = m_dark ? QColor("#0891B2")       : QColor("#2E6F63");

        if (m_bars.isEmpty()) {
            p.setPen(textClr);
            QFont f = font(); f.setPointSize(10); f.setBold(true); p.setFont(f);
            p.drawText(rect(), Qt::AlignCenter, "Aucune donnée");
            return;
        }

        const int leftPad   = 110;
        const int rightPad  = 48;
        const int topPad    = 10;
        const int botPad    = 10;
        QRect r = rect().adjusted(leftPad, topPad, -rightPad, -botPad);

        double maxV = 0;
        for (auto& b : m_bars) maxV = std::max(maxV, b.value);
        if (maxV <= 0) maxV = 1;

        int n     = m_bars.size();
        int gap   = qMax(4, (r.height() - n*16) / qMax(1,n-1));
        int barH  = qMax(10, (r.height() - gap*(n-1)) / n);

        // Vertical grid lines
        p.setPen(QPen(gridClr, 1));
        for (int i = 0; i <= 4; ++i) {
            int x = r.left() + (int)(r.width() * i / 4.0);
            p.drawLine(x, r.top(), x, r.bottom());
        }

        for (int i = 0; i < n; ++i) {
            int y  = r.top() + i * (barH + gap);
            int bw = (int)((m_bars[i].value / maxV) * r.width());

            // Bar
            QRect bar(r.left(), y, bw, barH);
            QLinearGradient g(bar.topLeft(), bar.topRight());
            g.setColorAt(0, bar1);
            g.setColorAt(1, bar2);
            p.setPen(Qt::NoPen);
            p.setBrush(g);
            p.drawRoundedRect(bar, 5, 5);

            // Label left
            p.setPen(textClr);
            QFont lf = font(); lf.setPointSize(9); lf.setBold(true);
            p.setFont(lf);
            QString lbl = m_bars[i].label;
            if (lbl.length() > 13) lbl = lbl.left(12) + "…";
            p.drawText(QRect(0, y, leftPad - 6, barH),
                       Qt::AlignRight | Qt::AlignVCenter, lbl);

            // Value right
            p.drawText(QRect(r.right() + 5, y, rightPad - 4, barH),
                       Qt::AlignLeft | Qt::AlignVCenter,
                       QString::number((int)m_bars[i].value));
        }
    }

private:
    QList<Bar> m_bars;
    bool m_dark = false;
};

class UsageBarChart : public QWidget {
public:
    explicit UsageBarChart(QWidget* parent = nullptr) : QWidget(parent) { setMinimumHeight(100); }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        QRect r = rect().adjusted(30, 10, -10, -25);

        p.setPen(QPen(QColor(0,0,0,15), 1));
        for (int i=0; i<=4; ++i) {
            int y = r.top() + (r.height() * i / 4);
            p.drawLine(r.left(), y, r.right(), y);
        }

        QList<double> data = {6.5, 8.2, 7.8, 9.1, 5.4, 8.7, 7.2};
        QStringList labels = {"Lun", "Mar", "Mer", "Jeu", "Ven", "Sam", "Dim"};

        double maxVal = 10.0;
        int barCount = data.size();
        int barWidth = (r.width() - (barCount-1)*8) / barCount;

        for (int i=0; i<barCount; ++i) {
            double val = data[i];
            int h = (int)((val / maxVal) * r.height());

            QRect bar(r.left() + i*(barWidth+8), r.bottom()-h, barWidth, h);

            QLinearGradient g(bar.topLeft(), bar.bottomLeft());
            g.setColorAt(0, QColor("#5AB9EA"));
            g.setColorAt(1, QColor("#4A90E2"));

            p.setPen(Qt::NoPen);
            p.setBrush(g);
            p.drawRoundedRect(bar, 4, 4);

            p.setPen(QColor(0,0,0,120));
            QFont f = font();
            f.setPointSize(8);
            f.setBold(true);
            p.setFont(f);
            p.drawText(QRect(bar.left(), r.bottom()+5, bar.width(), 15),
                       Qt::AlignCenter, labels[i]);
        }

        p.setPen(QColor(0,0,0,90));
        QFont yf = font();
        yf.setPointSize(8);
        p.setFont(yf);
        for (int i=0; i<=4; ++i) {
            int y = r.top() + (r.height() * i / 4);
            int val = (int)(maxVal * (4-i) / 4);
            p.drawText(QRect(0, y-8, 25, 16), Qt::AlignRight|Qt::AlignVCenter, QString::number(val));
        }
    }
};

// ===================== Widget4 helpers (BioSimple Rack) =====================
// Filter pill used in rack page filters.
static QFrame* w4FilterPill(const QString& text)
{
    QFrame* f = new QFrame;
    f->setStyleSheet("QFrame{ background: rgba(255,255,255,0.72); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QHBoxLayout* h = new QHBoxLayout(f);
    h->setContentsMargins(10,8,10,8);
    h->setSpacing(8);
    QLabel* t = new QLabel(text);
    t->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    QToolButton* dd = new QToolButton;
    dd->setAutoRaise(true);
    dd->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowDown));
    dd->setCursor(Qt::PointingHandCursor);
    h->addWidget(t);
    h->addStretch(1);
    h->addWidget(dd);
    return f;
}

// Initializes the rack grid with sizes, labels, and colors.
static void w4SetupRackTable(QTableWidget* rack)
{
    rack->setRowCount(6);
    rack->setColumnCount(6);
    rack->horizontalHeader()->setVisible(false);
    rack->verticalHeader()->setVisible(false);
    rack->setShowGrid(true);
    rack->setGridStyle(Qt::SolidLine);
    rack->setEditTriggers(QAbstractItemView::NoEditTriggers);
    rack->setSelectionMode(QAbstractItemView::NoSelection);

    rack->setStyleSheet(g_darkThemeEnabled ? R"(
        QTableWidget{
            background: #121920;
            border: 1px solid rgba(255,255,255,0.16);
            border-radius: 12px;
            gridline-color: rgba(255,255,255,0.12);
        }
        QTableWidget::item{
            border: none;
            font-weight: 900;
            color: #E8EEF2;
        }
    )" : R"(
        QTableWidget{
            background: rgba(255,255,255,0.65);
            border: 1px solid rgba(0,0,0,0.10);
            border-radius: 12px;
            gridline-color: rgba(0,0,0,0.10);
        }
        QTableWidget::item{
            border: none;
            font-weight: 900;
            color: rgba(0,0,0,0.65);
        }
    )");

    for (int c=0; c<6; ++c) rack->setColumnWidth(c, 52);
    for (int r=0; r<6; ++r) rack->setRowHeight(r, 34);

    int val = 1;
    for (int r=0; r<6; ++r){
        for(int c=0; c<6; ++c){
            QTableWidgetItem* it = new QTableWidgetItem(QString::number(val++));
            it->setTextAlignment(Qt::AlignCenter);
            rack->setItem(r,c,it);
        }
    }

    auto colorRow = [&](int r, const QColor& bg, const QColor& fg){
        for(int c=0;c<6;++c){
            rack->item(r,c)->setBackground(bg);
            rack->item(r,c)->setForeground(fg);
        }
    };
    colorRow(0, QColor("#9FBEB9"), QColor(0,0,0,140));
    colorRow(2, W_GREEN, QColor(255,255,255,230));
    colorRow(4, QColor("#9FBEB9"), QColor(0,0,0,140));

    rack->item(1,1)->setBackground(W_GREEN);
    rack->item(1,1)->setForeground(QColor(255,255,255,235));
    rack->item(3,4)->setBackground(W_GREEN);
    rack->item(3,4)->setForeground(QColor(255,255,255,235));
}

// Populates the constraints table for rack/BSL rules.
static void w4SetupAccountsTable(QTableWidget* t)
{
    t->setColumnCount(4);
    t->setRowCount(4);
    t->setHorizontalHeaderLabels({"ID","Type","Température","BSL"});
    t->verticalHeader()->setVisible(false);
    t->horizontalHeader()->setStretchLastSection(true);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setSelectionMode(QAbstractItemView::NoSelection);

    t->setStyleSheet(g_darkThemeEnabled ? QString(R"(
        QTableWidget{
            background: #121920;
            border: 1px solid rgba(255,255,255,0.16);
            border-radius: 12px;
            gridline-color: rgba(255,255,255,0.12);
        }
        QHeaderView::section{
            background: #324752;
            color: rgba(255,255,255,0.86);
            border: none;
            padding: 8px 10px;
            font-weight: 900;
        }
        QTableWidget::item{
            padding: 8px 10px;
            color: #E8EEF2;
            font-weight: 800;
        }
    )") : QString(R"(
        QTableWidget{
            background: rgba(255,255,255,0.65);
            border: 1px solid %1;
            border-radius: 12px;
            gridline-color: rgba(0,0,0,0.10);
        }
        QHeaderView::section{
            background: rgba(159,190,185,0.85);
            color: rgba(0,0,0,0.60);
            border: none;
            padding: 8px 10px;
            font-weight: 900;
        }
        QTableWidget::item{
            padding: 8px 10px;
            color: rgba(0,0,0,0.65);
            font-weight: 800;
        }
    )").arg(C_PANEL_BR));

    auto setR=[&](int r, const QString& id, const QString& type, const QString& temp, const QString& bsl){
        t->setItem(r,0,new QTableWidgetItem(id));
        t->setItem(r,1,new QTableWidgetItem(type));
        t->setItem(r,2,new QTableWidgetItem(temp));
        t->setItem(r,3,new QTableWidgetItem(bsl));
        for(int c=0;c<4;++c) t->item(r,c)->setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    };

    setR(0,"123456","DNA","-50°C","BSL-1");
    setR(1,"123458","DNA","-20°C","BSL-2");
    setR(2,"123459","RNA","-20°C","BSL-3");
    setR(3,"123460","RNA","-60°C","BSL-3");
}

// ===================== Employés helpers =====================
static QFrame* empInfoBlock(QStyle* st, const QString& line1, const QString& line2)
{
    QFrame* box = new QFrame;
    box->setStyleSheet(QString("QFrame{ background: rgba(255,255,255,0.70); border:1px solid %1; border-radius: 12px; }")
                           .arg(C_PANEL_BR));
    QVBoxLayout* v = new QVBoxLayout(box);
    v->setContentsMargins(12,10,12,10);
    v->setSpacing(8);

    auto line = [&](QStyle::StandardPixmap sp, const QString& t){
        QWidget* row = new QWidget;
        QHBoxLayout* h = new QHBoxLayout(row);
        h->setContentsMargins(0,0,0,0);
        h->setSpacing(10);
        QLabel* ic = new QLabel;
        ic->setPixmap(st->standardIcon(sp).pixmap(18,18));
        QLabel* lab = new QLabel(t);
        lab->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
        h->addWidget(ic);
        h->addWidget(lab);
        h->addStretch(1);
        return row;
    };

    v->addWidget(line(QStyle::SP_FileDialogInfoView, line1));
    v->addWidget(line(QStyle::SP_ArrowUp,            line2));
    return box;
}

Q_DECL_UNUSED static QFrame* empBottomBarWithText(QStyle* st, const QString& text)
{
    QFrame* bar = new QFrame;
    bar->setStyleSheet(QString("QFrame{ background: rgba(255,255,255,0.70); border:1px solid %1; border-radius: 12px; }")
                           .arg(C_PANEL_BR));
    QHBoxLayout* h = new QHBoxLayout(bar);
    h->setContentsMargins(10,8,10,8);
    h->setSpacing(10);

    QLabel* eye = new QLabel;
    eye->setPixmap(st->standardIcon(QStyle::SP_FileDialogContentsView).pixmap(18,18));

    QLabel* t = new QLabel(text);
    t->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");

    QToolButton* lock = new QToolButton;
    lock->setAutoRaise(true);
    lock->setIcon(st->standardIcon(QStyle::SP_MessageBoxInformation));
    lock->setCursor(Qt::PointingHandCursor);
    lock->setStyleSheet("QToolButton{ padding:6px; border-radius:10px; } QToolButton:hover{ background: rgba(0,0,0,0.06);}");

    QToolButton* dd = new QToolButton;
    dd->setAutoRaise(true);
    dd->setIcon(st->standardIcon(QStyle::SP_ArrowDown));
    dd->setCursor(Qt::PointingHandCursor);
    dd->setStyleSheet(lock->styleSheet());

    h->addWidget(eye);
    h->addWidget(t);
    h->addStretch(1);
    h->addWidget(lock);
    h->addWidget(dd);
    return bar;
}

static QFrame* empFilterPill(const QString& text)
{
    QFrame* f = new QFrame;
    f->setStyleSheet("QFrame{ background: rgba(255,255,255,0.72); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QHBoxLayout* h = new QHBoxLayout(f);
    h->setContentsMargins(10,8,10,8);
    h->setSpacing(8);
    QLabel* t = new QLabel(text);
    t->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    QToolButton* dd = new QToolButton;
    dd->setAutoRaise(true);
    dd->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowDown));
    dd->setCursor(Qt::PointingHandCursor);
    h->addWidget(t);
    h->addStretch(1);
    h->addWidget(dd);
    return f;
}

static void empSetupAvailabilityGrid(QTableWidget* grid)
{
    grid->setRowCount(6);
    grid->setColumnCount(6);
    grid->horizontalHeader()->setVisible(false);
    grid->verticalHeader()->setVisible(false);
    grid->setShowGrid(true);
    grid->setGridStyle(Qt::SolidLine);
    grid->setEditTriggers(QAbstractItemView::NoEditTriggers);
    grid->setSelectionMode(QAbstractItemView::NoSelection);

    grid->setStyleSheet(g_darkThemeEnabled ? R"(
        QTableWidget{
            background: #121920;
            border: 1px solid rgba(255,255,255,0.16);
            border-radius: 12px;
            gridline-color: rgba(255,255,255,0.12);
        }
        QTableWidget::item{
            border: none;
            font-weight: 900;
            color: #E8EEF2;
        }
    )" : R"(
        QTableWidget{
            background: rgba(255,255,255,0.65);
            border: 1px solid rgba(0,0,0,0.10);
            border-radius: 12px;
            gridline-color: rgba(0,0,0,0.10);
        }
        QTableWidget::item{
            border: none;
            font-weight: 900;
            color: rgba(0,0,0,0.65);
        }
    )");

    for (int c=0; c<6; ++c) grid->setColumnWidth(c, 52);
    for (int r=0; r<6; ++r) grid->setRowHeight(r, 34);

    int val = 1;
    for (int r=0; r<6; ++r){
        for(int c=0; c<6; ++c){
            QTableWidgetItem* it = new QTableWidgetItem(QString("%1").arg(val++));
            it->setTextAlignment(Qt::AlignCenter);
            grid->setItem(r,c,it);
        }
    }

    auto colorRow = [&](int r, const QColor& bg, const QColor& fg){
        for(int c=0;c<6;++c){
            grid->item(r,c)->setBackground(bg);
            grid->item(r,c)->setForeground(fg);
        }
    };
    colorRow(0, QColor("#9FBEB9"), QColor(0,0,0,140));
    colorRow(2, QColor("#2E6F63"), QColor(255,255,255,230));
    colorRow(4, QColor("#9FBEB9"), QColor(0,0,0,140));

    grid->item(1,1)->setBackground(QColor("#2E6F63"));
    grid->item(1,1)->setForeground(QColor(255,255,255,235));
    grid->item(3,4)->setBackground(QColor("#2E6F63"));
    grid->item(3,4)->setForeground(QColor(255,255,255,235));
}

static void empSetupConstraintsTable(QTableWidget* t)
{
    t->setColumnCount(4);
    t->setRowCount(4);
    t->setHorizontalHeaderLabels({"CIN","Rôle","Spécialisation","Laboratoire"});
    t->verticalHeader()->setVisible(false);
    t->horizontalHeader()->setStretchLastSection(true);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setSelectionMode(QAbstractItemView::NoSelection);

    t->setStyleSheet(g_darkThemeEnabled ? QString(R"(
        QTableWidget{
            background: #121920;
            border: 1px solid rgba(255,255,255,0.16);
            border-radius: 12px;
            gridline-color: rgba(255,255,255,0.12);
        }
        QHeaderView::section{
            background: #324752;
            color: rgba(255,255,255,0.86);
            border: none;
            padding: 8px 10px;
            font-weight: 900;
        }
        QTableWidget::item{
            padding: 8px 10px;
            color: #E8EEF2;
            font-weight: 800;
        }
    )") : QString(R"(
        QTableWidget{
            background: rgba(255,255,255,0.65);
            border: 1px solid %1;
            border-radius: 12px;
            gridline-color: rgba(0,0,0,0.10);
        }
        QHeaderView::section{
            background: rgba(159,190,185,0.85);
            color: rgba(0,0,0,0.60);
            border: none;
            padding: 8px 10px;
            font-weight: 900;
        }
        QTableWidget::item{
            padding: 8px 10px;
            color: rgba(0,0,0,0.65);
            font-weight: 800;
        }
    )").arg(C_PANEL_BR));

    auto setR=[&](int r, const QString& cin, const QString& role, const QString& spec, const QString& lab){
        t->setItem(r,0,new QTableWidgetItem(cin));
        t->setItem(r,1,new QTableWidgetItem(role));
        t->setItem(r,2,new QTableWidgetItem(spec));
        t->setItem(r,3,new QTableWidgetItem(lab));
        for(int c=0;c<4;++c) t->item(r,c)->setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    };

    setR(0,"AA123456","Chercheur","Biomol","Lab A");
    setR(1,"BB654321","Technicien","Chimie","Lab B");
    setR(2,"CC998877","Chercheur","Bioinfo","Lab C");
    setR(3,"DD112233","Technicien","Général","Lab A");
}

// ===================== Dialog: Confirm delete (design) =====================
// Styled confirmation dialog used before deleting rows.
class ConfirmDeleteDialog : public QDialog
{
public:
    ConfirmDeleteDialog(QStyle* st, const QString& lineText, QWidget* parent=nullptr)
        : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint)
    {
        setModal(true);
        setAttribute(Qt::WA_TranslucentBackground);
        setFixedSize(520, 230);

        // ── Card ──
        QWidget* card = new QWidget(this);
        card->setGeometry(0, 0, 520, 230);
        card->setStyleSheet(
            "QWidget#card{"
            "  background: #F4F9F8;"
            "  border-radius: 16px;"
            "  border: 1.5px solid #A3CAD3;"
            "}"
        );
        card->setObjectName("card");

        QVBoxLayout* root = new QVBoxLayout(card);
        root->setContentsMargins(16, 16, 16, 16);
        root->setSpacing(12);

        // ── Header ──
        QFrame* head = new QFrame;
        head->setFixedHeight(50);
        head->setStyleSheet(
            "QFrame{ background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
            " stop:0 #0A5F58, stop:1 #12443B);"
            " border-radius: 12px; }");
        QHBoxLayout* hl = new QHBoxLayout(head);
        hl->setContentsMargins(14, 0, 14, 0);
        hl->setSpacing(10);

        QLabel* ic = new QLabel("⚠");
        ic->setStyleSheet(
            "color:#F5C842; font-size:20px; background:transparent; border:none;");

        QLabel* t = new QLabel("  Supprimer l’élément ?");
        QFont ft = t->font(); ft.setBold(true); ft.setPointSize(11);
        t->setFont(ft);
        t->setStyleSheet("color:white; background:transparent; border:none;");

        hl->addWidget(ic);
        hl->addWidget(t);
        hl->addStretch(1);
        root->addWidget(head);

        // ── Body ──
        QFrame* body = new QFrame;
        body->setStyleSheet(
            "QFrame{ background: rgba(255,255,255,0.85);"
            " border: 1px solid rgba(10,95,88,0.20);"
            " border-radius: 12px; }");
        QVBoxLayout* bl = new QVBoxLayout(body);
        bl->setContentsMargins(14, 12, 14, 12);
        bl->setSpacing(8);

        QLabel* msg = new QLabel("Voulez-vous vraiment supprimer la ligne sélectionnée ?");
        msg->setStyleSheet(
            "color:#12443B; font-weight:700; background:transparent; border:none;");
        msg->setWordWrap(true);

        QLabel* details = new QLabel(lineText);
        details->setStyleSheet(
            "color:#0A5F58; font-weight:600; background:transparent; border:none;");
        details->setWordWrap(true);

        bl->addWidget(msg);
        bl->addWidget(details);
        root->addWidget(body, 1);

        // ── Buttons ──
        QHBoxLayout* btns = new QHBoxLayout;
        btns->setSpacing(10);
        btns->addStretch(1);

        QPushButton* cancel = new QPushButton(
            st->standardIcon(QStyle::SP_DialogCancelButton), "  Annuler");
        cancel->setCursor(Qt::PointingHandCursor);
        cancel->setFixedHeight(40);
        cancel->setStyleSheet(
            "QPushButton{ background:white; border:1.5px solid #A3CAD3;"
            " border-radius:12px; padding:8px 18px; font-weight:700;"
            " color:#12443B; }"
            "QPushButton:hover{ background:#E8F5F3; border-color:#0A5F58; }");

        QPushButton* del = new QPushButton(
            st->standardIcon(QStyle::SP_TrashIcon), "  Supprimer");
        del->setCursor(Qt::PointingHandCursor);
        del->setFixedHeight(40);
        del->setStyleSheet(
            "QPushButton{ background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
            " stop:0 #0A5F58, stop:1 #12443B);"
            " border:none; border-radius:12px; padding:8px 18px;"
            " font-weight:700; color:white; }"
            "QPushButton:hover{ background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
            " stop:0 #0d7a71, stop:1 #1a5c50); }");

        QObject::connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
        QObject::connect(del,    &QPushButton::clicked, this, &QDialog::accept);

        btns->addWidget(cancel);
        btns->addWidget(del);
        root->addLayout(btns);

        // ── Fade-in animation ──
        setWindowOpacity(0.0);
        QPropertyAnimation* anim = new QPropertyAnimation(this, "windowOpacity", this);
        anim->setDuration(220);
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        QTimer::singleShot(0, anim, [anim]{ anim->start(QAbstractAnimation::DeleteWhenStopped); });
    }
};


// ===================== Dialog: Themed Alert (replaces QMessageBox) =====================
// type: "info" | "warning" | "error"
class ThemedAlertDialog : public QDialog
{
public:
    ThemedAlertDialog(QStyle* st, const QString& type,
                      const QString& title, const QString& message,
                      QWidget* parent = nullptr)
        : QDialog(parent, Qt::Dialog | Qt::FramelessWindowHint)
    {
        setModal(true);
        setAttribute(Qt::WA_TranslucentBackground);
        setFixedSize(620, 250);

        QWidget* card = new QWidget(this);
        card->setGeometry(0, 0, 620, 250);
        card->setObjectName("card");
        card->setStyleSheet(
            "QWidget#card{ background:#F4F9F8; border-radius:16px;"
            " border:1.5px solid #A3CAD3; }");

        QVBoxLayout* root = new QVBoxLayout(card);
        root->setContentsMargins(16, 16, 16, 16);
        root->setSpacing(12);

        // ── Type-based color theme (matches showSanteAlert in gestproj.cpp) ──
        QString headColor = (type == "error")   ? "qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #8B2F3C,stop:1 #6A1E29)"
                          : (type == "warning") ? "qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #B5672C,stop:1 #8C4E1E)"
                                                : "qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #0A5F58,stop:1 #12443B)";
        QString btnColor  = headColor;
        QString bodyBorderColor = (type == "error")   ? "rgba(139,47,60,0.20)"
                                : (type == "warning") ? "rgba(181,103,44,0.25)"
                                                      : "rgba(10,95,88,0.20)";
        QString msgColor  = (type == "error")   ? "#6A1E29"
                          : (type == "warning") ? "#8C4E1E"
                                                : "#12443B";

        // ── Header ──
        QFrame* head = new QFrame;
        head->setFixedHeight(50);
        head->setStyleSheet(QString("QFrame{ background: %1; border-radius:12px; }").arg(headColor));
        QHBoxLayout* hl = new QHBoxLayout(head);
        hl->setContentsMargins(14, 0, 14, 0);
        hl->setSpacing(10);

        QString icon = (type == "warning") ? "\u26a0" : (type == "error") ? "\u2715" : "\u2139";
        QLabel* ic = new QLabel(icon);
        ic->setStyleSheet("color:#F5C842; font-size:20px; background:transparent; border:none;");

        QLabel* t = new QLabel("  " + title);
        QFont ft = t->font(); ft.setBold(true); ft.setPointSize(11);
        t->setFont(ft);
        t->setStyleSheet("color:white; background:transparent; border:none;");

        hl->addWidget(ic);
        hl->addWidget(t);
        hl->addStretch(1);
        root->addWidget(head);

        // ── Body ──
        QFrame* body = new QFrame;
        body->setStyleSheet(QString(
            "QFrame{ background:rgba(255,255,255,0.85);"
            " border:1px solid %1; border-radius:12px; }").arg(bodyBorderColor));
        QVBoxLayout* bl = new QVBoxLayout(body);
        bl->setContentsMargins(14, 12, 14, 12);

        QLabel* msg = new QLabel(message);
        msg->setStyleSheet(QString("color:%1; font-weight:700; background:transparent; border:none;").arg(msgColor));
        msg->setWordWrap(true);
        bl->addWidget(msg);
        root->addWidget(body, 1);

        // ── OK Button ──
        QHBoxLayout* btns = new QHBoxLayout;
        btns->addStretch(1);
        QPushButton* ok = new QPushButton(
            st->standardIcon(QStyle::SP_DialogApplyButton), "  OK");
        ok->setCursor(Qt::PointingHandCursor);
        ok->setFixedHeight(40);
        ok->setStyleSheet(QString(
            "QPushButton{ background: %1;"
            " border:none; border-radius:12px; padding:8px 24px;"
            " font-weight:700; color:white; }"
            "QPushButton:hover{ opacity:0.85; }").arg(btnColor));
        QObject::connect(ok, &QPushButton::clicked, this, &QDialog::accept);
        btns->addWidget(ok);
        root->addLayout(btns);

        // ── Fade-in ──
        setWindowOpacity(0.0);
        QPropertyAnimation* anim = new QPropertyAnimation(this, "windowOpacity", this);
        anim->setDuration(220);
        anim->setStartValue(0.0); anim->setEndValue(1.0);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        QTimer::singleShot(0, anim, [anim]{ anim->start(QAbstractAnimation::DeleteWhenStopped); });
    }

    static void show(QStyle* st, QWidget* parent, const QString& type,
                     const QString& title, const QString& message)
    {
        ThemedAlertDialog d(st, type, title, message, parent);
        d.exec();
    }
};

// ===================== Dialog: Success (Modern Design) =====================
// Beautiful success dialog with modern design
class SuccessDialog : public QDialog
{
public:
    SuccessDialog(const QString& title, const QString& message, QWidget* parent=nullptr)
        : QDialog(parent)
    {
        setModal(true);
        setWindowTitle("Succès");
        setFixedSize(580, 380);
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);

        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);

        // Main container with rounded corners and shadow
        QFrame* container = new QFrame;
        container->setObjectName("successContainer");
        QVBoxLayout* containerLayout = new QVBoxLayout(container);
        containerLayout->setContentsMargins(0, 0, 0, 0);
        containerLayout->setSpacing(0);

        // Top gradient header with icon
        QFrame* header = new QFrame;
        header->setObjectName("successHeader");
        header->setFixedHeight(180);
        QVBoxLayout* headerLayout = new QVBoxLayout(header);
        headerLayout->setContentsMargins(0, 0, 0, 0);
        headerLayout->setAlignment(Qt::AlignCenter);

        // Success icon (checkmark circle)
        QLabel* iconLabel = new QLabel;
        iconLabel->setObjectName("successIcon");
        iconLabel->setFixedSize(100, 100);
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setText("✓");
        headerLayout->addWidget(iconLabel, 0, Qt::AlignCenter);

        // Content area
        QFrame* content = new QFrame;
        content->setObjectName("successContent");
        QVBoxLayout* contentLayout = new QVBoxLayout(content);
        contentLayout->setContentsMargins(40, 30, 40, 30);
        contentLayout->setSpacing(16);
        contentLayout->setAlignment(Qt::AlignCenter);

        // Title
        QLabel* titleLabel = new QLabel(title);
        titleLabel->setObjectName("successTitle");
        titleLabel->setAlignment(Qt::AlignCenter);
        titleLabel->setWordWrap(true);
        QFont titleFont;
        titleFont.setPointSize(24);
        titleFont.setBold(true);
        titleLabel->setFont(titleFont);

        // Message
        QLabel* messageLabel = new QLabel(message);
        messageLabel->setObjectName("successMessage");
        messageLabel->setAlignment(Qt::AlignCenter);
        messageLabel->setWordWrap(true);
        QFont messageFont;
        messageFont.setPointSize(13);
        messageLabel->setFont(messageFont);

        contentLayout->addWidget(titleLabel);
        contentLayout->addWidget(messageLabel);
        contentLayout->addSpacing(10);

        // OK Button
        QPushButton* okButton = new QPushButton("Continuer");
        okButton->setObjectName("successButton");
        okButton->setCursor(Qt::PointingHandCursor);
        okButton->setFixedHeight(50);
        okButton->setFixedWidth(220);
        QFont btnFont;
        btnFont.setPointSize(13);
        btnFont.setBold(true);
        okButton->setFont(btnFont);
        connect(okButton, &QPushButton::clicked, this, &QDialog::accept);

        contentLayout->addWidget(okButton, 0, Qt::AlignCenter);

        containerLayout->addWidget(header);
        containerLayout->addWidget(content);

        mainLayout->addWidget(container);

        // Apply modern stylesheet
        setStyleSheet(QString(R"(
            QFrame#successContainer {
                background: white;
                border-radius: 24px;
                border: 2px solid rgba(46, 111, 99, 0.2);
            }

            QFrame#successHeader {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                    stop:0 rgba(46, 111, 99, 1),
                    stop:0.7 rgba(10, 95, 88, 1),
                    stop:1 rgba(18, 68, 59, 1));
                border-top-left-radius: 22px;
                border-top-right-radius: 22px;
            }

            QLabel#successIcon {
                background: white;
                border-radius: 50px;
                color: #2E6F63;
                font-size: 64px;
                font-weight: bold;
                border: 4px solid rgba(255, 255, 255, 0.3);
            }

            QFrame#successContent {
                background: white;
                border-bottom-left-radius: 22px;
                border-bottom-right-radius: 22px;
            }

            QLabel#successTitle {
                color: #2E6F63;
            }

            QLabel#successMessage {
                color: rgba(0, 0, 0, 0.65);
            }

            QPushButton#successButton {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                    stop:0 #2E6F63,
                    stop:1 #0A5F58);
                color: white;
                border: none;
                border-radius: 25px;
                padding: 14px 40px;
            }

            QPushButton#successButton:hover {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                    stop:0 #3A8275,
                    stop:1 #12443B);
            }

            QPushButton#successButton:pressed {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                    stop:0 #12443B,
                    stop:1 #0A5F58);
            }
        )"));

        // Add drop shadow effect
        QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect;
        shadow->setBlurRadius(40);
        shadow->setOffset(0, 10);
        shadow->setColor(QColor(0, 0, 0, 80));
        container->setGraphicsEffect(shadow);
    }
};

namespace {

QString normalizeRememberedEmail(const QString& email)
{
    return email.trimmed().toLower();
}

QString encodeRememberedPassword(const QString& pass)
{
    return QString::fromLatin1(pass.toUtf8().toBase64(QByteArray::Base64Encoding));
}

QString decodeRememberedPassword(const QString& encoded)
{
    const QByteArray decoded = QByteArray::fromBase64(encoded.toLatin1());
    return QString::fromUtf8(decoded);
}

QMap<QString, QString> loadRememberedAccounts()
{
    QMap<QString, QString> out;
    QSettings settings("SmartVision", "BioSimple");
    const QString raw = settings.value("login/rememberedAccounts").toString();
    if (raw.trimmed().isEmpty()) return out;

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(raw.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray()) return out;

    const QJsonArray arr = doc.array();
    for (const QJsonValue& v : arr) {
        if (!v.isObject()) continue;
        const QJsonObject obj = v.toObject();
        const QString email = normalizeRememberedEmail(obj.value("email").toString());
        if (email.isEmpty()) continue;
        const QString pass = decodeRememberedPassword(obj.value("password").toString());
        out[email] = pass;
    }
    return out;
}

void saveRememberedAccounts(const QMap<QString, QString>& accounts)
{
    QJsonArray arr;
    for (auto it = accounts.constBegin(); it != accounts.constEnd(); ++it) {
        QJsonObject obj;
        obj["email"] = it.key();
        obj["password"] = encodeRememberedPassword(it.value());
        arr.append(obj);
    }

    QSettings settings("SmartVision", "BioSimple");
    settings.setValue("login/rememberedAccounts",
                      QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact)));
}

}

// ===================== LoginWindow =====================
class LoginWindow : public QWidget
{
public:
    explicit LoginWindow(QWidget *parent = nullptr) : QWidget(parent), passwordVisible(false)
    {
        setWindowTitle("SmartVision - Connexion");
        setMinimumSize(1100, 650);

        auto *root = new QVBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);
        root->setSpacing(0);

        // ============ Background label ============
        bgLabel = new QLabel(this);
        bgLabel->setScaledContents(true);
        bgLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        root->addWidget(bgLabel);

        // Overlay above background
        overlay = new QWidget(bgLabel);
        overlay->setObjectName("overlay");
        overlay->setAttribute(Qt::WA_StyledBackground, true);

        auto *overlayLayout = new QVBoxLayout(overlay);
        overlayLayout->setContentsMargins(0, 0, 0, 0);
        overlayLayout->setAlignment(Qt::AlignCenter);

        // ============ Card ============
        auto *card = new QFrame(overlay);
        card->setObjectName("card");
        card->setFixedWidth(560);

        auto *shadow = new QGraphicsDropShadowEffect(card);
        shadow->setBlurRadius(40);
        shadow->setOffset(0, 10);
        shadow->setColor(QColor(0, 0, 0, 70));
        card->setGraphicsEffect(shadow);

        overlayLayout->addWidget(card);

        auto *cardLay = new QVBoxLayout(card);
        cardLay->setContentsMargins(38, 34, 38, 26);
        cardLay->setSpacing(14);

        // ============ Header (logo + title) ============
        auto *topRow = new QHBoxLayout();
        topRow->setSpacing(14);

        logoLabel = new QLabel(card);
        logoLabel->setObjectName("logo");
        QPixmap logoPx(":/image/smartvision.png");
        if (!logoPx.isNull()) {
            logoLabel->setPixmap(logoPx.scaled(56, 56, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            logoLabel->setText("LOGO");
        }
        logoLabel->setFixedSize(56, 56);

        auto *titleCol = new QVBoxLayout();
        titleCol->setSpacing(2);

        titleLabel = new QLabel("SmartVision", card);
        titleLabel->setObjectName("title");

        subtitleLabel = new QLabel("Plateforme de gestion de la recherche scientifique", card);
        subtitleLabel->setObjectName("subtitle");

        titleCol->addWidget(titleLabel);
        titleCol->addWidget(subtitleLabel);

        topRow->addWidget(logoLabel);
        topRow->addLayout(titleCol);
        topRow->addStretch(1);

        cardLay->addLayout(topRow);

        // ============ Welcome ============
        auto *welcome = new QLabel("Bon retour", card);
        welcome->setObjectName("welcome");
        welcome->setAlignment(Qt::AlignCenter);

        auto *hint = new QLabel("Veuillez vous connecter pour continuer.", card);
        hint->setObjectName("hint");
        hint->setAlignment(Qt::AlignCenter);

        cardLay->addSpacing(4);
        cardLay->addWidget(welcome);
        cardLay->addWidget(hint);
        cardLay->addSpacing(6);

        // ============ Inputs ============
        emailEdit = new QLineEdit(card);
        emailEdit->setObjectName("input");
        emailEdit->setPlaceholderText("Adresse e-mail");

        rememberedEmailsList = new QListWidget(card);
        rememberedEmailsList->setObjectName("rememberedEmails");
        rememberedEmailsList->setMaximumHeight(118);
        rememberedEmailsList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        rememberedEmailsList->setVisible(false);

        auto *passRow = new QHBoxLayout();
        passRow->setSpacing(10);

        passEdit = new QLineEdit(card);
        passEdit->setObjectName("input");
        passEdit->setPlaceholderText("Mot de passe");
        passEdit->setEchoMode(QLineEdit::Password);

        showPassBtn = new QPushButton("Afficher", card);
        showPassBtn->setObjectName("btnGhost");
        showPassBtn->setFixedWidth(110);
        connect(showPassBtn, &QPushButton::clicked, this, &LoginWindow::togglePassword);

        passRow->addWidget(passEdit);
        passRow->addWidget(showPassBtn);

        cardLay->addWidget(emailEdit);
        cardLay->addWidget(rememberedEmailsList);
        cardLay->addLayout(passRow);

        rememberedAccounts = loadRememberedAccounts();
        updateRememberedEmailsList(QString());
        emailEdit->installEventFilter(this);

        connect(emailEdit, &QLineEdit::textChanged, this, [=](const QString& text){
            const QString normalized = normalizeRememberedEmail(text);
            updateRememberedEmailsList(text);
            if (rememberedAccounts.contains(normalized)) {
                passEdit->setText(rememberedAccounts.value(normalized));
                rememberCheck->setChecked(true);
            }
        });

        connect(rememberedEmailsList, &QListWidget::itemClicked, this, [=](QListWidgetItem* item){
            if (!item) return;
            const QString email = item->text();
            emailEdit->setText(email);
            const QString key = normalizeRememberedEmail(email);
            passEdit->setText(rememberedAccounts.value(key));
            rememberCheck->setChecked(true);
            rememberedEmailsList->hide();
            passEdit->setFocus();
        });

        // ============ Remember + forgot ============
        // ============ CAPTCHA ============
        captchaWidget = new CaptchaWidget(card);

        QLabel* captchaHint = new QLabel(
            "⚠  Vérification requise — tapez les 5 caractères affichés.", card);
        captchaHint->setObjectName("hint");
        captchaHint->setWordWrap(true);

        cardLay->addWidget(captchaWidget);
        cardLay->addWidget(captchaHint);

        // ============ Remember + forgot ============
        auto *row2 = new QHBoxLayout();
        row2->setSpacing(10);

        rememberCheck = new QCheckBox("Se souvenir de moi", card);
        rememberCheck->setObjectName("remember");

        forgotBtn = new QPushButton("Mot de passe oublié ?", card);
        forgotBtn->setObjectName("btnLink");
        forgotBtn->setCursor(Qt::PointingHandCursor);

        row2->addWidget(rememberCheck);
        row2->addStretch(1);
        row2->addWidget(forgotBtn);

        cardLay->addLayout(row2);

        // ============ Login button ============
        loginBtn = new QPushButton("Se connecter", card);
        loginBtn->setObjectName("btnPrimary");
        loginBtn->setCursor(Qt::PointingHandCursor);
        loginBtn->setFixedHeight(48);

        cardLay->addSpacing(6);
        cardLay->addWidget(loginBtn);

        googleLoginBtn = new QPushButton("Se connecter avec Google", card);
        googleLoginBtn->setObjectName("btnGoogle");
        googleLoginBtn->setCursor(Qt::PointingHandCursor);
        googleLoginBtn->setFixedHeight(42);
        cardLay->addWidget(googleLoginBtn);

        // ============ Face ID actions ============
        faceLoginBtn = new QPushButton("Connexion Face ID", card);
        faceLoginBtn->setObjectName("btnFace");
        faceLoginBtn->setCursor(Qt::PointingHandCursor);
        faceLoginBtn->setFixedHeight(38);
        faceLoginBtn->setFixedWidth(180);
        cardLay->addWidget(faceLoginBtn, 0, Qt::AlignCenter);

        faceRegisterBtn = new QPushButton("Enregistrer / mettre a jour Face ID (navigateur)", card);
        faceRegisterBtn->setObjectName("btnFaceLink");
        faceRegisterBtn->setCursor(Qt::PointingHandCursor);
        cardLay->addWidget(faceRegisterBtn, 0, Qt::AlignCenter);

        // ============ Create account link ============
        createBtn = new QPushButton("Nouveau sur SmartVision ?  Créer un compte", card);
        createBtn->setObjectName("btnLink");
        createBtn->setCursor(Qt::PointingHandCursor);
        cardLay->addWidget(createBtn, 0, Qt::AlignCenter);

        // Footer
        auto *footer = new QLabel("Besoin d'aide ?  |  Politique de confidentialité  |  Conditions d'utilisation", card);
        footer->setObjectName("footer");
        footer->setAlignment(Qt::AlignCenter);

        cardLay->addSpacing(6);
        cardLay->addWidget(footer);

        // ============ Style ============
        QFont f("Inter");
        f.setPointSize(11);
        setFont(f);

        static const QString C_LOGIN_DARK  = "#12443B";
        static const QString C_LOGIN_MAIN  = "#0A5F58";
        static const QString C_LOGIN_TEXT  = "#64533A";

        setStyleSheet(QString(R"(
            QWidget#overlay { background: transparent; }

            QFrame#card {
                background: rgba(246, 248, 247, 0.88);
                border: 1px solid rgba(0,0,0,0.10);
                border-radius: 18px;
            }

            QLabel#title {
                color: %1;
                font-size: 26px;
                font-weight: 700;
            }

            QLabel#subtitle {
                color: rgba(100,83,58,0.70);
                font-size: 12px;
            }

            QLabel#welcome {
                color: %1;
                font-size: 28px;
                font-weight: 700;
            }

            QLabel#hint {
                color: rgba(100,83,58,0.75);
                font-size: 13px;
            }

            QLineEdit#input {
                background: rgba(255,255,255,0.90);
                border: 1px solid rgba(18,68,59,0.22);
                border-radius: 10px;
                padding: 12px 12px;
                color: %3;
                font-size: 13px;
            }
            QLineEdit#input:focus { border: 2px solid %2; }

            QListWidget#rememberedEmails {
                background: rgba(255,255,255,0.95);
                border: 1px solid rgba(18,68,59,0.24);
                border-radius: 10px;
                padding: 4px;
                color: %3;
                font-size: 12.5px;
            }
            QListWidget#rememberedEmails::item {
                padding: 8px 10px;
                border-radius: 8px;
            }
            QListWidget#rememberedEmails::item:hover {
                background: rgba(10,95,88,0.12);
            }
            QListWidget#rememberedEmails::item:selected {
                background: rgba(10,95,88,0.18);
                color: %1;
            }

            QCheckBox#remember { color: rgba(100,83,58,0.85); spacing: 10px; }

            QPushButton#btnPrimary {
                background: %2;
                color: white;
                border: none;
                border-radius: 12px;
                font-size: 15px;
                font-weight: 600;
            }
            QPushButton#btnPrimary:hover { background: %1; }

            QPushButton#btnGoogle {
                background: rgba(255,255,255,0.96);
                color: #4a4a4a;
                border: 1px solid rgba(0,0,0,0.18);
                border-radius: 12px;
                font-size: 14px;
                font-weight: 700;
                padding: 8px 14px;
            }
            QPushButton#btnGoogle:hover {
                background: rgba(255,255,255,1);
                border: 1px solid rgba(10,95,88,0.35);
                color: #0A5F58;
            }

            QPushButton#btnFace {
                background: rgba(228, 236, 234, 0.92);
                color: #2a6761;
                border: 1px solid rgba(10,95,88,0.30);
                border-radius: 12px;
                font-size: 13.5px;
                font-weight: 700;
                padding: 6px 18px;
            }
            QPushButton#btnFace:hover {
                background: rgba(214, 228, 225, 0.98);
                border: 1px solid rgba(10,95,88,0.42);
            }

            QPushButton#btnFaceLink {
                background: transparent;
                color: rgba(10,95,88,0.82);
                border: none;
                font-size: 12px;
                padding: 2px 6px;
            }
            QPushButton#btnFaceLink:hover { text-decoration: underline; }

            QPushButton#btnLink {
                background: transparent;
                color: %2;
                border: none;
                font-size: 12.5px;
                padding: 6px 6px;
                text-align: center;
            }
            QPushButton#btnLink:hover { text-decoration: underline; }

            QPushButton#btnGhost {
                background: rgba(198,178,154,0.35);
                color: %3;
                border: 1px solid rgba(100,83,58,0.18);
                border-radius: 10px;
                padding: 8px 10px;
            }
            QPushButton#btnGhost:hover { background: rgba(198,178,154,0.55); }

            QLabel#footer { color: rgba(100,83,58,0.60); font-size: 11px; }
        )").arg(C_LOGIN_DARK, C_LOGIN_MAIN, C_LOGIN_TEXT));

        updateBackground();
    }

    QPushButton* getLoginButton() const { return loginBtn; }
    QPushButton* getGoogleLoginButton() const { return googleLoginBtn; }
    QPushButton* getCreateAccountButton() const { return createBtn; }
    QPushButton* getFaceLoginButton() const { return faceLoginBtn; }
    QPushButton* getFaceRegisterButton() const { return faceRegisterBtn; }
    QString getEmail() const { return emailEdit->text().trimmed(); }
    QString getPassword() const { return passEdit->text(); }
    bool isRemembered() const { return rememberCheck->isChecked(); }

    void updateRememberedCredentials(const QString& email, const QString& password, bool remember)
    {
        const QString key = normalizeRememberedEmail(email);
        if (key.isEmpty()) return;

        if (remember) {
            rememberedAccounts[key] = password;
            rememberCheck->setChecked(true);
        } else {
            rememberedAccounts.remove(key);
        }

        saveRememberedAccounts(rememberedAccounts);
        updateRememberedEmailsList(emailEdit ? emailEdit->text() : QString());
    }

    void clearFields() {
        emailEdit->clear();
        passEdit->clear();
        rememberCheck->setChecked(false);
        rememberedEmailsList->hide();
    }

    // Public members for styling + external access
    QLineEdit *emailEdit = nullptr;
    QLineEdit *passEdit = nullptr;

    // CAPTCHA public getter
    CaptchaWidget* getCaptchaWidget() const { return captchaWidget; }
protected:
    bool eventFilter(QObject *obj, QEvent *event) override
    {
        if (obj == emailEdit && (event->type() == QEvent::FocusIn || event->type() == QEvent::MouseButtonPress)) {
            updateRememberedEmailsList(emailEdit->text());
        }
        return QWidget::eventFilter(obj, event);
    }

    void resizeEvent(QResizeEvent *event) override
    {
        QWidget::resizeEvent(event);
        updateBackground();
        if (overlay && bgLabel) {
            overlay->setGeometry(0, 0, bgLabel->width(), bgLabel->height());
        }
    }

private:
    void updateBackground()
    {
        if (!bgLabel) return;

        QPixmap bg(":/image/background2.png");
        if (!bg.isNull()) {
            bgLabel->setPixmap(bg.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        } else {
            bgLabel->setStyleSheet("background: #A3CAD3;");
        }
    }

    void togglePassword()
    {
        passwordVisible = !passwordVisible;
        passEdit->setEchoMode(passwordVisible ? QLineEdit::Normal : QLineEdit::Password);
        showPassBtn->setText(passwordVisible ? "Masquer" : "Afficher");
    }

    void updateRememberedEmailsList(const QString& filterText)
    {
        if (!rememberedEmailsList) return;

        rememberedEmailsList->clear();
        const QString filter = normalizeRememberedEmail(filterText);

        for (auto it = rememberedAccounts.constBegin(); it != rememberedAccounts.constEnd(); ++it) {
            if (filter.isEmpty() || it.key().contains(filter)) {
                rememberedEmailsList->addItem(it.key());
            }
        }

        const bool shouldShow = emailEdit && emailEdit->hasFocus() && rememberedEmailsList->count() > 0;
        rememberedEmailsList->setVisible(shouldShow);
    }

    // Components
    // Components
    QLabel *bgLabel = nullptr;
    QWidget *overlay = nullptr;
    QLabel *logoLabel = nullptr;
    QLabel *titleLabel = nullptr;
    QLabel *subtitleLabel = nullptr;
    QPushButton *showPassBtn = nullptr;
    QCheckBox *rememberCheck = nullptr;
    QPushButton *loginBtn = nullptr;
    QPushButton *googleLoginBtn = nullptr;
    QPushButton *forgotBtn = nullptr;
    QPushButton *createBtn = nullptr;
    QPushButton *faceLoginBtn = nullptr;
    QPushButton *faceRegisterBtn = nullptr;
    QListWidget *rememberedEmailsList = nullptr;
    QMap<QString, QString> rememberedAccounts;
    bool passwordVisible = false;
    CaptchaWidget* captchaWidget = nullptr;   // ← ADD THIS LINE
};

// ===================== MainWindow =====================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Build the full UI: root, stacked pages, and navigation wiring.
    if (QScreen* screen = QGuiApplication::primaryScreen()) {
        const QRect available = screen->availableGeometry();
        const int initialWidth = qBound(980, available.width() - 120, 1500);
        const int initialHeight = qBound(620, available.height() - 120, 920);
        resize(initialWidth, initialHeight);
    } else {
        resize(1180, 700);
    }
    setMinimumSize(900, 580);

    QWidget* root = new QWidget(this);
    root->setObjectName("root");
    setCentralWidget(root);

    QStyle* st = style();

    // ── BioSimple CRUD instance + edit-mode state ──
    CrudeBioSimple* crud     = new CrudeBioSimple;
    bool*    bioEditMode     = new bool(false);
    QString* bioEditRef      = new QString;

    // ── Rôle de l'utilisateur connecté ──
    // Valeurs possibles : "Responsable", "Chercheur", "Technicien", "RH"
    QString* currentRole = new QString("Responsable");
    QString* currentUserFullName = new QString;
    QString* currentUserEmail = new QString;
    QString* smtpConfigError = new QString;
    QSet<QString>* smtpUiErrorsShown = new QSet<QString>;
    // Sera défini après la création de tous les boutons (fin du constructeur)
    std::function<void()>* applyPerms = new std::function<void()>();

    auto logoutCurrentUser = [=]() {
        if (!currentUserFullName->isEmpty() || !currentUserEmail->isEmpty()) {
            TracabiliteManager::logDeconnexion(*currentUserFullName, *currentUserEmail);
        }
        TracabiliteManager::clearUserContext();
        currentUserFullName->clear();
        currentUserEmail->clear();
        smtpUiErrorsShown->clear();
    };

    auto applyTheme = [=](bool dark){
        const QString bg         = dark ? "#22272E" : C_BG;
        const QString text       = dark ? "#E8ECF0"  : C_TEXT_DARK;
        const QString inputBg    = dark ? "rgba(255,255,255,0.08)" : "rgba(255,255,255,0.65)";
        const QString comboBg    = dark ? "rgba(255,255,255,0.10)" : "rgba(255,255,255,0.45)";
        const QString border     = dark ? "rgba(210,218,226,0.22)" : "rgba(0,0,0,0.12)";
        const QString headerBg   = dark ? "#3A4149" : C_TABLE_HDR;
        const QString headerText = dark ? "#E8ECF0"  : "rgba(0,0,0,0.60)";
        const QString gridColor  = dark ? "rgba(210,218,226,0.15)" : "rgba(0,0,0,0.10)";
        const QString tableBg    = dark ? "#1A1E24" : C_ROW_ODD;
        const QString tableAltBg = dark ? "#1E2328" : C_ROW_EVEN;
        const QString tableText  = dark ? "#E8ECF0"  : "rgba(0,0,0,0.70)";

        root->setStyleSheet(QString(R"(
            #root { background:%1; }
            QLabel { color: %2; }
            QLineEdit {
                background: %3;
                border: 1px solid %4;
                border-radius: 12px;
                padding: 11px 14px;
                min-height: 20px;
                color: %2;
            }
            QComboBox{
                background: %5;
                border: 1px solid %4;
                border-radius: 10px;
                padding: 10px 14px;
                color: %2;
                min-width: 92px;
                font-weight: 800;
            }
            QComboBox::drop-down{ border: 0px; width: 22px; }

            QHeaderView::section {
                background: %6;
                color: %7;
                border: none;
                padding: 10px 10px;
                font-weight: 800;
            }
            QTableWidget{
                background: %9;
                alternate-background-color: %10;
                border: none;
                gridline-color: %8;
                selection-background-color: rgba(10,95,88,0.25);
                selection-color: %11;
            }
            QTableWidget::item{ padding: 10px; color: %11; }

            QTreeWidget{ background: transparent; border: none; }
            QTreeWidget::item{
                padding: 7px; margin: 2px 4px; color: %2;
                font-weight: 900; border-radius: 10px;
            }
            QTreeWidget::item:selected{ background: rgba(10,95,88,0.28); }
            QListWidget{ background: transparent; border:none; }
        )").arg(bg, text, inputBg, border, comboBg, headerBg, headerText, gridColor,
                 tableBg, tableAltBg, tableText));
    };
    g_applyThemeFn = applyTheme;
    applyTheme(g_darkThemeEnabled);

    // Stacking layout: syringe animation behind all content
    QStackedLayout* rootSL = new QStackedLayout(root);
    rootSL->setStackingMode(QStackedLayout::StackAll);
    rootSL->setContentsMargins(0, 0, 0, 0);

    SyringeBackground* bgAnim = new SyringeBackground;
    rootSL->addWidget(bgAnim);

    QWidget* mainContent = new QWidget;
    mainContent->setAutoFillBackground(false);
    rootSL->addWidget(mainContent);
    rootSL->setCurrentIndex(1);

    QVBoxLayout* rootLayout = new QVBoxLayout(mainContent);
    rootLayout->setContentsMargins(0,0,0,0);
    rootLayout->setSpacing(0);

    // Create global modules bar (invisible - used only for tracking state)
    ModulesBar* globalBar = new ModulesBar(makeModulesBar(ModuleTab::Employee));
    g_globalBar = globalBar;
    syncThemeToggleButtons();

    QStackedWidget* stack = new QStackedWidget;
    rootLayout->addWidget(stack);

    auto addStackPage = [=](QWidget* page) {
        QScrollArea* pageScroll = new QScrollArea;
        pageScroll->setObjectName("stackPageScroll");
        pageScroll->setWidgetResizable(true);
        pageScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        pageScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        pageScroll->setFrameShape(QFrame::NoFrame);
        pageScroll->setStyleSheet(R"(
            QScrollArea#stackPageScroll {
                background: transparent;
                border: none;
            }
            QScrollArea#stackPageScroll > QWidget > QWidget {
                background: transparent;
            }
            QScrollBar:vertical {
                background: rgba(0,0,0,0.06);
                width: 6px;
                border-radius: 3px;
                margin: 0px;
            }
            QScrollBar::handle:vertical {
                background: rgba(45,212,191,0.50);
                border-radius: 3px;
                min-height: 30px;
            }
            QScrollBar::handle:vertical:hover {
                background: rgba(45,212,191,0.80);
            }
            QScrollBar::add-line:vertical,
            QScrollBar::sub-line:vertical { height: 0px; }
            QScrollBar::add-page:vertical,
            QScrollBar::sub-page:vertical { background: none; }
        )");
        pageScroll->setWidget(page);
        stack->addWidget(pageScroll);
    };

    QObject::connect(EmailSender::instance(), &EmailSender::sent, this,
                     [=](const QString& to) {
        appendSmtpLog(QString("[UI] Email sent successfully to=%1").arg(maskSmtpIdentity(to)));
    });
    QObject::connect(EmailSender::instance(), &EmailSender::failed, this,
                     [=](const QString& to, const QString& reason) {
        const QString cleanReason = normalizeSmtpUiReason(reason);
        appendSmtpLog(QString("[UI] Email send failed to=%1 reason=%2")
                          .arg(maskSmtpIdentity(to), cleanReason));

        const QString dedupeKey = cleanReason;
        if (smtpUiErrorsShown->contains(dedupeKey)) return;
        smtpUiErrorsShown->insert(dedupeKey);

        const QString recipientLabel = to.trimmed().isEmpty()
                                       ? QString()
                                       : QString(" vers %1").arg(to.trimmed());
        showToast(this,
                  QString("Échec d'envoi d'e-mail%1 : %2").arg(recipientLabel, cleanReason),
                  false);
    });

    // ---- Global button hover color animation ----
    if (QCoreApplication* app = QCoreApplication::instance()) {
        app->installEventFilter(new ButtonAnimator(this));
    }

    // ==========================================================
    // PAGE 0 : LOGIN
    // ==========================================================
    LoginWindow* loginPage = new LoginWindow;
    addStackPage(loginPage);

    SignupServer* signupServer = new SignupServer(this);
    signupServer->start();

    QObject::connect(loginPage->getCreateAccountButton(), &QPushButton::clicked, this, [=]() {
        QDesktopServices::openUrl(signupServer->signupUrl());
    });

    QObject::connect(loginPage->getGoogleLoginButton(), &QPushButton::clicked, this, [=]() {

        // ── 1. Lire les credentials Google (sans PHP) ─────────────
        // Priorité : variable d'environnement → fichier INI local
        QString clientId     = QString::fromLocal8Bit(qgetenv("GOOGLE_CLIENT_ID")).trimmed();
        QString clientSecret = QString::fromLocal8Bit(qgetenv("GOOGLE_CLIENT_SECRET")).trimmed();

        // Chercher un fichier google_oauth.ini dans le dossier source / exécutable
        if (clientId.isEmpty()) {
            const QStringList searchDirs = {
                QCoreApplication::applicationDirPath(),
                QDir::currentPath()
            };
            for (const QString& dir : searchDirs) {
                const QString ini = dir + "/google_oauth.ini";
                if (QFileInfo::exists(ini)) {
                    QSettings cfg(ini, QSettings::IniFormat);
                    clientId     = cfg.value("google/client_id").toString().trimmed();
                    clientSecret = cfg.value("google/client_secret").toString().trimmed();
                    break;
                }
            }
        }

        if (clientId.isEmpty()) {
            QMessageBox::information(this,
                "Connexion Google",
                "La connexion Google n'est pas configurée sur ce poste.\n\n"
                "Pour l'activer, créez le fichier google_oauth.ini\n"
                "dans le dossier de l'application avec :\n\n"
                "[google]\n"
                "client_id     = VOTRE_CLIENT_ID.apps.googleusercontent.com\n"
                "client_secret = VOTRE_CLIENT_SECRET\n\n"
                "Ou définissez les variables d'environnement :\n"
                "  GOOGLE_CLIENT_ID\n"
                "  GOOGLE_CLIENT_SECRET");
            return;
        }

        // ── 2. Enregistrer les credentials dans le serveur local ──
        signupServer->setGoogleCredentials(clientId, clientSecret);

        // ── 3. Construire l'URL Google OAuth directement ──────────
        const QString callbackUrl = signupServer->googleCallbackUrl();

        QUrlQuery params;
        params.addQueryItem("client_id",     clientId);
        params.addQueryItem("redirect_uri",  callbackUrl);
        params.addQueryItem("response_type", "code");
        params.addQueryItem("scope",         "openid email profile");
        params.addQueryItem("access_type",   "offline");
        params.addQueryItem("prompt",        "consent");

        QUrl authUrl("https://accounts.google.com/o/oauth2/v2/auth");
        authUrl.setQuery(params);

        // ── 4. Ouvrir dans le navigateur par défaut ───────────────
        if (!QDesktopServices::openUrl(authUrl)) {
#ifdef Q_OS_WIN
            QProcess::startDetached("cmd", {"/c", "start", "", authUrl.toString(QUrl::FullyEncoded)});
#endif
        }
    });

    QObject::connect(loginPage->getFaceLoginButton(), &QPushButton::clicked, this, [=]() {
        QUrl url = signupServer->signupUrl();
        url.setPath("/face-verify");
        if (!QDesktopServices::openUrl(url)) {
            QMessageBox::warning(this, "Face ID", "Impossible d'ouvrir la page Face ID : " + url.toString());
        }
    });

    QObject::connect(loginPage->getFaceRegisterButton(), &QPushButton::clicked, this, [=]() {
        QUrl url = signupServer->signupUrl();
        url.setPath("/face-register");
        if (!QDesktopServices::openUrl(url)) {
            QMessageBox::warning(this, "Face ID", "Impossible d'ouvrir la page d'enregistrement Face ID : " + url.toString());
        }
    });

    signupServer->setFaceLoginSucceededHandler([=](const QString& identity) {
        *currentUserFullName = identity.trimmed();
        *currentUserEmail = identity.contains('@') ? identity.trimmed() : QString();

        // Fetch role from DB using email if available, otherwise default to Responsable
        if (!currentUserEmail->isEmpty()) {
            QSqlDatabase db = QSqlDatabase::database();
            if (db.isOpen()) {
                QSqlQuery roleQ(db);
                roleQ.prepare(
                    "SELECT NVL(\"ROLE\", 'Responsable') FROM \"Employés\" "
                    "WHERE LOWER(\"EMAIL\") = LOWER(?) AND \"ACTIVE\" = 'O'");
                roleQ.addBindValue(*currentUserEmail);
                if (roleQ.exec() && roleQ.next()) {
                    const QString rawRole = roleQ.value(0).toString().trimmed();
                    if      (rawRole.contains("Responsable", Qt::CaseInsensitive)) *currentRole = "Responsable";
                    else if (rawRole.contains("Chercheur",   Qt::CaseInsensitive)) *currentRole = "Chercheur";
                    else if (rawRole.contains("Technicien",  Qt::CaseInsensitive)) *currentRole = "Technicien";
                    else if (rawRole.contains("RH",          Qt::CaseInsensitive)) *currentRole = "RH";
                    else                                                            *currentRole = "Responsable";
                }
            }
        }
        if (g_voiceCmd) g_voiceCmd->setCurrentRole(*currentRole);

        TracabiliteManager::setUserContext(*currentUserFullName, *currentUserEmail);
        TracabiliteManager::logConnexion(*currentUserFullName, *currentUserEmail, "Face ID");

        const bool smtpConfigured = configureEmailSenderFromLocalConfig(smtpConfigError);
        if (!smtpConfigured) {
            appendSmtpLog(QString("[CONFIG] Face ID login: notifications disabled reason=%1").arg(*smtpConfigError));
            showToast(this,
                      QString("Notifications e-mail désactivées : %1. Vérifiez smtp_mail.ini (host, port, username, password).")
                          .arg(smtpConfigError->isEmpty()
                                   ? QString("configuration SMTP manquante")
                                   : *smtpConfigError),
                      false);
        }

        // Apply role-based permissions on all buttons/tabs
        if (*applyPerms) (*applyPerms)();

        // Navigate to first visible module for this role
        ModuleTab firstVisibleTab = ModuleTab::Employee;
        if (firstVisibleModuleTab(*globalBar, &firstVisibleTab)) {
            syncModuleSelection(firstVisibleTab);
            setWindowTitle(windowTitleForModule(firstVisibleTab));
            stack->setCurrentIndex(rootPageForModule(firstVisibleTab));
        } else {
            clearAllModuleSelections();
            stack->setCurrentIndex(LOGIN);
        }

        // Bring window to front — must happen before the dialog
        this->showNormal();
        this->raise();
        this->activateWindow();

        // Use a non-blocking toast so the app is immediately navigable
        // even if the browser window is still in the foreground
        showToast(this, QString("Connexion Face ID réussie : %1").arg(*currentUserFullName), true);
    });

    signupServer->setGoogleLoginSucceededHandler([=](const QString& identity) {
        *currentUserFullName = identity.trimmed();
        *currentUserEmail = identity.contains('@') ? identity.trimmed() : QString();
        TracabiliteManager::setUserContext(*currentUserFullName, *currentUserEmail);
        TracabiliteManager::logConnexion(*currentUserFullName, *currentUserEmail, "Google");

        const bool smtpConfigured = configureEmailSenderFromLocalConfig(smtpConfigError);
        if (!smtpConfigured) {
            appendSmtpLog(QString("[CONFIG] Google login: notifications disabled reason=%1").arg(*smtpConfigError));
            showToast(this,
                      QString("Notifications e-mail désactivées : %1. Vérifiez smtp_mail.ini (host, port, username, password).")
                          .arg(smtpConfigError->isEmpty()
                                   ? QString("configuration SMTP manquante")
                                   : *smtpConfigError),
                      false);
        }

        setWindowTitle("Employés");
        syncModuleSelection(ModuleTab::Employee);
        stack->setCurrentIndex(EMP_LIST);

        this->showNormal();
        this->raise();
        this->activateWindow();

        QMessageBox::information(this,
                                 "Google OAuth",
                                 "Connexion Google reussie pour : " + identity);
    });

    // Handle login button
    QObject::connect(loginPage->getLoginButton(), &QPushButton::clicked, this, [=](){
        const QString email = loginPage->getEmail();
        const QString pass = loginPage->getPassword();

        if (email.isEmpty() || pass.isEmpty()) {
            loginPage->emailEdit->setPlaceholderText("Champ requis!");
            loginPage->passEdit->setPlaceholderText("Champ requis!");
            return;
        }

        // ── CAPTCHA validation ──────────────────────────────────────
        if (!loginPage->getCaptchaWidget()->validate()) {
            QMessageBox::warning(this,
                "CAPTCHA incorrect",
                "Le code de vérification est incorrect.\n"
                "Veuillez saisir les caractères affichés.");
            loginPage->getCaptchaWidget()->refresh();
            return;
        }
        // ────────────────────────────────────────────────────────────
        QSqlDatabase db = QSqlDatabase::database();
        if (!db.isOpen()) {
            QMessageBox::critical(this, "Connexion BD", "La base de données n'est pas connectée.");
            return;
        }

        // ── Authentification + récupération du rôle et du nom complet ──
        QSqlQuery authQuery(db);
        authQuery.prepare(
            "SELECT NVL(\"FULL_NAME\", TRIM(\"prenom\" || ' ' || \"nom\")) AS full_name, "
            "       NVL(\"ROLE\", 'Responsable') AS role_name "
            "FROM \"Employés\" "
            "WHERE LOWER(\"EMAIL\") = LOWER(?) "
            "AND \"USER_PASSWORD\" = ? "
            "AND \"ACTIVE\" = 'O'");
        authQuery.addBindValue(email);
        authQuery.addBindValue(pass);

        if (!authQuery.exec()) {
            QMessageBox::critical(this,
                                  "Erreur SQL",
                                  "Erreur lors de la vérification des identifiants :\n" + authQuery.lastError().text());
            return;
        }

        bool isAuthenticated = authQuery.next();
        if (!isAuthenticated) {
            QMessageBox::warning(this,
                                 "Connexion refusée",
                                 "E-mail ou mot de passe invalide.");
            loginPage->passEdit->clear();
            loginPage->passEdit->setFocus();
            return;
        }

        const QString fullName = authQuery.value(0).toString().trimmed();
        const QString rawRole = authQuery.value(1).toString().trimmed();

        // Normalisation du rôle

        if      (rawRole.contains("Responsable", Qt::CaseInsensitive)) *currentRole = "Responsable";
        else if (rawRole.contains("Chercheur",   Qt::CaseInsensitive)) *currentRole = "Chercheur";
        else if (rawRole.contains("Technicien",  Qt::CaseInsensitive)) *currentRole = "Technicien";
        else if (rawRole.contains("RH",          Qt::CaseInsensitive)) *currentRole = "RH";
        else                                                            *currentRole = "Responsable"; // fallback
        if (g_voiceCmd) g_voiceCmd->setCurrentRole(*currentRole);

        *currentUserFullName = fullName.isEmpty() ? email : fullName;
        *currentUserEmail = email;
        TracabiliteManager::setUserContext(*currentUserFullName, *currentUserEmail);
        TracabiliteManager::logConnexion(*currentUserFullName, *currentUserEmail, *currentRole);

        const bool smtpConfigured = configureEmailSenderFromLocalConfig(smtpConfigError);
        if (!smtpConfigured) {
            qWarning() << "[SMTP] notifications disabled:" << *smtpConfigError;
            appendSmtpLog(QString("[CONFIG] notifications disabled reason=%1").arg(*smtpConfigError));
            showToast(this,
                      QString("Notifications e-mail désactivées : %1. Vérifiez smtp_mail.ini (host, port, username, password).")
                          .arg(smtpConfigError->isEmpty()
                                   ? QString("configuration SMTP manquante")
                                   : *smtpConfigError),
                      false);
        }

        // ── Check biosamples expiring in exactly 1 day — notify connected user ──
        auto checkExpiringBiosamples = [=]() {
            configureEmailSenderFromLocalConfig(smtpConfigError);
            if (!EmailSender::instance()->isConfigured()) {
                appendSmtpLog(QString("[UI] Expiration alert skipped reason=%1")
                                  .arg(smtpConfigError->isEmpty()
                                           ? QString("configuration SMTP manquante")
                                           : *smtpConfigError));
                return;
            }

            QSqlDatabase db = QSqlDatabase::database();
            if (!db.isValid() || !db.isOpen()) return;

            // 1. Collect expiring samples
            QSqlQuery q(db);
            q.prepare("SELECT REFERENCE_ECHANTILLON, TYPE_ECHANTILLON, "
                      "       ORGANISME_SOURCE, DATE_EXPIRATION, "
                      "       NIVEAU_DE_DANGEROSITE, QUANTITE_RESTANTE "
                      "FROM   BIOSAMPLE "
                      "WHERE  TRUNC(DATE_EXPIRATION) = TRUNC(SYSDATE) + 1");
            if (!q.exec()) {
                qWarning() << "[ExpirationCheck]" << q.lastError().text();
                return;
            }

            struct ExpRow { QString ref, type, org, danger; int qty; QDate date; };
            QList<ExpRow> rows;
            while (q.next()) {
                ExpRow r;
                r.ref    = q.value(0).toString();
                r.type   = q.value(1).toString();
                r.org    = q.value(2).toString();
                r.date   = q.value(3).toDate();
                r.danger = q.value(4).toString();
                r.qty    = q.value(5).toInt();
                rows.append(r);
            }
            if (rows.isEmpty()) return;

            // 2. Build shared table rows HTML
            QString tableRows;
            for (const ExpRow& r : rows) {
                const QString dangerColor = r.danger.contains("Élevé", Qt::CaseInsensitive)  ? "#c0392b"
                                          : r.danger.contains("Moyen", Qt::CaseInsensitive)  ? "#e67e22"
                                          : "#27ae60";
                tableRows += QString(
                    "<tr>"
                    "<td style='padding:8px 12px;border-bottom:1px solid #e0e0e0;font-weight:600;color:#0a3830;'>%1</td>"
                    "<td style='padding:8px 12px;border-bottom:1px solid #e0e0e0;'>%2</td>"
                    "<td style='padding:8px 12px;border-bottom:1px solid #e0e0e0;'>%3</td>"
                    "<td style='padding:8px 12px;border-bottom:1px solid #e0e0e0;'>%4</td>"
                    "<td style='padding:8px 12px;border-bottom:1px solid #e0e0e0;color:%5;font-weight:600;'>%6</td>"
                    "<td style='padding:8px 12px;border-bottom:1px solid #e0e0e0;text-align:center;'>%7</td>"
                    "</tr>")
                    .arg(r.ref, r.type, r.org,
                         r.date.toString("dd/MM/yyyy"),
                         dangerColor, r.danger)
                    .arg(r.qty);
            }

            const QString subject = QString("⚠️ ALERTE — %1 échantillon(s) expirent demain")
                                    .arg(rows.size());

            // 3. Send one email to the connected user
            const QString recipientEmail = currentUserEmail->trimmed();
            const QString recipientName = currentUserFullName->trimmed().isEmpty()
                                          ? recipientEmail
                                          : currentUserFullName->trimmed();
            if (recipientEmail.isEmpty()) {
                appendSmtpLog("[UI] Expiration alert skipped reason=email utilisateur connecté indisponible");
                return;
            }

                const QString body = QString(R"(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8"/>
    <style>
        body {
            font-family: "Trebuchet MS", "Segoe UI", Arial, sans-serif;
            background: linear-gradient(180deg, #A3CAD3 0%%, #EAF3F5 52%%, #F2F0EB 100%%);
            margin: 0;
            padding: 24px 14px;
            color: #64533A;
        }
        .wrapper {
            max-width: 700px;
            margin: 0 auto;
            background: #FFFFFF;
            border-radius: 16px;
            overflow: hidden;
            border: 1px solid #AFC6C3;
            box-shadow: 0 10px 28px rgba(18, 68, 59, 0.16);
        }
        .header {
            background: linear-gradient(125deg, #12443B, #0A5F58 58%%, #2E6F63 100%%);
            padding: 24px 26px;
            text-align: left;
        }
        .header h1 {
            color: #FFFFFF;
            margin: 0;
            font-size: 22px;
            letter-spacing: 0.2px;
        }
        .header p {
            color: rgba(255, 255, 255, 0.90);
            margin: 7px 0 0;
            font-size: 13px;
        }
        .body {
            padding: 24px 26px 18px;
            background: linear-gradient(180deg, #FFFFFF, #F2F0EB 95%%);
        }
        .body h2 {
            color: #12443B;
            font-size: 20px;
            margin-top: 0;
            margin-bottom: 10px;
        }
        .alert-box {
            background: #FFF6F4;
            border: 1px solid #E2C8BF;
            border-left: 6px solid #8B2F3C;
            border-radius: 10px;
            padding: 14px 16px;
            margin-bottom: 16px;
            color: #64533A;
            font-size: 13px;
            line-height: 1.5;
        }
        table {
            width: 100%%;
            border-collapse: separate;
            border-spacing: 0;
            font-size: 13px;
            border: 1px solid #AFC6C3;
            border-radius: 10px;
            overflow: hidden;
        }
        thead th {
            background: #12443B;
            color: #FFFFFF;
            padding: 10px 12px;
            text-align: left;
            font-weight: 700;
        }
        tbody td {
            background: #FFFFFF;
        }
        tbody tr:nth-child(even) td {
            background: #F2F0EB;
        }
        .footer {
            background: #AFC6C3;
            border-top: 1px solid #90AEAA;
            text-align: center;
            padding: 14px;
            font-size: 11px;
            color: #12443B;
        }
    </style>
</head>
<body>
<div class="wrapper">
  <div class="header">
    <h1>⚠ Alerte d'expiration — SmartVision BioSimple</h1>
    <p>Notification automatique · %1</p>
  </div>
  <div class="body">
    <h2>Bonjour %2,</h2>
    <div class="alert-box">
      <strong>Attention !</strong> Les échantillons biologiques listés ci-dessous
            arriveront à expiration dans <strong>1 jour</strong> (%3).
      Veuillez prendre les mesures nécessaires (élimination, renouvellement ou transfert)
      avant cette date.
    </div>
    <table>
      <thead>
        <tr>
          <th>Référence</th><th>Type</th><th>Organisme</th>
          <th>Date d'expiration</th><th>Danger</th><th>Quantité</th>
        </tr>
      </thead>
      <tbody>%4</tbody>
    </table>
    <p style="margin-top:20px;color:#555;font-size:13px;">
      Connectez-vous à <strong>SmartVision BioSimple</strong> pour gérer ces échantillons
      depuis le module <em>Échantillons Biologiques</em>.
    </p>
  </div>
  <div class="footer">
    © %5 SmartVision BioSimple — Alerte automatique, merci de ne pas répondre à ce message.
  </div>
</div>
</body>
</html>
)")
                .arg(QDate::currentDate().toString("dd/MM/yyyy"))
                .arg(recipientName)
                .arg(QDate::currentDate().addDays(1).toString("dd/MM/yyyy"))
                .arg(tableRows)
                .arg(QDate::currentDate().year());

                EmailSender::instance()->send(recipientEmail, subject, body);
        };

        // Run once immediately after login
        QTimer::singleShot(3000, this, checkExpiringBiosamples);

        // Then repeat every 24 hours (stays active for the session)
        static QTimer* expirationTimer = nullptr;
        if (!expirationTimer) {
            expirationTimer = new QTimer(this);
            expirationTimer->setInterval(24 * 60 * 60 * 1000);  // 24 h
            QObject::connect(expirationTimer, &QTimer::timeout,
                             this, checkExpiringBiosamples);
        }
        expirationTimer->start();

        loginPage->updateRememberedCredentials(email, pass, loginPage->isRemembered());
        loginPage->clearFields();
        loginPage->getCaptchaWidget()->refresh();

        // Appliquer les permissions sur tous les boutons/onglets
        if (*applyPerms) (*applyPerms)();

        // Navigation initiale : premier bouton visible dans l'ordre réel de la barre
        ModuleTab firstVisibleTab = ModuleTab::Employee;
        if (firstVisibleModuleTab(*globalBar, &firstVisibleTab)) {
            syncModuleSelection(firstVisibleTab);
            setWindowTitle(windowTitleForModule(firstVisibleTab));
            stack->setCurrentIndex(rootPageForModule(firstVisibleTab));
        } else {
            clearAllModuleSelections();
            stack->setCurrentIndex(LOGIN);
        }

        SuccessDialog successDlg(
            QString("Connecté en tant que %1").arg(*currentRole),
            "Bienvenue sur SmartVision", this);
        successDlg.exec();
    });

    // Handle logout button
    QObject::connect(globalBar->bLogout, &QPushButton::clicked, this, [=](){
        logoutCurrentUser();
        setWindowTitle("SmartVision - Connexion");
        if (g_voiceCmd) g_voiceCmd->hide();
        if (g_voiceCmd) g_voiceCmd->setCurrentRole(QString());
        if (globalBar->bVoice) globalBar->bVoice->setChecked(false);
        stack->setCurrentIndex(LOGIN);
    });

    QObject::connect(globalBar->bTheme, &QPushButton::clicked, this, [=](){
        g_darkThemeEnabled = !g_darkThemeEnabled;
        if (g_applyThemeFn) g_applyThemeFn(g_darkThemeEnabled);
        syncThemeToggleButtons();
    });

    // ==========================================================
    // PAGE 1 : BioSimple - Échantillons (LIST)
    // ==========================================================
    QWidget* page1 = new QWidget;
    QVBoxLayout* p1 = new QVBoxLayout(page1);
    p1->setContentsMargins(22, 18, 22, 18);
    p1->setSpacing(14);

    ModulesBar barBioList;
    p1->addWidget(makeHeaderBlock(st, "Échantillons", ModuleTab::BioSimple, &barBioList));
    connectModulesSwitch(this, stack, barBioList);

    QFrame* bar1 = new QFrame;
    bar1->setFixedHeight(54);
    bar1->setStyleSheet("background: rgba(255,255,255,0.22); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* bar1L = new QHBoxLayout(bar1);
    bar1L->setContentsMargins(14, 8, 14, 8);
    bar1L->setSpacing(10);

    QLineEdit* search = new QLineEdit;
    search->setPlaceholderText("Rechercher (type)");
    search->addAction(st->standardIcon(QStyle::SP_FileDialogContentsView), QLineEdit::LeadingPosition);

    // ── Sort button ──
    QPushButton* bioSortBtn = new QPushButton("⇅  Trier");
    bioSortBtn->setCursor(Qt::PointingHandCursor);
    bioSortBtn->setFixedHeight(36);
    bioSortBtn->setStyleSheet(
        "QPushButton{"
        " background: rgba(10,95,88,0.12);"
        " border: 1.5px solid rgba(10,95,88,0.30);"
        " border-radius: 10px;"
        " padding: 0 16px;"
        " color: rgba(10,95,88,0.90);"
        " font-weight: 800;"
        " font-size: 13px;"
        "}"
        "QPushButton:hover{"
        " background: rgba(10,95,88,0.22);"
        " border-color: rgba(10,95,88,0.55);"
        "}"
        "QPushButton:pressed{ background: rgba(10,95,88,0.30); }"
    );

    bar1L->addWidget(search, 1);
    bar1L->addWidget(bioSortBtn);
    p1->addWidget(bar1);

    QFrame* card1 = makeCard();
    QVBoxLayout* card1L = new QVBoxLayout(card1);
    card1L->setContentsMargins(10,10,10,10);

    QTableWidget* table = new QTableWidget(0, 10);
    table->setHorizontalHeaderLabels({"Référence", "Emplacement de stockage", "Type", "Organisme", "Température", "Quantité restante", "Date de collecte", "Date d'expiration", "Statut", "Niveau de dangerosité"});
    table->verticalHeader()->setVisible(false);
    table->setShowGrid(true);
    table->setAlternatingRowColors(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setItemDelegateForColumn(8, new BadgeDelegate(table));

    table->setColumnWidth(0, 140);
    table->setColumnWidth(1, 170);
    table->setColumnWidth(2, 110);
    table->setColumnWidth(3, 140);
    table->setColumnWidth(4, 120);
    table->setColumnWidth(5, 130);
    table->setColumnWidth(6, 150);
    table->setColumnWidth(7, 160);
    table->setColumnWidth(8, 150);
    table->setColumnWidth(9, 170);

    // Initial load from database
    crud->loadAll(table);

    // Reload table every time the BioSimple tab becomes visible
    QObject::connect(stack, &QStackedWidget::currentChanged, stack, [=](int idx){
        if (idx == BIO_LIST) crud->loadAll(table);
    });

    card1L->addWidget(table);
    p1->addWidget(card1, 1);

    // Search by type
    QObject::connect(search, &QLineEdit::textChanged, this, [=](const QString& text){
        QString filter = text.trimmed().toLower();
        for (int r = 0; r < table->rowCount(); ++r) {
            bool match = false;
            if (filter.isEmpty()) { match = true; }
            else {
                for (int c = 0; c <= 9; ++c) {
                    QTableWidgetItem* it = table->item(r, c);
                    if (it && it->text().toLower().contains(filter)) { match = true; break; }
                }
            }
            table->setRowHidden(r, !match);
        }
    });

    // ── Client-side sort: dates (dd/MM/yyyy) and quantity ("90 µg") ──
    auto bioSortTable = [=](int col, Qt::SortOrder order) {
        int n = table->rowCount();
        if (n == 0) return;

        // Snapshot all cells (text + UserRole) and compute a numeric sort key
        struct BioRow {
            QVector<QPair<QString, QVariant>> cells;
            double key;
        };
        QVector<BioRow> rows(n);
        int ncols = table->columnCount();
        for (int r = 0; r < n; ++r) {
            rows[r].cells.resize(ncols);
            for (int c = 0; c < ncols; ++c) {
                auto* it = table->item(r, c);
                rows[r].cells[c] = { it ? it->text() : QString(),
                                     it ? it->data(Qt::UserRole) : QVariant() };
            }
            const QString& txt = rows[r].cells[col].first;
            if (col == 5) {                              // Quantité: "90 µg"
                rows[r].key = txt.split(' ').first().toDouble();
            } else {                                     // Dates: "dd/MM/yyyy"
                QDate d = QDate::fromString(txt, "dd/MM/yyyy");
                rows[r].key = d.isValid() ? static_cast<double>(d.toJulianDay()) : 0.0;
            }
        }

        std::stable_sort(rows.begin(), rows.end(), [order](const BioRow& a, const BioRow& b){
            return order == Qt::AscendingOrder ? a.key < b.key : a.key > b.key;
        });

        // Write sorted data back (no row insertion/removal — avoids delegate resets)
        table->setSortingEnabled(false);
        for (int r = 0; r < n; ++r) {
            for (int c = 0; c < ncols; ++c) {
                auto* it = table->item(r, c);
                if (!it) { it = new QTableWidgetItem; table->setItem(r, c, it); }
                it->setText(rows[r].cells[c].first);
                it->setData(Qt::UserRole, rows[r].cells[c].second);
            }
        }

        // Update button label to reflect active sort
        const QString colName = (col == 5) ? "Quantité"
                              : (col == 6) ? "Date collecte"
                                           : "Date expiration";
        const QString arrow = (order == Qt::AscendingOrder) ? " ↑" : " ↓";
        bioSortBtn->setText("⇅  " + colName + arrow);
    };

    // Sort button → dropdown menu
    QObject::connect(bioSortBtn, &QPushButton::clicked, this, [=](){
        QMenu* menu = new QMenu(bioSortBtn);
        menu->setStyleSheet(
            "QMenu{"
            " background: #ffffff;"
            " border: 1.5px solid rgba(10,95,88,0.25);"
            " border-radius: 10px;"
            " padding: 4px 0;"
            "}"
            "QMenu::item{"
            " padding: 8px 20px 8px 14px;"
            " font-size: 13px;"
            " color: rgba(0,0,0,0.75);"
            " border-radius: 6px;"
            " margin: 1px 4px;"
            "}"
            "QMenu::item:selected{"
            " background: rgba(10,95,88,0.12);"
            " color: rgba(10,95,88,0.95);"
            " font-weight: 700;"
            "}"
            "QMenu::separator{ height:1px; background:rgba(0,0,0,0.08); margin:4px 10px; }"
        );

        auto addSort = [&](const QString& label, int col, Qt::SortOrder ord){
            QAction* a = menu->addAction(label);
            QObject::connect(a, &QAction::triggered, this, [=]{ bioSortTable(col, ord); });
        };

        addSort("📅  Date de collecte  ↑ (plus ancienne)", 6, Qt::AscendingOrder);
        addSort("📅  Date de collecte  ↓ (plus récente)",  6, Qt::DescendingOrder);
        menu->addSeparator();
        addSort("⚗   Quantité  ↑ (plus petite)", 5, Qt::AscendingOrder);
        addSort("⚗   Quantité  ↓ (plus grande)", 5, Qt::DescendingOrder);
        menu->addSeparator();
        addSort("⏳  Date d'expiration  ↑ (plus proche)", 7, Qt::AscendingOrder);
        addSort("⏳  Date d'expiration  ↓ (plus éloignée)", 7, Qt::DescendingOrder);
        menu->addSeparator();

        QAction* reset = menu->addAction("↺  Réinitialiser");
        QObject::connect(reset, &QAction::triggered, this, [=]{
            crud->loadAll(table);
            bioSortBtn->setText("⇅  Trier");
        });

        menu->exec(bioSortBtn->mapToGlobal(QPoint(0, bioSortBtn->height() + 4)));
        menu->deleteLater();
    });

    QFrame* bottom1 = new QFrame;
    bottom1->setFixedHeight(64);
    bottom1->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* bottom1L = new QHBoxLayout(bottom1);
    bottom1L->setContentsMargins(14,10,14,10);
    bottom1L->setSpacing(12);

    QPushButton* btnAdd   = actionBtn("Ajouter",      "rgba(10,95,88,0.45)", "rgba(255,255,255,0.90)", st->standardIcon(QStyle::SP_DialogYesButton), true);
    QPushButton* btnEdit  = actionBtn("Modifier",     "rgba(198,178,154,0.55)", "rgba(255,255,255,0.85)", st->standardIcon(QStyle::SP_FileDialogContentsView), true);
    QPushButton* btnDel   = actionBtn("Supprimer",    "rgba(255,255,255,0.55)", "#B14A4A", st->standardIcon(QStyle::SP_TrashIcon), true);
    QPushButton* btnStats = actionBtn("Statistiques", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_ComputerIcon), true);

    // btnDel is wired in the NAVIGATION BioSimple section below

    bottom1L->addWidget(btnAdd);
    bottom1L->addWidget(btnEdit);
    bottom1L->addWidget(btnDel);
    bottom1L->addWidget(btnStats);
    bottom1L->addStretch(1);

    {
        QPushButton* btnAiCong = new QPushButton("\u2744  AI Cong\u00e9lateur");
        btnAiCong->setCursor(Qt::PointingHandCursor);
        btnAiCong->setStyleSheet(R"(
            QPushButton{ background:qlineargradient(x1:0,y1:0,x2:1,y2:1,
                stop:0 rgba(10,95,88,0.85),stop:1 rgba(18,68,59,0.90));
                border:none; border-radius:12px; padding:10px 20px;
                color:white; font-weight:900; font-size:13px; }
            QPushButton:hover{ background:qlineargradient(x1:0,y1:0,x2:1,y2:1,
                stop:0 rgba(14,115,106,0.95),stop:1 rgba(22,82,71,0.95)); }
            QPushButton:pressed{ background:rgba(10,95,88,1.0); }
        )");
        bottom1L->addWidget(btnAiCong);
        QObject::connect(btnAiCong, &QPushButton::clicked, this, [=](){
            CongelateurDialog* dlg = new CongelateurDialog(this);
            dlg->setAttribute(Qt::WA_DeleteOnClose);
            dlg->exec();
        });
    }

    QPushButton* btnMore = new QPushButton(st->standardIcon(QStyle::SP_FileDialogContentsView), "  Localisation & Stockage");
    btnMore->setCursor(Qt::PointingHandCursor);
    btnMore->setStyleSheet(R"(
        QPushButton{
            background: rgba(255,255,255,0.55);
            border: 1px solid rgba(0,0,0,0.12);
            border-radius: 12px;
            padding: 10px 14px;
            color: rgba(0,0,0,0.65);
            font-weight: 800;
        }
        QPushButton:hover{ background: rgba(255,255,255,0.75); }
    )");
    bottom1L->addWidget(btnMore);

    p1->addWidget(bottom1);
    addStackPage(page1);

    // ==========================================================
    // PAGE 1 : BioSimple - Ajouter / Modifier
    // ==========================================================
    QWidget* page2 = new QWidget;
    page2->setObjectName("bioFormPage");
    QVBoxLayout* p2 = new QVBoxLayout(page2);
    p2->setContentsMargins(22, 18, 22, 18);
    p2->setSpacing(14);

    ModulesBar barBioForm;
    p2->addWidget(makeHeaderBlock(st, "Ajouter / Modifier un échantillon", ModuleTab::BioSimple, &barBioForm));
    connectModulesSwitch(this, stack, barBioForm);

    QFrame* outer2 = new QFrame;
    outer2->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }")
                              .arg(C_PANEL_BG, C_PANEL_BR));
    QHBoxLayout* outer2L = new QHBoxLayout(outer2);
    outer2L->setContentsMargins(12,12,12,12);
    outer2L->setSpacing(12);

    QFrame* left2 = softBox();
    left2->setFixedWidth(360);
    QVBoxLayout* left2L = new QVBoxLayout(left2);
    left2L->setContentsMargins(12,12,12,12);
    left2L->setSpacing(16);

    auto sectionTitle = [&](const QString& t){
        QLabel* lab = new QLabel(t);
        lab->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
        return lab;
    };

    auto formRow = [&](QStyle::StandardPixmap sp, const QString& label, QWidget* input){
        QFrame* r = softBox();
        QHBoxLayout* l = new QHBoxLayout(r);
        l->setContentsMargins(12,14,12,14);
        l->setSpacing(12);

        QToolButton* ic = new QToolButton;
        ic->setAutoRaise(true);
        ic->setIcon(st->standardIcon(sp));

        QLabel* lab = new QLabel(label);
        lab->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");

        l->addWidget(ic);
        l->addWidget(lab);
        l->addStretch(1);
        l->addWidget(input);
        return r;
    };

    auto mkErrLbl = []() -> QLabel* {
        auto* l = new QLabel;
        l->setWordWrap(true);
        l->setStyleSheet("color:#c0392b; font-size:10px; font-weight:700; padding:0 2px; background:transparent;");
        l->hide();
        return l;
    };

    // helper that returns both the row frame and the date edit so we can later change the value
    auto blueDateRow = [&](const QString& label, const QDate& defDate, QDateEdit*& outDate){
        QFrame* r = new QFrame;
        r->setStyleSheet(
            "QFrame{ background: rgba(255,255,255,0.80);"
            "border: 2px solid rgba(54,92,245,0.70);"
            "border-radius: 12px; }"
            );
        QHBoxLayout* l = new QHBoxLayout(r);
        l->setContentsMargins(10,8,10,8);
        l->setSpacing(10);

        QLabel* lab = new QLabel(label);
        lab->setStyleSheet("color: rgba(54,92,245,0.95); font-weight: 900;");

        QToolButton* cal = new QToolButton;
        cal->setAutoRaise(true);
        cal->setIcon(st->standardIcon(QStyle::SP_FileDialogDetailedView));
        cal->setStyleSheet("QToolButton{ color: rgba(54,92,245,1); }");

        QDateEdit* d = new QDateEdit(defDate);
        d->setCalendarPopup(true);
        d->setDisplayFormat("dd/MM/yyyy");
        d->setMinimumWidth(115);
        d->setStyleSheet("QDateEdit{ background: transparent; border:0; color: rgba(54,92,245,0.95); font-weight: 900; }");

        l->addWidget(cal);
        l->addWidget(lab);
        l->addStretch(1);
        l->addWidget(d);
        outDate = d;
        return r;
    };
    // identical one with a red accent for expiration date
    auto redDateRow = [&](const QString& label, const QDate& defDate, QDateEdit*& outDate){
        QFrame* r = new QFrame;
        r->setStyleSheet(
            "QFrame{ background: rgba(255,255,255,0.80);"
            "border: 2px solid rgba(244,67,54,0.70);"
            "border-radius: 12px; }"
            );
        QHBoxLayout* l = new QHBoxLayout(r);
        l->setContentsMargins(10,8,10,8);
        l->setSpacing(10);

        QLabel* lab = new QLabel(label);
        lab->setStyleSheet("color: rgba(244,67,54,0.95); font-weight: 900;");

        QToolButton* cal = new QToolButton;
        cal->setAutoRaise(true);
        cal->setIcon(st->standardIcon(QStyle::SP_FileDialogDetailedView));
        cal->setStyleSheet("QToolButton{ color: rgba(244,67,54,1); }");

        QDateEdit* d = new QDateEdit(defDate);
        d->setCalendarPopup(true);
        d->setDisplayFormat("dd/MM/yyyy");
        d->setMinimumWidth(115);
        d->setStyleSheet("QDateEdit{ background: transparent; border:0; color: rgba(244,67,54,0.95); font-weight: 900; }");

        l->addWidget(cal);
        l->addWidget(lab);
        l->addStretch(1);
        l->addWidget(d);
        outDate = d;
        return r;
    };


    left2L->addWidget(sectionTitle("Identité"));
    // unique reference; when the user enters a value we will look up the record and
    // fill the remaining fields automatically
    QLineEdit* leRef = new QLineEdit;
    leRef->setPlaceholderText("REF + chiffres (max 5)");
    // Référence : format strict REF suivi de 1 à 5 chiffres (ex: REF12345)
    connect(leRef, &QLineEdit::textChanged, leRef, [leRef](const QString& text) {
        const QString upper = text.toUpper();
        const QString prefix = "REF";
        QString filtered;
        for (int i = 0; i < upper.length(); ++i) {
            if (i < 3) {
                if (upper[i] == prefix[i]) filtered += upper[i];
                else break;
            } else {
                if (upper[i].isDigit() && (filtered.length() - 3) < 5)
                    filtered += upper[i];
            }
        }
        if (filtered != text) {
            const int pos = leRef->cursorPosition();
            leRef->blockSignals(true);
            leRef->setText(filtered);
            leRef->setCursorPosition(qMin(pos, filtered.length()));
            leRef->blockSignals(false);
        }
    });
    left2L->addWidget(formRow(QStyle::SP_FileIcon, "Référence", leRef));
    QLabel* errRef = mkErrLbl(); left2L->addWidget(errRef);

    // Validation temps réel du format REFxxxxx
    connect(leRef, &QLineEdit::textChanged, leRef, [=](const QString& text){
        const QString t = text.trimmed();
        static const QRegularExpression validRef("^REF\\d{1,5}$");
        if (t.isEmpty()) {
            errRef->hide();
            leRef->setStyleSheet("QLineEdit{ border: 1.5px solid #f59e0b; border-radius:8px; padding:4px 8px; }");
            return;
        }
        if (validRef.match(t).hasMatch()) {
            errRef->hide();
            leRef->setStyleSheet("");
        } else {
            errRef->setText("⚠  Format : REF suivi de 1 à 5 chiffres (ex: REF12345)");
            errRef->show();
            leRef->setStyleSheet("QLineEdit{ border: 1.5px solid #f59e0b; border-radius:8px; padding:4px 8px; }");
        }
    });
    // Vérification unicité au focus-out
    connect(leRef, &QLineEdit::editingFinished, leRef, [=](){
        if (*bioEditMode) return;
        const QString ref = leRef->text().trimmed();
        if (ref.isEmpty()) return;
        static const QRegularExpression validRef("^REF\\d{1,5}$");
        if (!validRef.match(ref).hasMatch()) {
            leRef->setStyleSheet("QLineEdit{ border: 1.5px solid #dc2626; border-radius:8px; padding:4px 8px; }");
            return;
        }
        QSqlQuery dup;
        dup.prepare("SELECT COUNT(1) FROM \"BioSample\" WHERE \"Reference_de_léchantillon\" = ?");
        dup.addBindValue(ref);
        if (dup.exec() && dup.next() && dup.value(0).toInt() > 0) {
            errRef->setText("⚠  Cette référence est déjà utilisée.");
            errRef->show();
            leRef->setStyleSheet("QLineEdit{ border: 1.5px solid #dc2626; border-radius:8px; padding:4px 8px; }");
        }
    });

    // collection field removed per latest request

    left2L->addWidget(sectionTitle("Dates"));
    // keep pointers to the date edits so the lookup lambda can modify them
    QDateEdit *dCollect = nullptr, *dExpire = nullptr;
    left2L->addWidget(blueDateRow("Date de collecte", QDate::currentDate(), dCollect));
    QLabel* errCollect = mkErrLbl(); left2L->addWidget(errCollect);
    left2L->addWidget(redDateRow("Date d'expiration", QDate::currentDate().addDays(30), dExpire));
    QLabel* errExpire = mkErrLbl(); left2L->addWidget(errExpire);

    // Initialiser la date minimum d'expiration
    dExpire->setMinimumDate(dCollect->date().addDays(1));

    // Validation dynamique : expiration doit être strictement après collecte
    connect(dCollect, &QDateEdit::dateChanged, this, [=](const QDate& d){
        dExpire->setMinimumDate(d.addDays(1));
        if (dExpire->date() <= d) {
            errExpire->setText("⚠  La date d'expiration doit être postérieure à la date de collecte.");
            errExpire->show();
        } else {
            errExpire->hide();
        }
    });
    connect(dExpire, &QDateEdit::dateChanged, this, [=](const QDate& d){
        if (d <= dCollect->date()) {
            errExpire->setText("⚠  La date d'expiration doit être postérieure à la date de collecte.");
            errExpire->show();
        } else {
            errExpire->hide();
        }
    });

    // ── Calendrier vert — nombres bleus (collecte) / rouges (expiration) ──
    auto applyGreenCalendar = [](QDateEdit* de, const QString& numColor) {
        QCalendarWidget* cw = de->calendarWidget();
        if (!cw) return;
        cw->setStyleSheet(QString(
            // ── Barre de navigation (header vert) ──────────────────────────
            "QCalendarWidget QWidget#qt_calendar_navigationbar {"
            "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
            "    stop:0 #43a047, stop:1 #2e7d32);"
            "  padding: 4px 6px; border-radius: 10px 10px 0 0;"
            "}"
            // ── Boutons mois/année ──────────────────────────────────────────
            "QCalendarWidget QToolButton {"
            "  color: white; font-weight: 900; font-size: 13px;"
            "  background: transparent; border: none;"
            "  border-radius: 6px; padding: 4px 10px; min-width: 28px;"
            "}"
            "QCalendarWidget QToolButton:hover  { background: rgba(255,255,255,0.22); }"
            "QCalendarWidget QToolButton:pressed { background: rgba(255,255,255,0.12); }"
            // ── SpinBox année ───────────────────────────────────────────────
            "QCalendarWidget QSpinBox {"
            "  color: white; background: transparent; border: none;"
            "  font-weight: 900; selection-background-color: rgba(255,255,255,0.30);"
            "}"
            // ── En-têtes jours (Lun, Mar…) ─────────────────────────────────
            "QCalendarWidget QHeaderView::section {"
            "  background: #a5d6a7; color: #1b5e20;"
            "  font-weight: 900; font-size: 11px;"
            "  border: none; padding: 5px 0;"
            "}"
            // ── Grille des jours ────────────────────────────────────────────
            "QCalendarWidget QAbstractItemView {"
            "  background: #ffffff;"
            "  selection-background-color: #2e7d32;"
            "  selection-color: white;"
            "  border: 1px solid #c8e6c9;"
            "  outline: none;"
            "}"
            // Cellules actives : couleur thématique + gras
            "QCalendarWidget QAbstractItemView::item {"
            "  color: %1; font-weight: 700;"
            "}"
            // Cellules désactivées (avant setMinimumDate) : grisées, non cliquables
            "QCalendarWidget QAbstractItemView::item:disabled {"
            "  color: #c8c8c8; font-weight: 400; background: transparent;"
            "}"
            "QCalendarWidget QWidget { alternate-background-color: #f1f8e9; }"
            // ── Menu de sélection du mois ───────────────────────────────────
            "QCalendarWidget QMenu {"
            "  background: white; color: #1b5e20;"
            "  selection-background-color: #2e7d32; selection-color: white;"
            "}"
        ).arg(numColor));
        // Aujourd'hui mis en valeur
        QTextCharFormat todayFmt;
        todayFmt.setBackground(QColor("#c8e6c9"));
        todayFmt.setForeground(QColor(numColor));
        todayFmt.setFontWeight(QFont::Black);
        cw->setDateTextFormat(QDate::currentDate(), todayFmt);
    };
    applyGreenCalendar(dCollect, "#1565C0");   // nombres bleus — date de collecte
    applyGreenCalendar(dExpire,  "#c62828");   // nombres rouges — date d'expiration

    // Forcer le calendrier d'expiration à respecter la date minimum dès l'ouverture
    if (QCalendarWidget* cw = dExpire->calendarWidget())
        cw->setMinimumDate(dCollect->date().addDays(1));

    // Re-propager la date minimum sur le QCalendarWidget à chaque changement de collecte
    connect(dCollect, &QDateEdit::dateChanged, this, [=](const QDate& d){
        if (QCalendarWidget* cw = dExpire->calendarWidget())
            cw->setMinimumDate(d.addDays(1));
    });

    // Quantité + unité µg
    QFrame* qtyFrame = new QFrame;
    QHBoxLayout* qtyHL = new QHBoxLayout(qtyFrame);
    qtyHL->setContentsMargins(0,0,0,0); qtyHL->setSpacing(4);
    QSpinBox* qty = new QSpinBox;
    qty->setRange(0, 999999);
    qty->setValue(0);
    qty->setFixedWidth(90);
    qty->setStyleSheet("QSpinBox{ background: transparent; border:0; font-weight:900; color: rgba(0,0,0,0.60); }");
    QLabel* lbUg = new QLabel("µg");
    lbUg->setStyleSheet("color: rgba(0,0,0,0.50); font-weight:700;");
    qtyHL->addWidget(qty); qtyHL->addWidget(lbUg); qtyHL->addStretch(1);
    left2L->addWidget(formRow(QStyle::SP_ArrowUp, "Quantité", qtyFrame));
    QLabel* errQty = mkErrLbl(); left2L->addWidget(errQty);

    // Température + unité °C
    QFrame* tempFrame = new QFrame;
    QHBoxLayout* tempHL = new QHBoxLayout(tempFrame);
    tempHL->setContentsMargins(0,0,0,0); tempHL->setSpacing(4);
    QLineEdit* cbTemp2 = new QLineEdit;
    cbTemp2->setPlaceholderText("ex: -80");
    cbTemp2->setFixedWidth(80);
    // Température : entier cohérent en °C (ex: -80, -20, 4, 25)
    cbTemp2->setValidator(new QIntValidator(-273, 100, cbTemp2));
    QLabel* lbDeg = new QLabel("°C");
    lbDeg->setStyleSheet("color: rgba(0,0,0,0.50); font-weight:700;");
    tempHL->addWidget(cbTemp2); tempHL->addWidget(lbDeg); tempHL->addStretch(1);
    left2L->addWidget(formRow(QStyle::SP_BrowserStop, "Température", tempFrame));
    QLabel* errTemp = mkErrLbl(); left2L->addWidget(errTemp);
    const QString tempFieldNormalStyle = cbTemp2->styleSheet();
    const QString tempFieldErrorStyle =
        "QLineEdit{"
        " border:2px solid #C0392B;"
        " border-radius:8px;"
        " background:rgba(255,235,238,0.96);"
        " padding:4px 8px;"
        " font-weight:800;"
        "}";

    QComboBox* cbDanger = new QComboBox;
    cbDanger->addItems({"Niveau de danger", "BSL-1", "BSL-2", "BSL-3"});
    cbDanger->setFixedWidth(170);
    left2L->addWidget(formRow(QStyle::SP_MessageBoxWarning, "Niveau de danger", cbDanger));
    QLabel* errDanger = mkErrLbl(); left2L->addWidget(errDanger);

    left2L->addStretch(1);

    QFrame* right2 = softBox();
    QVBoxLayout* right2L = new QVBoxLayout(right2);
    right2L->setContentsMargins(12,12,12,12);
    right2L->setSpacing(16);

    right2L->addWidget(sectionTitle("Données"));

    // ── Type field + AI button ──
    QLineEdit* cbType2 = new QLineEdit;
    cbType2->setPlaceholderText("Type (ex: DNA, RNA, Protéine...)");
    cbType2->setFixedWidth(160);
    // Type : doit commencer par une lettre — pas de chiffres seuls
    connect(cbType2, &QLineEdit::textChanged, cbType2, [cbType2](const QString& text) {
        QString filtered;
        for (const QChar& c : text) {
            if (filtered.isEmpty()) {
                if (c.isLetter()) filtered += c;       // 1er caractère = lettre obligatoire
            } else {
                if (c.isLetterOrNumber() || c == ' ' || c == '-' || c == '\'')
                    filtered += c;
            }
        }
        if (filtered != text) {
            const int pos = cbType2->cursorPosition();
            cbType2->blockSignals(true);
            cbType2->setText(filtered);
            cbType2->setCursorPosition(qMin(pos, filtered.length()));
            cbType2->blockSignals(false);
        }
    });

    // Auto-complétion pour le type d'échantillon
    QCompleter* typeCompleter = new QCompleter(
        QStringList{"ADN", "ARN", "Protéine", "Cellule", "Tissu", "Organisme"}, cbType2);
    typeCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    typeCompleter->setCompletionMode(QCompleter::PopupCompletion);
    cbType2->setCompleter(typeCompleter);

    right2L->addWidget(formRow(QStyle::SP_FileIcon, "Type", cbType2));
    QLabel* errType = mkErrLbl(); right2L->addWidget(errType);

    // ── Organisme field + AI button ──
    QLineEdit* cbOrg2 = new QLineEdit;
    cbOrg2->setPlaceholderText("Organisme");
    cbOrg2->setFixedWidth(160);
    // Organisme : doit commencer par une lettre — pas de chiffres seuls
    connect(cbOrg2, &QLineEdit::textChanged, cbOrg2, [cbOrg2](const QString& text) {
        QString filtered;
        for (const QChar& c : text) {
            if (filtered.isEmpty()) {
                if (c.isLetter()) filtered += c;       // 1er caractère = lettre obligatoire
            } else {
                if (c.isLetterOrNumber() || c == ' ' || c == '-' || c == '.' || c == '\'')
                    filtered += c;
            }
        }
        if (filtered != text) {
            const int pos = cbOrg2->cursorPosition();
            cbOrg2->blockSignals(true);
            cbOrg2->setText(filtered);
            cbOrg2->setCursorPosition(qMin(pos, filtered.length()));
            cbOrg2->blockSignals(false);
        }
    });

    // Auto-complétion pour l'organisme source
    QCompleter* orgCompleter = new QCompleter(
        QStringList{"Homo sapiens", "Mus musculus", "Escherichia coli", "Rattus norvegicus",
                    "Drosophila melanogaster", "Saccharomyces cerevisiae", "Arabidopsis thaliana",
                    "Danio rerio", "Caenorhabditis elegans", "Bacillus subtilis"}, cbOrg2);
    orgCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    orgCompleter->setCompletionMode(QCompleter::PopupCompletion);
    cbOrg2->setCompleter(orgCompleter);

    right2L->addWidget(formRow(QStyle::SP_DirIcon, "Organisme", cbOrg2));
    QLabel* errOrg = mkErrLbl(); right2L->addWidget(errOrg);

    // Emplacement : bouton + popup Congélateur/Étagère
    QPushButton* emplacBtn = new QPushButton("Emplacement de stockage");
    emplacBtn->setFixedWidth(200);
    emplacBtn->setCursor(Qt::PointingHandCursor);
    emplacBtn->setStyleSheet(
        "QPushButton{ background:rgba(255,255,255,0.72); border:1px solid rgba(0,0,0,0.15);"
        " border-radius:8px; padding:4px 10px; text-align:left;"
        " color:rgba(0,0,0,0.40); font-size:13px; }"
        "QPushButton:hover{ background:rgba(255,255,255,0.92); }"
    );
    right2L->addWidget(formRow(QStyle::SP_DriveHDIcon, "Emplacement", emplacBtn));

    QFrame* emplacPopup = new QFrame;
    emplacPopup->setStyleSheet(
        "QFrame{ background:rgba(255,255,255,0.95); border:1px solid rgba(0,0,0,0.12);"
        " border-radius:10px; }"
    );
    QVBoxLayout* emplacPopupL = new QVBoxLayout(emplacPopup);
    emplacPopupL->setContentsMargins(10,8,10,8);
    emplacPopupL->setSpacing(6);

    QLineEdit* leCongelateur = new QLineEdit;
    leCongelateur->setPlaceholderText("Congélateur (ex: C01)");
    QLineEdit* leEtagere = new QLineEdit;
    leEtagere->setPlaceholderText("Étagère (ex: A3)");
    emplacPopupL->addWidget(leCongelateur);
    emplacPopupL->addWidget(leEtagere);
    emplacPopup->setVisible(false);
    right2L->addWidget(emplacPopup);
    QLabel* errEmplac = mkErrLbl(); right2L->addWidget(errEmplac);

    // Toggle popup
    QObject::connect(emplacBtn, &QPushButton::clicked, [=]{
        emplacPopup->setVisible(!emplacPopup->isVisible());
    });
    // Update button label as user types
    auto syncEmplacBtn = [=]{
        QString c = leCongelateur->text().trimmed();
        QString e = leEtagere->text().trimmed();
        if (c.isEmpty() && e.isEmpty()) {
            emplacBtn->setText("Emplacement de stockage");
            emplacBtn->setStyleSheet(
                "QPushButton{ background:rgba(255,255,255,0.72); border:1px solid rgba(0,0,0,0.15);"
                " border-radius:8px; padding:4px 10px; text-align:left;"
                " color:rgba(0,0,0,0.40); font-size:13px; }"
                "QPushButton:hover{ background:rgba(255,255,255,0.92); }"
            );
        } else {
            emplacBtn->setText(QString("Cong:%1/Etag:%2").arg(c, e));
            emplacBtn->setStyleSheet(
                "QPushButton{ background:rgba(255,255,255,0.72); border:1px solid rgba(0,0,0,0.15);"
                " border-radius:8px; padding:4px 10px; text-align:left;"
                " color:rgba(0,0,0,0.75); font-size:13px; font-weight:600; }"
                "QPushButton:hover{ background:rgba(255,255,255,0.92); }"
            );
        }
    };
    QObject::connect(leCongelateur, &QLineEdit::textChanged, syncEmplacBtn);
    QObject::connect(leEtagere,     &QLineEdit::textChanged, syncEmplacBtn);

    // Projet selector
    QComboBox* cbProjet = new QComboBox;
    cbProjet->setFixedWidth(200);
    cbProjet->addItem("-- Aucun projet --", 0);
    right2L->addWidget(formRow(QStyle::SP_DirOpenIcon, "Projet", cbProjet));
    QLabel* errProjet = mkErrLbl(); right2L->addWidget(errProjet);

    auto loadProjetCombo = [=]() {
        const int savedId = cbProjet->currentData().toInt();
        cbProjet->blockSignals(true);
        cbProjet->clear();
        cbProjet->addItem("-- Aucun projet --", 0);
        QSqlQuery pq;
        if (pq.exec("SELECT \"Id_projet\", \"nom_du_projet\" FROM \"projet\" ORDER BY \"nom_du_projet\"")) {
            while (pq.next())
                cbProjet->addItem(pq.value(1).toString(), pq.value(0).toInt());
        }
        for (int i = 0; i < cbProjet->count(); ++i) {
            if (cbProjet->itemData(i).toInt() == savedId) {
                cbProjet->setCurrentIndex(i);
                break;
            }
        }
        cbProjet->blockSignals(false);
    };
    loadProjetCombo();

    right2L->addStretch(1);

    // ----- automatic lookup when reference is entered (add mode only) ------
    QObject::connect(leRef, &QLineEdit::editingFinished, [=](){
        if (*bioEditMode) return;          // editing: fields already populated
        QString ref = leRef->text().trimmed();
        if (ref.isEmpty()) return;

        BioSample s = crud->get(ref);
        if (!s.isEmpty()) {
            cbType2->setText(s.type);
            cbOrg2->setText(s.organisme);
            // Parse "Cong:xx/Etag:xx" format
            {
                QString emp = s.emplacement;
                int slash = emp.indexOf("/Etag:");
                if (emp.startsWith("Cong:") && slash >= 0) {
                    leCongelateur->setText(emp.mid(5, slash - 5));
                    leEtagere->setText(emp.mid(slash + 6));
                } else {
                    leCongelateur->setText(emp);
                    leEtagere->clear();
                }
                emplacPopup->setVisible(!emp.isEmpty());
            }
            cbTemp2->setText(s.temperature);
            qty->setValue(s.quantite);
            cbDanger->setCurrentText(s.niveauDanger);
            if (s.dateCollecte.isValid())   dCollect->setDate(s.dateCollecte);
            if (s.dateExpiration.isValid()) dExpire->setDate(s.dateExpiration);
        }
    });

    outer2L->addWidget(left2);
    outer2L->addWidget(right2, 1);

    QScrollArea* bioFormScroll = new QScrollArea;
    bioFormScroll->setWidgetResizable(true);
    bioFormScroll->setFrameShape(QFrame::NoFrame);
    bioFormScroll->setStyleSheet(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollArea > QWidget > QWidget { background: transparent; }");
    bioFormScroll->setWidget(outer2);
    p2->addWidget(bioFormScroll, 1);

    QFrame* bottom2 = new QFrame;
    bottom2->setFixedHeight(64);
    bottom2->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* bottom2L = new QHBoxLayout(bottom2);
    bottom2L->setContentsMargins(14,10,14,10);
    bottom2L->setSpacing(12);

    QPushButton* saveBtn   = actionBtn("Enregistrer", "rgba(10,95,88,0.45)", "rgba(255,255,255,0.90)", st->standardIcon(QStyle::SP_DialogSaveButton), true);
    QPushButton* cancelBtn = actionBtn("Annuler",     "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_DialogCancelButton), true);

    bottom2L->addWidget(saveBtn);
    bottom2L->addWidget(cancelBtn);
    bottom2L->addStretch(1);
    p2->addWidget(bottom2);

    addStackPage(page2);

    // ==========================================================
    // PAGE 2 : BioSimple - Localisation & Stockage
    // ==========================================================
    QWidget* page3 = new QWidget;
    QVBoxLayout* p3 = new QVBoxLayout(page3);
    p3->setContentsMargins(22, 18, 22, 18);
    p3->setSpacing(14);

    ModulesBar barBioLoc;
    p3->addWidget(makeHeaderBlock(st, "Localisation & Stockage", ModuleTab::BioSimple, &barBioLoc));
    connectModulesSwitch(this, stack, barBioLoc);

    QFrame* outer3 = new QFrame;
    outer3->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }").arg(C_PANEL_BG, C_PANEL_BR));
    QHBoxLayout* outer3L = new QHBoxLayout(outer3);
    outer3L->setContentsMargins(10,10,10,10);
    outer3L->setSpacing(10);

    // ══════════════════════════════════════
    // PANEL 1 — Congélateurs & Échantillons
    // ══════════════════════════════════════
    QFrame* left3 = softBox();
    left3->setFixedWidth(210);
    QVBoxLayout* left3L = new QVBoxLayout(left3);
    left3L->setContentsMargins(8,8,8,8);
    left3L->setSpacing(6);

    auto p3SecTitle = [](const QString& t) -> QLabel* {
        auto* l = new QLabel(t);
        l->setStyleSheet("font-weight:900; font-size:12px; color:rgba(10,95,88,0.90); padding:2px 0;");
        return l;
    };

    left3L->addWidget(p3SecTitle("Congélateurs"));

    QTreeWidget* tree3 = new QTreeWidget;
    tree3->setHeaderHidden(true);
    tree3->setIndentation(14);
    tree3->setStyleSheet(
        "QTreeWidget{ border:none; background:transparent; }"
        "QTreeWidget::item{ padding:5px 3px; border-radius:6px; font-size:11px; color:rgba(0,0,0,0.65); }"
        "QTreeWidget::item:selected{ background:rgba(10,95,88,0.18); color:rgba(10,95,88,0.95); font-weight:700; }"
        "QTreeWidget::item:hover{ background:rgba(10,95,88,0.08); }");
    tree3->setMaximumHeight(180);
    left3L->addWidget(tree3);

    auto buildTree3 = [=](){
        tree3->clear();
        QStringList congs = BasicBio::loadCongelateurs();
        if (congs.isEmpty()) {
            auto* ni = new QTreeWidgetItem(tree3, QStringList() << "Aucun congélateur");
            ni->setFlags(Qt::NoItemFlags);
            return;
        }
        for (const QString& c : congs) {
            auto* cItem = new QTreeWidgetItem(tree3, QStringList() << c);
            cItem->setIcon(0, st->standardIcon(QStyle::SP_DriveHDIcon));
            cItem->setData(0, Qt::UserRole,     c);
            cItem->setData(0, Qt::UserRole + 1, QString());
            for (const QString& e : BasicBio::loadEtageres(c)) {
                auto* eItem = new QTreeWidgetItem(cItem, QStringList() << e);
                eItem->setIcon(0, st->standardIcon(QStyle::SP_DirIcon));
                eItem->setData(0, Qt::UserRole,     c);
                eItem->setData(0, Qt::UserRole + 1, e);
            }
        }
        tree3->expandAll();
        tree3->setCurrentItem(tree3->topLevelItem(0));
    };

    QFrame* sep3a = new QFrame; sep3a->setFrameShape(QFrame::HLine);
    sep3a->setStyleSheet("border-top:1px solid rgba(10,95,88,0.12);");
    left3L->addWidget(sep3a);

    left3L->addWidget(p3SecTitle("Échantillons"));

    QListWidget* list3 = new QListWidget;
    list3->setSpacing(4);
    list3->setSelectionMode(QAbstractItemView::SingleSelection);
    list3->setStyleSheet(
        "QListWidget{ border:none; background:transparent; }"
        "QListWidget::item{ background:rgba(255,255,255,0.80); border:1px solid rgba(10,95,88,0.10);"
        " border-radius:8px; padding:5px 8px; font-size:10px; color:rgba(0,0,0,0.70); }"
        "QListWidget::item:selected{ background:rgba(10,95,88,0.15); color:rgba(10,95,88,0.95);"
        " font-weight:700; border:1px solid rgba(10,95,88,0.35); }"
        "QListWidget::item:hover{ background:rgba(10,95,88,0.07); }");
    left3L->addWidget(list3, 1);

    // ══════════════════════════════════════
    // PANEL 2 — Détails de l'Échantillon
    // ══════════════════════════════════════
    QFrame* mid3 = softBox();
    QVBoxLayout* mid3L = new QVBoxLayout(mid3);
    mid3L->setContentsMargins(14,12,14,12);
    mid3L->setSpacing(10);

    mid3L->addWidget(p3SecTitle("Détails de l'Échantillon"));

    auto mkDetRow = [&](const QString& lbl, QLabel*& valOut) {
        QFrame* row = new QFrame;
        row->setStyleSheet("QFrame{ background:rgba(255,255,255,0.70); border:1px solid rgba(10,95,88,0.12); border-radius:10px; }");
        QHBoxLayout* hl = new QHBoxLayout(row);
        hl->setContentsMargins(12,8,12,8); hl->setSpacing(10);
        auto* key = new QLabel(lbl);
        key->setStyleSheet("color:rgba(10,95,88,0.75); font-size:11px; font-weight:700; min-width:90px;");
        valOut = new QLabel("—");
        valOut->setStyleSheet("color:rgba(0,0,0,0.80); font-size:12px; font-weight:900;");
        hl->addWidget(key); hl->addWidget(valOut, 1);
        mid3L->addWidget(row);
    };

    QLabel *dv3Ref=nullptr, *dv3Type=nullptr, *dv3Org=nullptr, *dv3Proj=nullptr;
    QLabel *dv3Bsl=nullptr, *dv3Qty=nullptr,  *dv3Cong=nullptr, *dv3Etag=nullptr, *dv3Temp=nullptr;

    mkDetRow("ID Échantillon",  dv3Ref);
    mkDetRow("Type",            dv3Type);
    mkDetRow("Organisme",       dv3Org);
    mkDetRow("Projet",          dv3Proj);
    mkDetRow("Niveau BSL",      dv3Bsl);
    mkDetRow("Quantité",        dv3Qty);
    mkDetRow("Congélateur",     dv3Cong);
    mkDetRow("Étagère",         dv3Etag);
    mkDetRow("Température",     dv3Temp);
    mid3L->addStretch(1);

    // ══════════════════════════════════════
    // PANEL 3 — Rapport + Export PDF
    // ══════════════════════════════════════
    QFrame* right3 = softBox();
    right3->setFixedWidth(320);
    QVBoxLayout* right3L = new QVBoxLayout(right3);
    right3L->setContentsMargins(10,10,10,10);
    right3L->setSpacing(8);

    // Report preview card (styled like the reference image, in green)
    QFrame* reportCard = new QFrame;
    reportCard->setStyleSheet(
        "QFrame{ background:white; border:1.5px solid rgba(10,95,88,0.25); border-radius:12px; }");
    QVBoxLayout* reportL = new QVBoxLayout(reportCard);
    reportL->setContentsMargins(14,10,14,12);
    reportL->setSpacing(6);

    // Header row
    QWidget* rHdr = new QWidget;
    QHBoxLayout* rHdrL = new QHBoxLayout(rHdr);
    rHdrL->setContentsMargins(0,0,0,0); rHdrL->setSpacing(8);
    auto* rLogo = new QLabel("⊕  SmartVision");
    rLogo->setStyleSheet("color:rgba(10,95,88,1); font-weight:900; font-size:13px;");
    auto* rDateLbl = new QLabel;
    rDateLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    rDateLbl->setStyleSheet("color:rgba(0,0,0,0.55); font-size:9px;");
    rDateLbl->setText("Date : " + QDate::currentDate().toString("dd/MM/yyyy"));
    rHdrL->addWidget(rLogo); rHdrL->addStretch(1); rHdrL->addWidget(rDateLbl);
    reportL->addWidget(rHdr);

    // Report title
    auto* rTitle = new QLabel("Rapport de Stockage & Suivi");
    rTitle->setAlignment(Qt::AlignCenter);
    rTitle->setStyleSheet(
        "font-weight:900; font-size:13px; color:rgba(10,95,88,0.95);"
        "background:rgba(10,95,88,0.10); border-radius:6px; padding:5px 0;");
    reportL->addWidget(rTitle);

    // Section helper
    auto rSec = [&](const QString& t) {
        auto* l = new QLabel(t);
        l->setStyleSheet(
            "background:rgba(10,95,88,0.85); color:white; font-weight:900; font-size:10px;"
            "border-radius:4px; padding:3px 8px;");
        reportL->addWidget(l);
    };
    auto rRow = [&](const QString& lbl, QLabel*& out) {
        auto* w = new QWidget;
        auto* hl = new QHBoxLayout(w); hl->setContentsMargins(4,0,4,0); hl->setSpacing(4);
        auto* k = new QLabel("• " + lbl + " :");
        k->setStyleSheet("color:rgba(0,0,0,0.55); font-size:9px; font-weight:700;");
        out = new QLabel("—");
        out->setStyleSheet("color:rgba(0,0,0,0.80); font-size:9px; font-weight:900;");
        hl->addWidget(k); hl->addWidget(out, 1);
        reportL->addWidget(w);
    };

    QLabel *rRef=nullptr, *rType=nullptr, *rOrg=nullptr, *rBsl=nullptr;
    QLabel *rQty=nullptr, *rCong=nullptr, *rEtag=nullptr, *rTemp=nullptr, *rProj=nullptr;

    rSec("Détails de l'Échantillon");
    rRow("ID Échantillon", rRef);
    rRow("Type",           rType);
    rRow("Organisme",      rOrg);
    rRow("Projet",         rProj);
    rRow("Niveau BSL",     rBsl);
    rRow("Quantité",       rQty);

    rSec("Localisation de Stockage");
    rRow("Congélateur",    rCong);
    rRow("Étagère",        rEtag);
    rRow("Température",    rTemp);

    rSec("Conformité");
    const QStringList complianceItems = {
        QString::fromUtf8("Protocoles BSL respectés"),
        QString::fromUtf8("Échantillons étiquetés & sécurisés"),
        QString::fromUtf8("Inventaire mis à jour"),
        QString::fromUtf8("Audit effectué")
    };
    for (const QString& txt : complianceItems) {
        auto* cl = new QLabel("  ☑  " + txt);
        cl->setStyleSheet("color:rgba(10,95,88,0.85); font-size:9px; font-weight:700;");
        reportL->addWidget(cl);
    }

    reportL->addStretch(1);
    right3L->addWidget(reportCard, 1);

    // Export PDF button
    QPushButton* pdfBtn = new QPushButton("  Exporter en PDF");
    pdfBtn->setIcon(st->standardIcon(QStyle::SP_DialogSaveButton));
    pdfBtn->setCursor(Qt::PointingHandCursor);
    pdfBtn->setStyleSheet(QString(R"(
        QPushButton{
            background:%1; color:rgba(255,255,255,0.95);
            border:none; border-radius:10px; padding:10px 18px;
            font-weight:900; font-size:12px;
        }
        QPushButton:hover{ background:%2; }
    )").arg(C_PRIMARY, C_TOPBAR));
    right3L->addWidget(pdfBtn);

    // ── Shared state ──
    auto* bioInfos3 = new QList<BasicBioInfo>;

    // Reset all detail / report labels
    auto resetAll3 = [=](){
        for (auto* l : {dv3Ref,dv3Type,dv3Org,dv3Proj,dv3Bsl,dv3Qty,dv3Cong,dv3Etag,dv3Temp})
            l->setText("—");
        for (auto* l : {rRef,rType,rOrg,rProj,rBsl,rQty,rCong,rEtag,rTemp})
            l->setText("—");
    };

    // ── Tree → sample list ──
    auto loadSamples3 = [=](const QString& cong, const QString& etag) {
        *bioInfos3 = BasicBio::loadSamples(cong, etag);
        list3->clear();
        resetAll3();
        if (bioInfos3->isEmpty()) {
            list3->addItem("Aucun échantillon.");
            return;
        }
        for (int i = 0; i < bioInfos3->size(); ++i) {
            const BasicBioInfo& bi = (*bioInfos3)[i];
            QString badge = bi.bslLevel.isEmpty() ? "" : "  [" + bi.bslLevel + "]";
            auto* it = new QListWidgetItem(bi.reference + badge + "\n" + bi.type
                                           + "  |  " + bi.etagere);
            it->setData(Qt::UserRole, i);
            list3->addItem(it);
        }
    };

    // ── Sample clicked → fill panels 2 & 3 ──
    QObject::connect(list3, &QListWidget::itemClicked, this, [=](QListWidgetItem* item) {
        int idx = item->data(Qt::UserRole).toInt();
        if (idx < 0 || idx >= bioInfos3->size()) return;
        const BasicBioInfo& bi = (*bioInfos3)[idx];
        auto val = [](const QString& s) { return s.isEmpty() ? "—" : s; };
        QString qtyStr = QString::number(bi.quantite) + " µg";
        QString tempStr = bi.temperature.isEmpty() ? "—" : bi.temperature + " °C";
        // Panel 2
        dv3Ref ->setText(val(bi.reference));
        dv3Type->setText(val(bi.type));
        dv3Org ->setText(val(bi.organisme));
        dv3Proj->setText(val(bi.projet));
        dv3Bsl ->setText(val(bi.bslLevel));
        dv3Qty ->setText(qtyStr);
        dv3Cong->setText(val(bi.congelateur));
        dv3Etag->setText(val(bi.etagere));
        dv3Temp->setText(tempStr);
        // Panel 3 (report)
        rRef ->setText(val(bi.reference));
        rType->setText(val(bi.type));
        rOrg ->setText(val(bi.organisme));
        rProj->setText(val(bi.projet));
        rBsl ->setText(val(bi.bslLevel));
        rQty ->setText(qtyStr);
        rCong->setText(val(bi.congelateur));
        rEtag->setText(val(bi.etagere));
        rTemp->setText(tempStr);
        rDateLbl->setText("Date : " + QDate::currentDate().toString("dd/MM/yyyy"));
    });

    QObject::connect(tree3, &QTreeWidget::itemClicked, this, [=](QTreeWidgetItem* item, int) {
        QString cong = item->data(0, Qt::UserRole).toString();
        QString etag = item->data(0, Qt::UserRole + 1).toString();
        if (cong.isEmpty()) return;
        loadSamples3(cong, etag);
    });

    // ── PDF Export ──
    QObject::connect(pdfBtn, &QPushButton::clicked, this, [=]() {
        if (bioInfos3->isEmpty() || list3->currentRow() < 0) {
            QMessageBox::information(this, "Information",
                "Veuillez sélectionner un échantillon avant d'exporter.");
            return;
        }
        QString path = QFileDialog::getSaveFileName(this, "Enregistrer le rapport PDF",
            "rapport_echantillon.pdf", "PDF (*.pdf)");
        if (path.isEmpty()) return;

        const BasicBioInfo& bi = (*bioInfos3)[list3->currentRow()];
        exportBioSamplePdf(bi, path);
        QMessageBox::information(this, "Succès",
            QString("Rapport exporté avec succès :\n%1").arg(path));
    });

    outer3L->addWidget(left3);
    outer3L->addWidget(mid3, 1);
    outer3L->addWidget(right3);

    p3->addWidget(outer3, 1);

    QFrame* bottom3 = new QFrame;
    bottom3->setFixedHeight(64);
    bottom3->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* bottom3L = new QHBoxLayout(bottom3);
    bottom3L->setContentsMargins(14,10,14,10);

    QPushButton* back3 = actionBtn("Retour", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_ArrowBack), true);
    bottom3L->addWidget(back3);
    bottom3L->addStretch(1);

    p3->addWidget(bottom3);
    addStackPage(page3);

    // ==========================================================
    // PAGE 3 : BioSimple - Rack + Contraintes
    // ==========================================================
    QWidget* page4 = new QWidget;
    QVBoxLayout* p4 = new QVBoxLayout(page4);
    p4->setContentsMargins(22, 18, 22, 18);
    p4->setSpacing(14);

    ModulesBar barBioRack;
    p4->addWidget(makeHeaderBlock(st, "Localisation & Stockage", ModuleTab::BioSimple, &barBioRack));
    connectModulesSwitch(this, stack, barBioRack);

    QFrame* outer4 = new QFrame;
    outer4->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }").arg(C_PANEL_BG, C_PANEL_BR));
    QHBoxLayout* outer4L = new QHBoxLayout(outer4);
    outer4L->setContentsMargins(12,12,12,12);
    outer4L->setSpacing(12);

    QFrame* left4 = softBox();
    left4->setFixedWidth(320);
    QVBoxLayout* left4L = new QVBoxLayout(left4);
    left4L->setContentsMargins(10,10,10,10);
    left4L->setSpacing(10);

    QFrame* dd4 = new QFrame;
    dd4->setStyleSheet("QFrame{ background: rgba(255,255,255,0.72); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QHBoxLayout* dd4L = new QHBoxLayout(dd4);
    dd4L->setContentsMargins(10,8,10,8);
    QLabel* dd4T = new QLabel("Congélateur 01");
    dd4T->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    QToolButton* dd4B = new QToolButton;
    dd4B->setAutoRaise(true);
    dd4B->setIcon(st->standardIcon(QStyle::SP_ArrowDown));
    dd4B->setCursor(Qt::PointingHandCursor);
    dd4L->addWidget(dd4T);
    dd4L->addStretch(1);
    dd4L->addWidget(dd4B);

    QTreeWidget* tree4 = new QTreeWidget;
    tree4->setHeaderHidden(true);
    tree4->setIndentation(18);

    auto* wf1 = new QTreeWidgetItem(tree4, QStringList() << "Congélateur 01");
    auto* wf2 = new QTreeWidgetItem(tree4, QStringList() << "Congélateur 02");
    auto* wf4 = new QTreeWidgetItem(tree4, QStringList() << "Congélateur 04");
    wf1->setIcon(0, st->standardIcon(QStyle::SP_DriveHDIcon));
    wf2->setIcon(0, st->standardIcon(QStyle::SP_DriveHDIcon));
    wf4->setIcon(0, st->standardIcon(QStyle::SP_DriveHDIcon));

    auto* wshA = new QTreeWidgetItem(wf2, QStringList() << "Étagère A");
    auto* wshB = new QTreeWidgetItem(wf2, QStringList() << "Étagère B");
    auto* wsh6 = new QTreeWidgetItem(wf2, QStringList() << "Étagère 6");
    auto* wrm  = new QTreeWidgetItem(wf2, QStringList() << "Temp. ambiante");
    wshA->setIcon(0, st->standardIcon(QStyle::SP_DirIcon));
    wshB->setIcon(0, st->standardIcon(QStyle::SP_DirIcon));
    wsh6->setIcon(0, st->standardIcon(QStyle::SP_DirIcon));
    wrm ->setIcon(0, st->standardIcon(QStyle::SP_FileDialogInfoView));

    tree4->expandAll();
    tree4->setCurrentItem(wf2);

    QFrame* temp4 = w3TempQtyBlock(st, "-80°C", "220");

    QPushButton* export4 = actionBtn("Exporter le rapport", "rgba(10,95,88,0.45)", "rgba(255,255,255,0.92)", st->standardIcon(QStyle::SP_DialogSaveButton), true);
    QPushButton* mark4   = actionBtn("Marquer comme traité", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_DialogApplyButton), true);

    left4L->addWidget(dd4);
    left4L->addWidget(tree4, 1);
    left4L->addWidget(temp4);
    left4L->addWidget(export4);
    left4L->addWidget(mark4);

    QFrame* right4 = softBox();
    QVBoxLayout* right4L = new QVBoxLayout(right4);
    right4L->setContentsMargins(10,10,10,10);
    right4L->setSpacing(10);

    QWidget* filtersRow = new QWidget;
    QHBoxLayout* fr = new QHBoxLayout(filtersRow);
    fr->setContentsMargins(0,0,0,0);
    fr->setSpacing(10);
    fr->addWidget(w4FilterPill("Étagère"));
    fr->addWidget(w4FilterPill("Température"));
    fr->addWidget(w4FilterPill("BSL-3"));
    right4L->addWidget(filtersRow);

    QFrame* rackCard = new QFrame;
    rackCard->setStyleSheet("QFrame{ background: rgba(255,255,255,0.55); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QVBoxLayout* rackCardL = new QVBoxLayout(rackCard);
    rackCardL->setContentsMargins(12,12,12,12);
    QTableWidget* rack = new QTableWidget;
    w4SetupRackTable(rack);
    rackCardL->addWidget(rack);
    right4L->addWidget(rackCard);

    QFrame* accCard = new QFrame;
    accCard->setStyleSheet("QFrame{ background: rgba(255,255,255,0.55); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QVBoxLayout* accCardL = new QVBoxLayout(accCard);
    accCardL->setContentsMargins(12,12,12,12);
    QLabel* accTitle = new QLabel("Contraintes");
    accTitle->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    QTableWidget* accTable = new QTableWidget;
    w4SetupAccountsTable(accTable);
    accCardL->addWidget(accTitle);
    accCardL->addWidget(accTable);
    right4L->addWidget(accCard, 1);

    QWidget* w4BottomRight = new QWidget;
    QHBoxLayout* w4br = new QHBoxLayout(w4BottomRight);
    w4br->setContentsMargins(0,0,0,0);
    w4br->setSpacing(10);
    w4br->addStretch(1);

    QPushButton* btnFolder = actionBtn("Dossier", "rgba(255,255,255,0.72)", C_TEXT_DARK, st->standardIcon(QStyle::SP_DirIcon), true);
    QPushButton* btnSec    = actionBtn("Statistiques", "rgba(255,255,255,0.72)", C_TEXT_DARK, st->standardIcon(QStyle::SP_ComputerIcon), true);

    w4br->addWidget(btnFolder);
    w4br->addWidget(btnSec);
    right4L->addWidget(w4BottomRight);

    outer4L->addWidget(left4);
    outer4L->addWidget(right4, 1);

    p4->addWidget(outer4, 1);

    QFrame* bottom4 = new QFrame;
    bottom4->setFixedHeight(64);
    bottom4->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* bottom4L = new QHBoxLayout(bottom4);
    bottom4L->setContentsMargins(14,10,14,10);

    QPushButton* back4 = actionBtn("Retour", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_ArrowBack), true);
    bottom4L->addWidget(back4);
    bottom4L->addStretch(1);

    p4->addWidget(bottom4);
    addStackPage(page4);

    // ==========================================================
    // PAGE 4 : BioSimple - STATISTIQUES (Dashboard analytique)
    // ==========================================================
    QWidget* page5 = new QWidget;
    QVBoxLayout* p5 = new QVBoxLayout(page5);
    p5->setContentsMargins(18, 14, 18, 14);
    p5->setSpacing(10);

    ModulesBar barBioStats;
    p5->addWidget(makeHeaderBlock(st, "Statistiques BioSample", ModuleTab::BioSimple, &barBioStats));
    connectModulesSwitch(this, stack, barBioStats);

    // ── Palette de couleurs des graphiques ──────────────────
    static const QList<QColor> statsTypePalette = {
        QColor("#2DD4BF"), QColor("#0891B2"), QColor("#6366F1"),
        QColor("#F59E0B"), QColor("#F43F5E"), QColor("#84CC16"),
        QColor("#A855F7"), QColor("#FB923C")
    };
    static const QList<QColor> statsTempPalette = {
        QColor("#60A5FA"), // -80°C (glace)
        QColor("#34D399"), // -20°C (vert froid)
        QColor("#FBBF24"), //  4°C  (ambre)
        QColor("#F87171")  // ambiant (rouge doux)
    };

    // ── Helper : carte de données chart ─────────────────────
    auto makeBioStatCard = [&](const QString& title) -> QPair<QFrame*, QVBoxLayout*> {
        QFrame* card = new QFrame;
        card->setObjectName("statCard");
        card->setStyleSheet(
            "QFrame#statCard{"
            "  background: rgba(30,42,52,0.88);"
            "  border: 1px solid rgba(255,255,255,0.09);"
            "  border-radius: 14px;"
            "}"
        );
        QVBoxLayout* lay = new QVBoxLayout(card);
        lay->setContentsMargins(14, 12, 14, 12);
        lay->setSpacing(8);
        QLabel* lbl = new QLabel(title);
        lbl->setObjectName("statCardTitle");
        lbl->setStyleSheet(
            "QLabel#statCardTitle{"
            "  color: rgba(80,210,190,1.0);"
            "  font-size: 12px; font-weight: 900;"
            "  border: none; background: none;"
            "}"
        );
        lay->addWidget(lbl);
        return QPair<QFrame*, QVBoxLayout*>{card, lay};
    };

    // ── Helper : légende pour donut chart ───────────────────
    auto buildLegendRow = [&](QVBoxLayout* lgL, const QColor& col, const QString& text) {
        QWidget* row = new QWidget;
        QHBoxLayout* h = new QHBoxLayout(row);
        h->setContentsMargins(0,1,0,1); h->setSpacing(8);
        QFrame* dot = new QFrame;
        dot->setFixedSize(10,10);
        dot->setStyleSheet(QString("background:%1; border-radius:5px;").arg(col.name()));
        QLabel* lab = new QLabel(text);
        lab->setStyleSheet("color: rgba(180,205,215,0.90); font-size: 11px; font-weight: 700;"
                           " background: none; border: none;");
        h->addWidget(dot);
        h->addWidget(lab, 1);
        lgL->addWidget(row);
    };
    Q_UNUSED(buildLegendRow); // used in updateBioStats

    // ── Helper KPI card (thème-aware via objectName) ─────────
    auto makeKpiCard = [&](const QString& icon, const QString& title, const QColor& accent)
        -> QPair<QFrame*, QLabel*>
    {
        QFrame* card = new QFrame;
        card->setObjectName("kpiCard");
        card->setMinimumHeight(96);
        QVBoxLayout* lay = new QVBoxLayout(card);
        lay->setContentsMargins(16,12,16,12);
        lay->setSpacing(3);

        QLabel* iconLbl = new QLabel(icon);
        iconLbl->setObjectName("kpiIcon");
        iconLbl->setStyleSheet(QString(
            "color:%1; font-size:20px; background:none; border:none;").arg(accent.name()));

        QLabel* valLbl = new QLabel("—");
        valLbl->setObjectName("kpiVal");
        valLbl->setStyleSheet(QString(
            "color:%1; font-size:24px; font-weight:900; background:none; border:none;"
        ).arg(accent.name()));

        QLabel* titleLbl = new QLabel(title);
        titleLbl->setObjectName("kpiTitle");
        titleLbl->setStyleSheet(
            "color: rgba(100,120,135,1.0); font-size:10px; font-weight:700;"
            " background:none; border:none;");
        titleLbl->setWordWrap(true);

        lay->addWidget(iconLbl);
        lay->addWidget(valLbl);
        lay->addWidget(titleLbl);
        lay->addStretch(1);
        return QPair<QFrame*, QLabel*>{card, valLbl};
    };

    // ── KPI cards ────────────────────────────────────────────
    QWidget* kpiRow = new QWidget;
    QHBoxLayout* bioKpiL = new QHBoxLayout(kpiRow);
    bioKpiL->setContentsMargins(0,0,0,0);
    bioKpiL->setSpacing(10);

    auto [kpiCardTotal,   kpiTotalVal]  = makeKpiCard("🔬", "Total Échantillons",       QColor("#2DD4BF"));
    auto [kpiCardTypes,   kpiTypesVal]  = makeKpiCard("🧬", "Types distincts",          QColor("#6366F1"));
    auto [kpiCardMonth,   kpiMonthVal]  = makeKpiCard("📅", "Collectés ce mois",        QColor("#F59E0B"));
    auto [kpiCardTopRef,  kpiTopRefVal] = makeKpiCard("📦", "Référence max (quantité)", QColor("#F43F5E"));
    auto [kpiCardTopTemp, kpiTopTempVal]= makeKpiCard("🌡", "Température dominante",    QColor("#60A5FA"));

    bioKpiL->addWidget(kpiCardTotal,   1);
    bioKpiL->addWidget(kpiCardTypes,   1);
    bioKpiL->addWidget(kpiCardMonth,   1);
    bioKpiL->addWidget(kpiCardTopRef,  1);
    bioKpiL->addWidget(kpiCardTopTemp, 1);

    // ── Donut types + Donut températures ────────────────────
    QWidget* donutRow = new QWidget;
    QHBoxLayout* donutL = new QHBoxLayout(donutRow);
    donutL->setContentsMargins(0,0,0,0);
    donutL->setSpacing(10);

    auto [typeCard, typeLay] = makeBioStatCard("Répartition par type d’échantillon");
    DonutChart* pie = new DonutChart;
    pie->setMinimumHeight(200);
    typeLay->addWidget(pie, 1);
    QFrame* typeLgFrame = new QFrame;
    typeLgFrame->setStyleSheet("QFrame{ background:none; border:none; }");
    QVBoxLayout* typeLgL = new QVBoxLayout(typeLgFrame);
    typeLgL->setContentsMargins(0,4,0,0);
    typeLgL->setSpacing(3);
    typeLay->addWidget(typeLgFrame);
    donutL->addWidget(typeCard, 1);

    auto [tempCard, tempLay] = makeBioStatCard("Répartition par température de stockage");
    DonutChart* tempPie = new DonutChart;
    tempPie->setMinimumHeight(200);
    tempLay->addWidget(tempPie, 1);
    QFrame* tempLgFrame = new QFrame;
    tempLgFrame->setStyleSheet("QFrame{ background:none; border:none; }");
    QVBoxLayout* tempLgL = new QVBoxLayout(tempLgFrame);
    tempLgL->setContentsMargins(0,4,0,0);
    tempLgL->setSpacing(3);
    tempLay->addWidget(tempLgFrame);
    donutL->addWidget(tempCard, 1);

    // ── Bar chart mensuel + Horizontal bar quantités ─────────
    QWidget* barRow = new QWidget;
    QHBoxLayout* barRowL = new QHBoxLayout(barRow);
    barRowL->setContentsMargins(0,0,0,0);
    barRowL->setSpacing(10);

    auto [monthCard, monthLay] = makeBioStatCard("Échantillons collectés par mois");
    BarChart* bars = new BarChart;
    bars->setMinimumHeight(200);
    monthLay->addWidget(bars, 1);
    barRowL->addWidget(monthCard, 3);

    auto [qtyCard, qtyLay] = makeBioStatCard("Quantité restante par référence (Top 8)");
    HorizontalBarChart* hbars = new HorizontalBarChart;
    hbars->setMinimumHeight(200);
    qtyLay->addWidget(hbars, 1);
    barRowL->addWidget(qtyCard, 2);

    // ── Zone scrollable (KPI + donuts + barres) ───────────────
    QWidget* scrollContent = new QWidget;
    scrollContent->setObjectName("scrollContent");
    scrollContent->setStyleSheet("QWidget#scrollContent{ background: transparent; }");
    QVBoxLayout* scrollLayout = new QVBoxLayout(scrollContent);
    scrollLayout->setContentsMargins(0, 0, 8, 0);   // 8px droite = espace barre scroll
    scrollLayout->setSpacing(10);
    scrollLayout->addWidget(kpiRow);
    scrollLayout->addWidget(donutRow);
    scrollLayout->addWidget(barRow);
    scrollLayout->addStretch(1);

    QScrollArea* statsScroll = new QScrollArea;
    statsScroll->setObjectName("statsScroll");
    statsScroll->setWidget(scrollContent);
    statsScroll->setWidgetResizable(true);
    statsScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    statsScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    statsScroll->setFrameShape(QFrame::NoFrame);
    statsScroll->setStyleSheet(R"(
        QScrollArea#statsScroll {
            background: transparent;
            border: none;
        }
        QScrollArea#statsScroll > QWidget > QWidget {
            background: transparent;
        }
        QScrollBar:vertical {
            background: rgba(0,0,0,0.06);
            width: 6px;
            border-radius: 3px;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background: rgba(45,212,191,0.50);
            border-radius: 3px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover {
            background: rgba(45,212,191,0.80);
        }
        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical { height: 0px; }
        QScrollBar::add-page:vertical,
        QScrollBar::sub-page:vertical { background: none; }
    )");

    p5->addWidget(statsScroll, 1);

    // ── Barre du bas ─────────────────────────────────────────
    QFrame* bottom5 = new QFrame;
    bottom5->setObjectName("statsBottom");
    bottom5->setFixedHeight(58);
    QHBoxLayout* bottom5L = new QHBoxLayout(bottom5);
    bottom5L->setContentsMargins(14, 8, 14, 8);

    QPushButton* back5 = actionBtn("Retour", "rgba(255,255,255,0.55)", C_TEXT_DARK,
                                   st->standardIcon(QStyle::SP_ArrowBack), true);
    bottom5L->addWidget(back5);
    bottom5L->addStretch(1);
    p5->addWidget(bottom5);

    addStackPage(page5);

    // ==========================================================
    // =====================  GESTION PROJET  ====================
    // ==========================================================

    // ==========================================================
    // PAGE 5 : Gestion Projet - Widget 1 (LISTE)
    // ==========================================================
    QWidget* proj1 = new QWidget;
    QVBoxLayout* gp1 = new QVBoxLayout(proj1);
    gp1->setContentsMargins(22, 18, 22, 18);
    gp1->setSpacing(14);

    ModulesBar barProjList;
    gp1->addWidget(makeHeaderBlock(st, "Projets de Recherche", ModuleTab::GestionProjet, &barProjList));
    connectModulesSwitch(this, stack, barProjList);

    QFrame* pBar = new QFrame;
    pBar->setFixedHeight(54);
    pBar->setAutoFillBackground(true);
    pBar->setStyleSheet("background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 rgba(255,255,255,0.18), stop:1 rgba(255,255,255,0.18)); border: 1px solid rgba(255,255,255,0.28); border-radius: 14px;");
    QHBoxLayout* pBarL = new QHBoxLayout(pBar);
    pBarL->setContentsMargins(14, 8, 14, 8);
    pBarL->setSpacing(10);

    QLineEdit* pSearch = new QLineEdit;
    pSearch->setPlaceholderText("Rechercher par tous");
    pSearch->addAction(st->standardIcon(QStyle::SP_FileDialogContentsView), QLineEdit::LeadingPosition);

    QComboBox* pDomain = new QComboBox;
    pDomain->addItems({"Domaine", "Génomique", "Protéomique", "Pharmacologie", "Immunologie", "Biologie végétale", "Microbiologie", "Neurosciences", "Biotechnologies", "Génétique", "Bioinformatique"});
    pDomain->setStyleSheet(
        "QComboBox{ background:rgba(255,255,255,0.92); border:1.5px solid rgba(0,0,0,0.20); "
        "border-radius:12px; padding:10px 14px; color:#12443B; font-weight:800; font-size:13px; }"
        "QComboBox:focus{ border:1.5px solid #0A5F58; background:rgba(255,255,255,1.0); }"
        "QComboBox::drop-down{ border:0px; width:24px; }"
        "QComboBox QAbstractItemView{ background:white; border:1.5px solid rgba(0,0,0,0.18); "
        "border-radius:8px; selection-background-color:#0A5F58; selection-color:white; "
        "color:#12443B; font-weight:700; font-size:12px; padding:4px; outline:none; }");

    QComboBox* pStatut = new QComboBox;
    pStatut->addItems({"Statut", "Planifié", "En cours", "En retard", "Critique", "Suspendu", "Terminé", "Annulé"});
    pStatut->setStyleSheet(
        "QComboBox{ background:rgba(255,255,255,0.92); border:1.5px solid rgba(0,0,0,0.20); "
        "border-radius:12px; padding:10px 14px; color:#12443B; font-weight:800; font-size:13px; }"
        "QComboBox:focus{ border:1.5px solid #0A5F58; background:rgba(255,255,255,1.0); }"
        "QComboBox::drop-down{ border:0px; width:24px; }"
        "QComboBox QAbstractItemView{ background:white; border:1.5px solid rgba(0,0,0,0.18); "
        "border-radius:8px; selection-background-color:#0A5F58; selection-color:white; "
        "color:#12443B; font-weight:700; font-size:12px; padding:4px; outline:none; }");

    QComboBox* pBudget = new QComboBox;
    pBudget->addItems({"Budget", "500 - 50 000 TND", "50 000 - 500 000 TND", "500 000 - 5 000 000 TND", "5 000 000+ TND"});
    pBudget->setStyleSheet(
        "QComboBox{ background:rgba(255,255,255,0.92); border:1.5px solid rgba(0,0,0,0.20); "
        "border-radius:12px; padding:10px 14px; color:#12443B; font-weight:800; font-size:13px; }"
        "QComboBox:focus{ border:1.5px solid #0A5F58; background:rgba(255,255,255,1.0); }"
        "QComboBox::drop-down{ border:0px; width:24px; }"
        "QComboBox QAbstractItemView{ background:white; border:1.5px solid rgba(0,0,0,0.18); "
        "border-radius:8px; selection-background-color:#0A5F58; selection-color:white; "
        "color:#12443B; font-weight:700; font-size:12px; padding:4px; outline:none; }");

    QPushButton* pFilters = new QPushButton(st->standardIcon(QStyle::SP_FileDialogDetailedView), "  Filtres");
    pFilters->setCursor(Qt::PointingHandCursor);
    pFilters->setStyleSheet(QString(R"(
        QPushButton{
            background:%1; color: rgba(255,255,255,0.92);
            border:1px solid rgba(0,0,0,0.18);
            border-radius: 12px; padding: 10px 16px; font-weight: 800;
        }
        QPushButton:hover{ background: %2; }
    )").arg(C_PRIMARY, C_TOPBAR));

    pBarL->addWidget(pSearch, 1);
    pBarL->addWidget(pDomain);
    pBarL->addWidget(pStatut);
    pBarL->addWidget(pBudget);
    pBarL->addWidget(pFilters);
    gp1->addWidget(pBar);

    QFrame* projCard = makeCard();
    QVBoxLayout* projCardL = new QVBoxLayout(projCard);
    projCardL->setContentsMargins(10,10,10,10);

    GestProjCrud* projCrud     = new GestProjCrud;
    bool*         projEditMode = new bool(false);
    int*          projEditId   = new int(0);

    QTableWidget* projTable = new QTableWidget(0, 10);
    projTable->setHorizontalHeaderLabels({"", "Nom du projet","Domaine","Date début","Date fin","Budget","Statut","Financement","Approbation éthique","Publications"});
    projTable->verticalHeader()->setVisible(false);
    projTable->setShowGrid(true);
    projTable->setAlternatingRowColors(true);
    projTable->horizontalHeader()->setStretchLastSection(true);
    projTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    projTable->setSelectionMode(QAbstractItemView::SingleSelection);
    projTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    projTable->setItemDelegateForColumn(6, new ProjetBadgeDelegate(projTable));

    projTable->setColumnWidth(0, 36);
    projTable->setColumnWidth(1, 180);
    projTable->setColumnWidth(2, 130);
    projTable->setColumnWidth(3, 100);
    projTable->setColumnWidth(4, 100);
    projTable->setColumnWidth(5, 90);
    projTable->setColumnWidth(6, 130);
    projTable->setColumnWidth(7, 120);
    projTable->setColumnWidth(8, 140);
    projTable->setColumnWidth(9, 90);

    auto setProjRow=[=](const ProjetRecord& rec)
    {
        const int r = projTable->rowCount();
        projTable->insertRow(r);

        QTableWidgetItem* iconItem = new QTableWidgetItem;
        iconItem->setIcon(st->standardIcon(QStyle::SP_ArrowRight));
        iconItem->setTextAlignment(Qt::AlignCenter);
        projTable->setItem(r, 0, iconItem);

        auto mk = [&](const QString& t){
            QTableWidgetItem* it = new QTableWidgetItem(t);
            it->setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);
            return it;
        };

        QTableWidgetItem* nameItem = mk(rec.nomDuProjet);
        nameItem->setData(Qt::UserRole, rec.idProjet);
        projTable->setItem(r, 1, nameItem);
        projTable->setItem(r, 2, mk(rec.domaineDeRecherche));
        projTable->setItem(r, 3, mk(rec.dateDeDebut.isValid() ? rec.dateDeDebut.toString("dd/MM/yyyy") : "-"));
        projTable->setItem(r, 4, mk(rec.dateDeFin.isValid() ? rec.dateDeFin.toString("dd/MM/yyyy") : "-"));

        QTableWidgetItem* b = mk(QString::number(rec.budget, 'f', 2));
        b->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
        projTable->setItem(r, 5, b);

        QTableWidgetItem* badge = new QTableWidgetItem;
        badge->setData(Qt::UserRole, (int)projStatusFromText(rec.statut));
        projTable->setItem(r, 6, badge);

        projTable->setItem(r, 7, mk(rec.sourceDeFinancement));
        projTable->setItem(r, 8, mk(rec.numeroDApprobationEthique));

        QTableWidgetItem* pubItem = mk(QString::number(rec.nombreDePublications));
        pubItem->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
        projTable->setItem(r, 9, pubItem);

        projTable->setRowHeight(r, 46);
    };

    auto loadProjTable = [=]() {
        QList<ProjetRecord> recs;
        QString err;

        const QString domFilter = (pDomain->currentIndex() > 0) ? pDomain->currentText() : QString();
        const QString statFilter = (pStatut->currentIndex() > 0) ? pStatut->currentText() : QString();

        if (!projCrud->loadProjets(recs, &err, pSearch->text().trimmed(), domFilter, statFilter)) {
            projTable->setRowCount(0);
            return;
        }

        projTable->setRowCount(0);

        for (const ProjetRecord& rec : recs) {
            setProjRow(rec);
        }
    };

    auto applyProjFilters = [=]() {
        const QString search = pSearch->text().trimmed().toLower();
        const QString budgetFilter = (pBudget->currentIndex() > 0) ? pBudget->currentText() : QString();

        for (int r = 0; r < projTable->rowCount(); ++r) {
            bool matchSearch = true;
            if (!search.isEmpty()) {
                matchSearch = false;
                for (int c = 1; c < projTable->columnCount(); ++c) {
                    QTableWidgetItem* it = projTable->item(r, c);
                    if (c == 6) {
                        ProjStatus ps = static_cast<ProjStatus>(it->data(Qt::UserRole).toInt());
                        if (projStatusText(ps).toLower().contains(search)) { matchSearch = true; break; }
                    } else if (it && it->text().toLower().contains(search)) { matchSearch = true; break; }
                }
            }

            bool matchBudget = true;
            if (!budgetFilter.isEmpty() && projTable->item(r, 5)) {
                double val = projTable->item(r, 5)->text().toDouble();
                if (budgetFilter == "500 - 50 000 TND")            matchBudget = val >= 500 && val < 50000;
                else if (budgetFilter == "50 000 - 500 000 TND")   matchBudget = val >= 50000 && val < 500000;
                else if (budgetFilter == "500 000 - 5 000 000 TND")matchBudget = val >= 500000 && val < 5000000;
                else if (budgetFilter == "5 000 000+ TND")          matchBudget = val >= 5000000;
            }

            projTable->setRowHidden(r, !(matchSearch && matchBudget));
        }
    };

    loadProjTable();

    QObject::connect(pSearch, &QLineEdit::textChanged, this, [=](const QString&){ applyProjFilters(); });
    QObject::connect(pDomain, &QComboBox::currentTextChanged, this, [=](const QString&){ loadProjTable(); applyProjFilters(); });
    QObject::connect(pStatut, &QComboBox::currentTextChanged, this, [=](const QString&){ loadProjTable(); applyProjFilters(); });
    QObject::connect(pBudget, &QComboBox::currentTextChanged, this, [=](const QString&){ applyProjFilters(); });
    QObject::connect(pFilters, &QPushButton::clicked, this, [=](){ loadProjTable(); applyProjFilters(); });

    QObject::connect(stack, &QStackedWidget::currentChanged, projTable, [=](int idx){
        if (idx == PROJ_LIST) {
            loadProjTable();
            applyProjFilters();
        }
    });

    projCardL->addWidget(projTable);
    gp1->addWidget(projCard, 1);

    QFrame* projBottom = new QFrame;
    projBottom->setFixedHeight(64);
    projBottom->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* projBottomL = new QHBoxLayout(projBottom);
    projBottomL->setContentsMargins(14,10,14,10);
    projBottomL->setSpacing(12);

    QPushButton* projAdd     = actionBtn("Ajouter",      "rgba(10,95,88,0.45)", "rgba(255,255,255,0.90)", st->standardIcon(QStyle::SP_DialogYesButton), true);
    QPushButton* projEdit    = actionBtn("Modifier",     "rgba(198,178,154,0.55)", "rgba(255,255,255,0.85)", st->standardIcon(QStyle::SP_FileDialogContentsView), true);
    QPushButton* projDel     = actionBtn("Supprimer",    "rgba(255,255,255,0.55)", "#B14A4A", st->standardIcon(QStyle::SP_TrashIcon), true);
    QPushButton* projDetails = actionBtn("Détails",      "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_DesktopIcon), true);

    QObject::connect(projDel, &QPushButton::clicked, this, [=](){
        if (*currentRole != "Responsable") { showToast(this, "Accès refusé : seul le Responsable peut supprimer.", false); return; }
        int r = projTable->currentRow();
        if (r < 0) {
            ThemedAlertDialog::show(style(), this, "info", "Projet", "Sélectionnez un projet dans la liste.");
            return;
        }
        if (!projTable->item(r,1)) {
            showToast(this, "Ligne projet invalide.", false);
            return;
        }
        const int id = projTable->item(r,1)->data(Qt::UserRole).toInt();
        if (id <= 0) {
            showToast(this, "ID projet invalide.", false);
            return;
        }
        ProjStatus ps = static_cast<ProjStatus>(projTable->item(r,6)->data(Qt::UserRole).toInt());
        QString resume = QString("Projet : %1 | Statut : %2")
                             .arg(projTable->item(r,1)->text(),
                                  projStatusText(ps));
        // Vérifier si le projet contient des expériences ou des échantillons
        {
            int nbExp = 0, nbEch = 0;
            QSqlQuery qChk;
            qChk.prepare("SELECT COUNT(*) FROM \"Expérience\" WHERE \"Id_projet\" = :id");
            qChk.bindValue(":id", id);
            if (qChk.exec() && qChk.next()) nbExp = qChk.value(0).toInt();

            QSqlQuery qChk2;
            qChk2.prepare("SELECT COUNT(*) FROM \"BioSample\" WHERE \"Id_projet\" = :id");
            qChk2.bindValue(":id", id);
            if (qChk2.exec() && qChk2.next()) nbEch = qChk2.value(0).toInt();

            if (nbExp > 0 || nbEch > 0) {
                QStringList raisons;
                if (nbExp > 0) raisons << QString("%1 expérience(s)").arg(nbExp);
                if (nbEch > 0) raisons << QString("%1 échantillon(s)").arg(nbEch);
                showToast(this,
                    "Impossible de supprimer : ce projet contient " + raisons.join(" et ") + ". Supprimez-les d'abord.",
                    false);
                return;
            }
        }

        ConfirmDeleteDialog confirm(style(), resume, this);
        if (confirm.exec() == QDialog::Accepted) {
            ProjetRecord deletedRec;
            QString err;
            if (!projCrud->fetchProjet(id, deletedRec, &err)) {
                showToast(this, "Erreur : " + err, false);
                return;
            }

            QString deleteErr;
            if (!projCrud->deleteProjet(id, &deleteErr)) {
                showToast(this, "Erreur : " + deleteErr, false);
                return;
            }
            TracabiliteManager::logSuppressionProjet(deletedRec);
            loadProjTable();
            applyProjFilters();
            showToast(this, "Projet supprimé.", true);
        }
    });

    projBottomL->addWidget(projAdd);
    projBottomL->addWidget(projEdit);
    projBottomL->addWidget(projDel);
    projBottomL->addWidget(projDetails);
    projBottomL->addStretch(1);

    QPushButton* projMore = new QPushButton(st->standardIcon(QStyle::SP_FileDialogContentsView), "  Détails avancés");
    projMore->setCursor(Qt::PointingHandCursor);
    projMore->setStyleSheet(R"(
        QPushButton{
            background: rgba(255,255,255,0.55);
            border: 1px solid rgba(0,0,0,0.12);
            border-radius: 12px;
            padding: 10px 14px;
            color: rgba(0,0,0,0.65);
            font-weight: 800;
        }
        QPushButton:hover{ background: rgba(255,255,255,0.75); }
    )");
    projBottomL->addWidget(projMore);

    QPushButton* projBtnStats = new QPushButton(st->standardIcon(QStyle::SP_ComputerIcon), "  Statistiques");
    projBtnStats->setCursor(Qt::PointingHandCursor);
    projBtnStats->setStyleSheet(QString(R"(
        QPushButton{
            background: rgba(10,95,88,0.55);
            color: rgba(255,255,255,0.92);
            border: 1px solid rgba(0,0,0,0.12);
            border-radius: 12px;
            padding: 10px 18px;
            font-weight: 800;
        }
        QPushButton:hover{ background: %1; }
    )").arg(C_TOPBAR));
    projBottomL->addWidget(projBtnStats);

    QToolButton* projExportPdf = new QToolButton;
    projExportPdf->setIcon(st->standardIcon(QStyle::SP_ArrowDown));
    projExportPdf->setToolTip("Exporter PDF");
    projExportPdf->setCursor(Qt::PointingHandCursor);
    projExportPdf->setFixedSize(42, 42);
    projExportPdf->setStyleSheet(R"(
        QToolButton{
            background: #719790;
            border: 1px solid rgba(0,0,0,0.15);
            border-radius: 10px;
            padding: 8px;
        }
        QToolButton:hover{ background: #4d8f83; }
    )");
    projBottomL->addWidget(projExportPdf);

    gp1->addWidget(projBottom);

    addStackPage(proj1);

    // ==========================================================
    // PAGE 6 : Gestion Projet - Widget 2 (AJOUT/MODIF)
    // ==========================================================
    QWidget* proj2 = new QWidget;
    proj2->setObjectName("projFormPage");
    proj2->setStyleSheet(QString("QWidget { background: %1; }").arg(C_BG));
    QVBoxLayout* gp2 = new QVBoxLayout(proj2);
    gp2->setContentsMargins(22, 18, 22, 18);
    gp2->setSpacing(14);

    ModulesBar barProjForm;
    QWidget* projFormHeader = makeHeaderBlock(st, "Ajouter un projet", ModuleTab::GestionProjet, &barProjForm);
    gp2->addWidget(projFormHeader);
    connectModulesSwitch(this, stack, barProjForm);

    QLabel* projFormHeaderTitle = projFormHeader->findChild<QLabel*>("topBarTitle");

    // ── Shared field styles ───────────────────────────────────
    auto projFieldStyle = [](bool error = false) {
        const bool dark = g_darkThemeEnabled;
        const QString bg = error
            ? (dark ? "rgba(122,48,58,0.24)" : "rgba(255,240,240,0.96)")
            : (dark ? "rgba(255,255,255,0.07)" : "rgba(255,255,255,0.92)");
        const QString br = error
            ? "#c0392b"
            : (dark ? "rgba(210,218,226,0.22)" : "rgba(0,0,0,0.20)");
        const QString fg = dark ? "#E8ECF0" : "rgba(0,0,0,0.88)";
        return QString(
            "background:%1; border:1.5px solid %2; border-radius:12px;"
            "padding:12px 16px; color:%3; font-weight:800; font-size:14px; min-height:20px;")
            .arg(bg, br, fg);
    };
    auto projComboStyle = [](bool error = false) {
        const bool dark = g_darkThemeEnabled;
        const QString bg = error
            ? (dark ? "rgba(122,48,58,0.24)" : "rgba(255,240,240,0.96)")
            : (dark ? "rgba(255,255,255,0.07)" : "rgba(255,255,255,0.92)");
        const QString br = error
            ? "#c0392b"
            : (dark ? "rgba(210,218,226,0.22)" : "rgba(0,0,0,0.20)");
        const QString fg = dark ? "#E8ECF0" : "rgba(0,0,0,0.88)";
        const QString popupBg = dark ? "rgba(20,27,34,0.98)" : "#FFFFFF";
        const QString popupSelBg = dark ? "rgba(45,212,191,0.22)" : "rgba(10,95,88,0.16)";
        const QString popupSelFg = dark ? "#F4FBFF" : "#0A5F58";
        return QString(
            "QComboBox{ background:%1; border:1.5px solid %2; border-radius:12px;"
            " padding:10px 14px; color:%3; font-weight:800; font-size:14px; min-height:20px; }"
            "QComboBox::drop-down{ border:0px; width:24px; }"
            "QComboBox QAbstractItemView{ background:%4; border:1px solid %2; border-radius:10px;"
            " selection-background-color:%5; selection-color:%6; color:%3; font-weight:700;"
            " font-size:13px; padding:4px; outline:none; }")
            .arg(bg, br, fg, popupBg, popupSelBg, popupSelFg);
    };
    auto projDateStyle = [](bool error = false) {
        const bool dark = g_darkThemeEnabled;
        const QString bg = error
            ? (dark ? "rgba(122,48,58,0.24)" : "rgba(255,240,240,0.96)")
            : (dark ? "rgba(255,255,255,0.07)" : "rgba(255,255,255,0.92)");
        const QString br = error
            ? "#c0392b"
            : (dark ? "rgba(210,218,226,0.22)" : "rgba(0,0,0,0.20)");
        const QString fg = dark ? "#E8ECF0" : "#12443B";
        return QString(
            "QDateEdit{ background:%1; border:1.5px solid %2; border-radius:12px;"
            " padding:12px 16px; color:%3; font-weight:800; font-size:14px; }"
            "QDateEdit::drop-down{ border:0px; width:20px; }"
            "QDateEdit::up-button{ width:0; } QDateEdit::down-button{ width:0; }")
            .arg(bg, br, fg);
    };
    auto projPubsStyle = []() {
        const bool dark = g_darkThemeEnabled;
        const QString bg = dark ? "rgba(255,255,255,0.07)" : "rgba(255,255,255,0.92)";
        const QString br = dark ? "rgba(210,218,226,0.22)" : "rgba(0,0,0,0.20)";
        const QString fg = dark ? "#E8ECF0" : "#12443B";
        const QString btnBr = dark ? "rgba(210,218,226,0.16)" : "rgba(0,0,0,0.12)";
        return QString(
            "QSpinBox{ background:%1; border:1.5px solid %2; border-radius:14px;"
            " padding:14px 18px; color:%3; font-weight:900; font-size:16px; }"
            "QSpinBox::up-button{ subcontrol-origin:border; subcontrol-position:right; width:24px;"
            " border-left:1px solid %4; border-radius:0 12px 12px 0; }"
            "QSpinBox::down-button{ width:0; }")
            .arg(bg, br, fg, btnBr);
    };
    auto projBudgetStyle = []() {
        const bool dark = g_darkThemeEnabled;
        const QString bg = dark ? "rgba(255,255,255,0.07)" : "rgba(255,255,255,0.92)";
        const QString br = dark ? "rgba(210,218,226,0.22)" : "rgba(0,0,0,0.20)";
        const QString fg = dark ? "#E8ECF0" : "#1a2e6e";
        const QString btnBr = dark ? "rgba(210,218,226,0.16)" : "rgba(0,0,0,0.12)";
        return QString(
            "QDoubleSpinBox{ background:%1; border:1.5px solid %2; border-radius:12px;"
            " padding:16px 44px 16px 24px; color:%3; font-weight:800; font-size:20px; }"
            "QDoubleSpinBox::up-button{ subcontrol-origin:border; subcontrol-position:top right; width:28px;"
            " border-left:1px solid %4; border-top-right-radius:12px; }"
            "QDoubleSpinBox::down-button{ subcontrol-origin:border; subcontrol-position:bottom right; width:28px;"
            " border-left:1px solid %4; border-bottom-right-radius:12px; }")
            .arg(bg, br, fg, btnBr);
    };

    // ── Helper: inline error label ────────────────────────────
    auto mkProjErr = []() -> QLabel* {
        auto* l = new QLabel;
        l->setWordWrap(true);
        l->setStyleSheet("color:#c0392b; font-size:10px; font-weight:700; padding:0 2px; background:transparent;");
        l->hide();
        return l;
    };
    auto showProjErr = [](QLabel* lbl, const QString& msg, QWidget* field, const QString& errStyle){
        lbl->setText("⚠  " + msg);
        lbl->show();
        field->setStyleSheet(errStyle);
    };
    auto clearProjErr = [](QLabel* lbl, QWidget* field, const QString& okStyle){
        lbl->hide();
        field->setStyleSheet(okStyle);
    };

    // ── Section title helper ──────────────────────────────────
    auto projTitle = [&](const QString& t){
        QLabel* lab = new QLabel(t);
        lab->setObjectName("projFormTitle");
        lab->setStyleSheet("color:rgba(10,95,88,1.0); font-weight:900; font-size:15px; padding:4px 0;");
        return lab;
    };

    // ── Row helper ────────────────────────────────────────────
    auto projRow = [&](QStyle::StandardPixmap sp, const QString& label, QWidget* input){
        QFrame* r = softBox();
        r->setObjectName("projFormRow");
        r->setMinimumHeight(68);
        QHBoxLayout* l = new QHBoxLayout(r);
        l->setContentsMargins(16,14,16,14);
        l->setSpacing(14);
        QToolButton* ic = new QToolButton;
        ic->setAutoRaise(true);
        ic->setIcon(st->standardIcon(sp));
        ic->setIconSize(QSize(22,22));
        QLabel* lab = new QLabel(label);
        lab->setObjectName("projFormRowLabel");
        lab->setStyleSheet("color:#12443B; font-weight:900; font-size:14px;");
        l->addWidget(ic); l->addWidget(lab); l->addStretch(1); l->addWidget(input);
        return r;
    };

    // ── Helper: apply white themed calendar to a QDateEdit ──────
    auto applyProjCalendar = [](QDateEdit* de) {
        QCalendarWidget* cw = de->calendarWidget();
        if (!cw) return;
        const bool dark = g_darkThemeEnabled;
        cw->setStyleSheet(QString(
            "QCalendarWidget QWidget#qt_calendar_navigationbar {"
            "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 %1, stop:1 %2);"
            "  padding: 4px 6px; border-radius: 10px 10px 0 0;"
            "}"
            "QCalendarWidget QToolButton {"
            "  color: white; font-weight: 900; font-size: 13px;"
            "  background: transparent; border: none;"
            "  border-radius: 6px; padding: 4px 10px; min-width: 28px;"
            "}"
            "QCalendarWidget QToolButton:hover  { background: rgba(255,255,255,0.22); }"
            "QCalendarWidget QToolButton:pressed { background: rgba(255,255,255,0.12); }"
            "QCalendarWidget QSpinBox {"
            "  color: white; background: transparent; border: none;"
            "  font-weight: 900; selection-background-color: rgba(255,255,255,0.30);"
            "}"
            "QCalendarWidget QHeaderView::section {"
            "  background: %3; color: %4;"
            "  font-weight: 900; font-size: 11px;"
            "  border: none; padding: 5px 0;"
            "}"
            "QCalendarWidget QAbstractItemView {"
            "  background: %5;"
            "  selection-background-color: %6;"
            "  selection-color: white;"
            "  color: %7;"
            "  border: 1px solid %8;"
            "  font-weight: 700; outline: none;"
            "}"
            "QCalendarWidget QAbstractItemView:disabled { color: %9; }"
            "QCalendarWidget QWidget { alternate-background-color: %10; }"
            "QCalendarWidget QMenu {"
            "  background: %5; color: %11;"
            "  selection-background-color: %6; selection-color: white;"
            "}").arg(
                dark ? "#16222D" : "#0A5F58",
                dark ? "#223646" : "#12443B",
                dark ? "rgba(45,212,191,0.18)" : "#A3CAD3",
                dark ? "#CFF7F1" : "#12443B",
                dark ? "#18212B" : "#ffffff",
                dark ? "rgba(45,212,191,0.25)" : "#0A5F58",
                dark ? "#E8ECF0" : "rgba(0,0,0,0.80)",
                dark ? "rgba(210,218,226,0.18)" : "#A3CAD3",
                dark ? "rgba(180,195,205,0.45)" : "#b0bec5",
                dark ? "#11181F" : "#EAF4F4",
                dark ? "#E8ECF0" : "#12443B"));
        QTextCharFormat todayFmt;
        todayFmt.setBackground(QColor(dark ? "#21484B" : "#A3CAD3"));
        todayFmt.setForeground(QColor(dark ? "#E8FFFB" : "#12443B"));
        todayFmt.setFontWeight(QFont::Black);
        cw->setDateTextFormat(QDate::currentDate(), todayFmt);
    };

    QFrame* outP2 = new QFrame;
    outP2->setObjectName("projFormPanel");
    outP2->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius:14px; }").arg(C_PANEL_BG, C_PANEL_BR));
    applyGlassShadow(outP2, QColor(0, 0, 0, 48), 32.0, QPointF(0.0, 10.0));
    QHBoxLayout* outP2L = new QHBoxLayout(outP2);
    outP2L->setContentsMargins(12,12,12,12);
    outP2L->setSpacing(12);

    // ══════════════════════════════════════════════════════════
    // LEFT PANEL — Identité du projet
    // ══════════════════════════════════════════════════════════
    QFrame* p2LeftPanel = softBox();
    p2LeftPanel->setObjectName("projFormSection");
    p2LeftPanel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    QVBoxLayout* p2LeftPanelL = new QVBoxLayout(p2LeftPanel);
    p2LeftPanelL->setContentsMargins(6,6,6,6);
    p2LeftPanelL->setSpacing(0);

    QScrollArea* p2LeftScroll = new QScrollArea;
    p2LeftScroll->setWidgetResizable(true);
    p2LeftScroll->setFrameShape(QFrame::NoFrame);
    p2LeftScroll->setStyleSheet("QScrollArea{background:transparent; border:none;}");
    p2LeftScroll->setFixedWidth(400);

    QWidget* p2LeftContent = new QWidget;
    QVBoxLayout* p2LeftL = new QVBoxLayout(p2LeftContent);
    p2LeftL->setContentsMargins(4,4,4,4);
    p2LeftL->setSpacing(14);

    // Nom du projet
    p2LeftL->addWidget(projTitle("Informations générales"));
    QLineEdit* projName = new QLineEdit;
    projName->setPlaceholderText("Nom du projet (3–150 caractères)");
    projName->setStyleSheet(projFieldStyle());
    projName->setMaxLength(150);
    QLabel* errProjName = mkProjErr();
    p2LeftL->addWidget(projRow(QStyle::SP_DirIcon, "Nom *", projName));
    p2LeftL->addWidget(errProjName);

    // Domaine de recherche
    QComboBox* projDomainEdit = new QComboBox;
    projDomainEdit->addItems({"— Sélectionner un domaine —",
                              "Domaine", "Génomique", "Protéomique", "Pharmacologie", "Immunologie", "Biologie végétale", "Microbiologie", "Neurosciences", "Biotechnologies", "Génétique", "Bioinformatique"});
    projDomainEdit->setFixedWidth(240);
    projDomainEdit->setMinimumHeight(46);
    projDomainEdit->setStyleSheet(projComboStyle());
    QLabel* errProjDomain = mkProjErr();
    p2LeftL->addWidget(projRow(QStyle::SP_ComputerIcon, "Domaine *", projDomainEdit));
    p2LeftL->addWidget(errProjDomain);

    // Statut — Combo box selection
    QComboBox* projStatus = new QComboBox;
    projStatus->addItems({"Planifié", "En cours", "En retard", "Critique", "Suspendu", "Terminé", "Annulé"});
    projStatus->setCurrentIndex(0);
    projStatus->setFixedWidth(240);
    projStatus->setMinimumHeight(46);
    projStatus->setStyleSheet(projComboStyle());
    p2LeftL->addWidget(projRow(QStyle::SP_MessageBoxInformation, "Statut *", projStatus));

    // Numéro d'approbation éthique
    QLineEdit* projEthique = new QLineEdit;
    projEthique->setPlaceholderText("ex: CPP-2024/017  (requis si En cours)");
    projEthique->setStyleSheet(projFieldStyle());
    projEthique->setMaxLength(100);
    QLabel* errProjEthique = mkProjErr();
    p2LeftL->addWidget(projRow(QStyle::SP_FileDialogInfoView, "Éthique", projEthique));
    p2LeftL->addWidget(errProjEthique);

    // ── Instant validation for status/ethics cross-rule ──────
    auto checkEthiqueRule = [=](){
        const bool enCours = (projStatus->currentText() == "En cours");
        if (enCours && projEthique->text().trimmed().isEmpty()) {
            showProjErr(errProjEthique,
                        "Obligatoire pour un projet « En cours ».",
                        projEthique, projFieldStyle(true));
        } else {
            clearProjErr(errProjEthique, projEthique, projFieldStyle());
        }
    };
    QObject::connect(projStatus, &QComboBox::currentTextChanged, this, [=](const QString&){ checkEthiqueRule(); });
    QObject::connect(projEthique, &QLineEdit::textChanged, this, [=](const QString&){ checkEthiqueRule(); });

    // ── Nom instant validation ────────────────────────────────
    QObject::connect(projName, &QLineEdit::textChanged, this, [=](const QString& v){
        const QString t = v.trimmed();
        if (t.isEmpty()) {
            showProjErr(errProjName, "Le nom est obligatoire.", projName, projFieldStyle(true));
        } else if (t.length() < 3) {
            showProjErr(errProjName, "Minimum 3 caractères.", projName, projFieldStyle(true));
        } else if (t.length() > 150) {
            showProjErr(errProjName, "Maximum 150 caractères.", projName, projFieldStyle(true));
        } else {
            static const QRegularExpression allowed(R"(^[A-Za-zÀ-ÖØ-öø-ÿ0-9 \-()/]+$)");
            if (!allowed.match(t).hasMatch())
                showProjErr(errProjName, "Caractères non autorisés (lettres, chiffres, espaces, - ( ) /).", projName, projFieldStyle(true));
            else
                clearProjErr(errProjName, projName, projFieldStyle());
        }
    });

    // ── Domaine instant validation ────────────────────────────
    QObject::connect(projDomainEdit, &QComboBox::currentTextChanged, this, [=](const QString& v){
        if (v.startsWith("—"))
            showProjErr(errProjDomain, "Sélectionnez un domaine.", projDomainEdit, projComboStyle(true));
        else
            clearProjErr(errProjDomain, projDomainEdit, projComboStyle());
    });

    p2LeftL->addStretch(1);
    p2LeftScroll->setWidget(p2LeftContent);
    p2LeftPanelL->addWidget(p2LeftScroll);

    // ══════════════════════════════════════════════════════════
    // RIGHT PANEL — Planification + Financement multi-sources
    // ══════════════════════════════════════════════════════════
    QFrame* p2RightPanel = softBox();
    p2RightPanel->setObjectName("projFormSection");
    QVBoxLayout* p2RightPanelL = new QVBoxLayout(p2RightPanel);
    p2RightPanelL->setContentsMargins(6,6,6,6);
    p2RightPanelL->setSpacing(0);

    QScrollArea* p2RightScroll = new QScrollArea;
    p2RightScroll->setWidgetResizable(true);
    p2RightScroll->setFrameShape(QFrame::NoFrame);
    p2RightScroll->setStyleSheet("QScrollArea{background:transparent; border:none;}");

    QWidget* p2RightContent = new QWidget;
    QVBoxLayout* p2RightL = new QVBoxLayout(p2RightContent);
    p2RightL->setContentsMargins(4,4,4,4);
    p2RightL->setSpacing(14);

    // Dates
    p2RightL->addWidget(projTitle("Planification"));

    QDateEdit* projStart = new QDateEdit(QDate::currentDate());
    projStart->setCalendarPopup(true);
    projStart->setDisplayFormat("dd/MM/yyyy");
    projStart->setMinimumWidth(200);
    projStart->setMinimumHeight(46);
    projStart->setStyleSheet(projDateStyle());
    applyProjCalendar(projStart);
    QLabel* errProjStart = mkProjErr();
    p2RightL->addWidget(projRow(QStyle::SP_FileDialogDetailedView, "Date début *", projStart));
    p2RightL->addWidget(errProjStart);

    QDateEdit* projEnd = new QDateEdit(QDate::currentDate().addMonths(3));
    projEnd->setCalendarPopup(true);
    projEnd->setDisplayFormat("dd/MM/yyyy");
    projEnd->setMinimumWidth(200);
    projEnd->setMinimumHeight(46);
    projEnd->setStyleSheet(projDateStyle());
    applyProjCalendar(projEnd);
    QLabel* errProjEnd = mkProjErr();
    p2RightL->addWidget(projRow(QStyle::SP_FileDialogDetailedView, "Date fin", projEnd));
    p2RightL->addWidget(errProjEnd);

    // Date instant cross-validation
    auto checkDates = [=](){
        const QDate d = projStart->date();
        const QDate f = projEnd->date();
        if (d.year() < 2000) {
            showProjErr(errProjStart, "Antérieure à 2000.", projStart, projDateStyle(true));
        } else {
            clearProjErr(errProjStart, projStart, projDateStyle());
        }
        if (f.isValid() && f <= d) {
            showProjErr(errProjEnd, "Doit être après la date de début.", projEnd, projDateStyle(true));
        } else if (f.isValid() && f < d.addMonths(1)) {
            showProjErr(errProjEnd, "Durée minimale : 1 mois.", projEnd, projDateStyle(true));
        } else if (f.isValid() && f > d.addYears(20)) {
            showProjErr(errProjEnd, "Durée > 20 ans — vérifiez les dates.", projEnd, projDateStyle(true));
        } else {
            clearProjErr(errProjEnd, projEnd, projDateStyle());
        }
    };
    QObject::connect(projStart, &QDateEdit::dateChanged, this, [=](const QDate&){ checkDates(); });
    QObject::connect(projEnd,   &QDateEdit::dateChanged, this, [=](const QDate&){ checkDates(); });

    // Publications (read-only computed)
    QSpinBox* projPubsEdit = new QSpinBox;
    projPubsEdit->setRange(0, 9999);
    projPubsEdit->setValue(0);
    projPubsEdit->setPrefix("Pub: ");
    projPubsEdit->setMinimumWidth(280);
    projPubsEdit->setFixedHeight(60);
    projPubsEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    projPubsEdit->setReadOnly(true);
    projPubsEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);
    projPubsEdit->setStyleSheet(projPubsStyle());
    projPubsEdit->setToolTip("Nombre de publications liées à ce projet.");
    p2RightL->addWidget(projRow(QStyle::SP_FileIcon, "Publications", projPubsEdit));

    // ── Multi-source financement ──────────────────────────────
    p2RightL->addSpacing(4);
    p2RightL->addWidget(projTitle("Sources de financement  (max 10)"));

    // Single funding source + budget fields
    QLabel* finLabel = new QLabel("Source de financement");
    finLabel->setObjectName("projFormSubLabel");
    finLabel->setStyleSheet("color:rgba(0,0,0,0.60); font-size:11px; font-weight:700; background:transparent;");
    p2RightL->addWidget(finLabel);

    QLineEdit* projFinancement = new QLineEdit;
    projFinancement->setPlaceholderText("Ex. : ANR, Horizon Europe, Budget interne…");
    projFinancement->setMaxLength(150);
    projFinancement->setStyleSheet(projFieldStyle());
    p2RightL->addWidget(projFinancement);

    QLabel* budgetLabel = new QLabel("Budget (TND)");
    budgetLabel->setObjectName("projFormSubLabel");
    budgetLabel->setStyleSheet("color:rgba(0,0,0,0.60); font-size:11px; font-weight:700; background:transparent;");
    p2RightL->addWidget(budgetLabel);

    // Budget ranges: label → representative double value stored in DB
    struct BudgetRange { QString label; double value; };
    QDoubleSpinBox* projBudgetSpin = new QDoubleSpinBox;
    projBudgetSpin->setRange(0.0, 99999999999.99);
    projBudgetSpin->setDecimals(2);
    projBudgetSpin->setSuffix(" TND");
    projBudgetSpin->setValue(0.0);
    projBudgetSpin->setSingleStep(1000.0);
    projBudgetSpin->setGroupSeparatorShown(true);
    projBudgetSpin->setMinimumHeight(72);
    projBudgetSpin->setMinimumWidth(160);
    projBudgetSpin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    projBudgetSpin->setStyleSheet(projBudgetStyle());
    p2RightL->addWidget(projBudgetSpin);

    p2RightL->addStretch(1);
    p2RightScroll->setWidget(p2RightContent);
    p2RightPanelL->addWidget(p2RightScroll);

    outP2L->addWidget(p2LeftPanel);
    outP2L->addWidget(p2RightPanel, 1);
    gp2->addWidget(outP2, 1);

    QFrame* p2Bottom = new QFrame;
    p2Bottom->setFixedHeight(64);
    p2Bottom->setStyleSheet("background:rgba(255,255,255,0.20); border:1px solid rgba(0,0,0,0.08); border-radius:14px;");
    QHBoxLayout* p2BottomL = new QHBoxLayout(p2Bottom);
    p2BottomL->setContentsMargins(14,10,14,10);
    p2BottomL->setSpacing(12);

    QPushButton* projSave = actionBtn("Enregistrer", "rgba(10,95,88,0.45)", "rgba(255,255,255,0.90)", st->standardIcon(QStyle::SP_DialogSaveButton), true);
    QPushButton* projCancel = actionBtn("Annuler", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_DialogCancelButton), true);

    p2BottomL->addWidget(projSave);
    p2BottomL->addWidget(projCancel);
    p2BottomL->addStretch(1);

    gp2->addWidget(p2Bottom);
    addStackPage(proj2);

    // projFinancement and projBudgetSpin are read directly at save time

    // ==========================================================
    // PAGE 7 : Gestion Projet - Widget 3 (STATISTIQUES - MENU)
    // ==========================================================
    QWidget* proj3 = new QWidget;
    QVBoxLayout* gp3 = new QVBoxLayout(proj3);
    gp3->setContentsMargins(22, 18, 22, 18);
    gp3->setSpacing(14);

    ModulesBar barProjStats;
    gp3->addWidget(makeHeaderBlock(st, "Statistiques Projet", ModuleTab::GestionProjet, &barProjStats));
    connectModulesSwitch(this, stack, barProjStats);

    // Keep pd alive so the navigation code at projBtnStats can call pd->setData()
    DonutChart* pd = new DonutChart;
    pd->hide();
    pd->setData({
        {3, W_GREEN,  "En cours"},
        {2, W_ORANGE, "Planifié"},
        {1, W_RED,    "En retard"},
        {2, QColor("#9FBEB9"), "Terminé"}
    });

    // Keep exportP3 alive so the existing connect block (PDF export) still compiles
    QPushButton* exportP3 = new QPushButton;
    exportP3->hide();

    QFrame* outP3 = new QFrame;
    outP3->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }").arg(C_PANEL_BG, C_PANEL_BR));
    QVBoxLayout* outP3L = new QVBoxLayout(outP3);
    outP3L->setContentsMargins(20, 20, 20, 20);
    outP3L->setSpacing(18);

    // ── Page title ──────────────────────────────────────────────
    QLabel* statsPageTitle = new QLabel("Choisissez une statistique");
    statsPageTitle->setAlignment(Qt::AlignCenter);
    statsPageTitle->setStyleSheet(
        "color: rgba(0,0,0,0.65); font-size: 18px; font-weight: 900;"
        "padding-bottom: 4px;"
    );
    outP3L->addWidget(statsPageTitle);

    QLabel* statsPageSub = new QLabel("Sélectionnez le type d'analyse à afficher");
    statsPageSub->setAlignment(Qt::AlignCenter);
    statsPageSub->setStyleSheet("color: rgba(0,0,0,0.40); font-size: 12px; font-weight: 600;");
    outP3L->addWidget(statsPageSub);

    // ── Section : Statistiques graphiques ───────────────────────
    QLabel* secGraph = new QLabel("  Statistiques graphiques");
    secGraph->setStyleSheet(
        "color: rgba(10,95,88,0.85); font-size: 13px; font-weight: 900;"
        "background: rgba(10,95,88,0.08); border-radius: 8px; padding: 6px 12px;"
    );
    outP3L->addWidget(secGraph);

    // Row 1 — 3 graphic stat buttons
    QFrame* row1 = new QFrame;
    row1->setStyleSheet("QFrame{ background: transparent; border: none; }");
    QHBoxLayout* row1L = new QHBoxLayout(row1);
    row1L->setContentsMargins(0,0,0,0);
    row1L->setSpacing(14);

    struct StatBtn { QString icon; QString label; QString color; };
    const QString statsBtnAccent = "rgba(81,129,149,0.55)";
    auto makeStatCard = [&](const StatBtn& s) -> QPushButton* {
        QPushButton* b = new QPushButton(s.icon + "  " + s.label);
        b->setEnabled(false);
        b->setCursor(Qt::ForbiddenCursor);
        b->setFixedHeight(80);
        b->setStyleSheet(QString(R"(
            QPushButton {
                background: %1;
                color: rgba(255,255,255,0.92);
                border: 1px solid rgba(0,0,0,0.10);
                border-radius: 14px;
                font-size: 12px;
                font-weight: 800;
                padding: 10px 14px;
                text-align: left;
            }
            QPushButton:disabled {
                background: %1;
                color: rgba(255,255,255,0.70);
            }
        )").arg(s.color));
        return b;
    };

    // ── "Répartition des projets par domaine" — ENABLED ──────
    QPushButton* btnDomaineProj = makeStatCard({"📊", "Répartition des projets\npar domaine", statsBtnAccent});
    btnDomaineProj->setEnabled(true);
    btnDomaineProj->setCursor(Qt::PointingHandCursor);
    btnDomaineProj->setStyleSheet(btnDomaineProj->styleSheet().replace("QPushButton:disabled","QPushButton:disabled_UNUSED"));
    row1L->addWidget(btnDomaineProj, 1);

    QPushButton* btnAnalyseBudget = makeStatCard({"💰", "Analyse budgétaire\ncomplète", statsBtnAccent});
    btnAnalyseBudget->setEnabled(true);
    btnAnalyseBudget->setCursor(Qt::PointingHandCursor);
    btnAnalyseBudget->setStyleSheet(btnAnalyseBudget->styleSheet().replace("QPushButton:disabled","QPushButton:disabled_UNUSED"));
    row1L->addWidget(btnAnalyseBudget, 1);

    QPushButton* btnEvolutionProj = makeStatCard({"📈", "Évolution de projet\ndans le temps", statsBtnAccent});
    btnEvolutionProj->setEnabled(true);
    btnEvolutionProj->setCursor(Qt::PointingHandCursor);
    btnEvolutionProj->setStyleSheet(btnEvolutionProj->styleSheet().replace("QPushButton:disabled","QPushButton:disabled_UNUSED"));
    row1L->addWidget(btnEvolutionProj, 1);
    outP3L->addWidget(row1);

    // ── Section : Rapports & analyses ───────────────────────────
    QLabel* secRapports = new QLabel("  Rapports & analyses détaillées");
    secRapports->setStyleSheet(
        "color: rgba(139,47,60,0.85); font-size: 13px; font-weight: 900;"
        "background: rgba(139,47,60,0.07); border-radius: 8px; padding: 6px 12px;"
    );
    outP3L->addWidget(secRapports);

    // Row 2 — 2 analysis buttons
    QFrame* row2 = new QFrame;
    row2->setStyleSheet("QFrame{ background: transparent; border: none; }");
    QHBoxLayout* row2L = new QHBoxLayout(row2);
    row2L->setContentsMargins(0,0,0,0);
    row2L->setSpacing(14);

    QPushButton* btnStatutProj = makeStatCard({"🥧", "Répartition des projets\npar statut", statsBtnAccent});
    btnStatutProj->setEnabled(true);
    btnStatutProj->setCursor(Qt::PointingHandCursor);
    btnStatutProj->setStyleSheet(btnStatutProj->styleSheet().replace("QPushButton:disabled","QPushButton:disabled_UNUSED"));
    row2L->addWidget(btnStatutProj, 1);

    QPushButton* btnTimelineProj = makeStatCard({"🗓️", "Timeline\ndes projets", statsBtnAccent});
    btnTimelineProj->setEnabled(true);
    btnTimelineProj->setCursor(Qt::PointingHandCursor);
    btnTimelineProj->setStyleSheet(btnTimelineProj->styleSheet().replace("QPushButton:disabled","QPushButton:disabled_UNUSED"));
    row2L->addWidget(btnTimelineProj, 1);

    row2L->addStretch(1);
    outP3L->addWidget(row2);

    // ── Section : Export ─────────────────────────────────────────
    QLabel* secExport = new QLabel("  Export automatique");
    secExport->setStyleSheet(
        "color: rgba(0,120,60,0.85); font-size: 13px; font-weight: 900;"
        "background: rgba(0,120,60,0.07); border-radius: 8px; padding: 6px 12px;"
    );
    outP3L->addWidget(secExport);

    QFrame* row4 = new QFrame;
    row4->setStyleSheet("QFrame{ background: transparent; border: none; }");
    QHBoxLayout* row4L = new QHBoxLayout(row4);
    row4L->setContentsMargins(0,0,0,0);
    row4L->setSpacing(14);

    QPushButton* btnRapportFinancier = makeStatCard({"📋", "Rapport financier trimestriel\n(Excel)", "rgba(0,120,60,0.55)"});
    btnRapportFinancier->setEnabled(true);
    btnRapportFinancier->setCursor(Qt::PointingHandCursor);
    btnRapportFinancier->setStyleSheet(btnRapportFinancier->styleSheet().replace("QPushButton:disabled","QPushButton:disabled_UNUSED"));
    row4L->addWidget(btnRapportFinancier, 1);
    row4L->addStretch(2);
    outP3L->addWidget(row4);

    outP3L->addStretch(1);

    // ── Bottom bar ───────────────────────────────────────────────
    QFrame* p3Bottom = new QFrame;
    p3Bottom->setFixedHeight(64);
    p3Bottom->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* p3BottomL = new QHBoxLayout(p3Bottom);
    p3BottomL->setContentsMargins(14,10,14,10);

    QPushButton* p3Back = actionBtn("Retour", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_ArrowBack), true);
    p3BottomL->addWidget(p3Back);
    p3BottomL->addStretch(1);

    outP3L->addWidget(p3Bottom);

    QScrollArea* proj3Scroll = new QScrollArea;
    proj3Scroll->setObjectName("proj3Scroll");
    proj3Scroll->setWidgetResizable(true);
    proj3Scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    proj3Scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    proj3Scroll->setFrameShape(QFrame::NoFrame);
    proj3Scroll->setStyleSheet(R"(
        QScrollArea#proj3Scroll {
            background: transparent;
            border: none;
        }
        QScrollArea#proj3Scroll > QWidget > QWidget {
            background: transparent;
        }
        QScrollBar:vertical {
            background: rgba(0,0,0,0.06);
            width: 6px;
            border-radius: 3px;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background: rgba(45,212,191,0.50);
            border-radius: 3px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover {
            background: rgba(45,212,191,0.80);
        }
        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical { height: 0px; }
        QScrollBar::add-page:vertical,
        QScrollBar::sub-page:vertical { background: none; }
    )");
    proj3Scroll->setWidget(outP3);
    gp3->addWidget(proj3Scroll, 1);

    addStackPage(proj3);
    // ==========================================================
    // =================  EXPERIENCES / PROTOCOLES  =============
    // ==========================================================

    // ==========================================================
    // PAGE 8 : Expériences & Protocoles - Widget 1 (LISTE)
    // ==========================================================
    QWidget* exp1 = new QWidget;
    QVBoxLayout* ep1 = new QVBoxLayout(exp1);
    ep1->setContentsMargins(22, 18, 22, 18);
    ep1->setSpacing(14);

    ModulesBar barExpList;
    ep1->addWidget(makeHeaderBlock(st, "Expériences", ModuleTab::ExperiencesProtocoles, &barExpList));
    connectModulesSwitch(this, stack, barExpList);

    QFrame* eBar = new QFrame;
    eBar->setFixedHeight(54);
    eBar->setStyleSheet("background: rgba(255,255,255,0.22); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* eBarL = new QHBoxLayout(eBar);
    eBarL->setContentsMargins(14, 8, 14, 8);
    eBarL->setSpacing(10);

    QLineEdit* eSearch = new QLineEdit;
    eSearch->setPlaceholderText("Rechercher par statut, type, disponibilité, résultat");
    eSearch->addAction(st->standardIcon(QStyle::SP_FileDialogContentsView), QLineEdit::LeadingPosition);

    QComboBox* eTrierCb = new QComboBox;
    eTrierCb->addItems({"Aucun tri", "Trier par statut", "Trier par date", "Trier par disponibilité"});

    QPushButton* eExportBtn = new QPushButton(st->standardIcon(QStyle::SP_DialogSaveButton), "  Exporter (PDF / CSV)");
    eExportBtn->setCursor(Qt::PointingHandCursor);
    eExportBtn->setStyleSheet(QString(R"(
        QPushButton{
            background:%1; color: rgba(255,255,255,0.92);
            border:1px solid rgba(0,0,0,0.18);
            border-radius: 12px; padding: 10px 16px; font-weight: 800;
        }
        QPushButton:hover{ background: %2; }
    )").arg(C_PRIMARY, C_TOPBAR));

    eBarL->addWidget(eSearch, 1);
    eBarL->addWidget(eTrierCb);
    ep1->addWidget(eBar);

    QFrame* expCard = makeCard();
    QVBoxLayout* expCardL = new QVBoxLayout(expCard);
    expCardL->setContentsMargins(10,10,10,10);

    ExperienceCrud* expCrud     = new ExperienceCrud;
    EquipementCrud* expEquipCrud = new EquipementCrud;
    bool*           expEditMode = new bool(false);
    int*            expEditId   = new int(0);

    auto syncFinishedExperienceEquipments = [=]() {
        QSqlQuery q;
        q.prepare(
            "UPDATE \"Équipement\" e "
            "SET e.\"statut\" = 'Actif' "
            "WHERE (LOWER(NVL(e.\"statut\",'')) LIKE '%hors%' OR LOWER(NVL(e.\"statut\",'')) LIKE '%service%') "
            "  AND EXISTS ("
            "      SELECT 1 FROM \"Expérience\" x "
            "      WHERE x.\"Id_exp\" = e.\"Id_exp\" "
            "        AND x.\"Date_fin\" < TRUNC(SYSDATE)"
            "  )");
        q.exec();
    };

    QTableWidget* expTable = new QTableWidget(0, 8);
    expTable->setHorizontalHeaderLabels({"Nom", "Hypothèse", "Date début", "Date fin", "Statut", "Type expérience", "Disponibilité", "Résultat"});
    expTable->verticalHeader()->setVisible(false);
    expTable->setShowGrid(true);
    expTable->setAlternatingRowColors(true);
    expTable->horizontalHeader()->setStretchLastSection(true);
    expTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    expTable->setSelectionMode(QAbstractItemView::SingleSelection);
    expTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    expTable->setItemDelegateForColumn(4, new ExpBadgeDelegate(expTable));

    expTable->setColumnWidth(0, 220);
    expTable->setColumnWidth(1, 180);
    expTable->setColumnWidth(2, 120);
    expTable->setColumnWidth(3, 120);
    expTable->setColumnWidth(4, 140);
    expTable->setColumnWidth(5, 170);
    expTable->setColumnWidth(6, 140);
    expTable->setColumnWidth(7, 180);

    auto statusFromString = [](const QString& s) -> ExpStatus {
        if (s == "En cours")  return ExpStatus::EnCours;
        if (s == "Concluante" || s == "Réussie") return ExpStatus::Termine;
        if (s == "Échouée") return ExpStatus::Suspendue;
        return ExpStatus::EnAttente;
    };

    auto currentExpSort = [=]() -> ExpSortKey {
        const int idx = eTrierCb->currentIndex();
        if (idx == 1) return ExpSortKey::Status;
        if (idx == 2) return ExpSortKey::DateFin;
        if (idx == 3) return ExpSortKey::Disponibilite;
        return ExpSortKey::None;
    };

    auto applyExpFilter = [=](){
        const QString filter = eSearch->text().trimmed().toLower();
        for (int r = 0; r < expTable->rowCount(); ++r) {
            bool match = false;
            if (filter.isEmpty()) {
                match = true;
            } else {
                for (int c = 0; c < expTable->columnCount(); ++c) {
                    QTableWidgetItem* it = expTable->item(r, c);
                    if (!it) continue;
                    const QString text = it->text();
                    const QString alt = it->data(Qt::UserRole + 1).toString();
                    if (text.toLower().contains(filter) || alt.toLower().contains(filter)) {
                        match = true;
                        break;
                    }
                }
            }
            expTable->setRowHidden(r, !match);
        }
    };

    auto loadExpTable = [=](){
        syncFinishedExperienceEquipments();
        expTable->setRowCount(0);
        QList<ExperienceRecord> recs;
        if (!expCrud->loadExperiences(recs)) return;

        QMap<int, QString> equipNameByExpId;
        {
            QSqlQuery qEq;
            if (qEq.exec("SELECT \"Id_exp\", \"nom_equipement\" FROM \"Équipement\" WHERE \"Id_exp\" IS NOT NULL ORDER BY \"equipement_id\"")) {
                while (qEq.next()) {
                    const int expId = qEq.value(0).toInt();
                    if (!equipNameByExpId.contains(expId)) {
                        equipNameByExpId.insert(expId, qEq.value(1).toString());
                    }
                }
            }
        }

        recs = ExperienceSorter::sort(recs, currentExpSort());
        auto mk = [](const QString& v){
            QTableWidgetItem* it = new QTableWidgetItem(v);
            it->setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);
            return it;
        };
        for (const ExperienceRecord& rec : recs) {
            int row = expTable->rowCount();
            expTable->insertRow(row);
            QTableWidgetItem* nameItem = mk(rec.titre);
            nameItem->setData(Qt::UserRole, rec.id);
            expTable->setItem(row, 0, nameItem);
            expTable->setItem(row, 1, mk(rec.hypothese));
            expTable->setItem(row, 2, mk(rec.dateDebut.toString("dd/MM/yyyy")));
            expTable->setItem(row, 3, mk(rec.dateFin.toString("dd/MM/yyyy")));
            QTableWidgetItem* badge = new QTableWidgetItem;
            const ExpStatus st = statusFromString(rec.status);
            badge->setData(Qt::UserRole, (int)st);
            badge->setData(Qt::UserRole + 1, expStatusText(st));
            expTable->setItem(row, 4, badge);
            expTable->setItem(row, 5, mk(rec.typeExperience));
            const QString eqName = equipNameByExpId.value(rec.id).trimmed();
            expTable->setItem(row, 6, mk(eqName.isEmpty() ? QString("-") : eqName));
            expTable->setItem(row, 7, mk(rec.resultat));
            expTable->setRowHeight(row, 46);
        }
        applyExpFilter();
    };
    loadExpTable();

    QObject::connect(stack, &QStackedWidget::currentChanged, expTable, [=](int idx){
        if (idx == EXP_LIST) loadExpTable();
    });

    // Search by titre or protocole
    QObject::connect(eSearch, &QLineEdit::textChanged, this, [=](const QString&){
        applyExpFilter();
    });

    QObject::connect(eTrierCb, &QComboBox::currentIndexChanged, this, [=](int){
        loadExpTable();
    });

    expCardL->addWidget(expTable);
    ep1->addWidget(expCard, 1);

    QFrame* expBottom = new QFrame;
    expBottom->setFixedHeight(64);
    expBottom->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* expBottomL = new QHBoxLayout(expBottom);
    expBottomL->setContentsMargins(14,10,14,10);
    expBottomL->setSpacing(12);

    QPushButton* expAdd     = actionBtn("Ajouter",      "rgba(10,95,88,0.45)", "rgba(255,255,255,0.90)", st->standardIcon(QStyle::SP_DialogYesButton), true);
    QPushButton* expEdit    = actionBtn("Modifier",     "rgba(198,178,154,0.55)", "rgba(255,255,255,0.85)", st->standardIcon(QStyle::SP_FileDialogContentsView), true);
    QPushButton* expDetails = actionBtn("Détails",      "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_DesktopIcon), true);
    QPushButton* expDel     = actionBtn("Supprimer",    "rgba(255,255,255,0.55)", "#B14A4A", st->standardIcon(QStyle::SP_TrashIcon), true);
    QPushButton* expStats   = actionBtn("Statistiques", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_MessageBoxInformation), true);

    QObject::connect(expDel, &QPushButton::clicked, this, [=](){
        if (*currentRole != "Responsable") { showToast(this, "Accès refusé : seul le Responsable peut supprimer.", false); return; }
        int r = expTable->currentRow();
        if (r < 0) { showToast(this, "Sélectionnez une expérience.", false); return; }
        int id = expTable->item(r, 0)->data(Qt::UserRole).toInt();
        QString resume = QString("Expérience : %1 | Hypothèse : %2")
                         .arg(expTable->item(r,0)->text(), expTable->item(r,1)->text());
        ConfirmDeleteDialog confirm(style(), resume, this);
        if (confirm.exec() != QDialog::Accepted) return;
        QString err;
        if (!expCrud->deleteExperience(id, &err)) { showToast(this, "Erreur : " + err, false); return; }
        showToast(this, "Expérience supprimée.", true);
        loadExpTable();
    });

    expBottomL->addWidget(expAdd);
    expBottomL->addWidget(expEdit);
    expBottomL->addWidget(expDetails);
    expBottomL->addWidget(expDel);
    expBottomL->addStretch(1);
    expBottomL->addWidget(expStats);
    expBottomL->addWidget(eExportBtn);

    ep1->addWidget(expBottom);
    addStackPage(exp1);
    // ==========================================================
    // PAGE 9 : Expériences & Protocoles - Widget 2 (AJOUT/MODIF)
    // ==========================================================
    QWidget* exp2 = new QWidget;
    exp2->setObjectName("expFormPage");
    exp2->setStyleSheet(
        "QFrame#expFormPanel{"
        " background: rgba(246,248,247,0.86);"
        " border: 1px solid rgba(0,0,0,0.10);"
        " border-radius: 14px;"
        "}"
        "QFrame#expFormSection{"
        " background: rgba(244,246,245,0.78);"
        " border: 1px solid rgba(0,0,0,0.10);"
        " border-radius: 12px;"
        "}"
        "QLabel#expFormTitle{"
        " color: rgba(0,0,0,0.55);"
        " font-weight: 900;"
        " font-size: 14px;"
        "}"
        "QFrame#expFormRow{"
        " background: rgba(244,246,245,0.78);"
        " border: 1px solid rgba(0,0,0,0.10);"
        " border-radius: 12px;"
        "}"
        "QLabel#expFormRowLabel{"
        " color: rgba(0,0,0,0.55);"
        " font-weight: 900;"
        " font-size: 13px;"
        "}"
        "QLineEdit#expFormInput, QComboBox#expFormInput, QDateEdit#expFormInput{"
        " background: rgba(255,255,255,0.92);"
        " color: rgba(0,0,0,0.82);"
        " border: 1px solid rgba(0,0,0,0.12);"
        " border-radius: 12px;"
        " padding: 7px 12px;"
        " font-weight: 800;"
        " font-size: 13px;"
        "}"
        "QPushButton#expEquipPickBtn{"
        " background: rgba(10,95,88,0.16);"
        " color:#0A5F58;"
        " border:1px solid rgba(10,95,88,0.35);"
        " border-radius:10px;"
        " padding:8px 12px;"
        " font-weight:900;"
        "}"
        "QPushButton#expEquipPickBtn:hover{ background: rgba(10,95,88,0.22); }"
        "QPushButton#expEquipPickBtn:pressed{ background: rgba(10,95,88,0.28); }"
        "QLabel#expEquipSelectedLbl{"
        " color: rgba(0,0,0,0.70);"
        " background: rgba(248,252,251,0.97);"
        " border: 1px solid rgba(12,75,68,0.15);"
        " border-radius: 12px;"
        " padding: 6px 12px;"
        " font-weight: 800;"
        " font-size: 13px;"
        "}"
    );
    QVBoxLayout* ep2 = new QVBoxLayout(exp2);
    ep2->setContentsMargins(22, 18, 22, 18);
    ep2->setSpacing(14);

    ModulesBar barExpForm;
    ep2->addWidget(makeHeaderBlock(st, "Ajouter / Modifier une expérience", ModuleTab::ExperiencesProtocoles, &barExpForm));
    connectModulesSwitch(this, stack, barExpForm);

    QFrame* outE2 = new QFrame;
    outE2->setObjectName("expFormPanel");
    applyGlassShadow(outE2, QColor(0, 0, 0, 48), 32.0, QPointF(0.0, 10.0));
    QHBoxLayout* outE2L = new QHBoxLayout(outE2);
    outE2L->setContentsMargins(12,12,12,12);
    outE2L->setSpacing(12);

    auto expTitle = [&](const QString& t){
        QLabel* lab = new QLabel(t);
        lab->setObjectName("expFormTitle");
        return lab;
    };
    auto expRow = [&](QStyle::StandardPixmap sp, const QString& label, QWidget* input){
        QFrame* r = softBox();
        r->setObjectName("expFormRow");
        QHBoxLayout* l = new QHBoxLayout(r);
        l->setContentsMargins(10,8,10,8);
        l->setSpacing(10);

        QToolButton* ic = new QToolButton;
        ic->setAutoRaise(true);
        ic->setIcon(st->standardIcon(sp));

        QLabel* lab = new QLabel(label);
        lab->setObjectName("expFormRowLabel");

        l->addWidget(ic);
        l->addWidget(lab);
        l->addStretch(1);
        l->addWidget(input);
        return r;
    };

    // ── Left panel: Informations ──────────────────────────────
    QFrame* e2Left = softBox();
    e2Left->setObjectName("expFormSection");
    e2Left->setFixedWidth(420);
    QVBoxLayout* e2LeftL = new QVBoxLayout(e2Left);
    e2LeftL->setContentsMargins(12,12,12,12);
    e2LeftL->setSpacing(10);

    QLineEdit* eName = new QLineEdit;
    eName->setObjectName("expFormInput");
    eName->setPlaceholderText("Nom de l’expérience");

    QRegularExpression noDigits("^[^0-9]*$");
    eName->setValidator(new QRegularExpressionValidator(noDigits, eName));

    QLineEdit* eHypo = new QLineEdit;
    eHypo->setObjectName("expFormInput");
    eHypo->setPlaceholderText("Hypothese");
    eHypo->setValidator(new QRegularExpressionValidator(noDigits, eHypo));

    QLineEdit* eTypeExp = new QLineEdit;
    eTypeExp->setObjectName("expFormInput");
    eTypeExp->setPlaceholderText("Type_Experience");
    eTypeExp->setValidator(new QRegularExpressionValidator(noDigits, eTypeExp));

    QLineEdit* eResultat = new QLineEdit;
    eResultat->setObjectName("expFormInput");
    eResultat->setPlaceholderText("Resultat");
    eResultat->setValidator(new QRegularExpressionValidator(noDigits, eResultat));

    QPushButton* ePickEquipBtn = new QPushButton("Choisir équipement disponible");
    ePickEquipBtn->setObjectName("expEquipPickBtn");
    ePickEquipBtn->setCursor(Qt::PointingHandCursor);
    ePickEquipBtn->setMinimumWidth(240);

    QLabel* eSelectedEquipLbl = new QLabel("Aucun équipement sélectionné");
    eSelectedEquipLbl->setObjectName("expEquipSelectedLbl");

    QWidget* eEquipSelectBox = new QWidget;
    eEquipSelectBox->setObjectName("expEquipSelectBox");
    QHBoxLayout* eEquipSelectL = new QHBoxLayout(eEquipSelectBox);
    eEquipSelectL->setContentsMargins(0,0,0,0);
    eEquipSelectL->setSpacing(10);
    eEquipSelectL->addWidget(ePickEquipBtn);
    eEquipSelectL->addWidget(eSelectedEquipLbl, 1);

    int* expSelectedEquipId = new int(-1);
    QList<EquipementRecord>* expAvailableEquipments = new QList<EquipementRecord>();

    QComboBox* eProjet = new QComboBox;
    eProjet->setObjectName("expFormInput");
    eProjet->addItem("Projet", QVariant());

    auto loadProjetsIntoCombo = [=]() {
        const int savedId = eProjet->currentData().isNull() ? -1 : eProjet->currentData().toInt();
        eProjet->blockSignals(true);
        eProjet->clear();
        eProjet->addItem("Projet", QVariant());
        QSqlQuery pq;
        if (pq.exec("SELECT \"Id_projet\", \"nom_du_projet\" FROM \"projet\" ORDER BY \"Id_projet\"")) {
            while (pq.next())
                eProjet->addItem(pq.value(1).toString(), pq.value(0).toInt());
        }
        int idx = eProjet->findData(savedId);
        if (idx >= 0) eProjet->setCurrentIndex(idx);
        eProjet->blockSignals(false);
    };

    auto refreshExpSelectedEquipLabel = [=]() {
        QString chosenName;
        for (const EquipementRecord& eq : *expAvailableEquipments) {
            if (eq.id == *expSelectedEquipId) {
                chosenName = eq.nomEquipement;
                break;
            }
        }
        if (chosenName.isEmpty()) {
            eSelectedEquipLbl->setText("Aucun équipement sélectionné");
        } else {
            eSelectedEquipLbl->setText(QString("Équipement sélectionné : %1").arg(chosenName));
        }
    };

    auto loadExperienceEquipments = [=](int selectedEquipId = -1) {
        syncFinishedExperienceEquipments();
        expAvailableEquipments->clear();
        QString err;
        if (!expEquipCrud->loadEquipements(*expAvailableEquipments, &err, QString(), QString(), "Disponible", QString())) {
            if (!err.trimmed().isEmpty()) showToast(this, "Erreur équipements : " + err, false);
            *expSelectedEquipId = -1;
            refreshExpSelectedEquipLabel();
            return;
        }

        if (expAvailableEquipments->isEmpty()) {
            *expSelectedEquipId = -1;
            refreshExpSelectedEquipLabel();
            return;
        }

        bool foundSelected = false;
        for (const EquipementRecord& eq : *expAvailableEquipments) {
            if (eq.id == selectedEquipId) {
                *expSelectedEquipId = selectedEquipId;
                foundSelected = true;
                break;
            }
        }
        if (!foundSelected) {
            *expSelectedEquipId = expAvailableEquipments->first().id;
        }
        refreshExpSelectedEquipLabel();
    };

    QObject::connect(ePickEquipBtn, &QPushButton::clicked, this, [=](){
        if (expAvailableEquipments->isEmpty()) {
            loadExperienceEquipments(*expSelectedEquipId);
        }
        if (expAvailableEquipments->isEmpty()) {
            showToast(this, "Aucun équipement disponible pour le moment.", false);
            return;
        }

        QDialog picker(this);
        picker.setWindowTitle("Choisir un équipement disponible");
        picker.resize(520, 360);
        const bool dark = g_darkThemeEnabled;
        picker.setStyleSheet(dark
            ? "QDialog{ background:#13181F; color:#E8ECF0; }"
            : "QDialog{ background:#F7FBFB; color:#1F2D3D; }");

        QVBoxLayout* pickerL = new QVBoxLayout(&picker);
        pickerL->setContentsMargins(12,12,12,12);
        pickerL->setSpacing(10);

        QLabel* pickerInfo = new QLabel("Sélectionnez un équipement puis cliquez sur Valider.");
        pickerInfo->setStyleSheet(QString("color:%1; font-weight:700;")
            .arg(dark ? "rgba(220,232,240,0.88)" : "rgba(0,0,0,0.65)"));
        pickerL->addWidget(pickerInfo);

        QListWidget* pickerList = new QListWidget;
        pickerList->setStyleSheet(QString(
            "QListWidget{ background:%1; color:%2; border:1px solid %3; border-radius:10px; }"
            "QListWidget::item{ padding:8px 10px; font-weight:700; }"
            "QListWidget::item:selected{ background:%4; color:%5; }")
            .arg(dark ? "rgba(22,34,46,0.96)" : "#FFFFFF",
                 dark ? "#E8ECF0" : "#1F2D3D",
                 dark ? "rgba(210,218,226,0.18)" : "rgba(15,72,84,0.18)",
                 dark ? "rgba(45,212,191,0.24)" : "rgba(168,214,225,0.45)",
                 dark ? "#EFFFFC" : "#0A5F58"));

        int rowToSelect = -1;
        for (const EquipementRecord& eq : *expAvailableEquipments) {
            QListWidgetItem* it = new QListWidgetItem(QString("[Disponible] %1").arg(eq.nomEquipement));
            it->setData(Qt::UserRole, eq.id);
            pickerList->addItem(it);
            if (eq.id == *expSelectedEquipId) rowToSelect = pickerList->count() - 1;
        }
        pickerList->setCurrentRow(rowToSelect >= 0 ? rowToSelect : 0);
        pickerL->addWidget(pickerList, 1);

        QHBoxLayout* pickerBtns = new QHBoxLayout;
        QPushButton* cancelBtn = new QPushButton("Annuler");
        QPushButton* okBtn = new QPushButton("Valider");
        okBtn->setDefault(true);
        cancelBtn->setStyleSheet(QString(
            "QPushButton{ background:%1; color:%2; border:1px solid %3; border-radius:10px;"
            " padding:8px 16px; font-weight:800; }"
            "QPushButton:hover{ background:%4; }")
            .arg(dark ? "rgba(255,255,255,0.07)" : "rgba(255,255,255,0.88)",
                 dark ? "#E8ECF0" : "#12443B",
                 dark ? "rgba(210,218,226,0.18)" : "rgba(0,0,0,0.10)",
                 dark ? "rgba(255,255,255,0.12)" : "rgba(255,255,255,0.97)"));
        okBtn->setStyleSheet(QString(
            "QPushButton{ background:%1; color:%2; border:1px solid %3; border-radius:10px;"
            " padding:8px 16px; font-weight:900; }"
            "QPushButton:hover{ background:%4; }")
            .arg(dark ? "rgba(45,212,191,0.18)" : "rgba(10,95,88,0.85)",
                 dark ? "#EFFFFC" : "#FFFFFF",
                 dark ? "rgba(45,212,191,0.32)" : "rgba(0,0,0,0.12)",
                 dark ? "rgba(45,212,191,0.28)" : "#12443B"));
        pickerBtns->addStretch(1);
        pickerBtns->addWidget(cancelBtn);
        pickerBtns->addWidget(okBtn);
        pickerL->addLayout(pickerBtns);

        QObject::connect(cancelBtn, &QPushButton::clicked, &picker, &QDialog::reject);
        QObject::connect(okBtn, &QPushButton::clicked, &picker, &QDialog::accept);
        QObject::connect(pickerList, &QListWidget::itemDoubleClicked, &picker, [&](QListWidgetItem*){ picker.accept(); });

        if (picker.exec() == QDialog::Accepted) {
            QListWidgetItem* it = pickerList->currentItem();
            if (it && it->data(Qt::UserRole).isValid()) {
                *expSelectedEquipId = it->data(Qt::UserRole).toInt();
                refreshExpSelectedEquipLabel();
            }
        }
    });

    e2LeftL->addWidget(expTitle("Informations"));
    e2LeftL->addWidget(expRow(QStyle::SP_DirIcon, "Expérience", eName));
    e2LeftL->addWidget(expRow(QStyle::SP_FileDialogDetailedView, "Hypothese", eHypo));
    e2LeftL->addWidget(expRow(QStyle::SP_FileDialogContentsView, "Type_Experience", eTypeExp));
    e2LeftL->addWidget(expRow(QStyle::SP_FileDialogListView, "Resultat", eResultat));
    e2LeftL->addWidget(expRow(QStyle::SP_DirIcon, "Nom projet", eProjet));
    e2LeftL->addStretch(1);

    // ── Right panel: Planification ────────────────────────────
    QFrame* e2Right = softBox();
    e2Right->setObjectName("expFormSection");
    QVBoxLayout* e2RightL = new QVBoxLayout(e2Right);
    e2RightL->setContentsMargins(12,12,12,12);
    e2RightL->setSpacing(10);

    auto makeDateEdit = [&](QDate d) {
        QDateEdit* de = new QDateEdit(d);
        de->setObjectName("expFormInput");
        de->setCalendarPopup(true);
        de->setDisplayFormat("dd/MM/yyyy");
        de->setMinimumWidth(190);
        return de;
    };

    auto applyExpCalendar = [](QDateEdit* de) {
        QCalendarWidget* cw = de->calendarWidget();
        if (!cw) return;
        const bool dark = g_darkThemeEnabled;

        cw->setStyleSheet(QString(
            "QCalendarWidget QWidget#qt_calendar_navigationbar {"
            "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 %1, stop:1 %2);"
            "  padding: 4px 6px; border-radius: 10px 10px 0 0;"
            "}"
            "QCalendarWidget QToolButton {"
            "  color: white; font-weight: 900; font-size: 13px;"
            "  background: transparent; border: none;"
            "  border-radius: 6px; padding: 4px 10px; min-width: 28px;"
            "}"
            "QCalendarWidget QToolButton:hover  { background: rgba(255,255,255,0.20); }"
            "QCalendarWidget QToolButton:pressed { background: rgba(255,255,255,0.12); }"
            "QCalendarWidget QSpinBox {"
            "  color: white; background: transparent; border: none;"
            "  font-weight: 900; selection-background-color: rgba(255,255,255,0.28);"
            "}"
            "QCalendarWidget QHeaderView::section {"
            "  background: %3; color: %4;"
            "  font-weight: 900; font-size: 11px;"
            "  border: none; padding: 5px 0;"
            "}"
            "QCalendarWidget QAbstractItemView {"
            "  background: %5;"
            "  selection-background-color: %6;"
            "  selection-color: %7;"
            "  color: %8;"
            "  border: 1px solid %9;"
            "  font-weight: 700; outline: none;"
            "}"
            "QCalendarWidget QAbstractItemView:disabled { color: %10; }"
            "QCalendarWidget QWidget { alternate-background-color: %11; }"
            "QCalendarWidget QMenu {"
            "  background: %5; color: %8;"
            "  selection-background-color: %6; selection-color: %7;"
            "}").arg(
                dark ? "#16222D" : "#5aa7be",
                dark ? "#223646" : "#3b879f",
                dark ? "rgba(45,212,191,0.18)" : "#d7edf3",
                dark ? "#D6FAF5" : "#245566",
                dark ? "#18212B" : "#ffffff",
                dark ? "rgba(45,212,191,0.24)" : "#cbeaf2",
                dark ? "#EFFFFC" : "#163746",
                dark ? "#E8ECF0" : "#234455",
                dark ? "rgba(210,218,226,0.18)" : "#c6e2ea",
                dark ? "rgba(180,195,205,0.45)" : "#a8b8bf",
                dark ? "#11181F" : "#f4fafc"));

        QTextCharFormat weekdayFmt;
        weekdayFmt.setForeground(QColor(dark ? "#B8EDE5" : "#245566"));
        cw->setWeekdayTextFormat(Qt::Saturday, weekdayFmt);
        cw->setWeekdayTextFormat(Qt::Sunday, weekdayFmt);

        QTextCharFormat todayFmt;
        todayFmt.setBackground(QColor(dark ? "#21484B" : "#e6f5fa"));
        todayFmt.setForeground(QColor(dark ? "#E8FFFB" : "#1f4f60"));
        todayFmt.setFontWeight(QFont::Black);
        cw->setDateTextFormat(QDate::currentDate(), todayFmt);
    };

    QDateEdit* eDateDebut = makeDateEdit(QDate::currentDate());
    QDateEdit* eDateFin   = makeDateEdit(QDate::currentDate().addDays(1));
    applyExpCalendar(eDateDebut);
    applyExpCalendar(eDateFin);
    eDateFin->setMinimumDate(eDateDebut->date().addDays(1));

    QObject::connect(eDateDebut, &QDateEdit::dateChanged, this, [=](const QDate& d){
        const QDate minFin = d.addDays(1);
        eDateFin->setMinimumDate(minFin);
        if (eDateFin->date() < minFin) {
            eDateFin->setDate(minFin);
        }
    });

    QComboBox* eStatus = new QComboBox;
    eStatus->setObjectName("expFormInput");
    eStatus->addItems({"Statut", "En cours", "Concluante", "Réussie", "Échouée", "Archivée"});
    eStatus->setFixedWidth(200);

    auto applyExpStatusControl = [=]() {
        const QDate today = QDate::currentDate();
        const QDate dEnd  = eDateFin->date();
        const QString prevStatus = eStatus->currentText();

        eStatus->blockSignals(true);
        eStatus->clear();

        if (dEnd > today) {
            // Si date fin > date système: statut par défaut = En cours
            eStatus->addItems({"Statut", "En cours"});
            eStatus->setCurrentIndex(1);
            eStatus->setEnabled(false);
        } else {
            // Si date fin <= date système: l'utilisateur choisit Échouée / Réussie
            eStatus->addItems({"Statut", "Échouée", "Réussie"});
            eStatus->setEnabled(true);
            int idx = eStatus->findText(prevStatus);
            if (idx < 0) idx = 0;
            eStatus->setCurrentIndex(idx);
        }

        eStatus->blockSignals(false);
    };

    QObject::connect(eDateFin, &QDateEdit::dateChanged, this, [=](const QDate&){
        applyExpStatusControl();
    });
    applyExpStatusControl();

    e2RightL->addWidget(expTitle("Planification"));
    e2RightL->addWidget(expRow(QStyle::SP_MessageBoxInformation, "Date début", eDateDebut));
    e2RightL->addWidget(expRow(QStyle::SP_MessageBoxInformation, "Date fin",   eDateFin));
    e2RightL->addWidget(expRow(QStyle::SP_MessageBoxInformation, "Statut",     eStatus));
    e2RightL->addWidget(expRow(QStyle::SP_FileDialogListView, "Équipement", eEquipSelectBox));
    e2RightL->addStretch(1);

    loadExperienceEquipments();

    outE2L->addWidget(e2Left);
    outE2L->addWidget(e2Right, 1);

    QScrollArea* expFormScrollWrap = new QScrollArea;
    expFormScrollWrap->setWidgetResizable(true);
    expFormScrollWrap->setFrameShape(QFrame::NoFrame);
    expFormScrollWrap->setStyleSheet(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollArea > QWidget > QWidget { background: transparent; }");
    expFormScrollWrap->setWidget(outE2);
    ep2->addWidget(expFormScrollWrap, 1);

    QFrame* e2Bottom = new QFrame;
    e2Bottom->setFixedHeight(64);
    e2Bottom->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* e2BottomL = new QHBoxLayout(e2Bottom);
    e2BottomL->setContentsMargins(14,10,14,10);
    e2BottomL->setSpacing(12);

    QPushButton* expSave   = actionBtn("Enregistrer", "rgba(10,95,88,0.45)", "rgba(255,255,255,0.90)", st->standardIcon(QStyle::SP_DialogSaveButton), true);
    QPushButton* expCancel = actionBtn("Annuler",     "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_DialogCancelButton), true);

    e2BottomL->addWidget(expSave);
    e2BottomL->addWidget(expCancel);
    e2BottomL->addStretch(1);

    ep2->addWidget(e2Bottom);
    addStackPage(exp2);
    // ==========================================================
    // PAGE 10 : Expériences & Protocoles - Widget 3 (STATISTIQUES)
    // ==========================================================
    QWidget* exp3 = new QWidget;
    QVBoxLayout* ep3 = new QVBoxLayout(exp3);
    ep3->setContentsMargins(22, 18, 22, 18);
    ep3->setSpacing(14);

    ModulesBar barExpStats;
    ep3->addWidget(makeHeaderBlock(st, "Statistiques Expériences", ModuleTab::ExperiencesProtocoles, &barExpStats));
    connectModulesSwitch(this, stack, barExpStats);

    QFrame* outE3 = new QFrame;
    outE3->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }").arg(C_PANEL_BG, C_PANEL_BR));
    QVBoxLayout* outE3L = new QVBoxLayout(outE3);
    outE3L->setContentsMargins(12,12,12,12);
    outE3L->setSpacing(12);

    QFrame* actE3 = new QFrame;
    actE3->setStyleSheet("QFrame{ background: rgba(255,255,255,0.35); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QHBoxLayout* actE3L = new QHBoxLayout(actE3);
    actE3L->setContentsMargins(12,10,12,10);

    QLabel* he = new QLabel("Aperçu : statut & volume mensuel");
    he->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");

    QPushButton* exportE3 = actionBtn("Exporter", "rgba(10,95,88,0.45)", "rgba(255,255,255,0.92)", st->standardIcon(QStyle::SP_DialogSaveButton), true);

    actE3L->addWidget(he);
    actE3L->addStretch(1);
    actE3L->addWidget(exportE3);
    outE3L->addWidget(actE3);

    QFrame* dashE3 = new QFrame;
    dashE3->setStyleSheet("QFrame{ background: rgba(255,255,255,0.55); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QHBoxLayout* dashE3L = new QHBoxLayout(dashE3);
    dashE3L->setContentsMargins(12,12,12,12);
    dashE3L->setSpacing(12);

    QFrame* pieE = softBox();
    QVBoxLayout* pieEL = new QVBoxLayout(pieE);
    pieEL->setContentsMargins(12,12,12,12);
    pieEL->setSpacing(10);

    QLabel* pieET = new QLabel("Répartition des expériences par statut");
    pieET->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");

    DonutChart* donutE = new DonutChart;

    pieEL->addWidget(pieET);
    pieEL->addWidget(donutE, 1);

    QFrame* barE = softBox();
    QVBoxLayout* barEL = new QVBoxLayout(barE);
    barEL->setContentsMargins(12,12,12,12);
    barEL->setSpacing(10);

    QLabel* barET = new QLabel("Nombre d’expériences par mois");
    barET->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");

    BarChart* barsE = new BarChart;

    barEL->addWidget(barET);
    barEL->addWidget(barsE, 1);

    dashE3L->addWidget(pieE, 1);
    dashE3L->addWidget(barE, 2);

    outE3L->addWidget(dashE3, 1);

    QFrame* e3Bottom = new QFrame;
    e3Bottom->setFixedHeight(64);
    e3Bottom->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* e3BottomL = new QHBoxLayout(e3Bottom);
    e3BottomL->setContentsMargins(14,10,14,10);

    QPushButton* expBackStats = actionBtn("Retour", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_ArrowBack), true);
    e3BottomL->addWidget(expBackStats);
    e3BottomL->addStretch(1);

    outE3L->addWidget(e3Bottom);

    QScrollArea* exp3Scroll = new QScrollArea;
    exp3Scroll->setObjectName("exp3Scroll");
    exp3Scroll->setWidgetResizable(true);
    exp3Scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    exp3Scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    exp3Scroll->setFrameShape(QFrame::NoFrame);
    exp3Scroll->setStyleSheet(R"(
        QScrollArea#exp3Scroll {
            background: transparent;
            border: none;
        }
        QScrollArea#exp3Scroll > QWidget > QWidget {
            background: transparent;
        }
        QScrollBar:vertical {
            background: rgba(0,0,0,0.06);
            width: 6px;
            border-radius: 3px;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background: rgba(45,212,191,0.50);
            border-radius: 3px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover {
            background: rgba(45,212,191,0.80);
        }
        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical { height: 0px; }
        QScrollBar::add-page:vertical,
        QScrollBar::sub-page:vertical { background: none; }
    )");
    exp3Scroll->setWidget(outE3);
    ep3->addWidget(exp3Scroll, 1);

    addStackPage(exp3);

    auto updateExpStats = [=](){
        QList<ExperienceRecord> recs;
        QString err;
        if (!expCrud->loadExperiences(recs, &err)) {
            donutE->setData({});
            barsE->setData({});
            return;
        }

        const QMap<QString, int> statusCounts = ExperienceAnalytics::countByStatus(recs);
        QList<DonutChart::Slice> slices;
        slices.push_back({(double)statusCounts.value("En cours"), W_GREEN, "En cours"});
        slices.push_back({(double)statusCounts.value("Suspendue"), W_RED, "Suspendue"});
        slices.push_back({(double)statusCounts.value("Terminée"), QColor("#3A7CA5"), "Terminée"});
        slices.push_back({(double)statusCounts.value("En attente"), W_ORANGE, "En attente"});
        donutE->setData(slices);

        QList<BarChart::Bar> bars;
        const QList<QPair<int, QString>> byMonth = ExperienceAnalytics::countByMonth(recs);
        for (const auto& p : byMonth) {
            bars.push_back({(double)p.first, p.second});
        }
        barsE->setData(bars);
    };

    QObject::connect(stack, &QStackedWidget::currentChanged, exp3, [=](int idx){
        if (idx == EXP_STATS) updateExpStats();
    });

    // ==========================================================
    // ======================  PUBLICATIONS  =====================
    // ==========================================================

    // ==========================================================
    // PAGE 11 : Publications - LISTE (PUB_LIST)
    // ==========================================================
    // PAGE 11 : Publications - LISTE (PUB_LIST)
    // ==========================================================
    QWidget* pub1 = new QWidget;
    QVBoxLayout* pb1 = new QVBoxLayout(pub1);
    pb1->setContentsMargins(22, 18, 22, 18);
    pb1->setSpacing(14);

    ModulesBar barPubList;
    pb1->addWidget(makeHeaderBlock(st, "Publications", ModuleTab::Publication, &barPubList));
    connectModulesSwitch(this, stack, barPubList);

    // Barre de recherche / filtres
    QFrame* pubBar = new QFrame;
    pubBar->setFixedHeight(54);
    pubBar->setStyleSheet("background: rgba(255,255,255,0.22); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* pubBarL = new QHBoxLayout(pubBar);
    pubBarL->setContentsMargins(14, 8, 14, 8);
    pubBarL->setSpacing(10);

    QLineEdit* pubSearch = new QLineEdit;
    pubSearch->setPlaceholderText("Rechercher (auteur, mots-clés, titre, journal, DOI...)");
    pubSearch->addAction(st->standardIcon(QStyle::SP_FileDialogContentsView), QLineEdit::LeadingPosition);

    QComboBox* pubType = new QComboBox;
    pubType->addItems({"Statut", "Brouillon", "Soumise", "Acceptée", "Publiée", "Rejetée"});

    QComboBox* pubYear = new QComboBox;
    pubYear->addItems({"Année", "2026", "2025", "2024", "2023", "2022"});

    QComboBox* pubSort = new QComboBox;
    pubSort->addItems({"Tri: Année", "Tri: Impact Factor", "Tri: Citation"});

    QComboBox* pubSortOrder = new QComboBox;
    pubSortOrder->addItems({"Décroissant", "Croissant"});

    QPushButton* pubFilters = new QPushButton(st->standardIcon(QStyle::SP_FileDialogDetailedView), "  Filtres");
    pubFilters->setCursor(Qt::PointingHandCursor);
    pubFilters->setStyleSheet(QString(R"(
QPushButton{
    background:%1; color: rgba(255,255,255,0.92);
    border:1px solid rgba(0,0,0,0.18);
    border-radius: 12px; padding: 10px 16px; font-weight: 800;
}
QPushButton:hover{ background: %2; }
)").arg(C_PRIMARY, C_TOPBAR));

    pubBarL->addWidget(pubSearch, 1);
    pubBarL->addWidget(pubType);
    pubBarL->addWidget(pubYear);
    pubBarL->addWidget(pubSort);
    pubBarL->addWidget(pubSortOrder);
    pubBarL->addWidget(pubFilters);
    pb1->addWidget(pubBar);

    // Table
    QFrame* pubCard = makeCard();
    QVBoxLayout* pubCardL = new QVBoxLayout(pubCard);
    pubCardL->setContentsMargins(10,10,10,10);

    QTableWidget* pubTable = new QTableWidget(0, 11);
    pubTable->setHorizontalHeaderLabels({"ID","Titre","Journal/Conf.","Année","DOI","Statut","Employé","Mots-clés","Impact","Citation","Santé"});
    pubTable->verticalHeader()->setVisible(false);
    pubTable->horizontalHeader()->setStretchLastSection(true);
    pubTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    pubTable->setSelectionMode(QAbstractItemView::SingleSelection);
    pubTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    pubTable->setColumnHidden(0, true);
    pubTable->setColumnHidden(7, true);
    pubTable->setColumnWidth(8, 90);
    pubTable->setColumnWidth(9, 260);
    pubTable->setColumnWidth(10, 100);

    auto formatPublicationCitation = [=](const QString& title, const QString& authors, const QString& journal, int year, const QString& doi) {
        QString citation;
        const QString authorText = authors.trimmed();
        if (!authorText.isEmpty() && authorText.compare("Aucun employé", Qt::CaseInsensitive) != 0) {
            citation += authorText;
        }
        if (year > 0) {
            if (!citation.isEmpty()) citation += " ";
            citation += "(" + QString::number(year) + ")";
        }
        if (!title.trimmed().isEmpty()) {
            if (!citation.isEmpty() && !citation.endsWith('.')) citation += ".";
            if (!citation.isEmpty()) citation += " ";
            citation += title.trimmed();
        }
        if (!journal.trimmed().isEmpty()) {
            if (!citation.endsWith('.')) citation += ".";
            citation += " " + journal.trimmed();
        }
        if (!doi.trimmed().isEmpty()) {
            QString doiText = doi.trimmed();
            if (!doiText.startsWith("http", Qt::CaseInsensitive) && !doiText.toLower().startsWith("doi:")) {
                doiText = "doi:" + doiText;
            }
            if (!citation.endsWith('.')) citation += ".";
            citation += " " + doiText;
        }
        citation = citation.trimmed();
        if (!citation.endsWith('.')) citation += ".";
        return citation;
    };

    auto reloadPublications = [=](){
        QString errorMessage;
        QSqlQueryModel* model = Publication::readAll(nullptr, &errorMessage);
        if (!model) {
            QMessageBox::warning(this, "Publication", "Chargement impossible :\n" + errorMessage);
            return;
        }

        pubTable->setRowCount(0);
        for (int r = 0; r < model->rowCount(); ++r) {
            pubTable->insertRow(r);
            const int maxCols = std::min(pubTable->columnCount() - 1, model->columnCount()); // -1 to exclude health column
            for (int c = 0; c < maxCols; ++c) {
                QTableWidgetItem* it = new QTableWidgetItem(model->data(model->index(r, c)).toString());
                it->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                pubTable->setItem(r, c, it);
            }

            // Calculate and display health score
            int pubId = pubTable->item(r, 0) ? pubTable->item(r, 0)->text().toInt() : 0;
            if (pubId > 0) {
                Publication pub;
                if (Publication::readById(pubId, pub, &errorMessage)) {
                    PublicationScorer::ScoreBreakdown scores = PublicationScorer::calculateScores(pub);
                    const QString healthStatus = PublicationScorer::getHealthStatus(scores.totalHealthScore);

                    QTableWidgetItem* healthItem = new QTableWidgetItem(healthStatus);
                    healthItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);

                    // Color code the health indicator
                    if (healthStatus == "Excellent") {
                        healthItem->setForeground(W_GREEN);
                        healthItem->setFont(QFont("Arial", 10, QFont::Bold));
                    } else if (healthStatus == "Good") {
                        healthItem->setForeground(QColor("#3498db"));
                        healthItem->setFont(QFont("Arial", 10, QFont::Bold));
                    } else if (healthStatus == "Fair") {
                        healthItem->setForeground(W_ORANGE);
                        healthItem->setFont(QFont("Arial", 10, QFont::Bold));
                    } else {
                        healthItem->setForeground(W_RED);
                        healthItem->setFont(QFont("Arial", 10, QFont::Bold));
                    }

                    // Add tooltip with score details
                    QString tooltipText = QString(
                        "Health Score: %1/100\n"
                        "Completeness: %2%\n"
                        "Citations: %3%\n"
                        "Impact: %4%\n"
                        "Recency: %5%\n"
                        "Project Link: %6%\n"
                        "Duplication: %7%"
                    )
                    .arg(static_cast<int>(scores.totalHealthScore))
                    .arg(static_cast<int>(scores.completenessScore))
                    .arg(static_cast<int>(scores.citationScore))
                    .arg(static_cast<int>(scores.impactFactorScore))
                    .arg(static_cast<int>(scores.recencyScore))
                    .arg(static_cast<int>(scores.projectLinkageScore))
                    .arg(static_cast<int>(scores.duplicationRiskScore));

                    healthItem->setToolTip(tooltipText);
                    pubTable->setItem(r, 10, healthItem);
                }
            }

            const QString title = pubTable->item(r, 1) ? pubTable->item(r, 1)->text() : QString();
            const QString author = pubTable->item(r, 6) ? pubTable->item(r, 6)->text() : QString();
            const QString journal = pubTable->item(r, 2) ? pubTable->item(r, 2)->text() : QString();
            const int year = pubTable->item(r, 3) ? pubTable->item(r, 3)->text().toInt() : 0;
            const QString doi = pubTable->item(r, 4) ? pubTable->item(r, 4)->text() : QString();
            if (QTableWidgetItem* citationItem = pubTable->item(r, 9)) {
                citationItem->setText(formatPublicationCitation(title, author, journal, year, doi));
            }
            pubTable->setRowHeight(r, 46);
        }
        delete model;
    };

    auto applyPublicationSort = [=](){
        struct PubRow {
            QStringList cols;
        };

        QList<PubRow> rows;
        rows.reserve(pubTable->rowCount());

        for (int r = 0; r < pubTable->rowCount(); ++r) {
            PubRow row;
            row.cols.reserve(pubTable->columnCount());
            for (int c = 0; c < pubTable->columnCount(); ++c) {
                row.cols << (pubTable->item(r, c) ? pubTable->item(r, c)->text() : QString());
            }
            rows.push_back(row);
        }

        const bool descending = (pubSortOrder->currentText() == "Décroissant");
        const int sortMode = pubSort->currentIndex();

        auto toDoubleValue = [](const QString& text) {
            bool ok = false;
            const double value = text.trimmed().toDouble(&ok);
            return ok ? value : 0.0;
        };
        auto toIntValue = [](const QString& text) {
            bool ok = false;
            const int value = text.trimmed().toInt(&ok);
            return ok ? value : 0;
        };

        std::stable_sort(rows.begin(), rows.end(), [&](const PubRow& a, const PubRow& b){
            if (sortMode == 1) {
                const double av = toDoubleValue(a.cols.value(8));
                const double bv = toDoubleValue(b.cols.value(8));
                return descending ? (av > bv) : (av < bv);
            }
            if (sortMode == 2) {
                const QString av = a.cols.value(9);
                const QString bv = b.cols.value(9);
                return descending ? (av > bv) : (av < bv);
            }

            const int av = toIntValue(a.cols.value(3));
            const int bv = toIntValue(b.cols.value(3));
            if (av == bv) {
                const int aid = toIntValue(a.cols.value(0));
                const int bid = toIntValue(b.cols.value(0));
                return descending ? (aid > bid) : (aid < bid);
            }
            return descending ? (av > bv) : (av < bv);
        });

        pubTable->setRowCount(0);
        for (int r = 0; r < rows.size(); ++r) {
            pubTable->insertRow(r);
            for (int c = 0; c < pubTable->columnCount(); ++c) {
                QTableWidgetItem* it = new QTableWidgetItem(rows[r].cols.value(c));
                it->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                pubTable->setItem(r, c, it);
            }
            pubTable->setRowHeight(r, 46);
        }
    };

    auto applyPublicationFilters = [=](){
        const QString search = pubSearch->text().trimmed().toLower();
        const QString status = pubType->currentText();
        const QString year = pubYear->currentText();

        for (int r = 0; r < pubTable->rowCount(); ++r) {
            const QString id = pubTable->item(r,0) ? pubTable->item(r,0)->text().toLower() : QString();
            const QString titre = pubTable->item(r,1) ? pubTable->item(r,1)->text().toLower() : QString();
            const QString journal = pubTable->item(r,2) ? pubTable->item(r,2)->text().toLower() : QString();
            const QString annee = pubTable->item(r,3) ? pubTable->item(r,3)->text() : QString();
            const QString doi = pubTable->item(r,4) ? pubTable->item(r,4)->text().toLower() : QString();
            const QString statut = pubTable->item(r,5) ? pubTable->item(r,5)->text() : QString();
            const QString employe = pubTable->item(r,6) ? pubTable->item(r,6)->text().toLower() : QString();
            const QString keywords = pubTable->item(r,7) ? pubTable->item(r,7)->text().toLower() : QString();

            const bool matchesSearch = search.isEmpty()
                || id.contains(search)
                || titre.contains(search)
                || journal.contains(search)
                || annee.toLower().contains(search)
                || doi.contains(search)
                || employe.contains(search)
                || keywords.contains(search);

            const bool matchesStatus = (status == "Statut") || (statut == status);
            const bool matchesYear = (year == "Année") || (annee == year);
            pubTable->setRowHidden(r, !(matchesSearch && matchesStatus && matchesYear));
        }
    };

    QObject::connect(pubSearch, &QLineEdit::textChanged, this, [=](){ applyPublicationFilters(); });
    QObject::connect(pubType, &QComboBox::currentTextChanged, this, [=](){ applyPublicationFilters(); });
    QObject::connect(pubYear, &QComboBox::currentTextChanged, this, [=](){ applyPublicationFilters(); });
    QObject::connect(pubSort, &QComboBox::currentIndexChanged, this, [=](int){ applyPublicationSort(); applyPublicationFilters(); });
    QObject::connect(pubSortOrder, &QComboBox::currentIndexChanged, this, [=](int){ applyPublicationSort(); applyPublicationFilters(); });
    reloadPublications();
    applyPublicationSort();
    applyPublicationFilters();

    pubCardL->addWidget(pubTable);
    pb1->addWidget(pubCard, 1);

    // Bottom actions (Ajouter / Modifier / Stats / Retour optionnel)
    QFrame* pubBottom = new QFrame;
    pubBottom->setFixedHeight(64);
    pubBottom->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* pubBottomL = new QHBoxLayout(pubBottom);
    pubBottomL->setContentsMargins(14,10,14,10);
    pubBottomL->setSpacing(12);

    QPushButton* pubAdd     = actionBtn("Ajouter",      "rgba(10,95,88,0.45)", "rgba(255,255,255,0.90)", st->standardIcon(QStyle::SP_DialogYesButton), true);
    QPushButton* pubEdit    = actionBtn("Modifier",     "rgba(198,178,154,0.55)", "rgba(255,255,255,0.85)", st->standardIcon(QStyle::SP_FileDialogContentsView), true);
    QPushButton* pubDetails = actionBtn("Détails",      "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_DialogHelpButton), true);
    QPushButton* pubDel     = actionBtn("Supprimer",    "rgba(255,255,255,0.55)", "#B14A4A", st->standardIcon(QStyle::SP_TrashIcon), true);
    QPushButton* pubStats   = actionBtn("Statistiques", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_ComputerIcon), true);

    QObject::connect(pubDel, &QPushButton::clicked, this, [=](){
        if (*currentRole != "Responsable") { showToast(this, "Accès refusé : seul le Responsable peut supprimer.", false); return; }
        int r = pubTable->currentRow();
        if (r < 0) {
            QMessageBox::information(this, "Information", "Veuillez sélectionner une publication à supprimer.");
            return;
        }
           QString resume = QString("Titre : %1 | Année : %2 | Statut : %3")
                            .arg(pubTable->item(r,1)->text(),
                                pubTable->item(r,3)->text(),
                                pubTable->item(r,5)->text());
        ConfirmDeleteDialog confirm(style(), resume, this);
        if (confirm.exec() != QDialog::Accepted) {
            return;
        }

        QString errorMessage;
        if (!Publication::remove(pubTable->item(r,0)->text().toInt(), &errorMessage)) {
            QMessageBox::warning(this, "Suppression", "Échec de suppression :\n" + errorMessage);
            return;
        }

        reloadPublications();
        applyPublicationFilters();
    });

    pubBottomL->addWidget(pubAdd);
    pubBottomL->addWidget(pubEdit);
    pubBottomL->addWidget(pubDetails);
    pubBottomL->addWidget(pubDel);
    pubBottomL->addWidget(pubStats);
    pubBottomL->addStretch(1);

    pb1->addWidget(pubBottom);
    addStackPage(pub1);

    // ==========================================================
    // PAGE 12 : Publications - AJOUT / MODIF (PUB_FORM)
    // ==========================================================
    QWidget* pub2 = new QWidget;
    pub2->setObjectName("pubFormPage");
    QVBoxLayout* pb2 = new QVBoxLayout(pub2);
    pb2->setContentsMargins(22, 18, 22, 18);
    pb2->setSpacing(14);

    ModulesBar barPubForm;
    pb2->addWidget(makeHeaderBlock(st, "Ajouter / Modifier une publication", ModuleTab::Publication, &barPubForm));
    connectModulesSwitch(this, stack, barPubForm);

    QFrame* outPUB2 = new QFrame;
    outPUB2->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }").arg(C_PANEL_BG, C_PANEL_BR));
    QHBoxLayout* outPUB2L = new QHBoxLayout(outPUB2);
    outPUB2L->setContentsMargins(12,12,12,12);
    outPUB2L->setSpacing(12);

    auto pubTitle = [&](const QString& t){
        QLabel* lab = new QLabel(t);
        lab->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
        return lab;
    };
    auto pubRow = [&](QStyle::StandardPixmap sp, const QString& label, QWidget* input){
        QFrame* r = softBox();
        QHBoxLayout* l = new QHBoxLayout(r);
        l->setContentsMargins(12,14,12,14);
        l->setSpacing(12);

        QToolButton* ic = new QToolButton;
        ic->setAutoRaise(true);
        ic->setIcon(st->standardIcon(sp));

        QLabel* lab = new QLabel(label);
        lab->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");

        l->addWidget(ic);
        l->addWidget(lab);
        l->addStretch(1);
        l->addWidget(input);
        return r;
    };

    QFrame* pub2Left = softBox();
    pub2Left->setFixedWidth(430);
    QVBoxLayout* pub2LeftL = new QVBoxLayout(pub2Left);
    pub2LeftL->setContentsMargins(12,12,12,12);
    pub2LeftL->setSpacing(16);

    QSpinBox* sbPubId = new QSpinBox;
    sbPubId->setRange(0, 1000000000);
    sbPubId->setVisible(false);

    QLineEdit* leTitle = new QLineEdit;
    leTitle->setPlaceholderText("Titre de la publication");

    QSpinBox* sbYear = new QSpinBox;
    sbYear->setRange(1950, 2100);
    sbYear->setValue(QDate::currentDate().year());
    sbYear->setFixedWidth(180);
    sbYear->setStyleSheet("QSpinBox{ background: rgba(255,255,255,0.65); border: 1px solid rgba(0,0,0,0.15); border-radius: 12px; padding: 10px 14px; color: rgba(0,0,0,0.65); font-weight: 900; }");

    pub2LeftL->addWidget(pubTitle("Informations"));
    pub2LeftL->addWidget(pubRow(QStyle::SP_FileDialogDetailedView, "Titre", leTitle));
    pub2LeftL->addWidget(pubRow(QStyle::SP_FileDialogInfoView, "Année", sbYear));
    pub2LeftL->addStretch(1);

    QFrame* pub2Right = softBox();
    QVBoxLayout* pub2RightL = new QVBoxLayout(pub2Right);
    pub2RightL->setContentsMargins(12,12,12,12);
    pub2RightL->setSpacing(16);

    QLineEdit* leJournal = new QLineEdit;
    leJournal->setPlaceholderText("Journal / Conférence");

    QLineEdit* leDOI = new QLineEdit;
    leDOI->setPlaceholderText("DOI (ex: 10.1000/xyz)");

    QDoubleSpinBox* sbImpactFactor = new QDoubleSpinBox;
    sbImpactFactor->setRange(0.0, 1000.0);
    sbImpactFactor->setDecimals(2);
    sbImpactFactor->setSingleStep(0.1);
    sbImpactFactor->setFixedWidth(220);
    sbImpactFactor->setStyleSheet("QDoubleSpinBox{ background: rgba(255,255,255,0.65); border: 1px solid rgba(0,0,0,0.15); border-radius: 12px; padding: 10px 14px; color: rgba(0,0,0,0.65); font-weight: 900; }");

    QTextEdit* leCitations = new QTextEdit;
    leCitations->setPlaceholderText("Citations (ex: Smith et al., 2020; Johnson, 2021)");
    leCitations->setFixedSize(220, 80);
    leCitations->setStyleSheet("QTextEdit{ background: rgba(255,255,255,0.65); border: 1px solid rgba(0,0,0,0.15); border-radius: 12px; padding: 10px 14px; color: rgba(0,0,0,0.65); font-weight: 900; }");

    QComboBox* cbStatus = new QComboBox;
    cbStatus->addItems({"Statut", "Brouillon", "Soumise", "Acceptée", "Publiée", "Rejetée"});
    cbStatus->setFixedWidth(220);

    QComboBox* cbEmployee = new QComboBox;
    cbEmployee->setEditable(false);
    cbEmployee->setInsertPolicy(QComboBox::NoInsert);
    cbEmployee->setFixedWidth(260);
    cbEmployee->setStyleSheet("QComboBox{ background: rgba(255,255,255,0.65); border: 1px solid rgba(0,0,0,0.15); border-radius: 12px; padding: 10px 14px; color: rgba(0,0,0,0.65); font-weight: 900; } QComboBox::drop-down{ border: none; width: 24px; }");

    cbEmployee->addItem("Sélectionner un employé", QVariant());
    {
        QSqlQuery employeeQuery;
        if (employeeQuery.exec("SELECT \"employee_id\", NVL(NULLIF(TRIM(\"FULL_NAME\"), ''), TRIM(\"prenom\" || ' ' || \"nom\")) "
                               "FROM \"Employés\" WHERE NVL(\"ACTIVE\", 'O') = 'O' ORDER BY \"nom\", \"prenom\", \"employee_id\"")) {
            while (employeeQuery.next()) {
                cbEmployee->addItem(employeeQuery.value(1).toString().trimmed(), employeeQuery.value(0).toInt());
            }
        }
    }
    cbEmployee->setCurrentIndex(0);

    QTextEdit* teAbstract = new QTextEdit;
    teAbstract->setPlaceholderText("Résumé / Notes / Mots-clés (traçabilité scientifique)");
    teAbstract->setStyleSheet("QTextEdit{ background: rgba(255,255,255,0.65); border: 1px solid rgba(0,0,0,0.15); border-radius: 12px; padding: 10px 14px; color: rgba(0,0,0,0.65); font-weight: 800; }");

    pub2RightL->addWidget(pubTitle("Détails"));
    pub2RightL->addWidget(pubRow(QStyle::SP_DirHomeIcon, "Journal/Conf.", leJournal));
    pub2RightL->addWidget(pubRow(QStyle::SP_FileDialogContentsView, "DOI", leDOI));
    pub2RightL->addWidget(pubRow(QStyle::SP_ArrowUp, "Impact Factor", sbImpactFactor));
    pub2RightL->addWidget(pubRow(QStyle::SP_ArrowUp, "Citations", leCitations));
    pub2RightL->addWidget(pubRow(QStyle::SP_MessageBoxInformation, "Statut", cbStatus));
    pub2RightL->addWidget(pubRow(QStyle::SP_DirHomeIcon, "Employé", cbEmployee));
    pub2RightL->addWidget(teAbstract, 1);

    outPUB2L->addWidget(pub2Left);
    outPUB2L->addWidget(pub2Right, 1);
    pb2->addWidget(outPUB2, 1);

    // Bottom : Save / Annuler
    QFrame* pub2Bottom = new QFrame;
    pub2Bottom->setFixedHeight(64);
    pub2Bottom->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* pub2BottomL = new QHBoxLayout(pub2Bottom);
    pub2BottomL->setContentsMargins(14,10,14,10);
    pub2BottomL->setSpacing(12);

    QPushButton* pubSave   = actionBtn("Enregistrer", "rgba(10,95,88,0.45)", "rgba(255,255,255,0.90)", st->standardIcon(QStyle::SP_DialogSaveButton), true);
    QPushButton* pubCancel = actionBtn("Annuler",     "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_DialogCancelButton), true);

    pub2BottomL->addWidget(pubSave);
    pub2BottomL->addWidget(pubCancel);
    pub2BottomL->addStretch(1);

    pb2->addWidget(pub2Bottom);
    addStackPage(pub2);

    // ==========================================================
    // PAGE 13 : Publications - STATISTIQUES (PUB_STATS)
    // ==========================================================
    QWidget* pub3 = new QWidget;
    QVBoxLayout* pb3 = new QVBoxLayout(pub3);
    pb3->setContentsMargins(22, 18, 22, 18);
    pb3->setSpacing(14);

    ModulesBar barPubStats;
    pb3->addWidget(makeHeaderBlock(st, "Statistiques Publications", ModuleTab::Publication, &barPubStats));
    connectModulesSwitch(this, stack, barPubStats);

    QFrame* outPUB3 = new QFrame;
    outPUB3->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }").arg(C_PANEL_BG, C_PANEL_BR));
    QVBoxLayout* outPUB3L = new QVBoxLayout(outPUB3);
    outPUB3L->setContentsMargins(12,12,12,12);
    outPUB3L->setSpacing(12);

    QFrame* actPUB3 = new QFrame;
    actPUB3->setStyleSheet("QFrame{ background: rgba(255,255,255,0.35); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QHBoxLayout* actPUB3L = new QHBoxLayout(actPUB3);
    actPUB3L->setContentsMargins(12,10,12,10);

    QLabel* hpPUB = new QLabel("Statistiques des publications");
    hpPUB->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");

    actPUB3L->addWidget(hpPUB);
    actPUB3L->addStretch(1);
    outPUB3L->addWidget(actPUB3);

    QFrame* dashPUB = new QFrame;
    dashPUB->setStyleSheet("QFrame{ background: rgba(255,255,255,0.55); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QHBoxLayout* dashPUBL = new QHBoxLayout(dashPUB);
    dashPUBL->setContentsMargins(12,12,12,12);
    dashPUBL->setSpacing(12);

    QFrame* piePUB = softBox();
    QVBoxLayout* piePUBL = new QVBoxLayout(piePUB);
    piePUBL->setContentsMargins(12,12,12,12);
    piePUBL->setSpacing(10);

    QLabel* piePUBT = new QLabel("Répartition des publications par statut");
    piePUBT->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");

    DonutChart* donutPUB = new DonutChart;
    QWidget* piePUBLegend = new QWidget;
    QGridLayout* piePUBLegendL = new QGridLayout(piePUBLegend);
    piePUBLegendL->setContentsMargins(2, 0, 2, 0);
    piePUBLegendL->setHorizontalSpacing(10);
    piePUBLegendL->setVerticalSpacing(6);

    piePUBL->addWidget(piePUBT);
    piePUBL->addWidget(donutPUB, 1);
    piePUBL->addWidget(piePUBLegend);

    QFrame* barPUB = softBox();
    QVBoxLayout* barPUBL = new QVBoxLayout(barPUB);
    barPUBL->setContentsMargins(12,12,12,12);
    barPUBL->setSpacing(10);

    QLabel* barPUBT = new QLabel("Publications par année");
    barPUBT->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");

    BarChart* barsPUB = new BarChart;

    barPUBL->addWidget(barPUBT);
    barPUBL->addWidget(barsPUB, 1);

    dashPUBL->addWidget(piePUB, 1);
    dashPUBL->addWidget(barPUB, 2);

    outPUB3L->addWidget(dashPUB, 1);

    // Health Coach Section
    QFrame* healthCoachFrame = new QFrame;
    healthCoachFrame->setStyleSheet("QFrame{ background: rgba(255,255,255,0.55); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QVBoxLayout* healthCoachLayout = new QVBoxLayout(healthCoachFrame);
    healthCoachLayout->setContentsMargins(12,12,12,12);
    healthCoachLayout->setSpacing(10);

    QLabel* healthCoachTitle = new QLabel("Publication Impact Coach");
    QFont hcFont = healthCoachTitle->font();
    hcFont.setBold(true);
    hcFont.setPointSize(11);
    healthCoachTitle->setFont(hcFont);
    healthCoachTitle->setStyleSheet("color: rgba(0,0,0,0.75);");

    // Health score distribution
    QFrame* scoreDistFrame = new QFrame;
    scoreDistFrame->setStyleSheet("QFrame{ background: rgba(255,255,255,0.70); border: 1px solid rgba(0,0,0,0.08); border-radius: 8px; }");
    QHBoxLayout* scoreDistLayout = new QHBoxLayout(scoreDistFrame);
    scoreDistLayout->setContentsMargins(10,8,10,8);
    scoreDistLayout->setSpacing(8);

    QLabel* scoreDistLabel = new QLabel("Health Score Distribution:");
    scoreDistLabel->setStyleSheet("color: rgba(0,0,0,0.65); font-size: 10px;");
    scoreDistLayout->addWidget(scoreDistLabel);

    QLabel* scoreExcellent = new QLabel("Excellent: 0");
    scoreExcellent->setStyleSheet("color: #2ecc71; font-weight: bold; font-size: 10px;");
    QLabel* scoreGood = new QLabel("Good: 0");
    scoreGood->setStyleSheet("color: #3498db; font-weight: bold; font-size: 10px;");
    QLabel* scoreFair = new QLabel("Fair: 0");
    scoreFair->setStyleSheet("color: #f39c12; font-weight: bold; font-size: 10px;");
    QLabel* scorePoor = new QLabel("Poor: 0");
    scorePoor->setStyleSheet("color: #e74c3c; font-weight: bold; font-size: 10px;");

    scoreDistLayout->addWidget(scoreExcellent);
    scoreDistLayout->addWidget(scoreGood);
    scoreDistLayout->addWidget(scoreFair);
    scoreDistLayout->addWidget(scorePoor);
    scoreDistLayout->addStretch(1);

    // Component scores breakdown (horizontal bar)
    QFrame* componentScoresFrame = new QFrame;
    componentScoresFrame->setStyleSheet("QFrame{ background: rgba(255,255,255,0.70); border: 1px solid rgba(0,0,0,0.08); border-radius: 8px; }");
    QVBoxLayout* componentScoresLayout = new QVBoxLayout(componentScoresFrame);
    componentScoresLayout->setContentsMargins(10,8,10,8);
    componentScoresLayout->setSpacing(6);

    QLabel* compLabel = new QLabel("Average Component Scores:");
    compLabel->setStyleSheet("color: rgba(0,0,0,0.65); font-weight: bold; font-size: 10px;");
    componentScoresLayout->addWidget(compLabel);

    QLabel* compCompleteness = new QLabel("Completeness: 0%");
    compCompleteness->setStyleSheet("color: rgba(0,0,0,0.70); font-size: 9px;");
    QLabel* compCitations = new QLabel("Citations: 0%");
    compCitations->setStyleSheet("color: rgba(0,0,0,0.70); font-size: 9px;");
    QLabel* compImpact = new QLabel("Impact Factor: 0%");
    compImpact->setStyleSheet("color: rgba(0,0,0,0.70); font-size: 9px;");
    QLabel* compRecency = new QLabel("Recency: 0%");
    compRecency->setStyleSheet("color: rgba(0,0,0,0.70); font-size: 9px;");
    QLabel* compProjectLink = new QLabel("Project Linkage: 0%");
    compProjectLink->setStyleSheet("color: rgba(0,0,0,0.70); font-size: 9px;");
    QLabel* compDuplication = new QLabel("Duplication Risk: 0%");
    compDuplication->setStyleSheet("color: rgba(0,0,0,0.70); font-size: 9px;");

    componentScoresLayout->addWidget(compCompleteness);
    componentScoresLayout->addWidget(compCitations);
    componentScoresLayout->addWidget(compImpact);
    componentScoresLayout->addWidget(compRecency);
    componentScoresLayout->addWidget(compProjectLink);
    componentScoresLayout->addWidget(compDuplication);

    // Next best actions
    QFrame* actionsFrame = new QFrame;
    actionsFrame->setStyleSheet("QFrame{ background: rgba(255,255,255,0.70); border: 1px solid rgba(0,0,0,0.08); border-radius: 8px; }");
    QVBoxLayout* actionsLayout = new QVBoxLayout(actionsFrame);
    actionsLayout->setContentsMargins(10,8,10,8);
    actionsLayout->setSpacing(6);

    QLabel* actionsLabel = new QLabel("Top Next Best Actions:");
    actionsLabel->setStyleSheet("color: rgba(0,0,0,0.65); font-weight: bold; font-size: 10px;");
    actionsLayout->addWidget(actionsLabel);

    QListWidget* actionsList = new QListWidget;
    actionsList->setMaximumHeight(80);
    actionsList->setStyleSheet(
        "QListWidget { border: none; background: transparent; }"
        "QListWidget::item { padding: 4px; border-left: 3px solid #3498db; padding-left: 8px; }"
        "QListWidget::item:hover { background: rgba(52, 152, 219, 0.1); }"
    );
    actionsList->setIconSize(QSize(16, 16));
    actionsLayout->addWidget(actionsList);

    healthCoachLayout->addWidget(healthCoachTitle);
    healthCoachLayout->addWidget(scoreDistFrame);
    healthCoachLayout->addWidget(componentScoresFrame);
    healthCoachLayout->addWidget(actionsFrame);
    healthCoachLayout->addStretch(1);

    outPUB3L->addWidget(healthCoachFrame, 1);

    // Bottom : Retour
    QFrame* pub3Bottom = new QFrame;
    pub3Bottom->setFixedHeight(64);
    pub3Bottom->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* pub3BottomL = new QHBoxLayout(pub3Bottom);
    pub3BottomL->setContentsMargins(14,10,14,10);

    QPushButton* pub3Back = actionBtn("Retour", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_ArrowBack), true);
    pub3BottomL->addWidget(pub3Back);
    pub3BottomL->addStretch(1);

    outPUB3L->addWidget(pub3Bottom);

    QScrollArea* pub3Scroll = new QScrollArea;
    pub3Scroll->setObjectName("pub3Scroll");
    pub3Scroll->setWidgetResizable(true);
    pub3Scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    pub3Scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    pub3Scroll->setFrameShape(QFrame::NoFrame);
    pub3Scroll->setStyleSheet(R"(
        QScrollArea#pub3Scroll {
            background: transparent;
            border: none;
        }
        QScrollArea#pub3Scroll > QWidget > QWidget {
            background: transparent;
        }
        QScrollBar:vertical {
            background: rgba(0,0,0,0.06);
            width: 6px;
            border-radius: 3px;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background: rgba(45,212,191,0.50);
            border-radius: 3px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover {
            background: rgba(45,212,191,0.80);
        }
        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical { height: 0px; }
        QScrollBar::add-page:vertical,
        QScrollBar::sub-page:vertical { background: none; }
    )");
    pub3Scroll->setWidget(outPUB3);

    pb3->addWidget(pub3Scroll, 1);

    addStackPage(pub3);

    auto refreshPublicationLegend = [=](const QList<DonutChart::Slice>& slices){
        while (QLayoutItem* item = piePUBLegendL->takeAt(0)) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }

        if (slices.isEmpty()) {
            QLabel* emptyLegend = new QLabel("Légende indisponible");
            emptyLegend->setStyleSheet("color: rgba(0,0,0,0.45); font-size: 10px;");
            piePUBLegendL->addWidget(emptyLegend, 0, 0, 1, 2);
            return;
        }

        double total = 0.0;
        for (const auto& s : slices) total += s.value;
        if (total <= 0.0) total = 1.0;

        int row = 0;
        for (const auto& s : slices) {
            QWidget* swatch = new QWidget;
            swatch->setFixedSize(12, 12);
            swatch->setStyleSheet(QString("background:%1; border:1px solid rgba(0,0,0,0.16); border-radius:6px;")
                                  .arg(s.color.name()));

            const int pct = static_cast<int>(std::round((s.value / total) * 100.0));
            QLabel* keyLabel = new QLabel(QString("%1 : %2 (%3%)")
                                          .arg(s.label)
                                          .arg(static_cast<int>(s.value))
                                          .arg(pct));
            keyLabel->setStyleSheet("color: rgba(0,0,0,0.62); font-size: 10px;");

            piePUBLegendL->addWidget(swatch, row, 0, Qt::AlignVCenter);
            piePUBLegendL->addWidget(keyLabel, row, 1);
            ++row;
        }

        piePUBLegendL->setColumnStretch(1, 1);
    };

    auto updatePublicationStats = [=](){
        QString errorMessage;
        QSqlQueryModel* model = Publication::readAll(nullptr, &errorMessage);
        if (!model) {
            donutPUB->setData({});
            refreshPublicationLegend({});
            barsPUB->setData({});
            return;
        }

        QMap<QString, int> statusCounts;
        QMap<int, int> yearCounts;
        QList<Publication> publications;

        // Collect publications and their counts
        for (int r = 0; r < model->rowCount(); ++r) {
            const QString status = model->data(model->index(r, 5)).toString().trimmed();
            const int year = model->data(model->index(r, 3)).toInt();
            if (!status.isEmpty()) statusCounts[status] += 1;
            if (year > 0) yearCounts[year] += 1;

            // Get full publication data for scoring
            int pubId = model->data(model->index(r, 0)).toInt();
            Publication pub;
            if (Publication::readById(pubId, pub, &errorMessage)) {
                publications.append(pub);
            }
        }
        delete model;

        // Update standard charts
        QList<DonutChart::Slice> slices;
        auto colorForStatus = [&](const QString& s) -> QColor {
            const QString low = s.toLower();
            if (low.contains("publi")) return W_GREEN;
            if (low.contains("accept")) return W_ORANGE;
            if (low.contains("soumis")) return QColor("#9FBEB9");
            if (low.contains("rejet")) return W_RED;
            if (low.contains("brouillon")) return W_GRAY;
            return QColor("#3A7CA5");
        };

        for (auto it = statusCounts.constBegin(); it != statusCounts.constEnd(); ++it) {
            slices.push_back({(double)it.value(), colorForStatus(it.key()), it.key()});
        }
        donutPUB->setData(slices);
        refreshPublicationLegend(slices);

        QList<BarChart::Bar> bars;
        for (auto it = yearCounts.constBegin(); it != yearCounts.constEnd(); ++it) {
            bars.push_back({(double)it.value(), QString::number(it.key())});
        }
        barsPUB->setData(bars);

        // Calculate health scores
        int excellentCount = 0, goodCount = 0, fairCount = 0, poorCount = 0;
        double avgCompleteness = 0, avgCitations = 0, avgImpact = 0, avgRecency = 0, avgProjectLink = 0, avgDuplication = 0;

        QMap<int, PublicationScorer::NextAction> allActions;

        for (const Publication& pub : publications) {
            PublicationScorer::ScoreBreakdown scores = PublicationScorer::calculateScores(pub);

            // Count health scores
            const QString status = PublicationScorer::getHealthStatus(scores.totalHealthScore);
            if (status == "Excellent") excellentCount++;
            else if (status == "Good") goodCount++;
            else if (status == "Fair") fairCount++;
            else poorCount++;

            // Accumulate component scores
            avgCompleteness += scores.completenessScore;
            avgCitations += scores.citationScore;
            avgImpact += scores.impactFactorScore;
            avgRecency += scores.recencyScore;
            avgProjectLink += scores.projectLinkageScore;
            avgDuplication += scores.duplicationRiskScore;

            // Collect next actions
            QList<PublicationScorer::NextAction> actions = PublicationScorer::getNextActions(pub);
            for (const auto& action : actions) {
                if (!allActions.contains(action.priority * 1000 + pub.id())) {
                    allActions.insert(action.priority * 1000 + pub.id(), action);
                }
            }
        }

        // Calculate averages
        int pubCount = publications.size();
        if (pubCount > 0) {
            avgCompleteness /= pubCount;
            avgCitations /= pubCount;
            avgImpact /= pubCount;
            avgRecency /= pubCount;
            avgProjectLink /= pubCount;
            avgDuplication /= pubCount;
        }

        // Update score distribution labels
        scoreExcellent->setText(QString("Excellent: %1").arg(excellentCount));
        scoreGood->setText(QString("Good: %1").arg(goodCount));
        scoreFair->setText(QString("Fair: %1").arg(fairCount));
        scorePoor->setText(QString("Poor: %1").arg(poorCount));

        // Update component scores
        compCompleteness->setText(QString("Completeness: %1%").arg(static_cast<int>(avgCompleteness)));
        compCitations->setText(QString("Citations: %1%").arg(static_cast<int>(avgCitations)));
        compImpact->setText(QString("Impact Factor: %1%").arg(static_cast<int>(avgImpact)));
        compRecency->setText(QString("Recency: %1%").arg(static_cast<int>(avgRecency)));
        compProjectLink->setText(QString("Project Linkage: %1%").arg(static_cast<int>(avgProjectLink)));
        compDuplication->setText(QString("Duplication Risk: %1%").arg(static_cast<int>(avgDuplication)));

        // Update next actions list (top 5)
        actionsList->clear();
        int actionCount = 0;
        for (auto it = allActions.constBegin(); it != allActions.constEnd() && actionCount < 5; ++it, ++actionCount) {
            const auto& action = it.value();
            QString itemText = QString("%1 • %2").arg(action.action, action.reason);
            QListWidgetItem* item = new QListWidgetItem(itemText);
            item->setToolTip(action.reason);

            // Set priority color
            QColor priorityColor;
            if (action.priority <= 1) priorityColor = W_RED;
            else if (action.priority == 2) priorityColor = W_ORANGE;
            else if (action.priority == 3) priorityColor = QColor("#f39c12");
            else priorityColor = QColor("#3498db");

            item->setForeground(priorityColor);
            actionsList->addItem(item);
        }
    };

    QObject::connect(stack, &QStackedWidget::currentChanged, pub3, [=](int idx){
        if (idx == PUB_STATS) updatePublicationStats();
    });

    // ==========================================================
    // ======================  EQUIPEMENTS  =====================
    // ==========================================================

    // ==========================================================
    // PAGE 14 : Équipements - LISTE (EQUIP_LIST)
    // ==========================================================
    QWidget* equip1 = new QWidget;
    QVBoxLayout* eq1 = new QVBoxLayout(equip1);
    eq1->setContentsMargins(22, 18, 22, 18);
    eq1->setSpacing(14);

    ModulesBar barEquipList;
    eq1->addWidget(makeHeaderBlock(st, "Équipements", ModuleTab::Equipement, &barEquipList));
    connectModulesSwitch(this, stack, barEquipList);

    QFrame* eqBar = new QFrame;
    eqBar->setFixedHeight(54);
    eqBar->setStyleSheet("background: rgba(255,255,255,0.22); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* eqBarL = new QHBoxLayout(eqBar);
    eqBarL->setContentsMargins(14, 8, 14, 8);
    eqBarL->setSpacing(10);

    QLineEdit* eqSearch = new QLineEdit;
    eqSearch->setPlaceholderText("Rechercher (nom / fabricant / modele)");
    eqSearch->addAction(st->standardIcon(QStyle::SP_FileDialogContentsView), QLineEdit::LeadingPosition);

    QComboBox* cbEquipType = new QComboBox;
    cbEquipType->addItems({"Type équipement", "Équipement", "Médicament", "Instrument", "Consommable"});

    QComboBox* cbEquipStatus = new QComboBox;
    cbEquipStatus->addItems({"Statut", "Actif", "Hors service", "Archivé"});

    QComboBox* cbEquipLoc = new QComboBox;
    cbEquipLoc->addItems({"Localisation", "Lab 101", "Lab 102", "Lab 103", "Lab 201"});

    QPushButton* eqSortDate = new QPushButton;
    eqSortDate->setCursor(Qt::PointingHandCursor);
    eqSortDate->setStyleSheet(QString(R"(
        QPushButton{
            background:%1; color: rgba(255,255,255,0.92);
            border:1px solid rgba(0,0,0,0.18);
            border-radius: 12px; padding: 10px 16px; font-weight: 800;
        }
        QPushButton:hover{ background: %2; }
    )").arg(C_PRIMARY, C_TOPBAR));
    bool* eqSortNewestFirst = new bool(true);
    auto refreshEqSortButton = [=](){
        eqSortDate->setIcon(st->standardIcon(*eqSortNewestFirst ? QStyle::SP_ArrowDown
                                                                : QStyle::SP_ArrowUp));
        eqSortDate->setText(*eqSortNewestFirst ? "  Tri date recent"
                                               : "  Tri date ancien");
        eqSortDate->setToolTip(*eqSortNewestFirst
                               ? "Tri par date d'achat : plus recent au plus ancien"
                               : "Tri par date d'achat : plus ancien au plus recent");
    };
    refreshEqSortButton();

    eqBarL->addWidget(eqSearch, 1);
    eqBarL->addWidget(cbEquipType);
    eqBarL->addWidget(cbEquipStatus);
    eqBarL->addWidget(cbEquipLoc);
    eqBarL->addWidget(eqSortDate);
    eq1->addWidget(eqBar);

    QFrame* eqCard = makeCard();
    QVBoxLayout* eqCardL = new QVBoxLayout(eqCard);
    eqCardL->setContentsMargins(10,10,10,10);

    EquipementCrud* eqCrud     = new EquipementCrud;
    bool*           eqEditMode = new bool(false);
    int*            eqEditId   = new int(0);

    QTableWidget* eqTable = new QTableWidget(0, 9);
    eqTable->setHorizontalHeaderLabels({"", "Nom", "Fabricant", "Modèle",
                                         "Localisation", "Date achat", "Prochaine maintenance", "Statut", "Calibration"});
    eqTable->verticalHeader()->setVisible(false);
    eqTable->setShowGrid(true);
    eqTable->setAlternatingRowColors(true);
    eqTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    eqTable->setSelectionMode(QAbstractItemView::NoSelection);
    eqTable->horizontalHeader()->setStretchLastSection(true);
    eqTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    eqTable->setItemDelegateForColumn(7, new StatusBadgeDelegate(eqTable));

    eqTable->setColumnWidth(0, 36);
    eqTable->setColumnWidth(1, 150);
    eqTable->setColumnWidth(2, 130);
    eqTable->setColumnWidth(3, 110);
    eqTable->setColumnWidth(4, 110);
    eqTable->setColumnWidth(5, 120);
    eqTable->setColumnWidth(6, 150);
    eqTable->setColumnWidth(7, 140);
    eqTable->setColumnWidth(8, 120);

    QTimer* eqAlertBlinkTimer = new QTimer(eqTable);
    eqAlertBlinkTimer->setInterval(520);
    bool* eqAlertBlinkOn = new bool(false);

    auto equipmentBadgeFromDbStatus = [](const QString& dbStatus) {
        const QString s = dbStatus.toLower();
        if (s.contains("hors") || s.contains("service")) return EquipmentStatus::OutOfOrder;
        if (s.contains("rchiv") || s.contains("archive")) return EquipmentStatus::UnderMaintenance;
        return EquipmentStatus::Available;
    };

    auto equipmentTypeFromName = [](const QString& name, const QString& model) {
        const QString full = (name + " " + model).toLower();
        if (full.contains("med") || full.contains("drug") || full.contains("pharma") || full.contains("reagent")) {
            return QString("Médicament");
        }
        if (full.contains("pcr") || full.contains("centrif") || full.contains("micro") || full.contains("incubat")
            || full.contains("spectro") || full.contains("analyseur")) {
            return QString("Instrument");
        }
        if (full.contains("tube") || full.contains("gants") || full.contains("masque") || full.contains("consommable")) {
            return QString("Consommable");
        }
        return QString("Équipement");
    };

    auto applyEqAlertRowStyles = [=]() {
        for (int row = 0; row < eqTable->rowCount(); ++row) {
            QTableWidgetItem* keyItem = eqTable->item(row, 1);
            if (!keyItem) continue;

            const int alertLevel = keyItem->data(Qt::UserRole + 1).toInt();
            QColor bgColor;
            QColor fgColor;

            if (alertLevel >= 2) {
                bgColor = *eqAlertBlinkOn ? QColor(214, 48, 49) : QColor(255, 230, 230);
                fgColor = *eqAlertBlinkOn ? QColor(255, 255, 255) : QColor(105, 12, 18);
            } else if (alertLevel == 1) {
                bgColor = QColor(255, 248, 210);
                fgColor = QColor(98, 74, 0);
            } else {
                continue;
            }

            for (int c = 0; c < eqTable->columnCount(); ++c) {
                if (QTableWidgetItem* item = eqTable->item(row, c)) {
                    item->setBackground(bgColor);
                    if (c != 7) item->setForeground(fgColor);
                }
            }
        }
        eqTable->viewport()->update();
    };
    QObject::connect(eqAlertBlinkTimer, &QTimer::timeout, eqTable, [=]() {
        *eqAlertBlinkOn = !*eqAlertBlinkOn;
        applyEqAlertRowStyles();
    });

    auto loadEqTable = [=](){
        syncFinishedExperienceEquipments();
        eqTable->setRowCount(0);
        QList<EquipementRecord> recs;
        QString err;
        const QString searchFilter = eqSearch->text().trimmed();
        const QString typeFilter = (cbEquipType->currentIndex() <= 0) ? QString() : cbEquipType->currentText();
        const QString nomFilter = QString();
        const QString statusFilter = (cbEquipStatus->currentIndex() <= 0) ? QString() : cbEquipStatus->currentText();
        const QString locFilter    = (cbEquipLoc->currentIndex() <= 0) ? QString() : cbEquipLoc->currentText();
        if (!eqCrud->loadEquipements(recs, &err, searchFilter, nomFilter, statusFilter, locFilter)) {
            showToast(this, "Erreur : " + err, false);
            return;
        }

        std::stable_sort(recs.begin(), recs.end(), [=](const EquipementRecord& a, const EquipementRecord& b){
            const bool aValid = a.dateAchat.isValid();
            const bool bValid = b.dateAchat.isValid();
            if (aValid != bValid) return aValid > bValid;
            if (!aValid && !bValid)
                return QString::localeAwareCompare(a.nomEquipement, b.nomEquipement) < 0;
            if (a.dateAchat != b.dateAchat)
                return *eqSortNewestFirst ? (a.dateAchat > b.dateAchat)
                                          : (a.dateAchat < b.dateAchat);
            return QString::localeAwareCompare(a.nomEquipement, b.nomEquipement) < 0;
        });

        auto mk = [](const QString& t){
            QTableWidgetItem* it = new QTableWidgetItem(t);
            it->setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);
            return it;
        };

        bool hasOverdueAlert = false;
        for (const EquipementRecord& rec : recs) {
            if (!typeFilter.isEmpty()) {
                const QString detectedType = equipmentTypeFromName(rec.nomEquipement, rec.numeroModele);
                if (detectedType != typeFilter) continue;
            }

            int row = eqTable->rowCount();
            eqTable->insertRow(row);

            QTableWidgetItem* iconItem = new QTableWidgetItem;
            iconItem->setIcon(style()->standardIcon(QStyle::SP_ArrowRight));
            iconItem->setTextAlignment(Qt::AlignCenter);
            eqTable->setItem(row, 0, iconItem);

            QTableWidgetItem* nameItem = mk(rec.nomEquipement);
            nameItem->setData(Qt::UserRole, rec.id);
            eqTable->setItem(row, 1, nameItem);
            eqTable->setItem(row, 2, mk(rec.fabricant));
            eqTable->setItem(row, 3, mk(rec.numeroModele));
            eqTable->setItem(row, 4, mk(rec.localisation));
            eqTable->setItem(row, 5, mk(rec.dateAchat.isValid() ? rec.dateAchat.toString("dd/MM/yyyy") : ""));
            eqTable->setItem(row, 6, mk(rec.dateProchaineMaintenance.isValid() ? rec.dateProchaineMaintenance.toString("dd/MM/yyyy") : ""));

            QTableWidgetItem* badge = new QTableWidgetItem;
            badge->setData(Qt::UserRole, (int)equipmentBadgeFromDbStatus(rec.statut));
            eqTable->setItem(row, 7, badge);

            eqTable->setItem(row, 8, mk(rec.dateLimiteCalibration.isValid() ? rec.dateLimiteCalibration.toString("dd/MM/yyyy") : ""));
            eqTable->setRowHeight(row, 46);

            // Colorer la ligne si maintenance dépassée ou imminente
            int alertLevel = 0;
            if (rec.dateProchaineMaintenance.isValid()) {
                const int daysLeft = QDate::currentDate().daysTo(rec.dateProchaineMaintenance);
                QColor rowColor;
                if (daysLeft <= 0) {
                    rowColor = QColor(255, 230, 230); // rouge clair — dépassé
                    alertLevel = 2;
                    hasOverdueAlert = true;
                } else if (daysLeft <= 7) {
                    rowColor = QColor(255, 248, 210); // jaune clair — imminent
                    alertLevel = 1;
                }
                if (rowColor.isValid()) {
                    for (int c = 0; c < eqTable->columnCount(); ++c)
                        if (eqTable->item(row, c))
                            eqTable->item(row, c)->setBackground(rowColor);
                }
            }
            nameItem->setData(Qt::UserRole + 1, alertLevel);
        }

        *eqAlertBlinkOn = false;
        applyEqAlertRowStyles();
        if (hasOverdueAlert) eqAlertBlinkTimer->start();
        else eqAlertBlinkTimer->stop();

        // Alerte sonore + toast si maintenance dépassée ou dans 7 jours
        QStringList overdueNames, soonNames;
        for (const EquipementRecord& rec : recs) {
            if (!rec.dateProchaineMaintenance.isValid()) continue;
            const int daysLeft = QDate::currentDate().daysTo(rec.dateProchaineMaintenance);
            if (daysLeft <= 0)      overdueNames << rec.nomEquipement;
            else if (daysLeft <= 7) soonNames    << rec.nomEquipement;
        }
        if (!overdueNames.isEmpty()) {
            QApplication::beep(); // alerte sonore
            showToast(this,
                QString("⚠ Maintenance EN RETARD : %1").arg(overdueNames.join(", ")),
                false);
        } else if (!soonNames.isEmpty()) {
            QApplication::beep();
            showToast(this,
                QString("🔔 Maintenance dans ≤7 jours : %1").arg(soonNames.join(", ")),
                false);
        }
    };
    loadEqTable();

    QObject::connect(stack, &QStackedWidget::currentChanged, eqTable, [=](int idx){
        if (idx == EQUIP_LIST) loadEqTable();
    });

    eqCardL->addWidget(eqTable);
    eq1->addWidget(eqCard, 1);

    QObject::connect(eqSearch, &QLineEdit::textChanged, this, [=](const QString&){ loadEqTable(); });
    QObject::connect(cbEquipType, &QComboBox::currentIndexChanged, this, [=](int){ loadEqTable(); });
    QObject::connect(cbEquipStatus, &QComboBox::currentIndexChanged, this, [=](int){ loadEqTable(); });
    QObject::connect(cbEquipLoc, &QComboBox::currentIndexChanged, this, [=](int){ loadEqTable(); });
    QObject::connect(eqSortDate, &QPushButton::clicked, this, [=](){
        *eqSortNewestFirst = !*eqSortNewestFirst;
        refreshEqSortButton();
        loadEqTable();
    });

    QFrame* eqBottom = new QFrame;
    eqBottom->setFixedHeight(64);
    eqBottom->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* eqBottomL = new QHBoxLayout(eqBottom);
    eqBottomL->setContentsMargins(14,10,14,10);
    eqBottomL->setSpacing(12);

    QPushButton* eqAdd   = actionBtn("Ajouter", "rgba(10,95,88,0.45)", "rgba(255,255,255,0.90)", st->standardIcon(QStyle::SP_DialogYesButton), true);
    QPushButton* eqEdit  = actionBtn("Modifier", "rgba(198,178,154,0.55)", "rgba(255,255,255,0.85)", st->standardIcon(QStyle::SP_FileDialogContentsView), true);
    QPushButton* eqDel   = actionBtn("Supprimer", "rgba(255,255,255,0.55)", "#B14A4A", st->standardIcon(QStyle::SP_TrashIcon), true);
    QPushButton* eqDet   = actionBtn("Statistique", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_MessageBoxInformation), true);

    QObject::connect(eqDel, &QPushButton::clicked, this, [=](){
        if (*currentRole != "Responsable") { showToast(this, "Accès refusé : seul le Responsable peut supprimer.", false); return; }
        int r = eqTable->currentRow();
        if (r < 0) {
            QMessageBox::information(this, "Information", "Veuillez sélectionner un équipement à supprimer.");
            return;
        }
        const int id = eqTable->item(r,1)->data(Qt::UserRole).toInt();
        QString resume = QString("Équipement : %1 | Localisation : %2")
                            .arg(eqTable->item(r,1)->text(),
                                 eqTable->item(r,4)->text());
        ConfirmDeleteDialog confirm(style(), resume, this);
        if (confirm.exec() != QDialog::Accepted) return;
        QString err;
        if (!eqCrud->deleteEquipement(id, &err)) {
            showToast(this, "Erreur : " + err, false);
            return;
        }
        showToast(this, "Équipement supprimé.", true);
        loadEqTable();
    });

    eqBottomL->addWidget(eqAdd);
    eqBottomL->addWidget(eqEdit);
    eqBottomL->addWidget(eqDel);
    eqBottomL->addWidget(eqDet);

    QPushButton* eqExportPdf = actionBtn("Exporter PDF", "rgba(198,178,154,0.55)", "rgba(255,255,255,0.85)", st->standardIcon(QStyle::SP_FileDialogDetailedView), true);
    eqBottomL->addWidget(eqExportPdf);

    eqBottomL->addStretch(1);

    eqBottomL->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_DirIcon)));
    eqBottomL->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_FileIcon)));
    eqBottomL->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_DialogSaveButton)));
    eqBottomL->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_BrowserReload)));

    QPushButton* eqMore = new QPushButton(st->standardIcon(QStyle::SP_FileDialogContentsView), "  Statistiques");
    eqMore->setCursor(Qt::PointingHandCursor);
    eqMore->setStyleSheet(R"(
        QPushButton{
            background: rgba(255,255,255,0.55);
            border: 1px solid rgba(0,0,0,0.12);
            border-radius: 12px;
            padding: 10px 14px;
            color: rgba(0,0,0,0.65);
            font-weight: 800;
        }
        QPushButton:hover{ background: rgba(255,255,255,0.75); }
    )");
    eqBottomL->addWidget(eqMore);

    eq1->addWidget(eqBottom);
    addStackPage(equip1);

    // ==========================================================
    // PAGE 15 : Équipements - AJOUT / MODIF (EQUIP_FORM)
    // ==========================================================
    QWidget* equip2 = new QWidget;
    equip2->setObjectName("equipFormPage");
    QVBoxLayout* eq2 = new QVBoxLayout(equip2);
    eq2->setContentsMargins(22, 18, 22, 18);
    eq2->setSpacing(14);

    ModulesBar barEquipForm;
    eq2->addWidget(makeHeaderBlock(st, "Ajouter / Modifier un équipement", ModuleTab::Equipement, &barEquipForm));
    connectModulesSwitch(this, stack, barEquipForm);

    QFrame* eqOuter2 = new QFrame;
    eqOuter2->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }").arg(C_PANEL_BG, C_PANEL_BR));
    QHBoxLayout* eqOuter2L = new QHBoxLayout(eqOuter2);
    eqOuter2L->setContentsMargins(12,12,12,12);
    eqOuter2L->setSpacing(12);

    QFrame* eqLeft2 = softBox();
    eqLeft2->setFixedWidth(300);
    QVBoxLayout* eqLeft2L = new QVBoxLayout(eqLeft2);
    eqLeft2L->setContentsMargins(10,10,10,10);
    eqLeft2L->setSpacing(16);

    auto leftAction = [&](const QString& title, QStyle::StandardPixmap sp, const QString& text){
        QLabel* head = new QLabel(title);
        head->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
        QToolButton* b = new QToolButton;
        b->setIcon(st->standardIcon(sp));
        b->setText("  " + text);
        b->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(R"(
            QToolButton{
                background: rgba(255,255,255,0.70);
                border: 1px solid rgba(0,0,0,0.12);
                border-radius: 12px;
                padding: 10px 12px;
                text-align: left;
                color: rgba(0,0,0,0.60);
                font-weight: 800;
            }
            QToolButton:hover{ background: rgba(255,255,255,0.85); }
        )");
        eqLeft2L->addWidget(head);
        eqLeft2L->addWidget(b);
        return b;
    };

    QToolButton* eqTypeSummary = leftAction("Nom d’équipement", QStyle::SP_FileIcon, "Nom d'équipement");
    QToolButton* eqFabSummary  = leftAction("Fabricant", QStyle::SP_DirIcon, "Fabricant");

    QLabel* locHead = new QLabel("Localisation");
    locHead->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    eqLeft2L->addWidget(locHead);

    auto colBtn = [&](QStyle::StandardPixmap sp, const QString& txt){
        QToolButton* b = new QToolButton;
        b->setIcon(st->standardIcon(sp));
        b->setText("  " + txt);
        b->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(R"(
            QToolButton{
                background: rgba(255,255,255,0.70);
                border: 1px solid rgba(0,0,0,0.12);
                border-radius: 12px;
                padding: 10px 12px;
                text-align: left;
                color: rgba(0,0,0,0.60);
                font-weight: 800;
            }
            QToolButton:hover{ background: rgba(255,255,255,0.85); }
        )");
        return b;
    };

    QToolButton* eqSalleSummary = colBtn(QStyle::SP_DriveHDIcon, "Salle : Salle");
    eqLeft2L->addWidget(eqSalleSummary);
    eqLeft2L->addWidget(colBtn(QStyle::SP_FileDialogListView, "Bâtiment"));
    eqLeft2L->addWidget(colBtn(QStyle::SP_ArrowDown, "Étage"));
    eqLeft2L->addStretch(1);

    QFrame* eqRight2 = softBox();
    QVBoxLayout* eqRight2L = new QVBoxLayout(eqRight2);
    eqRight2L->setContentsMargins(12,12,12,12);
    eqRight2L->setSpacing(16);

    QFrame* eqTinyTop = softBox();
    QHBoxLayout* eqTinyTopL = new QHBoxLayout(eqTinyTop);
    eqTinyTopL->setContentsMargins(12,8,12,8);

    QToolButton* addDrop = new QToolButton;
    addDrop->setIcon(st->standardIcon(QStyle::SP_DialogYesButton));
    addDrop->setText("Ajouter équipement");
    addDrop->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    addDrop->setStyleSheet("QToolButton{ color: rgba(0,0,0,0.55); font-weight: 900; }");

    eqTinyTopL->addWidget(addDrop);
    eqTinyTopL->addStretch(1);
    eqRight2L->addWidget(eqTinyTop);

    auto comboRow = [&](QComboBox* cb){
        QFrame* r = softBox();
        r->setMinimumHeight(64);
        QHBoxLayout* l = new QHBoxLayout(r);
        l->setContentsMargins(12,14,12,14);
        cb->setMinimumHeight(44);
        cb->setStyleSheet(
            "QComboBox{ background: rgba(255,255,255,0.96); color: rgba(0,0,0,0.82);"
            " border:1px solid rgba(0,0,0,0.20); border-radius:8px; padding:6px 10px; font-weight:700; }"
            "QComboBox::drop-down{ border:none; width:22px; }"
            "QComboBox QAbstractItemView{ background:white; color:black; selection-background-color: rgba(10,95,88,0.20); }"
        );
        if (cb->isEditable() && cb->lineEdit()) {
            cb->lineEdit()->setStyleSheet("background: transparent; color: rgba(0,0,0,0.84); border:0; font-weight:700;");
        }
        l->addWidget(cb);
        return r;
    };

    auto lineRow = [&](QLineEdit* le){
        QFrame* r = softBox();
        r->setMinimumHeight(64);
        QHBoxLayout* l = new QHBoxLayout(r);
        l->setContentsMargins(12,14,12,14);
        le->setMinimumHeight(44);
        le->setStyleSheet(
            "QLineEdit{ background: rgba(255,255,255,0.96); color: rgba(0,0,0,0.82);"
            " border:1px solid rgba(0,0,0,0.20); border-radius:8px; padding:6px 10px; font-weight:700; }"
            "QLineEdit::placeholder{ color: rgba(0,0,0,0.45); }");
        l->addWidget(le);
        return r;
    };

    auto sectionHint = [&](const QString& text){
        QLabel* l = new QLabel(text);
        l->setStyleSheet("color: rgba(0,0,0,0.78); font-weight: 900; font-size: 13px; padding-left: 2px;");
        return l;
    };

    QLineEdit* fcb1 = new QLineEdit;
    QLineEdit* fcb2 = new QLineEdit;
    QComboBox* fcb3 = new QComboBox; fcb3->addItems({"Actif","Hors service","Archivé"});
    QComboBox* fcbType = new QComboBox; fcbType->addItems({"Type équipement", "Équipement", "Médicament", "Instrument", "Consommable"});
    fcb1->setText("");
    fcb2->setText("");
    fcb1->setPlaceholderText("Nom d'équipement");
    fcb2->setPlaceholderText("Fabricant");
    const QRegularExpression eqInputRegex(QStringLiteral("^[\\p{L}\\p{N}\\s'\\-_.()/]*$"));
    fcb1->setValidator(new QRegularExpressionValidator(eqInputRegex, fcb1));
    fcb2->setValidator(new QRegularExpressionValidator(eqInputRegex, fcb2));

    QScrollArea* eqFormScroll = new QScrollArea;
    eqFormScroll->setWidgetResizable(true);
    eqFormScroll->setFrameShape(QFrame::NoFrame);
    eqFormScroll->setStyleSheet("QScrollArea{ background: transparent; border: 0; }");

    QWidget* eqFormContent = new QWidget;
    QGridLayout* eqFormGrid = new QGridLayout(eqFormContent);
    eqFormGrid->setContentsMargins(0, 2, 0, 2);
    eqFormGrid->setHorizontalSpacing(14);
    eqFormGrid->setVerticalSpacing(10);
    eqFormGrid->setColumnStretch(0, 1);
    eqFormGrid->setColumnStretch(1, 1);

    auto fieldBlock = [&](const QString& title, QWidget* inputRow){
        QWidget* block = new QWidget;
        QVBoxLayout* blockL = new QVBoxLayout(block);
        blockL->setContentsMargins(0,0,0,0);
        blockL->setSpacing(4);
        blockL->addWidget(sectionHint(title));
        blockL->addWidget(inputRow);
        return block;
    };

    QFrame* modelRow = softBox();
    QHBoxLayout* modelL = new QHBoxLayout(modelRow);
    modelL->setContentsMargins(10,8,10,8);
    QLabel* modelLabel = new QLabel("Modèle :");
    modelLabel->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    QLineEdit* modelEdit = new QLineEdit;
    modelEdit->setPlaceholderText("ex: TX-500");
    modelEdit->setStyleSheet("background: transparent; border:0; font-weight: 900; color: rgba(0,0,0,0.55);");
    modelL->addWidget(modelLabel);
    modelL->addWidget(modelEdit, 1);
    eqRight2L->addWidget(modelRow);

    QFrame* dateRow = softBox();
    QHBoxLayout* dateRowL = new QHBoxLayout(dateRow);
    dateRowL->setContentsMargins(10,8,10,8);
    dateRowL->setSpacing(8);

    QToolButton* cal = new QToolButton; cal->setAutoRaise(true); cal->setIcon(st->standardIcon(QStyle::SP_FileDialogDetailedView));
    QLabel* dateLabel = new QLabel("Date d'achat :");
    dateLabel->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    QDateEdit* date = new QDateEdit(QDate(2024,1,15));
    date->setCalendarPopup(true);
    date->setDisplayFormat("dd/MM/yyyy");
    date->setStyleSheet("QDateEdit{ background: transparent; border:0; font-weight: 900; color: rgba(0,0,0,0.55);} ");

    dateRowL->addWidget(cal);
    dateRowL->addWidget(dateLabel);
    dateRowL->addWidget(date, 1);
    eqRight2L->addWidget(dateRow);

    // ── Dernière maintenance + intervalle ───────────────────────────
    QFrame* lastMaintRow = softBox();
    QHBoxLayout* lastMaintL = new QHBoxLayout(lastMaintRow);
    lastMaintL->setContentsMargins(10,8,10,8);
    lastMaintL->setSpacing(8);
    QToolButton* cal2b = new QToolButton; cal2b->setAutoRaise(true); cal2b->setIcon(st->standardIcon(QStyle::SP_FileDialogDetailedView));
    QLabel* lastMaintLabel = new QLabel("Dernière maintenance :");
    lastMaintLabel->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    QDateEdit* lastMaintDate = new QDateEdit(QDate::currentDate());
    lastMaintDate->setCalendarPopup(true);
    lastMaintDate->setDisplayFormat("dd/MM/yyyy");
    lastMaintDate->setStyleSheet("QDateEdit{ background: transparent; border:0; font-weight: 900; color: rgba(0,0,0,0.55);} ");
    lastMaintL->addWidget(cal2b);
    lastMaintL->addWidget(lastMaintLabel);
    lastMaintL->addWidget(lastMaintDate, 1);
    eqRight2L->addWidget(lastMaintRow);

    QFrame* intervalRow = softBox();
    QHBoxLayout* intervalL = new QHBoxLayout(intervalRow);
    intervalL->setContentsMargins(10,8,10,8);
    intervalL->setSpacing(8);
    QLabel* intervalLabel = new QLabel("Intervalle maintenance :");
    intervalLabel->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    QComboBox* intervalCb = new QComboBox;
    intervalCb->addItem("30 jours", QString("days:30"));
    intervalCb->addItem("3 mois",   QString("months:3"));
    intervalCb->addItem("6 mois",   QString("months:6"));
    intervalCb->addItem("1 an",     QString("years:1"));
    intervalCb->addItem("2 ans",    QString("years:2"));
    intervalCb->setStyleSheet(
        "QComboBox{ background:rgba(255,255,255,0.96); color:rgba(0,0,0,0.82);"
        " border:1px solid rgba(0,0,0,0.18); border-radius:7px; padding:4px 8px; font-weight:700; }"
        "QComboBox::drop-down{ border:none; width:20px; }"
        "QComboBox QAbstractItemView{ background:white; color:black; selection-background-color:rgba(10,95,88,0.20); }");
    intervalL->addWidget(intervalLabel);
    intervalL->addStretch(1);
    intervalL->addWidget(intervalCb);
    eqRight2L->addWidget(intervalRow);

    // ── Prochaine maintenance (calculée automatiquement) ─────────────
    QFrame* maintRow = softBox();
    QHBoxLayout* maintL = new QHBoxLayout(maintRow);
    maintL->setContentsMargins(10,8,10,8);
    maintL->setSpacing(8);

    QToolButton* cal2 = new QToolButton; cal2->setAutoRaise(true); cal2->setIcon(st->standardIcon(QStyle::SP_FileDialogDetailedView));
    QLabel* maintLabel = new QLabel("Prochaine maintenance :");
    maintLabel->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    QDateEdit* maintDate = new QDateEdit(QDate::currentDate().addDays(30));
    maintDate->setCalendarPopup(true);
    maintDate->setDisplayFormat("dd/MM/yyyy");
    maintDate->setReadOnly(true);
    maintDate->setStyleSheet("QDateEdit{ background: rgba(10,95,88,0.07); border:1px solid rgba(10,95,88,0.20); border-radius:7px; font-weight:900; color:rgba(10,95,88,0.85); padding:4px 8px;} ");

    maintL->addWidget(cal2);
    maintL->addWidget(maintLabel);
    maintL->addWidget(maintDate, 1);
    eqRight2L->addWidget(maintRow);

    bool* isMedMode = new bool(false); // true quand "Médicament" est sélectionné comme type
    bool* keepLoadedDerivedDate = new bool(false);
    QString* eqDerivedAlertKey = new QString;
    auto computeIntervalDate = [=](const QDate& baseDate, const QVariant& intervalData) -> QDate {
        if (!baseDate.isValid()) return QDate();

        const QString spec = intervalData.toString();
        const QStringList parts = spec.split(':');
        if (parts.size() != 2) return baseDate;

        bool ok = false;
        const int amount = parts[1].toInt(&ok);
        if (!ok) return baseDate;

        const QString unit = parts[0].trimmed().toLower();
        if (unit == "days")   return baseDate.addDays(amount);
        if (unit == "months") return baseDate.addMonths(amount);
        if (unit == "years")  return baseDate.addYears(amount);
        return baseDate;
    };
    auto findIntervalIndexForDates = [=](const QDate& from, const QDate& to) -> int {
        if (!from.isValid() || !to.isValid()) return -1;
        for (int i = 0; i < intervalCb->count(); ++i) {
            if (computeIntervalDate(from, intervalCb->itemData(i)) == to) {
                return i;
            }
        }
        return -1;
    };
    auto setDerivedDateStyle = [=](const QDate& targetDate, bool alertUser = true) {
        if (!targetDate.isValid()) {
            maintDate->setStyleSheet(
                "QDateEdit{ background:rgba(255,255,255,0.96); border:1px solid rgba(0,0,0,0.20);"
                " border-radius:7px; font-weight:900; color:rgba(0,0,0,0.82); padding:4px 8px;}");
            eqDerivedAlertKey->clear();
            return;
        }

        const int daysLeft = QDate::currentDate().daysTo(targetDate);
        QString nextAlertKey;
        QString nextAlertText;

        if (daysLeft <= 0) {
            maintDate->setStyleSheet(
                "QDateEdit{ background:rgba(220,38,38,0.10); border:1.5px solid rgba(220,38,38,0.50);"
                " border-radius:7px; font-weight:900; color:rgba(180,0,0,1); padding:4px 8px;}");
            if (*isMedMode) {
                nextAlertKey = QString("med_overdue_%1").arg(targetDate.toString(Qt::ISODate));
                nextAlertText = QString("ALERTE : Ce médicament a expiré il y a %1 jour(s) !").arg(-daysLeft);
            } else {
                nextAlertKey = QString("eq_overdue_%1").arg(targetDate.toString(Qt::ISODate));
                nextAlertText = QString("ALERTE : Maintenance en retard depuis %1 jour(s).").arg(-daysLeft);
            }
        } else if (daysLeft <= 7) {
            maintDate->setStyleSheet(
                "QDateEdit{ background:rgba(234,179,8,0.10); border:1.5px solid rgba(234,179,8,0.55);"
                " border-radius:7px; font-weight:900; color:rgba(146,112,0,1); padding:4px 8px;}");
            if (*isMedMode) {
                nextAlertKey = QString("med_soon_%1").arg(targetDate.toString(Qt::ISODate));
                nextAlertText = QString("ALERTE : Ce médicament expire dans %1 jour(s) !").arg(daysLeft);
            } else {
                nextAlertKey = QString("eq_soon_%1").arg(targetDate.toString(Qt::ISODate));
                nextAlertText = QString("ALERTE : Maintenance dans %1 jour(s).").arg(daysLeft);
            }
        } else {
            maintDate->setStyleSheet(
                "QDateEdit{ background:rgba(10,95,88,0.07); border:1px solid rgba(10,95,88,0.20);"
                " border-radius:7px; font-weight:900; color:rgba(10,95,88,0.85); padding:4px 8px;}");
            eqDerivedAlertKey->clear();
            return;
        }

        if (alertUser && !nextAlertText.isEmpty() && *eqDerivedAlertKey != nextAlertKey) {
            *eqDerivedAlertKey = nextAlertKey;
            QApplication::beep();
            showToast(this, nextAlertText, false);
        } else if (!alertUser) {
            *eqDerivedAlertKey = nextAlertKey;
        }
    };
    auto refreshDerivedEquipDate = [=](bool alertUser = true) {
        if (!*keepLoadedDerivedDate) {
            const QDate baseDate = lastMaintDate->date();
            const QDate derivedDate = computeIntervalDate(baseDate, intervalCb->currentData());
            if (derivedDate.isValid()) {
                maintDate->setDate(derivedDate);
            }
        }
        setDerivedDateStyle(maintDate->date(), alertUser);
    };
    QObject::connect(lastMaintDate, &QDateEdit::dateChanged, lastMaintDate, [=](const QDate&){
        *keepLoadedDerivedDate = false;
        refreshDerivedEquipDate();
    });
    QObject::connect(intervalCb, QOverload<int>::of(&QComboBox::currentIndexChanged), intervalCb, [=](int){
        *keepLoadedDerivedDate = false;
        refreshDerivedEquipDate();
    });
    refreshDerivedEquipDate(false);

    QFrame* calRow = softBox();
    QHBoxLayout* calL = new QHBoxLayout(calRow);
    calL->setContentsMargins(10,8,10,8);
    calL->setSpacing(8);

    QToolButton* cal3 = new QToolButton; cal3->setAutoRaise(true); cal3->setIcon(st->standardIcon(QStyle::SP_FileDialogDetailedView));
    QLabel* calLabel = new QLabel("Calibration :");
    calLabel->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    QDateEdit* calDate = new QDateEdit(QDate(2026,6,15));
    calDate->setCalendarPopup(true);
    calDate->setDisplayFormat("dd/MM/yyyy");
    calDate->setStyleSheet("QDateEdit{ background: transparent; border:0; font-weight: 900; color: rgba(0,0,0,0.55);} ");

    calL->addWidget(cal3);
    calL->addWidget(calLabel);
    calL->addWidget(calDate, 1);
    eqRight2L->addWidget(calRow);

    QLineEdit* labRoom = new QLineEdit;
    labRoom->setMinimumWidth(320);
    labRoom->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    labRoom->setClearButtonEnabled(true);
    labRoom->setPlaceholderText("Salle");
    labRoom->setStyleSheet(
        "QLineEdit{ background:#FFFFFF; color:#0F172A;"
        " border:1px solid rgba(0,0,0,0.28); border-radius:8px; padding:6px 10px; font-weight:900; font-size:14px; }"
        "QLineEdit::placeholder{ color: rgba(0,0,0,0.45); }");

    QListWidget* eqExpCombo = new QListWidget;
    eqExpCombo->setStyleSheet(
        "QListWidget{ background: rgba(255,255,255,0.96); color: rgba(0,0,0,0.82);"
        "  border:1px solid rgba(0,0,0,0.20); border-radius:8px; outline:none; }"
        "QListWidget::item{ padding:7px 10px; font-weight:700; font-size:12px; }"
        "QListWidget::item:selected{ background:rgba(10,95,88,0.18); color:rgba(10,95,88,1); }"
        "QListWidget::item:hover{ background:rgba(10,95,88,0.08); }"
        "QScrollBar:vertical{ background:transparent; width:5px; }"
        "QScrollBar::handle:vertical{ background:rgba(10,95,88,0.30); border-radius:2px; }"
        "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{ height:0; }"
    );
    eqExpCombo->setFixedHeight(130);
    eqExpCombo->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    eqExpCombo->setSelectionMode(QAbstractItemView::NoSelection);
    eqExpCombo->setFocusPolicy(Qt::NoFocus);
    auto reloadEqExperienceChoices = [=](int selectedExpId = -1){
        eqExpCombo->clear();

        QListWidgetItem* none = new QListWidgetItem("— Expérience liée (affichage uniquement) —");
        none->setData(Qt::UserRole, QVariant());
        none->setForeground(QColor(0,0,0,100));
        eqExpCombo->addItem(none);

        QList<ExperienceRecord> expList;
        QString err;
        if (!expCrud->loadExperiences(expList, &err)) {
            if (!err.trimmed().isEmpty())
                showToast(this, "Erreur : " + err, false);
        } else {
            for (const auto& e : expList) {
                QListWidgetItem* it = new QListWidgetItem(
                    QString("[%1]  %2").arg(e.id).arg(e.titre));
                it->setData(Qt::UserRole, e.id);
                eqExpCombo->addItem(it);
            }
        }

        int rowToSelect = -1;
        if (selectedExpId > 0) {
            for (int i = 0; i < eqExpCombo->count(); ++i) {
                if (eqExpCombo->item(i)->data(Qt::UserRole).toInt() == selectedExpId) {
                    rowToSelect = i;
                    break;
                }
            }
        }

        if (rowToSelect < 0) rowToSelect = 0;
        eqExpCombo->setCurrentRow(rowToSelect);
    };
    reloadEqExperienceChoices();

    eqFormGrid->addWidget(fieldBlock("Nom d'équipement", lineRow(fcb1)), 0, 0);
    eqFormGrid->addWidget(fieldBlock("Fabricant", lineRow(fcb2)), 0, 1);
    eqFormGrid->addWidget(fieldBlock("Type d'équipement", comboRow(fcbType)), 1, 0);
    eqFormGrid->addWidget(fieldBlock("Statut", comboRow(fcb3)), 1, 1);
    eqFormGrid->addWidget(fieldBlock("Date d'achat", dateRow), 2, 0);
    eqFormGrid->addWidget(fieldBlock("Modèle", modelRow), 2, 1);
    QWidget* lastMaintBlock = fieldBlock("Dernière maintenance", lastMaintRow);
    QWidget* intervalBlock  = fieldBlock("Intervalle", intervalRow);
    QWidget* maintBlock     = fieldBlock("Prochaine maintenance (auto)", maintRow);
    eqFormGrid->addWidget(lastMaintBlock, 3, 0);
    eqFormGrid->addWidget(fieldBlock("Calibration", calRow), 3, 1);
    eqFormGrid->addWidget(intervalBlock, 4, 1);
    eqFormGrid->addWidget(maintBlock, 5, 0, 1, 2);
    eqFormGrid->addWidget(fieldBlock("Salle", lineRow(labRoom)), 6, 0);
    eqFormGrid->addWidget(fieldBlock("Expérience liée (affichage)", eqExpCombo), 7, 0, 1, 2);
    eqFormGrid->setRowStretch(8, 1);

    eqFormScroll->setWidget(eqFormContent);
    eqRight2L->addWidget(eqFormScroll, 1);

    auto syncEqSidebar = [=](){
        const QString typeTxt = fcb1->text().trimmed();
        const QString fabTxt  = fcb2->text().trimmed();
        const QString locTxt  = labRoom->text().trimmed();
        eqTypeSummary->setText("  " + (typeTxt.isEmpty() ? QString("Nom d'équipement") : typeTxt));
        eqFabSummary->setText("  " + (fabTxt.isEmpty() ? QString("Fabricant") : fabTxt));
        eqSalleSummary->setText("  Salle : " + (locTxt.isEmpty() ? QString("Salle") : locTxt));
    };

    QObject::connect(fcb1, &QLineEdit::textChanged, this, [=](const QString&){ syncEqSidebar(); });
    QObject::connect(fcb2, &QLineEdit::textChanged, this, [=](const QString&){ syncEqSidebar(); });
    QObject::connect(labRoom, &QLineEdit::textChanged, this, [=](const QString&){ syncEqSidebar(); });
    syncEqSidebar();

    // ── Changement de type : mise à jour dynamique de l'interface ────────
    QObject::connect(fcbType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int){
        const bool isMed = (fcbType->currentText() == "Médicament");
        *isMedMode = isMed;

        // Titre extérieur du bloc (section header)
        if (QLabel* lbl = lastMaintBlock->findChild<QLabel*>())
            lbl->setText(isMed ? "Date de fabrication" : "Dernière maintenance");
        if (QLabel* lbl = maintBlock->findChild<QLabel*>())
            lbl->setText(isMed ? "Date d'expiration (auto)" : "Prochaine maintenance (auto)");
        if (QLabel* lbl = intervalBlock->findChild<QLabel*>())
            lbl->setText(isMed ? "Durée de validité" : "Intervalle");

        // Label intérieur de la ligne
        lastMaintLabel->setText(isMed ? "Date de fabrication :" : "Dernière maintenance :");
        maintLabel->setText(isMed ? "Date d'expiration :" : "Prochaine maintenance :");
        intervalLabel->setText(isMed ? "Durée de validité :" : "Intervalle maintenance :");

        // Même logique auto pour équipement et médicament.
        intervalBlock->setVisible(true);
        maintDate->setReadOnly(true);
        refreshDerivedEquipDate();
    });

    eqOuter2L->addWidget(eqLeft2);
    eqLeft2->setVisible(false);
    eqOuter2L->addWidget(eqRight2, 1);

    QScrollArea* eqOuterScroll = new QScrollArea;
    eqOuterScroll->setWidgetResizable(true);
    eqOuterScroll->setFrameShape(QFrame::NoFrame);
    eqOuterScroll->setStyleSheet(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollArea > QWidget > QWidget { background: transparent; }");
    eqOuterScroll->setWidget(eqOuter2);
    eq2->addWidget(eqOuterScroll, 1);

    QFrame* eqBottom2 = new QFrame;
    eqBottom2->setFixedHeight(64);
    eqBottom2->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* eqBottom2L = new QHBoxLayout(eqBottom2);
    eqBottom2L->setContentsMargins(14,10,14,10);
    eqBottom2L->setSpacing(12);

    QPushButton* eqSave = actionBtn("Enregistrer", "rgba(10,95,88,0.45)", "rgba(255,255,255,0.90)", st->standardIcon(QStyle::SP_DialogSaveButton), true);
    QPushButton* eqCancel = actionBtn("Annuler", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_DialogCancelButton), true);

    eqBottom2L->addWidget(eqSave);
    eqBottom2L->addWidget(eqCancel);
    eqBottom2L->addStretch(1);
    eq2->addWidget(eqBottom2);

    addStackPage(equip2);

    // ==========================================================
    // PAGE 16 : Équipements - STATISTIQUES (EQUIP_LOC)
    // ==========================================================
    QWidget* equip3 = new QWidget;
    QVBoxLayout* eq3 = new QVBoxLayout(equip3);
    eq3->setContentsMargins(22, 18, 22, 18);
    eq3->setSpacing(14);

    ModulesBar barEquipLoc;
    eq3->addWidget(makeHeaderBlock(st, "Statistiques des équipements", ModuleTab::Equipement, &barEquipLoc));
    connectModulesSwitch(this, stack, barEquipLoc);

    QFrame* eqOuter3 = new QFrame;
    eqOuter3->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }").arg(C_PANEL_BG, C_PANEL_BR));
    QHBoxLayout* eqOuter3L = new QHBoxLayout(eqOuter3);
    eqOuter3L->setContentsMargins(12,12,12,12);
    eqOuter3L->setSpacing(12);

    QFrame* eqLeft3 = softBox();
    eqLeft3->setFixedWidth(320);
    QVBoxLayout* eqLeft3L = new QVBoxLayout(eqLeft3);
    eqLeft3L->setContentsMargins(12,12,12,12);
    eqLeft3L->setSpacing(10);

    QLabel* eqTreeTitle3 = new QLabel("Organigramme par modèle");
    eqTreeTitle3->setStyleSheet("color: rgba(0,0,0,0.62); font-weight: 900; font-size: 15px;");

    QLabel* eqTreeHint3 = new QLabel("Sélectionnez un équipement dans l'arborescence pour consulter ses détails.");
    eqTreeHint3->setWordWrap(true);
    eqTreeHint3->setStyleSheet("color: rgba(0,0,0,0.48); font-weight: 600;");

    QTreeWidget* eqTree = new QTreeWidget;
    eqTree->setHeaderHidden(true);
    eqTree->setIndentation(18);
    eqTree->setRootIsDecorated(true);
    eqTree->setAnimated(true);
    eqTree->setUniformRowHeights(false);
    eqTree->setCursor(Qt::PointingHandCursor);

    eqLeft3L->addWidget(eqTreeTitle3);
    eqLeft3L->addWidget(eqTreeHint3);
    eqLeft3L->addWidget(eqTree, 1);

    QFrame* eqRight3 = softBox();
    QVBoxLayout* eqRight3L = new QVBoxLayout(eqRight3);
    eqRight3L->setContentsMargins(10,10,10,10);
    eqRight3L->setSpacing(10);

    auto eqChip = [&](const QString& t){
        QLabel* c = new QLabel(t);
        c->setStyleSheet("background: rgba(255,255,255,0.90); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; padding: 8px 12px; font-weight:900; color: rgba(0,0,0,0.55);");
        return c;
    };

    QFrame* eqHeader3 = new QFrame;
    eqHeader3->setStyleSheet("QFrame{ background: rgba(255,255,255,0.72); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QHBoxLayout* eqHeader3L = new QHBoxLayout(eqHeader3);
    eqHeader3L->setContentsMargins(10,8,10,8);
    eqHeader3L->setSpacing(8);

    QPushButton* eqDetails3 = new QPushButton(st->standardIcon(QStyle::SP_FileDialogDetailedView), "  Voir les détails");
    eqDetails3->setCursor(Qt::PointingHandCursor);
    eqDetails3->setStyleSheet(QString(R"(
        QPushButton{
            background:%1; color: rgba(255,255,255,0.95);
            border:1px solid rgba(0,0,0,0.18);
            border-radius: 12px; padding: 10px 16px; font-weight: 900;
        }
        QPushButton:hover{ background: %2; }
    )").arg(C_PRIMARY, C_TOPBAR));

    QLabel* eqTotalChip3 = eqChip("Total : 0");
    QLabel* eqModelChip3 = eqChip("Modèles : 0");
    QLabel* eqDateChip3  = eqChip("Achats datés : 0");

    eqHeader3L->addWidget(eqDetails3);
    eqHeader3L->addStretch(1);
    eqHeader3L->addWidget(eqTotalChip3);
    eqHeader3L->addWidget(eqModelChip3);
    eqHeader3L->addWidget(eqDateChip3);

    QHBoxLayout* eqChartsRow3 = new QHBoxLayout;
    eqChartsRow3->setContentsMargins(0,0,0,0);
    eqChartsRow3->setSpacing(10);

    QFrame* eqDonutCard3 = new QFrame;
    QVBoxLayout* eqDonutCard3L = new QVBoxLayout(eqDonutCard3);
    eqDonutCard3L->setContentsMargins(12,12,12,12);
    eqDonutCard3L->setSpacing(8);
    QLabel* eqDonutTitle3 = new QLabel("Cercle par modèle");
    eqDonutTitle3->setStyleSheet("color: rgba(0,0,0,0.62); font-weight: 900; font-size: 14px;");
    QLabel* eqDonutSub3 = new QLabel("Répartition des équipements selon leur numéro de modèle.");
    eqDonutSub3->setWordWrap(true);
    eqDonutSub3->setStyleSheet("color: rgba(0,0,0,0.48); font-weight: 600;");
    DonutChart* eqDonut3 = new DonutChart;
    eqDonutCard3L->addWidget(eqDonutTitle3);
    eqDonutCard3L->addWidget(eqDonutSub3);
    eqDonutCard3L->addWidget(eqDonut3, 1);

    QFrame* eqBarCard3 = new QFrame;
    QVBoxLayout* eqBarCard3L = new QVBoxLayout(eqBarCard3);
    eqBarCard3L->setContentsMargins(12,12,12,12);
    eqBarCard3L->setSpacing(8);
    QLabel* eqBarTitle3 = new QLabel("Achats par année");
    eqBarTitle3->setStyleSheet("color: rgba(0,0,0,0.62); font-weight: 900; font-size: 14px;");
    QLabel* eqBarSub3 = new QLabel("Statistiques basées sur la date d'achat des équipements.");
    eqBarSub3->setWordWrap(true);
    eqBarSub3->setStyleSheet("color: rgba(0,0,0,0.48); font-weight: 600;");
    BarChart* eqBars3 = new BarChart;
    eqBarCard3L->addWidget(eqBarTitle3);
    eqBarCard3L->addWidget(eqBarSub3);
    eqBarCard3L->addWidget(eqBars3, 1);

    eqChartsRow3->addWidget(eqDonutCard3, 1);
    eqChartsRow3->addWidget(eqBarCard3, 1);

    QFrame* eqFocusCard3 = new QFrame;
    QVBoxLayout* eqFocusCard3L = new QVBoxLayout(eqFocusCard3);
    eqFocusCard3L->setContentsMargins(12,12,12,12);
    eqFocusCard3L->setSpacing(6);

    QLabel* eqFocusTitle3 = new QLabel("Équipement sélectionné");
    eqFocusTitle3->setStyleSheet("color: rgba(0,0,0,0.62); font-weight: 900; font-size: 14px;");
    QLabel* eqFocusName3 = new QLabel("Aucun équipement sélectionné");
    eqFocusName3->setStyleSheet("color: rgba(0,0,0,0.68); font-weight: 900; font-size: 18px;");
    eqFocusName3->setWordWrap(true);
    QLabel* eqFocusLine13 = new QLabel("Modèle : -");
    QLabel* eqFocusLine23 = new QLabel("Date d'achat : -");
    QLabel* eqFocusLine33 = new QLabel("Localisation : -");
    QLabel* eqFocusLine43 = new QLabel("Maintenance : -");
    for (QLabel* infoLbl : {eqFocusLine13, eqFocusLine23, eqFocusLine33, eqFocusLine43}) {
        infoLbl->setWordWrap(true);
        infoLbl->setStyleSheet("color: rgba(0,0,0,0.56); font-weight: 700;");
    }

    eqFocusCard3L->addWidget(eqFocusTitle3);
    eqFocusCard3L->addWidget(eqFocusName3);
    eqFocusCard3L->addWidget(eqFocusLine13);
    eqFocusCard3L->addWidget(eqFocusLine23);
    eqFocusCard3L->addWidget(eqFocusLine33);
    eqFocusCard3L->addWidget(eqFocusLine43);
    eqFocusCard3L->addStretch(1);

    eqRight3L->addWidget(eqHeader3);
    eqRight3L->addLayout(eqChartsRow3, 1);
    eqRight3L->addWidget(eqFocusCard3);

    eqOuter3L->addWidget(eqLeft3);
    eqOuter3L->addWidget(eqRight3, 1);

    eq3->addWidget(eqOuter3, 1);

    QFrame* eqBottom3 = new QFrame;
    eqBottom3->setFixedHeight(64);
    eqBottom3->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* eqBottom3L = new QHBoxLayout(eqBottom3);
    eqBottom3L->setContentsMargins(14,10,14,10);

    QPushButton* eqBack3 = actionBtn("Retour", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_ArrowBack), true);
    eqBottom3L->addWidget(eqBack3);
    eqBottom3L->addStretch(1);

    eq3->addWidget(eqBottom3);
    addStackPage(equip3);

    int* eqStatsSelectedId = new int(-1);
    QMap<int, EquipementRecord>* eqStatsCache = new QMap<int, EquipementRecord>();

    auto updateEqStatsSelectionCard = [=](int equipId) {
        *eqStatsSelectedId = equipId;
        if (equipId <= 0 || !eqStatsCache->contains(equipId)) {
            eqFocusName3->setText("Aucun équipement sélectionné");
            eqFocusLine13->setText("Modèle : -");
            eqFocusLine23->setText("Date d'achat : -");
            eqFocusLine33->setText("Localisation : -");
            eqFocusLine43->setText("Maintenance : -");
            return;
        }

        const EquipementRecord rec = eqStatsCache->value(equipId);
        const QString name = rec.nomEquipement.trimmed().isEmpty()
                                 ? QString("Équipement #%1").arg(rec.id)
                                 : rec.nomEquipement.trimmed();
        const QString fabricant = rec.fabricant.trimmed().isEmpty() ? QString("Fabricant non précisé")
                                                                    : rec.fabricant.trimmed();
        const QString modele = rec.numeroModele.trimmed().isEmpty() ? QString("Non précisé")
                                                                    : rec.numeroModele.trimmed();
        const QString achat = rec.dateAchat.isValid() ? rec.dateAchat.toString("dd/MM/yyyy")
                                                      : QString("Non renseignée");
        const QString localisation = rec.localisation.trimmed().isEmpty() ? QString("Non renseignée")
                                                                          : rec.localisation.trimmed();
        const QString maint = rec.dateProchaineMaintenance.isValid() ? rec.dateProchaineMaintenance.toString("dd/MM/yyyy")
                                                                     : QString("Non planifiée");
        const QString statut = rec.statut.trimmed().isEmpty() ? QString("Non précisé")
                                                              : rec.statut.trimmed();

        eqFocusName3->setText(QString("%1  |  %2").arg(name, fabricant));
        eqFocusLine13->setText("Modèle : " + modele);
        eqFocusLine23->setText("Date d'achat : " + achat);
        eqFocusLine33->setText(QString("Localisation : %1  |  Statut : %2").arg(localisation, statut));
        eqFocusLine43->setText("Prochaine maintenance : " + maint);
    };

    auto updateEqStats = [=]() {
        QList<EquipementRecord> recs;
        QString err;
        if (!eqCrud->loadEquipements(recs, &err, QString(), QString(), QString(), QString())) {
            eqTree->clear();
            eqDonut3->setData({});
            eqBars3->setData({});
            eqStatsCache->clear();
            eqTotalChip3->setText("Total : 0");
            eqModelChip3->setText("Modèles : 0");
            eqDateChip3->setText("Achats datés : 0");
            updateEqStatsSelectionCard(-1);
            showToast(this, "Erreur statistiques équipement : " + err, false);
            return;
        }

        eqStatsCache->clear();
        QMap<QString, int> modelCounts;
        QMap<QString, int> purchaseYearCounts;
        QMap<QString, QMap<QString, QList<EquipementRecord>>> treeData;
        int datedPurchases = 0;

        for (const EquipementRecord& rec : recs) {
            eqStatsCache->insert(rec.id, rec);

            const QString modelKey = rec.numeroModele.trimmed().isEmpty()
                                         ? QString("Modèle non précisé")
                                         : rec.numeroModele.trimmed();
            const QString locKey = rec.localisation.trimmed().isEmpty()
                                       ? QString("Localisation non précisée")
                                       : rec.localisation.trimmed();

            modelCounts[modelKey] += 1;
            treeData[modelKey][locKey].push_back(rec);

            if (rec.dateAchat.isValid()) {
                purchaseYearCounts[rec.dateAchat.toString("yyyy")] += 1;
                ++datedPurchases;
            } else {
                purchaseYearCounts["N/D"] += 1;
            }
        }

        eqTotalChip3->setText(QString("Total : %1").arg(recs.size()));
        eqModelChip3->setText(QString("Modèles : %1").arg(modelCounts.size()));
        eqDateChip3->setText(QString("Achats datés : %1").arg(datedPurchases));

        QList<QPair<QString, int>> modelsSorted;
        for (auto it = modelCounts.constBegin(); it != modelCounts.constEnd(); ++it)
            modelsSorted.push_back(qMakePair(it.key(), it.value()));
        std::sort(modelsSorted.begin(), modelsSorted.end(), [](const QPair<QString, int>& a, const QPair<QString, int>& b) {
            if (a.second != b.second) return a.second > b.second;
            return a.first.toLower() < b.first.toLower();
        });

        QList<DonutChart::Slice> donutSlices;
        const QList<QColor> eqPalette = {
            QColor("#2DD4BF"), QColor("#38BDF8"), QColor("#F59E0B"),
            QColor("#FB7185"), QColor("#A78BFA"), QColor("#94A3B8")
        };
        int othersCount = 0;
        const int maxSlices = 5;
        for (int i = 0; i < modelsSorted.size(); ++i) {
            if (i < maxSlices) {
                donutSlices.push_back({(double)modelsSorted[i].second,
                                       eqPalette[i % eqPalette.size()],
                                       modelsSorted[i].first});
            } else {
                othersCount += modelsSorted[i].second;
            }
        }
        if (othersCount > 0)
            donutSlices.push_back({(double)othersCount, QColor("#64748B"), "Autres"});
        eqDonut3->setData(donutSlices);

        QList<BarChart::Bar> yearBars;
        for (auto it = purchaseYearCounts.constBegin(); it != purchaseYearCounts.constEnd(); ++it)
            yearBars.push_back({(double)it.value(), it.key()});
        eqBars3->setData(yearBars);

        eqTree->clear();
        int firstEquipId = -1;
        for (auto itModel = treeData.constBegin(); itModel != treeData.constEnd(); ++itModel) {
            int modelTotal = 0;
            for (auto itLoc = itModel.value().constBegin(); itLoc != itModel.value().constEnd(); ++itLoc)
                modelTotal += itLoc.value().size();

            QTreeWidgetItem* modelItem = new QTreeWidgetItem(eqTree, QStringList()
                << QString("%1 (%2)").arg(itModel.key()).arg(modelTotal));
            modelItem->setIcon(0, st->standardIcon(QStyle::SP_DirClosedIcon));

            for (auto itLoc = itModel.value().constBegin(); itLoc != itModel.value().constEnd(); ++itLoc) {
                QTreeWidgetItem* locItem = new QTreeWidgetItem(modelItem, QStringList()
                    << QString("%1 (%2)").arg(itLoc.key()).arg(itLoc.value().size()));
                locItem->setIcon(0, st->standardIcon(QStyle::SP_DriveHDIcon));

                QList<EquipementRecord> locRecs = itLoc.value();
                std::sort(locRecs.begin(), locRecs.end(), [](const EquipementRecord& a, const EquipementRecord& b) {
                    const bool aValid = a.dateAchat.isValid();
                    const bool bValid = b.dateAchat.isValid();
                    if (aValid != bValid) return aValid;
                    if (a.dateAchat != b.dateAchat) return a.dateAchat > b.dateAchat;
                    return a.nomEquipement.toLower() < b.nomEquipement.toLower();
                });

                for (const EquipementRecord& rec : locRecs) {
                    const QString equipName = rec.nomEquipement.trimmed().isEmpty()
                                                  ? QString("Équipement #%1").arg(rec.id)
                                                  : rec.nomEquipement.trimmed();
                    const QString purchaseText = rec.dateAchat.isValid()
                                                     ? rec.dateAchat.toString("dd/MM/yyyy")
                                                     : QString("date d'achat non renseignée");
                    QTreeWidgetItem* leaf = new QTreeWidgetItem(locItem, QStringList()
                        << QString("%1  |  %2").arg(equipName, purchaseText));
                    leaf->setIcon(0, st->standardIcon(QStyle::SP_FileIcon));
                    leaf->setData(0, Qt::UserRole, rec.id);
                    if (firstEquipId <= 0) firstEquipId = rec.id;
                }
            }
        }
        eqTree->expandAll();

        if (*eqStatsSelectedId > 0 && eqStatsCache->contains(*eqStatsSelectedId)) {
            QList<QTreeWidgetItem*> matches = eqTree->findItems(QString(), Qt::MatchContains | Qt::MatchRecursive, 0);
            for (QTreeWidgetItem* item : matches) {
                if (item->data(0, Qt::UserRole).toInt() == *eqStatsSelectedId) {
                    eqTree->setCurrentItem(item);
                    break;
                }
            }
            updateEqStatsSelectionCard(*eqStatsSelectedId);
        } else {
            updateEqStatsSelectionCard(firstEquipId);
            if (firstEquipId > 0) {
                QList<QTreeWidgetItem*> matches = eqTree->findItems(QString(), Qt::MatchContains | Qt::MatchRecursive, 0);
                for (QTreeWidgetItem* item : matches) {
                    if (item->data(0, Qt::UserRole).toInt() == firstEquipId) {
                        eqTree->setCurrentItem(item);
                        break;
                    }
                }
            }
        }
    };

    QObject::connect(eqTree, &QTreeWidget::itemSelectionChanged, this, [=]() {
        QTreeWidgetItem* current = eqTree->currentItem();
        updateEqStatsSelectionCard(current ? current->data(0, Qt::UserRole).toInt() : -1);
    });

    QObject::connect(stack, &QStackedWidget::currentChanged, equip3, [=](int idx){
        if (idx == EQUIP_LOC) updateEqStats();
    });

    // ==========================================================
    // PAGE 17 : Équipements - DÉTAILS (EQUIP_DETAILS)
    // ==========================================================
    QWidget* equip4 = new QWidget;
    QVBoxLayout* eq4 = new QVBoxLayout(equip4);
    eq4->setContentsMargins(22, 18, 22, 18);
    eq4->setSpacing(14);

    ModulesBar barEquipDetails;
    eq4->addWidget(makeHeaderBlock(st, "Détails équipement", ModuleTab::Equipement, &barEquipDetails));
    connectModulesSwitch(this, stack, barEquipDetails);

    QScrollArea* eqScrollArea = new QScrollArea;
    eqScrollArea->setWidgetResizable(true);
    eqScrollArea->setFrameShape(QFrame::NoFrame);
    eqScrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    QWidget* eqScrollContent = new QWidget;
    QVBoxLayout* eqScrollL = new QVBoxLayout(eqScrollContent);
    eqScrollL->setContentsMargins(0,0,0,0);
    eqScrollL->setSpacing(12);

    QFrame* eqOuter4 = new QFrame;
    eqOuter4->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }").arg(C_PANEL_BG, C_PANEL_BR));
    QVBoxLayout* eqOuter4L = new QVBoxLayout(eqOuter4);
    eqOuter4L->setContentsMargins(12,12,12,12);
    eqOuter4L->setSpacing(12);

    QFrame* titleFrame = softBox();
    QHBoxLayout* titleL = new QHBoxLayout(titleFrame);
    titleL->setContentsMargins(14,12,14,12);

    QLabel* equipIcon = new QLabel;
    equipIcon->setPixmap(st->standardIcon(QStyle::SP_ComputerIcon).pixmap(48,48));

    QLabel* equipTitle = new QLabel("<b>PCR Machine - Thermo Fisher</b>");
    QFont titleFont = equipTitle->font();
    titleFont.setPointSize(16);
    equipTitle->setFont(titleFont);

    QLabel* statusBadge = new QLabel("Disponible");
    statusBadge->setAlignment(Qt::AlignCenter);
    statusBadge->setFixedSize(120, 32);
    statusBadge->setStyleSheet("QLabel{ background:#2E6F63; color:white; border-radius:16px; font-weight:900; padding:4px 12px; }");

    titleL->addWidget(equipIcon);
    titleL->addWidget(equipTitle, 1);
    titleL->addWidget(statusBadge);
    eqOuter4L->addWidget(titleFrame);

    QFrame* detailsFrame = softBox();
    QVBoxLayout* detailsMainL = new QVBoxLayout(detailsFrame);
    detailsMainL->setContentsMargins(14,14,14,14);
    detailsMainL->setSpacing(10);

    QLabel* detailsHeader = new QLabel("<b>Informations équipement</b>");
    detailsHeader->setStyleSheet("color: rgba(0,0,0,0.65); font-weight: 900; font-size: 14px;");
    detailsMainL->addWidget(detailsHeader);

    QGridLayout* detailsGrid = new QGridLayout;
    detailsGrid->setSpacing(12);
    detailsGrid->setColumnStretch(1, 1);
    detailsGrid->setColumnStretch(3, 1);

    auto addDetailRow = [&](int row, int col, const QString& label, const QString& value, QLabel** outValue = nullptr){
        QLabel* lbl = new QLabel("<b>" + label + " :</b>");
        lbl->setStyleSheet("color: rgba(0,0,0,0.65); font-weight: 900; font-size: 12px;");
        QLabel* val = new QLabel(value);
        val->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 600; font-size: 12px;");
        detailsGrid->addWidget(lbl, row, col*2);
        detailsGrid->addWidget(val, row, col*2+1);
        if (outValue) *outValue = val;
    };

    QLabel* eqDetFabricant = nullptr;
    QLabel* eqDetModele = nullptr;
    QLabel* eqDetLocalisation = nullptr;
    QLabel* eqDetDateAchat = nullptr;
    QLabel* eqDetDerniereMaint = nullptr;
    QLabel* eqDetProchaineMaint = nullptr;
    QLabel* eqDetCalibration = nullptr;
    QLabel* eqDetUtilisateur = nullptr;

    addDetailRow(0, 0, "Fabricant", "-", &eqDetFabricant);
    addDetailRow(1, 0, "Modèle", "-", &eqDetModele);
    addDetailRow(2, 0, "Localisation", "-", &eqDetLocalisation);
    addDetailRow(3, 0, "Date d'achat", "-", &eqDetDateAchat);

    addDetailRow(0, 1, "Dernière maintenance", "-", &eqDetDerniereMaint);
    addDetailRow(1, 1, "Prochaine maintenance", "-", &eqDetProchaineMaint);
    addDetailRow(2, 1, "Calibration", "-", &eqDetCalibration);
    addDetailRow(3, 1, "Utilisateur", "-", &eqDetUtilisateur);

    detailsMainL->addLayout(detailsGrid);
    eqOuter4L->addWidget(detailsFrame);

    QFrame* statsContainer = new QFrame;
    statsContainer->setStyleSheet("QFrame{ background: transparent; }");
    QHBoxLayout* statsContainerL = new QHBoxLayout(statsContainer);
    statsContainerL->setContentsMargins(0,0,0,0);
    statsContainerL->setSpacing(12);

    QWidget* kpiWidget = new QWidget;
    QVBoxLayout* kpiL = new QVBoxLayout(kpiWidget);
    kpiL->setContentsMargins(0,0,0,0);
    kpiL->setSpacing(10);

    auto createKpiCard = [&](const QString& title, const QString& value, const QString& subtitle, const QColor& color) {
        QFrame* card = softBox();
        card->setMinimumHeight(110);
        card->setMinimumWidth(150);
        QVBoxLayout* cardL = new QVBoxLayout(card);
        cardL->setContentsMargins(18,14,18,14);
        cardL->setSpacing(8);

        QLabel* titleLbl = new QLabel(title);
        titleLbl->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 800; font-size: 12px;");

        QLabel* valueLbl = new QLabel(value);
        QFont vFont = valueLbl->font();
        vFont.setPointSize(28);
        vFont.setBold(true);
        valueLbl->setFont(vFont);
        valueLbl->setStyleSheet(QString("color: %1;").arg(color.name()));

        QLabel* subLbl = new QLabel(subtitle);
        subLbl->setStyleSheet("color: rgba(0,0,0,0.45); font-weight: 600; font-size: 11px;");

        cardL->addWidget(titleLbl);
        cardL->addWidget(valueLbl);
        cardL->addWidget(subLbl);
        cardL->addStretch(1);

        return card;
    };

    kpiL->addWidget(createKpiCard("DISPONIBILITE", "98.5%", "30 derniers jours", W_GREEN));
    kpiL->addWidget(createKpiCard("HEURES D'USAGE", "847h", "Total", QColor("#4A90E2")));
    kpiL->addWidget(createKpiCard("EFFICACITE", "94%", "Performance", W_ORANGE));

    QWidget* chartsWidget = new QWidget;
    QVBoxLayout* chartsL = new QVBoxLayout(chartsWidget);
    chartsL->setContentsMargins(0,0,0,0);
    chartsL->setSpacing(10);

    QFrame* usageChartFrame = softBox();
    usageChartFrame->setMinimumHeight(180);
    QVBoxLayout* usageChartL = new QVBoxLayout(usageChartFrame);
    usageChartL->setContentsMargins(16,12,16,12);

    QLabel* chartTitle = new QLabel("<b>Usage - 7 derniers jours</b>");
    chartTitle->setStyleSheet("color: rgba(0,0,0,0.65); font-weight: 900; font-size: 13px;");
    usageChartL->addWidget(chartTitle);

    UsageBarChart* usageChart = new UsageBarChart;
    usageChartL->addWidget(usageChart, 1);

    QFrame* timelineFrame = softBox();
    timelineFrame->setMinimumHeight(150);
    QVBoxLayout* timelineL = new QVBoxLayout(timelineFrame);
    timelineL->setContentsMargins(16,12,16,12);
    timelineL->setSpacing(8);

    QLabel* timelineTitle = new QLabel("<b>Historique statut</b>");
    timelineTitle->setStyleSheet("color: rgba(0,0,0,0.65); font-weight: 900; font-size: 13px;");
    timelineL->addWidget(timelineTitle);

    auto createTimelineItem = [&](const QString& date, const QString& event, const QColor& dotColor) {
        QWidget* item = new QWidget;
        QHBoxLayout* itemL = new QHBoxLayout(item);
        itemL->setContentsMargins(0,4,0,4);
        itemL->setSpacing(12);

        QFrame* dot = new QFrame;
        dot->setFixedSize(10, 10);
        dot->setStyleSheet(QString("background: %1; border-radius: 5px;").arg(dotColor.name()));

        QLabel* dateLbl = new QLabel(date);
        dateLbl->setFixedWidth(90);
        dateLbl->setStyleSheet("color: rgba(0,0,0,0.45); font-weight: 700; font-size: 10px;");

        QLabel* eventLbl = new QLabel(event);
        eventLbl->setStyleSheet("color: rgba(0,0,0,0.65); font-weight: 600; font-size: 11px;");

        itemL->addWidget(dot);
        itemL->addWidget(dateLbl);
        itemL->addWidget(eventLbl, 1);

        return item;
    };

    timelineL->addWidget(createTimelineItem("01/02/2026", "Disponible pour usage", W_GREEN));
    timelineL->addWidget(createTimelineItem("28/01/2026", "Maintenance terminée", QColor("#4A90E2")));
    timelineL->addWidget(createTimelineItem("25/01/2026", "Maintenance en cours", W_ORANGE));
    timelineL->addStretch(1);

    chartsL->addWidget(usageChartFrame);
    chartsL->addWidget(timelineFrame);

    statsContainerL->addWidget(kpiWidget);
    statsContainerL->addWidget(chartsWidget, 1);

    eqOuter4L->addWidget(statsContainer);

    QFrame* historyFrame = softBox();
    QVBoxLayout* historyL = new QVBoxLayout(historyFrame);
    historyL->setContentsMargins(14,14,14,14);

    QLabel* historyTitle = new QLabel("<b>Historique maintenance</b>");
    historyTitle->setStyleSheet("color: rgba(0,0,0,0.65); font-weight: 900; font-size: 14px;");
    historyL->addWidget(historyTitle);

    QTableWidget* historyTable = new QTableWidget(3, 3);
    historyTable->setHorizontalHeaderLabels({"Date", "Type", "Technicien"});
    historyTable->verticalHeader()->setVisible(false);
    historyTable->horizontalHeader()->setStretchLastSection(true);
    historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    historyTable->setSelectionMode(QAbstractItemView::NoSelection);
    historyTable->setMaximumHeight(150);

    auto setHistoryRow = [&](int r, const QString& date, const QString& type, const QString& tech){
        historyTable->setItem(r, 0, new QTableWidgetItem(date));
        historyTable->setItem(r, 1, new QTableWidgetItem(type));
        historyTable->setItem(r, 2, new QTableWidgetItem(tech));
    };

    setHistoryRow(0, "15/12/2025", "Maintenance régulière", "Tech-001");
    setHistoryRow(1, "15/09/2025", "Calibration", "Tech-002");
    setHistoryRow(2, "15/06/2025", "Réparation", "Tech-001");

    historyL->addWidget(historyTable);
    eqOuter4L->addWidget(historyFrame);

    QFrame* metricsContainer = new QFrame;
    metricsContainer->setStyleSheet("QFrame{ background: transparent; }");
    QHBoxLayout* metricsL = new QHBoxLayout(metricsContainer);
    metricsL->setContentsMargins(0,0,0,0);
    metricsL->setSpacing(12);

    auto createMetricCard = [&](const QString& icon, const QString& label, const QString& value, const QString& trend) {
        QFrame* card = softBox();
        card->setMinimumHeight(100);
        QVBoxLayout* cardL = new QVBoxLayout(card);
        cardL->setContentsMargins(16,12,16,12);
        cardL->setSpacing(6);

        QHBoxLayout* topRow = new QHBoxLayout;
        topRow->setSpacing(8);

        QLabel* iconLbl = new QLabel(icon);
        QFont iconFont = iconLbl->font();
        iconFont.setPointSize(24);
        iconLbl->setFont(iconFont);

        QLabel* labelLbl = new QLabel(label);
        labelLbl->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 800; font-size: 12px;");

        topRow->addWidget(iconLbl);
        topRow->addWidget(labelLbl);
        topRow->addStretch(1);

        QLabel* valueLbl = new QLabel(value);
        QFont vFont = valueLbl->font();
        vFont.setPointSize(22);
        vFont.setBold(true);
        valueLbl->setFont(vFont);
        valueLbl->setStyleSheet("color: rgba(0,0,0,0.75);");

        QLabel* trendLbl = new QLabel(trend);
        trendLbl->setStyleSheet("color: rgba(0,0,0,0.45); font-weight: 600; font-size: 11px;");

        cardL->addLayout(topRow);
        cardL->addWidget(valueLbl);
        cardL->addWidget(trendLbl);

        return card;
    };

    metricsL->addWidget(createMetricCard("⚡", "CYCLES", "1,247", "+12% ce mois"));
    metricsL->addWidget(createMetricCard("🔧", "MAINTENANCES", "18", "Dernière : 15 jours"));
    metricsL->addWidget(createMetricCard("⏱️", "DUREE MOY.", "4.2h", "-8% vs. mois dernier"));
    metricsL->addWidget(createMetricCard("📊", "TAUX SUCCES", "96.8%", "+2.1%"));

    eqOuter4L->addWidget(metricsContainer);

    eqScrollL->addWidget(eqOuter4);
    eqScrollArea->setWidget(eqScrollContent);

    eq4->addWidget(eqScrollArea, 1);

    QFrame* eqBottom4 = new QFrame;
    eqBottom4->setFixedHeight(64);
    eqBottom4->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* eqBottom4L = new QHBoxLayout(eqBottom4);
    eqBottom4L->setContentsMargins(14,10,14,10);
    eqBottom4L->setSpacing(12);

    QPushButton* eqBack4 = actionBtn("Retour", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_ArrowBack), true);
    QPushButton* eqEditFromDetails = actionBtn("Modifier", "rgba(10,95,88,0.45)", "rgba(255,255,255,0.90)", st->standardIcon(QStyle::SP_FileDialogContentsView), true);

    eqBottom4L->addWidget(eqBack4);
    eqBottom4L->addWidget(eqEditFromDetails);
    eqBottom4L->addStretch(1);

    eq4->addWidget(eqBottom4);
    addStackPage(equip4);

    // ==========================================================
    // ======================  EMPLOYES  ========================
    // ==========================================================

    // ==========================================================
    // PAGE 18 : Employés - LISTE (EMP_LIST)
    // ==========================================================
    QWidget* empListPage = new QWidget;
    QVBoxLayout* emp1 = new QVBoxLayout(empListPage);
    emp1->setContentsMargins(22, 18, 22, 18);
    emp1->setSpacing(14);

    ModulesBar barEmpList;
    emp1->addWidget(makeHeaderBlock(st, "Employés", ModuleTab::Employee, &barEmpList));
    connectModulesSwitch(this, stack, barEmpList);

    QFrame* empBar = new QFrame;
    empBar->setFixedHeight(54);
    empBar->setStyleSheet("background: rgba(255,255,255,0.22); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* empBarL = new QHBoxLayout(empBar);
    empBarL->setContentsMargins(14, 8, 14, 8);
    empBarL->setSpacing(10);

    QLineEdit* empSearch = new QLineEdit;
    empSearch->setPlaceholderText("Rechercher (CIN, Nom, Prénom)");
    empSearch->addAction(st->standardIcon(QStyle::SP_FileDialogContentsView), QLineEdit::LeadingPosition);

    QComboBox* empRole = new QComboBox; empRole->addItems({"Role", "Chercheur", "Technicien", "Responsable", "RH"});
    QComboBox* empSpec = new QComboBox; empSpec->addItems({"Specialisation", "Biomol", "Bioinfo", "Chimie", "General"});
    QComboBox* empLab  = new QComboBox; empLab->addItems({"Laboratoire", "Lab A", "Lab B", "Lab C"});
    QComboBox* empFT   = new QComboBox; empFT->addItems({"Temps", "Plein", "Partiel", "Contrat", "Absence"});

    QPushButton* empFilters = new QPushButton(st->standardIcon(QStyle::SP_FileDialogDetailedView), "  Filtres");
    empFilters->setCursor(Qt::PointingHandCursor);
    empFilters->setStyleSheet(QString(R"(
        QPushButton{
            background:%1; color: rgba(255,255,255,0.92);
            border:1px solid rgba(0,0,0,0.18);
            border-radius: 12px; padding: 10px 16px; font-weight: 800;
        }
        QPushButton:hover{ background: %2; }
    )").arg(C_PRIMARY, C_TOPBAR));

    empBarL->addWidget(empSearch, 1);
    empBarL->addWidget(empRole);
    empBarL->addWidget(empSpec);
    empBarL->addWidget(empLab);
    empBarL->addWidget(empFT);
    empBarL->addWidget(empFilters);
    emp1->addWidget(empBar);

    QFrame* empCard = makeCard();
    QVBoxLayout* empCardL = new QVBoxLayout(empCard);
    empCardL->setContentsMargins(10,10,10,10);

    EmployesCrud* empCrud     = new EmployesCrud;
    bool*         empEditMode = new bool(false);
    int*          empEditId   = new int(0);

    QTableWidget* empTable = new QTableWidget(0, 11);
    empTable->setHorizontalHeaderLabels({"", "CIN", "Nom", "Prenom", "Role", "Specialisation", "Qualification", "Publications", "Temps", "Laboratoire", "Projet"});
    empTable->verticalHeader()->setVisible(false);
    empTable->setShowGrid(true);
    empTable->setAlternatingRowColors(false); // handled manually to avoid conflicts with setBackground()
    empTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    empTable->setSelectionMode(QAbstractItemView::SingleSelection);
    empTable->horizontalHeader()->setStretchLastSection(true);
    empTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    empTable->setItemDelegateForColumn(8, new EmployeeBadgeDelegate(empTable));
    empTable->setStyleSheet(QString(R"(
        QTableWidget{
            background: #F8FAF9;
            border: none;
            gridline-color: rgba(0,0,0,0.07);
            color: rgba(0,0,0,0.80);
            font-size: 13px;
        }
        QTableWidget::item{
            color: rgba(0,0,0,0.80);
            padding: 4px 6px;
            border: none;
        }
        QTableWidget::item:selected{
            background: rgba(10,95,88,0.18);
            color: rgba(0,0,0,0.90);
        }
    )"));

    empTable->setColumnWidth(0, 36);
    empTable->setColumnWidth(1, 110);
    empTable->setColumnWidth(2, 120);
    empTable->setColumnWidth(3, 120);
    empTable->setColumnWidth(4, 120);
    empTable->setColumnWidth(5, 140);
    empTable->setColumnWidth(6, 140);
    empTable->setColumnWidth(7, 110);
    empTable->setColumnWidth(8, 120);
    empTable->setColumnWidth(9, 110);
    empTable->setColumnWidth(10, 140);

    // Sorting enabled only after data is loaded — prevents mid-insert repaint glitches
    empTable->setSortingEnabled(false);
    empTable->horizontalHeader()->setSectionsClickable(true);
    empTable->horizontalHeader()->setStyleSheet(QString(R"(
        QHeaderView::section{
            background: %1; color: rgba(0,0,0,0.70);
            border: none; border-right: 1px solid rgba(0,0,0,0.08);
            padding: 8px 6px; font-weight: 900;
        }
        QHeaderView::section:hover{ background: %2; }
    )").arg(C_TABLE_HDR, C_BG));
    // Re-enable sorting on header click after load (won't glitch because insertRow is done)
    QObject::connect(empTable->horizontalHeader(), &QHeaderView::sectionClicked,
                     empTable, [=](int col){
        empTable->setSortingEnabled(true);
        empTable->sortByColumn(col, empTable->horizontalHeader()->sortIndicatorOrder());
    });

    auto ftStatusFromText = [](const QString& value) -> FTStatus {
        const QString v = value.trimmed().toLower();
        if (v == "partiel" || v == "temps partiel") return FTStatus::PartTime;
        if (v == "contrat") return FTStatus::Contract;
        if (v == "absence" || v == "absent") return FTStatus::OnLeave;
        return FTStatus::FullTime;
    };

    auto setEmpRow=[=](const EmployeRecord& rec,
                       const QString& qualif,
                       const QString& pubs,
                       FTStatus ft,
                       const QString& lab,
                       const QString& proj)
    {
        const int r = empTable->rowCount();
        empTable->insertRow(r);

        QTableWidgetItem* iconItem = new QTableWidgetItem;
        iconItem->setIcon(st->standardIcon(QStyle::SP_ArrowRight));
        iconItem->setTextAlignment(Qt::AlignCenter);
        empTable->setItem(r, 0, iconItem);

        auto mk = [&](const QString& t){
            QTableWidgetItem* it = new QTableWidgetItem(t);
            it->setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);
            return it;
        };

        QTableWidgetItem* cinItem = mk(rec.cin);
        cinItem->setData(Qt::UserRole, rec.employeeId);
        empTable->setItem(r, 1, cinItem);
        empTable->setItem(r, 2, mk(rec.nom));
        empTable->setItem(r, 3, mk(rec.prenom));
        empTable->setItem(r, 4, mk(rec.role));
        empTable->setItem(r, 5, mk(rec.specialization));
        empTable->setItem(r, 6, mk(qualif));
        QTableWidgetItem* p = mk(pubs);
        p->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
        empTable->setItem(r, 7, p);

        QTableWidgetItem* badge = new QTableWidgetItem;
        badge->setData(Qt::UserRole, (int)ft);
        empTable->setItem(r, 8, badge);

        empTable->setItem(r, 9, mk(lab));
        empTable->setItem(r,10, mk(proj));

        empTable->setRowHeight(r, 46);
    };

    auto loadEmpTable = [=]() {
        QList<EmployeRecord> recs;
        QString err;

        const QString roleFilter = (empRole->currentIndex() > 0) ? empRole->currentText() : QString();
        const QString specFilter = (empSpec->currentIndex() > 0) ? empSpec->currentText() : QString();

        const QString searchTerm = empSearch->text().trimmed();
        // Pass searchTerm only to cin; nom/prenom search is handled client-side by applyEmpFilters
        if (!empCrud->loadEmployes(recs, &err, searchTerm, QString(), QString(), roleFilter, specFilter)) {
            empTable->setRowCount(0);
            return;
        }

        empTable->setSortingEnabled(false); // disable during bulk insert to prevent repaint glitches
        empTable->setRowCount(0);

        int visibleRow = 0;
        for (const EmployeRecord& rec : recs) {
            const QString qualif = rec.qualification.trimmed().isEmpty() ? "-" : rec.qualification.trimmed();
            const QString temps  = rec.tempsTravail.trimmed().isEmpty()  ? "Plein" : rec.tempsTravail.trimmed();
            const QString labo   = rec.laboratoire.trimmed().isEmpty()   ? "-" : rec.laboratoire.trimmed();
            const QString pubs   = QString::number(rec.nbPublications);

            setEmpRow(rec, qualif, pubs, ftStatusFromText(temps), labo, "-");

            // Apply solid alternating row colors (no alpha — avoids compositing issues)
            const bool incomplete = rec.cin.trimmed().isEmpty() && rec.nom.trimmed().isEmpty();
            const QColor rowBg = incomplete
                ? QColor(255, 235, 180)                              // solid warm yellow for incomplete
                : (visibleRow % 2 == 0 ? QColor(242,244,243) : QColor(250,251,251)); // solid alternating

            const int r = empTable->rowCount() - 1;
            for (int c = 0; c < empTable->columnCount(); ++c) {
                if (empTable->item(r, c)) {
                    empTable->item(r, c)->setBackground(rowBg);
                    empTable->item(r, c)->setForeground(QColor(30, 30, 30)); // always readable text
                }
            }
            ++visibleRow;
        }
    };

    auto applyEmpFilters = [=]() {
        const QString search = empSearch->text().trimmed().toLower();
        const QString roleFilter = (empRole->currentIndex() > 0) ? empRole->currentText().toLower() : QString();
        const QString specFilter = (empSpec->currentIndex() > 0) ? empSpec->currentText().toLower() : QString();
        const QString labFilter = (empLab->currentIndex() > 0) ? empLab->currentText().toLower() : QString();
        const QString ftFilter = (empFT->currentIndex() > 0) ? empFT->currentText().toLower() : QString();

        for (int r = 0; r < empTable->rowCount(); ++r) {
            const QString cin = empTable->item(r, 1) ? empTable->item(r, 1)->text().toLower() : QString();
            const QString nom = empTable->item(r, 2) ? empTable->item(r, 2)->text().toLower() : QString();
            const QString prenom = empTable->item(r, 3) ? empTable->item(r, 3)->text().toLower() : QString();
            const QString role = empTable->item(r, 4) ? empTable->item(r, 4)->text().toLower() : QString();
            const QString spec = empTable->item(r, 5) ? empTable->item(r, 5)->text().toLower() : QString();
            const QString lab = empTable->item(r, 9) ? empTable->item(r, 9)->text().toLower() : QString();
            const QString temps = empTable->item(r, 8) ? empStatusText(static_cast<FTStatus>(empTable->item(r, 8)->data(Qt::UserRole).toInt())).toLower() : QString();

            const bool matchSearch = search.isEmpty() || cin.contains(search) || nom.contains(search) || prenom.contains(search);
            const bool matchRole = roleFilter.isEmpty() || role.contains(roleFilter);
            const bool matchSpec = specFilter.isEmpty() || spec.contains(specFilter);
            const bool matchLab = labFilter.isEmpty() || lab.contains(labFilter);
            const bool matchFt = ftFilter.isEmpty() || temps.contains(ftFilter);

            empTable->setRowHidden(r, !(matchSearch && matchRole && matchSpec && matchLab && matchFt));
        }
    };

    loadEmpTable();
    applyEmpFilters();

    empCardL->addWidget(empTable);
    emp1->addWidget(empCard, 1);

    QObject::connect(empSearch, &QLineEdit::textChanged, this, [=](const QString&){ applyEmpFilters(); });
    QObject::connect(empRole, &QComboBox::currentTextChanged, this, [=](const QString&){ loadEmpTable(); applyEmpFilters(); });
    QObject::connect(empSpec, &QComboBox::currentTextChanged, this, [=](const QString&){ loadEmpTable(); applyEmpFilters(); });
    QObject::connect(empLab, &QComboBox::currentTextChanged, this, [=](const QString&){ applyEmpFilters(); });
    QObject::connect(empFT, &QComboBox::currentTextChanged, this, [=](const QString&){ applyEmpFilters(); });
    QObject::connect(empFilters, &QPushButton::clicked, this, [=](){ applyEmpFilters(); });

    QObject::connect(stack, &QStackedWidget::currentChanged, empTable, [=](int idx){
        if (idx == EMP_LIST) {
            loadEmpTable();
            applyEmpFilters();
        }
    });

    QFrame* empBottom = new QFrame;
    empBottom->setFixedHeight(64);
    empBottom->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* empBottomL = new QHBoxLayout(empBottom);
    empBottomL->setContentsMargins(14,10,14,10);
    empBottomL->setSpacing(12);

    QPushButton* empAdd    = actionBtn("Creer", "rgba(10,95,88,0.45)", "rgba(255,255,255,0.90)", st->standardIcon(QStyle::SP_DialogYesButton), true);
    QPushButton* empEdit   = actionBtn("Modifier", "rgba(198,178,154,0.55)", "rgba(255,255,255,0.85)", st->standardIcon(QStyle::SP_FileDialogContentsView), true);
    QPushButton* empDel    = actionBtn("Supprimer", "rgba(255,255,255,0.55)", "#B14A4A", st->standardIcon(QStyle::SP_TrashIcon), true);
    QPushButton* empStats  = actionBtn("Statistiques", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_MessageBoxInformation), true);

    empBottomL->addWidget(empAdd);
    empBottomL->addWidget(empEdit);
    empBottomL->addWidget(empDel);
    empBottomL->addWidget(empStats);
    empBottomL->addStretch(1);

    QPushButton* empPdfBtn = actionBtn("Exporter PDF", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_DialogSaveButton), true);
    empBottomL->addWidget(empPdfBtn);

    QPushButton* empMore = new QPushButton(st->standardIcon(QStyle::SP_DialogApplyButton), "  Affectation Intelligente");
    empMore->setCursor(Qt::PointingHandCursor);
    empMore->setStyleSheet(R"(
        QPushButton{
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 rgba(10,95,88,0.75),stop:1 rgba(18,68,59,0.80));
            border: 1px solid rgba(10,95,88,0.40);
            border-radius: 12px;
            padding: 10px 16px;
            color: rgba(255,255,255,0.92);
            font-weight: 900;
        }
        QPushButton:hover{
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 rgba(10,95,88,0.90),stop:1 rgba(18,68,59,0.95));
        }
    )");
    empBottomL->addWidget(empMore);

    emp1->addWidget(empBottom);
    addStackPage(empListPage);

    // ==========================================================
    // PAGE 19 : Employés - CREER / MODIFIER (EMP_FORM)
    // ==========================================================
    QWidget* empFormPage = new QWidget;
    empFormPage->setObjectName("empFormPage");
    QVBoxLayout* emp2 = new QVBoxLayout(empFormPage);
    emp2->setContentsMargins(22, 18, 22, 18);
    emp2->setSpacing(14);

    ModulesBar barEmpForm;
    emp2->addWidget(makeHeaderBlock(st, "Creer / Modifier Employe", ModuleTab::Employee, &barEmpForm));
    connectModulesSwitch(this, stack, barEmpForm);

    QFrame* empOuter2 = new QFrame;
    empOuter2->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }").arg(C_PANEL_BG, C_PANEL_BR));
    QHBoxLayout* empOuter2L = new QHBoxLayout(empOuter2);
    empOuter2L->setContentsMargins(12,12,12,12);
    empOuter2L->setSpacing(12);

    // Panneau gauche supprimé — le formulaire occupe toute la largeur

    QFrame* empRight2 = softBox();
    QVBoxLayout* empRight2L = new QVBoxLayout(empRight2);
    empRight2L->setContentsMargins(12,12,12,12);
    empRight2L->setSpacing(16);

    QFrame* empTinyTop = softBox();
    QHBoxLayout* empTinyTopL = new QHBoxLayout(empTinyTop);
    empTinyTopL->setContentsMargins(12,8,12,8);

    QToolButton* empAddDrop = new QToolButton;
    empAddDrop->setIcon(st->standardIcon(QStyle::SP_DialogYesButton));
    empAddDrop->setText("Ajouter");
    empAddDrop->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    empAddDrop->setStyleSheet("QToolButton{ color: rgba(0,0,0,0.55); font-weight: 900; }");

    empTinyTopL->addWidget(empAddDrop);
    empTinyTopL->addStretch(1);
    empRight2L->addWidget(empTinyTop);

    auto empComboRow = [&](QWidget* wLeft, QWidget* wRight){
        QFrame* r = softBox();
        QHBoxLayout* l = new QHBoxLayout(r);
        l->setContentsMargins(12,14,12,14);
        l->setSpacing(8);
        l->addWidget(wLeft, 1);
        l->addWidget(wRight, 1);
        return r;
    };

    // ── helper : colore un QLineEdit en rouge si erreur ──────────
    auto setFieldErr = [](QLineEdit* le, bool error){
        le->setStyleSheet(error
            ? "QLineEdit{ border: 1.5px solid #B14A4A; border-radius: 8px; background: rgba(177,74,74,0.06); }"
            : "");
    };

    // ── style commun des labels d'erreur ─────────────────────────
    static const char* ERR_STYLE = "color: #B14A4A; font-size: 11px; padding: 0 4px;";

    QLineEdit* empCinEdit = new QLineEdit; empCinEdit->setPlaceholderText("CIN (8 chiffres)");
    QLineEdit* empNomEdit = new QLineEdit; empNomEdit->setPlaceholderText("Nom");
    empRight2L->addWidget(empComboRow(empCinEdit, empNomEdit));
    QLabel* errCinNom = new QLabel; errCinNom->setStyleSheet(ERR_STYLE); errCinNom->setVisible(false);
    empRight2L->addWidget(errCinNom);

    QLineEdit* empPrenomEdit = new QLineEdit; empPrenomEdit->setPlaceholderText("Prenom");
    QComboBox* empRoleCb = new QComboBox; empRoleCb->addItems({"Chercheur","Technicien","Responsable","RH"});
    empRight2L->addWidget(empComboRow(empPrenomEdit, empRoleCb));
    QLabel* errPrenomRole = new QLabel; errPrenomRole->setStyleSheet(ERR_STYLE); errPrenomRole->setVisible(false);
    empRight2L->addWidget(errPrenomRole);

    QLineEdit* empEmailEdit = new QLineEdit; empEmailEdit->setPlaceholderText("Email (ex: nom@labo.org)");
    QLineEdit* empPwdEdit   = new QLineEdit; empPwdEdit->setPlaceholderText("Mot de passe (création uniquement)");
    empPwdEdit->setEchoMode(QLineEdit::Password);
    empRight2L->addWidget(empComboRow(empEmailEdit, empPwdEdit));
    QLabel* errEmailPwd = new QLabel; errEmailPwd->setStyleSheet(ERR_STYLE); errEmailPwd->setVisible(false);
    empRight2L->addWidget(errEmailPwd);

    QComboBox* empSpecCb = new QComboBox; empSpecCb->addItems({"Biomol","Bioinfo","Chimie","General"});
    QLineEdit* empQualifEdit = new QLineEdit; empQualifEdit->setPlaceholderText("Qualification (PhD, MSc...)");
    empRight2L->addWidget(empComboRow(empSpecCb, empQualifEdit));
    QLabel* errSpec = new QLabel; errSpec->setStyleSheet(ERR_STYLE); errSpec->setVisible(false);
    empRight2L->addWidget(errSpec);

    QSpinBox* empPubs = new QSpinBox; empPubs->setRange(0,1000); empPubs->setValue(0);
    empPubs->setPrefix("Pub: ");
    QComboBox* empFtCb = new QComboBox; empFtCb->addItems({"Plein","Partiel","Contrat","Absence"});
    empRight2L->addWidget(empComboRow(empPubs, empFtCb));

    QComboBox* empLabCb = new QComboBox; empLabCb->addItems({"Lab A","Lab B","Lab C"});
    QComboBox* empProjCb = new QComboBox; empProjCb->addItems({"-","Projet GENOME","Projet AI-BIO","Projet PROTEO","Projet MATERIA"});
    empRight2L->addWidget(empComboRow(empLabCb, empProjCb));

    QFrame* empDateRow = softBox();
    QHBoxLayout* empDateRowL = new QHBoxLayout(empDateRow);
    empDateRowL->setContentsMargins(10,8,10,8);
    empDateRowL->setSpacing(8);

    QToolButton* empCal = new QToolButton; empCal->setAutoRaise(true); empCal->setIcon(st->standardIcon(QStyle::SP_FileDialogDetailedView));
    QDateEdit* empDate = new QDateEdit(QDate::currentDate());
    empDate->setCalendarPopup(true);
    empDate->setDisplayFormat("dd/MM/yyyy");
    empDate->setStyleSheet("QDateEdit{ background: transparent; border:0; font-weight: 900; color: rgba(0,0,0,0.55);} ");

    QToolButton* empI1 = new QToolButton; empI1->setAutoRaise(true); empI1->setIcon(st->standardIcon(QStyle::SP_BrowserReload));
    QToolButton* empI2 = new QToolButton; empI2->setAutoRaise(true); empI2->setIcon(st->standardIcon(QStyle::SP_FileDialogListView));
    QToolButton* empI3 = new QToolButton; empI3->setAutoRaise(true); empI3->setIcon(st->standardIcon(QStyle::SP_DialogSaveButton));

    empDateRowL->addWidget(empCal);
    empDateRowL->addWidget(new QLabel("Date d'embauche: "));
    empDateRowL->addWidget(empDate, 1);
    empDateRowL->addWidget(empI1);
    empDateRowL->addWidget(empI2);
    empDateRowL->addWidget(empI3);
    empRight2L->addWidget(empDateRow);

    empRight2L->addStretch(1);

    empOuter2L->addWidget(empRight2, 1);

    QScrollArea* empFormScrollWrap = new QScrollArea;
    empFormScrollWrap->setWidgetResizable(true);
    empFormScrollWrap->setFrameShape(QFrame::NoFrame);
    empFormScrollWrap->setStyleSheet(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollArea > QWidget > QWidget { background: transparent; }");
    empFormScrollWrap->setWidget(empOuter2);
    emp2->addWidget(empFormScrollWrap, 1);

    QFrame* empBottom2 = new QFrame;
    empBottom2->setFixedHeight(64);
    empBottom2->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* empBottom2L = new QHBoxLayout(empBottom2);
    empBottom2L->setContentsMargins(14,10,14,10);
    empBottom2L->setSpacing(12);

    QPushButton* empSave = actionBtn("Enregistrer", "rgba(10,95,88,0.45)", "rgba(255,255,255,0.90)", st->standardIcon(QStyle::SP_DialogSaveButton), true);
    QPushButton* empCancel = actionBtn("Annuler", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_DialogCancelButton), true);

    empBottom2L->addWidget(empSave);
    empBottom2L->addWidget(empCancel);
    empBottom2L->addStretch(1);
    emp2->addWidget(empBottom2);

    addStackPage(empFormPage);

    // ==========================================================
    // PAGE 20 : Employés - AFFECTATION INTELLIGENTE (EMP_AFF)
    // ==========================================================
    QWidget* empAffPage = new QWidget;
    QVBoxLayout* emp3 = new QVBoxLayout(empAffPage);
    emp3->setContentsMargins(22, 18, 22, 18);
    emp3->setSpacing(14);

    ModulesBar barEmpAff;
    emp3->addWidget(makeHeaderBlock(st, "Affectation Intelligente — Projet", ModuleTab::Employee, &barEmpAff));
    connectModulesSwitch(this, stack, barEmpAff);

    // ── Main two-column layout ───────────────────────────────────
    QFrame* empOuter3 = new QFrame;
    empOuter3->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }").arg(C_PANEL_BG, C_PANEL_BR));
    QHBoxLayout* empOuter3L = new QHBoxLayout(empOuter3);
    empOuter3L->setContentsMargins(12,12,12,12);
    empOuter3L->setSpacing(12);

    // ── LEFT : project tree ──────────────────────────────────────
    QFrame* affLeft = softBox();
    affLeft->setFixedWidth(260);
    QVBoxLayout* affLeftL = new QVBoxLayout(affLeft);
    affLeftL->setContentsMargins(10,10,10,10);
    affLeftL->setSpacing(8);

    // Header row: label + refresh
    QFrame* affLeftHdr = new QFrame;
    affLeftHdr->setStyleSheet("QFrame{ background:rgba(255,255,255,0.70); border:1px solid rgba(0,0,0,0.10); border-radius:10px; }");
    QHBoxLayout* affLeftHdrL = new QHBoxLayout(affLeftHdr);
    affLeftHdrL->setContentsMargins(10,6,8,6);
    affLeftHdrL->setSpacing(6);
    QLabel* affProjTitle = new QLabel("Projets disponibles");
    affProjTitle->setStyleSheet("color:rgba(0,0,0,0.65); font-weight:900; font-size:12px;");
    QToolButton* affRefreshBtn = new QToolButton;
    affRefreshBtn->setAutoRaise(true);
    affRefreshBtn->setIcon(st->standardIcon(QStyle::SP_BrowserReload));
    affRefreshBtn->setCursor(Qt::PointingHandCursor);
    affRefreshBtn->setToolTip("Rafraîchir les projets");
    affLeftHdrL->addWidget(affProjTitle, 1);
    affLeftHdrL->addWidget(affRefreshBtn);
    affLeftL->addWidget(affLeftHdr);

    // Project tree
    QTreeWidget* affProjTree = new QTreeWidget;
    affProjTree->setHeaderHidden(true);
    affProjTree->setIndentation(16);
    affProjTree->setAnimated(true);
    affProjTree->setStyleSheet(
        "QTreeWidget{ border:none; background:transparent; outline:none; }"
        "QTreeWidget::item{ padding:7px 4px; border-radius:7px; font-size:12px; color:rgba(0,0,0,0.70); }"
        "QTreeWidget::item:selected{ background:rgba(10,95,88,0.18); color:rgba(10,95,88,0.95); font-weight:900; }"
        "QTreeWidget::item:hover{ background:rgba(10,95,88,0.09); }"
        "QTreeWidget::branch{ background:transparent; }"
    );
    affLeftL->addWidget(affProjTree, 1);

    // Role filter
    QFrame* affRoleBox = new QFrame;
    affRoleBox->setStyleSheet("QFrame{ background:rgba(255,255,255,0.60); border:1px solid rgba(0,0,0,0.09); border-radius:9px; }");
    QHBoxLayout* affRoleBoxL = new QHBoxLayout(affRoleBox);
    affRoleBoxL->setContentsMargins(10,6,10,6);
    affRoleBoxL->setSpacing(8);
    QLabel* affRoleFiltLbl = new QLabel("Rôle :");
    affRoleFiltLbl->setStyleSheet("color:rgba(0,0,0,0.55); font-weight:900; font-size:11px;");
    QComboBox* affRoleCb = new QComboBox;
    affRoleCb->addItems({"Tous", "Chercheur", "Technicien"});
    affRoleCb->setStyleSheet(
        "QComboBox{ background:rgba(255,255,255,0.90); border:1px solid rgba(0,0,0,0.12);"
        " border-radius:8px; padding:5px 10px; font-weight:800; font-size:11px; color:rgba(0,0,0,0.65); }"
        "QComboBox::drop-down{ border:0; padding-right:6px; }"
    );
    affRoleBoxL->addWidget(affRoleFiltLbl);
    affRoleBoxL->addWidget(affRoleCb, 1);
    affLeftL->addWidget(affRoleBox);

    empOuter3L->addWidget(affLeft);

    // ── RIGHT : results panel ────────────────────────────────────
    QFrame* affRight = softBox();
    QVBoxLayout* affRightL = new QVBoxLayout(affRight);
    affRightL->setContentsMargins(12,12,12,12);
    affRightL->setSpacing(10);

    // Top bar: selected project name + "Affecter meilleur" button + status
    QFrame* affTopBar = new QFrame;
    affTopBar->setStyleSheet("QFrame{ background:transparent; border:none; }");
    QHBoxLayout* affTopBarL = new QHBoxLayout(affTopBar);
    affTopBarL->setContentsMargins(0,0,0,0);
    affTopBarL->setSpacing(10);

    QPushButton* affBestBtn = new QPushButton("  Affecter meilleur candidat");
    affBestBtn->setIcon(st->standardIcon(QStyle::SP_DialogYesButton));
    affBestBtn->setCursor(Qt::PointingHandCursor);
    affBestBtn->setEnabled(false);
    affBestBtn->setStyleSheet(QString(
        "QPushButton{ background:%1; color:rgba(255,255,255,0.93);"
        " border:none; border-radius:10px; padding:10px 18px; font-weight:900; font-size:13px; }"
        "QPushButton:hover{ background:%2; }"
        "QPushButton:disabled{ background:rgba(10,95,88,0.20); color:rgba(255,255,255,0.40); }"
    ).arg(C_PRIMARY, C_TOPBAR));

    QPushButton* affResultsBtn = new QPushButton("  Results");
    affResultsBtn->setIcon(st->standardIcon(QStyle::SP_FileDialogDetailedView));
    affResultsBtn->setCursor(Qt::PointingHandCursor);
    affResultsBtn->setStyleSheet(
        "QPushButton{ background:rgba(255,255,255,0.70); color:rgba(0,0,0,0.72);"
        " border:1px solid rgba(0,0,0,0.13); border-radius:10px;"
        " padding:10px 14px; font-weight:900; font-size:12px; }"
        "QPushButton:hover{ background:rgba(255,255,255,0.86); }"
    );

    QLabel* affStatusLbl = new QLabel("Sélectionnez un projet à gauche");
    affStatusLbl->setStyleSheet("color:rgba(0,0,0,0.40); font-size:12px; font-weight:700; font-style:italic;");

    affTopBarL->addWidget(affBestBtn);
    affTopBarL->addWidget(affResultsBtn);
    affTopBarL->addStretch(1);
    affTopBarL->addWidget(affStatusLbl);
    affRightL->addWidget(affTopBar);

    // Separator
    QFrame* affSep = new QFrame;
    affSep->setFrameShape(QFrame::HLine);
    affSep->setStyleSheet("color: rgba(10,95,88,0.20);");
    affRightL->addWidget(affSep);

    // Scroll area for cards
    QScrollArea* affScroll = new QScrollArea;
    affScroll->setWidgetResizable(true);
    affScroll->setFrameShape(QFrame::NoFrame);
    affScroll->setStyleSheet("QScrollArea{ background:transparent; border:none; }");

    QWidget* affResultsCtr = new QWidget;
    affResultsCtr->setStyleSheet("background:transparent;");
    QVBoxLayout* affResultsL = new QVBoxLayout(affResultsCtr);
    affResultsL->setContentsMargins(0,0,6,0);
    affResultsL->setSpacing(8);
    affResultsL->setAlignment(Qt::AlignTop);

    QLabel* affPlaceholder = new QLabel("Cliquez sur un projet dans l'arbre\npour afficher les meilleurs candidats.");
    affPlaceholder->setAlignment(Qt::AlignCenter);
    affPlaceholder->setWordWrap(true);
    affPlaceholder->setStyleSheet("color:rgba(0,0,0,0.28); font-size:14px; font-weight:700; padding:50px 20px;");
    affResultsL->addWidget(affPlaceholder);

    affScroll->setWidget(affResultsCtr);
    affRightL->addWidget(affScroll, 1);

    // Bottom info bar
    QFrame* affInfoBar = new QFrame;
    affInfoBar->setStyleSheet(
        "QFrame{ background:rgba(10,95,88,0.07); border:1px solid rgba(10,95,88,0.15); border-radius:8px; }");
    QHBoxLayout* affInfoBarL = new QHBoxLayout(affInfoBar);
    affInfoBarL->setContentsMargins(12,7,12,7);
    affInfoBarL->setSpacing(16);
    auto mkInfoChip = [](const QString& icon, const QString& text) -> QLabel* {
        QLabel* l = new QLabel(icon + "  " + text);
        l->setStyleSheet("color:rgba(10,95,88,0.75); font-weight:700; font-size:11px; background:transparent; border:none;");
        return l;
    };
    affInfoBarL->addWidget(mkInfoChip("ⓘ", "Affectation par projet"));
    affInfoBarL->addWidget(mkInfoChip("⊕", "Affectation en base en temps réel"));
    affInfoBarL->addStretch(1);
    affRightL->addWidget(affInfoBar);

    empOuter3L->addWidget(affRight, 1);
    emp3->addWidget(empOuter3, 1);

    // ── Load projects into tree ─────────────────────────────────
    auto affLoadProjects = [=](){
        affProjTree->clear();
        QTreeWidgetItem* root = new QTreeWidgetItem(affProjTree);
        root->setText(0, "Projets");
        root->setIcon(0, st->standardIcon(QStyle::SP_DirOpenIcon));
        root->setExpanded(true);
        root->setFlags(Qt::ItemIsEnabled); // not selectable

        QSqlQuery q;
        if (q.exec("SELECT \"Id_projet\", \"nom_du_projet\", NVL(\"domaine_de_recherche\",'') "
                   "FROM \"projet\" ORDER BY \"nom_du_projet\"")) {
            while (q.next()) {
                QTreeWidgetItem* item = new QTreeWidgetItem(root);
                item->setText(0, q.value(1).toString());
                item->setIcon(0, st->standardIcon(QStyle::SP_FileIcon));
                item->setData(0, Qt::UserRole,   q.value(0).toInt());   // projet id
                item->setData(0, Qt::UserRole+1, q.value(2).toString()); // domaine
            }
        }
        if (root->childCount() == 0) {
            QTreeWidgetItem* empty = new QTreeWidgetItem(root);
            empty->setText(0, "Aucun projet");
            empty->setFlags(Qt::NoItemFlags);
        }
    };

    auto affOpenResultsDialog = [=](){
        QDialog* dlg = new QDialog(this);
        dlg->setWindowTitle("Résultats d'affectation — Employés / Projets");
        dlg->setMinimumSize(980, 620);
        dlg->setModal(true);

        QVBoxLayout* dl = new QVBoxLayout(dlg);
        dl->setContentsMargins(14, 14, 14, 14);
        dl->setSpacing(10);

        QFrame* topBar = new QFrame;
        topBar->setStyleSheet("QFrame{ background:rgba(255,255,255,0.65); border:1px solid rgba(0,0,0,0.10); border-radius:10px; }");
        QHBoxLayout* topBarL = new QHBoxLayout(topBar);
        topBarL->setContentsMargins(10, 8, 10, 8);
        topBarL->setSpacing(8);

        QLabel* title = new QLabel("Affectations enregistrées");
        title->setStyleSheet("color:rgba(0,0,0,0.70); font-weight:900; font-size:12px;");

        QLabel* summary = new QLabel;
        summary->setStyleSheet("color:rgba(0,0,0,0.45); font-weight:700; font-size:11px;");

        QToolButton* refreshBtn = new QToolButton;
        refreshBtn->setAutoRaise(true);
        refreshBtn->setCursor(Qt::PointingHandCursor);
        refreshBtn->setIcon(st->standardIcon(QStyle::SP_BrowserReload));
        refreshBtn->setToolTip("Rafraîchir");

        topBarL->addWidget(title);
        topBarL->addStretch(1);
        topBarL->addWidget(summary);
        topBarL->addWidget(refreshBtn);
        dl->addWidget(topBar);

        QTableWidget* table = new QTableWidget;
        table->setColumnCount(6);
        table->setHorizontalHeaderLabels({"ID Projet", "Projet", "ID Employé", "Employé", "Rôle", "Spécialisation"});
        table->setColumnHidden(0, true);
        table->setColumnHidden(2, true);
        table->verticalHeader()->setVisible(false);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setAlternatingRowColors(true);
        table->setShowGrid(false);
        table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
        table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
        table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
        table->setStyleSheet(
            "QTableWidget{ background:rgba(255,255,255,0.90); border:1px solid rgba(0,0,0,0.09);"
            " border-radius:10px; color:rgba(0,0,0,0.78); }"
            "QHeaderView::section{ background:rgba(175,198,195,0.85); color:rgba(0,0,0,0.70);"
            " border:0; border-right:1px solid rgba(0,0,0,0.05); padding:8px; font-weight:900; }"
            "QTableWidget::item{ padding:6px 8px; }"
        );
        dl->addWidget(table, 1);

        auto loadAffResults = [=](){
            table->setRowCount(0);

            QSqlQuery q;
            const QString sql =
                "SELECT a.\"Id_projet\", "
                "       NVL(p.\"nom_du_projet\", 'Projet #' || TO_CHAR(a.\"Id_projet\")) AS projet_nom, "
                "       a.\"employee_id\", "
                "       NVL(e.\"FULL_NAME\", TRIM(e.\"prenom\" || ' ' || e.\"nom\")) AS employe_nom, "
                "       NVL(e.\"ROLE\", '') AS role_nom, "
                "       NVL(e.\"specialization\", '') AS spec "
                "FROM \"Associer\" a "
                "LEFT JOIN \"projet\" p ON p.\"Id_projet\" = a.\"Id_projet\" "
                "LEFT JOIN \"Employés\" e ON e.\"employee_id\" = a.\"employee_id\" "
                "WHERE LOWER(NVL(e.\"ROLE\", 'chercheur')) IN ('chercheur','technicien') "
                "ORDER BY LOWER(projet_nom), LOWER(employe_nom), a.\"Id_projet\", a.\"employee_id\"";

            if (!q.exec(sql)) {
                summary->setText("Erreur DB");
                showToast(dlg, "Erreur chargement résultats : " + q.lastError().text(), false);
                return;
            }

            while (q.next()) {
                const int row = table->rowCount();
                table->insertRow(row);

                auto mk = [](const QString& t){
                    QTableWidgetItem* it = new QTableWidgetItem(t);
                    it->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                    return it;
                };

                table->setItem(row, 0, mk(q.value(0).toString()));
                table->setItem(row, 1, mk(q.value(1).toString()));
                table->setItem(row, 2, mk(q.value(2).toString()));
                table->setItem(row, 3, mk(q.value(3).toString()));
                table->setItem(row, 4, mk(q.value(4).toString()));
                table->setItem(row, 5, mk(q.value(5).toString()));
                table->setRowHeight(row, 34);
            }

            summary->setText(QString("%1 affectation(s)").arg(table->rowCount()));
        };

        QFrame* bottomBar = new QFrame;
        bottomBar->setStyleSheet("QFrame{ background:rgba(255,255,255,0.65); border:1px solid rgba(0,0,0,0.10); border-radius:10px; }");
        QHBoxLayout* bottomBarL = new QHBoxLayout(bottomBar);
        bottomBarL->setContentsMargins(10, 8, 10, 8);
        bottomBarL->setSpacing(8);

        QPushButton* exportPdfBtn = actionBtn("Exporter PDF", "rgba(10,95,88,0.45)", "rgba(255,255,255,0.92)",
                                              st->standardIcon(QStyle::SP_DialogSaveButton), true);
        QPushButton* exportCsvBtn = actionBtn("Exporter CSV", "rgba(255,255,255,0.55)", C_TEXT_DARK,
                                              st->standardIcon(QStyle::SP_FileDialogDetailedView), true);
        QPushButton* closeBtn = actionBtn("Fermer", "rgba(255,255,255,0.55)", C_TEXT_DARK,
                                          st->standardIcon(QStyle::SP_DialogCloseButton), true);

        bottomBarL->addWidget(exportPdfBtn);
        bottomBarL->addWidget(exportCsvBtn);
        bottomBarL->addStretch(1);
        bottomBarL->addWidget(closeBtn);
        dl->addWidget(bottomBar);

        QObject::connect(refreshBtn, &QToolButton::clicked, dlg, [=](){
            loadAffResults();
        });

        QObject::connect(exportPdfBtn, &QPushButton::clicked, dlg, [=](){
            if (table->rowCount() == 0) {
                showToast(dlg, "Aucune affectation à exporter.", false);
                return;
            }

            QString fileName = QFileDialog::getSaveFileName(
                dlg,
                "Exporter les affectations en PDF",
                QString("affectations_employes_%1.pdf").arg(QDate::currentDate().toString("yyyyMMdd")),
                "PDF Files (*.pdf)");
            if (fileName.isEmpty()) return;
            if (!fileName.endsWith(".pdf", Qt::CaseInsensitive)) fileName += ".pdf";

            QString html;
            html += "<h2 style='color:#0A5F58;'>Résultats d'affectation intelligente</h2>";
            html += "<p style='color:#555;'>Généré le " + QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm") + "</p>";
            html += "<table border='1' cellspacing='0' cellpadding='6' width='100%'>";
            html += "<tr style='background:#e8f2f1;'>"
                    "<th>ID Projet</th><th>Projet</th><th>ID Employé</th><th>Employé</th><th>Rôle</th><th>Spécialisation</th></tr>";

            for (int r = 0; r < table->rowCount(); ++r) {
                html += "<tr>";
                for (int c = 0; c < table->columnCount(); ++c) {
                    const QString cell = table->item(r, c) ? table->item(r, c)->text().toHtmlEscaped() : QString();
                    html += "<td>" + cell + "</td>";
                }
                html += "</tr>";
            }
            html += "</table>";

            QPrinter printer;
            printer.setOutputFormat(QPrinter::PdfFormat);
            printer.setPageOrientation(QPageLayout::Landscape);
            printer.setOutputFileName(fileName);

            QTextDocument doc;
            doc.setHtml(html);
            doc.print(&printer);

            showToast(dlg, "PDF exporté : " + fileName, true);
        });

        QObject::connect(exportCsvBtn, &QPushButton::clicked, dlg, [=](){
            if (table->rowCount() == 0) {
                showToast(dlg, "Aucune affectation à exporter.", false);
                return;
            }

            QString fileName = QFileDialog::getSaveFileName(
                dlg,
                "Exporter les affectations en CSV",
                QString("affectations_employes_%1.csv").arg(QDate::currentDate().toString("yyyyMMdd")),
                "CSV Files (*.csv)");
            if (fileName.isEmpty()) return;
            if (!fileName.endsWith(".csv", Qt::CaseInsensitive)) fileName += ".csv";

            QFile f(fileName);
            if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                showToast(dlg, "Impossible d'écrire le fichier CSV.", false);
                return;
            }

            QTextStream ts(&f);
            ts.setEncoding(QStringConverter::Utf8);
            ts << "ID Projet;Projet;ID Employe;Employe;Role;Specialisation\n";
            for (int r = 0; r < table->rowCount(); ++r) {
                QStringList vals;
                for (int c = 0; c < table->columnCount(); ++c) {
                    QString v = table->item(r, c) ? table->item(r, c)->text() : QString();
                    v.replace('"', "\"\"");
                    vals << QString("\"%1\"").arg(v);
                }
                ts << vals.join(';') << "\n";
            }
            f.close();

            showToast(dlg, "CSV exporté : " + fileName, true);
        });

        QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);

        loadAffResults();
        dlg->exec();
        dlg->deleteLater();
    };

    // ── Build one candidate card ────────────────────────────────
    auto affBuildCard = [=](const SmartEmpSuggestion& s, int projId, int rank) -> QFrame* {
        QFrame* card = new QFrame;
        card->setFrameShape(QFrame::NoFrame);

        const QColor barC = (s.matchPercent >= 75) ? QColor("#2e7d32") :
                            (s.matchPercent >= 45) ? QColor("#e65100") : QColor("#b71c1c");

        card->setStyleSheet(QString(
            "QFrame{ background:rgba(255,255,255,0.85);"
            " border:1px solid rgba(0,0,0,0.09);"
            " border-left: 5px solid %1;"
            " border-radius: 11px; }").arg(barC.name()));

        QHBoxLayout* cl = new QHBoxLayout(card);
        cl->setContentsMargins(14, 10, 12, 10);
        cl->setSpacing(14);

        // Rank circle
        QLabel* rankLbl = new QLabel(QString("#%1").arg(rank));
        rankLbl->setFixedSize(34, 34);
        rankLbl->setAlignment(Qt::AlignCenter);
        rankLbl->setStyleSheet(
            "QLabel{ background:rgba(10,95,88,0.10); color:rgba(10,95,88,0.80);"
            " border-radius:17px; font-size:11px; font-weight:900; border:none; }");

        // Score badge
        QLabel* badge = new QLabel(QString("%1%").arg(s.matchPercent));
        badge->setFixedSize(52, 52);
        badge->setAlignment(Qt::AlignCenter);
        badge->setStyleSheet(QString(
            "QLabel{ background:%1; color:white; border-radius:26px;"
            " font-size:14px; font-weight:900; border:none; }").arg(barC.name()));

        // Info
        QWidget* info = new QWidget;
        QVBoxLayout* il = new QVBoxLayout(info);
        il->setContentsMargins(0,0,0,0);
        il->setSpacing(2);

        QLabel* nameLbl = new QLabel(s.fullName);
        nameLbl->setStyleSheet("color:rgba(0,0,0,0.88); font-size:13px; font-weight:900;");

        // Role pill
        QFrame* rolePill = new QFrame;
        rolePill->setStyleSheet(QString(
            "QFrame{ background:%1; border-radius:8px; padding:0 6px; }")
            .arg(s.role.toLower().contains("chercheur") ? "rgba(99,102,241,0.15)" :
                 s.role.toLower().contains("technicien") ? "rgba(234,88,12,0.15)" : "rgba(10,95,88,0.12)"));
        QHBoxLayout* rpL = new QHBoxLayout(rolePill);
        rpL->setContentsMargins(8,3,8,3);
        rpL->setSpacing(6);
        QLabel* roleTxt = new QLabel(s.role);
        roleTxt->setStyleSheet(QString(
            "color:%1; font-size:11px; font-weight:900; background:transparent; border:none;")
            .arg(s.role.toLower().contains("chercheur") ? "#4338ca" :
                 s.role.toLower().contains("technicien") ? "#c2410c" : "rgba(10,95,88,0.90)"));
        QLabel* specTxt = new QLabel(s.specialization.isEmpty() ? "" : "  •  " + s.specialization);
        specTxt->setStyleSheet("color:rgba(0,0,0,0.45); font-size:11px; font-weight:700; background:transparent; border:none;");
        rpL->addWidget(roleTxt);
        if (!s.specialization.isEmpty()) rpL->addWidget(specTxt);

        // Score breakdown bar
        QFrame* barBg = new QFrame;
        barBg->setFixedHeight(5);
        barBg->setStyleSheet("QFrame{ background:rgba(0,0,0,0.08); border-radius:3px; border:none; }");
        QFrame* barFg = new QFrame(barBg);
        barFg->setFixedHeight(5);
        barFg->setStyleSheet(QString("QFrame{ background:%1; border-radius:3px; border:none; }").arg(barC.name()));

        QLabel* detLbl = new QLabel(
            QString("Projets actifs : <b>%1</b>   ·   %2").arg(s.activeProjects).arg(s.explanation));
        detLbl->setStyleSheet("color:rgba(0,0,0,0.38); font-size:10px; background:transparent; border:none;");
        detLbl->setTextFormat(Qt::RichText);

        il->addWidget(nameLbl);
        il->addWidget(rolePill);
        il->addWidget(detLbl);

        // Affecter button
        QPushButton* affBtn = new QPushButton("  Affecter");
        affBtn->setIcon(st->standardIcon(QStyle::SP_DialogYesButton));
        affBtn->setCursor(Qt::PointingHandCursor);
        affBtn->setFixedHeight(36);
        affBtn->setFixedWidth(100);
        affBtn->setStyleSheet(
            "QPushButton{ background:rgba(10,95,88,0.12); color:rgba(10,95,88,1.0);"
            " border:1.5px solid rgba(10,95,88,0.30); border-radius:9px;"
            " padding:4px 10px; font-weight:900; font-size:12px; }"
            "QPushButton:hover{ background:rgba(10,95,88,0.25); border-color:rgba(10,95,88,0.60); }"
        );

        const int empId       = s.employeeId;
        const QString empName = s.fullName;
        QObject::connect(affBtn, &QPushButton::clicked, affBtn, [=](){
            QSqlQuery chk;
            chk.prepare("SELECT COUNT(1) FROM \"Associer\" WHERE \"employee_id\"=:eid AND \"Id_projet\"=:pid");
            chk.bindValue(":eid", empId);
            chk.bindValue(":pid", projId);
            if (chk.exec() && chk.next() && chk.value(0).toInt() > 0) {
                affBtn->setText("  Déjà affecté");
                affBtn->setEnabled(false);
                affBtn->setStyleSheet(
                    "QPushButton{ background:rgba(100,100,100,0.10); color:rgba(0,0,0,0.35);"
                    " border:1.5px solid rgba(0,0,0,0.15); border-radius:9px;"
                    " padding:4px 10px; font-weight:900; font-size:12px; }"
                );
                return;
            }
            QSqlQuery ins;
            ins.prepare("INSERT INTO \"Associer\" (\"employee_id\", \"Id_projet\") VALUES (:eid, :pid)");
            ins.bindValue(":eid", empId);
            ins.bindValue(":pid", projId);
            if (!ins.exec()) {
                showToast(affBtn->window(), "Impossible d'affecter : " + ins.lastError().text(), false);
            } else {
                affBtn->setText("  Affecté ✓");
                affBtn->setEnabled(false);
                affBtn->setStyleSheet(
                    "QPushButton{ background:rgba(46,111,99,0.18); color:rgba(46,111,99,1.0);"
                    " border:1.5px solid rgba(46,111,99,0.40); border-radius:9px;"
                    " padding:4px 10px; font-weight:900; font-size:12px; }"
                );
                showToast(affBtn->window(), empName + " affecté au projet.", true);
            }
        });

        cl->addWidget(rankLbl);
        cl->addWidget(badge);
        cl->addWidget(info, 1);
        cl->addWidget(affBtn);

        // Resize bar after card is laid out
        QObject::connect(card, &QFrame::destroyed, [barFg]{});
        QTimer::singleShot(0, barBg, [barBg, barFg, pct = s.matchPercent](){
            barFg->setFixedWidth(barBg->width() * pct / 100);
        });

        return card;
    };

    // ── Core: run scoring and populate cards ────────────────────
    auto affRunAnalysis = [=](int projId, const QString& domaine){
        // Clear previous results
        while (affResultsL->count()) {
            QLayoutItem* it = affResultsL->takeAt(0);
            if (it->widget()) it->widget()->deleteLater();
            delete it;
        }

        const QString role = (affRoleCb->currentIndex() == 0) ? "" : affRoleCb->currentText();
        QVector<SmartEmpSuggestion> suggestions;
        QString err;
        if (!loadSmartProjectSuggestions(projId, domaine, role, suggestions, &err)) {
            affStatusLbl->setText("Erreur DB");
            affBestBtn->setEnabled(false);
            return;
        }
        if (suggestions.isEmpty()) {
            QLabel* empty = new QLabel("Aucun candidat disponible pour ce projet.");
            empty->setAlignment(Qt::AlignCenter);
            empty->setStyleSheet("color:rgba(0,0,0,0.30); font-size:13px; padding:40px;");
            affResultsL->addWidget(empty);
            affStatusLbl->setText("0 candidat disponible");
            affBestBtn->setEnabled(false);
            return;
        }
        const int shown = qMin((int)suggestions.size(), 10);
        affStatusLbl->setText(QString("%1 candidat(s) — classés par score").arg(shown));
        affBestBtn->setEnabled(true);

        // Wire "Affecter meilleur" to the top candidate
        const SmartEmpSuggestion best = suggestions[0];
        QObject::disconnect(affBestBtn, &QPushButton::clicked, nullptr, nullptr);
        QObject::connect(affBestBtn, &QPushButton::clicked, affBestBtn, [=](){
            QSqlQuery chk;
            chk.prepare("SELECT COUNT(1) FROM \"Associer\" WHERE \"employee_id\"=:eid AND \"Id_projet\"=:pid");
            chk.bindValue(":eid", best.employeeId);
            chk.bindValue(":pid", projId);
            if (chk.exec() && chk.next() && chk.value(0).toInt() > 0) {
                showToast(affBestBtn->window(), best.fullName + " est déjà affecté à ce projet.", false);
                affBestBtn->setEnabled(false);
                return;
            }
            QSqlQuery ins;
            ins.prepare("INSERT INTO \"Associer\" (\"employee_id\", \"Id_projet\") VALUES (:eid, :pid)");
            ins.bindValue(":eid", best.employeeId);
            ins.bindValue(":pid", projId);
            if (!ins.exec())
                showToast(affBestBtn->window(), "Impossible d'affecter : " + ins.lastError().text(), false);
            else {
                showToast(affBestBtn->window(), best.fullName + " (meilleur score) affecté.", true);
                affBestBtn->setEnabled(false);
            }
        });

        for (int i = 0; i < shown; ++i)
            affResultsL->addWidget(affBuildCard(suggestions[i], projId, i + 1));
    };

    // ── Tree selection → auto-trigger analysis ──────────────────
    QObject::connect(affProjTree, &QTreeWidget::currentItemChanged, affProjTree,
        [=](QTreeWidgetItem* cur, QTreeWidgetItem*){
            if (!cur) return;
            const int projId = cur->data(0, Qt::UserRole).toInt();
            if (projId <= 0) return;
            const QString domaine = cur->data(0, Qt::UserRole+1).toString();
            affStatusLbl->setText("Analyse en cours…");
            affRunAnalysis(projId, domaine);
        });

    // Role filter change → re-run analysis for current selection
    QObject::connect(affRoleCb, QOverload<int>::of(&QComboBox::currentIndexChanged), affRoleCb, [=](int){
        QTreeWidgetItem* cur = affProjTree->currentItem();
        if (!cur) return;
        const int projId = cur->data(0, Qt::UserRole).toInt();
        if (projId <= 0) return;
        affRunAnalysis(projId, cur->data(0, Qt::UserRole+1).toString());
    });

    // Refresh button
    QObject::connect(affRefreshBtn, &QToolButton::clicked, affRefreshBtn, [=](){
        affLoadProjects();
        affBestBtn->setEnabled(false);
        affStatusLbl->setText("Sélectionnez un projet à gauche");
        while (affResultsL->count()) {
            QLayoutItem* it = affResultsL->takeAt(0);
            if (it->widget()) it->widget()->deleteLater();
            delete it;
        }
        affResultsL->addWidget(affPlaceholder);
    });

    QObject::connect(affResultsBtn, &QPushButton::clicked, affResultsBtn, [=](){
        affOpenResultsDialog();
    });

    emp3->addWidget(empOuter3, 1);

    QFrame* empBottom3 = new QFrame;
    empBottom3->setFixedHeight(64);
    empBottom3->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* empBottom3L = new QHBoxLayout(empBottom3);
    empBottom3L->setContentsMargins(14,10,14,10);
    empBottom3L->setSpacing(10);

    QPushButton* empBack3 = actionBtn("Retour", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_ArrowBack), true);
    empBottom3L->addWidget(empBack3);
    empBottom3L->addStretch(1);

    // Mode-switch tabs
    auto modeTabStyle = [](bool active) -> QString {
        return active
            ? "QPushButton{ background:rgba(10,95,88,0.85); color:white; border:none;"
              " border-radius:9px; padding:8px 18px; font-weight:900; font-size:12px; }"
            : "QPushButton{ background:rgba(255,255,255,0.55); color:rgba(0,0,0,0.55); border:1px solid rgba(0,0,0,0.12);"
              " border-radius:9px; padding:8px 18px; font-weight:800; font-size:12px; }"
              "QPushButton:hover{ background:rgba(10,95,88,0.12); }";
    };
    QPushButton* tabProjBtn = new QPushButton("🗂  Projet");
    tabProjBtn->setCursor(Qt::PointingHandCursor);
    tabProjBtn->setStyleSheet(modeTabStyle(true));
    empBottom3L->addWidget(tabProjBtn);

    QObject::connect(tabProjBtn, &QPushButton::clicked, this, [=](){
        tabProjBtn->setStyleSheet(modeTabStyle(true));
        stack->setCurrentIndex(EMP_AFF);
    });

    emp3->addWidget(empBottom3);
    addStackPage(empAffPage);

    // ==========================================================
    // PAGE 27 : Employés - AFFECTATION INTELLIGENTE — EXPÉRIENCE (EMP_AFF_EXP)
    // ==========================================================
    QWidget* empAffExpPage = new QWidget;
    QVBoxLayout* empExpL = new QVBoxLayout(empAffExpPage);
    empExpL->setContentsMargins(22, 18, 22, 18);
    empExpL->setSpacing(14);

    ModulesBar barEmpAffExp;
    empExpL->addWidget(makeHeaderBlock(st, "Affectation Intelligente — Expérience", ModuleTab::Employee, &barEmpAffExp));
    connectModulesSwitch(this, stack, barEmpAffExp);

    QFrame* empExpOuter = new QFrame;
    empExpOuter->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius:14px; }").arg(C_PANEL_BG, C_PANEL_BR));
    QHBoxLayout* empExpOuterL = new QHBoxLayout(empExpOuter);
    empExpOuterL->setContentsMargins(12,12,12,12);
    empExpOuterL->setSpacing(12);

    // ── LEFT: experience tree ────────────────────────────────────
    QFrame* expAffLeft = softBox();
    expAffLeft->setFixedWidth(260);
    QVBoxLayout* expAffLeftL = new QVBoxLayout(expAffLeft);
    expAffLeftL->setContentsMargins(10,10,10,10);
    expAffLeftL->setSpacing(8);

    QFrame* expAffHdr = new QFrame;
    expAffHdr->setStyleSheet("QFrame{ background:rgba(255,255,255,0.70); border:1px solid rgba(0,0,0,0.10); border-radius:10px; }");
    QHBoxLayout* expAffHdrL = new QHBoxLayout(expAffHdr);
    expAffHdrL->setContentsMargins(10,6,8,6);
    expAffHdrL->setSpacing(6);
    QLabel* expAffTitle = new QLabel("Expériences disponibles");
    expAffTitle->setStyleSheet("color:rgba(0,0,0,0.65); font-weight:900; font-size:12px;");
    QToolButton* expAffRefreshBtn = new QToolButton;
    expAffRefreshBtn->setAutoRaise(true);
    expAffRefreshBtn->setIcon(st->standardIcon(QStyle::SP_BrowserReload));
    expAffRefreshBtn->setCursor(Qt::PointingHandCursor);
    expAffHdrL->addWidget(expAffTitle, 1);
    expAffHdrL->addWidget(expAffRefreshBtn);
    expAffLeftL->addWidget(expAffHdr);

    QTreeWidget* expAffTree = new QTreeWidget;
    expAffTree->setHeaderHidden(true);
    expAffTree->setIndentation(16);
    expAffTree->setAnimated(true);
    expAffTree->setStyleSheet(
        "QTreeWidget{ border:none; background:transparent; outline:none; }"
        "QTreeWidget::item{ padding:7px 4px; border-radius:7px; font-size:12px; color:rgba(0,0,0,0.70); }"
        "QTreeWidget::item:selected{ background:rgba(10,95,88,0.18); color:rgba(10,95,88,0.95); font-weight:900; }"
        "QTreeWidget::item:hover{ background:rgba(10,95,88,0.09); }"
        "QTreeWidget::branch{ background:transparent; }"
    );
    expAffLeftL->addWidget(expAffTree, 1);

    // Role filter
    QFrame* expRoleBox = new QFrame;
    expRoleBox->setStyleSheet("QFrame{ background:rgba(255,255,255,0.60); border:1px solid rgba(0,0,0,0.09); border-radius:9px; }");
    QHBoxLayout* expRoleBoxL = new QHBoxLayout(expRoleBox);
    expRoleBoxL->setContentsMargins(10,6,10,6);
    expRoleBoxL->setSpacing(8);
    QLabel* expRoleLbl = new QLabel("Rôle :");
    expRoleLbl->setStyleSheet("color:rgba(0,0,0,0.55); font-weight:900; font-size:11px;");
    QComboBox* expRoleCb = new QComboBox;
    expRoleCb->addItems({"Tous", "Chercheur", "Technicien", "Responsable"});
    expRoleCb->setStyleSheet(
        "QComboBox{ background:rgba(255,255,255,0.90); border:1px solid rgba(0,0,0,0.12);"
        " border-radius:8px; padding:5px 10px; font-weight:800; font-size:11px; color:rgba(0,0,0,0.65); }"
        "QComboBox::drop-down{ border:0; padding-right:6px; }"
    );
    expRoleBoxL->addWidget(expRoleLbl);
    expRoleBoxL->addWidget(expRoleCb, 1);
    expAffLeftL->addWidget(expRoleBox);
    empExpOuterL->addWidget(expAffLeft);

    // ── RIGHT: results ───────────────────────────────────────────
    QFrame* expAffRight = softBox();
    QVBoxLayout* expAffRightL = new QVBoxLayout(expAffRight);
    expAffRightL->setContentsMargins(12,12,12,12);
    expAffRightL->setSpacing(10);

    QFrame* expTopBar = new QFrame;
    expTopBar->setStyleSheet("QFrame{ background:transparent; border:none; }");
    QHBoxLayout* expTopBarL = new QHBoxLayout(expTopBar);
    expTopBarL->setContentsMargins(0,0,0,0);
    expTopBarL->setSpacing(10);

    QPushButton* expBestBtn = new QPushButton("  Affecter meilleur candidat");
    expBestBtn->setIcon(st->standardIcon(QStyle::SP_DialogYesButton));
    expBestBtn->setCursor(Qt::PointingHandCursor);
    expBestBtn->setEnabled(false);
    expBestBtn->setStyleSheet(QString(
        "QPushButton{ background:%1; color:rgba(255,255,255,0.93);"
        " border:none; border-radius:10px; padding:10px 18px; font-weight:900; font-size:13px; }"
        "QPushButton:hover{ background:%2; }"
        "QPushButton:disabled{ background:rgba(10,95,88,0.20); color:rgba(255,255,255,0.40); }"
    ).arg(C_PRIMARY, C_TOPBAR));

    QLabel* expStatusLbl = new QLabel("Sélectionnez une expérience à gauche");
    expStatusLbl->setStyleSheet("color:rgba(0,0,0,0.40); font-size:12px; font-weight:700; font-style:italic;");

    expTopBarL->addWidget(expBestBtn);
    expTopBarL->addStretch(1);
    expTopBarL->addWidget(expStatusLbl);
    expAffRightL->addWidget(expTopBar);

    QFrame* expSep = new QFrame;
    expSep->setFrameShape(QFrame::HLine);
    expSep->setStyleSheet("color: rgba(10,95,88,0.20);");
    expAffRightL->addWidget(expSep);

    QScrollArea* expAffScroll = new QScrollArea;
    expAffScroll->setWidgetResizable(true);
    expAffScroll->setFrameShape(QFrame::NoFrame);
    expAffScroll->setStyleSheet("QScrollArea{ background:transparent; border:none; }");

    QWidget* expResultsCtr = new QWidget;
    expResultsCtr->setStyleSheet("background:transparent;");
    QVBoxLayout* expResultsL = new QVBoxLayout(expResultsCtr);
    expResultsL->setContentsMargins(0,0,6,0);
    expResultsL->setSpacing(8);
    expResultsL->setAlignment(Qt::AlignTop);

    QLabel* expPlaceholder = new QLabel("Cliquez sur une expérience dans l'arbre\npour afficher les meilleurs candidats.");
    expPlaceholder->setAlignment(Qt::AlignCenter);
    expPlaceholder->setWordWrap(true);
    expPlaceholder->setStyleSheet("color:rgba(0,0,0,0.28); font-size:14px; font-weight:700; padding:50px 20px;");
    expResultsL->addWidget(expPlaceholder);

    expAffScroll->setWidget(expResultsCtr);
    expAffRightL->addWidget(expAffScroll, 1);

    // Info bar
    QFrame* expInfoBar = new QFrame;
    expInfoBar->setStyleSheet("QFrame{ background:rgba(10,95,88,0.07); border:1px solid rgba(10,95,88,0.15); border-radius:8px; }");
    QHBoxLayout* expInfoBarL = new QHBoxLayout(expInfoBar);
    expInfoBarL->setContentsMargins(12,7,12,7);
    expInfoBarL->setSpacing(16);
    expInfoBarL->addWidget(mkInfoChip("ⓘ", "Affectation par expérience"));
    expInfoBarL->addWidget(mkInfoChip("⊕", "Score : Spéc 40% · Charge 35% · Rôle 25%"));
    expInfoBarL->addStretch(1);
    expAffRightL->addWidget(expInfoBar);

    empExpOuterL->addWidget(expAffRight, 1);
    empExpL->addWidget(empExpOuter, 1);

    // ── Load experiences into tree ───────────────────────────────
    auto expAffLoadExps = [=](){
        expAffTree->clear();
        QTreeWidgetItem* root = new QTreeWidgetItem(expAffTree);
        root->setText(0, "Expériences");
        root->setIcon(0, st->standardIcon(QStyle::SP_DirOpenIcon));
        root->setExpanded(true);
        root->setFlags(Qt::ItemIsEnabled);

        QSqlQuery q;
        if (q.exec("SELECT \"Id_exp\", \"Titre\", NVL(\"Type_Experience\",'') FROM \"Expérience\" ORDER BY \"Titre\"")) {
            while (q.next()) {
                QTreeWidgetItem* item = new QTreeWidgetItem(root);
                const QString titre = q.value(1).toString().trimmed();
                const QString type  = q.value(2).toString().trimmed();
                item->setText(0, titre.isEmpty() ? QString("Expérience #%1").arg(q.value(0).toInt()) : titre);
                item->setIcon(0, st->standardIcon(QStyle::SP_FileIcon));
                item->setData(0, Qt::UserRole,   q.value(0).toInt());  // Id_exp
                item->setData(0, Qt::UserRole+1, type);                 // Type_Experience
            }
        }
        if (root->childCount() == 0) {
            QTreeWidgetItem* empty = new QTreeWidgetItem(root);
            empty->setText(0, "Aucune expérience");
            empty->setFlags(Qt::NoItemFlags);
        }
    };

    // ── Build candidate card for experience ──────────────────────
    auto expBuildCard = [=](const SmartExpSuggestion& s, int rank) -> QFrame* {
        QFrame* card = new QFrame;
        card->setFrameShape(QFrame::NoFrame);
        const QColor barC = (s.matchPercent >= 75) ? QColor("#2e7d32") :
                            (s.matchPercent >= 45) ? QColor("#e65100") : QColor("#b71c1c");
        card->setStyleSheet(QString(
            "QFrame{ background:rgba(255,255,255,0.85);"
            " border:1px solid rgba(0,0,0,0.09);"
            " border-left:5px solid %1; border-radius:11px; }").arg(barC.name()));

        QHBoxLayout* cl = new QHBoxLayout(card);
        cl->setContentsMargins(14, 10, 12, 10);
        cl->setSpacing(14);

        QLabel* rankLbl = new QLabel(QString("#%1").arg(rank));
        rankLbl->setFixedSize(34, 34);
        rankLbl->setAlignment(Qt::AlignCenter);
        rankLbl->setStyleSheet("QLabel{ background:rgba(10,95,88,0.10); color:rgba(10,95,88,0.80);"
                               " border-radius:17px; font-size:11px; font-weight:900; border:none; }");

        QLabel* badge = new QLabel(QString("%1%").arg(s.matchPercent));
        badge->setFixedSize(52, 52);
        badge->setAlignment(Qt::AlignCenter);
        badge->setStyleSheet(QString("QLabel{ background:%1; color:white; border-radius:26px;"
                                     " font-size:14px; font-weight:900; border:none; }").arg(barC.name()));

        QWidget* info = new QWidget;
        QVBoxLayout* il = new QVBoxLayout(info);
        il->setContentsMargins(0,0,0,0);
        il->setSpacing(2);

        QLabel* nameLbl = new QLabel(s.fullName);
        nameLbl->setStyleSheet("color:rgba(0,0,0,0.88); font-size:13px; font-weight:900;");

        // Role pill
        QFrame* rolePill = new QFrame;
        rolePill->setStyleSheet(QString(
            "QFrame{ background:%1; border-radius:8px; }")
            .arg(s.role.toLower().contains("chercheur") ? "rgba(99,102,241,0.15)" :
                 s.role.toLower().contains("technicien") ? "rgba(234,88,12,0.15)" : "rgba(10,95,88,0.12)"));
        QHBoxLayout* rpL = new QHBoxLayout(rolePill);
        rpL->setContentsMargins(8,3,8,3); rpL->setSpacing(4);
        QLabel* roleTxt = new QLabel(s.role);
        roleTxt->setStyleSheet(QString("color:%1; font-size:11px; font-weight:900; background:transparent; border:none;")
            .arg(s.role.toLower().contains("chercheur") ? "#4338ca" :
                 s.role.toLower().contains("technicien") ? "#c2410c" : "rgba(10,95,88,0.90)"));
        QLabel* specTxt = new QLabel(s.specialization.isEmpty() ? "" : "  ·  " + s.specialization);
        specTxt->setStyleSheet("color:rgba(0,0,0,0.45); font-size:11px; background:transparent; border:none;");
        rpL->addWidget(roleTxt);
        if (!s.specialization.isEmpty()) rpL->addWidget(specTxt);

        QLabel* detLbl = new QLabel(
            QString("Projets actifs : <b>%1</b>   ·   %2").arg(s.activeProjects).arg(s.explanation));
        detLbl->setStyleSheet("color:rgba(0,0,0,0.38); font-size:10px; background:transparent; border:none;");
        detLbl->setTextFormat(Qt::RichText);

        il->addWidget(nameLbl);
        il->addWidget(rolePill);
        il->addWidget(detLbl);

        // Suggérer button (read-only recommendation — no DB write for experience)
        QPushButton* sugBtn = new QPushButton("  Suggérer");
        sugBtn->setIcon(st->standardIcon(QStyle::SP_MessageBoxInformation));
        sugBtn->setCursor(Qt::PointingHandCursor);
        sugBtn->setFixedHeight(36);
        sugBtn->setFixedWidth(110);
        sugBtn->setStyleSheet(
            "QPushButton{ background:rgba(10,95,88,0.12); color:rgba(10,95,88,1.0);"
            " border:1.5px solid rgba(10,95,88,0.30); border-radius:9px;"
            " padding:4px 10px; font-weight:900; font-size:12px; }"
            "QPushButton:hover{ background:rgba(10,95,88,0.25); }"
        );
        const QString empName = s.fullName;
        const int     empId   = s.employeeId;
        const int     pct     = s.matchPercent;
        QObject::connect(sugBtn, &QPushButton::clicked, sugBtn, [=](){
            sugBtn->setText("  Sélectionné ✓");
            sugBtn->setEnabled(false);
            sugBtn->setStyleSheet(
                "QPushButton{ background:rgba(46,111,99,0.18); color:rgba(46,111,99,1.0);"
                " border:1.5px solid rgba(46,111,99,0.40); border-radius:9px;"
                " padding:4px 10px; font-weight:900; font-size:12px; }"
            );
            Q_UNUSED(empId)
            showToast(sugBtn->window(),
                QString("%1 sélectionné — score %2%").arg(empName).arg(pct), true);
        });

        cl->addWidget(rankLbl);
        cl->addWidget(badge);
        cl->addWidget(info, 1);
        cl->addWidget(sugBtn);
        return card;
    };

    // ── Core analysis for experiences ────────────────────────────
    auto expRunAnalysis = [=](int expId, const QString& typeExp){
        while (expResultsL->count()) {
            QLayoutItem* it = expResultsL->takeAt(0);
            if (it->widget()) it->widget()->deleteLater();
            delete it;
        }
        const QString role = (expRoleCb->currentIndex() == 0) ? "" : expRoleCb->currentText();
        QVector<SmartExpSuggestion> suggestions;
        QString err;
        if (!loadSmartExpSuggestions(expId, typeExp, role, suggestions, &err)) {
            expStatusLbl->setText("Erreur DB");
            expBestBtn->setEnabled(false);
            return;
        }
        if (suggestions.isEmpty()) {
            QLabel* empty = new QLabel("Aucun candidat disponible.");
            empty->setAlignment(Qt::AlignCenter);
            empty->setStyleSheet("color:rgba(0,0,0,0.30); font-size:13px; padding:40px;");
            expResultsL->addWidget(empty);
            expStatusLbl->setText("0 candidat disponible");
            expBestBtn->setEnabled(false);
            return;
        }
        const int shown = qMin((int)suggestions.size(), 10);
        expStatusLbl->setText(QString("%1 candidat(s) — classés par score").arg(shown));
        expBestBtn->setEnabled(true);

        const SmartExpSuggestion best = suggestions[0];
        QObject::disconnect(expBestBtn, &QPushButton::clicked, nullptr, nullptr);
        QObject::connect(expBestBtn, &QPushButton::clicked, expBestBtn, [=](){
            showToast(expBestBtn->window(),
                QString("%1 (score %2%) — meilleur candidat pour cette expérience.")
                    .arg(best.fullName).arg(best.matchPercent), true);
            expBestBtn->setEnabled(false);
        });

        for (int i = 0; i < shown; ++i)
            expResultsL->addWidget(expBuildCard(suggestions[i], i + 1));
    };

    // Tree click → auto-analyse
    QObject::connect(expAffTree, &QTreeWidget::currentItemChanged, expAffTree,
        [=](QTreeWidgetItem* cur, QTreeWidgetItem*){
            if (!cur) return;
            const int expId = cur->data(0, Qt::UserRole).toInt();
            if (expId <= 0) return;
            const QString typeExp = cur->data(0, Qt::UserRole+1).toString();
            expStatusLbl->setText("Analyse en cours…");
            expRunAnalysis(expId, typeExp);
        });

    // Role filter change → re-run
    QObject::connect(expRoleCb, QOverload<int>::of(&QComboBox::currentIndexChanged), expRoleCb, [=](int){
        QTreeWidgetItem* cur = expAffTree->currentItem();
        if (!cur) return;
        const int expId = cur->data(0, Qt::UserRole).toInt();
        if (expId <= 0) return;
        expRunAnalysis(expId, cur->data(0, Qt::UserRole+1).toString());
    });

    // Refresh
    QObject::connect(expAffRefreshBtn, &QToolButton::clicked, expAffRefreshBtn, [=](){
        expAffLoadExps();
        expBestBtn->setEnabled(false);
        expStatusLbl->setText("Sélectionnez une expérience à gauche");
        while (expResultsL->count()) {
            QLayoutItem* it = expResultsL->takeAt(0);
            if (it->widget()) it->widget()->deleteLater();
            delete it;
        }
        expResultsL->addWidget(expPlaceholder);
    });

    // Bottom bar (mirrors EMP_AFF bottom bar with inverted tab state)
    QFrame* empExpBottom = new QFrame;
    empExpBottom->setFixedHeight(64);
    empExpBottom->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* empExpBottomL = new QHBoxLayout(empExpBottom);
    empExpBottomL->setContentsMargins(14,10,14,10);
    empExpBottomL->setSpacing(10);

    QPushButton* empBack3b = actionBtn("Retour", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_ArrowBack), true);
    empExpBottomL->addWidget(empBack3b);
    empExpBottomL->addStretch(1);

    // Mirror tabs — Expérience active
    QPushButton* tabProjBtn2 = new QPushButton("🗂  Projet");
    QPushButton* tabExpBtn2  = new QPushButton("🔬  Expérience");
    tabProjBtn2->setCursor(Qt::PointingHandCursor);
    tabExpBtn2->setCursor(Qt::PointingHandCursor);
    tabProjBtn2->setStyleSheet(modeTabStyle(false));
    tabExpBtn2->setStyleSheet(modeTabStyle(true));
    empExpBottomL->addWidget(tabProjBtn2);
    empExpBottomL->addWidget(tabExpBtn2);

    QObject::connect(tabProjBtn2, &QPushButton::clicked, this, [=](){
        tabProjBtn2->setStyleSheet(modeTabStyle(true));
        tabExpBtn2->setStyleSheet(modeTabStyle(false));
        stack->setCurrentIndex(EMP_AFF);
    });
    QObject::connect(tabExpBtn2, &QPushButton::clicked, this, [=](){
        tabProjBtn2->setStyleSheet(modeTabStyle(false));
        tabExpBtn2->setStyleSheet(modeTabStyle(true));
        stack->setCurrentIndex(EMP_AFF);
    });

    empExpL->addWidget(empExpBottom);

    // Load experiences on page init
    expAffLoadExps();

    // Wire back buttons
    QObject::connect(empBack3b, &QPushButton::clicked, this, [=]{
        setWindowTitle("Employés");
        stack->setCurrentIndex(EMP_LIST);
    });

    addStackPage(empAffExpPage);

    // ==========================================================
    // PAGE 21 : Employés - DISPONIBILITES (EMP_AVAIL)
    // ==========================================================
    QWidget* empAvailPage = new QWidget;
    QVBoxLayout* emp4 = new QVBoxLayout(empAvailPage);
    emp4->setContentsMargins(22, 18, 22, 18);
    emp4->setSpacing(14);

    ModulesBar barEmpAvail;
    emp4->addWidget(makeHeaderBlock(st, "Disponibilites & Contraintes", ModuleTab::Employee, &barEmpAvail));
    connectModulesSwitch(this, stack, barEmpAvail);

    QFrame* empOuter4 = new QFrame;
    empOuter4->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }").arg(C_PANEL_BG, C_PANEL_BR));
    QHBoxLayout* empOuter4L = new QHBoxLayout(empOuter4);
    empOuter4L->setContentsMargins(12,12,12,12);
    empOuter4L->setSpacing(12);

    QFrame* empLeft4 = softBox();
    empLeft4->setFixedWidth(310);
    QVBoxLayout* empLeft4L = new QVBoxLayout(empLeft4);
    empLeft4L->setContentsMargins(10,10,10,10);
    empLeft4L->setSpacing(10);

    QFrame* empDd4 = new QFrame;
    empDd4->setStyleSheet("QFrame{ background: rgba(255,255,255,0.72); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QHBoxLayout* empDd4L = new QHBoxLayout(empDd4);
    empDd4L->setContentsMargins(10,8,10,8);
    QLabel* empDd4T = new QLabel("Laboratoires");
    empDd4T->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    QToolButton* empDd4B = new QToolButton;
    empDd4B->setAutoRaise(true);
    empDd4B->setIcon(st->standardIcon(QStyle::SP_ArrowDown));
    empDd4B->setCursor(Qt::PointingHandCursor);
    empDd4L->addWidget(empDd4T);
    empDd4L->addStretch(1);
    empDd4L->addWidget(empDd4B);

    QTreeWidget* empTree4 = new QTreeWidget;
    empTree4->setHeaderHidden(true);
    empTree4->setIndentation(18);

    auto* empWf1 = new QTreeWidgetItem(empTree4, QStringList() << "Lab A");
    auto* empWf2 = new QTreeWidgetItem(empTree4, QStringList() << "Lab B");
    auto* empWf4 = new QTreeWidgetItem(empTree4, QStringList() << "Lab C");
    empWf1->setIcon(0, st->standardIcon(QStyle::SP_DirIcon));
    empWf2->setIcon(0, st->standardIcon(QStyle::SP_DirIcon));
    empWf4->setIcon(0, st->standardIcon(QStyle::SP_DirIcon));

    auto* empShA = new QTreeWidgetItem(empWf2, QStringList() << "Projet AI-BIO");
    auto* empShB = new QTreeWidgetItem(empWf2, QStringList() << "Projet PROTEO");
    auto* empSh6 = new QTreeWidgetItem(empWf2, QStringList() << "Projet GENOME");
    auto* empRm  = new QTreeWidgetItem(empWf2, QStringList() << "Techniciens");
    empShA->setIcon(0, st->standardIcon(QStyle::SP_FileIcon));
    empShB->setIcon(0, st->standardIcon(QStyle::SP_FileIcon));
    empSh6->setIcon(0, st->standardIcon(QStyle::SP_FileIcon));
    empRm ->setIcon(0, st->standardIcon(QStyle::SP_FileDialogInfoView));

    empTree4->expandAll();
    empTree4->setCurrentItem(empWf2);

    QFrame* empTemp4 = empInfoBlock(st, "Lab B • FT: 3 / PT: 2", "Total: 5");

    QPushButton* empExport4 = actionBtn("Exporter Liste", "rgba(10,95,88,0.45)", "rgba(255,255,255,0.92)", st->standardIcon(QStyle::SP_DialogSaveButton), true);
    QPushButton* empMark4   = actionBtn("Affecter au Projet", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_DialogApplyButton), true);

    empLeft4L->addWidget(empDd4);
    empLeft4L->addWidget(empTree4, 1);
    empLeft4L->addWidget(empTemp4);
    empLeft4L->addWidget(empExport4);
    empLeft4L->addWidget(empMark4);

    QFrame* empRight4 = softBox();
    QVBoxLayout* empRight4L = new QVBoxLayout(empRight4);
    empRight4L->setContentsMargins(10,10,10,10);
    empRight4L->setSpacing(10);

    QWidget* empFiltersRow = new QWidget;
    QHBoxLayout* empFr = new QHBoxLayout(empFiltersRow);
    empFr->setContentsMargins(0,0,0,0);
    empFr->setSpacing(10);
    empFr->addWidget(empFilterPill("Role"));
    empFr->addWidget(empFilterPill("Temps"));
    empFr->addWidget(empFilterPill("Lab"));
    empRight4L->addWidget(empFiltersRow);

    QFrame* empRackCard = new QFrame;
    empRackCard->setStyleSheet("QFrame{ background: rgba(255,255,255,0.55); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QVBoxLayout* empRackCardL = new QVBoxLayout(empRackCard);
    empRackCardL->setContentsMargins(12,12,12,12);
    QTableWidget* empAvailability = new QTableWidget;
    empSetupAvailabilityGrid(empAvailability);
    empRackCardL->addWidget(empAvailability);
    empRight4L->addWidget(empRackCard);

    QFrame* empAccCard = new QFrame;
    empAccCard->setStyleSheet("QFrame{ background: rgba(255,255,255,0.55); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QVBoxLayout* empAccCardL = new QVBoxLayout(empAccCard);
    empAccCardL->setContentsMargins(12,12,12,12);
    QLabel* empAccTitle = new QLabel("Contraintes");
    empAccTitle->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    QTableWidget* empAccTable = new QTableWidget;
    empSetupConstraintsTable(empAccTable);
    empAccCardL->addWidget(empAccTitle);
    empAccCardL->addWidget(empAccTable);
    empRight4L->addWidget(empAccCard, 1);

    QWidget* empBottomRight = new QWidget;
    QHBoxLayout* empBr = new QHBoxLayout(empBottomRight);
    empBr->setContentsMargins(0,0,0,0);
    empBr->setSpacing(10);
    empBr->addStretch(1);

    QPushButton* empBtnFolder = actionBtn("Lab", "rgba(255,255,255,0.72)", C_TEXT_DARK, st->standardIcon(QStyle::SP_DirIcon), true);
    QPushButton* empBtnSec    = actionBtn("Statistiques", "rgba(255,255,255,0.72)", C_TEXT_DARK, st->standardIcon(QStyle::SP_MessageBoxWarning), true);

    empBr->addWidget(empBtnFolder);
    empBr->addWidget(empBtnSec);
    empRight4L->addWidget(empBottomRight);

    empOuter4L->addWidget(empLeft4);
    empOuter4L->addWidget(empRight4, 1);

    emp4->addWidget(empOuter4, 1);

    QFrame* empBottom4 = new QFrame;
    empBottom4->setFixedHeight(64);
    empBottom4->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* empBottom4L = new QHBoxLayout(empBottom4);
    empBottom4L->setContentsMargins(14,10,14,10);

    QPushButton* empBack4 = actionBtn("Retour", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_ArrowBack), true);
    empBottom4L->addWidget(empBack4);
    empBottom4L->addStretch(1);

    emp4->addWidget(empBottom4);
    addStackPage(empAvailPage);

    // ==========================================================
    // PAGE 22 : Employés - STATISTIQUES (EMP_STATS)
    // ==========================================================
    QWidget* empStatsPage = new QWidget;
    QVBoxLayout* empS = new QVBoxLayout(empStatsPage);
    empS->setContentsMargins(22, 18, 22, 18);
    empS->setSpacing(14);

    ModulesBar barEmpStats;
    empS->addWidget(makeHeaderBlock(st, "Statistiques Employes", ModuleTab::Employee, &barEmpStats));
    connectModulesSwitch(this, stack, barEmpStats);

    QFrame* empOuterStats = new QFrame;
    empOuterStats->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }").arg(C_PANEL_BG, C_PANEL_BR));
    QVBoxLayout* empOuterStatsL = new QVBoxLayout(empOuterStats);
    empOuterStatsL->setContentsMargins(12,12,12,12);
    empOuterStatsL->setSpacing(12);

    QFrame* empDash = new QFrame;
    empDash->setStyleSheet("QFrame{ background: rgba(255,255,255,0.55); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QHBoxLayout* empDashL = new QHBoxLayout(empDash);
    empDashL->setContentsMargins(12,12,12,12);
    empDashL->setSpacing(12);

    QFrame* empDonutCard = softBox();
    QVBoxLayout* empDcL = new QVBoxLayout(empDonutCard);
    empDcL->setContentsMargins(12,12,12,12);
    QLabel* empTotalLbl = new QLabel("Total Employes:");
    empTotalLbl->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    DonutChart* empDonutStats = new DonutChart;
    empDcL->addWidget(empTotalLbl);
    empDcL->addWidget(empDonutStats, 1);

    QFrame* empLegendCard = softBox();
    QVBoxLayout* empLgL = new QVBoxLayout(empLegendCard);
    empLgL->setContentsMargins(12,12,12,12);
    empLgL->setSpacing(10);
    auto empLegendRow=[&](const QColor& c, const QString& t){
        QWidget* row = new QWidget;
        QHBoxLayout* h = new QHBoxLayout(row);
        h->setContentsMargins(0,0,0,0);
        h->setSpacing(10);
        QFrame* dot = new QFrame;
        dot->setFixedSize(12,12);
        dot->setStyleSheet(QString("background:%1; border-radius:6px;").arg(c.name()));
        QLabel* lab = new QLabel(t);
        lab->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
        h->addWidget(dot);
        h->addWidget(lab);
        h->addStretch(1);
        return row;
    };
    empLgL->addWidget(empLegendRow(W_GREEN, "Chercheurs"));
    empLgL->addWidget(empLegendRow(QColor("#9FBEB9"), "Techniciens"));
    empLgL->addWidget(empLegendRow(W_ORANGE, "Temps partiel"));
    empLgL->addWidget(empLegendRow(W_RED, "Absents"));
    empLgL->addStretch(1);

    QFrame* empBarCard = softBox();
    QVBoxLayout* empBcL = new QVBoxLayout(empBarCard);
    empBcL->setContentsMargins(12,12,12,12);
    BarChart* empBarStats = new BarChart;
    empBcL->addWidget(empBarStats, 1);

    empDashL->addWidget(empDonutCard, 1);
    empDashL->addWidget(empLegendCard, 1);
    empDashL->addWidget(empBarCard, 1);

    empOuterStatsL->addWidget(empDash);

    QScrollArea* empStatsScroll = new QScrollArea;
    empStatsScroll->setObjectName("empStatsScroll");
    empStatsScroll->setWidgetResizable(true);
    empStatsScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    empStatsScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    empStatsScroll->setFrameShape(QFrame::NoFrame);
    empStatsScroll->setStyleSheet(R"(
        QScrollArea#empStatsScroll {
            background: transparent;
            border: none;
        }
        QScrollArea#empStatsScroll > QWidget > QWidget {
            background: transparent;
        }
        QScrollBar:vertical {
            background: rgba(0,0,0,0.06);
            width: 6px;
            border-radius: 3px;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background: rgba(45,212,191,0.50);
            border-radius: 3px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover {
            background: rgba(45,212,191,0.80);
        }
        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical { height: 0px; }
        QScrollBar::add-page:vertical,
        QScrollBar::sub-page:vertical { background: none; }
    )");
    empStatsScroll->setWidget(empOuterStats);
    empS->addWidget(empStatsScroll, 1);

    QFrame* empBottomStats = new QFrame;
    empBottomStats->setFixedHeight(64);
    empBottomStats->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* empBottomStatsL = new QHBoxLayout(empBottomStats);
    empBottomStatsL->setContentsMargins(14,10,14,10);
    QPushButton* empBackStats = actionBtn("Retour", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_ArrowBack), true);
    empBottomStatsL->addWidget(empBackStats);
    empBottomStatsL->addStretch(1);
    empS->addWidget(empBottomStats);

    addStackPage(empStatsPage);

    auto updateEmpStatsFromTable = [=](){
        QMap<QString,int> roleCount;
        QMap<QString,int> specCount;
        int total = 0;

        for (int r=0; r<empTable->rowCount(); ++r) {
            if (empTable->isRowHidden(r)) continue;
            if (!empTable->item(r, 4) || !empTable->item(r, 5)) continue;
            total += 1;
            const QString role = empTable->item(r, 4)->text();
            const QString spec = empTable->item(r, 5)->text();
            roleCount[role] += 1;
            specCount[spec] += 1;
        }

        empTotalLbl->setText(QString("Total Employes: %1").arg(total));

        QList<DonutChart::Slice> slices;
        auto colorForRole = [&](const QString& role)->QColor{
            if (role == "Chercheur")  return W_GREEN;
            if (role == "Technicien") return QColor("#9FBEB9");
            return QColor("#7A8B8A");
        };
        for (auto it = roleCount.constBegin(); it != roleCount.constEnd(); ++it) {
            slices.push_back({(double)it.value(), colorForRole(it.key()), it.key()});
        }
        empDonutStats->setData(slices);

        QList<BarChart::Bar> bars;
        for (auto it = specCount.constBegin(); it != specCount.constEnd(); ++it) {
            bars.push_back({(double)it.value(), it.key()});
        }
        empBarStats->setData(bars);
    };

    // ==========================================================
    // PAGE 23 : Publications - DETAILS (PUB_DETAILS)
    // ==========================================================
    QWidget* pubDetailsPage = new QWidget;
    QVBoxLayout* pb4 = new QVBoxLayout(pubDetailsPage);
    pb4->setContentsMargins(22, 18, 22, 18);
    pb4->setSpacing(14);

    ModulesBar barPubDetails;
    pb4->addWidget(makeHeaderBlock(st, "Détails publication", ModuleTab::Publication, &barPubDetails));
    connectModulesSwitch(this, stack, barPubDetails);

    QFrame* pubDetailsCard = softBox();
    QVBoxLayout* pubDetailsL = new QVBoxLayout(pubDetailsCard);
    pubDetailsL->setContentsMargins(14,14,14,14);
    pubDetailsL->setSpacing(10);

    QLabel* pubDetTitle = new QLabel("Titre");
    QFont pubTitleFont = pubDetTitle->font();
    pubTitleFont.setPointSize(14);
    pubTitleFont.setBold(true);
    pubDetTitle->setFont(pubTitleFont);

    auto pubDetailRow = [&](const QString& label, QLabel*& valueOut){
        QWidget* row = new QWidget;
        QHBoxLayout* h = new QHBoxLayout(row);
        h->setContentsMargins(0,0,0,0);
        h->setSpacing(8);
        QLabel* lab = new QLabel(label + " :");
        lab->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
        QLabel* val = new QLabel;
        val->setStyleSheet("color: rgba(0,0,0,0.70); font-weight: 700;");
        val->setWordWrap(true);
        h->addWidget(lab);
        h->addWidget(val, 1);
        valueOut = val;
        return row;
    };

    QLabel* pubDetJournal = nullptr;
    QLabel* pubDetYear = nullptr;
    QLabel* pubDetDoi = nullptr;
    QLabel* pubDetStatus = nullptr;
    QLabel* pubDetEmployeeId = nullptr;
    QLabel* pubDetImpact = nullptr;
    QLabel* pubDetCitations = nullptr;
    QLabel* pubDetAbstract = nullptr;

    pubDetailsL->addWidget(pubDetTitle);
    pubDetailsL->addWidget(pubDetailRow("Journal/Conf.", pubDetJournal));
    pubDetailsL->addWidget(pubDetailRow("Année", pubDetYear));
    pubDetailsL->addWidget(pubDetailRow("DOI", pubDetDoi));
    pubDetailsL->addWidget(pubDetailRow("Statut", pubDetStatus));
    pubDetailsL->addWidget(pubDetailRow("Employé", pubDetEmployeeId));
    pubDetailsL->addWidget(pubDetailRow("Impact Factor", pubDetImpact));
    pubDetailsL->addWidget(pubDetailRow("Citation", pubDetCitations));

    // Health Score Section
    QFrame* healthScoreSep = new QFrame;
    healthScoreSep->setFixedHeight(1);
    healthScoreSep->setStyleSheet("background: rgba(0,0,0,0.10);");
    pubDetailsL->addWidget(healthScoreSep);

    QLabel* healthScoreTitle = new QLabel("📊 Publication Health Score");
    QFont healthScoreTitleFont = healthScoreTitle->font();
    healthScoreTitleFont.setBold(true);
    healthScoreTitle->setFont(healthScoreTitleFont);
    healthScoreTitle->setStyleSheet("color: rgba(0,0,0,0.75);");
    pubDetailsL->addWidget(healthScoreTitle);

    QLabel* pubDetHealthStatus = nullptr;
    QLabel* pubDetHealthScore = nullptr;
    pubDetailsL->addWidget(pubDetailRow("Overall Health", pubDetHealthStatus));
    pubDetailsL->addWidget(pubDetailRow("Health Score", pubDetHealthScore));

    QLabel* pubDetCompleteness = nullptr;
    QLabel* pubDetCitationScore = nullptr;
    QLabel* pubDetImpactScore = nullptr;
    QLabel* pubDetRecencyScore = nullptr;
    QLabel* pubDetProjectScore = nullptr;
    QLabel* pubDetDupScore = nullptr;
    pubDetailsL->addWidget(pubDetailRow("Completeness", pubDetCompleteness));
    pubDetailsL->addWidget(pubDetailRow("Citation Score", pubDetCitationScore));
    pubDetailsL->addWidget(pubDetailRow("Impact Score", pubDetImpactScore));
    pubDetailsL->addWidget(pubDetailRow("Recency Score", pubDetRecencyScore));
    pubDetailsL->addWidget(pubDetailRow("Project Linkage", pubDetProjectScore));
    pubDetailsL->addWidget(pubDetailRow("Duplication Risk", pubDetDupScore));

    // Next Actions Box
    QFrame* nextActionsFrame = new QFrame;
    nextActionsFrame->setStyleSheet("QFrame{ background: rgba(52, 152, 219, 0.1); border: 1px solid rgba(52, 152, 219, 0.3); border-radius: 8px; }");
    QVBoxLayout* nextActionsLayout = new QVBoxLayout(nextActionsFrame);
    nextActionsLayout->setContentsMargins(10, 8, 10, 8);
    nextActionsLayout->setSpacing(6);

    QLabel* nextActionsLabel = new QLabel("💡 Next Best Actions:");
    nextActionsLabel->setStyleSheet("color: #3498db; font-weight: bold; font-size: 11px;");
    nextActionsLayout->addWidget(nextActionsLabel);

    QListWidget* pubDetailActionsList = new QListWidget;
    pubDetailActionsList->setMaximumHeight(100);
    pubDetailActionsList->setStyleSheet(
        "QListWidget { border: none; background: transparent; }"
        "QListWidget::item { padding: 4px; border-left: 3px solid #3498db; padding-left: 8px; }"
        "QListWidget::item:hover { background: rgba(52, 152, 219, 0.15); }"
    );
    nextActionsLayout->addWidget(pubDetailActionsList);

    pubDetailsL->addWidget(nextActionsFrame);

    QLabel* abstractLabel = new QLabel("Résumé :");
    abstractLabel->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    QLabel* abstractValue = new QLabel;
    abstractValue->setWordWrap(true);
    abstractValue->setStyleSheet("color: rgba(0,0,0,0.65); font-weight: 700;");
    pubDetAbstract = abstractValue;

    pubDetailsL->addWidget(abstractLabel);
    pubDetailsL->addWidget(abstractValue);

    QHBoxLayout* pubDetQrRow = new QHBoxLayout;
    pubDetQrRow->setContentsMargins(0, 4, 0, 0);
    pubDetQrRow->setSpacing(0);
    pubDetQrRow->addStretch(1);

    QFrame* pubDetQrCard = softBox();
    pubDetQrCard->setFixedSize(108, 108);
    QVBoxLayout* pubDetQrCardL = new QVBoxLayout(pubDetQrCard);
    pubDetQrCardL->setContentsMargins(8, 8, 8, 8);

    QLabel* pubDetQrImage = new QLabel;
    pubDetQrImage->setFixedSize(92, 92);
    pubDetQrImage->setAlignment(Qt::AlignCenter);
    pubDetQrImage->setText("QR");
    pubDetQrImage->setStyleSheet("QLabel{ background: rgba(255,255,255,0.75); border: 1px solid rgba(0,0,0,0.10); border-radius: 8px; color: rgba(0,0,0,0.55); font-weight: 900; }");

    pubDetQrCardL->addWidget(pubDetQrImage, 0, Qt::AlignCenter);
    pubDetQrRow->addWidget(pubDetQrCard, 0, Qt::AlignRight);
    pubDetailsL->addLayout(pubDetQrRow);

    pb4->addWidget(pubDetailsCard, 1);

    QFrame* pubDetailsBottom = new QFrame;
    pubDetailsBottom->setFixedHeight(64);
    pubDetailsBottom->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* pubDetailsBottomL = new QHBoxLayout(pubDetailsBottom);
    pubDetailsBottomL->setContentsMargins(14,10,14,10);
    QPushButton* pubDetailsExport = actionBtn("Exporter PDF", "rgba(10,95,88,0.45)", "rgba(255,255,255,0.90)", st->standardIcon(QStyle::SP_DialogSaveButton), true);
    QPushButton* pubDetailsBack = actionBtn("Retour", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_ArrowBack), true);
    pubDetailsBottomL->addWidget(pubDetailsExport);
    pubDetailsBottomL->addWidget(pubDetailsBack);
    pubDetailsBottomL->addStretch(1);
    pb4->addWidget(pubDetailsBottom);

    addStackPage(pubDetailsPage);

    QNetworkAccessManager* pubDetailsQrManager = new QNetworkAccessManager(this);
    auto updatePubDetailsQr = [=](const QString& targetUrl){
        const QString safeTarget = targetUrl.trimmed().isEmpty() ? QString("https://www.biorxiv.org/") : targetUrl.trimmed();
        pubDetQrImage->setToolTip(safeTarget);
        const QByteArray encoded = QUrl::toPercentEncoding(safeTarget);
        const QUrl qrUrl(QString("https://api.qrserver.com/v1/create-qr-code/?size=320x320&data=%1").arg(QString::fromLatin1(encoded)));
        pubDetailsQrManager->get(QNetworkRequest(qrUrl));
    };

    QObject::connect(pubDetailsQrManager, &QNetworkAccessManager::finished, this, [=](QNetworkReply* reply){
        if (reply->error() == QNetworkReply::NoError) {
            QPixmap qrPixmap;
            if (qrPixmap.loadFromData(reply->readAll())) {
                pubDetQrImage->setPixmap(qrPixmap.scaled(pubDetQrImage->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            } else {
                pubDetQrImage->setText("QR");
            }
        } else {
            pubDetQrImage->setText("QR");
        }
        reply->deleteLater();
    });

    auto clearPublicationForm = [=]() {
        sbPubId->setReadOnly(false);
        sbPubId->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
        sbPubId->setValue(0);
        leTitle->clear();
        sbYear->setValue(QDate::currentDate().year());
        leJournal->clear();
        leDOI->clear();
        cbStatus->setCurrentText("Statut");
        teAbstract->clear();
        cbEmployee->setCurrentIndex(0);
        sbImpactFactor->setValue(0.0);
        leCitations->clear();
    };

    auto fillPublicationFormFromSelection = [=]() -> bool {
        const int row = pubTable->currentRow();
        if (row < 0) {
            QMessageBox::information(this, "Information", "Sélectionnez une publication à modifier.");
            return false;
        }

        Publication publication;
        QString errorMessage;
        const int id = pubTable->item(row, 0)->text().toInt();
        if (!Publication::readById(id, publication, &errorMessage)) {
            QMessageBox::warning(this, "Publication", "Lecture impossible :\n" + errorMessage);
            return false;
        }

        sbPubId->setValue(publication.id());
        sbPubId->setReadOnly(true);
        sbPubId->setButtonSymbols(QAbstractSpinBox::NoButtons);
        leTitle->setText(publication.titre());
        sbYear->setValue(publication.annee());
        leJournal->setText(publication.journal());
        leDOI->setText(publication.doi());
        if (cbStatus->findText(publication.status()) >= 0)
            cbStatus->setCurrentText(publication.status());
        else
            cbStatus->setCurrentText("Statut");
        teAbstract->setPlainText(publication.abstractText());
        const QString employeeText = pubTable->item(row, 6) ? pubTable->item(row, 6)->text().trimmed() : QString();
        const int employeeIndex = cbEmployee->findText(employeeText, Qt::MatchFixedString);
        if (employeeIndex >= 0) {
            cbEmployee->setCurrentIndex(employeeIndex);
        } else {
            cbEmployee->setCurrentIndex(0);
        }
        sbImpactFactor->setValue(pubTable->item(row, 8) ? pubTable->item(row, 8)->text().toDouble() : 0.0);
        leCitations->setPlainText(pubTable->item(row, 9) ? pubTable->item(row, 9)->text() : QString());
        return true;
    };

    auto updatePubDetailsFromRow = [=]()->bool{
        int r = pubTable->currentRow();
        if (r < 0) {
            QMessageBox::information(this, "Information", "Sélectionnez une publication.");
            return false;
        }

        Publication publication;
        QString errorMessage;
        const int id = pubTable->item(r,0)->text().toInt();
        if (!Publication::readById(id, publication, &errorMessage)) {
            QMessageBox::warning(this, "Publication", "Lecture impossible :\n" + errorMessage);
            return false;
        }

        pubDetTitle->setText(publication.titre());
        pubDetJournal->setText(publication.journal());
        pubDetYear->setText(QString::number(publication.annee()));
        pubDetDoi->setText(publication.doi());
        pubDetStatus->setText(publication.status());
        const QString employeNom = pubTable->item(r,6) ? pubTable->item(r,6)->text() : QString();
        pubDetEmployeeId->setText(employeNom.isEmpty() ? "Aucun employé" : employeNom);
        pubDetImpact->setText(QString::number(publication.impactFactor(), 'f', 2));
        pubDetCitations->setText(formatPublicationCitation(publication.titre(), employeNom, publication.journal(), publication.annee(), publication.doi()));

        // Calculate and display health score
        PublicationScorer::ScoreBreakdown scores = PublicationScorer::calculateScores(publication);
        const QString healthStatus = PublicationScorer::getHealthStatus(scores.totalHealthScore);

        pubDetHealthStatus->setText(healthStatus);
        pubDetHealthScore->setText(QString("%1/100").arg(static_cast<int>(scores.totalHealthScore)));
        pubDetCompleteness->setText(QString("%1%").arg(static_cast<int>(scores.completenessScore)));
        pubDetCitationScore->setText(QString("%1%").arg(static_cast<int>(scores.citationScore)));
        pubDetImpactScore->setText(QString("%1%").arg(static_cast<int>(scores.impactFactorScore)));
        pubDetRecencyScore->setText(QString("%1%").arg(static_cast<int>(scores.recencyScore)));
        pubDetProjectScore->setText(QString("%1%").arg(static_cast<int>(scores.projectLinkageScore)));
        pubDetDupScore->setText(QString("%1%").arg(static_cast<int>(scores.duplicationRiskScore)));

        // Color code the health status
        if (healthStatus == "Excellent") {
            pubDetHealthStatus->setStyleSheet("color: #2ecc71; font-weight: bold; font-size: 12px;");
        } else if (healthStatus == "Good") {
            pubDetHealthStatus->setStyleSheet("color: #3498db; font-weight: bold; font-size: 12px;");
        } else if (healthStatus == "Fair") {
            pubDetHealthStatus->setStyleSheet("color: #f39c12; font-weight: bold; font-size: 12px;");
        } else {
            pubDetHealthStatus->setStyleSheet("color: #e74c3c; font-weight: bold; font-size: 12px;");
        }

        // Update next actions
        QList<PublicationScorer::NextAction> actions = PublicationScorer::getNextActions(publication);
        pubDetailActionsList->clear();
        for (const auto& action : actions) {
            QString itemText = QString("✓ %1").arg(action.action);
            QListWidgetItem* item = new QListWidgetItem(itemText);
            item->setToolTip(QString("Priority %1: %2").arg(action.priority).arg(action.reason));

            // Set priority color
            QColor priorityColor;
            if (action.priority <= 1) priorityColor = W_RED;
            else if (action.priority == 2) priorityColor = W_ORANGE;
            else if (action.priority == 3) priorityColor = QColor("#f39c12");
            else priorityColor = QColor("#3498db");

            item->setForeground(priorityColor);
            pubDetailActionsList->addItem(item);
        }

        pubDetAbstract->setText(publication.abstractText().isEmpty() ? "Résumé non renseigné." : publication.abstractText());
        const QString qrTarget = QString("https://www.biorxiv.org/");
        updatePubDetailsQr(qrTarget);
        return true;
    };

    auto exportSelectedPublicationPdf = [=]() -> bool {
        const int row = pubTable->currentRow();
        if (row < 0 || !pubTable->item(row, 0)) {
            QMessageBox::information(this, "Information", "Sélectionnez une publication.");
            return false;
        }

        Publication publication;
        QString errorMessage;
        const int id = pubTable->item(row, 0)->text().toInt();
        if (!Publication::readById(id, publication, &errorMessage)) {
            QMessageBox::warning(this, "Publication", "Lecture impossible :\n" + errorMessage);
            return false;
        }

        const QString fileName = QFileDialog::getSaveFileName(
            this,
            "Exporter la publication en PDF",
            QString("Publication_%1.pdf").arg(QDate::currentDate().toString("yyyyMMdd")),
            "PDF Files (*.pdf)");
        if (fileName.isEmpty()) {
            return false;
        }

        const QString qrTarget = QString("https://bioxrv.org/");

        QString qrDataUri;
        const QPixmap qrPixmap = pubDetQrImage->pixmap(Qt::ReturnByValue);
        if (!qrPixmap.isNull()) {
            QByteArray qrBytes;
            QBuffer buffer(&qrBytes);
            if (buffer.open(QIODevice::WriteOnly)) {
                if (qrPixmap.toImage().save(&buffer, "PNG")) {
                    qrDataUri = QString("data:image/png;base64,%1").arg(QString::fromLatin1(qrBytes.toBase64()));
                }
                buffer.close();
            }
        }

        QString html;
        html += "<h2 style='color:#0A5F58;'>Publication</h2>";
        html += "<p style='color:#555;'>Export généré le " + QDate::currentDate().toString("dd/MM/yyyy") + "</p>";
        html += "<table border='1' cellpadding='6' cellspacing='0' width='100%' style='border-collapse:collapse;'>";
        html += "<tr style='background:#AFC6C3;'><th>Champ</th><th>Valeur</th></tr>";
        html += "<tr><td>Titre</td><td>" + publication.titre().toHtmlEscaped() + "</td></tr>";
        html += "<tr><td>Journal / Conf.</td><td>" + publication.journal().toHtmlEscaped() + "</td></tr>";
        html += "<tr><td>Année</td><td>" + QString::number(publication.annee()) + "</td></tr>";
        html += "<tr><td>DOI</td><td>" + publication.doi().toHtmlEscaped() + "</td></tr>";
        html += "<tr><td>Statut</td><td>" + publication.status().toHtmlEscaped() + "</td></tr>";
        html += "<tr><td>Impact Factor</td><td>" + QString::number(publication.impactFactor(), 'f', 2) + "</td></tr>";
        const QString citationText = formatPublicationCitation(publication.titre(), pubTable->item(row,6) ? pubTable->item(row,6)->text() : QString(), publication.journal(), publication.annee(), publication.doi());
        html += "<tr><td>Citation</td><td>" + citationText.toHtmlEscaped() + "</td></tr>";
        html += "<tr><td>Auteur(s)</td><td>" + (pubTable->item(row, 6) ? pubTable->item(row, 6)->text().toHtmlEscaped() : QString("Aucun employé")) + "</td></tr>";
        html += "<tr><td>Résumé</td><td>" + publication.abstractText().toHtmlEscaped().replace("\n", "<br/>") + "</td></tr>";
        html += "</table>";
        if (!qrDataUri.isEmpty()) {
            html += "<div style='margin-top: 18px; text-align:right;'>";
            html += "<div style='font-size:12px; color:#555; margin-bottom:4px;'>QR de référence (bioxrv.org)</div>";
            html += "<img src='" + qrDataUri + "' width='120' height='120'/>";
            html += "</div>";
        } else {
            html += "<p style='margin-top: 18px;'><b>Lien QR :</b> " + qrTarget.toHtmlEscaped() + "</p>";
        }

        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(fileName);

        QTextDocument doc;
        doc.setHtml(html);
        doc.print(&printer);
        showToast(this, "PDF de la publication exporté.", true);
        return true;
    };

    // ==========================================================
    // PAGE 24 : Expériences - DETAILS (EXP_DETAILS)
    // ==========================================================
    QWidget* expDetailsPage = new QWidget;
    expDetailsPage->setObjectName("expDetailsPage");
    expDetailsPage->setStyleSheet(
        "QFrame#expDetailsPanel{"
        " background: rgba(255,255,255,0.90);"
        " border: 1px solid rgba(8,72,63,0.18);"
        " border-radius: 16px;"
        "}"
        "QFrame#expDetailsSection{"
        " background: rgba(255,255,255,0.80);"
        " border: 1px solid rgba(12,75,68,0.12);"
        " border-radius: 12px;"
        "}"
        "QLabel#expDetailsValue{"
        " background: rgba(248,252,251,0.97);"
        " color: #111827;"
        " border: 1px solid rgba(12,75,68,0.18);"
        " border-radius: 12px;"
        " padding: 7px 12px;"
        " font-weight: 800;"
        " font-size: 13px;"
        "}"
    );
    QVBoxLayout* ep4 = new QVBoxLayout(expDetailsPage);
    ep4->setContentsMargins(22, 18, 22, 18);
    ep4->setSpacing(14);

    ModulesBar barExpDetails;
    ep4->addWidget(makeHeaderBlock(st, "Détails expérience", ModuleTab::ExperiencesProtocoles, &barExpDetails));
    connectModulesSwitch(this, stack, barExpDetails);

    QFrame* expDetailsCard = new QFrame;
    expDetailsCard->setObjectName("expDetailsPanel");
    QHBoxLayout* expDetailsCardL = new QHBoxLayout(expDetailsCard);
    expDetailsCardL->setContentsMargins(12,12,12,12);
    expDetailsCardL->setSpacing(12);

    auto mkExpDetValue = [&]() {
        QLabel* val = new QLabel("-");
        val->setObjectName("expDetailsValue");
        val->setMinimumHeight(34);
        val->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        val->setTextInteractionFlags(Qt::TextSelectableByMouse);
        return val;
    };

    QLabel* expDetTitle = mkExpDetValue();
    QLabel* expDetProto = mkExpDetValue();
    QLabel* expDetResp = mkExpDetValue();
    QLabel* expDetDate = mkExpDetValue();
    QLabel* expDetStatus = mkExpDetValue();
    QLabel* expDetType = mkExpDetValue();
    QLabel* expDetProjet = mkExpDetValue();
    QLabel* expDetDisponibilite = mkExpDetValue();
    QLabel* expDetEquipement = mkExpDetValue();
    QLabel* expDetResultat = mkExpDetValue();

    QFrame* expDetLeft = softBox();
    expDetLeft->setObjectName("expDetailsSection");
    expDetLeft->setFixedWidth(420);
    QVBoxLayout* expDetLeftL = new QVBoxLayout(expDetLeft);
    expDetLeftL->setContentsMargins(12,12,12,12);
    expDetLeftL->setSpacing(10);

    expDetLeftL->addWidget(expTitle("Informations"));
    expDetLeftL->addWidget(expRow(QStyle::SP_DirIcon, "Expérience", expDetTitle));
    expDetLeftL->addWidget(expRow(QStyle::SP_FileDialogDetailedView, "Hypothèse", expDetProto));
    expDetLeftL->addWidget(expRow(QStyle::SP_DialogApplyButton, "Projet", expDetResp));
    expDetLeftL->addWidget(expRow(QStyle::SP_FileDialogContentsView, "Type_Experience", expDetType));
    expDetLeftL->addWidget(expRow(QStyle::SP_DirOpenIcon, "Nom projet", expDetProjet));
    expDetLeftL->addWidget(expRow(QStyle::SP_FileDialogListView, "Resultat", expDetResultat));
    expDetLeftL->addStretch(1);

    QFrame* expDetRight = softBox();
    expDetRight->setObjectName("expDetailsSection");
    QVBoxLayout* expDetRightL = new QVBoxLayout(expDetRight);
    expDetRightL->setContentsMargins(12,12,12,12);
    expDetRightL->setSpacing(10);

    expDetRightL->addWidget(expTitle("Planification"));
    expDetRightL->addWidget(expRow(QStyle::SP_MessageBoxInformation, "Date", expDetDate));
    expDetRightL->addWidget(expRow(QStyle::SP_MessageBoxInformation, "Statut", expDetStatus));
    expDetRightL->addWidget(expRow(QStyle::SP_FileDialogListView, "Disponibilité équipement", expDetDisponibilite));
    expDetRightL->addWidget(expRow(QStyle::SP_DriveHDIcon, "Équipement lié", expDetEquipement));
    expDetRightL->addStretch(1);

    expDetailsCardL->addWidget(expDetLeft);
    expDetailsCardL->addWidget(expDetRight, 1);
    ep4->addWidget(expDetailsCard, 1);

    QFrame* expDetailsBottom = new QFrame;
    expDetailsBottom->setFixedHeight(64);
    expDetailsBottom->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* expDetailsBottomL = new QHBoxLayout(expDetailsBottom);
    expDetailsBottomL->setContentsMargins(14,10,14,10);
    QPushButton* expDetailsBack = actionBtn("Retour", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_ArrowBack), true);
    expDetailsBottomL->addWidget(expDetailsBack);
    expDetailsBottomL->addStretch(1);
    ep4->addWidget(expDetailsBottom);

    addStackPage(expDetailsPage);

    auto updateExpDetailsFromRow = [=]()->bool{
        int r = expTable->currentRow();
        if (r < 0) {
            QMessageBox::information(this, "Information", "Sélectionnez une expérience.");
            return false;
        }
        int id = expTable->item(r, 0)->data(Qt::UserRole).toInt();
        ExperienceRecord rec;
        QString err;
        if (!expCrud->fetchExperience(id, rec, &err)) {
            showToast(this, "Erreur : " + err, false);
            return false;
        }

        expDetTitle->setText(rec.titre.isEmpty() ? QString("-") : rec.titre);
        expDetProto->setText(rec.hypothese.isEmpty() ? QString("-") : rec.hypothese);
        expDetResp->setText(rec.projetId.isNull() ? QString("-") : QString::number(rec.projetId.toInt()));
        QString projetNom = "-";
        if (!rec.projetId.isNull()) {
            QSqlQuery qProj;
            qProj.prepare("SELECT \"nom_du_projet\" FROM \"projet\" WHERE \"Id_projet\"=:id");
            qProj.bindValue(":id", rec.projetId.toInt());
            if (qProj.exec() && qProj.next()) {
                const QString p = qProj.value(0).toString().trimmed();
                if (!p.isEmpty()) projetNom = p;
            }
        }
        expDetProjet->setText(projetNom);
        expDetDate->setText(QString("%1 -> %2")
                                .arg(rec.dateDebut.isValid() ? rec.dateDebut.toString("dd/MM/yyyy") : "-")
                                .arg(rec.dateFin.isValid() ? rec.dateFin.toString("dd/MM/yyyy") : "-"));
        ExpStatus est = statusFromString(rec.status);
        expDetStatus->setText(expStatusText(est));
        expDetStatus->setStyleSheet(QString());
        expDetType->setText(rec.typeExperience.isEmpty() ? QString("-") : rec.typeExperience);
        QString eqName = "-";
        QString eqAvailability = rec.disponibiliteEquipement.trimmed();
        {
            QSqlQuery qEq;
            qEq.prepare("SELECT \"nom_equipement\", NVL(TRIM(\"statut\"),'') FROM \"Équipement\" WHERE \"Id_exp\" = :id ORDER BY \"equipement_id\"");
            qEq.bindValue(":id", rec.id);
            if (qEq.exec() && qEq.next()) {
                const QString n = qEq.value(0).toString().trimmed();
                if (!n.isEmpty()) eqName = n;
                if (eqAvailability.isEmpty()) {
                    const QString stEq = qEq.value(1).toString().trimmed().toLower();
                    if (stEq.contains("hors") || stEq.contains("service")) eqAvailability = "Non disponible";
                    else if (!stEq.isEmpty()) eqAvailability = "Disponible";
                }
            }
        }
        expDetDisponibilite->setText(eqAvailability.isEmpty() ? QString("-") : eqAvailability);
        expDetEquipement->setText(eqName);
        expDetDisponibilite->setStyleSheet(QString());
        expDetResultat->setText(rec.resultat.isEmpty() ? QString("-") : rec.resultat);
        return true;
    };

    // ==========================================================
    // PAGE 25 : Gestion Projet - DETAILS (PROJ_DETAILS)
    // ==========================================================
    QWidget* projDetailsPage = new QWidget;
    QVBoxLayout* gp4 = new QVBoxLayout(projDetailsPage);
    gp4->setContentsMargins(22, 18, 22, 18);
    gp4->setSpacing(14);

    ModulesBar barProjDetails;
    gp4->addWidget(makeHeaderBlock(st, "Détails projet", ModuleTab::GestionProjet, &barProjDetails));
    connectModulesSwitch(this, stack, barProjDetails);

    QFrame* projDetailsCard = softBox();
    QVBoxLayout* projDetailsL = new QVBoxLayout(projDetailsCard);
    projDetailsL->setContentsMargins(14,14,14,14);
    projDetailsL->setSpacing(10);

    QLabel* projDetTitle = new QLabel("Projet");
    QFont projTitleFont = projDetTitle->font();
    projTitleFont.setPointSize(14);
    projTitleFont.setBold(true);
    projDetTitle->setFont(projTitleFont);

    QLabel* projDetDomain = nullptr;
    QLabel* projDetBudget = nullptr;
    QLabel* projDetStart = nullptr;
    QLabel* projDetEnd = nullptr;
    QLabel* projDetStatus = nullptr;
    QLabel* projDetFinancement = nullptr;
    QLabel* projDetEthique = nullptr;
    QLabel* projDetPubs = nullptr;

    auto projDetailRow = [&](const QString& label, QLabel*& valueOut){
        QWidget* row = new QWidget;
        QHBoxLayout* h = new QHBoxLayout(row);
        h->setContentsMargins(0,0,0,0);
        h->setSpacing(8);
        QLabel* lab = new QLabel(label + " :");
        lab->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
        QLabel* val = new QLabel;
        val->setStyleSheet("color: rgba(0,0,0,0.70); font-weight: 700;");
        h->addWidget(lab);
        h->addWidget(val, 1);
        valueOut = val;
        return row;
    };

    projDetailsL->addWidget(projDetTitle);
    projDetailsL->addWidget(projDetailRow("Domaine", projDetDomain));
    projDetailsL->addWidget(projDetailRow("Budget", projDetBudget));
    projDetailsL->addWidget(projDetailRow("Date début", projDetStart));
    projDetailsL->addWidget(projDetailRow("Date fin", projDetEnd));
    projDetailsL->addWidget(projDetailRow("Statut", projDetStatus));
    projDetailsL->addWidget(projDetailRow("Financement", projDetFinancement));
    projDetailsL->addWidget(projDetailRow("Approbation éthique", projDetEthique));
    projDetailsL->addWidget(projDetailRow("Publications", projDetPubs));

    gp4->addWidget(projDetailsCard, 1);

    QFrame* projDetailsBottom = new QFrame;
    projDetailsBottom->setFixedHeight(64);
    projDetailsBottom->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* projDetailsBottomL = new QHBoxLayout(projDetailsBottom);
    projDetailsBottomL->setContentsMargins(14,10,14,10);
    QPushButton* projDetailsBack = actionBtn("Retour", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_ArrowBack), true);
    projDetailsBottomL->addWidget(projDetailsBack);
    projDetailsBottomL->addStretch(1);
    gp4->addWidget(projDetailsBottom);

    addStackPage(projDetailsPage);

    // PAGE 28 — PROJ_ADVANCED
    QWidget* projAdvancedPage = new QWidget;
    QVBoxLayout* gp5 = new QVBoxLayout(projAdvancedPage);
    gp5->setContentsMargins(uiMargin(this), uiMargin(this), uiMargin(this), uiMargin(this));
    gp5->setSpacing(16);

    ModulesBar barProjAdvanced;
    gp5->addWidget(makeHeaderBlock(st, "Métier Avancé — Projet", ModuleTab::GestionProjet, &barProjAdvanced));
    connectModulesSwitch(this, stack, barProjAdvanced);

    QLabel* advTitle = new QLabel("  Fonctionnalités Avancées");
    advTitle->setStyleSheet(
        "color: #0A5F58; font-size: 16px; font-weight: 900;"
        "background: rgba(10,95,88,0.09); border-radius: 12px; padding: 10px 18px;");
    gp5->addWidget(advTitle);

    QLabel* advSubtitle = new QLabel("Sélectionnez une fonctionnalité avancée à utiliser pour la gestion intelligente de vos projets.");
    advSubtitle->setStyleSheet("color: rgba(0,0,0,0.55); font-size: 12px; font-weight: 600; padding-left: 4px;");
    advSubtitle->setWordWrap(true);
    gp5->addWidget(advSubtitle);

    QScrollArea* advScroll = new QScrollArea;
    advScroll->setWidgetResizable(true);
    advScroll->setFrameShape(QFrame::NoFrame);
    advScroll->setStyleSheet("QScrollArea{ background: transparent; border: none; }");

    QWidget* advGrid = new QWidget;
    advGrid->setStyleSheet("background: transparent;");
    QGridLayout* advGridL = new QGridLayout(advGrid);
    advGridL->setContentsMargins(4,4,4,4); advGridL->setSpacing(14);

    struct AdvFeature { QStyle::StandardPixmap icon; QString title; QString description; QString accent; };
    static const AdvFeature features[] = {
        { QStyle::SP_MessageBoxInformation, "Santé du Projet",
          "Calcul d'un score de santé global\net alerte automatique si score ≤ 30%", "#518195" },
        { QStyle::SP_FileDialogDetailedView, "Estimation Réaliste",
          "Estimation de la durée et du budget\nnécessaires pour chaque projet", "#518195" },
        { QStyle::SP_DialogApplyButton, "Collaborateurs Suggérés",
          "Suggestion de collaborateurs\npossibles selon le domaine du projet", "#518195" },
        { QStyle::SP_MessageBoxWarning, "Spécialisations Manquantes",
          "Identification des spécialisations\nmanquantes dans l'équipe projet", "#518195" },
        { QStyle::SP_MessageBoxCritical, "Risques Probables",
          "Identification et analyse\ndes risques probables du projet", "#518195" },
        { QStyle::SP_FileDialogInfoView, "Leçons Apprises",
          "Apprentissages tirés d'autres\nprojets similaires dans le monde", "#518195" },
        { QStyle::SP_ComputerIcon, "Analyse Intelligente",
          "Résumé analytique et insights\ndes statistiques du projet", "#518195" },
        { QStyle::SP_BrowserReload, "Milestone Tracker",
          "Suivi des jalons et étapes\nimportantes du projet", "#518195" },
        { QStyle::SP_FileIcon, "Traçabilité des Actions",
          "Historique de toutes les modifications\navec nom de l'auteur", "#518195" },
    };
    static const int featureCount = (int)(sizeof(features)/sizeof(features[0]));

    for (int i = 0; i < featureCount; ++i) {
        const AdvFeature& f = features[i];
        const bool isSante     = (i == 0);
        const bool isMilestone = (i == 7);

        QPushButton* card = new QPushButton;
        card->setCursor(Qt::PointingHandCursor);
        card->setMinimumHeight(130);
        card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        QVBoxLayout* cardL = new QVBoxLayout(card);
        cardL->setContentsMargins(14,14,14,14); cardL->setSpacing(8);
        cardL->setAlignment(Qt::AlignTop);

        QHBoxLayout* iconRow = new QHBoxLayout;
        iconRow->setContentsMargins(0,0,0,0);
        QLabel* iconLbl = new QLabel;
        iconLbl->setPixmap(st->standardIcon(f.icon).pixmap(28,28));
        iconLbl->setStyleSheet("background: transparent; border: none;");
        iconRow->addWidget(iconLbl); iconRow->addStretch(1);
        cardL->addLayout(iconRow);

        QLabel* titleLbl = new QLabel(f.title);
        titleLbl->setStyleSheet("background: transparent; border: none; color: white; font-size: 13px; font-weight: 900;");
        titleLbl->setWordWrap(false);
        cardL->addWidget(titleLbl);

        QLabel* descLbl = new QLabel(f.description);
        descLbl->setStyleSheet("background: transparent; border: none; color: rgba(255,255,255,0.80); font-size: 10px; font-weight: 600;");
        descLbl->setWordWrap(true); descLbl->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        cardL->addWidget(descLbl, 1);

        card->setStyleSheet(QString(R"(
            QPushButton { background: %1; border: 1.5px solid rgba(0,0,0,0.08);
                border-left: 4px solid %1; border-radius: 14px; text-align: left; }
            QPushButton:hover { background: %1; border: 1.5px solid rgba(255,255,255,0.40);
                border-left: 4px solid rgba(255,255,255,0.60); }
            QPushButton:pressed { background: %1; }
        )").arg(f.accent));

        if (isSante) {
            card->setToolTip("Santé du Projet — Sélectionnez un projet puis calculez son score de santé");
            QObject::connect(card, &QPushButton::clicked, this, [=](){
                QDialog* picker = new QDialog(this, Qt::Dialog|Qt::WindowTitleHint|Qt::WindowCloseButtonHint);
                picker->setWindowTitle("Santé du Projet — Choisir un projet");
                picker->setMinimumSize(480, 360);
                picker->setAttribute(Qt::WA_DeleteOnClose);
                picker->setStyleSheet("QDialog{ background:#1C2A35; }");

                QVBoxLayout* pl = new QVBoxLayout(picker);
                pl->setContentsMargins(20,18,20,16); pl->setSpacing(12);

                QLabel* plTitle = new QLabel("  Sélectionnez le projet à analyser");
                plTitle->setStyleSheet("color:#0A5F58; font-size:13px; font-weight:900;"
                    "background:rgba(10,95,88,0.08); border-radius:10px; padding:7px 14px;");
                pl->addWidget(plTitle);

                QListWidget* projList = new QListWidget;
                projList->setStyleSheet(
                    "QListWidget{ background:rgba(255,255,255,0.85); border-radius:10px; border:1px solid rgba(10,95,88,0.20); }"
                    "QListWidget::item{ padding:8px 12px; color:#12443B; font-weight:600; }"
                    "QListWidget::item:selected{ background:#0A5F58; color:white; border-radius:6px; }");

                GestProjCrud pickerCrud;
                QList<ProjetRecord> allProjs; QString perr;
                pickerCrud.loadProjets(allProjs, &perr);
                for (const ProjetRecord& pr : allProjs) {
                    QListWidgetItem* item = new QListWidgetItem(pr.nomDuProjet);
                    item->setData(Qt::UserRole, pr.idProjet);
                    projList->addItem(item);
                }
                pl->addWidget(projList, 1);

                QHBoxLayout* pbl = new QHBoxLayout; pbl->setSpacing(10);
                QPushButton* cancelBtn  = new QPushButton("Annuler");
                cancelBtn->setFixedHeight(36);
                cancelBtn->setStyleSheet("QPushButton{ background:rgba(255,255,255,0.60); border:1px solid rgba(0,0,0,0.15);"
                    " border-radius:8px; font-weight:700; font-size:12px; padding:0 16px; color:#12443B; }"
                    "QPushButton:hover{ background:rgba(255,255,255,0.85); }");
                QPushButton* analyseBtn = new QPushButton("Analyser la santé");
                analyseBtn->setFixedHeight(36);
                analyseBtn->setStyleSheet("QPushButton{ background:#0A5F58; color:white; border-radius:8px;"
                    " font-weight:700; font-size:12px; padding:0 16px; }"
                    "QPushButton:hover{ background:#12443B; }");
                pbl->addWidget(cancelBtn); pbl->addStretch(1); pbl->addWidget(analyseBtn);
                pl->addLayout(pbl);

                QObject::connect(cancelBtn,  &QPushButton::clicked, picker, &QDialog::reject);
                QObject::connect(analyseBtn, &QPushButton::clicked, picker, [=](){
                    QListWidgetItem* sel = projList->currentItem();
                    if (!sel) {
                        ThemedAlertDialog::show(style(), picker, "info", "Santé du Projet", "Veuillez sélectionner un projet.");
                        return;
                    }
                    const int projId = sel->data(Qt::UserRole).toInt();
                    picker->accept();
                    GestProjCrud::showSanteRadar(projId, this);
                });
                QObject::connect(projList, &QListWidget::doubleClicked, picker, [=](){ analyseBtn->click(); });
                picker->exec();
            });
        } else if (isMilestone) {
            card->setToolTip("Milestone Tracker — Suivez les jalons clés de votre projet");
            QObject::connect(card, &QPushButton::clicked, this, [=](){
                GestProjCrud::showMilestoneTracker(this);
            });
        } else if (i == 8) {
            card->setToolTip("Traçabilité des Actions — Exporter l'historique des modifications");
            QObject::connect(card, &QPushButton::clicked, this, [=](){
                const QString fileName = QFileDialog::getSaveFileName(
                    this,
                    "Exporter le fichier de traçabilité",
                    "tracabilite_smartvision.txt",
                    "Text Files (*.txt);;All Files (*)");
                if (fileName.isEmpty()) return;

                QString target = fileName;
                if (!target.endsWith(".txt", Qt::CaseInsensitive)) target += ".txt";

                QString error;
                if (!copyTraceLogFile(target, &error)) {
                    showToast(this, "Échec de l'export : " + error, false);
                    return;
                }

                showToast(this, "Fichier de traçabilité exporté avec succès.", true);
            });
        } else if (i == 1) {
            card->setToolTip("Estimation Réaliste — Estimez durée et budget avant le démarrage");
            QObject::connect(card, &QPushButton::clicked, this, [=](){
                GestProjCrud::showEstimationRealiste(this);
            });
        } else if (i == 2) {
            card->setToolTip("Collaborateurs Suggérés — Suggère des employés et sponsors IA");
            QObject::connect(card, &QPushButton::clicked, this, [=](){
                GestProjCrud::showCollaborateursSuggeres(this);
            });
        } else if (i == 3) {
            card->setToolTip("Spécialisations Manquantes — Analyse les compétences manquantes dans l'équipe");
            QObject::connect(card, &QPushButton::clicked, this, [=](){
                GestProjCrud::showSpecialisationsManquantes(this);
            });
        } else if (i == 4) {
            card->setToolTip("Risques Probables — Identifie et score les risques du projet");
            QObject::connect(card, &QPushButton::clicked, this, [=](){
                GestProjCrud::showRisquesProbables(this);
            });
        } else if (i == 6) {
            card->setToolTip("Analyse Intelligente — Résumé analytique des statistiques du projet");
            QObject::connect(card, &QPushButton::clicked, this, [=](){
                GestProjCrud::showAnalyseIntelligente(this);
            });

         } else if (i == 5) {
            card->setToolTip("Leçons Apprises — Analyse des projets similaires terminés");
            QObject::connect(card, &QPushButton::clicked, this, [=](){
                LeconsApprises::showDialog(this);
            });

        } else {
            card->setToolTip(QString("%1 — Fonctionnalité en cours de développement").arg(f.title));
            card->setEnabled(true);
        }

        advGridL->addWidget(card, i/3, i%3);
    }

    advGridL->setColumnStretch(0,1); advGridL->setColumnStretch(1,1); advGridL->setColumnStretch(2,1);
    advScroll->setWidget(advGrid);
    gp5->addWidget(advScroll, 1);

    QFrame* advBottom = new QFrame;
    advBottom->setFixedHeight(64);
    advBottom->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* advBottomL = new QHBoxLayout(advBottom);
    advBottomL->setContentsMargins(14,10,14,10);
    QPushButton* advBack = actionBtn("Retour","rgba(255,255,255,0.55)",C_TEXT_DARK,st->standardIcon(QStyle::SP_ArrowBack),true);
    advBottomL->addWidget(advBack); advBottomL->addStretch(1);
    gp5->addWidget(advBottom);
    addStackPage(projAdvancedPage);

    // ── Dark / Light mode support for all Gestion Projet pages ──
    {
        auto prevThemeFn = g_applyThemeFn;
        g_applyThemeFn = [=](bool dark) {
            if (prevThemeFn) prevThemeFn(dark);

            const QString bg        = dark ? "#181C22"                : C_BG;
            const QString panelBg   = dark ? "rgba(44,50,58,0.78)"    : C_PANEL_BG;
            const QString panelBr   = dark ? "rgba(224,232,240,0.18)" : C_PANEL_BR;
            const QString fieldBg   = dark ? "rgba(255,255,255,0.06)" : "rgba(244,246,245,0.78)";
            const QString textColor = dark ? "#E8ECF0"                : "rgba(0,0,0,0.88)";
            const QString labelClr  = dark ? "rgba(214,224,232,0.92)" : "#12443B";
            const QString subLabelClr = dark ? "rgba(182,194,204,0.82)" : "rgba(0,0,0,0.60)";
            const QString border    = dark ? "rgba(224,232,240,0.16)" : "rgba(0,0,0,0.20)";
            const QString cardBg    = dark ? "rgba(34,40,48,0.74)"    : "rgba(255,255,255,0.20)";
            const QString titleClr  = dark ? "rgba(80,210,190,1.0)"   : "rgba(10,95,88,1.0)";
            const QString statsTitleClr = dark ? "#E8EEF2"             : "rgba(0,0,0,0.75)";
            const QString statsSubClr   = dark ? "rgba(180,200,210,0.70)" : "rgba(0,0,0,0.45)";
            const QString projBgImg = dark ? ":/image/crudeimagesombre.png"
                                           : ":/image/crudeimage.png";

            // ── proj2 — Add/Edit form page ───────────────────────
            proj2->setStyleSheet(QString(
                "QWidget#projFormPage{"
                "  border-image: url(%1) 0 0 0 0 stretch stretch;"
                "}"
                "QFrame#projFormPanel{"
                "  background:%2;"
                "  border:1px solid %3;"
                "  border-radius:16px;"
                "}"
                "QFrame#projFormSection{"
                "  background:%4;"
                "  border:1px solid %5;"
                "  border-radius:14px;"
                "}"
                "QFrame#projFormRow{"
                "  background:%4;"
                "  border:1px solid %5;"
                "  border-radius:12px;"
                "}"
                "QLabel#projFormTitle{"
                "  color:%6;"
                "  font-weight:900;"
                "  font-size:14px;"
                "  padding:2px 0;"
                "  background:transparent;"
                "}"
                "QLabel#projFormRowLabel{"
                "  color:%7;"
                "  font-weight:900;"
                "  font-size:13px;"
                "  background:transparent;"
                "}"
                "QLabel#projFormSubLabel{"
                "  color:%8;"
                "  font-size:11px;"
                "  font-weight:700;"
                "  background:transparent;"
                "}").arg(projBgImg, panelBg, panelBr, fieldBg, border,
                         titleClr, labelClr, subLabelClr));
            applyGlassShadow(outP2,
                             QColor(0, 0, 0, dark ? 92 : 38),
                             dark ? 46.0 : 28.0,
                             QPointF(0.0, dark ? 14.0 : 10.0));

            projName->setStyleSheet(projFieldStyle());
            projDomainEdit->setStyleSheet(projComboStyle());
            projStatus->setStyleSheet(projComboStyle());
            projEthique->setStyleSheet(projFieldStyle());
            projStart->setStyleSheet(projDateStyle());
            projEnd->setStyleSheet(projDateStyle());
            projPubsEdit->setStyleSheet(projPubsStyle());
            projFinancement->setStyleSheet(projFieldStyle());
            projBudgetSpin->setStyleSheet(projBudgetStyle());
            applyProjCalendar(projStart);
            applyProjCalendar(projEnd);

            // Bottom bar of form page
            p2Bottom->setStyleSheet(QString(
                "background:%1; border:1px solid %2; border-radius:14px;")
                .arg(cardBg, dark ? "rgba(224,232,240,0.14)" : "rgba(0,0,0,0.08)"));
            projSave->setStyleSheet(QString(
                "QPushButton{ background:%1; color:%2; border:1px solid %3; border-radius:10px; padding:10px 18px; font-weight:900; }"
                "QPushButton:hover{ background:%4; }"
                "QPushButton:pressed{ background:%5; }")
                .arg(dark ? "rgba(45,212,191,0.16)" : "rgba(10,95,88,0.45)",
                     dark ? "#CFFFF6" : "rgba(255,255,255,0.90)",
                     dark ? "rgba(45,212,191,0.30)" : "rgba(0,0,0,0.12)",
                     dark ? "rgba(45,212,191,0.24)" : "rgba(255,255,255,0.70)",
                     dark ? "rgba(45,212,191,0.32)" : "rgba(255,255,255,0.60)"));
            projCancel->setStyleSheet(QString(
                "QPushButton{ background:%1; color:%2; border:1px solid %3; border-radius:10px; padding:10px 18px; font-weight:800; }"
                "QPushButton:hover{ background:%4; }"
                "QPushButton:pressed{ background:%5; }")
                .arg(dark ? "rgba(255,255,255,0.06)" : "rgba(255,255,255,0.55)",
                     dark ? "#E8ECF0" : C_TEXT_DARK,
                     dark ? "rgba(224,232,240,0.14)" : "rgba(0,0,0,0.12)",
                     dark ? "rgba(255,255,255,0.10)" : "rgba(255,255,255,0.70)",
                     dark ? "rgba(255,255,255,0.14)" : "rgba(255,255,255,0.62)"));

            // ── proj3 — Statistics page ──────────────────────────
            proj3->setStyleSheet(QString("QWidget { background: %1; }").arg(bg));
            outP3->setStyleSheet(QString(
                "QFrame{ background:%1; border:1px solid %2; border-radius:14px; }")
                .arg(panelBg, panelBr));
            statsPageTitle->setStyleSheet(QString(
                "color:%1; font-size:18px; font-weight:900; padding-bottom:4px;")
                .arg(statsTitleClr));
            statsPageSub->setStyleSheet(QString(
                "color:%1; font-size:12px; font-weight:600;").arg(statsSubClr));
            secGraph->setStyleSheet(QString(
                "color:%1; font-size:13px; font-weight:900;"
                "background:rgba(10,95,88,0.%2); border-radius:8px; padding:6px 12px;")
                .arg(titleClr, dark ? "18" : "08"));
            secRapports->setStyleSheet(QString(
                "color:%1; font-size:13px; font-weight:900;"
                "background:rgba(139,47,60,0.%2); border-radius:8px; padding:6px 12px;")
                .arg(dark ? "rgba(220,100,110,1.0)" : "rgba(139,47,60,0.85)",
                     dark ? "18" : "07"));
            secExport->setStyleSheet(QString(
                "color:%1; font-size:13px; font-weight:900;"
                "background:rgba(0,120,60,0.%2); border-radius:8px; padding:6px 12px;")
                .arg(dark ? "rgba(60,210,120,1.0)" : "rgba(0,120,60,0.85)",
                     dark ? "18" : "07"));
            // Stats bottom bar
            p3Bottom->setStyleSheet(QString(
                "background:%1; border:1px solid %2; border-radius:14px;")
                .arg(cardBg, dark ? "rgba(210,218,226,0.18)" : "rgba(0,0,0,0.08)"));

            // ── projDetailsPage — Details page ───────────────────
            projDetailsPage->setStyleSheet(QString("QWidget { background: %1; }").arg(bg));
            projDetailsCard->setStyleSheet(QString(
                "QFrame{ background:%1; border:1px solid %2; border-radius:14px; }")
                .arg(panelBg, panelBr));
            projDetailsBottom->setStyleSheet(QString(
                "background:%1; border:1px solid %2; border-radius:14px;")
                .arg(cardBg, dark ? "rgba(210,218,226,0.18)" : "rgba(0,0,0,0.08)"));
            for (QLabel* lab : projDetailsPage->findChildren<QLabel*>()) {
                const QString s = lab->styleSheet();
                if (s.contains("rgba(0,0,0,0.55)") || s.contains("font-weight:900"))
                    lab->setStyleSheet(QString("color:%1; font-weight:900;").arg(labelClr));
                else if (s.contains("rgba(0,0,0,0.70)") || s.contains("font-weight:700"))
                    lab->setStyleSheet(QString("color:%1; font-weight:700;").arg(textColor));
            }

            // ── projAdvancedPage — Advanced features page ───────
            projAdvancedPage->setStyleSheet(QString("QWidget { background: %1; }").arg(bg));
            advTitle->setStyleSheet(QString(
                "color:%1; font-size:16px; font-weight:900;"
                "background:rgba(10,95,88,0.%2); border-radius:12px; padding:10px 18px;")
                .arg(titleClr, dark ? "18" : "09"));
            advSubtitle->setStyleSheet(QString(
                "color:%1; font-size:12px; font-weight:600; padding-left:4px;")
                .arg(dark ? "rgba(180,200,210,0.70)" : "rgba(0,0,0,0.55)"));
        };
        // Apply immediately to match current theme
        g_applyThemeFn(g_darkThemeEnabled);
    }

    auto updateProjDetailsFromRow = [=]()->bool{
        int r = projTable->currentRow();
        if (r < 0 || !projTable->item(r,1)) {
            ThemedAlertDialog::show(style(), this, "info", "Projet", "Sélectionnez un projet dans la liste.");
            return false;
        }
        projDetTitle->setText(projTable->item(r,1)->text());
        projDetDomain->setText(projTable->item(r,2) ? projTable->item(r,2)->text() : "-");
        projDetBudget->setText(projTable->item(r,5) ? projTable->item(r,5)->text() : "-");
        projDetStart->setText(projTable->item(r,3) ? projTable->item(r,3)->text() : "-");
        projDetEnd->setText(projTable->item(r,4) ? projTable->item(r,4)->text() : "-");
        ProjStatus ps = static_cast<ProjStatus>(projTable->item(r,6)->data(Qt::UserRole).toInt());
        projDetStatus->setText(projStatusText(ps));
        projDetFinancement->setText(projTable->item(r,7) ? projTable->item(r,7)->text() : "-");
        projDetEthique->setText(projTable->item(r,8) ? projTable->item(r,8)->text() : "-");
        projDetPubs->setText(projTable->item(r,9) ? projTable->item(r,9)->text() : "0");
        return true;
    };
    // ==========================================================
    // ✅ Marges adaptatives (initial)
    // ==========================================================
    p1->setContentsMargins(uiMargin(this), uiMargin(this), uiMargin(this), uiMargin(this));
    p2->setContentsMargins(uiMargin(this), uiMargin(this), uiMargin(this), uiMargin(this));
    p3->setContentsMargins(uiMargin(this), uiMargin(this), uiMargin(this), uiMargin(this));
    p4->setContentsMargins(uiMargin(this), uiMargin(this), uiMargin(this), uiMargin(this));
    p5->setContentsMargins(uiMargin(this), uiMargin(this), uiMargin(this), uiMargin(this));
    gp1->setContentsMargins(uiMargin(this), uiMargin(this), uiMargin(this), uiMargin(this));
    gp2->setContentsMargins(uiMargin(this), uiMargin(this), uiMargin(this), uiMargin(this));
    gp3->setContentsMargins(uiMargin(this), uiMargin(this), uiMargin(this), uiMargin(this));
    eq1->setContentsMargins(uiMargin(this), uiMargin(this), uiMargin(this), uiMargin(this));
    eq2->setContentsMargins(uiMargin(this), uiMargin(this), uiMargin(this), uiMargin(this));
    eq3->setContentsMargins(uiMargin(this), uiMargin(this), uiMargin(this), uiMargin(this));
    eq4->setContentsMargins(uiMargin(this), uiMargin(this), uiMargin(this), uiMargin(this));
    emp1->setContentsMargins(uiMargin(this), uiMargin(this), uiMargin(this), uiMargin(this));
    emp2->setContentsMargins(uiMargin(this), uiMargin(this), uiMargin(this), uiMargin(this));
    emp3->setContentsMargins(uiMargin(this), uiMargin(this), uiMargin(this), uiMargin(this));
    emp4->setContentsMargins(uiMargin(this), uiMargin(this), uiMargin(this), uiMargin(this));
    empS->setContentsMargins(uiMargin(this), uiMargin(this), uiMargin(this), uiMargin(this));
    pb4->setContentsMargins(uiMargin(this), uiMargin(this), uiMargin(this), uiMargin(this));
    ep4->setContentsMargins(uiMargin(this), uiMargin(this), uiMargin(this), uiMargin(this));
    gp4->setContentsMargins(uiMargin(this), uiMargin(this), uiMargin(this), uiMargin(this));

    // ==========================================================
    // Background dynamique (clair / sombre) — écrans Ajouter/Modifier
    // Centralisé ici pour tous les modules : BioSimple, Projet,
    // Expériences, Publication, Équipement, Employé.
    // ==========================================================
    {
        auto prevThemeFn = g_applyThemeFn;
        g_applyThemeFn = [=](bool dark) {
            if (prevThemeFn) prevThemeFn(dark);

            const QString bgImg  = dark ? ":/image/crudeimagesombre.png"
                                        : ":/image/crudeimage.png";

            // Styles du panneau pour exp2 (dark-aware) — alignés sur BioSample
            const QString expPanel   = dark ? "rgba(44,50,58,0.78)"      : "rgba(246,248,247,0.86)";
            const QString expPanelBr = dark ? "rgba(224,232,240,0.18)"   : "rgba(0,0,0,0.10)";
            const QString expSection = dark ? "rgba(255,255,255,0.06)"   : "rgba(244,246,245,0.78)";
            const QString expSectBr  = dark ? "rgba(224,232,240,0.16)"   : "rgba(0,0,0,0.10)";
            const QString expRow     = dark ? "rgba(255,255,255,0.06)"   : "rgba(244,246,245,0.78)";
            const QString expRowBr   = dark ? "rgba(224,232,240,0.16)"   : "rgba(0,0,0,0.10)";
            const QString expTitle   = dark ? "#E8ECF0"                  : "rgba(0,0,0,0.55)";
            const QString expLabel   = dark ? "rgba(210,220,230,0.90)"   : "rgba(0,0,0,0.55)";
            const QString expInputBg = dark ? "rgba(255,255,255,0.06)"   : "rgba(255,255,255,0.92)";
            const QString expInputFg = dark ? "#E8ECF0"                  : "rgba(0,0,0,0.82)";
            const QString expInputBr = dark ? "rgba(224,232,240,0.16)"   : "rgba(0,0,0,0.12)";
            const QString expEqLbl   = dark ? "rgba(210,220,230,0.88)"   : "rgba(0,0,0,0.55)";
            const QString expPopupBg = dark ? "rgba(20,27,34,0.98)"      : "#FFFFFF";
            const QString expPopupSelBg = dark ? "rgba(45,212,191,0.22)" : "rgba(10,95,88,0.16)";
            const QString expPopupSelFg = dark ? "#F4FBFF"               : "#0A5F58";
            const QString expBtnBg   = dark ? "rgba(45,212,191,0.16)"    : "rgba(10,95,88,0.16)";
            const QString expBtnFg   = dark ? "#9AF4E8"                  : "#0A5F58";
            const QString expBtnBr   = dark ? "rgba(45,212,191,0.30)"    : "rgba(10,95,88,0.35)";
            const QString expBtnHover= dark ? "rgba(45,212,191,0.24)"    : "rgba(10,95,88,0.22)";
            const QString expBtnDown = dark ? "rgba(45,212,191,0.32)"    : "rgba(10,95,88,0.28)";

            // Helper : applique le background CRUD sur une page simple (plein écran)
            auto setCrudBg = [&](QWidget* page, const QString& id) {
                page->setStyleSheet(QString(
                    "QWidget#%1 {"
                    "  border-image: url(%2) 0 0 0 0 stretch stretch;"
                    "}").arg(id, bgImg));
            };

            setCrudBg(page2,       "bioFormPage");
            setCrudBg(pub2,        "pubFormPage");
            setCrudBg(equip2,      "equipFormPage");
            setCrudBg(empFormPage, "empFormPage");

            // Couleurs communes gris clair pour tous les panneaux CRUD en mode sombre
            const QString crudPanel   = dark ? "rgba(52,58,67,0.92)"    : QString("rgba(246,248,247,0.86)");
            const QString crudPanelBr = dark ? "rgba(210,218,226,0.22)" : QString("rgba(0,0,0,0.10)");
            const QString crudField   = dark ? "rgba(255,255,255,0.07)" : QString("rgba(244,246,245,0.78)");

            // BioSample form theming
            outer2->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius:14px; }").arg(crudPanel, crudPanelBr));
            for (auto* f : outer2->findChildren<QFrame*>())
                f->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius:12px; }").arg(crudField, crudPanelBr));

            // Publication form theming
            outPUB2->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius:14px; }").arg(crudPanel, crudPanelBr));
            for (auto* f : outPUB2->findChildren<QFrame*>())
                f->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius:12px; }").arg(crudField, crudPanelBr));

            // Equipment form theming
            eqOuter2->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius:14px; }").arg(crudPanel, crudPanelBr));
            for (auto* f : eqOuter2->findChildren<QFrame*>())
                f->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius:12px; }").arg(crudField, crudPanelBr));

            // Employee form dark-aware panel theming
            {
                const QString empPanel   = dark ? "rgba(52,58,67,0.92)"    : QString("rgba(246,248,247,0.86)");
                const QString empPanelBr = dark ? "rgba(210,218,226,0.22)" : QString("rgba(0,0,0,0.10)");
                const QString empField   = dark ? "rgba(255,255,255,0.07)" : QString("rgba(244,246,245,0.78)");
                const QString empTextClr = dark ? "#FFFFFF"                : "rgba(0,0,0,0.55)";

                empOuter2->setStyleSheet(QString(
                    "QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }")
                    .arg(empPanel, empPanelBr));
                empRight2->setStyleSheet(QString(
                    "QFrame{ background:%1; border:1px solid %2; border-radius: 12px; }")
                    .arg(empField, empPanelBr));
                for (auto* f : empRight2->findChildren<QFrame*>())
                    f->setStyleSheet(QString(
                        "QFrame{ background:%1; border:1px solid %2; border-radius: 12px; }")
                        .arg(empField, empPanelBr));
                empDate->setStyleSheet(QString(
                    "QDateEdit{ background: transparent; border:0; font-weight: 900; color: %1; }")
                    .arg(empTextClr));
                empAddDrop->setStyleSheet(QString(
                    "QToolButton{ color: %1; font-weight: 900; }").arg(empTextClr));
                empBottom2->setStyleSheet(dark
                    ? "background: rgba(52,58,67,0.88); border: 1px solid rgba(210,218,226,0.22); border-radius: 14px;"
                    : "background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
            }

            // Employee list table — dark-aware (hardcoded stylesheet must be overridden here)
            empTable->setStyleSheet(QString(R"(
                QTableWidget{
                    background: %1;
                    border: none;
                    gridline-color: %3;
                    color: %2;
                    font-size: 13px;
                }
                QTableWidget::item{
                    color: %2;
                    padding: 4px 6px;
                    border: none;
                }
                QTableWidget::item:selected{
                    background: rgba(10,95,88,0.25);
                    color: %2;
                }
            )").arg(dark ? "#1A1E24" : "#F8FAF9",
                    dark ? "#E8ECF0" : "rgba(0,0,0,0.80)",
                    dark ? "rgba(210,218,226,0.15)" : "rgba(0,0,0,0.07)"));
            empTable->horizontalHeader()->setStyleSheet(QString(R"(
                QHeaderView::section{
                    background: %1; color: %2;
                    border: none; border-right: 1px solid %3;
                    padding: 8px 6px; font-weight: 900;
                }
                QHeaderView::section:hover{ background: %4; }
            )").arg(dark ? "#3A4149" : C_TABLE_HDR,
                    dark ? "#E8ECF0" : "rgba(0,0,0,0.70)",
                    dark ? "rgba(210,218,226,0.15)" : "rgba(0,0,0,0.08)",
                    dark ? "#434B55" : C_BG));

            // exp2 : background + panneaux dark-aware (styles inline déclarés sur la page)
            exp2->setStyleSheet(QString(
                "QWidget#expFormPage{"
                "  border-image: url(%1) 0 0 0 0 stretch stretch;"
                "}"
                "QFrame#expFormPanel{"
                "  background: %2;"
                "  border: 1px solid %3;"
                "  border-radius: 16px;"
                "}"
                "QFrame#expFormSection{"
                "  background: %4;"
                "  border: 1px solid %5;"
                "  border-radius: 12px;"
                "}"
                "QLabel#expFormTitle{"
                "  color: %6;"
                "  font-weight: 900;"
                "  font-size: 14px;"
                "}"
                "QFrame#expFormRow{"
                "  background: %7;"
                "  border: 1px solid %8;"
                "  border-radius: 12px;"
                "}"
                "QLabel#expFormRowLabel{"
                "  color: %9;"
                "  font-weight: 900;"
                "  font-size: 13px;"
                "}"
                "QLineEdit#expFormInput, QComboBox#expFormInput, QDateEdit#expFormInput, QLabel#expFormInput{"
                "  background: %10;"
                "  color: %11;"
                "  border: 1px solid %12;"
                "  border-radius: 12px;"
                "  padding: 7px 12px;"
                "  font-weight: 800;"
                "  font-size: 13px;"
                "}"
                "QComboBox#expFormInput QAbstractItemView{"
                "  background:%14;"
                "  color:%11;"
                "  border:1px solid %12;"
                "  selection-background-color:%15;"
                "  selection-color:%16;"
                "}"
                "QPushButton#expEquipPickBtn{"
                "  background: %17;"
                "  color:%18;"
                "  border:1px solid %19;"
                "  border-radius:10px;"
                "  padding:8px 12px;"
                "  font-weight:900;"
                "}"
                "QPushButton#expEquipPickBtn:hover{ background: %20; }"
                "QPushButton#expEquipPickBtn:pressed{ background: %21; }"
                "QLabel#expEquipSelectedLbl{"
                "  color: %13;"
                "  background: %10;"
                "  border: 1px solid %12;"
                "  border-radius: 12px;"
                "  padding: 6px 12px;"
                "  font-weight: 800;"
                "  font-size: 13px;"
                "}"
            ).arg(bgImg,
                  expPanel,   expPanelBr,
                  expSection, expSectBr,
                  expTitle,
                  expRow,     expRowBr,
                  expLabel,
                  expInputBg, expInputFg, expInputBr,
                  expEqLbl,
                  expPopupBg, expPopupSelBg, expPopupSelFg,
                  expBtnBg, expBtnFg, expBtnBr, expBtnHover, expBtnDown));
            applyGlassShadow(outE2,
                             QColor(0, 0, 0, dark ? 92 : 38),
                             dark ? 46.0 : 28.0,
                             QPointF(0.0, dark ? 14.0 : 10.0));

            // Bottom bar du formulaire expérience
            e2Bottom->setStyleSheet(dark
                ? "background: rgba(34,40,48,0.74); border: 1px solid rgba(224,232,240,0.14); border-radius: 14px;"
                : "background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
            expSave->setStyleSheet(QString(
                "QPushButton{ background:%1; color:%2; border:1px solid %3; border-radius:10px; padding:10px 18px; font-weight:900; }"
                "QPushButton:hover{ background:%4; }"
                "QPushButton:pressed{ background:%5; }")
                .arg(dark ? "rgba(45,212,191,0.16)" : "rgba(10,95,88,0.45)",
                     dark ? "#CFFFF6" : "rgba(255,255,255,0.90)",
                     dark ? "rgba(45,212,191,0.30)" : "rgba(0,0,0,0.12)",
                     dark ? "rgba(45,212,191,0.24)" : "rgba(255,255,255,0.70)",
                     dark ? "rgba(45,212,191,0.32)" : "rgba(255,255,255,0.60)"));
            expCancel->setStyleSheet(QString(
                "QPushButton{ background:%1; color:%2; border:1px solid %3; border-radius:10px; padding:10px 18px; font-weight:800; }"
                "QPushButton:hover{ background:%4; }"
                "QPushButton:pressed{ background:%5; }")
                .arg(dark ? "rgba(255,255,255,0.06)" : "rgba(255,255,255,0.55)",
                     dark ? "#E8ECF0" : C_TEXT_DARK,
                     dark ? "rgba(224,232,240,0.14)" : "rgba(0,0,0,0.12)",
                     dark ? "rgba(255,255,255,0.10)" : "rgba(255,255,255,0.70)",
                     dark ? "rgba(255,255,255,0.14)" : "rgba(255,255,255,0.62)"));
            applyExpCalendar(eDateDebut);
            applyExpCalendar(eDateFin);

            // expDetails : même langage visuel que Ajouter / Modifier
            expDetailsPage->setStyleSheet(QString(
                "QWidget#expDetailsPage{"
                "  border-image: url(%1) 0 0 0 0 stretch stretch;"
                "}"
                "QFrame#expFormPanel{"
                "  background: %2;"
                "  border: 1px solid %3;"
                "  border-radius: 16px;"
                "}"
                "QFrame#expFormSection{"
                "  background: %4;"
                "  border: 1px solid %5;"
                "  border-radius: 12px;"
                "}"
                "QLabel#expFormTitle{"
                "  color: %6;"
                "  font-weight: 900;"
                "  font-size: 14px;"
                "}"
                "QFrame#expFormRow{"
                "  background: %7;"
                "  border: 1px solid %8;"
                "  border-radius: 12px;"
                "}"
                "QLabel#expFormRowLabel{"
                "  color: %9;"
                "  font-weight: 900;"
                "  font-size: 13px;"
                "}"
                "QLabel#expFormInput{"
                "  background: %10;"
                "  color: %11;"
                "  border: 1px solid %12;"
                "  border-radius: 12px;"
                "  padding: 7px 12px;"
                "  font-weight: 800;"
                "  font-size: 13px;"
                "}"
            ).arg(bgImg,
                  expPanel,   expPanelBr,
                  expSection, expSectBr,
                  expTitle,
                  expRow,     expRowBr,
                  expLabel,
                  expInputBg, expInputFg, expInputBr));
        };
        g_applyThemeFn(g_darkThemeEnabled);
    }

    // ── Thème dynamique : page5 dashboard statistiques ─────────
    {
        auto prevThemeFn = g_applyThemeFn;
        g_applyThemeFn = [=](bool dark) {
            if (prevThemeFn) prevThemeFn(dark);

            // ── Couleurs selon le mode ────────────────────────
            const QString cardBg   = dark ? "rgba(28,40,52,0.92)"    : "rgba(255,255,255,0.92)";
            const QString cardBr   = dark ? "rgba(255,255,255,0.09)" : "rgba(0,0,0,0.10)";
            const QString titleClr = dark ? "rgba(80,210,190,1.0)"   : "rgba(10,95,88,1.0)";
            const QString kpiBg    = dark ? "rgba(22,34,46,0.94)"    : "rgba(255,255,255,0.95)";
            const QString kpiBr    = dark ? "rgba(255,255,255,0.09)" : "rgba(0,0,0,0.10)";
            const QString subClr   = dark ? "rgba(120,145,160,1.0)"  : "rgba(90,110,125,1.0)";

            // ── Cartes graphiques ─────────────────────────────
            const QString cardSS = QString(
                "QFrame#statCard{"
                "  background:%1; border:1px solid %2; border-radius:14px;"
                "}"
                "QLabel#statCardTitle{"
                "  color:%3; font-size:12px; font-weight:900;"
                "  border:none; background:none;"
                "}").arg(cardBg, cardBr, titleClr);

            for (QFrame* f : page5->findChildren<QFrame*>())
                if (f->objectName() == "statCard") f->setStyleSheet(cardSS);

            // ── KPI cards ─────────────────────────────────────
            const QString kpiSS = QString(
                "QFrame#kpiCard{"
                "  background:%1; border:1px solid %2; border-radius:14px;"
                "}").arg(kpiBg, kpiBr);

            for (QFrame* f : page5->findChildren<QFrame*>())
                if (f->objectName() == "kpiCard") f->setStyleSheet(kpiSS);

            // Couleurs accentuées des valeurs KPI
            struct { QLabel* lbl; const char* dark_col; const char* light_col; } kpiDef[] = {
                { kpiTotalVal,   "#2DD4BF", "#0A5F58" },
                { kpiTypesVal,   "#818CF8", "#4F46E5" },
                { kpiMonthVal,   "#FBBF24", "#B45309" },
                { kpiTopRefVal,  "#FB7185", "#BE123C" },
                { kpiTopTempVal, "#93C5FD", "#1D4ED8" },
            };
            for (auto& k : kpiDef)
                k.lbl->setStyleSheet(QString(
                    "color:%1; font-size:24px; font-weight:900; background:none; border:none;"
                ).arg(dark ? k.dark_col : k.light_col));

            // Labels subtitle KPI
            for (QLabel* lbl : page5->findChildren<QLabel*>())
                if (lbl->objectName() == "kpiTitle")
                    lbl->setStyleSheet(QString(
                        "color:%1; font-size:10px; font-weight:700;"
                        " background:none; border:none;").arg(subClr));

            // ── Barre du bas ──────────────────────────────────
            for (QFrame* f : page5->findChildren<QFrame*>())
                if (f->objectName() == "statsBottom")
                    f->setStyleSheet(QString(
                        "QFrame#statsBottom{"
                        "  background:%1; border:1px solid %2; border-radius:14px;"
                        "}").arg(kpiBg, kpiBr));

            // ── Scrollbar selon le thème ──────────────────────
            const QString handleClr = dark ? "rgba(45,212,191,0.50)" : "rgba(10,95,88,0.35)";
            const QString trackClr  = dark ? "rgba(255,255,255,0.06)" : "rgba(0,0,0,0.06)";
            statsScroll->setStyleSheet(QString(R"(
                QScrollArea#statsScroll { background: transparent; border: none; }
                QScrollArea#statsScroll > QWidget > QWidget { background: transparent; }
                QScrollBar:vertical {
                    background: %1; width: 6px;
                    border-radius: 3px; margin: 0px;
                }
                QScrollBar::handle:vertical {
                    background: %2; border-radius: 3px; min-height: 30px;
                }
                QScrollBar::handle:vertical:hover { background: %3; }
                QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
                QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }
            )").arg(trackClr, handleClr,
                    dark ? "rgba(45,212,191,0.85)" : "rgba(10,95,88,0.65)"));

            // ── Graphiques ────────────────────────────────────
            pie->setDark(dark);
            tempPie->setDark(dark);
            bars->setDark(dark);
            hbars->setDark(dark);
        };
        g_applyThemeFn(g_darkThemeEnabled);
    }

    // ── Thème dynamique : exp3 statistiques expériences ──────
    {
        auto prevThemeFn = g_applyThemeFn;
        g_applyThemeFn = [=](bool dark) {
            if (prevThemeFn) prevThemeFn(dark);

            const QString pageBg  = dark ? "#13181F"                  : C_BG;
            const QString panelBg = dark ? "rgba(28,40,52,0.92)"      : C_PANEL_BG;
            const QString panelBr = dark ? "rgba(255,255,255,0.09)"   : C_PANEL_BR;
            const QString cardBg  = dark ? "rgba(22,34,46,0.94)"      : "rgba(255,255,255,0.55)";
            const QString cardBr  = dark ? "rgba(210,218,226,0.22)"   : "rgba(0,0,0,0.10)";
            const QString actBg   = dark ? "rgba(22,34,46,0.94)"      : "rgba(255,255,255,0.35)";
            const QString titleClr= dark ? "rgba(80,210,190,1.0)"     : "rgba(0,0,0,0.55)";
            const QString bottomBg= dark ? "rgba(22,34,46,0.94)"      : "rgba(255,255,255,0.20)";
            const QString bottomBr= dark ? "rgba(255,255,255,0.09)"   : "rgba(0,0,0,0.08)";

            exp3->setStyleSheet(QString("QWidget { background: %1; }").arg(pageBg));
            outE3->setStyleSheet(QString(
                "QFrame{ background:%1; border:1px solid %2; border-radius:14px; }")
                .arg(panelBg, panelBr));
            actE3->setStyleSheet(QString(
                "QFrame{ background:%1; border:1px solid %2; border-radius:12px; }")
                .arg(actBg, cardBr));
            dashE3->setStyleSheet(QString(
                "QFrame{ background:%1; border:1px solid %2; border-radius:12px; }")
                .arg(cardBg, cardBr));
            pieE->setStyleSheet(QString(
                "QFrame{ background:%1; border:1px solid %2; border-radius:12px; }")
                .arg(cardBg, cardBr));
            barE->setStyleSheet(QString(
                "QFrame{ background:%1; border:1px solid %2; border-radius:12px; }")
                .arg(cardBg, cardBr));
            he->setStyleSheet(QString("color:%1; font-weight:900;").arg(titleClr));
            pieET->setStyleSheet(QString("color:%1; font-weight:900;").arg(titleClr));
            barET->setStyleSheet(QString("color:%1; font-weight:900;").arg(titleClr));
            e3Bottom->setStyleSheet(QString(
                "background:%1; border:1px solid %2; border-radius:14px;")
                .arg(bottomBg, bottomBr));
            donutE->setDark(dark);
            barsE->setDark(dark);

            const QString eqPanelBg   = dark ? "rgba(26,34,44,0.90)"      : "rgba(255,255,255,0.22)";
            const QString eqPanelBr   = dark ? "rgba(225,235,245,0.14)"   : "rgba(0,0,0,0.08)";
            const QString eqCardBg    = dark ? "rgba(255,255,255,0.05)"   : "rgba(255,255,255,0.58)";
            const QString eqCardBr    = dark ? "rgba(225,235,245,0.12)"   : "rgba(0,0,0,0.10)";
            const QString eqTitleClr  = dark ? "#9AF4E8"                  : "rgba(0,0,0,0.62)";
            const QString eqTextClr   = dark ? "rgba(226,234,241,0.92)"   : "rgba(0,0,0,0.56)";
            const QString eqHintClr   = dark ? "rgba(180,196,208,0.78)"   : "rgba(0,0,0,0.48)";
            const QString eqTreeSelBg = dark ? "rgba(45,212,191,0.18)"    : "rgba(10,95,88,0.10)";
            const QString eqTreeHover = dark ? "rgba(255,255,255,0.06)"   : "rgba(255,255,255,0.38)";
            const QString eqBtnBg     = dark ? "rgba(45,212,191,0.18)"    : C_PRIMARY;
            const QString eqBtnHover  = dark ? "rgba(45,212,191,0.28)"    : C_TOPBAR;
            const QString eqBtnBorder = dark ? "rgba(45,212,191,0.30)"    : "rgba(0,0,0,0.18)";
            const QString eqBottomBg  = dark ? "rgba(22,34,46,0.94)"      : "rgba(255,255,255,0.20)";
            const QString eqBottomBr  = dark ? "rgba(255,255,255,0.09)"   : "rgba(0,0,0,0.08)";

            eqOuter3->setStyleSheet(QString(
                "QFrame{ background:%1; border:1px solid %2; border-radius:14px; }")
                .arg(eqPanelBg, eqPanelBr));
            eqLeft3->setStyleSheet(QString(
                "QFrame{ background:%1; border:1px solid %2; border-radius:12px; }")
                .arg(eqCardBg, eqCardBr));
            eqRight3->setStyleSheet(QString(
                "QFrame{ background:%1; border:1px solid %2; border-radius:12px; }")
                .arg(eqCardBg, eqCardBr));
            eqHeader3->setStyleSheet(QString(
                "QFrame{ background:%1; border:1px solid %2; border-radius:12px; }")
                .arg(eqCardBg, eqCardBr));
            eqDonutCard3->setStyleSheet(QString(
                "QFrame{ background:%1; border:1px solid %2; border-radius:12px; }")
                .arg(eqCardBg, eqCardBr));
            eqBarCard3->setStyleSheet(QString(
                "QFrame{ background:%1; border:1px solid %2; border-radius:12px; }")
                .arg(eqCardBg, eqCardBr));
            eqFocusCard3->setStyleSheet(QString(
                "QFrame{ background:%1; border:1px solid %2; border-radius:12px; }")
                .arg(eqCardBg, eqCardBr));
            eqBottom3->setStyleSheet(QString(
                "background:%1; border:1px solid %2; border-radius:14px;")
                .arg(eqBottomBg, eqBottomBr));
            eqDetails3->setStyleSheet(QString(R"(
                QPushButton{
                    background:%1; color: rgba(255,255,255,0.96);
                    border:1px solid %3; border-radius:12px;
                    padding: 10px 16px; font-weight:900;
                }
                QPushButton:hover{ background:%2; }
            )").arg(eqBtnBg, eqBtnHover, eqBtnBorder));

            for (QLabel* titleLbl : {eqTreeTitle3, eqDonutTitle3, eqBarTitle3, eqFocusTitle3})
                titleLbl->setStyleSheet(QString("color:%1; font-weight:900;").arg(eqTitleClr));
            for (QLabel* hintLbl : {eqTreeHint3, eqDonutSub3, eqBarSub3})
                hintLbl->setStyleSheet(QString("color:%1; font-weight:600;").arg(eqHintClr));
            for (QLabel* chipLbl : {eqTotalChip3, eqModelChip3, eqDateChip3})
                chipLbl->setStyleSheet(QString(
                    "background:%1; border:1px solid %2; border-radius:12px; padding:8px 12px; font-weight:900; color:%3;")
                    .arg(dark ? "rgba(255,255,255,0.05)" : "rgba(255,255,255,0.90)",
                         dark ? "rgba(225,235,245,0.12)" : "rgba(0,0,0,0.10)",
                         eqTextClr));
            eqFocusName3->setStyleSheet(QString("color:%1; font-weight:900; font-size:18px;").arg(eqTitleClr));
            for (QLabel* infoLbl : {eqFocusLine13, eqFocusLine23, eqFocusLine33, eqFocusLine43})
                infoLbl->setStyleSheet(QString("color:%1; font-weight:700;").arg(eqTextClr));

            eqTree->setStyleSheet(QString(R"(
                QTreeWidget{
                    background: transparent;
                    border:none;
                    outline:none;
                    color:%1;
                }
                QTreeWidget::item{
                    padding:8px 6px;
                    border-radius:8px;
                }
                QTreeWidget::item:hover{
                    background:%2;
                }
                QTreeWidget::item:selected{
                    background:%3;
                    color:%4;
                }
            )").arg(eqTextClr, eqTreeHover, eqTreeSelBg, dark ? "#F4FBFF" : "#0A5F58"));

            eqDonut3->setDark(dark);
            eqBars3->setDark(dark);
        };
        g_applyThemeFn(g_darkThemeEnabled);
    }

    // ==========================================================
    // ==========================================================
    // NAVIGATION BioSimple — CRUD complet
    // ==========================================================

    auto clearTempValidationUi = [=](){
        errTemp->hide();
        cbTemp2->setStyleSheet(tempFieldNormalStyle);
    };
    auto failTempValidation = [=](const QString& message){
        errTemp->setText("⚠  " + message);
        errTemp->show();
        cbTemp2->setStyleSheet(tempFieldErrorStyle);
        cbTemp2->setFocus();
        cbTemp2->selectAll();
        showToast(this, message, false);
    };
    QObject::connect(cbTemp2, &QLineEdit::textChanged, this, [=](const QString& text){
        const QString trimmed = text.trimmed();
        bool ok = false;
        const int value = trimmed.toInt(&ok);
        if (trimmed.isEmpty() || (ok && value >= -273 && value <= 100)) {
            clearTempValidationUi();
        }
    });

    // ── AJOUTER : vider le formulaire, passer en mode création ──
    QObject::connect(btnAdd, &QPushButton::clicked, this, [=]{
        *bioEditMode = false;
        bioEditRef->clear();
        leRef->clear();
        leRef->setReadOnly(false);
        cbType2->clear();
        cbOrg2->clear();
        leCongelateur->clear();
        leEtagere->clear();
        emplacPopup->setVisible(false);
        qty->setValue(0);
        cbTemp2->clear();
        clearTempValidationUi();
        cbDanger->setCurrentIndex(0);
        dCollect->setDate(QDate::currentDate());
        dExpire->setDate(QDate::currentDate().addDays(30));
        loadProjetCombo();
        cbProjet->setCurrentIndex(0);
        setWindowTitle("Ajouter un échantillon");
        stack->setCurrentIndex(BIO_FORM);
    });

    // ── MODIFIER : charger la ligne sélectionnée dans le formulaire ──
    QObject::connect(btnEdit, &QPushButton::clicked, this, [=]{
        int r = table->currentRow();
        if (r < 0) {
            QMessageBox::information(this, "Information",
                "Veuillez sélectionner un échantillon à modifier.");
            return;
        }
        // REFERENCE stored in col-0 UserRole by CrudeBioSimple::loadAll
        QString ref = table->item(r, 0)
                          ? table->item(r, 0)->data(Qt::UserRole).toString()
                          : QString();
        if (ref.isEmpty()) {
            QMessageBox::warning(this, "Erreur", "Référence introuvable dans la ligne.");
            return;
        }
        BioSample s = crud->get(ref);
        *bioEditMode = true;
        *bioEditRef  = ref;
        leRef->setText(s.reference);
        leRef->setReadOnly(false);
        cbType2->setText(s.type);
        cbOrg2->setText(s.organisme);
        {
            QString emp = s.emplacement;
            int slash = emp.indexOf("/Etag:");
            if (emp.startsWith("Cong:") && slash >= 0) {
                leCongelateur->setText(emp.mid(5, slash - 5));
                leEtagere->setText(emp.mid(slash + 6));
            } else {
                leCongelateur->setText(emp);
                leEtagere->clear();
            }
            emplacPopup->setVisible(true);
        }
        qty->setValue(s.quantite);
        cbTemp2->setText(s.temperature);
        clearTempValidationUi();
        cbDanger->setCurrentText(s.niveauDanger);
        if (s.dateCollecte.isValid())   dCollect->setDate(s.dateCollecte);
        if (s.dateExpiration.isValid()) dExpire->setDate(s.dateExpiration);
        // Select project in combo
        loadProjetCombo();
        for (int i = 0; i < cbProjet->count(); ++i) {
            if (cbProjet->itemData(i).toInt() == s.idProjet) {
                cbProjet->setCurrentIndex(i);
                break;
            }
        }
        setWindowTitle("Modifier un échantillon");
        stack->setCurrentIndex(BIO_FORM);
    });

    // ── SUPPRIMER : confirmation puis DELETE en BDD ──
    QObject::connect(btnDel, &QPushButton::clicked, this, [=]{
        if (*currentRole != "Responsable") {
            showToast(this, "Accès refusé : seul le Responsable peut supprimer.", false);
            return;
        }
        int r = table->currentRow();
        if (r < 0) {
            QMessageBox::information(this, "Information",
                "Veuillez sélectionner une ligne à supprimer.");
            return;
        }
        QString ref = table->item(r, 0)
                          ? table->item(r, 0)->data(Qt::UserRole).toString()
                          : QString();
        auto cell = [&](int c) -> QString {
            auto* it = table->item(r, c);
            return it ? it->text() : "";
        };
        QString resume =
            QString("Réf : %1   |   Type : %2   |   Organisme : %3   |   Expiration : %4")
                .arg(cell(0), cell(2), cell(3), cell(7));
        ConfirmDeleteDialog confirm(style(), resume, this);
        if (confirm.exec() == QDialog::Accepted) {
            if (ref.isEmpty() || !crud->remove(ref)) {
                showToast(this, "Impossible de supprimer cet échantillon.", false);
                return;
            }
            showToast(this, QString("Échantillon « %1 » supprimé.").arg(ref), true);
            crud->loadAll(table);
        }
    });

    // ── ENREGISTRER : INSERT ou UPDATE selon le mode ──
    QObject::connect(saveBtn, &QPushButton::clicked, this, [=]{
        // ── Reset all error labels ──
        for (auto* e : {errRef, errCollect, errExpire, errQty, errTemp, errDanger,
                        errType, errOrg, errEmplac, errProjet})
            e->hide();
        clearTempValidationUi();

        QString ref = leRef->text().trimmed();

        // ① Référence
        if (ref.isEmpty()) { showToast(this, "Référence : Ce champ est obligatoire.", false); return; }
        static const QRegularExpression validRef("^REF\\d{1,5}$");
        if (!validRef.match(ref).hasMatch()) { showToast(this, "Référence : Format invalide. Utilisez REF suivi de 1 à 5 chiffres (ex: REF12345).", false); return; }
        // Vérifier doublon si : mode ajout, OU mode édition avec référence modifiée
        if (!*bioEditMode || ref != *bioEditRef) {
            QSqlQuery dup;
            dup.prepare("SELECT COUNT(1) FROM \"BioSample\" WHERE \"Reference_de_léchantillon\" = ?");
            dup.addBindValue(ref);
            if (dup.exec() && dup.next() && dup.value(0).toInt() > 0) { showToast(this, "Référence : Cette référence est déjà utilisée.", false); return; }
        }

        // ② Type
        if (cbType2->text().trimmed().isEmpty()) { showToast(this, "Type d'échantillon : Ce champ est obligatoire.", false); return; }

        // ③ Organisme
        if (cbOrg2->text().trimmed().isEmpty()) { showToast(this, "Organisme source : Ce champ est obligatoire.", false); return; }

        // ④ Emplacement
        if (leCongelateur->text().trimmed().isEmpty() || leEtagere->text().trimmed().isEmpty()) {
            emplacPopup->setVisible(true);
            showToast(this, "Emplacement : Renseignez le congélateur et l'étagère.", false); return;
        }

        // ⑤ Quantité
        if (qty->value() <= 0) { showToast(this, "Quantité : La valeur doit être supérieure à 0.", false); return; }

        // ⑥ Température
        const QString tempText = cbTemp2->text().trimmed();
        if (tempText.isEmpty()) {
            failTempValidation("La température de stockage est obligatoire.");
            return;
        }
        bool tempOk = false;
        const int tempValue = tempText.toInt(&tempOk);
        if (!tempOk || tempValue < -273 || tempValue > 100) {
            failTempValidation("Veuillez renseigner une température valide entre -273°C et 100°C.");
            return;
        }

        // ⑦ Niveau de danger
        if (cbDanger->currentIndex() == 0) { showToast(this, "Niveau BSL : Veuillez sélectionner un niveau.", false); return; }

        // ⑧ Date de collecte
        if (!*bioEditMode && dCollect->date() < QDate::currentDate()) { showToast(this, "Date de collecte : Ne peut pas être antérieure à aujourd'hui.", false); return; }

        // ⑨ Date d'expiration
        if (dExpire->date() <= dCollect->date()) { showToast(this, "Date d'expiration : Doit être postérieure à la date de collecte.", false); return; }

        // ⑩ Projet
        if (cbProjet->currentData().toInt() <= 0) { showToast(this, "Projet associé : Veuillez sélectionner un projet.", false); return; }

        // ── Build BioSample from form ──
        BioSample s;
        s.reference      = ref;
        s.type           = cbType2->text().trimmed();
        s.organisme      = cbOrg2->text().trimmed();
        s.emplacement    = QString("Cong:%1/Etag:%2")
                           .arg(leCongelateur->text().trimmed(),
                                leEtagere->text().trimmed());
        s.quantite       = qty->value();
        s.temperature    = tempText;
        s.niveauDanger   = cbDanger->currentText();
        s.dateCollecte   = dCollect->date();
        s.dateExpiration = dExpire->date();
        s.idProjet       = cbProjet->currentData().toInt();

        bool ok = false;
        if (*bioEditMode) {
            ok = crud->update(s, *bioEditRef);
            if (ok)
                showToast(this, QString("Échantillon « %1 » modifié avec succès.").arg(ref), true);
            else
                showToast(this, QString("Échec de la mise à jour : %1").arg(crud->lastError()), false);
        } else {
            ok = crud->add(s);
            if (ok)
                showToast(this, QString("Échantillon « %1 » ajouté avec succès.").arg(ref), true);
            else
                showToast(this, QString("Échec de l'ajout : %1").arg(crud->lastError()), false);
        }

        if (ok) {
            crud->loadAll(table);
            setWindowTitle("Échantillons");
            stack->setCurrentIndex(BIO_LIST);
        }
    });

    // ── ANNULER ──
    QObject::connect(cancelBtn, &QPushButton::clicked, this, [=]{
        setWindowTitle("Échantillons");
        stack->setCurrentIndex(BIO_LIST);
    });

    // ── Localisation & Stockage ──
    QObject::connect(btnMore,  &QPushButton::clicked, this, [=]{
        setWindowTitle("Localisation & Stockage");
        buildTree3();
        if (tree3->topLevelItemCount() > 0) {
            auto* first = tree3->topLevelItem(0);
            tree3->setCurrentItem(first);
            QString cong = first->data(0, Qt::UserRole).toString();
            if (!cong.isEmpty()) {
                loadSamples3(cong, QString());
            }
        }
        stack->setCurrentIndex(BIO_LOC);
    });
    QObject::connect(back3, &QPushButton::clicked, this, [=]{
        setWindowTitle("Échantillons");
        stack->setCurrentIndex(BIO_LIST);
    });
    QObject::connect(back4, &QPushButton::clicked, this, [=]{
        setWindowTitle("Localisation & Stockage");
        stack->setCurrentIndex(BIO_LOC);
    });

    // ── STATISTIQUES — mise à jour depuis la BDD avant affichage ──
    // Helper interne : vider un layout de ses widgets dynamiques
    auto clearLayout = [](QVBoxLayout* l) {
        while (l->count() > 0) {
            QLayoutItem* item = l->takeAt(0);
            if (item->widget()) delete item->widget();
            delete item;
        }
    };

    auto updateBioStats = [=]{
        const bool dark = g_darkThemeEnabled;

        // ── KPI 1 : Total ──────────────────────────────────────
        kpiTotalVal->setText(QString::number(crud->totalCount()));

        // ── KPI 2 : Types distincts ────────────────────────────
        kpiTypesVal->setText(QString::number(crud->countDistinctTypes()));

        // ── KPI 3 : Collectés ce mois ──────────────────────────
        kpiMonthVal->setText(QString::number(crud->countThisMonth()));

        // ── KPI 4 : Référence max ──────────────────────────────
        {
            QString ref = crud->topReference();
            if (ref.length() > 12) ref = ref.left(11) + "…";
            kpiTopRefVal->setText(ref);
        }

        // ── KPI 5 : Température dominante ─────────────────────
        kpiTopTempVal->setText(crud->topTemperature());

        // ── Donut 1 : Types d'échantillons ────────────────────
        {
            auto typeMap = crud->countByType();
            QList<DonutChart::Slice> slices;
            int ci = 0;
            for (auto it = typeMap.cbegin(); it != typeMap.cend(); ++it, ++ci) {
                QColor col = statsTypePalette[ci % statsTypePalette.size()];
                slices.append({(double)it.value(), col, it.key()});
            }
            pie->setDark(dark);
            pie->setData(slices);

            clearLayout(typeLgL);
            for (const auto& sl : slices) {
                QWidget* row = new QWidget;
                QHBoxLayout* h = new QHBoxLayout(row);
                h->setContentsMargins(0,1,0,1); h->setSpacing(8);
                QFrame* dot = new QFrame;
                dot->setFixedSize(10,10);
                dot->setStyleSheet(QString("background:%1; border-radius:5px;").arg(sl.color.name()));
                QLabel* lab = new QLabel(QString("%1  (%2)").arg(sl.label).arg((int)sl.value));
                lab->setStyleSheet("color: rgba(180,205,215,0.90); font-size:11px; font-weight:700;"
                                   " background:none; border:none;");
                h->addWidget(dot); h->addWidget(lab, 1);
                typeLgL->addWidget(row);
            }
        }

        // ── Donut 2 : Températures ─────────────────────────────
        {
            auto tempMap = crud->countByTemperature();
            QList<DonutChart::Slice> slices;
            int ci = 0;
            for (auto it = tempMap.cbegin(); it != tempMap.cend(); ++it, ++ci) {
                QColor col = statsTempPalette[ci % statsTempPalette.size()];
                slices.append({(double)it.value(), col, it.key()});
            }
            tempPie->setDark(dark);
            tempPie->setData(slices);

            clearLayout(tempLgL);
            for (const auto& sl : slices) {
                QWidget* row = new QWidget;
                QHBoxLayout* h = new QHBoxLayout(row);
                h->setContentsMargins(0,1,0,1); h->setSpacing(8);
                QFrame* dot = new QFrame;
                dot->setFixedSize(10,10);
                dot->setStyleSheet(QString("background:%1; border-radius:5px;").arg(sl.color.name()));
                QLabel* lab = new QLabel(QString("%1  (%2)").arg(sl.label).arg((int)sl.value));
                lab->setStyleSheet("color: rgba(180,205,215,0.90); font-size:11px; font-weight:700;"
                                   " background:none; border:none;");
                h->addWidget(dot); h->addWidget(lab, 1);
                tempLgL->addWidget(row);
            }
        }

        // ── Bar chart mensuel ──────────────────────────────────
        {
            auto monthVec = crud->countByMonth();
            QList<BarChart::Bar> barList;
            for (auto& pr : monthVec)
                barList.append({(double)pr.first, pr.second});
            bars->setDark(dark);
            bars->setData(barList);
        }

        // ── Horizontal bar : quantité restante ────────────────
        {
            auto topQ = crud->topByQuantity(8);
            QList<HorizontalBarChart::Bar> hList;
            for (auto& pr : topQ)
                hList.append({(double)pr.second, pr.first});
            hbars->setDark(dark);
            hbars->setData(hList);
        }
    };

    QObject::connect(btnSec,   &QPushButton::clicked, this, [=]{
        updateBioStats();
        setWindowTitle("Statistiques BioSimple");
        stack->setCurrentIndex(BIO_STATS);
    });
    QObject::connect(btnStats, &QPushButton::clicked, this, [=]{
        updateBioStats();
        setWindowTitle("Statistiques BioSimple");
        stack->setCurrentIndex(BIO_STATS);
    });
    QObject::connect(back5, &QPushButton::clicked, this, [=]{
        setWindowTitle("Échantillons");
        stack->setCurrentIndex(BIO_LIST);
    });

    // ==========================================================
    // NAVIGATION Gestion Projet (3 widgets) — CRUD complet
    // ==========================================================
    // Helper: clear funding source fields
    auto clearSourceRows = [=](){
        projFinancement->clear();
        projFinancement->setStyleSheet(projFieldStyle());
        projBudgetSpin->setValue(0.0);
        projBudgetSpin->setStyleSheet(projBudgetStyle());
    };

    auto clearProjForm = [=](){
        *projEditMode = false;
        *projEditId = 0;
        projName->clear();
        projName->setStyleSheet(projFieldStyle());
        errProjName->hide();
        projDomainEdit->setCurrentIndex(0);
        projDomainEdit->setStyleSheet(projComboStyle());
        errProjDomain->hide();
        projStatus->setCurrentIndex(0); // default = Planifié
        projStatus->setStyleSheet(projComboStyle());
        projEthique->clear();
        projEthique->setStyleSheet(projFieldStyle());
        errProjEthique->hide();
        projStart->setDate(QDate::currentDate());
        projStart->setStyleSheet(projDateStyle());
        errProjStart->hide();
        projEnd->setDate(QDate::currentDate().addMonths(3));
        projEnd->setStyleSheet(projDateStyle());
        errProjEnd->hide();
        projPubsEdit->setValue(0);
        projPubsEdit->setStyleSheet(projPubsStyle());
        clearSourceRows();
        applyProjCalendar(projStart);
        applyProjCalendar(projEnd);
    };

    QObject::connect(projAdd, &QPushButton::clicked, this, [=](){
        clearProjForm();
        setWindowTitle("Ajouter un projet");
        if (projFormHeaderTitle) projFormHeaderTitle->setText("Ajouter un projet");
        stack->setCurrentIndex(PROJ_FORM);
    });
    QObject::connect(projEdit, &QPushButton::clicked, this, [=](){
        const int row = projTable->currentRow();
        if (row < 0 || !projTable->item(row, 1)) {
            ThemedAlertDialog::show(style(), this, "info", "Projet", "Sélectionnez un projet dans la liste.");
            return;
        }

        const int id = projTable->item(row, 1)->data(Qt::UserRole).toInt();
        ProjetRecord rec;
        QString err;
        if (!projCrud->fetchProjet(id, rec, &err)) {
            showToast(this, "Erreur : " + err, false);
            return;
        }

        *projEditMode = true;
        *projEditId = id;

        projName->setText(rec.nomDuProjet);
        projName->setStyleSheet(projFieldStyle());
        errProjName->hide();

        { int di = projDomainEdit->findText(rec.domaineDeRecherche, Qt::MatchFixedString);
            projDomainEdit->setCurrentIndex(di >= 0 ? di : 0); }
        projDomainEdit->setStyleSheet(projComboStyle());
        errProjDomain->hide();

        projEthique->setText(rec.numeroDApprobationEthique);
        projEthique->setStyleSheet(projFieldStyle());
        errProjEthique->hide();

        projStart->setDate(rec.dateDeDebut.isValid() ? rec.dateDeDebut : QDate::currentDate());
        projStart->setStyleSheet(projDateStyle());
        errProjStart->hide();
        projEnd->setDate(rec.dateDeFin.isValid() ? rec.dateDeFin : QDate::currentDate().addMonths(3));
        projEnd->setStyleSheet(projDateStyle());
        errProjEnd->hide();

        projPubsEdit->setValue(rec.nombreDePublications);
        projPubsEdit->setStyleSheet(projPubsStyle());

        {
            QString statTxt = rec.statut.trimmed();
            int idx = projStatus->findText(statTxt, Qt::MatchFixedString | Qt::MatchCaseSensitive);
            projStatus->setCurrentIndex(idx >= 0 ? idx : 0);
        }

        // Load single funding source
        projFinancement->setText(rec.sourceDeFinancement);
        projFinancement->setStyleSheet(projFieldStyle());
        projBudgetSpin->setValue(rec.budget);
        projBudgetSpin->setStyleSheet(projBudgetStyle());
        applyProjCalendar(projStart);
        applyProjCalendar(projEnd);

        setWindowTitle("Modifier un projet");
        if (projFormHeaderTitle) projFormHeaderTitle->setText("Modifier un projet");
        stack->setCurrentIndex(PROJ_FORM);
    });
    QObject::connect(projCancel, &QPushButton::clicked, this, [=](){
        clearProjForm();
        setWindowTitle("Projet");
        stack->setCurrentIndex(PROJ_LIST);
    });
    QObject::connect(projSave, &QPushButton::clicked, this, [=](){
        ProjetRecord rec;
        rec.idProjet = *projEditMode ? *projEditId : 0;
        rec.nomDuProjet = projName->text().trimmed();
        rec.domaineDeRecherche = projDomainEdit->currentText().startsWith("—") ? QString() : projDomainEdit->currentText();
        rec.dateDeDebut = projStart->date();
        rec.dateDeFin   = projEnd->date();
        rec.budget = projBudgetSpin->value();
        rec.statut = projStatus->currentText();
        rec.sourceDeFinancement = projFinancement->text().trimmed();
        rec.numeroDApprobationEthique = projEthique->text().trimmed();
        rec.nombreDePublications = projPubsEdit->value();

        QString err;
        ProjetRecord oldRec;
        if (*projEditMode) {
            if (!projCrud->fetchProjet(rec.idProjet, oldRec, &err)) {
                showToast(this, "Erreur lors de la récupération du projet : " + err, false);
                return;
            }
        }

        const bool ok = *projEditMode ? projCrud->updateProjet(rec, &err)
                                      : projCrud->insertProjet(rec, &err);
        if (!ok) {
            // Show the error as a themed popup for invalid/duplicate project names
            // and for finance source / budget validation errors; otherwise use toast.
            const bool nameAlert = err.contains("nom du projet", Qt::CaseInsensitive)
                                || err.contains("projet portant ce nom", Qt::CaseInsensitive)
                                || (err.contains("nom", Qt::CaseInsensitive) && err.contains("projet", Qt::CaseInsensitive));
            const bool financAlert = err.contains("financement", Qt::CaseInsensitive)
                                  || err.contains("budget", Qt::CaseInsensitive)
                                  || err.contains("source", Qt::CaseInsensitive);
            if (nameAlert || financAlert) {
                // Determine alert type: use "error" for ❌ errors, "warning" for ⚠ warnings
                const QString alertType = (err.contains("❌") || err.contains("insuffisant", Qt::CaseInsensitive))
                                          ? "error" : "warning";
                ThemedAlertDialog::show(style(), this, alertType, "Projet — Financement", err);
            } else {
                showToast(this, err, false);
            }
            // Highlight fields based on error keywords
            if (err.contains("nom", Qt::CaseInsensitive))
            { projName->setStyleSheet(projFieldStyle(true)); errProjName->setText("⚠  " + err); errProjName->show(); }
            else if (err.contains("domaine", Qt::CaseInsensitive))
            { projDomainEdit->setStyleSheet(projComboStyle(true)); errProjDomain->setText("⚠  " + err); errProjDomain->show(); }
            else if (err.contains("éthique", Qt::CaseInsensitive) || err.contains("ethique", Qt::CaseInsensitive))
            { projEthique->setStyleSheet(projFieldStyle(true)); errProjEthique->setText("⚠  " + err); errProjEthique->show(); }
            else if (err.contains("début", Qt::CaseInsensitive) || err.contains("debut", Qt::CaseInsensitive))
            { projStart->setStyleSheet(projDateStyle(true)); errProjStart->setText("⚠  " + err); errProjStart->show(); }
            else if (err.contains("fin", Qt::CaseInsensitive) || err.contains("durée", Qt::CaseInsensitive))
            { projEnd->setStyleSheet(projDateStyle(true)); errProjEnd->setText("⚠  " + err); errProjEnd->show(); }
            return;
        }

        // Log tracability BEFORE clearing the form (which resets projEditMode)
        if (*projEditMode) {
            TracabiliteManager::logModificationProjet(oldRec, rec);
        } else {
            TracabiliteManager::logAjoutProjet(rec);

            // ── Send confirmation email to the connected user only ─────────
            configureEmailSenderFromLocalConfig(smtpConfigError);
            if (EmailSender::instance()->isConfigured()) {
                const QString subject = QString("✅ Nouveau projet créé — %1").arg(rec.nomDuProjet);

                // Shared project info fields (same for every recipient)
                const QString projDomaine   = rec.domaineDeRecherche.isEmpty()        ? "—" : rec.domaineDeRecherche;
                const QString projFinanc    = rec.sourceDeFinancement.isEmpty()        ? "—" : rec.sourceDeFinancement;
                const QString projEthique2  = rec.numeroDApprobationEthique.isEmpty()  ? "—" : rec.numeroDApprobationEthique;

                const QString recipientEmail = currentUserEmail->trimmed();
                const QString recipientName = currentUserFullName->trimmed().isEmpty()
                                              ? recipientEmail
                                              : currentUserFullName->trimmed();

                if (recipientEmail.isEmpty()) {
                    appendSmtpLog("[UI] Project notification skipped reason=email utilisateur connecté indisponible");
                } else {
                    const QString body = QString(R"(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8"/>
    <style>
        body {
            font-family: Arial, "Segoe UI", sans-serif;
            background: #EEF6F4;
            margin: 0;
            padding: 26px 14px;
            color: #2F3A39;
        }
        .wrapper {
            max-width: 680px;
            margin: 0 auto;
            background: #FFFFFF;
            border-radius: 10px;
            overflow: hidden;
            border: 1px solid #C8DCD9;
            box-shadow: 0 8px 24px rgba(10, 95, 88, 0.10);
        }
        .header {
            background: #FFFFFF;
            border-bottom: 2px solid #C8DCD9;
            padding: 20px 28px 14px;
            text-align: left;
        }
        .header h1 {
            color: #0A5F58;
            margin: 0;
            font-size: 44px;
            line-height: 1.1;
            letter-spacing: 0;
            font-weight: 800;
        }
        .header p {
            color: #7A7A7A;
            margin: 8px 0 0;
            font-size: 15px;
        }
        .body {
            padding: 24px 28px 20px;
            background: #FFFFFF;
            border-top: 1px solid #E3EFED;
        }
        .body h2 {
            color: #1F2728;
            font-size: 42px;
            line-height: 1.1;
            margin-top: 0;
            margin-bottom: 16px;
            font-weight: 800;
        }
        .body p {
            color: #2F3A39;
            font-size: 14px;
            line-height: 1.55;
        }
        .card {
            background: #F5FAF9;
            border: 1px solid #C8DCD9;
            border-radius: 8px;
            padding: 14px 16px;
            margin: 16px 0;
        }
        .card .row {
            display: flex;
            justify-content: space-between;
            align-items: flex-start;
            gap: 14px;
            padding: 8px 0;
            border-bottom: 1px solid #D9E9E6;
        }
        .card .row:last-child {
            border-bottom: none;
        }
        .card .lbl {
            color: #0A5F58;
            font-size: 16px;
            font-weight: 700;
        }
        .card .val {
            color: #1F2728;
            font-weight: 500;
            font-size: 16px;
            text-align: right;
            max-width: 58%%;
            word-break: break-word;
        }
        .app-section {
            margin-top: 22px;
            border-top: 1px solid #D9E9E6;
            padding-top: 18px;
        }
        .app-section h3 {
            color: #0A5F58;
            background: #E8F5F3;
            border: 1px solid #C8DCD9;
            border-radius: 6px;
            font-size: 20px;
            margin: 0 0 12px;
            padding: 10px 14px;
        }
        .app-section ul {
            padding-left: 18px;
            color: #2F3A39;
            font-size: 14px;
            line-height: 1.65;
            margin-bottom: 0;
        }
        .app-section li {
            margin-bottom: 4px;
        }
        .footer {
            background: #F5FAF9;
            border-top: 1px solid #C8DCD9;
            text-align: center;
            padding: 14px;
            font-size: 12px;
            color: #6E7E7B;
        }
    </style>
</head>
<body style="margin:0;padding:26px 14px;background:#EEF6F4;color:#2F3A39;font-family:Arial,'Segoe UI',sans-serif;">
<div class="wrapper" style="max-width:680px;margin:0 auto;background:#FFFFFF;border-radius:10px;overflow:hidden;border:1px solid #C8DCD9;box-shadow:0 8px 24px rgba(10,95,88,.10);">
    <div class="header" style="background:#FFFFFF;border-bottom:2px solid #C8DCD9;padding:20px 28px 14px;text-align:left;">
        <h1 style="margin:0;color:#0A5F58;font-size:44px;line-height:1.1;font-weight:800;">SmartVision</h1>
        <p style="margin:8px 0 0;color:#7A7A7A;font-size:15px;">Notification de création de projet</p>
  </div>
    <div class="body" style="padding:24px 28px 20px;background:#FFFFFF;border-top:1px solid #E3EFED;">
        <h2 style="margin:0 0 16px;color:#1F2728;font-size:42px;line-height:1.1;font-weight:800;">Bonjour %1,</h2>
        <p style="margin:0 0 12px;color:#2F3A39;font-size:14px;line-height:1.55;">Un nouveau projet a été enregistré dans la plateforme
       <strong>SmartVision BioSimple</strong> par <strong>%2</strong>.</p>
        <div class="card" style="background:#F5FAF9;border:1px solid #C8DCD9;border-radius:8px;padding:14px 16px;margin:16px 0;">
            <div class="row" style="display:flex;justify-content:space-between;align-items:flex-start;gap:14px;padding:8px 0;border-bottom:1px solid #D9E9E6;"><span class="lbl" style="color:#0A5F58;font-size:16px;font-weight:700;">Nom du projet</span>        <span class="val" style="color:#1F2728;font-weight:500;font-size:16px;text-align:right;max-width:58%;word-break:break-word;">%3</span></div>
            <div class="row" style="display:flex;justify-content:space-between;align-items:flex-start;gap:14px;padding:8px 0;border-bottom:1px solid #D9E9E6;"><span class="lbl" style="color:#0A5F58;font-size:16px;font-weight:700;">Domaine de recherche</span>  <span class="val" style="color:#1F2728;font-weight:500;font-size:16px;text-align:right;max-width:58%;word-break:break-word;">%4</span></div>
            <div class="row" style="display:flex;justify-content:space-between;align-items:flex-start;gap:14px;padding:8px 0;border-bottom:1px solid #D9E9E6;"><span class="lbl" style="color:#0A5F58;font-size:16px;font-weight:700;">Date de début</span>         <span class="val" style="color:#1F2728;font-weight:500;font-size:16px;text-align:right;max-width:58%;word-break:break-word;">%5</span></div>
            <div class="row" style="display:flex;justify-content:space-between;align-items:flex-start;gap:14px;padding:8px 0;border-bottom:1px solid #D9E9E6;"><span class="lbl" style="color:#0A5F58;font-size:16px;font-weight:700;">Date de fin</span>           <span class="val" style="color:#1F2728;font-weight:500;font-size:16px;text-align:right;max-width:58%;word-break:break-word;">%6</span></div>
            <div class="row" style="display:flex;justify-content:space-between;align-items:flex-start;gap:14px;padding:8px 0;border-bottom:1px solid #D9E9E6;"><span class="lbl" style="color:#0A5F58;font-size:16px;font-weight:700;">Budget alloué</span>         <span class="val" style="color:#1F2728;font-weight:500;font-size:16px;text-align:right;max-width:58%;word-break:break-word;">%7 DT</span></div>
            <div class="row" style="display:flex;justify-content:space-between;align-items:flex-start;gap:14px;padding:8px 0;border-bottom:1px solid #D9E9E6;"><span class="lbl" style="color:#0A5F58;font-size:16px;font-weight:700;">Statut</span>                <span class="val" style="color:#1F2728;font-weight:500;font-size:16px;text-align:right;max-width:58%;word-break:break-word;">%8</span></div>
            <div class="row" style="display:flex;justify-content:space-between;align-items:flex-start;gap:14px;padding:8px 0;border-bottom:1px solid #D9E9E6;"><span class="lbl" style="color:#0A5F58;font-size:16px;font-weight:700;">Source de financement</span> <span class="val" style="color:#1F2728;font-weight:500;font-size:16px;text-align:right;max-width:58%;word-break:break-word;">%9</span></div>
            <div class="row" style="display:flex;justify-content:space-between;align-items:flex-start;gap:14px;padding:8px 0;"><span class="lbl" style="color:#0A5F58;font-size:16px;font-weight:700;">N° approbation éthique</span><span class="val" style="color:#1F2728;font-weight:500;font-size:16px;text-align:right;max-width:58%;word-break:break-word;">%10</span></div>
    </div>
        <div class="app-section" style="margin-top:22px;border-top:1px solid #D9E9E6;padding-top:18px;">
            <h3 style="color:#0A5F58;background:#E8F5F3;border:1px solid #C8DCD9;border-radius:6px;font-size:20px;margin:0 0 12px;padding:10px 14px;">À propos SmartVision</h3>
      <p style="color:#444;font-size:13px;">
                <strong>SmartVision</strong> est une platforme de gestion de laboratoire
        intégrée conçue pour centraliser et sécuriser toutes les activités de recherche
        scientifique. Voici ce que vous pouvez réaliser depuis l'application :
      </p>
    <ul style="padding-left:18px;color:#2F3A39;font-size:14px;line-height:1.65;margin-bottom:0;">
        <li><strong>Échantillons Biologiques</strong> — Suivi complet avec dates d'expiration,
            niveaux de dangerosité, quantités et alertes automatiques.</li>
        <li><strong>Projets</strong> — Planification, budget, indicateurs de santé
            (délais, impact scientifique, conformité éthique).</li>
        <li><strong>Expériences</strong> — Enregistrement et suivi des protocoles associés
            aux projets de recherche.</li>
        <li><strong>Équipements</strong> — Inventaire, maintenance et disponibilité du matériel.</li>
        <li><strong>Employés</strong> — Fiches RH, rôles et permissions par profil.</li>
        <li><strong>Publications</strong> — Référencement avec DOI, auteurs et statut.</li>
        <li><strong>Tableau de Bord</strong> — Statistiques visuelles (donut, barres, radar).</li>
        <li><strong>Chatbot IA</strong> — Assistant intelligent sur les données du laboratoire.</li>
        <li><strong>Traçabilité</strong> — Journal complet des actions, export PDF pour audit.</li>
        <li><strong>Authentification</strong> — Email/mot de passe, Face ID ou Google OAuth.</li>
        <li><strong>Notifications e-mail</strong> — Alertes automatiques pour tous les utilisateurs.</li>
      </ul>
    </div>
  </div>
    <div class="footer" style="background:#F5FAF9;border-top:1px solid #C8DCD9;text-align:center;padding:14px;font-size:12px;color:#6E7E7B;">
    © %11 SmartVision BioSimple — Message généré automatiquement, merci de ne pas y répondre.
  </div>
</div>
</body>
</html>
)")
                    .arg(recipientName)                                          // %1  greeting
                    .arg(*currentUserFullName)                                   // %2  creator
                    .arg(rec.nomDuProjet)                                        // %3
                    .arg(projDomaine)                                            // %4
                    .arg(rec.dateDeDebut.toString("dd/MM/yyyy"))                 // %5
                    .arg(rec.dateDeFin.toString("dd/MM/yyyy"))                   // %6
                    .arg(QString::number(rec.budget, 'f', 2))                   // %7
                    .arg(rec.statut)                                             // %8
                    .arg(projFinanc)                                             // %9
                    .arg(projEthique2)                                           // %10
                    .arg(QDate::currentDate().year());                           // %11

                    EmailSender::instance()->send(recipientEmail, subject, body);
                }
            } else {
                appendSmtpLog(QString("[UI] Project notification skipped reason=%1").arg(*smtpConfigError));
                showToast(this,
                          QString("Projet enregistré, mais l'e-mail n'a pas été envoyé : %1")
                              .arg(smtpConfigError->isEmpty()
                                       ? QString("configuration SMTP manquante")
                                       : *smtpConfigError),
                          false);
            }
        }

        clearProjForm();
        loadProjTable();
        applyProjFilters();

        setWindowTitle("Projet");
        stack->setCurrentIndex(PROJ_LIST);
        showToast(this, "Projet enregistré.", true);
    });
    QObject::connect(projDetails, &QPushButton::clicked, this, [=](){
        if (!updateProjDetailsFromRow()) return;
        setWindowTitle("Détails projet");
        stack->setCurrentIndex(PROJ_DETAILS);
    });
    QObject::connect(p3Back, &QPushButton::clicked, this, [=](){
        setWindowTitle("Projet");
        stack->setCurrentIndex(PROJ_LIST);
    });

    // ── Répartition des projets par domaine (gestproj stat) ──
    QObject::connect(projDetailsBack, &QPushButton::clicked, this, [=](){
        setWindowTitle("Projet");
        stack->setCurrentIndex(PROJ_LIST);
    });

    QObject::connect(projBtnStats, &QPushButton::clicked, this, [=](){
        // Rebuild pie chart from live projTable data
        QMap<ProjStatus,int> counts;
        for (int r = 0; r < projTable->rowCount(); ++r) {
            if (projTable->isRowHidden(r)) continue;
            if (projTable->item(r,6)) {
                ProjStatus ps = static_cast<ProjStatus>(projTable->item(r,6)->data(Qt::UserRole).toInt());
                counts[ps]++;
            }
        }
        QList<DonutChart::Slice> slices;
        if (counts.value(ProjStatus::EnCours)  > 0) slices.append({(double)counts[ProjStatus::EnCours],  QColor("#416e66"), "En cours"});
        if (counts.value(ProjStatus::Planifie) > 0) slices.append({(double)counts[ProjStatus::Planifie], W_ORANGE,          "Planifié"});
        if (counts.value(ProjStatus::EnRetard) > 0) slices.append({(double)counts[ProjStatus::EnRetard], W_RED,             "En retard"});
        if (counts.value(ProjStatus::Critique) > 0) slices.append({(double)counts[ProjStatus::Critique], QColor("#8B2F3C"), "Critique"});
        if (counts.value(ProjStatus::Suspendu) > 0) slices.append({(double)counts[ProjStatus::Suspendu], W_GRAY,            "Suspendu"});
        if (counts.value(ProjStatus::Termine)  > 0) slices.append({(double)counts[ProjStatus::Termine],  QColor("#429787"), "Terminé"});
        if (counts.value(ProjStatus::Annule)  > 0) slices.append({(double)counts[ProjStatus::Annule],   QColor("#84091a"), "Annulé"});
        if (!slices.isEmpty()) pd->setData(slices);
        setWindowTitle("Statistiques Projet");
        stack->setCurrentIndex(PROJ_STATS);
    });

    QObject::connect(btnDomaineProj, &QPushButton::clicked, this, [=](){ GestProjCrud::showDomaineChart(this); });
    QObject::connect(btnAnalyseBudget, &QPushButton::clicked, this, [=](){ GestProjCrud::showAnalyseBudgetaire(this); });
    QObject::connect(btnStatutProj, &QPushButton::clicked, this, [=](){ GestProjCrud::showStatutChart(this); });
    QObject::connect(btnEvolutionProj, &QPushButton::clicked, this, [=](){ GestProjCrud::showEvolutionChart(this); });

    QObject::connect(btnTimelineProj, &QPushButton::clicked, this, [=](){
        struct GanttRow { QString name; QDate start; QDate end; QString statut; };
        QList<GanttRow> rows;
        {
            QSqlQuery q;
            q.prepare("SELECT \"nom_du_projet\", \"date_de_début\", \"date_de_fin\", \"statut\" "
                      "FROM \"projet\" ORDER BY \"date_de_début\", \"nom_du_projet\"");
            if (q.exec()) {
                while (q.next()) {
                    GanttRow r;
                    r.name   = q.value(0).toString();
                    r.start  = q.value(1).toDate();
                    r.end    = q.value(2).toDate();
                    r.statut = q.value(3).toString().trimmed();
                    if (!r.start.isValid()) continue;
                    if (!r.end.isValid()) r.end = r.start.addMonths(3);
                    rows.append(r);
                }
            }
        }

        QDialog* dlg = new QDialog(this, Qt::Dialog|Qt::WindowTitleHint|Qt::WindowCloseButtonHint);
        dlg->setWindowTitle("Timeline des projets — Vue Gantt");
        dlg->setMinimumSize(900, 500);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->setStyleSheet("QDialog{ background:#1C2A35; }");

        QVBoxLayout* dl = new QVBoxLayout(dlg);
        dl->setContentsMargins(20,18,20,18); dl->setSpacing(12);

        QLabel* hdr = new QLabel("🗓️  Timeline des projets");
        hdr->setStyleSheet("color:white; font-size:15px; font-weight:900;"
            "background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #518195,stop:1 #3d6475);"
            "border-radius:10px; padding:10px 16px;");
        dl->addWidget(hdr);

        if (rows.isEmpty()) {
            QLabel* empty = new QLabel("Aucun projet trouvé dans la base de données.");
            empty->setAlignment(Qt::AlignCenter);
            empty->setStyleSheet("color:rgba(255,255,255,0.5); font-size:13px;");
            dl->addWidget(empty, 1);
        } else {
            QDate globalMin = rows[0].start, globalMax = rows[0].end;
            for (const GanttRow& r : rows) {
                if (r.start < globalMin) globalMin = r.start;
                if (r.end   > globalMax) globalMax = r.end;
            }
            globalMin = QDate(globalMin.year(), globalMin.month(), 1);
            globalMax = QDate(globalMax.year(), globalMax.month(), 1).addMonths(1);

            auto statutColor = [](const QString& s) -> QColor {
                const QString sl = s.toLower();
                if (sl=="en cours")   return QColor("#2E6F63");
                if (sl=="terminé"||sl=="termine") return QColor("#4A90E2");
                if (sl=="planifié"||sl=="planifie") return QColor("#B5672C");
                if (sl=="suspendu")   return QColor("#7A8B8A");
                if (sl=="annulé"||sl=="annule") return QColor("#8B2F3C");
                if (sl=="en retard")  return QColor("#C0392B");
                if (sl=="critique")   return QColor("#922B21");
                return QColor("#518195");
            };

            const int rowH=38, labelW=210, padding=8;
            const int totalDays = globalMin.daysTo(globalMax);

            QScrollArea* scroll = new QScrollArea;
            scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame);
            scroll->setStyleSheet("QScrollArea{background:transparent;border:none;}"
                "QScrollBar:horizontal{height:10px;background:rgba(255,255,255,0.08);border-radius:5px;}"
                "QScrollBar::handle:horizontal{background:rgba(81,129,149,0.6);border-radius:5px;}"
                "QScrollBar:vertical{width:10px;background:rgba(255,255,255,0.08);border-radius:5px;}"
                "QScrollBar::handle:vertical{background:rgba(81,129,149,0.6);border-radius:5px;}");

            QWidget* canvas = new QWidget;
            const int canvasW = labelW+padding*2+qMax(600,totalDays*2);
            const int canvasH = 30+rows.size()*rowH+10;
            canvas->setMinimumSize(canvasW, canvasH);
            canvas->setStyleSheet("background:transparent;");

            struct GanttPF : public QObject {
                QList<GanttRow> rows; QDate gMin;
                int totalDays, labelW, rowH, padding;
                std::function<QColor(const QString&)> colorFn;
                bool eventFilter(QObject* obj, QEvent* ev) override {
                    if (ev->type()!=QEvent::Paint) return false;
                    QWidget* w = static_cast<QWidget*>(obj);
                    QPainter p(w); p.setRenderHint(QPainter::Antialiasing);
                    const int chartW = w->width()-labelW-padding*2;
                    const double dayPx = (double)chartW/totalDays;
                    p.fillRect(w->rect(), QColor(28,42,53));
                    {
                        QDate cur=gMin;
                        while(cur<gMin.addDays(totalDays)){
                            int x=labelW+padding+(int)(gMin.daysTo(cur)*dayPx);
                            int nx=labelW+padding+(int)(gMin.daysTo(cur.addMonths(1))*dayPx);
                            bool alt=((cur.month()+cur.year())%2==0);
                            p.fillRect(QRect(x,0,nx-x,28),alt?QColor(81,129,149,40):QColor(81,129,149,20));
                            p.setPen(QColor(255,255,255,60));
                            p.drawLine(x,0,x,w->height());
                            QFont mf; mf.setPointSize(8); mf.setBold(true); p.setFont(mf);
                            p.setPen(QColor(255,255,255,160));
                            p.drawText(QRect(x,0,nx-x,28).adjusted(4,0,0,0),Qt::AlignVCenter|Qt::AlignLeft,cur.toString("MMM yy"));
                            cur=cur.addMonths(1);
                        }
                    }
                    {
                        int tx=labelW+padding+(int)(gMin.daysTo(QDate::currentDate())*dayPx);
                        if(tx>labelW&&tx<w->width()){
                            p.setPen(QPen(QColor(255,200,60,180),1.5,Qt::DashLine));
                            p.drawLine(tx,28,tx,w->height());
                        }
                    }
                    for(int i=0;i<rows.size();++i){
                        const GanttRow& r=rows[i]; int y=30+i*rowH;
                        p.fillRect(0,y,w->width(),rowH,(i%2==0)?QColor(255,255,255,8):QColor(0,0,0,0));
                        p.setPen(QColor(255,255,255,200));
                        QFont lf; lf.setPointSize(9); lf.setBold(true); p.setFont(lf);
                        QString lbl=r.name.length()>26?r.name.left(24)+"…":r.name;
                        p.drawText(QRect(6,y,labelW-10,rowH),Qt::AlignVCenter|Qt::AlignLeft,lbl);
                        int barX=labelW+padding+(int)(gMin.daysTo(r.start)*dayPx);
                        int barW=qMax(8,(int)(r.start.daysTo(r.end)*dayPx));
                        QRectF bar(barX,y+7,barW,rowH-16);
                        p.setPen(Qt::NoPen); p.setBrush(colorFn(r.statut));
                        p.drawRoundedRect(bar,5,5);
                        if(barW>50){
                            QFont bf; bf.setPointSize(8); bf.setBold(true); p.setFont(bf);
                            p.setPen(QColor(255,255,255,220));
                            p.drawText(bar.adjusted(6,0,-2,0),Qt::AlignVCenter|Qt::AlignLeft,r.statut);
                        }
                        p.setPen(QPen(QColor(255,255,255,12),1));
                        p.drawLine(0,y+rowH-1,w->width(),y+rowH-1);
                    }
                    return true;
                }
            };
            auto* gpf=new GanttPF;
            gpf->rows=rows; gpf->gMin=globalMin; gpf->totalDays=totalDays;
            gpf->labelW=labelW; gpf->rowH=rowH; gpf->padding=padding;
            gpf->colorFn=statutColor;
            canvas->installEventFilter(gpf);
            scroll->setWidget(canvas); dl->addWidget(scroll,1);

            QFrame* leg=new QFrame;
            leg->setStyleSheet("QFrame{background:rgba(255,255,255,0.05);border-radius:8px;}");
            QHBoxLayout* ll=new QHBoxLayout(leg);
            ll->setContentsMargins(12,8,12,8); ll->setSpacing(16);
            auto addLeg=[&](const QString& label,const QColor& col){
                QLabel* dot=new QLabel("●");
                dot->setStyleSheet(QString("color:%1;font-size:14px;background:transparent;").arg(col.name()));
                QLabel* txt=new QLabel(label);
                txt->setStyleSheet("color:rgba(255,255,255,0.70);font-size:10px;background:transparent;");
                ll->addWidget(dot); ll->addWidget(txt);
            };
            addLeg("En cours",QColor("#2E6F63")); addLeg("Terminé",QColor("#4A90E2"));
            addLeg("Planifié",QColor("#B5672C")); addLeg("En retard",QColor("#C0392B"));
            addLeg("Suspendu",QColor("#7A8B8A")); addLeg("Annulé",QColor("#8B2F3C"));
            addLeg("Critique",QColor("#922B21")); ll->addStretch(1);
            QLabel* todayLeg=new QLabel("— Aujourd'hui");
            todayLeg->setStyleSheet("color:rgba(255,200,60,0.80);font-size:10px;background:transparent;");
            ll->addWidget(todayLeg); dl->addWidget(leg);
        }

        QHBoxLayout* fbl=new QHBoxLayout; fbl->addStretch(1);
        QPushButton* closeBtn=new QPushButton("Fermer"); closeBtn->setFixedSize(100,34);
        closeBtn->setStyleSheet("QPushButton{background:#518195;color:white;border-radius:8px;font-weight:700;font-size:12px;} QPushButton:hover{background:#3d6475;}");
        QObject::connect(closeBtn,&QPushButton::clicked,dlg,&QDialog::accept);
        fbl->addWidget(closeBtn); dl->addLayout(fbl);
        dlg->exec();
    });

    QObject::connect(btnRapportFinancier, &QPushButton::clicked, this, [=](){
           GestProjCrud::generateFinancialReport(0, 0, this);
       });

    QObject::connect(projMore, &QPushButton::clicked, this, [=](){
        setWindowTitle("Métier Avancé — Projet");
        stack->setCurrentIndex(PROJ_ADVANCED);
    });

    QObject::connect(advBack, &QPushButton::clicked, this, [=](){
        setWindowTitle("Projet");
        stack->setCurrentIndex(PROJ_LIST);
    });

    QObject::connect(projExportPdf, &QToolButton::clicked, this, [=](){
        QString fileName = QFileDialog::getSaveFileName(this, "Exporter PDF", "projets.pdf", "PDF Files (*.pdf)");
        if (fileName.isEmpty()) return;
        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(fileName);
        QTextDocument doc;
        QString html = "<h2 style='color:#0A5F58;'>Projets de Recherche</h2>"
                       "<table border='1' cellpadding='6' cellspacing='0' width='100%' style='border-collapse:collapse;'>"
                       "<tr style='background:#AFC6C3;'><th>Nom du projet</th><th>Domaine</th>"
                       "<th>Date début</th><th>Date fin</th><th>Budget</th>"
                       "<th>Statut</th><th>Financement</th><th>Publications</th></tr>";
        for (int r = 0; r < projTable->rowCount(); ++r) {
            if (projTable->isRowHidden(r)) continue;
            ProjStatus ps = static_cast<ProjStatus>(projTable->item(r,6)->data(Qt::UserRole).toInt());
            html += "<tr>";
            html += "<td>" + (projTable->item(r,1) ? projTable->item(r,1)->text() : "") + "</td>";
            html += "<td>" + (projTable->item(r,2) ? projTable->item(r,2)->text() : "") + "</td>";
            html += "<td>" + (projTable->item(r,3) ? projTable->item(r,3)->text() : "") + "</td>";
            html += "<td>" + (projTable->item(r,4) ? projTable->item(r,4)->text() : "") + "</td>";
            html += "<td>" + (projTable->item(r,5) ? projTable->item(r,5)->text() : "") + "</td>";
            html += "<td>" + projStatusText(ps) + "</td>";
            html += "<td>" + (projTable->item(r,7) ? projTable->item(r,7)->text() : "") + "</td>";
            html += "<td>" + (projTable->item(r,9) ? projTable->item(r,9)->text() : "") + "</td>";
            html += "</tr>";
        }
        html += "</table>";
        doc.setHtml(html);
        doc.print(&printer);
        showToast(this, "PDF exporté avec succès.", true);
    });

    // Exports (démo)
    QObject::connect(export4, &QPushButton::clicked, this, [=](){
        ThemedAlertDialog::show(style(), this, "info", "Export", "Export rapport (à connecter à PDF/Excel).");
    });

    QObject::connect(exportP3, &QPushButton::clicked, this, [=](){
        QString fileName = QFileDialog::getSaveFileName(this, "Exporter Statistiques PDF", "statistiques_projets.pdf", "PDF Files (*.pdf)");
        if (fileName.isEmpty()) return;

        QMap<QString,int> statusCount;
        double totalBudget = 0.0;
        int totalPubs = 0;
        for (int r = 0; r < projTable->rowCount(); ++r) {
            if (projTable->isRowHidden(r)) continue;
            if (projTable->item(r,6)) {
                ProjStatus ps = static_cast<ProjStatus>(projTable->item(r,6)->data(Qt::UserRole).toInt());
                statusCount[projStatusText(ps)]++;
            }
            if (projTable->item(r,5)) totalBudget += projTable->item(r,5)->text().toDouble();
            if (projTable->item(r,9)) totalPubs   += projTable->item(r,9)->text().toInt();
        }

        QString html =
            "<h2 style='color:#0A5F58;'>Statistiques des Projets de Recherche</h2>"
            "<p style='color:#555;'>Généré le " + QDate::currentDate().toString("dd/MM/yyyy") + "</p>"
                                                            "<hr/>"
                                                            "<h3 style='color:#12443B;'>Répartition par statut</h3>"
                                                            "<table border='1' cellpadding='6' cellspacing='0' width='60%' style='border-collapse:collapse;'>"
                                                            "<tr style='background:#AFC6C3;'><th>Statut</th><th>Nombre de projets</th></tr>";
        for (auto it = statusCount.begin(); it != statusCount.end(); ++it)
            html += "<tr><td>" + it.key() + "</td><td>" + QString::number(it.value()) + "</td></tr>";
        html +=
            "</table><br/>"
            "<h3 style='color:#12443B;'>Indicateurs globaux</h3>"
            "<table border='1' cellpadding='6' cellspacing='0' width='60%' style='border-collapse:collapse;'>"
            "<tr style='background:#AFC6C3;'><th>Indicateur</th><th>Valeur</th></tr>"
            "<tr><td>Total projets</td><td>" + QString::number(projTable->rowCount()) + "</td></tr>"
                                                       "<tr><td>Budget total</td><td>" + QString::number(totalBudget, 'f', 2) + " TND</td></tr>"
                                                     "<tr><td>Publications totales</td><td>" + QString::number(totalPubs) + "</td></tr>"
                                           "</table>";

        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(fileName);
        QTextDocument doc;
        doc.setHtml(html);
        doc.print(&printer);
        showToast(this, "Statistiques exportées en PDF.", true);
    });
    // ==========================================================
    // NAVIGATION Expériences / Protocoles (3 widgets)
    // ==========================================================
    QObject::connect(expAdd, &QPushButton::clicked, this, [=](){
        *expEditMode = false; *expEditId = 0;
        loadProjetsIntoCombo();
        loadExperienceEquipments();
        eName->clear(); eHypo->clear(); eTypeExp->clear(); eResultat->clear();
        eDateDebut->setDate(QDate::currentDate());
        eDateFin->setDate(QDate::currentDate().addDays(1));
        applyExpStatusControl();
        eProjet->setCurrentIndex(0);
        setWindowTitle("Ajouter une expérience");
        stack->setCurrentIndex(EXP_FORM);
    });
    QObject::connect(expEdit, &QPushButton::clicked, this, [=](){
        int r = expTable->currentRow();
        if (r < 0) { showToast(this, "Sélectionnez une expérience.", false); return; }
        int id = expTable->item(r, 0)->data(Qt::UserRole).toInt();
        ExperienceRecord rec; QString err;
        if (!expCrud->fetchExperience(id, rec, &err)) { showToast(this, "Erreur : " + err, false); return; }
        *expEditMode = true; *expEditId = id;
        loadProjetsIntoCombo();
        eName->setText(rec.titre);
        eHypo->setText(rec.hypothese);
        eTypeExp->setText(rec.typeExperience);
        eResultat->setText(rec.resultat);
        eDateDebut->setDate(rec.dateDebut.isValid() ? rec.dateDebut : QDate::currentDate());
        eDateFin->setDate(rec.dateFin.isValid() ? rec.dateFin : QDate::currentDate().addDays(1));
        applyExpStatusControl();
        {
            int idx = eStatus->findText(rec.status);
            if (idx >= 0) eStatus->setCurrentIndex(idx);
            if (eDateFin->date() > QDate::currentDate()) eStatus->setCurrentIndex(1);
        }
        int linkedEqId = -1;
        {
            QSqlQuery q;
            q.prepare("SELECT \"equipement_id\" FROM \"Équipement\" WHERE \"Id_exp\" = :id ORDER BY \"equipement_id\"");
            q.bindValue(":id", id);
            if (q.exec() && q.next()) linkedEqId = q.value(0).toInt();
        }
        loadExperienceEquipments(linkedEqId);

        if (!rec.projetId.isNull()) {
            int idx = eProjet->findData(rec.projetId.toInt());
            if (idx >= 0) eProjet->setCurrentIndex(idx);
        }
        setWindowTitle("Modifier une expérience");
        stack->setCurrentIndex(EXP_FORM);
    });
    QObject::connect(expCancel, &QPushButton::clicked, this, [=](){
        setWindowTitle("Expériences");
        stack->setCurrentIndex(EXP_LIST);
    });
    QObject::connect(expSave, &QPushButton::clicked, this, [=](){
        const QString nameTxt = eName->text().trimmed();
        const QString hypoTxt = eHypo->text().trimmed();
        const QString typeTxt = eTypeExp->text().trimmed();
        const QString resultatTxt = eResultat->text().trimmed();
        if (nameTxt.isEmpty()) { showToast(this, "Saisir le nom de l'expérience.", false); return; }
        if (typeTxt.isEmpty()) { showToast(this, "Saisir le Type_Experience.", false); return; }
        if (resultatTxt.isEmpty()) { showToast(this, "Saisir le Resultat.", false); return; }
        if (eStatus->currentIndex() == 0)      { showToast(this, "Sélectionner un statut.", false); return; }
        if (eProjet->currentData().isNull())   { showToast(this, "Sélectionner un projet.", false); return; }

        // Nom d'expérience unique (insensible à la casse)
        {
            QSqlQuery uq;
            uq.prepare(
                "SELECT COUNT(*) FROM \"Expérience\" "
                "WHERE LOWER(TRIM(\"Titre\")) = LOWER(TRIM(:titre)) "
                "AND (:id = 0 OR \"Id_exp\" <> :id)");
            uq.bindValue(":titre", nameTxt);
            uq.bindValue(":id", *expEditMode ? *expEditId : 0);
            if (!uq.exec() || !uq.next()) {
                showToast(this, "Erreur vérification unicité nom expérience.", false);
                return;
            }
            if (uq.value(0).toInt() > 0) {
                showToast(this, "Le nom d'expérience doit être unique.", false);
                return;
            }
        }

        QRegularExpression hasDigits("\\d");
        if (hasDigits.match(nameTxt).hasMatch()) {
            showToast(this, "Le nom ne doit pas contenir de chiffres.", false); return;
        }
        if (!hypoTxt.isEmpty() && hasDigits.match(hypoTxt).hasMatch()) {
            showToast(this, "L'hypothese ne doit pas contenir de chiffres.", false); return;
        }
        if (hasDigits.match(typeTxt).hasMatch()) {
            showToast(this, "Le Type_Experience ne doit pas contenir de chiffres.", false); return;
        }
        if (hasDigits.match(resultatTxt).hasMatch()) {
            showToast(this, "Le Resultat ne doit pas contenir de chiffres.", false); return;
        }
        if (*expSelectedEquipId <= 0) {
            showToast(this, "Choisir un équipement disponible.", false); return;
        }

        const QDate today = QDate::currentDate();
        const QDate dStart = eDateDebut->date();
        const QDate dEnd = eDateFin->date();
        if (dStart >= dEnd) {
            showToast(this, "La date debut doit etre inferieure a la date fin.", false); return;
        }
        QString computedStatus;
        if (dEnd > today) {
            computedStatus = "En cours";
        } else {
            const QString chosen = eStatus->currentText();
            if (chosen != "Échouée" && chosen != "Réussie") {
                showToast(this, "Quand la date fin est inferieure ou egale a la date systeme, choisissez Échouée ou Réussie.", false);
                return;
            }
            computedStatus = chosen;
        }
        ExperienceRecord rec;
        QString err;
        const int expId = *expEditMode ? *expEditId : expCrud->nextExperienceId(&err);
        if (expId <= 0) { showToast(this, "Erreur ID expérience : " + err, false); return; }

        rec.id        = expId;
        rec.titre     = nameTxt;
        rec.hypothese = hypoTxt;
        rec.dateDebut = dStart;
        rec.dateFin   = dEnd;
        rec.status    = computedStatus;
        rec.typeExperience = typeTxt;
        rec.resultat = resultatTxt;
        rec.disponibiliteEquipement = "Non disponible";
        rec.projetId  = eProjet->currentData().isNull() ? QVariant() : QVariant(eProjet->currentData().toInt());

        bool ok = *expEditMode ? expCrud->updateExperience(rec, &err) : expCrud->insertExperience(rec, &err);
        if (!ok) { showToast(this, "Erreur : " + err, false); return; }

        {
            EquipementRecord eqRec;
            QString eqErr;
            if (expEquipCrud->fetchEquipement(*expSelectedEquipId, eqRec, &eqErr)) {
                eqRec.idExp = expId;
                eqRec.statut = (dEnd < today) ? "Actif" : "Hors service";
                if (!expEquipCrud->updateEquipement(eqRec, &eqErr)) {
                    showToast(this, "Expérience enregistrée, mais équipement non mis à jour : " + eqErr, false);
                }
            } else {
                showToast(this, "Expérience enregistrée, mais équipement introuvable : " + eqErr, false);
            }
        }

        showToast(this, *expEditMode ? "Expérience modifiée." : "Expérience ajoutée.", true);
        loadExpTable();
        setWindowTitle("Expériences");
        stack->setCurrentIndex(EXP_LIST);
    });
    QObject::connect(expStats, &QPushButton::clicked, this, [=](){
        updateExpStats();
        setWindowTitle("Statistiques Expériences");
        stack->setCurrentIndex(EXP_STATS);
    });
    QObject::connect(expDetails, &QPushButton::clicked, this, [=](){
        if (!updateExpDetailsFromRow()) return;
        setWindowTitle("Détails expérience");
        stack->setCurrentIndex(EXP_DETAILS);
    });
    QObject::connect(expBackStats, &QPushButton::clicked, this, [=](){
        setWindowTitle("Expériences");
        stack->setCurrentIndex(EXP_LIST);
    });
    QObject::connect(expDetailsBack, &QPushButton::clicked, this, [=](){
        setWindowTitle("Expériences");
        stack->setCurrentIndex(EXP_LIST);
    });

    QObject::connect(eExportBtn, &QPushButton::clicked, this, [=](){
        const int r = expTable->currentRow();
        if (r < 0) {
            showToast(this, "Sélectionnez une expérience à exporter.", false);
            return;
        }

        const auto cellText = [&](int col) -> QString {
            QTableWidgetItem* it = expTable->item(r, col);
            if (!it) return QString();
            if (col == 4) {
                const QString badgeText = it->data(Qt::UserRole + 1).toString();
                if (!badgeText.isEmpty()) return badgeText;
            }
            return it->text();
        };

        const int id = expTable->item(r, 0)->data(Qt::UserRole).toInt();
        ExperienceRecord rec;
        QString err;
        if (!expCrud->fetchExperience(id, rec, &err)) {
            showToast(this, "Erreur export : " + err, false);
            return;
        }

        QString suggestedName = cellText(0).trimmed();
        if (suggestedName.isEmpty()) suggestedName = "experience";
        suggestedName.replace("/", "_").replace("\\", "_").replace(":", "_")
                     .replace("*", "_").replace("?", "_").replace("\"", "_")
                     .replace("<", "_").replace(">", "_").replace("|", "_");

        QString path = QFileDialog::getSaveFileName(
            this,
            "Exporter l'expérience en PDF",
            QString("%1_%2.pdf").arg(suggestedName, QDate::currentDate().toString("yyyyMMdd")),
            "PDF Files (*.pdf)");

        if (path.isEmpty()) return;
        if (!path.endsWith(".pdf", Qt::CaseInsensitive)) {
            path += ".pdf";
        }

        ExperiencePdfInfo info;
        info.id = id;
        info.titre = cellText(0);
        info.hypothese = cellText(1);
        info.dateDebut = cellText(2);
        info.dateFin = cellText(3);
        info.statut = cellText(4);
        info.typeExperience = cellText(5);
        info.disponibilite = cellText(6);
        info.resultat = cellText(7);
        info.projet = rec.projetId.isNull() ? QString("-") : QString::number(rec.projetId.toInt());

        exportExperiencePdf(info, path);

        showToast(this, "PDF exporté : " + path, true);
    });

    // Export (démo)
    QObject::connect(exportE3, &QPushButton::clicked, this, [=](){
        QMessageBox::information(this, "Export", "Export statistiques Expériences (à connecter à PDF/Excel).");
    });

    // ==========================================================
    // NAVIGATION Équipements (4 widgets)
    // ==========================================================
    auto clearEquipForm = [=](){
        *keepLoadedDerivedDate = false;
        fcb1->setText("");
        fcb2->setText("");
        fcb3->setCurrentIndex(0);
        fcbType->setCurrentIndex(0);
        modelEdit->clear();
        date->setDate(QDate::currentDate());
        lastMaintDate->setDate(QDate::currentDate());
        intervalCb->setCurrentIndex(0); // 30 jours par défaut
        refreshDerivedEquipDate(false);
        calDate->setDate(QDate::currentDate());
        labRoom->clear();
        reloadEqExperienceChoices();
        const QString typeTxt = fcb1->text().trimmed();
        const QString fabTxt  = fcb2->text().trimmed();
        const QString locTxt  = labRoom->text().trimmed();
        eqTypeSummary->setText("  " + (typeTxt.isEmpty() ? QString("Nom d'équipement") : typeTxt));
        eqFabSummary->setText("  " + (fabTxt.isEmpty() ? QString("Fabricant") : fabTxt));
        eqSalleSummary->setText("  Salle : " + (locTxt.isEmpty() ? QString("Salle") : locTxt));
    };

    auto selectedEquipementId = [=]() -> int {
        const int r = eqTable->currentRow();
        if (r < 0 || !eqTable->item(r, 1)) return -1;
        return eqTable->item(r, 1)->data(Qt::UserRole).toInt();
    };

    auto eqValText = [](const QString& s) -> QString {
        return s.trimmed().isEmpty() ? QString("-") : s.trimmed();
    };

    auto eqDateText = [](const QDate& d) -> QString {
        return d.isValid() ? d.toString("dd/MM/yyyy") : QString("-");
    };

    auto equipStatusUiColor = [](const QString& status) -> QString {
        const QString s = status.toLower();
        if (s.contains("hors") || s.contains("service")) return QString("#8B2F3C");
        if (s.contains("maint") || s.contains("arch")) return QString("#7A8D92");
        return QString("#2E6F63");
    };

    auto updateEquipDetailsById = [=](int id) -> bool {
        if (id <= 0) {
            showToast(this, "Sélectionnez un équipement.", false);
            return false;
        }

        EquipementRecord rec;
        QString err;
        if (!eqCrud->fetchEquipement(id, rec, &err)) {
            showToast(this, "Erreur : " + err, false);
            return false;
        }

        equipTitle->setText(QString("<b>%1 - %2</b>")
                                .arg(eqValText(rec.nomEquipement), eqValText(rec.fabricant)));
        statusBadge->setText(eqValText(rec.statut));
        statusBadge->setStyleSheet(QString("QLabel{ background:%1; color:white; border-radius:16px; font-weight:900; padding:4px 12px; }")
                                   .arg(equipStatusUiColor(rec.statut)));

        if (eqDetFabricant)      eqDetFabricant->setText(eqValText(rec.fabricant));
        if (eqDetModele)         eqDetModele->setText(eqValText(rec.numeroModele));
        if (eqDetLocalisation)   eqDetLocalisation->setText(eqValText(rec.localisation));
        if (eqDetDateAchat)      eqDetDateAchat->setText(eqDateText(rec.dateAchat));
        if (eqDetDerniereMaint)  eqDetDerniereMaint->setText(eqDateText(rec.dateDerniereMaintenance));
        if (eqDetProchaineMaint) eqDetProchaineMaint->setText(eqDateText(rec.dateProchaineMaintenance));
        if (eqDetCalibration)    eqDetCalibration->setText(eqDateText(rec.dateLimiteCalibration));
        if (eqDetUtilisateur) {
            eqDetUtilisateur->setText(rec.idExp.isNull() ? QString("-")
                                                          : QString("EXP-") + QString::number(rec.idExp.toInt()));
        }
        return true;
    };

    auto updateEquipDetailsFromSelection = [=]() -> bool {
        return updateEquipDetailsById(selectedEquipementId());
    };

    QObject::connect(eqAdd, &QPushButton::clicked, this, [=](){
        *eqEditMode = false;
        *eqEditId = 0;
        clearEquipForm();
        setWindowTitle("Ajouter / Modifier un équipement");
        stack->setCurrentIndex(EQUIP_FORM);
    });

    QObject::connect(eqEdit, &QPushButton::clicked, this, [=](){
        const int id = selectedEquipementId();
        if (id <= 0) {
            showToast(this, "Sélectionnez un équipement.", false);
            return;
        }
        EquipementRecord rec;
        QString err;
        if (!eqCrud->fetchEquipement(id, rec, &err)) {
            showToast(this, "Erreur : " + err, false);
            return;
        }

        reloadEqExperienceChoices(rec.idExp.isNull() ? -1 : rec.idExp.toInt());

        *eqEditMode = true;
        *eqEditId = id;
        fcb1->setText(rec.nomEquipement);
        fcb2->setText(rec.fabricant);
        fcb3->setCurrentText(rec.statut);
        modelEdit->setText(rec.numeroModele);
        if (rec.dateAchat.isValid()) date->setDate(rec.dateAchat);
        if (rec.dateDerniereMaintenance.isValid()) lastMaintDate->setDate(rec.dateDerniereMaintenance);
        if (rec.dateProchaineMaintenance.isValid()) {
            const int intervalIdx = findIntervalIndexForDates(rec.dateDerniereMaintenance, rec.dateProchaineMaintenance);
            if (intervalIdx >= 0) {
                *keepLoadedDerivedDate = false;
                intervalCb->setCurrentIndex(intervalIdx);
                refreshDerivedEquipDate(false);
            } else {
                *keepLoadedDerivedDate = true;
                maintDate->setDate(rec.dateProchaineMaintenance);
                setDerivedDateStyle(rec.dateProchaineMaintenance, false);
            }
        } else {
            *keepLoadedDerivedDate = false;
            refreshDerivedEquipDate(false);
        }
        if (rec.dateLimiteCalibration.isValid()) calDate->setDate(rec.dateLimiteCalibration);
        if (!rec.localisation.trimmed().isEmpty()) labRoom->setText(rec.localisation);
        {
            const QString inferredType = equipmentTypeFromName(rec.nomEquipement, rec.numeroModele);
            int typeIdx = fcbType->findText(inferredType, Qt::MatchFixedString);
            if (typeIdx < 0) typeIdx = 0;
            fcbType->setCurrentIndex(typeIdx);
        }
        eqTypeSummary->setText("  " + (rec.nomEquipement.trimmed().isEmpty() ? QString("Nom d'équipement") : rec.nomEquipement.trimmed()));
        eqFabSummary->setText("  " + (rec.fabricant.trimmed().isEmpty() ? QString("Fabricant") : rec.fabricant.trimmed()));
        eqSalleSummary->setText("  Salle : " + (rec.localisation.trimmed().isEmpty() ? QString("Salle") : rec.localisation.trimmed()));

        setWindowTitle("Ajouter / Modifier un équipement");
        stack->setCurrentIndex(EQUIP_FORM);
    });

    QObject::connect(eqTable, &QTableWidget::cellDoubleClicked, this, [=](int, int){
        eqEdit->click();
    });

    QObject::connect(eqCancel, &QPushButton::clicked, this, [=](){
        clearEquipForm();
        setWindowTitle("Équipements");
        stack->setCurrentIndex(EQUIP_LIST);
    });

    QObject::connect(eqSave, &QPushButton::clicked, this, [=](){
        const QString equipType = fcb1->text().trimmed();
        const QString fabricant = fcb2->text().trimmed();
        const QRegularExpression eqInputStrict(QStringLiteral("^[\\p{L}\\p{N}\\s'\\-_.()/]+$"));

        if (equipType.isEmpty()) {
            showToast(this, "Le nom de l'équipement est obligatoire.", false);
            return;
        }

        if (!eqInputStrict.match(equipType).hasMatch()) {
            showToast(this, "Le nom d'équipement contient des caractères non autorisés.", false);
            return;
        }

        if (!fabricant.isEmpty() && !eqInputStrict.match(fabricant).hasMatch()) {
            showToast(this, "Le fabricant contient des caractères non autorisés.", false);
            return;
        }

        QListWidgetItem* selIt = eqExpCombo->currentItem();

        // Pas de blocage sur la date de maintenance — elle peut être dans le passé (retard réel)
        refreshDerivedEquipDate(false);

        EquipementRecord rec;
        rec.id = *eqEditId;
        rec.nomEquipement = equipType;
        rec.fabricant = fabricant;
        rec.numeroModele = modelEdit->text().trimmed();
        rec.dateAchat = date->date();
        rec.dateDerniereMaintenance = lastMaintDate->date();
        rec.dateProchaineMaintenance = maintDate->date(); // calculée automatiquement
        rec.statut = fcb3->currentText();
        rec.localisation = labRoom->text().trimmed();
        rec.dateLimiteCalibration = calDate->date();
        if (selIt && selIt->data(Qt::UserRole).isValid() && selIt->data(Qt::UserRole).toInt() > 0) {
            rec.idExp = QVariant(selIt->data(Qt::UserRole).toInt());
        } else {
            rec.idExp = QVariant();
        }

        QString err;
        const bool ok = *eqEditMode ? eqCrud->updateEquipement(rec, &err)
                                    : eqCrud->insertEquipement(rec, &err);
        if (!ok) {
            showToast(this, "Erreur : " + err, false);
            return;
        }

        showToast(this, *eqEditMode ? "Équipement modifié." : "Équipement ajouté.", true);
        *eqEditMode = false;
        *eqEditId = 0;
        clearEquipForm();
        loadEqTable();
        setWindowTitle("Équipements");
        stack->setCurrentIndex(EQUIP_LIST);
    });

    QObject::connect(eqDet, &QPushButton::clicked, this, [=](){
        if (!updateEquipDetailsFromSelection()) return;
        setWindowTitle("Détails équipement");
        stack->setCurrentIndex(EQUIP_DETAILS);
    });

    QObject::connect(eqExportPdf, &QPushButton::clicked, this, [=](){
        const int id = selectedEquipementId();
        if (id <= 0) {
            showToast(this, "Sélectionnez un équipement à exporter.", false);
            return;
        }

        EquipementRecord rec;
        QString err;
        if (!eqCrud->fetchEquipement(id, rec, &err)) {
            showToast(this, "Erreur export : " + err, false);
            return;
        }

        QString suggestedName = rec.nomEquipement.trimmed();
        if (suggestedName.isEmpty()) suggestedName = "equipement";
        suggestedName.replace("/", "_").replace("\\", "_").replace(":", "_")
                     .replace("*", "_").replace("?", "_").replace("\"", "_")
                     .replace("<", "_").replace(">", "_").replace("|", "_");

        QString path = QFileDialog::getSaveFileName(
            this,
            "Exporter l'équipement en PDF",
            QString("%1_%2.pdf").arg(suggestedName, QDate::currentDate().toString("yyyyMMdd")),
            "PDF Files (*.pdf)");

        if (path.isEmpty()) return;
        if (!path.endsWith(".pdf", Qt::CaseInsensitive)) {
            path += ".pdf";
        }

        exportEquipementPdf(rec, path);
        showToast(this, "PDF exporté : " + path, true);
    });

    QObject::connect(eqMore, &QPushButton::clicked, this, [=](){
        setWindowTitle("Statistiques des équipements");
        stack->setCurrentIndex(EQUIP_LOC);
    });
    QObject::connect(eqBack3, &QPushButton::clicked, this, [=](){
        setWindowTitle("Équipements");
        stack->setCurrentIndex(EQUIP_LIST);
    });
    QObject::connect(eqDetails3, &QPushButton::clicked, this, [=](){
        if (!updateEquipDetailsById(*eqStatsSelectedId)) return;
        setWindowTitle("Détails équipement");
        stack->setCurrentIndex(EQUIP_DETAILS);
    });
    QObject::connect(eqTree, &QTreeWidget::itemDoubleClicked, this, [=](QTreeWidgetItem* item, int){
        const int equipId = item ? item->data(0, Qt::UserRole).toInt() : -1;
        if (!updateEquipDetailsById(equipId)) return;
        setWindowTitle("Détails équipement");
        stack->setCurrentIndex(EQUIP_DETAILS);
    });
    QObject::connect(eqBack4, &QPushButton::clicked, this, [=](){
        setWindowTitle("Équipements");
        stack->setCurrentIndex(EQUIP_LIST);
    });
    QObject::connect(eqEditFromDetails, &QPushButton::clicked, this, [=](){
        const int id = selectedEquipementId();
        if (id > 0) {
            EquipementRecord rec;
            QString err;
            if (eqCrud->fetchEquipement(id, rec, &err)) {
                *eqEditMode = true;
                *eqEditId = id;
                fcb1->setText(rec.nomEquipement);
                fcb2->setText(rec.fabricant);
                fcb3->setCurrentText(rec.statut);
                fcbType->setCurrentIndex(0);
                modelEdit->setText(rec.numeroModele);
                if (rec.dateAchat.isValid()) date->setDate(rec.dateAchat);
                if (rec.dateProchaineMaintenance.isValid()) maintDate->setDate(rec.dateProchaineMaintenance);
                if (rec.dateLimiteCalibration.isValid()) calDate->setDate(rec.dateLimiteCalibration);
                if (!rec.localisation.trimmed().isEmpty()) labRoom->setText(rec.localisation);
                if (!rec.idExp.isNull()) {
                    for (int i = 0; i < eqExpCombo->count(); ++i) {
                        if (eqExpCombo->item(i)->data(Qt::UserRole).toInt() == rec.idExp.toInt()) {
                            eqExpCombo->setCurrentRow(i);
                            break;
                        }
                    }
                }
                eqTypeSummary->setText("  " + (rec.nomEquipement.trimmed().isEmpty() ? QString("Nom d'équipement") : rec.nomEquipement.trimmed()));
                eqFabSummary->setText("  " + (rec.fabricant.trimmed().isEmpty() ? QString("Fabricant") : rec.fabricant.trimmed()));
                eqSalleSummary->setText("  Salle : " + (rec.localisation.trimmed().isEmpty() ? QString("Salle") : rec.localisation.trimmed()));
            }
        }
        setWindowTitle("Ajouter / Modifier un équipement");
        stack->setCurrentIndex(EQUIP_FORM);
    });

    // ==========================================================
    // NAVIGATION Employés (5 widgets)
    // ==========================================================

    // ── Regex réutilisés pour la validation ───────────────────────
    static const QRegularExpression reCin("^[0-9]{8}$");
    static const QRegularExpression reAlpha("^[A-Za-z\xC0-\xFF\\- ]+$");
    static const QRegularExpression reEmail("^[A-Z0-9._%+\\-]+@[A-Z0-9.\\-]+\\.[A-Z]{2,}$",
                                            QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression reLower("[a-z]");
    static const QRegularExpression reUpper("[A-Z]");
    static const QRegularExpression reDigit("[0-9]");
    static const QRegularExpression reSpecial("[^A-Za-z0-9]");

    // ── Aide pour effacer tous les indicateurs d'erreur ──────────
    auto clearEmpErrors = [=](){
        setFieldErr(empCinEdit, false);  setFieldErr(empNomEdit, false);
        setFieldErr(empPrenomEdit, false); setFieldErr(empEmailEdit, false);
        setFieldErr(empPwdEdit, false);
        errCinNom->setVisible(false); errPrenomRole->setVisible(false);
        errEmailPwd->setVisible(false); errSpec->setVisible(false);
        errSpec->setStyleSheet("color: #B14A4A; font-size: 11px; padding: 0 4px;");
    };

    // ── Validation temps-réel : CIN ───────────────────────────────
    QObject::connect(empCinEdit, &QLineEdit::textChanged, this, [=](const QString& t){
        const QString v = t.trimmed();
        if (v.isEmpty()) { setFieldErr(empCinEdit, false); errCinNom->setVisible(false); return; }
        const bool ok = reCin.match(v).hasMatch();
        setFieldErr(empCinEdit, !ok);
        if (!ok) {
            errCinNom->setText(v.size() < 8 ? QString("CIN trop court (%1/8 chiffres)").arg(v.size())
                                             : "CIN invalide : 8 chiffres uniquement");
            errCinNom->setVisible(true);
        } else {
            if (!errCinNom->text().startsWith("Nom")) errCinNom->setVisible(false);
        }
    });

    // ── Validation temps-réel : Nom ───────────────────────────────
    QObject::connect(empNomEdit, &QLineEdit::textChanged, this, [=](const QString& t){
        const QString v = t.trimmed();
        if (v.isEmpty()) { setFieldErr(empNomEdit, false); return; }
        const bool ok = reAlpha.match(v).hasMatch();
        setFieldErr(empNomEdit, !ok);
        if (!ok && !errCinNom->isVisible()) {
            errCinNom->setText("Nom invalide : lettres uniquement (pas de chiffres)");
            errCinNom->setVisible(true);
        } else if (ok && errCinNom->text().startsWith("Nom")) {
            errCinNom->setVisible(false);
        }
    });

    // ── Validation temps-réel : Prénom ───────────────────────────
    QObject::connect(empPrenomEdit, &QLineEdit::textChanged, this, [=](const QString& t){
        const QString v = t.trimmed();
        if (v.isEmpty()) { setFieldErr(empPrenomEdit, false); errPrenomRole->setVisible(false); return; }
        const bool ok = reAlpha.match(v).hasMatch();
        setFieldErr(empPrenomEdit, !ok);
        if (!ok) {
            errPrenomRole->setText("Prénom invalide : lettres uniquement (pas de chiffres)");
            errPrenomRole->setVisible(true);
        } else {
            if (!errPrenomRole->text().startsWith("Sp")) errPrenomRole->setVisible(false);
        }
    });

    // ── Logique métier : Spécialisation requise si Chercheur ─────
    QObject::connect(empRoleCb, &QComboBox::currentTextChanged, this, [=](const QString& role){
        errSpec->setVisible(false);
        if (role.compare("Chercheur", Qt::CaseInsensitive) == 0) {
            errSpec->setText("Spécialisation requise pour un Chercheur — sélectionnez ci-dessus");
            errSpec->setStyleSheet("color: #0A5F58; font-size: 11px; padding: 0 4px;");
            errSpec->setVisible(true);
        }
    });

    auto clearEmpForm = [=](){
        *empEditMode = false;
        *empEditId = 0;
        empCinEdit->clear();
        empNomEdit->clear();
        empPrenomEdit->clear();
        empEmailEdit->clear();
        empPwdEdit->clear();
        empRoleCb->setCurrentIndex(0);
        empSpecCb->setCurrentIndex(0);
        empQualifEdit->clear();
        empPubs->setValue(0);
        empFtCb->setCurrentIndex(0);
        empLabCb->setCurrentIndex(0);
        empProjCb->setCurrentIndex(0);
        clearEmpErrors();
    };

    QObject::connect(empAdd, &QPushButton::clicked, this, [=](){
        clearEmpForm();
        setWindowTitle("Creer / Modifier Employe");
        stack->setCurrentIndex(EMP_FORM);
    });
    QObject::connect(empEdit, &QPushButton::clicked, this, [=](){
        const int row = empTable->currentRow();
        if (row < 0 || !empTable->item(row, 1)) {
            QMessageBox::information(this, "Employe", "Selectionnez un employe dans la liste.");
            return;
        }

        const int id = empTable->item(row, 1)->data(Qt::UserRole).toInt();
        EmployeRecord rec;
        QString err;
        if (!empCrud->fetchEmploye(id, rec, &err)) {
            showToast(this, "Erreur : " + err, false);
            return;
        }

        *empEditMode = true;
        *empEditId = id;

        empCinEdit->setText(rec.cin);
        empNomEdit->setText(rec.nom);
        empPrenomEdit->setText(rec.prenom);
        empEmailEdit->setText(rec.email);
        empPwdEdit->clear(); // password not shown on edit
        if (!rec.role.trimmed().isEmpty()) empRoleCb->setCurrentText(rec.role);
        if (!rec.specialization.trimmed().isEmpty()) empSpecCb->setCurrentText(rec.specialization);

        if (empTable->item(row, 6)) empQualifEdit->setText(empTable->item(row, 6)->text() == "-" ? QString() : empTable->item(row, 6)->text());
        if (empTable->item(row, 7)) empPubs->setValue(empTable->item(row, 7)->text().toInt());
        if (empTable->item(row, 8)) empFtCb->setCurrentText(empStatusText(static_cast<FTStatus>(empTable->item(row, 8)->data(Qt::UserRole).toInt())));
        if (empTable->item(row, 9)) empLabCb->setCurrentText(empTable->item(row, 9)->text());

        setWindowTitle("Creer / Modifier Employe");
        stack->setCurrentIndex(EMP_FORM);
    });
    QObject::connect(empCancel, &QPushButton::clicked, this, [=](){
        clearEmpForm();
        setWindowTitle("Employés");
        stack->setCurrentIndex(EMP_LIST);
    });
    QObject::connect(empSave, &QPushButton::clicked, this, [=](){
        clearEmpErrors();
        bool hasError = false;

        // ── 1. CIN : obligatoire, exactement 8 chiffres ──────────────
        const QString cin = empCinEdit->text().trimmed();
        if (cin.isEmpty()) {
            errCinNom->setText("CIN obligatoire");
            errCinNom->setVisible(true); setFieldErr(empCinEdit, true); hasError = true;
        } else if (!reCin.match(cin).hasMatch()) {
            errCinNom->setText(cin.size() < 8
                ? QString("CIN trop court (%1/8 chiffres)").arg(cin.size())
                : "CIN invalide : exactement 8 chiffres, pas de lettres");
            errCinNom->setVisible(true); setFieldErr(empCinEdit, true); hasError = true;
        }

        // ── 2. Nom : obligatoire, lettres uniquement ──────────────────
        const QString nom = empNomEdit->text().trimmed();
        if (nom.isEmpty()) {
            if (!errCinNom->isVisible()) { errCinNom->setText("Nom obligatoire"); errCinNom->setVisible(true); }
            setFieldErr(empNomEdit, true); hasError = true;
        } else if (!reAlpha.match(nom).hasMatch()) {
            if (!errCinNom->isVisible()) { errCinNom->setText("Nom invalide : lettres uniquement (pas de chiffres)"); errCinNom->setVisible(true); }
            setFieldErr(empNomEdit, true); hasError = true;
        }

        // ── 3. Prénom : obligatoire, lettres uniquement ───────────────
        const QString prenom = empPrenomEdit->text().trimmed();
        if (prenom.isEmpty()) {
            errPrenomRole->setText("Prénom obligatoire");
            errPrenomRole->setVisible(true); setFieldErr(empPrenomEdit, true); hasError = true;
        } else if (!reAlpha.match(prenom).hasMatch()) {
            errPrenomRole->setText("Prénom invalide : lettres uniquement (pas de chiffres)");
            errPrenomRole->setVisible(true); setFieldErr(empPrenomEdit, true); hasError = true;
        }

        // ── 4. Email : obligatoire + format valide ────────────────────
        const QString email = empEmailEdit->text().trimmed();
        if (email.isEmpty()) {
            errEmailPwd->setText("Email obligatoire");
            errEmailPwd->setVisible(true); setFieldErr(empEmailEdit, true); hasError = true;
        } else if (!reEmail.match(email).hasMatch()) {
            errEmailPwd->setText("Email invalide — exemple correct : nom@labo.org");
            errEmailPwd->setVisible(true); setFieldErr(empEmailEdit, true); hasError = true;
        }

        // ── 5. Mot de passe : requis en création, fort ────────────────
        if (!*empEditMode) {
            const QString pwd = empPwdEdit->text().trimmed();
            const bool pwdOk = pwd.size() >= 8
                && reLower.match(pwd).hasMatch()
                && reUpper.match(pwd).hasMatch()
                && reDigit.match(pwd).hasMatch()
                && reSpecial.match(pwd).hasMatch();
            if (!pwdOk) {
                if (!errEmailPwd->isVisible())
                    errEmailPwd->setText("Mot de passe faible : min 8 car., 1 maj, 1 min, 1 chiffre, 1 spécial");
                errEmailPwd->setVisible(true); setFieldErr(empPwdEdit, true); hasError = true;
            }
        }

        // ── 6. Logique métier : publications > 0 ⟹ Chercheur ──────────
        const QString role = empRoleCb->currentText();
        if (empPubs->value() > 0 && role.compare("Chercheur", Qt::CaseInsensitive) != 0) {
            errSpec->setText(QString("Note : %1 publication(s) enregistrée(s) mais rôle = %2. Un Chercheur est attendu.")
                             .arg(empPubs->value()).arg(role));
            errSpec->setStyleSheet("color: #B5672C; font-size: 11px; padding: 0 4px;");
            errSpec->setVisible(true);
            // avertissement seulement, pas un blocage
        }

        if (hasError) return;

        EmployeRecord rec;
        rec.employeeId     = *empEditMode ? *empEditId : 0;
        rec.cin            = cin;
        rec.nom            = nom;
        rec.prenom         = prenom;
        rec.email          = email;
        rec.password       = empPwdEdit->text().trimmed();
        rec.role           = role;
        rec.specialization = empSpecCb->currentText();
        rec.qualification  = empQualifEdit->text().trimmed();
        rec.nbPublications = empPubs->value();
        rec.tempsTravail   = empFtCb->currentText();
        rec.laboratoire    = empLabCb->currentText();

        QString err;
        const bool ok = *empEditMode ? empCrud->updateEmploye(rec, &err)
                                     : empCrud->insertEmploye(rec, &err);
        if (!ok) {
            // Mapper l'erreur DB vers le bon champ visuel
            if (err.contains("CIN", Qt::CaseInsensitive) || err.contains("existe", Qt::CaseInsensitive)) {
                errCinNom->setText(err); errCinNom->setVisible(true); setFieldErr(empCinEdit, true);
            } else if (err.contains("EMAIL", Qt::CaseInsensitive)) {
                errEmailPwd->setText(err); errEmailPwd->setVisible(true); setFieldErr(empEmailEdit, true);
            } else if (err.contains("passe", Qt::CaseInsensitive) || err.contains("password", Qt::CaseInsensitive)) {
                errEmailPwd->setText(err); errEmailPwd->setVisible(true); setFieldErr(empPwdEdit, true);
            } else {
                showToast(this, "Erreur : " + err, false);
            }
            return;
        }

        clearEmpForm();
        loadEmpTable();
        applyEmpFilters();
        updateEmpStatsFromTable();

        setWindowTitle("Employés");
        stack->setCurrentIndex(EMP_LIST);
        showToast(this, *empEditMode ? "Employe modifie avec succes." : "Employe cree avec succes.", true);
    });
    QObject::connect(empMore, &QPushButton::clicked, this, [=](){
        affLoadProjects();
        setWindowTitle("Affectation Intelligente");
        stack->setCurrentIndex(EMP_AFF);
    });
    QObject::connect(empBack3, &QPushButton::clicked, this, [=](){
        setWindowTitle("Employés");
        stack->setCurrentIndex(EMP_LIST);
    });
    QObject::connect(empBack4, &QPushButton::clicked, this, [=](){
        setWindowTitle("Employés");
        stack->setCurrentIndex(EMP_LIST);
    });
    QObject::connect(empStats, &QPushButton::clicked, this, [=](){
        setWindowTitle("Statistiques Employes");
        stack->setCurrentIndex(EMP_STATS);
        updateEmpStatsFromTable();
    });
    QObject::connect(empBtnSec, &QPushButton::clicked, this, [=](){
        setWindowTitle("Statistiques Employes");
        stack->setCurrentIndex(EMP_STATS);
        updateEmpStatsFromTable();
    });
    QObject::connect(empBackStats, &QPushButton::clicked, this, [=](){
        setWindowTitle("Employés");
        stack->setCurrentIndex(EMP_LIST);
    });
    QObject::connect(empDel, &QPushButton::clicked, this, [=](){
        if (*currentRole != "Responsable" && *currentRole != "RH") { showToast(this, "Accès refusé.", false); return; }
        int r = empTable->currentRow();
        if (r < 0) {
            QMessageBox::information(this, "Suppression", "Selectionnez un employe dans la liste.");
            return;
        }
        if (!empTable->item(r,1) || !empTable->item(r,2) || !empTable->item(r,4)) {
            showToast(this, "Ligne employe invalide.", false);
            return;
        }
        const int id = empTable->item(r,1) ? empTable->item(r,1)->data(Qt::UserRole).toInt() : 0;
        if (id <= 0) {
            showToast(this, "ID employe invalide.", false);
            return;
        }
           QString resume = QString("CIN : %1 | Nom : %2 | Role : %3")
                            .arg(empTable->item(r,1)->text(),
                                empTable->item(r,2)->text(),
                                empTable->item(r,4)->text());
        ConfirmDeleteDialog confirm(style(), resume, this);
        if (confirm.exec() == QDialog::Accepted) {
            QString err;
            if (!empCrud->deleteEmploye(id, &err)) {
                showToast(this, "Erreur : " + err, false);
                return;
            }
            loadEmpTable();
            applyEmpFilters();
            updateEmpStatsFromTable();
            showToast(this, "Employe supprime.", true);
        }
    });
    QObject::connect(empPdfBtn, &QPushButton::clicked, this, [=](){
        const int r = empTable->currentRow();
        if (r < 0 || !empTable->item(r, 1)) {
            showToast(this, "Sélectionnez un employé d'abord.", false);
            return;
        }
        const int id = empTable->item(r, 1)->data(Qt::UserRole).toInt();
        EmployeRecord rec;
        QString err;
        if (!empCrud->fetchEmploye(id, rec, &err)) {
            showToast(this, "Erreur : " + err, false);
            return;
        }
        const QString path = QFileDialog::getSaveFileName(
            this, "Exporter PDF Employé",
            QString("Employe_%1_%2.pdf").arg(rec.nom, rec.prenom),
            "PDF (*.pdf)");
        if (path.isEmpty()) return;
        exportEmployePdf(rec, path);
        showToast(this, "PDF exporté.", true);
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    });
    // ===================== NAVIGATION PUBLICATION =====================

    // LIST -> FORM
    QObject::connect(pubAdd, &QPushButton::clicked, this, [=](){
        clearPublicationForm();
        stack->setCurrentIndex(PUB_FORM);
    });
    QObject::connect(pubEdit, &QPushButton::clicked, this, [=](){
        if (!fillPublicationFormFromSelection()) return;
        stack->setCurrentIndex(PUB_FORM);
    });

    // LIST -> STATS
    QObject::connect(pubStats, &QPushButton::clicked, this, [=](){
        updatePublicationStats();
        stack->setCurrentIndex(PUB_STATS);
    });

    QObject::connect(pubDetails, &QPushButton::clicked, this, [=](){
        if (!updatePubDetailsFromRow()) return;
        stack->setCurrentIndex(PUB_DETAILS);
    });

    // FORM -> LIST
    QObject::connect(pubCancel, &QPushButton::clicked, this, [=](){
        clearPublicationForm();
        stack->setCurrentIndex(PUB_LIST);
    });
    QObject::connect(pubSave, &QPushButton::clicked, this, [=](){
        if (leTitle->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "Validation", "Le titre est obligatoire.");
            return;
        }
        if (cbStatus->currentText() == "Statut") {
            QMessageBox::warning(this, "Validation", "Veuillez sélectionner un statut.");
            return;
        }
        if (cbEmployee->currentIndex() <= 0 || cbEmployee->currentData().toInt() <= 0) {
            QMessageBox::warning(this, "Validation", "Veuillez sélectionner un employé.");
            return;
        }

        QString journal = leJournal->text().trimmed();
        if (!journal.isEmpty()) {
            bool isOnlyNumbers = true;
            for (QChar ch : journal) {
                if (!ch.isDigit()) {
                    isOnlyNumbers = false;
                    break;
                }
            }
            if (isOnlyNumbers) {
                QMessageBox::warning(this, "Validation", "Le journal ne peut pas être composé uniquement de chiffres.");
                return;
            }
        }

        QString doi = leDOI->text().trimmed();
        if (!doi.isEmpty()) {
            QRegularExpression doiRegex("^[0-9]+$");
            if (!doiRegex.match(doi).hasMatch()) {
                QMessageBox::warning(this, "Validation", "Le DOI ne peut contenir que des chiffres.");
                return;
            }
        }

        Publication publication(
            sbPubId->value(),
            leTitle->text().trimmed(),
            leJournal->text().trimmed(),
            sbYear->value(),
            leDOI->text().trimmed(),
            cbStatus->currentText(),
            teAbstract->toPlainText().trimmed(),
            0,
            [&](){
                const int currentIndex = cbEmployee->currentIndex();
                if (currentIndex > 0) {
                    const int currentId = cbEmployee->currentData().toInt();
                    if (currentId > 0) {
                        return currentId;
                    }
                }
                const int matchedIndex = cbEmployee->findText(cbEmployee->currentText().trimmed(), Qt::MatchFixedString);
                return (matchedIndex >= 0) ? cbEmployee->itemData(matchedIndex).toInt() : 0;
            }(),
            sbImpactFactor->value(),
            leCitations->toPlainText().trimmed()
            );

        QString errorMessage;
        const bool ok = sbPubId->isReadOnly()
                            ? publication.update(&errorMessage)
                            : publication.create(&errorMessage);

        if (!ok) {
            QMessageBox::warning(this, "Publication", "Échec d'enregistrement :\n" + errorMessage);
            return;
        }

        reloadPublications();
        applyPublicationFilters();
        clearPublicationForm();
        stack->setCurrentIndex(PUB_LIST);
    });

    QObject::connect(pub3Back, &QPushButton::clicked, this, [=](){
        stack->setCurrentIndex(PUB_LIST);
    });

    QObject::connect(pubDetailsBack, &QPushButton::clicked, this, [=](){
        stack->setCurrentIndex(PUB_LIST);
    });

    QObject::connect(pubDetailsExport, &QPushButton::clicked, this, [=](){
        exportSelectedPublicationPdf();
    });

    // STATS -> LIST




    setWindowTitle("SmartVision - Connexion");
    stack->setCurrentIndex(LOGIN);

    // ── Bouton chatbot flottant (visible après login) ───────────────
    FloatingChatBtn* floatChat = new FloatingChatBtn(root);
    floatChat->raise();
    // Re-raise après chaque changement de page pour rester au-dessus
    QObject::connect(stack, &QStackedWidget::currentChanged, floatChat, [=]() {
        floatChat->raise();
    });

    // ── Commandes vocales flottantes (cachées sur LOGIN) ─────────────
    VoiceCommand* voiceCmd = new VoiceCommand(nullptr);
    g_voiceCmd = voiceCmd;
    voiceCmd->positionBottomRight();
    voiceCmd->setCurrentRole(QString());
    voiceCmd->hide(); // hidden until user activates via 🎙 button

    floatChat->setVisible(false);
    QObject::connect(stack, &QStackedWidget::currentChanged, this, [=](int idx){
        const bool onLogin = (idx == LOGIN);
        floatChat->setVisible(!onLogin);
        if (!onLogin)
            floatChat->raise();
        // Sync bVoice checked state with voiceCmd visibility
        if (onLogin && g_voiceCmd) {
            g_voiceCmd->hide();
            g_voiceCmd->setCurrentRole(QString());
            if (g_globalBar && g_globalBar->bVoice)
                g_globalBar->bVoice->setChecked(false);
        }
        // Update voice command context
        QString pageName;
        if      (idx == BIO_LIST  || (idx > BIO_LIST  && idx < PROJ_LIST))  pageName = "biosample";
        else if (idx == PROJ_LIST || (idx > PROJ_LIST && idx < EXP_LIST))   pageName = "projet";
        else if (idx == EXP_LIST  || (idx > EXP_LIST  && idx < PUB_LIST))   pageName = "experience";
        else if (idx == PUB_LIST  || (idx > PUB_LIST  && idx < EQUIP_LIST)) pageName = "publication";
        else if (idx == EQUIP_LIST|| (idx > EQUIP_LIST&& idx < EMP_LIST))   pageName = "equipement";
        else if (idx >= EMP_LIST)                                             pageName = "employee";
        voiceCmd->setCurrentContext(idx, pageName);

        ModuleTab activeTab = ModuleTab::Employee;
        if (moduleTabForPageIndex(idx, &activeTab)) syncModuleSelection(activeTab);
        else clearAllModuleSelections();
    });

    // ── Auto-fill form fields extracted from voice command ────────
    // Matches field key (e.g. "reference") to a QLineEdit placeholder or objectName
    auto autoFillForm = [=](QWidget* root, const QVariantMap& fields) {
        if (!root || fields.isEmpty()) return;
        for (auto it = fields.begin(); it != fields.end(); ++it) {
            const QString key = it.key().toLower();
            const QString val = it.value().toString();
            if (val.isEmpty()) continue;
            // Try QLineEdit first
            bool filled = false;
            for (QLineEdit* le : root->findChildren<QLineEdit*>()) {
                QString hint = le->placeholderText().toLower().remove(' ');
                QString name = le->objectName().toLower();
                if (hint.contains(key) || name.contains(key) ||
                    key.contains(hint.left(4))) {
                    le->setText(val);
                    filled = true;
                    break;
                }
            }
            if (filled) continue;
            // Try QComboBox (editable)
            for (QComboBox* cb : root->findChildren<QComboBox*>()) {
                QString hint = cb->placeholderText().toLower().remove(' ');
                QString name = cb->objectName().toLower();
                if (hint.contains(key) || name.contains(key)) {
                    if (cb->isEditable()) cb->setCurrentText(val);
                    else {
                        int i = cb->findText(val, Qt::MatchContains | Qt::MatchCaseSensitive);
                        if (i < 0) i = cb->findText(val, Qt::MatchContains);
                        if (i >= 0) cb->setCurrentIndex(i);
                    }
                    break;
                }
            }
        }
    };

    QObject::connect(voiceCmd, &VoiceCommand::commandExecute, this,
    [=](const QString& action, const QString& module, const QVariantMap& params){
        const QVariantMap fields   = params.value("fields").toMap();
        const QString     srchText = params.value("text").toString();

        if (!module.isEmpty() &&
            action != "chatbot" &&
            action != "logout" &&
            !isModuleAllowedForRole(*currentRole, module)) {
            showToast(this, "Accès refusé : ce module n'est pas autorisé pour votre rôle.", false);
            return;
        }

        // 1) Navigate to module first (for all actions that target a module)
        if (!module.isEmpty() && action != "chatbot" && action != "logout") {
            if      (module == "biosample")   stack->setCurrentIndex(BIO_LIST);
            else if (module == "employee")    stack->setCurrentIndex(EMP_LIST);
            else if (module == "equipement")  stack->setCurrentIndex(EQUIP_LIST);
            else if (module == "experience")  stack->setCurrentIndex(EXP_LIST);
            else if (module == "projet")      stack->setCurrentIndex(PROJ_LIST);
            else if (module == "publication") stack->setCurrentIndex(PUB_LIST);
            else if (module == "congelateur") {
                CongelateurDialog* dlg = new CongelateurDialog(this);
                dlg->setAttribute(Qt::WA_DeleteOnClose);
                dlg->show(); dlg->raise();
                return;
            }
        }

        // 2) Perform action
        if (action == "navigate") {
            // Already navigated above
        }
        else if (action == "add") {
            QPushButton* addBtn = nullptr;
            if      (module == "biosample")   addBtn = btnAdd;
            else if (module == "equipement")  addBtn = eqAdd;
            else if (module == "experience")  addBtn = expAdd;
            else if (module == "projet")      addBtn = projAdd;
            else if (module == "publication") addBtn = pubAdd;
            else if (module == "employee")    addBtn = empAdd;
            if (addBtn) {
                addBtn->click();
                // Auto-fill form after it opens
                if (!fields.isEmpty()) {
                    QTimer::singleShot(400, this, [=](){
                        autoFillForm(stack->currentWidget(), fields);
                    });
                }
            }
        }
        else if (action == "edit") {
            QPushButton* editBtn = nullptr;
            if      (module == "biosample")   editBtn = btnEdit;
            else if (module == "equipement")  editBtn = eqEdit;
            else if (module == "experience")  editBtn = expEdit;
            else if (module == "projet")      editBtn = projEdit;
            else if (module == "publication") editBtn = pubEdit;
            else if (module == "employee")    editBtn = empEdit;
            if (editBtn) {
                editBtn->click();
                if (!fields.isEmpty()) {
                    QTimer::singleShot(400, this, [=](){
                        autoFillForm(stack->currentWidget(), fields);
                    });
                }
            }
        }
        else if (action == "delete") {
            if      (module == "biosample")   btnDel->click();
            else if (module == "equipement")  eqDel->click();
            else if (module == "experience")  expDel->click();
            else if (module == "projet")      projDel->click();
            else if (module == "publication") pubDel->click();
            else if (module == "employee")    empDel->click();
        }
        else if (action == "details") {
            if      (module == "experience")  expDetails->click();
            else if (module == "equipement")  eqDet->click();
            else if (module == "projet")      projDetails->click();
            else if (module == "publication") pubDetails->click();
        }
        else if (action == "stats") {
            if      (module == "biosample")   btnStats->click();
            else if (module == "experience")  expStats->click();
            else if (module == "publication") pubStats->click();
            else if (module == "employee")    empStats->click();
        }
        else if (action == "export_pdf") {
            if (module == "biosample") pdfBtn->click();
        }
        else if (action == "search") {
            // Set search text if provided, then focus the search field
            if (module == "employee" || module.isEmpty()) {
                if (!srchText.isEmpty()) empSearch->setText(srchText);
                empSearch->setFocus(); empSearch->selectAll();
            }
        }
        else if (action == "refresh") {
            // Re-trigger currentChanged to reload: toggle to adjacent non-login page then back
            const int cur = stack->currentIndex();
            if (cur > 0) {
                const int tmp = (cur > 1) ? cur - 1 : cur + 1;
                stack->setCurrentIndex(tmp);
                QTimer::singleShot(50, this, [=](){ stack->setCurrentIndex(cur); });
            }
        }
        else if (action == "save") {
            // Find and click the Save/OK button in the active top-level widget
            QWidget* active = QApplication::activeWindow();
            if (!active) active = this;
            for (QPushButton* btn : active->findChildren<QPushButton*>()) {
                const QString t = btn->text().toLower();
                if (t.contains("enregistr") || t.contains("sauvegarder") ||
                    t.contains("valider") || t == "ok" || t.contains("terminer")) {
                    btn->click(); break;
                }
            }
        }
        else if (action == "cancel") {
            QWidget* active = QApplication::activeWindow();
            if (!active) active = this;
            for (QPushButton* btn : active->findChildren<QPushButton*>()) {
                const QString t = btn->text().toLower();
                if (t.contains("annuler") || t.contains("fermer") ||
                    t.contains("retour") || t == "cancel") {
                    btn->click(); break;
                }
            }
        }
        else if (action == "congelateur" || action == "ia_congelateur") {
            CongelateurDialog* dlg = new CongelateurDialog(this);
            dlg->setAttribute(Qt::WA_DeleteOnClose);
            dlg->show(); dlg->raise();
        }
        else if (action == "chatbot") {
            ChatBotBioSimple* bot = new ChatBotBioSimple(this);
            QPoint center = geometry().center();
            bot->move(center.x() - bot->width() / 2,
                      center.y() - bot->height() / 2);
            bot->exec();
        }
        else if (action == "logout") {
            logoutCurrentUser();
            stack->setCurrentIndex(0);
        }
    });

    // ══════════════════════════════════════════════════════════════
    // SYSTÈME DE RÔLES ET PERMISSIONS
    // Défini ici car tous les boutons sont maintenant en scope.
    // ══════════════════════════════════════════════════════════════
    *applyPerms = [=]() {
        const QString& role = *currentRole;

        const bool isResp  = (role == "Responsable");
        const bool isCherc = (role == "Chercheur");
        const bool isTech  = (role == "Technicien");
        const bool isRH    = (role == "RH");

        // ── Helper : show/hide un bouton précis ──
        auto show = [](QPushButton* b, bool v){
            if (b) { b->setVisible(v); b->setEnabled(v); }
        };

        // ── Onglets — visibilité dans TOUTES les barres ────────────
        for (const ModulesBar& moduleBar : registeredModuleBars()) {
            applyRoleToModulesBar(moduleBar, role);
        }

        // ── Boutons CRUD — BioSample ───────────────────────────────
        show(btnAdd,  isResp || isTech);
        show(btnEdit, isResp || isTech);
        show(btnDel,  isResp);

        // ── Boutons CRUD — Expériences ─────────────────────────────
        show(expAdd, isResp || isCherc);
        show(expDel, isResp);

        // ── Boutons CRUD — Publications ────────────────────────────
        show(pubAdd, isResp || isCherc);
        show(pubDel, isResp);

        // ── Boutons CRUD — Projet ──────────────────────────────────
        show(projAdd, isResp);
        show(projDel, isResp);

        // ── Boutons CRUD — Équipements ─────────────────────────────
        show(eqAdd,  isResp || isTech);
        show(eqEdit, isResp || isTech);
        show(eqDel,  isResp);

        // ── Boutons CRUD — Employés ────────────────────────────────
        show(empAdd,  isResp || isRH);
        show(empEdit, isResp || isRH);
        show(empDel,  isResp || isRH);
    };
    // Appliquer une première fois avec le rôle par défaut (Responsable)
    if (*applyPerms) (*applyPerms)();

}
