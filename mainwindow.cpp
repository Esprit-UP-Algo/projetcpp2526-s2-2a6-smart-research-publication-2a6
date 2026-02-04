#include "mainwindow.h"
#include <QToolButton>  // AJOUT IMPORTANT !
#include <QStyle>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QTextEdit>
#include <QDateEdit>
#include <QDialog>
#include <QMessageBox>
#include <QStackedWidget>
#include <QListWidget>
#include <QTreeWidget>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QtMath>
#include <cmath>
#include <algorithm>

// ===================== COLORS =====================
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

static QFrame* makeTopBar(QStyle* st, const QString& titleText, QWidget* parent=nullptr)
{
    QFrame* top = new QFrame(parent);
    top->setFixedHeight(54);
    top->setStyleSheet(QString("background:%1; border-radius: 14px;").arg(C_TOPBAR));

    QHBoxLayout* L = new QHBoxLayout(top);
    L->setContentsMargins(14,10,14,10);

    // LEFT: logo/brand
    QLabel* logo = new QLabel("🧬");
    QFont logoFont = logo->font();
    logoFont.setPointSize(18);
    logo->setFont(logoFont);
    logo->setStyleSheet("color: rgba(255,255,255,0.90);");

    // CENTER: title
    QLabel* title = new QLabel(titleText);
    QFont f = title->font();
    f.setPointSize(14);
    f.setBold(true);
    title->setFont(f);
    title->setStyleSheet("color: rgba(255,255,255,0.90);");

    // RIGHT: icons
    QWidget* icons = new QWidget;
    QHBoxLayout* icL = new QHBoxLayout(icons);
    icL->setContentsMargins(0,0,0,0);
    icL->setSpacing(4);

    auto iconBtn = [&](QStyle::StandardPixmap sp, const QString& tooltip){
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
    };

    QToolButton* bHome = iconBtn(QStyle::SP_DirHomeIcon, "Home");
    QToolButton* bInfo = iconBtn(QStyle::SP_MessageBoxInformation, "Info");
    QToolButton* bClose= iconBtn(QStyle::SP_TitleBarCloseButton, "Fermer");

    QObject::connect(bClose, &QToolButton::clicked, top, [=](){
        QWidget* w = top->window();
        if (w) w->close();
    });

    icL->addWidget(bHome);
    icL->addWidget(bInfo);
    icL->addWidget(bClose);

    L->addWidget(logo);
    L->addStretch(1);
    L->addWidget(title, 0, Qt::AlignCenter);
    L->addStretch(1);
    L->addWidget(icons, 0, Qt::AlignRight);
    return top;
}

// ===================== Widget5 charts (reused) =====================
class DonutChart : public QWidget {
public:
    struct Slice { double value; QColor color; QString label; };
    explicit DonutChart(QWidget* parent=nullptr) : QWidget(parent) { setMinimumHeight(170); }
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
        p.drawText(inner, Qt::AlignCenter, "🧪");
    }

private:
    QList<Slice> m_slices;
};

class BarChart : public QWidget {
public:
    struct Bar { double value; QString label; };
    explicit BarChart(QWidget* parent=nullptr) : QWidget(parent) { setMinimumHeight(170); }
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

        int leftPad = 34;
        int bottomPad = 28;
        QRect plot = r.adjusted(leftPad, 18, -10, -bottomPad);

        p.setPen(QColor(0,0,0,120));
        QFont t = font(); t.setBold(true); t.setPointSize(9);
        p.setFont(t);
        p.drawText(QRect(r.left()+10, r.top()+4, r.width()-20, 16), Qt::AlignLeft|Qt::AlignVCenter, "Répartition");

        QList<int> ticks = {100, 75, 50, 25};
        QFont f = font(); f.setPointSize(8); f.setBold(true);
        p.setFont(f);

        for (int tick : ticks) {
            double y = plot.bottom() - (tick / maxV) * plot.height();
            p.setPen(QColor(0,0,0,90));
            p.drawText(QRect(r.left(), (int)y-8, leftPad-6, 16), Qt::AlignRight|Qt::AlignVCenter, QString::number(tick));
            p.setPen(QColor(0,0,0,25));
            p.drawLine(plot.left(), (int)y, plot.right(), (int)y);
        }

        int n = m_bars.size();
        int gap = 10;
        int bw = (plot.width() - gap*(n-1)) / n;

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

// ===================== Gradient Row =====================
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

// ===================== CONSTRUCTEUR =====================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Module – Gestion des Expériences");
    resize(1200, 700);

