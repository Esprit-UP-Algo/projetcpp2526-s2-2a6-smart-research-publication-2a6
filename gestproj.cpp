#include "gestproj.h"
#include <QRegularExpression>

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
        static const QRegularExpression allowed(R"(^[A-Za-zÀ-ÖØ-öø-ÿ0-9 \-()/]+$)");
        if (!allowed.match(nom).hasMatch())
            return "Le nom du projet contient des caractères non autorisés.\nAutorisés : lettres, chiffres, espaces, tirets, parenthèses.";
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
        return "La source de financement est obligatoire lorsque le budget est supérieur à 0.";
    if (in.sourceDeFinancement.trimmed().length() > 150)
        return "La source de financement ne peut pas dépasser 150 caractères.";

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
        const QColor* pal;
        int palSz;

        PaintFilter(QList<QPair<QString,int>> it, int mv,
                    const QColor* p, int ps, QObject* parent)
            : QObject(parent), items(it), maxValue(mv), pal(p), palSz(ps) {}

        bool eventFilter(QObject* obj, QEvent* ev) override {
            if (ev->type() != QEvent::Paint) return false;
            QWidget* w = static_cast<QWidget*>(obj);
            QPainter painter(w);
            painter.setRenderHint(QPainter::Antialiasing);

            const int W       = w->width();
            const int barH    = 34;
            const int rowH    = 52;
            const int labelW  = 190;
            const int valW    = 80;
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

                // Count text
                QFont nf("Segoe UI", 10, QFont::Bold);
                painter.setFont(nf);
                painter.setPen(QColor("#0A5F58"));
                QRect numRect(padLeft + labelW + barW + 8, y, valW, barH);
                painter.drawText(numRect, Qt::AlignVCenter | Qt::AlignLeft,
                                 QString::number(cnt) + " projet" + (cnt > 1 ? "s" : ""));
            }
            return true;
        }
    };

    chart->installEventFilter(
        new PaintFilter(sorted, maxVal, palette, palSize, chart));

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
    int totalProjects = 0;
    for (const auto& kv : sorted) totalProjects += kv.second;

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
        const QColor* pal;
        int palSz;

        BudgetPaintFilter(QList<QPair<QString,double>> it, double mv,
                          const QColor* p, int ps, QObject* parent)
            : QObject(parent), items(it), maxValue(mv), pal(p), palSz(ps) {}

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

                // Format: use k / M suffixes for readability
                QString budStr;
                if (bud >= 1000000.0)
                    budStr = QString::number(bud / 1000000.0, 'f', 2) + " M TND";
                else if (bud >= 1000.0)
                    budStr = QString::number(bud / 1000.0, 'f', 1) + " k TND";
                else
                    budStr = QString::number(bud, 'f', 2) + " TND";

                QRect numRect(padLeft + labelW + barW + 8, y, valW, barH);
                painter.drawText(numRect, Qt::AlignVCenter | Qt::AlignLeft, budStr);
            }
            return true;
        }
    };

    chart->installEventFilter(
        new BudgetPaintFilter(sorted, maxVal, palette, palSize, chart));

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
    double totalBudget = 0.0;
    for (const auto& kv : sorted) totalBudget += kv.second;

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
        const QColor* pal;
        int palSz;

        DomBudgetPF(QList<QPair<QString,double>> it, double mv,
                    const QColor* p, int ps, QObject* par)
            : QObject(par), items(it), maxValue(mv), pal(p), palSz(ps) {}

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

                QString budStr;
                if (bud >= 1000000.0)
                    budStr = QString::number(bud / 1000000.0, 'f', 2) + " M TND";
                else if (bud >= 1000.0)
                    budStr = QString::number(bud / 1000.0, 'f', 1) + " k TND";
                else
                    budStr = QString::number(bud, 'f', 2) + " TND";

                QRect numRect(padLeft + labelW + barW + 8, y, valW, barH);
                painter.drawText(numRect, Qt::AlignVCenter | Qt::AlignLeft, budStr);
            }
            return true;
        }
    };

    chart->installEventFilter(
        new DomBudgetPF(sorted, maxVal, palette, palSize, chart));

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
    double totalBudget = 0.0;
    for (const auto& kv : sorted) totalBudget += kv.second;

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
        { "En cours",     QColor("#416e66") },
        { "Planifié",     QColor("#518195") },
        { "Terminé",      QColor("#367e71") },
        { "Suspendu",     QColor("#7A8B8A") },
        { "En retard",    QColor("#ae7040") },
        { "Critique",     QColor("#8B2F3C") },
        { "Annulé",       QColor("#547e76") },
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
                if (pct >= 8.0) {
                    double lr = qDegreesToRadians(start - span / 2.0);
                    double ld = (r + holeR) / 2.0;
                    QPointF lp(cx + ld * std::cos(lr), cy - ld * std::sin(lr));
                    QFont lf("Segoe UI", 9, QFont::Bold);
                    p.setFont(lf);
                    p.setPen(QColor(255,255,255,230));
                    p.drawText(QRectF(lp.x()-26, lp.y()-10, 52, 20),
                               Qt::AlignCenter,
                               QString::number(pct,'f',1)+"%");
                }
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
