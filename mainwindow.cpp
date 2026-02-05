#include "mainwindow.h"
<<<<<<< HEAD
#include <QVBoxLayout>
#include <QHBoxLayout>
=======
#include <QToolButton>  // AJOUT IMPORTANT !
#include <QStyle>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
>>>>>>> a5f2e61294e377957bdd8d01540afbd830e739b8
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QTableWidget>
#include <QHeaderView>
<<<<<<< HEAD
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
=======
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
>>>>>>> a5f2e61294e377957bdd8d01540afbd830e739b8
    QFrame* card = new QFrame(parent);
    card->setObjectName("card");
    card->setFrameShape(QFrame::NoFrame);
    card->setStyleSheet("#card { background: rgba(238,242,241,0.78); border: 1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
<<<<<<< HEAD
=======

>>>>>>> a5f2e61294e377957bdd8d01540afbd830e739b8
    auto* shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(25);
    shadow->setOffset(0, 8);
    shadow->setColor(QColor(0,0,0,45));
    card->setGraphicsEffect(shadow);
    return card;
}

<<<<<<< HEAD
static QFrame* softBox(QWidget* parent=nullptr) {
=======
static QFrame* softBox(QWidget* parent=nullptr)
{
>>>>>>> a5f2e61294e377957bdd8d01540afbd830e739b8
    QFrame* f = new QFrame(parent);
    f->setStyleSheet(QString("QFrame{ background:%1; border: 1px solid %2; border-radius: 12px; }")
                         .arg(C_PANEL_IN, C_PANEL_BR));
    return f;
}

<<<<<<< HEAD
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
=======
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
>>>>>>> a5f2e61294e377957bdd8d01540afbd830e739b8
        QPushButton:hover{ background: rgba(255,255,255,0.70); }
    )").arg(bg, fg));
    return b;
}

<<<<<<< HEAD
static QToolButton* tinySquareBtn(const QIcon& icon) {
=======
static QToolButton* tinySquareBtn(const QIcon& icon)
{
>>>>>>> a5f2e61294e377957bdd8d01540afbd830e739b8
    QToolButton* b = new QToolButton;
    b->setIcon(icon);
    b->setCursor(Qt::PointingHandCursor);
    b->setStyleSheet(R"(
<<<<<<< HEAD
        QToolButton{ background: rgba(255,255,255,0.55); border: 1px solid rgba(0,0,0,0.12); border-radius: 10px; padding: 10px; }
=======
        QToolButton{
            background: rgba(255,255,255,0.55);
            border: 1px solid rgba(0,0,0,0.12);
            border-radius: 10px;
            padding: 10px;
        }
>>>>>>> a5f2e61294e377957bdd8d01540afbd830e739b8
        QToolButton:hover{ background: rgba(255,255,255,0.75); }
    )");
    return b;
}

<<<<<<< HEAD
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

=======
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
>>>>>>> a5f2e61294e377957bdd8d01540afbd830e739b8
    QLabel* title = new QLabel(titleText);
    QFont f = title->font();
    f.setPointSize(14);
    f.setBold(true);
    title->setFont(f);
    title->setStyleSheet("color: rgba(255,255,255,0.90);");
<<<<<<< HEAD
    L->addStretch(1);
    L->addWidget(title, 0, Qt::AlignCenter);
    L->addStretch(1);

=======

    // RIGHT: icons
>>>>>>> a5f2e61294e377957bdd8d01540afbd830e739b8
    QWidget* icons = new QWidget;
    QHBoxLayout* icL = new QHBoxLayout(icons);
    icL->setContentsMargins(0,0,0,0);
    icL->setSpacing(4);
<<<<<<< HEAD
    QToolButton* bHome = topIconBtn(st, QStyle::SP_DirHomeIcon, "Home");
    QToolButton* bInfo = topIconBtn(st, QStyle::SP_MessageBoxInformation, "Info");
    QToolButton* bClose= topIconBtn(st, QStyle::SP_TitleBarCloseButton, "Fermer");
=======

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

>>>>>>> a5f2e61294e377957bdd8d01540afbd830e739b8
    QObject::connect(bClose, &QToolButton::clicked, top, [=](){
        QWidget* w = top->window();
        if (w) w->close();
    });
<<<<<<< HEAD
    icL->addWidget(bHome);
    icL->addWidget(bInfo);
    icL->addWidget(bClose);
=======

    icL->addWidget(bHome);
    icL->addWidget(bInfo);
    icL->addWidget(bClose);

    L->addWidget(logo);
    L->addStretch(1);
    L->addWidget(title, 0, Qt::AlignCenter);
    L->addStretch(1);
>>>>>>> a5f2e61294e377957bdd8d01540afbd830e739b8
    L->addWidget(icons, 0, Qt::AlignRight);
    return top;
}

