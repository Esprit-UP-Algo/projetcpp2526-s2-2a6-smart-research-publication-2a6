#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QTreeWidget>
#include <QListWidget>
#include <QStackedWidget>
#include <QFrame>
#include <QStyle>
#include <QToolButton>
#include <QSpinBox>
#include <QDateEdit>
#include <QTextEdit>
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsDropShadowEffect>
#include <QMessageBox>
#include <QDialog>
#include <QtMath>
#include <QStyledItemDelegate>
#include <QAbstractItemView>

// ===================== COLORS =====================
static const QString C_TOPBAR = "#12443B";
static const QString C_PRIMARY = "#0A5F58";
static const QString C_BG = "#A3CAD3";
static const QString C_BEIGE = "#C6B29A";
static const QString C_TEXT_DARK = "#64533A";
static const QString C_TABLE_HDR = "#AFC6C3";
static const QString C_ROW_ODD = "#F2F0EB";
static const QString C_ROW_EVEN = "#ECE9E2";
static const QString C_BORDER = "rgba(0,0,0,0.10)";
static const QString C_PANEL_BG = "rgba(246,248,247,0.86)";
static const QString C_PANEL_IN = "rgba(244,246,245,0.78)";
static const QString C_PANEL_BR = "rgba(0,0,0,0.10)";

static const QColor W_GREEN = QColor("#2E6F63");     // Réussie
static const QColor W_ORANGE = QColor("#B5672C");    // Non concluante
static const QColor W_RED = QColor("#8B2F3C");       // Échouée
static const QColor W_GRAY = QColor("#7A8B8A");      // En cours

static int uiMargin(QWidget* w) {
    int W = w->width();
    if (W < 1100) return 6;
    if (W < 1400) return 10;
    return 14;
}

