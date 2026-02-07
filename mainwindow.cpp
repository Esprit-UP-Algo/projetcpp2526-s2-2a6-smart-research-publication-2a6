// ===================== mainwindow.cpp (COMPLET & CORRIGÉ - FR + STATISTIQUES) =====================
#include "mainwindow.h"

#include <QPainterPath>
#include <QDialog>
#include <QMessageBox>

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

static int uiMargin(QWidget* w)
{
    int W = w->width();
    if (W < 1100) return 6;    // petit écran
    if (W < 1400) return 10;   // écran moyen
    return 14;                 // grand écran
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

// ===== Brand (logo + 3 dots) used in ALL topbars =====
static QWidget* makeBrandWidget()
{
    QWidget* w = new QWidget;
    QHBoxLayout* h = new QHBoxLayout(w);
    h->setContentsMargins(0,0,0,0);
    h->setSpacing(10);
    h->setAlignment(Qt::AlignVCenter);

    QLabel* logo = new QLabel;
    logo->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    logo->setContentsMargins(0, 4, 0, 0);

    QPixmap px(":image/smartvision.png");
    if (!px.isNull())
        logo->setPixmap(px.scaled(36, 36, Qt::KeepAspectRatio, Qt::SmoothTransformation));

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

// ===================== Widget1 badge delegate =====================
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

// ===================== Widget STATISTIQUES (Graphiques) =====================
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
        p.drawText(inner, Qt::AlignCenter, "🧪");
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

// ===================== Widget4 helpers =====================
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

// ===================== Dialog: Delete list with buttons =====================
class DeleteDialog : public QDialog
{
public:
    DeleteDialog(QStyle* st, QWidget* parent=nullptr)
        : QDialog(parent), m_style(st)
    {
        setWindowTitle("Supprimer des échantillons");
        resize(800, 420);
        setModal(true);

        QVBoxLayout* root = new QVBoxLayout(this);
        root->setContentsMargins(14,14,14,14);
        root->setSpacing(12);

        QLabel* title = new QLabel("Liste des échantillons");
        QFont tf = title->font(); tf.setBold(true); tf.setPointSize(12);
        title->setFont(tf);
        title->setStyleSheet("color: rgba(0,0,0,0.65);");
        root->addWidget(title);

        table = new QTableWidget(0, 5, this);
        table->setHorizontalHeaderLabels({"Type","Organisme","Stockage","Expiration","Action"});
        table->verticalHeader()->setVisible(false);
        table->horizontalHeader()->setStretchLastSection(true);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSelectionMode(QAbstractItemView::NoSelection);

        table->setStyleSheet(R"(
            QHeaderView::section{
                background: rgba(159,190,185,0.85);
                padding: 8px;
                font-weight: 900;
            }
            QTableWidget{
                background: rgba(255,255,255,0.75);
                border: 1px solid rgba(0,0,0,0.12);
                border-radius: 12px;
            }
            QTableWidget::item{
                padding: 8px;
                font-weight: 800;
                color: rgba(0,0,0,0.65);
            }
        )");

        root->addWidget(table, 1);

        QPushButton* closeBtn = new QPushButton("Fermer");
        closeBtn->setCursor(Qt::PointingHandCursor);
        closeBtn->setStyleSheet("QPushButton{ padding:10px 18px; border-radius:10px; font-weight:900; }");
        QObject::connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
        root->addWidget(closeBtn, 0, Qt::AlignRight);

        // exemples
        addRow("DNA","E. coli","-80°C","16/05/2024");
        addRow("RNA","Humain","-20°C","01/05/2024");
        addRow("Protéine","Levure","-20°C","20/04/2024");
        addRow("DNA","Souris","Temp. ambiante","24/04/2024");
    }

private:
    QStyle* m_style = nullptr;
    QTableWidget* table = nullptr;

    void addRow(const QString& type,
                const QString& org,
                const QString& storage,
                const QString& expiry)
    {
        int r = table->rowCount();
        table->insertRow(r);

        table->setItem(r,0,new QTableWidgetItem(type));
        table->setItem(r,1,new QTableWidgetItem(org));
        table->setItem(r,2,new QTableWidgetItem(storage));
        table->setItem(r,3,new QTableWidgetItem(expiry));

        for (int c=0;c<4;++c)
            table->item(r,c)->setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);

        QPushButton* del = new QPushButton("Supprimer");
        del->setCursor(Qt::PointingHandCursor);
        del->setStyleSheet("QPushButton{ color:#B14A4A; font-weight:900; padding:6px 12px; }");

        QObject::connect(del, &QPushButton::clicked, this, [this, del](){
            int row = -1;
            for (int i=0; i<table->rowCount(); ++i) {
                if (table->cellWidget(i,4) == del) { row = i; break; }
            }
            if (row < 0) return;

            auto rep = QMessageBox::question(
                this,
                "Confirmation",
                "Voulez-vous supprimer cet échantillon ?",
                QMessageBox::Yes | QMessageBox::No
                );
            if (rep == QMessageBox::Yes) {
                table->removeRow(row);
            }
        });

        table->setCellWidget(r, 4, del);
        table->setRowHeight(r, 42);
    }
};

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

        QLabel* t = new QLabel("Supprimer l’échantillon ?");
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
    // PAGE 0 : Widget 1 - Gestion des Échantillons
    // ==========================================================
    QWidget* page1 = new QWidget;
    QVBoxLayout* p1 = new QVBoxLayout(page1);
    p1->setContentsMargins(22, 18, 22, 18);
    p1->setSpacing(14);

    p1->addWidget(makeTopBar(st, "Gestion des Échantillons"));

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
    table->setColumnWidth(2, 120);
    table->setColumnWidth(3, 170);
    table->setColumnWidth(4, 120);
    table->setColumnWidth(5, 140);
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

    // ✅ CHANGÉ: Détails -> Statistiques (ouvre la page Statistiques)
    QPushButton* btnStats = actionBtn("Statistiques", "rgba(255,255,255,0.55)", C_TEXT_DARK, st->standardIcon(QStyle::SP_ComputerIcon), true);

    QObject::connect(btnDel, &QPushButton::clicked, this, [=](){
        int r = table->currentRow();

        if (r < 0) {
            QDialog dlg(this);
            dlg.setModal(true);
            dlg.setWindowTitle("Information");
            dlg.setFixedSize(420, 170);
            dlg.setStyleSheet(QString("QDialog{ background:%1; border:1px solid %2; border-radius:14px; }")
                                  .arg(C_PANEL_BG, C_PANEL_BR));

            QVBoxLayout* L = new QVBoxLayout(&dlg);
            L->setContentsMargins(14,14,14,14);
            L->setSpacing(12);

            QLabel* t = new QLabel("Veuillez sélectionner une ligne à supprimer.");
            t->setStyleSheet("color: rgba(0,0,0,0.65); font-weight:900;");
            t->setWordWrap(true);

            QPushButton* ok = new QPushButton("OK");
            ok->setCursor(Qt::PointingHandCursor);
            ok->setStyleSheet("QPushButton{ background: rgba(10,95,88,0.45); color:white; font-weight:900; border-radius:12px; padding:10px 16px; }"
                              "QPushButton:hover{ background: rgba(18,68,59,0.95);}");

            QObject::connect(ok, &QPushButton::clicked, &dlg, &QDialog::accept);

            L->addWidget(t);
            L->addStretch(1);
            L->addWidget(ok, 0, Qt::AlignRight);

            dlg.exec();
            return;
        }

        auto cell = [&](int c)->QString{
            QTableWidgetItem* it = table->item(r, c);
            return it ? it->text() : "";
        };

        QString resume = QString("Type : %1   |   Organisme : %2   |   Stockage : %3   |   Expiration : %4")
                             .arg(cell(1), cell(2), cell(3), cell(5));

        ConfirmDeleteDialog confirm(style(), resume, this);

        if (confirm.exec() == QDialog::Accepted) {
            table->removeRow(r);
        }
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
    // PAGE 1 : Widget 2 - Ajouter / Modifier
    // ==========================================================
    QWidget* page2 = new QWidget;
    QVBoxLayout* p2 = new QVBoxLayout(page2);
    p2->setContentsMargins(22, 18, 22, 18);
    p2->setSpacing(14);

    p2->addWidget(makeTopBar(st, "Ajouter / Modifier un échantillon"));

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
    // PAGE 2 : Widget 3 - Localisation & Stockage
    // ==========================================================
    QWidget* page3 = new QWidget;
    QVBoxLayout* p3 = new QVBoxLayout(page3);
    p3->setContentsMargins(22, 18, 22, 18);
    p3->setSpacing(14);

    p3->addWidget(makeTopBar(st, "Localisation & Stockage"));

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
    // PAGE 3 : Widget 4 - Rack + Contraintes
    // ==========================================================
    QWidget* page4 = new QWidget;
    QVBoxLayout* p4 = new QVBoxLayout(page4);
    p4->setContentsMargins(22, 18, 22, 18);
    p4->setSpacing(14);

    p4->addWidget(makeTopBar(st, "Localisation & Stockage"));

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
    // PAGE 4 : STATISTIQUES (2 graphes demandés)
    //   1) Camembert: Répartition par type d’échantillon
    //   2) Histogramme: Nombre d’échantillons collectés par mois
    // ==========================================================
    QWidget* page5 = new QWidget;
    QVBoxLayout* p5 = new QVBoxLayout(page5);
    p5->setContentsMargins(22, 18, 22, 18);
    p5->setSpacing(14);

    p5->addWidget(makeTopBar(st, "Statistiques"));

    QFrame* outer5 = new QFrame;
    outer5->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }").arg(C_PANEL_BG, C_PANEL_BR));
    QVBoxLayout* outer5L = new QVBoxLayout(outer5);
    outer5L->setContentsMargins(12,12,12,12);
    outer5L->setSpacing(12);

    // Barre d’action simple (optionnel)
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

    // Dashboard (3 colonnes)
    QFrame* dash = new QFrame;
    dash->setStyleSheet("QFrame{ background: rgba(255,255,255,0.55); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QHBoxLayout* dashL = new QHBoxLayout(dash);
    dashL->setContentsMargins(12,12,12,12);
    dashL->setSpacing(12);

    // --- Camembert: répartition par type ---
    QFrame* pieCard = softBox();
    QVBoxLayout* pcL = new QVBoxLayout(pieCard);
    pcL->setContentsMargins(12,12,12,12);
    pcL->setSpacing(10);

    QLabel* pieTitle = new QLabel("Répartition par type d’échantillon");
    pieTitle->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");

    DonutChart* pie = new DonutChart;
    // Exemple de données (à remplacer par tes vraies stats)
    pie->setData({
        {95,  QColor("#9FBEB9"), "DNA"},
        {55,  W_GREEN,           "RNA"},
        {30,  W_ORANGE,          "Protéine"}
    });

    pcL->addWidget(pieTitle);
    pcL->addWidget(pie, 1);

    // --- Légende (type) ---
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

    // --- Histogramme: nombre collecté par mois ---
    QFrame* barCard = softBox();
    QVBoxLayout* bcL = new QVBoxLayout(barCard);
    bcL->setContentsMargins(12,12,12,12);
    bcL->setSpacing(10);

    QLabel* barTitle = new QLabel("Nombre d’échantillons collectés par mois");
    barTitle->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900;");

    BarChart* bars = new BarChart;
    // Exemple de données (12 mois). Remplace par tes vraies stats.
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

    // ✅ Appliquer les marges adaptatives
    p1->setContentsMargins(uiMargin(this), uiMargin(this), uiMargin(this), uiMargin(this));
    p2->setContentsMargins(uiMargin(this), uiMargin(this), uiMargin(this), uiMargin(this));
    p3->setContentsMargins(uiMargin(this), uiMargin(this), uiMargin(this), uiMargin(this));
    p4->setContentsMargins(uiMargin(this), uiMargin(this), uiMargin(this), uiMargin(this));
    p5->setContentsMargins(uiMargin(this), uiMargin(this), uiMargin(this), uiMargin(this));

    // ==========================================================
    // NAVIGATION
    // ==========================================================
    QObject::connect(btnAdd,  &QPushButton::clicked, this, [=]{ setWindowTitle("Ajouter / Modifier un échantillon"); stack->setCurrentIndex(1); });
    QObject::connect(btnEdit, &QPushButton::clicked, this, [=]{ setWindowTitle("Ajouter / Modifier un échantillon"); stack->setCurrentIndex(1); });

    QObject::connect(cancelBtn, &QPushButton::clicked, this, [=]{ setWindowTitle("Gestion des Échantillons"); stack->setCurrentIndex(0); });
    QObject::connect(saveBtn,   &QPushButton::clicked, this, [=]{ setWindowTitle("Gestion des Échantillons"); stack->setCurrentIndex(0); });

    QObject::connect(btnMore, &QPushButton::clicked, this, [=]{ setWindowTitle("Localisation & Stockage"); stack->setCurrentIndex(2); });
    QObject::connect(back3,   &QPushButton::clicked, this, [=]{ setWindowTitle("Gestion des Échantillons"); stack->setCurrentIndex(0); });

    QObject::connect(details3, &QPushButton::clicked, this, [=]{ setWindowTitle("Localisation & Stockage"); stack->setCurrentIndex(3); });
    QObject::connect(back4,    &QPushButton::clicked, this, [=]{ setWindowTitle("Localisation & Stockage"); stack->setCurrentIndex(2); });

    // ✅ Accès aux statistiques depuis page4 + depuis le bouton du bas page1
    QObject::connect(btnSec,   &QPushButton::clicked, this, [=]{ setWindowTitle("Statistiques"); stack->setCurrentIndex(4); });
    QObject::connect(btnStats, &QPushButton::clicked, this, [=]{ setWindowTitle("Statistiques"); stack->setCurrentIndex(4); });

    // ✅ Retour statistiques -> gestion
    QObject::connect(back5,  &QPushButton::clicked, this, [=]{ setWindowTitle("Gestion des Échantillons"); stack->setCurrentIndex(0); });

    setWindowTitle("Gestion des Échantillons");
    stack->setCurrentIndex(0);
}