<<<<<<< HEAD
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
=======
// ===================== Widget5 charts (reused) =====================
class DonutChart : public QWidget {
public:
    struct Slice { double value; QColor color; QString label; };
    explicit DonutChart(QWidget* parent=nullptr) : QWidget(parent) { setMinimumHeight(170); }
>>>>>>> a5f2e61294e377957bdd8d01540afbd830e739b8
    void setData(const QList<Slice>& s) { m_slices = s; update(); }

protected:
    void paintEvent(QPaintEvent*) override {
<<<<<<< HEAD
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
=======
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

>>>>>>> a5f2e61294e377957bdd8d01540afbd830e739b8
        QRect r = rect().adjusted(10,10,-10,-10);
        int d = qMin(r.width(), r.height());
        QRect pie(r.left(), r.top(), d, d);

        double total = 0;
        for (auto &s : m_slices) total += s.value;
        if (total <= 0) return;

<<<<<<< HEAD
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255,255,255,60));
        painter.drawEllipse(pie);
=======
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255,255,255,60));
        p.drawEllipse(pie);
>>>>>>> a5f2e61294e377957bdd8d01540afbd830e739b8

        int thickness = (int)(d * 0.28);
        QRect inner = pie.adjusted(thickness, thickness, -thickness, -thickness);

        double start = 90.0 * 16;
        for (auto &s : m_slices) {
            double span = - (s.value / total) * 360.0 * 16;
<<<<<<< HEAD
            painter.setBrush(s.color);
            painter.drawPie(pie, (int)start, (int)span);
=======
            p.setBrush(s.color);
            p.drawPie(pie, (int)start, (int)span);
>>>>>>> a5f2e61294e377957bdd8d01540afbd830e739b8

            double midDeg = (start + span/2.0) / 16.0;
            double rad = qDegreesToRadians(midDeg);
            QPointF c = pie.center();
            double rr = d * 0.38;
            QPointF pos(c.x() + rr * std::cos(rad), c.y() - rr * std::sin(rad));

            int pct = (int)std::round((s.value / total) * 100.0);
<<<<<<< HEAD
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
=======
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
>>>>>>> a5f2e61294e377957bdd8d01540afbd830e739b8
    }

private:
    QList<Slice> m_slices;
};

class BarChart : public QWidget {
public:
    struct Bar { double value; QString label; };
<<<<<<< HEAD
    explicit BarChart(QWidget* parent=nullptr) : QWidget(parent) {
        setMinimumHeight(170);
    }
=======
    explicit BarChart(QWidget* parent=nullptr) : QWidget(parent) { setMinimumHeight(170); }
>>>>>>> a5f2e61294e377957bdd8d01540afbd830e739b8
    void setData(const QList<Bar>& b) { m_bars = b; update(); }

protected:
    void paintEvent(QPaintEvent*) override {
<<<<<<< HEAD
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        QRect r = rect().adjusted(12,12,-12,-12);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255,255,255,60));
        painter.drawRoundedRect(r, 12, 12);
=======
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        QRect r = rect().adjusted(12,12,-12,-12);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255,255,255,60));
        p.drawRoundedRect(r, 12, 12);
>>>>>>> a5f2e61294e377957bdd8d01540afbd830e739b8

        if (m_bars.isEmpty()) return;

        double maxV = 0;
        for (auto &b : m_bars) maxV = std::max(maxV, b.value);
        if (maxV <= 0) maxV = 1;

        int leftPad = 34;
        int bottomPad = 28;
        QRect plot = r.adjusted(leftPad, 18, -10, -bottomPad);

<<<<<<< HEAD
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
=======
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
>>>>>>> a5f2e61294e377957bdd8d01540afbd830e739b8
        }

        int n = m_bars.size();
        int gap = 10;
        int bw = (plot.width() - gap*(n-1)) / n;
<<<<<<< HEAD
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
=======

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
>>>>>>> a5f2e61294e377957bdd8d01540afbd830e739b8
        }
    }

private:
    QList<Bar> m_bars;
};

<<<<<<< HEAD
// ===================== MainWindow =====================
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    resize(1170, 560);
    QWidget* root = new QWidget(this);