    QWidget *root = new QWidget(this);
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
        QTextEdit {
            background: rgba(255,255,255,0.65);
            border: 1px solid rgba(0,0,0,0.15);
            border-radius: 12px;
            padding: 10px 14px;
            color: %2;
        }
        QDateEdit {
            background: rgba(255,255,255,0.65);
            border: 1px solid rgba(0,0,0,0.15);
            border-radius: 12px;
            padding: 10px 14px;
            color: %2;
        }
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
    )").arg(C_BG, C_TEXT_DARK, C_TABLE_HDR));

    stack = new QStackedWidget;
    QVBoxLayout *main = new QVBoxLayout(root);
    main->setContentsMargins(22, 18, 22, 18);
    main->setSpacing(14);

    main->addWidget(stack);

    // IMPORTANT: Correction du nom de la fonction - avec 'e' à la fin
    stack->addWidget(pageListeExperiences(st)); // 0
    stack->addWidget(pageAjoutExperience(st));  // 1
    stack->addWidget(pageStats(st));            // 2
    stack->addWidget(pageDetails(st));          // 3

    setWindowTitle("Gestion des Expériences");
    stack->setCurrentIndex(0);
}

// ==================== Méthodes supplémentaires requises ====================
void MainWindow::on_ajouterExperience_clicked() {
    // Implémentation simple - vous pouvez la modifier selon vos besoins
    stack->setCurrentIndex(1);
}

void MainWindow::setupUI() {
    // Implémentation simple
}

void MainWindow::demonstrateFixedCalls() {
    // Implémentation simple
}

void MainWindow::showMessage(const QString &message) {
    QMessageBox::information(this, "Message", message);
}

MainWindow::~MainWindow() {
    // Destructeur si nécessaire
}

