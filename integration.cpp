// ===================== biosimple.cpp (UN SEUL FICHIER - BioSimple + Gestion Projet - 3 Widgets) =====================
#include "integration.h"
#include "crudebiosimple.h"
#include "publication.h"
#include "chatbotbiosimple.h"
#include "crudexperience.h"
#include "equipement.h"
#include "cong.h"
#include "signupserver.h"
#include <QTextEdit>

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
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QGraphicsColorizeEffect>
#include <QEasingCurve>
#include <QStackedLayout>
#include <QUrl>

#include <cmath>
#include <algorithm>

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
static const int EMP_LIST  = 19; // Page 1 : Liste / Gestion
static const int EMP_FORM  = 20; // Page 2 : Créer / Modifier
static const int EMP_AFF   = 21; // Page 3 : Affectations & Labs
static const int EMP_AVAIL = 22; // Page 4 : Disponibilités
static const int EMP_STATS = 23; // Page 5 : Statistiques

// Détails (pages additionnelles)
static const int PUB_DETAILS  = 24;
static const int EXP_DETAILS  = 25;
static const int PROJ_DETAILS = 26;

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

static void connectModulesSwitch(MainWindow* self, QStackedWidget* stack, const ModulesBar& mb)
{
    if (!g_globalBar) return;
    const ModulesBar& globalBar = *g_globalBar;

    // Helper: uncheck all buttons except the clicked one (in both local and global bars)
    auto uncheckOthers = [=](QPushButton* btnClicked, ModuleTab selectedTab){
        // Update global bar
        if(globalBar.bEmployee != btnClicked) globalBar.bEmployee->setChecked(false);
        if(globalBar.bPublication != btnClicked) globalBar.bPublication->setChecked(false);
        if(globalBar.bBioSimple != btnClicked) globalBar.bBioSimple->setChecked(false);
        if(globalBar.bEquipement != btnClicked) globalBar.bEquipement->setChecked(false);
        if(globalBar.bExp != btnClicked) globalBar.bExp->setChecked(false);
        if(globalBar.bProjet != btnClicked) globalBar.bProjet->setChecked(false);
        btnClicked->setChecked(true);

        // Update local bar
        if(mb.bEmployee != btnClicked) mb.bEmployee->setChecked(false);
        if(mb.bPublication != btnClicked) mb.bPublication->setChecked(false);
        if(mb.bBioSimple != btnClicked) mb.bBioSimple->setChecked(false);
        if(mb.bEquipement != btnClicked) mb.bEquipement->setChecked(false);
        if(mb.bExp != btnClicked) mb.bExp->setChecked(false);
        if(mb.bProjet != btnClicked) mb.bProjet->setChecked(false);

        // Find and check the corresponding button in local bar
        if(selectedTab == ModuleTab::BioSimple) mb.bBioSimple->setChecked(true);
        else if(selectedTab == ModuleTab::Equipement) mb.bEquipement->setChecked(true);
        else if(selectedTab == ModuleTab::ExperiencesProtocoles) mb.bExp->setChecked(true);
        else if(selectedTab == ModuleTab::GestionProjet) mb.bProjet->setChecked(true);
        else if(selectedTab == ModuleTab::Employee) mb.bEmployee->setChecked(true);
        else if(selectedTab == ModuleTab::Publication) mb.bPublication->setChecked(true);
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
        stack->setCurrentIndex(EMP_LIST);
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

    rack->setStyleSheet(R"(
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

    t->setStyleSheet(QString(R"(
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

    grid->setStyleSheet(R"(
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

    t->setStyleSheet(QString(R"(
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
        cardLay->addLayout(passRow);

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
    QPushButton* getCreateAccountButton() const { return createBtn; }
    QString getEmail() const { return emailEdit->text().trimmed(); }
    QString getPassword() const { return passEdit->text(); }
    bool isRemembered() const { return rememberCheck->isChecked(); }

    void clearFields() {
        emailEdit->clear();
        passEdit->clear();
        rememberCheck->setChecked(false);
    }

    // Public members for styling
    QLineEdit *emailEdit = nullptr;
    QLineEdit *passEdit = nullptr;

protected:
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

    // Components
    QLabel *bgLabel = nullptr;
    QWidget *overlay = nullptr;
    QLabel *logoLabel = nullptr;
    QLabel *titleLabel = nullptr;
    QLabel *subtitleLabel = nullptr;
    QPushButton *showPassBtn = nullptr;
    QCheckBox *rememberCheck = nullptr;
    QPushButton *loginBtn = nullptr;
    QPushButton *forgotBtn = nullptr;
    QPushButton *createBtn = nullptr;
    bool passwordVisible = false;
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

    root->setStyleSheet(QString(R"(
        #root { background:%1; }
        QLabel { color: %2; }
        QLineEdit {
            background: rgba(255,255,255,0.65);
            border: 1px solid rgba(0,0,0,0.15);
            border-radius: 12px;
            padding: 10px 14px;
            color: %2;
        }
        QComboBox{
            background: rgba(255,255,255,0.45);
            border: 1px solid rgba(0,0,0,0.12);
            border-radius: 10px;
            padding: 8px 12px;
            color: %2;
            min-width: 92px;
            font-weight: 800;
        }
        QComboBox::drop-down{ border: 0px; width: 22px; }

        QHeaderView::section {
            background: %3;
            color: rgba(0,0,0,0.60);
            border: none;
            padding: 10px 10px;
            font-weight: 800;
        }
        QTableWidget{
            background: transparent;
            border: none;
            gridline-color: rgba(0,0,0,0.10);
            selection-background-color: rgba(10,95,88,0.18);
            selection-color: %2;
        }
        QTableWidget::item{ padding: 10px; color: rgba(0,0,0,0.70); }

        QTreeWidget{ background: transparent; border: none; }
        QTreeWidget::item{
            padding: 7px; margin: 2px 4px; color: rgba(0,0,0,0.55);
            font-weight: 900; border-radius: 10px;
        }
        QTreeWidget::item:selected{ background: rgba(10,95,88,0.20); }
        QListWidget{ background: transparent; border:none; }
    )").arg(C_BG, C_TEXT_DARK, C_TABLE_HDR));

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
    ModulesBar globalBar = makeModulesBar(ModuleTab::Employee);
    g_globalBar = &globalBar;

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

    // Handle login button
    QObject::connect(loginPage->getLoginButton(), &QPushButton::clicked, this, [=](){
        const QString email = loginPage->getEmail();
        const QString pass = loginPage->getPassword();

        if (email.isEmpty() || pass.isEmpty()) {
            // Simple validation feedback
            loginPage->emailEdit->setPlaceholderText("Champ requis!");
            loginPage->passEdit->setPlaceholderText("Champ requis!");
            return;
        }

        QSqlDatabase db = QSqlDatabase::database();
        if (!db.isOpen()) {
            QMessageBox::critical(this, "Connexion BD", "La base de données n'est pas connectée.");
            return;
        }

        QSqlQuery authQuery(db);
        authQuery.prepare("SELECT COUNT(*) FROM APP_USERS "
                          "WHERE LOWER(EMAIL) = LOWER(?) "
                          "AND USER_PASSWORD = ? "
                          "AND ACTIVE = 'O'");
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

        // Clear field (show success visually)
        loginPage->clearFields();

        // Show beautiful success dialog
        SuccessDialog successDlg("Connexion réussie !",
                                  "Bienvenue sur SmartVision",
                                  this);
        successDlg.exec();

        // Go to employee module
        setWindowTitle("Gestion des Employés");
        stack->setCurrentIndex(EMP_LIST);
    });

    // Handle logout button
    QObject::connect(globalBar.bLogout, &QPushButton::clicked, this, [=](){
        setWindowTitle("SmartVision - Connexion");
        stack->setCurrentIndex(LOGIN);
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

    QPushButton* chatbotBtn = new QPushButton(st->standardIcon(QStyle::SP_MessageBoxInformation), "  Chatbot");
    chatbotBtn->setCursor(Qt::PointingHandCursor);
    chatbotBtn->setStyleSheet(QString(R"(
        QPushButton{
            background:%1; color: rgba(255,255,255,0.92);
            border:1px solid rgba(0,0,0,0.18);
            border-radius: 12px; padding: 10px 16px; font-weight: 800;
        }
        QPushButton:hover{ background: %2; }
    )").arg(C_PRIMARY, C_TOPBAR));

    bar1L->addWidget(search, 1);
    bar1L->addWidget(chatbotBtn);
    p1->addWidget(bar1);

    // ── Chatbot popup ──
    QObject::connect(chatbotBtn, &QPushButton::clicked, this, [=]{
        ChatBotBioSimple* bot = new ChatBotBioSimple(this);
        // Center near the button
        QPoint center = this->geometry().center();
        bot->move(center.x() - bot->width()/2, center.y() - bot->height()/2);
        bot->exec();
    });

    QFrame* card1 = makeCard();
    QVBoxLayout* card1L = new QVBoxLayout(card1);
    card1L->setContentsMargins(10,10,10,10);

    QTableWidget* table = new QTableWidget(0, 10);
    table->setHorizontalHeaderLabels({"Référence", "Emplacement de stockage", "Type", "Organisme", "Température", "Quantité restante", "Date de collecte", "Date d'expiration", "Statut", "Niveau de dangerosité"});
    table->verticalHeader()->setVisible(false);
    table->setShowGrid(true);
    table->setAlternatingRowColors(true);
    table->setStyleSheet(QString("QTableWidget{ alternate-background-color:%1; background-color:%2; }").arg(C_ROW_EVEN, C_ROW_ODD));
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
    left2L->addWidget(formRow(QStyle::SP_FileIcon, "Référence", leRef));

    // collection field removed per latest request

    left2L->addWidget(sectionTitle("Dates"));
    // keep pointers to the date edits so the lookup lambda can modify them
    QDateEdit *dCollect = nullptr, *dExpire = nullptr;
    left2L->addWidget(blueDateRow("Date de collecte", QDate::currentDate(), dCollect));
    left2L->addWidget(redDateRow("Date d'expiration", QDate::currentDate().addDays(30), dExpire));

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

    QLineEdit* cbType2 = new QLineEdit;
    cbType2->setPlaceholderText("Type (ex: DNA, RNA, Protéine...)");
    cbType2->setFixedWidth(200);
    right2L->addWidget(formRow(QStyle::SP_FileIcon, "Type", cbType2));

    QLineEdit* cbOrg2 = new QLineEdit;
    cbOrg2->setPlaceholderText("Organisme");
    cbOrg2->setFixedWidth(200);
    right2L->addWidget(formRow(QStyle::SP_DirIcon, "Organisme", cbOrg2));

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
    outer3L->setContentsMargins(12,12,12,12);
    outer3L->setSpacing(12);

    QFrame* left3 = softBox();
    left3->setFixedWidth(320);
    QVBoxLayout* left3L = new QVBoxLayout(left3);
    left3L->setContentsMargins(10,10,10,10);
    left3L->setSpacing(10);

    QFrame* ddBox = new QFrame;
    ddBox->setStyleSheet("QFrame{ background: rgba(255,255,255,0.72); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QHBoxLayout* ddBoxL = new QHBoxLayout(ddBox);
    ddBoxL->setContentsMargins(10,8,10,8);

    QLabel* ddText = new QLabel("Congélateur 01");
    ddText->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    QToolButton* ddBtn = new QToolButton;
    ddBtn->setAutoRaise(true);
    ddBtn->setIcon(st->standardIcon(QStyle::SP_ArrowDown));
    ddBtn->setCursor(Qt::PointingHandCursor);

    ddBoxL->addWidget(ddText);
    ddBoxL->addStretch(1);
    ddBoxL->addWidget(ddBtn);

    QTreeWidget* tree3 = new QTreeWidget;
    tree3->setHeaderHidden(true);
    tree3->setIndentation(18);

    auto* tF1 = new QTreeWidgetItem(tree3, QStringList() << "Congélateur 01");
    auto* tF2 = new QTreeWidgetItem(tree3, QStringList() << "Congélateur 02");
    auto* tF3 = new QTreeWidgetItem(tree3, QStringList() << "Congélateur 03");

    tF1->setIcon(0, st->standardIcon(QStyle::SP_DriveHDIcon));
    tF2->setIcon(0, st->standardIcon(QStyle::SP_DriveHDIcon));
    tF3->setIcon(0, st->standardIcon(QStyle::SP_DriveHDIcon));

    auto* tA = new QTreeWidgetItem(tF2, QStringList() << "Étagère A");
    auto* t6 = new QTreeWidgetItem(tF2, QStringList() << "Étagère 6");
    auto* tB = new QTreeWidgetItem(tF2, QStringList() << "Étagère B");
    auto* tR = new QTreeWidgetItem(tF2, QStringList() << "Temp. ambiante");
    tA->setIcon(0, st->standardIcon(QStyle::SP_DirIcon));
    t6->setIcon(0, st->standardIcon(QStyle::SP_DirIcon));
    tB->setIcon(0, st->standardIcon(QStyle::SP_DirIcon));
    tR->setIcon(0, st->standardIcon(QStyle::SP_FileDialogInfoView));

    tree3->expandAll();
    tree3->setCurrentItem(tF2);

    QPushButton* exportReport3 = actionBtn("Exporter le rapport",
                                           "rgba(10,95,88,0.45)",
                                           "rgba(255,255,255,0.92)",
                                           st->standardIcon(QStyle::SP_DialogSaveButton),
                                           true);

    left3L->addWidget(ddBox);
    left3L->addWidget(tree3, 1);
    left3L->addWidget(exportReport3);

    QFrame* right3 = softBox();
    QVBoxLayout* right3L = new QVBoxLayout(right3);
    right3L->setContentsMargins(10,10,10,10);
    right3L->setSpacing(10);

    QFrame* header3 = new QFrame;
    header3->setStyleSheet("QFrame{ background: rgba(255,255,255,0.72); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QHBoxLayout* header3L = new QHBoxLayout(header3);
    header3L->setContentsMargins(10,8,10,8);

    QPushButton* details3 = new QPushButton(st->standardIcon(QStyle::SP_FileDialogDetailedView), "  Détails");
    details3->setCursor(Qt::PointingHandCursor);
    details3->setStyleSheet(QString(R"(
        QPushButton{
            background:%1; color: rgba(255,255,255,0.95);
            border:1px solid rgba(0,0,0,0.18);
            border-radius: 12px; padding: 10px 16px; font-weight: 900;
        }
        QPushButton:hover{ background: %2; }
    )").arg(C_PRIMARY, C_TOPBAR));

    auto eqChip = [&](const QString& t){
        QLabel* c = new QLabel(t);
        c->setStyleSheet("background: rgba(255,255,255,0.90); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; padding: 8px 12px; font-weight:900; color: rgba(0,0,0,0.55);");
        return c;
    };

    header3L->addWidget(details3);
    header3L->addStretch(1);
    header3L->addWidget(eqChip("Enregistrements"));
    header3L->addWidget(eqChip("Événements"));

    QFrame* listBox3 = new QFrame;
    listBox3->setStyleSheet("QFrame{ background: rgba(255,255,255,0.55); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QVBoxLayout* listBox3L = new QVBoxLayout(listBox3);
    listBox3L->setContentsMargins(12,12,12,12);

    QListWidget* list3 = new QListWidget;
    list3->setSpacing(8);
    list3->setSelectionMode(QAbstractItemView::NoSelection);

    auto addListRow=[&](QWidget* w){
        QListWidgetItem* it = new QListWidgetItem;
        it->setSizeHint(QSize(10, 40));
        list3->addItem(it);
        list3->setItemWidget(it, w);
    };

    addListRow(new GradientRowWidget(st, "Échantillon A", "5×",   W_GREEN,  QStyle::SP_FileIcon, false));
    addListRow(new GradientRowWidget(st, "Échantillon B", "0×",   W_GREEN,  QStyle::SP_FileIcon, false));
    addListRow(new GradientRowWidget(st, "Échantillon C", "OK",   W_ORANGE, QStyle::SP_FileIcon, false));
    addListRow(new GradientRowWidget(st, "Échantillon D", "BSL",  W_RED,    QStyle::SP_FileIcon, true));

    listBox3L->addWidget(list3);

    QWidget* bottomInfo3 = new QWidget;
    QHBoxLayout* bottomInfo3L = new QHBoxLayout(bottomInfo3);
    bottomInfo3L->setContentsMargins(0,0,0,0);
    bottomInfo3L->setSpacing(12);
    bottomInfo3L->addWidget(w3TempQtyBlock(st, "-80°C", "220"));
    bottomInfo3L->addWidget(w3BottomLocationBar(st, "Congélateur 02, Étagère A"), 1);

    right3L->addWidget(header3);
    right3L->addWidget(listBox3, 1);
    right3L->addWidget(bottomInfo3);

    outer3L->addWidget(left3);
    outer3L->addWidget(right3, 1);

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

    QPushButton* exportStats = actionBtn("Exporter", "rgba(10,95,88,0.45)", "rgba(255,255,255,0.92)", st->standardIcon(QStyle::SP_DialogSaveButton), true);

    actL->addWidget(hint);
    actL->addStretch(1);
    actL->addWidget(exportStats);
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
    pie->setData({
        {95,  QColor("#9FBEB9"), "DNA"},
        {55,  W_GREEN,           "RNA"},
        {30,  W_ORANGE,          "Protéine"}
    });

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

    lgL->addWidget(legendRow(QColor("#9FBEB9"), "DNA"));
    lgL->addWidget(legendRow(W_GREEN,          "RNA"));
    lgL->addWidget(legendRow(W_ORANGE,         "Protéine"));
    lgL->addStretch(1);

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
    pDomain->addItems({"Domaine", "Génomique", "Protéomique", "Pharmacologie", "Immunologie"});

    QComboBox* pStatut = new QComboBox;
    pStatut->addItems({"Statut", "En cours", "En retard", "Critique", "Suspendu"});

    QComboBox* pBudget = new QComboBox;
    pBudget->addItems({"Budget", "< 100k", "100k - 200k", "> 200k"});

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

    QTableWidget* projTable = new QTableWidget(5, 8);
    projTable->setHorizontalHeaderLabels({"", "Nom du projet","Domaine","Responsable","Budget","Date début","Statut","Financement"});
    projTable->verticalHeader()->setVisible(false);
    projTable->setShowGrid(true);
    projTable->setAlternatingRowColors(true);
    projTable->setStyleSheet(QString("QTableWidget{ alternate-background-color:%1; background-color:%2; }").arg(C_ROW_EVEN, C_ROW_ODD));
    projTable->horizontalHeader()->setStretchLastSection(true);
    projTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    projTable->setSelectionMode(QAbstractItemView::SingleSelection);
    projTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    projTable->setColumnWidth(0, 36);
    projTable->setColumnWidth(1, 200);
    projTable->setColumnWidth(2, 150);
    projTable->setColumnWidth(3, 140);
    projTable->setColumnWidth(4, 90);
    projTable->setColumnWidth(5, 120);
    projTable->setColumnWidth(6, 130);
    projTable->setColumnWidth(7, 100);

    auto setProjRow=[&](int r, const QString& name,
                        const QString& domain, const QString& investigator,
                        const QString& budget, const QString& startDate,
                        const QString& status, const QString& funding)
    {
        QTableWidgetItem* iconItem = new QTableWidgetItem;
        iconItem->setIcon(st->standardIcon(QStyle::SP_ArrowRight));
        iconItem->setTextAlignment(Qt::AlignCenter);
        projTable->setItem(r, 0, iconItem);

        auto mk = [&](const QString& t){
            QTableWidgetItem* it = new QTableWidgetItem(t);
            it->setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);
            return it;
        };

        projTable->setItem(r, 1, mk(name));
        projTable->setItem(r, 2, mk(domain));
        projTable->setItem(r, 3, mk(investigator));

        QTableWidgetItem* b = mk(budget);
        b->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
        projTable->setItem(r, 4, b);

        projTable->setItem(r, 5, mk(startDate));
        projTable->setItem(r, 6, mk(status));
        projTable->setItem(r, 7, mk(funding));
        projTable->setRowHeight(r, 46);
    };

    setProjRow(0, "Séquençage Génomique",    "Génomique",      "Dr. Amal",   "150k", "15/01/2024", "En cours",  "ANR");
    setProjRow(1, "Analyse Protéique",        "Protéomique",    "Dr. Nader",  "85k",  "10/03/2024", "En retard", "CNRS");
    setProjRow(2, "Découverte Médicaments",   "Pharmacologie",  "Dr. Hichem", "220k", "01/02/2024", "Critique",  "Privé");
    setProjRow(3, "Recherche Cancer",         "Génomique",      "Mme. Sara",  "180k", "05/12/2023", "En cours",  "ANR");
    setProjRow(4, "Développement Vaccin",     "Immunologie",    "M. Karim",   "95k",  "20/04/2024", "Suspendu",  "OMS");

    // Search by all fields
    QObject::connect(pSearch, &QLineEdit::textChanged, this, [=](const QString& text){
        QString filter = text.trimmed().toLower();
        for (int r = 0; r < projTable->rowCount(); ++r) {
            bool match = false;
            if (filter.isEmpty()) { match = true; }
            else {
                for (int c = 1; c < projTable->columnCount(); ++c) {
                    QTableWidgetItem* it = projTable->item(r, c);
                    if (it && it->text().toLower().contains(filter)) { match = true; break; }
                }
            }
            projTable->setRowHidden(r, !match);
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
            QMessageBox::information(this, "Information", "Veuillez sélectionner une ligne à supprimer.");
            return;
        }
           QString resume = QString("Projet : %1 | Investigator : %2 | Status : %3")
                            .arg(projTable->item(r,1)->text(),
                                projTable->item(r,3)->text(),
                                projTable->item(r,6)->text());
        ConfirmDeleteDialog confirm(style(), resume, this);
        if (confirm.exec() == QDialog::Accepted) projTable->removeRow(r);
    });

    projBottomL->addWidget(projAdd);
    projBottomL->addWidget(projEdit);
    projBottomL->addWidget(projDel);
    projBottomL->addWidget(projDetails);
    projBottomL->addStretch(1);

    projBottomL->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_DirIcon)));
    projBottomL->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_FileIcon)));
    projBottomL->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_DialogSaveButton)));
    projBottomL->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_BrowserReload)));

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

    gp1->addWidget(projBottom);

    stack->addWidget(proj1);

    // ==========================================================
    // PAGE 6 : Gestion Projet - Widget 2 (AJOUT/MODIF)
    // ==========================================================
    QWidget* proj2 = new QWidget;
    QVBoxLayout* gp2 = new QVBoxLayout(proj2);
    gp2->setContentsMargins(22, 18, 22, 18);
    gp2->setSpacing(14);

    ModulesBar barProjForm;
    gp2->addWidget(makeHeaderBlock(st, "Ajouter / Modifier un projet", ModuleTab::GestionProjet, &barProjForm));
    connectModulesSwitch(this, stack, barProjForm);

    QFrame* outP2 = new QFrame;
    outP2->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }").arg(C_PANEL_BG, C_PANEL_BR));
    QHBoxLayout* outP2L = new QHBoxLayout(outP2);
    outP2L->setContentsMargins(12,12,12,12);
    outP2L->setSpacing(12);

    QFrame* p2Left = softBox();
    p2Left->setFixedWidth(380);
    QVBoxLayout* p2LeftL = new QVBoxLayout(p2Left);
    p2LeftL->setContentsMargins(12,12,12,12);
    p2LeftL->setSpacing(10);

    auto projTitle = [&](const QString& t){
        QLabel* lab = new QLabel(t);
        lab->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
        return lab;
    };

    auto projRow = [&](QStyle::StandardPixmap sp, const QString& label, QWidget* input){
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

    QLineEdit* projName = new QLineEdit;
    projName->setPlaceholderText("Nom du projet");

    QComboBox* projDomain = new QComboBox;
    projDomain->addItems({"Domaine", "Biotech", "Recherche", "Qualité", "Santé", "Laboratoire"});
    projDomain->setFixedWidth(200);

    QComboBox* projStatus = new QComboBox;
    projStatus->addItems({"Statut", "Planifié", "En cours", "Terminé", "En retard"});
    projStatus->setFixedWidth(200);

    p2LeftL->addWidget(projTitle("Informations"));
    p2LeftL->addWidget(projRow(QStyle::SP_DirIcon, "Nom", projName));
    p2LeftL->addWidget(projRow(QStyle::SP_ComputerIcon, "Domaine", projDomain));
    p2LeftL->addWidget(projRow(QStyle::SP_MessageBoxInformation, "Statut", projStatus));
    p2LeftL->addStretch(1);

    QFrame* p2Right = softBox();
    QVBoxLayout* p2RightL = new QVBoxLayout(p2Right);
    p2RightL->setContentsMargins(12,12,12,12);
    p2RightL->setSpacing(10);

    QLineEdit* projOwner = new QLineEdit;
    projOwner->setPlaceholderText("Responsable");

    QSpinBox* projBudget = new QSpinBox;
    projBudget->setRange(0, 999999999);
    projBudget->setValue(0);
    projBudget->setFixedWidth(180);
    projBudget->setStyleSheet("QSpinBox{ background: rgba(255,255,255,0.65); border: 1px solid rgba(0,0,0,0.15); border-radius: 12px; padding: 10px 14px; color: rgba(0,0,0,0.65); font-weight: 900; }");

    QDateEdit* projStart = new QDateEdit(QDate::currentDate());
    projStart->setCalendarPopup(true);
    projStart->setDisplayFormat("dd/MM/yyyy");
    projStart->setStyleSheet("QDateEdit{ background: rgba(255,255,255,0.65); border: 1px solid rgba(0,0,0,0.15); border-radius: 12px; padding: 10px 14px; color: rgba(0,0,0,0.65); font-weight: 900; }");

    QDateEdit* projEnd = new QDateEdit(QDate::currentDate().addDays(60));
    projEnd->setCalendarPopup(true);
    projEnd->setDisplayFormat("dd/MM/yyyy");
    projEnd->setStyleSheet(projStart->styleSheet());

    QComboBox* projPriority = new QComboBox;
    projPriority->addItems({"Priorité", "Haute", "Moyenne", "Basse"});
    projPriority->setFixedWidth(200);

    p2RightL->addWidget(projTitle("Planification"));
    p2RightL->addWidget(projRow(QStyle::SP_DirHomeIcon, "Responsable", projOwner));
    p2RightL->addWidget(projRow(QStyle::SP_DialogApplyButton, "Budget", projBudget));
    p2RightL->addWidget(projRow(QStyle::SP_FileDialogDetailedView, "Date début", projStart));
    p2RightL->addWidget(projRow(QStyle::SP_FileDialogDetailedView, "Date fin", projEnd));
    p2RightL->addWidget(projRow(QStyle::SP_ArrowUp, "Priorité", projPriority));
    p2RightL->addStretch(1);

    outP2L->addWidget(p2Left);
    outP2L->addWidget(p2Right, 1);
    gp2->addWidget(outP2, 1);

    QFrame* p2Bottom = new QFrame;
    p2Bottom->setFixedHeight(64);
    p2Bottom->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
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

    // ==========================================================
    // PAGE 7 : Gestion Projet - Widget 3 (STATISTIQUES)
    // ==========================================================
    QWidget* proj3 = new QWidget;
    QVBoxLayout* gp3 = new QVBoxLayout(proj3);
    gp3->setContentsMargins(22, 18, 22, 18);
    gp3->setSpacing(14);

    ModulesBar barProjStats;
    gp3->addWidget(makeHeaderBlock(st, "Statistiques Projet", ModuleTab::GestionProjet, &barProjStats));
    connectModulesSwitch(this, stack, barProjStats);

    QFrame* outP3 = new QFrame;
    outP3->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }").arg(C_PANEL_BG, C_PANEL_BR));
    QVBoxLayout* outP3L = new QVBoxLayout(outP3);
    outP3L->setContentsMargins(12,12,12,12);
    outP3L->setSpacing(12);

    QFrame* actP3 = new QFrame;
    actP3->setStyleSheet("QFrame{ background: rgba(255,255,255,0.35); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QHBoxLayout* actP3L = new QHBoxLayout(actP3);
    actP3L->setContentsMargins(12,10,12,10);

    QLabel* hp = new QLabel("Aperçu : statut & charge mensuelle");
    hp->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    QPushButton* exportP3 = actionBtn("Exporter", "rgba(10,95,88,0.45)", "rgba(255,255,255,0.92)", st->standardIcon(QStyle::SP_DialogSaveButton), true);

    actP3L->addWidget(hp);
    actP3L->addStretch(1);
    actP3L->addWidget(exportP3);
    outP3L->addWidget(actP3);

    QFrame* dashP3 = new QFrame;
    dashP3->setStyleSheet("QFrame{ background: rgba(255,255,255,0.55); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QHBoxLayout* dashP3L = new QHBoxLayout(dashP3);
    dashP3L->setContentsMargins(12,12,12,12);
    dashP3L->setSpacing(12);

    QFrame* pieP = softBox();
    QVBoxLayout* piePL = new QVBoxLayout(pieP);
    piePL->setContentsMargins(12,12,12,12);
    piePL->setSpacing(10);
    QLabel* pt = new QLabel("Répartition des projets par statut");
    pt->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    DonutChart* pd = new DonutChart;
    pd->setData({
        {3, W_GREEN,  "En cours"},
        {2, W_ORANGE, "Planifié"},
        {1, W_RED,    "En retard"},
        {2, QColor("#9FBEB9"), "Terminé"}
    });
    piePL->addWidget(pt);
    piePL->addWidget(pd, 1);

    QFrame* barP = softBox();
    QVBoxLayout* barPL = new QVBoxLayout(barP);
    barPL->setContentsMargins(12,12,12,12);
    barPL->setSpacing(10);
    QLabel* bt = new QLabel("Tâches / charge estimée par mois");
    bt->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    BarChart* bd = new BarChart;
    bd->setData({
        {8, "Jan"}, {12, "Fév"}, {14, "Mar"}, {10, "Avr"},
        {18, "Mai"}, {9, "Juin"}, {16, "Juil"}, {11, "Août"},
        {13, "Sep"}, {20, "Oct"}, {17, "Nov"}, {9, "Déc"}
    });
    barPL->addWidget(bt);
    barPL->addWidget(bd, 1);

    dashP3L->addWidget(pieP, 1);
    dashP3L->addWidget(barP, 2);

    outP3L->addWidget(dashP3, 1);

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
    eSearch->setPlaceholderText("Rechercher par status, type");
    eSearch->addAction(st->standardIcon(QStyle::SP_FileDialogContentsView), QLineEdit::LeadingPosition);

    QComboBox* eTrierCb = new QComboBox;
    eTrierCb->addItems({"Trier par date", "Trier par type", "Trier par statut"});

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
    eBarL->addWidget(eExportBtn);
    ep1->addWidget(eBar);

    QFrame* expCard = makeCard();
    QVBoxLayout* expCardL = new QVBoxLayout(expCard);
    expCardL->setContentsMargins(10,10,10,10);

    ExperienceCrud* expCrud     = new ExperienceCrud;
    bool*           expEditMode = new bool(false);
    int*            expEditId   = new int(0);

    QTableWidget* expTable = new QTableWidget(0, 5);
    expTable->setHorizontalHeaderLabels({"Nom", "Hypothèse", "Date début", "Date fin", "Statut"});
    expTable->verticalHeader()->setVisible(false);
    expTable->setShowGrid(true);
    expTable->setAlternatingRowColors(true);
    expTable->setStyleSheet(QString("QTableWidget{ alternate-background-color:%1; background-color:%2; }").arg(C_ROW_EVEN, C_ROW_ODD));
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

    auto statusFromString = [](const QString& s) -> ExpStatus {
        if (s == "En cours")  return ExpStatus::EnCours;
        if (s == "Concluante" || s == "Réussie") return ExpStatus::Termine;
        if (s == "Échouée") return ExpStatus::Suspendue;
        return ExpStatus::EnAttente;
    };

    auto loadExpTable = [=](){
        expTable->setRowCount(0);
        QList<ExperienceRecord> recs;
        if (!expCrud->loadExperiences(recs)) return;
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
            badge->setData(Qt::UserRole, (int)statusFromString(rec.status));
            expTable->setItem(row, 4, badge);
            expTable->setRowHeight(row, 46);
        }
    };
    loadExpTable();

    QObject::connect(stack, &QStackedWidget::currentChanged, expTable, [=](int idx){
        if (idx == EXP_LIST) loadExpTable();
    });

    // Search by titre or protocole
    QObject::connect(eSearch, &QLineEdit::textChanged, this, [=](const QString& text){
        QString filter = text.trimmed().toLower();
        for (int r = 0; r < expTable->rowCount(); ++r) {
            bool match = false;
            if (filter.isEmpty()) { match = true; }
            else {
                for (int c = 0; c < expTable->columnCount(); ++c) {
                    QTableWidgetItem* it = expTable->item(r, c);
                    if (it && it->text().toLower().contains(filter)) { match = true; break; }
                }
            }
            expTable->setRowHidden(r, !match);
        }
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

    expBottomL->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_DirIcon)));
    expBottomL->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_FileIcon)));
    expBottomL->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_DialogSaveButton)));
    expBottomL->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_BrowserReload)));

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

    QLineEdit* eHypo = new QLineEdit;
    eHypo->setPlaceholderText("Hypothese");

    QComboBox* eProjet = new QComboBox;
    eProjet->addItem("Projet", QVariant());

    auto loadProjetsIntoCombo = [=]() {
        const int savedId = eProjet->currentData().isNull() ? -1 : eProjet->currentData().toInt();
        eProjet->blockSignals(true);
        eProjet->clear();
        eProjet->addItem("Projet", QVariant());
        QSqlQuery pq;
        if (pq.exec("SELECT ID_PROJET, NOM_DU_PROJET FROM PROJET ORDER BY ID_PROJET")) {
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

    QComboBox* eStatus = new QComboBox;
    eStatus->addItems({"Statut", "En cours", "Concluante", "Réussie", "Échouée", "Archivée"});
    eStatus->setFixedWidth(200);

    e2RightL->addWidget(expTitle("Planification"));
    e2RightL->addWidget(expRow(QStyle::SP_MessageBoxInformation, "Date début", eDateDebut));
    e2RightL->addWidget(expRow(QStyle::SP_MessageBoxInformation, "Date fin",   eDateFin));
    e2RightL->addWidget(expRow(QStyle::SP_MessageBoxInformation, "Statut",     eStatus));
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
    donutE->setData({
        {4, W_GREEN,  "En cours"},
        {3, W_ORANGE, "Planifiée"},
        {2, W_RED,    "Suspendue"},
        {5, QColor("#9FBEB9"), "Terminée"}
    });

    pieEL->addWidget(pieET);
    pieEL->addWidget(donutE, 1);

    QFrame* barE = softBox();
    QVBoxLayout* barEL = new QVBoxLayout(barE);
    barEL->setContentsMargins(12,12,12,12);
    barEL->setSpacing(10);

    QLabel* barET = new QLabel("Nombre d’expériences par mois");
    barET->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");

    BarChart* barsE = new BarChart;
    barsE->setData({
        {6, "Jan"}, {8, "Fév"}, {9, "Mar"}, {7, "Avr"},
        {10,"Mai"}, {5, "Juin"}, {11,"Juil"}, {6, "Août"},
        {8, "Sep"}, {12,"Oct"}, {9, "Nov"}, {7, "Déc"}
    });

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
    pubSearch->setPlaceholderText("Rechercher (ID, titre, journal, année, DOI, projet, employé...)");
    pubSearch->addAction(st->standardIcon(QStyle::SP_FileDialogContentsView), QLineEdit::LeadingPosition);

    QComboBox* pubType = new QComboBox;
    pubType->addItems({"Statut", "Brouillon", "Soumise", "Acceptée", "Publiée", "Rejetée"});

    QComboBox* pubYear = new QComboBox;
    pubYear->addItems({"Année", "2026", "2025", "2024", "2023", "2022"});

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
    pubBarL->addWidget(pubFilters);
    pb1->addWidget(pubBar);

    // Table
    QFrame* pubCard = makeCard();
    QVBoxLayout* pubCardL = new QVBoxLayout(pubCard);
    pubCardL->setContentsMargins(10,10,10,10);

    QTableWidget* pubTable = new QTableWidget(0, 8);
    pubTable->setHorizontalHeaderLabels({"ID","Titre","Journal/Conf.","Année","DOI","Statut","Id_projet","Employee_id"});
    pubTable->verticalHeader()->setVisible(false);
    pubTable->horizontalHeader()->setStretchLastSection(true);
    pubTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    pubTable->setSelectionMode(QAbstractItemView::SingleSelection);
    pubTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    pubTable->setColumnHidden(0, true);

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
            for (int c = 0; c < pubTable->columnCount(); ++c) {
                QTableWidgetItem* it = new QTableWidgetItem(model->data(model->index(r, c)).toString());
                it->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                pubTable->setItem(r, c, it);
            }
            pubTable->setRowHeight(r, 46);
        }
        delete model;
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
            const QString projet = pubTable->item(r,6) ? pubTable->item(r,6)->text().toLower() : QString();
            const QString employe = pubTable->item(r,7) ? pubTable->item(r,7)->text().toLower() : QString();

            const bool matchesSearch = search.isEmpty()
                || id.contains(search)
                || titre.contains(search)
                || journal.contains(search)
                || annee.toLower().contains(search)
                || doi.contains(search)
                || projet.contains(search)
                || employe.contains(search);

            const bool matchesStatus = (status == "Statut") || (statut == status);
            const bool matchesYear = (year == "Année") || (annee == year);
            pubTable->setRowHidden(r, !(matchesSearch && matchesStatus && matchesYear));
        }
    };

    QObject::connect(pubSearch, &QLineEdit::textChanged, this, [=](){ applyPublicationFilters(); });
    QObject::connect(pubType, &QComboBox::currentTextChanged, this, [=](){ applyPublicationFilters(); });
    QObject::connect(pubYear, &QComboBox::currentTextChanged, this, [=](){ applyPublicationFilters(); });

    reloadPublications();
    applyPublicationFilters();

    pubCardL->addWidget(pubTable);
    pb1->addWidget(pubCard, 1);

    QHBoxLayout* pubQrRow = new QHBoxLayout;
    pubQrRow->setContentsMargins(0, 0, 0, 0);
    pubQrRow->setSpacing(0);
    pubQrRow->addStretch(1);

    QFrame* pubQrCard = softBox();
    pubQrCard->setFixedSize(104, 104);
    QVBoxLayout* pubQrCardL = new QVBoxLayout(pubQrCard);
    pubQrCardL->setContentsMargins(8, 8, 8, 8);

    QLabel* pubQrImage = new QLabel;
    pubQrImage->setFixedSize(88, 88);
    pubQrImage->setAlignment(Qt::AlignCenter);
    pubQrImage->setText("QR");
    pubQrImage->setToolTip("https://www.biorxiv.org/");
    pubQrImage->setStyleSheet("QLabel{ background: rgba(255,255,255,0.75); border: 1px solid rgba(0,0,0,0.10); border-radius: 8px; color: rgba(0,0,0,0.55); font-weight: 900; }");

    pubQrCardL->addWidget(pubQrImage, 0, Qt::AlignCenter);
    pubQrRow->addWidget(pubQrCard, 0, Qt::AlignRight);
    pb1->addLayout(pubQrRow);

    QNetworkAccessManager* qrManager = new QNetworkAccessManager(this);
    const QUrl qrUrl("https://api.qrserver.com/v1/create-qr-code/?size=320x320&data=https%3A%2F%2Fwww.biorxiv.org%2F");
    QObject::connect(qrManager, &QNetworkAccessManager::finished, this, [=](QNetworkReply* reply){
        const bool ok = (reply->error() == QNetworkReply::NoError);
        if (ok) {
            QPixmap qrPixmap;
            if (qrPixmap.loadFromData(reply->readAll())) {
                pubQrImage->setPixmap(qrPixmap.scaled(pubQrImage->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            } else {
                pubQrImage->setText("QR");
            }
        } else {
            pubQrImage->setText("QR");
        }
        reply->deleteLater();
    });
    qrManager->get(QNetworkRequest(qrUrl));

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

    QComboBox* cbStatus = new QComboBox;
    cbStatus->addItems({"Statut", "Brouillon", "Soumise", "Acceptée", "Publiée", "Rejetée"});
    cbStatus->setFixedWidth(220);

    QSpinBox* sbIdProjet = new QSpinBox;
    sbIdProjet->setRange(0, 1000000000);
    sbIdProjet->setFixedWidth(220);
    sbIdProjet->setStyleSheet("QSpinBox{ background: rgba(255,255,255,0.65); border: 1px solid rgba(0,0,0,0.15); border-radius: 12px; padding: 10px 14px; color: rgba(0,0,0,0.65); font-weight: 900; }");

    QSpinBox* sbEmployeeId = new QSpinBox;
    sbEmployeeId->setRange(0, 1000000000);
    sbEmployeeId->setFixedWidth(220);
    sbEmployeeId->setStyleSheet("QSpinBox{ background: rgba(255,255,255,0.65); border: 1px solid rgba(0,0,0,0.15); border-radius: 12px; padding: 10px 14px; color: rgba(0,0,0,0.65); font-weight: 900; }");

    QTextEdit* teAbstract = new QTextEdit;
    teAbstract->setPlaceholderText("Résumé / Notes / Mots-clés (traçabilité scientifique)");
    teAbstract->setStyleSheet("QTextEdit{ background: rgba(255,255,255,0.65); border: 1px solid rgba(0,0,0,0.15); border-radius: 12px; padding: 10px 14px; color: rgba(0,0,0,0.65); font-weight: 800; }");

    pub2RightL->addWidget(pubTitle("Détails"));
    pub2RightL->addWidget(pubRow(QStyle::SP_DirHomeIcon, "Journal/Conf.", leJournal));
    pub2RightL->addWidget(pubRow(QStyle::SP_FileDialogContentsView, "DOI", leDOI));
    pub2RightL->addWidget(pubRow(QStyle::SP_MessageBoxInformation, "Statut", cbStatus));
    pub2RightL->addWidget(pubRow(QStyle::SP_FileDialogToParent, "Id_projet", sbIdProjet));
    pub2RightL->addWidget(pubRow(QStyle::SP_DirHomeIcon, "Employee_id", sbEmployeeId));
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

    QLabel* hpPUB = new QLabel("Aperçu : statut & publications par année");
    hpPUB->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");

    QPushButton* exportPUB = actionBtn("Exporter", "rgba(10,95,88,0.45)", "rgba(255,255,255,0.92)", st->standardIcon(QStyle::SP_DialogSaveButton), true);

    actPUB3L->addWidget(hpPUB);
    actPUB3L->addStretch(1);
    actPUB3L->addWidget(exportPUB);
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
    donutPUB->setData({
        {3, W_GREEN,  "Publiée"},
        {2, W_ORANGE, "Acceptée"},
        {2, QColor("#9FBEB9"), "Soumise"},
        {1, W_RED,    "Rejetée"}
    });

    piePUBL->addWidget(piePUBT);
    piePUBL->addWidget(donutPUB, 1);

    QFrame* barPUB = softBox();
    QVBoxLayout* barPUBL = new QVBoxLayout(barPUB);
    barPUBL->setContentsMargins(12,12,12,12);
    barPUBL->setSpacing(10);

    QLabel* barPUBT = new QLabel("Publications par année");
    barPUBT->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");

    BarChart* barsPUB = new BarChart;
    barsPUB->setData({
        {1, "2022"}, {2, "2023"}, {3, "2024"}, {4, "2025"}, {2, "2026"}
    });

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
    eqSearch->setPlaceholderText("Rechercher (equipement)");
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
    eqTable->setStyleSheet(QString("QTableWidget{ alternate-background-color:%1; background-color:%2; }").arg(C_ROW_EVEN, C_ROW_ODD));
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
        QString nomFilter = eqSearch->text().trimmed();
        if (cbEquipType->currentIndex() > 0) {
            if (!nomFilter.isEmpty()) nomFilter += " ";
            nomFilter += cbEquipType->currentText();
        }
        const QString statusFilter = (cbEquipStatus->currentIndex() <= 0) ? QString() : cbEquipStatus->currentText();
        const QString locFilter    = (cbEquipLoc->currentIndex() <= 0) ? QString() : cbEquipLoc->currentText();
        if (!eqCrud->loadEquipements(recs, &err, QString(), nomFilter, statusFilter, locFilter)) {
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
    fcb1->setCurrentText("");
    fcb2->setCurrentText("");
    if (fcb1->lineEdit()) fcb1->lineEdit()->setPlaceholderText("Type d'équipement");
    if (fcb2->lineEdit()) fcb2->lineEdit()->setPlaceholderText("Fabricant");

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

    QFrame* maintRow = softBox();
    QHBoxLayout* maintL = new QHBoxLayout(maintRow);
    maintL->setContentsMargins(10,8,10,8);
    maintL->setSpacing(8);

    QToolButton* cal2 = new QToolButton; cal2->setAutoRaise(true); cal2->setIcon(st->standardIcon(QStyle::SP_FileDialogDetailedView));
    QLabel* maintLabel = new QLabel("Prochaine maintenance :");
    maintLabel->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    QDateEdit* maintDate = new QDateEdit(QDate(2026,3,15));
    maintDate->setCalendarPopup(true);
    maintDate->setDisplayFormat("dd/MM/yyyy");
    maintDate->setStyleSheet("QDateEdit{ background: transparent; border:0; font-weight: 900; color: rgba(0,0,0,0.55);} ");

    maintL->addWidget(cal2);
    maintL->addWidget(maintLabel);
    maintL->addWidget(maintDate, 1);
    eqRight2L->addWidget(maintRow);

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

    eqFormGrid->addWidget(fieldBlock("Type d'équipement", comboRow(fcb1)), 0, 0);
    eqFormGrid->addWidget(fieldBlock("Fabricant", comboRow(fcb2)), 0, 1);
    eqFormGrid->addWidget(fieldBlock("Statut", comboRow(fcb3)), 1, 0);
    eqFormGrid->addWidget(fieldBlock("Modèle", modelRow), 1, 1);
    eqFormGrid->addWidget(fieldBlock("Date d'achat", dateRow), 2, 0);
    eqFormGrid->addWidget(fieldBlock("Prochaine maintenance", maintRow), 2, 1);
    eqFormGrid->addWidget(fieldBlock("Calibration", calRow), 3, 0);
    eqFormGrid->addWidget(fieldBlock("Salle", miniRow(QStyle::SP_DirIcon, "Salle :", labRoom)), 3, 1);
    eqFormGrid->setRowStretch(4, 1);

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

    auto chip = [&](const QString& t){
        QLabel* c = new QLabel(t);
        c->setStyleSheet("background: rgba(255,255,255,0.90); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; padding: 8px 12px; font-weight:900; color: rgba(0,0,0,0.55);");
        return c;
    };

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

    auto addDetailRow = [&](int row, int col, const QString& label, const QString& value){
        QLabel* lbl = new QLabel("<b>" + label + " :</b>");
        lbl->setStyleSheet("color: rgba(0,0,0,0.65); font-weight: 900; font-size: 12px;");
        QLabel* val = new QLabel(value);
        val->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 600; font-size: 12px;");
        detailsGrid->addWidget(lbl, row, col*2);
        detailsGrid->addWidget(val, row, col*2+1);
    };

    addDetailRow(0, 0, "Fabricant", "Thermo Fisher");
    addDetailRow(1, 0, "Modèle", "TX-500");
    addDetailRow(2, 0, "Localisation", "Lab 101");
    addDetailRow(3, 0, "Date d'achat", "15/01/2023");

    addDetailRow(0, 1, "Dernière maintenance", "15/12/2025");
    addDetailRow(1, 1, "Prochaine maintenance", "15/03/2026");
    addDetailRow(2, 1, "Calibration", "15/06/2026");
    addDetailRow(3, 1, "Utilisateur", "Dr. Smith (USER-123)");

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
    empSearch->setPlaceholderText("Rechercher (CIN)");
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

    QTableWidget* empTable = new QTableWidget(6, 11);
    empTable->setHorizontalHeaderLabels({"", "CIN", "Nom", "Prenom", "Role", "Specialisation", "Qualification", "Publications", "Temps", "Laboratoire", "Projet"});
    empTable->verticalHeader()->setVisible(false);
    empTable->setShowGrid(true);
    empTable->setAlternatingRowColors(true);
    empTable->setStyleSheet(QString("QTableWidget{ alternate-background-color:%1; background-color:%2; }").arg(C_ROW_EVEN, C_ROW_ODD));
    empTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    empTable->setSelectionMode(QAbstractItemView::SingleSelection);
    empTable->horizontalHeader()->setStretchLastSection(true);
    empTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    empTable->setItemDelegateForColumn(8, new EmployeeBadgeDelegate(empTable));

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

    auto setEmpRow=[&](int r, const QString& cin,
                       const QString& nom, const QString& prenom,
                       const QString& role, const QString& spec,
                       const QString& qualif, const QString& pubs,
                       FTStatus ft, const QString& lab, const QString& proj)
    {
        QTableWidgetItem* iconItem = new QTableWidgetItem;
        iconItem->setIcon(st->standardIcon(QStyle::SP_ArrowRight));
        iconItem->setTextAlignment(Qt::AlignCenter);
        empTable->setItem(r, 0, iconItem);

        auto mk = [&](const QString& t){
            QTableWidgetItem* it = new QTableWidgetItem(t);
            it->setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);
            return it;
        };

        empTable->setItem(r, 1, mk(cin));
        empTable->setItem(r, 2, mk(nom));
        empTable->setItem(r, 3, mk(prenom));
        empTable->setItem(r, 4, mk(role));
        empTable->setItem(r, 5, mk(spec));
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

    setEmpRow(0, "AA123456", "Ali", "Ben Salem", "Chercheur", "Biomol", "PhD",  "25", FTStatus::FullTime, "Lab A", "Projet GENOME");
    setEmpRow(1, "BB654321", "Sara", "Bouaziz", "Technicien", "Chimie",  "BSc",  "5",  FTStatus::PartTime, "Lab B", "-");
    setEmpRow(2, "CC998877", "Youssef", "K.",    "Chercheur", "Bioinfo", "PhD",  "12", FTStatus::Contract, "Lab C", "Projet AI-BIO");
    setEmpRow(3, "DD112233", "Meriem", "H.",     "Technicien","General", "BTS",  "2",  FTStatus::FullTime, "Lab A", "-");
    setEmpRow(4, "EE667788", "Omar",   "A.",     "Chercheur", "Biomol",  "PhD",  "40", FTStatus::OnLeave,  "Lab B", "Projet PROTEO");
    setEmpRow(5, "FF334455", "Nada",   "B.",     "Chercheur", "Chimie",  "MSc",  "10", FTStatus::FullTime, "Lab C", "Projet MATERIA");

    empCardL->addWidget(empTable);
    emp1->addWidget(empCard, 1);

    // Search by CIN and other fields
    QObject::connect(empSearch, &QLineEdit::textChanged, this, [=](const QString& text){
        QString filter = text.trimmed().toLower();
        for (int r = 0; r < empTable->rowCount(); ++r) {
            bool match = false;
            if (filter.isEmpty()) { match = true; }
            else {
                for (int c = 1; c <= 10; ++c) {
                    QTableWidgetItem* it = empTable->item(r, c);
                    if (it && it->text().toLower().contains(filter)) { match = true; break; }
                }
            }
            empTable->setRowHidden(r, !match);
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

    empBottomL->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_DirIcon)));
    empBottomL->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_FileIcon)));
    empBottomL->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_DialogSaveButton)));
    empBottomL->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_BrowserReload)));

    QPushButton* empMore = new QPushButton(st->standardIcon(QStyle::SP_FileDialogContentsView), "  Affectations & Labs");
    empMore->setCursor(Qt::PointingHandCursor);
    empMore->setStyleSheet(R"(
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

    auto empLeftAction = [&](const QString& title, QStyle::StandardPixmap sp, const QString& text){
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
        empLeft2L->addWidget(head);
        empLeft2L->addWidget(b);
    };

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
    QComboBox* empRoleCb = new QComboBox; empRoleCb->addItems({"Chercheur","Technicien"});
    empRight2L->addWidget(empComboRow(empPrenomEdit, empRoleCb));

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
    // PAGE 20 : Employés - AFFECTATIONS (EMP_AFF)
    // ==========================================================
    QWidget* empAffPage = new QWidget;
    QVBoxLayout* emp3 = new QVBoxLayout(empAffPage);
    emp3->setContentsMargins(22, 18, 22, 18);
    emp3->setSpacing(14);

    ModulesBar barEmpAff;
    emp3->addWidget(makeHeaderBlock(st, "Affectations & Laboratoires", ModuleTab::Employee, &barEmpAff));
    connectModulesSwitch(this, stack, barEmpAff);

    QFrame* empOuter3 = new QFrame;
    empOuter3->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }").arg(C_PANEL_BG, C_PANEL_BR));
    QHBoxLayout* empOuter3L = new QHBoxLayout(empOuter3);
    empOuter3L->setContentsMargins(12,12,12,12);
    empOuter3L->setSpacing(12);

    QFrame* empLeft3 = softBox();
    empLeft3->setFixedWidth(300);
    QVBoxLayout* empLeft3L = new QVBoxLayout(empLeft3);
    empLeft3L->setContentsMargins(10,10,10,10);
    empLeft3L->setSpacing(10);

    QFrame* empDdBox = new QFrame;
    empDdBox->setStyleSheet("QFrame{ background: rgba(255,255,255,0.72); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QHBoxLayout* empDdBoxL = new QHBoxLayout(empDdBox);
    empDdBoxL->setContentsMargins(10,8,10,8);

    QLabel* empDdText = new QLabel("Laboratoire: Lab A");
    empDdText->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    QToolButton* empDdBtn = new QToolButton;
    empDdBtn->setAutoRaise(true);
    empDdBtn->setIcon(st->standardIcon(QStyle::SP_ArrowDown));
    empDdBtn->setCursor(Qt::PointingHandCursor);

    empDdBoxL->addWidget(empDdText);
    empDdBoxL->addStretch(1);
    empDdBoxL->addWidget(empDdBtn);

    QTreeWidget* empTree3 = new QTreeWidget;
    empTree3->setHeaderHidden(true);
    empTree3->setIndentation(18);

    auto* empLabA = new QTreeWidgetItem(empTree3, QStringList() << "Lab A");
    auto* empLabB = new QTreeWidgetItem(empTree3, QStringList() << "Lab B");
    auto* empLabC = new QTreeWidgetItem(empTree3, QStringList() << "Lab C");

    empLabA->setIcon(0, st->standardIcon(QStyle::SP_DirIcon));
    empLabB->setIcon(0, st->standardIcon(QStyle::SP_DirIcon));
    empLabC->setIcon(0, st->standardIcon(QStyle::SP_DirIcon));

    auto* empP1 = new QTreeWidgetItem(empLabB, QStringList() << "Projet AI-BIO");
    auto* empP2 = new QTreeWidgetItem(empLabB, QStringList() << "Projet GENOME");
    auto* empP3 = new QTreeWidgetItem(empLabB, QStringList() << "Projet PROTEO");
    auto* empP4 = new QTreeWidgetItem(empLabB, QStringList() << "Groupe Techniciens");
    empP1->setIcon(0, st->standardIcon(QStyle::SP_FileIcon));
    empP2->setIcon(0, st->standardIcon(QStyle::SP_FileIcon));
    empP3->setIcon(0, st->standardIcon(QStyle::SP_FileIcon));
    empP4->setIcon(0, st->standardIcon(QStyle::SP_FileDialogInfoView));

    empTree3->expandAll();
    empTree3->setCurrentItem(empLabB);

    empLeft3L->addWidget(empDdBox);
    empLeft3L->addWidget(empTree3, 1);

    QFrame* empRight3 = softBox();
    QVBoxLayout* empRight3L = new QVBoxLayout(empRight3);
    empRight3L->setContentsMargins(10,10,10,10);
    empRight3L->setSpacing(10);

    QFrame* empHeader3 = new QFrame;
    empHeader3->setStyleSheet("QFrame{ background: rgba(255,255,255,0.72); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QHBoxLayout* empHeader3L = new QHBoxLayout(empHeader3);
    empHeader3L->setContentsMargins(10,8,10,8);

    QPushButton* empDetails3 = new QPushButton(st->standardIcon(QStyle::SP_FileDialogDetailedView), "  Details");
    empDetails3->setCursor(Qt::PointingHandCursor);
    empDetails3->setStyleSheet(QString(R"(
        QPushButton{
            background:%1; color: rgba(255,255,255,0.95);
            border:1px solid rgba(0,0,0,0.18);
            border-radius: 12px; padding: 10px 16px; font-weight: 900;
        }
        QPushButton:hover{ background: %2; }
    )").arg(C_PRIMARY, C_TOPBAR));

    auto empChip = [&](const QString& t){
        QLabel* c = new QLabel(t);
        c->setStyleSheet("background: rgba(255,255,255,0.90); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; padding: 8px 12px; font-weight:900; color: rgba(0,0,0,0.55);");
        return c;
    };

    empHeader3L->addWidget(empDetails3);
    empHeader3L->addStretch(1);
    empHeader3L->addWidget(empChip("Affectations"));
    empHeader3L->addWidget(empChip("Groupes"));

    QFrame* empListBox3 = new QFrame;
    empListBox3->setStyleSheet("QFrame{ background: rgba(255,255,255,0.55); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QVBoxLayout* empListBox3L = new QVBoxLayout(empListBox3);
    empListBox3L->setContentsMargins(12,12,12,12);

    QListWidget* empList3 = new QListWidget;
    empList3->setSpacing(8);
    empList3->setSelectionMode(QAbstractItemView::NoSelection);

    auto addEmpListRow=[&](QWidget* w){
        QListWidgetItem* it = new QListWidgetItem;
        it->setSizeHint(QSize(10, 40));
        empList3->addItem(it);
        empList3->setItemWidget(it, w);
    };

    addEmpListRow(new GradientRowWidget(st, "Ali Ben Salem (Chercheur)", "Pub: 25", W_GREEN,  QStyle::SP_FileIcon, false));
    addEmpListRow(new GradientRowWidget(st, "Sara Bouaziz (Technicien)", "Partiel", W_ORANGE, QStyle::SP_FileIcon, false));
    addEmpListRow(new GradientRowWidget(st, "Youssef K. (Chercheur)",    "Contrat", W_GRAY,   QStyle::SP_FileIcon, false));
    addEmpListRow(new GradientRowWidget(st, "Omar A. (Chercheur)",       "Absence", W_RED,    QStyle::SP_FileIcon, true));

    empListBox3L->addWidget(empList3);

    QWidget* empBottomInfo3 = new QWidget;
    QHBoxLayout* empBottomInfo3L = new QHBoxLayout(empBottomInfo3);
    empBottomInfo3L->setContentsMargins(0,0,0,0);
    empBottomInfo3L->setSpacing(12);
    empBottomInfo3L->addWidget(empInfoBlock(st, "Lab B: Chercheurs: 3, Techniciens: 2", "Disponibles: 4"));
    empBottomInfo3L->addWidget(empBottomBarWithText(st, "Lab B • Projet AI-BIO"), 1);

    empRight3L->addWidget(empHeader3);
    empRight3L->addWidget(empListBox3, 1);
    empRight3L->addWidget(empBottomInfo3);

    empOuter3L->addWidget(empLeft3);
    empOuter3L->addWidget(empRight3, 1);

    emp3->addWidget(empOuter3, 1);

    QFrame* empBottom3 = new QFrame;
    empBottom3->setFixedHeight(64);
    empBottom3->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* empBottom3L = new QHBoxLayout(empBottom3);
    empBottom3L->setContentsMargins(14,10,14,10);

    QPushButton* empBack3 = actionBtn("Retour", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_ArrowBack), true);
    empBottom3L->addWidget(empBack3);
    empBottom3L->addStretch(1);

    emp3->addWidget(empBottom3);
    stack->addWidget(empAffPage);

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
        int total = empTable->rowCount();

        for (int r=0; r<empTable->rowCount(); ++r) {
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
    QLabel* pubDetIdProjet = nullptr;
    QLabel* pubDetEmployeeId = nullptr;
    QLabel* pubDetAbstract = nullptr;

    pubDetailsL->addWidget(pubDetTitle);
    pubDetailsL->addWidget(pubDetailRow("Journal/Conf.", pubDetJournal));
    pubDetailsL->addWidget(pubDetailRow("Année", pubDetYear));
    pubDetailsL->addWidget(pubDetailRow("DOI", pubDetDoi));
    pubDetailsL->addWidget(pubDetailRow("Statut", pubDetStatus));
    pubDetailsL->addWidget(pubDetailRow("Id_projet", pubDetIdProjet));
    pubDetailsL->addWidget(pubDetailRow("Employee_id", pubDetEmployeeId));

    QLabel* abstractLabel = new QLabel("Résumé :");
    abstractLabel->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    QLabel* abstractValue = new QLabel;
    abstractValue->setWordWrap(true);
    abstractValue->setStyleSheet("color: rgba(0,0,0,0.65); font-weight: 700;");
    pubDetAbstract = abstractValue;

    pubDetailsL->addWidget(abstractLabel);
    pubDetailsL->addWidget(abstractValue);

    pb4->addWidget(pubDetailsCard, 1);

    QFrame* pubDetailsBottom = new QFrame;
    pubDetailsBottom->setFixedHeight(64);
    pubDetailsBottom->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* pubDetailsBottomL = new QHBoxLayout(pubDetailsBottom);
    pubDetailsBottomL->setContentsMargins(14,10,14,10);
    QPushButton* pubDetailsBack = actionBtn("Retour", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_ArrowBack), true);
    pubDetailsBottomL->addWidget(pubDetailsBack);
    pubDetailsBottomL->addStretch(1);
    pb4->addWidget(pubDetailsBottom);

    stack->addWidget(pubDetailsPage);

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
        sbIdProjet->setValue(0);
        sbEmployeeId->setValue(0);
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
        sbIdProjet->setValue(publication.idProjet());
        sbEmployeeId->setValue(publication.employeeId());
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
        pubDetIdProjet->setText(QString::number(publication.idProjet()));
        pubDetEmployeeId->setText(QString::number(publication.employeeId()));
        pubDetAbstract->setText(publication.abstractText().isEmpty() ? "Résumé non renseigné." : publication.abstractText());
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
    QLabel* expDetBsl = nullptr;

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
    expDetailsL->addWidget(expDetailRow("BSL", expDetBsl));

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
        expDetTitle->setText(expTable->item(r,0)->text());
        expDetProto->setText(expTable->item(r,1)->text());
        expDetResp->setText(expTable->item(r,2)->text());
        expDetDate->setText(expTable->item(r,3)->text());
        ExpStatus est = static_cast<ExpStatus>(expTable->item(r,4)->data(Qt::UserRole).toInt());
        expDetStatus->setText(expStatusText(est));
        expDetBsl->setText("");
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
    QLabel* projDetOwner = nullptr;
    QLabel* projDetBudget = nullptr;
    QLabel* projDetStart = nullptr;
    QLabel* projDetStatus = nullptr;

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
    projDetailsL->addWidget(projDetailRow("Responsable", projDetOwner));
    projDetailsL->addWidget(projDetailRow("Budget", projDetBudget));
    projDetailsL->addWidget(projDetailRow("Début", projDetStart));
    projDetailsL->addWidget(projDetailRow("Statut", projDetStatus));

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

    auto updateProjDetailsFromRow = [=]()->bool{
        int r = projTable->currentRow();
        if (r < 0) {
            QMessageBox::information(this, "Information", "Sélectionnez un projet.");
            return false;
        }
        projDetTitle->setText(projTable->item(r,1)->text());
        projDetDomain->setText(projTable->item(r,2)->text());
        projDetOwner->setText(projTable->item(r,3)->text());
        projDetBudget->setText(projTable->item(r,4)->text());
        projDetStart->setText(projTable->item(r,5)->text());
        projDetStatus->setText(projTable->item(r,6)->text());
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
        if (ref.isEmpty()) {
            showToast(this, "Veuillez saisir la référence de l'échantillon.", false);
            return;
        }
        if (cbType2->text().trimmed().isEmpty()) {
            showToast(this, "Veuillez saisir le type d'échantillon.", false);
            return;
        }
        // Build BioSample from form
        BioSample s;
        s.reference      = ref;
        s.type           = cbType2->text().trimmed();
        s.organisme      = cbOrg2->text().trimmed();
        s.emplacement    = QString("Cong:%1/Etag:%2")
                           .arg(leCongelateur->text().trimmed(),
                                leEtagere->text().trimmed());
        s.quantite       = qty->value();
        s.temperature    = cbTemp2->text().trimmed();
        s.niveauDanger   = (cbDanger->currentText() == "Niveau de danger")
                           ? "" : cbDanger->currentText();
        s.dateCollecte   = dCollect->date();
        s.dateExpiration = dExpire->date();

        s.idProjet = 1; // handled by add() auto-seed

        bool ok = false;
        if (*bioEditMode) {
            // ── UPDATE ──
            ok = crud->update(s);
            if (ok)
                showToast(this, QString("Échantillon « %1 » modifié avec succès.").arg(ref), true);
            else
                showToast(this, QString("Échec de la mise à jour : %1").arg(crud->lastError()), false);
        } else {
            // ── INSERT — check duplicate first ──
            QSqlQuery dup;
            dup.prepare("SELECT COUNT(1) FROM ECHANTILLONS WHERE REFERENCE = ?");
            dup.addBindValue(ref);
            if (dup.exec() && dup.next() && dup.value(0).toInt() > 0) {
                showToast(this, "Un échantillon avec cette référence existe déjà.", false);
                return;
            }
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
        stack->setCurrentIndex(BIO_LOC);
    });
    QObject::connect(back3, &QPushButton::clicked, this, [=]{
        setWindowTitle("Gestion des Échantillons");
        stack->setCurrentIndex(BIO_LIST);
    });
    QObject::connect(details3, &QPushButton::clicked, this, [=]{
        setWindowTitle("Localisation & Stockage");
        stack->setCurrentIndex(BIO_RACK);
    });
    QObject::connect(back4, &QPushButton::clicked, this, [=]{
        setWindowTitle("Localisation & Stockage");
        stack->setCurrentIndex(BIO_LOC);
    });

    // ── STATISTIQUES — mise à jour depuis la BDD avant affichage ──
    auto updateBioStats = [=]{
        // Donut chart : répartition par type
        auto typeMap = crud->countByType();
        QList<DonutChart::Slice> slices;
        if (typeMap.contains("DNA"))
            slices.append({(double)typeMap["DNA"],      QColor("#9FBEB9"), "DNA"});
        if (typeMap.contains("RNA"))
            slices.append({(double)typeMap["RNA"],      W_GREEN,           "RNA"});
        if (typeMap.contains("Protéine"))
            slices.append({(double)typeMap["Protéine"], W_ORANGE,          "Protéine"});
        // Fallback si base vide
        if (slices.isEmpty())
            slices = {{95, QColor("#9FBEB9"), "DNA"},
                      {55, W_GREEN,           "RNA"},
                      {30, W_ORANGE,          "Protéine"}};
        pie->setData(slices);

        // Bar chart : échantillons par mois
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
    // NAVIGATION Gestion Projet (3 widgets)
    // ==========================================================
    QObject::connect(projAdd, &QPushButton::clicked, this, [=](){
        setWindowTitle("Ajouter / Modifier un projet");
        stack->setCurrentIndex(PROJ_FORM);
    });
    QObject::connect(projEdit, &QPushButton::clicked, this, [=](){
        setWindowTitle("Ajouter / Modifier un projet");
        stack->setCurrentIndex(PROJ_FORM);
    });
    QObject::connect(projCancel, &QPushButton::clicked, this, [=](){
        setWindowTitle("Gestion Projet");
        stack->setCurrentIndex(PROJ_LIST);
    });
    QObject::connect(projSave, &QPushButton::clicked, this, [=](){
        setWindowTitle("Gestion Projet");
        stack->setCurrentIndex(PROJ_LIST);
        QMessageBox::information(this, "Projet", "Enregistrement (à connecter à la base de données).");
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
    QObject::connect(projDetailsBack, &QPushButton::clicked, this, [=](){
        setWindowTitle("Gestion Projet");
        stack->setCurrentIndex(PROJ_LIST);
    });

    // Exports (démo)
    QObject::connect(exportReport3, &QPushButton::clicked, this, [=](){
        QMessageBox::information(this, "Export", "Export rapport (à connecter à PDF/Excel).");
    });
    QObject::connect(export4, &QPushButton::clicked, this, [=](){
        QMessageBox::information(this, "Export", "Export rapport (à connecter à PDF/Excel).");
    });
    QObject::connect(exportStats, &QPushButton::clicked, this, [=](){
        QMessageBox::information(this, "Export", "Export statistiques BioSimple (à connecter à PDF/Excel).");
    });
    QObject::connect(exportP3, &QPushButton::clicked, this, [=](){
        QMessageBox::information(this, "Export", "Export statistiques Projet (à connecter à PDF/Excel).");
    });
    // ==========================================================
    // NAVIGATION Expériences / Protocoles (3 widgets)
    // ==========================================================
    QObject::connect(expAdd, &QPushButton::clicked, this, [=](){
        *expEditMode = false; *expEditId = 0;
        loadProjetsIntoCombo();
        eName->clear(); eHypo->clear();
        eDateDebut->setDate(QDate::currentDate());
        eDateFin->setDate(QDate::currentDate().addDays(1));
        eStatus->setCurrentIndex(0);
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
        eDateDebut->setDate(rec.dateDebut.isValid() ? rec.dateDebut : QDate::currentDate());
        eDateFin->setDate(rec.dateFin.isValid() ? rec.dateFin : QDate::currentDate().addDays(1));
        if      (rec.status == "En cours")   eStatus->setCurrentIndex(1);
        else if (rec.status == "Concluante") eStatus->setCurrentIndex(2);
        else if (rec.status == "Réussie")    eStatus->setCurrentIndex(3);
        else if (rec.status == "Échouée")    eStatus->setCurrentIndex(4);
        else if (rec.status == "Archivée")   eStatus->setCurrentIndex(5);
        else                                 eStatus->setCurrentIndex(0);
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
        if (eName->text().trimmed().isEmpty()) { showToast(this, "Saisir le nom de l'expérience.", false); return; }
        if (eStatus->currentIndex() == 0)      { showToast(this, "Sélectionner un statut.", false); return; }
        ExperienceRecord rec;
        rec.id        = *expEditId;
        rec.titre     = eName->text().trimmed();
        rec.hypothese = eHypo->text().trimmed();
        rec.dateDebut = eDateDebut->date();
        rec.dateFin   = eDateFin->date();
        rec.status    = eStatus->currentText();
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
        maintDate->setDate(QDate::currentDate());
        calDate->setDate(QDate::currentDate());
        labRoom->setCurrentText("");
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
        if (rec.dateProchaineMaintenance.isValid()) maintDate->setDate(rec.dateProchaineMaintenance);
        if (rec.dateLimiteCalibration.isValid()) calDate->setDate(rec.dateLimiteCalibration);
        if (!rec.localisation.trimmed().isEmpty()) labRoom->setCurrentText(rec.localisation);
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
        if (fcb1->currentText().trimmed().isEmpty()) {
            showToast(this, "Le nom de l'équipement est obligatoire.", false);
            return;
        }

        EquipementRecord rec;
        rec.id = *eqEditId;
        rec.nomEquipement = fcb1->currentText().trimmed();
        rec.fabricant = fcb2->currentText().trimmed();
        rec.numeroModele = modelEdit->text().trimmed();
        rec.dateAchat = date->date();
        rec.dateDerniereMaintenance = QDate();
        rec.dateProchaineMaintenance = maintDate->date();
        rec.statut = fcb3->currentText();
        rec.localisation = labRoom->currentText().trimmed();
        rec.dateLimiteCalibration = calDate->date();
        rec.idExp = QVariant();

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
        if (selectedEquipementId() <= 0) {
            showToast(this, "Sélectionnez un équipement.", false);
            return;
        }
        setWindowTitle("Détails équipement");
        stack->setCurrentIndex(EQUIP_DETAILS);
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
    QObject::connect(empAdd, &QPushButton::clicked, this, [=](){
        setWindowTitle("Creer / Modifier Employe");
        stack->setCurrentIndex(EMP_FORM);
    });
    QObject::connect(empEdit, &QPushButton::clicked, this, [=](){
        setWindowTitle("Creer / Modifier Employe");
        stack->setCurrentIndex(EMP_FORM);
    });
    QObject::connect(empCancel, &QPushButton::clicked, this, [=](){
        setWindowTitle("Gestion des Employés");
        stack->setCurrentIndex(EMP_LIST);
    });
    QObject::connect(empSave, &QPushButton::clicked, this, [=](){
        setWindowTitle("Gestion des Employés");
        stack->setCurrentIndex(EMP_LIST);
        QMessageBox::information(this, "Employe", "Enregistrement (a connecter a la base de donnees).");
    });
    QObject::connect(empMore, &QPushButton::clicked, this, [=](){
        setWindowTitle("Affectations & Laboratoires");
        stack->setCurrentIndex(EMP_AFF);
    });
    QObject::connect(empBack3, &QPushButton::clicked, this, [=](){
        setWindowTitle("Gestion des Employés");
        stack->setCurrentIndex(EMP_LIST);
    });
    QObject::connect(empDetails3, &QPushButton::clicked, this, [=](){
        setWindowTitle("Disponibilites & Contraintes");
        stack->setCurrentIndex(EMP_AVAIL);
    });
    QObject::connect(empBack4, &QPushButton::clicked, this, [=](){
        setWindowTitle("Affectations & Laboratoires");
        stack->setCurrentIndex(EMP_AFF);
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
           QString resume = QString("CIN : %1 | Nom : %2 | Role : %3")
                            .arg(empTable->item(r,1)->text(),
                                empTable->item(r,2)->text(),
                                empTable->item(r,4)->text());
        ConfirmDeleteDialog confirm(style(), resume, this);
        if (confirm.exec() == QDialog::Accepted) {
            empTable->removeRow(r);
            updateEmpStatsFromTable();
        }
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

        Publication publication(
            sbPubId->value(),
            leTitle->text().trimmed(),
            leJournal->text().trimmed(),
            sbYear->value(),
            leDOI->text().trimmed(),
            cbStatus->currentText(),
            teAbstract->toPlainText().trimmed(),
            sbIdProjet->value(),
            sbEmployeeId->value()
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

    // STATS -> LIST




    setWindowTitle("SmartVision - Connexion");
    stack->setCurrentIndex(LOGIN);

}
