#include "pdfemploye.h"

#include <QPrinter>
#include <QPainter>
#include <QPageSize>
#include <QPageLayout>
#include <QFont>
#include <QColor>
#include <QDate>

void exportEmployePdf(const EmployeRecord& rec, const QString& path)
{
    auto val = [](const QString& s) -> QString { return s.trimmed().isEmpty() ? "-" : s.trimmed(); };

    const QString fullName = QString("%1 %2").arg(val(rec.prenom), val(rec.nom)).trimmed();
    const QString dateStr = QDate::currentDate().toString("dd/MM/yyyy");

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(path);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageMargins(QMarginsF(0, 0, 0, 0), QPageLayout::Millimeter);

    QPainter p(&printer);
    if (!p.isActive()) return;
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    const QRectF vp = QRectF(printer.pageRect(QPrinter::DevicePixel));
    const qreal W = vp.width();
    const qreal H = vp.height();
    const qreal mg = W * 0.055;
    const qreal cW = W - (2 * mg);

    const QColor teal(10, 95, 88);
    const QColor tealBg(232, 245, 243);
    const QColor tealHdr(10, 95, 88, 210);
    const QColor gray(120, 120, 120);
    const QColor lineClr(200, 220, 218);
    const QColor white(255, 255, 255);

    auto font = [](int sz, bool bold = false) {
        QFont f("Arial", sz);
        f.setBold(bold);
        return f;
    };

    qreal y = mg * 0.6;

    p.setFont(font(22, true));
    p.setPen(teal);
    p.drawText(QRectF(mg, y, cW * 0.6, H * 0.045), Qt::AlignVCenter | Qt::AlignLeft, "SmartVision Labs");

    p.setFont(font(13));
    p.setPen(gray);
    p.drawText(QRectF(mg + cW * 0.55, y, cW * 0.45, H * 0.022), Qt::AlignTop | Qt::AlignRight,
               "Date du rapport : " + dateStr);
    p.drawText(QRectF(mg + cW * 0.55, y + H * 0.02, cW * 0.45, H * 0.022), Qt::AlignTop | Qt::AlignRight,
               "ID Employe : " + QString::number(rec.employeeId));

    y += H * 0.055;

    p.setPen(QPen(teal, 2.5));
    p.drawLine(QPointF(mg, y), QPointF(mg + cW, y));
    y += mg * 0.5;

    const qreal titleH = H * 0.04;
    p.setBrush(tealBg);
    p.setPen(QPen(teal, 1.2));
    p.drawRoundedRect(QRectF(mg, y, cW, titleH), 6, 6);
    p.setFont(font(20, true));
    p.setPen(teal);
    p.drawText(QRectF(mg, y, cW, titleH), Qt::AlignCenter, "Rapport Employe");
    y += titleH + mg * 0.6;

    const qreal col1W = cW;
    qreal yL = y;

    auto secHdr = [&](qreal& cy, qreal colX, qreal colW, const QString& title) {
        const qreal hh = H * 0.022;
        p.setBrush(tealHdr);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(QRectF(colX, cy, colW, hh), 4, 4);
        p.setFont(font(13, true));
        p.setPen(white);
        p.drawText(QRectF(colX + 10, cy, colW - 10, hh), Qt::AlignVCenter | Qt::AlignLeft, title);
        cy += hh + H * 0.008;
    };

    auto row = [&](qreal& cy, qreal colX, qreal colW, const QString& lbl, const QString& value) {
        const qreal rh = H * 0.022;
        p.setFont(font(12));
        p.setPen(teal);
        p.drawText(QRectF(colX + 14, cy, colW * 0.44, rh), Qt::AlignVCenter | Qt::AlignLeft, QString("- %1 :").arg(lbl));
        p.setPen(QColor(30, 30, 30));
        p.drawText(QRectF(colX + colW * 0.46, cy, colW * 0.54, rh), Qt::AlignVCenter | Qt::AlignLeft, value);
        cy += rh + H * 0.003;
    };

    secHdr(yL, mg, col1W, "Identite");
    row(yL, mg, col1W, "Nom complet", fullName);
    row(yL, mg, col1W, "CIN", val(rec.cin));
    row(yL, mg, col1W, "Email", val(rec.email));
    row(yL, mg, col1W, "Role", val(rec.role));

    yL += H * 0.014;
    secHdr(yL, mg, col1W, "Profil scientifique");
    row(yL, mg, col1W, "Specialisation", val(rec.specialization));
    row(yL, mg, col1W, "Qualification", val(rec.qualification));
    row(yL, mg, col1W, "Publications", QString::number(qMax(0, rec.nbPublications)));

    yL += H * 0.014;
    secHdr(yL, mg, col1W, "Affectation");
    row(yL, mg, col1W, "Temps de travail", val(rec.tempsTravail));
    row(yL, mg, col1W, "Laboratoire", val(rec.laboratoire));
    row(yL, mg, col1W, "Projet", val(rec.projetAffecte));

    const qreal footerY = H - mg * 0.8;
    p.setPen(QPen(lineClr, 1.2));
    p.drawLine(QPointF(mg, footerY), QPointF(mg + cW, footerY));
    p.setFont(font(10));
    p.setPen(gray);
    p.drawText(QRectF(mg, footerY + 8, cW, H * 0.025), Qt::AlignCenter,
               "Genere par SmartVision Labs  -  " + dateStr);

    p.end();
}