//
// ================= INTERFACE 1 : LISTE =================
//
QWidget* MainWindow::pageListeExperiences(QStyle* st)
{
    QWidget *w = new QWidget;
    QVBoxLayout *l = new QVBoxLayout(w);
    l->setContentsMargins(0,0,0,0);
    l->setSpacing(14);

    // TopBar
    l->addWidget(makeTopBar(st, "Liste des Expériences"));

    // Barre recherche + tri + export
    QFrame *bar = new QFrame;
    bar->setFixedHeight(54);
    bar->setStyleSheet("background: rgba(255,255,255,0.22); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout *b = new QHBoxLayout(bar);
    b->setContentsMargins(14, 8, 14, 8);
    b->setSpacing(10);

    QLineEdit *search = new QLineEdit;
    search->setPlaceholderText("Rechercher par nom, type, chercheur, date...");
    search->addAction(st->standardIcon(QStyle::SP_FileDialogContentsView), QLineEdit::LeadingPosition);

    QComboBox *tri = new QComboBox;
    tri->addItems({"Trier par date", "Trier par type", "Trier par statut"});

    QPushButton *exporter = actionBtn("Exporter (PDF / CSV)", "rgba(10,95,88,0.45)", "rgba(255,255,255,0.90)", st->standardIcon(QStyle::SP_DialogSaveButton), true);

    b->addWidget(search, 1);
    b->addWidget(tri);
    b->addWidget(exporter);

    l->addWidget(bar);

    // Tableau
    QFrame *tableCard = makeCard();
    QVBoxLayout *tc = new QVBoxLayout(tableCard);
    tc->setContentsMargins(10,10,10,10);

    QTableWidget *table = new QTableWidget(5, 8);
    table->setHorizontalHeaderLabels({
        "ID", "Nom", "Type", "Responsable",
        "Date début", "Date fin", "Statut", "Actions"
    });
    table->verticalHeader()->setVisible(false);
    table->setShowGrid(true);
    table->setAlternatingRowColors(true);
    table->setStyleSheet(QString("QTableWidget{ alternate-background-color:%1; background-color:%2; }").arg(C_ROW_EVEN, C_ROW_ODD));
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    table->setColumnWidth(0, 80);
    table->setColumnWidth(1, 180);
    table->setColumnWidth(2, 120);
    table->setColumnWidth(3, 140);
    table->setColumnWidth(4, 120);
    table->setColumnWidth(5, 120);
    table->setColumnWidth(6, 100);

    // Sample data
    auto setRow=[&](int r, const QString& id, const QString& name,
                      const QString& type, const QString& resp,
                      const QString& start, const QString& end, const QString& status)
    {
        auto mk = [&](const QString& t){
            QTableWidgetItem* it = new QTableWidgetItem(t);
            it->setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);
            return it;
        };

        table->setItem(r, 0, mk(id));
        table->setItem(r, 1, mk(name));
        table->setItem(r, 2, mk(type));
        table->setItem(r, 3, mk(resp));
        table->setItem(r, 4, mk(start));
        table->setItem(r, 5, mk(end));
        table->setItem(r, 6, mk(status));

        // Actions column
        QWidget* actionsWidget = new QWidget;
        QHBoxLayout* actionsLayout = new QHBoxLayout(actionsWidget);
        actionsLayout->setContentsMargins(5,5,5,5);
        actionsLayout->setSpacing(5);

        QToolButton* editBtn = tinySquareBtn(st->standardIcon(QStyle::SP_FileDialogContentsView));
        editBtn->setToolTip("Modifier");
        QToolButton* viewBtn = tinySquareBtn(st->standardIcon(QStyle::SP_DesktopIcon));
        viewBtn->setToolTip("Détails");
        QToolButton* delBtn = tinySquareBtn(st->standardIcon(QStyle::SP_TrashIcon));
        delBtn->setToolTip("Supprimer");

        actionsLayout->addWidget(editBtn);
        actionsLayout->addWidget(viewBtn);
        actionsLayout->addWidget(delBtn);
        actionsLayout->addStretch(1);

        table->setCellWidget(r, 7, actionsWidget);
        table->setRowHeight(r, 52);

        connect(editBtn, &QToolButton::clicked, this, [=](){
            stack->setCurrentIndex(1);
        });
        connect(viewBtn, &QToolButton::clicked, this, [=](){
            stack->setCurrentIndex(3);
        });
        connect(delBtn, &QToolButton::clicked, this, [=](){
            dialogSuppression(st)->exec();
        });
    };

    setRow(0, "EXP01", "Culture cellulaire", "Cellulaire", "Dr Sami", "01/03/2024", "30/03/2024", "En cours");
    setRow(1, "EXP02", "Séquençage ADN", "Génétique", "Dr Martin", "15/02/2024", "30/04/2024", "Terminé");
    setRow(2, "EXP03", "Analyse protéomique", "Protéomique", "Dr Leroy", "10/04/2024", "20/05/2024", "En attente");
    setRow(3, "EXP04", "Test pharmacologique", "Pharmacologie", "Dr Dubois", "05/03/2024", "15/06/2024", "En cours");
    setRow(4, "EXP05", "Étude comportementale", "Neuroscience", "Dr Moreau", "20/01/2024", "10/03/2024", "Terminé");

    tc->addWidget(table);
    l->addWidget(tableCard, 1);

    // Boutons bas
    QFrame* bottom = new QFrame;
    bottom->setFixedHeight(64);
    bottom->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout *actions = new QHBoxLayout(bottom);
    actions->setContentsMargins(14,10,14,10);
    actions->setSpacing(12);

    QPushButton *add = actionBtn("➕ Ajouter", "rgba(10,95,88,0.45)", "rgba(255,255,255,0.90)", st->standardIcon(QStyle::SP_DialogYesButton), true);
    QPushButton *edit = actionBtn("✏️ Modifier", "rgba(198,178,154,0.55)", "rgba(255,255,255,0.85)", st->standardIcon(QStyle::SP_FileDialogContentsView), true);
    QPushButton *details = actionBtn("👁️ Détails", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_DesktopIcon), true);
    QPushButton *del = actionBtn("🗑️ Supprimer", "rgba(255,255,255,0.55)", "#B14A4A", st->standardIcon(QStyle::SP_TrashIcon), true);

    actions->addWidget(add);
    actions->addWidget(edit);
    actions->addWidget(details);
    actions->addWidget(del);
    actions->addStretch(1);

    QPushButton *stats = actionBtn("📊 Statistiques", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_FileDialogInfoView), true);
    actions->addWidget(stats);

    actions->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_DirIcon)));
    actions->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_FileIcon)));
    actions->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_DialogSaveButton)));
    actions->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_BrowserReload)));

    l->addWidget(bottom);

    connect(add, &QPushButton::clicked, this, [=](){
        setWindowTitle("Ajouter / Modifier Expérience");
        stack->setCurrentIndex(1);
    });
    connect(edit, &QPushButton::clicked, this, [=](){
        setWindowTitle("Ajouter / Modifier Expérience");
        stack->setCurrentIndex(1);
    });
    connect(stats, &QPushButton::clicked, this, [=](){
        setWindowTitle("Statistiques des Expériences");
        stack->setCurrentIndex(2);
    });

    return w;
}

