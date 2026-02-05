#include "mainwindow.h"
#include <QPainterPath>

#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
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
#include <QMessageBox>
#include <QGridLayout>
#include <QScrollArea>

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

// ===== Brand (logo + 3 dots) used in ALL topbars =====
static QWidget* makeBrandWidget()
{
    QWidget* w = new QWidget;
    QHBoxLayout* h = new QHBoxLayout(w);
    h->setContentsMargins(0,0,0,0);
    h->setSpacing(8);

    QLabel* logo = new QLabel;
    QPixmap px(":/smartvision.png");
    if (!px.isNull())
        logo->setPixmap(px.scaledToHeight(28, Qt::SmoothTransformation));
    logo->setFixedHeight(30);

    auto dot = [&](const QColor& c){
        QFrame* d = new QFrame;
        d->setFixedSize(8,8);
        d->setStyleSheet(QString("background:%1; border-radius:4px;").arg(c.name()));
        return d;
    };

    QWidget* dots = new QWidget;
    QHBoxLayout* dh = new QHBoxLayout(dots);
    dh->setContentsMargins(0,0,0,0);
    dh->setSpacing(6);
    dh->addWidget(dot(QColor("#C6B29A")));
    dh->addWidget(dot(QColor("#8FD3E8")));
    dh->addWidget(dot(QColor("#C6B29A")));

    h->addWidget(logo);
    h->addWidget(dots);
    return w;
}

static QFrame* makeTopBar(QStyle* st, const QString& titleText, QWidget* parent=nullptr)
{
    QFrame* top = new QFrame(parent);
    top->setFixedHeight(54);
    top->setStyleSheet(QString("background:%1; border-radius: 14px;").arg(C_TOPBAR));

    QHBoxLayout* L = new QHBoxLayout(top);
    L->setContentsMargins(14,10,14,10);

    L->addWidget(makeBrandWidget(), 0, Qt::AlignLeft);

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

// ===================== Equipment Status Badge Delegate =====================
enum class EquipmentStatus { Available=0, InUse=1, UnderMaintenance=2, OutOfOrder=3 };

static QString statusText(EquipmentStatus s)
{
    switch (s) {
    case EquipmentStatus::Available:        return "Available";
    case EquipmentStatus::InUse:            return "In Use";
    case EquipmentStatus::UnderMaintenance: return "Maintenance";
    case EquipmentStatus::OutOfOrder:       return "Out of Order";
    }
    return "Available";
}

static QColor statusColor(EquipmentStatus s)
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

        QColor bg = statusColor(st);
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
        p->drawText(textRect, Qt::AlignVCenter|Qt::AlignLeft, statusText(st));

        p->restore();
    }
};

// ===================== Widget3 gradient row =====================
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

static QFrame* w3TempQtyBlock(QStyle* st, const QString& room, const QString& capacity)
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

    v->addWidget(line(QStyle::SP_DirIcon, QString("Lab Room: %1").arg(room)));
    v->addWidget(line(QStyle::SP_ArrowUp, QString("Capacity: %1").arg(capacity)));
    return box;
}

static QFrame* w3BottomLocationBar(QStyle* st, const QString& text)
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

// ===================== UsageBarChart Widget =====================
class UsageBarChart : public QWidget {
public:
    UsageBarChart(QWidget* parent = nullptr) : QWidget(parent) {
        setMinimumHeight(100);
    }
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        QRect r = rect().adjusted(30, 10, -10, -25);

        // Grid lines
        p.setPen(QPen(QColor(0,0,0,15), 1));
        for(int i=0; i<=4; ++i) {
            int y = r.top() + (r.height() * i / 4);
            p.drawLine(r.left(), y, r.right(), y);
        }

        // Data: usage hours per day
        QList<double> data = {6.5, 8.2, 7.8, 9.1, 5.4, 8.7, 7.2};
        QStringList labels = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};

        double maxVal = 10.0;
        int barCount = data.size();
        int barWidth = (r.width() - (barCount-1)*8) / barCount;

        for(int i=0; i<barCount; ++i) {
            double val = data[i];
            int h = (int)((val / maxVal) * r.height());

            QRect bar(r.left() + i*(barWidth+8), r.bottom()-h, barWidth, h);

            QLinearGradient g(bar.topLeft(), bar.bottomLeft());
            g.setColorAt(0, QColor("#5AB9EA"));
            g.setColorAt(1, QColor("#4A90E2"));

            p.setPen(Qt::NoPen);
            p.setBrush(g);
            p.drawRoundedRect(bar, 4, 4);

            // Label
            p.setPen(QColor(0,0,0,120));
            QFont f = font();
            f.setPointSize(8);
            f.setBold(true);
            p.setFont(f);
            p.drawText(QRect(bar.left(), r.bottom()+5, bar.width(), 15),
                       Qt::AlignCenter, labels[i]);
        }

        // Y-axis labels
        p.setPen(QColor(0,0,0,90));
        QFont yf = font();
        yf.setPointSize(8);
        p.setFont(yf);
        for(int i=0; i<=4; ++i) {
            int y = r.top() + (r.height() * i / 4);
            int val = (int)(maxVal * (4-i) / 4);
            p.drawText(QRect(0, y-8, 25, 16), Qt::AlignRight|Qt::AlignVCenter, QString::number(val));
        }
    }
};

