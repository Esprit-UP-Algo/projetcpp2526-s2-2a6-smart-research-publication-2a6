#include "mainwindow.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QComboBox>
#include <QHeaderView>
#include <QFrame>
#include <QStackedWidget>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QToolButton>
#include <QGraphicsDropShadowEffect>
#include <QTreeWidget>
#include <QListWidget>
#include <QSpinBox>
#include <QDateEdit>
#include <QTextEdit>
#include <QDoubleSpinBox>
#include <QMessageBox>
#include <QDialog>
#include <QFont>
#include <QPainterPath>
#include <QtMath>
#include <algorithm>

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

static const QColor W_GREEN = QColor("#2E6F63");    // Published
static const QColor W_ORANGE = QColor("#B5672C");   // Under Review
static const QColor W_RED = QColor("#8B2F3C");      // Submitted
static const QColor W_BLUE = QColor("#365CF5");     // Accepted
static const QColor W_GRAY = QColor("#7A8B8A");

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
    f->setStyleSheet(QString("QFrame{ background:%1; border: 1px solid %2; border-radius: 12px; }").arg(C_PANEL_IN, C_PANEL_BR));
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
        QPushButton{ background:%1; color:%2; border:1px solid rgba(0,0,0,0.12);
                     border-radius:10px; padding:10px 18px; font-weight:800; }
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
        QToolButton{ background: rgba(255,255,255,0.55); border: 1px solid rgba(0,0,0,0.12);
                     border-radius: 10px; padding: 10px; }
        QToolButton:hover{ background: rgba(255,255,255,0.75); }
    )");
    return b;
}