//
// ================= INTERFACE 2 & 5 : AJOUT / MODIFIER =================
//
QWidget* MainWindow::pageAjoutExperience(QStyle* st)
{
    QWidget *w = new QWidget;
    QVBoxLayout *l = new QVBoxLayout(w);
    l->setContentsMargins(0,0,0,0);
    l->setSpacing(14);

    // TopBar
    l->addWidget(makeTopBar(st, "Ajouter / Modifier Expérience"));

    QFrame *outer = new QFrame;
    outer->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }").arg(C_PANEL_BG, C_PANEL_BR));
    QHBoxLayout *outerL = new QHBoxLayout(outer);
    outerL->setContentsMargins(12,12,12,12);
    outerL->setSpacing(12);

    // Formulaire gauche
    QFrame *form = softBox();
    QVBoxLayout *f = new QVBoxLayout(form);
    f->setContentsMargins(12,12,12,12);
    f->setSpacing(10);

    auto formRow = [&](const QString& label, QWidget* widget){
        QWidget* row = new QWidget;
        QHBoxLayout* h = new QHBoxLayout(row);
        h->setContentsMargins(0,0,0,0);
        h->setSpacing(10);

        QLabel* lbl = new QLabel(label);
        lbl->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900; min-width: 120px;");

        h->addWidget(lbl);
        h->addWidget(widget, 1);
        return row;
    };

    QLineEdit* nameEdit = new QLineEdit("Culture cellulaire");
    QComboBox* typeCombo = new QComboBox;
    typeCombo->addItems({"Cellulaire", "Génétique", "Protéomique", "Pharmacologie", "Neuroscience"});

    QTextEdit* descEdit = new QTextEdit("Objectif : Étudier la prolifération cellulaire sous différentes conditions...");
    descEdit->setMaximumHeight(120);

    QLineEdit* respEdit = new QLineEdit("Dr Sami");

    QDateEdit* startEdit = new QDateEdit(QDate(2024, 3, 1));
    startEdit->setCalendarPopup(true);

    QDateEdit* endEdit = new QDateEdit(QDate(2024, 3, 30));
    endEdit->setCalendarPopup(true);

    QLineEdit* equipEdit = new QLineEdit("Microscope, Centrifugeuse, Incubateur");
    QLineEdit* sampleEdit = new QLineEdit("Cellules HeLa, Milieu de culture");

    f->addWidget(formRow("Nom :", nameEdit));
    f->addWidget(formRow("Type :", typeCombo));

    QLabel* descLabel = new QLabel("Description :");
    descLabel->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    f->addWidget(descLabel);
    f->addWidget(descEdit);

    f->addWidget(formRow("Responsable :", respEdit));
    f->addWidget(formRow("Date début :", startEdit));
    f->addWidget(formRow("Date fin :", endEdit));
    f->addWidget(formRow("Équipements :", equipEdit));
    f->addWidget(formRow("Échantillons :", sampleEdit));

    f->addStretch(1);

    // Tableau droit
    QFrame *tableFrame = softBox();
    QVBoxLayout *t = new QVBoxLayout(tableFrame);
    t->setContentsMargins(12,12,12,12);

    QLabel *lt = new QLabel("Étapes / Protocoles");
    lt->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900; font-size: 14px;");
    t->addWidget(lt);

    QTableWidget *tab = new QTableWidget(4, 3);
    tab->setHorizontalHeaderLabels({"Étape", "Durée", "Statut"});
    tab->verticalHeader()->setVisible(false);
    tab->setShowGrid(true);
    tab->setAlternatingRowColors(true);
    tab->setStyleSheet(QString("QTableWidget{ alternate-background-color:%1; background-color:%2; }").arg(C_ROW_EVEN, C_ROW_ODD));
    tab->horizontalHeader()->setStretchLastSection(true);
    tab->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto setStep = [&](int r, const QString& step, const QString& duration, const QString& status){
        tab->setItem(r, 0, new QTableWidgetItem(step));
        tab->setItem(r, 1, new QTableWidgetItem(duration));
        tab->setItem(r, 2, new QTableWidgetItem(status));
        for(int c=0; c<3; ++c) tab->item(r,c)->setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    };

    setStep(0, "Préparation échantillons", "2h", "✓ Terminé");
    setStep(1, "Incubation", "24h", "⏳ En cours");
    setStep(2, "Observation microscopique", "3h", "○ En attente");
    setStep(3, "Analyse résultats", "4h", "○ En attente");

    t->addWidget(tab, 1);

    QPushButton* addStep = actionBtn("➕ Ajouter étape", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_DialogYesButton), true);
    t->addWidget(addStep);

    outerL->addWidget(form, 2);
    outerL->addWidget(tableFrame, 3);

    l->addWidget(outer, 1);

    // Bottom buttons
    QFrame* bottom = new QFrame;
    bottom->setFixedHeight(64);
    bottom->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout *btns = new QHBoxLayout(bottom);
    btns->setContentsMargins(14,10,14,10);
    btns->setSpacing(12);

    QPushButton *save = actionBtn("💾 Enregistrer", "rgba(10,95,88,0.45)", "rgba(255,255,255,0.90)", st->standardIcon(QStyle::SP_DialogSaveButton), true);
    QPushButton *cancel = actionBtn("❌ Annuler", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_DialogCancelButton), true);

    btns->addWidget(save);
    btns->addWidget(cancel);
    btns->addStretch(1);

    l->addWidget(bottom);

    connect(cancel, &QPushButton::clicked, this, [=](){
        setWindowTitle("Liste des Expériences");
        stack->setCurrentIndex(0);
    });
    connect(save, &QPushButton::clicked, this, [=](){
        setWindowTitle("Liste des Expériences");
        stack->setCurrentIndex(0);
    });

    return w;
}

