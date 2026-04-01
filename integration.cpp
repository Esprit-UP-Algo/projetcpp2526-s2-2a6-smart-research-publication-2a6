// ===================== biosimple.cpp (UN SEUL FICHIER - BioSimple + Gestion Projet - 3 Widgets) =====================
#include "integration.h"
#include "crudebiosimple.h"
#include "publication.h"
#include "chatbotbiosimple.h"
#include "crudexperience.h"
#include "simple.h"
#include "crudEquipement.h"
#include "employes.h"
#include "gestproj.h"
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
#include <QMainWindow>
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
#include <QStackedWidget>
#include <QDateEdit>
#include <QSpinBox>
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
#include <QFileInfo>

#include <QPrinter>
#include <QTextDocument>
#include <QRegularExpressionValidator>
#include <QFileDialog>
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
            else if (roleNorm == "responsable") roleMatch = 80;
            else roleMatch = 65;
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

static bool g_darkThemeEnabled = true;
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
            px.load("smartvision.png");
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
    QPushButton* bLogout = nullptr;
};

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
    const ModulesBar& globalBar = *g_globalBar;

    // Helper: uncheck all buttons except the clicked one (in both local and global bars)
    auto uncheckOthers = [=](QPushButton* btnClicked, ModuleTab selectedTab){
        if (!btnClicked) return;
        // Update global bar
        if(globalBar.bEmployee && globalBar.bEmployee != btnClicked) globalBar.bEmployee->setChecked(false);
        if(globalBar.bPublication && globalBar.bPublication != btnClicked) globalBar.bPublication->setChecked(false);
        if(globalBar.bBioSimple && globalBar.bBioSimple != btnClicked) globalBar.bBioSimple->setChecked(false);
        if(globalBar.bEquipement && globalBar.bEquipement != btnClicked) globalBar.bEquipement->setChecked(false);
        if(globalBar.bExp && globalBar.bExp != btnClicked) globalBar.bExp->setChecked(false);
        if(globalBar.bProjet && globalBar.bProjet != btnClicked) globalBar.bProjet->setChecked(false);
        btnClicked->setChecked(true);

        // Update local bar
        if(mb.bEmployee && mb.bEmployee != btnClicked) mb.bEmployee->setChecked(false);
        if(mb.bPublication && mb.bPublication != btnClicked) mb.bPublication->setChecked(false);
        if(mb.bBioSimple && mb.bBioSimple != btnClicked) mb.bBioSimple->setChecked(false);
        if(mb.bEquipement && mb.bEquipement != btnClicked) mb.bEquipement->setChecked(false);
        if(mb.bExp && mb.bExp != btnClicked) mb.bExp->setChecked(false);
        if(mb.bProjet && mb.bProjet != btnClicked) mb.bProjet->setChecked(false);

        // Find and check the corresponding button in local bar
        if(selectedTab == ModuleTab::BioSimple && mb.bBioSimple) mb.bBioSimple->setChecked(true);
        else if(selectedTab == ModuleTab::Equipement && mb.bEquipement) mb.bEquipement->setChecked(true);
        else if(selectedTab == ModuleTab::ExperiencesProtocoles && mb.bExp) mb.bExp->setChecked(true);
        else if(selectedTab == ModuleTab::GestionProjet && mb.bProjet) mb.bProjet->setChecked(true);
        else if(selectedTab == ModuleTab::Employee && mb.bEmployee) mb.bEmployee->setChecked(true);
        else if(selectedTab == ModuleTab::Publication && mb.bPublication) mb.bPublication->setChecked(true);
    };

    // Modules activés
    QObject::connect(mb.bBioSimple, &QPushButton::clicked, self, [=](){
        uncheckOthers(globalBar.bBioSimple, ModuleTab::BioSimple);
        self->setWindowTitle("Gestion des Échantillons");
        stack->setCurrentIndex(BIO_LIST);
    });

    QObject::connect(mb.bProjet, &QPushButton::clicked, self, [=](){
        uncheckOthers(globalBar.bProjet, ModuleTab::GestionProjet);
        self->setWindowTitle("Projet");
        stack->setCurrentIndex(PROJ_LIST);
    });

    QObject::connect(mb.bExp, &QPushButton::clicked, self, [=](){
        uncheckOthers(globalBar.bExp, ModuleTab::ExperiencesProtocoles);
        self->setWindowTitle("Expériences");
        stack->setCurrentIndex(EXP_LIST);
    });

    QObject::connect(mb.bEmployee, &QPushButton::clicked, self, [=](){
        uncheckOthers(globalBar.bEmployee, ModuleTab::Employee);
        self->setWindowTitle("Gestion des Employés");
        if (EMP_LIST >= 0 && EMP_LIST < stack->count()) {
            stack->setCurrentIndex(EMP_LIST);
        }
    });

    QObject::connect(mb.bPublication, &QPushButton::clicked, self, [=](){
        uncheckOthers(globalBar.bPublication, ModuleTab::Publication);
        self->setWindowTitle("Gestion des Publications");
        stack->setCurrentIndex(PUB_LIST);
    });

    QObject::connect(mb.bEquipement, &QPushButton::clicked, self, [=](){
        uncheckOthers(globalBar.bEquipement, ModuleTab::Equipement);
        self->setWindowTitle("Gestion des Équipements");
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
enum class ProjStatus { EnCours=0, EnRetard=1, Critique=2, Suspendu=3, Termine=4, Planifie=5 };

static QString projStatusText(ProjStatus s)
{
    switch (s) {
    case ProjStatus::EnCours:  return "En cours";
    case ProjStatus::EnRetard: return "En retard";
    case ProjStatus::Critique: return "Critique";
    case ProjStatus::Suspendu: return "Suspendu";
    case ProjStatus::Termine:  return "Terminé";
    case ProjStatus::Planifie: return "Planifié";
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

static QFrame* eqRoomCapacityBlock(QStyle* st, const QString& room, const QString& capacity)
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

    v->addWidget(line(QStyle::SP_DirIcon,  QString("Salle : %1").arg(room)));
    v->addWidget(line(QStyle::SP_ArrowUp, QString("Capacité : %1").arg(capacity)));
    return box;
}

// Bottom bar showing a location string and quick actions.
static QFrame* w3BottomLocationBar(QStyle* st, const QString& text)
{
    QFrame* bar = new QFrame;
    bar->setStyleSheet(QString("QFrame{ background: rgba(255,255,255,0.70); border:1px solid %1; border-radius: 12px; }").arg(C_PANEL_BR));
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

// ===================== STATISTIQUES (Graphiques) =====================
// Simple donut chart widget for stats dashboards.
class DonutChart : public QWidget {
public:
    struct Slice { double value; QColor color; QString label; };
    explicit DonutChart(QWidget* parent=nullptr) : QWidget(parent) { setMinimumHeight(180); }
    void setData(const QList<Slice>& s) { m_slices = s; update(); }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        QRect r = rect().adjusted(10,10,-10,-10);
        int d = qMin(r.width(), r.height());
        QRect pie(r.left(), r.top(), d, d);

        double total = 0;
        for (auto &s : m_slices) total += s.value;
        if (total <= 0) return;

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255,255,255,60));
        p.drawEllipse(pie);

        int thickness = (int)(d * 0.28);
        QRect inner = pie.adjusted(thickness, thickness, -thickness, -thickness);

        double start = 90.0 * 16;
        for (auto &s : m_slices) {
            double span = - (s.value / total) * 360.0 * 16;
            p.setBrush(s.color);
            p.drawPie(pie, (int)start, (int)span);

            double midDeg = (start + span/2.0) / 16.0;
            double rad = qDegreesToRadians(midDeg);
            QPointF c = pie.center();
            double rr = d * 0.38;
            QPointF pos(c.x() + rr * std::cos(rad), c.y() - rr * std::sin(rad));

            int pct = (int)std::round((s.value / total) * 100.0);
            p.setPen(QColor(255,255,255,230));
            QFont f = font(); f.setBold(true); f.setPointSize(9);
            p.setFont(f);
            p.drawText(QRectF(pos.x()-18, pos.y()-10, 36, 20), Qt::AlignCenter, QString("%1%").arg(pct));

            start += span;
        }

        p.setBrush(QColor(245,247,246,255));
        p.setPen(Qt::NoPen);
        p.drawEllipse(inner);

        p.setPen(QColor(0,0,0,120));
        QFont f = font(); f.setBold(true); f.setPointSize(10);
        p.setFont(f);
        p.drawText(inner, Qt::AlignCenter, "📊");
    }

private:
    QList<Slice> m_slices;
};

// Simple bar chart widget for stats dashboards.
class BarChart : public QWidget {
public:
    struct Bar { double value; QString label; };
    explicit BarChart(QWidget* parent=nullptr) : QWidget(parent) { setMinimumHeight(180); }
    void setData(const QList<Bar>& b) { m_bars = b; update(); }

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

        p.setPen(QColor(0,0,0,120));
        QFont t = font(); t.setBold(true); t.setPointSize(9);
        p.setFont(t);
        p.drawText(QRect(r.left()+10, r.top()-2, r.width()-20, 16),
                   Qt::AlignLeft|Qt::AlignVCenter, "Nombre");

        QFont f = font(); f.setPointSize(8); f.setBold(true);
        p.setFont(f);

        int tickCount = 4;
        for (int i=0;i<=tickCount;i++){
            double frac = (double)i / (double)tickCount;
            double val = maxV * (1.0 - frac);
            int y = (int)(plot.top() + frac * plot.height());
            p.setPen(QColor(0,0,0,90));
            p.drawText(QRect(r.left(), y-8, leftPad-6, 16),
                       Qt::AlignRight|Qt::AlignVCenter, QString::number((int)std::round(val)));
            p.setPen(QColor(0,0,0,25));
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
            g.setColorAt(0, W_GREEN.lighter(130));
            g.setColorAt(1, W_GREEN.darker(120));

            p.setPen(Qt::NoPen);
            p.setBrush(g);
            p.drawRoundedRect(bar, 6, 6);

            p.setPen(QColor(0,0,0,120));
            p.drawText(QRect(bar.left(), plot.bottom()+6, bar.width(), 18),
                       Qt::AlignCenter, m_bars[i].label);
        }
    }

private:
    QList<Bar> m_bars;
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
    dd->setIcon(qApp->style()->standardIcon(QStyle::SP_ArrowDown));
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

static QFrame* empBottomBarWithText(QStyle* st, const QString& text)
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
    dd->setIcon(qApp->style()->standardIcon(QStyle::SP_ArrowDown));
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
        setFixedSize(520, 210);

        QWidget* card = new QWidget(this);
        card->setGeometry(0, 0, 520, 210);
        card->setObjectName("card");
        card->setStyleSheet(
            "QWidget#card{ background:#F4F9F8; border-radius:16px;"
            " border:1.5px solid #A3CAD3; }");

        QVBoxLayout* root = new QVBoxLayout(card);
        root->setContentsMargins(16, 16, 16, 16);
        root->setSpacing(12);

        // ── Header ──
        QFrame* head = new QFrame;
        head->setFixedHeight(50);
        head->setStyleSheet(
            "QFrame{ background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
            " stop:0 #0A5F58, stop:1 #12443B); border-radius:12px; }");
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
        body->setStyleSheet(
            "QFrame{ background:rgba(255,255,255,0.85);"
            " border:1px solid rgba(10,95,88,0.20); border-radius:12px; }");
        QVBoxLayout* bl = new QVBoxLayout(body);
        bl->setContentsMargins(14, 12, 14, 12);

        QLabel* msg = new QLabel(message);
        msg->setStyleSheet("color:#12443B; font-weight:700; background:transparent; border:none;");
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
        ok->setStyleSheet(
            "QPushButton{ background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
            " stop:0 #0A5F58, stop:1 #12443B);"
            " border:none; border-radius:12px; padding:8px 24px;"
            " font-weight:700; color:white; }"
            "QPushButton:hover{ background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
            " stop:0 #0d7a71, stop:1 #1a5c50); }");
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
    resize(1320, 680);

    QWidget* root = new QWidget(this);
    root->setObjectName("root");
    setCentralWidget(root);

    QStyle* st = style();

    // ── BioSimple CRUD instance + edit-mode state ──
    CrudeBioSimple* crud     = new CrudeBioSimple;
    bool*    bioEditMode     = new bool(false);
    QString* bioEditRef      = new QString;

    auto applyTheme = [=](bool dark){
        const QString bg = dark ? "#1F2A33" : C_BG;
        const QString text = dark ? "#E8EEF2" : C_TEXT_DARK;
        const QString inputBg = dark ? "rgba(27,36,45,0.88)" : "rgba(255,255,255,0.65)";
        const QString comboBg = dark ? "rgba(30,40,49,0.86)" : "rgba(255,255,255,0.45)";
        const QString border = dark ? "rgba(255,255,255,0.16)" : "rgba(0,0,0,0.12)";
        const QString headerBg = dark ? "#324752" : C_TABLE_HDR;
        const QString headerText = dark ? "rgba(255,255,255,0.86)" : "rgba(0,0,0,0.60)";
        const QString gridColor = dark ? "rgba(255,255,255,0.12)" : "rgba(0,0,0,0.10)";
        const QString tableBg = dark ? "#121920" : C_ROW_ODD;
        const QString tableAltBg = dark ? "#19232D" : C_ROW_EVEN;
        const QString tableText = dark ? "#E8EEF2" : "rgba(0,0,0,0.70)";

        root->setStyleSheet(QString(R"(
            #root { background:%1; }
            QLabel { color: %2; }
            QLineEdit {
                background: %3;
                border: 1px solid %4;
                border-radius: 12px;
                padding: 10px 14px;
                color: %2;
            }
            QComboBox{
                background: %5;
                border: 1px solid %4;
                border-radius: 10px;
                padding: 8px 12px;
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

    // ---- Global button hover color animation ----
    qApp->installEventFilter(new ButtonAnimator(this));

    // ==========================================================
    // PAGE 0 : LOGIN
    // ==========================================================
    LoginWindow* loginPage = new LoginWindow;
    stack->addWidget(loginPage);

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

    QObject::connect(signupServer, &SignupServer::faceLoginSucceeded, this, [=](const QString& identity) {
        setWindowTitle("Gestion des Employés");
        if (globalBar->bBioSimple)   globalBar->bBioSimple->setChecked(false);
        if (globalBar->bPublication) globalBar->bPublication->setChecked(false);
        if (globalBar->bEquipement)  globalBar->bEquipement->setChecked(false);
        if (globalBar->bExp)         globalBar->bExp->setChecked(false);
        if (globalBar->bProjet)      globalBar->bProjet->setChecked(false);
        if (globalBar->bEmployee)    globalBar->bEmployee->setChecked(true);
        stack->setCurrentIndex(EMP_LIST);

        this->showNormal();
        this->raise();
        this->activateWindow();

        QMessageBox::information(this,
                                 "Face ID",
                                 "Connexion Face ID reussie pour : " + identity);
    });

    QObject::connect(signupServer, &SignupServer::googleLoginSucceeded, this, [=](const QString& identity) {
        setWindowTitle("Gestion des Employés");
        if (globalBar->bBioSimple)   globalBar->bBioSimple->setChecked(false);
        if (globalBar->bPublication) globalBar->bPublication->setChecked(false);
        if (globalBar->bEquipement)  globalBar->bEquipement->setChecked(false);
        if (globalBar->bExp)         globalBar->bExp->setChecked(false);
        if (globalBar->bProjet)      globalBar->bProjet->setChecked(false);
        if (globalBar->bEmployee)    globalBar->bEmployee->setChecked(true);
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

        QSqlQuery authQuery(db);
        authQuery.prepare("SELECT COUNT(*) FROM \"Employés\" "
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

        bool isAuthenticated = false;
        if (authQuery.next()) {
            isAuthenticated = authQuery.value(0).toInt() > 0;
        }

        if (!isAuthenticated) {
            QMessageBox::warning(this,
                                 "Connexion refusée",
                                 "E-mail ou mot de passe invalide.");
            loginPage->passEdit->clear();
            loginPage->passEdit->setFocus();
            return;
        }

        loginPage->updateRememberedCredentials(email, pass, loginPage->isRemembered());

        // Clear field (show success visually)
        loginPage->clearFields();
        loginPage->getCaptchaWidget()->refresh();   // reset CAPTCHA for next login // reset CAPTCHA for next login

        // Show beautiful success dialog

        // Show beautiful success dialog
        SuccessDialog successDlg("Connexion réussie !",
                                  "Bienvenue sur SmartVision",
                                  this);
        successDlg.exec();

        // Go to Employee module after login
        setWindowTitle("Gestion des Employés");
        if (globalBar->bBioSimple)   globalBar->bBioSimple->setChecked(false);
        if (globalBar->bPublication) globalBar->bPublication->setChecked(false);
        if (globalBar->bEquipement)  globalBar->bEquipement->setChecked(false);
        if (globalBar->bExp)         globalBar->bExp->setChecked(false);
        if (globalBar->bProjet)      globalBar->bProjet->setChecked(false);
        if (globalBar->bEmployee)    globalBar->bEmployee->setChecked(true);
        stack->setCurrentIndex(EMP_LIST);
    });

    // Handle logout button
    QObject::connect(globalBar->bLogout, &QPushButton::clicked, this, [=](){
        setWindowTitle("SmartVision - Connexion");
        if (g_voiceCmd) g_voiceCmd->hide();
        if (globalBar->bVoice) globalBar->bVoice->setChecked(false);
        stack->setCurrentIndex(LOGIN);
    });

    QObject::connect(globalBar->bTheme, &QPushButton::clicked, this, [=](){
        g_darkThemeEnabled = !g_darkThemeEnabled;
        if (g_applyThemeFn) g_applyThemeFn(g_darkThemeEnabled);
        syncThemeToggleButtons();
    });

    // ==========================================================
    // PAGE 1 : BioSimple - Gestion des Échantillons (LIST)
    // ==========================================================
    QWidget* page1 = new QWidget;
    QVBoxLayout* p1 = new QVBoxLayout(page1);
    p1->setContentsMargins(22, 18, 22, 18);
    p1->setSpacing(14);

    ModulesBar barBioList;
    p1->addWidget(makeHeaderBlock(st, "Gestion des Échantillons", ModuleTab::BioSimple, &barBioList));
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
    stack->addWidget(page1);

    // ==========================================================
    // PAGE 1 : BioSimple - Ajouter / Modifier
    // ==========================================================
    QWidget* page2 = new QWidget;
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
    left2L->setSpacing(10);

    auto sectionTitle = [&](const QString& t){
        QLabel* lab = new QLabel(t);
        lab->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
        return lab;
    };

    auto formRow = [&](QStyle::StandardPixmap sp, const QString& label, QWidget* input){
        QFrame* r = softBox();
        QHBoxLayout* l = new QHBoxLayout(r);
        l->setContentsMargins(10,8,10,8);
        l->setSpacing(10);

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
    leRef->setPlaceholderText("Référence");
    // Référence : chiffres + lettres (alphanumérique, tiret, underscore)
    connect(leRef, &QLineEdit::textChanged, leRef, [leRef](const QString& text) {
        QString filtered;
        for (const QChar& c : text)
            if (c.isLetterOrNumber() || c == '-' || c == '_')
                filtered += c;
        if (filtered != text) {
            const int pos = leRef->cursorPosition();
            leRef->blockSignals(true);
            leRef->setText(filtered);
            leRef->setCursorPosition(qMin(pos, filtered.length()));
            leRef->blockSignals(false);
        }
    });
    left2L->addWidget(formRow(QStyle::SP_FileIcon, "Référence", leRef));

    // Real-time reference format validation — border highlight only (text shown in bioErrPanel on save)
    connect(leRef, &QLineEdit::textChanged, leRef, [=](const QString& text){
        const QString t = text.trimmed();
        if (t.isEmpty()) {
            leRef->setStyleSheet("QLineEdit{ border: 1.5px solid #f59e0b; border-radius:8px; padding:4px 8px; }");
            return;
        }
        bool hasLetter = false, hasDigit = false;
        for (const QChar& c : t) {
            if (c.isLetter()) hasLetter = true;
            if (c.isDigit())  hasDigit  = true;
        }
        leRef->setStyleSheet((!hasLetter || !hasDigit)
            ? "QLineEdit{ border: 1.5px solid #f59e0b; border-radius:8px; padding:4px 8px; }"
            : "");
    });
    // Uniqueness check on focus-out — red border if duplicate
    connect(leRef, &QLineEdit::editingFinished, leRef, [=](){
        if (*bioEditMode) return;
        const QString ref = leRef->text().trimmed();
        if (ref.isEmpty()) return;
        bool hasLetter = false, hasDigit = false;
        for (const QChar& c : ref) {
            if (c.isLetter()) hasLetter = true;
            if (c.isDigit())  hasDigit  = true;
        }
        if (!hasLetter || !hasDigit) return;
        QSqlQuery dup;
        dup.prepare("SELECT COUNT(1) FROM \"BioSample\" WHERE \"Reference_de_léchantillon\" = ?");
        dup.addBindValue(ref);
        if (dup.exec() && dup.next() && dup.value(0).toInt() > 0)
            leRef->setStyleSheet("QLineEdit{ border: 1.5px solid #dc2626; border-radius:8px; padding:4px 8px; }");
    });

    // collection field removed per latest request

    left2L->addWidget(sectionTitle("Dates"));
    // keep pointers to the date edits so the lookup lambda can modify them
    QDateEdit *dCollect = nullptr, *dExpire = nullptr;
    left2L->addWidget(blueDateRow("Date de collecte", QDate::currentDate(), dCollect));
    left2L->addWidget(redDateRow("Date d'expiration", QDate::currentDate().addDays(30), dExpire));

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
            "  color: %1;"
            "  border: 1px solid #c8e6c9;"
            "  font-weight: 700; outline: none;"
            "}"
            "QCalendarWidget QAbstractItemView:disabled { color: #b0bec5; }"
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

    // Température + unité °C
    QFrame* tempFrame = new QFrame;
    QHBoxLayout* tempHL = new QHBoxLayout(tempFrame);
    tempHL->setContentsMargins(0,0,0,0); tempHL->setSpacing(4);
    QLineEdit* cbTemp2 = new QLineEdit;
    cbTemp2->setPlaceholderText("ex: -80");
    cbTemp2->setFixedWidth(80);
    // Température : chiffres + signe moins uniquement (ex: -80, -20, 4, 37)
    cbTemp2->setValidator(new QRegularExpressionValidator(
        QRegularExpression("^-?\\d{0,5}$"), cbTemp2));
    QLabel* lbDeg = new QLabel("°C");
    lbDeg->setStyleSheet("color: rgba(0,0,0,0.50); font-weight:700;");
    tempHL->addWidget(cbTemp2); tempHL->addWidget(lbDeg); tempHL->addStretch(1);
    left2L->addWidget(formRow(QStyle::SP_BrowserStop, "Température", tempFrame));

    QComboBox* cbDanger = new QComboBox;
    cbDanger->addItems({"Niveau de danger", "BSL-1", "BSL-2", "BSL-3"});
    cbDanger->setFixedWidth(170);
    left2L->addWidget(formRow(QStyle::SP_MessageBoxWarning, "Niveau de danger", cbDanger));

    left2L->addStretch(1);

    QFrame* right2 = softBox();
    QVBoxLayout* right2L = new QVBoxLayout(right2);
    right2L->setContentsMargins(12,12,12,12);
    right2L->setSpacing(10);

    right2L->addWidget(sectionTitle("Données"));

    // ── Network manager for AI field correction ──
    QNetworkAccessManager* bioAiNet = new QNetworkAccessManager(this);

    // ── Helper: call Groq to correct a field value ──
    auto callGroqCorrect = [=](const QString& fieldType,
                                const QString& userInput,
                                QLineEdit* targetField,
                                QPushButton* btn)
    {
        if (userInput.trimmed().isEmpty()) return;
        btn->setEnabled(false);
        btn->setText("⏳");

        QString system;
        if (fieldType == "type") {
            system = "Tu es un expert en biologie. L'utilisateur saisit un type d'échantillon biologique. "
                     "Corrige et normalise le terme en UN SEUL MOT parmi exactement : "
                     "ADN, ARN, Protéine, Cellule, Tissu, Organisme. "
                     "Réponds UNIQUEMENT avec le terme corrigé, rien d'autre, aucune explication.";
        } else {
            system = "Tu es un expert en biologie. L'utilisateur saisit un nom d'organisme source. "
                     "Corrige et retourne le nom scientifique correct de l'organisme. "
                     "Réponds UNIQUEMENT avec le nom scientifique (ex: Homo sapiens, Mus musculus, "
                     "Escherichia coli), rien d'autre, aucune explication.";
        }

        QJsonArray msgs = {
            QJsonObject{{"role","system"},{"content",system}},
            QJsonObject{{"role","user"},  {"content",userInput}}
        };
        QJsonObject body;
        body["model"]       = GROQ_API_MODEL;
        body["messages"]    = msgs;
        body["max_tokens"]  = 20;
        body["temperature"] = 0.1;

        QUrl bioAiUrl(GROQ_API_URL);
        QNetworkRequest req(bioAiUrl);
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        req.setRawHeader("Authorization", ("Bearer " + GROQ_API_KEY).toUtf8());
        QSslConfiguration ssl = QSslConfiguration::defaultConfiguration();
        ssl.setPeerVerifyMode(QSslSocket::VerifyNone);
        req.setSslConfiguration(ssl);

        QNetworkReply* rpl = bioAiNet->post(req, QJsonDocument(body).toJson());
        connect(rpl, &QNetworkReply::sslErrors, rpl,
                [rpl](const QList<QSslError>&){ rpl->ignoreSslErrors(); });
        connect(rpl, &QNetworkReply::finished, this, [=](){
            btn->setEnabled(true);
            btn->setText("🤖");
            QByteArray data = rpl->readAll();
            rpl->deleteLater();
            QJsonObject root = QJsonDocument::fromJson(data).object();
            if (root.contains("error")) return;
            QString corrected = root["choices"].toArray().first()
                                    .toObject()["message"].toObject()["content"]
                                    .toString().trimmed();
            if (!corrected.isEmpty()) targetField->setText(corrected);
        });
    };

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

    QPushButton* btnAiType = new QPushButton("🤖");
    btnAiType->setFixedSize(30, 30);
    btnAiType->setToolTip("Corriger avec IA (Groq)");
    btnAiType->setCursor(Qt::PointingHandCursor);
    btnAiType->setStyleSheet(
        "QPushButton{ background:rgba(109,40,217,0.15); border-radius:8px; border:none; font-size:14px; }"
        "QPushButton:hover{ background:rgba(109,40,217,0.30); }"
        "QPushButton:disabled{ color:rgba(0,0,0,0.3); }"
    );

    QWidget* typeContainer = new QWidget;
    QHBoxLayout* typeCL = new QHBoxLayout(typeContainer);
    typeCL->setContentsMargins(0,0,0,0);
    typeCL->setSpacing(4);
    typeCL->addWidget(cbType2);
    typeCL->addWidget(btnAiType);

    right2L->addWidget(formRow(QStyle::SP_FileIcon, "Type", typeContainer));

    connect(btnAiType, &QPushButton::clicked, this, [=](){
        callGroqCorrect("type", cbType2->text(), cbType2, btnAiType);
    });

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

    QPushButton* btnAiOrg = new QPushButton("🤖");
    btnAiOrg->setFixedSize(30, 30);
    btnAiOrg->setToolTip("Corriger avec IA (Groq)");
    btnAiOrg->setCursor(Qt::PointingHandCursor);
    btnAiOrg->setStyleSheet(
        "QPushButton{ background:rgba(109,40,217,0.15); border-radius:8px; border:none; font-size:14px; }"
        "QPushButton:hover{ background:rgba(109,40,217,0.30); }"
        "QPushButton:disabled{ color:rgba(0,0,0,0.3); }"
    );

    QWidget* orgContainer = new QWidget;
    QHBoxLayout* orgCL = new QHBoxLayout(orgContainer);
    orgCL->setContentsMargins(0,0,0,0);
    orgCL->setSpacing(4);
    orgCL->addWidget(cbOrg2);
    orgCL->addWidget(btnAiOrg);

    right2L->addWidget(formRow(QStyle::SP_DirIcon, "Organisme", orgContainer));

    connect(btnAiOrg, &QPushButton::clicked, this, [=](){
        callGroqCorrect("organisme", cbOrg2->text(), cbOrg2, btnAiOrg);
    });

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
    p2->addWidget(outer2, 1);

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

    stack->addWidget(page2);

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
    stack->addWidget(page3);

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
    stack->addWidget(page4);

    // ==========================================================
    // PAGE 4 : BioSimple - STATISTIQUES
    // ==========================================================
    QWidget* page5 = new QWidget;
    QVBoxLayout* p5 = new QVBoxLayout(page5);
    p5->setContentsMargins(22, 18, 22, 18);
    p5->setSpacing(14);

    ModulesBar barBioStats;
    p5->addWidget(makeHeaderBlock(st, "Statistiques", ModuleTab::BioSimple, &barBioStats));
    connectModulesSwitch(this, stack, barBioStats);

    QFrame* outer5 = new QFrame;
    outer5->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }").arg(C_PANEL_BG, C_PANEL_BR));
    QVBoxLayout* outer5L = new QVBoxLayout(outer5);
    outer5L->setContentsMargins(12,12,12,12);
    outer5L->setSpacing(12);

    QFrame* actBar = new QFrame;
    actBar->setStyleSheet("QFrame{ background: rgba(255,255,255,0.35); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QHBoxLayout* actL = new QHBoxLayout(actBar);
    actL->setContentsMargins(12,10,12,10);
    actL->setSpacing(12);

    QLabel* hint = new QLabel("Aperçu des indicateurs clés");
    hint->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");

    actL->addWidget(hint);
    actL->addStretch(1);
    outer5L->addWidget(actBar);

    QFrame* dash = new QFrame;
    dash->setStyleSheet("QFrame{ background: rgba(255,255,255,0.55); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QHBoxLayout* dashL = new QHBoxLayout(dash);
    dashL->setContentsMargins(12,12,12,12);
    dashL->setSpacing(12);

    QFrame* pieCard = softBox();
    QVBoxLayout* pcL = new QVBoxLayout(pieCard);
    pcL->setContentsMargins(12,12,12,12);
    pcL->setSpacing(10);

    QLabel* pieTitle = new QLabel("Répartition par type d’échantillon");
    pieTitle->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");

    DonutChart* pie = new DonutChart;
    // Data will be loaded dynamically from DB in updateBioStats

    pcL->addWidget(pieTitle);
    pcL->addWidget(pie, 1);

    QFrame* legendCard = softBox();
    QVBoxLayout* lgL = new QVBoxLayout(legendCard);
    lgL->setContentsMargins(12,12,12,12);
    lgL->setSpacing(10);

    QLabel* lgTitle = new QLabel("Légende");
    lgTitle->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    lgL->addWidget(lgTitle);

    auto legendRow=[&](const QColor& c, const QString& t){
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

    lgL->addStretch(1); // rows added dynamically in updateBioStats

    QFrame* barCard = softBox();
    QVBoxLayout* bcL = new QVBoxLayout(barCard);
    bcL->setContentsMargins(12,12,12,12);
    bcL->setSpacing(10);

    QLabel* barTitle = new QLabel("Nombre d’échantillons collectés par mois");
    barTitle->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");

    BarChart* bars = new BarChart;
    bars->setData({
        {12, "Jan"}, {18, "Fév"}, {22, "Mar"}, {15, "Avr"},
        {28, "Mai"}, {20, "Juin"}, {26, "Juil"}, {19, "Août"},
        {24, "Sep"}, {30, "Oct"}, {27, "Nov"}, {21, "Déc"}
    });

    bcL->addWidget(barTitle);
    bcL->addWidget(bars, 1);

    dashL->addWidget(pieCard, 1);
    dashL->addWidget(legendCard, 1);
    dashL->addWidget(barCard, 2);

    outer5L->addWidget(dash);
    p5->addWidget(outer5, 1);

    QFrame* bottom5 = new QFrame;
    bottom5->setFixedHeight(64);
    bottom5->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* bottom5L = new QHBoxLayout(bottom5);
    bottom5L->setContentsMargins(14,10,14,10);

    QPushButton* back5 = actionBtn("Retour", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_ArrowBack), true);
    bottom5L->addWidget(back5);
    bottom5L->addStretch(1);

    p5->addWidget(bottom5);
    stack->addWidget(page5);

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
    gp1->addWidget(makeHeaderBlock(st, "Gestion des Projets de Recherche", ModuleTab::GestionProjet, &barProjList));
    connectModulesSwitch(this, stack, barProjList);

    QFrame* pBar = new QFrame;
    pBar->setFixedHeight(54);
    pBar->setStyleSheet("background: rgba(255,255,255,0.22); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* pBarL = new QHBoxLayout(pBar);
    pBarL->setContentsMargins(14, 8, 14, 8);
    pBarL->setSpacing(10);

    QLineEdit* pSearch = new QLineEdit;
    pSearch->setPlaceholderText("Rechercher par tous");
    pSearch->addAction(st->standardIcon(QStyle::SP_FileDialogContentsView), QLineEdit::LeadingPosition);

    QComboBox* pDomain = new QComboBox;
    pDomain->addItems({"Domaine", "Génomique", "Protéomique", "Pharmacologie", "Immunologie", "Biologie végétale", "Microbiologie", "Neurosciences", "Biotechnologies", "Génétique", "Bioinformatique"});

    QComboBox* pStatut = new QComboBox;
    pStatut->addItems({"Statut", "Planifié", "En cours", "En retard", "Critique", "Suspendu", "Terminé", "Annulé"});

    QComboBox* pBudget = new QComboBox;
    pBudget->addItems({"Budget", "500 - 50 000 TND", "50 000 - 500 000 TND", "500 000 - 5 000 000 TND", "5 000 000+ TND"});

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
            QString err;
            if (!projCrud->deleteProjet(id, &err)) {
                showToast(this, "Erreur : " + err, false);
                return;
            }
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

    stack->addWidget(proj1);

    // ==========================================================
    // PAGE 6 : Gestion Projet - Widget 2 (AJOUT/MODIF)
    // ==========================================================
    QWidget* proj2 = new QWidget;
    proj2->setStyleSheet(QString("QWidget { background: %1; }").arg(C_BG));
    QVBoxLayout* gp2 = new QVBoxLayout(proj2);
    gp2->setContentsMargins(22, 18, 22, 18);
    gp2->setSpacing(14);

    ModulesBar barProjForm;
    gp2->addWidget(makeHeaderBlock(st, "Ajouter / Modifier un projet", ModuleTab::GestionProjet, &barProjForm));
    connectModulesSwitch(this, stack, barProjForm);

    // ── Shared field styles ───────────────────────────────────
    const QString fldOk  = "background:rgba(255,255,255,0.92); border:1.5px solid rgba(0,0,0,0.20); border-radius:12px; padding:12px 16px; color:rgba(0,0,0,0.88); font-weight:800; font-size:14px; min-height:20px;";
    const QString fldErr = "background:rgba(255,240,240,0.96); border:1.5px solid #c0392b; border-radius:12px; padding:12px 16px; color:rgba(0,0,0,0.88); font-weight:800; font-size:14px; min-height:20px;";
    const QString fldOkCb= "background:rgba(255,255,255,0.92); border:1.5px solid rgba(0,0,0,0.20); border-radius:12px; padding:10px 14px; color:rgba(0,0,0,0.88); font-weight:800; font-size:14px; min-height:20px;";
    const QString fldErrCb="background:rgba(255,240,240,0.96); border:1.5px solid #c0392b; border-radius:12px; padding:10px 14px; color:rgba(0,0,0,0.88); font-weight:800; font-size:14px; min-height:20px;";

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
        lab->setStyleSheet("color:rgba(10,95,88,1.0); font-weight:900; font-size:15px; padding:4px 0;");
        return lab;
    };

    // ── Row helper ────────────────────────────────────────────
    auto projRow = [&](QStyle::StandardPixmap sp, const QString& label, QWidget* input){
        QFrame* r = softBox();
        r->setMinimumHeight(58);
        QHBoxLayout* l = new QHBoxLayout(r);
        l->setContentsMargins(14,10,14,10);
        l->setSpacing(12);
        QToolButton* ic = new QToolButton;
        ic->setAutoRaise(true);
        ic->setIcon(st->standardIcon(sp));
        ic->setIconSize(QSize(22,22));
        QLabel* lab = new QLabel(label);
        lab->setStyleSheet("color:#12443B; font-weight:900; font-size:14px;");
        l->addWidget(ic); l->addWidget(lab); l->addStretch(1); l->addWidget(input);
        return r;
    };

    // ── Helper: apply white themed calendar to a QDateEdit ──────
    auto applyProjCalendar = [](QDateEdit* de) {
        QCalendarWidget* cw = de->calendarWidget();
        if (!cw) return;
        cw->setStyleSheet(
            "QCalendarWidget QWidget#qt_calendar_navigationbar {"
            "  background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
            "    stop:0 #0A5F58, stop:1 #12443B);"
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
            "  background: #A3CAD3; color: #12443B;"
            "  font-weight: 900; font-size: 11px;"
            "  border: none; padding: 5px 0;"
            "}"
            "QCalendarWidget QAbstractItemView {"
            "  background: #ffffff;"
            "  selection-background-color: #0A5F58;"
            "  selection-color: white;"
            "  color: rgba(0,0,0,0.80);"
            "  border: 1px solid #A3CAD3;"
            "  font-weight: 700; outline: none;"
            "}"
            "QCalendarWidget QAbstractItemView:disabled { color: #b0bec5; }"
            "QCalendarWidget QWidget { alternate-background-color: #EAF4F4; }"
            "QCalendarWidget QMenu {"
            "  background: white; color: #12443B;"
            "  selection-background-color: #0A5F58; selection-color: white;"
            "}"
        );
        QTextCharFormat todayFmt;
        todayFmt.setBackground(QColor("#A3CAD3"));
        todayFmt.setForeground(QColor("#12443B"));
        todayFmt.setFontWeight(QFont::Black);
        cw->setDateTextFormat(QDate::currentDate(), todayFmt);
    };

    QFrame* outP2 = new QFrame;
    outP2->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius:14px; }").arg(C_PANEL_BG, C_PANEL_BR));
    QHBoxLayout* outP2L = new QHBoxLayout(outP2);
    outP2L->setContentsMargins(12,12,12,12);
    outP2L->setSpacing(12);

    // ══════════════════════════════════════════════════════════
    // LEFT PANEL — Identité du projet
    // ══════════════════════════════════════════════════════════
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
    projName->setStyleSheet(fldOk);
    projName->setMaxLength(150);
    QLabel* errProjName = mkProjErr();
    p2LeftL->addWidget(projRow(QStyle::SP_DirIcon, "Nom *", projName));
    p2LeftL->addWidget(errProjName);

    // Domaine de recherche
    QComboBox* projDomainEdit = new QComboBox;
    projDomainEdit->addItems({"— Sélectionner un domaine —",
                              "Génomique", "Protéomique", "Pharmacologie", "Immunologie",
                              "Biologie végétale", "Microbiologie", "Neurosciences",
                              "Biotechnologies", "Génétique", "Bioinformatique",
                              "Biochimie", "Santé publique", "Virologie", "Oncologie"});
    projDomainEdit->setFixedWidth(240);
    projDomainEdit->setMinimumHeight(46);
    projDomainEdit->setStyleSheet(fldOkCb);
    QLabel* errProjDomain = mkProjErr();
    p2LeftL->addWidget(projRow(QStyle::SP_ComputerIcon, "Domaine *", projDomainEdit));
    p2LeftL->addWidget(errProjDomain);

    // Statut — QListWidget scrollable
    QListWidget* projStatus = new QListWidget;
    const QStringList projStatusItems = {"Planifié", "En cours", "En retard", "Critique", "Suspendu", "Terminé", "Annulé"};
    for (const QString& s : projStatusItems) {
        QListWidgetItem* it = new QListWidgetItem(s);
        it->setData(Qt::UserRole, s);
        projStatus->addItem(it);
    }
    projStatus->setCurrentRow(0);
    projStatus->setFixedHeight(160);
    projStatus->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    projStatus->setStyleSheet(
        "QListWidget{ background:rgba(255,255,255,0.96); color:rgba(0,0,0,0.82);"
        "  border:1.5px solid rgba(0,0,0,0.15); border-radius:8px; outline:none; }"
        "QListWidget::item{ padding:8px 12px; font-weight:700; font-size:12px; }"
        "QListWidget::item:selected{ background:rgba(10,95,88,0.18); color:rgba(10,95,88,1); }"
        "QListWidget::item:hover{ background:rgba(10,95,88,0.08); }"
        "QScrollBar:vertical{ background:transparent; width:5px; }"
        "QScrollBar::handle:vertical{ background:rgba(10,95,88,0.30); border-radius:2px; }"
        "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{ height:0; }"
    );
    p2LeftL->addWidget(projRow(QStyle::SP_MessageBoxInformation, "Statut *", projStatus));

    // Numéro d'approbation éthique
    QLineEdit* projEthique = new QLineEdit;
    projEthique->setPlaceholderText("ex: CPP-2024/017  (requis si En cours)");
    projEthique->setStyleSheet(fldOk);
    projEthique->setMaxLength(100);
    QLabel* errProjEthique = mkProjErr();
    p2LeftL->addWidget(projRow(QStyle::SP_FileDialogInfoView, "Éthique", projEthique));
    p2LeftL->addWidget(errProjEthique);

    // ── Instant validation for status/ethics cross-rule ──────
    auto checkEthiqueRule = [=](){
        const bool enCours = (projStatus->currentItem() && projStatus->currentItem()->text() == "En cours");
        if (enCours && projEthique->text().trimmed().isEmpty()) {
            showProjErr(errProjEthique,
                        "Obligatoire pour un projet « En cours ».",
                        projEthique, fldErr);
        } else {
            clearProjErr(errProjEthique, projEthique, fldOk);
        }
    };
    QObject::connect(projStatus, &QListWidget::currentRowChanged, this, [=](int){ checkEthiqueRule(); });
    QObject::connect(projEthique, &QLineEdit::textChanged, this, [=](const QString&){ checkEthiqueRule(); });

    // ── Nom instant validation ────────────────────────────────
    QObject::connect(projName, &QLineEdit::textChanged, this, [=](const QString& v){
        const QString t = v.trimmed();
        if (t.isEmpty()) {
            showProjErr(errProjName, "Le nom est obligatoire.", projName, fldErr);
        } else if (t.length() < 3) {
            showProjErr(errProjName, "Minimum 3 caractères.", projName, fldErr);
        } else if (t.length() > 150) {
            showProjErr(errProjName, "Maximum 150 caractères.", projName, fldErr);
        } else {
            static const QRegularExpression allowed(R"(^[A-Za-zÀ-ÖØ-öø-ÿ0-9 \-()/]+$)");
            if (!allowed.match(t).hasMatch())
                showProjErr(errProjName, "Caractères non autorisés (lettres, chiffres, espaces, - ( ) /).", projName, fldErr);
            else
                clearProjErr(errProjName, projName, fldOk);
        }
    });

    // ── Domaine instant validation ────────────────────────────
    QObject::connect(projDomainEdit, &QComboBox::currentTextChanged, this, [=](const QString& v){
        if (v.startsWith("—"))
            showProjErr(errProjDomain, "Sélectionnez un domaine.", projDomainEdit, fldErrCb);
        else
            clearProjErr(errProjDomain, projDomainEdit, fldOkCb);
    });

    p2LeftL->addStretch(1);
    p2LeftScroll->setWidget(p2LeftContent);

    // ══════════════════════════════════════════════════════════
    // RIGHT PANEL — Planification + Financement multi-sources
    // ══════════════════════════════════════════════════════════
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
    projStart->setStyleSheet(
        "QDateEdit{ background:rgba(255,255,255,0.92); border:1.5px solid rgba(0,0,0,0.20);"
        " border-radius:12px; padding:12px 16px; color:#12443B; font-weight:800; font-size:14px; }"
        "QDateEdit::drop-down{ border:0px; width:20px; }"
        "QDateEdit::up-button{ width:0; } QDateEdit::down-button{ width:0; }");
    applyProjCalendar(projStart);
    QLabel* errProjStart = mkProjErr();
    p2RightL->addWidget(projRow(QStyle::SP_FileDialogDetailedView, "Date début *", projStart));
    p2RightL->addWidget(errProjStart);

    QDateEdit* projEnd = new QDateEdit(QDate::currentDate().addMonths(3));
    projEnd->setCalendarPopup(true);
    projEnd->setDisplayFormat("dd/MM/yyyy");
    projEnd->setMinimumWidth(200);
    projEnd->setMinimumHeight(46);
    projEnd->setStyleSheet(
        "QDateEdit{ background:rgba(255,255,255,0.92); border:1.5px solid rgba(0,0,0,0.20);"
        " border-radius:12px; padding:12px 16px; color:#12443B; font-weight:800; font-size:14px; }"
        "QDateEdit::drop-down{ border:0px; width:20px; }"
        "QDateEdit::up-button{ width:0; } QDateEdit::down-button{ width:0; }");
    applyProjCalendar(projEnd);
    QLabel* errProjEnd = mkProjErr();
    p2RightL->addWidget(projRow(QStyle::SP_FileDialogDetailedView, "Date fin", projEnd));
    p2RightL->addWidget(errProjEnd);

    // Date instant cross-validation
    auto checkDates = [=](){
        const QDate d = projStart->date();
        const QDate f = projEnd->date();
        if (d.year() < 2000) {
            showProjErr(errProjStart, "Antérieure à 2000.", projStart, fldErr);
        } else {
            clearProjErr(errProjStart, projStart, fldOk);
        }
        if (f.isValid() && f <= d) {
            showProjErr(errProjEnd, "Doit être après la date de début.", projEnd, fldErr);
        } else if (f.isValid() && f < d.addMonths(1)) {
            showProjErr(errProjEnd, "Durée minimale : 1 mois.", projEnd, fldErr);
        } else if (f.isValid() && f > d.addYears(20)) {
            showProjErr(errProjEnd, "Durée > 20 ans — vérifiez les dates.", projEnd, fldErr);
        } else {
            clearProjErr(errProjEnd, projEnd, fldOk);
        }
    };
    QObject::connect(projStart, &QDateEdit::dateChanged, this, [=](const QDate&){ checkDates(); });
    QObject::connect(projEnd,   &QDateEdit::dateChanged, this, [=](const QDate&){ checkDates(); });

    // Publications (read-only computed)
    QSpinBox* projPubsEdit = new QSpinBox;
    projPubsEdit->setRange(0, 9999);
    projPubsEdit->setValue(0);
    projPubsEdit->setPrefix("Pub: ");
    projPubsEdit->setFixedWidth(200);
    projPubsEdit->setMinimumHeight(46);
    projPubsEdit->setReadOnly(false);
    projPubsEdit->setStyleSheet(
        "QSpinBox{ background:rgba(255,255,255,0.92); border:1.5px solid rgba(0,0,0,0.20);"
        " border-radius:12px; padding:12px 16px; color:#12443B; font-weight:800; font-size:14px; }"
        "QSpinBox::up-button{ subcontrol-origin:border; subcontrol-position:right; width:24px;"
        " border-left:1px solid rgba(0,0,0,0.12); border-radius:0 12px 12px 0; }"
        "QSpinBox::down-button{ width:0; }");
    projPubsEdit->setToolTip("Nombre de publications liées à ce projet.");
    p2RightL->addWidget(projRow(QStyle::SP_FileIcon, "Publications", projPubsEdit));

    // ── Multi-source financement ──────────────────────────────
    p2RightL->addSpacing(4);
    p2RightL->addWidget(projTitle("Sources de financement  (max 10)"));

    // Single funding source + budget fields
    QLabel* finLabel = new QLabel("Source de financement");
    finLabel->setStyleSheet("color:rgba(0,0,0,0.60); font-size:11px; font-weight:700; background:transparent;");
    p2RightL->addWidget(finLabel);

    QLineEdit* projFinancement = new QLineEdit;
    projFinancement->setPlaceholderText("Ex. : ANR, Horizon Europe, Budget interne…");
    projFinancement->setMaxLength(150);
    projFinancement->setStyleSheet(fldOk);
    p2RightL->addWidget(projFinancement);

    QLabel* budgetLabel = new QLabel("Budget (TND)");
    budgetLabel->setStyleSheet("color:rgba(0,0,0,0.60); font-size:11px; font-weight:700; background:transparent;");
    p2RightL->addWidget(budgetLabel);

    // Budget ranges: label → representative double value stored in DB
    struct BudgetRange { QString label; double value; };
    const QList<BudgetRange> budgetRanges = {
        {"500 - 50 000 TND",           500.0},
        {"50 000 - 500 000 TND",       50000.0},
        {"500 000 - 5 000 000 TND",    500000.0},
        {"5 000 000+ TND",             5000000.0}
    };
    QListWidget* projBudgetSpin = new QListWidget;
    for (const auto& br : budgetRanges) {
        QListWidgetItem* it = new QListWidgetItem(br.label);
        it->setData(Qt::UserRole, br.value);
        projBudgetSpin->addItem(it);
    }
    projBudgetSpin->setCurrentRow(0);
    projBudgetSpin->setFixedHeight(110);
    projBudgetSpin->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    projBudgetSpin->setStyleSheet(
        "QListWidget{ background:rgba(255,255,255,0.96); color:rgba(0,0,0,0.82);"
        "  border:1.5px solid rgba(0,0,0,0.15); border-radius:8px; outline:none; }"
        "QListWidget::item{ padding:8px 12px; font-weight:700; font-size:12px; }"
        "QListWidget::item:selected{ background:rgba(10,95,88,0.18); color:rgba(10,95,88,1); }"
        "QListWidget::item:hover{ background:rgba(10,95,88,0.08); }"
        "QScrollBar:vertical{ background:transparent; width:5px; }"
        "QScrollBar::handle:vertical{ background:rgba(10,95,88,0.30); border-radius:2px; }"
        "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{ height:0; }"
    );
    p2RightL->addWidget(projBudgetSpin);

    p2RightL->addStretch(1);
    p2RightScroll->setWidget(p2RightContent);

    outP2L->addWidget(p2LeftScroll);
    outP2L->addWidget(p2RightScroll, 1);
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
    stack->addWidget(proj2);

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
    QPushButton* btnDomaineProj = makeStatCard({"📊", "Répartition des projets\npar domaine", "rgba(10,95,88,0.55)"});
    btnDomaineProj->setEnabled(true);
    btnDomaineProj->setCursor(Qt::PointingHandCursor);
    btnDomaineProj->setStyleSheet(btnDomaineProj->styleSheet().replace("QPushButton:disabled","QPushButton:disabled_UNUSED"));
    row1L->addWidget(btnDomaineProj, 1);

    QPushButton* btnBudgetProj = makeStatCard({"💰", "Distribution des budgets\npar projet", "rgba(42,100,155,0.55)"});
    btnBudgetProj->setEnabled(true);
    btnBudgetProj->setCursor(Qt::PointingHandCursor);
    btnBudgetProj->setStyleSheet(btnBudgetProj->styleSheet().replace("QPushButton:disabled","QPushButton:disabled_UNUSED"));
    row1L->addWidget(btnBudgetProj, 1);

    row1L->addWidget(makeStatCard({"📈", "Évolution de projet\ndans le temps", "rgba(90,65,140,0.55)"}), 1);
    outP3L->addWidget(row1);

    // ── Section : Rapports & analyses ───────────────────────────
    QLabel* secRapports = new QLabel("  Rapports & analyses détaillées");
    secRapports->setStyleSheet(
        "color: rgba(139,47,60,0.85); font-size: 13px; font-weight: 900;"
        "background: rgba(139,47,60,0.07); border-radius: 8px; padding: 6px 12px;"
    );
    outP3L->addWidget(secRapports);

    // Row 2 — 3 analysis buttons
    QFrame* row2 = new QFrame;
    row2->setStyleSheet("QFrame{ background: transparent; border: none; }");
    QHBoxLayout* row2L = new QHBoxLayout(row2);
    row2L->setContentsMargins(0,0,0,0);
    row2L->setSpacing(14);

    QPushButton* btnStatutProj = makeStatCard({"🥧", "Répartition des projets\npar statut", "rgba(139,47,60,0.50)"});
    btnStatutProj->setEnabled(true);
    btnStatutProj->setCursor(Qt::PointingHandCursor);
    btnStatutProj->setStyleSheet(btnStatutProj->styleSheet().replace("QPushButton:disabled","QPushButton:disabled_UNUSED"));
    row2L->addWidget(btnStatutProj, 1);
    QPushButton* btnDomaineBudgetProj = makeStatCard({"🔬", "Distribution par domaine\nde recherche", "rgba(181,103,44,0.55)"});
    btnDomaineBudgetProj->setEnabled(true);
    btnDomaineBudgetProj->setCursor(Qt::PointingHandCursor);
    btnDomaineBudgetProj->setStyleSheet(btnDomaineBudgetProj->styleSheet().replace("QPushButton:disabled","QPushButton:disabled_UNUSED"));
    row2L->addWidget(btnDomaineBudgetProj, 1);
    row2L->addWidget(makeStatCard({"💼", "Répartition budgétaire\npar projet", "rgba(42,100,155,0.50)"}), 1);
    outP3L->addWidget(row2);

    // Row 3 — 3 more analysis buttons
    QFrame* row3 = new QFrame;
    row3->setStyleSheet("QFrame{ background: transparent; border: none; }");
    QHBoxLayout* row3L = new QHBoxLayout(row3);
    row3L->setContentsMargins(0,0,0,0);
    row3L->setSpacing(14);

    row3L->addWidget(makeStatCard({"🗓️", "Timeline\ndes projets", "rgba(55,110,90,0.55)"}), 1);
    row3L->addWidget(makeStatCard({"📚", "Évolution du nombre\nde publications", "rgba(90,65,140,0.50)"}), 1);
    row3L->addWidget(makeStatCard({"⚖️", "Budget prévu\nvs budget dépensé", "rgba(181,103,44,0.50)"}), 1);
    outP3L->addWidget(row3);

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

    row4L->addWidget(makeStatCard({"📋", "Rapport financier trimestriel\n(Excel)", "rgba(0,120,60,0.55)"}), 1);
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
    gp3->addWidget(outP3, 1);

    stack->addWidget(proj3);
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
    ep1->addWidget(makeHeaderBlock(st, "Liste des Expériences", ModuleTab::ExperiencesProtocoles, &barExpList));
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
    bool*           expEditMode = new bool(false);
    int*            expEditId   = new int(0);

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
        expTable->setRowCount(0);
        QList<ExperienceRecord> recs;
        if (!expCrud->loadExperiences(recs)) return;
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
            expTable->setItem(row, 6, mk(rec.disponibiliteEquipement));
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
    stack->addWidget(exp1);
    // ==========================================================
    // PAGE 9 : Expériences & Protocoles - Widget 2 (AJOUT/MODIF)
    // ==========================================================
    QWidget* exp2 = new QWidget;
    QVBoxLayout* ep2 = new QVBoxLayout(exp2);
    ep2->setContentsMargins(22, 18, 22, 18);
    ep2->setSpacing(14);

    ModulesBar barExpForm;
    ep2->addWidget(makeHeaderBlock(st, "Ajouter / Modifier une expérience", ModuleTab::ExperiencesProtocoles, &barExpForm));
    connectModulesSwitch(this, stack, barExpForm);

    QFrame* outE2 = new QFrame;
    outE2->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }").arg(C_PANEL_BG, C_PANEL_BR));
    QHBoxLayout* outE2L = new QHBoxLayout(outE2);
    outE2L->setContentsMargins(12,12,12,12);
    outE2L->setSpacing(12);

    auto expTitle = [&](const QString& t){
        QLabel* lab = new QLabel(t);
        lab->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
        return lab;
    };
    auto expRow = [&](QStyle::StandardPixmap sp, const QString& label, QWidget* input){
        QFrame* r = softBox();
        QHBoxLayout* l = new QHBoxLayout(r);
        l->setContentsMargins(10,8,10,8);
        l->setSpacing(10);

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

    // ── Left panel: Informations ──────────────────────────────
    QFrame* e2Left = softBox();
    e2Left->setFixedWidth(420);
    QVBoxLayout* e2LeftL = new QVBoxLayout(e2Left);
    e2LeftL->setContentsMargins(12,12,12,12);
    e2LeftL->setSpacing(10);

    QLineEdit* eName = new QLineEdit;
    eName->setPlaceholderText("Nom de l’expérience");

    QRegularExpression noDigits("^[^0-9]*$");
    eName->setValidator(new QRegularExpressionValidator(noDigits, eName));

    QLineEdit* eHypo = new QLineEdit;
    eHypo->setPlaceholderText("Hypothese");
    eHypo->setValidator(new QRegularExpressionValidator(noDigits, eHypo));

    QLineEdit* eTypeExp = new QLineEdit;
    eTypeExp->setPlaceholderText("Type_Experience");
    eTypeExp->setValidator(new QRegularExpressionValidator(noDigits, eTypeExp));

    QLineEdit* eResultat = new QLineEdit;
    eResultat->setPlaceholderText("Resultat");
    eResultat->setValidator(new QRegularExpressionValidator(noDigits, eResultat));

    QComboBox* eDisponibilite = new QComboBox;
    eDisponibilite->addItems({"Disponibilité équipement", "Disponible", "Non disponible"});
    eDisponibilite->setFixedWidth(220);
    eDisponibilite->setEnabled(false);
    eDisponibilite->setToolTip("Calculée automatiquement à partir du statut des équipements liés.");

    QComboBox* eProjet = new QComboBox;
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

    e2LeftL->addWidget(expTitle("Informations"));
    e2LeftL->addWidget(expRow(QStyle::SP_DirIcon, "Expérience", eName));
    e2LeftL->addWidget(expRow(QStyle::SP_FileDialogDetailedView, "Hypothese", eHypo));
    e2LeftL->addWidget(expRow(QStyle::SP_FileDialogContentsView, "Type_Experience", eTypeExp));
    e2LeftL->addWidget(expRow(QStyle::SP_FileDialogListView, "Resultat", eResultat));
    e2LeftL->addWidget(expRow(QStyle::SP_DirIcon, "Projet", eProjet));
    e2LeftL->addStretch(1);

    // ── Right panel: Planification ────────────────────────────
    QFrame* e2Right = softBox();
    QVBoxLayout* e2RightL = new QVBoxLayout(e2Right);
    e2RightL->setContentsMargins(12,12,12,12);
    e2RightL->setSpacing(10);

    auto makeDateEdit = [&](QDate d) {
        QDateEdit* de = new QDateEdit(d);
        de->setCalendarPopup(true);
        de->setDisplayFormat("dd/MM/yy");
        de->setStyleSheet("QDateEdit{ background: rgba(255,255,255,0.65); border: 1px solid rgba(0,0,0,0.15); border-radius: 12px; padding: 10px 14px; color: rgba(0,0,0,0.65); font-weight: 900; }");
        return de;
    };

    QDateEdit* eDateDebut = makeDateEdit(QDate::currentDate());
    QDateEdit* eDateFin   = makeDateEdit(QDate::currentDate().addDays(1));
    eDateDebut->setMinimumDate(QDate::currentDate().addDays(1));
    eDateFin->setMinimumDate(eDateDebut->date().addDays(1));

    QObject::connect(eDateDebut, &QDateEdit::dateChanged, this, [=](const QDate& d){
        const QDate minFin = d.addDays(1);
        if (eDateFin->date() < minFin) eDateFin->setDate(minFin);
        eDateFin->setMinimumDate(minFin);
    });

    QComboBox* eStatus = new QComboBox;
    eStatus->addItems({"Statut", "En cours", "Concluante", "Réussie", "Échouée", "Archivée"});
    eStatus->setFixedWidth(200);

    e2RightL->addWidget(expTitle("Planification"));
    e2RightL->addWidget(expRow(QStyle::SP_MessageBoxInformation, "Date début", eDateDebut));
    e2RightL->addWidget(expRow(QStyle::SP_MessageBoxInformation, "Date fin",   eDateFin));
    e2RightL->addWidget(expRow(QStyle::SP_MessageBoxInformation, "Statut",     eStatus));
    e2RightL->addWidget(expRow(QStyle::SP_DriveFDIcon, "Disponibilité équipement", eDisponibilite));
    e2RightL->addStretch(1);

    outE2L->addWidget(e2Left);
    outE2L->addWidget(e2Right, 1);
    ep2->addWidget(outE2, 1);

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
    stack->addWidget(exp2);
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
    ep3->addWidget(outE3, 1);

    stack->addWidget(exp3);

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
    pubSort->addItems({"Tri: Année", "Tri: Impact Factor", "Tri: Citations"});

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

    QTableWidget* pubTable = new QTableWidget(0, 10);
    pubTable->setHorizontalHeaderLabels({"ID","Titre","Journal/Conf.","Année","DOI","Statut","Employé","Mots-clés","Impact","Citations"});
    pubTable->verticalHeader()->setVisible(false);
    pubTable->horizontalHeader()->setStretchLastSection(true);
    pubTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    pubTable->setSelectionMode(QAbstractItemView::SingleSelection);
    pubTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    pubTable->setColumnHidden(0, true);
    pubTable->setColumnHidden(7, true);
    pubTable->setColumnWidth(8, 90);
    pubTable->setColumnWidth(9, 90);

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
            const int maxCols = std::min(pubTable->columnCount(), model->columnCount());
            for (int c = 0; c < maxCols; ++c) {
                QTableWidgetItem* it = new QTableWidgetItem(model->data(model->index(r, c)).toString());
                it->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                pubTable->setItem(r, c, it);
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
                const int av = toIntValue(a.cols.value(9));
                const int bv = toIntValue(b.cols.value(9));
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
    stack->addWidget(pub1);

    // ==========================================================
    // PAGE 12 : Publications - AJOUT / MODIF (PUB_FORM)
    // ==========================================================
    QWidget* pub2 = new QWidget;
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
        l->setContentsMargins(10,8,10,8);
        l->setSpacing(10);

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
    pub2LeftL->setSpacing(10);

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
    pub2RightL->setSpacing(10);

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

    QSpinBox* sbCitationCount = new QSpinBox;
    sbCitationCount->setRange(0, 1000000);
    sbCitationCount->setFixedWidth(220);
    sbCitationCount->setStyleSheet("QSpinBox{ background: rgba(255,255,255,0.65); border: 1px solid rgba(0,0,0,0.15); border-radius: 12px; padding: 10px 14px; color: rgba(0,0,0,0.65); font-weight: 900; }");

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
    pub2RightL->addWidget(pubRow(QStyle::SP_ArrowUp, "Citations", sbCitationCount));
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
    stack->addWidget(pub2);

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

    piePUBL->addWidget(piePUBT);
    piePUBL->addWidget(donutPUB, 1);

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
    pb3->addWidget(outPUB3, 1);

    stack->addWidget(pub3);

    auto updatePublicationStats = [=](){
        QString errorMessage;
        QSqlQueryModel* model = Publication::readAll(nullptr, &errorMessage);
        if (!model) {
            donutPUB->setData({});
            barsPUB->setData({});
            return;
        }

        QMap<QString, int> statusCounts;
        QMap<int, int> yearCounts;

        for (int r = 0; r < model->rowCount(); ++r) {
            const QString status = model->data(model->index(r, 5)).toString().trimmed();
            const int year = model->data(model->index(r, 3)).toInt();
            if (!status.isEmpty()) statusCounts[status] += 1;
            if (year > 0) yearCounts[year] += 1;
        }
        delete model;

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

        QList<BarChart::Bar> bars;
        for (auto it = yearCounts.constBegin(); it != yearCounts.constEnd(); ++it) {
            bars.push_back({(double)it.value(), QString::number(it.key())});
        }
        barsPUB->setData(bars);
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
    eq1->addWidget(makeHeaderBlock(st, "Gestion des Équipements", ModuleTab::Equipement, &barEquipList));
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
    cbEquipType->addItems({"Équipement", "PCR", "Centrifugeuse", "Microscope", "Incubateur"});

    QComboBox* cbEquipStatus = new QComboBox;
    cbEquipStatus->addItems({"Statut", "Actif", "Hors service", "Archivé"});

    QComboBox* cbEquipLoc = new QComboBox;
    cbEquipLoc->addItems({"Localisation", "Lab 101", "Lab 102", "Lab 103", "Lab 201"});

    QPushButton* eqFilters = new QPushButton(st->standardIcon(QStyle::SP_FileDialogDetailedView), "  Filtres");
    eqFilters->setCursor(Qt::PointingHandCursor);
    eqFilters->setStyleSheet(QString(R"(
        QPushButton{
            background:%1; color: rgba(255,255,255,0.92);
            border:1px solid rgba(0,0,0,0.18);
            border-radius: 12px; padding: 10px 16px; font-weight: 800;
        }
        QPushButton:hover{ background: %2; }
    )").arg(C_PRIMARY, C_TOPBAR));

    eqBarL->addWidget(eqSearch, 1);
    eqBarL->addWidget(cbEquipType);
    eqBarL->addWidget(cbEquipStatus);
    eqBarL->addWidget(cbEquipLoc);
    eqBarL->addWidget(eqFilters);
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
    eqTable->setSelectionMode(QAbstractItemView::SingleSelection);
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

    auto equipmentBadgeFromDbStatus = [](const QString& dbStatus) {
        const QString s = dbStatus.toLower();
        if (s.contains("hors") || s.contains("service")) return EquipmentStatus::OutOfOrder;
        if (s.contains("rchiv") || s.contains("archive")) return EquipmentStatus::UnderMaintenance;
        return EquipmentStatus::Available;
    };

    auto loadEqTable = [=](){
        eqTable->setRowCount(0);
        QList<EquipementRecord> recs;
        QString err;
        const QString searchFilter = eqSearch->text().trimmed();
        const QString nomFilter = (cbEquipType->currentIndex() <= 0) ? QString() : cbEquipType->currentText();
        const QString statusFilter = (cbEquipStatus->currentIndex() <= 0) ? QString() : cbEquipStatus->currentText();
        const QString locFilter    = (cbEquipLoc->currentIndex() <= 0) ? QString() : cbEquipLoc->currentText();
        if (!eqCrud->loadEquipements(recs, &err, searchFilter, nomFilter, statusFilter, locFilter)) {
            showToast(this, "Erreur : " + err, false);
            return;
        }

        auto mk = [](const QString& t){
            QTableWidgetItem* it = new QTableWidgetItem(t);
            it->setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);
            return it;
        };

        for (const EquipementRecord& rec : recs) {
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
            if (rec.dateProchaineMaintenance.isValid()) {
                const int daysLeft = QDate::currentDate().daysTo(rec.dateProchaineMaintenance);
                QColor rowColor;
                if (daysLeft <= 0)     rowColor = QColor(255, 230, 230); // rouge clair — dépassé
                else if (daysLeft <= 7) rowColor = QColor(255, 248, 210); // jaune clair — imminent
                if (rowColor.isValid()) {
                    for (int c = 0; c < eqTable->columnCount(); ++c)
                        if (eqTable->item(row, c))
                            eqTable->item(row, c)->setBackground(rowColor);
                }
            }
        }

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
    QObject::connect(eqFilters, &QPushButton::clicked, this, [=](){ loadEqTable(); });

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

    QPushButton* eqMore = new QPushButton(st->standardIcon(QStyle::SP_FileDialogContentsView), "  Planning maintenance");
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
    stack->addWidget(equip1);

    // ==========================================================
    // PAGE 15 : Équipements - AJOUT / MODIF (EQUIP_FORM)
    // ==========================================================
    QWidget* equip2 = new QWidget;
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
    eqLeft2L->setSpacing(10);

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

    QToolButton* eqTypeSummary = leftAction("Type d’équipement", QStyle::SP_FileIcon, "—");
    QToolButton* eqFabSummary  = leftAction("Fabricant", QStyle::SP_DirIcon, "—");

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

    QToolButton* eqSalleSummary = colBtn(QStyle::SP_DriveHDIcon, "Salle : —");
    eqLeft2L->addWidget(eqSalleSummary);
    eqLeft2L->addWidget(colBtn(QStyle::SP_FileDialogListView, "Bâtiment"));
    eqLeft2L->addWidget(colBtn(QStyle::SP_ArrowDown, "Étage"));
    eqLeft2L->addStretch(1);

    QFrame* eqRight2 = softBox();
    QVBoxLayout* eqRight2L = new QVBoxLayout(eqRight2);
    eqRight2L->setContentsMargins(12,12,12,12);
    eqRight2L->setSpacing(10);

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
        r->setMinimumHeight(54);
        QHBoxLayout* l = new QHBoxLayout(r);
        l->setContentsMargins(10,8,10,8);
        cb->setMinimumHeight(34);
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

    auto sectionHint = [&](const QString& text){
        QLabel* l = new QLabel(text);
        l->setStyleSheet("color: rgba(0,0,0,0.78); font-weight: 900; font-size: 13px; padding-left: 2px;");
        return l;
    };

    QComboBox* fcb1 = new QComboBox; fcb1->addItems({"PCR Machine","Centrifugeuse","Microscope","Incubateur"});
    QComboBox* fcb2 = new QComboBox; fcb2->addItems({"Thermo Fisher","Eppendorf","Zeiss","Bio-Rad"});
    QComboBox* fcb3 = new QComboBox; fcb3->addItems({"Actif","Hors service","Archivé"});
    fcb1->setEditable(true);
    fcb2->setEditable(true);
    fcb1->setInsertPolicy(QComboBox::NoInsert);
    fcb2->setInsertPolicy(QComboBox::NoInsert);
    fcb1->setCurrentText("");
    fcb2->setCurrentText("");
    if (fcb1->lineEdit()) fcb1->lineEdit()->setPlaceholderText("Type d'équipement");
    if (fcb2->lineEdit()) fcb2->lineEdit()->setPlaceholderText("Fabricant");
    const QRegularExpression lettersOnlyEqRegex(QStringLiteral("^[\\p{L}\\s'\\-]*$"));
    if (fcb1->lineEdit()) {
        fcb1->lineEdit()->setReadOnly(false);
        fcb1->lineEdit()->setValidator(new QRegularExpressionValidator(lettersOnlyEqRegex, fcb1));
    }
    if (fcb2->lineEdit()) {
        fcb2->lineEdit()->setReadOnly(false);
        fcb2->lineEdit()->setValidator(new QRegularExpressionValidator(lettersOnlyEqRegex, fcb2));
    }

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
    intervalCb->addItem("30 jours",  30);
    intervalCb->addItem("3 mois",    90);
    intervalCb->addItem("6 mois",   180);
    intervalCb->addItem("1 an",     365);
    intervalCb->addItem("2 ans",    730);
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

    // ── Auto-calcul : Dernière maintenance + intervalle → prochaine date ──
    auto recalcMaintDate = [=]() {
        const QDate last  = lastMaintDate->date();
        const int   days  = intervalCb->currentData().toInt();
        const QDate next  = last.addDays(days);
        maintDate->setDate(next);

        // Alerte si dépassée ou dans les 7 jours
        const int daysLeft = QDate::currentDate().daysTo(next);
        if (daysLeft <= 0) {
            maintDate->setStyleSheet(
                "QDateEdit{ background:rgba(220,38,38,0.10); border:1.5px solid rgba(220,38,38,0.50);"
                " border-radius:7px; font-weight:900; color:rgba(180,0,0,1); padding:4px 8px;}");
        } else if (daysLeft <= 7) {
            maintDate->setStyleSheet(
                "QDateEdit{ background:rgba(234,179,8,0.10); border:1.5px solid rgba(234,179,8,0.55);"
                " border-radius:7px; font-weight:900; color:rgba(146,112,0,1); padding:4px 8px;}");
        } else {
            maintDate->setStyleSheet(
                "QDateEdit{ background:rgba(10,95,88,0.07); border:1px solid rgba(10,95,88,0.20);"
                " border-radius:7px; font-weight:900; color:rgba(10,95,88,0.85); padding:4px 8px;}");
        }
    };
    QObject::connect(lastMaintDate, &QDateEdit::dateChanged, lastMaintDate, [=](const QDate&){ recalcMaintDate(); });
    QObject::connect(intervalCb, QOverload<int>::of(&QComboBox::currentIndexChanged), intervalCb, [=](int){ recalcMaintDate(); });
    recalcMaintDate();

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

    auto miniRow = [&](QStyle::StandardPixmap sp, const QString& label, QWidget* input){
        QFrame* r = softBox();
        QHBoxLayout* l = new QHBoxLayout(r);
        l->setContentsMargins(10,8,10,8);
        l->setSpacing(8);

        QToolButton* ic = new QToolButton; ic->setAutoRaise(true); ic->setIcon(st->standardIcon(sp));
        QLabel* lab = new QLabel(label);
        lab->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");

        l->addWidget(ic);
        l->addWidget(lab);
        l->addStretch(1);
        l->addWidget(input);
        return r;
    };

    QComboBox* labRoom = new QComboBox; labRoom->addItems({"Lab 101","Lab 102","Lab 103","Lab 201"});
    labRoom->setMinimumWidth(180);
    labRoom->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    labRoom->setEditable(true);
    labRoom->setCurrentText("");
    if (labRoom->lineEdit()) labRoom->lineEdit()->setPlaceholderText("Localisation");
    labRoom->setStyleSheet(
        "QComboBox{ background: rgba(255,255,255,0.96); color: rgba(0,0,0,0.82);"
        " border:1px solid rgba(0,0,0,0.20); border-radius:8px; padding:6px 10px; font-weight:700; }"
        "QComboBox::drop-down{ border:none; width:22px; }"
        "QComboBox QAbstractItemView{ background:white; color:black; selection-background-color: rgba(10,95,88,0.20); }"
    );
    if (labRoom->lineEdit()) {
        labRoom->lineEdit()->setStyleSheet("background: transparent; color: rgba(0,0,0,0.84); border:0; font-weight:700;");
    }

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
    {
        // Item vide = "aucune expérience"
        QListWidgetItem* none = new QListWidgetItem("— Aucune expérience —");
        none->setData(Qt::UserRole, QVariant());
        none->setForeground(QColor(0,0,0,100));
        eqExpCombo->addItem(none);

        QList<ExperienceRecord> expList;
        expCrud->loadExperiences(expList);
        for (const auto& e : expList) {
            QListWidgetItem* it = new QListWidgetItem(
                QString("[%1]  %2").arg(e.id).arg(e.titre));
            it->setData(Qt::UserRole, e.id);
            eqExpCombo->addItem(it);
        }
        eqExpCombo->setCurrentRow(0);
    }

    eqFormGrid->addWidget(fieldBlock("Type d'équipement", comboRow(fcb1)), 0, 0);
    eqFormGrid->addWidget(fieldBlock("Fabricant", comboRow(fcb2)), 0, 1);
    eqFormGrid->addWidget(fieldBlock("Statut", comboRow(fcb3)), 1, 0);
    eqFormGrid->addWidget(fieldBlock("Modèle", modelRow), 1, 1);
    eqFormGrid->addWidget(fieldBlock("Date d'achat", dateRow), 2, 0);
    eqFormGrid->addWidget(fieldBlock("Calibration", calRow), 2, 1);
    eqFormGrid->addWidget(fieldBlock("Dernière maintenance", lastMaintRow), 3, 0);
    eqFormGrid->addWidget(fieldBlock("Intervalle", intervalRow), 3, 1);
    eqFormGrid->addWidget(fieldBlock("Prochaine maintenance (auto)", maintRow), 4, 0, 1, 2);
    eqFormGrid->addWidget(fieldBlock("Salle", miniRow(QStyle::SP_DirIcon, "Salle :", labRoom)), 5, 0);
    eqFormGrid->addWidget(fieldBlock("Expérience liée", eqExpCombo), 6, 0, 1, 2);
    eqFormGrid->setRowStretch(7, 1);

    eqFormScroll->setWidget(eqFormContent);
    eqRight2L->addWidget(eqFormScroll, 1);

    auto syncEqSidebar = [=](){
        const QString typeTxt = fcb1->currentText().trimmed();
        const QString fabTxt  = fcb2->currentText().trimmed();
        const QString locTxt  = labRoom->currentText().trimmed();
        eqTypeSummary->setText("  " + (typeTxt.isEmpty() ? QString("—") : typeTxt));
        eqFabSummary->setText("  " + (fabTxt.isEmpty() ? QString("—") : fabTxt));
        eqSalleSummary->setText("  Salle : " + (locTxt.isEmpty() ? QString("—") : locTxt));
    };

    QObject::connect(fcb1, &QComboBox::currentTextChanged, this, [=](const QString&){ syncEqSidebar(); });
    QObject::connect(fcb2, &QComboBox::currentTextChanged, this, [=](const QString&){ syncEqSidebar(); });
    QObject::connect(labRoom, &QComboBox::currentTextChanged, this, [=](const QString&){ syncEqSidebar(); });
    syncEqSidebar();

    eqOuter2L->addWidget(eqLeft2);
    eqOuter2L->addWidget(eqRight2, 1);
    eq2->addWidget(eqOuter2, 1);

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

    stack->addWidget(equip2);

    // ==========================================================
    // PAGE 16 : Équipements - LOCALISATION (EQUIP_LOC)
    // ==========================================================
    QWidget* equip3 = new QWidget;
    QVBoxLayout* eq3 = new QVBoxLayout(equip3);
    eq3->setContentsMargins(22, 18, 22, 18);
    eq3->setSpacing(14);

    ModulesBar barEquipLoc;
    eq3->addWidget(makeHeaderBlock(st, "Localisation des équipements", ModuleTab::Equipement, &barEquipLoc));
    connectModulesSwitch(this, stack, barEquipLoc);

    QFrame* eqOuter3 = new QFrame;
    eqOuter3->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }").arg(C_PANEL_BG, C_PANEL_BR));
    QHBoxLayout* eqOuter3L = new QHBoxLayout(eqOuter3);
    eqOuter3L->setContentsMargins(12,12,12,12);
    eqOuter3L->setSpacing(12);

    QFrame* eqLeft3 = softBox();
    eqLeft3->setFixedWidth(300);
    QVBoxLayout* eqLeft3L = new QVBoxLayout(eqLeft3);
    eqLeft3L->setContentsMargins(10,10,10,10);
    eqLeft3L->setSpacing(10);

    QFrame* eqDdBox = new QFrame;
    eqDdBox->setStyleSheet("QFrame{ background: rgba(255,255,255,0.72); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QHBoxLayout* eqDdBoxL = new QHBoxLayout(eqDdBox);
    eqDdBoxL->setContentsMargins(10,8,10,8);

    QLabel* eqDdText = new QLabel("Lab 101");
    eqDdText->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    QToolButton* eqDdBtn = new QToolButton;
    eqDdBtn->setAutoRaise(true);
    eqDdBtn->setIcon(st->standardIcon(QStyle::SP_ArrowDown));
    eqDdBtn->setCursor(Qt::PointingHandCursor);

    eqDdBoxL->addWidget(eqDdText);
    eqDdBoxL->addStretch(1);
    eqDdBoxL->addWidget(eqDdBtn);

    QTreeWidget* eqTree = new QTreeWidget;
    eqTree->setHeaderHidden(true);
    eqTree->setIndentation(18);

    auto* tL1 = new QTreeWidgetItem(eqTree, QStringList() << "Lab 101");
    auto* tL2 = new QTreeWidgetItem(eqTree, QStringList() << "Lab 102");
    auto* tL3 = new QTreeWidgetItem(eqTree, QStringList() << "Lab 103");
    auto* tL4 = new QTreeWidgetItem(eqTree, QStringList() << "Lab 201");

    tL1->setIcon(0, st->standardIcon(QStyle::SP_DriveHDIcon));
    tL2->setIcon(0, st->standardIcon(QStyle::SP_DriveHDIcon));
    tL3->setIcon(0, st->standardIcon(QStyle::SP_DriveHDIcon));
    tL4->setIcon(0, st->standardIcon(QStyle::SP_DriveHDIcon));

    auto* tP1 = new QTreeWidgetItem(tL1, QStringList() << "PCR Machines");
    auto* tP2 = new QTreeWidgetItem(tL1, QStringList() << "Microscopes");
    auto* tP3 = new QTreeWidgetItem(tL2, QStringList() << "Centrifugeuses");
    auto* tP4 = new QTreeWidgetItem(tL4, QStringList() << "Incubateurs");

    tP1->setIcon(0, st->standardIcon(QStyle::SP_DirIcon));
    tP2->setIcon(0, st->standardIcon(QStyle::SP_DirIcon));
    tP3->setIcon(0, st->standardIcon(QStyle::SP_DirIcon));
    tP4->setIcon(0, st->standardIcon(QStyle::SP_DirIcon));

    eqTree->expandAll();
    eqTree->setCurrentItem(tL1);

    eqLeft3L->addWidget(eqDdBox);
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

    QPushButton* eqDetails3 = new QPushButton(st->standardIcon(QStyle::SP_FileDialogDetailedView), "  Détails");
    eqDetails3->setCursor(Qt::PointingHandCursor);
    eqDetails3->setStyleSheet(QString(R"(
        QPushButton{
            background:%1; color: rgba(255,255,255,0.95);
            border:1px solid rgba(0,0,0,0.18);
            border-radius: 12px; padding: 10px 16px; font-weight: 900;
        }
        QPushButton:hover{ background: %2; }
    )").arg(C_PRIMARY, C_TOPBAR));

    eqHeader3L->addWidget(eqDetails3);
    eqHeader3L->addStretch(1);
    eqHeader3L->addWidget(eqChip("Total équipements"));
    eqHeader3L->addWidget(eqChip("Maintenance à venir"));

    QFrame* eqListBox3 = new QFrame;
    eqListBox3->setStyleSheet("QFrame{ background: rgba(255,255,255,0.55); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QVBoxLayout* eqListBox3L = new QVBoxLayout(eqListBox3);
    eqListBox3L->setContentsMargins(12,12,12,12);

    QListWidget* eqList3 = new QListWidget;
    eqList3->setSpacing(8);
    eqList3->setSelectionMode(QAbstractItemView::NoSelection);

    auto addEqListRow=[&](QWidget* w){
        QListWidgetItem* it = new QListWidgetItem;
        it->setSizeHint(QSize(10, 40));
        eqList3->addItem(it);
        eqList3->setItemWidget(it, w);
    };

    addEqListRow(new GradientRowWidget(st, "PCR Machine",   "EQ-001", W_GREEN,  QStyle::SP_FileIcon, false));
    addEqListRow(new GradientRowWidget(st, "Centrifugeuse", "EQ-002", W_ORANGE, QStyle::SP_FileIcon, false));
    addEqListRow(new GradientRowWidget(st, "Microscope",    "EQ-003", W_GRAY,   QStyle::SP_FileIcon, false));
    addEqListRow(new GradientRowWidget(st, "Incubateur",    "EQ-004", W_GREEN,  QStyle::SP_FileIcon, false));
    addEqListRow(new GradientRowWidget(st, "PCR Machine",   "EQ-005", W_RED,    QStyle::SP_FileIcon, true));

    eqListBox3L->addWidget(eqList3);

    QWidget* eqBottomInfo3 = new QWidget;
    QHBoxLayout* eqBottomInfo3L = new QHBoxLayout(eqBottomInfo3);
    eqBottomInfo3L->setContentsMargins(0,0,0,0);
    eqBottomInfo3L->setSpacing(12);
    eqBottomInfo3L->addWidget(eqRoomCapacityBlock(st, "Lab 101", "15 unités"));
    eqBottomInfo3L->addWidget(w3BottomLocationBar(st, "Bâtiment A, Étage 1"), 1);

    eqRight3L->addWidget(eqHeader3);
    eqRight3L->addWidget(eqListBox3, 1);
    eqRight3L->addWidget(eqBottomInfo3);

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
    stack->addWidget(equip3);

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
    stack->addWidget(equip4);

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
    emp1->addWidget(makeHeaderBlock(st, "Gestion des Employés", ModuleTab::Employee, &barEmpList));
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

    QComboBox* empRole = new QComboBox; empRole->addItems({"Role", "Chercheur", "Technicien"});
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
    stack->addWidget(empListPage);

    // ==========================================================
    // PAGE 19 : Employés - CREER / MODIFIER (EMP_FORM)
    // ==========================================================
    QWidget* empFormPage = new QWidget;
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

    QFrame* empLeft2 = softBox();
    empLeft2->setFixedWidth(280);
    QVBoxLayout* empLeft2L = new QVBoxLayout(empLeft2);
    empLeft2L->setContentsMargins(10,10,10,10);
    empLeft2L->setSpacing(10);

    {
        QLabel* roleHead = new QLabel("Role");
        roleHead->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
        empLeft2L->addWidget(roleHead);

        QComboBox* empRoleLeft = new QComboBox;
        empRoleLeft->addItems({"Chercheur", "Technicien"});
        empRoleLeft->setCursor(Qt::PointingHandCursor);
        empRoleLeft->setStyleSheet(R"(
            QComboBox{
                background: rgba(255,255,255,0.70);
                border: 1px solid rgba(0,0,0,0.12);
                border-radius: 12px;
                padding: 10px 12px;
                color: rgba(0,0,0,0.60);
                font-weight: 800;
            }
            QComboBox:hover{ background: rgba(255,255,255,0.85); }
            QComboBox::drop-down{ border:0; padding-right:8px; }
        )");
        empLeft2L->addWidget(empRoleLeft);
    }

    QToolButton* empSpecBtn = new QToolButton;
    empSpecBtn->setIcon(st->standardIcon(QStyle::SP_DirIcon));
    empSpecBtn->setText("  Specialisation");
    empSpecBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    empSpecBtn->setCursor(Qt::PointingHandCursor);
    empSpecBtn->setStyleSheet(R"(
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
    empLeft2L->addWidget(empSpecBtn);

    QLabel* empAssignHead = new QLabel("Affectations");
    empAssignHead->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    empLeft2L->addWidget(empAssignHead);

    auto empColBtn = [&](QStyle::StandardPixmap sp, const QString& txt){
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

    empLeft2L->addWidget(empColBtn(QStyle::SP_DriveHDIcon, "Laboratoire"));
    empLeft2L->addWidget(empColBtn(QStyle::SP_FileDialogListView, "Projet"));
    empLeft2L->addWidget(empColBtn(QStyle::SP_ArrowDown, "Temps Plein / Partiel"));
    empLeft2L->addStretch(1);

    QFrame* empRight2 = softBox();
    QVBoxLayout* empRight2L = new QVBoxLayout(empRight2);
    empRight2L->setContentsMargins(12,12,12,12);
    empRight2L->setSpacing(10);

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
        l->setContentsMargins(10,8,10,8);
        l->setSpacing(8);
        l->addWidget(wLeft, 1);
        l->addWidget(wRight, 1);
        return r;
    };

    QLineEdit* empCinEdit = new QLineEdit; empCinEdit->setPlaceholderText("CIN");
    QLineEdit* empNomEdit = new QLineEdit; empNomEdit->setPlaceholderText("Nom");
    empRight2L->addWidget(empComboRow(empCinEdit, empNomEdit));

    QLineEdit* empPrenomEdit = new QLineEdit; empPrenomEdit->setPlaceholderText("Prenom");
    QComboBox* empRoleCb = new QComboBox; empRoleCb->addItems({"Chercheur","Technicien","Responsable"});
    empRight2L->addWidget(empComboRow(empPrenomEdit, empRoleCb));

    QLineEdit* empEmailEdit = new QLineEdit; empEmailEdit->setPlaceholderText("Email (ex: nom@labo.org)");
    QLineEdit* empPwdEdit   = new QLineEdit; empPwdEdit->setPlaceholderText("Mot de passe (création uniquement)");
    empPwdEdit->setEchoMode(QLineEdit::Password);
    empRight2L->addWidget(empComboRow(empEmailEdit, empPwdEdit));

    QComboBox* empSpecCb = new QComboBox; empSpecCb->addItems({"Biomol","Bioinfo","Chimie","General"});
    QLineEdit* empQualifEdit = new QLineEdit; empQualifEdit->setPlaceholderText("Qualification (PhD, MSc...)");
    empRight2L->addWidget(empComboRow(empSpecCb, empQualifEdit));

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

    empOuter2L->addWidget(empLeft2);
    empOuter2L->addWidget(empRight2, 1);
    emp2->addWidget(empOuter2, 1);

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

    stack->addWidget(empFormPage);

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
    affRoleCb->addItems({"Tous", "Chercheur", "Technicien", "Responsable"});
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

    QLabel* affStatusLbl = new QLabel("Sélectionnez un projet à gauche");
    affStatusLbl->setStyleSheet("color:rgba(0,0,0,0.40); font-size:12px; font-weight:700; font-style:italic;");

    affTopBarL->addWidget(affBestBtn);
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
    affInfoBarL->addWidget(mkInfoChip("⊕", "Sélection DB en temps réel"));
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
            // Check duplicate first
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
    QPushButton* tabExpBtn  = new QPushButton("🔬  Expérience");
    tabProjBtn->setCursor(Qt::PointingHandCursor);
    tabExpBtn->setCursor(Qt::PointingHandCursor);
    tabProjBtn->setStyleSheet(modeTabStyle(true));
    tabExpBtn->setStyleSheet(modeTabStyle(false));
    empBottom3L->addWidget(tabProjBtn);
    empBottom3L->addWidget(tabExpBtn);

    QObject::connect(tabProjBtn, &QPushButton::clicked, this, [=](){
        tabProjBtn->setStyleSheet(modeTabStyle(true));
        tabExpBtn->setStyleSheet(modeTabStyle(false));
        stack->setCurrentIndex(EMP_AFF);
    });
    QObject::connect(tabExpBtn, &QPushButton::clicked, this, [=](){
        tabProjBtn->setStyleSheet(modeTabStyle(false));
        tabExpBtn->setStyleSheet(modeTabStyle(true));
        stack->setCurrentIndex(EMP_AFF_EXP);
    });

    emp3->addWidget(empBottom3);
    stack->addWidget(empAffPage);

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
        stack->setCurrentIndex(EMP_AFF_EXP);
    });

    empExpL->addWidget(empExpBottom);

    // Load experiences on page init
    expAffLoadExps();

    // Wire back buttons
    QObject::connect(empBack3b, &QPushButton::clicked, this, [=]{
        setWindowTitle("Gestion des Employés");
        stack->setCurrentIndex(EMP_LIST);
    });

    stack->addWidget(empAffExpPage);

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
    stack->addWidget(empAvailPage);

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
    empS->addWidget(empOuterStats, 1);

    QFrame* empBottomStats = new QFrame;
    empBottomStats->setFixedHeight(64);
    empBottomStats->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* empBottomStatsL = new QHBoxLayout(empBottomStats);
    empBottomStatsL->setContentsMargins(14,10,14,10);
    QPushButton* empBackStats = actionBtn("Retour", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_ArrowBack), true);
    empBottomStatsL->addWidget(empBackStats);
    empBottomStatsL->addStretch(1);
    empS->addWidget(empBottomStats);

    stack->addWidget(empStatsPage);

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
    pubDetailsL->addWidget(pubDetailRow("Citations", pubDetCitations));

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

    stack->addWidget(pubDetailsPage);

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
        sbCitationCount->setValue(0);
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
        sbCitationCount->setValue(pubTable->item(row, 9) ? pubTable->item(row, 9)->text().toInt() : 0);
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
        pubDetCitations->setText(QString::number(publication.citationCount()));
        pubDetAbstract->setText(publication.abstractText().isEmpty() ? "Résumé non renseigné." : publication.abstractText());
        QString qrTarget;
        const QString doi = publication.doi().trimmed();
        if (doi.startsWith("http://", Qt::CaseInsensitive) || doi.startsWith("https://", Qt::CaseInsensitive)) {
            qrTarget = doi;
        } else if (!doi.isEmpty()) {
            qrTarget = QString("https://doi.org/%1").arg(doi);
        } else {
            qrTarget = QString("https://www.biorxiv.org/");
        }
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
            QString("Publication_%1_%2.pdf").arg(QString::number(publication.id()), QDate::currentDate().toString("yyyyMMdd")),
            "PDF Files (*.pdf)");
        if (fileName.isEmpty()) {
            return false;
        }

        QString html;
        html += "<h2 style='color:#0A5F58;'>Publication</h2>";
        html += "<p style='color:#555;'>Export généré le " + QDate::currentDate().toString("dd/MM/yyyy") + "</p>";
        html += "<table border='1' cellpadding='6' cellspacing='0' width='100%' style='border-collapse:collapse;'>";
        html += "<tr style='background:#AFC6C3;'><th>Champ</th><th>Valeur</th></tr>";
        html += "<tr><td>ID</td><td>" + QString::number(publication.id()) + "</td></tr>";
        html += "<tr><td>Titre</td><td>" + publication.titre().toHtmlEscaped() + "</td></tr>";
        html += "<tr><td>Journal / Conf.</td><td>" + publication.journal().toHtmlEscaped() + "</td></tr>";
        html += "<tr><td>Année</td><td>" + QString::number(publication.annee()) + "</td></tr>";
        html += "<tr><td>DOI</td><td>" + publication.doi().toHtmlEscaped() + "</td></tr>";
        html += "<tr><td>Statut</td><td>" + publication.status().toHtmlEscaped() + "</td></tr>";
        html += "<tr><td>Impact Factor</td><td>" + QString::number(publication.impactFactor(), 'f', 2) + "</td></tr>";
        html += "<tr><td>Citations</td><td>" + QString::number(publication.citationCount()) + "</td></tr>";
        html += "<tr><td>Auteur(s)</td><td>" + (pubTable->item(row, 6) ? pubTable->item(row, 6)->text().toHtmlEscaped() : QString("Aucun employé")) + "</td></tr>";
        html += "<tr><td>Résumé</td><td>" + publication.abstractText().toHtmlEscaped().replace("\n", "<br/>") + "</td></tr>";
        html += "</table>";

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
    QVBoxLayout* ep4 = new QVBoxLayout(expDetailsPage);
    ep4->setContentsMargins(22, 18, 22, 18);
    ep4->setSpacing(14);

    ModulesBar barExpDetails;
    ep4->addWidget(makeHeaderBlock(st, "Détails expérience", ModuleTab::ExperiencesProtocoles, &barExpDetails));
    connectModulesSwitch(this, stack, barExpDetails);

    QFrame* expDetailsCard = softBox();
    QVBoxLayout* expDetailsL = new QVBoxLayout(expDetailsCard);
    expDetailsL->setContentsMargins(14,14,14,14);
    expDetailsL->setSpacing(10);

    QLabel* expDetTitle = new QLabel("Expérience");
    QFont expTitleFont = expDetTitle->font();
    expTitleFont.setPointSize(14);
    expTitleFont.setBold(true);
    expDetTitle->setFont(expTitleFont);

    QLabel* expDetProto = nullptr;
    QLabel* expDetResp = nullptr;
    QLabel* expDetDate = nullptr;
    QLabel* expDetStatus = nullptr;
    QLabel* expDetType = nullptr;
    QLabel* expDetDisponibilite = nullptr;
    QLabel* expDetResultat = nullptr;

    auto expDetailRow = [&](const QString& label, QLabel*& valueOut){
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

    expDetailsL->addWidget(expDetTitle);
    expDetailsL->addWidget(expDetailRow("Protocole", expDetProto));
    expDetailsL->addWidget(expDetailRow("Responsable", expDetResp));
    expDetailsL->addWidget(expDetailRow("Date", expDetDate));
    expDetailsL->addWidget(expDetailRow("Statut", expDetStatus));
    expDetailsL->addWidget(expDetailRow("Type_Experience", expDetType));
    expDetailsL->addWidget(expDetailRow("Disponibilité équipement", expDetDisponibilite));
    expDetailsL->addWidget(expDetailRow("Resultat", expDetResultat));

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

    stack->addWidget(expDetailsPage);

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

        expDetTitle->setText(rec.titre);
        expDetProto->setText(rec.hypothese);
        expDetResp->setText(rec.projetId.isNull() ? QString("-") : QString::number(rec.projetId.toInt()));
        expDetDate->setText(QString("%1 -> %2")
                                .arg(rec.dateDebut.isValid() ? rec.dateDebut.toString("dd/MM/yyyy") : "-")
                                .arg(rec.dateFin.isValid() ? rec.dateFin.toString("dd/MM/yyyy") : "-"));
        ExpStatus est = statusFromString(rec.status);
        expDetStatus->setText(expStatusText(est));
        expDetType->setText(rec.typeExperience.isEmpty() ? QString("-") : rec.typeExperience);
        expDetDisponibilite->setText(rec.disponibiliteEquipement.isEmpty() ? QString("-") : rec.disponibiliteEquipement);
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

    stack->addWidget(projDetailsPage);

    // ── Dark / Light mode support for all Gestion Projet pages ──
    {
        auto prevThemeFn = g_applyThemeFn;
        g_applyThemeFn = [=](bool dark) {
            if (prevThemeFn) prevThemeFn(dark);

            const QString bg        = dark ? "#1F2A33"                : C_BG;
            const QString panelBg   = dark ? "rgba(30,42,52,0.92)"    : C_PANEL_BG;
            const QString panelBr   = dark ? "rgba(255,255,255,0.10)" : C_PANEL_BR;
            const QString inputBg   = dark ? "rgba(27,36,45,0.88)"    : "rgba(255,255,255,0.92)";
            const QString textColor = dark ? "#E8EEF2"                 : "rgba(0,0,0,0.88)";
            const QString labelClr  = dark ? "rgba(220,235,240,0.90)" : "#12443B";
            const QString border    = dark ? "rgba(255,255,255,0.16)" : "rgba(0,0,0,0.20)";
            const QString cardBg    = dark ? "rgba(25,35,45,0.92)"    : "rgba(255,255,255,0.20)";
            const QString titleClr  = dark ? "rgba(80,210,190,1.0)"   : "rgba(10,95,88,1.0)";
            const QString statsTitleClr = dark ? "#E8EEF2"             : "rgba(0,0,0,0.75)";
            const QString statsSubClr   = dark ? "rgba(180,200,210,0.70)" : "rgba(0,0,0,0.45)";

            // ── proj2 — Add/Edit form page ───────────────────────
            proj2->setStyleSheet(QString("QWidget { background: %1; }").arg(bg));
            outP2->setStyleSheet(QString(
                "QFrame{ background:%1; border:1px solid %2; border-radius:14px; }")
                .arg(panelBg, panelBr));

            // Section title labels (Informations générales, Planification…)
            for (QLabel* lab : proj2->findChildren<QLabel*>()) {
                const QString s = lab->styleSheet();
                if (s.contains("rgba(10,95,88")) // projTitle labels
                    lab->setStyleSheet(QString(
                        "color:%1; font-weight:900; font-size:14px; padding:2px 0;").arg(titleClr));
                else if (s.contains("rgba(0,0,0,0.80)") || s.contains("rgba(0,0,0,0.55)"))
                    lab->setStyleSheet(QString(
                        "color:%1; font-weight:900; font-size:13px;").arg(labelClr));
                else if (s.contains("rgba(255,165") || s.contains("Sources de financement"))
                    lab->setStyleSheet(s); // keep coloured labels as-is
            }

            // Input fields
            const QString fldStyle = QString(
                "background:%1; border:1.5px solid %2;"
                "border-radius:10px; padding:8px 12px;"
                "color:%3; font-weight:800; font-size:13px;").arg(inputBg, border, textColor);
            const QString cbStyle = QString(
                "background:%1; border:1.5px solid %2;"
                "border-radius:10px; padding:6px 10px;"
                "color:%3; font-weight:800; font-size:13px;"
                "QComboBox::drop-down{border:0px;width:22px;}").arg(inputBg, border, textColor);
            for (QLineEdit* w : proj2->findChildren<QLineEdit*>())
                if (!w->styleSheet().contains("#c0392b")) w->setStyleSheet(fldStyle);
            for (QComboBox* w : proj2->findChildren<QComboBox*>())
                w->setStyleSheet(cbStyle);
            for (QDateEdit* w : proj2->findChildren<QDateEdit*>())
                w->setStyleSheet(QString(
                    "QDateEdit{ background:%1; border:1.5px solid %2;"
                    " border-radius:10px; padding:8px 12px; color:%3; font-weight:800; font-size:13px; }"
                    "QDateEdit::drop-down{ border:0px; width:20px; }"
                    "QDateEdit::up-button{ width:0; } QDateEdit::down-button{ width:0; }")
                    .arg(inputBg, border, textColor));
            for (QSpinBox* w : proj2->findChildren<QSpinBox*>())
                w->setStyleSheet(fldStyle);

            // Bottom bar of form page
            for (QFrame* f : proj2->findChildren<QFrame*>()) {
                if (f->minimumHeight() == 64 || f->maximumHeight() == 64)
                    f->setStyleSheet(QString(
                        "background:%1; border:1px solid rgba(0,0,0,0.08);"
                        "border-radius:14px;").arg(cardBg));
            }

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
                "background:%1; border:1px solid rgba(0,0,0,0.08); border-radius:14px;")
                .arg(cardBg));

            // ── projDetailsPage — Details page ───────────────────
            projDetailsPage->setStyleSheet(QString("QWidget { background: %1; }").arg(bg));
            projDetailsCard->setStyleSheet(QString(
                "QFrame{ background:%1; border:1px solid %2; border-radius:14px; }")
                .arg(panelBg, panelBr));
            projDetailsBottom->setStyleSheet(QString(
                "background:%1; border:1px solid rgba(0,0,0,0.08); border-radius:14px;")
                .arg(cardBg));
            for (QLabel* lab : projDetailsPage->findChildren<QLabel*>()) {
                const QString s = lab->styleSheet();
                if (s.contains("rgba(0,0,0,0.55)") || s.contains("font-weight:900"))
                    lab->setStyleSheet(QString("color:%1; font-weight:900;").arg(labelClr));
                else if (s.contains("rgba(0,0,0,0.70)") || s.contains("font-weight:700"))
                    lab->setStyleSheet(QString("color:%1; font-weight:700;").arg(textColor));
            }
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
    // ==========================================================
    // NAVIGATION BioSimple — CRUD complet
    // ==========================================================

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
        leRef->setReadOnly(true);          // reference cannot change on update
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
        QString ref = leRef->text().trimmed();

        // ① Référence
        if (ref.isEmpty()) { showToast(this, "Référence : Ce champ est obligatoire.", false); return; }
        bool refHasLetter = false, refHasDigit = false;
        for (const QChar& c : ref) {
            if (c.isLetter()) refHasLetter = true;
            if (c.isDigit())  refHasDigit  = true;
        }
        if (!refHasLetter || !refHasDigit) { showToast(this, "Référence : Doit contenir des lettres ET des chiffres.", false); return; }
        if (!*bioEditMode) {
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
        if (cbTemp2->text().trimmed().isEmpty()) { showToast(this, "Température : Ce champ est obligatoire.", false); return; }

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
        s.temperature    = cbTemp2->text().trimmed();
        s.niveauDanger   = cbDanger->currentText();
        s.dateCollecte   = dCollect->date();
        s.dateExpiration = dExpire->date();
        s.idProjet       = cbProjet->currentData().toInt();

        bool ok = false;
        if (*bioEditMode) {
            ok = crud->update(s);
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
            setWindowTitle("Gestion des Échantillons");
            stack->setCurrentIndex(BIO_LIST);
        }
    });

    // ── ANNULER ──
    QObject::connect(cancelBtn, &QPushButton::clicked, this, [=]{
        setWindowTitle("Gestion des Échantillons");
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
        setWindowTitle("Gestion des Échantillons");
        stack->setCurrentIndex(BIO_LIST);
    });
    QObject::connect(back4, &QPushButton::clicked, this, [=]{
        setWindowTitle("Localisation & Stockage");
        stack->setCurrentIndex(BIO_LOC);
    });

    // ── STATISTIQUES — mise à jour depuis la BDD avant affichage ──
    auto updateBioStats = [=]{
        // Palette de couleurs pour les types d'échantillons
        static const QList<QColor> palette = {
            QColor("#9FBEB9"), QColor("#2E6F63"), QColor("#B5672C"),
            QColor("#6366f1"), QColor("#f43f5e"), QColor("#f59e0b"),
            QColor("#06b6d4"), QColor("#84cc16"), QColor("#a855f7")
        };

        // Donut chart : répartition par type (depuis BDD)
        auto typeMap = crud->countByType();
        QList<DonutChart::Slice> slices;
        int ci = 0;
        for (auto it = typeMap.cbegin(); it != typeMap.cend(); ++it, ++ci) {
            QColor col = palette[ci % palette.size()];
            slices.append({(double)it.value(), col, it.key()});
        }
        pie->setData(slices);

        // Légende dynamique : vider (garder titre index 0), puis reconstruire
        while (lgL->count() > 1) {
            QLayoutItem* item = lgL->takeAt(1);
            if (item->widget()) delete item->widget();
            delete item;
        }
        for (const auto& sl : slices) {
            QWidget* row = new QWidget;
            QHBoxLayout* h = new QHBoxLayout(row);
            h->setContentsMargins(0,0,0,0);
            h->setSpacing(10);
            QFrame* dot = new QFrame;
            dot->setFixedSize(12,12);
            dot->setStyleSheet(QString("background:%1; border-radius:6px;").arg(sl.color.name()));
            QLabel* lab = new QLabel(sl.label);
            lab->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
            h->addWidget(dot);
            h->addWidget(lab);
            h->addStretch(1);
            lgL->addWidget(row);
        }
        lgL->addStretch(1);

        // Bar chart : échantillons par mois (depuis BDD)
        auto monthVec = crud->countByMonth();
        QList<BarChart::Bar> barList;
        for (auto& p : monthVec)
            barList.append({(double)p.first, p.second});
        if (!barList.isEmpty())
            bars->setData(barList);
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
        setWindowTitle("Gestion des Échantillons");
        stack->setCurrentIndex(BIO_LIST);
    });

    // ==========================================================
    // NAVIGATION Gestion Projet (3 widgets) — CRUD complet
    // ==========================================================
    // Helper: clear funding source fields
    auto clearSourceRows = [=](){
        projFinancement->clear();
        projFinancement->setStyleSheet(fldOk);
        projBudgetSpin->setCurrentRow(0);
    };

    auto clearProjForm = [=](){
        *projEditMode = false;
        *projEditId = 0;
        projName->clear();
        projName->setStyleSheet(fldOk);
        errProjName->hide();
        projDomainEdit->setCurrentIndex(0);
        projDomainEdit->setStyleSheet(fldOkCb);
        errProjDomain->hide();
        projStatus->setCurrentRow(0); // default = Planifié
        projEthique->clear();
        projEthique->setStyleSheet(fldOk);
        errProjEthique->hide();
        projStart->setDate(QDate::currentDate());
        projStart->setStyleSheet(fldOk);
        errProjStart->hide();
        projEnd->setDate(QDate::currentDate().addMonths(3));
        projEnd->setStyleSheet(fldOk);
        errProjEnd->hide();
        projPubsEdit->setValue(0);
        clearSourceRows();
    };

    QObject::connect(projAdd, &QPushButton::clicked, this, [=](){
        clearProjForm();
        setWindowTitle("Ajouter un projet");
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
        projName->setStyleSheet(fldOk);
        errProjName->hide();

        { int di = projDomainEdit->findText(rec.domaineDeRecherche, Qt::MatchFixedString);
            projDomainEdit->setCurrentIndex(di >= 0 ? di : 0); }
        projDomainEdit->setStyleSheet(fldOkCb);
        errProjDomain->hide();

        projEthique->setText(rec.numeroDApprobationEthique);
        projEthique->setStyleSheet(fldOk);
        errProjEthique->hide();

        projStart->setDate(rec.dateDeDebut.isValid() ? rec.dateDeDebut : QDate::currentDate());
        projStart->setStyleSheet(fldOk);
        errProjStart->hide();
        projEnd->setDate(rec.dateDeFin.isValid() ? rec.dateDeFin : QDate::currentDate().addMonths(3));
        projEnd->setStyleSheet(fldOk);
        errProjEnd->hide();

        projPubsEdit->setValue(rec.nombreDePublications);

        {
            QString statTxt = rec.statut.trimmed();
            projStatus->setCurrentRow(0);
            for (int i = 0; i < projStatus->count(); ++i) {
                if (projStatus->item(i)->text().compare(statTxt, Qt::CaseInsensitive) == 0) {
                    projStatus->setCurrentRow(i);
                    break;
                }
            }
        }

        // Load single funding source
        projFinancement->setText(rec.sourceDeFinancement);
        projFinancement->setStyleSheet(fldOk);
        // Select nearest budget range
        {
            const double bv = rec.budget;
            int bestRow = 0;
            if (bv >= 5000000.0)     bestRow = 3;
            else if (bv >= 500000.0) bestRow = 2;
            else if (bv >= 50000.0)  bestRow = 1;
            else                     bestRow = 0;
            projBudgetSpin->setCurrentRow(bestRow);
        }

        setWindowTitle("Modifier un projet");
        stack->setCurrentIndex(PROJ_FORM);
    });
    QObject::connect(projCancel, &QPushButton::clicked, this, [=](){
        clearProjForm();
        setWindowTitle("Gestion Projet");
        stack->setCurrentIndex(PROJ_LIST);
    });
    QObject::connect(projSave, &QPushButton::clicked, this, [=](){
        ProjetRecord rec;
        rec.idProjet = *projEditMode ? *projEditId : 0;
        rec.nomDuProjet = projName->text().trimmed();
        rec.domaineDeRecherche = projDomainEdit->currentText().startsWith("—") ? QString() : projDomainEdit->currentText();
        rec.dateDeDebut = projStart->date();
        rec.dateDeFin   = projEnd->date();
        rec.budget = projBudgetSpin->currentItem() ? projBudgetSpin->currentItem()->data(Qt::UserRole).toDouble() : 500.0;
        rec.statut = projStatus->currentItem() ? projStatus->currentItem()->text() : QString("Planifié");
        rec.sourceDeFinancement = projFinancement->text().trimmed();
        rec.numeroDApprobationEthique = projEthique->text().trimmed();
        rec.nombreDePublications = projPubsEdit->value();

        QString err;
        const bool ok = *projEditMode ? projCrud->updateProjet(rec, &err)
                                      : projCrud->insertProjet(rec, &err);
        if (!ok) {
            // Show the error as a toast AND highlight the relevant field if possible
            showToast(this, err, false);
            // Highlight fields based on error keywords
            if (err.contains("nom", Qt::CaseInsensitive))
            { projName->setStyleSheet(fldErr); errProjName->setText("⚠  " + err); errProjName->show(); }
            else if (err.contains("domaine", Qt::CaseInsensitive))
            { projDomainEdit->setStyleSheet(fldErrCb); errProjDomain->setText("⚠  " + err); errProjDomain->show(); }
            else if (err.contains("éthique", Qt::CaseInsensitive) || err.contains("ethique", Qt::CaseInsensitive))
            { projEthique->setStyleSheet(fldErr); errProjEthique->setText("⚠  " + err); errProjEthique->show(); }
            else if (err.contains("début", Qt::CaseInsensitive) || err.contains("debut", Qt::CaseInsensitive))
            { projStart->setStyleSheet(fldErr); errProjStart->setText("⚠  " + err); errProjStart->show(); }
            else if (err.contains("fin", Qt::CaseInsensitive) || err.contains("durée", Qt::CaseInsensitive))
            { projEnd->setStyleSheet(fldErr); errProjEnd->setText("⚠  " + err); errProjEnd->show(); }
            return;
        }

        clearProjForm();
        loadProjTable();
        applyProjFilters();

        setWindowTitle("Gestion Projet");
        stack->setCurrentIndex(PROJ_LIST);
        showToast(this, "Projet enregistré.", true);
    });
    QObject::connect(projDetails, &QPushButton::clicked, this, [=](){
        if (!updateProjDetailsFromRow()) return;
        setWindowTitle("Détails projet");
        stack->setCurrentIndex(PROJ_DETAILS);
    });
    QObject::connect(p3Back, &QPushButton::clicked, this, [=](){
        setWindowTitle("Gestion Projet");
        stack->setCurrentIndex(PROJ_LIST);
    });

    // ── Répartition des projets par domaine (gestproj stat) ──
    QObject::connect(btnDomaineProj, &QPushButton::clicked, this, [=](){
        GestProjCrud::showDomaineChart(this);
    });
    QObject::connect(btnBudgetProj, &QPushButton::clicked, this, [=](){
        GestProjCrud::showBudgetChart(this);
    });
    QObject::connect(btnStatutProj, &QPushButton::clicked, this, [=](){
        GestProjCrud::showStatutChart(this);
    });
    QObject::connect(btnDomaineBudgetProj, &QPushButton::clicked, this, [=](){
        GestProjCrud::showDomaineBudgetChart(this);
    });
    QObject::connect(projDetailsBack, &QPushButton::clicked, this, [=](){
        setWindowTitle("Gestion Projet");
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
        if (!slices.isEmpty()) pd->setData(slices);
        setWindowTitle("Statistiques Projet");
        stack->setCurrentIndex(PROJ_STATS);
    });

    QObject::connect(projExportPdf, &QToolButton::clicked, this, [=](){
        QString fileName = QFileDialog::getSaveFileName(this, "Exporter PDF", "projets.pdf", "PDF Files (*.pdf)");
        if (fileName.isEmpty()) return;
        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(fileName);
        QTextDocument doc;
        QString html = "<h2 style='color:#0A5F58;'>Gestion des Projets de Recherche</h2>"
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
        eName->clear(); eHypo->clear(); eTypeExp->clear(); eResultat->clear();
        eDateDebut->setDate(QDate::currentDate());
        eDateFin->setDate(QDate::currentDate().addDays(1));
        eStatus->setCurrentIndex(0);
        eProjet->setCurrentIndex(0);
        {
            QString err;
            const QString suggested = expCrud->suggestedEquipAvailability(&err);
            int idx = eDisponibilite->findText(suggested);
            eDisponibilite->setCurrentIndex(idx >= 0 ? idx : 0);
        }
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
        if      (rec.status == "En cours")   eStatus->setCurrentIndex(1);
        else if (rec.status == "Concluante") eStatus->setCurrentIndex(2);
        else if (rec.status == "Réussie")    eStatus->setCurrentIndex(3);
        else if (rec.status == "Échouée")    eStatus->setCurrentIndex(4);
        else if (rec.status == "Archivée")   eStatus->setCurrentIndex(5);
        else                                 eStatus->setCurrentIndex(0);
        {
            int idx = eDisponibilite->findText(rec.disponibiliteEquipement);
            if (idx < 0) {
                QString err;
                idx = eDisponibilite->findText(expCrud->suggestedEquipAvailability(&err));
            }
            eDisponibilite->setCurrentIndex(idx >= 0 ? idx : 0);
        }
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

        const QDate today = QDate::currentDate();
        const QDate dStart = eDateDebut->date();
        const QDate dEnd = eDateFin->date();
        if (dStart <= today) {
            showToast(this, "La date debut doit etre superieure a la date actuelle.", false); return;
        }
        if (dStart >= dEnd) {
            showToast(this, "La date debut doit etre inferieure a la date fin.", false); return;
        }
        ExperienceRecord rec;
        rec.id        = *expEditId;
        rec.titre     = nameTxt;
        rec.hypothese = hypoTxt;
        rec.dateDebut = dStart;
        rec.dateFin   = dEnd;
        rec.status    = eStatus->currentText();
        rec.typeExperience = typeTxt;
        rec.resultat = resultatTxt;
        rec.disponibiliteEquipement = eDisponibilite->currentText();
        rec.projetId  = eProjet->currentData().isNull() ? QVariant() : QVariant(eProjet->currentData().toInt());
        QString err;
        bool ok = *expEditMode ? expCrud->updateExperience(rec, &err) : expCrud->insertExperience(rec, &err);
        if (!ok) { showToast(this, "Erreur : " + err, false); return; }
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
        fcb1->setCurrentText("");
        fcb2->setCurrentText("");
        fcb3->setCurrentIndex(0);
        modelEdit->clear();
        date->setDate(QDate::currentDate());
        lastMaintDate->setDate(QDate::currentDate());
        intervalCb->setCurrentIndex(0); // 30 jours par défaut
        recalcMaintDate();
        calDate->setDate(QDate::currentDate());
        labRoom->setCurrentText("");
        if (eqExpCombo->count() > 0) eqExpCombo->setCurrentRow(0);
        const QString typeTxt = fcb1->currentText().trimmed();
        const QString fabTxt  = fcb2->currentText().trimmed();
        const QString locTxt  = labRoom->currentText().trimmed();
        eqTypeSummary->setText("  " + (typeTxt.isEmpty() ? QString("—") : typeTxt));
        eqFabSummary->setText("  " + (fabTxt.isEmpty() ? QString("—") : fabTxt));
        eqSalleSummary->setText("  Salle : " + (locTxt.isEmpty() ? QString("—") : locTxt));
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

    auto updateEquipDetailsFromSelection = [=]() -> bool {
        const int id = selectedEquipementId();
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

        *eqEditMode = true;
        *eqEditId = id;
        fcb1->setCurrentText(rec.nomEquipement);
        fcb2->setCurrentText(rec.fabricant);
        fcb3->setCurrentText(rec.statut);
        modelEdit->setText(rec.numeroModele);
        if (rec.dateAchat.isValid()) date->setDate(rec.dateAchat);
        if (rec.dateDerniereMaintenance.isValid()) lastMaintDate->setDate(rec.dateDerniereMaintenance);
        if (rec.dateProchaineMaintenance.isValid()) maintDate->setDate(rec.dateProchaineMaintenance);
        if (rec.dateLimiteCalibration.isValid()) calDate->setDate(rec.dateLimiteCalibration);
        if (!rec.localisation.trimmed().isEmpty()) labRoom->setCurrentText(rec.localisation);
        if (!rec.idExp.isNull()) {
            for (int i = 0; i < eqExpCombo->count(); ++i) {
                if (eqExpCombo->item(i)->data(Qt::UserRole).toInt() == rec.idExp.toInt()) {
                    eqExpCombo->setCurrentRow(i);
                    break;
                }
            }
        }
        eqTypeSummary->setText("  " + (rec.nomEquipement.trimmed().isEmpty() ? QString("—") : rec.nomEquipement.trimmed()));
        eqFabSummary->setText("  " + (rec.fabricant.trimmed().isEmpty() ? QString("—") : rec.fabricant.trimmed()));
        eqSalleSummary->setText("  Salle : " + (rec.localisation.trimmed().isEmpty() ? QString("—") : rec.localisation.trimmed()));

        setWindowTitle("Ajouter / Modifier un équipement");
        stack->setCurrentIndex(EQUIP_FORM);
    });

    QObject::connect(eqTable, &QTableWidget::cellDoubleClicked, this, [=](int, int){
        eqEdit->click();
    });

    QObject::connect(eqCancel, &QPushButton::clicked, this, [=](){
        clearEquipForm();
        setWindowTitle("Gestion des Équipements");
        stack->setCurrentIndex(EQUIP_LIST);
    });

    QObject::connect(eqSave, &QPushButton::clicked, this, [=](){
        const QString equipType = fcb1->currentText().trimmed();
        const QString fabricant = fcb2->currentText().trimmed();
        const QRegularExpression lettersOnlyStrict(QStringLiteral("^[\\p{L}\\s'\\-]+$"));

        if (equipType.isEmpty()) {
            showToast(this, "Le nom de l'équipement est obligatoire.", false);
            return;
        }

        if (!lettersOnlyStrict.match(equipType).hasMatch()) {
            showToast(this, "Le type d'équipement doit contenir uniquement des lettres.", false);
            return;
        }

        if (!fabricant.isEmpty() && !lettersOnlyStrict.match(fabricant).hasMatch()) {
            showToast(this, "Le fabricant doit contenir uniquement des lettres.", false);
            return;
        }

        // Pas de blocage sur la date de maintenance — elle peut être dans le passé (retard réel)

        EquipementRecord rec;
        rec.id = *eqEditId;
        rec.nomEquipement = equipType;
        rec.fabricant = fabricant;
        rec.numeroModele = modelEdit->text().trimmed();
        rec.dateAchat = date->date();
        rec.dateDerniereMaintenance = lastMaintDate->date();
        rec.dateProchaineMaintenance = maintDate->date(); // calculée automatiquement
        rec.statut = fcb3->currentText();
        rec.localisation = labRoom->currentText().trimmed();
        rec.dateLimiteCalibration = calDate->date();
        {
            QListWidgetItem* selIt = eqExpCombo->currentItem();
            rec.idExp = (selIt && selIt->data(Qt::UserRole).isValid())
                        ? QVariant(selIt->data(Qt::UserRole).toInt())
                        : QVariant(QMetaType::fromType<int>());
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
        setWindowTitle("Gestion des Équipements");
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
        setWindowTitle("Localisation des équipements");
        stack->setCurrentIndex(EQUIP_LOC);
    });
    QObject::connect(eqBack3, &QPushButton::clicked, this, [=](){
        setWindowTitle("Gestion des Équipements");
        stack->setCurrentIndex(EQUIP_LIST);
    });
    QObject::connect(eqDetails3, &QPushButton::clicked, this, [=](){
        if (!updateEquipDetailsFromSelection()) return;
        setWindowTitle("Détails équipement");
        stack->setCurrentIndex(EQUIP_DETAILS);
    });
    QObject::connect(eqBack4, &QPushButton::clicked, this, [=](){
        setWindowTitle("Gestion des Équipements");
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
                fcb1->setCurrentText(rec.nomEquipement);
                fcb2->setCurrentText(rec.fabricant);
                fcb3->setCurrentText(rec.statut);
                modelEdit->setText(rec.numeroModele);
                if (rec.dateAchat.isValid()) date->setDate(rec.dateAchat);
                if (rec.dateProchaineMaintenance.isValid()) maintDate->setDate(rec.dateProchaineMaintenance);
                if (rec.dateLimiteCalibration.isValid()) calDate->setDate(rec.dateLimiteCalibration);
                if (!rec.localisation.trimmed().isEmpty()) labRoom->setCurrentText(rec.localisation);
                if (!rec.idExp.isNull()) {
                    for (int i = 0; i < eqExpCombo->count(); ++i) {
                        if (eqExpCombo->item(i)->data(Qt::UserRole).toInt() == rec.idExp.toInt()) {
                            eqExpCombo->setCurrentRow(i);
                            break;
                        }
                    }
                }
                eqTypeSummary->setText("  " + (rec.nomEquipement.trimmed().isEmpty() ? QString("—") : rec.nomEquipement.trimmed()));
                eqFabSummary->setText("  " + (rec.fabricant.trimmed().isEmpty() ? QString("—") : rec.fabricant.trimmed()));
                eqSalleSummary->setText("  Salle : " + (rec.localisation.trimmed().isEmpty() ? QString("—") : rec.localisation.trimmed()));
            }
        }
        setWindowTitle("Ajouter / Modifier un équipement");
        stack->setCurrentIndex(EQUIP_FORM);
    });

    // ==========================================================
    // NAVIGATION Employés (5 widgets)
    // ==========================================================
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
        setWindowTitle("Gestion des Employés");
        stack->setCurrentIndex(EMP_LIST);
    });
    QObject::connect(empSave, &QPushButton::clicked, this, [=](){
        if (empCinEdit->text().trimmed().isEmpty() || empNomEdit->text().trimmed().isEmpty() || empPrenomEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "Validation", "CIN, Nom et Prenom sont obligatoires.");
            return;
        }

        EmployeRecord rec;
        rec.employeeId = *empEditMode ? *empEditId : 0;
        rec.cin = empCinEdit->text().trimmed();
        rec.nom = empNomEdit->text().trimmed();
        rec.prenom = empPrenomEdit->text().trimmed();
        rec.email = empEmailEdit->text().trimmed();
        rec.password = empPwdEdit->text().trimmed();
        rec.role = empRoleCb->currentText();
        rec.specialization = empSpecCb->currentText();
        rec.qualification = empQualifEdit->text().trimmed();
        rec.nbPublications = empPubs->value();
        rec.tempsTravail = empFtCb->currentText();
        rec.laboratoire = empLabCb->currentText();

        QString err;
        const bool ok = *empEditMode ? empCrud->updateEmploye(rec, &err)
                                     : empCrud->insertEmploye(rec, &err);
        if (!ok) {
            showToast(this, "Erreur : " + err, false);
            return;
        }

        clearEmpForm();
        loadEmpTable();
        applyEmpFilters();
        updateEmpStatsFromTable();

        setWindowTitle("Gestion des Employés");
        stack->setCurrentIndex(EMP_LIST);
        showToast(this, "Employe enregistre.", true);
    });
    QObject::connect(empMore, &QPushButton::clicked, this, [=](){
        affLoadProjects();
        setWindowTitle("Affectation Intelligente");
        stack->setCurrentIndex(EMP_AFF);
    });
    QObject::connect(empBack3, &QPushButton::clicked, this, [=](){
        setWindowTitle("Gestion des Employés");
        stack->setCurrentIndex(EMP_LIST);
    });
    QObject::connect(empBack4, &QPushButton::clicked, this, [=](){
        setWindowTitle("Gestion des Employés");
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
        setWindowTitle("Gestion des Employés");
        stack->setCurrentIndex(EMP_LIST);
    });
    QObject::connect(empDel, &QPushButton::clicked, this, [=](){
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
            sbCitationCount->value()
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
            stack->setCurrentIndex(0);
        }
    });

}
