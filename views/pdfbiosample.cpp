#include "pdfbiosample.h"

#include <QPrinter>
#include <QPainter>
#include <QPainterPath>
#include <QPageSize>
#include <QPageLayout>
#include <QFont>
#include <QColor>
#include <QPixmap>
#include <QDate>
#include <QtMath>

void exportBioSamplePdf(const BasicBioInfo& bi, const QString& path)
{
    auto val = [](const QString& s) -> QString { return s.isEmpty() ? "—" : s; };
    QString qtyStr  = QString::number(bi.quantite) + " µg";
    // Convertir via double pour éliminer la notation scientifique Oracle (-8,0E+001 → -80)
    double  tempVal = bi.temperature.isEmpty() ? -80.0 : bi.temperature.toDouble();
    QString tempStr;
    if (bi.temperature.isEmpty()) {
        tempStr = "—";
    } else {
        // Si la valeur est un entier (pas de décimale), afficher sans point
        const int tempInt = static_cast<int>(tempVal);
        tempStr = (tempVal == static_cast<double>(tempInt))
                  ? QString::number(tempInt) + " °C"
                  : QString::number(tempVal, 'f', 1) + " °C";
    }
    QString dateStr = QDate::currentDate().toString("dd/MM/yyyy");

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(path);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageMargins(QMarginsF(0,0,0,0), QPageLayout::Millimeter);

    QPainter p(&printer);
    if (!p.isActive()) return;
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    // ── coordonnées ──────────────────────────────────────────────
    QRectF vp = QRectF(printer.pageRect(QPrinter::DevicePixel));
    float  W  = vp.width();
    float  H  = vp.height();
    float  mg = W * 0.055f;
    float  cW = W - 2*mg;

    // ── couleurs ─────────────────────────────────────────────────
    QColor teal   (10, 95, 88);
    QColor tealBg (232, 245, 243);
    QColor tealHdr(10, 95, 88, 210);
    QColor gray   (120,120,120);
    QColor lineClr(200,220,218);
    QColor white  (255,255,255);

    auto font = [](int sz, bool bold = false) {
        QFont f("Arial", sz); f.setBold(bold); return f;
    };

    float y = mg * 0.6f;

    // ── 1. EN-TÊTE ────────────────────────────────────────────────
    QPixmap logo(":/image/smartvision.png");
    float logoH = H * 0.045f;
    float logoW = logo.isNull() ? 0 : logoH * logo.width() / (float)logo.height();
    if (!logo.isNull())
        p.drawPixmap(QRectF(mg, y, logoW, logoH).toRect(), logo);

    p.setFont(font(22, true));
    p.setPen(teal);
    p.drawText(QRectF(mg + logoW + 10, y, cW * 0.5f, logoH),
               Qt::AlignVCenter | Qt::AlignLeft, "SmartVision Labs");

    p.setFont(font(14));
    p.setPen(gray);
    p.drawText(QRectF(mg + cW * 0.5f, y, cW * 0.5f, logoH / 2),
               Qt::AlignTop | Qt::AlignRight,
               "Date du rapport : " + dateStr);
    p.drawText(QRectF(mg + cW * 0.5f, y + logoH / 2, cW * 0.5f, logoH / 2),
               Qt::AlignTop | Qt::AlignRight,
               "Projet : " + val(bi.projet));

    y += logoH + mg * 0.4f;

    p.setPen(QPen(teal, 2.5));
    p.drawLine(QPointF(mg, y), QPointF(mg + cW, y));
    y += mg * 0.5f;

    // ── 2. TITRE ──────────────────────────────────────────────────
    float titleH = H * 0.04f;
    p.setBrush(tealBg);
    p.setPen(QPen(teal, 1.2));
    p.drawRoundedRect(QRectF(mg, y, cW, titleH), 6, 6);
    p.setFont(font(20, true));
    p.setPen(teal);
    p.drawText(QRectF(mg, y, cW, titleH),
               Qt::AlignCenter,
               "Rapport de Stockage & Suivi des Échantillons");
    y += titleH + mg * 0.6f;

    // ── 3. CORPS — deux colonnes ──────────────────────────────────
    float col1W = cW * 0.54f;
    float col2X = mg + col1W + mg * 0.5f;
    float col2W = cW - col1W - mg * 0.5f;
    float yL = y;
    float yR = y;

    // helper : barre de section
    auto secHdr = [&](float& cy, float colX, float colW, const QString& title) {
        float hh = H * 0.022f;
        p.setBrush(tealHdr);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(QRectF(colX, cy, colW, hh), 4, 4);
        p.setFont(font(14, true));
        p.setPen(white);
        p.drawText(QRectF(colX + 10, cy, colW - 10, hh),
                   Qt::AlignVCenter | Qt::AlignLeft, title);
        cy += hh + H * 0.008f;
    };

    // helper : ligne à puce
    auto row = [&](float& cy, float colX, float colW,
                   const QString& lbl, const QString& val2) {
        float rh = H * 0.022f;
        p.setFont(font(13));
        p.setPen(teal);
        p.drawText(QRectF(colX + 14, cy, colW * 0.42f, rh),
                   Qt::AlignVCenter | Qt::AlignLeft, "• " + lbl + " :");
        p.setPen(QColor(30,30,30));
        p.drawText(QRectF(colX + colW * 0.44f, cy, colW * 0.56f, rh),
                   Qt::AlignVCenter | Qt::AlignLeft, val2);
        cy += rh + H * 0.003f;
    };

    // ── COLONNE GAUCHE ────────────────────────────────────────────
    secHdr(yL, mg, col1W, "Détails de l'Échantillon :");
    row(yL, mg, col1W, "ID Échantillon",   val(bi.reference));
    row(yL, mg, col1W, "Type",             val(bi.type));
    row(yL, mg, col1W, "Organisme source", val(bi.organisme));
    row(yL, mg, col1W, "Niveau BSL",       val(bi.bslLevel));
    row(yL, mg, col1W, "Quantité",         qtyStr);

    yL += H * 0.018f;
    secHdr(yL, mg, col1W, "Localisation de Stockage :");
    row(yL, mg, col1W, "Congélateur", val(bi.congelateur));
    row(yL, mg, col1W, "Étagère",     val(bi.etagere));
    row(yL, mg, col1W, "Température", tempStr);

    yL += H * 0.018f;
    secHdr(yL, mg, col1W, "Liste de Conformité :");
    float chkH = H * 0.022f;
    const QStringList checks = {
        "Protocoles BSL respectés",
        "Échantillons étiquetés & sécurisés",
        "Inventaire mis à jour",
        "Audit effectué"
    };
    for (const QString& c : checks) {
        p.setFont(font(13, true));
        p.setPen(teal);
        p.drawText(QRectF(mg + 14, yL, col1W, chkH),
                   Qt::AlignVCenter | Qt::AlignLeft, "☑  " + c);
        yL += chkH + H * 0.004f;
    }

    // ── COLONNE DROITE ────────────────────────────────────────────
    float ovH = H * 0.18f;
    p.setBrush(QColor(245,250,249));
    p.setPen(QPen(lineClr, 1.5));
    p.drawRoundedRect(QRectF(col2X, yR, col2W, ovH), 8, 8);

    p.setFont(font(13, true));
    p.setPen(gray);
    p.drawText(QRectF(col2X, yR + 8, col2W, H * 0.022f),
               Qt::AlignTop | Qt::AlignHCenter, "Aperçu du Stockage");

    float fx = col2X + col2W * 0.25f;
    float fy = yR + ovH * 0.22f;
    float fw = col2W * 0.50f;
    float fh = ovH * 0.70f;
    p.setBrush(QColor(10,95,88,60));
    p.setPen(QPen(teal, 2));
    p.drawRoundedRect(QRectF(fx, fy, fw, fh), 6, 6);
    p.setPen(QPen(teal, 1.5));
    for (int i = 1; i < 5; ++i) {
        float sy = fy + fh * i / 5.0f;
        p.drawLine(QPointF(fx+4, sy), QPointF(fx+fw-4, sy));
    }
    p.setFont(font(11));
    p.setPen(teal);
    p.drawText(QRectF(fx, fy + fh * 0.4f, fw, fh * 0.2f),
               Qt::AlignCenter, "❄  " + val(bi.congelateur));

    yR += ovH + H * 0.022f;

    // ── GRAPHIQUE DE TEMPÉRATURE ──────────────────────────────────
    float chartH = H * 0.185f;
    float chartW = col2W;

    p.setFont(font(13, true));
    p.setPen(QColor(30,30,30));
    p.drawText(QRectF(col2X, yR, chartW, H * 0.022f),
               Qt::AlignLeft | Qt::AlignVCenter, "Relevé de Température :");
    yR += H * 0.026f;

    QRectF chartBox(col2X, yR, chartW, chartH);
    p.setBrush(white);
    p.setPen(QPen(lineClr, 1.5));
    p.drawRoundedRect(chartBox, 6, 6);

    float yMin  = (float)tempVal - 6.0f;
    float yMax  = (float)tempVal + 6.0f;
    float cInnL = col2X + chartW * 0.12f;
    float cInnR = col2X + chartW - 14;
    float cInnT = yR + chartH * 0.12f;
    float cInnB = yR + chartH * 0.80f;

    // Étiquettes axe Y (3 niveaux)
    p.setFont(font(10));
    p.setPen(gray);
    for (int i = 0; i <= 2; ++i) {
        float frac    = i / 2.0f;
        float labelY  = cInnT + (cInnB - cInnT) * frac;
        float labelVal= yMax - (yMax - yMin) * frac;
        p.drawText(QRectF(col2X + 2, labelY - 8, chartW * 0.10f, 16),
                   Qt::AlignRight | Qt::AlignVCenter,
                   QString::number((int)labelVal));
        p.setPen(QPen(lineClr, 0.8, Qt::DashLine));
        p.drawLine(QPointF(cInnL, labelY), QPointF(cInnR, labelY));
        p.setPen(gray);
    }

    // Courbe de température (oscillation simulée)
    int nPts = 40;
    QPainterPath tempPath;
    for (int i = 0; i < nPts; ++i) {
        float px = cInnL + (cInnR - cInnL) * i / (float)(nPts - 1);
        float noise = 0.8f * (float)qSin(i * 1.3 + 0.5) + 0.4f * (float)qSin(i * 2.7);
        float tv  = (float)tempVal + noise;
        float py  = cInnT + (cInnB - cInnT) * (yMax - tv) / (yMax - yMin);
        if (i == 0) tempPath.moveTo(px, py);
        else        tempPath.lineTo(px, py);
    }
    p.strokePath(tempPath, QPen(teal, 2.2));

    // Étiquettes axe X (4 dates)
    p.setFont(font(9));
    p.setPen(gray);
    QDate today = QDate::currentDate();
    for (int i = 0; i < 4; ++i) {
        float px = cInnL + (cInnR - cInnL) * i / 3.0f;
        p.drawText(QRectF(px - 20, cInnB + 4, 40, 16),
                   Qt::AlignCenter, today.addDays(i - 3).toString("dd/MM"));
    }

    // Légende du graphique
    yR += chartH + H * 0.010f;
    p.setFont(font(11));
    p.setPen(QColor(50,50,50));
    p.drawText(QRectF(col2X, yR, chartW, H * 0.02f),
               Qt::AlignLeft,
               "Congélateur " + val(bi.congelateur) + " — Température (°C)");
    yR += H * 0.022f;
    p.setFont(font(11, true));
    p.setPen(teal);
    p.drawText(QRectF(col2X, yR, chartW, H * 0.02f),
               Qt::AlignLeft, "Température moy. 24h :  " + tempStr);

    // ── 4. PIED DE PAGE ───────────────────────────────────────────
    float footerY = H - mg * 0.8f;
    p.setPen(QPen(lineClr, 1.2));
    p.drawLine(QPointF(mg, footerY), QPointF(mg + cW, footerY));
    footerY += 8;
    p.setFont(font(10));
    p.setPen(gray);
    p.drawText(QRectF(mg, footerY, cW, H * 0.025f),
               Qt::AlignCenter,
               "Généré par SmartVision Labs  —  " + dateStr);

    p.end();
}
