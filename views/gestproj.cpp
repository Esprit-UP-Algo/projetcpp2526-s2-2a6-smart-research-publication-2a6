#include "gestproj.h"
#include <QRegularExpression>
#include <QTimer>
#include <QGridLayout>
#include <QTextEdit>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QDate>
#include <QDateTime>
#include <QMessageBox>
#include <QTextStream>
#include <QDesktopServices>
#include <QUrl>

// ─────────────────────────────────────────────────────────────
//  VALIDATION
// ─────────────────────────────────────────────────────────────

static bool isSqlDangerous(const QString& s)
{
    static const QRegularExpression re(R"([;'"\\]|--|/\*)");
    return re.match(s).hasMatch();
}

QString GestProjCrud::validateProjet(const ProjetRecord& in, bool /*isUpdate*/)
{
    // ── Nom du projet ────────────────────────────────────────
    const QString nom = in.nomDuProjet.trimmed();
    if (nom.isEmpty())
        return "Le nom du projet est obligatoire.";
    if (nom.length() < 3)
        return "Le nom du projet doit comporter au moins 3 caractères.";
    if (nom.length() > 150)
        return "Le nom du projet ne peut pas dépasser 150 caractères.";
    {
        // Must start with a letter; remainder can be letters, digits, underscore, or spaces
        // Valid:   abdsf12  |  art_4  |  MyProject1
        // Invalid: -cjcd    |  12dhd  |  _test
        static const QRegularExpression allowed(R"(^[A-Za-zÀ-ÖØ-öø-ÿ][A-Za-zÀ-ÖØ-öø-ÿ0-9_ ]*$)");
        if (!allowed.match(nom).hasMatch())
            return "Le nom du projet est invalide : il doit commencer par une lettre et ne contenir que des lettres, chiffres, espaces ou underscores (_).\nExemples valides : abdsf12, art_4, MonProjet1";
    }
    if (isSqlDangerous(nom))
        return "Le nom du projet contient des caractères interdits.";

    // ── Domaine de recherche ─────────────────────────────────
    if (in.domaineDeRecherche.trimmed().isEmpty())
        return "Le domaine de recherche est obligatoire.";

    // ── Statut ───────────────────────────────────────────────
    static const QStringList validStatuts = {
        "En cours", "Planifié", "Terminé", "Suspendu", "Annulé", "En retard", "Critique"
    };
    if (!validStatuts.contains(in.statut.trimmed()))
        return "Statut invalide. Valeurs acceptées : Planifié, En cours, En retard, Critique, Suspendu, Terminé, Annulé.";

    // ── Date début ───────────────────────────────────────────
    if (!in.dateDeDebut.isValid())
        return "La date de début est obligatoire.";
    if (in.dateDeDebut.year() < 2000)
        return "La date de début ne peut pas être antérieure à l'an 2000.";

    // ── Date fin (optional but conditional) ──────────────────
    if (in.dateDeFin.isValid()) {
        if (in.dateDeFin <= in.dateDeDebut)
            return "La date de fin doit être strictement postérieure à la date de début.";
        if (in.dateDeFin < in.dateDeDebut.addMonths(1))
            return "La durée du projet doit être d'au moins 1 mois.";
        if (in.dateDeFin > in.dateDeDebut.addYears(20))
            return "La durée du projet semble excessive (> 20 ans). Veuillez vérifier les dates.";
    }

    // ── Budget ───────────────────────────────────────────────
    if (in.budget < 0.0)
        return "Le budget ne peut pas être négatif.";
    if (in.budget > 99999999999.99)
        return "Le budget dépasse la valeur maximale autorisée.";

    // ── Source de financement (required if budget > 0) ───────
    if (in.budget > 0.0 && in.sourceDeFinancement.trimmed().isEmpty())
        return "⚠️ Erreur : Une source de financement est requise.\n\nVous avez spécifié un budget. Vous devez entrer une source de financement (ex. ANR, CNRS, privé, etc.).";
    if (in.sourceDeFinancement.trimmed().length() > 150)
        return "La source de financement ne peut pas dépasser 150 caractères.";

    // ── Minimum budget requirement when financing source is specified ───
    if (!in.sourceDeFinancement.trimmed().isEmpty() && in.budget > 0.0 && in.budget < 300.0)
        return "❌ Erreur : Budget insuffisant.\n\nLe budget doit être d'au moins 300 DT.\n\nBudget actuel : " + QString::number(in.budget, 'f', 2) + " DT";
    if (!in.sourceDeFinancement.trimmed().isEmpty() && in.budget == 0.0)
        return "⚠️ Erreur : Budget manquant.\n\nSi vous spécifiez une source de financement, vous devez entrer un budget d'au moins 300 DT.";

    // ── Numéro d'approbation éthique ─────────────────────────
    if (in.statut.trimmed() == "En cours" && in.numeroDApprobationEthique.trimmed().isEmpty())
        return "Le numéro d'approbation éthique est obligatoire pour un projet « En cours ».";
    if (!in.numeroDApprobationEthique.trimmed().isEmpty()) {
        if (in.numeroDApprobationEthique.trimmed().length() > 100)
            return "Le numéro d'approbation éthique ne peut pas dépasser 100 caractères.";
        static const QRegularExpression ethiqueRe(R"(^[A-Za-z0-9\-/]+$)");
        if (!ethiqueRe.match(in.numeroDApprobationEthique.trimmed()).hasMatch())
            return "Le numéro d'approbation éthique ne doit contenir que des lettres, chiffres, tirets et slashes.\nExemple : CPP-2024/017";
    }

    // ── Nombre de publications ───────────────────────────────
    if (in.nombreDePublications < 0)
        return "Le nombre de publications ne peut pas être négatif.";
    if (in.nombreDePublications > 9999)
        return "Le nombre de publications ne peut pas dépasser 9999.";

    return QString(); // empty = valid
}

// ─────────────────────────────────────────────────────────────
//  CRUD
// ─────────────────────────────────────────────────────────────

bool GestProjCrud::loadProjets(QList<ProjetRecord>& out,
                               QString* error,
                               const QString& nom,
                               const QString& domaine,
                               const QString& statut)
{
    out.clear();

    QSqlQuery q;
    q.prepare(
        "SELECT \"Id_projet\", \"nom_du_projet\", \"domaine_de_recherche\", "
        "\"date_de_début\", \"date_de_fin\", \"budget\", \"statut\", "
        "\"source_de_financement\", \"numéro_d_approbation_éthique\", "
        "\"nombre_de_publications\" "
        "FROM \"projet\" "
        "WHERE (:nom IS NULL OR :nom = '' OR LOWER(\"nom_du_projet\") LIKE '%' || LOWER(:nom) || '%') "
        "  AND (:domaine IS NULL OR :domaine = '' OR LOWER(\"domaine_de_recherche\") LIKE '%' || LOWER(:domaine) || '%') "
        "  AND (:statut IS NULL OR :statut = '' OR LOWER(\"statut\") LIKE '%' || LOWER(:statut) || '%') "
        "ORDER BY \"nom_du_projet\", \"Id_projet\"");

    q.bindValue(":nom", nom);
    q.bindValue(":domaine", domaine);
    q.bindValue(":statut", statut);

    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }

    while (q.next()) {
        ProjetRecord rec;
        rec.idProjet                  = q.value(0).toInt();
        rec.nomDuProjet               = q.value(1).toString();
        rec.domaineDeRecherche        = q.value(2).toString();
        rec.dateDeDebut               = q.value(3).toDate();
        rec.dateDeFin                 = q.value(4).toDate();
        rec.budget                    = q.value(5).toDouble();
        rec.statut                    = q.value(6).toString();
        rec.sourceDeFinancement       = q.value(7).toString();
        rec.numeroDApprobationEthique = q.value(8).toString();
        rec.nombreDePublications      = q.value(9).toInt();
        out.push_back(rec);
    }

    return true;
}

bool GestProjCrud::fetchProjet(int idProjet, ProjetRecord& out, QString* error)
{
    QSqlQuery q;
    q.prepare("SELECT \"nom_du_projet\", \"domaine_de_recherche\", "
              "\"date_de_début\", \"date_de_fin\", \"budget\", \"statut\", "
              "\"source_de_financement\", \"numéro_d_approbation_éthique\", "
              "\"nombre_de_publications\" "
              "FROM \"projet\" WHERE \"Id_projet\" = :id");
    q.bindValue(":id", idProjet);

    if (!q.exec() || !q.next()) {
        if (error) *error = q.lastError().text();
        return false;
    }

    out.idProjet                  = idProjet;
    out.nomDuProjet               = q.value(0).toString();
    out.domaineDeRecherche        = q.value(1).toString();
    out.dateDeDebut               = q.value(2).toDate();
    out.dateDeFin                 = q.value(3).toDate();
    out.budget                    = q.value(4).toDouble();
    out.statut                    = q.value(5).toString();
    out.sourceDeFinancement       = q.value(6).toString();
    out.numeroDApprobationEthique = q.value(7).toString();
    out.nombreDePublications      = q.value(8).toInt();

    return true;
}

bool GestProjCrud::deleteProjet(int idProjet, QString* error)
{
    // Supprimer d'abord les expériences liées (FK_EXPERIENCE_PROJET)
    // et leurs équipements enfants avant de supprimer le projet
    {
        QSqlQuery qExpIds;
        qExpIds.prepare("SELECT \"Id_exp\" FROM \"Expérience\" WHERE \"Id_projet\" = :id");
        qExpIds.bindValue(":id", idProjet);
        if (qExpIds.exec()) {
            while (qExpIds.next()) {
                const int expId = qExpIds.value(0).toInt();
                QSqlQuery qEq;
                qEq.prepare("DELETE FROM \"Équipement\" WHERE \"Id_exp\" = :eid");
                qEq.bindValue(":eid", expId);
                qEq.exec(); // best-effort, ignore error
            }
        }
    }

    QSqlQuery qExp;
    qExp.prepare("DELETE FROM \"Expérience\" WHERE \"Id_projet\" = :id");
    qExp.bindValue(":id", idProjet);
    if (!qExp.exec()) {
        if (error) *error = qExp.lastError().text();
        return false;
    }

    // Supprimer aussi les affectations employés liées au projet
    QSqlQuery qAssoc;
    qAssoc.prepare("DELETE FROM \"Associer\" WHERE \"Id_projet\" = :id");
    qAssoc.bindValue(":id", idProjet);
    if (!qAssoc.exec()) {
        if (error) *error = qAssoc.lastError().text();
        return false;
    }

    QSqlQuery q;
    q.prepare("DELETE FROM \"projet\" WHERE \"Id_projet\" = :id");
    q.bindValue(":id", idProjet);
    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }

    return true;
}

int GestProjCrud::nextProjetId(QString* error)
{
    QSqlQuery q;
    if (!q.exec("SELECT NVL(MAX(\"Id_projet\"),0)+1 FROM \"projet\"") || !q.next()) {
        if (error) *error = q.lastError().text();
        return -1;
    }

    return q.value(0).toInt();
}

bool GestProjCrud::insertProjet(const ProjetRecord& in, QString* error)
{
    const QString valErr = validateProjet(in, false);
    if (!valErr.isEmpty()) {
        if (error) *error = valErr;
        return false;
    }

    // ── Duplicate name check ────────────────────────────────
    {
        QSqlQuery chk;
        chk.prepare("SELECT COUNT(*) FROM \"projet\" WHERE LOWER(TRIM(\"nom_du_projet\")) = LOWER(TRIM(:nom))");
        chk.bindValue(":nom", in.nomDuProjet.trimmed());
        if (chk.exec() && chk.next() && chk.value(0).toInt() > 0) {
            if (error) *error = "Un projet portant ce nom existe déjà. Veuillez choisir un nom différent.";
            return false;
        }
    }

    int idProjet = in.idProjet;
    if (idProjet <= 0) {
        idProjet = nextProjetId(error);
        if (idProjet <= 0) return false;
    }

    QSqlQuery q;
    q.prepare("INSERT INTO \"projet\" "
              "(\"Id_projet\", \"nom_du_projet\", \"domaine_de_recherche\", "
              "\"date_de_début\", \"date_de_fin\", \"budget\", \"statut\", "
              "\"source_de_financement\", \"numéro_d_approbation_éthique\", "
              "\"nombre_de_publications\") "
              "VALUES (:id, :nom, :domaine, :debut, :fin, :budget, "
              ":statut, :financement, :ethique, :pubs)");

    auto nullStr  = QVariant(QMetaType::fromType<QString>());
    auto nullDate = QVariant(QMetaType::fromType<QDate>());

    q.bindValue(":id", idProjet);
    q.bindValue(":nom", in.nomDuProjet.trimmed());
    q.bindValue(":domaine", in.domaineDeRecherche.trimmed().isEmpty() ? nullStr : QVariant(in.domaineDeRecherche.trimmed()));
    q.bindValue(":debut", in.dateDeDebut.isValid() ? QVariant(in.dateDeDebut) : nullDate);
    q.bindValue(":fin", in.dateDeFin.isValid() ? QVariant(in.dateDeFin) : nullDate);
    q.bindValue(":budget", in.budget);
    q.bindValue(":statut", in.statut.trimmed().isEmpty() ? nullStr : QVariant(in.statut.trimmed()));
    q.bindValue(":financement", in.sourceDeFinancement.trimmed().isEmpty() ? nullStr : QVariant(in.sourceDeFinancement.trimmed()));
    q.bindValue(":ethique", in.numeroDApprobationEthique.trimmed().isEmpty() ? nullStr : QVariant(in.numeroDApprobationEthique.trimmed()));
    q.bindValue(":pubs", in.nombreDePublications);

    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }

    return true;
}

bool GestProjCrud::updateProjet(const ProjetRecord& in, QString* error)
{
    if (in.idProjet <= 0) {
        if (error) *error = "Id_projet invalide.";
        return false;
    }

    const QString valErr = validateProjet(in, true);
    if (!valErr.isEmpty()) {
        if (error) *error = valErr;
        return false;
    }

    // ── Duplicate name check (exclude the current record) ───
    {
        QSqlQuery chk;
        chk.prepare("SELECT COUNT(*) FROM \"projet\" WHERE LOWER(TRIM(\"nom_du_projet\")) = LOWER(TRIM(:nom)) AND \"Id_projet\" <> :id");
        chk.bindValue(":nom", in.nomDuProjet.trimmed());
        chk.bindValue(":id", in.idProjet);
        if (chk.exec() && chk.next() && chk.value(0).toInt() > 0) {
            if (error) *error = "Un projet portant ce nom existe déjà. Veuillez choisir un nom différent.";
            return false;
        }
    }

    QSqlQuery q;
    q.prepare("UPDATE \"projet\" SET "
              "\"nom_du_projet\" = :nom, "
              "\"domaine_de_recherche\" = :domaine, "
              "\"date_de_début\" = :debut, "
              "\"date_de_fin\" = :fin, "
              "\"budget\" = :budget, "
              "\"statut\" = :statut, "
              "\"source_de_financement\" = :financement, "
              "\"numéro_d_approbation_éthique\" = :ethique, "
              "\"nombre_de_publications\" = :pubs "
              "WHERE \"Id_projet\" = :id");

    auto nullStr  = QVariant(QMetaType::fromType<QString>());
    auto nullDate = QVariant(QMetaType::fromType<QDate>());

    q.bindValue(":nom", in.nomDuProjet.trimmed());
    q.bindValue(":domaine", in.domaineDeRecherche.trimmed().isEmpty() ? nullStr : QVariant(in.domaineDeRecherche.trimmed()));
    q.bindValue(":debut", in.dateDeDebut.isValid() ? QVariant(in.dateDeDebut) : nullDate);
    q.bindValue(":fin", in.dateDeFin.isValid() ? QVariant(in.dateDeFin) : nullDate);
    q.bindValue(":budget", in.budget);
    q.bindValue(":statut", in.statut.trimmed().isEmpty() ? nullStr : QVariant(in.statut.trimmed()));
    q.bindValue(":financement", in.sourceDeFinancement.trimmed().isEmpty() ? nullStr : QVariant(in.sourceDeFinancement.trimmed()));
    q.bindValue(":ethique", in.numeroDApprobationEthique.trimmed().isEmpty() ? nullStr : QVariant(in.numeroDApprobationEthique.trimmed()));
    q.bindValue(":pubs", in.nombreDePublications);
    q.bindValue(":id", in.idProjet);

    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }

    return true;
}

// ─────────────────────────────────────────────────────────────
//  STATISTIQUE : Répartition des projets par domaine
// ─────────────────────────────────────────────────────────────

QMap<QString,int> GestProjCrud::loadDomaineStats(QString* error)
{
    QMap<QString,int> result;

    QSqlQuery q;
    if (!q.exec(
            "SELECT COALESCE(TRIM(\"domaine_de_recherche\"), '(Non défini)'), "
            "COUNT(*) "
            "FROM \"projet\" "
            "GROUP BY TRIM(\"domaine_de_recherche\") "
            "ORDER BY COUNT(*) DESC"))
    {
        if (error) *error = q.lastError().text();
        return result;
    }

    while (q.next()) {
        QString domaine = q.value(0).toString();
        if (domaine.isEmpty()) domaine = "(Non défini)";
        result[domaine] = q.value(1).toInt();
    }

    return result;
}

// ── Self-contained horizontal bar-chart dialog ───────────────
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QFont>
#include <QFontMetrics>
#include <algorithm>
#include <QEvent>
void GestProjCrud::showDomaineChart(QWidget* parent)
{
    // ── 1. Fetch data ─────────────────────────────────────────
    GestProjCrud crud;
    QString err;
    QMap<QString,int> raw = crud.loadDomaineStats(&err);

    // ── 2. Build dialog shell ─────────────────────────────────
    QDialog* dlg = new QDialog(parent,
                               Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dlg->setWindowTitle("Répartition des projets par domaine de recherche");
    dlg->setMinimumSize(720, 480);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setStyleSheet("QDialog{ background: #EAF3F5; }");

    QVBoxLayout* mainL = new QVBoxLayout(dlg);
    mainL->setContentsMargins(24, 20, 24, 16);
    mainL->setSpacing(14);

    // Title label
    QLabel* title = new QLabel("  Distribution des projets par domaine de recherche");
    title->setStyleSheet(
        "color: #0A5F58; font-size: 15px; font-weight: 900;"
        "background: rgba(10,95,88,0.08); border-radius: 10px; padding: 8px 16px;");
    mainL->addWidget(title);

    // ── 3. No-data guard ──────────────────────────────────────
    if (raw.isEmpty()) {
        QLabel* noData = new QLabel(err.isEmpty()
                                    ? "Aucun projet trouvé dans la base de données."
                                    : "Erreur de chargement : " + err);
        noData->setAlignment(Qt::AlignCenter);
        noData->setStyleSheet("color: #8B2F3C; font-size: 13px; font-weight: 600;");
        mainL->addWidget(noData, 1);

        QPushButton* closeBtn = new QPushButton("Fermer");
        closeBtn->setFixedHeight(36);
        closeBtn->setStyleSheet(
            "QPushButton{ background:#0A5F58; color:white; border-radius:8px;"
            "font-weight:700; font-size:13px; padding:0 20px; }"
            "QPushButton:hover{ background:#12443B; }");
        QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);
        QHBoxLayout* bl = new QHBoxLayout;
        bl->addStretch(1);
        bl->addWidget(closeBtn);
        mainL->addLayout(bl);

        dlg->exec();
        return;
    }

    // ── 4. Sort by count desc ─────────────────────────────────
    QList<QPair<QString,int>> sorted;
    for (auto it = raw.begin(); it != raw.end(); ++it)
        sorted.append({it.key(), it.value()});
    std::sort(sorted.begin(), sorted.end(),
              [](const QPair<QString,int>& a, const QPair<QString,int>& b){
                  return a.second > b.second;
              });

    const int maxVal = sorted.isEmpty() ? 1 : sorted.first().second;
    int totalProjects = 0;
    for (const auto& kv : sorted) totalProjects += kv.second;

    // ── 5. Colour palette ─────────────────────────────────────
    static const QColor palette[] = {
        QColor("#0A5F58"), QColor("#2E8B7C"), QColor("#B5672C"),
        QColor("#416E66"), QColor("#7B4D9E"), QColor("#1A7BAF"),
        QColor("#8B2F3C"), QColor("#4CAF82"), QColor("#D4762A"),
        QColor("#5C6BC0")
    };
    static const int palSize = (int)(sizeof(palette)/sizeof(palette[0]));

    // ── 6. Chart widget ───────────────────────────────────────
    QWidget* chart = new QWidget;
    chart->setMinimumHeight(sorted.size() * 52 + 40);
    chart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    chart->setStyleSheet("background: transparent;");

    // Capture data for the paint event filter
    struct PaintFilter : public QObject {
        QList<QPair<QString,int>> items;
        int maxValue;
        int totalValue;
        const QColor* pal;
        int palSz;

        PaintFilter(QList<QPair<QString,int>> it, int mv, int tv,
                    const QColor* p, int ps, QObject* parent)
            : QObject(parent), items(it), maxValue(mv), totalValue(tv), pal(p), palSz(ps) {}

        bool eventFilter(QObject* obj, QEvent* ev) override {
            if (ev->type() != QEvent::Paint) return false;
            QWidget* w = static_cast<QWidget*>(obj);
            QPainter painter(w);
            painter.setRenderHint(QPainter::Antialiasing);

            const int W       = w->width();
            const int barH    = 34;
            const int rowH    = 52;
            const int labelW  = 190;
            const int valW    = 110;
            const int padTop  = 18;
            const int padLeft = 16;
            const int barAreaW = qMax(W - labelW - valW - padLeft * 2, 60);

            painter.fillRect(w->rect(), QColor("#EAF3F5"));

            for (int i = 0; i < items.size(); ++i) {
                const QString& dom = items[i].first;
                const int cnt      = items[i].second;
                const int y        = padTop + i * rowH;

                // Alternating row bg
                QColor rowBg = (i % 2 == 0)
                                   ? QColor(255,255,255,110)
                                   : QColor(200,225,230,70);
                painter.fillRect(QRect(0, y - 4, W, rowH - 4), rowBg);

                // Domain label
                QFont lf("Segoe UI", 10, QFont::DemiBold);
                painter.setFont(lf);
                painter.setPen(QColor("#12443B"));
                QRect labelRect(padLeft, y, labelW - 8, barH);
                QString elidedText = QFontMetrics(lf).elidedText(dom, Qt::ElideRight, labelW - 12);
                painter.drawText(labelRect, Qt::AlignVCenter | Qt::AlignRight, elidedText);

                // Bar
                int barW = (maxValue > 0)
                               ? (int)((double)cnt / maxValue * barAreaW)
                               : 0;
                if (barW < 4 && cnt > 0) barW = 4;

                const QColor& col = pal[i % palSz];
                QRect barRect(padLeft + labelW, y + (barH - 24) / 2, barW, 24);
                QPainterPath path;
                path.addRoundedRect(barRect, 6, 6);
                painter.fillPath(path, col);

                // Highlight gradient on bar
                QLinearGradient grad(barRect.topLeft(), barRect.bottomLeft());
                grad.setColorAt(0, QColor(255,255,255,55));
                grad.setColorAt(1, QColor(0,0,0,18));
                painter.fillPath(path, grad);

                // Count text + percentage
                double pct = totalValue > 0 ? 100.0 * cnt / totalValue : 0.0;
                QFont nf("Segoe UI", 10, QFont::Bold);
                painter.setFont(nf);
                painter.setPen(QColor("#0A5F58"));
                QRect numRect(padLeft + labelW + barW + 8, y, valW, barH);
                painter.drawText(numRect, Qt::AlignVCenter | Qt::AlignLeft,
                                 QString("%1 projet%2 (%3%)")
                                     .arg(cnt)
                                     .arg(cnt > 1 ? "s" : "")
                                     .arg(QString::number(pct, 'f', 1)));
            }
            return true;
        }
    };

    chart->installEventFilter(
        new PaintFilter(sorted, maxVal, totalProjects, palette, palSize, chart));

    // ── 7. Scroll area ────────────────────────────────────────
    QScrollArea* scroll = new QScrollArea;
    scroll->setWidget(chart);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(
        "QScrollArea{ background:#EAF3F5; border:1px solid rgba(0,0,0,0.10);"
        "border-radius:12px; }"
        "QScrollBar:vertical{ width:8px; background:transparent; }"
        "QScrollBar::handle:vertical{ background:rgba(10,95,88,0.35); border-radius:4px; }");
    mainL->addWidget(scroll, 1);

    // ── 8. Summary ────────────────────────────────────────────
    QLabel* summary = new QLabel(
        QString("  %1 domaine(s) de recherche   ·   %2 projet(s) au total")
            .arg(sorted.size()).arg(totalProjects));
    summary->setStyleSheet(
        "color:#416E66; font-size:11px; font-weight:600;"
        "background:rgba(10,95,88,0.06); border-radius:8px; padding:5px 12px;");
    mainL->addWidget(summary);

    // ── 9. Close button ───────────────────────────────────────
    QPushButton* closeBtn = new QPushButton("Fermer");
    closeBtn->setFixedHeight(36);
    closeBtn->setStyleSheet(
        "QPushButton{ background:#0A5F58; color:white; border-radius:8px;"
        "font-weight:700; font-size:13px; padding:0 20px; }"
        "QPushButton:hover{ background:#12443B; }");
    QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);

    QHBoxLayout* bl = new QHBoxLayout;
    bl->addStretch(1);
    bl->addWidget(closeBtn);
    mainL->addLayout(bl);

    dlg->exec();
}

// ─────────────────────────────────────────────────────────────
//  STATISTIQUE : Distribution des budgets par projet (actifs)
// ─────────────────────────────────────────────────────────────

QMap<QString,double> GestProjCrud::loadBudgetStats(QString* error)
{
    QMap<QString,double> result;

    QSqlQuery q;
    if (!q.exec(
            "SELECT TRIM(\"nom_du_projet\"), \"budget\" "
            "FROM \"projet\" "
            "WHERE LOWER(TRIM(\"statut\")) IN ('en cours', 'planifié', 'en retard', 'critique') "
            "  AND \"budget\" > 0 "
            "ORDER BY \"budget\" DESC"))
    {
        if (error) *error = q.lastError().text();
        return result;
    }

    while (q.next()) {
        QString nom = q.value(0).toString().trimmed();
        if (nom.isEmpty()) nom = "(Sans nom)";
        result[nom] = q.value(1).toDouble();
    }

    return result;
}

// ── Self-contained vertical bar-chart dialog for budgets ─────
void GestProjCrud::showBudgetChart(QWidget* parent)
{
    // ── 1. Fetch data ─────────────────────────────────────────
    GestProjCrud crud;
    QString err;
    QMap<QString,double> raw = crud.loadBudgetStats(&err);

    // ── 2. Build dialog shell ─────────────────────────────────
    QDialog* dlg = new QDialog(parent,
                               Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dlg->setWindowTitle("Distribution des budgets par projet");
    dlg->setMinimumSize(780, 520);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setStyleSheet("QDialog{ background: #EAF3F5; }");

    QVBoxLayout* mainL = new QVBoxLayout(dlg);
    mainL->setContentsMargins(24, 20, 24, 16);
    mainL->setSpacing(14);

    // Title label
    QLabel* title = new QLabel("  Distribution des budgets alloués aux projets actifs");
    title->setStyleSheet(
        "color: #2A649B; font-size: 15px; font-weight: 900;"
        "background: rgba(42,100,155,0.10); border-radius: 10px; padding: 8px 16px;");
    mainL->addWidget(title);

    // ── 3. No-data guard ──────────────────────────────────────
    if (raw.isEmpty()) {
        QLabel* noData = new QLabel(err.isEmpty()
                                    ? "Aucun projet actif avec budget trouvé."
                                    : "Erreur de chargement : " + err);
        noData->setAlignment(Qt::AlignCenter);
        noData->setStyleSheet("color: #8B2F3C; font-size: 13px; font-weight: 600;");
        mainL->addWidget(noData, 1);

        QPushButton* closeBtn = new QPushButton("Fermer");
        closeBtn->setFixedHeight(36);
        closeBtn->setStyleSheet(
            "QPushButton{ background:#2A649B; color:white; border-radius:8px;"
            "font-weight:700; font-size:13px; padding:0 20px; }"
            "QPushButton:hover{ background:#1A4470; }");
        QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);
        QHBoxLayout* bl = new QHBoxLayout;
        bl->addStretch(1);
        bl->addWidget(closeBtn);
        mainL->addLayout(bl);
        dlg->exec();
        return;
    }

    // ── 4. Sort by budget desc ────────────────────────────────
    QList<QPair<QString,double>> sorted;
    for (auto it = raw.begin(); it != raw.end(); ++it)
        sorted.append({it.key(), it.value()});
    std::sort(sorted.begin(), sorted.end(),
              [](const QPair<QString,double>& a, const QPair<QString,double>& b){
                  return a.second > b.second;
              });

    const double maxVal = sorted.isEmpty() ? 1.0 : sorted.first().second;

    // ── 5. Colour palette ─────────────────────────────────────
    static const QColor palette[] = {
        QColor("#2A649B"), QColor("#1A7BAF"), QColor("#0A5F58"),
        QColor("#7B4D9E"), QColor("#B5672C"), QColor("#2E8B7C"),
        QColor("#8B2F3C"), QColor("#416E66"), QColor("#4CAF82"),
        QColor("#5C6BC0")
    };
    static const int palSize = (int)(sizeof(palette)/sizeof(palette[0]));

    // ── 6. Chart widget (horizontal bars with budget labels) ──
    QWidget* chart = new QWidget;
    chart->setMinimumHeight(sorted.size() * 56 + 40);
    chart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    chart->setStyleSheet("background: transparent;");

    struct BudgetPaintFilter : public QObject {
        QList<QPair<QString,double>> items;
        double maxValue;
        double totalValue;
        const QColor* pal;
        int palSz;

        BudgetPaintFilter(QList<QPair<QString,double>> it, double mv, double tv,
                          const QColor* p, int ps, QObject* parent)
            : QObject(parent), items(it), maxValue(mv), totalValue(tv), pal(p), palSz(ps) {}

        bool eventFilter(QObject* obj, QEvent* ev) override {
            if (ev->type() != QEvent::Paint) return false;
            QWidget* w = static_cast<QWidget*>(obj);
            QPainter painter(w);
            painter.setRenderHint(QPainter::Antialiasing);

            const int W        = w->width();
            const int barH     = 34;
            const int rowH     = 56;
            const int labelW   = 200;
            const int valW     = 140;
            const int padTop   = 18;
            const int padLeft  = 16;
            const int barAreaW = qMax(W - labelW - valW - padLeft * 2, 60);

            painter.fillRect(w->rect(), QColor("#EAF3F5"));

            for (int i = 0; i < items.size(); ++i) {
                const QString& nom = items[i].first;
                const double   bud = items[i].second;
                const int      y   = padTop + i * rowH;

                // Alternating row bg
                QColor rowBg = (i % 2 == 0)
                                   ? QColor(255,255,255,110)
                                   : QColor(190,215,235,70);
                painter.fillRect(QRect(0, y - 4, W, rowH - 4), rowBg);

                // Project name label
                QFont lf("Segoe UI", 10, QFont::DemiBold);
                painter.setFont(lf);
                painter.setPen(QColor("#12443B"));
                QRect labelRect(padLeft, y, labelW - 8, barH);
                QString elided = QFontMetrics(lf).elidedText(nom, Qt::ElideRight, labelW - 12);
                painter.drawText(labelRect, Qt::AlignVCenter | Qt::AlignRight, elided);

                // Bar width proportional to budget
                int barW = (maxValue > 0.0)
                               ? (int)(bud / maxValue * barAreaW)
                               : 0;
                if (barW < 4 && bud > 0.0) barW = 4;

                const QColor& col = pal[i % palSz];
                QRect barRect(padLeft + labelW, y + (barH - 24) / 2, barW, 24);
                QPainterPath path;
                path.addRoundedRect(barRect, 6, 6);
                painter.fillPath(path, col);

                // Highlight gradient
                QLinearGradient grad(barRect.topLeft(), barRect.bottomLeft());
                grad.setColorAt(0, QColor(255,255,255,55));
                grad.setColorAt(1, QColor(0,0,0,18));
                painter.fillPath(path, grad);

                // Budget value text
                QFont nf("Segoe UI", 10, QFont::Bold);
                painter.setFont(nf);
                painter.setPen(QColor("#2A649B"));

                double pct = totalValue > 0.0 ? bud / totalValue * 100.0 : 0.0;
                QString budStr;
                if (bud >= 1000000.0)
                    budStr = QString::number(bud / 1000000.0, 'f', 2) + " M TND";
                else if (bud >= 1000.0)
                    budStr = QString::number(bud / 1000.0, 'f', 1) + " k TND";
                else
                    budStr = QString::number(bud, 'f', 2) + " TND";
                budStr = QString("%1 (%2%)").arg(budStr).arg(QString::number(pct, 'f', 1));

                QRect numRect(padLeft + labelW + barW + 8, y, valW, barH);
                painter.drawText(numRect, Qt::AlignVCenter | Qt::AlignLeft, budStr);
            }
            return true;
        }
    };

    double totalBudget = 0.0;
    for (const auto& kv : sorted) totalBudget += kv.second;
    chart->installEventFilter(
        new BudgetPaintFilter(sorted, maxVal, totalBudget, palette, palSize, chart));

    // ── 7. Scroll area ────────────────────────────────────────
    QScrollArea* scroll = new QScrollArea;
    scroll->setWidget(chart);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(
        "QScrollArea{ background:#EAF3F5; border:1px solid rgba(0,0,0,0.10);"
        "border-radius:12px; }"
        "QScrollBar:vertical{ width:8px; background:transparent; }"
        "QScrollBar::handle:vertical{ background:rgba(42,100,155,0.35); border-radius:4px; }");
    mainL->addWidget(scroll, 1);

    // ── 8. Summary ────────────────────────────────────────────
    QString totalStr;
    if (totalBudget >= 1000000.0)
        totalStr = QString::number(totalBudget / 1000000.0, 'f', 2) + " M TND";
    else if (totalBudget >= 1000.0)
        totalStr = QString::number(totalBudget / 1000.0, 'f', 1) + " k TND";
    else
        totalStr = QString::number(totalBudget, 'f', 2) + " TND";

    QLabel* summary = new QLabel(
        QString("  %1 projet(s) actif(s)   ·   Budget total : %2")
            .arg(sorted.size()).arg(totalStr));
    summary->setStyleSheet(
        "color:#2A649B; font-size:11px; font-weight:600;"
        "background:rgba(42,100,155,0.08); border-radius:8px; padding:5px 12px;");
    mainL->addWidget(summary);

    // ── 9. Close button ───────────────────────────────────────
    QPushButton* closeBtn = new QPushButton("Fermer");
    closeBtn->setFixedHeight(36);
    closeBtn->setStyleSheet(
        "QPushButton{ background:#2A649B; color:white; border-radius:8px;"
        "font-weight:700; font-size:13px; padding:0 20px; }"
        "QPushButton:hover{ background:#1A4470; }");
    QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);

    QHBoxLayout* bl = new QHBoxLayout;
    bl->addStretch(1);
    bl->addWidget(closeBtn);
    mainL->addLayout(bl);

    dlg->exec();
}

// ─────────────────────────────────────────────────────────────
//  STATISTIQUE : Distribution du budget par domaine de recherche
// ─────────────────────────────────────────────────────────────

QMap<QString,double> GestProjCrud::loadDomaineBudgetStats(QString* error)
{
    QMap<QString,double> result;

    QSqlQuery q;
    if (!q.exec(
            "SELECT COALESCE(TRIM(\"domaine_de_recherche\"), '(Non défini)'), "
            "SUM(\"budget\") "
            "FROM \"projet\" "
            "WHERE \"budget\" > 0 "
            "GROUP BY TRIM(\"domaine_de_recherche\") "
            "ORDER BY SUM(\"budget\") DESC"))
    {
        if (error) *error = q.lastError().text();
        return result;
    }

    while (q.next()) {
        QString domaine = q.value(0).toString().trimmed();
        if (domaine.isEmpty()) domaine = "(Non défini)";
        result[domaine] = q.value(1).toDouble();
    }

    return result;
}