=======
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
>>>>>>> a5f2e61294e377957bdd8d01540afbd830e739b8
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
<<<<<<< HEAD
        QTextEdit {
            background: rgba(255,255,255,0.65);
            border: 1px solid rgba(0,0,0,0.15);
            border-radius: 12px;
            padding: 10px 14px;
            color: %2;
        }
=======
>>>>>>> a5f2e61294e377957bdd8d01540afbd830e739b8
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
<<<<<<< HEAD
=======
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
>>>>>>> a5f2e61294e377957bdd8d01540afbd830e739b8
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

<<<<<<< HEAD
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
=======
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
>>>>>>> a5f2e61294e377957bdd8d01540afbd830e739b8
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
<<<<<<< HEAD
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

=======

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
>>>>>>> a5f2e61294e377957bdd8d01540afbd830e739b8
        auto mk = [&](const QString& t){
            QTableWidgetItem* it = new QTableWidgetItem(t);
            it->setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);
            return it;
        };

<<<<<<< HEAD
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
=======
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
>>>>>>> a5f2e61294e377957bdd8d01540afbd830e739b8

    // Filter row
    QFrame* filterRow = new QFrame;
    filterRow->setStyleSheet("QFrame{ background: rgba(255,255,255,0.72); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
<<<<<<< HEAD
    QHBoxLayout* fr3 = new QHBoxLayout(filterRow);
    fr3->setContentsMargins(10,8,10,8);
    fr3->setSpacing(10);

    auto dropPill = [&](const QString& t){
=======
    QHBoxLayout* fr = new QHBoxLayout(filterRow);
    fr->setContentsMargins(10,8,10,8);
    fr->setSpacing(10);

    auto filterPill = [&](const QString& text, const QString& badge = ""){
>>>>>>> a5f2e61294e377957bdd8d01540afbd830e739b8
        QFrame* p = new QFrame;
        p->setStyleSheet("QFrame{ background: rgba(255,255,255,0.90); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
        QHBoxLayout* l = new QHBoxLayout(p);
        l->setContentsMargins(10,6,10,6);
<<<<<<< HEAD
        QLabel* tx = new QLabel(t);
        tx->setStyleSheet("color: rgba(0,0,0,0.55); font-weight:900;");
=======
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

>>>>>>> a5f2e61294e377957bdd8d01540afbd830e739b8
        QToolButton* dd = new QToolButton;
        dd->setAutoRaise(true);
        dd->setIcon(st->standardIcon(QStyle::SP_ArrowDown));
        dd->setCursor(Qt::PointingHandCursor);
<<<<<<< HEAD
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
=======
        l->addWidget(dd);

        return p;
    };

    fr->addWidget(filterPill("Cette année", "45"));
    fr->addWidget(filterPill("Par type"));
    fr->addWidget(filterPill("Par chercheur"));
    fr->addWidget(filterPill("Période"));

    outerL->addWidget(filterRow);

    // Charts section
>>>>>>> a5f2e61294e377957bdd8d01540afbd830e739b8
    QFrame* dash = new QFrame;
    dash->setStyleSheet("QFrame{ background: rgba(255,255,255,0.55); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QHBoxLayout* dashL = new QHBoxLayout(dash);
    dashL->setContentsMargins(12,12,12,12);
    dashL->setSpacing(12);

<<<<<<< HEAD
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

=======
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
>>>>>>> a5f2e61294e377957bdd8d01540afbd830e739b8
    QFrame* legendCard = softBox();
    QVBoxLayout* lgL = new QVBoxLayout(legendCard);
    lgL->setContentsMargins(12,12,12,12);
    lgL->setSpacing(10);

<<<<<<< HEAD
    auto legendRow=[&](const QColor& c, const QString& t){
=======
    auto legendRow=[&](const QColor& c, const QString& t, const QString& count = ""){
>>>>>>> a5f2e61294e377957bdd8d01540afbd830e739b8
        QWidget* row = new QWidget;
        QHBoxLayout* h = new QHBoxLayout(row);
        h->setContentsMargins(0,0,0,0);
        h->setSpacing(10);
<<<<<<< HEAD
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
=======

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
>>>>>>> a5f2e61294e377957bdd8d01540afbd830e739b8
        setWindowTitle("Ajouter / Modifier Expérience");
        stack->setCurrentIndex(1);
    });

<<<<<<< HEAD
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
=======
    return w;
>>>>>>> a5f2e61294e377957bdd8d01540afbd830e739b8
}
