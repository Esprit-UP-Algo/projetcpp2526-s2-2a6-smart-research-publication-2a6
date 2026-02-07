// ===================== biosimple.cpp (UN SEUL FICHIER - BioSimple + Gestion Projet - 3 Widgets) =====================
#include "biosimple.h"
#include <QTextEdit>

#include <QPainterPath>
#include <QDialog>
#include <QMessageBox>

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
#include <QtMath>

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

// ===================== STACK INDEX =====================
// BioSimple (5 pages)
static const int BIO_LIST  = 0;
static const int BIO_FORM  = 1;
static const int BIO_LOC   = 2;
static const int BIO_RACK  = 3;
static const int BIO_STATS = 4;

// Gestion Projet (3 widgets/pages)
static const int PROJ_LIST  = 5; // Widget 1
static const int PROJ_FORM  = 6; // Widget 2
static const int PROJ_STATS = 7; // Widget 3

// Expériences / Protocoles (3 widgets/pages)
static const int EXP_LIST  = 8;  // Widget 1
static const int EXP_FORM  = 9;  // Widget 2
static const int EXP_STATS = 10; // Widget 3

// Publications (4 pages)

// ===================== Publications (4 pages) =====================
static const int PUB_LIST    = 11;  // Page 1 : Liste / Gestion
static const int PUB_FORM    = 12;  // Page 2 : Ajouter / Modifier
static const int PUB_STATS   = 13;  // Page 3 : Statistiques
static const int PUB_DETAILS = 14;  // Page 4 : Détails

// ===================== UI responsive margin =====================
static int uiMargin(QWidget* w)
{
    int W = w->width();
    if (W < 1100) return 6;
    if (W < 1400) return 10;
    return 14;
}

// ===================== Helpers =====================
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

static QFrame* softBox(QWidget* parent=nullptr)
{
    QFrame* f = new QFrame(parent);
    f->setStyleSheet(QString("QFrame{ background:%1; border: 1px solid %2; border-radius: 12px; }")
                         .arg(C_PANEL_IN, C_PANEL_BR));
    return f;
}

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

// ===================== LOGO TRÈS GRAND CENTRÉ =====================
// ===================== CENTERED LOGO CARD =====================
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

    QPixmap px(":/image/smartvision.png"); // vérifie ton chemin qrc

    if (!px.isNull())
    {
        logo->setPixmap(
            px.scaled(160, 160, Qt::KeepAspectRatio, Qt::SmoothTransformation)
            );
    }
    else
    {
        logo->setText("LOGO");
        logo->setStyleSheet("color:#bbb; font-size:18px;");
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
};

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
    out.bBioSimple   = modulePill("BioSimple",       selected == ModuleTab::BioSimple);
    out.bEquipement  = modulePill("Équipement",      selected == ModuleTab::Equipement);
    out.bExp         = modulePill("Expériences  Protocoles", selected == ModuleTab::ExperiencesProtocoles);
    out.bProjet      = modulePill("Gestion Projet",  selected == ModuleTab::GestionProjet);

    h->addWidget(out.bEmployee);
    h->addWidget(out.bPublication);
    h->addWidget(out.bBioSimple);
    h->addWidget(out.bEquipement);
    h->addWidget(out.bExp);
    h->addWidget(out.bProjet);
    h->addStretch(1);

    return out;
}

// ===================== TOPBAR (sans logo) =====================
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
static void connectModulesSwitch(MainWindow* self, QStackedWidget* stack, const ModulesBar& mb)
{
    // Modules activés dans ce fichier: BioSimple + Gestion Projet + Expériences/Protocoles
    QObject::connect(mb.bBioSimple, &QPushButton::clicked, self, [=](){
        self->setWindowTitle("Gestion des Échantillons");
        stack->setCurrentIndex(BIO_LIST);
    });

    QObject::connect(mb.bProjet, &QPushButton::clicked, self, [=](){
        self->setWindowTitle("Gestion Projet");
        stack->setCurrentIndex(PROJ_LIST);
    });

    QObject::connect(mb.bExp, &QPushButton::clicked, self, [=](){
        self->setWindowTitle("Expériences & Protocoles");
        stack->setCurrentIndex(EXP_LIST);
    });

    // Les autres modules: non activés
    auto notImpl = [=](const QString& name){
        QMessageBox::information(self, "Module",
                                 QString("Module \"%1\" non activé dans ce fichier.").arg(name));
    };

    QObject::connect(mb.bEmployee,    &QPushButton::clicked, self, [=]{ notImpl("Employés"); });
    QObject::connect(mb.bPublication, &QPushButton::clicked, self, [=](){
        self->setWindowTitle("Publications");
        stack->setCurrentIndex(PUB_LIST);
    });

    QObject::connect(mb.bEquipement,  &QPushButton::clicked, self, [=]{ notImpl("Équipements"); });
}


// ===================== Widget1 badge delegate (BioSimple) =====================
enum class ExpireStatus { Ok=0, Soon=1, Expired=2, Bsl=3 };

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

// ===================== Widget3 (BioSimple) gradient row =====================
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

// ===================== Widget4 helpers (BioSimple Rack) =====================
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

