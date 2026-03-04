#include "captchawidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QRandomGenerator>
#include <QFont>
#include <QLinearGradient>
#include <QtMath>

// Characters used: no I, O, 0, 1 to avoid confusion
static const QString CAPTCHA_CHARS = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";

// ─────────────────────────────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────────────────────────────
CaptchaWidget::CaptchaWidget(QWidget* parent)
    : QWidget(parent)
{
    // ── Canvas: fixed size drawn by paintEvent ──
    setFixedSize(260, 130);

    // ── Input field ──
    m_input = new QLineEdit(this);
    m_input->setPlaceholderText("Tapez les caractères");
    m_input->setMaxLength(5);
    m_input->setAlignment(Qt::AlignCenter);
    m_input->setFixedHeight(38);
    m_input->setStyleSheet(R"(
        QLineEdit {
            background: rgba(255,255,255,0.92);
            border: 1.5px solid rgba(18,68,59,0.30);
            border-radius: 10px;
            padding: 6px 12px;
            font-size: 14px;
            font-weight: 700;
            color: #12443B;
            letter-spacing: 4px;
        }
        QLineEdit:focus {
            border: 2px solid #0A5F58;
        }
    )");

    // ── Refresh button ──
    m_refresh = new QPushButton("↻  Nouveau code", this);
    m_refresh->setCursor(Qt::PointingHandCursor);
    m_refresh->setFixedHeight(34);
    m_refresh->setStyleSheet(R"(
        QPushButton {
            background: rgba(10,95,88,0.12);
            border: 1px solid rgba(10,95,88,0.25);
            border-radius: 8px;
            padding: 4px 12px;
            font-size: 12px;
            font-weight: 700;
            color: #0A5F58;
        }
        QPushButton:hover {
            background: rgba(10,95,88,0.22);
        }
        QPushButton:pressed {
            background: rgba(10,95,88,0.35);
        }
    )");

    // ── Layout ──
    QVBoxLayout* vlay = new QVBoxLayout(this);
    vlay->setContentsMargins(0, 0, 0, 0);
    vlay->setSpacing(6);

    // Canvas placeholder label (actual drawing done in paintEvent)
    QLabel* canvasLabel = new QLabel(this);
    canvasLabel->setFixedHeight(62);
    canvasLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    canvasLabel->setStyleSheet("background: transparent;");
    canvasLabel->setVisible(false); // paintEvent covers full widget area

    vlay->addSpacing(68); // reserve space for the canvas drawn by paintEvent
    vlay->addWidget(m_input);
    vlay->addWidget(m_refresh);

    // ── Wire refresh button ──
    connect(m_refresh, &QPushButton::clicked, this, &CaptchaWidget::refresh);

    // ── Generate first code ──
    generateCode();
}

// ─────────────────────────────────────────────────────────────────────
//  Public API
// ─────────────────────────────────────────────────────────────────────
bool CaptchaWidget::validate() const
{
    return m_input->text().trimmed().toUpper() == m_code;
}

void CaptchaWidget::refresh()
{
    generateCode();
    m_input->clear();
    m_input->setFocus();
}

void CaptchaWidget::clearInput()
{
    m_input->clear();
}

// ─────────────────────────────────────────────────────────────────────
//  Code generation
// ─────────────────────────────────────────────────────────────────────
void CaptchaWidget::generateCode()
{
    m_code.clear();
    for (int i = 0; i < 5; ++i) {
        int idx = static_cast<int>(
            QRandomGenerator::global()->bounded(CAPTCHA_CHARS.size()));
        m_code += CAPTCHA_CHARS.at(idx);
    }
    update(); // trigger repaint
}

// ─────────────────────────────────────────────────────────────────────
//  paintEvent — draws the CAPTCHA canvas in the top 64px of the widget
// ───────────────────��─────────────────────────────────────────────────
void CaptchaWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);

    // Canvas rect: full width, top 64 px
    QRect canvas(0, 0, width(), 64);

    drawBackground(p, canvas);
    drawNoise(p, canvas);
    drawLines(p, canvas);
    drawCharacters(p, canvas);

    // Border around canvas
    p.setPen(QPen(QColor(10, 95, 88, 80), 1.5));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(canvas.adjusted(1, 1, -1, -1), 10, 10);
}