// ===================== Helpers =====================
static QFrame* makeCard(QWidget* parent=nullptr) {
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

static QFrame* softBox(QWidget* parent=nullptr) {
    QFrame* f = new QFrame(parent);
    f->setStyleSheet(QString("QFrame{ background:%1; border: 1px solid %2; border-radius: 12px; }")
                         .arg(C_PANEL_IN, C_PANEL_BR));
    return f;
}

static QToolButton* topIconBtn(QStyle* st, QStyle::StandardPixmap sp, const QString& tooltip) {
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

static QPushButton* actionBtn(const QString& text, const QString& bg, const QString& fg, const QIcon& icon, bool enabled=true) {
    QPushButton* b = new QPushButton(icon, " " + text);
    b->setEnabled(enabled);
    b->setCursor(Qt::PointingHandCursor);
    b->setStyleSheet(QString(R"(
        QPushButton{ background:%1; color:%2; border:1px solid rgba(0,0,0,0.12); border-radius:10px; padding:10px 18px; font-weight:800; }
        QPushButton:disabled{ background: rgba(200,200,200,0.55); color: rgba(90,90,90,0.55); }
        QPushButton:hover{ background: rgba(255,255,255,0.70); }
    )").arg(bg, fg));
    return b;
}

static QToolButton* tinySquareBtn(const QIcon& icon) {
    QToolButton* b = new QToolButton;
    b->setIcon(icon);
    b->setCursor(Qt::PointingHandCursor);
    b->setStyleSheet(R"(
        QToolButton{ background: rgba(255,255,255,0.55); border: 1px solid rgba(0,0,0,0.12); border-radius: 10px; padding: 10px; }
        QToolButton:hover{ background: rgba(255,255,255,0.75); }
    )");
    return b;
}

// ===== Brand (logo + 3 dots) =====
static QWidget* makeBrandWidget() {
    QWidget* w = new QWidget;
    QHBoxLayout* h = new QHBoxLayout(w);
    h->setContentsMargins(0,0,0,0);
    h->setSpacing(10);
    h->setAlignment(Qt::AlignVCenter);

    QLabel* logo = new QLabel;
    logo->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    logo->setContentsMargins(0, 4, 0, 0);

    // Essayer plusieurs chemins possibles pour le logo
    QPixmap px;
    QStringList paths = {
        ":/image/smartvision.png",
        ":/images/smartvision.png",
        "./smartvision.png",
        "../smartvision.png"
    };

    for (const QString& path : paths) {
        px = QPixmap(path);
        if (!px.isNull()) break;
    }

    if (!px.isNull()) {
        logo->setPixmap(px.scaled(36, 36, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        // Si aucune image trouvée, utiliser un emoji comme fallback
        logo->setText("🧪");
        QFont f = logo->font();
        f.setPointSize(24);
        logo->setFont(f);
    }

    QLabel* name = new QLabel("SmartVision");
    QFont nf = name->font();
    nf.setBold(true);
    nf.setPointSize(11);
    name->setFont(nf);
    name->setStyleSheet(QString("color:%1; letter-spacing: 0.6px;").arg(C_BEIGE));
    name->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    auto dot = [&](const QColor& c){
        QFrame* d = new QFrame;
        d->setFixedSize(6,6);
        d->setStyleSheet(QString("background:%1; border-radius:3px;").arg(c.name()));
        return d;
    };
    QWidget* dots = new QWidget;
    QHBoxLayout* dh = new QHBoxLayout(dots);
    dh->setContentsMargins(0,0,0,0);
    dh->setSpacing(6);
    dh->setAlignment(Qt::AlignVCenter);
    dh->addWidget(dot(QColor("#C6B29A")));
    dh->addWidget(dot(QColor("#8FD3E8")));
    dh->addWidget(dot(QColor("#C6B29A")));

    h->addWidget(logo);
    h->addWidget(name);
    h->addWidget(dots);
    return w;
}

static QFrame* makeTopBar(QStyle* st, const QString& titleText, QWidget* parent=nullptr) {
    QFrame* top = new QFrame(parent);
    top->setFixedHeight(54);
    top->setStyleSheet(QString("background:%1; border-radius: 14px;").arg(C_TOPBAR));
    QHBoxLayout* L = new QHBoxLayout(top);
    L->setContentsMargins(14,10,14,10);

    L->addWidget(makeBrandWidget(), 0, Qt::AlignLeft);

    QLabel* title = new QLabel(titleText);
    QFont f = title->font();
    f.setPointSize(14);
    f.setBold(true);
    title->setFont(f);
    title->setStyleSheet("color: rgba(255,255,255,0.90);");
    L->addStretch(1);
    L->addWidget(title, 0, Qt::AlignCenter);
    L->addStretch(1);

    QWidget* icons = new QWidget;
    QHBoxLayout* icL = new QHBoxLayout(icons);
    icL->setContentsMargins(0,0,0,0);
    icL->setSpacing(4);
    QToolButton* bHome = topIconBtn(st, QStyle::SP_DirHomeIcon, "Home");
    QToolButton* bInfo = topIconBtn(st, QStyle::SP_MessageBoxInformation, "Info");
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

// ===================== Status Badge Delegate =====================
enum class ExpStatus { Reussie=0, EnCours=1, Echouee=2, NonConcluante=3 };

static QString statusText(ExpStatus s) {
    switch (s) {
    case ExpStatus::Reussie: return "Réussie";
    case ExpStatus::EnCours: return "En Cours";
    case ExpStatus::Echouee: return "Échouée";
    case ExpStatus::NonConcluante: return "Non Concluante";
    }
    return "En Cours";
}

static QColor statusColor(ExpStatus s) {
    switch (s) {
    case ExpStatus::Reussie: return W_GREEN;
    case ExpStatus::EnCours: return W_GRAY;
    case ExpStatus::Echouee: return W_RED;
    case ExpStatus::NonConcluante: return W_ORANGE;
    }
    return W_GRAY;
}

class BadgeDelegate : public QStyledItemDelegate {
public:
    explicit BadgeDelegate(QObject* parent=nullptr) : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QVariant v = index.data(Qt::UserRole);
        ExpStatus st = ExpStatus::EnCours;
        if (v.isValid()) st = static_cast<ExpStatus>(v.toInt());

        QStyledItemDelegate::paint(painter, option, index);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        QRect r = option.rect.adjusted(8, 6, -8, -6);
        int h = qMin(r.height(), 28);
        int w = qMin(r.width(), 140);
        QRect pill(r.left() + (r.width() - w)/2, r.top() + (r.height()-h)/2, w, h);

        QColor bg = statusColor(st);
        painter->setPen(Qt::NoPen);
        painter->setBrush(bg);
        painter->drawRoundedRect(pill, 14, 14);

        QRect iconCircle(pill.left()+10, pill.top()+6, 16, 16);
        painter->setBrush(QColor(255,255,255,35));
        painter->drawEllipse(iconCircle);

        painter->setPen(QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        if (st == ExpStatus::Reussie) {
            QPoint a(iconCircle.left()+4, iconCircle.top()+9);
            QPoint b(iconCircle.left()+7, iconCircle.top()+12);
            QPoint c(iconCircle.left()+13, iconCircle.top()+5);
            painter->drawLine(a,b);
            painter->drawLine(b,c);
        } else if (st == ExpStatus::Echouee) {
            painter->drawLine(iconCircle.left()+5, iconCircle.top()+5, iconCircle.right()-5, iconCircle.bottom()-5);
            painter->drawLine(iconCircle.right()-5, iconCircle.top()+5, iconCircle.left()+5, iconCircle.bottom()-5);
        } else if (st == ExpStatus::NonConcluante) {
            QPainterPath path;
            path.moveTo(iconCircle.center().x(), iconCircle.top()+2);
            path.lineTo(iconCircle.left()+2, iconCircle.bottom()-2);
            path.lineTo(iconCircle.right()-2, iconCircle.bottom()-2);
            path.closeSubpath();
            painter->setPen(QPen(Qt::white, 1.8));
            painter->drawPath(path);
        } else {
            painter->drawEllipse(iconCircle.adjusted(4,4,-4,-4));
        }

        painter->setPen(Qt::white);
        QFont f = option.font;
        f.setBold(true);
        f.setPointSizeF(f.pointSizeF()-0.5);
        painter->setFont(f);
        QRect textRect = pill.adjusted(34, 4, -10, -4);
        painter->drawText(textRect, Qt::AlignVCenter|Qt::AlignLeft, statusText(st));

        painter->restore();
    }
};

// ===================== Delete Dialog =====================
class DeleteDialog : public QDialog {
public:
    DeleteDialog(QStyle* st, QWidget* parent=nullptr) : QDialog(parent) {
        setWindowTitle("Supprimer des expériences");
        resize(900, 420);
        setModal(true);

        QVBoxLayout* root = new QVBoxLayout(this);
        root->setContentsMargins(14,14,14,14);
        root->setSpacing(12);

        QLabel* title = new QLabel("Liste des expériences");
        QFont tf = title->font();
        tf.setBold(true);
        tf.setPointSize(12);
        title->setFont(tf);
        title->setStyleSheet("color: rgba(0,0,0,0.65);");
        root->addWidget(title);

        table = new QTableWidget(0, 6);
        table->setHorizontalHeaderLabels({"ID Exp","Titre","Hypothèse","Date Début","Statut","Action"});
        table->verticalHeader()->setVisible(false);
        table->horizontalHeader()->setStretchLastSection(true);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSelectionMode(QAbstractItemView::NoSelection);
        table->setStyleSheet(R"(
            QHeaderView::section{ background: rgba(159,190,185,0.85); padding: 8px; font-weight: 900; }
            QTableWidget{ background: rgba(255,255,255,0.75); border: 1px solid rgba(0,0,0,0.12); border-radius: 12px; }
            QTableWidget::item{ padding: 8px; font-weight: 800; color: rgba(0,0,0,0.65); }
        )");
        root->addWidget(table, 1);

        QPushButton* closeBtn = new QPushButton("Fermer");
        closeBtn->setCursor(Qt::PointingHandCursor);
        closeBtn->setStyleSheet("QPushButton{ padding:10px 18px; border-radius:10px; font-weight:900; }");
        connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
        root->addWidget(closeBtn, 0, Qt::AlignRight);

        // Données exemple
        addRow("EXP001","Effet température","H0: Pas d'effet","05/01/2024","Réussie");
        addRow("EXP002","Culture bactérienne","H1: Croissance accélérée","03/15/2024","En Cours");
        addRow("EXP003","Test enzymatique","H0: Activité normale","02/20/2024","Échouée");
        addRow("EXP004","Réaction PCR","H1: Amplification optimale","04/10/2024","Non Concluante");
    }

private:
    QTableWidget* table;

    void addRow(const QString& id, const QString& titre, const QString& hyp, const QString& date, const QString& status) {
        int r = table->rowCount();
        table->insertRow(r);
        table->setItem(r,0,new QTableWidgetItem(id));
        table->setItem(r,1,new QTableWidgetItem(titre));
        table->setItem(r,2,new QTableWidgetItem(hyp));
        table->setItem(r,3,new QTableWidgetItem(date));
        table->setItem(r,4,new QTableWidgetItem(status));

        for (int c=0;c<5;++c)
            table->item(r,c)->setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);

        QPushButton* del = new QPushButton("Supprimer");
        del->setCursor(Qt::PointingHandCursor);
        del->setStyleSheet("QPushButton{ color:#B14A4A; font-weight:900; padding:6px 12px; }");
        connect(del, &QPushButton::clicked, this, [=](){
            auto rep = QMessageBox::question(this, "Confirmation",
                                             "Voulez-vous supprimer cette expérience ?",
                                             QMessageBox::Yes | QMessageBox::No);
            if (rep == QMessageBox::Yes) {
                table->removeRow(r);
            }
        });
        table->setCellWidget(r, 5, del);
        table->setRowHeight(r, 42);
    }
};

// ===================== Charts =====================
class DonutChart : public QWidget {
public:
    struct Slice { double value; QColor color; QString label; };
    explicit DonutChart(QWidget* parent=nullptr) : QWidget(parent) {
        setMinimumHeight(170);
    }
    void setData(const QList<Slice>& s) { m_slices = s; update(); }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        QRect r = rect().adjusted(10,10,-10,-10);
        int d = qMin(r.width(), r.height());
        QRect pie(r.left(), r.top(), d, d);

        double total = 0;
        for (auto &s : m_slices) total += s.value;
        if (total <= 0) return;

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255,255,255,60));
        painter.drawEllipse(pie);

        int thickness = (int)(d * 0.28);
        QRect inner = pie.adjusted(thickness, thickness, -thickness, -thickness);

        double start = 90.0 * 16;
        for (auto &s : m_slices) {
            double span = - (s.value / total) * 360.0 * 16;
            painter.setBrush(s.color);
            painter.drawPie(pie, (int)start, (int)span);

            double midDeg = (start + span/2.0) / 16.0;
            double rad = qDegreesToRadians(midDeg);
            QPointF c = pie.center();
            double rr = d * 0.38;
            QPointF pos(c.x() + rr * std::cos(rad), c.y() - rr * std::sin(rad));

            int pct = (int)std::round((s.value / total) * 100.0);
            painter.setPen(QColor(255,255,255,230));
            QFont f = font();
            f.setBold(true);
            f.setPointSize(9);
            painter.setFont(f);
            painter.drawText(QRectF(pos.x()-18, pos.y()-10, 36, 20), Qt::AlignCenter, QString("%1%").arg(pct));
            start += span;
        }

        painter.setBrush(QColor(245,247,246,255));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(inner);

        painter.setPen(QColor(0,0,0,120));
        QFont f = font();
        f.setBold(true);
        f.setPointSize(10);
        painter.setFont(f);
        painter.drawText(inner, Qt::AlignCenter, "🧪");
    }

private:
    QList<Slice> m_slices;
};

class BarChart : public QWidget {
public:
    struct Bar { double value; QString label; };
    explicit BarChart(QWidget* parent=nullptr) : QWidget(parent) {
        setMinimumHeight(170);
    }
    void setData(const QList<Bar>& b) { m_bars = b; update(); }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        QRect r = rect().adjusted(12,12,-12,-12);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255,255,255,60));
        painter.drawRoundedRect(r, 12, 12);

        if (m_bars.isEmpty()) return;

        double maxV = 0;
        for (auto &b : m_bars) maxV = std::max(maxV, b.value);
        if (maxV <= 0) maxV = 1;

        int leftPad = 34;
        int bottomPad = 28;
        QRect plot = r.adjusted(leftPad, 18, -10, -bottomPad);

        painter.setPen(QColor(0,0,0,120));
        QFont t = font();
        t.setBold(true);
        t.setPointSize(9);
        painter.setFont(t);
        painter.drawText(QRect(r.left()+10, r.top()-2, r.width()-20, 16),
                         Qt::AlignLeft|Qt::AlignVCenter, "Nombre");

        QList<int> ticks = {100, 90, 50, 30};
        QFont f = font();
        f.setPointSize(8);
        f.setBold(true);
        painter.setFont(f);
        for (int tick : ticks) {
            double y = plot.bottom() - (tick / maxV) * plot.height();
            painter.setPen(QColor(0,0,0,90));
            painter.drawText(QRect(r.left(), (int)y-8, leftPad-6, 16),
                             Qt::AlignRight|Qt::AlignVCenter, QString::number(tick));
            painter.setPen(QColor(0,0,0,25));
            painter.drawLine(plot.left(), (int)y, plot.right(), (int)y);
        }

        int n = m_bars.size();
        int gap = 10;
        int bw = (plot.width() - gap*(n-1)) / n;
        for (int i=0; i<n; ++i) {
            double val = m_bars[i].value;
            int bh = (int)((val / maxV) * plot.height());
            int x = plot.left() + i*(bw+gap);
            int y = plot.bottom() - bh;

            QLinearGradient g(x, y, x, plot.bottom());
            g.setColorAt(0.0, W_GREEN.lighter(120));
            g.setColorAt(1.0, W_GREEN);
            painter.setBrush(g);
            painter.setPen(Qt::NoPen);
            painter.drawRoundedRect(QRect(x, y, bw, bh), 6, 6);

            painter.setPen(QColor(0,0,0,120));
            painter.setFont(f);
            painter.drawText(QRect(x, plot.bottom()+4, bw, bottomPad-6),
                             Qt::AlignCenter, m_bars[i].label);
        }
    }