//
// ================= INTERFACE 3 : DIALOG SUPPRESSION =================
//
QDialog* MainWindow::dialogSuppression(QStyle* st)
{
    QDialog *d = new QDialog(this);
    d->setWindowTitle("Confirmation de suppression");
    d->setModal(true);
    d->setFixedSize(450, 300);

    d->setStyleSheet(QString(R"(
        QDialog {
            background: %1;
            border-radius: 14px;
        }
        QLabel {
            color: %2;
        }
    )").arg(C_PANEL_BG, C_TEXT_DARK));

    QVBoxLayout *l = new QVBoxLayout(d);
    l->setContentsMargins(24, 24, 24, 24);
    l->setSpacing(20);

    QFrame* header = new QFrame;
    header->setFixedHeight(60);
    header->setStyleSheet("background: rgba(255,255,255,0.35); border-radius: 12px;");
    QHBoxLayout* hh = new QHBoxLayout(header);
    hh->setContentsMargins(16,10,16,10);

    QLabel* warnIcon = new QLabel("⚠️");
    warnIcon->setStyleSheet("font-size: 24px;");
    QLabel* title = new QLabel("Confirmation de suppression");
    title->setStyleSheet("color: rgba(0,0,0,0.75); font-weight: 900; font-size: 16px;");

    hh->addWidget(warnIcon);
    hh->addWidget(title);
    hh->addStretch(1);

    l->addWidget(header);

    QLabel *msg = new QLabel(
        "Êtes-vous sûr de vouloir supprimer cette expérience ?\n\n"
        "ID : EXP01\n"
        "Nom : Culture cellulaire\n"
        "Responsable : Dr Sami\n"
        "Date : 01/03/2024 - 30/03/2024"
        );
    msg->setStyleSheet("color: rgba(0,0,0,0.65); font-weight: 800; font-size: 14px; line-height: 1.5;");
    msg->setAlignment(Qt::AlignLeft);
    l->addWidget(msg);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    QPushButton *ok = actionBtn("✅ Confirmer", "rgba(139,47,60,0.75)", "rgba(255,255,255,0.95)", st->standardIcon(QStyle::SP_DialogApplyButton), true);
    QPushButton *cancelBtn = actionBtn("❌ Annuler", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_DialogCancelButton), true);

    btnLayout->addStretch(1);
    btnLayout->addWidget(ok);
    btnLayout->addWidget(cancelBtn);

    l->addLayout(btnLayout);

    connect(cancelBtn, &QPushButton::clicked, d, &QDialog::reject);
    connect(ok, &QPushButton::clicked, d, &QDialog::accept);

    return d;
}