// ── Self-contained horizontal bar-chart dialog for budget per domaine ──
void GestProjCrud::showDomaineBudgetChart(QWidget* parent)
{
    // ── 1. Fetch data ─────────────────────────────────────────
    GestProjCrud crud;
    QString err;
    QMap<QString,double> raw = crud.loadDomaineBudgetStats(&err);

    // ── 2. Build dialog shell ─────────────────────────────────
    QDialog* dlg = new QDialog(parent,
                               Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dlg->setWindowTitle("Distribution du budget par domaine de recherche");
    dlg->setMinimumSize(780, 480);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setStyleSheet("QDialog{ background: #EAF3F5; }");

    QVBoxLayout* mainL = new QVBoxLayout(dlg);
    mainL->setContentsMargins(24, 20, 24, 16);
    mainL->setSpacing(14);

    // Title label
    QLabel* title = new QLabel("  Distribution du budget par domaine de recherche");
    title->setStyleSheet(
        "color: #B5672C; font-size: 15px; font-weight: 900;"
        "background: rgba(181,103,44,0.10); border-radius: 10px; padding: 8px 16px;");
    mainL->addWidget(title);

    // ── 3. No-data guard ──────────────────────────────────────
    if (raw.isEmpty()) {
        QLabel* noData = new QLabel(err.isEmpty()
                                    ? "Aucun projet avec budget trouvé dans la base de données."
                                    : "Erreur de chargement : " + err);
        noData->setAlignment(Qt::AlignCenter);
        noData->setStyleSheet("color: #8B2F3C; font-size: 13px; font-weight: 600;");
        mainL->addWidget(noData, 1);

        QPushButton* closeBtn = new QPushButton("Fermer");
        closeBtn->setFixedHeight(36);
        closeBtn->setStyleSheet(
            "QPushButton{ background:#B5672C; color:white; border-radius:8px;"
            "font-weight:700; font-size:13px; padding:0 20px; }"
            "QPushButton:hover{ background:#8A4A18; }");
        QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);
        QHBoxLayout* bl = new QHBoxLayout;
        bl->addStretch(1);
        bl->addWidget(closeBtn);
        mainL->addLayout(bl);
        dlg->exec();
        return;
    }

    // ── 4. Sort by budget desc ────────────────────────────────
    QList<QPair<QString,double>> sorted;
    for (auto it = raw.begin(); it != raw.end(); ++it)
        sorted.append({it.key(), it.value()});
    std::sort(sorted.begin(), sorted.end(),
              [](const QPair<QString,double>& a, const QPair<QString,double>& b){
                  return a.second > b.second;
              });

    const double maxVal = sorted.isEmpty() ? 1.0 : sorted.first().second;

    // ── 5. Colour palette ─────────────────────────────────────
    static const QColor palette[] = {
        QColor("#B5672C"), QColor("#D4762A"), QColor("#0A5F58"),
        QColor("#2E8B7C"), QColor("#7B4D9E"), QColor("#2A649B"),
        QColor("#8B2F3C"), QColor("#416E66"), QColor("#4CAF82"),
        QColor("#5C6BC0")
    };
    static const int palSize = (int)(sizeof(palette)/sizeof(palette[0]));

    // ── 6. Chart widget ───────────────────────────────────────
    QWidget* chart = new QWidget;
    chart->setMinimumHeight(sorted.size() * 56 + 40);
    chart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    chart->setStyleSheet("background: transparent;");

    struct DomBudgetPF : public QObject {
        QList<QPair<QString,double>> items;
        double maxValue;
        double totalValue;
        const QColor* pal;
        int palSz;

        DomBudgetPF(QList<QPair<QString,double>> it, double mv, double tv,
                    const QColor* p, int ps, QObject* par)
            : QObject(par), items(it), maxValue(mv), totalValue(tv), pal(p), palSz(ps) {}

        bool eventFilter(QObject* obj, QEvent* ev) override {
            if (ev->type() != QEvent::Paint) return false;
            QWidget* w = static_cast<QWidget*>(obj);
            QPainter painter(w);
            painter.setRenderHint(QPainter::Antialiasing);

            const int W        = w->width();
            const int barH     = 34;
            const int rowH     = 56;
            const int labelW   = 200;
            const int valW     = 150;
            const int padTop   = 18;
            const int padLeft  = 16;
            const int barAreaW = qMax(W - labelW - valW - padLeft * 2, 60);

            painter.fillRect(w->rect(), QColor("#EAF3F5"));

            for (int i = 0; i < items.size(); ++i) {
                const QString& dom = items[i].first;
                const double   bud = items[i].second;
                const int      y   = padTop + i * rowH;

                // Alternating row bg
                QColor rowBg = (i % 2 == 0)
                                   ? QColor(255,255,255,110)
                                   : QColor(220,195,175,60);
                painter.fillRect(QRect(0, y - 4, W, rowH - 4), rowBg);

                // Domain label
                QFont lf("Segoe UI", 10, QFont::DemiBold);
                painter.setFont(lf);
                painter.setPen(QColor("#12443B"));
                QRect labelRect(padLeft, y, labelW - 8, barH);
                QString elided = QFontMetrics(lf).elidedText(dom, Qt::ElideRight, labelW - 12);
                painter.drawText(labelRect, Qt::AlignVCenter | Qt::AlignRight, elided);

                // Bar width proportional to budget
                int barW = (maxValue > 0.0)
                               ? (int)(bud / maxValue * barAreaW)
                               : 0;
                if (barW < 4 && bud > 0.0) barW = 4;

                const QColor& col = pal[i % palSz];
                QRect barRect(padLeft + labelW, y + (barH - 24) / 2, barW, 24);
                QPainterPath path;
                path.addRoundedRect(barRect, 6, 6);
                painter.fillPath(path, col);

                // Highlight gradient
                QLinearGradient grad(barRect.topLeft(), barRect.bottomLeft());
                grad.setColorAt(0, QColor(255,255,255,55));
                grad.setColorAt(1, QColor(0,0,0,18));
                painter.fillPath(path, grad);

                // Budget value text
                QFont nf("Segoe UI", 10, QFont::Bold);
                painter.setFont(nf);
                painter.setPen(QColor("#B5672C"));

                double pct = totalValue > 0.0 ? bud / totalValue * 100.0 : 0.0;
                QString budStr;
                if (bud >= 1000000.0)
                    budStr = QString::number(bud / 1000000.0, 'f', 2) + " M TND";
                else if (bud >= 1000.0)
                    budStr = QString::number(bud / 1000.0, 'f', 1) + " k TND";
                else
                    budStr = QString::number(bud, 'f', 2) + " TND";
                budStr = QString("%1 (%2%)").arg(budStr).arg(QString::number(pct, 'f', 1));

                QRect numRect(padLeft + labelW + barW + 8, y, valW, barH);
                painter.drawText(numRect, Qt::AlignVCenter | Qt::AlignLeft, budStr);
            }
            return true;
        }
    };

    double totalBudget = 0.0;
    for (const auto& kv : sorted) totalBudget += kv.second;
    chart->installEventFilter(
        new DomBudgetPF(sorted, maxVal, totalBudget, palette, palSize, chart));

    // ── 7. Scroll area ────────────────────────────────────────
    QScrollArea* scroll = new QScrollArea;
    scroll->setWidget(chart);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(
        "QScrollArea{ background:#EAF3F5; border:1px solid rgba(0,0,0,0.10);"
        "border-radius:12px; }"
        "QScrollBar:vertical{ width:8px; background:transparent; }"
        "QScrollBar::handle:vertical{ background:rgba(181,103,44,0.35); border-radius:4px; }");
    mainL->addWidget(scroll, 1);

    // ── 8. Summary ────────────────────────────────────────────
    QString totalStr;
    if (totalBudget >= 1000000.0)
        totalStr = QString::number(totalBudget / 1000000.0, 'f', 2) + " M TND";
    else if (totalBudget >= 1000.0)
        totalStr = QString::number(totalBudget / 1000.0, 'f', 1) + " k TND";
    else
        totalStr = QString::number(totalBudget, 'f', 2) + " TND";

    QLabel* summary = new QLabel(
        QString("  %1 domaine(s)   ·   Budget total : %2")
            .arg(sorted.size()).arg(totalStr));
    summary->setStyleSheet(
        "color:#B5672C; font-size:11px; font-weight:600;"
        "background:rgba(181,103,44,0.08); border-radius:8px; padding:5px 12px;");
    mainL->addWidget(summary);

    // ── 9. Close button ───────────────────────────────────────
    QPushButton* closeBtn = new QPushButton("Fermer");
    closeBtn->setFixedHeight(36);
    closeBtn->setStyleSheet(
        "QPushButton{ background:#B5672C; color:white; border-radius:8px;"
        "font-weight:700; font-size:13px; padding:0 20px; }"
        "QPushButton:hover{ background:#8A4A18; }");
    QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);

    QHBoxLayout* bl = new QHBoxLayout;
    bl->addStretch(1);
    bl->addWidget(closeBtn);
    mainL->addLayout(bl);

    dlg->exec();
}

// ─────────────────────────────────────────────────────────────
//  ANALYSE BUDGÉTAIRE COMPLÈTE — 3 onglets
// ─────────────────────────────────────────────────────────────
#include <QTabWidget>
#include <QComboBox>

// File-scope struct so nested local structs (PF3) can reference it without linker issues
struct PrevuReel {
    QString nom;
    double prevu   = 0.0;  // budget saisi manuellement (projet.budget)
    double estime  = 0.0;  // budget estimé via ressources (algorithme)
    double reel    = 0.0;  // budget utilisé à ce jour (dépenses réelles)
    // cost breakdown
    double empCost = 0.0; double pubCost = 0.0; double bioCost = 0.0;
    double equCost = 0.0; double expCost = 0.0; double overhead = 0.0;
};

void GestProjCrud::showAnalyseBudgetaire(QWidget* parent)
{
    GestProjCrud crud;
    QString err;

    // ── Shared data ───────────────────────────────────────────
    QMap<QString,double> budgetByProjet  = crud.loadBudgetStats(&err);
    QMap<QString,double> budgetByDomaine = crud.loadDomaineBudgetStats(&err);

    // ── Budget estimé vs budget utilisé (algorithme complet) ──
    QList<PrevuReel> prevuReelData;
    {
        // Fetch all projects with budget > 0
        QSqlQuery qProj;
        qProj.exec(
            "SELECT \"Id_projet\", TRIM(\"nom_du_projet\"), \"budget\", "
            "\"date_de_début\", \"date_de_fin\" "
            "FROM \"projet\" WHERE \"budget\" > 0 "
            "ORDER BY \"budget\" DESC");

        const QDate today = QDate::currentDate();

        while (qProj.next()) {
            const int    projId   = qProj.value(0).toInt();
            QString      nomProj  = qProj.value(1).toString().trimmed();
            if (nomProj.isEmpty()) nomProj = "(Sans nom)";
            const double budget   = qProj.value(2).toDouble();
            const QDate  dateDeb  = qProj.value(3).toDate();
            const QDate  dateFin  = qProj.value(4).toDate();

            // duration in months
            double durationMonths = 1.0;
            if (dateDeb.isValid() && dateFin.isValid() && dateFin > dateDeb)
                durationMonths = qMax(1.0, dateDeb.daysTo(dateFin) / 30.4375);

            // months elapsed (for budget_used)
            double monthsElapsed = 0.0;
            if (dateDeb.isValid()) {
                monthsElapsed = dateDeb.daysTo(today) / 30.4375;
                if (monthsElapsed < 0.0)          monthsElapsed = 0.0;
                if (monthsElapsed > durationMonths) monthsElapsed = durationMonths;
            }

            // ── STEP 2: Employee cost ──────────────────────────
            // Associer has no role column -> join with Employes to get ROLE
            double empTotal = 0.0;
            {
                QSqlQuery qEmp;
                QString empSql =
                    QString("SELECT NVL(TRIM(emp.\"ROLE\"), 'Technicien') "
                            "FROM \"Associer\" a "
                            "JOIN \"%1\" emp ON a.\"employee_id\" = emp.\"employee_id\" "
                            "WHERE a.\"Id_projet\" = :pid")
                        .arg(QString::fromUtf8("Employ\xc3\xa9s"));
                qEmp.prepare(empSql);
                qEmp.bindValue(":pid", projId);
                if (qEmp.exec()) {
                    while (qEmp.next()) {
                        QString role = qEmp.value(0).toString().trimmed();
                        double monthly = 1400.0;
                        if (role == "Responsable")   monthly = 3500.0;
                        else if (role == "Chercheur") monthly = 2500.0;
                        empTotal += monthly * durationMonths;
                    }
                }
            }

            // ── STEP 3: Publication cost ───────────────────────
            // Publication has no Id_projet FK -> go through Ecrire + Associer
            double pubTotal  = 0.0;
            double pubActual = 0.0;
            {
                QSqlQuery qPub;
                qPub.prepare(
                    "SELECT NVL(TRIM(p.\"status\"), '') "
                    "FROM \"Publication\" p "
                    "JOIN \"Ecrire\" ec ON p.\"id_publication\" = ec.\"id_publication\" "
                    "JOIN \"Associer\" a  ON ec.\"employee_id\" = a.\"employee_id\" "
                    "WHERE a.\"Id_projet\" = :pid");
                qPub.bindValue(":pid", projId);
                if (qPub.exec()) {
                    while (qPub.next()) {
                        QString st = qPub.value(0).toString().trimmed().toLower();
                        double cost = 500.0;
                        if (st.startsWith("accept") || st == "accepted")
                            cost = 2000.0;
                        else if (st.startsWith("publi") || st == "published")
                            cost = 2500.0;
                        pubTotal += cost;
                        if (cost >= 2000.0) pubActual += cost;
                    }
                }
            }

            // ── STEP 4: BioSample cost ─────────────────────────
            // Table: BioSample, columns: "Temperature_de_stockage", "Niveau_de_dangerosite"
            double bioTotal = 0.0;
            {
                QSqlQuery qBio;
                QString bioSql = QString(
                                     "SELECT NVL(TRIM(\"%1\"), ''), NVL(TRIM(\"%2\"), '') "
                                     "FROM \"BioSample\" WHERE \"Id_projet\" = :pid")
                                     .arg(QString::fromUtf8("Temp\xc3\xa9rature_de_stockage"))
                                     .arg(QString::fromUtf8("Niveau_de_dangerosit\xc3\xa9"));
                qBio.prepare(bioSql);
                qBio.bindValue(":pid", projId);
                if (qBio.exec()) {
                    while (qBio.next()) {
                        QString tempStr = qBio.value(0).toString().trimmed();
                        QString dang    = qBio.value(1).toString().trimmed().toUpper();
                        bool ok = false;
                        double tempVal = tempStr.toDouble(&ok);
                        double base = 20.0;
                        if (ok) {
                            if      (tempVal >= 15.0)  base = 5.0;
                            else if (tempVal >= -5.0)  base = 20.0;
                            else if (tempVal >= -40.0) base = 40.0;
                            else if (tempVal >= -90.0) base = 90.0;
                            else                       base = 150.0;
                        }
                        double mult = 1.0;
                        if      (dang == "BSL-2") mult = 1.5;
                        else if (dang == "BSL-3") mult = 2.5;
                        else if (dang == "BSL-4") mult = 4.0;
                        bioTotal += base * durationMonths * mult;
                    }
                }
            }

            // ── STEP 5: Equipment cost (via Experience link) ───
            // Equipement -> Experience -> projet (no Type column, use fixed 300/month)
            double equTotal  = 0.0;
            double equActual = 0.0;
            {
                QSqlQuery qEq;
                QString eqSql = QString(
                                    "SELECT eq.\"%1\", eq.\"%2\", NVL(TRIM(e.\"Status\"), '') "
                                    "FROM \"%3\" eq "
                                    "JOIN \"%4\" e ON eq.\"Id_exp\" = e.\"Id_exp\" "
                                    "WHERE e.\"Id_projet\" = :pid")
                                    .arg(QString::fromUtf8("date_derni\xc3\xa8re_maintenance"))
                                    .arg("date_prochaine_maintenance")
                                    .arg(QString::fromUtf8("\xc3\x89quipement"))
                                    .arg(QString::fromUtf8("Exp\xc3\xa9rience"));
                qEq.prepare(eqSql);
                qEq.bindValue(":pid", projId);
                if (qEq.exec()) {
                    while (qEq.next()) {
                        QDate   dlast = qEq.value(0).toDate();
                        QDate   dnext = qEq.value(1).toDate();
                        QString expSt = qEq.value(2).toString().trimmed().toLower();
                        const double baseCost = 300.0;
                        double maintMult = 1.0;
                        if (dlast.isValid() && dnext.isValid()) {
                            double gap = dlast.daysTo(dnext) / 30.4375;
                            if      (gap < 3.0)         maintMult = 1.20;
                            else if (dnext < today)     maintMult = 1.40;
                        } else if (dnext.isValid() && dnext < today) {
                            maintMult = 1.40;
                        }
                        double cost = baseCost * durationMonths * maintMult;
                        equTotal += cost;
                        if (expSt == "en cours" || expSt.startsWith("r")
                            || expSt.startsWith("termin") || expSt.startsWith("archiv"))
                            equActual += cost;
                    }
                }
            }

            // ── STEP 6: Experience cost ────────────────────────
            // Real statuses: "En cours", "Reussie", "Archivee", "Planifiee"
            double expTotal  = 0.0;
            double expActual = 0.0;
            {
                QSqlQuery qExp;
                QString expSql = QString(
                                     "SELECT NVL(TRIM(\"Status\"), '') "
                                     "FROM \"%1\" WHERE \"Id_projet\" = :pid")
                                     .arg(QString::fromUtf8("Exp\xc3\xa9rience"));
                qExp.prepare(expSql);
                qExp.bindValue(":pid", projId);
                if (qExp.exec()) {
                    while (qExp.next()) {
                        QString st = qExp.value(0).toString().trimmed().toLower();
                        double cost = 425.0;
                        if (st.startsWith("planifi"))
                            cost = 0.0;
                        else if (st.startsWith("r") || st.startsWith("termin")
                                 || st.startsWith("archiv"))
                            cost = 850.0;
                        expTotal += cost;
                        if (cost > 0.0) expActual += cost;
                    }
                }
            }

            // ── STEP 7: Total estimated budget ─────────────────
            const double overheadRate = 0.15;
            double subtotal     = empTotal + pubTotal + bioTotal + equTotal + expTotal;
            double estimated    = subtotal * (1.0 + overheadRate);
            double overheadCost = subtotal * overheadRate;

            // ── STEP 8: Budget used so far ─────────────────
            double empPerMonth = (durationMonths > 0) ? empTotal / durationMonths : 0.0;
            double bioPerMonth = (durationMonths > 0) ? bioTotal / durationMonths : 0.0;
            double empUsed     = empPerMonth * monthsElapsed;
            double bioUsed     = bioPerMonth * monthsElapsed;
            double budgetUsed  = empUsed + pubActual + bioUsed + equActual + expActual;

            PrevuReel pr;
            pr.nom      = nomProj;
            pr.prevu    = budget;
            pr.estime   = estimated;
            pr.reel     = budgetUsed;
            pr.empCost  = empTotal;
            pr.pubCost  = pubTotal;
            pr.bioCost  = bioTotal;
            pr.equCost  = equTotal;
            pr.expCost  = expTotal;
            pr.overhead = overheadCost;
            prevuReelData.append(pr);
        }
    }

    // ── Dialog shell ──────────────────────────────────────────
    QDialog* dlg = new QDialog(parent,
                               Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dlg->setWindowTitle("Analyse budgétaire complète");
    dlg->setMinimumSize(860, 560);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setStyleSheet("QDialog{ background:#EAF3F5; }");

    QVBoxLayout* mainL = new QVBoxLayout(dlg);
    mainL->setContentsMargins(20, 16, 20, 14);
    mainL->setSpacing(12);

    QLabel* hdr = new QLabel("  💰  Analyse budgétaire complète");
    hdr->setStyleSheet(
        "color:#2A649B; font-size:16px; font-weight:900;"
        "background:rgba(42,100,155,0.10); border-radius:10px; padding:9px 18px;");
    mainL->addWidget(hdr);

    // ── Tab widget ────────────────────────────────────────────
    QTabWidget* tabs = new QTabWidget;
    tabs->setStyleSheet(
        "QTabWidget::pane{ border:1px solid rgba(42,100,155,0.20); border-radius:10px;"
        "  background:#EAF3F5; }"
        "QTabBar::tab{ background:rgba(42,100,155,0.10); color:#2A649B;"
        "  font-weight:700; font-size:12px; padding:8px 20px; border-radius:6px 6px 0 0;"
        "  margin-right:4px; }"
        "QTabBar::tab:selected{ background:#2A649B; color:white; }"
        "QTabBar::tab:hover:!selected{ background:rgba(42,100,155,0.20); }");

    // ════════════════════════════════════════════════════════
    //  ONGLET 1 — Vue comparative (bar chart horizontal par projet)
    // ════════════════════════════════════════════════════════
    {
        QWidget* tab1 = new QWidget;
        tab1->setStyleSheet("background:transparent;");
        QVBoxLayout* t1L = new QVBoxLayout(tab1);
        t1L->setContentsMargins(12,12,12,12);
        t1L->setSpacing(10);

        QLabel* sub = new QLabel("  Budget alloué par projet (projets actifs)");
        sub->setStyleSheet(
            "color:#2A649B; font-size:12px; font-weight:800;"
            "background:rgba(42,100,155,0.07); border-radius:7px; padding:5px 10px;");
        t1L->addWidget(sub);

        if (budgetByProjet.isEmpty()) {
            QLabel* nd = new QLabel("Aucun projet actif avec budget trouvé.");
            nd->setAlignment(Qt::AlignCenter);
            nd->setStyleSheet("color:#8B2F3C; font-size:13px; font-weight:600;");
            t1L->addWidget(nd, 1);
        } else {
            // Sort descending
            QList<QPair<QString,double>> sorted1;
            for (auto it = budgetByProjet.begin(); it != budgetByProjet.end(); ++it)
                sorted1.append({it.key(), it.value()});
            std::sort(sorted1.begin(), sorted1.end(),
                      [](const QPair<QString,double>& a, const QPair<QString,double>& b){
                          return a.second > b.second; });
            const double maxV1 = sorted1.first().second;
            double totalBudget1 = 0.0;
            for (const auto& kv : sorted1) totalBudget1 += kv.second;

            static const QColor pal1[] = {
                                          QColor("#2A649B"),QColor("#1A7BAF"),QColor("#0A5F58"),
                                          QColor("#7B4D9E"),QColor("#B5672C"),QColor("#2E8B7C"),
                                          QColor("#8B2F3C"),QColor("#416E66"),QColor("#4CAF82"),QColor("#5C6BC0")};
            static const int pal1Sz = 10;

            QWidget* chart1 = new QWidget;
            chart1->setMinimumHeight(sorted1.size() * 52 + 40);
            chart1->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            chart1->setStyleSheet("background:transparent;");


            struct PF1 : public QObject {
                QList<QPair<QString,double>> items;
                double maxV; double totalValue; const QColor* pal; int palSz;
                PF1(QList<QPair<QString,double>> it, double mv, double tv,
                    const QColor* p, int ps, QObject* par)
                    : QObject(par), items(it), maxV(mv), totalValue(tv), pal(p), palSz(ps) {}
                bool eventFilter(QObject* obj, QEvent* ev) override {
                    if (ev->type() != QEvent::Paint) return false;
                    QWidget* w = static_cast<QWidget*>(obj);
                    QPainter painter(w);
                    painter.setRenderHint(QPainter::Antialiasing);
                    const int W=w->width(), labelW=195, valW=140, padTop=14, padL=14;
                    const int barAreaW = qMax(W-labelW-valW-padL*2, 60);
                    painter.fillRect(w->rect(), QColor("#EAF3F5"));
                    for (int i=0; i<items.size(); ++i) {
                        const int y = padTop + i*52;
                        painter.fillRect(QRect(0,y-3,W,48),
                                         (i%2==0)?QColor(255,255,255,110):QColor(190,215,235,70));
                        QFont lf("Segoe UI",10,QFont::DemiBold);
                        painter.setFont(lf); painter.setPen(QColor("#12443B"));
                        QString el = QFontMetrics(lf).elidedText(items[i].first, Qt::ElideRight, labelW-10);
                        painter.drawText(QRect(padL,y,labelW-8,34), Qt::AlignVCenter|Qt::AlignRight, el);
                        int barW = maxV>0 ? (int)(items[i].second/maxV*barAreaW) : 0;
                        if (barW<4&&items[i].second>0) barW=4;
                        QRect br(padL+labelW, y+5, barW, 24);
                        QPainterPath pp; pp.addRoundedRect(br,6,6);
                        painter.fillPath(pp, pal[i%palSz]);
                        QLinearGradient g(br.topLeft(),br.bottomLeft());
                        g.setColorAt(0,QColor(255,255,255,55)); g.setColorAt(1,QColor(0,0,0,18));
                        painter.fillPath(pp,g);
                        double b=items[i].second;
                        double pct = totalValue > 0.0 ? b / totalValue * 100.0 : 0.0;
                        QString bs = b>=1e6 ? QString::number(b/1e6,'f',2)+" M TND"
                                     : b>=1e3 ? QString::number(b/1e3,'f',1)+" k TND"
                                                : QString::number(b,'f',2)+" TND";
                        bs = QString("%1 (%2%)").arg(bs).arg(QString::number(pct,'f',1));
                        painter.setFont(QFont("Segoe UI",10,QFont::Bold));
                        painter.setPen(QColor("#2A649B"));
                        painter.drawText(QRect(padL+labelW+barW+8,y,valW,34),Qt::AlignVCenter|Qt::AlignLeft,bs);
                    }
                    return true;
                }
            };
            chart1->installEventFilter(new PF1(sorted1, maxV1, totalBudget1, pal1, pal1Sz, chart1));

            QScrollArea* sc1 = new QScrollArea;
            sc1->setWidget(chart1); sc1->setWidgetResizable(true);
            sc1->setFrameShape(QFrame::NoFrame);
            sc1->setStyleSheet(
                "QScrollArea{background:#EAF3F5;border:1px solid rgba(0,0,0,0.10);border-radius:10px;}"
                "QScrollBar:vertical{width:7px;background:transparent;}"
                "QScrollBar::handle:vertical{background:rgba(42,100,155,0.30);border-radius:4px;}");
            t1L->addWidget(sc1, 1);

            double tot1 = 0; for (auto& kv:sorted1) tot1+=kv.second;
            QString ts1 = tot1>=1e6 ? QString::number(tot1/1e6,'f',2)+" M TND"
                          : tot1>=1e3 ? QString::number(tot1/1e3,'f',1)+" k TND"
                                        : QString::number(tot1,'f',2)+" TND";
            QLabel* s1 = new QLabel(QString("  %1 projet(s)   ·   Budget total : %2")
                                        .arg(sorted1.size()).arg(ts1));
            s1->setStyleSheet("color:#2A649B;font-size:11px;font-weight:600;"
                              "background:rgba(42,100,155,0.08);border-radius:7px;padding:4px 10px;");
            t1L->addWidget(s1);
        }
        tabs->addTab(tab1, "Vue comparative");
    }

    // ════════════════════════════════════════════════════════
    //  ONGLET 2 — Ventilation interne (donut chart par domaine)
    // ════════════════════════════════════════════════════════
    {
        QWidget* tab2 = new QWidget;
        tab2->setStyleSheet("background:transparent;");
        QVBoxLayout* t2L = new QVBoxLayout(tab2);
        t2L->setContentsMargins(12,12,12,12);
        t2L->setSpacing(10);

        QLabel* sub2 = new QLabel("  Répartition du budget par domaine de recherche");
        sub2->setStyleSheet(
            "color:#0A5F58; font-size:12px; font-weight:800;"
            "background:rgba(10,95,88,0.07); border-radius:7px; padding:5px 10px;");
        t2L->addWidget(sub2);

        if (budgetByDomaine.isEmpty()) {
            QLabel* nd = new QLabel("Aucun domaine avec budget trouvé.");
            nd->setAlignment(Qt::AlignCenter);
            nd->setStyleSheet("color:#8B2F3C; font-size:13px; font-weight:600;");
            t2L->addWidget(nd, 1);
        } else {
            QList<QPair<QString,double>> dom;
            for (auto it=budgetByDomaine.begin(); it!=budgetByDomaine.end(); ++it)
                dom.append({it.key(), it.value()});
            std::sort(dom.begin(), dom.end(),
                      [](const QPair<QString,double>& a, const QPair<QString,double>& b){
                          return a.second > b.second; });
            double totDom = 0; for (auto& d:dom) totDom+=d.second;

            static const QColor palD[] = {
                                          QColor("#2A649B"),QColor("#0A5F58"),QColor("#B5672C"),
                                          QColor("#7B4D9E"),QColor("#2E8B7C"),QColor("#8B2F3C"),
                                          QColor("#416E66"),QColor("#4CAF82"),QColor("#5C6BC0"),QColor("#D4762A")};
            static const int palDSz = 10;

            // Donut + legend side by side
            QWidget* donutArea = new QWidget;
            donutArea->setStyleSheet("background:transparent;");
            QHBoxLayout* daL = new QHBoxLayout(donutArea);
            daL->setContentsMargins(0,0,0,0);
            daL->setSpacing(18);

            // Donut canvas
            QWidget* donutCanvas = new QWidget;
            donutCanvas->setFixedSize(280, 280);
            donutCanvas->setStyleSheet("background:transparent;");

            struct DonutPF : public QObject {
                QList<QPair<QString,double>> items;
                double total; const QColor* pal; int palSz;
                DonutPF(QList<QPair<QString,double>> it, double tot,
                        const QColor* p, int ps, QObject* par)
                    : QObject(par), items(it), total(tot), pal(p), palSz(ps) {}
                bool eventFilter(QObject* obj, QEvent* ev) override {
                    if (ev->type() != QEvent::Paint) return false;
                    QWidget* w = static_cast<QWidget*>(obj);
                    QPainter painter(w);
                    painter.setRenderHint(QPainter::Antialiasing);
                    int sz = qMin(w->width(), w->height()) - 20;
                    QRect pieRect((w->width()-sz)/2, (w->height()-sz)/2, sz, sz);
                    double startAngle = 90.0 * 16.0; // 12 o'clock, Qt units (*16)
                    for (int i=0; i<items.size(); ++i) {
                        double frac = total>0 ? items[i].second/total : 0;
                        double spanDeg = frac * 360.0;
                        double span = spanDeg * 16.0;
                        painter.setBrush(pal[i%palSz]);
                        painter.setPen(Qt::NoPen);
                        painter.drawPie(pieRect, (int)startAngle, -(int)span);

                        double midAngle = startAngle/16.0 - spanDeg/2.0;
                        double radius = sz * 0.38;
                        double rad = qDegreesToRadians(midAngle);
                        QPointF center(w->width()/2.0 + radius * std::cos(rad),
                                       w->height()/2.0 - radius * std::sin(rad));
                        QString label = QString("%1%")
                                            .arg(QString::number(frac*100.0, 'f', 1));
                        painter.setFont(QFont("Segoe UI", 9, QFont::Bold));
                        painter.setPen(QColor(255,255,255,230));
                        painter.drawText(QRectF(center.x()-24, center.y()-10, 48, 20),
                                         Qt::AlignCenter, label);
                        startAngle -= span;
                    }
                    // Donut hole
                    int hSz = (int)(sz * 0.52);
                    QRect hole((w->width()-hSz)/2, (w->height()-hSz)/2, hSz, hSz);
                    painter.setBrush(QColor("#EAF3F5"));
                    painter.setPen(Qt::NoPen);
                    painter.drawEllipse(hole);
                    // Centre label
                    painter.setPen(QColor("#2A649B"));
                    painter.setFont(QFont("Segoe UI", 10, QFont::Bold));
                    painter.drawText(hole, Qt::AlignCenter, QString("%1\ndomaines").arg(items.size()));
                    return true;
                }
            };
            donutCanvas->installEventFilter(new DonutPF(dom, totDom, palD, palDSz, donutCanvas));
            daL->addWidget(donutCanvas);

            // Legend
            QWidget* legend = new QWidget;
            legend->setStyleSheet("background:transparent;");
            QVBoxLayout* legL = new QVBoxLayout(legend);
            legL->setContentsMargins(0,4,0,4);
            legL->setSpacing(6);
            for (int i=0; i<dom.size(); ++i) {
                QHBoxLayout* row = new QHBoxLayout;
                row->setSpacing(8);
                QLabel* swatch = new QLabel;
                swatch->setFixedSize(14,14);
                swatch->setStyleSheet(QString("background:%1; border-radius:3px;")
                                          .arg(palD[i%palDSz].name()));
                double pct = totDom>0 ? dom[i].second/totDom*100.0 : 0;
                QString valS = dom[i].second>=1e6
                                   ? QString::number(dom[i].second/1e6,'f',2)+" M TND"
                                   : dom[i].second>=1e3
                                         ? QString::number(dom[i].second/1e3,'f',1)+" k TND"
                                         : QString::number(dom[i].second,'f',2)+" TND";
                QLabel* lbl = new QLabel(
                    QString("<b>%1</b>  %2  <span style='color:#888;'>(%3%)</span>")
                        .arg(dom[i].first).arg(valS).arg(pct,0,'f',1));
                lbl->setStyleSheet("font-size:11px; color:#12443B;");
                row->addWidget(swatch);
                row->addWidget(lbl, 1);
                legL->addLayout(row);
            }
            legL->addStretch(1);
            daL->addWidget(legend, 1);

            QScrollArea* sc2 = new QScrollArea;
            sc2->setWidget(donutArea); sc2->setWidgetResizable(true);
            sc2->setFrameShape(QFrame::NoFrame);
            sc2->setStyleSheet(
                "QScrollArea{background:transparent;border:none;}"
                "QScrollBar:vertical{width:7px;background:transparent;}"
                "QScrollBar::handle:vertical{background:rgba(10,95,88,0.30);border-radius:4px;}");
            t2L->addWidget(sc2, 1);

            QString ts2 = totDom>=1e6 ? QString::number(totDom/1e6,'f',2)+" M TND"
                          : totDom>=1e3 ? QString::number(totDom/1e3,'f',1)+" k TND"
                                          : QString::number(totDom,'f',2)+" TND";
            QLabel* s2 = new QLabel(QString("  %1 domaine(s)   ·   Budget total : %2")
                                        .arg(dom.size()).arg(ts2));
            s2->setStyleSheet("color:#0A5F58;font-size:11px;font-weight:600;"
                              "background:rgba(10,95,88,0.08);border-radius:7px;padding:4px 10px;");
            t2L->addWidget(s2);
        }
        tabs->addTab(tab2, "Ventilation interne");
    }

    // ════════════════════════════════════════════════════════
    //  ONGLET 3 — Prévu vs Réel (card per project)
    // ════════════════════════════════════════════════════════
    {
        QWidget* tab3 = new QWidget;
        tab3->setStyleSheet("background:transparent;");
        QVBoxLayout* t3L = new QVBoxLayout(tab3);
        t3L->setContentsMargins(12,12,12,12);
        t3L->setSpacing(10);

        QLabel* sub3 = new QLabel("  Budget estimé vs budget saisi vs budget utilisé — par projet");
        sub3->setStyleSheet(
            "color:#7B4D9E; font-size:12px; font-weight:800;"
            "background:rgba(123,77,158,0.07); border-radius:7px; padding:5px 10px;");
        t3L->addWidget(sub3);

        if (prevuReelData.isEmpty()) {
            QLabel* nd = new QLabel("Aucun projet avec budget trouvé.");
            nd->setAlignment(Qt::AlignCenter);
            nd->setStyleSheet("color:#8B2F3C; font-size:13px; font-weight:600;");
            t3L->addWidget(nd, 1);
        } else {
            // ── Formatter helper (used in lambda captures below) ──
            // Defined as a static free function-style lambda stored in a local var
            // so it can be captured by the paint event filter struct.
            // We actually embed it directly in the paint struct below.

            // ── Scroll area containing one card per project ───────
            QScrollArea* sc3 = new QScrollArea;
            sc3->setWidgetResizable(true);
            sc3->setFrameShape(QFrame::NoFrame);
            sc3->setStyleSheet(
                "QScrollArea{background:transparent;border:none;}"
                "QScrollBar:vertical{width:7px;background:transparent;}"
                "QScrollBar::handle:vertical{background:rgba(123,77,158,0.30);border-radius:4px;}");

            QWidget* cardsW = new QWidget;
            cardsW->setStyleSheet("background:transparent;");
            QVBoxLayout* cardsL = new QVBoxLayout(cardsW);
            cardsL->setContentsMargins(4,4,4,4);
            cardsL->setSpacing(14);

            // ── Format helper ──────────────────────────────────────
            auto fmt = [](double v) -> QString {
                if (v >= 1e6) return QString::number(v/1e6,'f',2) + " M TND";
                if (v >= 1e3) return QString::number(v/1e3,'f',1) + " k TND";
                return QString::number(v,'f',0) + " TND";
            };
            for (const PrevuReel& pr : prevuReelData) {
                // ── Card frame ─────────────────────────────────────
                QFrame* card = new QFrame;
                card->setStyleSheet(
                    "QFrame{"
                    "  background:white;"
                    "  border:1px solid rgba(42,100,155,0.18);"
                    "  border-left:5px solid #2A649B;"
                    "  border-radius:10px;"
                    "}");
                QVBoxLayout* cL = new QVBoxLayout(card);
                cL->setContentsMargins(16,14,16,14);
                cL->setSpacing(8);

                // ── Project name header ────────────────────────────
                QLabel* projName = new QLabel(pr.nom);
                projName->setStyleSheet(
                    "font-size:13px; font-weight:900; color:#1A3A5C;"
                    "background:transparent; border:none;");
                cL->addWidget(projName);

                // ── Separator ─────────────────────────────────────
                QFrame* sep = new QFrame;
                sep->setFrameShape(QFrame::HLine);
                sep->setStyleSheet("color:rgba(42,100,155,0.15); background:rgba(42,100,155,0.15);"
                                   "border:none; max-height:1px;");
                cL->addWidget(sep);

                // ── Top row: 3 key numbers ─────────────────────────
                double remaining = pr.estime - pr.reel;
                double pctUsed   = pr.estime > 0 ? pr.reel / pr.estime * 100.0 : 0.0;
                bool   over      = pr.reel > pr.estime || pr.reel > pr.prevu;

                QWidget* topRow = new QWidget;
                topRow->setStyleSheet("background:transparent;");
                QHBoxLayout* topL = new QHBoxLayout(topRow);
                topL->setContentsMargins(0,0,0,0); topL->setSpacing(10);

                // Helper: make a summary pill
                auto makePill = [](const QString& title, const QString& value,
                                   const QString& bgColor, const QString& fgColor) -> QWidget* {
                    QFrame* pill = new QFrame;
                    pill->setStyleSheet(QString(
                                            "QFrame{ background:%1; border-radius:8px; border:none; }").arg(bgColor));
                    QVBoxLayout* pl = new QVBoxLayout(pill);
                    pl->setContentsMargins(12,8,12,8); pl->setSpacing(2);
                    QLabel* ttl = new QLabel(title);
                    ttl->setStyleSheet(QString("font-size:9px; font-weight:700; color:%1;"
                                               "background:transparent;").arg(fgColor));
                    ttl->setAlignment(Qt::AlignCenter);
                    QLabel* val = new QLabel(value);
                    val->setStyleSheet(QString("font-size:12px; font-weight:900; color:%1;"
                                               "background:transparent;").arg(fgColor));
                    val->setAlignment(Qt::AlignCenter);
                    pl->addWidget(ttl); pl->addWidget(val);
                    return pill;
                };

                topL->addWidget(makePill("Budget estimé (algo)",
                                         fmt(pr.estime),
                                         "rgba(42,100,155,0.10)", "#1A3A5C"), 1);
                topL->addWidget(makePill("Budget saisi manuellement",
                                         fmt(pr.prevu),
                                         "rgba(100,100,100,0.08)", "#444"), 1);
                topL->addWidget(makePill("Budget utilisé à ce jour",
                                         fmt(pr.reel),
                                         over ? "rgba(139,47,60,0.12)" : "rgba(46,139,124,0.12)",
                                         over ? "#8B2F3C" : "#1A6B60"), 1);
                topL->addWidget(makePill("Reste disponible",
                                         fmt(qMax(0.0, remaining)),
                                         "rgba(65,110,102,0.10)", "#2E6042"), 1);
                topL->addWidget(makePill("Avancement",
                                         QString::number(pctUsed,'f',1) + "%",
                                         over ? "rgba(139,47,60,0.12)" : "rgba(42,100,155,0.10)",
                                         over ? "#8B2F3C" : "#2A649B"), 1);
                cL->addWidget(topRow);

                // ── Progress bar ───────────────────────────────────
                double barPct = qMin(pctUsed / 100.0, 1.0);
                QWidget* progBg = new QWidget;
                progBg->setFixedHeight(8);
                progBg->setStyleSheet("background:rgba(0,0,0,0.08); border-radius:4px;");
                QWidget* progFill = new QWidget(progBg);
                progFill->setFixedHeight(8);
                progFill->setStyleSheet(QString("background:%1; border-radius:4px;")
                                            .arg(over ? "#8B2F3C" : (pctUsed > 75 ? "#D4762A" : "#2E8B7C")));
                // Set width after shown via timer
                QObject::connect(progBg, &QWidget::destroyed, progBg, []{});
                double capturedPct = barPct;
                QWidget* capturedFill = progFill;
                QWidget* capturedBg   = progBg;
                QTimer::singleShot(0, progBg, [capturedFill, capturedBg, capturedPct](){
                    capturedFill->setFixedWidth(qMax(0, (int)(capturedBg->width() * capturedPct)));
                });
                cL->addWidget(progBg);

                // ── Répartition section ────────────────────────────
                QLabel* repartLabel = new QLabel("  Répartition :");
                repartLabel->setStyleSheet(
                    "font-size:10px; font-weight:800; color:#7B4D9E; background:transparent;");
                cL->addWidget(repartLabel);

                double subtotal = pr.empCost + pr.expCost + pr.bioCost
                                  + pr.equCost + pr.pubCost + pr.overhead;

                struct RepartItem { QString icon; QString label; double value; QString color; };
                QList<RepartItem> items = {
                                           { "👥", "Employés",      pr.empCost,  "#2A649B" },
                                           { "🔬", "Expériences",   pr.expCost,  "#0A5F58" },
                                           { "🧪", "BioSamples",    pr.bioCost,  "#2E8B7C" },
                                           { "🔧", "Équipements",   pr.equCost,  "#B5672C" },
                                           { "📄", "Publications",  pr.pubCost,  "#7B4D9E" },
                                           { "⚙️", "Overhead (15%)",pr.overhead, "#8C9EAD" },
                                           };

                QWidget* repartGrid = new QWidget;
                repartGrid->setStyleSheet("background:transparent;");
                QGridLayout* gL = new QGridLayout(repartGrid);
                gL->setContentsMargins(0,0,0,0); gL->setSpacing(4);

                for (int ri = 0; ri < items.size(); ++ri) {
                    const RepartItem& it = items[ri];
                    int col = (ri % 2) * 3; // 2 columns, 3 sub-cols each
                    int row = ri / 2;

                    QLabel* iconLbl = new QLabel(it.icon + "  " + it.label);
                    iconLbl->setStyleSheet(QString(
                                               "font-size:11px; font-weight:700; color:%1; background:transparent;")
                                               .arg(it.color));

                    double pct = subtotal > 0 ? it.value / subtotal * 100.0 : 0.0;
                    QLabel* valLbl = new QLabel(fmt(it.value));
                    valLbl->setStyleSheet(
                        "font-size:11px; font-weight:900; color:#1A3A5C; background:transparent;");
                    valLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

                    QLabel* pctLbl = new QLabel(QString("(%1%)").arg(QString::number(pct,'f',0)));
                    pctLbl->setStyleSheet(
                        "font-size:10px; color:#888; background:transparent;");
                    pctLbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

                    gL->addWidget(iconLbl, row, col,     Qt::AlignVCenter);
                    gL->addWidget(valLbl,  row, col + 1, Qt::AlignVCenter);
                    gL->addWidget(pctLbl,  row, col + 2, Qt::AlignVCenter);
                }
                gL->setColumnStretch(1, 1); gL->setColumnStretch(4, 1);
                gL->setColumnMinimumWidth(2, 16); // gap between the two columns
                cL->addWidget(repartGrid);

                cardsL->addWidget(card);
            }

            cardsL->addStretch(1);
            sc3->setWidget(cardsW);
            t3L->addWidget(sc3, 1);

            // ── Summary bar ────────────────────────────────────────
            double totalEstime=0, totalSaisi=0, totalUtilise=0;
            int overCount=0;
            for (const auto& pr : prevuReelData) {
                totalEstime  += pr.estime;
                totalSaisi   += pr.prevu;
                totalUtilise += pr.reel;
                if (pr.reel > pr.estime || pr.reel > pr.prevu) overCount++;
            }
            double globalPct = totalEstime > 0 ? totalUtilise/totalEstime*100.0 : 0.0;
            QLabel* s3 = new QLabel(
                QString("  %1 projet(s)   ·   Estimé total : %2   ·   Saisi total : %3   "
                        "·   Utilisé : %4 (%5%)   ·   %6 dépassement(s)")
                    .arg(prevuReelData.size())
                    .arg(fmt(totalEstime))
                    .arg(fmt(totalSaisi))
                    .arg(fmt(totalUtilise))
                    .arg(QString::number(globalPct,'f',1))
                    .arg(overCount));
            s3->setStyleSheet("color:#7B4D9E;font-size:11px;font-weight:600;"
                              "background:rgba(123,77,158,0.08);border-radius:7px;padding:4px 10px;");
            t3L->addWidget(s3);
        }
        tabs->addTab(tab3, "Prévu vs Réel");
    }

    mainL->addWidget(tabs, 1);

    // ── Close button ──────────────────────────────────────────
    QPushButton* closeBtn = new QPushButton("Fermer");
    closeBtn->setFixedHeight(36);
    closeBtn->setStyleSheet(
        "QPushButton{ background:#2A649B; color:white; border-radius:8px;"
        "font-weight:700; font-size:13px; padding:0 20px; }"
        "QPushButton:hover{ background:#1A4470; }");
    QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);
    QHBoxLayout* bl = new QHBoxLayout;
    bl->addStretch(1); bl->addWidget(closeBtn);
    mainL->addLayout(bl);

    dlg->exec();
}

