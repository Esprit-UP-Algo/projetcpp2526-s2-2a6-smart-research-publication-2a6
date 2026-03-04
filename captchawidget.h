#ifndef CAPTCHAWIDGET_H
#define CAPTCHAWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QLabel>

// =====================================================================
//  CaptchaWidget
//  A self-contained CAPTCHA widget drawn with QPainter.
//  Usage:
//    CaptchaWidget* captcha = new CaptchaWidget(parent);
//    if (!captcha->validate()) { /* wrong code */ }
// =====================================================================
class CaptchaWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CaptchaWidget(QWidget* parent = nullptr);

    // Returns true if the user's input matches the generated code
    bool validate() const;

    // Regenerates a new CAPTCHA code and clears the input
    void refresh();

    // Clears only the input field
    void clearInput();

    // Returns the input field (for tab-order wiring)
    QLineEdit* inputField() const { return m_input; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QString    m_code;       // current CAPTCHA code (5 chars)
    QLineEdit* m_input;      // user types here
    QPushButton* m_refresh;  // ↻ button

    void generateCode();
    void drawBackground(QPainter& p, const QRect& r);
    void drawNoise(QPainter& p, const QRect& r);
    void drawLines(QPainter& p, const QRect& r);
    void drawCharacters(QPainter& p, const QRect& r);
};

#endif // CAPTCHAWIDGET_H