// ===================== Dialog: Confirm delete (design) =====================
class ConfirmDeleteDialog : public QDialog
{
public:
    ConfirmDeleteDialog(QStyle* st, const QString& lineText, QWidget* parent=nullptr)
        : QDialog(parent)
    {
        setModal(true);
        setWindowTitle("Confirmation");
        setFixedSize(520, 220);
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

        setStyleSheet(QString(R"(
            QDialog{
                background: %1;
                border: 1px solid %2;
                border-radius: 14px;
            }
            QLabel{ color: rgba(0,0,0,0.70); }
        )").arg(C_PANEL_BG, C_PANEL_BR));

        QVBoxLayout* root = new QVBoxLayout(this);
        root->setContentsMargins(16,16,16,16);
        root->setSpacing(12);

        QFrame* head = new QFrame;
        head->setFixedHeight(46);
        head->setStyleSheet(QString("background:%1; border-radius: 12px;").arg(C_TOPBAR));
        QHBoxLayout* hl = new QHBoxLayout(head);
        hl->setContentsMargins(12,8,12,8);

        QLabel* ic = new QLabel;
        ic->setPixmap(st->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(18,18));

        QLabel* t = new QLabel("Supprimer l’élément ?");
        QFont ft = t->font(); ft.setBold(true); ft.setPointSize(11);
        t->setFont(ft);
        t->setStyleSheet(QString("color:%1;").arg(C_BEIGE));

        hl->addWidget(ic);
        hl->addSpacing(8);
        hl->addWidget(t);
        hl->addStretch(1);

        root->addWidget(head);

        QFrame* body = new QFrame;
        body->setStyleSheet("QFrame{ background: rgba(255,255,255,0.70); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
        QVBoxLayout* bl = new QVBoxLayout(body);
        bl->setContentsMargins(14,12,14,12);
        bl->setSpacing(8);

        QLabel* msg = new QLabel("Voulez-vous vraiment supprimer la ligne sélectionnée ?");
        msg->setStyleSheet("color: rgba(0,0,0,0.62); font-weight: 900;");
        msg->setWordWrap(true);

        QLabel* details = new QLabel(lineText);
        details->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 800;");
        details->setWordWrap(true);

        bl->addWidget(msg);
        bl->addWidget(details);

        root->addWidget(body, 1);

        QHBoxLayout* btns = new QHBoxLayout;
        btns->setContentsMargins(0,0,0,0);
        btns->setSpacing(10);
        btns->addStretch(1);

        QPushButton* cancel = new QPushButton(st->standardIcon(QStyle::SP_DialogCancelButton), "  Annuler");
        cancel->setCursor(Qt::PointingHandCursor);
        cancel->setStyleSheet(QString(R"(
            QPushButton{
                background: rgba(255,255,255,0.65);
                border: 1px solid rgba(0,0,0,0.12);
                border-radius: 12px;
                padding: 10px 14px;
                font-weight: 900;
                color: %1;
            }
            QPushButton:hover{ background: rgba(255,255,255,0.80); }
        )").arg(C_TEXT_DARK));

        QPushButton* del = new QPushButton(st->standardIcon(QStyle::SP_TrashIcon), "  Supprimer");
        del->setCursor(Qt::PointingHandCursor);
        del->setStyleSheet(R"(
            QPushButton{
                background: rgba(177,74,74,0.85);
                border: 1px solid rgba(0,0,0,0.12);
                border-radius: 12px;
                padding: 10px 14px;
                font-weight: 900;
                color: white;
            }
            QPushButton:hover{ background: rgba(177,74,74,0.95); }
        )");

        QObject::connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
        QObject::connect(del, &QPushButton::clicked, this, &QDialog::accept);

        btns->addWidget(cancel);
        btns->addWidget(del);

        root->addLayout(btns);
    }
};

// ===================== MainWindow =====================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    resize(1320, 680);

    QWidget* root = new QWidget(this);
    root->setObjectName("root");
    setCentralWidget(root);

    QStyle* st = style();

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

    QVBoxLayout* rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(0,0,0,0);

    QStackedWidget* stack = new QStackedWidget;
    rootLayout->addWidget(stack);

    // ==========================================================
    // PAGE 0 : BioSimple - Gestion des Échantillons (LIST)
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
    search->setPlaceholderText("Rechercher (ID, type, organisme, stockage, BSL...)");
    search->addAction(st->standardIcon(QStyle::SP_FileDialogContentsView), QLineEdit::LeadingPosition);

    QComboBox* cbType = new QComboBox; cbType->addItems({"Type", "DNA", "RNA", "Protéine"});
    QComboBox* cbTemp = new QComboBox; cbTemp->addItems({"Température", "-80°C", "-20°C", "Temp. ambiante"});
    QComboBox* cbBsl  = new QComboBox; cbBsl->addItems({"BSL", "BSL-1", "BSL-2", "BSL-3"});

    QPushButton* filters = new QPushButton(st->standardIcon(QStyle::SP_FileDialogDetailedView), "  Filtres");
    filters->setCursor(Qt::PointingHandCursor);
    filters->setStyleSheet(QString(R"(
        QPushButton{
            background:%1; color: rgba(255,255,255,0.92);
            border:1px solid rgba(0,0,0,0.18);
            border-radius: 12px; padding: 10px 16px; font-weight: 800;
        }
        QPushButton:hover{ background: %2; }
    )").arg(C_PRIMARY, C_TOPBAR));

    bar1L->addWidget(search, 1);
    bar1L->addWidget(cbType);
    bar1L->addWidget(cbTemp);
    bar1L->addWidget(cbBsl);
    bar1L->addWidget(filters);
    p1->addWidget(bar1);

    QFrame* card1 = makeCard();
    QVBoxLayout* card1L = new QVBoxLayout(card1);
    card1L->setContentsMargins(10,10,10,10);

    QTableWidget* table = new QTableWidget(5, 8);
    table->setHorizontalHeaderLabels({"", "Type", "Organisme", "Stockage", "Quantité", "Date d'expiration", "Statut", "BSL"});
    table->verticalHeader()->setVisible(false);
    table->setShowGrid(true);
    table->setAlternatingRowColors(true);
    table->setStyleSheet(QString("QTableWidget{ alternate-background-color:%1; background-color:%2; }").arg(C_ROW_EVEN, C_ROW_ODD));
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setItemDelegateForColumn(6, new BadgeDelegate(table));

    table->setColumnWidth(0, 36);
    table->setColumnWidth(1, 110);
    table->setColumnWidth(2, 140);
    table->setColumnWidth(3, 170);
    table->setColumnWidth(4, 120);
    table->setColumnWidth(5, 160);
    table->setColumnWidth(6, 150);
    table->setColumnWidth(7, 90);

    auto setRow=[&](int r,
                      const QString& type,
                      const QString& org,
                      const QString& storage,
                      const QString& qty,
                      const QString& date,
                      ExpireStatus stt,
                      const QString& bsl)
    {
        QTableWidgetItem* iconItem = new QTableWidgetItem;
        iconItem->setIcon(st->standardIcon(QStyle::SP_ArrowRight));
        iconItem->setTextAlignment(Qt::AlignCenter);
        table->setItem(r, 0, iconItem);

        auto mk = [&](const QString& t){
            QTableWidgetItem* it = new QTableWidgetItem(t);
            it->setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);
            return it;
        };

        table->setItem(r, 1, mk(type));
        table->setItem(r, 2, mk(org));
        table->setItem(r, 3, mk(storage));

        QTableWidgetItem* q = mk(qty);
        q->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
        table->setItem(r, 4, q);

        table->setItem(r, 5, mk(date));

        QTableWidgetItem* badge = new QTableWidgetItem;
        badge->setData(Qt::UserRole, (int)stt);
        table->setItem(r, 6, badge);

        table->setItem(r, 7, mk(bsl));
        table->setRowHeight(r, 46);
    };

    setRow(0,  "DNA",      "E. coli", "-80°C",           "150", "16/05/2024", ExpireStatus::Ok,      "BSL-1");
    setRow(1,  "RNA",      "Humain",  "-20°C",            "75", "01/05/2024", ExpireStatus::Soon,    "BSL-2");
    setRow(2,  "Protéine", "Levure",  "-20°C",            "40", "20/04/2024", ExpireStatus::Expired, "BSL-3");
    setRow(3,  "DNA",      "Souris",  "Temp. ambiante",    "5", "24/04/2024", ExpireStatus::Ok,      "BSL-2");
    setRow(4,  "DNA",      "Humain",  "-80°C",           "200", "20/02/2024", ExpireStatus::Bsl,     "BSL-3");

    card1L->addWidget(table);
    p1->addWidget(card1, 1);

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

    QObject::connect(btnDel, &QPushButton::clicked, this, [=](){
        int r = table->currentRow();
        if (r < 0) {
            QMessageBox::information(this, "Information", "Veuillez sélectionner une ligne à supprimer.");
            return;
        }
        auto cell = [&](int c)->QString{
            QTableWidgetItem* it = table->item(r, c);
            return it ? it->text() : "";
        };
        QString resume = QString("Type : %1   |   Organisme : %2   |   Stockage : %3   |   Expiration : %4")
                             .arg(cell(1), cell(2), cell(3), cell(5));
        ConfirmDeleteDialog confirm(style(), resume, this);
        if (confirm.exec() == QDialog::Accepted) table->removeRow(r);
    });

    bottom1L->addWidget(btnAdd);
    bottom1L->addWidget(btnEdit);
    bottom1L->addWidget(btnDel);
    bottom1L->addWidget(btnStats);
    bottom1L->addStretch(1);

    bottom1L->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_DirIcon)));
    bottom1L->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_FileIcon)));
    bottom1L->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_DialogSaveButton)));
    bottom1L->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_BrowserReload)));

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

    auto blueDateRow = [&](const QString& label, const QDate& defDate){
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
        return r;
    };

    left2L->addWidget(sectionTitle("Collection"));
    QLineEdit* leCollection = new QLineEdit;
    leCollection->setPlaceholderText("Nom de la collection");
    left2L->addWidget(formRow(QStyle::SP_DirIcon, "Collection", leCollection));

    left2L->addWidget(sectionTitle("Dates"));
    left2L->addWidget(blueDateRow("Date de collecte", QDate::currentDate()));
    left2L->addWidget(blueDateRow("Date d'expiration", QDate::currentDate().addDays(30)));

    QSpinBox* qty = new QSpinBox;
    qty->setRange(0, 999999);
    qty->setValue(0);
    qty->setFixedWidth(120);
    qty->setStyleSheet("QSpinBox{ background: transparent; border:0; font-weight:900; color: rgba(0,0,0,0.60); }");
    left2L->addWidget(formRow(QStyle::SP_ArrowUp, "Quantité", qty));

    QComboBox* cbTemp2 = new QComboBox;
    cbTemp2->addItems({"Température", "-80°C", "-20°C", "Temp. ambiante"});
    cbTemp2->setFixedWidth(170);
    left2L->addWidget(formRow(QStyle::SP_BrowserStop, "Température", cbTemp2));

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

    QComboBox* cbType2 = new QComboBox;
    cbType2->addItems({"Type", "DNA", "RNA", "Protéine"});
    cbType2->setFixedWidth(200);
    right2L->addWidget(formRow(QStyle::SP_FileIcon, "Type", cbType2));

    QComboBox* cbOrg2 = new QComboBox;
    cbOrg2->addItems({"Organisme", "Humain", "Souris", "Levure", "E. coli"});
    cbOrg2->setFixedWidth(200);
    right2L->addWidget(formRow(QStyle::SP_DirIcon, "Organisme", cbOrg2));

    QComboBox* cbFreezer2 = new QComboBox;
    cbFreezer2->addItems({"Congélateur", "Congélateur-01", "Congélateur-02", "Congélateur-03"});
    cbFreezer2->setFixedWidth(200);
    right2L->addWidget(formRow(QStyle::SP_DriveHDIcon, "Congélateur", cbFreezer2));

    QSpinBox* sbShelf2 = new QSpinBox;
    sbShelf2->setRange(1, 99);
    sbShelf2->setValue(1);
    sbShelf2->setFixedWidth(120);
    sbShelf2->setStyleSheet("QSpinBox{ background: transparent; border:0; font-weight:900; color: rgba(0,0,0,0.60); }");
    right2L->addWidget(formRow(QStyle::SP_FileDialogListView, "Étagère", sbShelf2));

    QSpinBox* sbPos2 = new QSpinBox;
    sbPos2->setRange(1, 999);
    sbPos2->setValue(1);
    sbPos2->setFixedWidth(120);
    sbPos2->setStyleSheet("QSpinBox{ background: transparent; border:0; font-weight:900; color: rgba(0,0,0,0.60); }");
    right2L->addWidget(formRow(QStyle::SP_ArrowDown, "Position", sbPos2));

    right2L->addStretch(1);

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

    auto chip = [&](const QString& t){
        QLabel* c = new QLabel(t);
        c->setStyleSheet("background: rgba(255,255,255,0.90); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; padding: 8px 12px; font-weight:900; color: rgba(0,0,0,0.55);");
        return c;
    };

    header3L->addWidget(details3);
    header3L->addStretch(1);
    header3L->addWidget(chip("Enregistrements"));
    header3L->addWidget(chip("Événements"));

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
    gp1->addWidget(makeHeaderBlock(st, "Gestion Projet", ModuleTab::GestionProjet, &barProjList));
    connectModulesSwitch(this, stack, barProjList);

    QFrame* pBar = new QFrame;
    pBar->setFixedHeight(54);
    pBar->setStyleSheet("background: rgba(255,255,255,0.22); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* pBarL = new QHBoxLayout(pBar);
    pBarL->setContentsMargins(14, 8, 14, 8);
    pBarL->setSpacing(10);

    QLineEdit* pSearch = new QLineEdit;
    pSearch->setPlaceholderText("Rechercher (ID, nom, statut, responsable...)");
    pSearch->addAction(st->standardIcon(QStyle::SP_FileDialogContentsView), QLineEdit::LeadingPosition);

    QComboBox* pStatut = new QComboBox;
    pStatut->addItems({"Statut", "Planifié", "En cours", "Terminé", "En retard"});

    QComboBox* pPriorite = new QComboBox;
    pPriorite->addItems({"Priorité", "Haute", "Moyenne", "Basse"});

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
    pBarL->addWidget(pStatut);
    pBarL->addWidget(pPriorite);
    pBarL->addWidget(pFilters);
    gp1->addWidget(pBar);

    QFrame* projCard = makeCard();
    QVBoxLayout* projCardL = new QVBoxLayout(projCard);
    projCardL->setContentsMargins(10,10,10,10);

    QTableWidget* projTable = new QTableWidget(5, 7);
    projTable->setHorizontalHeaderLabels({"ID","Nom","Domaine","Responsable","Budget","Début","Statut"});
    projTable->verticalHeader()->setVisible(false);
    projTable->horizontalHeader()->setStretchLastSection(true);
    projTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    projTable->setSelectionMode(QAbstractItemView::SingleSelection);
    projTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto putP=[&](int r,int c,const QString& v){
        QTableWidgetItem* it=new QTableWidgetItem(v);
        it->setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);
        projTable->setItem(r,c,it);
    };

    putP(0,0,"P-001"); putP(0,1,"Projet Vaccin");        putP(0,2,"Biotech");   putP(0,3,"Dr. Amal");   putP(0,4,"120000"); putP(0,5,"01/02/2026"); putP(0,6,"En cours");
    putP(1,0,"P-002"); putP(1,1,"Génomique");            putP(1,2,"Recherche"); putP(1,3,"Dr. Nader");  putP(1,4,"80000");  putP(1,5,"15/01/2026"); putP(1,6,"Planifié");
    putP(2,0,"P-003"); putP(2,1,"Culture Cellulaire");   putP(2,2,"Laboratoire");putP(2,3,"Mme. Sara");  putP(2,4,"45000");  putP(2,5,"10/12/2025"); putP(2,6,"Terminé");
    putP(3,0,"P-004"); putP(3,1,"Essais Cliniques");     putP(3,2,"Santé");     putP(3,3,"Dr. Hichem"); putP(3,4,"200000"); putP(3,5,"05/11/2025"); putP(3,6,"En cours");
    putP(4,0,"P-005"); putP(4,1,"Optimisation Stockage");putP(4,2,"Qualité");   putP(4,3,"M. Karim");   putP(4,4,"20000");  putP(4,5,"01/10/2025"); putP(4,6,"En retard");

    for(int r=0;r<projTable->rowCount();++r) projTable->setRowHeight(r, 46);

    projCardL->addWidget(projTable);
    gp1->addWidget(projCard, 1);

    QFrame* projBottom = new QFrame;
    projBottom->setFixedHeight(64);
    projBottom->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* projBottomL = new QHBoxLayout(projBottom);
    projBottomL->setContentsMargins(14,10,14,10);
    projBottomL->setSpacing(12);

    QPushButton* projAdd   = actionBtn("Ajouter",      "rgba(10,95,88,0.45)", "rgba(255,255,255,0.90)", st->standardIcon(QStyle::SP_DialogYesButton), true);
    QPushButton* projEdit  = actionBtn("Modifier",     "rgba(198,178,154,0.55)", "rgba(255,255,255,0.85)", st->standardIcon(QStyle::SP_FileDialogContentsView), true);
    QPushButton* projDel   = actionBtn("Supprimer",    "rgba(255,255,255,0.55)", "#B14A4A", st->standardIcon(QStyle::SP_TrashIcon), true);
    QPushButton* projStatB = actionBtn("Statistiques", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_ComputerIcon), true);

    QObject::connect(projDel, &QPushButton::clicked, this, [=](){
        int r = projTable->currentRow();
        if (r < 0) {
            QMessageBox::information(this, "Information", "Veuillez sélectionner une ligne à supprimer.");
            return;
        }
        QString resume = QString("ID : %1 | Projet : %2 | Statut : %3")
                             .arg(projTable->item(r,0)->text(),
                                  projTable->item(r,1)->text(),
                                  projTable->item(r,6)->text());
        ConfirmDeleteDialog confirm(style(), resume, this);
        if (confirm.exec() == QDialog::Accepted) projTable->removeRow(r);
    });

    projBottomL->addWidget(projAdd);
    projBottomL->addWidget(projEdit);
    projBottomL->addWidget(projDel);
    projBottomL->addWidget(projStatB);
    projBottomL->addStretch(1);
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

    QLineEdit* projId = new QLineEdit;
    projId->setPlaceholderText("P-XXX");
    projId->setFixedWidth(160);

    QLineEdit* projName = new QLineEdit;
    projName->setPlaceholderText("Nom du projet");

    QComboBox* projDomain = new QComboBox;
    projDomain->addItems({"Domaine", "Biotech", "Recherche", "Qualité", "Santé", "Laboratoire"});
    projDomain->setFixedWidth(200);

    QComboBox* projStatus = new QComboBox;
    projStatus->addItems({"Statut", "Planifié", "En cours", "Terminé", "En retard"});
    projStatus->setFixedWidth(200);

    p2LeftL->addWidget(projTitle("Informations"));
    p2LeftL->addWidget(projRow(QStyle::SP_FileIcon, "ID", projId));
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
    ep1->addWidget(makeHeaderBlock(st, "Expériences & Protocoles", ModuleTab::ExperiencesProtocoles, &barExpList));
    connectModulesSwitch(this, stack, barExpList);

    QFrame* eBar = new QFrame;
    eBar->setFixedHeight(54);
    eBar->setStyleSheet("background: rgba(255,255,255,0.22); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* eBarL = new QHBoxLayout(eBar);
    eBarL->setContentsMargins(14, 8, 14, 8);
    eBarL->setSpacing(10);

    QLineEdit* eSearch = new QLineEdit;
    eSearch->setPlaceholderText("Rechercher (ID, expérience, protocole, responsable, statut...)");
    eSearch->addAction(st->standardIcon(QStyle::SP_FileDialogContentsView), QLineEdit::LeadingPosition);

    QComboBox* eStatut = new QComboBox;
    eStatut->addItems({"Statut", "Planifiée", "En cours", "Terminée", "Suspendue"});

    QComboBox* eBsl = new QComboBox;
    eBsl->addItems({"BSL", "BSL-1", "BSL-2", "BSL-3"});

    QPushButton* eFilters = new QPushButton(st->standardIcon(QStyle::SP_FileDialogDetailedView), "  Filtres");
    eFilters->setCursor(Qt::PointingHandCursor);
    eFilters->setStyleSheet(QString(R"(
    QPushButton{
        background:%1; color: rgba(255,255,255,0.92);
        border:1px solid rgba(0,0,0,0.18);
        border-radius: 12px; padding: 10px 16px; font-weight: 800;
    }
    QPushButton:hover{ background: %2; }
)").arg(C_PRIMARY, C_TOPBAR));

    eBarL->addWidget(eSearch, 1);
    eBarL->addWidget(eStatut);
    eBarL->addWidget(eBsl);
    eBarL->addWidget(eFilters);
    ep1->addWidget(eBar);

    QFrame* expCard = makeCard();
    QVBoxLayout* expCardL = new QVBoxLayout(expCard);
    expCardL->setContentsMargins(10,10,10,10);

    QTableWidget* expTable = new QTableWidget(6, 7);
    expTable->setHorizontalHeaderLabels({"ID", "Expérience", "Protocole", "Responsable", "Date", "Statut", "BSL"});
    expTable->verticalHeader()->setVisible(false);
    expTable->horizontalHeader()->setStretchLastSection(true);
    expTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    expTable->setSelectionMode(QAbstractItemView::SingleSelection);
    expTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto putE = [&](int r,int c,const QString& v){
        QTableWidgetItem* it = new QTableWidgetItem(v);
        it->setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);
        expTable->setItem(r,c,it);
    };

    putE(0,0,"E-001"); putE(0,1,"Culture cellulaire"); putE(0,2,"PROTO-CC-01"); putE(0,3,"Dr. Amal");  putE(0,4,"07/02/2026"); putE(0,5,"En cours");   putE(0,6,"BSL-2");
    putE(1,0,"E-002"); putE(1,1,"Extraction ADN");     putE(1,2,"PROTO-DNA-03");putE(1,3,"Mme. Sara"); putE(1,4,"05/02/2026"); putE(1,5,"Terminée"); putE(1,6,"BSL-1");
    putE(2,0,"E-003"); putE(2,1,"PCR temps réel");     putE(2,2,"PROTO-PCR-07");putE(2,3,"Dr. Nader"); putE(2,4,"10/02/2026"); putE(2,5,"Planifiée");putE(2,6,"BSL-2");
    putE(3,0,"E-004"); putE(3,1,"Séquençage");         putE(3,2,"PROTO-SEQ-02");putE(3,3,"Dr. Hichem");putE(3,4,"28/01/2026"); putE(3,5,"Suspendue");putE(3,6,"BSL-3");
    putE(4,0,"E-005"); putE(4,1,"Dosage protéique");   putE(4,2,"PROTO-PROT-01");putE(4,3,"M. Karim"); putE(4,4,"02/02/2026"); putE(4,5,"En cours");  putE(4,6,"BSL-1");
    putE(5,0,"E-006"); putE(5,1,"Stérilité");          putE(5,2,"PROTO-STER-04");putE(5,3,"Dr. Amal");  putE(5,4,"01/02/2026"); putE(5,5,"Terminée"); putE(5,6,"BSL-2");

    for(int r=0;r<expTable->rowCount();++r) expTable->setRowHeight(r, 46);

    expCardL->addWidget(expTable);
    ep1->addWidget(expCard, 1);

    QFrame* expBottom = new QFrame;
    expBottom->setFixedHeight(64);
    expBottom->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* expBottomL = new QHBoxLayout(expBottom);
    expBottomL->setContentsMargins(14,10,14,10);
    expBottomL->setSpacing(12);

    QPushButton* expAdd   = actionBtn("Ajouter",      "rgba(10,95,88,0.45)", "rgba(255,255,255,0.90)", st->standardIcon(QStyle::SP_DialogYesButton), true);
    QPushButton* expEdit  = actionBtn("Modifier",     "rgba(198,178,154,0.55)", "rgba(255,255,255,0.85)", st->standardIcon(QStyle::SP_FileDialogContentsView), true);
    QPushButton* expDel   = actionBtn("Supprimer",    "rgba(255,255,255,0.55)", "#B14A4A", st->standardIcon(QStyle::SP_TrashIcon), true);
    QPushButton* expStats = actionBtn("Statistiques", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_ComputerIcon), true);

    QObject::connect(expDel, &QPushButton::clicked, this, [=](){
        int r = expTable->currentRow();
        if (r < 0) {
            QMessageBox::information(this, "Information", "Veuillez sélectionner une ligne à supprimer.");
            return;
        }
        QString resume = QString("ID : %1 | Expérience : %2 | Protocole : %3")
                             .arg(expTable->item(r,0)->text(), expTable->item(r,1)->text(), expTable->item(r,2)->text());
        ConfirmDeleteDialog confirm(style(), resume, this);
        if (confirm.exec() == QDialog::Accepted) expTable->removeRow(r);
    });

    expBottomL->addWidget(expAdd);
    expBottomL->addWidget(expEdit);
    expBottomL->addWidget(expDel);
    expBottomL->addWidget(expStats);
    expBottomL->addStretch(1);

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

    QFrame* e2Left = softBox();
    e2Left->setFixedWidth(420);
    QVBoxLayout* e2LeftL = new QVBoxLayout(e2Left);
    e2LeftL->setContentsMargins(12,12,12,12);
    e2LeftL->setSpacing(10);

    QLineEdit* eId = new QLineEdit;
    eId->setPlaceholderText("E-XXX");
    eId->setFixedWidth(160);

    QLineEdit* eName = new QLineEdit;
    eName->setPlaceholderText("Nom de l’expérience");

    QLineEdit* eProto = new QLineEdit;
    eProto->setPlaceholderText("Code du protocole (ex: PROTO-PCR-07)");

    QComboBox* eStatus = new QComboBox;
    eStatus->addItems({"Statut", "Planifiée", "En cours", "Terminée", "Suspendue"});
    eStatus->setFixedWidth(200);

    QComboBox* eBsl2 = new QComboBox;
    eBsl2->addItems({"BSL", "BSL-1", "BSL-2", "BSL-3"});
    eBsl2->setFixedWidth(200);

    QDateEdit* eDate = new QDateEdit(QDate::currentDate());
    eDate->setCalendarPopup(true);
    eDate->setDisplayFormat("dd/MM/yyyy");
    eDate->setStyleSheet("QDateEdit{ background: rgba(255,255,255,0.65); border: 1px solid rgba(0,0,0,0.15); border-radius: 12px; padding: 10px 14px; color: rgba(0,0,0,0.65); font-weight: 900; }");

    e2LeftL->addWidget(expTitle("Informations"));
    e2LeftL->addWidget(expRow(QStyle::SP_FileIcon, "ID", eId));
    e2LeftL->addWidget(expRow(QStyle::SP_DirIcon,  "Expérience", eName));
    e2LeftL->addWidget(expRow(QStyle::SP_FileDialogDetailedView, "Protocole", eProto));
    e2LeftL->addWidget(expRow(QStyle::SP_FileDialogInfoView, "Date", eDate));
    e2LeftL->addWidget(expRow(QStyle::SP_MessageBoxInformation, "Statut", eStatus));
    e2LeftL->addWidget(expRow(QStyle::SP_MessageBoxWarning, "Niveau BSL", eBsl2));
    e2LeftL->addStretch(1);

    QFrame* e2Right = softBox();
    QVBoxLayout* e2RightL = new QVBoxLayout(e2Right);
    e2RightL->setContentsMargins(12,12,12,12);
    e2RightL->setSpacing(10);

    QLineEdit* eOwner = new QLineEdit;
    eOwner->setPlaceholderText("Responsable");

    QSpinBox* eDuree = new QSpinBox;
    eDuree->setRange(0, 9999);
    eDuree->setValue(0);
    eDuree->setSuffix(" min");
    eDuree->setFixedWidth(180);
    eDuree->setStyleSheet("QSpinBox{ background: rgba(255,255,255,0.65); border: 1px solid rgba(0,0,0,0.15); border-radius: 12px; padding: 10px 14px; color: rgba(0,0,0,0.65); font-weight: 900; }");

    QLineEdit* eLieu = new QLineEdit;
    eLieu->setPlaceholderText("Lieu / Salle / Paillasse");

    QTextEdit* eNotes = new QTextEdit;
    eNotes->setPlaceholderText("Notes / étapes / observations (traçabilité)");
    eNotes->setStyleSheet("QTextEdit{ background: rgba(255,255,255,0.65); border: 1px solid rgba(0,0,0,0.15); border-radius: 12px; padding: 10px 14px; color: rgba(0,0,0,0.65); font-weight: 800; }");

    e2RightL->addWidget(expTitle("Traçabilité"));
    e2RightL->addWidget(expRow(QStyle::SP_DirHomeIcon, "Responsable", eOwner));
    e2RightL->addWidget(expRow(QStyle::SP_ArrowUp, "Durée", eDuree));
    e2RightL->addWidget(expRow(QStyle::SP_DriveHDIcon, "Lieu", eLieu));
    e2RightL->addWidget(eNotes, 1);

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
    pubSearch->setPlaceholderText("Rechercher (titre, auteur, journal, année, DOI...)");
    pubSearch->addAction(st->standardIcon(QStyle::SP_FileDialogContentsView), QLineEdit::LeadingPosition);

    QComboBox* pubType = new QComboBox;
    pubType->addItems({"Type", "Article", "Conférence", "Poster", "Thèse"});

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

    QTableWidget* pubTable = new QTableWidget(6, 7);
    pubTable->setHorizontalHeaderLabels({"ID","Titre","Auteurs","Journal/Conf.","Année","DOI","Statut"});
    pubTable->verticalHeader()->setVisible(false);
    pubTable->horizontalHeader()->setStretchLastSection(true);
    pubTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    pubTable->setSelectionMode(QAbstractItemView::SingleSelection);
    pubTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto putPUB = [&](int r,int c,const QString& v){
        QTableWidgetItem* it = new QTableWidgetItem(v);
        it->setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);
        pubTable->setItem(r,c,it);
    };

    putPUB(0,0,"PUB-001"); putPUB(0,1,"Analyse génomique des souches"); putPUB(0,2,"A. Ben Ali; S. Trabelsi"); putPUB(0,3,"Nature Methods"); putPUB(0,4,"2026"); putPUB(0,5,"10.1000/xyz001"); putPUB(0,6,"Soumise");
    putPUB(1,0,"PUB-002"); putPUB(1,1,"PCR temps réel : optimisation"); putPUB(1,2,"H. Chaachou; M. Karim"); putPUB(1,3,"BioTech Conf"); putPUB(1,4,"2025"); putPUB(1,5,"10.1000/xyz002"); putPUB(1,6,"Acceptée");
    putPUB(2,0,"PUB-003"); putPUB(2,1,"Culture cellulaire avancée"); putPUB(2,2,"Dr. Amal; Dr. Nader"); putPUB(2,3,"Cell Reports"); putPUB(2,4,"2025"); putPUB(2,5,"10.1000/xyz003"); putPUB(2,6,"Publiée");
    putPUB(3,0,"PUB-004"); putPUB(3,1,"Traçabilité scientifique en labo"); putPUB(3,2,"S. Sara; H. Hichem"); putPUB(3,3,"Lab Quality"); putPUB(3,4,"2024"); putPUB(3,5,"10.1000/xyz004"); putPUB(3,6,"Brouillon");
    putPUB(4,0,"PUB-005"); putPUB(4,1,"Séquençage : protocole comparatif"); putPUB(4,2,"A. Ben Ali; K. Yassmine"); putPUB(4,3,"Genomics Journal"); putPUB(4,4,"2024"); putPUB(4,5,"10.1000/xyz005"); putPUB(4,6,"Soumise");
    putPUB(5,0,"PUB-006"); putPUB(5,1,"Dosage protéique : étude"); putPUB(5,2,"M. Karim; Dr. Amal"); putPUB(5,3,"Protein Conf"); putPUB(5,4,"2023"); putPUB(5,5,"10.1000/xyz006"); putPUB(5,6,"Rejetée");

    for(int r=0;r<pubTable->rowCount();++r) pubTable->setRowHeight(r, 46);

    pubCardL->addWidget(pubTable);
    pb1->addWidget(pubCard, 1);

    // Bottom actions (Ajouter / Modifier / Stats / Retour optionnel)
    QFrame* pubBottom = new QFrame;
    pubBottom->setFixedHeight(64);
    pubBottom->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* pubBottomL = new QHBoxLayout(pubBottom);
    pubBottomL->setContentsMargins(14,10,14,10);
    pubBottomL->setSpacing(12);

    QPushButton* pubAdd   = actionBtn("Ajouter",      "rgba(10,95,88,0.45)", "rgba(255,255,255,0.90)", st->standardIcon(QStyle::SP_DialogYesButton), true);
    QPushButton* pubEdit  = actionBtn("Modifier",     "rgba(198,178,154,0.55)", "rgba(255,255,255,0.85)", st->standardIcon(QStyle::SP_FileDialogContentsView), true);
    QPushButton* pubDel   = actionBtn("Supprimer",    "rgba(255,255,255,0.55)", "#B14A4A", st->standardIcon(QStyle::SP_TrashIcon), true);
    QPushButton* pubStats = actionBtn("Statistiques", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_ComputerIcon), true);

    QObject::connect(pubDel, &QPushButton::clicked, this, [=](){
        int r = pubTable->currentRow();
        if (r < 0) {
            QMessageBox::information(this, "Information", "Veuillez sélectionner une publication à supprimer.");
            return;
        }
        QString resume = QString("ID : %1 | Titre : %2 | Année : %3")
                             .arg(pubTable->item(r,0)->text(),
                                  pubTable->item(r,1)->text(),
                                  pubTable->item(r,4)->text());
        ConfirmDeleteDialog confirm(style(), resume, this);
        if (confirm.exec() == QDialog::Accepted) pubTable->removeRow(r);
    });

    pubBottomL->addWidget(pubAdd);
    pubBottomL->addWidget(pubEdit);
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

    QLineEdit* lePubId = new QLineEdit;
    lePubId->setPlaceholderText("PUB-XXX");
    lePubId->setFixedWidth(180);

    QLineEdit* leTitle = new QLineEdit;
    leTitle->setPlaceholderText("Titre de la publication");

    QLineEdit* leAuthors = new QLineEdit;
    leAuthors->setPlaceholderText("Auteurs (séparés par ; )");

    QComboBox* cbPubType = new QComboBox;
    cbPubType->addItems({"Type", "Article", "Conférence", "Poster", "Thèse"});
    cbPubType->setFixedWidth(220);

    QSpinBox* sbYear = new QSpinBox;
    sbYear->setRange(1950, 2100);
    sbYear->setValue(QDate::currentDate().year());
    sbYear->setFixedWidth(180);
    sbYear->setStyleSheet("QSpinBox{ background: rgba(255,255,255,0.65); border: 1px solid rgba(0,0,0,0.15); border-radius: 12px; padding: 10px 14px; color: rgba(0,0,0,0.65); font-weight: 900; }");

    pub2LeftL->addWidget(pubTitle("Informations"));
    pub2LeftL->addWidget(pubRow(QStyle::SP_FileIcon, "ID", lePubId));
    pub2LeftL->addWidget(pubRow(QStyle::SP_FileDialogDetailedView, "Titre", leTitle));
    pub2LeftL->addWidget(pubRow(QStyle::SP_DirIcon, "Auteurs", leAuthors));
    pub2LeftL->addWidget(pubRow(QStyle::SP_ComputerIcon, "Type", cbPubType));
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

    QTextEdit* teAbstract = new QTextEdit;
    teAbstract->setPlaceholderText("Résumé / Notes / Mots-clés (traçabilité scientifique)");
    teAbstract->setStyleSheet("QTextEdit{ background: rgba(255,255,255,0.65); border: 1px solid rgba(0,0,0,0.15); border-radius: 12px; padding: 10px 14px; color: rgba(0,0,0,0.65); font-weight: 800; }");

    pub2RightL->addWidget(pubTitle("Détails"));
    pub2RightL->addWidget(pubRow(QStyle::SP_DirHomeIcon, "Journal/Conf.", leJournal));
    pub2RightL->addWidget(pubRow(QStyle::SP_FileDialogContentsView, "DOI", leDOI));
    pub2RightL->addWidget(pubRow(QStyle::SP_MessageBoxInformation, "Statut", cbStatus));
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

    // ==========================================================
    // NAVIGATION BioSimple
    // ==========================================================
    QObject::connect(btnAdd,  &QPushButton::clicked, this, [=]{ setWindowTitle("Ajouter / Modifier un échantillon"); stack->setCurrentIndex(BIO_FORM); });
    QObject::connect(btnEdit, &QPushButton::clicked, this, [=]{ setWindowTitle("Ajouter / Modifier un échantillon"); stack->setCurrentIndex(BIO_FORM); });

    QObject::connect(cancelBtn, &QPushButton::clicked, this, [=]{ setWindowTitle("Gestion des Échantillons"); stack->setCurrentIndex(BIO_LIST); });
    QObject::connect(saveBtn,   &QPushButton::clicked, this, [=]{ setWindowTitle("Gestion des Échantillons"); stack->setCurrentIndex(BIO_LIST); });

    QObject::connect(btnMore, &QPushButton::clicked, this, [=]{ setWindowTitle("Localisation & Stockage"); stack->setCurrentIndex(BIO_LOC); });
    QObject::connect(back3,   &QPushButton::clicked, this, [=]{ setWindowTitle("Gestion des Échantillons"); stack->setCurrentIndex(BIO_LIST); });

    QObject::connect(details3, &QPushButton::clicked, this, [=]{ setWindowTitle("Localisation & Stockage"); stack->setCurrentIndex(BIO_RACK); });
    QObject::connect(back4,    &QPushButton::clicked, this, [=]{ setWindowTitle("Localisation & Stockage"); stack->setCurrentIndex(BIO_LOC); });

    QObject::connect(btnSec,   &QPushButton::clicked, this, [=]{ setWindowTitle("Statistiques"); stack->setCurrentIndex(BIO_STATS); });
    QObject::connect(btnStats, &QPushButton::clicked, this, [=]{ setWindowTitle("Statistiques"); stack->setCurrentIndex(BIO_STATS); });
    QObject::connect(back5,    &QPushButton::clicked, this, [=]{ setWindowTitle("Gestion des Échantillons"); stack->setCurrentIndex(BIO_LIST); });

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
    QObject::connect(projStatB, &QPushButton::clicked, this, [=](){
        setWindowTitle("Statistiques Projet");
        stack->setCurrentIndex(PROJ_STATS);
    });
    QObject::connect(p3Back, &QPushButton::clicked, this, [=](){
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
        setWindowTitle("Ajouter / Modifier une expérience");
        stack->setCurrentIndex(EXP_FORM);
    });
    QObject::connect(expEdit, &QPushButton::clicked, this, [=](){
        setWindowTitle("Ajouter / Modifier une expérience");
        stack->setCurrentIndex(EXP_FORM);
    });
    QObject::connect(expCancel, &QPushButton::clicked, this, [=](){
        setWindowTitle("Expériences & Protocoles");
        stack->setCurrentIndex(EXP_LIST);
    });
    QObject::connect(expSave, &QPushButton::clicked, this, [=](){
        setWindowTitle("Expériences & Protocoles");
        stack->setCurrentIndex(EXP_LIST);
        QMessageBox::information(this, "Expérience", "Enregistrement (à connecter à la base de données).");
    });
    QObject::connect(expStats, &QPushButton::clicked, this, [=](){
        setWindowTitle("Statistiques Expériences");
        stack->setCurrentIndex(EXP_STATS);
    });
    QObject::connect(expBackStats, &QPushButton::clicked, this, [=](){
        setWindowTitle("Expériences & Protocoles");
        stack->setCurrentIndex(EXP_LIST);
    });

    // Export (démo)
    QObject::connect(exportE3, &QPushButton::clicked, this, [=](){
        QMessageBox::information(this, "Export", "Export statistiques Expériences (à connecter à PDF/Excel).");
    });
    // ===================== NAVIGATION PUBLICATION =====================

    // LIST -> FORM
    QObject::connect(pubAdd, &QPushButton::clicked, this, [=](){
        stack->setCurrentIndex(PUB_FORM);
    });
    QObject::connect(pubEdit, &QPushButton::clicked, this, [=](){
        if (pubTable->currentRow() < 0) {
            QMessageBox::information(this, "Information", "Sélectionnez une publication à modifier.");
            return;
        }
        stack->setCurrentIndex(PUB_FORM);
    });

    // LIST -> STATS
    QObject::connect(pubStats, &QPushButton::clicked, this, [=](){
        stack->setCurrentIndex(PUB_STATS);
    });

    // FORM -> LIST
    QObject::connect(pubCancel, &QPushButton::clicked, this, [=](){
        stack->setCurrentIndex(PUB_LIST);
    });
    QObject::connect(pubSave, &QPushButton::clicked, this, [=](){
        // (démo) : tu peux ajouter ici l’insertion dans pubTable si tu veux
        stack->setCurrentIndex(PUB_LIST);
    });

    // STATS -> LIST




    setWindowTitle("Gestion des Échantillons");
    stack->setCurrentIndex(BIO_LIST);
}
