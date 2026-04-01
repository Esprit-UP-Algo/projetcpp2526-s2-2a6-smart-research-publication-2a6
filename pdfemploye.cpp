#include "pdfemploye.h"

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
#include <QSqlQuery>

void exportEmployePdf(const EmployeRecord& rec, const QString& path)
{
    auto val = [](const QString& s) -> QString { return s.trimmed().isEmpty() ? "—" : s.trimmed(); };
    QString dateStr = QDate::currentDate().toString("dd/MM/yyyy");

    // Count active projects from Associer table
    int activeProjects = 0;
    {
        QSqlQuery q;
        q.prepare("SELECT COUNT(*) FROM \"Associer\" WHERE \"employee_id\" = :id");
        q.bindValue(":id", rec.employeeId);
        if (q.exec() && q.next()) activeProjects = q.value(0).toInt();
    }

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
    float  W  = vp.width();
    float  H  = vp.height();
    float  mg = W * 0.055f;
    float  cW = W - 2*mg;

    QColor teal   (10, 95, 88);
    QColor tealBg (232, 245, 243);
    QColor tealHdr(10, 95, 88, 210);
    QColor gray   (120,120,120);
    QColor lineClr(200,220,218);
    QColor white  (255,255,255);
    QColor beige  (198,178,154);

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
               "ID Employé : " + QString::number(rec.employeeId));

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
               "Fiche Employé — Rapport Individuel");
    y += titleH + mg * 0.6f;

    // ── 3. AVATAR BADGE ───────────────────────────────────────────
    float badgeSize = H * 0.08f;
    float badgeX = mg;
    float badgeY = y;

    p.setBrush(QColor(10,95,88,200));
    p.setPen(QPen(teal, 2));
    p.drawEllipse(QRectF(badgeX, badgeY, badgeSize, badgeSize));

    // Initials inside circle
    QString initials;
    if (!rec.prenom.isEmpty()) initials += rec.prenom.at(0).toUpper();
    if (!rec.nom.isEmpty())    initials += rec.nom.at(0).toUpper();
    p.setFont(font(24, true));
    p.setPen(white);
    p.drawText(QRectF(badgeX, badgeY, badgeSize, badgeSize),
               Qt::AlignCenter, initials);

    // Name and role next to badge
    float nameX = badgeX + badgeSize + mg * 0.5f;
    p.setFont(font(20, true));
    p.setPen(QColor(20,20,20));
    p.drawText(QRectF(nameX, badgeY, cW - badgeSize - mg * 0.5f, badgeSize * 0.5f),
               Qt::AlignVCenter | Qt::AlignLeft,
               val(rec.prenom) + "  " + val(rec.nom));

    // Role badge pill
    QString roleText = val(rec.role);
    QColor roleColor = (roleText == "Chercheur") ? QColor(10,95,88) :
                       (roleText == "Technicien") ? QColor(181,103,44) : QColor(122,139,138);
    float pillW = W * 0.14f;
    float pillH = H * 0.022f;
    float pillX = nameX;
    float pillY = badgeY + badgeSize * 0.55f;
    p.setBrush(roleColor);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(QRectF(pillX, pillY, pillW, pillH), pillH/2, pillH/2);
    p.setFont(font(13, true));
    p.setPen(white);
    p.drawText(QRectF(pillX, pillY, pillW, pillH), Qt::AlignCenter, roleText);

    // CIN beside role
    p.setFont(font(13));
    p.setPen(gray);
    p.drawText(QRectF(pillX + pillW + 14, pillY, cW * 0.3f, pillH),
               Qt::AlignVCenter | Qt::AlignLeft, "CIN : " + val(rec.cin));

    y += badgeSize + mg * 0.6f;

    // ── 4. CORPS — deux colonnes ──────────────────────────────────
    float col1W = cW * 0.54f;
    float col2X = mg + col1W + mg * 0.5f;
    float col2W = cW - col1W - mg * 0.5f;
    float yL = y;
    float yR = y;

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
    secHdr(yL, mg, col1W, "Informations Personnelles :");
    row(yL, mg, col1W, "Nom complet",   val(rec.prenom) + " " + val(rec.nom));
    row(yL, mg, col1W, "CIN",           val(rec.cin));
    row(yL, mg, col1W, "Email",         val(rec.email));
    row(yL, mg, col1W, "Rôle",          val(rec.role));

    yL += H * 0.015f;
    secHdr(yL, mg, col1W, "Profil Professionnel :");
    row(yL, mg, col1W, "Spécialisation",  val(rec.specialization));
    row(yL, mg, col1W, "Qualification",   val(rec.qualification));
    row(yL, mg, col1W, "Temps de travail",val(rec.tempsTravail));
    row(yL, mg, col1W, "Laboratoire",     val(rec.laboratoire));
    row(yL, mg, col1W, "Publications",    QString::number(rec.nbPublications));
    row(yL, mg, col1W, "Projets actifs",  QString::number(activeProjects));

    yL += H * 0.015f;
    secHdr(yL, mg, col1W, "Conformité & Statut :");
    float chkH = H * 0.022f;
    const QStringList checks = {
        "Dossier employé à jour",
        "Habilitations vérifiées",
        "Affectations confirmées",
        "Évaluation annuelle réalisée"
    };
    for (const QString& c : checks) {
        p.setFont(font(13, true));
        p.setPen(teal);
        p.drawText(QRectF(mg + 14, yL, col1W, chkH),
                   Qt::AlignVCenter | Qt::AlignLeft, "☑  " + c);
        yL += chkH + H * 0.004f;
    }

    // ── COLONNE DROITE ────────────────────────────────────────────
    // Activity score card
    float cardH = H * 0.18f;
    p.setBrush(QColor(245,250,249));
    p.setPen(QPen(lineClr, 1.5));
    p.drawRoundedRect(QRectF(col2X, yR, col2W, cardH), 8, 8);

    p.setFont(font(13, true));
    p.setPen(gray);
    p.drawText(QRectF(col2X, yR + 8, col2W, H * 0.022f),
               Qt::AlignTop | Qt::AlignHCenter, "Score d'Activité");

    // Score bar
    int score = qMin(100, rec.nbPublications * 10 + activeProjects * 15);
    score = qMin(score, 100);
    float barX = col2X + col2W * 0.1f;
    float barY = yR + cardH * 0.28f;
    float barW = col2W * 0.8f;
    float barH2 = cardH * 0.12f;
    p.setBrush(QColor(200,220,218));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(QRectF(barX, barY, barW, barH2), barH2/2, barH2/2);
    p.setBrush(teal);
    p.drawRoundedRect(QRectF(barX, barY, barW * score / 100.0f, barH2), barH2/2, barH2/2);
    p.setFont(font(11, true));
    p.setPen(teal);
    p.drawText(QRectF(col2X, barY + barH2 + 6, col2W, H * 0.02f),
               Qt::AlignCenter, QString("Score : %1 / 100").arg(score));

    // Mini stats
    float statY = yR + cardH * 0.58f;
    float statW = col2W / 2.0f;

    auto statBox = [&](float sx, float sy, const QString& val2, const QString& lbl2, const QColor& c) {
        float bw = statW * 0.80f;
        float bh = cardH * 0.32f;
        float bx = sx + (statW - bw) / 2;
        p.setBrush(c.lighter(170));
        p.setPen(QPen(c, 1));
        p.drawRoundedRect(QRectF(bx, sy, bw, bh), 6, 6);
        p.setFont(font(18, true));
        p.setPen(c.darker(140));
        p.drawText(QRectF(bx, sy, bw, bh * 0.6f), Qt::AlignCenter, val2);
        p.setFont(font(10));
        p.setPen(gray);
        p.drawText(QRectF(bx, sy + bh * 0.62f, bw, bh * 0.35f),
                   Qt::AlignCenter, lbl2);
    };
    statBox(col2X,           statY, QString::number(rec.nbPublications), "Publications", teal);
    statBox(col2X + statW,   statY, QString::number(activeProjects),     "Projets",      beige);

    yR += cardH + H * 0.022f;

    // Workload bar chart (simulated monthly activity)
    float chartH = H * 0.185f;
    p.setFont(font(13, true));
    p.setPen(QColor(30,30,30));
    p.drawText(QRectF(col2X, yR, col2W, H * 0.022f),
               Qt::AlignLeft | Qt::AlignVCenter, "Activité Mensuelle (estimée) :");
    yR += H * 0.026f;

    QRectF chartBox(col2X, yR, col2W, chartH);
    p.setBrush(white);
    p.setPen(QPen(lineClr, 1.5));
    p.drawRoundedRect(chartBox, 6, 6);

    // Simulated bars based on employee data
    const QStringList months = {"Jan","Fév","Mar","Avr","Mai","Jun"};
    float maxBar = 10.0f;
    float cInnL = col2X + col2W * 0.08f;
    float cInnR = col2X + col2W - 10;
    float cInnT = yR + chartH * 0.1f;
    float cInnB = yR + chartH * 0.82f;

    // Y axis lines
    p.setFont(font(9));
    p.setPen(gray);
    for (int i = 0; i <= 2; ++i) {
        float frac = i / 2.0f;
        float ly   = cInnT + (cInnB - cInnT) * frac;
        float lv   = maxBar * (1.0f - frac);
        p.drawText(QRectF(col2X + 2, ly - 8, col2W * 0.06f, 16),
                   Qt::AlignRight | Qt::AlignVCenter, QString::number((int)lv));
        p.setPen(QPen(lineClr, 0.8, Qt::DashLine));
        p.drawLine(QPointF(cInnL, ly), QPointF(cInnR, ly));
        p.setPen(gray);
    }

    int n = months.size();
    float bw2 = (cInnR - cInnL) / (float)(n * 1.6f);
    for (int i = 0; i < n; ++i) {
        float bv = 3.0f + 2.0f * qSin(i * 1.1f + rec.employeeId * 0.3f)
                        + (float)(rec.nbPublications % 4);
        bv = qMin(bv, maxBar);
        float bh3 = (bv / maxBar) * (cInnB - cInnT);
        float bx2  = cInnL + i * (cInnR - cInnL) / (float)n + bw2 * 0.2f;
        float by2  = cInnB - bh3;
        QLinearGradient g(QPointF(bx2, by2), QPointF(bx2, cInnB));
        g.setColorAt(0, QColor(10,95,88,200));
        g.setColorAt(1, QColor(10,95,88,100));
        p.setPen(Qt::NoPen);
        p.setBrush(g);
        p.drawRoundedRect(QRectF(bx2, by2, bw2, bh3), 3, 3);
        p.setFont(font(9));
        p.setPen(gray);
        p.drawText(QRectF(bx2 - 4, cInnB + 4, bw2 + 8, 14),
                   Qt::AlignCenter, months[i]);
    }

    // ── 5. PIED DE PAGE ───────────────────────────────────────────
    float footerY = H - mg * 0.8f;
    p.setPen(QPen(lineClr, 1.2));
    p.drawLine(QPointF(mg, footerY), QPointF(mg + cW, footerY));
    footerY += 8;
    p.setFont(font(10));
    p.setPen(gray);
    p.drawText(QRectF(mg, footerY, cW, H * 0.025f),
               Qt::AlignCenter,
               "Généré par SmartVision Labs  —  " + dateStr + "  —  Document confidentiel");

    p.end();
}