// ===== Brand (logo + 3 dots) used in ALL topbars =====
static QWidget* makeBrandWidget() {
    QWidget* w = new QWidget;
    QHBoxLayout* h = new QHBoxLayout(w);
    h->setContentsMargins(0,0,0,0);
    h->setSpacing(10);
    h->setAlignment(Qt::AlignVCenter);

    // Logo
    QLabel* logo = new QLabel;
    logo->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    logo->setContentsMargins(0, 4, 0, 0);
    QPixmap px(":image/smartvision.png");
    if (!px.isNull())
        logo->setPixmap(px.scaled(36, 36, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    // Name
    QLabel* name = new QLabel("SmartVision");
    QFont nf = name->font();
    nf.setBold(true);
    nf.setPointSize(11);
    name->setFont(nf);
    name->setStyleSheet(QString("color:%1; letter-spacing: 0.6px;").arg(C_BEIGE));
    name->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    // 3 Dots
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

    // LEFT: brand
    L->addWidget(makeBrandWidget(), 0, Qt::AlignLeft);

    // CENTER: title
    QLabel* title = new QLabel(titleText);
    QFont f = title->font();
    f.setPointSize(14);
    f.setBold(true);
    title->setFont(f);
    title->setStyleSheet("color: rgba(255,255,255,0.90);");
    L->addStretch(1);
    L->addWidget(title, 0, Qt::AlignCenter);
    L->addStretch(1);

    // RIGHT: icons
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

// ===================== Publication Status Badge Delegate =====================
enum class PublicationStatus { Submitted=0, UnderReview=1, Accepted=2, Published=3 };

static QString statusText(PublicationStatus s) {
    switch (s) {
    case PublicationStatus::Submitted: return "Submitted";
    case PublicationStatus::UnderReview: return "Under Review";
    case PublicationStatus::Accepted: return "Accepted";
    case PublicationStatus::Published: return "Published";
    }
    return "Submitted";
}

static QColor statusColor(PublicationStatus s) {
    switch (s) {
    case PublicationStatus::Submitted: return W_RED;
    case PublicationStatus::UnderReview: return W_ORANGE;
    case PublicationStatus::Accepted: return W_BLUE;
    case PublicationStatus::Published: return W_GREEN;
    }
    return W_GRAY;
}

class StatusBadgeDelegate : public QStyledItemDelegate {
public:
    explicit StatusBadgeDelegate(QObject* parent=nullptr) : QStyledItemDelegate(parent) {}

    void paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &idx) const override {
        QVariant v = idx.data(Qt::UserRole);
        PublicationStatus st = PublicationStatus::Submitted;
        if (v.isValid()) st = static_cast<PublicationStatus>(v.toInt());

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

        // Icon circle
        QRect iconCircle(pill.left()+10, pill.top()+6, 16, 16);
        p->setBrush(QColor(255,255,255,35));
        p->drawEllipse(iconCircle);

        p->setPen(QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

        // Draw icon based on status
        if (st == PublicationStatus::Published) {
            // Checkmark
            QPoint a(iconCircle.left()+4, iconCircle.top()+9);
            QPoint b(iconCircle.left()+7, iconCircle.top()+12);
            QPoint c(iconCircle.left()+13, iconCircle.top()+5);
            p->drawLine(a,b);
            p->drawLine(b,c);
        } else if (st == PublicationStatus::UnderReview) {
            // Clock/Review icon
            p->drawEllipse(iconCircle.adjusted(2,2,-2,-2));
            p->drawLine(iconCircle.center(), QPoint(iconCircle.center().x(), iconCircle.top()+5));
        } else if (st == PublicationStatus::Accepted) {
            // Star icon
            p->drawLine(iconCircle.center().x(), iconCircle.top()+3, iconCircle.center().x(), iconCircle.bottom()-3);
            p->drawLine(iconCircle.left()+3, iconCircle.center().y(), iconCircle.right()-3, iconCircle.center().y());
        } else {
            // Upload arrow for Submitted
            p->drawLine(QPoint(iconCircle.center().x(), iconCircle.bottom()-3),
                        QPoint(iconCircle.center().x(), iconCircle.top()+5));
            p->drawLine(QPoint(iconCircle.center().x(), iconCircle.top()+5),
                        QPoint(iconCircle.left()+5, iconCircle.top()+8));
            p->drawLine(QPoint(iconCircle.center().x(), iconCircle.top()+5),
                        QPoint(iconCircle.right()-5, iconCircle.top()+8));
        }

        // Text
        p->setPen(Qt::white);
        QFont f = opt.font;
        f.setBold(true);
        f.setPointSizeF(f.pointSizeF()-0.5);
        p->setFont(f);
        QRect textRect = pill.adjusted(34, 4, -10, -4);
        p->drawText(textRect, Qt::AlignVCenter|Qt::AlignLeft, statusText(st));

        p->restore();
    }
};

// ===================== Citation Trend Chart =====================
class CitationTrendChart : public QWidget {
public:
    struct DataPoint {
        int year;
        int citations;
    };

    explicit CitationTrendChart(QWidget* parent=nullptr) : QWidget(parent) {
        setMinimumHeight(200);
    }

    void setData(const QList<DataPoint>& d) {
        m_data = d;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        QRect r = rect().adjusted(12,12,-12,-12);

        // Background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255,255,255,60));
        p.drawRoundedRect(r, 12, 12);

        if (m_data.isEmpty()) return;

        int leftPad = 50;
        int bottomPad = 35;
        int topPad = 20;
        QRect plot = r.adjusted(leftPad, topPad, -10, -bottomPad);

        // Find max citations
        int maxCite = 0;
        for (auto &d : m_data) maxCite = std::max(maxCite, d.citations);
        if (maxCite <= 0) maxCite = 100;

        // Draw title
        p.setPen(QColor(0,0,0,120));
        QFont t = font();
        t.setBold(true);
        t.setPointSize(10);
        p.setFont(t);
        p.drawText(QRect(r.left()+10, r.top(), r.width()-20, topPad),
                   Qt::AlignLeft|Qt::AlignVCenter, "Citation Trend Analysis");

        // Draw grid lines and Y-axis labels
        QFont f = font();
        f.setPointSize(8);
        f.setBold(true);
        p.setFont(f);

        QList<int> ticks = {maxCite, maxCite*3/4, maxCite/2, maxCite/4, 0};
        for (int tick : ticks) {
            double y = plot.bottom() - (double(tick) / maxCite) * plot.height();
            p.setPen(QColor(0,0,0,90));
            p.drawText(QRect(r.left(), (int)y-8, leftPad-6, 16),
                       Qt::AlignRight|Qt::AlignVCenter, QString::number(tick));
            p.setPen(QColor(0,0,0,25));
            p.drawLine(plot.left(), (int)y, plot.right(), (int)y);
        }

        // Draw line chart
        if (m_data.size() < 2) return;

        int n = m_data.size();
        double xStep = double(plot.width()) / (n - 1);

        // Draw line
        QPainterPath path;
        for (int i = 0; i < n; ++i) {
            double x = plot.left() + i * xStep;
            double y = plot.bottom() - (double(m_data[i].citations) / maxCite) * plot.height();
            if (i == 0)
                path.moveTo(x, y);
            else
                path.lineTo(x, y);
        }

        p.setPen(QPen(W_GREEN, 3));
        p.drawPath(path);

        // Draw points and labels
        p.setPen(Qt::NoPen);
        p.setBrush(W_GREEN);
        for (int i = 0; i < n; ++i) {
            double x = plot.left() + i * xStep;
            double y = plot.bottom() - (double(m_data[i].citations) / maxCite) * plot.height();

            // Point
            p.drawEllipse(QPointF(x, y), 5, 5);

            // Year label
            p.setPen(QColor(0,0,0,140));
            p.drawText(QRectF(x-25, plot.bottom()+5, 50, 25),
                       Qt::AlignCenter, QString::number(m_data[i].year));
            p.setPen(Qt::NoPen);
        }

        // Prediction line (dashed)
        if (n >= 2) {
            double lastX = plot.left() + (n-1) * xStep;
            double lastY = plot.bottom() - (double(m_data[n-1].citations) / maxCite) * plot.height();

            // Simple linear prediction
            double slope = (m_data[n-1].citations - m_data[n-2].citations);
            int predictedCite = m_data[n-1].citations + (int)slope;
            if (predictedCite < 0) predictedCite = 0;

            double predX = plot.right();
            double predY = plot.bottom() - (double(predictedCite) / maxCite) * plot.height();

            QPen dashedPen(W_ORANGE, 2, Qt::DashLine);
            p.setPen(dashedPen);
            p.drawLine(QPointF(lastX, lastY), QPointF(predX, predY));

            // Prediction label
            p.setPen(W_ORANGE);
            QFont pf = font();
            pf.setBold(true);
            pf.setPointSize(8);
            p.setFont(pf);
            p.drawText(QRectF(predX-60, predY-20, 55, 15),
                       Qt::AlignRight, QString("~%1").arg(predictedCite));
        }
    }

private:
    QList<DataPoint> m_data;
};

// ===================== Publications per Year Bar Chart =====================
class PublicationsPerYearChart : public QWidget {
public:
    struct YearData {
        int year;
        int count;
    };

    explicit PublicationsPerYearChart(QWidget* parent=nullptr) : QWidget(parent) {
        setMinimumHeight(200);
    }

    void setData(const QList<YearData>& d) {
        m_data = d;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        QRect r = rect().adjusted(12,12,-12,-12);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255,255,255,60));
        p.drawRoundedRect(r, 12, 12);

        if (m_data.isEmpty()) return;

        int leftPad = 50;
        int bottomPad = 35;
        int topPad = 20;
        QRect plot = r.adjusted(leftPad, topPad, -10, -bottomPad);

        // Title
        p.setPen(QColor(0,0,0,120));
        QFont t = font();
        t.setBold(true);
        t.setPointSize(10);
        p.setFont(t);
        p.drawText(QRect(r.left()+10, r.top(), r.width()-20, topPad),
                   Qt::AlignLeft|Qt::AlignVCenter, "Publications per Year");

        // Find max count
        int maxCount = 0;
        for (auto &d : m_data) maxCount = std::max(maxCount, d.count);
        if (maxCount <= 0) maxCount = 10;

        // Draw Y-axis
        QFont f = font();
        f.setPointSize(8);
        f.setBold(true);
        p.setFont(f);

        QList<int> ticks = {maxCount, maxCount*3/4, maxCount/2, maxCount/4, 0};
        for (int tick : ticks) {
            double y = plot.bottom() - (double(tick) / maxCount) * plot.height();
            p.setPen(QColor(0,0,0,90));
            p.drawText(QRect(r.left(), (int)y-8, leftPad-6, 16),
                       Qt::AlignRight|Qt::AlignVCenter, QString::number(tick));
            p.setPen(QColor(0,0,0,25));
            p.drawLine(plot.left(), (int)y, plot.right(), (int)y);
        }

        // Draw bars
        int n = m_data.size();
        int gap = 10;
        int bw = (plot.width() - gap*(n-1)) / n;

        for (int i = 0; i < n; ++i) {
            int x = plot.left() + i * (bw + gap);
            double barHeight = (double(m_data[i].count) / maxCount) * plot.height();
            int y = plot.bottom() - (int)barHeight;

            // Gradient bar
            QLinearGradient grad(x, y, x, plot.bottom());
            grad.setColorAt(0.0, W_GREEN.lighter(120));
            grad.setColorAt(1.0, W_GREEN);

            p.setPen(Qt::NoPen);
            p.setBrush(grad);
            p.drawRoundedRect(QRect(x, y, bw, (int)barHeight), 6, 6);

            // Year label
            p.setPen(QColor(0,0,0,140));
            p.drawText(QRect(x, plot.bottom()+5, bw, 25),
                       Qt::AlignCenter, QString::number(m_data[i].year));

            // Count label on top of bar
            QFont cf = font();
            cf.setBold(true);
            cf.setPointSize(9);
            p.setFont(cf);
            p.setPen(W_GREEN);
            p.drawText(QRect(x, y-20, bw, 18),
                       Qt::AlignCenter, QString::number(m_data[i].count));
        }
    }

private:
    QList<YearData> m_data;
};

// ===================== Delete Dialog =====================
class DeletePublicationDialog : public QDialog {
public:
    DeletePublicationDialog(QStyle* st, QWidget* parent=nullptr) : QDialog(parent) {
        setWindowTitle("Supprimer des Publications");
        resize(900, 450);
        setModal(true);

        QVBoxLayout* root = new QVBoxLayout(this);
        root->setContentsMargins(14,14,14,14);
        root->setSpacing(12);

        QLabel* title = new QLabel("Liste des Publications");
        QFont tf = title->font();
        tf.setBold(true);
        tf.setPointSize(12);
        title->setFont(tf);
        title->setStyleSheet("color: rgba(0,0,0,0.65);");
        root->addWidget(title);

        table = new QTableWidget(0, 6);
        table->setHorizontalHeaderLabels({"Pub ID","Title","Authors","Journal","Year","Action"});
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

        // Sample data
        addRow("PUB001", "Machine Learning in Biology", "Smith J, Doe A", "Nature", "2024");
        addRow("PUB002", "Quantum Computing Advances", "Johnson M", "Science", "2023");
        addRow("PUB003", "Climate Change Models", "Lee K, Park S", "Cell", "2024");
    }

private:
    QTableWidget* table;

    void addRow(const QString& id, const QString& title, const QString& authors,
                const QString& journal, const QString& year) {
        int r = table->rowCount();
        table->insertRow(r);

        table->setItem(r,0,new QTableWidgetItem(id));
        table->setItem(r,1,new QTableWidgetItem(title));
        table->setItem(r,2,new QTableWidgetItem(authors));
        table->setItem(r,3,new QTableWidgetItem(journal));
        table->setItem(r,4,new QTableWidgetItem(year));

        for (int c=0;c<5;++c)
            table->item(r,c)->setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);

        QPushButton* del = new QPushButton("Supprimer");
        del->setCursor(Qt::PointingHandCursor);
        del->setStyleSheet("QPushButton{ color:#B14A4A; font-weight:900; padding:6px 12px; }");
        connect(del, &QPushButton::clicked, this, [=](){
            auto rep = QMessageBox::question(this, "Confirmation",
                                             "Voulez-vous supprimer cette publication ?",
                                             QMessageBox::Yes | QMessageBox::No);
            if (rep == QMessageBox::Yes) {
                table->removeRow(r);
            }
        });

        table->setCellWidget(r, 5, del);
        table->setRowHeight(r, 42);
    }
};

