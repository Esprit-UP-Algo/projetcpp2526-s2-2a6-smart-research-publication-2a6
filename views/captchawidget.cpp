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
    setFixedSize(360, 210);

    // ── Input field ──
    m_input = new QLineEdit(this);
    m_input->setPlaceholderText("Tapez les 5 caractères");
    m_input->setMaxLength(5);
    m_input->setAlignment(Qt::AlignCenter);
    m_input->setFixedHeight(40);
    m_input->setStyleSheet(R"(
        QLineEdit {
            background: rgba(4,16,22,0.92);
            border: 1.5px solid rgba(0,240,200,0.34);
            border-radius: 10px;
            padding: 6px 12px;
            font-size: 14px;
            font-weight: 700;
            color: #EAFBFF;
            letter-spacing: 4px;
            selection-background-color: rgba(0,240,200,0.35);
        }
        QLineEdit:focus {
            border: 2px solid #00F0C8;
            background: rgba(0,34,36,0.95);
        }
    )");

    // ── Refresh button ──
    m_refresh = new QPushButton("↻  Nouveau code", this);
    m_refresh->setObjectName("captchaRefresh");
    m_refresh->setCursor(Qt::PointingHandCursor);
    m_refresh->setFixedHeight(24);
    m_refresh->setStyleSheet(R"(
        QPushButton {
            background: transparent;
            border: none;
            padding: 0 8px;
            font-size: 12px;
            font-weight: 700;
            color: rgba(0,240,200,0.84);
        }
        QPushButton:hover {
            color: #00F0C8;
            text-decoration: underline;
        }
        QPushButton:pressed {
            color: rgba(0,240,200,0.62);
        }
    )");

    // ── Verification message ──
    m_hint = new QLabel("⚠️ Vérification requise — tapez les 5 caractères affichés.", this);
    m_hint->setWordWrap(true);
    m_hint->setAlignment(Qt::AlignCenter);
    m_hint->setFixedHeight(32);
    m_hint->setStyleSheet(R"(
        QLabel {
            color: rgba(234,251,255,0.82);
            background: transparent;
            font-size: 11.5px;
            font-weight: 600;
        }
    )");

    // ── Layout ──
    QVBoxLayout* vlay = new QVBoxLayout(this);
    vlay->setContentsMargins(0, 0, 0, 0);
    vlay->setSpacing(0);

    vlay->addSpacing(80); // 68px canvas + 12px bottom margin
    vlay->addWidget(m_input);
    vlay->addSpacing(8);
    vlay->addWidget(m_hint);
    vlay->addSpacing(8);
    vlay->addWidget(m_refresh, 0, Qt::AlignCenter);
    vlay->addSpacing(18);

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

    // Canvas rect: full width, top area
    QRect canvas(0, 0, width(), 68);

    drawBackground(p, canvas);
    drawNoise(p, canvas);
    drawLines(p, canvas);
    drawCharacters(p, canvas);

    // Border around canvas
    p.setPen(QPen(QColor(0, 240, 200, 110), 1.5));
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
    grad.setColorAt(0.0, QColor(8, 32, 40, 238));
    grad.setColorAt(0.5, QColor(16, 60, 68, 232));
    grad.setColorAt(1.0, QColor(4, 18, 26, 238));

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
            QRandomGenerator::global()->bounded(40, 130));
        p.setBrush(QColor(0,
                          qMin(255, brightness + 105),
                          qMin(255, brightness + 90),
                          120));
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

        // Bright cyan/teal tones for the dark glassmorphism canvas
        int rv = static_cast<int>(QRandomGenerator::global()->bounded(120,  230));
        int gv = static_cast<int>(QRandomGenerator::global()->bounded(210, 255));
        int bv = static_cast<int>(QRandomGenerator::global()->bounded(220, 255));

        QFont font("Courier New");
        font.setPointSize(fontSize);
        font.setBold(true);
        font.setItalic(QRandomGenerator::global()->bounded(2) == 0);

        p.save();
        p.translate(cx, cy);
        p.rotate(angleDeg);

        // Subtle shadow
        p.setPen(QColor(0, 240, 200, 90));
        p.setFont(font);
        p.drawText(QPointF(1.5, 1.5), QString(m_code.at(i)));

        // Main character
        p.setPen(QColor(rv, gv, bv));
        p.drawText(QPointF(0, 0), QString(m_code.at(i)));

        p.restore();
    }
}
