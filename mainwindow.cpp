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
#include <QMap>
#include <QMessageBox>

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

// ===================== Widget1 table badge delegate =====================
enum class FTStatus { FullTime=0, PartTime=1, Contract=2, OnLeave=3 };

static QString statusText(FTStatus s)
{
    switch (s) {
    case FTStatus::FullTime: return "Plein";
    case FTStatus::PartTime: return "Partiel";
    case FTStatus::Contract: return "Contrat";
    case FTStatus::OnLeave:  return "Absence";
    }
    return "Plein";
}

static QColor statusColor(FTStatus s)
{
    switch (s) {
    case FTStatus::FullTime: return QColor("#2E6F63");  // green
    case FTStatus::PartTime: return QColor("#B5672C");  // orange
    case FTStatus::Contract: return QColor("#7A8B8A");  // gray
    case FTStatus::OnLeave:  return QColor("#8B2F3C");  // red
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
        FTStatus st = FTStatus::FullTime;
        if (v.isValid()) st = static_cast<FTStatus>(v.toInt());

        QStyledItemDelegate::paint(p, opt, idx);

        p->save();
        p->setRenderHint(QPainter::Antialiasing, true);

        QRect r = opt.rect.adjusted(8, 6, -8, -6);
        int h = qMin(r.height(), 28);
        int w = qMin(r.width(), 120);
        QRect pill(r.left() + (r.width() - w)/2, r.top() + (r.height()-h)/2, w, h);

        QColor bg = statusColor(st);
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
        p->drawText(textRect, Qt::AlignVCenter|Qt::AlignLeft, statusText(st));

        p->restore();
    }
};

// ===================== GradientRowWidget reused for employees =====================
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

static QFrame* infoBlock(QStyle* st, const QString& line1, const QString& line2)
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

static QFrame* bottomBarWithText(QStyle* st, const QString& text)
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

// ===================== Widget5 charts =====================
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
        p.drawText(inner, Qt::AlignCenter, "👥");
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
        p.drawText(QRect(r.left()+10, r.top()+4, r.width()-20, 16), Qt::AlignLeft|Qt::AlignVCenter, "Publications");

        QList<int> ticks = {100, 80, 50, 20};
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

// ===================== Widget4 helpers =====================
static QFrame* filterPill(const QString& text)
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

static void setupAvailabilityGrid(QTableWidget* grid)
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

    QStringList dayNames;
    dayNames << "Lun" << "Mar" << "Mer" << "Jeu" << "Ven" << "Sam";

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

static void setupConstraintsTable(QTableWidget* t)
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