// ─────────────────────────────────────────────────────────────────────
//  Drawing helpers
// ─────────────────────────────────────────────────────────────────────
void CaptchaWidget::drawBackground(QPainter& p, const QRect& r)
{
    // Soft gradient background matching app theme
    QLinearGradient grad(r.topLeft(), r.bottomRight());
    grad.setColorAt(0.0, QColor("#d8edf0"));
    grad.setColorAt(0.5, QColor("#c8e2e8"));
    grad.setColorAt(1.0, QColor("#b8d8de"));

    p.setPen(Qt::NoPen);
    p.setBrush(grad);
    p.drawRoundedRect(r, 10, 10);
}

void CaptchaWidget::drawNoise(QPainter& p, const QRect& r)
{
    // 90 random dots of varying size and color
    p.setPen(Qt::NoPen);
    for (int i = 0; i < 90; ++i) {
        int x = static_cast<int>(QRandomGenerator::global()->bounded(r.width()));
        int y = static_cast<int>(QRandomGenerator::global()->bounded(r.height()));
        int sz = static_cast<int>(QRandomGenerator::global()->bounded(1, 3));

        int brightness = static_cast<int>(
            QRandomGenerator::global()->bounded(80, 190));
        p.setBrush(QColor(brightness,
                          brightness + 10,
                          brightness + 20,
                          180));
        p.drawEllipse(r.left() + x, r.top() + y, sz, sz);
    }
}

void CaptchaWidget::drawLines(QPainter& p, const QRect& r)
{
    // 6 bezier curves as interference lines
    for (int i = 0; i < 6; ++i) {
        auto rx = [&](){ return r.left() + static_cast<int>(
                                     QRandomGenerator::global()->bounded(r.width())); };
        auto ry = [&](){ return r.top() + static_cast<int>(
                                     QRandomGenerator::global()->bounded(r.height())); };

        QPainterPath path;
        path.moveTo(rx(), ry());
        path.cubicTo(rx(), ry(), rx(), ry(), rx(), ry());

        int c = static_cast<int>(QRandomGenerator::global()->bounded(60, 140));
        p.setPen(QPen(QColor(c, c + 15, c + 25, 140),
                      QRandomGenerator::global()->bounded(1, 2) == 1 ? 1.0 : 1.5));
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);
    }
}

void CaptchaWidget::drawCharacters(QPainter& p, const QRect& r)
{
    // Each character is drawn at a slight random rotation, size, and position
    const int charCount = m_code.size();
    const double slotW  = static_cast<double>(r.width()) / (charCount + 1);

    for (int i = 0; i < charCount; ++i)
    {
        double cx = r.left() + slotW * (i + 0.7)
        + QRandomGenerator::global()->bounded(-4, 5);
        double cy = r.top() + r.height() / 2.0
                    + QRandomGenerator::global()->bounded(-7, 8);

        double angleDeg = QRandomGenerator::global()->bounded(-28, 29);
        int    fontSize = static_cast<int>(
            QRandomGenerator::global()->bounded(20, 28));

        // Pick a dark-ish color so it's readable on light background
        int rv = static_cast<int>(QRandomGenerator::global()->bounded(10,  80));
        int gv = static_cast<int>(QRandomGenerator::global()->bounded(40, 100));
        int bv = static_cast<int>(QRandomGenerator::global()->bounded(60, 130));

        QFont font("Courier New");
        font.setPointSize(fontSize);
        font.setBold(true);
        font.setItalic(QRandomGenerator::global()->bounded(2) == 0);

        p.save();
        p.translate(cx, cy);
        p.rotate(angleDeg);

        // Subtle shadow
        p.setPen(QColor(255, 255, 255, 100));
        p.setFont(font);
        p.drawText(QPointF(1.5, 1.5), QString(m_code.at(i)));

        // Main character
        p.setPen(QColor(rv, gv, bv));
        p.drawText(QPointF(0, 0), QString(m_code.at(i)));

        p.restore();
    }
}
