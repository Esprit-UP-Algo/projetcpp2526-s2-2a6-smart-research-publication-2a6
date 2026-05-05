#include "pdfequipement.h"

#include <QColor>
#include <QDate>
#include <QFont>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPixmap>
#include <QPrinter>

void exportEquipementPdf(const EquipementRecord& info, const QString& path)
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
               "Statut : " + val(info.statut));

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
               "Rapport de Suivi des Equipements");
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

    auto dateText = [&](const QDate& d) -> QString {
        return d.isValid() ? d.toString("dd/MM/yyyy") : QString("-");
    };

    secHdr(yL, mg, col1W, "Details de l'equipement :");
    row(yL, mg, col1W, "ID", QString::number(info.id));
    row(yL, mg, col1W, "Nom", info.nomEquipement);
    row(yL, mg, col1W, "Fabricant", info.fabricant);
    row(yL, mg, col1W, "Modele", info.numeroModele);

    yL += H * 0.018f;
    secHdr(yL, mg, col1W, "Maintenance & calibration :");
    row(yL, mg, col1W, "Date d'achat", dateText(info.dateAchat));
    row(yL, mg, col1W, "Derniere maintenance", dateText(info.dateDerniereMaintenance));
    row(yL, mg, col1W, "Prochaine maintenance", dateText(info.dateProchaineMaintenance));
    row(yL, mg, col1W, "Calibration", dateText(info.dateLimiteCalibration));

    yL += H * 0.018f;
    secHdr(yL, mg, col1W, "Affectation :");
    row(yL, mg, col1W, "Localisation", info.localisation);
    row(yL, mg, col1W, "Lien experience", info.idExp.isNull() ? QString("-") : QString("ID ") + QString::number(info.idExp.toInt()));

    const float ovH = H * 0.18f;
    p.setBrush(QColor(245,250,249));
    p.setPen(QPen(lineClr, 1.5));
    p.drawRoundedRect(QRectF(col2X, yR, col2W, ovH), 8, 8);

    p.setFont(font(13, true));
    p.setPen(gray);
    p.drawText(QRectF(col2X, yR + 8, col2W, H * 0.022f),
               Qt::AlignTop | Qt::AlignHCenter, "Apercu du statut");

    auto statusColor = [&](const QString& status) -> QColor {
        const QString s = status.toLower();
        if (s.contains("hors") || s.contains("service")) return QColor("#8B2F3C");
        if (s.contains("maint") || s.contains("arch")) return QColor("#B5672C");
        return QColor("#2E6F63");
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
               Qt::AlignLeft | Qt::AlignVCenter, "Synthese equipement :");
    yR += H * 0.026f;

    p.setBrush(white);
    p.setPen(QPen(lineClr, 1.5));
    p.drawRoundedRect(QRectF(col2X, yR, col2W, chartH), 6, 6);

    p.setFont(font(11));
    p.setPen(QColor(65,65,65));
    const QString synth = QString("* Nom : %1\n* Localisation : %2\n* Statut : %3\n* Prochaine maintenance : %4")
                              .arg(val(info.nomEquipement),
                                   val(info.localisation),
                                   val(info.statut),
                                   dateText(info.dateProchaineMaintenance));
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