// ===================== MainWindow =====================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    resize(1280, 720);

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
    // PAGE 0 : Widget 1 - Equipment Management
    // ==========================================================
    QWidget* page1 = new QWidget;
    QVBoxLayout* p1 = new QVBoxLayout(page1);
    p1->setContentsMargins(22, 18, 22, 18);
    p1->setSpacing(14);

    p1->addWidget(makeTopBar(st, "Laboratory Equipment Management"));

    QFrame* bar1 = new QFrame;
    bar1->setFixedHeight(54);
    bar1->setStyleSheet("background: rgba(255,255,255,0.22); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* bar1L = new QHBoxLayout(bar1);
    bar1L->setContentsMargins(14, 8, 14, 8);
    bar1L->setSpacing(10);

    QLineEdit* search = new QLineEdit;
    search->setPlaceholderText("Search by Equipment ID, Name, Manufacturer, Model...");
    search->addAction(st->standardIcon(QStyle::SP_FileDialogContentsView), QLineEdit::LeadingPosition);

    QComboBox* cbEquipType = new QComboBox;
    cbEquipType->addItems({"Equipment", "PCR Machine", "Centrifuge", "Microscope", "Incubator"});

    QComboBox* cbStatus = new QComboBox;
    cbStatus->addItems({"Status", "Available", "In Use", "Under Maintenance", "Out of Order"});

    QComboBox* cbLocation = new QComboBox;
    cbLocation->addItems({"Location", "Lab 101", "Lab 102", "Lab 103", "Lab 201"});

    QPushButton* filters = new QPushButton(st->standardIcon(QStyle::SP_FileDialogDetailedView), "  Filters");
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
    bar1L->addWidget(cbEquipType);
    bar1L->addWidget(cbStatus);
    bar1L->addWidget(cbLocation);
    bar1L->addWidget(filters);
    p1->addWidget(bar1);

    QFrame* card1 = makeCard();
    QVBoxLayout* card1L = new QVBoxLayout(card1);
    card1L->setContentsMargins(10,10,10,10);

    QTableWidget* table = new QTableWidget(5, 10);
    table->setHorizontalHeaderLabels({"", "Equipment ID", "Equipment Name", "Manufacturer", "Model",
                                      "Location", "Purchase Date", "Next Maintenance", "Status", "Calibration Due"});
    table->verticalHeader()->setVisible(false);
    table->setShowGrid(true);
    table->setAlternatingRowColors(true);
    table->setStyleSheet(QString("QTableWidget{ alternate-background-color:%1; background-color:%2; }").arg(C_ROW_EVEN, C_ROW_ODD));
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setItemDelegateForColumn(8, new StatusBadgeDelegate(table));

    table->setColumnWidth(0, 36);
    table->setColumnWidth(1, 110);
    table->setColumnWidth(2, 140);
    table->setColumnWidth(3, 130);
    table->setColumnWidth(4, 110);
    table->setColumnWidth(5, 90);
    table->setColumnWidth(6, 120);
    table->setColumnWidth(7, 140);
    table->setColumnWidth(8, 150);

    auto setRow=[&](int r, const QString& id, const QString& name,
                      const QString& manufacturer, const QString& model, const QString& location,
                      const QString& purchaseDate, const QString& nextMaint, EquipmentStatus stt,
                      const QString& calibDate)
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

        table->setItem(r, 1, mk(id));
        table->setItem(r, 2, mk(name));
        table->setItem(r, 3, mk(manufacturer));
        table->setItem(r, 4, mk(model));
        table->setItem(r, 5, mk(location));
        table->setItem(r, 6, mk(purchaseDate));
        table->setItem(r, 7, mk(nextMaint));

        QTableWidgetItem* badge = new QTableWidgetItem;
        badge->setData(Qt::UserRole, (int)stt);
        table->setItem(r, 8, badge);

        table->setItem(r, 9, mk(calibDate));
        table->setRowHeight(r, 46);
    };

    setRow(0, "EQ-001", "PCR Machine",   "Thermo Fisher", "TX-500",  "Lab 101", "01/15/2023", "03/15/2026", EquipmentStatus::Available, "06/15/2026");
    setRow(1, "EQ-002", "Centrifuge",    "Eppendorf",     "5424R",   "Lab 102", "03/20/2023", "02/20/2026", EquipmentStatus::InUse, "05/20/2026");
    setRow(2, "EQ-003", "Microscope",    "Zeiss",         "AX-10",   "Lab 101", "06/10/2022", "02/10/2026", EquipmentStatus::UnderMaintenance, "04/10/2026");
    setRow(3, "EQ-004", "Incubator",     "Thermo Fisher", "HI-3000", "Lab 201", "09/05/2023", "03/05/2026", EquipmentStatus::Available, "09/05/2026");
    setRow(4, "EQ-005", "PCR Machine",   "Bio-Rad",       "PCR-200", "Lab 103", "11/12/2021", "01/12/2026", EquipmentStatus::OutOfOrder, "01/12/2026");

    card1L->addWidget(table);
    p1->addWidget(card1, 1);

    QFrame* bottom1 = new QFrame;
    bottom1->setFixedHeight(64);
    bottom1->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* bottom1L = new QHBoxLayout(bottom1);
    bottom1L->setContentsMargins(14,10,14,10);
    bottom1L->setSpacing(12);

    QPushButton* btnAdd   = actionBtn("Add Equipment",   "rgba(10,95,88,0.45)", "rgba(255,255,255,0.90)", st->standardIcon(QStyle::SP_DialogYesButton), true);
    QPushButton* btnEdit  = actionBtn("Edit",  "rgba(198,178,154,0.55)", "rgba(255,255,255,0.85)", st->standardIcon(QStyle::SP_FileDialogContentsView), true);
    QPushButton* btnDel   = actionBtn("Delete", "rgba(255,255,255,0.55)", "#B14A4A", st->standardIcon(QStyle::SP_TrashIcon), true);
    QPushButton* btnDet   = actionBtn("Details",   "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_DesktopIcon), true);

    bottom1L->addWidget(btnAdd);
    bottom1L->addWidget(btnEdit);
    bottom1L->addWidget(btnDel);
    bottom1L->addWidget(btnDet);
    bottom1L->addStretch(1);

    bottom1L->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_DirIcon)));
    bottom1L->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_FileIcon)));
    bottom1L->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_DialogSaveButton)));
    bottom1L->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_BrowserReload)));

    QPushButton* btnMore = new QPushButton(st->standardIcon(QStyle::SP_FileDialogContentsView), "  Maintenance Schedule");
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
    // PAGE 1 : Widget 2 - Add / Edit Equipment
    // ==========================================================
    QWidget* page2 = new QWidget;
    QVBoxLayout* p2 = new QVBoxLayout(page2);
    p2->setContentsMargins(22, 18, 22, 18);
    p2->setSpacing(14);

    p2->addWidget(makeTopBar(st, "Add / Edit Equipment"));

    QFrame* outer2 = new QFrame;
    outer2->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }").arg(C_PANEL_BG, C_PANEL_BR));
    QHBoxLayout* outer2L = new QHBoxLayout(outer2);
    outer2L->setContentsMargins(12,12,12,12);
    outer2L->setSpacing(12);

    QFrame* left2 = softBox();
    left2->setFixedWidth(300);
    QVBoxLayout* left2L = new QVBoxLayout(left2);
    left2L->setContentsMargins(10,10,10,10);
    left2L->setSpacing(10);

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
        left2L->addWidget(head);
        left2L->addWidget(b);
    };

    leftAction("Equipment Type", QStyle::SP_FileIcon, "PCR Machine");
    leftAction("Manufacturer", QStyle::SP_DirIcon, "Thermo Fisher");

    QLabel* locHead = new QLabel("Location Details");
    locHead->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    left2L->addWidget(locHead);

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

    left2L->addWidget(colBtn(QStyle::SP_DriveHDIcon, "Lab Room"));
    left2L->addWidget(colBtn(QStyle::SP_FileDialogListView, "Building"));
    left2L->addWidget(colBtn(QStyle::SP_ArrowDown, "Floor"));
    left2L->addStretch(1);

    QFrame* right2 = softBox();
    QVBoxLayout* right2L = new QVBoxLayout(right2);
    right2L->setContentsMargins(12,12,12,12);
    right2L->setSpacing(10);

    QFrame* tinyTop = softBox();
    QHBoxLayout* tinyTopL = new QHBoxLayout(tinyTop);
    tinyTopL->setContentsMargins(12,8,12,8);

    QToolButton* addDrop = new QToolButton;
    addDrop->setIcon(st->standardIcon(QStyle::SP_DialogYesButton));
    addDrop->setText("Add Equipment");
    addDrop->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    addDrop->setStyleSheet("QToolButton{ color: rgba(0,0,0,0.55); font-weight: 900; }");

    QLabel* idLbl = new QLabel("EQ-006");
    idLbl->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");

    tinyTopL->addWidget(addDrop);
    tinyTopL->addSpacing(10);
    tinyTopL->addWidget(idLbl);
    tinyTopL->addStretch(1);
    right2L->addWidget(tinyTop);

    auto comboRow = [&](QComboBox* cb){
        QFrame* r = softBox();
        QHBoxLayout* l = new QHBoxLayout(r);
        l->setContentsMargins(10,8,10,8);
        l->addWidget(cb);
        return r;
    };

    QComboBox* fcb1 = new QComboBox; fcb1->addItems({"PCR Machine","Centrifuge","Microscope","Incubator"});
    QComboBox* fcb2 = new QComboBox; fcb2->addItems({"Thermo Fisher","Eppendorf","Zeiss","Bio-Rad"});
    QComboBox* fcb3 = new QComboBox; fcb3->addItems({"Available","In Use","Under Maintenance","Out of Order"});
    right2L->addWidget(comboRow(fcb1));
    right2L->addWidget(comboRow(fcb2));
    right2L->addWidget(comboRow(fcb3));

    // Model Number input
    QFrame* modelRow = softBox();
    QHBoxLayout* modelL = new QHBoxLayout(modelRow);
    modelL->setContentsMargins(10,8,10,8);
    QLabel* modelLabel = new QLabel("Model Number:");
    modelLabel->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    QLineEdit* modelEdit = new QLineEdit;
    modelEdit->setPlaceholderText("e.g. TX-500");
    modelEdit->setStyleSheet("background: transparent; border:0; font-weight: 900; color: rgba(0,0,0,0.55);");
    modelL->addWidget(modelLabel);
    modelL->addWidget(modelEdit, 1);
    right2L->addWidget(modelRow);

    // Purchase Date
    QFrame* dateRow = softBox();
    QHBoxLayout* dateRowL = new QHBoxLayout(dateRow);
    dateRowL->setContentsMargins(10,8,10,8);
    dateRowL->setSpacing(8);

    QToolButton* cal = new QToolButton; cal->setAutoRaise(true); cal->setIcon(st->standardIcon(QStyle::SP_FileDialogDetailedView));
    QLabel* dateLabel = new QLabel("Purchase Date:");
    dateLabel->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    QDateEdit* date = new QDateEdit(QDate(2024,1,15));
    date->setCalendarPopup(true);
    date->setDisplayFormat("MM/dd/yyyy");
    date->setStyleSheet("QDateEdit{ background: transparent; border:0; font-weight: 900; color: rgba(0,0,0,0.55);} ");

    dateRowL->addWidget(cal);
    dateRowL->addWidget(dateLabel);
    dateRowL->addWidget(date, 1);
    right2L->addWidget(dateRow);

    // Next Maintenance Date
    QFrame* maintRow = softBox();
    QHBoxLayout* maintL = new QHBoxLayout(maintRow);
    maintL->setContentsMargins(10,8,10,8);
    maintL->setSpacing(8);

    QToolButton* cal2 = new QToolButton; cal2->setAutoRaise(true); cal2->setIcon(st->standardIcon(QStyle::SP_FileDialogDetailedView));
    QLabel* maintLabel = new QLabel("Next Maintenance:");
    maintLabel->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    QDateEdit* maintDate = new QDateEdit(QDate(2026,3,15));
    maintDate->setCalendarPopup(true);
    maintDate->setDisplayFormat("MM/dd/yyyy");
    maintDate->setStyleSheet("QDateEdit{ background: transparent; border:0; font-weight: 900; color: rgba(0,0,0,0.55);} ");

    maintL->addWidget(cal2);
    maintL->addWidget(maintLabel);
    maintL->addWidget(maintDate, 1);
    right2L->addWidget(maintRow);

    // Calibration Due Date
    QFrame* calRow = softBox();
    QHBoxLayout* calL = new QHBoxLayout(calRow);
    calL->setContentsMargins(10,8,10,8);
    calL->setSpacing(8);

    QToolButton* cal3 = new QToolButton; cal3->setAutoRaise(true); cal3->setIcon(st->standardIcon(QStyle::SP_FileDialogDetailedView));
    QLabel* calLabel = new QLabel("Calibration Due:");
    calLabel->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    QDateEdit* calDate = new QDateEdit(QDate(2026,6,15));
    calDate->setCalendarPopup(true);
    calDate->setDisplayFormat("MM/dd/yyyy");
    calDate->setStyleSheet("QDateEdit{ background: transparent; border:0; font-weight: 900; color: rgba(0,0,0,0.55);} ");

    calL->addWidget(cal3);
    calL->addWidget(calLabel);
    calL->addWidget(calDate, 1);
    right2L->addWidget(calRow);

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
    labRoom->setFixedWidth(160);

    right2L->addWidget(miniRow(QStyle::SP_DirIcon, "Lab Room:", labRoom));
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

    QPushButton* saveBtn   = actionBtn("Save", "rgba(10,95,88,0.45)", "rgba(255,255,255,0.90)", st->standardIcon(QStyle::SP_DialogSaveButton), true);
    QPushButton* cancelBtn = actionBtn("Cancel", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_DialogCancelButton), true);

    bottom2L->addWidget(saveBtn);
    bottom2L->addWidget(cancelBtn);
    bottom2L->addStretch(1);
    p2->addWidget(bottom2);

    stack->addWidget(page2);

    // ==========================================================
    // PAGE 2 : Widget 3 - Location & Storage
    // ==========================================================
    QWidget* page3 = new QWidget;
    QVBoxLayout* p3 = new QVBoxLayout(page3);
    p3->setContentsMargins(22, 18, 22, 18);
    p3->setSpacing(14);

    p3->addWidget(makeTopBar(st, "Equipment Location & Storage"));

    QFrame* outer3 = new QFrame;
    outer3->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }").arg(C_PANEL_BG, C_PANEL_BR));
    QHBoxLayout* outer3L = new QHBoxLayout(outer3);
    outer3L->setContentsMargins(12,12,12,12);
    outer3L->setSpacing(12);

    QFrame* left3 = softBox();
    left3->setFixedWidth(300);
    QVBoxLayout* left3L = new QVBoxLayout(left3);
    left3L->setContentsMargins(10,10,10,10);
    left3L->setSpacing(10);

    QFrame* ddBox = new QFrame;
    ddBox->setStyleSheet("QFrame{ background: rgba(255,255,255,0.72); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QHBoxLayout* ddBoxL = new QHBoxLayout(ddBox);
    ddBoxL->setContentsMargins(10,8,10,8);

    QLabel* ddText = new QLabel("Lab 101");
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

    auto* tL1 = new QTreeWidgetItem(tree3, QStringList() << "Lab 101");
    auto* tL2 = new QTreeWidgetItem(tree3, QStringList() << "Lab 102");
    auto* tL3 = new QTreeWidgetItem(tree3, QStringList() << "Lab 103");
    auto* tL4 = new QTreeWidgetItem(tree3, QStringList() << "Lab 201");

    tL1->setIcon(0, st->standardIcon(QStyle::SP_DriveHDIcon));
    tL2->setIcon(0, st->standardIcon(QStyle::SP_DriveHDIcon));
    tL3->setIcon(0, st->standardIcon(QStyle::SP_DriveHDIcon));
    tL4->setIcon(0, st->standardIcon(QStyle::SP_DriveHDIcon));

    auto* tP1 = new QTreeWidgetItem(tL1, QStringList() << "PCR Machines");
    auto* tP2 = new QTreeWidgetItem(tL1, QStringList() << "Microscopes");
    auto* tP3 = new QTreeWidgetItem(tL2, QStringList() << "Centrifuges");
    auto* tP4 = new QTreeWidgetItem(tL4, QStringList() << "Incubators");

    tP1->setIcon(0, st->standardIcon(QStyle::SP_DirIcon));
    tP2->setIcon(0, st->standardIcon(QStyle::SP_DirIcon));
    tP3->setIcon(0, st->standardIcon(QStyle::SP_DirIcon));
    tP4->setIcon(0, st->standardIcon(QStyle::SP_DirIcon));

    tree3->expandAll();
    tree3->setCurrentItem(tL1);

    left3L->addWidget(ddBox);
    left3L->addWidget(tree3, 1);

    QFrame* right3 = softBox();
    QVBoxLayout* right3L = new QVBoxLayout(right3);
    right3L->setContentsMargins(10,10,10,10);
    right3L->setSpacing(10);

    QFrame* header3 = new QFrame;
    header3->setStyleSheet("QFrame{ background: rgba(255,255,255,0.72); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QHBoxLayout* header3L = new QHBoxLayout(header3);
    header3L->setContentsMargins(10,8,10,8);

    QPushButton* details3 = new QPushButton(st->standardIcon(QStyle::SP_FileDialogDetailedView), "  Details");
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
    header3L->addWidget(chip("Total Equipment"));
    header3L->addWidget(chip("Maintenance Due"));

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

    addListRow(new GradientRowWidget(st, "PCR Machine",   "EQ-001", W_GREEN,  QStyle::SP_FileIcon, false));
    addListRow(new GradientRowWidget(st, "Centrifuge",    "EQ-002", W_ORANGE, QStyle::SP_FileIcon, false));
    addListRow(new GradientRowWidget(st, "Microscope",    "EQ-003", W_GRAY,   QStyle::SP_FileIcon, false));
    addListRow(new GradientRowWidget(st, "Incubator",     "EQ-004", W_GREEN,  QStyle::SP_FileIcon, false));
    addListRow(new GradientRowWidget(st, "PCR Machine",   "EQ-005", W_RED,    QStyle::SP_FileIcon, true));

    listBox3L->addWidget(list3);

    QWidget* bottomInfo3 = new QWidget;
    QHBoxLayout* bottomInfo3L = new QHBoxLayout(bottomInfo3);
    bottomInfo3L->setContentsMargins(0,0,0,0);
    bottomInfo3L->setSpacing(12);
    bottomInfo3L->addWidget(w3TempQtyBlock(st, "Lab 101", "15 Units"));
    bottomInfo3L->addWidget(w3BottomLocationBar(st, "Building A, Floor 1"), 1);

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

    QPushButton* back3 = actionBtn("Back", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_ArrowBack), true);
    bottom3L->addWidget(back3);
    bottom3L->addStretch(1);

    p3->addWidget(bottom3);
    stack->addWidget(page3);

    // ==========================================================
    // PAGE 3 : Widget 4 - Equipment Details
    // ==========================================================
    QWidget* page4 = new QWidget;
    QVBoxLayout* p4 = new QVBoxLayout(page4);
    p4->setContentsMargins(22, 18, 22, 18);
    p4->setSpacing(14);

    p4->addWidget(makeTopBar(st, "Equipment Details"));

    // Create scroll area for content
    QScrollArea* scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    QWidget* scrollContent = new QWidget;
    QVBoxLayout* scrollL = new QVBoxLayout(scrollContent);
    scrollL->setContentsMargins(0,0,0,0);
    scrollL->setSpacing(12);

    QFrame* outer4 = new QFrame;
    outer4->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }").arg(C_PANEL_BG, C_PANEL_BR));
    QVBoxLayout* outer4L = new QVBoxLayout(outer4);
    outer4L->setContentsMargins(12,12,12,12);
    outer4L->setSpacing(12);

    // Title section
    QFrame* titleFrame = softBox();
    QHBoxLayout* titleL = new QHBoxLayout(titleFrame);
    titleL->setContentsMargins(14,12,14,12);

    QLabel* equipIcon = new QLabel;
    equipIcon->setPixmap(st->standardIcon(QStyle::SP_ComputerIcon).pixmap(48,48));

    QLabel* equipTitle = new QLabel("<b>PCR Machine - Thermo Fisher</b><br><small style='color: rgba(0,0,0,0.55);'>Equipment ID: EQ-001</small>");
    QFont titleFont = equipTitle->font();
    titleFont.setPointSize(16);
    equipTitle->setFont(titleFont);

    QLabel* statusBadge = new QLabel("Available");
    statusBadge->setAlignment(Qt::AlignCenter);
    statusBadge->setFixedSize(120, 32);
    statusBadge->setStyleSheet("QLabel{ background:#2E6F63; color:white; border-radius:16px; font-weight:900; padding:4px 12px; }");

    titleL->addWidget(equipIcon);
    titleL->addWidget(equipTitle, 1);
    titleL->addWidget(statusBadge);
    outer4L->addWidget(titleFrame);

    // Details grid
    QFrame* detailsFrame = softBox();
    QVBoxLayout* detailsMainL = new QVBoxLayout(detailsFrame);
    detailsMainL->setContentsMargins(14,14,14,14);
    detailsMainL->setSpacing(10);

    QLabel* detailsHeader = new QLabel("<b>Equipment Information</b>");
    detailsHeader->setStyleSheet("color: rgba(0,0,0,0.65); font-weight: 900; font-size: 14px;");
    detailsMainL->addWidget(detailsHeader);

    QGridLayout* detailsGrid = new QGridLayout;
    detailsGrid->setSpacing(12);
    detailsGrid->setColumnStretch(1, 1);
    detailsGrid->setColumnStretch(3, 1);

    auto addDetailRow = [&](int row, int col, const QString& label, const QString& value){
        QLabel* lbl = new QLabel("<b>" + label + ":</b>");
        lbl->setStyleSheet("color: rgba(0,0,0,0.65); font-weight: 900; font-size: 12px;");
        QLabel* val = new QLabel(value);
        val->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 600; font-size: 12px;");
        detailsGrid->addWidget(lbl, row, col*2);
        detailsGrid->addWidget(val, row, col*2+1);
    };

    // Column 1
    addDetailRow(0, 0, "Manufacturer", "Thermo Fisher");
    addDetailRow(1, 0, "Model Number", "TX-500");
    addDetailRow(2, 0, "Location", "Lab 101");
    addDetailRow(3, 0, "Purchase Date", "01/15/2023");

    // Column 2
    addDetailRow(0, 1, "Last Maintenance", "12/15/2025");
    addDetailRow(1, 1, "Next Maintenance", "03/15/2026");
    addDetailRow(2, 1, "Calibration Due", "06/15/2026");
    addDetailRow(3, 1, "Current User", "Dr. Smith (USER-123)");

    detailsMainL->addLayout(detailsGrid);
    outer4L->addWidget(detailsFrame);

    // ========== STATISTICS SECTION ==========
    QFrame* statsContainer = new QFrame;
    statsContainer->setStyleSheet("QFrame{ background: transparent; }");
    QHBoxLayout* statsContainerL = new QHBoxLayout(statsContainer);
    statsContainerL->setContentsMargins(0,0,0,0);
    statsContainerL->setSpacing(12);

    // Left: KPI Cards
    QWidget* kpiWidget = new QWidget;
    QVBoxLayout* kpiL = new QVBoxLayout(kpiWidget);
    kpiL->setContentsMargins(0,0,0,0);
    kpiL->setSpacing(10);

    // KPI Card Helper
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

    kpiL->addWidget(createKpiCard("UPTIME", "98.5%", "Last 30 days", W_GREEN));
    kpiL->addWidget(createKpiCard("USAGE HOURS", "847h", "Total runtime", QColor("#4A90E2")));
    kpiL->addWidget(createKpiCard("EFFICIENCY", "94%", "Performance rate", W_ORANGE));

    // Right: Usage Chart + Timeline
    QWidget* chartsWidget = new QWidget;
    QVBoxLayout* chartsL = new QVBoxLayout(chartsWidget);
    chartsL->setContentsMargins(0,0,0,0);
    chartsL->setSpacing(10);

    // Usage Chart
    QFrame* usageChartFrame = softBox();
    usageChartFrame->setMinimumHeight(180);
    QVBoxLayout* usageChartL = new QVBoxLayout(usageChartFrame);
    usageChartL->setContentsMargins(16,12,16,12);

    QLabel* chartTitle = new QLabel("<b>Usage Statistics - Last 7 Days</b>");
    chartTitle->setStyleSheet("color: rgba(0,0,0,0.65); font-weight: 900; font-size: 13px;");
    usageChartL->addWidget(chartTitle);

    UsageBarChart* usageChart = new UsageBarChart;
    usageChartL->addWidget(usageChart, 1);

    // Timeline / Status History
    QFrame* timelineFrame = softBox();
    timelineFrame->setMinimumHeight(150);
    QVBoxLayout* timelineL = new QVBoxLayout(timelineFrame);
    timelineL->setContentsMargins(16,12,16,12);
    timelineL->setSpacing(8);

    QLabel* timelineTitle = new QLabel("<b>Status Timeline</b>");
    timelineTitle->setStyleSheet("color: rgba(0,0,0,0.65); font-weight: 900; font-size: 13px;");
    timelineL->addWidget(timelineTitle);

    // Timeline items
    auto createTimelineItem = [&](const QString& date, const QString& event, const QColor& dotColor) {
        QWidget* item = new QWidget;
        QHBoxLayout* itemL = new QHBoxLayout(item);
        itemL->setContentsMargins(0,4,0,4);
        itemL->setSpacing(12);

        // Dot
        QFrame* dot = new QFrame;
        dot->setFixedSize(10, 10);
        dot->setStyleSheet(QString("background: %1; border-radius: 5px;").arg(dotColor.name()));

        // Date
        QLabel* dateLbl = new QLabel(date);
        dateLbl->setFixedWidth(80);
        dateLbl->setStyleSheet("color: rgba(0,0,0,0.45); font-weight: 700; font-size: 10px;");

        // Event
        QLabel* eventLbl = new QLabel(event);
        eventLbl->setStyleSheet("color: rgba(0,0,0,0.65); font-weight: 600; font-size: 11px;");

        itemL->addWidget(dot);
        itemL->addWidget(dateLbl);
        itemL->addWidget(eventLbl, 1);

        return item;
    };

    timelineL->addWidget(createTimelineItem("Feb 01, 2026", "Equipment available for use", W_GREEN));
    timelineL->addWidget(createTimelineItem("Jan 28, 2026", "Maintenance completed", QColor("#4A90E2")));
    timelineL->addWidget(createTimelineItem("Jan 25, 2026", "Under maintenance", W_ORANGE));
    timelineL->addStretch(1);

    chartsL->addWidget(usageChartFrame);
    chartsL->addWidget(timelineFrame);

    statsContainerL->addWidget(kpiWidget);
    statsContainerL->addWidget(chartsWidget, 1);

    outer4L->addWidget(statsContainer);

    // Maintenance History
    QFrame* historyFrame = softBox();
    QVBoxLayout* historyL = new QVBoxLayout(historyFrame);
    historyL->setContentsMargins(14,14,14,14);

    QLabel* historyTitle = new QLabel("<b>Maintenance History</b>");
    historyTitle->setStyleSheet("color: rgba(0,0,0,0.65); font-weight: 900; font-size: 14px;");
    historyL->addWidget(historyTitle);

    QTableWidget* historyTable = new QTableWidget(3, 3);
    historyTable->setHorizontalHeaderLabels({"Date", "Type", "Technician"});
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

    setHistoryRow(0, "12/15/2025", "Regular Maintenance", "Tech-001");
    setHistoryRow(1, "09/15/2025", "Calibration", "Tech-002");
    setHistoryRow(2, "06/15/2025", "Repair", "Tech-001");

    historyL->addWidget(historyTable);
    outer4L->addWidget(historyFrame);

    // ========== PERFORMANCE METRICS ==========
    QFrame* metricsContainer = new QFrame;
    metricsContainer->setStyleSheet("QFrame{ background: transparent; }");
    QHBoxLayout* metricsL = new QHBoxLayout(metricsContainer);
    metricsL->setContentsMargins(0,0,0,0);
    metricsL->setSpacing(12);

    // Metric Card Helper
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

    metricsL->addWidget(createMetricCard("⚡", "POWER ON CYCLES", "1,247", "↑ 12% this month"));
    metricsL->addWidget(createMetricCard("🔧", "MAINTENANCE COUNT", "18", "Last: 15 days ago"));
    metricsL->addWidget(createMetricCard("⏱️", "AVG. SESSION TIME", "4.2h", "↓ 8% vs. last month"));
    metricsL->addWidget(createMetricCard("📊", "SUCCESS RATE", "96.8%", "↑ 2.1% improvement"));

    outer4L->addWidget(metricsContainer);

    scrollL->addWidget(outer4);
    scrollArea->setWidget(scrollContent);

    p4->addWidget(scrollArea, 1);

    QFrame* bottom4 = new QFrame;
    bottom4->setFixedHeight(64);
    bottom4->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* bottom4L = new QHBoxLayout(bottom4);
    bottom4L->setContentsMargins(14,10,14,10);
    bottom4L->setSpacing(12);

    QPushButton* back4 = actionBtn("Back", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_ArrowBack), true);
    QPushButton* editFromDetails = actionBtn("Edit Equipment", "rgba(10,95,88,0.45)", "rgba(255,255,255,0.90)", st->standardIcon(QStyle::SP_FileDialogContentsView), true);

    bottom4L->addWidget(back4);
    bottom4L->addWidget(editFromDetails);
    bottom4L->addStretch(1);

    p4->addWidget(bottom4);
    stack->addWidget(page4);

    // ==========================================================
    // NAVIGATION
    // ==========================================================
    connect(btnAdd,  &QPushButton::clicked, this, [=]{ setWindowTitle("Add / Edit Equipment"); stack->setCurrentIndex(1); });
    connect(btnEdit, &QPushButton::clicked, this, [=]{ setWindowTitle("Add / Edit Equipment"); stack->setCurrentIndex(1); });

    // Delete button with confirmation dialog
    connect(btnDel, &QPushButton::clicked, this, [=](){
        QMessageBox msgBox;
        msgBox.setWindowTitle("Confirm Delete");
        msgBox.setText("Are you sure you want to delete this equipment?");
        msgBox.setInformativeText("This action cannot be undone.");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);

        if (msgBox.exec() == QMessageBox::Yes) {
            // Delete the selected row
            int currentRow = table->currentRow();
            if (currentRow >= 0) {
                table->removeRow(currentRow);
                QMessageBox::information(this, "Success", "Equipment deleted successfully!");
            }
        }
    });

    // Details button
    connect(btnDet, &QPushButton::clicked, this, [=]{
        setWindowTitle("Equipment Details");
        stack->setCurrentIndex(3);
    });

    connect(cancelBtn, &QPushButton::clicked, this, [=]{ setWindowTitle("Laboratory Equipment Management"); stack->setCurrentIndex(0); });
    connect(saveBtn,   &QPushButton::clicked, this, [=]{ setWindowTitle("Laboratory Equipment Management"); stack->setCurrentIndex(0); });

    connect(btnMore, &QPushButton::clicked, this, [=]{ setWindowTitle("Equipment Location & Storage"); stack->setCurrentIndex(2); });

    connect(back3, &QPushButton::clicked, this, [=]{ setWindowTitle("Laboratory Equipment Management"); stack->setCurrentIndex(0); });

    connect(details3, &QPushButton::clicked, this, [=]{ setWindowTitle("Equipment Location & Storage"); stack->setCurrentIndex(2); });

    connect(back4, &QPushButton::clicked, this, [=]{ setWindowTitle("Laboratory Equipment Management"); stack->setCurrentIndex(0); });
    connect(editFromDetails, &QPushButton::clicked, this, [=]{ setWindowTitle("Add / Edit Equipment"); stack->setCurrentIndex(1); });

    // start
    setWindowTitle("Laboratory Equipment Management");
    stack->setCurrentIndex(0);
}