private:
    QList<Bar> m_bars;
};

// ===================== MainWindow =====================
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    resize(1170, 560);
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
        QTextEdit {
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
    )").arg(C_BG, C_TEXT_DARK, C_TABLE_HDR));

    QVBoxLayout* rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(0,0,0,0);

    QStackedWidget* stack = new QStackedWidget;
    rootLayout->addWidget(stack);

    // ==========================================================
    // PAGE 0 : Widget 1 - Gestion des Expériences
    // ==========================================================
    QWidget* page1 = new QWidget;
    QVBoxLayout* p1 = new QVBoxLayout(page1);
    p1->setContentsMargins(22, 18, 22, 18);
    p1->setSpacing(14);

    p1->addWidget(makeTopBar(st, "Gestion des Expériences"));

    QFrame* bar1 = new QFrame;
    bar1->setFixedHeight(54);
    bar1->setStyleSheet("background: rgba(255,255,255,0.22); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* bar1L = new QHBoxLayout(bar1);
    bar1L->setContentsMargins(14, 8, 14, 8);
    bar1L->setSpacing(10);

    QLineEdit* search = new QLineEdit;
    search->setPlaceholderText("Rechercher par ID, Titre, Hypothèse, Équipement...");
    search->addAction(st->standardIcon(QStyle::SP_FileDialogContentsView), QLineEdit::LeadingPosition);

    QComboBox* cbStatus = new QComboBox;
    cbStatus->addItems({"Statut", "Réussie", "Échouée", "Non Concluante", "En Cours"});

    QComboBox* cbEquip = new QComboBox;
    cbEquip->addItems({"Équipement", "Microscope", "Centrifugeuse", "PCR", "Spectromètre"});

    QPushButton* filters = new QPushButton(st->standardIcon(QStyle::SP_FileDialogDetailedView), " Filtres");
    filters->setCursor(Qt::PointingHandCursor);
    filters->setStyleSheet(QString(R"(
        QPushButton{
            background:%1;
            color: rgba(255,255,255,0.92);
            border:1px solid rgba(0,0,0,0.18);
            border-radius: 12px;
            padding: 10px 16px;
            font-weight: 800;
        }
        QPushButton:hover{ background: %2; }
    )").arg(C_PRIMARY, C_TOPBAR));

    bar1L->addWidget(search, 1);
    bar1L->addWidget(cbStatus);
    bar1L->addWidget(cbEquip);
    bar1L->addWidget(filters);

    p1->addWidget(bar1);

    QFrame* card1 = makeCard();
    QVBoxLayout* card1L = new QVBoxLayout(card1);
    card1L->setContentsMargins(10,10,10,10);

    QTableWidget* table = new QTableWidget(5, 8);
    table->setHorizontalHeaderLabels({"", "ID Exp", "Titre", "Hypothèse", "Date Début", "Date Fin", "Équipement", "Statut"});
    table->verticalHeader()->setVisible(false);
    table->setShowGrid(true);
    table->setAlternatingRowColors(true);
    table->setStyleSheet(QString("QTableWidget{ alternate-background-color:%1; background-color:%2; }")
                             .arg(C_ROW_EVEN, C_ROW_ODD));
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setItemDelegateForColumn(7, new BadgeDelegate(table));

    table->setColumnWidth(0, 36);
    table->setColumnWidth(1, 90);
    table->setColumnWidth(2, 180);
    table->setColumnWidth(3, 200);
    table->setColumnWidth(4, 110);
    table->setColumnWidth(5, 110);
    table->setColumnWidth(6, 150);
    table->setColumnWidth(7, 150);

    auto setRow=[&](int r, const QString& id, const QString& titre, const QString& hyp,
                      const QString& dateD, const QString& dateF, const QString& equip, ExpStatus stt) {
        QTableWidgetItem* iconItem = new QTableWidgetItem;
        iconItem->setIcon(st->standardIcon(QStyle::SP_ArrowRight));
        iconItem->setTextAlignment(Qt::AlignCenter);
        table->setItem(r, 0, iconItem);

        auto mk = [&](const QString& t){
            QTableWidgetItem* it = new QTableWidgetItem(t);
            it->setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);
            return it;
        };

        table->setItem(r, 1, mk(id));
        table->setItem(r, 2, mk(titre));
        table->setItem(r, 3, mk(hyp));
        table->setItem(r, 4, mk(dateD));
        table->setItem(r, 5, mk(dateF));
        table->setItem(r, 6, mk(equip));

        QTableWidgetItem* badge = new QTableWidgetItem;
        badge->setData(Qt::UserRole, (int)stt);
        table->setItem(r, 7, badge);

        table->setRowHeight(r, 46);
    };

    setRow(0, "EXP001", "Effet température", "H0: Pas d'effet significatif", "01/03/2024", "15/03/2024", "Incubateur", ExpStatus::Reussie);
    setRow(1, "EXP002", "Culture bactérienne", "H1: Croissance accélérée", "05/02/2024", "En cours", "Microscope", ExpStatus::EnCours);
    setRow(2, "EXP003", "Test enzymatique", "H0: Activité normale", "10/01/2024", "20/01/2024", "Spectromètre", ExpStatus::Echouee);
    setRow(3, "EXP004", "Réaction PCR", "H1: Amplification optimale", "15/03/2024", "25/03/2024", "PCR", ExpStatus::NonConcluante);
    setRow(4, "EXP005", "Analyse protéines", "H1: Expression augmentée", "20/02/2024", "En cours", "Centrifugeuse", ExpStatus::EnCours);

    card1L->addWidget(table);
    p1->addWidget(card1, 1);

    QFrame* bottom1 = new QFrame;
    bottom1->setFixedHeight(64);
    bottom1->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* bottom1L = new QHBoxLayout(bottom1);
    bottom1L->setContentsMargins(14,10,14,10);
    bottom1L->setSpacing(12);

    QPushButton* btnAdd = actionBtn("Ajouter", "rgba(10,95,88,0.45)", "rgba(255,255,255,0.90)",
                                    st->standardIcon(QStyle::SP_DialogYesButton), true);
    QPushButton* btnEdit = actionBtn("Modifier", "rgba(198,178,154,0.55)", "rgba(255,255,255,0.85)",
                                     st->standardIcon(QStyle::SP_FileDialogContentsView), true);
    QPushButton* btnDel = actionBtn("Supprimer", "rgba(255,255,255,0.55)", "#B14A4A",
                                    st->standardIcon(QStyle::SP_TrashIcon), true);
    QPushButton* btnDet = actionBtn("Détails", "rgba(255,255,255,0.55)", C_TEXT_DARK,
                                    st->standardIcon(QStyle::SP_DesktopIcon), true);

    connect(btnDel, &QPushButton::clicked, this, [=](){
        DeleteDialog dlg(style(), this);
        dlg.exec();
    });

    bottom1L->addWidget(btnAdd);
    bottom1L->addWidget(btnEdit);
    bottom1L->addWidget(btnDel);
    bottom1L->addWidget(btnDet);
    bottom1L->addStretch(1);
    bottom1L->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_DirIcon)));
    bottom1L->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_FileIcon)));
    bottom1L->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_DialogSaveButton)));
    bottom1L->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_BrowserReload)));

    QPushButton* btnMore = new QPushButton(st->standardIcon(QStyle::SP_FileDialogContentsView), " Statistiques");
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
    // PAGE 1 : Widget 2 - Ajouter / Modifier Expérience
    // ==========================================================
    QWidget* page2 = new QWidget;
    QVBoxLayout* p2 = new QVBoxLayout(page2);
    p2->setContentsMargins(22, 18, 22, 18);
    p2->setSpacing(14);

    p2->addWidget(makeTopBar(st, "Ajouter / Modifier Expérience"));

    QFrame* outer2 = new QFrame;
    outer2->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }")
                              .arg(C_PANEL_BG, C_PANEL_BR));
    QHBoxLayout* outer2L = new QHBoxLayout(outer2);
    outer2L->setContentsMargins(12,12,12,12);
    outer2L->setSpacing(12);

    // LEFT PANEL
    QFrame* left2 = softBox();
    left2->setFixedWidth(380);
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
        d->setStyleSheet(
            "QDateEdit{ background: transparent; border:0;"
            "color: rgba(54,92,245,0.95); font-weight: 900; }"
            );
        l->addWidget(cal);
        l->addWidget(lab);
        l->addStretch(1);
        l->addWidget(d);
        return r;
    };

    left2L->addWidget(sectionTitle("Informations Générales"));

    QLineEdit* leID = new QLineEdit;
    leID->setPlaceholderText("ID Expérience");
    left2L->addWidget(formRow(QStyle::SP_FileIcon, "ID Exp", leID));

    QLineEdit* leTitre = new QLineEdit;
    leTitre->setPlaceholderText("Titre");
    left2L->addWidget(formRow(QStyle::SP_FileDialogDetailedView, "Titre", leTitre));

    left2L->addWidget(sectionTitle("Dates"));
    left2L->addWidget(blueDateRow("Date Début", QDate::currentDate()));
    left2L->addWidget(blueDateRow("Date Fin", QDate::currentDate().addMonths(1)));

    QComboBox* cbStatut = new QComboBox;
    cbStatut->addItems({"Statut", "En Cours", "Réussie", "Échouée", "Non Concluante"});
    cbStatut->setFixedWidth(200);
    left2L->addWidget(formRow(QStyle::SP_MessageBoxInformation, "Statut", cbStatut));

    left2L->addStretch(1);

    // RIGHT PANEL
    QFrame* right2 = softBox();
    QVBoxLayout* right2L = new QVBoxLayout(right2);
    right2L->setContentsMargins(12,12,12,12);
    right2L->setSpacing(10);

    right2L->addWidget(sectionTitle("Hypothèse & Résultats"));

    QFrame* hypBox = softBox();
    QVBoxLayout* hypL = new QVBoxLayout(hypBox);
    hypL->setContentsMargins(10,10,10,10);
    QLabel* hypLbl = new QLabel("Hypothèse:");
    hypLbl->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    QTextEdit* teHyp = new QTextEdit;
    teHyp->setPlaceholderText("Décrivez votre hypothèse...");
    teHyp->setMaximumHeight(80);
    hypL->addWidget(hypLbl);
    hypL->addWidget(teHyp);

    right2L->addWidget(hypBox);

    QFrame* resBox = softBox();
    QVBoxLayout* resL = new QVBoxLayout(resBox);
    resL->setContentsMargins(10,10,10,10);
    QLabel* resLbl = new QLabel("Résultats:");
    resLbl->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    QTextEdit* teRes = new QTextEdit;
    teRes->setPlaceholderText("Décrivez les résultats obtenus...");
    teRes->setMaximumHeight(80);
    resL->addWidget(resLbl);
    resL->addWidget(teRes);

    right2L->addWidget(resBox);

    right2L->addWidget(sectionTitle("Équipement"));

    QComboBox* cbEquip2 = new QComboBox;
    cbEquip2->addItems({"Équipement", "Microscope", "Centrifugeuse", "PCR", "Spectromètre", "Incubateur"});
    cbEquip2->setFixedWidth(220);
    right2L->addWidget(formRow(QStyle::SP_ComputerIcon, "Équipement", cbEquip2));

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

    QPushButton* saveBtn = actionBtn("Enregistrer", "rgba(10,95,88,0.45)", "rgba(255,255,255,0.90)",
                                     st->standardIcon(QStyle::SP_DialogSaveButton), true);
    QPushButton* cancelBtn = actionBtn("Annuler", "rgba(255,255,255,0.55)", C_TEXT_DARK,
                                       st->standardIcon(QStyle::SP_DialogCancelButton), true);

    bottom2L->addWidget(saveBtn);
    bottom2L->addWidget(cancelBtn);
    bottom2L->addStretch(1);

    p2->addWidget(bottom2);
    stack->addWidget(page2);

    // ==========================================================
    // PAGE 2 : Widget 3 - Statistiques & Rapports
    // ==========================================================
    QWidget* page3 = new QWidget;
    QVBoxLayout* p3 = new QVBoxLayout(page3);
    p3->setContentsMargins(22, 18, 22, 18);
    p3->setSpacing(14);

    p3->addWidget(makeTopBar(st, "Statistiques & Rapports"));

    QFrame* outer3 = new QFrame;
    outer3->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }")
                              .arg(C_PANEL_BG, C_PANEL_BR));
    QVBoxLayout* outer3L = new QVBoxLayout(outer3);
    outer3L->setContentsMargins(12,12,12,12);
    outer3L->setSpacing(12);

    // Filter row
    QFrame* filterRow = new QFrame;
    filterRow->setStyleSheet("QFrame{ background: rgba(255,255,255,0.72); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QHBoxLayout* fr3 = new QHBoxLayout(filterRow);
    fr3->setContentsMargins(10,8,10,8);
    fr3->setSpacing(10);

    auto dropPill = [&](const QString& t){
        QFrame* p = new QFrame;
        p->setStyleSheet("QFrame{ background: rgba(255,255,255,0.90); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
        QHBoxLayout* l = new QHBoxLayout(p);
        l->setContentsMargins(10,6,10,6);
        QLabel* tx = new QLabel(t);
        tx->setStyleSheet("color: rgba(0,0,0,0.55); font-weight:900;");
        QToolButton* dd = new QToolButton;
        dd->setAutoRaise(true);
        dd->setIcon(st->standardIcon(QStyle::SP_ArrowDown));
        dd->setCursor(Qt::PointingHandCursor);
        l->addWidget(tx);
        l->addStretch(1);
        l->addWidget(dd);
        return p;
    };

    fr3->addWidget(dropPill("Période"), 1);
    fr3->addWidget(dropPill("Statut"), 1);
    fr3->addWidget(dropPill("Équipement"), 1);

    outer3L->addWidget(filterRow);

    // Dashboard (donut + legend + bar chart)
    QFrame* dash = new QFrame;
    dash->setStyleSheet("QFrame{ background: rgba(255,255,255,0.55); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QHBoxLayout* dashL = new QHBoxLayout(dash);
    dashL->setContentsMargins(12,12,12,12);
    dashL->setSpacing(12);

    QFrame* donutCard = softBox();
    QVBoxLayout* dcL = new QVBoxLayout(donutCard);
    dcL->setContentsMargins(12,12,12,12);
    QLabel* totalLbl = new QLabel("Total Expériences: 150");
    totalLbl->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    DonutChart* donut = new DonutChart;
    donut->setData({
        {60, W_GREEN, "Réussie"},
        {30, W_GRAY, "En Cours"},
        {40, W_RED, "Échouée"},
        {20, W_ORANGE, "Non Concluante"}
    });
    dcL->addWidget(totalLbl);
    dcL->addWidget(donut, 1);

    QFrame* legendCard = softBox();
    QVBoxLayout* lgL = new QVBoxLayout(legendCard);
    lgL->setContentsMargins(12,12,12,12);
    lgL->setSpacing(10);

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

    lgL->addWidget(legendRow(W_GREEN, "Réussie"));
    lgL->addWidget(legendRow(W_GRAY, "En Cours"));
    lgL->addWidget(legendRow(W_RED, "Échouée"));
    lgL->addWidget(legendRow(W_ORANGE, "Non Concluante"));
    lgL->addStretch(1);

    QFrame* barCard = softBox();
    QVBoxLayout* bcL = new QVBoxLayout(barCard);
    bcL->setContentsMargins(12,12,12,12);
    BarChart* bars = new BarChart;
    bars->setData({
        {100,"Jan"},
        {80, "Fév"},
        {60, "Mar"},
        {90, "Avr"}
    });
    bcL->addWidget(bars, 1);

    dashL->addWidget(donutCard, 1);
    dashL->addWidget(legendCard, 1);
    dashL->addWidget(barCard, 1);

    outer3L->addWidget(dash, 1);

    // Action bar
    QFrame* actBar = new QFrame;
    actBar->setStyleSheet("QFrame{ background: rgba(255,255,255,0.35); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QHBoxLayout* actL = new QHBoxLayout(actBar);
    actL->setContentsMargins(12,10,12,10);
    actL->setSpacing(12);

    QPushButton* expReport = actionBtn("Exporter Rapport", "rgba(10,95,88,0.45)", "rgba(255,255,255,0.92)",
                                       st->standardIcon(QStyle::SP_DialogSaveButton), true);
    QPushButton* printBtn = actionBtn("Imprimer", "rgba(255,255,255,0.55)", C_TEXT_DARK,
                                      st->standardIcon(QStyle::SP_FileIcon), true);

    actL->addWidget(expReport);
    actL->addWidget(printBtn);
    actL->addStretch(1);

    outer3L->addWidget(actBar);

    p3->addWidget(outer3, 1);

    QFrame* bottom3 = new QFrame;
    bottom3->setFixedHeight(64);
    bottom3->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* bottom3L = new QHBoxLayout(bottom3);
    bottom3L->setContentsMargins(14,10,14,10);

    QPushButton* back3 = actionBtn("Retour", "rgba(255,255,255,0.55)", C_TEXT_DARK,
                                   st->standardIcon(QStyle::SP_ArrowBack), true);
    bottom3L->addWidget(back3);
    bottom3L->addStretch(1);

    p3->addWidget(bottom3);
    stack->addWidget(page3);

    // ==========================================================
    // PAGE 3 : Widget 4 - Détails d'une Expérience
    // ==========================================================
    QWidget* page4 = new QWidget;
    QVBoxLayout* p4 = new QVBoxLayout(page4);
    p4->setContentsMargins(22, 18, 22, 18);
    p4->setSpacing(14);

    p4->addWidget(makeTopBar(st, "Détails de l'Expérience"));

    QFrame* outer4 = new QFrame;
    outer4->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }")
                              .arg(C_PANEL_BG, C_PANEL_BR));
    QVBoxLayout* outer4L = new QVBoxLayout(outer4);
    outer4L->setContentsMargins(12,12,12,12);
    outer4L->setSpacing(12);

    // En-tête avec ID et statut
    QFrame* header4 = new QFrame;
    header4->setStyleSheet("QFrame{ background: rgba(255,255,255,0.72); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QHBoxLayout* header4L = new QHBoxLayout(header4);
    header4L->setContentsMargins(14,12,14,12);
    header4L->setSpacing(12);

    QLabel* idExp = new QLabel("EXP001");
    QFont idFont = idExp->font();
    idFont.setBold(true);
    idFont.setPointSize(16);
    idExp->setFont(idFont);
    idExp->setStyleSheet("color: rgba(0,0,0,0.75);");

    QLabel* statusBadge = new QLabel("Réussie");
    statusBadge->setAlignment(Qt::AlignCenter);
    statusBadge->setStyleSheet(QString(
                                   "QLabel{ background:%1; color:white; border-radius:14px; padding: 8px 16px; font-weight:900; }"
                                   ).arg(W_GREEN.name()));

    header4L->addWidget(idExp);
    header4L->addStretch(1);
    header4L->addWidget(statusBadge);

    outer4L->addWidget(header4);

    // Contenu principal
    QFrame* content4 = new QFrame;
    content4->setStyleSheet("QFrame{ background: rgba(255,255,255,0.55); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QVBoxLayout* content4L = new QVBoxLayout(content4);
    content4L->setContentsMargins(16,16,16,16);
    content4L->setSpacing(14);

    auto detailRow = [&](const QString& label, const QString& value, QStyle::StandardPixmap icon){
        QFrame* row = softBox();
        QHBoxLayout* l = new QHBoxLayout(row);
        l->setContentsMargins(12,10,12,10);
        l->setSpacing(12);

        QLabel* ic = new QLabel;
        ic->setPixmap(st->standardIcon(icon).pixmap(20,20));

        QLabel* lbl = new QLabel(label);
        lbl->setStyleSheet("color: rgba(0,0,0,0.50); font-weight: 800;");
        lbl->setMinimumWidth(120);

        QLabel* val = new QLabel(value);
        val->setStyleSheet("color: rgba(0,0,0,0.75); font-weight: 900;");
        val->setWordWrap(true);

        l->addWidget(ic);
        l->addWidget(lbl);
        l->addWidget(val, 1);
        return row;
    };

    content4L->addWidget(detailRow("Titre", "Effet de la température sur la croissance bactérienne",
                                   QStyle::SP_FileDialogDetailedView));
    content4L->addWidget(detailRow("Date Début", "01/03/2024", QStyle::SP_ArrowRight));
    content4L->addWidget(detailRow("Date Fin", "15/03/2024", QStyle::SP_ArrowRight));
    content4L->addWidget(detailRow("Équipement", "Incubateur, Microscope, Spectromètre",
                                   QStyle::SP_ComputerIcon));

    // Hypothèse
    QFrame* hypSection = softBox();
    QVBoxLayout* hypSL = new QVBoxLayout(hypSection);
    hypSL->setContentsMargins(12,10,12,10);
    hypSL->setSpacing(8);

    QLabel* hypTitle = new QLabel("📋 Hypothèse");
    QFont hypTF = hypTitle->font();
    hypTF.setBold(true);
    hypTF.setPointSize(11);
    hypTitle->setFont(hypTF);
    hypTitle->setStyleSheet("color: rgba(0,0,0,0.65);");

    QLabel* hypText = new QLabel(
        "H0: La température n'a pas d'effet significatif sur le taux de croissance "
        "des colonies bactériennes E. coli dans des conditions contrôlées."
        );
    hypText->setWordWrap(true);
    hypText->setStyleSheet("color: rgba(0,0,0,0.60); padding: 8px;");

    hypSL->addWidget(hypTitle);
    hypSL->addWidget(hypText);

    content4L->addWidget(hypSection);

    // Résultats
    QFrame* resSection = softBox();
    QVBoxLayout* resSL = new QVBoxLayout(resSection);
    resSL->setContentsMargins(12,10,12,10);
    resSL->setSpacing(8);

    QLabel* resTitle = new QLabel("📊 Résultats");
    QFont resTF = resTitle->font();
    resTF.setBold(true);
    resTF.setPointSize(11);
    resTitle->setFont(resTF);
    resTitle->setStyleSheet("color: rgba(0,0,0,0.65);");

    QLabel* resText = new QLabel(
        "Les résultats montrent une corrélation positive significative entre la température "
        "et le taux de croissance. À 37°C, le taux de croissance était optimal (p < 0.05). "
        "Les températures de 25°C et 42°C ont montré des taux réduits de 30% et 45% respectivement. "
        "L'hypothèse nulle est rejetée."
        );
    resText->setWordWrap(true);
    resText->setStyleSheet("color: rgba(0,0,0,0.60); padding: 8px;");

    resSL->addWidget(resTitle);
    resSL->addWidget(resText);

    content4L->addWidget(resSection);

    outer4L->addWidget(content4, 1);

    // Boutons d'action
    QFrame* actions4 = new QFrame;
    actions4->setStyleSheet("QFrame{ background: rgba(255,255,255,0.35); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QHBoxLayout* actions4L = new QHBoxLayout(actions4);
    actions4L->setContentsMargins(12,10,12,10);
    actions4L->setSpacing(12);

    QPushButton* editExp = actionBtn("Modifier", "rgba(198,178,154,0.55)", "rgba(255,255,255,0.85)",
                                     st->standardIcon(QStyle::SP_FileDialogContentsView), true);
    QPushButton* exportExp = actionBtn("Exporter", "rgba(10,95,88,0.45)", "rgba(255,255,255,0.92)",
                                       st->standardIcon(QStyle::SP_DialogSaveButton), true);
    QPushButton* deleteExp = actionBtn("Supprimer", "rgba(255,255,255,0.55)", "#B14A4A",
                                       st->standardIcon(QStyle::SP_TrashIcon), true);

    actions4L->addWidget(editExp);
    actions4L->addWidget(exportExp);
    actions4L->addStretch(1);
    actions4L->addWidget(deleteExp);

    outer4L->addWidget(actions4);

    p4->addWidget(outer4, 1);

    QFrame* bottom4 = new QFrame;
    bottom4->setFixedHeight(64);
    bottom4->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* bottom4L = new QHBoxLayout(bottom4);
    bottom4L->setContentsMargins(14,10,14,10);

    QPushButton* back4 = actionBtn("Retour", "rgba(255,255,255,0.55)", C_TEXT_DARK,
                                   st->standardIcon(QStyle::SP_ArrowBack), true);
    bottom4L->addWidget(back4);
    bottom4L->addStretch(1);

    p4->addWidget(bottom4);
    stack->addWidget(page4);

    // ==========================================================
    // NAVIGATION
    // ==========================================================
    connect(btnAdd, &QPushButton::clicked, this, [=]{
        setWindowTitle("Ajouter / Modifier Expérience");
        stack->setCurrentIndex(1);
    });

    connect(btnEdit, &QPushButton::clicked, this, [=]{
        setWindowTitle("Ajouter / Modifier Expérience");
        stack->setCurrentIndex(1);
    });

    connect(btnDet, &QPushButton::clicked, this, [=]{
        setWindowTitle("Détails de l'Expérience");
        stack->setCurrentIndex(3);
    });

    connect(cancelBtn, &QPushButton::clicked, this, [=]{
        setWindowTitle("Gestion des Expériences");
        stack->setCurrentIndex(0);
    });

    connect(saveBtn, &QPushButton::clicked, this, [=]{
        setWindowTitle("Gestion des Expériences");
        stack->setCurrentIndex(0);
    });

    connect(btnMore, &QPushButton::clicked, this, [=]{
        setWindowTitle("Statistiques & Rapports");
        stack->setCurrentIndex(2);
    });

    connect(back3, &QPushButton::clicked, this, [=]{
        setWindowTitle("Gestion des Expériences");
        stack->setCurrentIndex(0);
    });

    connect(back4, &QPushButton::clicked, this, [=]{
        setWindowTitle("Gestion des Expériences");
        stack->setCurrentIndex(0);
    });

    connect(editExp, &QPushButton::clicked, this, [=]{
        setWindowTitle("Ajouter / Modifier Expérience");
        stack->setCurrentIndex(1);
    });

    // Start
    setWindowTitle("Gestion des Expériences");
    stack->setCurrentIndex(0);
}