//
// ================= INTERFACE 4 : STATISTIQUES =================
//
QWidget* MainWindow::pageStats(QStyle* st)
{
    QWidget *w = new QWidget;
    QVBoxLayout *l = new QVBoxLayout(w);
    l->setContentsMargins(0,0,0,0);
    l->setSpacing(14);

    // TopBar
    l->addWidget(makeTopBar(st, "Statistiques des Expériences"));

    QFrame *outer = new QFrame;
    outer->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }").arg(C_PANEL_BG, C_PANEL_BR));
    QVBoxLayout *outerL = new QVBoxLayout(outer);
    outerL->setContentsMargins(12,12,12,12);
    outerL->setSpacing(12);

    // Filter row
    QFrame* filterRow = new QFrame;
    filterRow->setStyleSheet("QFrame{ background: rgba(255,255,255,0.72); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QHBoxLayout* fr = new QHBoxLayout(filterRow);
    fr->setContentsMargins(10,8,10,8);
    fr->setSpacing(10);

    auto filterPill = [&](const QString& text, const QString& badge = ""){
        QFrame* p = new QFrame;
        p->setStyleSheet("QFrame{ background: rgba(255,255,255,0.90); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
        QHBoxLayout* l = new QHBoxLayout(p);
        l->setContentsMargins(10,6,10,6);
        l->setSpacing(8);

        QLabel* tx = new QLabel(text);
        tx->setStyleSheet("color: rgba(0,0,0,0.55); font-weight:900;");

        l->addWidget(tx);
        l->addStretch(1);

        if (!badge.isEmpty()) {
            QLabel* b = new QLabel(badge);
            b->setAlignment(Qt::AlignCenter);
            b->setFixedSize(24,20);
            b->setStyleSheet("QLabel{ background:#0A5F58; color:white; border-radius:10px; font-weight:900;}");
            l->addWidget(b);
        }

        QToolButton* dd = new QToolButton;
        dd->setAutoRaise(true);
        dd->setIcon(st->standardIcon(QStyle::SP_ArrowDown));
        dd->setCursor(Qt::PointingHandCursor);
        l->addWidget(dd);

        return p;
    };

    fr->addWidget(filterPill("Cette année", "45"));
    fr->addWidget(filterPill("Par type"));
    fr->addWidget(filterPill("Par chercheur"));
    fr->addWidget(filterPill("Période"));

    outerL->addWidget(filterRow);

    // Charts section
    QFrame* dash = new QFrame;
    dash->setStyleSheet("QFrame{ background: rgba(255,255,255,0.55); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QHBoxLayout* dashL = new QHBoxLayout(dash);
    dashL->setContentsMargins(12,12,12,12);
    dashL->setSpacing(12);

    // Donut chart
    QFrame* donutCard = softBox();
    QVBoxLayout* dcL = new QVBoxLayout(donutCard);
    dcL->setContentsMargins(12,12,12,12);

    QLabel* totalLbl = new QLabel("Total Expériences: 180");
    totalLbl->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900; font-size: 14px;");

    DonutChart* donut = new DonutChart;
    donut->setData({
        {80, W_GREEN, "Terminées"},
        {45, QColor("#9FBEB9"), "En cours"},
        {25, W_ORANGE, "En attente"},
        {30, W_RED, "Annulées"}
    });

    dcL->addWidget(totalLbl);
    dcL->addWidget(donut, 1);
    dashL->addWidget(donutCard, 1);

    // Legend
    QFrame* legendCard = softBox();
    QVBoxLayout* lgL = new QVBoxLayout(legendCard);
    lgL->setContentsMargins(12,12,12,12);
    lgL->setSpacing(10);

    auto legendRow=[&](const QColor& c, const QString& t, const QString& count = ""){
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

        if (!count.isEmpty()) {
            QLabel* cnt = new QLabel(count);
            cnt->setStyleSheet("color: rgba(0,0,0,0.45); font-weight: 800;");
            h->addWidget(cnt);
        }

        return row;
    };

    QLabel* legendTitle = new QLabel("Répartition par statut");
    legendTitle->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900; font-size: 14px;");
    lgL->addWidget(legendTitle);
    lgL->addWidget(legendRow(W_GREEN, "Terminées", "80"));
    lgL->addWidget(legendRow(QColor("#9FBEB9"), "En cours", "45"));
    lgL->addWidget(legendRow(W_ORANGE, "En attente", "25"));
    lgL->addWidget(legendRow(W_RED, "Annulées", "30"));
    lgL->addStretch(1);
    dashL->addWidget(legendCard, 1);

    // Bar chart
    QFrame* barCard = softBox();
    QVBoxLayout* bcL = new QVBoxLayout(barCard);
    bcL->setContentsMargins(12,12,12,12);

    QLabel* barTitle = new QLabel("Expériences par mois");
    barTitle->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900; font-size: 14px;");

    BarChart* bars = new BarChart;
    bars->setData({
        {18, "Jan"},
        {22, "Fév"},
        {25, "Mar"},
        {20, "Avr"},
        {15, "Mai"},
        {12, "Juin"}
    });

    bcL->addWidget(barTitle);
    bcL->addWidget(bars, 1);
    dashL->addWidget(barCard, 1);

    outerL->addWidget(dash, 1);

    // Chercheurs section
    QFrame* chercheursCard = makeCard();
    QVBoxLayout* ccL = new QVBoxLayout(chercheursCard);
    ccL->setContentsMargins(12,12,12,12);

    QLabel* cherTitle = new QLabel("👩‍🔬 Par chercheur");
    cherTitle->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900; font-size: 16px;");
    ccL->addWidget(cherTitle);

    QListWidget* cherList = new QListWidget;
    cherList->setSpacing(8);
    cherList->setSelectionMode(QAbstractItemView::NoSelection);

    auto addCherRow = [&](const QString& name, int count, const QColor& color){
        QListWidgetItem* it = new QListWidgetItem;
        it->setSizeHint(QSize(10, 40));
        cherList->addItem(it);
        cherList->setItemWidget(it, new GradientRowWidget(st, name, QString::number(count), color, QStyle::SP_DirIcon, false));
    };

    addCherRow("Dr Sami", 25, W_GREEN);
    addCherRow("Dr Martin", 18, QColor("#9FBEB9"));
    addCherRow("Dr Leroy", 12, W_ORANGE);
    addCherRow("Dr Dubois", 8, W_GREEN);
    addCherRow("Dr Moreau", 6, QColor("#9FBEB9"));

    ccL->addWidget(cherList, 1);
    outerL->addWidget(chercheursCard);

    l->addWidget(outer, 1);

    // Bottom
    QFrame* bottom = new QFrame;
    bottom->setFixedHeight(64);
    bottom->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout *bottomL = new QHBoxLayout(bottom);
    bottomL->setContentsMargins(14,10,14,10);

    QPushButton *back = actionBtn("🔙 Retour", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_ArrowBack), true);
    bottomL->addWidget(back);
    bottomL->addStretch(1);

    QPushButton* exportBtn = actionBtn("📤 Exporter rapport", "rgba(10,95,88,0.45)", "rgba(255,255,255,0.90)", st->standardIcon(QStyle::SP_DialogSaveButton), true);
    bottomL->addWidget(exportBtn);

    l->addWidget(bottom);

    connect(back, &QPushButton::clicked, this, [=](){
        setWindowTitle("Liste des Expériences");
        stack->setCurrentIndex(0);
    });

    return w;
}

//
// ================= INTERFACE 6 : DETAILS =================
//
QWidget* MainWindow::pageDetails(QStyle* st)
{
    QWidget *w = new QWidget;
    QVBoxLayout *l = new QVBoxLayout(w);
    l->setContentsMargins(0,0,0,0);
    l->setSpacing(14);

    // TopBar
    l->addWidget(makeTopBar(st, "Détails de l'Expérience"));

    QFrame *outer = new QFrame;
    outer->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }").arg(C_PANEL_BG, C_PANEL_BR));
    QHBoxLayout *outerL = new QHBoxLayout(outer);
    outerL->setContentsMargins(12,12,12,12);
    outerL->setSpacing(12);

    // Left panel - Informations
    QFrame *leftPanel = softBox();
    leftPanel->setFixedWidth(320);
    QVBoxLayout *leftL = new QVBoxLayout(leftPanel);
    leftL->setContentsMargins(12,12,12,12);
    leftL->setSpacing(10);

    auto infoItem = [&](const QString& label, const QString& value){
        QWidget* row = new QWidget;
        QVBoxLayout* v = new QVBoxLayout(row);
        v->setContentsMargins(0,0,0,0);
        v->setSpacing(4);

        QLabel* lbl = new QLabel(label);
        lbl->setStyleSheet("color: rgba(0,0,0,0.45); font-weight: 800; font-size: 12px;");

        QLabel* val = new QLabel(value);
        val->setStyleSheet("color: rgba(0,0,0,0.75); font-weight: 900; font-size: 14px;");
        val->setWordWrap(true);

        v->addWidget(lbl);
        v->addWidget(val);
        return row;
    };

    QLabel* mainTitle = new QLabel("Culture cellulaire");
    mainTitle->setStyleSheet("color: rgba(0,0,0,0.85); font-weight: 900; font-size: 18px;");
    leftL->addWidget(mainTitle);

    leftL->addWidget(infoItem("ID", "EXP01"));
    leftL->addWidget(infoItem("Type", "Cellulaire"));
    leftL->addWidget(infoItem("Responsable", "Dr Sami"));
    leftL->addWidget(infoItem("Statut", "En cours"));
    leftL->addWidget(infoItem("Date début", "01/03/2024"));
    leftL->addWidget(infoItem("Date fin", "30/03/2024"));
    leftL->addWidget(infoItem("Durée", "30 jours"));

    QLabel* equipTitle = new QLabel("Équipements utilisés");
    equipTitle->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900; margin-top: 10px;");
    leftL->addWidget(equipTitle);

    QFrame* equipBox = new QFrame;
    equipBox->setStyleSheet("background: rgba(255,255,255,0.65); border-radius: 10px; padding: 10px;");
    QVBoxLayout* equipL = new QVBoxLayout(equipBox);
    equipL->setSpacing(5);

    auto equipItem = [&](const QString& name){
        QLabel* e = new QLabel("• " + name);
        e->setStyleSheet("color: rgba(0,0,0,0.65); font-weight: 800;");
        return e;
    };

    equipL->addWidget(equipItem("Microscope inversé"));
    equipL->addWidget(equipItem("Centrifugeuse"));
    equipL->addWidget(equipItem("Incubateur CO₂"));
    equipL->addWidget(equipItem("Hotte à flux laminaire"));

    leftL->addWidget(equipBox);
    leftL->addStretch(1);

    // Right panel - Description et résultats
    QFrame *rightPanel = softBox();
    QVBoxLayout *rightL = new QVBoxLayout(rightPanel);
    rightL->setContentsMargins(12,12,12,12);
    rightL->setSpacing(10);

    QLabel* descTitle = new QLabel("Description scientifique");
    descTitle->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900; font-size: 16px;");
    rightL->addWidget(descTitle);

    QFrame* descBox = makeCard();
    QVBoxLayout* descBoxL = new QVBoxLayout(descBox);
    descBoxL->setContentsMargins(12,12,12,12);

    QLabel* descText = new QLabel(
        "Objectif : Étudier la prolifération cellulaire sous différentes conditions de culture.\n\n"
        "Méthodologie : Les cellules HeLa sont cultivées dans un milieu DMEM supplémenté avec 10% de sérum fœtal bovin. "
        "Trois conditions sont testées : contrôle, traitement A (10µM), traitement B (25µM).\n\n"
        "Observations préliminaires : Le traitement A montre une inhibition de 30% de la prolifération après 48h, "
        "tandis que le traitement B induit une inhibition de 60%."
        );
    descText->setStyleSheet("color: rgba(0,0,0,0.65); font-weight: 800; line-height: 1.5;");
    descText->setWordWrap(true);
    descBoxL->addWidget(descText);

    rightL->addWidget(descBox, 1);

    QLabel* resultsTitle = new QLabel("Résultats & Observations");
    resultsTitle->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900; font-size: 16px;");
    rightL->addWidget(resultsTitle);

    QFrame* resultsBox = makeCard();
    QVBoxLayout* resultsL = new QVBoxLayout(resultsBox);
    resultsL->setContentsMargins(12,12,12,12);

    QTableWidget* resultsTable = new QTableWidget(4, 3);
    resultsTable->setHorizontalHeaderLabels({"Condition", "Prolifération", "Observation"});
    resultsTable->verticalHeader()->setVisible(false);
    resultsTable->setShowGrid(true);
    resultsTable->setAlternatingRowColors(true);
    resultsTable->setStyleSheet(QString("QTableWidget{ alternate-background-color:%1; background-color:%2; }").arg(C_ROW_EVEN, C_ROW_ODD));
    resultsTable->horizontalHeader()->setStretchLastSection(true);
    resultsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto setResult = [&](int r, const QString& cond, const QString& prolif, const QString& obs){
        resultsTable->setItem(r, 0, new QTableWidgetItem(cond));
        resultsTable->setItem(r, 1, new QTableWidgetItem(prolif));
        resultsTable->setItem(r, 2, new QTableWidgetItem(obs));
        for(int c=0; c<3; ++c) resultsTable->item(r,c)->setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    };

    setResult(0, "Contrôle", "100%", "Croissance normale");
    setResult(1, "Traitement A", "70%", "Inhibition modérée");
    setResult(2, "Traitement B", "40%", "Inhibition significative");
    setResult(3, "Témoin négatif", "15%", "Toxicité élevée");

    resultsL->addWidget(resultsTable);
    rightL->addWidget(resultsBox, 1);

    outerL->addWidget(leftPanel);
    outerL->addWidget(rightPanel, 1);

    l->addWidget(outer, 1);

    // Bottom
    QFrame* bottom = new QFrame;
    bottom->setFixedHeight(64);
    bottom->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout *bottomL = new QHBoxLayout(bottom);
    bottomL->setContentsMargins(14,10,14,10);

    QPushButton *back = actionBtn("🔙 Retour", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_ArrowBack), true);
    bottomL->addWidget(back);
    bottomL->addStretch(1);

    QPushButton* editBtn = actionBtn("✏️ Modifier", "rgba(198,178,154,0.55)", "rgba(255,255,255,0.85)", st->standardIcon(QStyle::SP_FileDialogContentsView), true);
    QPushButton* exportBtn = actionBtn("📤 Exporter", "rgba(10,95,88,0.45)", "rgba(255,255,255,0.90)", st->standardIcon(QStyle::SP_DialogSaveButton), true);
    QPushButton* printBtn = actionBtn("🖨️ Imprimer", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_FileDialogListView), true);

    bottomL->addWidget(editBtn);
    bottomL->addWidget(exportBtn);
    bottomL->addWidget(printBtn);

    l->addWidget(bottom);

    connect(back, &QPushButton::clicked, this, [=](){
        setWindowTitle("Liste des Expériences");
        stack->setCurrentIndex(0);
    });
    connect(editBtn, &QPushButton::clicked, this, [=](){
        setWindowTitle("Ajouter / Modifier Expérience");
        stack->setCurrentIndex(1);
    });

    return w;
}