// ===================== MainWindow =====================
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    resize(1200, 680);

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
        QSpinBox, QDoubleSpinBox {
            background: rgba(255,255,255,0.65);
            border: 1px solid rgba(0,0,0,0.15);
            border-radius: 10px;
            padding: 8px 12px;
            font-weight: 800;
        }
        QDateEdit {
            background: rgba(255,255,255,0.65);
            border: 1px solid rgba(0,0,0,0.15);
            border-radius: 10px;
            padding: 8px 12px;
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

    QVBoxLayout* rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(0,0,0,0);

    QStackedWidget* stack = new QStackedWidget;
    rootLayout->addWidget(stack);

    // ==========================================================
    // PAGE 0 : Publications List Management
    // ==========================================================
    QWidget* page1 = new QWidget;
    QVBoxLayout* p1 = new QVBoxLayout(page1);
    p1->setContentsMargins(22, 18, 22, 18);
    p1->setSpacing(14);

    p1->addWidget(makeTopBar(st, "Gestion des Publications"));

    // Search and Filter Bar
    QFrame* bar1 = new QFrame;
    bar1->setFixedHeight(54);
    bar1->setStyleSheet("background: rgba(255,255,255,0.22); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* bar1L = new QHBoxLayout(bar1);
    bar1L->setContentsMargins(14, 8, 14, 8);
    bar1L->setSpacing(10);

    QLineEdit* search = new QLineEdit;
    search->setPlaceholderText("Rechercher par titre, auteur, mots-clés, DOI…");
    search->addAction(st->standardIcon(QStyle::SP_FileDialogContentsView), QLineEdit::LeadingPosition);

    QComboBox* cbType = new QComboBox;
    cbType->addItems({"Type", "Journal Article", "Conference Paper", "Patent"});

    QComboBox* cbStatus = new QComboBox;
    cbStatus->addItems({"Status", "Submitted", "Under Review", "Accepted", "Published"});

    QComboBox* cbYear = new QComboBox;
    cbYear->addItems({"Year", "2024", "2023", "2022", "2021", "2020"});

    QPushButton* filters = new QPushButton(st->standardIcon(QStyle::SP_FileDialogDetailedView), " Filtres");
    filters->setCursor(Qt::PointingHandCursor);
    filters->setStyleSheet(QString(R"(
        QPushButton{ background:%1; color: rgba(255,255,255,0.92);
                     border:1px solid rgba(0,0,0,0.18); border-radius: 12px;
                     padding: 10px 16px; font-weight: 800; }
        QPushButton:hover{ background: %2; }
    )").arg(C_PRIMARY, C_TOPBAR));

    bar1L->addWidget(search, 1);
    bar1L->addWidget(cbType);
    bar1L->addWidget(cbStatus);
    bar1L->addWidget(cbYear);
    bar1L->addWidget(filters);

    p1->addWidget(bar1);

    // Publications Table
    QFrame* card1 = makeCard();
    QVBoxLayout* card1L = new QVBoxLayout(card1);
    card1L->setContentsMargins(10,10,10,10);

    QTableWidget* table = new QTableWidget(6, 10);
    table->setHorizontalHeaderLabels({"", "Pub ID", "Title", "Authors", "Journal",
                                      "Year", "Impact", "Citations", "Status", "Type"});
    table->verticalHeader()->setVisible(false);
    table->setShowGrid(true);
    table->setAlternatingRowColors(true);
    table->setStyleSheet(QString("QTableWidget{ alternate-background-color:%1; background-color:%2; }")
                             .arg(C_ROW_EVEN, C_ROW_ODD));
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    table->setItemDelegateForColumn(8, new StatusBadgeDelegate(table));

    table->setColumnWidth(0, 36);
    table->setColumnWidth(1, 90);
    table->setColumnWidth(2, 250);
    table->setColumnWidth(3, 180);
    table->setColumnWidth(4, 150);
    table->setColumnWidth(5, 70);
    table->setColumnWidth(6, 80);
    table->setColumnWidth(7, 90);
    table->setColumnWidth(8, 140);
    table->setColumnWidth(9, 130);

    auto setRow=[&](int r, const QString& id, const QString& title, const QString& authors,
                      const QString& journal, const QString& year, const QString& impact,
                      const QString& citations, PublicationStatus status, const QString& type) {
        QTableWidgetItem* iconItem = new QTableWidgetItem;
        iconItem->setIcon(st->standardIcon(QStyle::SP_FileIcon));
        iconItem->setTextAlignment(Qt::AlignCenter);
        table->setItem(r, 0, iconItem);

        auto mk = [&](const QString& t){
            QTableWidgetItem* it = new QTableWidgetItem(t);
            it->setTextAlignment(Qt::AlignLeft|Qt::AlignVCenter);
            return it;
        };

        table->setItem(r, 1, mk(id));
        table->setItem(r, 2, mk(title));
        table->setItem(r, 3, mk(authors));
        table->setItem(r, 4, mk(journal));

        QTableWidgetItem* y = mk(year);
        y->setTextAlignment(Qt::AlignCenter);
        table->setItem(r, 5, y);

        QTableWidgetItem* imp = mk(impact);
        imp->setTextAlignment(Qt::AlignCenter);
        table->setItem(r, 6, imp);

        QTableWidgetItem* cit = mk(citations);
        cit->setTextAlignment(Qt::AlignCenter);
        table->setItem(r, 7, cit);

        QTableWidgetItem* badge = new QTableWidgetItem;
        badge->setData(Qt::UserRole, (int)status);
        table->setItem(r, 8, badge);

        table->setItem(r, 9, mk(type));

        table->setRowHeight(r, 48);
    };

    setRow(0, "PUB001", "Machine Learning Applications in Genomics", "Smith J, Doe A",
           "Nature Genetics", "2024", "45.2", "156", PublicationStatus::Published, "Journal Article");
    setRow(1, "PUB002", "Quantum Computing for Drug Discovery", "Johnson M, Lee K",
           "Science", "2024", "56.9", "203", PublicationStatus::Accepted, "Journal Article");
    setRow(2, "PUB003", "Climate Change Impact on Biodiversity", "Park S, Kim H",
           "Cell", "2023", "66.8", "412", PublicationStatus::Published, "Journal Article");
    setRow(3, "PUB004", "Neural Network Optimization Techniques", "Brown R",
           "ICML 2024", "2024", "12.3", "45", PublicationStatus::UnderReview, "Conference Paper");
    setRow(4, "PUB005", "Novel Cancer Treatment Method", "Davis L, Miller T",
           "The Lancet", "2023", "202.7", "1024", PublicationStatus::Published, "Journal Article");
    setRow(5, "PUB006", "Renewable Energy Storage System", "Wilson C",
           "Patent Office", "2024", "N/A", "12", PublicationStatus::Submitted, "Patent");

    card1L->addWidget(table);
    p1->addWidget(card1, 1);

    // Bottom Action Bar
    QFrame* bottom1 = new QFrame;
    bottom1->setFixedHeight(64);
    bottom1->setStyleSheet("background: rgba(255,255,255,0.20); border: 1px solid rgba(0,0,0,0.08); border-radius: 14px;");
    QHBoxLayout* bottom1L = new QHBoxLayout(bottom1);
    bottom1L->setContentsMargins(14,10,14,10);
    bottom1L->setSpacing(12);

    QPushButton* btnAdd = actionBtn("Ajouter", "rgba(10,95,88,0.45)", "rgba(255,255,255,0.90)",
                                    st->standardIcon(QStyle::SP_FileIcon), true);
    QPushButton* btnEdit = actionBtn("Modifier", "rgba(198,178,154,0.55)", "rgba(255,255,255,0.85)",
                                     st->standardIcon(QStyle::SP_FileDialogContentsView), true);
    QPushButton* btnDel = actionBtn("Supprimer", "rgba(255,255,255,0.55)", "#B14A4A",
                                    st->standardIcon(QStyle::SP_TrashIcon), true);
    QPushButton* btnDetails = actionBtn("Détails", "rgba(255,255,255,0.55)", C_TEXT_DARK,
                                        st->standardIcon(QStyle::SP_FileDialogDetailedView), true);

    connect(btnDel, &QPushButton::clicked, this, [=](){
        DeletePublicationDialog dlg(style(), this);
        dlg.exec();
    });

    bottom1L->addWidget(btnAdd);
    bottom1L->addWidget(btnEdit);
    bottom1L->addWidget(btnDel);
    bottom1L->addWidget(btnDetails);
    bottom1L->addStretch(1);

    bottom1L->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_FileDialogDetailedView)));
    bottom1L->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_DialogSaveButton)));
    bottom1L->addWidget(tinySquareBtn(st->standardIcon(QStyle::SP_BrowserReload)));

    QPushButton* btnStats = new QPushButton(st->standardIcon(QStyle::SP_FileDialogInfoView), " Statistiques");
    btnStats->setCursor(Qt::PointingHandCursor);
    btnStats->setStyleSheet(R"(
        QPushButton{ background: rgba(255,255,255,0.55); border: 1px solid rgba(0,0,0,0.12);
                     border-radius: 12px; padding: 10px 14px; color: rgba(0,0,0,0.65); font-weight: 800; }
        QPushButton:hover{ background: rgba(255,255,255,0.75); }
    )");
    bottom1L->addWidget(btnStats);

    p1->addWidget(bottom1);

    stack->addWidget(page1);

    // ==========================================================
    // PAGE 1 : Add/Edit Publication
    // ==========================================================
    QWidget* page2 = new QWidget;
    QVBoxLayout* p2 = new QVBoxLayout(page2);
    p2->setContentsMargins(22, 18, 22, 18);
    p2->setSpacing(14);

    p2->addWidget(makeTopBar(st, "Ajouter / Modifier Publication"));

    QFrame* outer2 = new QFrame;
    outer2->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }")
                              .arg(C_PANEL_BG, C_PANEL_BR));
    QHBoxLayout* outer2L = new QHBoxLayout(outer2);
    outer2L->setContentsMargins(12,12,12,12);
    outer2L->setSpacing(12);

    // LEFT PANEL - Publication Details
    QFrame* left2 = softBox();
    left2->setFixedWidth(480);
    QVBoxLayout* left2L = new QVBoxLayout(left2);
    left2L->setContentsMargins(12,12,12,12);
    left2L->setSpacing(12);

    auto sectionTitle = [&](const QString& t){
        QLabel* lab = new QLabel(t);
        lab->setStyleSheet("color: rgba(0,0,0,0.55); font-weight: 900; font-size: 11pt;");
        return lab;
    };

    auto formRow = [&](const QString& label, QWidget* input){
        QFrame* r = new QFrame;
        QVBoxLayout* l = new QVBoxLayout(r);
        l->setContentsMargins(8,6,8,6);
        l->setSpacing(6);
        QLabel* lab = new QLabel(label);
        lab->setStyleSheet("color: rgba(0,0,0,0.60); font-weight: 800; font-size: 9pt;");
        l->addWidget(lab);
        l->addWidget(input);
        return r;
    };

    left2L->addWidget(sectionTitle("📄 Informations Principales"));

    QLineEdit* leTitle = new QLineEdit;
    leTitle->setPlaceholderText("Titre de la publication");
    left2L->addWidget(formRow("Titre", leTitle));

    QLineEdit* leAuthors = new QLineEdit;
    leAuthors->setPlaceholderText("Ex: Smith J, Doe A, Johnson M");
    left2L->addWidget(formRow("Auteurs", leAuthors));

    QLineEdit* leJournal = new QLineEdit;
    leJournal->setPlaceholderText("Nom du journal");
    left2L->addWidget(formRow("Journal", leJournal));

    QLineEdit* leDOI = new QLineEdit;
    leDOI->setPlaceholderText("10.1234/example.doi");
    left2L->addWidget(formRow("DOI", leDOI));

    left2L->addWidget(sectionTitle("📊 Métriques"));

    QHBoxLayout* metricsRow = new QHBoxLayout;
    QDoubleSpinBox* sbImpact = new QDoubleSpinBox;
    sbImpact->setRange(0, 999.9);
    sbImpact->setDecimals(1);
    sbImpact->setValue(0.0);

    QSpinBox* sbCitations = new QSpinBox;
    sbCitations->setRange(0, 999999);
    sbCitations->setValue(0);

    metricsRow->addWidget(formRow("Impact Factor", sbImpact));
    metricsRow->addWidget(formRow("Citations", sbCitations));
    left2L->addLayout(metricsRow);

    left2L->addStretch(1);

    // RIGHT PANEL - Additional Info
    QFrame* right2 = softBox();
    QVBoxLayout* right2L = new QVBoxLayout(right2);
    right2L->setContentsMargins(12,12,12,12);
    right2L->setSpacing(12);

    right2L->addWidget(sectionTitle("🏷️ Classification"));

    QComboBox* cbType2 = new QComboBox;
    cbType2->addItems({"Type", "Journal Article", "Conference Paper", "Patent"});
    right2L->addWidget(formRow("Type de Publication", cbType2));

    QComboBox* cbStatus2 = new QComboBox;
    cbStatus2->addItems({"Status", "Submitted", "Under Review", "Accepted", "Published"});
    right2L->addWidget(formRow("Statut", cbStatus2));

    QDateEdit* dePublicationDate = new QDateEdit(QDate::currentDate());
    dePublicationDate->setCalendarPopup(true);
    dePublicationDate->setDisplayFormat("dd/MM/yyyy");
    right2L->addWidget(formRow("Date de Publication", dePublicationDate));

    right2L->addWidget(sectionTitle("🔍 Mots-clés"));

    QLineEdit* leKeywords = new QLineEdit;
    leKeywords->setPlaceholderText("machine learning, genomics, AI");
    right2L->addWidget(formRow("Mots-clés (séparés par virgules)", leKeywords));

    right2L->addWidget(sectionTitle("📝 Résumé"));

    QTextEdit* teAbstract = new QTextEdit;
    teAbstract->setPlaceholderText("Résumé de la publication...");
    teAbstract->setMaximumHeight(180);
    right2L->addWidget(teAbstract);

    right2L->addStretch(1);

    outer2L->addWidget(left2);
    outer2L->addWidget(right2, 1);

    p2->addWidget(outer2, 1);

    // Bottom buttons
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
    // PAGE 2 : Statistics & Analytics
    // ==========================================================
    QWidget* page3 = new QWidget;
    QVBoxLayout* p3 = new QVBoxLayout(page3);
    p3->setContentsMargins(22, 18, 22, 18);
    p3->setSpacing(14);

    p3->addWidget(makeTopBar(st, "Statistiques & Analyses"));

    QFrame* outer3 = new QFrame;
    outer3->setStyleSheet(QString("QFrame{ background:%1; border:1px solid %2; border-radius: 14px; }")
                              .arg(C_PANEL_BG, C_PANEL_BR));
    QVBoxLayout* outer3L = new QVBoxLayout(outer3);
    outer3L->setContentsMargins(12,12,12,12);
    outer3L->setSpacing(12);

    // Top metrics cards
    QWidget* metricsCards = new QWidget;
    QHBoxLayout* mcL = new QHBoxLayout(metricsCards);
    mcL->setContentsMargins(0,0,0,0);
    mcL->setSpacing(12);

    auto metricCard = [&](const QString& title, const QString& value, const QColor& color){
        QFrame* card = softBox();
        card->setStyleSheet(QString("QFrame{ background: %1; border: 2px solid %2; border-radius: 12px; }")
                                .arg("rgba(255,255,255,0.80)", color.name()));
        QVBoxLayout* l = new QVBoxLayout(card);
        l->setContentsMargins(16,12,16,12);

        QLabel* val = new QLabel(value);
        QFont vf = val->font();
        vf.setBold(true);
        vf.setPointSize(24);
        val->setFont(vf);
        val->setStyleSheet(QString("color: %1;").arg(color.name()));

        QLabel* tit = new QLabel(title);
        tit->setStyleSheet("color: rgba(0,0,0,0.60); font-weight: 800;");

        l->addWidget(val);
        l->addWidget(tit);
        return card;
    };

    mcL->addWidget(metricCard("Total Publications", "142", W_GREEN));
    mcL->addWidget(metricCard("Citations Totales", "3,547", W_BLUE));
    mcL->addWidget(metricCard("H-Index", "28", W_ORANGE));
    mcL->addWidget(metricCard("Under Review", "7", W_RED));

    outer3L->addWidget(metricsCards);

    // Charts section
    QWidget* chartsWidget = new QWidget;
    QHBoxLayout* chartsL = new QHBoxLayout(chartsWidget);
    chartsL->setContentsMargins(0,0,0,0);
    chartsL->setSpacing(12);

    // Publications per year
    QFrame* pubYearCard = softBox();
    QVBoxLayout* pycL = new QVBoxLayout(pubYearCard);
    pycL->setContentsMargins(12,12,12,12);

    PublicationsPerYearChart* pubChart = new PublicationsPerYearChart;
    pubChart->setData({
        {2020, 18},
        {2021, 24},
        {2022, 31},
        {2023, 38},
        {2024, 31}
    });
    pycL->addWidget(pubChart);

    // Citation trend
    QFrame* citeTrendCard = softBox();
    QVBoxLayout* ctcL = new QVBoxLayout(citeTrendCard);
    ctcL->setContentsMargins(12,12,12,12);

    CitationTrendChart* citeChart = new CitationTrendChart;
    citeChart->setData({
        {2020, 245},
        {2021, 412},
        {2022, 658},
        {2023, 891},
        {2024, 1341}
    });
    ctcL->addWidget(citeChart);

    chartsL->addWidget(pubYearCard);
    chartsL->addWidget(citeTrendCard);

    outer3L->addWidget(chartsWidget, 1);

    // Export options
    QFrame* exportBar = new QFrame;
    exportBar->setStyleSheet("QFrame{ background: rgba(255,255,255,0.35); border:1px solid rgba(0,0,0,0.10); border-radius: 12px; }");
    QHBoxLayout* exportL = new QHBoxLayout(exportBar);
    exportL->setContentsMargins(12,10,12,10);
    exportL->setSpacing(12);

    QPushButton* exportPDF = actionBtn("Exporter CV PDF", "rgba(10,95,88,0.45)", "rgba(255,255,255,0.92)",
                                       st->standardIcon(QStyle::SP_FileIcon), true);
    QPushButton* exportCSV = actionBtn("Exporter CSV", "rgba(255,255,255,0.72)", C_TEXT_DARK,
                                       st->standardIcon(QStyle::SP_DialogSaveButton), true);
    QPushButton* viewTrends = actionBtn("Analyse Détaillée", "rgba(255,255,255,0.72)", C_TEXT_DARK,
                                        st->standardIcon(QStyle::SP_FileDialogDetailedView), true);

    exportL->addWidget(exportPDF);
    exportL->addWidget(exportCSV);
    exportL->addStretch(1);
    exportL->addWidget(viewTrends);

    outer3L->addWidget(exportBar);

    p3->addWidget(outer3, 1);

    // Bottom navigation
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
    // NAVIGATION CONNECTIONS
    // ==========================================================
    connect(btnAdd, &QPushButton::clicked, this, [=](){
        setWindowTitle("Ajouter / Modifier Publication");
        stack->setCurrentIndex(1);
    });

    connect(btnEdit, &QPushButton::clicked, this, [=](){
        setWindowTitle("Ajouter / Modifier Publication");
        stack->setCurrentIndex(1);
    });

    connect(cancelBtn, &QPushButton::clicked, this, [=](){
        setWindowTitle("Gestion des Publications");
        stack->setCurrentIndex(0);
    });

    connect(saveBtn, &QPushButton::clicked, this, [=](){
        setWindowTitle("Gestion des Publications");
        stack->setCurrentIndex(0);
    });

    connect(btnStats, &QPushButton::clicked, this, [=](){
        setWindowTitle("Statistiques & Analyses");
        stack->setCurrentIndex(2);
    });

    connect(back3, &QPushButton::clicked, this, [=](){
        setWindowTitle("Gestion des Publications");
        stack->setCurrentIndex(0);
    });

    // Start
    setWindowTitle("Gestion des Publications");
    stack->setCurrentIndex(0);
}