// ===================== MainWindow =====================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
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
    // PAGE 0 : Widget 1 - Gestion des Employés
    // ==========================================================
    QWidget* page1 = new QWidget;
    QVBoxLayout* p1 = new QVBoxLayout(page1);
    p1->setContentsMargins(22, 18, 22, 18);
    p1->setSpacing(14);

    p1->addWidget(makeTopBar(st, "Gestion des Employés"));

    QFrame* bar1 = new QFrame;
    bar1->setFixedHeight(54);
    bar1->setStyleSheet("background: rgba(255,255,255,0.22); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* bar1L = new QHBoxLayout(bar1);
    bar1L->setContentsMargins(14, 8, 14, 8);
    bar1L->setSpacing(10);

    QLineEdit* search = new QLineEdit;
    search->setPlaceholderText("Rechercher par CIN, Nom, Prénom, Rôle, Laboratoire…");
    search->addAction(st->standardIcon(QStyle::SP_FileDialogContentsView), QLineEdit::LeadingPosition);

    QComboBox* cbRole = new QComboBox; cbRole->addItems({"Rôle", "Chercheur", "Technicien"});
    QComboBox* cbSpec = new QComboBox; cbSpec->addItems({"Spécialisation", "Biomol", "Bioinfo", "Chimie", "Général"});
    QComboBox* cbLab  = new QComboBox; cbLab->addItems({"Laboratoire", "Lab A", "Lab B", "Lab C"});
    QComboBox* cbFT   = new QComboBox; cbFT->addItems({"Temps", "Plein", "Partiel", "Contrat", "Absence"});

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
    bar1L->addWidget(cbRole);
    bar1L->addWidget(cbSpec);
    bar1L->addWidget(cbLab);
    bar1L->addWidget(cbFT);
    bar1L->addWidget(filters);
    p1->addWidget(bar1);

    QFrame* card1 = makeCard();
    QVBoxLayout* card1L = new QVBoxLayout(card1);
    card1L->setContentsMargins(10,10,10,10);

    QTableWidget* table = new QTableWidget(6, 12);
    table->setHorizontalHeaderLabels({"", "Employé ID", "CIN", "Nom", "Prénom", "Rôle", "Spécialisation", "Qualification", "Publications", "Temps", "Laboratoire", "Projet"});
    table->verticalHeader()->setVisible(false);
    table->setShowGrid(true);
    table->setAlternatingRowColors(true);
    table->setStyleSheet(QString("QTableWidget{ alternate-background-color:%1; background-color:%2; }").arg(C_ROW_EVEN, C_ROW_ODD));
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setItemDelegateForColumn(9, new BadgeDelegate(table)); // "Temps" badge

    table->setColumnWidth(0, 36);
    table->setColumnWidth(1, 100);
    table->setColumnWidth(2, 110);
    table->setColumnWidth(3, 120);
    table->setColumnWidth(4, 120);
    table->setColumnWidth(5, 120);
    table->setColumnWidth(6, 140);
    table->setColumnWidth(7, 140);
    table->setColumnWidth(8, 110);
    table->setColumnWidth(9, 120);
    table->setColumnWidth(10, 110);
    table->setColumnWidth(11, 140);

    auto setRow=[&](int r, const QString& empId, const QString& cin,
                      const QString& nom, const QString& prenom,
                      const QString& role, const QString& spec,
                      const QString& qualif, const QString& pubs,
                      FTStatus ft, const QString& lab, const QString& proj)
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

        table->setItem(r, 1, mk(empId));
        table->setItem(r, 2, mk(cin));
        table->setItem(r, 3, mk(nom));
        table->setItem(r, 4, mk(prenom));
        table->setItem(r, 5, mk(role));
        table->setItem(r, 6, mk(spec));
        table->setItem(r, 7, mk(qualif));
        QTableWidgetItem* p = mk(pubs);
        p->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
        table->setItem(r, 8, p);

        QTableWidgetItem* badge = new QTableWidgetItem;
        badge->setData(Qt::UserRole, (int)ft);
        table->setItem(r, 9, badge);

        table->setItem(r,10, mk(lab));
        table->setItem(r,11, mk(proj));

        table->setRowHeight(r, 46);
    };

    setRow(0, "E001", "AA123456", "Ali", "Ben Salem", "Chercheur", "Biomol", "PhD",  "25", FTStatus::FullTime, "Lab A", "Projet GENOME");
    setRow(1, "E002", "BB654321", "Sara", "Bouaziz", "Technicien", "Chimie",  "BSc",  "5",  FTStatus::PartTime, "Lab B", "—");
    setRow(2, "E003", "CC998877", "Youssef", "K.",    "Chercheur", "Bioinfo", "PhD",  "12", FTStatus::Contract, "Lab C", "Projet AI-BIO");
    setRow(3, "E004", "DD112233", "Meriem", "H.",     "Technicien","Général", "BTS",  "2",  FTStatus::FullTime, "Lab A", "—");
    setRow(4, "E005", "EE667788", "Omar",   "A.",     "Chercheur", "Biomol",  "PhD",  "40", FTStatus::OnLeave,  "Lab B", "Projet PROTEO");
    setRow(5, "E006", "FF334455", "Nada",   "B.",     "Chercheur", "Chimie",  "MSc",  "10", FTStatus::FullTime, "Lab C", "Projet MATERIA");

    card1L->addWidget(table);
    p1->addWidget(card1, 1);

    QFrame* bottom1 = new QFrame;
    bottom1->setFixedHeight(64);
    bottom1->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* bottom1L = new QHBoxLayout(bottom1);
    bottom1L->setContentsMargins(14,10,14,10);
    bottom1L->setSpacing(12);

    QPushButton* btnAdd    = actionBtn("Créer",        "rgba(10,95,88,0.45)", "rgba(255,255,255,0.90)", st->standardIcon(QStyle::SP_DialogYesButton), true);
    QPushButton* btnEdit   = actionBtn("Modifier",     "rgba(198,178,154,0.55)", "rgba(255,255,255,0.85)", st->standardIcon(QStyle::SP_FileDialogContentsView), true);
    QPushButton* btnDel    = actionBtn("Supprimer",    "rgba(255,255,255,0.55)", "#B14A4A", st->standardIcon(QStyle::SP_TrashIcon), true);
    // Renamed button: Statistiques (was Détails)
    QPushButton* btnStats  = actionBtn("Statistiques", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_MessageBoxInformation), true);

    bottom1L->addWidget(btnAdd);
    bottom1L->addWidget(btnEdit);
    bottom1L->addWidget(btnDel);
    bottom1L->addWidget(btnStats);
    bottom1L->addStretch(1);

    bottom1L->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_DirIcon)));
    bottom1L->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_FileIcon)));
    bottom1L->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_DialogSaveButton)));
    bottom1L->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_BrowserReload)));

    QPushButton* btnMore = new QPushButton(st->standardIcon(QStyle::SP_FileDialogContentsView), "  Affectations & Labs");
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
    // PAGE 1 : Widget 2 - Créer / Modifier Employé
    // ==========================================================
    QWidget* page2 = new QWidget;
    QVBoxLayout* p2 = new QVBoxLayout(page2);
    p2->setContentsMargins(22, 18, 22, 18);
    p2->setSpacing(14);

    p2->addWidget(makeTopBar(st, "Créer / Modifier Employé"));

    QFrame* outer2 = new QFrame;
    outer2->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }").arg(C_PANEL_BG, C_PANEL_BR));
    QHBoxLayout* outer2L = new QHBoxLayout(outer2);
    outer2L->setContentsMargins(12,12,12,12);
    outer2L->setSpacing(12);

    QFrame* left2 = softBox();
    left2->setFixedWidth(280);
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

    leftAction("Rôle", QStyle::SP_FileIcon, "Chercheur / Technicien");

    QToolButton* orgBtn = new QToolButton;
    orgBtn->setIcon(st->standardIcon(QStyle::SP_DirIcon));
    orgBtn->setText("  Spécialisation");
    orgBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    orgBtn->setCursor(Qt::PointingHandCursor);
    orgBtn->setStyleSheet(R"(
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
    left2L->addWidget(orgBtn);

    QLabel* colHead = new QLabel("Affectations");
    colHead->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    left2L->addWidget(colHead);

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

    left2L->addWidget(colBtn(QStyle::SP_DriveHDIcon, "Laboratoire"));
    left2L->addWidget(colBtn(QStyle::SP_FileDialogListView, "Projet"));
    left2L->addWidget(colBtn(QStyle::SP_ArrowDown, "Temps Plein / Partiel"));
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
    addDrop->setText("Ajouter");
    addDrop->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    addDrop->setStyleSheet("QToolButton{ color: rgba(0,0,0,0.55); font-weight: 900; }");

    QLabel* idLbl = new QLabel("E007");
    idLbl->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");

    tinyTopL->addWidget(addDrop);
    tinyTopL->addSpacing(10);
    tinyTopL->addWidget(idLbl);
    tinyTopL->addStretch(1);
    right2L->addWidget(tinyTop);

    auto comboRow = [&](QWidget* wLeft, QWidget* wRight){
        QFrame* r = softBox();
        QHBoxLayout* l = new QHBoxLayout(r);
        l->setContentsMargins(10,8,10,8);
        l->setSpacing(8);
        l->addWidget(wLeft, 1);
        l->addWidget(wRight, 1);
        return r;
    };

    QLineEdit* cinEdit = new QLineEdit; cinEdit->setPlaceholderText("CIN");
    QLineEdit* nomEdit = new QLineEdit; nomEdit->setPlaceholderText("Nom");
    right2L->addWidget(comboRow(cinEdit, nomEdit));

    QLineEdit* prenomEdit = new QLineEdit; prenomEdit->setPlaceholderText("Prénom");
    QComboBox* roleCb = new QComboBox; roleCb->addItems({"Chercheur","Technicien"});
    right2L->addWidget(comboRow(prenomEdit, roleCb));

    QComboBox* specCb = new QComboBox; specCb->addItems({"Biomol","Bioinfo","Chimie","Général"});
    QLineEdit* qualifEdit = new QLineEdit; qualifEdit->setPlaceholderText("Qualification (PhD, MSc…)");
    right2L->addWidget(comboRow(specCb, qualifEdit));

    QSpinBox* pubs = new QSpinBox; pubs->setRange(0,1000); pubs->setValue(0);
    pubs->setPrefix("Pub: ");
    QComboBox* ftCb = new QComboBox; ftCb->addItems({"Plein","Partiel","Contrat","Absence"});
    right2L->addWidget(comboRow(pubs, ftCb));

    QComboBox* labCb = new QComboBox; labCb->addItems({"Lab A","Lab B","Lab C"});
    QComboBox* projCb = new QComboBox; projCb->addItems({"—","Projet GENOME","Projet AI-BIO","Projet PROTEO","Projet MATERIA"});
    right2L->addWidget(comboRow(labCb, projCb));

    QFrame* dateRow = softBox();
    QHBoxLayout* dateRowL = new QHBoxLayout(dateRow);
    dateRowL->setContentsMargins(10,8,10,8);
    dateRowL->setSpacing(8);

    QToolButton* cal = new QToolButton; cal->setAutoRaise(true); cal->setIcon(st->standardIcon(QStyle::SP_FileDialogDetailedView));
    QDateEdit* date = new QDateEdit(QDate::currentDate());
    date->setCalendarPopup(true);
    date->setDisplayFormat("MM/dd/yyyy");
    date->setStyleSheet("QDateEdit{ background: transparent; border:0; font-weight: 900; color: rgba(0,0,0,0.55);} ");

    QToolButton* i1 = new QToolButton; i1->setAutoRaise(true); i1->setIcon(st->standardIcon(QStyle::SP_BrowserReload));
    QToolButton* i2 = new QToolButton; i2->setAutoRaise(true); i2->setIcon(st->standardIcon(QStyle::SP_FileDialogListView));
    QToolButton* i3 = new QToolButton; i3->setAutoRaise(true); i3->setIcon(st->standardIcon(QStyle::SP_DialogSaveButton));

    dateRowL->addWidget(cal);
    dateRowL->addWidget(new QLabel("Date d’embauche: "));
    dateRowL->addWidget(date, 1);
    dateRowL->addWidget(i1);
    dateRowL->addWidget(i2);
    dateRowL->addWidget(i3);
    right2L->addWidget(dateRow);

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
    // PAGE 2 : Widget 3 - Affectations & Laboratoires
    // ==========================================================
    QWidget* page3 = new QWidget;
    QVBoxLayout* p3 = new QVBoxLayout(page3);
    p3->setContentsMargins(22, 18, 22, 18);
    p3->setSpacing(14);

    p3->addWidget(makeTopBar(st, "Affectations & Laboratoires"));

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

    QLabel* ddText = new QLabel("Laboratoire: Lab A");
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

    auto* tF1 = new QTreeWidgetItem(tree3, QStringList() << "Lab A");
    auto* tF2 = new QTreeWidgetItem(tree3, QStringList() << "Lab B");
    auto* tF3 = new QTreeWidgetItem(tree3, QStringList() << "Lab C");

    tF1->setIcon(0, st->standardIcon(QStyle::SP_DirIcon));
    tF2->setIcon(0, st->standardIcon(QStyle::SP_DirIcon));
    tF3->setIcon(0, st->standardIcon(QStyle::SP_DirIcon));

    auto* tA = new QTreeWidgetItem(tF2, QStringList() << "Projet AI-BIO");
    auto* t6 = new QTreeWidgetItem(tF2, QStringList() << "Projet GENOME");
    auto* tB = new QTreeWidgetItem(tF2, QStringList() << "Projet PROTEO");
    auto* tR = new QTreeWidgetItem(tF2, QStringList() << "Groupe Techniciens");
    tA->setIcon(0, st->standardIcon(QStyle::SP_FileIcon));
    t6->setIcon(0, st->standardIcon(QStyle::SP_FileIcon));
    tB->setIcon(0, st->standardIcon(QStyle::SP_FileIcon));
    tR->setIcon(0, st->standardIcon(QStyle::SP_FileDialogInfoView));

    tree3->expandAll();
    tree3->setCurrentItem(tF2);

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
    header3L->addWidget(chip("Affectations"));
    header3L->addWidget(chip("Groupes"));

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

    addListRow(new GradientRowWidget(st, "Ali Ben Salem (Chercheur)", "Pub: 25", W_GREEN,  QStyle::SP_FileIcon, false));
    addListRow(new GradientRowWidget(st, "Sara Bouaziz (Technicien)", "Partiel", W_ORANGE, QStyle::SP_FileIcon, false));
    addListRow(new GradientRowWidget(st, "Youssef K. (Chercheur)",    "Contrat", W_GRAY,   QStyle::SP_FileIcon, false));
    addListRow(new GradientRowWidget(st, "Omar A. (Chercheur)",       "Absence", W_RED,    QStyle::SP_FileIcon, true));

    listBox3L->addWidget(list3);

    QWidget* bottomInfo3 = new QWidget;
    QHBoxLayout* bottomInfo3L = new QHBoxLayout(bottomInfo3);
    bottomInfo3L->setContentsMargins(0,0,0,0);
    bottomInfo3L->setSpacing(12);
    bottomInfo3L->addWidget(infoBlock(st, "Lab B: Chercheurs: 3, Techniciens: 2", "Disponibles: 4"));
    bottomInfo3L->addWidget(bottomBarWithText(st, "Lab B • Projet AI-BIO"), 1);

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
    // PAGE 3 : Widget 4 - Disponibilités & Contraintes
    // ==========================================================
    QWidget* page4 = new QWidget;
    QVBoxLayout* p4 = new QVBoxLayout(page4);
    p4->setContentsMargins(22, 18, 22, 18);
    p4->setSpacing(14);

    p4->addWidget(makeTopBar(st, "Disponibilités & Contraintes"));

    QFrame* outer4 = new QFrame;
    outer4->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }").arg(C_PANEL_BG, C_PANEL_BR));
    QHBoxLayout* outer4L = new QHBoxLayout(outer4);
    outer4L->setContentsMargins(12,12,12,12);
    outer4L->setSpacing(12);

    QFrame* left4 = softBox();
    left4->setFixedWidth(310);
    QVBoxLayout* left4L = new QVBoxLayout(left4);
    left4L->setContentsMargins(10,10,10,10);
    left4L->setSpacing(10);

    QFrame* dd4 = new QFrame;
    dd4->setStyleSheet("QFrame{ background: rgba(255,255,255,0.72); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QHBoxLayout* dd4L = new QHBoxLayout(dd4);
    dd4L->setContentsMargins(10,8,10,8);
    QLabel* dd4T = new QLabel("Laboratoires");
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

    auto* wf1 = new QTreeWidgetItem(tree4, QStringList() << "Lab A");
    auto* wf2 = new QTreeWidgetItem(tree4, QStringList() << "Lab B");
    auto* wf4 = new QTreeWidgetItem(tree4, QStringList() << "Lab C");
    wf1->setIcon(0, st->standardIcon(QStyle::SP_DirIcon));
    wf2->setIcon(0, st->standardIcon(QStyle::SP_DirIcon));
    wf4->setIcon(0, st->standardIcon(QStyle::SP_DirIcon));

    auto* wshA = new QTreeWidgetItem(wf2, QStringList() << "Projet AI-BIO");
    auto* wshB = new QTreeWidgetItem(wf2, QStringList() << "Projet PROTEO");
    auto* wsh6 = new QTreeWidgetItem(wf2, QStringList() << "Projet GENOME");
    auto* wrm  = new QTreeWidgetItem(wf2, QStringList() << "Techniciens");
    wshA->setIcon(0, st->standardIcon(QStyle::SP_FileIcon));
    wshB->setIcon(0, st->standardIcon(QStyle::SP_FileIcon));
    wsh6->setIcon(0, st->standardIcon(QStyle::SP_FileIcon));
    wrm ->setIcon(0, st->standardIcon(QStyle::SP_FileDialogInfoView));

    tree4->expandAll();
    tree4->setCurrentItem(wf2);

    QFrame* temp4 = infoBlock(st, "Lab B • FT: 3 / PT: 2", "Total: 5");

    QPushButton* export4 = actionBtn("Exporter Liste", "rgba(10,95,88,0.45)", "rgba(255,255,255,0.92)", st->standardIcon(QStyle::SP_DialogSaveButton), true);
    QPushButton* mark4   = actionBtn("Affecter au Projet", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_DialogApplyButton), true);

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
    fr->addWidget(filterPill("Rôle"));
    fr->addWidget(filterPill("Temps"));
    fr->addWidget(filterPill("Lab"));
    right4L->addWidget(filtersRow);

    QFrame* rackCard = new QFrame;
    rackCard->setStyleSheet("QFrame{ background: rgba(255,255,255,0.55); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QVBoxLayout* rackCardL = new QVBoxLayout(rackCard);
    rackCardL->setContentsMargins(12,12,12,12);
    QTableWidget* availability = new QTableWidget;
    setupAvailabilityGrid(availability);
    rackCardL->addWidget(availability);
    right4L->addWidget(rackCard);

    QFrame* accCard = new QFrame;
    accCard->setStyleSheet("QFrame{ background: rgba(255,255,255,0.55); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QVBoxLayout* accCardL = new QVBoxLayout(accCard);
    accCardL->setContentsMargins(12,12,12,12);
    QLabel* accTitle = new QLabel("Contraintes");
    accTitle->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    QTableWidget* accTable = new QTableWidget;
    setupConstraintsTable(accTable);
    accCardL->addWidget(accTitle);
    accCardL->addWidget(accTable);
    right4L->addWidget(accCard, 1);

    QWidget* w4BottomRight = new QWidget;
    QHBoxLayout* w4br = new QHBoxLayout(w4BottomRight);
    w4br->setContentsMargins(0,0,0,0);
    w4br->setSpacing(10);
    w4br->addStretch(1);

    QPushButton* btnFolder = actionBtn("Lab", "rgba(255,255,255,0.72)", C_TEXT_DARK, st->standardIcon(QStyle::SP_DirIcon), true);
    QPushButton* btnSec    = actionBtn("Statistiques", "rgba(255,255,255,0.72)", C_TEXT_DARK, st->standardIcon(QStyle::SP_MessageBoxWarning), true);

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
    // PAGE 4 : Statistiques Employés
    // ==========================================================
    QWidget* pageStats = new QWidget;
    QVBoxLayout* ps = new QVBoxLayout(pageStats);
    ps->setContentsMargins(22, 18, 22, 18);
    ps->setSpacing(14);

    // Topbar
    ps->addWidget(makeTopBar(st, "Statistiques Employés"));

    // Contenu
    QFrame* outerStats = new QFrame;
    outerStats->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }").arg(C_PANEL_BG, C_PANEL_BR));
    QVBoxLayout* outerStatsL = new QVBoxLayout(outerStats);
    outerStatsL->setContentsMargins(12,12,12,12);
    outerStatsL->setSpacing(12);

    // Carte des graphiques
    QFrame* dash = new QFrame;
    dash->setStyleSheet("QFrame{ background: rgba(255,255,255,0.55); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QHBoxLayout* dashL = new QHBoxLayout(dash);
    dashL->setContentsMargins(12,12,12,12);
    dashL->setSpacing(12);

    // Donut (rôles)
    QFrame* donutCard = softBox();
    QVBoxLayout* dcL = new QVBoxLayout(donutCard);
    dcL->setContentsMargins(12,12,12,12);
    QLabel* totalLbl = new QLabel("Total Employés:");
    totalLbl->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");
    DonutChart* donutStats = new DonutChart;
    dcL->addWidget(totalLbl);
    dcL->addWidget(donutStats, 1);

    // Légende
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
    lgL->addWidget(legendRow(W_GREEN, "Chercheurs"));
    lgL->addWidget(legendRow(QColor("#9FBEB9"), "Techniciens"));
    lgL->addWidget(legendRow(W_ORANGE, "Temps partiel"));
    lgL->addWidget(legendRow(W_RED, "Absents"));
    lgL->addStretch(1);

    // Bar chart (spécialisations)
    QFrame* barCard = softBox();
    QVBoxLayout* bcL = new QVBoxLayout(barCard);
    bcL->setContentsMargins(12,12,12,12);
    BarChart* barStats = new BarChart;
    bcL->addWidget(barStats, 1);

    dashL->addWidget(donutCard, 1);
    dashL->addWidget(legendCard, 1);
    dashL->addWidget(barCard, 1);

    outerStatsL->addWidget(dash);
    ps->addWidget(outerStats, 1);

    // Bouton retour
    QFrame* bottomStats = new QFrame;
    bottomStats->setFixedHeight(64);
    bottomStats->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* bottomStatsL = new QHBoxLayout(bottomStats);
    bottomStatsL->setContentsMargins(14,10,14,10);
    QPushButton* backStats = actionBtn("Retour", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_ArrowBack), true);
    bottomStatsL->addWidget(backStats);
    bottomStatsL->addStretch(1);
    ps->addWidget(bottomStats);

    // Ajout au stack (index 4)
    stack->addWidget(pageStats);

    // Fonction de mise à jour des graphiques depuis le tableau
    auto updateStatsFromTable = [=](){
        QMap<QString,int> roleCount;
        QMap<QString,int> specCount;
        int total = table->rowCount();

        for (int r=0; r<table->rowCount(); ++r) {
            const QString role = table->item(r, 5)->text(); // colonne Rôle
            const QString spec = table->item(r, 6)->text(); // colonne Spécialisation
            roleCount[role] += 1;
            specCount[spec] += 1;
        }

        totalLbl->setText(QString("Total Employés: %1").arg(total));

        // Construire le donut (rôles)
        QList<DonutChart::Slice> slices;
        auto colorForRole = [&](const QString& role)->QColor{
            if (role == "Chercheur")  return W_GREEN;
            if (role == "Technicien") return QColor("#9FBEB9");
            return QColor("#7A8B8A");
        };
        for (auto it = roleCount.constBegin(); it != roleCount.constEnd(); ++it) {
            slices.push_back({(double)it.value(), colorForRole(it.key()), it.key()});
        }
        donutStats->setData(slices);

        // Construire la barre (spécialisations)
        QList<BarChart::Bar> bars;
        for (auto it = specCount.constBegin(); it != specCount.constEnd(); ++it) {
            bars.push_back({(double)it.value(), it.key()});
        }
        barStats->setData(bars);
    };

    // ==========================================================
    // NAVIGATION
    // ==========================================================
    connect(btnAdd,  &QPushButton::clicked, this, [=]{ setWindowTitle("Créer / Modifier Employé"); stack->setCurrentIndex(1); });
    connect(btnEdit, &QPushButton::clicked, this, [=]{ setWindowTitle("Créer / Modifier Employé"); stack->setCurrentIndex(1); });

    connect(cancelBtn, &QPushButton::clicked, this, [=]{ setWindowTitle("Gestion des Employés"); stack->setCurrentIndex(0); });
    connect(saveBtn,   &QPushButton::clicked, this, [=]{ setWindowTitle("Gestion des Employés"); stack->setCurrentIndex(0); });

    connect(btnMore, &QPushButton::clicked, this, [=]{ setWindowTitle("Affectations & Laboratoires"); stack->setCurrentIndex(2); });

    connect(back3, &QPushButton::clicked, this, [=]{ setWindowTitle("Gestion des Employés"); stack->setCurrentIndex(0); });

    connect(details3, &QPushButton::clicked, this, [=]{ setWindowTitle("Disponibilités & Contraintes"); stack->setCurrentIndex(3); });

    connect(back4, &QPushButton::clicked, this, [=]{ setWindowTitle("Affectations & Laboratoires"); stack->setCurrentIndex(2); });

    // New behavior: btnStats goes to Statistiques page and updates charts
    connect(btnStats, &QPushButton::clicked, this, [=]{
        setWindowTitle("Statistiques Employés");
        stack->setCurrentIndex(4);
        updateStatsFromTable();
    });

    // Also from the Disponibilités page shortcut button
    connect(btnSec, &QPushButton::clicked, this, [=]{
        setWindowTitle("Statistiques Employés");
        stack->setCurrentIndex(4);
        updateStatsFromTable();
    });

    // Retour depuis statistiques
    connect(backStats, &QPushButton::clicked, this, [=]{
        setWindowTitle("Gestion des Employés");
        stack->setCurrentIndex(0);
    });

    // Supprimer with stat refresh
    connect(btnDel, &QPushButton::clicked, this, [=]{
        int row = table->currentRow();
        if (row < 0) {
            QMessageBox::information(this, "Suppression", "Sélectionnez un employé dans la liste.");
            return;
        }
        const QString empId = table->item(row, 1)->text();
        auto ret = QMessageBox::question(this, "Confirmer la suppression",
                                         QString("Supprimer l’employé %1 ?").arg(empId),
                                         QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret == QMessageBox::Yes) {
            table->removeRow(row);
            updateStatsFromTable();
            // TODO: supprimer aussi côté modèle/base de données
        }
    });

    // start
    setWindowTitle("Gestion des Employés");
    stack->setCurrentIndex(0);
}
