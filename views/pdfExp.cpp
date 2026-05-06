#include "pdfExp.h"

#include <QColor>
#include <QDate>
#include <QFont>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPrinter>

void exportExperiencePdf(const ExperiencePdfInfo& info, const QString& path)
{
    auto val = [](const QString& s) -> QString { return s.trimmed().isEmpty() ? "-" : s; };
    const QString dateStr = QDate::currentDate().toString("dd/MM/yyyy");

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(path);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageMargins(QMarginsF(0,0,0,0), QPageLayout::Millimeter);

    QPainter p(&printer);
    if (!p.isActive()) return;
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    QRectF vp = QRectF(printer.pageRect(QPrinter::DevicePixel));
    const float W = vp.width();
    const float H = vp.height();
    const float mg = W * 0.055f;
    const float cW = W - 2 * mg;

    QColor teal(10, 95, 88);
    QColor tealBg(232, 245, 243);
    QColor tealHdr(10, 95, 88, 210);
    QColor gray(120,120,120);
    QColor lineClr(200,220,218);
    QColor white(255,255,255);

    auto font = [](int sz, bool bold = false) {
        QFont f("Arial", sz);
        f.setBold(bold);
        return f;
    };

    float y = mg * 0.6f;

    QPixmap logo(":/image/smartvision.png");
    float logoH = H * 0.045f;
    float logoW = logo.isNull() ? 0 : logoH * logo.width() / (float)logo.height();
    if (!logo.isNull()) {
        p.drawPixmap(QRectF(mg, y, logoW, logoH).toRect(), logo);
    }

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
               "Projet : " + val(info.projet));

    y += logoH + mg * 0.4f;
    p.setPen(QPen(teal, 2.5));
    p.drawLine(QPointF(mg, y), QPointF(mg + cW, y));
    y += mg * 0.5f;

    const float titleH = H * 0.04f;
    p.setBrush(tealBg);
    p.setPen(QPen(teal, 1.2));
    p.drawRoundedRect(QRectF(mg, y, cW, titleH), 6, 6);
    p.setFont(font(20, true));
    p.setPen(teal);
    p.drawText(QRectF(mg, y, cW, titleH),
               Qt::AlignCenter,
               "Rapport de Suivi des Experiences");
    y += titleH + mg * 0.6f;

    const float col1W = cW * 0.54f;
    const float col2X = mg + col1W + mg * 0.5f;
    const float col2W = cW - col1W - mg * 0.5f;
    float yL = y;
    float yR = y;

    auto secHdr = [&](float& cy, float colX, float colW, const QString& title) {
        const float hh = H * 0.022f;
        p.setBrush(tealHdr);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(QRectF(colX, cy, colW, hh), 4, 4);
        p.setFont(font(14, true));
        p.setPen(white);
        p.drawText(QRectF(colX + 10, cy, colW - 10, hh),
                   Qt::AlignVCenter | Qt::AlignLeft, title);
        cy += hh + H * 0.008f;
    };

    auto row = [&](float& cy, float colX, float colW,
                   const QString& lbl, const QString& value) {
        const float rh = H * 0.022f;
        p.setFont(font(13));
        p.setPen(teal);
        p.drawText(QRectF(colX + 14, cy, colW * 0.42f, rh),
                   Qt::AlignVCenter | Qt::AlignLeft, "* " + lbl + " :");
        p.setPen(QColor(30,30,30));
        p.drawText(QRectF(colX + colW * 0.44f, cy, colW * 0.56f, rh),
                   Qt::AlignVCenter | Qt::AlignLeft, val(value));
        cy += rh + H * 0.003f;
    };

    secHdr(yL, mg, col1W, "Details de l'Experience :");
    row(yL, mg, col1W, "Titre", info.titre);
    row(yL, mg, col1W, "Hypothese", info.hypothese);
    row(yL, mg, col1W, "Type experience", info.typeExperience);
    row(yL, mg, col1W, "Statut", info.statut);

    yL += H * 0.018f;
    secHdr(yL, mg, col1W, "Planification :");
    row(yL, mg, col1W, "Date debut", info.dateDebut);
    row(yL, mg, col1W, "Date fin", info.dateFin);
    row(yL, mg, col1W, "Equipement utilise", info.disponibilite);

    yL += H * 0.018f;
    secHdr(yL, mg, col1W, "Resultat :");
    const float resultBoxH = H * 0.18f;
    p.setBrush(QColor(245,250,249));
    p.setPen(QPen(lineClr, 1.2));
    p.drawRoundedRect(QRectF(mg, yL, col1W, resultBoxH), 8, 8);
    p.setFont(font(12));
    p.setPen(QColor(35,35,35));
    p.drawText(QRectF(mg + 14, yL + 12, col1W - 28, resultBoxH - 18),
               Qt::AlignTop | Qt::TextWordWrap, val(info.resultat));

    const float ovH = H * 0.18f;
    p.setBrush(QColor(245,250,249));
    p.setPen(QPen(lineClr, 1.5));
    p.drawRoundedRect(QRectF(col2X, yR, col2W, ovH), 8, 8);

    p.setFont(font(13, true));
    p.setPen(gray);
    p.drawText(QRectF(col2X, yR + 8, col2W, H * 0.022f),
               Qt::AlignTop | Qt::AlignHCenter, "Apercu du Statut");

    auto statusColor = [&](const QString& status) -> QColor {
        const QString s = status.toLower();
        if (s.contains("cours")) return QColor("#2E6F63");
        if (s.contains("conclu") || s.contains("reuss")) return QColor("#3E7FA7");
        if (s.contains("chou") || s.contains("suspend")) return QColor("#8B2F3C");
        return QColor("#B5672C");
    };

    const QColor badge = statusColor(info.statut);
    const float pillW = col2W * 0.64f;
    const float pillH = H * 0.042f;
    const float pillX = col2X + (col2W - pillW) * 0.5f;
    const float pillY = yR + ovH * 0.42f;

    p.setBrush(badge);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(QRectF(pillX, pillY, pillW, pillH), pillH / 2.0f, pillH / 2.0f);
    p.setPen(Qt::white);
    p.setFont(font(12, true));
    p.drawText(QRectF(pillX, pillY, pillW, pillH), Qt::AlignCenter, val(info.statut));

    yR += ovH + H * 0.022f;
    const float chartH = H * 0.185f;
    p.setFont(font(13, true));
    p.setPen(QColor(30,30,30));
    p.drawText(QRectF(col2X, yR, col2W, H * 0.022f),
               Qt::AlignLeft | Qt::AlignVCenter, "Synthese de l'experience :");
    yR += H * 0.026f;

    p.setBrush(white);
    p.setPen(QPen(lineClr, 1.5));
    p.drawRoundedRect(QRectF(col2X, yR, col2W, chartH), 6, 6);

    p.setFont(font(11));
    p.setPen(QColor(65,65,65));
    const QString synth = QString("* Projet : %1\n* Type : %2\n* Equipement utilise : %3\n* Periode : %4 -> %5")
                              .arg(val(info.projet),
                                   val(info.typeExperience),
                                   val(info.disponibilite),
                                   val(info.dateDebut),
                                   val(info.dateFin));
    p.drawText(QRectF(col2X + 14, yR + 12, col2W - 28, chartH - 18),
               Qt::AlignTop | Qt::TextWordWrap, synth);

    const float footerY = H - mg * 0.8f;
    p.setPen(QPen(lineClr, 1.2));
    p.drawLine(QPointF(mg, footerY), QPointF(mg + cW, footerY));
    p.setFont(font(10));
    p.setPen(gray);
    p.drawText(QRectF(mg, footerY + 8, cW, H * 0.025f),
               Qt::AlignCenter,
               "Genere par SmartVision Labs  -  " + dateStr);

    p.end();
}