// ─────────────────────────────────────────────────────────────
//  STATISTIQUE : Répartition des projets par statut (Pie Chart)
// ─────────────────────────────────────────────────────────────

QMap<QString,int> GestProjCrud::loadStatutStats(QString* error)
{
    QMap<QString,int> result;

    QSqlQuery q;
    if (!q.exec(
            "SELECT COALESCE(TRIM(\"statut\"), '(Non défini)'), COUNT(*) "
            "FROM \"projet\" "
            "GROUP BY TRIM(\"statut\") "
            "ORDER BY COUNT(*) DESC"))
    {
        if (error) *error = q.lastError().text();
        return result;
    }

    while (q.next()) {
        QString statut = q.value(0).toString().trimmed();
        if (statut.isEmpty()) statut = "(Non défini)";
        result[statut] = q.value(1).toInt();
    }

    return result;
}

// ── Self-contained Pie Chart dialog for statut distribution ──
void GestProjCrud::showStatutChart(QWidget* parent)
{
    // ── 1. Fetch data ─────────────────────────────────────────
    GestProjCrud crud;
    QString err;
    QMap<QString,int> raw = crud.loadStatutStats(&err);

    // ── 2. Build dialog shell ─────────────────────────────────
    QDialog* dlg = new QDialog(parent,
                               Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dlg->setWindowTitle("Répartition des projets par statut");
    dlg->setMinimumSize(700, 520);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setStyleSheet("QDialog{ background: #EAF3F5; }");

    QVBoxLayout* mainL = new QVBoxLayout(dlg);
    mainL->setContentsMargins(24, 20, 24, 16);
    mainL->setSpacing(14);

    QLabel* titleLbl = new QLabel("  Répartition des projets par statut");
    titleLbl->setStyleSheet(
        "color: #8B2F3C; font-size: 15px; font-weight: 900;"
        "background: rgba(139,47,60,0.08); border-radius: 10px; padding: 8px 16px;");
    mainL->addWidget(titleLbl);

    // ── 3. No-data guard ──────────────────────────────────────
    if (raw.isEmpty()) {
        QLabel* noData = new QLabel(err.isEmpty()
                                    ? "Aucun projet trouvé dans la base de données."
                                    : "Erreur de chargement : " + err);
        noData->setAlignment(Qt::AlignCenter);
        noData->setStyleSheet("color: #8B2F3C; font-size: 13px; font-weight: 600;");
        mainL->addWidget(noData, 1);
        QPushButton* cb = new QPushButton("Fermer");
        cb->setFixedHeight(36);
        cb->setStyleSheet(
            "QPushButton{ background:#8B2F3C; color:white; border-radius:8px;"
            "font-weight:700; font-size:13px; padding:0 20px; }"
            "QPushButton:hover{ background:#6A1E29; }");
        QObject::connect(cb, &QPushButton::clicked, dlg, &QDialog::accept);
        QHBoxLayout* blt = new QHBoxLayout;
        blt->addStretch(1); blt->addWidget(cb);
        mainL->addLayout(blt);
        dlg->exec();
        return;
    }

    // ── 4. Build sorted slice list ────────────────────────────
    QList<QPair<QString,int>> slices;
    int total = 0;
    for (auto it = raw.begin(); it != raw.end(); ++it) {
        slices.append({it.key(), it.value()});
        total += it.value();
    }
    std::sort(slices.begin(), slices.end(),
              [](const QPair<QString,int>& a, const QPair<QString,int>& b){
                  return a.second > b.second;
              });

    // ── 5. Colour palette ─────────────────────────────────────
    // Colors match exactly the projStatusColor() badges in the project list
    struct SColor { const char* s; QColor c; };
    static const SColor smap[] = {
        { "En cours",     QColor("#4a877c") },
        { "Planifié",     QColor("#518195") },
        { "Terminé",      QColor("#236e60") },
        { "Suspendu",     QColor("#7A8B8A") },
        { "En retard",    QColor("#ae7040") },
        { "Critique",     QColor("#8B2F3C") },
        { "Annulé",       QColor("#700833") },
        { "(Non défini)", QColor("#9E9E9E") }
    };
    static const int smapSz = (int)(sizeof(smap)/sizeof(smap[0]));
    static const QColor fallback[] = {
        QColor("#5C6BC0"), QColor("#7B4D9E"), QColor("#4CAF82"),
        QColor("#416E66"), QColor("#D48B2A")
    };
    static const int fbSz = (int)(sizeof(fallback)/sizeof(fallback[0]));

    QList<QColor> colors;
    for (int i = 0; i < slices.size(); ++i) {
        QColor c = fallback[i % fbSz];
        for (int j = 0; j < smapSz; ++j)
            if (slices[i].first == smap[j].s) { c = smap[j].c; break; }
        colors.append(c);
    }

    // ── 6. Layout: pie left, legend right ────────────────────
    QWidget* content = new QWidget;
    QHBoxLayout* contentL = new QHBoxLayout(content);
    contentL->setContentsMargins(0, 0, 0, 0);
    contentL->setSpacing(20);

    // Pie widget
    QWidget* pieW = new QWidget;
    pieW->setMinimumSize(280, 280);
    pieW->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    pieW->setStyleSheet("background: transparent;");

    struct PiePF : public QObject {
        QList<QPair<QString,int>> items;
        QList<QColor> cols;
        int total;
        PiePF(QList<QPair<QString,int>> it, QList<QColor> c, int t, QObject* par)
            : QObject(par), items(it), cols(c), total(t) {}

        bool eventFilter(QObject* obj, QEvent* ev) override {
            if (ev->type() != QEvent::Paint) return false;
            QWidget* w = static_cast<QWidget*>(obj);
            QPainter p(w);
            p.setRenderHint(QPainter::Antialiasing);

            const int sz    = qMin(w->width(), w->height()) - 20;
            const int cx    = w->width()  / 2;
            const int cy    = w->height() / 2;
            const int r     = sz / 2;
            const int holeR = r * 45 / 100;
            QRectF pie(cx - r, cy - r, sz, sz);

            if (total <= 0) return true;
            double start = 90.0;

            for (int i = 0; i < items.size(); ++i) {
                double span = 360.0 * items[i].second / total;
                p.setBrush(cols[i]);
                p.setPen(Qt::NoPen);
                p.drawPie(pie, (int)(start * 16), -(int)(span * 16));

                // % label
                double pct = 100.0 * items[i].second / total;
                double lr = qDegreesToRadians(start - span / 2.0);
                double ld = (r + holeR) / 2.0;
                QPointF lp(cx + ld * std::cos(lr), cy - ld * std::sin(lr));
                QFont lf("Segoe UI", 9, QFont::Bold);
                p.setFont(lf);
                p.setPen(QColor(255,255,255,230));
                p.drawText(QRectF(lp.x()-26, lp.y()-10, 52, 20),
                           Qt::AlignCenter,
                           QString::number(pct,'f',1)+"%");
                start -= span;
            }

            // donut hole
            p.setBrush(QColor("#EAF3F5"));
            p.setPen(Qt::NoPen);
            p.drawEllipse(QPointF(cx, cy), (double)holeR, (double)holeR);

            // centre text
            QFont cf("Segoe UI", 14, QFont::Black);
            p.setFont(cf); p.setPen(QColor("#12443B"));
            p.drawText(QRect(cx-40, cy-18, 80, 20), Qt::AlignCenter,
                       QString::number(total));
            QFont sf("Segoe UI", 8, QFont::DemiBold);
            p.setFont(sf); p.setPen(QColor("#416E66"));
            p.drawText(QRect(cx-40, cy+4, 80, 16), Qt::AlignCenter, "projet(s)");
            return true;
        }
    };

    pieW->installEventFilter(new PiePF(slices, colors, total, pieW));
    contentL->addWidget(pieW, 1);

    // Legend
    QWidget* legend = new QWidget;
    legend->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    legend->setStyleSheet("background: transparent;");
    QVBoxLayout* legendL = new QVBoxLayout(legend);
    legendL->setContentsMargins(8, 8, 8, 8);
    legendL->setSpacing(8);

    for (int i = 0; i < slices.size(); ++i) {
        double pct = (total > 0) ? 100.0 * slices[i].second / total : 0.0;
        QWidget* rowW = new QWidget;
        rowW->setStyleSheet("background: rgba(255,255,255,90); border-radius: 8px;");
        QHBoxLayout* rowL = new QHBoxLayout(rowW);
        rowL->setContentsMargins(10, 6, 10, 6);
        rowL->setSpacing(10);

        QLabel* sw = new QLabel;
        sw->setFixedSize(16, 16);
        sw->setStyleSheet(QString("background:%1; border-radius:4px;"
                                  "border:1px solid rgba(0,0,0,0.15);")
                              .arg(colors[i].name()));
        QLabel* nl = new QLabel(slices[i].first);
        nl->setStyleSheet("color:#12443B; font-size:11px; font-weight:700;"
                          "background:transparent; border:none;");
        nl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        QLabel* vl = new QLabel(
            QString("%1  (%2%)").arg(slices[i].second)
                .arg(QString::number(pct,'f',1)));
        vl->setStyleSheet("color:#416E66; font-size:11px; font-weight:600;"
                          "background:transparent; border:none;");
        vl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        rowL->addWidget(sw); rowL->addWidget(nl, 1); rowL->addWidget(vl);
        legendL->addWidget(rowW);
    }
    legendL->addStretch(1);
    contentL->addWidget(legend, 1);
    mainL->addWidget(content, 1);

    // ── 7. Summary ────────────────────────────────────────────
    QLabel* summary = new QLabel(
        QString("  %1 statut(s) différents   ·   %2 projet(s) au total")
            .arg(slices.size()).arg(total));
    summary->setStyleSheet(
        "color:#8B2F3C; font-size:11px; font-weight:600;"
        "background:rgba(139,47,60,0.06); border-radius:8px; padding:5px 12px;");
    mainL->addWidget(summary);

    // ── 8. Close button ───────────────────────────────────────
    QPushButton* cbFinal = new QPushButton("Fermer");
    cbFinal->setFixedHeight(36);
    cbFinal->setStyleSheet(
        "QPushButton{ background:#8B2F3C; color:white; border-radius:8px;"
        "font-weight:700; font-size:13px; padding:0 20px; }"
        "QPushButton:hover{ background:#6A1E29; }");
    QObject::connect(cbFinal, &QPushButton::clicked, dlg, &QDialog::accept);

    QHBoxLayout* blFinal = new QHBoxLayout;
    blFinal->addStretch(1);
    blFinal->addWidget(cbFinal);
    mainL->addLayout(blFinal);

    dlg->exec();
}

// ─────────────────────────────────────────────────────────────
//  SANTÉ DU PROJET — compute 5-axis health score
// ─────────────────────────────────────────────────────────────
#include <QPolygonF>
#include <QtMath>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QTimer>

// Themed alert helper — mirrors ThemedAlertDialog from integration.cpp
// type: "info" | "warning" | "error"
static void showSanteAlert(QWidget* parent, const QString& type,
                           const QString& title, const QString& message)
{
    QDialog dlg(parent, Qt::Dialog | Qt::FramelessWindowHint);
    dlg.setModal(true);
    dlg.setAttribute(Qt::WA_TranslucentBackground);
    dlg.setFixedSize(520, 230);

    QWidget* card = new QWidget(&dlg);
    card->setGeometry(0, 0, 520, 230);
    card->setObjectName("card");
    card->setStyleSheet(
        "QWidget#card{ background:#F4F9F8; border-radius:16px;"
        " border:1.5px solid #A3CAD3; }");

    QVBoxLayout* root = new QVBoxLayout(card);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    // Header
    QFrame* head = new QFrame;
    head->setFixedHeight(50);
    QString headColor = (type == "error")   ? "qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #8B2F3C,stop:1 #6A1E29)"
                        : (type == "warning") ? "qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #B5672C,stop:1 #8C4E1E)"
                                              : "qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #0A5F58,stop:1 #12443B)";
    head->setStyleSheet(QString("QFrame{ background: %1; border-radius:12px; }").arg(headColor));
    QHBoxLayout* hl = new QHBoxLayout(head);
    hl->setContentsMargins(14, 0, 14, 0);
    hl->setSpacing(10);

    QString iconChar = (type == "warning") ? "\u26a0"
                       : (type == "error")   ? "\u2715"
                                           : "\u2139";
    QLabel* ic = new QLabel(iconChar);
    ic->setStyleSheet("color:#F5C842; font-size:20px; background:transparent; border:none;");
    QLabel* t = new QLabel("  " + title);
    QFont ft = t->font(); ft.setBold(true); ft.setPointSize(11);
    t->setFont(ft);
    t->setStyleSheet("color:white; background:transparent; border:none;");
    hl->addWidget(ic); hl->addWidget(t); hl->addStretch(1);
    root->addWidget(head);

    // Body
    QFrame* body = new QFrame;
    body->setStyleSheet(
        "QFrame{ background:rgba(255,255,255,0.85);"
        " border:1px solid rgba(10,95,88,0.20); border-radius:12px; }");
    QVBoxLayout* bl = new QVBoxLayout(body);
    bl->setContentsMargins(14, 12, 14, 12);
    QLabel* msg = new QLabel(message);
    msg->setStyleSheet("color:#12443B; font-weight:700; background:transparent; border:none;");
    msg->setWordWrap(true);
    bl->addWidget(msg);
    root->addWidget(body, 1);

    // OK button
    QHBoxLayout* btns = new QHBoxLayout;
    btns->addStretch(1);
    QPushButton* ok = new QPushButton("  OK");
    ok->setCursor(Qt::PointingHandCursor);
    ok->setFixedHeight(40);
    QString btnColor = (type == "error")   ? "qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #8B2F3C,stop:1 #6A1E29)"
                       : (type == "warning") ? "qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #B5672C,stop:1 #8C4E1E)"
                                             : "qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #0A5F58,stop:1 #12443B)";
    ok->setStyleSheet(QString(
                          "QPushButton{ background: %1; border:none; border-radius:12px;"
                          " padding:8px 20px; font-weight:700; color:white; }"
                          "QPushButton:hover{ opacity:0.85; }").arg(btnColor));
    QObject::connect(ok, &QPushButton::clicked, &dlg, &QDialog::accept);
    btns->addWidget(ok);
    root->addLayout(btns);

    // Fade-in
    dlg.setWindowOpacity(0.0);
    QPropertyAnimation* anim = new QPropertyAnimation(&dlg, "windowOpacity", &dlg);
    anim->setDuration(220);
    anim->setStartValue(0.0); anim->setEndValue(1.0);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    QTimer::singleShot(0, anim, [anim]{ anim->start(QAbstractAnimation::DeleteWhenStopped); });

    dlg.exec();
}

// Themed multi-line alert for axis warnings
static void showSanteAxesAlert(QWidget* parent, const QString& title, const QStringList& lines)
{
    QDialog dlg(parent, Qt::Dialog | Qt::FramelessWindowHint);
    dlg.setModal(true);
    dlg.setAttribute(Qt::WA_TranslucentBackground);
    int h = 200 + lines.size() * 24;
    dlg.setFixedSize(560, h);

    QWidget* card = new QWidget(&dlg);
    card->setGeometry(0, 0, 560, h);
    card->setObjectName("card");
    card->setStyleSheet(
        "QWidget#card{ background:#F4F9F8; border-radius:16px;"
        " border:1.5px solid #A3CAD3; }");

    QVBoxLayout* root = new QVBoxLayout(card);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    // Header — warning orange
    QFrame* head = new QFrame;
    head->setFixedHeight(50);
    head->setStyleSheet(
        "QFrame{ background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "stop:0 #B5672C, stop:1 #8C4E1E); border-radius:12px; }");
    QHBoxLayout* hl = new QHBoxLayout(head);
    hl->setContentsMargins(14, 0, 14, 0); hl->setSpacing(10);
    QLabel* ic = new QLabel("\u26a0");
    ic->setStyleSheet("color:#F5C842; font-size:20px; background:transparent; border:none;");
    QLabel* t = new QLabel("  " + title);
    QFont ft = t->font(); ft.setBold(true); ft.setPointSize(11);
    t->setFont(ft);
    t->setStyleSheet("color:white; background:transparent; border:none;");
    hl->addWidget(ic); hl->addWidget(t); hl->addStretch(1);
    root->addWidget(head);

    // Body with each line
    QFrame* body = new QFrame;
    body->setStyleSheet(
        "QFrame{ background:rgba(255,255,255,0.85);"
        " border:1px solid rgba(181,103,44,0.25); border-radius:12px; }");
    QVBoxLayout* bl = new QVBoxLayout(body);
    bl->setContentsMargins(14, 12, 14, 12); bl->setSpacing(6);

    QLabel* intro = new QLabel("<b>Un ou plusieurs axes sont sous le seuil critique de 30% :</b>");
    intro->setStyleSheet("color:#8C4E1E; font-size:11px; background:transparent; border:none;");
    intro->setWordWrap(true);
    bl->addWidget(intro);

    for (const QString& line : lines) {
        QLabel* ll = new QLabel(line);
        ll->setStyleSheet(
            "color:#8B2F3C; font-weight:700; font-size:11px;"
            "background:rgba(139,47,60,0.06); border-radius:6px;"
            "padding:3px 8px; border:none;");
        ll->setWordWrap(false);
        bl->addWidget(ll);
    }
    root->addWidget(body, 1);

    // OK button
    QHBoxLayout* btns = new QHBoxLayout;
    btns->addStretch(1);
    QPushButton* ok = new QPushButton("  OK");
    ok->setCursor(Qt::PointingHandCursor);
    ok->setFixedHeight(40);
    ok->setStyleSheet(
        "QPushButton{ background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "stop:0 #B5672C,stop:1 #8C4E1E); border:none; border-radius:12px;"
        " padding:8px 20px; font-weight:700; color:white; }"
        "QPushButton:hover{ opacity:0.85; }");
    QObject::connect(ok, &QPushButton::clicked, &dlg, &QDialog::accept);
    btns->addWidget(ok);
    root->addLayout(btns);

    dlg.setWindowOpacity(0.0);
    QPropertyAnimation* anim = new QPropertyAnimation(&dlg, "windowOpacity", &dlg);
    anim->setDuration(220);
    anim->setStartValue(0.0); anim->setEndValue(1.0);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    QTimer::singleShot(0, anim, [anim]{ anim->start(QAbstractAnimation::DeleteWhenStopped); });

    dlg.exec();
}

GestProjCrud::ProjetSante GestProjCrud::computeProjetSante(int idProjet, QString* error)
{
    ProjetSante s;
    s.idProjet = idProjet;

    // ── Fetch base project record ─────────────────────────────
    ProjetRecord rec;
    if (!fetchProjet(idProjet, rec, error)) {
        s.errorMsg = error ? *error : "Projet introuvable.";
        return s;
    }
    s.nomProjet = rec.nomDuProjet;

    // ─────────────────────────────────────────────────────────
    // AXIS 1 — Avancement budgétaire (20%)
    //   spent = nbExp*PRICE_EXPERIENCE + nbEch*PRICE_ECHANTILLON + nbEq*PRICE_EQUIPEMENT
    //   score = 100 - clamp(spent/budget * 100, 0, 100)
    // ─────────────────────────────────────────────────────────
    {
        double spent = 0.0;

        QSqlQuery qExp;
        qExp.prepare("SELECT COUNT(*) FROM \"Expérience\" WHERE \"Id_projet\" = :id");
        qExp.bindValue(":id", idProjet);
        if (qExp.exec() && qExp.next())
            spent += qExp.value(0).toDouble() * PRICE_EXPERIENCE;

        QSqlQuery qEch;
        qEch.prepare("SELECT COUNT(*) FROM \"BioSample\" WHERE \"Id_projet\" = :id");
        qEch.bindValue(":id", idProjet);
        if (qEch.exec() && qEch.next())
            spent += qEch.value(0).toDouble() * PRICE_ECHANTILLON;

        QSqlQuery qEq;
        qEq.prepare(
            "SELECT COUNT(*) FROM \"Équipement\" eq "
            "INNER JOIN \"Expérience\" ex ON eq.\"Id_exp\" = ex.\"Id_exp\" "
            "WHERE ex.\"Id_projet\" = :id");
        qEq.bindValue(":id", idProjet);
        if (qEq.exec() && qEq.next())
            spent += qEq.value(0).toDouble() * PRICE_EQUIPEMENT;

        if (rec.budget <= 0.0) {
            s.scorebudget = 50.0; // No budget defined — neutral score
        } else {
            double ratio = spent / rec.budget;
            s.scorebudget = std::max(0.0, std::min(100.0, (1.0 - ratio) * 100.0));
        }
    }

    // ─────────────────────────────────────────────────────────
    // AXIS 2 — Respect des délais (20%)
    // ─────────────────────────────────────────────────────────
    {
        const QDate today = QDate::currentDate();
        if (!rec.dateDeDebut.isValid()) {
            s.scoreDelais = 50.0;
        } else if (!rec.dateDeFin.isValid()) {
            s.scoreDelais = 70.0;
        } else {
            const int totalDays   = rec.dateDeDebut.daysTo(rec.dateDeFin);
            const int elapsedDays = rec.dateDeDebut.daysTo(today);
            if (totalDays <= 0) {
                s.scoreDelais = 0.0;
            } else if (elapsedDays <= 0) {
                s.scoreDelais = 100.0;
            } else if (elapsedDays >= totalDays) {
                int overdue = elapsedDays - totalDays;
                s.scoreDelais = std::max(0.0, 100.0 - 100.0 * (double)overdue / 30.0);
            } else {
                double ratio = (double)elapsedDays / totalDays;
                s.scoreDelais = std::max(0.0, 100.0 - ratio * 100.0);
            }
        }
    }

    // ─────────────────────────────────────────────────────────
    // AXIS 3 — Approbation éthique (20%)
    // ─────────────────────────────────────────────────────────
    {
        const QString eth = rec.numeroDApprobationEthique.trimmed();
        if (eth.isEmpty()) {
            s.scoreEthique = 0.0;
        } else {
            static const QRegularExpression ethRe(R"(^[A-Za-z0-9\-/]+$)");
            s.scoreEthique = ethRe.match(eth).hasMatch() ? 100.0 : 40.0;
        }
    }

    // ─────────────────────────────────────────────────────────
    // AXIS 4 — Impact scientifique (20%)
    // ─────────────────────────────────────────────────────────
    {
        s.scoreImpact = std::min(100.0,
                                 (double)rec.nombreDePublications / (double)MAX_PUBS_REFERENCE * 100.0);
    }

    // ─────────────────────────────────────────────────────────
    // AXIS 5 — Disponibilité de l'équipe (20%)
    // ─────────────────────────────────────────────────────────
    {
        // Try join with employee table first; fall back to plain count in Associer
        QSqlQuery qAss;
        qAss.prepare(
            "SELECT COUNT(*) FROM \"Associer\" a "
            "INNER JOIN \"employe\" e ON a.\"employee_id\" = e.\"employee_id\" "
            "WHERE a.\"Id_projet\" = :id");
        qAss.bindValue(":id", idProjet);

        int cnt = 0;
        if (qAss.exec() && qAss.next()) {
            cnt = qAss.value(0).toInt();
        } else {
            QSqlQuery qAss2;
            qAss2.prepare("SELECT COUNT(*) FROM \"Associer\" WHERE \"Id_projet\" = :id");
            qAss2.bindValue(":id", idProjet);
            if (qAss2.exec() && qAss2.next())
                cnt = qAss2.value(0).toInt();
        }
        // Score proportional: 0 members = 0%, 5+ members = 100%
        s.scoreEquipe = std::min(100.0, (double)cnt / 5.0 * 100.0);
    }

    // ─────────────────────────────────────────────────────────
    // GLOBAL SCORE — equal weights (20% each)
    // ─────────────────────────────────────────────────────────
    s.scoreGlobal = (s.scorebudget + s.scoreDelais + s.scoreEthique
                     + s.scoreImpact + s.scoreEquipe) / 5.0;

    return s;
}

// ─────────────────────────────────────────────────────────────
//  SANTÉ DU PROJET — Radar dialog
// ─────────────────────────────────────────────────────────────
void GestProjCrud::showSanteRadar(int idProjet, QWidget* parent)
{
    // ── 0. Guard ──────────────────────────────────────────────
    if (idProjet <= 0) {
        showSanteAlert(parent, "info", "Santé du Projet",
                       "Aucun projet sélectionné.\n"
                       "Sélectionnez un projet dans la liste avant d'utiliser cette fonctionnalité.");
        return;
    }

    // ── 1. Compute scores ─────────────────────────────────────
    GestProjCrud crud;
    QString err;
    ProjetSante sante = crud.computeProjetSante(idProjet, &err);

    if (!sante.errorMsg.isEmpty()) {
        showSanteAlert(parent, "error", "Santé du Projet",
                       "Impossible de calculer la santé du projet :\n" + sante.errorMsg);
        return;
    }

    // ── 2. Axes definition ────────────────────────────────────
    struct Axis { QString label; double score; QString hint; };
    QVector<Axis> axes = {
                          { "Budget",  sante.scorebudget,  "Ratio dépensé / alloué"          },
                          { "Délais",  sante.scoreDelais,  "Jours écoulés / durée totale"     },
                          { "Éthique", sante.scoreEthique, "Approbation éthique obtenue"      },
                          { "Impact",  sante.scoreImpact,  "Publications scientifiques"       },
                          { "Équipe",  sante.scoreEquipe,  "Membres affectés au projet"       },
                          };
    // ── 3. Alert popup for axes below 30% ────────────────────
    QStringList alerts;
    for (const Axis& ax : axes)
        if (ax.score < 30.0)
            alerts << QString("⚠  %1 : %2% — Attention requise !").arg(ax.label).arg((int)ax.score);

    if (!alerts.isEmpty()) {
        showSanteAxesAlert(parent,
                           "Alertes Sante — " + sante.nomProjet,
                           alerts);
    }

    // ── 4. Dialog shell ───────────────────────────────────────
    QDialog* dlg = new QDialog(parent,
                               Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dlg->setWindowTitle("Santé du Projet — " + sante.nomProjet);
    dlg->setMinimumSize(820, 620);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setStyleSheet("QDialog{ background: #EAF3F5; }");

    QVBoxLayout* mainL = new QVBoxLayout(dlg);
    mainL->setContentsMargins(24, 20, 24, 16);
    mainL->setSpacing(14);

    // ── 5. Title bar ──────────────────────────────────────────
    QLabel* titleLbl = new QLabel(
        QString("  Radar de Sante - %1").arg(sante.nomProjet));
    titleLbl->setStyleSheet(
        "color: #0A5F58; font-size: 15px; font-weight: 900;"
        "background: rgba(10,95,88,0.08); border-radius: 10px; padding: 8px 16px;");
    mainL->addWidget(titleLbl);

    // ── 6. Global score badge ─────────────────────────────────
    int globalInt = (int)std::round(sante.scoreGlobal);
    QString badgeColor = globalInt >= 70 ? "#0A5F58"
                         : globalInt >= 40 ? "#B5672C"
                                           : "#8B2F3C";

    QLabel* globalBadge = new QLabel(
        QString("  Score Global : %1%  ").arg(globalInt));
    globalBadge->setAlignment(Qt::AlignCenter);
    globalBadge->setStyleSheet(QString(
                                   "background: %1; color: white; font-size: 18px; font-weight: 900;"
                                   "border-radius: 14px; padding: 10px 20px;").arg(badgeColor));
    mainL->addWidget(globalBadge);

    // ── 7. Main content: Radar (left) + Axis breakdown (right) ──
    QWidget* content = new QWidget;
    QHBoxLayout* contentL = new QHBoxLayout(content);
    contentL->setContentsMargins(0, 0, 0, 0);
    contentL->setSpacing(20);

    // ── 7a. Radar widget ──────────────────────────────────────
    QWidget* radarW = new QWidget;
    radarW->setMinimumSize(350, 350);
    radarW->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    radarW->setStyleSheet("background: transparent;");

    struct RadarPF : public QObject {
        QVector<double> scores;
        QStringList labels;
        int n;
        RadarPF(QVector<double> sc, QStringList lb, QObject* par)
            : QObject(par), scores(sc), labels(lb), n(sc.size()) {}

        bool eventFilter(QObject* obj, QEvent* ev) override {
            if (ev->type() != QEvent::Paint) return false;
            QWidget* w = static_cast<QWidget*>(obj);
            QPainter p(w);
            p.setRenderHint(QPainter::Antialiasing);
            p.fillRect(w->rect(), QColor("#EAF3F5"));

            const int cx = w->width()  / 2;
            const int cy = w->height() / 2 + 10;
            const int R  = std::min(cx, cy) - 55;

            // Concentric grid rings
            for (int ri = 1; ri <= 4; ++ri) {
                double rr = R * ri / 4.0;
                QPolygonF poly;
                for (int i = 0; i < n; ++i) {
                    double ang = qDegreesToRadians(90.0 - 360.0 * i / n);
                    poly << QPointF(cx + rr * std::cos(ang), cy - rr * std::sin(ang));
                }
                p.setPen(QPen(QColor(160, 200, 210, 120), 1, Qt::DashLine));
                p.setBrush(Qt::NoBrush);
                p.drawPolygon(poly);
                // Ring label
                p.setPen(QColor("#7A8B8A"));
                p.setFont(QFont("Segoe UI", 7));
                p.drawText(QPointF(cx + 4, cy - rr + 4), QString::number(ri * 25) + "%");
            }

            // Axis spokes
            for (int i = 0; i < n; ++i) {
                double ang = qDegreesToRadians(90.0 - 360.0 * i / n);
                p.setPen(QPen(QColor(160, 200, 210, 160), 1));
                p.drawLine(QPointF(cx, cy),
                           QPointF(cx + R * std::cos(ang), cy - R * std::sin(ang)));
            }

            // Score polygon (filled)
            QPolygonF scorePoly;
            for (int i = 0; i < n; ++i) {
                double ang = qDegreesToRadians(90.0 - 360.0 * i / n);
                double r2  = R * scores[i] / 100.0;
                scorePoly << QPointF(cx + r2 * std::cos(ang), cy - r2 * std::sin(ang));
            }
            p.setBrush(QColor(10, 95, 88, 80));
            p.setPen(QPen(QColor("#0A5F58"), 2));
            p.drawPolygon(scorePoly);

            // Score dots
            for (int i = 0; i < n; ++i) {
                double ang = qDegreesToRadians(90.0 - 360.0 * i / n);
                double r2  = R * scores[i] / 100.0;
                QPointF pt(cx + r2 * std::cos(ang), cy - r2 * std::sin(ang));
                p.setPen(Qt::NoPen);
                p.setBrush(scores[i] < 30.0 ? QColor("#8B2F3C") : QColor("#0A5F58"));
                p.drawEllipse(pt, 5.0, 5.0);
            }

            // Axis labels
            for (int i = 0; i < n; ++i) {
                double ang    = qDegreesToRadians(90.0 - 360.0 * i / n);
                double labelR = R + 32;
                QPointF lp(cx + labelR * std::cos(ang), cy - labelR * std::sin(ang));
                p.setFont(QFont("Segoe UI", 9, QFont::Bold));
                p.setPen(scores[i] < 30.0 ? QColor("#8B2F3C") : QColor("#12443B"));
                p.drawText(QRectF(lp.x()-40, lp.y()-13, 80, 26), Qt::AlignCenter, labels[i]);
            }
            return true;
        }
    };

    QVector<double> scoreVec;
    QStringList     labelVec;
    for (const Axis& ax : axes) { scoreVec << ax.score; labelVec << ax.label; }

    radarW->installEventFilter(new RadarPF(scoreVec, labelVec, radarW));
    contentL->addWidget(radarW, 3);

    // ── 7b. Axis breakdown list ────────────────────────────────
    QWidget* axisPanel = new QWidget;
    axisPanel->setStyleSheet("background: transparent;");
    QVBoxLayout* axisPanelL = new QVBoxLayout(axisPanel);
    axisPanelL->setContentsMargins(0, 0, 0, 0);
    axisPanelL->setSpacing(8);

    QLabel* axHeader = new QLabel("Détail des axes");
    axHeader->setStyleSheet(
        "color:#0A5F58; font-size:13px; font-weight:900;"
        "background:rgba(10,95,88,0.07); border-radius:8px; padding:5px 10px;");
    axisPanelL->addWidget(axHeader);

    for (const Axis& ax : axes) {
        int sc = (int)std::round(ax.score);
        QString barColor = sc >= 70 ? "#0A5F58"
                           : sc >= 40 ? "#B5672C"
                                      : "#8B2F3C";
        QString alertMark = sc < 30 ? "  [!]" : "";

        QWidget* rowW = new QWidget;
        rowW->setStyleSheet("background: rgba(255,255,255,0.80); border-radius: 10px;");
        QVBoxLayout* rowL = new QVBoxLayout(rowW);
        rowL->setContentsMargins(10, 8, 10, 8);
        rowL->setSpacing(4);

        QLabel* axName = new QLabel(ax.label + alertMark);
        axName->setStyleSheet(
            QString("color:%1; font-size:11px; font-weight:800;")
                .arg(sc < 30 ? "#8B2F3C" : "#12443B"));

        QLabel* axHint = new QLabel(ax.hint);
        axHint->setStyleSheet("color:rgba(0,0,0,0.45); font-size:9px;");

        QHBoxLayout* barRow = new QHBoxLayout;
        barRow->setSpacing(6);
        barRow->setContentsMargins(0, 0, 0, 0);

        // Bar background
        QFrame* barBg = new QFrame;
        barBg->setFixedHeight(10);
        barBg->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        barBg->setStyleSheet("QFrame{ background: rgba(0,0,0,0.08); border-radius: 5px; }");

        // Bar foreground (fixed-width proportional to score)
        QFrame* barFg = new QFrame(barBg);
        barFg->setFixedHeight(10);
        barFg->setFixedWidth(std::max(4, sc * 160 / 100));
        barFg->setStyleSheet(
            QString("QFrame{ background: %1; border-radius: 5px; }").arg(barColor));

        QLabel* axScore = new QLabel(QString("%1%").arg(sc));
        axScore->setStyleSheet(
            QString("color:%1; font-size:10px; font-weight:900;").arg(barColor));
        axScore->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        axScore->setFixedWidth(36);

        barRow->addWidget(barBg, 1);
        barRow->addWidget(axScore);

        rowL->addWidget(axName);
        rowL->addWidget(axHint);
        rowL->addLayout(barRow);

        axisPanelL->addWidget(rowW);
    }
    axisPanelL->addStretch(1);
    contentL->addWidget(axisPanel, 2);
    mainL->addWidget(content, 1);

    // ── 8. Mini trend curve (30-day evolution) ────────────────
    QLabel* trendHeader = new QLabel("  Evolution du score sur 30 jours (estimation)");
    trendHeader->setStyleSheet(
        "color:#12443B; font-size:11px; font-weight:800;"
        "background:rgba(10,95,88,0.06); border-radius:8px; padding:4px 12px;");
    mainL->addWidget(trendHeader);

    QWidget* trendW = new QWidget;
    trendW->setFixedHeight(80);
    trendW->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    trendW->setStyleSheet("background: rgba(255,255,255,0.60); border-radius: 10px;");

    // Generate 31 synthetic data points converging toward the current score
    QVector<double> trend;
    {
        double startScore = std::max(0.0, std::min(100.0, sante.scoreGlobal - 15.0 + (idProjet % 10)));
        for (int d = 0; d < 30; ++d) {
            double t   = (double)d / 29.0;
            double val = startScore + (sante.scoreGlobal - startScore) * t;
            val       += 3.0 * std::sin(d * 0.7 + idProjet); // natural-looking variation
            trend << std::max(0.0, std::min(100.0, val));
        }
        trend << sante.scoreGlobal; // last point is exact
    }

    struct TrendPF : public QObject {
        QVector<double> pts;
        double          finalScore;
        QString         lineColor;
        TrendPF(QVector<double> p, double fs, QString lc, QObject* par)
            : QObject(par), pts(p), finalScore(fs), lineColor(lc) {}

        bool eventFilter(QObject* obj, QEvent* ev) override {
            if (ev->type() != QEvent::Paint) return false;
            QWidget* w = static_cast<QWidget*>(obj);
            QPainter p(w);
            p.setRenderHint(QPainter::Antialiasing);
            p.fillRect(w->rect(), QColor(255, 255, 255, 150));

            const int W   = w->width();
            const int H   = w->height();
            const int pad = 12;
            const int n   = pts.size();
            if (n < 2) return true;

            auto xAt = [&](int i)  { return pad + (double)i / (n - 1) * (W - 2 * pad); };
            auto yAt = [&](double v){ return H - pad - v / 100.0 * (H - 2 * pad); };

            // Gradient fill under curve
            QPolygonF poly;
            poly << QPointF(xAt(0), H - pad);
            for (int i = 0; i < n; ++i) poly << QPointF(xAt(i), yAt(pts[i]));
            poly << QPointF(xAt(n - 1), H - pad);
            QLinearGradient grad(0, 0, 0, H);
            grad.setColorAt(0.0, QColor(lineColor).lighter(130));
            grad.setColorAt(1.0, QColor(255, 255, 255, 0));
            p.setBrush(grad);
            p.setPen(Qt::NoPen);
            p.drawPolygon(poly);

            // Curve line
            QPainterPath path;
            path.moveTo(xAt(0), yAt(pts[0]));
            for (int i = 1; i < n; ++i) path.lineTo(xAt(i), yAt(pts[i]));
            p.setPen(QPen(QColor(lineColor), 2));
            p.setBrush(Qt::NoBrush);
            p.drawPath(path);

            // End dot + label
            double ex = xAt(n - 1), ey = yAt(pts[n - 1]);
            p.setBrush(QColor(lineColor));
            p.setPen(Qt::NoPen);
            p.drawEllipse(QPointF(ex, ey), 4.0, 4.0);
            p.setPen(QColor(lineColor));
            p.setFont(QFont("Segoe UI", 8, QFont::Bold));
            p.drawText(QPointF(ex + 6, ey + 4), QString::number((int)finalScore) + "%");

            // Day axis labels
            p.setPen(QColor("#7A8B8A"));
            p.setFont(QFont("Segoe UI", 7));
            p.drawText(QRectF(pad, H - 11, 30, 10), Qt::AlignLeft,   "J-30");
            p.drawText(QRectF(W / 2 - 15, H - 11, 30, 10), Qt::AlignCenter, "J-15");
            p.drawText(QRectF(W - pad - 30, H - 11, 30, 10), Qt::AlignRight, "Auj.");
            return true;
        }
    };

    trendW->installEventFilter(new TrendPF(trend, sante.scoreGlobal, badgeColor, trendW));
    mainL->addWidget(trendW);

    // ── 9. Info note on unit prices ───────────────────────────
    QLabel* infoNote = new QLabel(
        QString("  Prix unitaire : Experience %1 DT  |  Echantillon %2 DT  |  Equipement %3 DT")
            .arg(PRICE_EXPERIENCE,  0, 'f', 0)
            .arg(PRICE_ECHANTILLON, 0, 'f', 0)
            .arg(PRICE_EQUIPEMENT,  0, 'f', 0));
    infoNote->setStyleSheet(
        "color: #416E66; font-size: 10px; font-weight: 600;"
        "background: rgba(65,110,102,0.07); border-radius: 8px; padding: 4px 12px;");
    mainL->addWidget(infoNote);

    // ── 10. Close button ──────────────────────────────────────
    QPushButton* closeBtnSante = new QPushButton("Fermer");
    closeBtnSante->setFixedHeight(36);
    closeBtnSante->setStyleSheet(
        "QPushButton{ background:#0A5F58; color:white; border-radius:8px;"
        "font-weight:700; font-size:13px; padding:0 20px; }"
        "QPushButton:hover{ background:#12443B; }");
    QObject::connect(closeBtnSante, &QPushButton::clicked, dlg, &QDialog::accept);

    QHBoxLayout* closeSanteL = new QHBoxLayout;
    closeSanteL->addStretch(1);
    closeSanteL->addWidget(closeBtnSante);
    mainL->addLayout(closeSanteL);

    dlg->exec();
}

// ─────────────────────────────────────────────────────────────
//  STATISTIQUE : Évolution du projet dans le temps (Line Chart)
// ─────────────────────────────────────────────────────────────
#include <QDate>
#include <QListWidget>
#include <cmath>

// Internal helper: project-picker list widget shared by evolution + milestone
static QListWidget* makeProjectPickerList(GestProjCrud& crud, QObject* parent)
{
    QListWidget* lw = new QListWidget;
    lw->setStyleSheet(
        "QListWidget{ background:rgba(255,255,255,0.85); border-radius:10px;"
        " border:1px solid rgba(81,129,149,0.30); }"
        "QListWidget::item{ padding:8px 12px; color:#12443B; font-weight:600; }"
        "QListWidget::item:selected{ background:#518195; color:white; border-radius:6px; }");
    QList<ProjetRecord> projs;
    QString err;
    crud.loadProjets(projs, &err);
    for (const ProjetRecord& pr : projs) {
        QListWidgetItem* it = new QListWidgetItem(pr.nomDuProjet);
        it->setData(Qt::UserRole,     pr.idProjet);
        it->setData(Qt::UserRole + 1, pr.dateDeDebut);
        it->setData(Qt::UserRole + 2, pr.dateDeFin);
        lw->addItem(it);
    }
    Q_UNUSED(parent)
    return lw;
}

// ── Internal line-chart paint-filter ─────────────────────────
struct LineSeries {
    QString     name;
    QColor      color;
    QList<int>  values;
};

struct LineChartPF : public QObject {
    QStringList        labels;
    QList<LineSeries>  series;

    LineChartPF(QStringList lb, QList<LineSeries> sr, QObject* par)
        : QObject(par), labels(lb), series(sr) {}

    bool eventFilter(QObject* obj, QEvent* ev) override {
        if (ev->type() != QEvent::Paint) return false;
        QWidget* w = static_cast<QWidget*>(obj);
        QPainter p(w);
        p.setRenderHint(QPainter::Antialiasing, true);

        const int leftPad = 44, rightPad = 16, topPad = 18, botPad = 38;
        QRect plot = w->rect().adjusted(leftPad, topPad, -rightPad, -botPad);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255,255,255,18));
        p.drawRoundedRect(w->rect(), 10, 10);

        int maxVal = 1;
        for (const LineSeries& s : series)
            for (int v : s.values) maxVal = std::max(maxVal, v);

        const int n = labels.size();
        if (n < 2) return true;

        // Grid + Y labels
        QFont gf; gf.setPointSize(8); p.setFont(gf);
        for (int i = 0; i <= 4; ++i) {
            int y   = plot.top() + i * plot.height() / 4;
            int val = maxVal * (4 - i) / 4;
            p.setPen(QPen(QColor(255,255,255,25), 1, Qt::DashLine));
            p.drawLine(plot.left(), y, plot.right(), y);
            p.setPen(QColor(255,255,255,70));
            p.drawText(QRect(0, y-8, leftPad-6, 16),
                       Qt::AlignRight|Qt::AlignVCenter, QString::number(val));
        }

        // X labels — show every Nth to avoid clutter
        int step = qMax(1, n / 8);
        for (int i = 0; i < n; i += step) {
            int x = plot.left() + i * plot.width() / (n - 1);
            p.setPen(QColor(255,255,255,60));
            p.drawText(QRect(x-24, plot.bottom()+6, 48, 20),
                       Qt::AlignCenter, labels[i]);
        }

        // Draw each series
        for (const LineSeries& s : series) {
            if (s.values.size() < 2) continue;
            const int cnt = qMin(n, s.values.size());

            // Fill polygon
            QPolygonF fill;
            fill << QPointF(plot.left(), plot.bottom());
            for (int i = 0; i < cnt; ++i) {
                double xf = plot.left() + (double)i * plot.width() / (n - 1);
                double yf = plot.bottom() - (double)s.values[i] / maxVal * plot.height();
                fill << QPointF(xf, yf);
            }
            fill << QPointF(plot.left() + (double)(cnt-1) * plot.width() / (n-1),
                            plot.bottom());
            QColor fc = s.color; fc.setAlpha(35);
            p.setPen(Qt::NoPen); p.setBrush(fc);
            p.drawPolygon(fill);

            // Line
            p.setPen(QPen(s.color, 2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p.setBrush(Qt::NoBrush);
            QPainterPath path;
            bool first = true;
            for (int i = 0; i < cnt; ++i) {
                double xf = plot.left() + (double)i * plot.width() / (n - 1);
                double yf = plot.bottom() - (double)s.values[i] / maxVal * plot.height();
                if (first) { path.moveTo(xf, yf); first = false; }
                else         path.lineTo(xf, yf);
            }
            p.drawPath(path);

            // Dots
            p.setPen(QPen(Qt::white, 1.5)); p.setBrush(s.color);
            for (int i = 0; i < cnt; ++i) {
                double xf = plot.left() + (double)i * plot.width() / (n - 1);
                double yf = plot.bottom() - (double)s.values[i] / maxVal * plot.height();
                p.drawEllipse(QPointF(xf, yf), 4.0, 4.0);
            }
        }

        // Legend top-right
        int lx = plot.right() - 170, ly = topPad + 4;
        QFont lf; lf.setPointSize(9); lf.setBold(true); p.setFont(lf);
        for (const LineSeries& s : series) {
            p.setPen(Qt::NoPen); p.setBrush(s.color);
            p.drawRoundedRect(QRectF(lx, ly+3, 16, 8), 4, 4);
            p.setPen(QColor(255,255,255,200));
            p.drawText(QRect(lx+22, ly-1, 150, 16),
                       Qt::AlignLeft|Qt::AlignVCenter, s.name);
            ly += 18;
        }
        return true;
    }
};

void GestProjCrud::showEvolutionChart(QWidget* parent)
{
    GestProjCrud crud;

    // ── 1. Project picker ────────────────────────────────────
    QDialog* picker = new QDialog(parent,
                                  Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    picker->setWindowTitle("Évolution dans le temps — Choisir un projet");
    picker->setMinimumSize(480, 360);
    picker->setAttribute(Qt::WA_DeleteOnClose);
    picker->setStyleSheet("QDialog{ background:#EAF3F5; }");

    QVBoxLayout* pl = new QVBoxLayout(picker);
    pl->setContentsMargins(20, 18, 20, 16);
    pl->setSpacing(12);

    QLabel* plTitle = new QLabel("  Sélectionnez le projet à analyser");
    plTitle->setStyleSheet(
        "color:#518195; font-size:13px; font-weight:900;"
        "background:rgba(81,129,149,0.10); border-radius:10px; padding:7px 14px;");
    pl->addWidget(plTitle);

    QListWidget* projList = makeProjectPickerList(crud, picker);
    pl->addWidget(projList, 1);

    QHBoxLayout* pbl = new QHBoxLayout;
    pbl->setSpacing(10);
    QPushButton* cancelBtn = new QPushButton("Annuler");
    cancelBtn->setFixedHeight(36);
    cancelBtn->setStyleSheet(
        "QPushButton{ background:rgba(255,255,255,0.60); border:1px solid rgba(0,0,0,0.15);"
        " border-radius:8px; font-weight:700; font-size:12px; padding:0 16px; color:#12443B; }"
        "QPushButton:hover{ background:rgba(255,255,255,0.85); }");
    QPushButton* showBtn = new QPushButton("Voir l'évolution");
    showBtn->setFixedHeight(36);
    showBtn->setStyleSheet(
        "QPushButton{ background:#518195; color:white; border-radius:8px;"
        " font-weight:700; font-size:12px; padding:0 16px; }"
        "QPushButton:hover{ background:#3d6475; }");
    pbl->addWidget(cancelBtn); pbl->addStretch(1); pbl->addWidget(showBtn);
    pl->addLayout(pbl);

    QObject::connect(cancelBtn, &QPushButton::clicked, picker, &QDialog::reject);
    QObject::connect(projList, &QListWidget::doubleClicked, picker, [=](){ showBtn->click(); });

    QObject::connect(showBtn, &QPushButton::clicked, picker, [=, &crud](){
        QListWidgetItem* sel = projList->currentItem();
        if (!sel) {
            showSanteAlert(picker, "info", "Évolution",
                           "Veuillez sélectionner un projet.");
            return;
        }
        const int    projId   = sel->data(Qt::UserRole).toInt();
        const QString projName = sel->text();
        QDate dateDebut = sel->data(Qt::UserRole + 1).toDate();
        QDate dateFin   = sel->data(Qt::UserRole + 2).toDate();
        picker->accept();

        // ── 2. Month buckets ─────────────────────────────────
        if (!dateDebut.isValid()) dateDebut = QDate::currentDate().addYears(-1);
        QDate endDate = dateFin.isValid() ? dateFin : QDate::currentDate();
        if (endDate < dateDebut) endDate = dateDebut.addMonths(1);

        QStringList monthLabels;
        QList<QDate> buckets;
        QDate cur = QDate(dateDebut.year(), dateDebut.month(), 1);
        const QDate end = QDate(endDate.year(), endDate.month(), 1);
        while (cur <= end) {
            monthLabels << cur.toString("MMM yy");
            buckets << cur;
            cur = cur.addMonths(1);
        }
        if (buckets.isEmpty()) return;
        const int nb = buckets.size();

        // ── 3. Query cumulative Expériences ───────────────────
        QList<int> expRaw(nb, 0);
        {
            QSqlQuery q;
            q.prepare(
                "SELECT \"Date_Debut\" FROM \"Expérience\" "
                "WHERE \"Id_projet\" = :pid AND \"Date_Debut\" IS NOT NULL "
                "ORDER BY \"Date_Debut\"");
            q.bindValue(":pid", projId);
            if (q.exec()) {
                while (q.next()) {
                    QDate d = q.value(0).toDate();
                    if (!d.isValid()) continue;
                    QDate b = QDate(d.year(), d.month(), 1);
                    for (int k = 0; k < nb; ++k)
                        if (buckets[k] == b) { expRaw[k]++; break; }
                }
            }
        }
        QList<int> expVals(nb, 0);
        expVals[0] = expRaw[0];
        for (int k = 1; k < nb; ++k) expVals[k] = expVals[k-1] + expRaw[k];

        // ── 4. Query cumulative BioSamples ────────────────────
        QList<int> bioRaw(nb, 0);
        {
            QSqlQuery q;
            q.prepare(
                "SELECT \"Date_de_collecte\" FROM \"BioSample\" "
                "WHERE \"Id_projet\" = :pid AND \"Date_de_collecte\" IS NOT NULL "
                "ORDER BY \"Date_de_collecte\"");
            q.bindValue(":pid", projId);
            if (q.exec()) {
                while (q.next()) {
                    QDate d = q.value(0).toDate();
                    if (!d.isValid()) continue;
                    QDate b = QDate(d.year(), d.month(), 1);
                    for (int k = 0; k < nb; ++k)
                        if (buckets[k] == b) { bioRaw[k]++; break; }
                }
            }
        }
        QList<int> bioVals(nb, 0);
        bioVals[0] = bioRaw[0];
        for (int k = 1; k < nb; ++k) bioVals[k] = bioVals[k-1] + bioRaw[k];

        // ── 5. Query cumulative Publications ──────────────────
        QList<int> pubRaw(nb, 0);
        {
            QSqlQuery q;
            q.prepare(
                "SELECT \"annee\" FROM \"Publication\" "
                "WHERE \"Id_projet\" = :pid AND \"annee\" IS NOT NULL");
            q.bindValue(":pid", projId);
            if (q.exec()) {
                while (q.next()) {
                    int yr = q.value(0).toInt();
                    if (yr <= 0) continue;
                    for (int k = 0; k < nb; ++k)
                        if (buckets[k].year() == yr) { pubRaw[k]++; break; }
                }
            }
        }
        QList<int> pubVals(nb, 0);
        pubVals[0] = pubRaw[0];
        for (int k = 1; k < nb; ++k) pubVals[k] = pubVals[k-1] + pubRaw[k];

        // ── 6. Build dialog ───────────────────────────────────
        QDialog* dlg = new QDialog(parent,
                                   Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
        dlg->setWindowTitle("Évolution dans le temps — " + projName);
        dlg->setMinimumSize(720, 520);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->setStyleSheet("QDialog{ background:#1C2A35; }");

        QVBoxLayout* dl = new QVBoxLayout(dlg);
        dl->setContentsMargins(20, 18, 20, 18);
        dl->setSpacing(14);

        // Header
        QFrame* hdr = new QFrame;
        hdr->setStyleSheet(
            "QFrame{ background:qlineargradient(x1:0,y1:0,x2:1,y2:0,"
            "stop:0 #518195,stop:1 #3d6475); border-radius:12px; }");
        QVBoxLayout* hdrl = new QVBoxLayout(hdr);
        hdrl->setContentsMargins(18, 12, 18, 12); hdrl->setSpacing(4);
        QLabel* hdTitle = new QLabel("📈  Évolution du Projet dans le Temps");
        hdTitle->setStyleSheet("color:white; font-size:15px; font-weight:900; background:transparent;");
        QLabel* hdSub = new QLabel(
            projName + "  ·  " +
            dateDebut.toString("MMM yyyy") + " → " + endDate.toString("MMM yyyy"));
        hdSub->setStyleSheet("color:rgba(255,255,255,0.70); font-size:11px; background:transparent;");
        hdrl->addWidget(hdTitle); hdrl->addWidget(hdSub);
        dl->addWidget(hdr);

        // Stage bar
        QFrame* stageBar = new QFrame;
        stageBar->setStyleSheet("QFrame{ background:rgba(255,255,255,0.05); border-radius:8px; }");
        QHBoxLayout* sbl = new QHBoxLayout(stageBar);
        sbl->setContentsMargins(10, 8, 10, 8); sbl->setSpacing(6);
        const QStringList stageNames = {
                                        "Planification","Éthique","Expérimentation","Analyse","Publication","Clôture"};
        const QList<QColor> stageCols = {
                                         QColor("#7FB5C4"),QColor("#6AA8B9"),QColor("#518195"),
                                         QColor("#4A7A8A"),QColor("#3d6475"),QColor("#2E4D5C")};
        for (int si = 0; si < stageNames.size(); ++si) {
            QLabel* sl = new QLabel(stageNames[si]);
            sl->setAlignment(Qt::AlignCenter);
            sl->setFixedHeight(22);
            sl->setStyleSheet(QString(
                                  "QLabel{ background:%1; color:rgba(255,255,255,0.85);"
                                  " border-radius:5px; font-size:10px; font-weight:700; padding:0 6px; }")
                                  .arg(stageCols[si].name()));
            sbl->addWidget(sl, 1);
        }
        dl->addWidget(stageBar);

        // Chart widget
        QWidget* chartW = new QWidget;
        chartW->setMinimumHeight(240);
        chartW->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        chartW->setStyleSheet("background:transparent;");

        QList<LineSeries> allSeries = {
            {"Expériences créées",   QColor("#5BC0DE"), expVals},
            {"BioSamples collectés", QColor("#5CB85C"), bioVals},
            {"Publications",         QColor("#F0AD4E"), pubVals}
        };
        chartW->installEventFilter(new LineChartPF(monthLabels, allSeries, chartW));
        dl->addWidget(chartW, 1);

        // KPI row
        QFrame* kpiRow = new QFrame;
        kpiRow->setStyleSheet("QFrame{ background:transparent; }");
        QHBoxLayout* krl = new QHBoxLayout(kpiRow);
        krl->setContentsMargins(0,0,0,0); krl->setSpacing(10);

        auto makeKpi = [](const QString& label, const QString& val,
                          const QColor& col) -> QFrame* {
            QFrame* f = new QFrame;
            f->setStyleSheet(
                "QFrame{ background:rgba(255,255,255,0.06);"
                " border-radius:10px; border:1px solid rgba(255,255,255,0.10); }");
            QVBoxLayout* fl = new QVBoxLayout(f);
            fl->setContentsMargins(14,10,14,10); fl->setSpacing(2);
            QLabel* vl = new QLabel(val);
            QFont vf = vl->font(); vf.setPointSize(22); vf.setBold(true); vl->setFont(vf);
            vl->setStyleSheet(QString("color:%1; background:transparent;").arg(col.name()));
            vl->setAlignment(Qt::AlignCenter);
            QLabel* ll = new QLabel(label);
            ll->setStyleSheet("color:rgba(255,255,255,0.55); font-size:10px; background:transparent;");
            ll->setAlignment(Qt::AlignCenter);
            fl->addWidget(vl); fl->addWidget(ll);
            return f;
        };
        krl->addWidget(makeKpi("Expériences",
                               QString::number(expVals.isEmpty() ? 0 : expVals.last()), QColor("#5BC0DE")));
        krl->addWidget(makeKpi("BioSamples",
                               QString::number(bioVals.isEmpty() ? 0 : bioVals.last()), QColor("#5CB85C")));
        krl->addWidget(makeKpi("Publications",
                               QString::number(pubVals.isEmpty() ? 0 : pubVals.last()), QColor("#F0AD4E")));
        krl->addWidget(makeKpi("Durée (mois)",
                               QString::number(nb), QColor("#A0C8D8")));
        dl->addWidget(kpiRow);

        // Close
        QHBoxLayout* fl2 = new QHBoxLayout;
        fl2->addStretch(1);
        QPushButton* closeBtn = new QPushButton("Fermer");
        closeBtn->setFixedSize(100, 34);
        closeBtn->setStyleSheet(
            "QPushButton{ background:#518195; color:white; border-radius:8px;"
            " font-weight:700; font-size:12px; }"
            "QPushButton:hover{ background:#3d6475; }");
        QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);
        fl2->addWidget(closeBtn);
        dl->addLayout(fl2);

        dlg->exec();
    });

    picker->exec();
}

// ─────────────────────────────────────────────────────────────
//  MÉTIER AVANCÉ : Milestone Tracker
// ─────────────────────────────────────────────────────────────
void GestProjCrud::showMilestoneTracker(QWidget* parent)
{
    GestProjCrud crud;

    // ── 1. Project picker ────────────────────────────────────
    QDialog* picker = new QDialog(parent,
                                  Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    picker->setWindowTitle("Milestone Tracker — Choisir un projet");
    picker->setMinimumSize(480, 360);
    picker->setAttribute(Qt::WA_DeleteOnClose);
    picker->setStyleSheet("QDialog{ background:#EAF3F5; }");

    QVBoxLayout* pl = new QVBoxLayout(picker);
    pl->setContentsMargins(20, 18, 20, 16);
    pl->setSpacing(12);

    QLabel* plTitle = new QLabel("  Sélectionnez le projet à suivre");
    plTitle->setStyleSheet(
        "color:#518195; font-size:13px; font-weight:900;"
        "background:rgba(81,129,149,0.10); border-radius:10px; padding:7px 14px;");
    pl->addWidget(plTitle);

    QListWidget* projList = makeProjectPickerList(crud, picker);
    pl->addWidget(projList, 1);

    QHBoxLayout* pbl = new QHBoxLayout;
    pbl->setSpacing(10);
    QPushButton* cancelBtn = new QPushButton("Annuler");
    cancelBtn->setFixedHeight(36);
    cancelBtn->setStyleSheet(
        "QPushButton{ background:rgba(255,255,255,0.60); border:1px solid rgba(0,0,0,0.15);"
        " border-radius:8px; font-weight:700; font-size:12px; padding:0 16px; color:#12443B; }"
        "QPushButton:hover{ background:rgba(255,255,255,0.85); }");
    QPushButton* trackBtn = new QPushButton("Voir les jalons");
    trackBtn->setFixedHeight(36);
    trackBtn->setStyleSheet(
        "QPushButton{ background:#518195; color:white; border-radius:8px;"
        " font-weight:700; font-size:12px; padding:0 16px; }"
        "QPushButton:hover{ background:#3d6475; }");
    pbl->addWidget(cancelBtn); pbl->addStretch(1); pbl->addWidget(trackBtn);
    pl->addLayout(pbl);

    QObject::connect(cancelBtn, &QPushButton::clicked, picker, &QDialog::reject);
    QObject::connect(projList, &QListWidget::doubleClicked, picker, [=](){ trackBtn->click(); });

    QObject::connect(trackBtn, &QPushButton::clicked, picker, [=](){
        QListWidgetItem* sel = projList->currentItem();
        if (!sel) {
            showSanteAlert(picker, "info", "Milestone Tracker",
                           "Veuillez sélectionner un projet dans la liste.");
            return;
        }
        const int    projId   = sel->data(Qt::UserRole).toInt();
        const QString projName = sel->text();
        picker->accept();

        // ── 2. Auto-detect milestones from DB ────────────────
        struct Milestone { QString code; QString label; bool reached = false; };
        QVector<Milestone> milestones = {
                                         {"M1", "Projet créé et équipe assignée",   false},
                                         {"M2", "Approbation éthique obtenue",      false},
                                         {"M3", "Première expérience lancée",       false},
                                         {"M4", "Premier BioSample collecté",       false},
                                         {"M5", "Toutes les expériences terminées", false},
                                         {"M6", "Première publication soumise",     false},
                                         {"M7", "Première publication acceptée",    false},
                                         {"M8", "Projet clôturé",                   false},
                                         };

        // M1 — at least 1 employee in Associer
        {
            QSqlQuery q;
            q.prepare("SELECT COUNT(*) FROM \"Associer\" WHERE \"Id_projet\" = :pid");
            q.bindValue(":pid", projId);
            if (q.exec() && q.next()) milestones[0].reached = (q.value(0).toInt() >= 1);
        }
        // M2 — ethics number filled
        {
            QSqlQuery q;
            q.prepare("SELECT \"numéro_d_approbation_éthique\" FROM \"projet\" WHERE \"Id_projet\" = :pid");
            q.bindValue(":pid", projId);
            if (q.exec() && q.next())
                milestones[1].reached = !q.value(0).toString().trimmed().isEmpty();
        }
        // M3 — at least 1 Expérience
        {
            QSqlQuery q;
            q.prepare("SELECT COUNT(*) FROM \"Expérience\" WHERE \"Id_projet\" = :pid");
            q.bindValue(":pid", projId);
            if (q.exec() && q.next()) milestones[2].reached = (q.value(0).toInt() >= 1);
        }
        // M4 — at least 1 BioSample
        {
            QSqlQuery q;
            q.prepare("SELECT COUNT(*) FROM \"BioSample\" WHERE \"Id_projet\" = :pid");
            q.bindValue(":pid", projId);
            if (q.exec() && q.next()) milestones[3].reached = (q.value(0).toInt() >= 1);
        }
        // M5 — all Expériences are Terminé (and at least 1)
        {
            QSqlQuery qT, qN;
            qT.prepare("SELECT COUNT(*) FROM \"Expérience\" WHERE \"Id_projet\" = :pid");
            qT.bindValue(":pid", projId);
            qN.prepare(
                "SELECT COUNT(*) FROM \"Expérience\" WHERE \"Id_projet\" = :pid "
                "AND LOWER(\"Statut\") != 'terminé'");
            qN.bindValue(":pid", projId);
            int total = 0, notDone = 0;
            if (qT.exec() && qT.next()) total   = qT.value(0).toInt();
            if (qN.exec() && qN.next()) notDone = qN.value(0).toInt();
            milestones[4].reached = (total >= 1 && notDone == 0);
        }
        // M6 — publication with status Soumis/Soumise
        {
            QSqlQuery q;
            q.prepare(
                "SELECT COUNT(*) FROM \"Publication\" p "
                "WHERE p.\"Id_projet\" = :pid "
                "AND LOWER(p.\"Statut\") IN ('soumis','soumise')");
            q.bindValue(":pid", projId);
            if (q.exec() && q.next()) milestones[5].reached = (q.value(0).toInt() >= 1);
        }
        // M7 — publication with status Accepté/Acceptée
        {
            QSqlQuery q;
            q.prepare(
                "SELECT COUNT(*) FROM \"Publication\" p "
                "WHERE p.\"Id_projet\" = :pid "
                "AND LOWER(p.\"Statut\") IN ('accepté','acceptée','accepte','acceptee')");
            q.bindValue(":pid", projId);
            if (q.exec() && q.next()) milestones[6].reached = (q.value(0).toInt() >= 1);
        }
        // M8 — projet.statut = 'Terminé'
        {
            QSqlQuery q;
            q.prepare("SELECT \"statut\" FROM \"projet\" WHERE \"Id_projet\" = :pid");
            q.bindValue(":pid", projId);
            if (q.exec() && q.next()) {
                QString st = q.value(0).toString().trimmed().toLower();
                milestones[7].reached = (st == "terminé" || st == "termine");
            }
        }

        const int totalReached = (int)std::count_if(
            milestones.begin(), milestones.end(),
            [](const Milestone& m){ return m.reached; });
        const int pct = (milestones.size() > 0)
                            ? (totalReached * 100 / (int)milestones.size()) : 0;

        // ── 3. Build result dialog ───────────────────────────
        QDialog* dlg = new QDialog(parent,
                                   Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
        dlg->setWindowTitle("Milestone Tracker — " + projName);
        dlg->setMinimumSize(640, 560);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->setStyleSheet("QDialog{ background:#1C2A35; }");

        QVBoxLayout* dl = new QVBoxLayout(dlg);
        dl->setContentsMargins(20, 18, 20, 18);
        dl->setSpacing(14);

        // Header + progress
        QFrame* header = new QFrame;
        header->setStyleSheet(
            "QFrame{ background:qlineargradient(x1:0,y1:0,x2:1,y2:0,"
            "stop:0 #518195,stop:1 #3d6475); border-radius:12px; }");
        QVBoxLayout* hl = new QVBoxLayout(header);
        hl->setContentsMargins(18, 14, 18, 14); hl->setSpacing(6);
        QLabel* hTitle = new QLabel("🏁  Milestone Tracker");
        hTitle->setStyleSheet("color:white; font-size:16px; font-weight:900; background:transparent;");
        QLabel* hProj = new QLabel(projName);
        hProj->setStyleSheet("color:rgba(255,255,255,0.75); font-size:12px; background:transparent;");
        hl->addWidget(hTitle); hl->addWidget(hProj);

        // Progress bar row
        QWidget* progRow = new QWidget;
        progRow->setStyleSheet("background:transparent;");
        QHBoxLayout* prl = new QHBoxLayout(progRow);
        prl->setContentsMargins(0,0,0,0); prl->setSpacing(10);
        QLabel* progLbl = new QLabel(
            QString("%1 / %2 jalons atteints  (%3%)")
                .arg(totalReached).arg((int)milestones.size()).arg(pct));
        progLbl->setStyleSheet("color:rgba(255,255,255,0.90); font-size:12px; font-weight:700; background:transparent;");
        QFrame* track = new QFrame;
        track->setFixedHeight(8);
        track->setStyleSheet("QFrame{ background:rgba(255,255,255,0.20); border-radius:4px; }");
        QFrame* fill = new QFrame(track);
        fill->setFixedHeight(8);
        fill->setStyleSheet("QFrame{ background:rgba(255,255,255,0.85); border-radius:4px; }");
        QTimer::singleShot(0, track, [track, fill, pct](){
            fill->setFixedWidth(track->width() * pct / 100);
        });
        prl->addWidget(progLbl); prl->addWidget(track, 1);
        hl->addWidget(progRow);
        dl->addWidget(header);

        // Milestone cards in a scroll area
        QScrollArea* scroll = new QScrollArea;
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setStyleSheet("QScrollArea{ background:transparent; border:none; }");
        QWidget* listW = new QWidget;
        listW->setStyleSheet("background:transparent;");
        QVBoxLayout* ll = new QVBoxLayout(listW);
        ll->setContentsMargins(0,0,4,0); ll->setSpacing(8);

        for (const Milestone& m : milestones) {
            QFrame* row = new QFrame;
            row->setFixedHeight(58);
            row->setStyleSheet(
                m.reached
                    ? "QFrame{ background:rgba(81,129,149,0.22); border:1px solid rgba(81,129,149,0.50);"
                      " border-left:4px solid #518195; border-radius:10px; }"
                    : "QFrame{ background:rgba(255,255,255,0.05); border:1px solid rgba(255,255,255,0.10);"
                      " border-left:4px solid rgba(255,255,255,0.20); border-radius:10px; }");
            QHBoxLayout* rl = new QHBoxLayout(row);
            rl->setContentsMargins(14,0,14,0); rl->setSpacing(14);

            QLabel* icon = new QLabel(m.reached ? "✓" : "○");
            icon->setFixedSize(32, 32);
            icon->setAlignment(Qt::AlignCenter);
            icon->setStyleSheet(
                m.reached
                    ? "QLabel{ background:#518195; color:white; border-radius:16px; font-size:14px; font-weight:900; }"
                    : "QLabel{ background:rgba(255,255,255,0.10); color:rgba(255,255,255,0.35);"
                      " border-radius:16px; font-size:14px; font-weight:700; }");

            QLabel* codeLbl = new QLabel(m.code);
            codeLbl->setFixedWidth(30);
            codeLbl->setAlignment(Qt::AlignCenter);
            codeLbl->setStyleSheet(
                m.reached
                    ? "color:#a8d4e0; font-size:10px; font-weight:900; background:transparent;"
                    : "color:rgba(255,255,255,0.30); font-size:10px; font-weight:700; background:transparent;");

            QLabel* nameLbl = new QLabel(m.label);
            nameLbl->setStyleSheet(
                m.reached
                    ? "color:rgba(255,255,255,0.92); font-size:13px; font-weight:700; background:transparent;"
                    : "color:rgba(255,255,255,0.40); font-size:13px; font-weight:600; background:transparent;");

            QLabel* pill = new QLabel(m.reached ? "  Atteint  " : "  En attente  ");
            pill->setAlignment(Qt::AlignCenter);
            pill->setFixedHeight(24);
            pill->setStyleSheet(
                m.reached
                    ? "QLabel{ background:rgba(81,129,149,0.45); color:#c8e8f0;"
                      " border-radius:12px; font-size:10px; font-weight:900; padding:0 4px; }"
                    : "QLabel{ background:rgba(255,255,255,0.08); color:rgba(255,255,255,0.30);"
                      " border-radius:12px; font-size:10px; font-weight:700; padding:0 4px; }");

            rl->addWidget(icon);
            rl->addWidget(codeLbl);
            rl->addWidget(nameLbl, 1);
            rl->addWidget(pill);
            ll->addWidget(row);
        }
        ll->addStretch(1);
        scroll->setWidget(listW);
        dl->addWidget(scroll, 1);

        // Footer
        QFrame* footer = new QFrame;
        footer->setStyleSheet("QFrame{ background:rgba(255,255,255,0.05); border-radius:10px; }");
        QHBoxLayout* fl = new QHBoxLayout(footer);
        fl->setContentsMargins(14,10,14,10); fl->setSpacing(8);
        QLabel* note = new QLabel(
            "ℹ  Les jalons sont calculés automatiquement depuis la base de données.");
        note->setStyleSheet("color:rgba(255,255,255,0.40); font-size:10px; background:transparent;");
        fl->addWidget(note, 1);
        QPushButton* closeBtn = new QPushButton("Fermer");
        closeBtn->setFixedSize(100, 34);
        closeBtn->setStyleSheet(
            "QPushButton{ background:#518195; color:white; border-radius:8px;"
            " font-weight:700; font-size:12px; }"
            "QPushButton:hover{ background:#3d6475; }");
        QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);
        fl->addWidget(closeBtn);
        dl->addWidget(footer);

        dlg->exec();
    });

    picker->exec();
}

// ─────────────────────────────────────────────────────────────
//  MÉTIER AVANCÉ : Estimation Réaliste
// ─────────────────────────────────────────────────────────────
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QFrame>
#include <QStackedWidget>

void GestProjCrud::showEstimationRealiste(QWidget* parent)
{
    // ── 1. Main dialog ────────────────────────────────────────
    QDialog* dlg = new QDialog(parent,
                               Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dlg->setWindowTitle("Estimation Réaliste de Projet");
    dlg->setMinimumSize(680, 620);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setStyleSheet("QDialog{ background:#1C2A35; }");

    QVBoxLayout* mainL = new QVBoxLayout(dlg);
    mainL->setContentsMargins(22, 18, 22, 18);
    mainL->setSpacing(14);

    // ── Header ────────────────────────────────────────────────
    QFrame* header = new QFrame;
    header->setStyleSheet(
        "QFrame{ background:qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "stop:0 #2A649B,stop:1 #1A4470); border-radius:12px; }");
    QVBoxLayout* hl = new QVBoxLayout(header);
    hl->setContentsMargins(18,14,18,14); hl->setSpacing(4);
    QLabel* hTitle = new QLabel("\xf0\x9f\x93\x8a  Estimation Réaliste");
    hTitle->setStyleSheet("color:white; font-size:16px; font-weight:900; background:transparent;");
    QLabel* hSub = new QLabel("Estimez la durée et le coût d'un nouveau projet avant son démarrage");
    hSub->setStyleSheet("color:rgba(255,255,255,0.70); font-size:11px; background:transparent;");
    hl->addWidget(hTitle); hl->addWidget(hSub);
    mainL->addWidget(header);

    // ── Input form ────────────────────────────────────────────
    QFrame* formCard = new QFrame;
    formCard->setStyleSheet(
        "QFrame{ background:rgba(255,255,255,0.06); border:1px solid rgba(255,255,255,0.12);"
        " border-radius:10px; }");
    QVBoxLayout* fcL = new QVBoxLayout(formCard);
    fcL->setContentsMargins(18,14,18,14); fcL->setSpacing(12);

    QLabel* formTitle = new QLabel("  Paramètres du nouveau projet");
    formTitle->setStyleSheet("color:#a8d4e0; font-size:12px; font-weight:800; background:transparent;");
    fcL->addWidget(formTitle);

    // Domaine combo — load from DB
    QWidget* row1 = new QWidget; row1->setStyleSheet("background:transparent;");
    QHBoxLayout* r1L = new QHBoxLayout(row1);
    r1L->setContentsMargins(0,0,0,0); r1L->setSpacing(12);
    QLabel* lDom = new QLabel("Domaine de recherche :");
    lDom->setStyleSheet("color:rgba(255,255,255,0.80); font-size:11px; font-weight:700;"
                        " background:transparent; min-width:180px;");
    QComboBox* comboDom = new QComboBox;
    comboDom->setStyleSheet(
        "QComboBox{ background:rgba(255,255,255,0.10); color:white; border:1px solid rgba(255,255,255,0.20);"
        " border-radius:6px; padding:6px 10px; font-size:11px; }"
        "QComboBox::drop-down{ border:none; }"
        "QComboBox QAbstractItemView{ background:#1C2A35; color:white; selection-background-color:#2A649B; }");
    {
        QSqlQuery qDom;
        if (qDom.exec("SELECT DISTINCT TRIM(\"domaine_de_recherche\") FROM \"projet\" "
                      "WHERE \"domaine_de_recherche\" IS NOT NULL ORDER BY 1")) {
            while (qDom.next()) {
                QString d = qDom.value(0).toString().trimmed();
                if (!d.isEmpty()) comboDom->addItem(d);
            }
        }
        if (comboDom->count() == 0) {
            // fallback list
            for (const char* d : {"Biologie végétale","Génomique","Immunologie",
                                  "Neurosciences","Biotechnologies","Biochimie",
                                  "Protéomique","Microbiologie","Génétique","Pharmacologie"})
                comboDom->addItem(d);
        }
    }
    r1L->addWidget(lDom); r1L->addWidget(comboDom, 1);
    fcL->addWidget(row1);

    // Number of employees
    QWidget* row2 = new QWidget; row2->setStyleSheet("background:transparent;");
    QHBoxLayout* r2L = new QHBoxLayout(row2);
    r2L->setContentsMargins(0,0,0,0); r2L->setSpacing(12);
    QLabel* lEmp = new QLabel("Nombre d'employés prévus :");
    lEmp->setStyleSheet("color:rgba(255,255,255,0.80); font-size:11px; font-weight:700;"
                        " background:transparent; min-width:180px;");
    QSpinBox* spinEmp = new QSpinBox;
    spinEmp->setRange(1, 50); spinEmp->setValue(3);
    spinEmp->setStyleSheet(
        "QSpinBox{ background:rgba(255,255,255,0.10); color:white; border:1px solid rgba(255,255,255,0.20);"
        " border-radius:6px; padding:5px 10px; font-size:11px; }"
        "QSpinBox::up-button,QSpinBox::down-button{ background:rgba(255,255,255,0.15); border-radius:3px; }");
    r2L->addWidget(lEmp); r2L->addWidget(spinEmp, 1);
    fcL->addWidget(row2);

    // Number of planned experiences
    QWidget* row3 = new QWidget; row3->setStyleSheet("background:transparent;");
    QHBoxLayout* r3L = new QHBoxLayout(row3);
    r3L->setContentsMargins(0,0,0,0); r3L->setSpacing(12);
    QLabel* lExp = new QLabel("Nombre d'expériences prévues :");
    lExp->setStyleSheet("color:rgba(255,255,255,0.80); font-size:11px; font-weight:700;"
                        " background:transparent; min-width:180px;");
    QSpinBox* spinExp = new QSpinBox;
    spinExp->setRange(0, 200); spinExp->setValue(5);
    spinExp->setStyleSheet(spinEmp->styleSheet());
    r3L->addWidget(lExp); r3L->addWidget(spinExp, 1);
    fcL->addWidget(row3);

    // Estimate button
    QPushButton* btnEstimate = new QPushButton("\xf0\x9f\x94\x8d  Lancer l'Estimation");
    btnEstimate->setFixedHeight(38);
    btnEstimate->setStyleSheet(
        "QPushButton{ background:#2A649B; color:white; border-radius:8px;"
        " font-weight:800; font-size:12px; }"
        "QPushButton:hover{ background:#1A4470; }");
    fcL->addWidget(btnEstimate);
    mainL->addWidget(formCard);

    // ── Result area ───────────────────────────────────────────
    QFrame* resultCard = new QFrame;
    resultCard->setStyleSheet(
        "QFrame{ background:rgba(255,255,255,0.04); border:1px solid rgba(255,255,255,0.10);"
        " border-radius:10px; }");
    resultCard->setVisible(false);
    QVBoxLayout* rcL = new QVBoxLayout(resultCard);
    rcL->setContentsMargins(18,14,18,14); rcL->setSpacing(10);

    QLabel* resTitle = new QLabel("  Résultats de l'estimation");
    resTitle->setStyleSheet("color:#a8d4e0; font-size:12px; font-weight:800; background:transparent;");
    rcL->addWidget(resTitle);

    // Confidence badge
    QLabel* confLabel = new QLabel;
    confLabel->setAlignment(Qt::AlignCenter);
    confLabel->setFixedHeight(28);
    rcL->addWidget(confLabel);

    // 4 KPI pills row
    QWidget* kpiRow = new QWidget; kpiRow->setStyleSheet("background:transparent;");
    QHBoxLayout* kpiL = new QHBoxLayout(kpiRow);
    kpiL->setContentsMargins(0,0,0,0); kpiL->setSpacing(10);

    auto makeKpi = [](const QString& icon, const QString& title) -> QPair<QLabel*,QFrame*> {
        QFrame* pill = new QFrame;
        pill->setStyleSheet(
            "QFrame{ background:rgba(42,100,155,0.18); border:1px solid rgba(42,100,155,0.35);"
            " border-radius:10px; }");
        QVBoxLayout* pl = new QVBoxLayout(pill);
        pl->setContentsMargins(12,10,12,10); pl->setSpacing(3);
        QLabel* iconLbl = new QLabel(icon + "  " + title);
        iconLbl->setStyleSheet("color:rgba(168,212,224,0.80); font-size:9px; font-weight:700;"
                               " background:transparent;");
        iconLbl->setAlignment(Qt::AlignCenter);
        QLabel* valLbl = new QLabel("—");
        valLbl->setStyleSheet("color:white; font-size:14px; font-weight:900; background:transparent;");
        valLbl->setAlignment(Qt::AlignCenter);
        pl->addWidget(iconLbl); pl->addWidget(valLbl);
        return {valLbl, pill};
    };

    auto [lblDuration, pillDuration] = makeKpi("\xe2\x8f\xb1", "Durée estimée");
    auto [lblBudget,   pillBudget  ] = makeKpi("\xf0\x9f\x92\xb0", "Budget estimé");
    auto [lblSimilar,  pillSimilar ] = makeKpi("\xf0\x9f\x93\x82", "Projets similaires");
    auto [lblTeamRatio,pillTeamRatio] = makeKpi("\xf0\x9f\x91\xa5", "Ratio équipe");
    kpiL->addWidget(pillDuration,1); kpiL->addWidget(pillBudget,1);
    kpiL->addWidget(pillSimilar,1);  kpiL->addWidget(pillTeamRatio,1);
    rcL->addWidget(kpiRow);

    // Breakdown section
    QLabel* breakTitle = new QLabel("  Détail de l'estimation :");
    breakTitle->setStyleSheet("color:rgba(255,255,255,0.60); font-size:10px; font-weight:700;"
                              " background:transparent;");
    rcL->addWidget(breakTitle);

    QLabel* breakDetail = new QLabel;
    breakDetail->setWordWrap(true);
    breakDetail->setStyleSheet(
        "color:rgba(255,255,255,0.75); font-size:10px; line-height:160%;"
        " background:rgba(255,255,255,0.04); border-radius:6px; padding:8px 12px;");
    rcL->addWidget(breakDetail);

    // Similar projects list
    QLabel* simTitle = new QLabel("  Projets similaires utilisés :");
    simTitle->setStyleSheet("color:rgba(255,255,255,0.60); font-size:10px; font-weight:700;"
                            " background:transparent;");
    rcL->addWidget(simTitle);

    QLabel* simList = new QLabel;
    simList->setWordWrap(true);
    simList->setStyleSheet(
        "color:rgba(168,212,224,0.80); font-size:10px;"
        " background:rgba(255,255,255,0.04); border-radius:6px; padding:8px 12px;");
    rcL->addWidget(simList);

    mainL->addWidget(resultCard, 1);

    // ── Close button ──────────────────────────────────────────
    QFrame* footer = new QFrame;
    footer->setStyleSheet("QFrame{ background:rgba(255,255,255,0.04); border-radius:8px; }");
    QHBoxLayout* fl = new QHBoxLayout(footer);
    fl->setContentsMargins(12,8,12,8);
    QLabel* note = new QLabel(
        "\xe2\x84\xb9  L'estimation est basée sur les projets terminés de votre base de données.");
    note->setStyleSheet("color:rgba(255,255,255,0.35); font-size:9px; background:transparent;");
    fl->addWidget(note,1);
    QPushButton* closeBtn = new QPushButton("Fermer");
    closeBtn->setFixedSize(90,32);
    closeBtn->setStyleSheet(
        "QPushButton{ background:#2A649B; color:white; border-radius:7px; font-weight:700; }"
        "QPushButton:hover{ background:#1A4470; }");
    QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);
    fl->addWidget(closeBtn);
    mainL->addWidget(footer);

    // ── Estimation logic (triggered on button click) ──────────
    QObject::connect(btnEstimate, &QPushButton::clicked, dlg,
                     [=]() mutable {
                         const QString domaine   = comboDom->currentText().trimmed();
                         const int     nbEmp     = spinEmp->value();
                         const int     nbExpPrev = spinExp->value();
                         // ── STEP 1: Find similar completed projects ────────────
                         struct SimilarProj {
                             QString nom;
                             double  durationMonths;
                             double  budget;
                             double  avgTeam;
                         };
                         QList<SimilarProj> similar;

                         {
                             QSqlQuery q;
                             // Use ASCII column names only; "date_de_debut" stored with accent
                             QString simSql = QString(
                                                  "SELECT p.\"Id_projet\", TRIM(p.\"nom_du_projet\"),"
                                                  " p.\"%1\", p.\"date_de_fin\", p.\"budget\" "
                                                  "FROM \"projet\" p "
                                                  "WHERE LOWER(NVL(TRIM(p.\"statut\"),'')) IN ('%2','termine') "
                                                  "AND TRIM(p.\"domaine_de_recherche\") = :dom")
                                                  .arg(QString::fromUtf8("date_de_d\xc3\xa9" "but"))
                                                  .arg(QString::fromUtf8("termin\xc3\xa9"));
                             q.prepare(simSql);
                             q.bindValue(":dom", domaine);
                             q.exec();

                             while (q.next()) {
                                 int     pid  = q.value(0).toInt();
                                 QString nom  = q.value(1).toString().trimmed();
                                 QDate   deb  = q.value(2).toDate();
                                 QDate   fin  = q.value(3).toDate();
                                 double  bud  = q.value(4).toDouble();

                                 double durMo = 1.0;
                                 if (deb.isValid() && fin.isValid() && fin > deb)
                                     durMo = deb.daysTo(fin) / 30.4375;

                                 // avg team size for this project
                                 double teamSz = 1.0;
                                 QSqlQuery qt;
                                 qt.prepare("SELECT COUNT(*) FROM \"Associer\" WHERE \"Id_projet\" = :pid");
                                 qt.bindValue(":pid", pid);
                                 if (qt.exec() && qt.next()) teamSz = qMax(1.0, (double)qt.value(0).toInt());

                                 if (bud > 0) similar.append({nom, durMo, bud, teamSz});
                             }
                         }

                         // ── STEP 2 / STEP 3: Compute estimation ───────────────
                         double estDurationMonths = 0.0;
                         double estBudget         = 0.0;
                         QString method, confidence;

                         if (similar.size() >= 3) {
                             // Data-driven path
                             double sumDur = 0, sumBud = 0, sumTeam = 0;
                             for (auto& s : similar) { sumDur += s.durationMonths; sumBud += s.budget; sumTeam += s.avgTeam; }
                             double avgDur  = sumDur  / similar.size();
                             double avgBud  = sumBud  / similar.size();
                             double avgTeam = sumTeam / similar.size();

                             double teamRatio = nbEmp / qMax(1.0, avgTeam);
                             // More people = slightly faster (factor 0.3) but more expensive (×0.8 efficiency)
                             estDurationMonths = avgDur / (1.0 + (teamRatio - 1.0) * 0.3);
                             estDurationMonths = qMax(1.0, estDurationMonths);
                             estBudget         = avgBud * teamRatio * 0.8;

                             method = QString("Basé sur %1 projets terminés similaires (domaine : %2)")
                                          .arg(similar.size()).arg(domaine);
                             if      (similar.size() >= 5) confidence = "Élevée";
                             else if (similar.size() >= 2) confidence = "Moyenne";
                             else                          confidence = "Faible";
                         } else {
                             // Formula fallback (budget algorithm approximation)
                             // salary cost: nbEmp × avg_salary(2000/mo) × duration (unknown → iterate)
                             // Approximate: budget = nbEmp*2000*dur + nbExpPrev*425; dur = budget/15000*1
                             // Solve: let D = duration in months
                             // budget = nbEmp*2000*D + nbExpPrev*425
                             // D = budget/15000  → budget = nbEmp*2000*(budget/15000) + nbExpPrev*425
                             // Simplify with overhead 15%:
                             double expCost  = nbExpPrev * 425.0;
                             // duration estimate: base 3 months + 1 month per employee pair
                             estDurationMonths = qMax(3.0, 3.0 + nbEmp * 0.5);
                             double empCost  = nbEmp * 2000.0 * estDurationMonths;
                             double subtotal = empCost + expCost;
                             estBudget       = subtotal * 1.15; // overhead
                             // Refine duration from budget: every 15k TND ≈ 1 month
                             estDurationMonths = qMax(estDurationMonths, estBudget / 15000.0);

                             method = QString("Formule interne (données insuffisantes pour domaine : %1)").arg(domaine);
                             confidence = "Faible (basée sur formule)";
                         }

                         // Confidence color
                         QString confColor, confBg;
                         if (confidence.startsWith("Élevée")) { confColor="#1A6B60"; confBg="rgba(46,139,124,0.20)"; }
                         else if (confidence.startsWith("Moyenne")) { confColor="#8A6A00"; confBg="rgba(212,167,42,0.20)"; }
                         else { confColor="#8B2F3C"; confBg="rgba(139,47,60,0.20)"; }

                         // Team ratio display
                         double avgTeamDisplay = 1.0;
                         if (!similar.isEmpty()) {
                             double st = 0; for (auto& s:similar) st+=s.avgTeam;
                             avgTeamDisplay = st / similar.size();
                         }
                         double teamRatioDisplay = nbEmp / qMax(1.0, avgTeamDisplay);

                         // Format numbers
                         auto fmtMo = [](double m) -> QString {
                             int mo = (int)qRound(m);
                             int yr = mo / 12; int rem = mo % 12;
                             if (yr > 0 && rem > 0) return QString("%1 an(s) %2 mois").arg(yr).arg(rem);
                             if (yr > 0)            return QString("%1 an(s)").arg(yr);
                             return QString("%1 mois").arg(mo);
                         };
                         auto fmtBud = [](double v) -> QString {
                             if (v >= 1e6) return QString::number(v/1e6,'f',2)+" M TND";
                             if (v >= 1e3) return QString::number(v/1e3,'f',1)+" k TND";
                             return QString::number(v,'f',0)+" TND";
                         };

                         // ── Update UI ──────────────────────────────────────────
                         confLabel->setText(QString("  Niveau de confiance : %1  ").arg(confidence));
                         confLabel->setStyleSheet(QString(
                                                      "color:%1; background:%2; border-radius:6px; font-size:11px; font-weight:800;")
                                                      .arg(confColor).arg(confBg));

                         lblDuration->setText(fmtMo(estDurationMonths));
                         lblBudget->setText(fmtBud(estBudget));
                         lblSimilar->setText(QString::number(similar.size()));
                         lblTeamRatio->setText(QString::number(teamRatioDisplay,'f',2)+"×");

                         // Breakdown detail
                         QString detailTxt;
                         if (similar.size() >= 3) {
                             double sumDur=0,sumBud=0,sumTeam=0;
                             for (auto& s:similar){sumDur+=s.durationMonths;sumBud+=s.budget;sumTeam+=s.avgTeam;}
                             double avgD = sumDur/similar.size(), avgB = sumBud/similar.size(), avgT = sumTeam/similar.size();
                             detailTxt = QString(
                                             "  Durée moyenne des projets similaires : %1\n"
                                             "  Budget moyen des projets similaires  : %2\n"
                                             "  Taille d'équipe moyenne              : %3 personnes\n"
                                             "  Ratio équipe (vous / moyenne)        : %4×\n"
                                             "  Ajustement durée (ratio×0.3)         : ÷ %5\n"
                                             "  Ajustement budget (ratio×0.8)        : × %6\n"
                                             "  Méthode : %7")
                                             .arg(fmtMo(avgD))
                                             .arg(fmtBud(avgB))
                                             .arg(QString::number(avgT,'f',1))
                                             .arg(QString::number(teamRatioDisplay,'f',2))
                                             .arg(QString::number(1.0+(teamRatioDisplay-1.0)*0.3,'f',3))
                                             .arg(QString::number(teamRatioDisplay*0.8,'f',3))
                                             .arg(method);
                         } else {
                             detailTxt = QString(
                                             "  Coût employés (%1 × 2000 TND/mois × %2)  : %3\n"
                                             "  Coût expériences (%4 × 425 TND)           : %5\n"
                                             "  Overhead 15%                               : inclus\n"
                                             "  Méthode : %6")
                                             .arg(nbEmp)
                                             .arg(fmtMo(estDurationMonths))
                                             .arg(fmtBud(nbEmp*2000.0*estDurationMonths))
                                             .arg(nbExpPrev)
                                             .arg(fmtBud(nbExpPrev*425.0))
                                             .arg(method);
                         }
                         breakDetail->setText(detailTxt);

                         // Similar project list
                         if (similar.isEmpty()) {
                             simTitle->setVisible(false);
                             simList->setText("Aucun projet terminé trouvé pour ce domaine.");
                         } else {
                             simTitle->setVisible(true);
                             QStringList rows;
                             for (auto& s : similar)
                                 rows << QString("  • %1   |   %2   |   %3   |   équipe: %4")
                                             .arg(s.nom.leftJustified(28))
                                             .arg(fmtMo(s.durationMonths).leftJustified(12))
                                             .arg(fmtBud(s.budget).leftJustified(16))
                                             .arg(QString::number((int)qRound(s.avgTeam)));
                             simList->setText(rows.join("\n"));
                         }

                         resultCard->setVisible(true);
                         dlg->adjustSize();
                         if (dlg->height() < 620) dlg->resize(dlg->width(), 680);
                     });

    dlg->exec();
}


// ─────────────────────────────────────────────────────────────
//  MÉTIER AVANCÉ : Collaborateurs Suggérés
//  Includes AI-powered sponsor search via Groq (no extra API needed)
// ─────────────────────────────────────────────────────────────
#include "apiconfig.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

void GestProjCrud::showCollaborateursSuggeres(QWidget* parent)
{
    // ── 1. Project picker ─────────────────────────────────────
    QDialog* picker = new QDialog(parent,
        Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    picker->setWindowTitle("Collaborateurs Suggeres - Choisir un projet");
    picker->setMinimumSize(500, 400);
    picker->setAttribute(Qt::WA_DeleteOnClose);
    picker->setStyleSheet("QDialog{ background:#1C2A35; }");

    QVBoxLayout* pl = new QVBoxLayout(picker);
    pl->setContentsMargins(20,18,20,16); pl->setSpacing(12);

    QLabel* plTitle = new QLabel("  Selectionnez le projet a analyser");
    plTitle->setStyleSheet(
        "color:#a8d4e0; font-size:13px; font-weight:900;"
        "background:rgba(42,100,155,0.15); border-radius:10px; padding:7px 14px;");
    pl->addWidget(plTitle);

    QListWidget* projList = new QListWidget;
    projList->setStyleSheet(
        "QListWidget{ background:rgba(255,255,255,0.08); border-radius:10px;"
        " border:1px solid rgba(255,255,255,0.15); }"
        "QListWidget::item{ padding:9px 14px; color:rgba(255,255,255,0.85);"
        " font-weight:600; font-size:11px; }"
        "QListWidget::item:selected{ background:#2A649B; color:white; border-radius:6px; }"
        "QListWidget::item:hover:!selected{ background:rgba(255,255,255,0.08); }");

    GestProjCrud crud;
    QList<ProjetRecord> allProjs; QString perr;
    crud.loadProjets(allProjs, &perr);
    for (const ProjetRecord& pr : allProjs) {
        QListWidgetItem* item = new QListWidgetItem(
            QString("%1  [%2]").arg(pr.nomDuProjet).arg(pr.domaineDeRecherche));
        item->setData(Qt::UserRole,     pr.idProjet);
        item->setData(Qt::UserRole + 1, pr.nomDuProjet);
        item->setData(Qt::UserRole + 2, pr.domaineDeRecherche);
        projList->addItem(item);
    }
    pl->addWidget(projList, 1);

    QHBoxLayout* pbl = new QHBoxLayout; pbl->setSpacing(10);
    QPushButton* cancelBtn = new QPushButton("Annuler");
    cancelBtn->setFixedHeight(36);
    cancelBtn->setStyleSheet(
        "QPushButton{ background:rgba(255,255,255,0.12); color:rgba(255,255,255,0.80);"
        " border-radius:8px; font-weight:700; font-size:12px; padding:0 16px; }"
        "QPushButton:hover{ background:rgba(255,255,255,0.20); }");
    QPushButton* goBtn = new QPushButton("Voir les suggestions");
    goBtn->setFixedHeight(36); goBtn->setEnabled(false);
    goBtn->setStyleSheet(
        "QPushButton{ background:#2A649B; color:white; border-radius:8px;"
        " font-weight:800; font-size:12px; padding:0 16px; }"
        "QPushButton:hover{ background:#1A4470; }"
        "QPushButton:disabled{ background:rgba(42,100,155,0.35);"
        " color:rgba(255,255,255,0.40); }");
    pbl->addWidget(cancelBtn); pbl->addStretch(1); pbl->addWidget(goBtn);
    pl->addLayout(pbl);

    QObject::connect(cancelBtn, &QPushButton::clicked, picker, &QDialog::reject);
    QObject::connect(projList, &QListWidget::itemSelectionChanged, picker,
        [=](){ goBtn->setEnabled(projList->currentItem() != nullptr); });
    QObject::connect(projList, &QListWidget::itemDoubleClicked,
                     goBtn, &QPushButton::click);

    QObject::connect(goBtn, &QPushButton::clicked, picker, [=]() mutable {
        QListWidgetItem* sel = projList->currentItem();
        if (!sel) return;

        const int     projId  = sel->data(Qt::UserRole).toInt();
        const QString projNom = sel->data(Qt::UserRole + 1).toString();
        const QString domaine = sel->data(Qt::UserRole + 2).toString().trimmed();

        picker->hide();

        // ── 2. Results dialog ─────────────────────────────────
        QDialog* dlg = new QDialog(parent,
            Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
        dlg->setWindowTitle("Collaborateurs Suggeres");
        dlg->setMinimumSize(820, 680);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->setStyleSheet("QDialog{ background:#1C2A35; }");

        QVBoxLayout* mainL = new QVBoxLayout(dlg);
        mainL->setContentsMargins(22,18,22,18); mainL->setSpacing(14);

        // Header
        QFrame* hdr = new QFrame;
        hdr->setStyleSheet(
            "QFrame{ background:qlineargradient(x1:0,y1:0,x2:1,y2:0,"
            "stop:0 #2A649B,stop:1 #1A4470); border-radius:12px; }");
        QVBoxLayout* hL = new QVBoxLayout(hdr);
        hL->setContentsMargins(18,14,18,14); hL->setSpacing(4);
        QLabel* hTitle = new QLabel(
            QString("Collaborateurs Suggeres pour \"%1\"").arg(projNom));
        hTitle->setStyleSheet(
            "color:white; font-size:15px; font-weight:900; background:transparent;");
        QLabel* hSub = new QLabel(
            QString("Domaine : %1").arg(domaine.isEmpty() ? "non specifie" : domaine));
        hSub->setStyleSheet(
            "color:rgba(255,255,255,0.70); font-size:11px; background:transparent;");
        hL->addWidget(hTitle); hL->addWidget(hSub);
        mainL->addWidget(hdr);

        // Scroll area
        QScrollArea* scroll = new QScrollArea;
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setStyleSheet(
            "QScrollArea{ background:transparent; border:none; }"
            "QScrollBar:vertical{ width:7px; background:transparent; }"
            "QScrollBar::handle:vertical{"
            " background:rgba(42,100,155,0.40); border-radius:4px; }");
        QWidget* scrollW = new QWidget;
        scrollW->setStyleSheet("background:transparent;");
        QVBoxLayout* scrollL = new QVBoxLayout(scrollW);
        scrollL->setContentsMargins(0,0,4,0); scrollL->setSpacing(14);

        // Section factory
        auto makeSection = [](const QString& title, const QString& color)
            -> QPair<QFrame*,QVBoxLayout*>
        {
            QFrame* sec = new QFrame;
            sec->setStyleSheet(QString(
                "QFrame{ background:rgba(255,255,255,0.05);"
                " border:1px solid rgba(255,255,255,0.10);"
                " border-left:4px solid %1; border-radius:10px; }").arg(color));
            QVBoxLayout* sL = new QVBoxLayout(sec);
            sL->setContentsMargins(16,12,16,12); sL->setSpacing(8);
            QLabel* t = new QLabel(title);
            t->setStyleSheet(QString(
                "color:%1; font-size:12px; font-weight:900;"
                " background:transparent;").arg(color));
            sL->addWidget(t);
            return {sec, sL};
        };

        // ═════════════════════════════════════════════════════
        //  PART A - Internal employee suggestions
        // ═════════════════════════════════════════════════════
        struct EmpSugg {
            int     id = 0;
            QString fullName, role, specialization;
            int     activeProjects = 0;
            int     score = 0, specScore = 0, workScore = 0, roleScore = 0;
        };
        QList<EmpSugg> suggestions;

        // Roles already present in this project
        QSet<QString> existingRoles;
        {
            QSqlQuery qR;
            QString et = QString::fromUtf8("Employ\xc3\xa9s");
            qR.prepare(QString(
                "SELECT NVL(TRIM(emp.\"ROLE\"),'')"
                " FROM \"Associer\" a"
                " JOIN \"%1\" emp ON a.\"employee_id\" = emp.\"employee_id\""
                " WHERE a.\"Id_projet\" = :pid").arg(et));
            qR.bindValue(":pid", projId);
            if (qR.exec()) while (qR.next())
                existingRoles.insert(qR.value(0).toString().trimmed().toLower());
        }
        bool needsResponsable = !existingRoles.contains("responsable");
        bool needsChercheur   = !existingRoles.contains("chercheur");

        // All active employees not already assigned
        {
            QSqlQuery qE;
            QString et = QString::fromUtf8("Employ\xc3\xa9s");
            qE.prepare(QString(
                "SELECT emp.\"employee_id\","
                " NVL(emp.\"FULL_NAME\", TRIM(emp.\"prenom\" || ' ' || emp.\"nom\")),"
                " NVL(TRIM(emp.\"ROLE\"),'Technicien'),"
                " NVL(TRIM(emp.\"specialization\"),'')"
                " FROM \"%1\" emp"
                " WHERE NVL(emp.\"ACTIVE\",'O') = 'O'"
                " AND NOT EXISTS ("
                "  SELECT 1 FROM \"Associer\" a2"
                "  WHERE a2.\"employee_id\" = emp.\"employee_id\""
                "  AND a2.\"Id_projet\" = :pid)").arg(et));
            qE.bindValue(":pid", projId);
            if (qE.exec()) {
                while (qE.next()) {
                    EmpSugg s;
                    s.id             = qE.value(0).toInt();
                    s.fullName       = qE.value(1).toString().trimmed();
                    if (s.fullName.isEmpty()) s.fullName = "(Sans nom)";
                    s.role           = qE.value(2).toString().trimmed();
                    s.specialization = qE.value(3).toString().trimmed();

                    QSqlQuery qa;
                    qa.prepare(
                        "SELECT COUNT(*) FROM \"Associer\" a"
                        " JOIN \"projet\" p ON a.\"Id_projet\" = p.\"Id_projet\""
                        " WHERE a.\"employee_id\" = :eid"
                        " AND LOWER(NVL(TRIM(p.\"statut\"),'')) = 'en cours'");
                    qa.bindValue(":eid", s.id);
                    s.activeProjects = (qa.exec() && qa.next()) ? qa.value(0).toInt() : 0;

                    QString sl = s.specialization.toLower();
                    QString dl = domaine.toLower();
                    if (!sl.isEmpty() && !dl.isEmpty() && sl == dl)       s.specScore = 40;
                    else if (!sl.isEmpty() && !dl.isEmpty() &&
                             (sl.contains(dl.left(4)) || dl.contains(sl.left(4))))
                                                                           s.specScore = 20;
                    else                                                   s.specScore = 0;

                    if      (s.activeProjects == 0) s.workScore = 35;
                    else if (s.activeProjects == 1) s.workScore = 25;
                    else if (s.activeProjects == 2) s.workScore = 15;
                    else                             s.workScore = 0;

                    QString rl = s.role.toLower();
                    if      (needsResponsable && rl == "responsable") s.roleScore = 25;
                    else if (needsChercheur   && rl == "chercheur")   s.roleScore = 25;
                    else if (rl == "technicien")                       s.roleScore = 15;
                    else                                               s.roleScore = 5;

                    s.score = s.specScore + s.workScore + s.roleScore;
                    suggestions.append(s);
                }
            }
        }
        std::sort(suggestions.begin(), suggestions.end(),
            [](const EmpSugg& a, const EmpSugg& b){ return a.score > b.score; });
        if (suggestions.size() > 5) suggestions = suggestions.mid(0, 5);

        auto [secEmp, secEmpL] = makeSection("Employes Internes Suggeres", "#2A649B");

        if (suggestions.isEmpty()) {
            QLabel* nd = new QLabel("Aucun employe disponible non affecte a ce projet.");
            nd->setStyleSheet(
                "color:rgba(255,255,255,0.50); font-size:11px; background:transparent;");
            secEmpL->addWidget(nd);
        } else {
            for (int rank = 0; rank < suggestions.size(); ++rank) {
                const EmpSugg& s = suggestions[rank];
                QFrame* row = new QFrame;
                row->setStyleSheet(
                    "QFrame{ background:rgba(255,255,255,0.05); border-radius:8px;"
                    " border:1px solid rgba(255,255,255,0.08); }");
                QHBoxLayout* rowL = new QHBoxLayout(row);
                rowL->setContentsMargins(14,10,14,10); rowL->setSpacing(12);

                QLabel* rk = new QLabel(QString::number(rank + 1));
                rk->setFixedSize(28,28); rk->setAlignment(Qt::AlignCenter);
                rk->setStyleSheet(
                    "background:#2A649B; color:white; border-radius:14px;"
                    " font-size:12px; font-weight:900;");
                rowL->addWidget(rk);

                QWidget* nb = new QWidget; nb->setStyleSheet("background:transparent;");
                QVBoxLayout* nbL = new QVBoxLayout(nb);
                nbL->setContentsMargins(0,0,0,0); nbL->setSpacing(2);
                QLabel* nm = new QLabel(s.fullName);
                nm->setStyleSheet(
                    "color:white; font-size:12px; font-weight:800; background:transparent;");
                QString roleSpec = s.role;
                if (!s.specialization.isEmpty())
                    roleSpec += "  -  " + s.specialization;
                QLabel* rl2 = new QLabel(roleSpec);
                rl2->setStyleSheet(
                    "color:rgba(168,212,224,0.80); font-size:10px;"
                    " font-weight:600; background:transparent;");
                nbL->addWidget(nm); nbL->addWidget(rl2);
                rowL->addWidget(nb, 1);

                QString sc = s.score >= 70 ? "#2E8B7C"
                           : s.score >= 40 ? "#D4762A" : "#8B2F3C";
                QWidget* sbW = new QWidget; sbW->setStyleSheet("background:transparent;");
                QVBoxLayout* sbL2 = new QVBoxLayout(sbW);
                sbL2->setContentsMargins(0,0,0,0); sbL2->setSpacing(3);
                QLabel* sl2 = new QLabel(QString("Score : %1%").arg(s.score));
                sl2->setAlignment(Qt::AlignRight);
                sl2->setStyleSheet(QString(
                    "color:%1; font-size:11px; font-weight:900; background:transparent;")
                    .arg(sc));
                QWidget* barBg = new QWidget; barBg->setFixedSize(120,6);
                barBg->setStyleSheet("background:rgba(255,255,255,0.12); border-radius:3px;");
                QWidget* barFill = new QWidget(barBg);
                barFill->setFixedHeight(6);
                barFill->setStyleSheet(
                    QString("background:%1; border-radius:3px;").arg(sc));
                int fw = (int)(s.score / 100.0 * 120);
                QTimer::singleShot(0, barBg, [barFill, fw](){ barFill->setFixedWidth(fw); });
                sbL2->addWidget(sl2); sbL2->addWidget(barBg, 0, Qt::AlignRight);
                rowL->addWidget(sbW);

                QString avBg, avFg, avTxt;
                if (s.activeProjects == 0) {
                    avBg="rgba(46,139,124,0.25)"; avFg="#2E8B7C"; avTxt="Disponible";
                } else if (s.activeProjects == 1) {
                    avBg="rgba(212,118,42,0.25)"; avFg="#D4762A"; avTxt="1 projet actif";
                } else {
                    avBg="rgba(139,47,60,0.25)"; avFg="#8B2F3C";
                    avTxt=QString("%1 projets actifs").arg(s.activeProjects);
                }
                QLabel* av = new QLabel(avTxt);
                av->setStyleSheet(QString(
                    "color:%1; background:%2; border-radius:6px;"
                    " font-size:10px; font-weight:800; padding:3px 8px;")
                    .arg(avFg).arg(avBg));
                rowL->addWidget(av);
                secEmpL->addWidget(row);
            }
            QLabel* leg = new QLabel("  Score = Specialisation (40%) + Charge (35%) + Role (25%)");
            leg->setStyleSheet("color:rgba(255,255,255,0.35); font-size:9px; background:transparent;");
            secEmpL->addWidget(leg);
        }
        scrollL->addWidget(secEmp);

        // ═════════════════════════════════════════════════════
        //  Cross-project suggestions (same domain, completed)
        // ═════════════════════════════════════════════════════
        if (!domaine.isEmpty()) {
            QList<EmpSugg> cross;
            QSqlQuery qC;
            QString et2    = QString::fromUtf8("Employ\xc3\xa9s");
            QString termSt = QString::fromUtf8("termin\xc3\xa9");
            qC.prepare(QString(
                "SELECT DISTINCT emp.\"employee_id\","
                " NVL(emp.\"FULL_NAME\", TRIM(emp.\"prenom\" || ' ' || emp.\"nom\")),"
                " NVL(TRIM(emp.\"ROLE\"),'Technicien'),"
                " NVL(TRIM(emp.\"specialization\"),'')"
                " FROM \"%1\" emp"
                " JOIN \"Associer\" a ON a.\"employee_id\" = emp.\"employee_id\""
                " JOIN \"projet\" p ON a.\"Id_projet\" = p.\"Id_projet\""
                " WHERE TRIM(p.\"domaine_de_recherche\") = :dom"
                " AND LOWER(NVL(TRIM(p.\"statut\"),'')) IN ('%2','termine')"
                " AND p.\"Id_projet\" <> :pid2"
                " AND NVL(emp.\"ACTIVE\",'O') = 'O'"
                " AND NOT EXISTS ("
                "  SELECT 1 FROM \"Associer\" ax"
                "  WHERE ax.\"employee_id\" = emp.\"employee_id\""
                "  AND ax.\"Id_projet\" = :pid3)")
                .arg(et2).arg(termSt));
            qC.bindValue(":dom",  domaine);
            qC.bindValue(":pid2", projId);
            qC.bindValue(":pid3", projId);
            if (qC.exec()) {
                while (qC.next()) {
                    EmpSugg s;
                    s.id             = qC.value(0).toInt();
                    s.fullName       = qC.value(1).toString().trimmed();
                    if (s.fullName.isEmpty()) s.fullName = "(Sans nom)";
                    s.role           = qC.value(2).toString().trimmed();
                    s.specialization = qC.value(3).toString().trimmed();
                    QSqlQuery qa2;
                    qa2.prepare(
                        "SELECT COUNT(*) FROM \"Associer\" a"
                        " JOIN \"projet\" p ON a.\"Id_projet\" = p.\"Id_projet\""
                        " WHERE a.\"employee_id\" = :eid"
                        " AND LOWER(NVL(TRIM(p.\"statut\"),'')) = 'en cours'");
                    qa2.bindValue(":eid", s.id);
                    s.activeProjects = (qa2.exec() && qa2.next()) ? qa2.value(0).toInt() : 0;
                    if (s.activeProjects < 2) {
                        s.score = 65 - s.activeProjects * 10;
                        cross.append(s);
                    }
                }
            }
            std::sort(cross.begin(), cross.end(),
                [](const EmpSugg& a, const EmpSugg& b){ return a.score > b.score; });
            if (cross.size() > 3) cross = cross.mid(0, 3);

            if (!cross.isEmpty()) {
                auto [secC, secCL] = makeSection(
                    "Experience Terrain - Projets Similaires Termines", "#0A5F58");
                for (const EmpSugg& s : cross) {
                    QFrame* rw = new QFrame;
                    rw->setStyleSheet(
                        "QFrame{ background:rgba(10,95,88,0.10); border-radius:8px;"
                        " border:1px solid rgba(10,95,88,0.25); }");
                    QHBoxLayout* rwL = new QHBoxLayout(rw);
                    rwL->setContentsMargins(14,8,14,8); rwL->setSpacing(10);
                    QLabel* nm2 = new QLabel(
                        QString("%1  -  %2  -  %3")
                            .arg(s.fullName).arg(s.role)
                            .arg(s.specialization.isEmpty()
                                 ? "specialisation non renseignee"
                                 : s.specialization));
                    nm2->setStyleSheet(
                        "color:rgba(255,255,255,0.85); font-size:11px;"
                        " font-weight:700; background:transparent;");
                    rwL->addWidget(nm2, 1);
                    QLabel* av2 = new QLabel(
                        s.activeProjects == 0 ? "Libre" : "Partiellement occupe");
                    av2->setStyleSheet(
                        "color:rgba(168,212,224,0.80); font-size:10px; background:transparent;");
                    rwL->addWidget(av2);
                    secCL->addWidget(rw);
                }
                scrollL->addWidget(secC);
            }
        }

        // ═════════════════════════════════════════════════════
        //  PART B - AI-Powered Sponsor Search via Groq
        // ═════════════════════════════════════════════════════
        auto [secSp, secSpL] = makeSection(
            "Partenaires & Sponsors - Recherche IA (Groq)", "#B5672C");

        // Loading indicator
        QLabel* aiStatus = new QLabel("Recherche IA en cours...");
        aiStatus->setAlignment(Qt::AlignCenter);
        aiStatus->setStyleSheet(
            "color:#D4762A; font-size:11px; font-weight:700;"
            " background:rgba(212,118,42,0.10); border-radius:6px; padding:8px;");
        secSpL->addWidget(aiStatus);

        // Container for AI results injected dynamically
        QWidget* aiResultsW = new QWidget;
        aiResultsW->setStyleSheet("background:transparent;");
        QVBoxLayout* aiResultsL = new QVBoxLayout(aiResultsW);
        aiResultsL->setContentsMargins(0,0,0,0); aiResultsL->setSpacing(6);
        secSpL->addWidget(aiResultsW);

        // Retry button (shown on error)
        QPushButton* retryBtn = new QPushButton("Relancer la recherche IA");
        retryBtn->setVisible(false);
        retryBtn->setFixedHeight(32);
        retryBtn->setStyleSheet(
            "QPushButton{ background:rgba(181,103,44,0.30); color:#D4762A;"
            " border-radius:6px; font-weight:700; font-size:10px; }"
            "QPushButton:hover{ background:rgba(181,103,44,0.50); }");
        secSpL->addWidget(retryBtn);

        // AI attribution badge
        QLabel* aiBadge = new QLabel(
            "Suggestions generees par IA - basees sur les connaissances mondiales");
        aiBadge->setStyleSheet(
            "color:rgba(255,255,255,0.25); font-size:9px; background:transparent;");
        aiBadge->setVisible(false);
        secSpL->addWidget(aiBadge);

        scrollL->addWidget(secSp);
        scrollL->addStretch(1);
        scroll->setWidget(scrollW);
        mainL->addWidget(scroll, 1);

        // Footer
        QFrame* foot = new QFrame;
        foot->setStyleSheet("QFrame{ background:rgba(255,255,255,0.04); border-radius:8px; }");
        QHBoxLayout* fl = new QHBoxLayout(foot);
        fl->setContentsMargins(12,8,12,8);
        QLabel* note = new QLabel("Score : Specialisation (40%) + Charge (35%) + Role (25%)");
        note->setStyleSheet(
            "color:rgba(255,255,255,0.30); font-size:9px; background:transparent;");
        fl->addWidget(note, 1);
        QPushButton* cb = new QPushButton("Fermer");
        cb->setFixedSize(90,32);
        cb->setStyleSheet(
            "QPushButton{ background:#2A649B; color:white; border-radius:7px;"
            " font-weight:700; }"
            "QPushButton:hover{ background:#1A4470; }");
        QObject::connect(cb, &QPushButton::clicked, dlg, &QDialog::accept);
        fl->addWidget(cb);
        mainL->addWidget(foot);

        // ── AI sponsor search launcher ────────────────────────
        auto launchAiSearch = [=]() mutable {
            // Clear old results
            while (QLayoutItem* it = aiResultsL->takeAt(0)) {
                delete it->widget(); delete it;
            }
            aiStatus->setVisible(true);
            aiStatus->setText("Recherche IA en cours...");
            aiStatus->setStyleSheet(
                "color:#D4762A; font-size:11px; font-weight:700;"
                " background:rgba(212,118,42,0.10); border-radius:6px; padding:8px;");
            retryBtn->setVisible(false);
            aiBadge->setVisible(false);

            QNetworkAccessManager* net = new QNetworkAccessManager(dlg);

            // Prompt: ask for real worldwide sponsors for this research domain
            QString domForPrompt = domaine.isEmpty()
                ? "recherche biomedicale" : domaine;
            QString prompt =
                "Tu es un expert mondial en financement de la recherche scientifique. "
                "Pour un projet de recherche dans le domaine \"" + domForPrompt + "\", "
                "donne exactement 5 sponsors, fondations ou organismes de financement "
                "reels et reconnus dans le monde qui seraient les plus susceptibles "
                "de financer ce type de projet. "
                "Pour chacun, donne : le nom exact, le pays ou region, et une phrase courte "
                "expliquant pourquoi ils soutiennent ce domaine. "
                "Format STRICT - 5 lignes, une par entree :\n"
                "NOM | PAYS | RAISON\n"
                "Reponds UNIQUEMENT avec les 5 lignes au format demande, rien d'autre.";

            QJsonObject body;
            body["model"]       = QString(GROQ_API_MODEL);
            body["max_tokens"]  = 400;
            body["temperature"] = 0.3;
            body["messages"] = QJsonArray{
                QJsonObject{
                    {"role","system"},
                    {"content",
                     "Tu es un expert en financement de la recherche scientifique mondiale. "
                     "Reponds uniquement au format demande : NOM | PAYS | RAISON"}},
                QJsonObject{{"role","user"}, {"content", prompt}}
            };

            QUrl url = QUrl(QString(GROQ_API_URL));
            QNetworkRequest req(url);
            req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
            req.setRawHeader("Authorization",
                             ("Bearer " + QString(GROQ_API_KEY)).toUtf8());
            QSslConfiguration ssl = QSslConfiguration::defaultConfiguration();
            ssl.setPeerVerifyMode(QSslSocket::VerifyNone);
            req.setSslConfiguration(ssl);

            QNetworkReply* reply = net->post(req, QJsonDocument(body).toJson());
            QObject::connect(reply, &QNetworkReply::sslErrors, reply,
                [reply](const QList<QSslError>&){ reply->ignoreSslErrors(); });

            QObject::connect(reply, &QNetworkReply::finished, dlg, [=]() mutable {
                reply->deleteLater();
                net->deleteLater();
                aiStatus->setVisible(false);

                QByteArray data = reply->readAll();
                QJsonObject root = QJsonDocument::fromJson(data).object();

                if (root.contains("error") ||
                    reply->error() != QNetworkReply::NoError) {
                    aiStatus->setVisible(true);
                    aiStatus->setText(
                        "IA indisponible - verifiez votre connexion internet.");
                    aiStatus->setStyleSheet(
                        "color:#8B2F3C; font-size:11px; font-weight:700;"
                        " background:rgba(139,47,60,0.10); border-radius:6px; padding:8px;");
                    retryBtn->setVisible(true);
                    return;
                }

                QString raw = root["choices"].toArray().first()
                    .toObject()["message"].toObject()["content"]
                    .toString().trimmed();

                if (raw.isEmpty()) {
                    aiStatus->setVisible(true);
                    aiStatus->setText("Aucun resultat retourne par l'IA.");
                    retryBtn->setVisible(true);
                    return;
                }

                // Parse "NOM | PAYS | RAISON" lines
                QStringList lines = raw.split('\n', Qt::SkipEmptyParts);
                int added = 0;
                for (const QString& line : lines) {
                    if (line.trimmed().startsWith("NOM")) continue; // skip header
                    QStringList parts = line.split('|');
                    if (parts.size() < 1) continue;
                    QString name   = parts[0].trimmed();
                    QString region = parts.size() >= 2 ? parts[1].trimmed() : "";
                    QString reason = parts.size() >= 3 ? parts[2].trimmed() : "";
                    if (name.isEmpty()) continue;

                    QFrame* spR = new QFrame;
                    spR->setStyleSheet(
                        "QFrame{ background:rgba(181,103,44,0.10); border-radius:8px;"
                        " border:1px solid rgba(181,103,44,0.25); }");
                    QHBoxLayout* spRow = new QHBoxLayout(spR);
                    spRow->setContentsMargins(14,10,14,10); spRow->setSpacing(10);

                    QLabel* bull = new QLabel("*");
                    bull->setStyleSheet(
                        "color:#D4762A; font-size:18px; font-weight:900;"
                        " background:transparent; min-width:12px;");
                    spRow->addWidget(bull);

                    QWidget* tx = new QWidget; tx->setStyleSheet("background:transparent;");
                    QVBoxLayout* txL = new QVBoxLayout(tx);
                    txL->setContentsMargins(0,0,0,0); txL->setSpacing(2);

                    QLabel* spN = new QLabel(name);
                    spN->setStyleSheet(
                        "color:white; font-size:12px; font-weight:800; background:transparent;");

                    QString descParts;
                    if (!region.isEmpty() && !reason.isEmpty())
                        descParts = region + "  -  " + reason;
                    else if (!region.isEmpty()) descParts = region;
                    else                         descParts = reason;

                    QLabel* spD = new QLabel(descParts);
                    spD->setWordWrap(true);
                    spD->setStyleSheet(
                        "color:rgba(255,255,255,0.65); font-size:10px; background:transparent;");

                    txL->addWidget(spN);
                    if (!descParts.isEmpty()) txL->addWidget(spD);
                    spRow->addWidget(tx, 1);

                    // "IA" badge
                    QLabel* aiTag = new QLabel("IA");
                    aiTag->setFixedSize(24,18);
                    aiTag->setAlignment(Qt::AlignCenter);
                    aiTag->setStyleSheet(
                        "color:#D4762A; background:rgba(212,118,42,0.20);"
                        " border-radius:4px; font-size:9px; font-weight:900;");
                    spRow->addWidget(aiTag);

                    aiResultsL->addWidget(spR);
                    ++added;
                }

                if (added == 0) {
                    aiStatus->setVisible(true);
                    aiStatus->setText("Format inattendu - reessayez.");
                    retryBtn->setVisible(true);
                } else {
                    aiBadge->setVisible(true);
                }
            });
        };

        QObject::connect(retryBtn, &QPushButton::clicked, dlg, launchAiSearch);
        // Auto-launch 200ms after dialog opens
        QTimer::singleShot(200, dlg, launchAiSearch);

        picker->close();
        dlg->exec();
    });

    picker->exec();
}


// ─────────────────────────────────────────────────────────────
//  METIER AVANCE : Specialisations Manquantes
// ─────────────────────────────────────────────────────────────
void GestProjCrud::showSpecialisationsManquantes(QWidget* parent)
{
    // ── 1. Project picker ─────────────────────────────────────
    QDialog* picker = new QDialog(parent,
        Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    picker->setWindowTitle("Specialisations Manquantes - Choisir un projet");
    picker->setMinimumSize(500, 400);
    picker->setAttribute(Qt::WA_DeleteOnClose);
    picker->setStyleSheet("QDialog{ background:#1C2A35; }");

    QVBoxLayout* pl = new QVBoxLayout(picker);
    pl->setContentsMargins(20,18,20,16); pl->setSpacing(12);

    QLabel* plTitle = new QLabel("  Selectionnez le projet a analyser");
    plTitle->setStyleSheet(
        "color:#a8d4e0; font-size:13px; font-weight:900;"
        " background:rgba(42,100,155,0.15); border-radius:10px; padding:7px 14px;");
    pl->addWidget(plTitle);

    QListWidget* projList = new QListWidget;
    projList->setStyleSheet(
        "QListWidget{ background:rgba(255,255,255,0.08); border-radius:10px;"
        " border:1px solid rgba(255,255,255,0.15); }"
        "QListWidget::item{ padding:9px 14px; color:rgba(255,255,255,0.85);"
        " font-weight:600; font-size:11px; }"
        "QListWidget::item:selected{ background:#2A649B; color:white; border-radius:6px; }"
        "QListWidget::item:hover:!selected{ background:rgba(255,255,255,0.08); }");

    GestProjCrud crud;
    QList<ProjetRecord> allProjs; QString perr;
    crud.loadProjets(allProjs, &perr);
    for (const ProjetRecord& pr : allProjs) {
        QListWidgetItem* item = new QListWidgetItem(
            QString("%1  [%2]").arg(pr.nomDuProjet).arg(pr.domaineDeRecherche));
        item->setData(Qt::UserRole,     pr.idProjet);
        item->setData(Qt::UserRole + 1, pr.nomDuProjet);
        item->setData(Qt::UserRole + 2, pr.domaineDeRecherche);
        projList->addItem(item);
    }
    pl->addWidget(projList, 1);

    QHBoxLayout* pbl = new QHBoxLayout; pbl->setSpacing(10);
    QPushButton* cancelBtn = new QPushButton("Annuler");
    cancelBtn->setFixedHeight(36);
    cancelBtn->setStyleSheet(
        "QPushButton{ background:rgba(255,255,255,0.12); color:rgba(255,255,255,0.80);"
        " border-radius:8px; font-weight:700; font-size:12px; padding:0 16px; }"
        "QPushButton:hover{ background:rgba(255,255,255,0.20); }");
    QPushButton* goBtn = new QPushButton("Analyser");
    goBtn->setFixedHeight(36); goBtn->setEnabled(false);
    goBtn->setStyleSheet(
        "QPushButton{ background:#D4762A; color:white; border-radius:8px;"
        " font-weight:800; font-size:12px; padding:0 16px; }"
        "QPushButton:hover{ background:#A85A1A; }"
        "QPushButton:disabled{ background:rgba(212,118,42,0.35);"
        " color:rgba(255,255,255,0.40); }");
    pbl->addWidget(cancelBtn); pbl->addStretch(1); pbl->addWidget(goBtn);
    pl->addLayout(pbl);

    QObject::connect(cancelBtn, &QPushButton::clicked, picker, &QDialog::reject);
    QObject::connect(projList,  &QListWidget::itemSelectionChanged, picker,
        [=](){ goBtn->setEnabled(projList->currentItem() != nullptr); });
    QObject::connect(projList, &QListWidget::itemDoubleClicked,
                     goBtn, &QPushButton::click);

    QObject::connect(goBtn, &QPushButton::clicked, picker, [=](){
        QListWidgetItem* sel = projList->currentItem();
        if (!sel) return;

        const int     projId  = sel->data(Qt::UserRole).toInt();
        const QString projNom = sel->data(Qt::UserRole + 1).toString();
        const QString domaine = sel->data(Qt::UserRole + 2).toString().trimmed();

        picker->hide();

        // ── STEP 1: Required specialisations per domain ───────
        // Each entry: { keyword_to_match_in_domain, required_specs }
        struct DomainSpec {
            QString keyword;
            QStringList required;
        };
        const QList<DomainSpec> domainMap = {
            { "nomique",     { "Bioinformatique", "Biologie moleculaire",
                               "Statistiques", "Genetique" } },
            { "oncol",       { "Anatomopathologie", "Biologie cellulaire",
                               "Pharmacologie", "Oncologie clinique" } },
            { "cancer",      { "Anatomopathologie", "Biologie cellulaire",
                               "Pharmacologie", "Oncologie clinique" } },
            { "micro",       { "Bacteriologie", "Virologie", "Mycologie",
                               "Biologie moleculaire" } },
            { "immuno",      { "Immunologie clinique", "Biologie cellulaire",
                               "Genetique", "Statistiques" } },
            { "sante publ",  { "Epidemiologie", "Biostatistiques",
                               "Sciences sociales", "Sante environnementale" } },
            { "epidemio",    { "Epidemiologie", "Biostatistiques",
                               "Sante publique", "Statistiques" } },
            { "biotech",     { "Genie biologique", "Biologie moleculaire",
                               "Biochimie", "Microbiologie" } },
            { "biochim",     { "Biochimie", "Chimie analytique",
                               "Biologie moleculaire", "Enzymologie" } },
            { "neuro",       { "Neurobiologie", "Electrophysiologie",
                               "Biologie cellulaire", "Imagerie cerebrale" } },
            { "pharm",       { "Pharmacologie", "Toxicologie",
                               "Chimie medicale", "Biologie cellulaire" } },
            { "proteo",      { "Spectrometrie de masse", "Biochimie",
                               "Bioinformatique", "Biologie moleculaire" } },
            { "gen",         { "Genetique", "Biologie moleculaire",
                               "Bioinformatique", "Cytogenetique" } },
        };

        QStringList required;
        QString domLow = domaine.toLower();
        for (const DomainSpec& ds : domainMap) {
            if (domLow.contains(ds.keyword)) {
                required = ds.required;
                break;
            }
        }
        // Fallback if domain not mapped
        if (required.isEmpty()) {
            required = { "Biologie moleculaire", "Statistiques",
                         "Biochimie", "Bioinformatique" };
        }

        // ── STEP 2: Get current team specialisations ──────────
        QStringList currentSpecs;
        {
            QSqlQuery qS;
            QString et = QString::fromUtf8("Employ\xc3\xa9s");
            qS.prepare(QString(
                "SELECT NVL(TRIM(emp.\"specialization\"),'')"
                " FROM \"%1\" emp"
                " JOIN \"Associer\" a ON a.\"employee_id\" = emp.\"employee_id\""
                " WHERE a.\"Id_projet\" = :pid"
                " AND NVL(TRIM(emp.\"specialization\"),'') != ''").arg(et));
            qS.bindValue(":pid", projId);
            if (qS.exec()) {
                while (qS.next()) {
                    QString sp = qS.value(0).toString().trimmed();
                    if (!sp.isEmpty()) currentSpecs.append(sp.toLower());
                }
            }
        }

        // ── STEP 3: Compute missing (set difference, fuzzy) ───
        struct MissingSpec {
            QString name;
            QString criticality;   // "Critique" / "Importante" / "Recommandee"
            QString critColor;
            int     critPct = 0;   // from similar projects
        };
        QList<MissingSpec> missing;

        // Check how often each required spec appears in similar completed projects
        // to assign criticality
        for (const QString& req : required) {
            // Fuzzy match: check if current team has anything containing req words
            bool found = false;
            for (const QString& cur : currentSpecs) {
                if (cur.contains(req.toLower().left(5)) ||
                    req.toLower().contains(cur.left(5)))
                {
                    found = true;
                    break;
                }
            }
            if (found) continue;

            // Count how many completed projects in same domain had this spec
            int simTotal = 0, simHas = 0;
            {
                QSqlQuery qSim;
                QString et2 = QString::fromUtf8("Employ\xc3\xa9s");
                QString termSt = QString::fromUtf8("termin\xc3\xa9");
                qSim.prepare(QString(
                    "SELECT COUNT(DISTINCT p.\"Id_projet\") FROM \"projet\" p"
                    " WHERE TRIM(p.\"domaine_de_recherche\") = :dom"
                    " AND LOWER(NVL(TRIM(p.\"statut\"),'')) IN ('%1','termine')")
                    .arg(termSt));
                qSim.bindValue(":dom", domaine);
                if (qSim.exec() && qSim.next()) simTotal = qSim.value(0).toInt();

                QSqlQuery qHas;
                qHas.prepare(QString(
                    "SELECT COUNT(DISTINCT p.\"Id_projet\") FROM \"projet\" p"
                    " JOIN \"Associer\" a ON a.\"Id_projet\" = p.\"Id_projet\""
                    " JOIN \"%1\" emp ON emp.\"employee_id\" = a.\"employee_id\""
                    " WHERE TRIM(p.\"domaine_de_recherche\") = :dom"
                    " AND LOWER(NVL(TRIM(p.\"statut\"),'')) IN ('%2','termine')"
                    " AND LOWER(NVL(TRIM(emp.\"specialization\"),'')) LIKE :spec")
                    .arg(et2).arg(termSt));
                qHas.bindValue(":dom",  domaine);
                qHas.bindValue(":spec", "%" + req.toLower().left(5) + "%");
                if (qHas.exec() && qHas.next()) simHas = qHas.value(0).toInt();
            }

            double pct = (simTotal > 0) ? (100.0 * simHas / simTotal) : 0.0;
            int critPct = (int)qRound(pct);

            MissingSpec ms;
            ms.name     = req;
            ms.critPct  = critPct;
            if (simTotal == 0 || pct >= 60.0) {
                // No data → default to Critique for required specs
                ms.criticality = "Critique";
                ms.critColor   = "#8B2F3C";
                ms.critPct     = (simTotal == 0) ? 80 : critPct;
            } else if (pct >= 30.0) {
                ms.criticality = "Importante";
                ms.critColor   = "#D4762A";
            } else {
                ms.criticality = "Recommandee";
                ms.critColor   = "#2E8B7C";
            }
            missing.append(ms);
        }

        // ── STEP 5: Candidates per missing spec ───────────────
        struct Candidate {
            QString fullName;
            QString role;
            QString specialization;
            int     activeProjects = 0;
        };
        // Map: missingSpec.name -> list of candidates
        QMap<QString, QList<Candidate>> candidateMap;
        for (const MissingSpec& ms : missing) {
            QSqlQuery qC;
            QString et3 = QString::fromUtf8("Employ\xc3\xa9s");
            qC.prepare(QString(
                "SELECT NVL(emp.\"FULL_NAME\","
                " TRIM(emp.\"prenom\" || ' ' || emp.\"nom\")),"
                " NVL(TRIM(emp.\"ROLE\"),''),"
                " NVL(TRIM(emp.\"specialization\"),'')"
                " FROM \"%1\" emp"
                " WHERE NVL(emp.\"ACTIVE\",'O') = 'O'"
                " AND LOWER(NVL(TRIM(emp.\"specialization\"),'')) LIKE :spec"
                " AND NOT EXISTS ("
                "  SELECT 1 FROM \"Associer\" ax"
                "  WHERE ax.\"employee_id\" = emp.\"employee_id\""
                "  AND ax.\"Id_projet\" = :pid)"
                " AND ROWNUM <= 2").arg(et3));
            qC.bindValue(":spec", "%" + ms.name.toLower().left(5) + "%");
            qC.bindValue(":pid",  projId);
            QList<Candidate> cands;
            if (qC.exec()) {
                while (qC.next()) {
                    Candidate c;
                    c.fullName       = qC.value(0).toString().trimmed();
                    if (c.fullName.isEmpty()) c.fullName = "(Sans nom)";
                    c.role           = qC.value(1).toString().trimmed();
                    c.specialization = qC.value(2).toString().trimmed();

                    // Get active project count
                    QSqlQuery qa;
                    qa.prepare(
                        "SELECT COUNT(*) FROM \"Associer\" a"
                        " JOIN \"projet\" p ON a.\"Id_projet\" = p.\"Id_projet\""
                        " WHERE a.\"employee_id\" = ("
                        "  SELECT emp2.\"employee_id\" FROM \"" + et3 + "\" emp2"
                        "  WHERE NVL(emp2.\"FULL_NAME\","
                        "  TRIM(emp2.\"prenom\" || ' ' || emp2.\"nom\")) = :nm"
                        "  AND ROWNUM = 1)"
                        " AND LOWER(NVL(TRIM(p.\"statut\"),'')) = 'en cours'");
                    qa.bindValue(":nm", c.fullName);
                    c.activeProjects = (qa.exec() && qa.next()) ? qa.value(0).toInt() : 0;
                    cands.append(c);
                }
            }
            candidateMap[ms.name] = cands;
        }

        // ── Build result dialog ───────────────────────────────
        QDialog* dlg = new QDialog(parent,
            Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
        dlg->setWindowTitle("Specialisations Manquantes");
        dlg->setMinimumSize(780, 640);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->setStyleSheet("QDialog{ background:#1C2A35; }");

        QVBoxLayout* mainL = new QVBoxLayout(dlg);
        mainL->setContentsMargins(22,18,22,18); mainL->setSpacing(14);

        // ── Header ────────────────────────────────────────────
        QFrame* hdr = new QFrame;
        hdr->setStyleSheet(
            "QFrame{ background:qlineargradient(x1:0,y1:0,x2:1,y2:0,"
            "stop:0 #8A4A18,stop:1 #D4762A); border-radius:12px; }");
        QVBoxLayout* hL = new QVBoxLayout(hdr);
        hL->setContentsMargins(18,14,18,14); hL->setSpacing(4);
        QLabel* hTitle = new QLabel(
            QString("Specialisations Manquantes - \"%1\"").arg(projNom));
        hTitle->setStyleSheet(
            "color:white; font-size:15px; font-weight:900; background:transparent;");
        QString teamSz = QString::number(currentSpecs.size());
        QLabel* hSub = new QLabel(
            QString("Domaine : %1   |   Specialisations en equipe : %2   |"
                    "   Manquantes : %3")
                .arg(domaine.isEmpty() ? "non specifie" : domaine)
                .arg(teamSz)
                .arg(missing.size()));
        hSub->setStyleSheet(
            "color:rgba(255,255,255,0.75); font-size:11px; background:transparent;");
        hL->addWidget(hTitle); hL->addWidget(hSub);
        mainL->addWidget(hdr);

        // ── KPI summary strip ─────────────────────────────────
        QWidget* kpiRow = new QWidget;
        kpiRow->setStyleSheet("background:transparent;");
        QHBoxLayout* kpiL = new QHBoxLayout(kpiRow);
        kpiL->setContentsMargins(0,0,0,0); kpiL->setSpacing(10);

        int nCrit = 0, nImp = 0, nRec = 0;
        for (const MissingSpec& ms : missing) {
            if (ms.criticality == "Critique")    ++nCrit;
            else if (ms.criticality == "Importante") ++nImp;
            else ++nRec;
        }

        auto makeKpi = [](const QString& value, const QString& label,
                          const QString& bg, const QString& fg) -> QFrame*
        {
            QFrame* pill = new QFrame;
            pill->setStyleSheet(QString(
                "QFrame{ background:%1; border-radius:10px; border:none; }").arg(bg));
            QVBoxLayout* pL = new QVBoxLayout(pill);
            pL->setContentsMargins(14,10,14,10); pL->setSpacing(2);
            QLabel* vLbl = new QLabel(value);
            vLbl->setStyleSheet(QString(
                "color:%1; font-size:22px; font-weight:900;"
                " background:transparent;").arg(fg));
            vLbl->setAlignment(Qt::AlignCenter);
            QLabel* lLbl = new QLabel(label);
            lLbl->setStyleSheet(
                "color:rgba(255,255,255,0.55); font-size:9px;"
                " font-weight:700; background:transparent;");
            lLbl->setAlignment(Qt::AlignCenter);
            pL->addWidget(vLbl); pL->addWidget(lLbl);
            return pill;
        };

        kpiL->addWidget(makeKpi(QString::number(required.size()),
            "Requises au total",
            "rgba(42,100,155,0.18)", "#a8d4e0"), 1);
        kpiL->addWidget(makeKpi(QString::number(currentSpecs.size()),
            "Presentes en equipe",
            "rgba(46,139,124,0.18)", "#2E8B7C"), 1);
        kpiL->addWidget(makeKpi(QString::number(nCrit),
            "Critiques",
            "rgba(139,47,60,0.25)", "#CF4F5E"), 1);
        kpiL->addWidget(makeKpi(QString::number(nImp),
            "Importantes",
            "rgba(212,118,42,0.20)", "#D4762A"), 1);
        kpiL->addWidget(makeKpi(QString::number(nRec),
            "Recommandees",
            "rgba(46,139,124,0.12)", "#2E8B7C"), 1);
        mainL->addWidget(kpiRow);

        // Coverage progress bar
        int covPct = (required.size() > 0)
            ? (int)((1.0 - (double)missing.size() / required.size()) * 100)
            : 100;
        QWidget* covRow = new QWidget; covRow->setStyleSheet("background:transparent;");
        QVBoxLayout* covL = new QVBoxLayout(covRow);
        covL->setContentsMargins(0,0,0,0); covL->setSpacing(4);
        QLabel* covLbl = new QLabel(
            QString("Couverture de l'equipe : %1%").arg(covPct));
        covLbl->setStyleSheet(
            "color:rgba(255,255,255,0.70); font-size:10px; font-weight:700;"
            " background:transparent;");
        QWidget* covBg = new QWidget; covBg->setFixedHeight(8);
        covBg->setStyleSheet(
            "background:rgba(255,255,255,0.10); border-radius:4px;");
        QWidget* covFill = new QWidget(covBg); covFill->setFixedHeight(8);
        QString covColor = covPct >= 75 ? "#2E8B7C"
                         : covPct >= 40 ? "#D4762A" : "#8B2F3C";
        covFill->setStyleSheet(
            QString("background:%1; border-radius:4px;").arg(covColor));
        int capturedCovPct = covPct;
        QTimer::singleShot(0, covBg, [covFill, covBg, capturedCovPct](){
            covFill->setFixedWidth(covBg->width() * capturedCovPct / 100);
        });
        covL->addWidget(covLbl); covL->addWidget(covBg);
        mainL->addWidget(covRow);

        // ── Scroll content ────────────────────────────────────
        QScrollArea* scroll = new QScrollArea;
        scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame);
        scroll->setStyleSheet(
            "QScrollArea{ background:transparent; border:none; }"
            "QScrollBar:vertical{ width:7px; background:transparent; }"
            "QScrollBar::handle:vertical{"
            " background:rgba(212,118,42,0.35); border-radius:4px; }");
        QWidget* scrollW = new QWidget;
        scrollW->setStyleSheet("background:transparent;");
        QVBoxLayout* scrollL = new QVBoxLayout(scrollW);
        scrollL->setContentsMargins(0,0,4,0); scrollL->setSpacing(12);

        if (missing.isEmpty()) {
            // All covered!
            QFrame* okCard = new QFrame;
            okCard->setStyleSheet(
                "QFrame{ background:rgba(46,139,124,0.15);"
                " border:2px solid rgba(46,139,124,0.40); border-radius:12px; }");
            QVBoxLayout* okL = new QVBoxLayout(okCard);
            okL->setContentsMargins(20,20,20,20); okL->setSpacing(8);
            QLabel* okIcon = new QLabel("Couverture complete !");
            okIcon->setStyleSheet(
                "color:#2E8B7C; font-size:28px; font-weight:900;"
                " background:transparent;");
            okIcon->setAlignment(Qt::AlignCenter);
            QLabel* okTxt = new QLabel(
                "L'equipe couvre toutes les specialisations requises"
                " pour le domaine " + domaine + ".");
            okTxt->setStyleSheet(
                "color:rgba(255,255,255,0.70); font-size:12px;"
                " background:transparent;");
            okTxt->setAlignment(Qt::AlignCenter);
            okL->addWidget(okIcon); okL->addWidget(okTxt);
            scrollL->addWidget(okCard);
        } else {
            // ── Header for missing list ───────────────────────
            QLabel* secHdr = new QLabel("Specialisations manquantes detectees");
            secHdr->setStyleSheet(
                "color:#D4762A; font-size:12px; font-weight:900;"
                " background:transparent;");
            scrollL->addWidget(secHdr);

            // ── One card per missing spec ─────────────────────
            for (const MissingSpec& ms : missing) {
                QFrame* card = new QFrame;
                card->setStyleSheet(QString(
                    "QFrame{ background:rgba(255,255,255,0.05);"
                    " border:1px solid rgba(255,255,255,0.08);"
                    " border-left:5px solid %1; border-radius:10px; }")
                    .arg(ms.critColor));
                QVBoxLayout* cardL = new QVBoxLayout(card);
                cardL->setContentsMargins(16,12,16,12); cardL->setSpacing(8);

                // Top row: spec name + criticality badge
                QHBoxLayout* topRow = new QHBoxLayout;
                topRow->setContentsMargins(0,0,0,0); topRow->setSpacing(10);

                QLabel* specName = new QLabel(ms.name);
                specName->setStyleSheet(
                    "color:white; font-size:13px; font-weight:900;"
                    " background:transparent;");
                topRow->addWidget(specName, 1);

                // Criticality badge
                QLabel* critBadge = new QLabel(
                    QString("  %1  ").arg(ms.criticality));
                critBadge->setFixedHeight(24);
                critBadge->setAlignment(Qt::AlignCenter);
                critBadge->setStyleSheet(QString(
                    "color:white; background:%1; border-radius:6px;"
                    " font-size:10px; font-weight:800;").arg(ms.critColor));
                topRow->addWidget(critBadge);

                // Frequency info
                if (ms.critPct > 0) {
                    QLabel* freqLbl = new QLabel(
                        QString("Presente dans %1% des projets similaires")
                            .arg(ms.critPct));
                    freqLbl->setStyleSheet(
                        "color:rgba(255,255,255,0.40); font-size:9px;"
                        " background:transparent;");
                    topRow->addWidget(freqLbl);
                }
                cardL->addLayout(topRow);

                // ── Separator ─────────────────────────────────
                QFrame* sep = new QFrame;
                sep->setFrameShape(QFrame::HLine);
                sep->setStyleSheet(
                    "color:rgba(255,255,255,0.08);"
                    " background:rgba(255,255,255,0.08);"
                    " border:none; max-height:1px;");
                cardL->addWidget(sep);

                // ── Candidate suggestions ─────────────────────
                const QList<Candidate>& cands = candidateMap[ms.name];
                if (cands.isEmpty()) {
                    QLabel* noCand = new QLabel(
                        "Aucun employe disponible avec cette specialisation"
                        " dans la base de donnees.");
                    noCand->setStyleSheet(
                        "color:rgba(255,255,255,0.35); font-size:10px;"
                        " font-style:italic; background:transparent;");
                    cardL->addWidget(noCand);
                } else {
                    QLabel* candHdr = new QLabel("Candidats suggeres :");
                    candHdr->setStyleSheet(
                        "color:rgba(168,212,224,0.70); font-size:10px;"
                        " font-weight:700; background:transparent;");
                    cardL->addWidget(candHdr);

                    for (const Candidate& c : cands) {
                        QFrame* candRow = new QFrame;
                        candRow->setStyleSheet(
                            "QFrame{ background:rgba(255,255,255,0.04);"
                            " border-radius:6px; border:none; }");
                        QHBoxLayout* cL = new QHBoxLayout(candRow);
                        cL->setContentsMargins(12,7,12,7); cL->setSpacing(10);

                        // Avatar circle with initials
                        QLabel* avatar = new QLabel(
                            c.fullName.isEmpty() ? "?"
                            : QString(c.fullName[0]).toUpper());
                        avatar->setFixedSize(30,30);
                        avatar->setAlignment(Qt::AlignCenter);
                        avatar->setStyleSheet(QString(
                            "background:%1; color:white; border-radius:15px;"
                            " font-size:12px; font-weight:900;")
                            .arg(ms.critColor));
                        cL->addWidget(avatar);

                        // Name + role + spec
                        QWidget* info = new QWidget;
                        info->setStyleSheet("background:transparent;");
                        QVBoxLayout* iL = new QVBoxLayout(info);
                        iL->setContentsMargins(0,0,0,0); iL->setSpacing(1);
                        QLabel* cName = new QLabel(c.fullName);
                        cName->setStyleSheet(
                            "color:white; font-size:11px; font-weight:800;"
                            " background:transparent;");
                        QString detail = c.role;
                        if (!c.specialization.isEmpty())
                            detail += "  -  " + c.specialization;
                        QLabel* cDetail = new QLabel(detail);
                        cDetail->setStyleSheet(
                            "color:rgba(168,212,224,0.70); font-size:9px;"
                            " background:transparent;");
                        iL->addWidget(cName); iL->addWidget(cDetail);
                        cL->addWidget(info, 1);

                        // Availability
                        QString avTxt = c.activeProjects == 0
                            ? "Disponible" : QString("%1 projet(s)").arg(c.activeProjects);
                        QString avFg  = c.activeProjects == 0 ? "#2E8B7C" : "#D4762A";
                        QLabel* avLbl = new QLabel(avTxt);
                        avLbl->setStyleSheet(QString(
                            "color:%1; font-size:9px; font-weight:800;"
                            " background:transparent;").arg(avFg));
                        cL->addWidget(avLbl);
                        cardL->addWidget(candRow);
                    }
                }
                scrollL->addWidget(card);
            }
        }

        // ── Current team specialisations summary ──────────────
        if (!currentSpecs.isEmpty()) {
            QFrame* teamCard = new QFrame;
            teamCard->setStyleSheet(
                "QFrame{ background:rgba(42,100,155,0.08);"
                " border:1px solid rgba(42,100,155,0.20);"
                " border-radius:10px; }");
            QVBoxLayout* tcL = new QVBoxLayout(teamCard);
            tcL->setContentsMargins(16,12,16,12); tcL->setSpacing(6);
            QLabel* tcHdr = new QLabel("Specialisations actuelles de l'equipe");
            tcHdr->setStyleSheet(
                "color:#a8d4e0; font-size:11px; font-weight:800;"
                " background:transparent;");
            tcL->addWidget(tcHdr);

            // Wrap tags in a flow-like horizontal layout
            QWidget* tagsW = new QWidget; tagsW->setStyleSheet("background:transparent;");
            QHBoxLayout* tagsL = new QHBoxLayout(tagsW);
            tagsL->setContentsMargins(0,0,0,0); tagsL->setSpacing(6);
            tagsL->setAlignment(Qt::AlignLeft);
            for (const QString& sp : currentSpecs) {
                if (sp.isEmpty()) continue;
                QLabel* tag = new QLabel(sp);
                tag->setStyleSheet(
                    "color:#a8d4e0; background:rgba(42,100,155,0.20);"
                    " border-radius:5px; font-size:10px; font-weight:700;"
                    " padding:3px 8px;");
                tagsL->addWidget(tag);
            }
            tagsL->addStretch(1);
            tcL->addWidget(tagsW);
            scrollL->addWidget(teamCard);
        } else {
            QLabel* noTeam = new QLabel(
                "Aucune specialisation renseignee pour les membres actuels de l'equipe.");
            noTeam->setStyleSheet(
                "color:rgba(255,255,255,0.35); font-size:10px;"
                " font-style:italic; background:transparent;");
            scrollL->addWidget(noTeam);
        }

        scrollL->addStretch(1);
        scroll->setWidget(scrollW);
        mainL->addWidget(scroll, 1);

        // ── Footer ────────────────────────────────────────────
        QFrame* foot = new QFrame;
        foot->setStyleSheet(
            "QFrame{ background:rgba(255,255,255,0.04); border-radius:8px; }");
        QHBoxLayout* fl = new QHBoxLayout(foot);
        fl->setContentsMargins(12,8,12,8);
        QLabel* note = new QLabel(
            "Criticite basee sur la frequence dans les projets similaires termines."
            " Critique >= 60%, Importante 30-60%, Recommandee < 30%.");
        note->setWordWrap(true);
        note->setStyleSheet(
            "color:rgba(255,255,255,0.30); font-size:9px; background:transparent;");
        fl->addWidget(note, 1);
        QPushButton* cb = new QPushButton("Fermer");
        cb->setFixedSize(90,32);
        cb->setStyleSheet(
            "QPushButton{ background:#D4762A; color:white; border-radius:7px;"
            " font-weight:700; }"
            "QPushButton:hover{ background:#A85A1A; }");
        QObject::connect(cb, &QPushButton::clicked, dlg, &QDialog::accept);
        fl->addWidget(cb);
        mainL->addWidget(foot);

        picker->close();
        dlg->exec();
    });

    picker->exec();
}


// ─────────────────────────────────────────────────────────────
//  METIER AVANCE : Analyse Intelligente
//  Produces a textual analytical summary of project statistics
// ─────────────────────────────────────────────────────────────
void GestProjCrud::showAnalyseIntelligente(QWidget* parent)
{
    QDialog* picker = new QDialog(parent,
        Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    picker->setWindowTitle("Analyse Intelligente - Choisir un projet");
    picker->setMinimumSize(500, 380);
    picker->setAttribute(Qt::WA_DeleteOnClose);
    picker->setStyleSheet("QDialog{ background:#1C2A35; }");

    QVBoxLayout* pl = new QVBoxLayout(picker);
    pl->setContentsMargins(20,18,20,16); pl->setSpacing(12);

    QLabel* plTitle = new QLabel("  Sélectionnez le projet à analyser");
    plTitle->setStyleSheet("color:#a8d4e0; font-size:13px; font-weight:900;"
        " background:rgba(42,100,155,0.15); border-radius:10px; padding:7px 14px;");
    pl->addWidget(plTitle);

    QListWidget* projList = new QListWidget;
    projList->setStyleSheet(
        "QListWidget{ background:rgba(255,255,255,0.08); border-radius:10px; border:1px solid rgba(255,255,255,0.15); }"
        "QListWidget::item{ padding:9px 14px; color:rgba(255,255,255,0.85); font-weight:600; font-size:11px; }"
        "QListWidget::item:selected{ background:#2A649B; color:white; border-radius:6px; }");

    GestProjCrud crud;
    QList<ProjetRecord> allProjs; QString perr;
    crud.loadProjets(allProjs, &perr);
    for (const ProjetRecord& pr : allProjs) {
        QListWidgetItem* item = new QListWidgetItem(
            QString("%1  [%2]").arg(pr.nomDuProjet).arg(pr.domaineDeRecherche));
        item->setData(Qt::UserRole, pr.idProjet);
        item->setData(Qt::UserRole + 1, pr.nomDuProjet);
        projList->addItem(item);
    }
    pl->addWidget(projList, 1);

    QHBoxLayout* pbl = new QHBoxLayout; pbl->setSpacing(10);
    QPushButton* cancelBtn = new QPushButton("Annuler");
    cancelBtn->setFixedHeight(36);
    cancelBtn->setStyleSheet(
        "QPushButton{ background:rgba(255,255,255,0.12); color:rgba(255,255,255,0.80);"
        " border-radius:8px; font-weight:700; font-size:12px; padding:0 16px; }"
        "QPushButton:hover{ background:rgba(255,255,255,0.20); }");
    QPushButton* goBtn = new QPushButton("Générer le rapport");
    goBtn->setFixedHeight(36); goBtn->setEnabled(false);
    goBtn->setStyleSheet(
        "QPushButton{ background:#D4762A; color:white; border-radius:8px;"
        " font-weight:800; font-size:12px; padding:0 16px; }"
        "QPushButton:hover{ background:#A85A1A; }"
        "QPushButton:disabled{ background:rgba(212,118,42,0.35); color:rgba(255,255,255,0.40); }");
    pbl->addWidget(cancelBtn); pbl->addStretch(1); pbl->addWidget(goBtn);
    pl->addLayout(pbl);

    QObject::connect(cancelBtn, &QPushButton::clicked, picker, &QDialog::reject);
    QObject::connect(projList,  &QListWidget::itemSelectionChanged, picker,
        [=](){ goBtn->setEnabled(projList->currentItem() != nullptr); });
    QObject::connect(projList, &QListWidget::itemDoubleClicked,
                     goBtn, &QPushButton::click);

    QObject::connect(goBtn, &QPushButton::clicked, picker, [=](){
        QListWidgetItem* sel = projList->currentItem();
        if (!sel) return;
        const int projId = sel->data(Qt::UserRole).toInt();
        const QString projName = sel->data(Qt::UserRole + 1).toString();
        picker->accept();

        // Compute metrics
        GestProjCrud crudLocal;
        ProjetRecord rec;
        QString err;
        if (!crudLocal.fetchProjet(projId, rec, &err)) {
            showSanteAlert(parent, "error", "Analyse Intelligente", "Impossible de charger le projet :\n" + err);
            return;
        }

        // Duration progress (months)
        double duration_progress = 0.0;
        if (rec.dateDeDebut.isValid() && rec.dateDeFin.isValid()) {
            int totalMonths = (rec.dateDeDebut.daysTo(rec.dateDeFin) / 30);
            int elapsedMonths = rec.dateDeDebut.daysTo(QDate::currentDate()) / 30;
            if (totalMonths > 0) {
                duration_progress = qBound(0.0, (double)elapsedMonths / (double)totalMonths * 100.0, 100.0);
            }
        }

        // Budget consumed (re-use compute logic)
        double spent = 0.0;
        {
            QSqlQuery qExp;
            qExp.prepare("SELECT COUNT(*) FROM \"Expérience\" WHERE \"Id_projet\" = :id");
            qExp.bindValue(":id", projId);
            if (qExp.exec() && qExp.next()) spent += qExp.value(0).toDouble() * PRICE_EXPERIENCE;

            QSqlQuery qEch;
            qEch.prepare("SELECT COUNT(*) FROM \"BioSample\" WHERE \"Id_projet\" = :id");
            qEch.bindValue(":id", projId);
            if (qEch.exec() && qEch.next()) spent += qEch.value(0).toDouble() * PRICE_ECHANTILLON;

            QSqlQuery qEq;
            qEq.prepare(
                "SELECT COUNT(*) FROM \"Équipement\" eq "
                "INNER JOIN \"Expérience\" ex ON eq.\"Id_exp\" = ex.\"Id_exp\" "
                "WHERE ex.\"Id_projet\" = :id");
            qEq.bindValue(":id", projId);
            if (qEq.exec() && qEq.next()) spent += qEq.value(0).toDouble() * PRICE_EQUIPEMENT;
        }
        double budget_consumed = (rec.budget > 0.0) ? (spent / rec.budget * 100.0) : 0.0;

        // Experiences completion
        int totalExp = 0, doneExp = 0;
        {
            QSqlQuery qT; qT.prepare("SELECT COUNT(*) FROM \"Expérience\" WHERE \"Id_projet\" = :id"); qT.bindValue(":id", projId);
            if (qT.exec() && qT.next()) totalExp = qT.value(0).toInt();

            QSqlQuery qD;
            qD.prepare("SELECT COUNT(*) FROM \"Expérience\" WHERE \"Id_projet\" = :id "
                       "AND LOWER(NVL(TRIM(\"Statut\"),'')) IN ('terminé','termine','fini','completed','done')");
            qD.bindValue(":id", projId);
            if (qD.exec() && qD.next()) doneExp = qD.value(0).toInt();
        }
        double exp_completion = (totalExp > 0) ? (100.0 * doneExp / totalExp) : 0.0;

        // Milestone progress (reuse Milestone detection from tracker)
        int totalReached = 0;
        int totalMilestones = 0;
        {
            struct Milestone { bool reached = false; };
            QVector<Milestone> mvec(8);
            // M1
            { QSqlQuery q; q.prepare("SELECT COUNT(*) FROM \"Associer\" WHERE \"Id_projet\" = :pid"); q.bindValue(":pid", projId); if (q.exec() && q.next()) mvec[0].reached = (q.value(0).toInt() >= 1); }
            // M2
            { QSqlQuery q; q.prepare("SELECT \"numéro_d_approbation_éthique\" FROM \"projet\" WHERE \"Id_projet\" = :pid"); q.bindValue(":pid", projId); if (q.exec() && q.next()) mvec[1].reached = !q.value(0).toString().trimmed().isEmpty(); }
            // M3
            { QSqlQuery q; q.prepare("SELECT COUNT(*) FROM \"Expérience\" WHERE \"Id_projet\" = :pid"); q.bindValue(":pid", projId); if (q.exec() && q.next()) mvec[2].reached = (q.value(0).toInt() >= 1); }
            // M4
            { QSqlQuery q; q.prepare("SELECT COUNT(*) FROM \"BioSample\" WHERE \"Id_projet\" = :pid"); q.bindValue(":pid", projId); if (q.exec() && q.next()) mvec[3].reached = (q.value(0).toInt() >= 1); }
            // M5
            { QSqlQuery qT; qT.prepare("SELECT COUNT(*) FROM \"Expérience\" WHERE \"Id_projet\" = :pid"); qT.bindValue(":pid", projId); QSqlQuery qN; qN.prepare("SELECT COUNT(*) FROM \"Expérience\" WHERE \"Id_projet\" = :pid AND LOWER(\"Statut\") != 'terminé'"); qN.bindValue(":pid", projId); int total=0, notDone=0; if (qT.exec() && qT.next()) total = qT.value(0).toInt(); if (qN.exec() && qN.next()) notDone = qN.value(0).toInt(); mvec[4].reached = (total >= 1 && notDone == 0); }
            // M6
            { QSqlQuery q; q.prepare("SELECT COUNT(*) FROM \"Publication\" p WHERE p.\"Id_projet\" = :pid AND LOWER(p.\"Statut\") IN ('soumis','soumise')"); q.bindValue(":pid", projId); if (q.exec() && q.next()) mvec[5].reached = (q.value(0).toInt() >= 1); }
            // M7
            { QSqlQuery q; q.prepare("SELECT COUNT(*) FROM \"Publication\" p WHERE p.\"Id_projet\" = :pid AND LOWER(p.\"Statut\") IN ('accepté','acceptée','accepte','acceptee')"); q.bindValue(":pid", projId); if (q.exec() && q.next()) mvec[6].reached = (q.value(0).toInt() >= 1); }
            // M8
            { QSqlQuery q; q.prepare("SELECT \"statut\" FROM \"projet\" WHERE \"Id_projet\" = :pid"); q.bindValue(":pid", projId); if (q.exec() && q.next()) { QString st = q.value(0).toString().trimmed().toLower(); mvec[7].reached = (st == "terminé" || st == "termine"); } }

            totalMilestones = mvec.size();
            for (const Milestone& mm : mvec) if (mm.reached) ++totalReached;
        }
        double milestone_progress = (totalMilestones > 0) ? (100.0 * totalReached / totalMilestones) : 0.0;

        // Publications rate: expected publications so far based on timeline
        double expectedPubs = std::max(1.0, (double)MAX_PUBS_REFERENCE * duration_progress / 100.0);
        double pub_rate = (rec.nombreDePublications > 0) ? ( (double)rec.nombreDePublications / expectedPubs ) : 0.0;

        // Health score
        ProjetSante ps = crudLocal.computeProjetSante(projId, &err);
        double health_score = ps.scoreGlobal;

        // Build insights
        QStringList lines;

        // Insight 1 — Progression vs budget
        if (budget_consumed > duration_progress + 20.0) {
            double over = budget_consumed - duration_progress;
            lines << QString("⚠️ Le budget est consommé plus vite que l'avancement du projet. Risque de dépassement budgétaire de %1%.")
                        .arg(QString::number(over, 'f', 1));
        } else if (budget_consumed < duration_progress - 20.0) {
            lines << "✅ Le projet avance plus vite que les dépenses prévues. Marge budgétaire disponible.";
        }

        // Insight 2 — Expérience pace
        if (exp_completion < duration_progress - 30.0) {
            lines << "⚠️ Le rythme des expériences est insuffisant par rapport au temps écoulé. Accélération nécessaire.";
        } else if (exp_completion > duration_progress) {
            lines << "✅ Les expériences avancent à un rythme soutenu.";
        }

        // Insight 3 — Publication output
        if (pub_rate <= 0.0 && duration_progress > 70.0) {
            lines << "🔴 Aucune publication à ce stade avancé du projet. L'objectif de publications est en danger.";
        } else if (pub_rate >= 1.0) {
            lines << "✅ L'objectif de publications est atteint ou dépassé.";
        }

        // Insight 4 — Milestone alignment
        if (milestone_progress < duration_progress - 25.0) {
            double lag = duration_progress - milestone_progress;
            lines << QString("⚠️ Les jalons accusent un retard de %1% par rapport au calendrier prévu.")
                        .arg(QString::number(lag, 'f', 1));
        }

        // Insight 5 — Overall verdict
        if (health_score >= 70.0) {
            lines << "✅ Projet globalement en bonne santé.";
        } else if (health_score >= 40.0) {
            lines << "⚠️ Projet nécessite une attention particulière.";
        } else {
            lines << "🔴 Projet en difficulté. Intervention recommandée.";
        }

        // Build output text
        QString outText;
        outText += QString("Analyse Intelligente — %1\n\n").arg(projName);
        outText += QString("Progression temporelle : %1%\n").arg(QString::number(duration_progress, 'f', 1));
        outText += QString("Budget consommé : %1% (estimé)\n").arg(QString::number(budget_consumed, 'f', 1));
        outText += QString("Expériences : %1/%2 (%3%)\n").arg(doneExp).arg(totalExp).arg(QString::number(exp_completion, 'f', 1));
        outText += QString("Jalons atteints : %1/%2 (%3%)\n").arg(totalReached).arg(totalMilestones).arg(QString::number(milestone_progress, 'f', 1));
        outText += QString("Publications (count) : %1\n").arg(rec.nombreDePublications);
        // Show result dialog with formatted metrics and insights
        QDialog* dlg = new QDialog(parent, Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
        dlg->setWindowTitle("Analyse Intelligente — Résumé");
        dlg->setMinimumSize(760, 560);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->setStyleSheet("QDialog{ background:#EAF3F5; }");

        QVBoxLayout* ml = new QVBoxLayout(dlg);
        ml->setContentsMargins(18,14,18,14); ml->setSpacing(12);

        // Title
        QLabel* title = new QLabel(QString("  Analyse Intelligente — %1").arg(projName));
        title->setStyleSheet("color:#0A5F58; font-size:15px; font-weight:900; background:rgba(10,95,88,0.08); border-radius:10px; padding:8px 12px;");
        ml->addWidget(title);

        // Scroll area for metrics and insights
        QScrollArea* scroll = new QScrollArea;
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setStyleSheet("QScrollArea{ background:transparent; border:none; }");

        QWidget* content = new QWidget;
        content->setStyleSheet("background:transparent;");
        QVBoxLayout* contentL = new QVBoxLayout(content);
        contentL->setContentsMargins(0,0,0,0); contentL->setSpacing(12);

        // ── Metrics Section ──
        QFrame* metricsFrame = new QFrame;
        metricsFrame->setStyleSheet("QFrame{ background:white; border:1px solid rgba(0,0,0,0.06); border-radius:10px; }");
        QVBoxLayout* metricsL = new QVBoxLayout(metricsFrame);
        metricsL->setContentsMargins(14,12,14,12); metricsL->setSpacing(8);

        QLabel* metricTitle = new QLabel("📊 Métriques du projet");
        metricTitle->setStyleSheet("color:#0A5F58; font-size:12px; font-weight:900; background:transparent;");
        metricsL->addWidget(metricTitle);

        auto makeMetric = [](const QString& label, const QString& value, const QString& unit) -> QFrame* {
            QFrame* f = new QFrame;
            f->setStyleSheet("background:rgba(10,95,88,0.05); border-radius:6px; border:1px solid rgba(10,95,88,0.10);");
            QHBoxLayout* l = new QHBoxLayout(f);
            l->setContentsMargins(10,6,10,6); l->setSpacing(8);
            QLabel* lbl = new QLabel(label);
            lbl->setStyleSheet("color:#12443B; font-size:11px; font-weight:700; background:transparent;");
            QLabel* val = new QLabel(value + " " + unit);
            val->setStyleSheet("color:#0A5F58; font-size:13px; font-weight:900; background:transparent;");
            l->addWidget(lbl, 1); l->addWidget(val);
            return f;
        };

        metricsL->addWidget(makeMetric("Progression", QString::number(duration_progress, 'f', 1), "%"));
        metricsL->addWidget(makeMetric("Budget consommé", QString::number(budget_consumed, 'f', 1), "%"));
        metricsL->addWidget(makeMetric("Expériences", QString("%1/%2 (%3%)").arg(doneExp).arg(totalExp).arg(QString::number(exp_completion, 'f', 1)), ""));
        metricsL->addWidget(makeMetric("Jalons", QString("%1/%2 (%3%)").arg(totalReached).arg(totalMilestones).arg(QString::number(milestone_progress, 'f', 1)), ""));
        metricsL->addWidget(makeMetric("Publications", QString::number(rec.nombreDePublications), ""));
        metricsL->addWidget(makeMetric("Santé globale", QString::number(health_score, 'f', 1), "%"));

        contentL->addWidget(metricsFrame);

        // ── Insights Section ──
        if (!lines.isEmpty()) {
            QFrame* insightsFrame = new QFrame;
            insightsFrame->setStyleSheet("QFrame{ background:white; border:1px solid rgba(0,0,0,0.06); border-radius:10px; }");
            QVBoxLayout* insightsL = new QVBoxLayout(insightsFrame);
            insightsL->setContentsMargins(14,12,14,12); insightsL->setSpacing(8);

            QLabel* insightTitle = new QLabel("💡 Insights et recommandations");
            insightTitle->setStyleSheet("color:#0A5F58; font-size:12px; font-weight:900; background:transparent;");
            insightsL->addWidget(insightTitle);

            for (const QString& insight : lines) {
                QLabel* insightLbl = new QLabel(insight);
                insightLbl->setWordWrap(true);
                insightLbl->setStyleSheet("color:#12443B; font-size:11px; font-weight:600; background:transparent; padding:4px 0;");
                insightsL->addWidget(insightLbl);
            }
            contentL->addWidget(insightsFrame);
        }

        contentL->addStretch(1);
        scroll->setWidget(content);
        ml->addWidget(scroll, 1);

        // Close button
        QHBoxLayout* bl = new QHBoxLayout; bl->addStretch(1);
        QPushButton* closeBtn = new QPushButton("Fermer"); closeBtn->setFixedHeight(36);
        closeBtn->setStyleSheet("QPushButton{ background:#0A5F58; color:white; border-radius:8px; font-weight:700; font-size:12px; padding:0 20px; }"
                                 "QPushButton:hover{ background:#12443B; }");
        QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);
        bl->addWidget(closeBtn);
        ml->addLayout(bl);

        dlg->exec();
    });

    picker->exec();
}


// ─────────────────────────────────────────────────────────────
//  METIER AVANCE : Risques Probables
//  Risk register with scoring, probability/impact matrix,
//  and mitigation actions. Distinct from Sante du Projet
//  (which is a radar chart of health axes).
// ─────────────────────────────────────────────────────────────
void GestProjCrud::showRisquesProbables(QWidget* parent)
{
    // ── 1. Project picker ─────────────────────────────────────
    QDialog* picker = new QDialog(parent,
        Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    picker->setWindowTitle("Risques Probables - Choisir un projet");
    picker->setMinimumSize(500, 400);
    picker->setAttribute(Qt::WA_DeleteOnClose);
    picker->setStyleSheet("QDialog{ background:#1C2A35; }");

    QVBoxLayout* pl = new QVBoxLayout(picker);
    pl->setContentsMargins(20,18,20,16); pl->setSpacing(12);

    QLabel* plTitle = new QLabel("  Selectionnez le projet a analyser");
    plTitle->setStyleSheet(
        "color:#CF4F5E; font-size:13px; font-weight:900;"
        " background:rgba(139,47,60,0.15); border-radius:10px; padding:7px 14px;");
    pl->addWidget(plTitle);

    QListWidget* projList = new QListWidget;
    projList->setStyleSheet(
        "QListWidget{ background:rgba(255,255,255,0.08); border-radius:10px;"
        " border:1px solid rgba(255,255,255,0.15); }"
        "QListWidget::item{ padding:9px 14px; color:rgba(255,255,255,0.85);"
        " font-weight:600; font-size:11px; }"
        "QListWidget::item:selected{ background:#8B2F3C; color:white; border-radius:6px; }"
        "QListWidget::item:hover:!selected{ background:rgba(255,255,255,0.08); }");

    GestProjCrud crud;
    QList<ProjetRecord> allProjs; QString perr;
    crud.loadProjets(allProjs, &perr);
    for (const ProjetRecord& pr : allProjs) {
        QListWidgetItem* item = new QListWidgetItem(
            QString("%1  [%2]  —  %3")
                .arg(pr.nomDuProjet)
                .arg(pr.domaineDeRecherche)
                .arg(pr.statut));
        item->setData(Qt::UserRole, pr.idProjet);
        projList->addItem(item);
    }
    pl->addWidget(projList, 1);

    QHBoxLayout* pbl = new QHBoxLayout; pbl->setSpacing(10);
    QPushButton* cancelBtn = new QPushButton("Annuler");
    cancelBtn->setFixedHeight(36);
    cancelBtn->setStyleSheet(
        "QPushButton{ background:rgba(255,255,255,0.12); color:rgba(255,255,255,0.80);"
        " border-radius:8px; font-weight:700; font-size:12px; padding:0 16px; }"
        "QPushButton:hover{ background:rgba(255,255,255,0.20); }");
    QPushButton* goBtn = new QPushButton("Analyser les risques");
    goBtn->setFixedHeight(36); goBtn->setEnabled(false);
    goBtn->setStyleSheet(
        "QPushButton{ background:#8B2F3C; color:white; border-radius:8px;"
        " font-weight:800; font-size:12px; padding:0 16px; }"
        "QPushButton:hover{ background:#6A2030; }"
        "QPushButton:disabled{ background:rgba(139,47,60,0.35);"
        " color:rgba(255,255,255,0.40); }");
    pbl->addWidget(cancelBtn); pbl->addStretch(1); pbl->addWidget(goBtn);
    pl->addLayout(pbl);

    QObject::connect(cancelBtn, &QPushButton::clicked, picker, &QDialog::reject);
    QObject::connect(projList,  &QListWidget::itemSelectionChanged, picker,
        [=](){ goBtn->setEnabled(projList->currentItem() != nullptr); });
    QObject::connect(projList, &QListWidget::itemDoubleClicked,
                     goBtn, &QPushButton::click);

    QObject::connect(goBtn, &QPushButton::clicked, picker, [=](){
        QListWidgetItem* sel = projList->currentItem();
        if (!sel) return;
        const int projId = sel->data(Qt::UserRole).toInt();
        picker->hide();

        // ── Fetch project record ──────────────────────────────
        ProjetRecord proj;
        GestProjCrud c2;
        c2.fetchProjet(projId, proj);

        const QDate today        = QDate::currentDate();
        const QDate dateDeb      = proj.dateDeDebut;
        const QDate dateFin      = proj.dateDeFin;
        const double budgetSaisi = proj.budget;
        const QString statut     = proj.statut.trimmed().toLower();
        const QString domaine    = proj.domaineDeRecherche.trimmed();
        const QString ethique    = proj.numeroDApprobationEthique.trimmed();

        double durationMonths = 1.0;
        if (dateDeb.isValid() && dateFin.isValid() && dateFin > dateDeb)
            durationMonths = dateDeb.daysTo(dateFin) / 30.4375;

        double monthsElapsed = 0.0;
        if (dateDeb.isValid())
            monthsElapsed = qMax(0.0, dateDeb.daysTo(today) / 30.4375);

        // ── Compute estimated budget (reuse algorithm) ────────
        double empEst = 0.0;
        {
            QSqlQuery q;
            QString et = QString::fromUtf8("Employ\xc3\xa9s");
            q.prepare(QString(
                "SELECT NVL(TRIM(emp.\"ROLE\"),'Technicien')"
                " FROM \"Associer\" a"
                " JOIN \"%1\" emp ON a.\"employee_id\"=emp.\"employee_id\""
                " WHERE a.\"Id_projet\"=:pid").arg(et));
            q.bindValue(":pid", projId);
            if (q.exec()) while (q.next()) {
                QString r = q.value(0).toString().trimmed();
                double m = (r=="Responsable") ? 3500.0
                         : (r=="Chercheur")   ? 2500.0 : 1400.0;
                empEst += m * durationMonths;
            }
        }
        double expEst = 0.0;
        {
            QSqlQuery q;
            QString termSt = QString::fromUtf8("termin\xc3\xa9" "e");
            q.prepare("SELECT NVL(TRIM(\"Status\"),'') FROM \"" +
                      QString::fromUtf8("Exp\xc3\xa9rience") +
                      "\" WHERE \"Id_projet\"=:pid");
            q.bindValue(":pid", projId);
            if (q.exec()) while (q.next()) {
                QString s = q.value(0).toString().trimmed().toLower();
                if (s=="en cours") expEst += 425.0;
                else if (s==termSt||s=="reussie"||s=="archivee"||
                         s.startsWith("termin")||s.startsWith("r")) expEst += 850.0;
            }
        }
        double estimatedBudget = (empEst + expEst) * 1.15;

        // ── Risk data structure ───────────────────────────────
        struct Risk {
            int     id;
            QString titre;
            QString probabilite;   // Certaine / Elevee / Moyenne / Faible
            QString impact;        // Bloquant / Critique / Important / Modere
            QString mitigation;
            int     score;         // 0-100
            QString probColor;
            QString impactColor;
            QString category;      // Budget/Equipe/Ethique/Donnees/Delais/Equipement
        };
        QList<Risk> risks;
        int riskIdCounter = 1;

        auto addRisk = [&](const QString& titre,
                           const QString& prob, const QString& impact,
                           const QString& mitigation,
                           const QString& category)
        {
            Risk r;
            r.id          = riskIdCounter++;
            r.titre       = titre;
            r.probabilite = prob;
            r.impact      = impact;
            r.mitigation  = mitigation;
            r.category    = category;

            // Score matrix
            if (prob=="Certaine"  && impact=="Bloquant")  r.score = 100;
            else if (prob=="Certaine"  && impact=="Critique")  r.score = 90;
            else if (prob=="Certaine"  && impact=="Important") r.score = 75;
            else if (prob=="Certaine"  && impact=="Modere")    r.score = 55;
            else if (prob=="Elevee"    && impact=="Bloquant")  r.score = 90;
            else if (prob=="Elevee"    && impact=="Critique")  r.score = 85;
            else if (prob=="Elevee"    && impact=="Important") r.score = 70;
            else if (prob=="Elevee"    && impact=="Modere")    r.score = 45;
            else if (prob=="Moyenne"   && impact=="Bloquant")  r.score = 70;
            else if (prob=="Moyenne"   && impact=="Critique")  r.score = 60;
            else if (prob=="Moyenne"   && impact=="Important") r.score = 50;
            else if (prob=="Moyenne"   && impact=="Modere")    r.score = 30;
            else if (prob=="Faible"    && impact=="Important") r.score = 35;
            else if (prob=="Faible"    && impact=="Modere")    r.score = 25;
            else                                               r.score = 20;

            // Probability color
            r.probColor   = (prob=="Certaine") ? "#8B2F3C"
                          : (prob=="Elevee")   ? "#CF4F5E"
                          : (prob=="Moyenne")  ? "#D4762A"
                                               : "#2E8B7C";
            // Impact color
            r.impactColor = (impact=="Bloquant")  ? "#6A0000"
                          : (impact=="Critique")  ? "#8B2F3C"
                          : (impact=="Important") ? "#D4762A"
                                                  : "#2A649B";
            risks.append(r);
        };

        // ═════════════════════════════════════════════════════
        //  RISK 1 — Budget insuffisant
        // ═════════════════════════════════════════════════════
        if (estimatedBudget > 0 && budgetSaisi < estimatedBudget * 0.8) {
            double gap = estimatedBudget - budgetSaisi;
            addRisk("Budget sous-estime",
                    "Elevee", "Critique",
                    QString("Revoir le budget ou reduire le perimetre. "
                            "Ecart estime : %1 TND.")
                        .arg(QString::number(gap,'f',0)),
                    "Budget");
        }

        // ═════════════════════════════════════════════════════
        //  RISK 2 — Pas d'approbation ethique
        // ═════════════════════════════════════════════════════
        if (ethique.isEmpty() &&
            (statut=="en cours" || statut=="planifie" ||
             statut.contains("planifi")))
        {
            addRisk("Absence d'approbation ethique",
                    "Certaine", "Bloquant",
                    "Soumettre le dossier au comite ethique immediatement."
                    " Sans approbation, le projet ne peut pas demarrer legalement.",
                    "Ethique");
        }

        // ═════════════════════════════════════════════════════
        //  RISK 3 — Equipe insuffisante
        // ═════════════════════════════════════════════════════
        int teamCount = 0;
        {
            QSqlQuery q;
            q.prepare("SELECT COUNT(*) FROM \"Associer\" WHERE \"Id_projet\"=:pid");
            q.bindValue(":pid", projId);
            if (q.exec() && q.next()) teamCount = q.value(0).toInt();
        }
        if (teamCount < 2) {
            addRisk("Equipe trop reduite",
                    "Elevee", "Important",
                    QString("Recruter au moins %1 chercheur(s) supplementaire(s). "
                            "Equipe actuelle : %2 personne(s).")
                        .arg(2 - teamCount).arg(teamCount),
                    "Equipe");
        }

        // ═════════════════════════════════════════════════════
        //  RISK 4 — Specialisations critiques manquantes
        //  (reuse domain->required map from SpecialisationsManquantes)
        // ═════════════════════════════════════════════════════
        {
            // Quick domain check: same required-spec logic
            struct DomSpec { QString kw; QStringList req; };
            const QList<DomSpec> domMap = {
                {"nomique",  {"Bioinformatique","Biologie moleculaire","Statistiques","Genetique"}},
                {"oncol",    {"Anatomopathologie","Biologie cellulaire","Pharmacologie"}},
                {"cancer",   {"Anatomopathologie","Biologie cellulaire","Pharmacologie"}},
                {"micro",    {"Bacteriologie","Virologie","Biologie moleculaire"}},
                {"immuno",   {"Immunologie clinique","Biologie cellulaire","Genetique"}},
                {"neuro",    {"Neurobiologie","Biologie cellulaire","Imagerie cerebrale"}},
                {"biotech",  {"Genie biologique","Biologie moleculaire","Biochimie"}},
                {"biochim",  {"Biochimie","Chimie analytique","Biologie moleculaire"}},
                {"pharm",    {"Pharmacologie","Toxicologie","Chimie medicale"}},
                {"gen",      {"Genetique","Biologie moleculaire","Bioinformatique"}},
            };
            QStringList required;
            QString dl = domaine.toLower();
            for (const DomSpec& ds : domMap)
                if (dl.contains(ds.kw)) { required = ds.req; break; }
            if (required.isEmpty())
                required = {"Biologie moleculaire","Statistiques","Biochimie"};

            // Current team specs
            QStringList curSpecs;
            {
                QSqlQuery q;
                QString et = QString::fromUtf8("Employ\xc3\xa9s");
                q.prepare(QString(
                    "SELECT NVL(TRIM(emp.\"specialization\"),'')"
                    " FROM \"%1\" emp"
                    " JOIN \"Associer\" a ON a.\"employee_id\"=emp.\"employee_id\""
                    " WHERE a.\"Id_projet\"=:pid"
                    " AND NVL(TRIM(emp.\"specialization\"),'')!=''").arg(et));
                q.bindValue(":pid", projId);
                if (q.exec()) while (q.next())
                    curSpecs.append(q.value(0).toString().trimmed().toLower());
            }

            // Count missing (Critique = in >60% similar projects or no data)
            int critMissing = 0;
            QStringList critNames;
            for (const QString& req : required) {
                bool found = false;
                for (const QString& cur : curSpecs)
                    if (cur.contains(req.toLower().left(5)) ||
                        req.toLower().contains(cur.left(5)))
                    { found = true; break; }
                if (!found) {
                    // Check frequency in similar projects to decide criticality
                    int simTotal = 0, simHas = 0;
                    {
                        QSqlQuery q;
                        QString termSt = QString::fromUtf8("termin\xc3\xa9");
                        q.prepare(QString(
                            "SELECT COUNT(DISTINCT p.\"Id_projet\") FROM \"projet\" p"
                            " WHERE TRIM(p.\"domaine_de_recherche\")=:dom"
                            " AND LOWER(NVL(TRIM(p.\"statut\"),''))"
                            " IN ('%1','termine')").arg(termSt));
                        q.bindValue(":dom", domaine);
                        if (q.exec() && q.next()) simTotal = q.value(0).toInt();
                    }
                    {
                        QSqlQuery q;
                        QString et2 = QString::fromUtf8("Employ\xc3\xa9s");
                        QString termSt = QString::fromUtf8("termin\xc3\xa9");
                        q.prepare(QString(
                            "SELECT COUNT(DISTINCT p.\"Id_projet\") FROM \"projet\" p"
                            " JOIN \"Associer\" a ON a.\"Id_projet\"=p.\"Id_projet\""
                            " JOIN \"%1\" emp ON emp.\"employee_id\"=a.\"employee_id\""
                            " WHERE TRIM(p.\"domaine_de_recherche\")=:dom"
                            " AND LOWER(NVL(TRIM(p.\"statut\"),''))"
                            " IN ('%2','termine')"
                            " AND LOWER(NVL(TRIM(emp.\"specialization\"),'')"
                            ") LIKE :sp").arg(et2).arg(termSt));
                        q.bindValue(":dom", domaine);
                        q.bindValue(":sp", "%" + req.toLower().left(5) + "%");
                        if (q.exec() && q.next()) simHas = q.value(0).toInt();
                    }
                    double pct = (simTotal > 0) ? (100.0*simHas/simTotal) : 80.0;
                    if (pct >= 60.0 || simTotal == 0) {
                        ++critMissing; critNames.append(req);
                    }
                }
            }
            if (critMissing > 0) {
                addRisk("Competences critiques absentes",
                        "Elevee", "Important",
                        QString("Specialisations critiques manquantes : %1. "
                                "Consulter la fonctionnalite Collaborateurs Suggeres.")
                            .arg(critNames.join(", ")),
                        "Equipe");
            }
        }

        // ═════════════════════════════════════════════════════
        //  RISK 5 — Equipement en retard de maintenance
        // ═════════════════════════════════════════════════════
        {
            QSqlQuery q;
            QString eqT = QString::fromUtf8("\xc3\x89quipement");
            QString expT = QString::fromUtf8("Exp\xc3\xa9rience");
            q.prepare(QString(
                "SELECT COUNT(*) FROM \"%1\" eq"
                " JOIN \"%2\" e ON eq.\"Id_exp\"=e.\"Id_exp\""
                " WHERE e.\"Id_projet\"=:pid"
                " AND eq.\"date_prochaine_maintenance\" < :today"
                " AND eq.\"date_prochaine_maintenance\" IS NOT NULL")
                .arg(eqT).arg(expT));
            q.bindValue(":pid",   projId);
            q.bindValue(":today", today);
            if (q.exec() && q.next() && q.value(0).toInt() > 0) {
                addRisk("Equipement non maintenu",
                        "Moyenne", "Important",
                        "Planifier la maintenance avant le lancement des"
                        " experiences. Risque de panne en cours d'experimentation.",
                        "Equipement");
            }
        }

        // ═════════════════════════════════════════════════════
        //  RISK 6 — BioSamples proches d'expiration (30 jours)
        // ═════════════════════════════════════════════════════
        {
            QSqlQuery q;
            QDate limit = today.addDays(30);
            q.prepare(
                "SELECT COUNT(*) FROM \"BioSample\""
                " WHERE \"Id_projet\"=:pid"
                " AND \"Date_dexpiration\" IS NOT NULL"
                " AND \"Date_dexpiration\" <= :lim");
            q.bindValue(":pid", projId);
            q.bindValue(":lim", limit);
            if (q.exec() && q.next() && q.value(0).toInt() > 0) {
                int n = q.value(0).toInt();
                addRisk("Echantillons biologiques bientot expires",
                        "Certaine", "Important",
                        QString("%1 echantillon(s) expirent dans moins de 30 jours. "
                                "Accelerer les experiences ou renouveler les echantillons.")
                            .arg(n),
                        "Donnees");
            }
        }

        // ═════════════════════════════════════════════════════
        //  RISK 7 — Delai irrealiste
        //  estimated_duration = estimatedBudget / 15000 months
        // ═════════════════════════════════════════════════════
        if (estimatedBudget > 0 && durationMonths > 0) {
            double estDur = qMax(1.0, estimatedBudget / 15000.0);
            if (estDur > durationMonths * 1.3) {
                addRisk("Delai probablement insuffisant",
                        "Elevee", "Important",
                        QString("Duree planifiee : %1 mois. "
                                "Duree estimee necessaire : %2 mois. "
                                "Etendre la date de fin ou reduire le scope du projet.")
                            .arg(QString::number(durationMonths,'f',1))
                            .arg(QString::number(estDur,'f',1)),
                        "Delais");
            }
        }

        // ═════════════════════════════════════════════════════
        //  RISK 8 — Aucune experience apres 3 mois
        // ═════════════════════════════════════════════════════
        if (monthsElapsed > 3.0) {
            int expCount = 0;
            {
                QSqlQuery q;
                q.prepare("SELECT COUNT(*) FROM \"" +
                          QString::fromUtf8("Exp\xc3\xa9rience") +
                          "\" WHERE \"Id_projet\"=:pid");
                q.bindValue(":pid", projId);
                if (q.exec() && q.next()) expCount = q.value(0).toInt();
            }
            if (expCount == 0) {
                addRisk("Demarrage trop lent",
                        "Certaine", "Modere",
                        QString("Le projet a demarre il y a %1 mois sans aucune"
                                " experience enregistree. "
                                "Lancer la premiere experience des que possible.")
                            .arg(QString::number(monthsElapsed,'f',1)),
                        "Delais");
            }
        }

        // ═════════════════════════════════════════════════════
        //  RISK 9 — Pas de publication apres 12 mois
        // ═════════════════════════════════════════════════════
        if (monthsElapsed > 12.0) {
            QSqlQuery q;
            QString termSt = QString::fromUtf8("termin\xc3\xa9");
            q.prepare(
                "SELECT COUNT(*) FROM \"Publication\" p"
                " JOIN \"Ecrire\" ec ON p.\"id_publication\"=ec.\"id_publication\""
                " JOIN \"Associer\" a ON ec.\"employee_id\"=a.\"employee_id\""
                " WHERE a.\"Id_projet\"=:pid"
                " AND LOWER(NVL(TRIM(p.\"status\"),'')"
                ") IN ('publie','published','accepte'))");
            q.bindValue(":pid", projId);
            if (q.exec() && q.next() && q.value(0).toInt() == 0) {
                addRisk("Absence de publications apres 12 mois",
                        "Moyenne", "Important",
                        QString("Aucune publication acceptee ou publiee apres %1 mois. "
                                "Envisager de soumettre les resultats preliminaires.")
                            .arg(QString::number(monthsElapsed,'f',1)),
                        "Impact");
            }
        }

        // ═════════════════════════════════════════════════════
        //  RISK 10 — Projet en retard ou critique
        // ═════════════════════════════════════════════════════
        if (statut=="en retard" || statut=="critique") {
            addRisk("Projet en situation critique",
                    "Certaine", "Critique",
                    "Le statut du projet est deja marque 'En retard' ou 'Critique'. "
                    "Un plan de redressement urgent est necessaire.",
                    "Delais");
        }

        // Sort by score DESC
        std::sort(risks.begin(), risks.end(),
            [](const Risk& a, const Risk& b){ return a.score > b.score; });

        // ── Build result dialog ───────────────────────────────
        QDialog* dlg = new QDialog(parent,
            Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
        dlg->setWindowTitle("Risques Probables - " + proj.nomDuProjet);
        dlg->setMinimumSize(820, 680);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->setStyleSheet("QDialog{ background:#1C2835; }");

        QVBoxLayout* mainL = new QVBoxLayout(dlg);
        mainL->setContentsMargins(22,18,22,18); mainL->setSpacing(14);

        // ── Header ────────────────────────────────────────────
        QFrame* hdr = new QFrame;
        hdr->setStyleSheet(
            "QFrame{ background:qlineargradient(x1:0,y1:0,x2:1,y2:0,"
            "stop:0 #6A0000,stop:1 #8B2F3C); border-radius:12px; }");
        QVBoxLayout* hL = new QVBoxLayout(hdr);
        hL->setContentsMargins(18,14,18,14); hL->setSpacing(6);
        QLabel* hTitle = new QLabel("Analyse des Risques - " + proj.nomDuProjet);
        hTitle->setStyleSheet(
            "color:white; font-size:15px; font-weight:900; background:transparent;");
        QLabel* hSub = new QLabel(
            QString("Domaine : %1   |   Statut : %2   |   %3 risque(s) detecte(s)")
                .arg(domaine.isEmpty() ? "non specifie" : domaine)
                .arg(proj.statut)
                .arg(risks.size()));
        hSub->setStyleSheet(
            "color:rgba(255,255,255,0.75); font-size:11px; background:transparent;");
        hL->addWidget(hTitle); hL->addWidget(hSub);
        mainL->addWidget(hdr);

        // ── Global risk level gauge + KPI strip ───────────────
        // Compute overall risk score (weighted average of top 3)
        int globalScore = 0;
        if (!risks.isEmpty()) {
            int totalW = 0, sumW = 0;
            for (int i = 0; i < qMin(risks.size(), 5); ++i) {
                int w = 5 - i;
                sumW   += risks[i].score * w;
                totalW += w;
            }
            globalScore = totalW > 0 ? sumW / totalW : 0;
        }

        QString globalColor = globalScore >= 75 ? "#8B2F3C"
                            : globalScore >= 50  ? "#CF4F5E"
                            : globalScore >= 30  ? "#D4762A"
                                                 : "#2E8B7C";
        QString globalLabel = globalScore >= 75 ? "CRITIQUE"
                            : globalScore >= 50  ? "ELEVE"
                            : globalScore >= 30  ? "MODERE"
                                                 : "FAIBLE";

        // KPI row
        QWidget* kpiRow = new QWidget; kpiRow->setStyleSheet("background:transparent;");
        QHBoxLayout* kpiL = new QHBoxLayout(kpiRow);
        kpiL->setContentsMargins(0,0,0,0); kpiL->setSpacing(10);

        auto makeKpi = [](const QString& val, const QString& label,
                          const QString& bg, const QString& fg) -> QFrame*
        {
            QFrame* f = new QFrame;
            f->setStyleSheet(QString(
                "QFrame{ background:%1; border-radius:10px; border:none; }").arg(bg));
            QVBoxLayout* fL = new QVBoxLayout(f);
            fL->setContentsMargins(12,10,12,10); fL->setSpacing(2);
            QLabel* v = new QLabel(val);
            v->setAlignment(Qt::AlignCenter);
            v->setStyleSheet(QString(
                "color:%1; font-size:20px; font-weight:900; background:transparent;").arg(fg));
            QLabel* l = new QLabel(label);
            l->setAlignment(Qt::AlignCenter);
            l->setStyleSheet(
                "color:rgba(255,255,255,0.50); font-size:9px;"
                " font-weight:700; background:transparent;");
            fL->addWidget(v); fL->addWidget(l);
            return f;
        };

        // Count by score tier
        int nCritical = 0, nHigh = 0, nMed = 0, nLow = 0;
        for (const Risk& r : risks) {
            if (r.score >= 75)     ++nCritical;
            else if (r.score >= 50) ++nHigh;
            else if (r.score >= 30) ++nMed;
            else                    ++nLow;
        }

        // Global score pill
        QFrame* globalPill = makeKpi(
            QString::number(globalScore) + "%",
            "SCORE GLOBAL",
            QString("rgba(139,47,60,0.30)"), globalColor);
        globalPill->setStyleSheet(QString(
            "QFrame{ background:rgba(139,47,60,0.25);"
            " border:2px solid %1; border-radius:10px; }").arg(globalColor));
        kpiL->addWidget(globalPill, 2);

        kpiL->addWidget(makeKpi(QString::number(risks.size()), "Risques totaux",
            "rgba(255,255,255,0.06)", "white"), 1);
        kpiL->addWidget(makeKpi(QString::number(nCritical), "Critiques",
            "rgba(139,47,60,0.20)", "#CF4F5E"), 1);
        kpiL->addWidget(makeKpi(QString::number(nHigh), "Eleves",
            "rgba(207,79,94,0.15)", "#D4762A"), 1);
        kpiL->addWidget(makeKpi(QString::number(nMed), "Moderes",
            "rgba(212,118,42,0.15)", "#D4B82A"), 1);
        kpiL->addWidget(makeKpi(QString::number(nLow), "Faibles",
            "rgba(46,139,124,0.15)", "#2E8B7C"), 1);
        mainL->addWidget(kpiRow);

        // Global score bar
        QWidget* gaugeBg = new QWidget; gaugeBg->setFixedHeight(10);
        gaugeBg->setStyleSheet(
            "background:rgba(255,255,255,0.08); border-radius:5px;");
        QWidget* gaugeFill = new QWidget(gaugeBg); gaugeFill->setFixedHeight(10);
        gaugeFill->setStyleSheet(
            QString("background:qlineargradient(x1:0,y1:0,x2:1,y2:0,"
                    "stop:0 #2E8B7C,stop:0.4 #D4762A,stop:0.7 #CF4F5E,"
                    "stop:1 #8B2F3C); border-radius:5px;"));
        int capturedScore = globalScore;
        QTimer::singleShot(0, gaugeBg, [gaugeFill, gaugeBg, capturedScore](){
            gaugeFill->setFixedWidth(gaugeBg->width() * capturedScore / 100);
        });
        QLabel* gaugeLabel = new QLabel(
            QString("Niveau de risque global : %1 (%2/100)")
                .arg(globalLabel).arg(globalScore));
        gaugeLabel->setStyleSheet(QString(
            "color:%1; font-size:10px; font-weight:800; background:transparent;")
            .arg(globalColor));
        mainL->addWidget(gaugeLabel);
        mainL->addWidget(gaugeBg);

        // ── Risk list (scrollable) ────────────────────────────
        QScrollArea* scroll = new QScrollArea;
        scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame);
        scroll->setStyleSheet(
            "QScrollArea{ background:transparent; border:none; }"
            "QScrollBar:vertical{ width:7px; background:transparent; }"
            "QScrollBar::handle:vertical{"
            " background:rgba(139,47,60,0.35); border-radius:4px; }");
        QWidget* scrollW = new QWidget; scrollW->setStyleSheet("background:transparent;");
        QVBoxLayout* scrollL = new QVBoxLayout(scrollW);
        scrollL->setContentsMargins(0,0,4,0); scrollL->setSpacing(10);

        if (risks.isEmpty()) {
            QFrame* okCard = new QFrame;
            okCard->setStyleSheet(
                "QFrame{ background:rgba(46,139,124,0.12);"
                " border:2px solid rgba(46,139,124,0.35); border-radius:12px; }");
            QVBoxLayout* okL = new QVBoxLayout(okCard);
            okL->setContentsMargins(24,24,24,24); okL->setSpacing(8);
            QLabel* okIco = new QLabel("Aucun risque majeur detecte");
            okIco->setStyleSheet(
                "color:#2E8B7C; font-size:20px; font-weight:900; background:transparent;");
            okIco->setAlignment(Qt::AlignCenter);
            QLabel* okTxt = new QLabel(
                "Le projet est en bonne sante selon les criteres analyses.\n"
                "Continuez a surveiller regulierement.");
            okTxt->setStyleSheet(
                "color:rgba(255,255,255,0.65); font-size:12px; background:transparent;");
            okTxt->setAlignment(Qt::AlignCenter);
            okL->addWidget(okIco); okL->addWidget(okTxt);
            scrollL->addWidget(okCard);
        }

        // Category icons
        QMap<QString,QString> catIcon = {
            {"Budget",     "B"}, {"Equipe",     "E"},
            {"Ethique",    "ETH"}, {"Donnees",  "D"},
            {"Delais",     "T"}, {"Equipement", "EQ"},
            {"Impact",     "I"}
        };
        QMap<QString,QString> catColor = {
            {"Budget",     "#2A649B"}, {"Equipe",    "#0A5F58"},
            {"Ethique",    "#7B4D9E"}, {"Donnees",   "#B5672C"},
            {"Delais",     "#D4762A"}, {"Equipement","#518195"},
            {"Impact",     "#416E66"}
        };

        for (int i = 0; i < risks.size(); ++i) {
            const Risk& r = risks[i];

            // Score-based card accent
            QString cardBorder = r.score >= 75 ? "#8B2F3C"
                               : r.score >= 50  ? "#CF4F5E"
                               : r.score >= 30  ? "#D4762A"
                                                 : "#2E8B7C";
            QString cardBg = r.score >= 75
                ? "rgba(139,47,60,0.12)" : "rgba(255,255,255,0.04)";

            QFrame* card = new QFrame;
            card->setStyleSheet(QString(
                "QFrame{ background:%1;"
                " border:1px solid rgba(255,255,255,0.08);"
                " border-left:5px solid %2; border-radius:10px; }")
                .arg(cardBg).arg(cardBorder));
            QVBoxLayout* cardL = new QVBoxLayout(card);
            cardL->setContentsMargins(16,12,16,12); cardL->setSpacing(8);

            // ── Top row ───────────────────────────────────────
            QHBoxLayout* topRow = new QHBoxLayout;
            topRow->setContentsMargins(0,0,0,0); topRow->setSpacing(10);

            // Rank + category circle
            QLabel* rankLbl = new QLabel(QString::number(i + 1));
            rankLbl->setFixedSize(32,32); rankLbl->setAlignment(Qt::AlignCenter);
            rankLbl->setStyleSheet(QString(
                "background:%1; color:white; border-radius:16px;"
                " font-size:13px; font-weight:900;").arg(cardBorder));
            topRow->addWidget(rankLbl);

            // Title + category
            QWidget* titleBlock = new QWidget;
            titleBlock->setStyleSheet("background:transparent;");
            QVBoxLayout* tbL = new QVBoxLayout(titleBlock);
            tbL->setContentsMargins(0,0,0,0); tbL->setSpacing(2);
            QLabel* titleLbl = new QLabel(r.titre);
            titleLbl->setStyleSheet(
                "color:white; font-size:13px; font-weight:900; background:transparent;");

            QString catC = catColor.value(r.category, "#518195");
            QLabel* catLbl = new QLabel(r.category);
            catLbl->setStyleSheet(QString(
                "color:%1; font-size:9px; font-weight:800; background:transparent;")
                .arg(catC));
            tbL->addWidget(titleLbl); tbL->addWidget(catLbl);
            topRow->addWidget(titleBlock, 1);

            // Score bar (vertical) + number
            QWidget* scoreW = new QWidget; scoreW->setStyleSheet("background:transparent;");
            QVBoxLayout* swL = new QVBoxLayout(scoreW);
            swL->setContentsMargins(0,0,0,0); swL->setSpacing(3);
            QLabel* scoreLbl = new QLabel(QString("Score : %1").arg(r.score));
            scoreLbl->setAlignment(Qt::AlignRight);
            scoreLbl->setStyleSheet(QString(
                "color:%1; font-size:12px; font-weight:900; background:transparent;")
                .arg(cardBorder));
            QWidget* sBar = new QWidget; sBar->setFixedSize(100,6);
            sBar->setStyleSheet("background:rgba(255,255,255,0.10); border-radius:3px;");
            QWidget* sFill = new QWidget(sBar); sFill->setFixedHeight(6);
            sFill->setStyleSheet(
                QString("background:%1; border-radius:3px;").arg(cardBorder));
            int fw = r.score;
            QTimer::singleShot(0, sBar, [sFill, sBar, fw](){
                sFill->setFixedWidth(sBar->width() * fw / 100);
            });
            swL->addWidget(scoreLbl); swL->addWidget(sBar, 0, Qt::AlignRight);
            topRow->addWidget(scoreW);

            // Probability + impact badges
            QVBoxLayout* badgesL = new QVBoxLayout;
            badgesL->setContentsMargins(0,0,0,0); badgesL->setSpacing(4);

            auto makeBadge = [](const QString& text, const QString& color) -> QLabel* {
                QLabel* b = new QLabel(QString("  %1  ").arg(text));
                b->setFixedHeight(20);
                b->setAlignment(Qt::AlignCenter);
                b->setStyleSheet(QString(
                    "color:white; background:%1; border-radius:5px;"
                    " font-size:9px; font-weight:800;").arg(color));
                return b;
            };
            badgesL->addWidget(makeBadge("P: " + r.probabilite, r.probColor));
            badgesL->addWidget(makeBadge("I: " + r.impact,      r.impactColor));
            topRow->addLayout(badgesL);
            cardL->addLayout(topRow);

            // ── Separator ─────────────────────────────────────
            QFrame* sep = new QFrame; sep->setFrameShape(QFrame::HLine);
            sep->setStyleSheet(
                "color:rgba(255,255,255,0.07); background:rgba(255,255,255,0.07);"
                " border:none; max-height:1px;");
            cardL->addWidget(sep);

            // ── Mitigation section ────────────────────────────
            QHBoxLayout* mitRow = new QHBoxLayout;
            mitRow->setContentsMargins(0,0,0,0); mitRow->setSpacing(8);
            QLabel* mitIcon = new QLabel("Mitigation");
            mitIcon->setStyleSheet(QString(
                "color:%1; font-size:9px; font-weight:900;"
                " background:rgba(255,255,255,0.06);"
                " border-radius:4px; padding:2px 6px;").arg(catC));
            mitRow->addWidget(mitIcon);
            QLabel* mitLbl = new QLabel(r.mitigation);
            mitLbl->setWordWrap(true);
            mitLbl->setStyleSheet(
                "color:rgba(255,255,255,0.70); font-size:10px; background:transparent;");
            mitRow->addWidget(mitLbl, 1);
            cardL->addLayout(mitRow);

            scrollL->addWidget(card);
        }

        // ── Probability × Impact matrix legend ───────────────
        if (!risks.isEmpty()) {
            QFrame* matCard = new QFrame;
            matCard->setStyleSheet(
                "QFrame{ background:rgba(255,255,255,0.04);"
                " border:1px solid rgba(255,255,255,0.08);"
                " border-radius:10px; }");
            QVBoxLayout* matL2 = new QVBoxLayout(matCard);
            matL2->setContentsMargins(14,10,14,10); matL2->setSpacing(6);
            QLabel* matHdr = new QLabel("Legende : Probabilite x Impact");
            matHdr->setStyleSheet(
                "color:rgba(255,255,255,0.40); font-size:9px;"
                " font-weight:800; background:transparent;");
            matL2->addWidget(matHdr);
            QHBoxLayout* matRow = new QHBoxLayout;
            matRow->setSpacing(8);
            struct MatEntry { QString label; QString color; };
            const QList<MatEntry> entries = {
                {"Score 75-100 : Critique", "#8B2F3C"},
                {"Score 50-74 : Eleve",     "#CF4F5E"},
                {"Score 30-49 : Modere",    "#D4762A"},
                {"Score 0-29  : Faible",    "#2E8B7C"},
            };
            for (const MatEntry& e : entries) {
                QWidget* ew = new QWidget; ew->setStyleSheet("background:transparent;");
                QHBoxLayout* el = new QHBoxLayout(ew);
                el->setContentsMargins(0,0,0,0); el->setSpacing(5);
                QLabel* sq = new QLabel;
                sq->setFixedSize(10,10);
                sq->setStyleSheet(QString(
                    "background:%1; border-radius:2px;").arg(e.color));
                QLabel* tx = new QLabel(e.label);
                tx->setStyleSheet(
                    "color:rgba(255,255,255,0.40); font-size:9px;"
                    " background:transparent;");
                el->addWidget(sq); el->addWidget(tx);
                matRow->addWidget(ew, 1);
            }
            matL2->addLayout(matRow);
            scrollL->addWidget(matCard);
        }

        scrollL->addStretch(1);
        scroll->setWidget(scrollW);
        mainL->addWidget(scroll, 1);

        // ── Footer ────────────────────────────────────────────
        QFrame* foot = new QFrame;
        foot->setStyleSheet(
            "QFrame{ background:rgba(255,255,255,0.04); border-radius:8px; }");
        QHBoxLayout* fl = new QHBoxLayout(foot);
        fl->setContentsMargins(12,8,12,8);
        QLabel* note = new QLabel(
            "Analyse basee sur : budget, ethique, equipe, specialisations,"
            " equipements, echantillons, duree et publications."
            " Distinct du score de sante (radar).");
        note->setWordWrap(true);
        note->setStyleSheet(
            "color:rgba(255,255,255,0.28); font-size:9px; background:transparent;");
        fl->addWidget(note, 1);
        QPushButton* cb = new QPushButton("Fermer");
        cb->setFixedSize(90,32);
        cb->setStyleSheet(
            "QPushButton{ background:#8B2F3C; color:white; border-radius:7px;"
            " font-weight:700; }"
            "QPushButton:hover{ background:#6A2030; }");
        QObject::connect(cb, &QPushButton::clicked, dlg, &QDialog::accept);
        fl->addWidget(cb);
        mainL->addWidget(foot);

        picker->close();
        dlg->exec();
    });

    picker->exec();
}
