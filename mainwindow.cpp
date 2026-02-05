#include "mainwindow.h"

#include <QApplication>
#include <QStackedWidget>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QTextEdit>
#include <QDateEdit>
#include <QDialog>
#include <QMessageBox>
#include <QGroupBox>
#include <QScrollArea>
#include <QSplitter>
#include <QDoubleSpinBox>
#include <QProgressBar>
#include <QPainter>
#include <QtMath>
#include <QItemSelectionModel>
#include <QLocale>

// ===== COLORS =====
static const QString C_DARK_GREEN = "#12443B";
static const QString C_PRIMARY    = "#0A5F58";
static const QString C_BG         = "#A3CAD3";
static const QString C_CARD       = "#EEF2F1";
static const QString C_BEIGE      = "#C6B29A";
static const QString C_TEXT       = "#2E3A33"; // darker text for better contrast

// ===== DonutChart (simple) =====
class DonutChart : public QWidget {
public:
    struct Slice { double value; QColor color; QString label; };
    explicit DonutChart(QWidget* parent=nullptr) : QWidget(parent) { setMinimumHeight(160); }
    void setData(const QList<Slice>& s) { m_slices = s; update(); }
    const QList<Slice>& data() const { return m_slices; }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        QRect r = rect().adjusted(10,10,-10,-10);
        int d = qMin(r.width(), r.height());
        QRect pie(r.left(), r.top(), d, d);

        double total = 0;
        for (auto &s : m_slices) total += s.value;
        if (total <= 0) {
            p.setPen(QColor(0,0,0,60));
            p.drawText(rect(), Qt::AlignCenter, "No data");
            return;
        }

        p.setPen(Qt::NoPen);

        int thickness = qMax(8, d/4);
        QRect inner = pie.adjusted(thickness, thickness, -thickness, -thickness);

        double start = 90.0 * 16;
        for (auto &s : m_slices) {
            double span = - (s.value / total) * 360.0 * 16;
            p.setBrush(s.color);
            p.drawPie(pie, (int)start, (int)span);

            double midDeg = (start + span/2.0) / 16.0;
            double rad = qDegreesToRadians(midDeg);
            QPointF c = pie.center();
            double rr = d * 0.38;
            QPointF pos(c.x() + rr * std::cos(rad), c.y() - rr * std::sin(rad));
            int pct = (int)std::round((s.value / total) * 100.0);
            p.setPen(QColor(255,255,255,230));
            QFont f = font(); f.setBold(true); f.setPointSize(9);
            p.setFont(f);
            p.drawText(QRectF(pos.x()-18, pos.y()-10, 36, 20), Qt::AlignCenter, QString("%1%").arg(pct));

            start += span;
        }

        p.setBrush(QColor(245,247,246,255));
        p.setPen(Qt::NoPen);
        p.drawEllipse(inner);

        p.setPen(QColor(0,0,0,120));
        QFont f = font(); f.setBold(true); f.setPointSize(10);
        p.setFont(f);
        p.drawText(inner, Qt::AlignCenter, " ");
    }

private:
    QList<Slice> m_slices;
};

// ===== Helper UI functions =====
static QFrame* card(QWidget *parent=nullptr)
{
    QFrame *c = new QFrame(parent);
    c->setStyleSheet(
        "QFrame { background:" + C_CARD +
        "; border-radius:12px; padding:12px; }"
        );
    return c;
}

static QPushButton* styledButton(const QString &text, const QString &color = C_PRIMARY)
{
    QPushButton *btn = new QPushButton(text);
    btn->setStyleSheet(
        "QPushButton {"
        "  background:" + color + ";"
                  "  color: white;"
                  "  border: none;"
                  "  border-radius: 6px;"
                  "  padding: 8px 14px;"
                  "  font-weight: 600;"
                  "  font-size: 13px;"
                  "}"
                  "QPushButton:hover {"
                  "  background:" + C_DARK_GREEN + ";"
                         "}"
                         "QPushButton:pressed {"
                         "  background: #083d37;"
                         "}"
        );
    btn->setCursor(Qt::PointingHandCursor);
    return btn;
}

// ================= MainWindow =================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), selectedRow(-1)
{
    setWindowTitle("🔬 Module – Gestion des Projets de Recherche");
    resize(1200, 720);

    QWidget *root = new QWidget(this);
    root->setStyleSheet("background:" + C_BG + ";");
    setCentralWidget(root);

    stack = new QStackedWidget;
    QVBoxLayout *main = new QVBoxLayout(root);
    main->setContentsMargins(0, 0, 0, 0);
    main->addWidget(stack);

    // create donut placeholders
    statsDonut = new DonutChart;
    budgetDonut = new DonutChart;

    stack->addWidget(pageListeProjets());      // 0
    stack->addWidget(pageAjoutProjet());       // 1
    stack->addWidget(pageStats());             // 2
    stack->addWidget(pageDetails());           // 3
    stack->addWidget(pageModifierProjet());    // 4
    stack->addWidget(pageSuiviBudgetaire());   // 5

    // apply style to any existing rows and update stats
    for (int r = 0; r < tableProjets->rowCount(); ++r) applyRowStyle(r);
    updateStats();
}

// ================= Page: Liste =================
QWidget* MainWindow::pageListeProjets()
{
    QWidget *w = new QWidget;
    QHBoxLayout *mainLayout = new QHBoxLayout(w);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Sidebar
    QFrame *sidebar = new QFrame;
    sidebar->setFixedWidth(220);
    sidebar->setStyleSheet("background:" + C_PRIMARY + ";");

    QVBoxLayout *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(20, 30, 20, 20);

    QLabel *logo = new QLabel("🔬");
    logo->setStyleSheet("font-size: 44px;");
    logo->setAlignment(Qt::AlignCenter);

    QLabel *appTitle = new QLabel("Gestion\nProjets");
    appTitle->setStyleSheet("color: white; font-size: 16px; font-weight: 700;");
    appTitle->setAlignment(Qt::AlignCenter);
    appTitle->setWordWrap(true);

    sidebarLayout->addWidget(logo);
    sidebarLayout->addWidget(appTitle);
    sidebarLayout->addSpacing(24);

    QPushButton *btnListe = styledButton("📋 Liste", C_BEIGE);
    QPushButton *btnStats = styledButton("📊 Statistiques", C_PRIMARY);
    QPushButton *btnBudget = styledButton("💰 Suivi Budgétaire", C_PRIMARY);

    sidebarLayout->addWidget(btnListe);
    sidebarLayout->addWidget(btnStats);
    sidebarLayout->addWidget(btnBudget);
    sidebarLayout->addStretch();

    connect(btnStats, &QPushButton::clicked, this, [=](){ updateStats(); stack->setCurrentIndex(2); });
    connect(btnBudget, &QPushButton::clicked, this, [=](){ updateStats(); stack->setCurrentIndex(5); });

    // Content
    QWidget *content = new QWidget;
    QVBoxLayout *l = new QVBoxLayout(content);
    l->setContentsMargins(24, 24, 24, 24);

    QLabel *title = new QLabel("📋 Liste des projets de recherche");
    title->setStyleSheet("font-size:20px; font-weight:700; color:" + C_TEXT + ";");
    l->addWidget(title);

    l->addSpacing(12);

    // Search / controls
    QFrame *bar = card();
    QHBoxLayout *b = new QHBoxLayout(bar);
    QLineEdit *search = new QLineEdit;
    search->setPlaceholderText("🔍 Rechercher par nom, domaine, chercheur, source...");
    search->setStyleSheet("QLineEdit { padding: 8px; color:" + C_TEXT + "; }");

    QComboBox *tri = new QComboBox;
    tri->addItems({"Trier par date", "Trier par budget", "Trier par statut", "Trier par domaine"});
    tri->setFixedWidth(200);

    QPushButton *exporter = styledButton("📤 Exporter (PDF / Excel)", C_BEIGE);

    b->addWidget(search, 3);
    b->addWidget(tri);
    b->addWidget(exporter);

    l->addWidget(bar);
    l->addSpacing(12);

    // Table card
    QFrame *tableCard = card();
    QVBoxLayout *tc = new QVBoxLayout(tableCard);

    tableProjets = new QTableWidget(0, 9);
    tableProjets->setHorizontalHeaderLabels({
        "ID", "Nom du projet", "Domaine", "Budget (DT)",
        "Date début", "Date fin", "Statut", "Publications", "Actions"
    });

    tableProjets->horizontalHeader()->setStyleSheet(
        "QHeaderView::section { background: "+ C_PRIMARY +"; color: white; padding: 8px; border:none; font-weight:600; }"
        );

    // Force item text color to black by default and don't rely on alternating rows (we style Terminé manually)
    tableProjets->setAlternatingRowColors(false);
    tableProjets->setStyleSheet(
        "QTableWidget { border: none; background: white; gridline-color: rgba(0,0,0,0.06); }"
        "QTableWidget::item { padding: 8px; color: black; }"
        );

    tableProjets->horizontalHeader()->setStretchLastSection(true);
    tableProjets->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableProjets->verticalHeader()->setVisible(false);
    tableProjets->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableProjets->setSelectionMode(QAbstractItemView::SingleSelection);

    // Add sample data
    addProjetToTable("PRJ001", "Étude génomique du cancer", "Génomique", "150000",
                     "01/01/2024", "31/12/2024", "En cours", "5");
    addProjetToTable("PRJ002", "Développement vaccin COVID", "Immunologie", "300000",
                     "15/02/2024", "15/02/2026", "En cours", "12");
    addProjetToTable("PRJ003", "Analyse protéomique", "Protéomique", "80000",
                     "10/06/2023", "10/12/2023", "Terminé", "8");

    // Ensure all existing rows get styled (Terminé rows)
    for (int r = 0; r < tableProjets->rowCount(); ++r) applyRowStyle(r);

    tc->addWidget(tableProjets);
    l->addWidget(tableCard);

    l->addSpacing(10);

    // Actions
    QHBoxLayout *actions = new QHBoxLayout;
    QPushButton *add = styledButton("➕ Ajouter", C_PRIMARY);
    QPushButton *edit = styledButton("✏️ Modifier", C_BEIGE);
    QPushButton *details = styledButton("��️ Détails", C_DARK_GREEN);
    QPushButton *del = styledButton("🗑️ Supprimer", "#c44536");

    actions->addWidget(add);
    actions->addWidget(edit);
    actions->addWidget(details);
    actions->addWidget(del);
    actions->addStretch();

    l->addLayout(actions);

    // Keep selectedRow in sync when user selects a row
    connect(tableProjets->selectionModel(), &QItemSelectionModel::selectionChanged, this, [=](const QItemSelection &sel, const QItemSelection &){
        if (!sel.indexes().isEmpty()) {
            selectedRow = sel.indexes().first().row();
        } else {
            selectedRow = -1;
        }
    });

    // Double-click also opens details
    connect(tableProjets, &QTableWidget::cellDoubleClicked, this, [=](int r, int){
        selectedRow = r;
        updateDetailsUI(selectedRow);
        stack->setCurrentIndex(3);
    });

    connect(add, &QPushButton::clicked, this, [=](){
        clearForm();
        stack->setCurrentIndex(1);
    });

    connect(edit, &QPushButton::clicked, this, [=](){
        int row = tableProjets->currentRow();
        if(row >= 0) {
            selectedRow = row;
            fillFormForEdit(selectedRow);
            stack->setCurrentIndex(4);
        } else {
            QMessageBox::warning(this, "Aucune sélection", "⚠️ Veuillez sélectionner un projet à modifier.");
        }
    });

    connect(details, &QPushButton::clicked, this, [=](){
        int row = tableProjets->currentRow();
        if(row >= 0) {
            selectedRow = row;
            updateDetailsUI(selectedRow);
            stack->setCurrentIndex(3);
        } else {
            QMessageBox::warning(this, "Aucune sélection", "⚠️ Veuillez sélectionner un projet.");
        }
    });

    connect(del, &QPushButton::clicked, this, [=](){
        int row = tableProjets->currentRow();
        if(row >= 0) {
            selectedRow = row;
            dialogSuppression()->exec();
            updateStats();
        } else {
            QMessageBox::warning(this, "Aucune sélection", "⚠️ Veuillez sélectionner un projet à supprimer.");
        }
    });

    mainLayout->addWidget(sidebar);
    mainLayout->addWidget(content);

    return w;
}

// ================= Page: Ajout =================
QWidget* MainWindow::pageAjoutProjet()
{
    QWidget *w = new QWidget;
    w->setStyleSheet("background:" + C_BG + ";");
    QVBoxLayout *mainLayout = new QVBoxLayout(w);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    QLabel *title = new QLabel("➕ Ajouter un nouveau projet de recherche");
    title->setStyleSheet("font-size:18px; font-weight:700; color:" + C_TEXT + ";");
    mainLayout->addWidget(title);

    mainLayout->addSpacing(10);

    QScrollArea *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);

    QWidget *scrollContent = new QWidget;
    QVBoxLayout *contentLayout = new QVBoxLayout(scrollContent);

    QFrame *form = card();
    // Make sure labels inside are readable
    form->setStyleSheet("QLabel { color: " + C_TEXT + "; } QLineEdit, QComboBox, QDateEdit, QDoubleSpinBox { color: " + C_TEXT + "; background: white; }");

    QVBoxLayout *f = new QVBoxLayout(form);

    QString labelStyle = "font-weight: 600; color: " + C_TEXT + "; font-size: 13px;";
    QString inputStyle = "QLineEdit, QComboBox, QDateEdit, QDoubleSpinBox { padding:6px; background:white; color:" + C_TEXT + "; }";

    QLabel *lblNom = new QLabel("Nom du projet *"); lblNom->setStyleSheet(labelStyle);
    editNomProjet = new QLineEdit; editNomProjet->setPlaceholderText("Ex: Étude génomique du cancer"); editNomProjet->setStyleSheet(inputStyle);
    f->addWidget(lblNom); f->addWidget(editNomProjet);

    QLabel *lblDomaine = new QLabel("Domaine de recherche *"); lblDomaine->setStyleSheet(labelStyle);
    comboDomaine = new QComboBox; comboDomaine->addItems({"Sélectionner...", "Génomique", "Protéomique", "Développement de médicaments", "Biologie moléculaire", "Immunologie"});
    comboDomaine->setStyleSheet(inputStyle);
    f->addWidget(lblDomaine); f->addWidget(comboDomaine);

    QLabel *lblBudget = new QLabel("Budget (DT) *"); lblBudget->setStyleSheet(labelStyle);
    spinBudget = new QDoubleSpinBox; spinBudget->setMaximum(100000000); spinBudget->setSuffix(" DT"); spinBudget->setStyleSheet(inputStyle);
    f->addWidget(lblBudget); f->addWidget(spinBudget);

    QLabel *lblFinancement = new QLabel("Source de financement *"); lblFinancement->setStyleSheet(labelStyle);
    editFinancement = new QLineEdit; editFinancement->setPlaceholderText("Ex: Ministère de la Recherche"); editFinancement->setStyleSheet(inputStyle);
    f->addWidget(lblFinancement); f->addWidget(editFinancement);

    // Dates
    QHBoxLayout *datesLayout = new QHBoxLayout;
    QLabel *lblStart = new QLabel("Date de début *"); lblStart->setStyleSheet(labelStyle);
    editStartDate = new QDateEdit; editStartDate->setCalendarPopup(true); editStartDate->setDate(QDate::currentDate()); editStartDate->setStyleSheet(inputStyle);
    QLabel *lblEnd = new QLabel("Date de fin prévue *"); lblEnd->setStyleSheet(labelStyle);
    editEndDate = new QDateEdit; editEndDate->setCalendarPopup(true); editEndDate->setDate(QDate::currentDate().addYears(1)); editEndDate->setStyleSheet(inputStyle);
    QVBoxLayout *sl = new QVBoxLayout; sl->addWidget(lblStart); sl->addWidget(editStartDate);
    QVBoxLayout *el = new QVBoxLayout; el->addWidget(lblEnd); el->addWidget(editEndDate);
    datesLayout->addLayout(sl); datesLayout->addLayout(el);
    f->addLayout(datesLayout);

    QLabel *lblStatus = new QLabel("Statut"); lblStatus->setStyleSheet(labelStyle);
    comboStatus = new QComboBox; comboStatus->addItems({"En cours", "Terminé", "Suspendu", "En attente"}); comboStatus->setStyleSheet(inputStyle);
    f->addWidget(lblStatus); f->addWidget(comboStatus);

    QLabel *lblEthique = new QLabel("Numéro d'approbation éthique"); lblEthique->setStyleSheet(labelStyle);
    editEthique = new QLineEdit; editEthique->setStyleSheet(inputStyle);
    f->addWidget(lblEthique); f->addWidget(editEthique);

    QLabel *lblPublications = new QLabel("Nombre de publications prévues"); lblPublications->setStyleSheet(labelStyle);
    spinPublications = new QSpinBox; spinPublications->setMaximum(1000); spinPublications->setStyleSheet(inputStyle);
    f->addWidget(lblPublications); f->addWidget(spinPublications);

    f->addStretch();

    QHBoxLayout *btns = new QHBoxLayout;
    QPushButton *save = styledButton("💾 Enregistrer", C_PRIMARY);
    QPushButton *cancel = styledButton("❌ Annuler", "#999");
    btns->addWidget(save); btns->addWidget(cancel); btns->addStretch();
    f->addLayout(btns);

    contentLayout->addWidget(form);
    scroll->setWidget(scrollContent);
    mainLayout->addWidget(scroll);

    connect(cancel, &QPushButton::clicked, this, [=](){ stack->setCurrentIndex(0); });

    connect(save, &QPushButton::clicked, this, [=](){
        if(editNomProjet->text().isEmpty() || comboDomaine->currentIndex() == 0 || editFinancement->text().isEmpty()) {
            QMessageBox::warning(this, "Champs requis", "⚠️ Veuillez remplir tous les champs obligatoires (*).");
            return;
        }

        QString newId = QString("PRJ%1").arg(tableProjets->rowCount() + 1, 3, 10, QChar('0'));
        addProjetToTable(
            newId,
            editNomProjet->text(),
            comboDomaine->currentText(),
            QString::number(spinBudget->value(), 'f', 2),
            editStartDate->date().toString("dd/MM/yyyy"),
            editEndDate->date().toString("dd/MM/yyyy"),
            comboStatus->currentText(),
            QString::number(spinPublications->value())
            );

        QMessageBox::information(this, "Succès", "✅ Projet ajouté avec succès !");
        clearForm();
        updateStats();
        stack->setCurrentIndex(0);
    });

    return w;
}

// ================= Dialog suppression =================
QDialog* MainWindow::dialogSuppression()
{
    QDialog *d = new QDialog(this);
    d->setWindowTitle("Confirmation de suppression");
    d->setModal(true);
    d->setFixedWidth(420);
    d->setStyleSheet("background:" + C_CARD + ";");

    QVBoxLayout *l = new QVBoxLayout(d);
    l->setContentsMargins(24, 24, 24, 24);

    QLabel *icon = new QLabel("⚠️");
    icon->setStyleSheet("font-size: 44px;");
    icon->setAlignment(Qt::AlignCenter);
    l->addWidget(icon);

    QLabel *msg = new QLabel("Êtes-vous sûr de vouloir supprimer ce projet ?\n\n");
    msg->setStyleSheet("font-size: 14px; font-weight: 600; color:" + C_TEXT + ";");
    msg->setAlignment(Qt::AlignCenter);
    l->addWidget(msg);

    if(selectedRow >= 0) {
        QString details = QString("ID : %1\nNom : %2\nBudget : %3 DT\nDate : %4")
        .arg(tableProjets->item(selectedRow, 0)->text())
            .arg(tableProjets->item(selectedRow, 1)->text())
            .arg(tableProjets->item(selectedRow, 3)->text())
            .arg(tableProjets->item(selectedRow, 4)->text());

        QLabel *detailsLabel = new QLabel(details);
        detailsLabel->setStyleSheet("font-size: 13px; color:" + C_TEXT + "; padding: 10px; background: " + C_BG + "; border-radius: 6px;");
        detailsLabel->setAlignment(Qt::AlignCenter);
        l->addWidget(detailsLabel);
    }

    l->addSpacing(12);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    QPushButton *ok = styledButton("✅ Confirmer", "#c44536");
    QPushButton *cancel = styledButton("❌ Annuler", "#999");

    btnLayout->addWidget(ok);
    btnLayout->addWidget(cancel);
    l->addLayout(btnLayout);

    connect(cancel, &QPushButton::clicked, d, &QDialog::reject);
    connect(ok, &QPushButton::clicked, this, [=](){
        if(selectedRow >= 0) {
            tableProjets->removeRow(selectedRow);
            QMessageBox::information(this, "Supprimé", "✅ Projet supprimé avec succès.");
            selectedRow = -1;
        }
        d->accept();
    });

    return d;
}

// ================= Page: Statistics =================
QWidget* MainWindow::pageStats()
{
    QWidget *w = new QWidget;
    w->setStyleSheet("background:" + C_BG + ";");
    QVBoxLayout *l = new QVBoxLayout(w);
    l->setContentsMargins(20, 20, 20, 20);

    QLabel *title = new QLabel("📊 Statistiques des projets");
    title->setStyleSheet("font-size:18px; font-weight:700; color:" + C_TEXT + ";");
    l->addWidget(title);

    l->addSpacing(10);

    QHBoxLayout *cardsLayout = new QHBoxLayout;

    // Total projects & counts & budget total computed here
    int enCours = 0, termines = 0;
    double budgetTotal = 0;
    QMap<QString,int> domaines;
    for (int i=0;i<tableProjets->rowCount();++i) {
        QString st = tableProjets->item(i,6)->text();
        if (st == "En cours") ++enCours;
        if (st == "Terminé") ++termines;
        QString bud = tableProjets->item(i,3)->text();
        budgetTotal += QLocale().toDouble(bud);
        QString dom = tableProjets->item(i,2)->text();
        domaines[dom] += 1;
    }

    // Card creation helper
    auto makeCardKPI = [&](const QString &value, const QString &label, const QString &color)->QFrame* {
        QFrame *c = card();
        QVBoxLayout *cl = new QVBoxLayout(c);
        QLabel *num = new QLabel(value);
        num->setStyleSheet("font-size:26px; font-weight:700; color:" + color + ";");
        QLabel *txt = new QLabel(label);
        txt->setStyleSheet("color:" + C_TEXT + ";");
        cl->addWidget(num, 0, Qt::AlignCenter);
        cl->addWidget(txt, 0, Qt::AlignCenter);
        return c;
    };

    cardsLayout->addWidget(makeCardKPI(QString::number(tableProjets->rowCount()), "Projets totaux", C_PRIMARY));
    cardsLayout->addWidget(makeCardKPI(QString::number(enCours), "En cours", "#FF9800"));
    cardsLayout->addWidget(makeCardKPI(QString::number(termines), "Terminés", "#4CAF50"));
    cardsLayout->addWidget(makeCardKPI(QString("%1 DT").arg(QString::number(budgetTotal,'f',0)), "Budget total (DT)", "#2196F3"));

    l->addLayout(cardsLayout);
    l->addSpacing(16);

    // Graphs row: left donut (domaines) and right budget distribution text
    QHBoxLayout *graphLayout = new QHBoxLayout;

    QFrame *left = card();
    QVBoxLayout *leftL = new QVBoxLayout(left);
    QLabel *g1Title = new QLabel("📊 Répartition par domaine");
    g1Title->setStyleSheet("font-weight:700; color:" + C_PRIMARY + ";");
    leftL->addWidget(g1Title);

    // compute donut data
    QList<DonutChart::Slice> slices;
    QColor colors[] = { QColor("#9FBEB9"), QColor("#2E6F63"), QColor("#B5672C"), QColor("#8B2F3C"), QColor("#8FD3E8") };
    int ci = 0;
    int domainTotal = 0;
    for (auto it = domaines.begin(); it != domaines.end(); ++it) domainTotal += it.value();
    for (auto it = domaines.begin(); it != domaines.end(); ++it) {
        slices.append({ (double)it.value(), colors[ci%5], it.key() });
        ++ci;
    }
    statsDonut->setData(slices);
    leftL->addWidget(statsDonut, 1);

    // Legend below donut (color dot + label + count + percent)
    QWidget *legend = new QWidget;
    QVBoxLayout *legLayout = new QVBoxLayout(legend);
    legLayout->setContentsMargins(6,6,6,6);
    for (int i=0; i < slices.size(); ++i) {
        QWidget *row = new QWidget;
        QHBoxLayout *r = new QHBoxLayout(row);
        r->setContentsMargins(4,2,4,2);
        QFrame *dot = new QFrame;
        dot->setFixedSize(12,12);
        dot->setStyleSheet(QString("background:%1; border-radius:6px;").arg(slices[i].color.name()));
        QLabel *lbl = new QLabel(QString("%1 — %2 (%3%)").arg(slices[i].label).arg((int)slices[i].value).arg(domainTotal>0 ? qRound(slices[i].value*100.0/domainTotal) : 0));
        lbl->setStyleSheet("color:" + C_TEXT + ";");
        r->addWidget(dot);
        r->addSpacing(8);
        r->addWidget(lbl);
        r->addStretch();
        legLayout->addWidget(row);
    }
    leftL->addWidget(legend);

    QFrame *right = card();
    QVBoxLayout *rightL = new QVBoxLayout(right);
    QLabel *g2Title = new QLabel("📈 Distribution des budgets");
    g2Title->setStyleSheet("font-weight:700; color:" + C_PRIMARY + ";");
    rightL->addWidget(g2Title);

    QString budgetInfo;
    if (tableProjets->rowCount() == 0) budgetInfo = "Aucun projet.";
    else {
        double avg = budgetTotal / qMax(1, tableProjets->rowCount());
        budgetInfo = QString("Budget moyen: %1 DT\nBudget total: %2 DT\nProjets: %3")
                         .arg(QString::number(avg, 'f', 0))
                         .arg(QString::number(budgetTotal, 'f', 0))
                         .arg(tableProjets->rowCount());
    }
    QLabel *g2Content = new QLabel(budgetInfo);
    g2Content->setStyleSheet("color:" + C_TEXT + ";");
    rightL->addWidget(g2Content);
    rightL->addStretch();

    graphLayout->addWidget(left, 1);
    graphLayout->addWidget(right, 1);
    l->addLayout(graphLayout);

    l->addSpacing(10);

    // Time evolution summary
    QFrame *timeline = card();
    QVBoxLayout *tl = new QVBoxLayout(timeline);
    QLabel *tlTitle = new QLabel("📅 Évolution des projets dans le temps");
    tlTitle->setStyleSheet("font-weight:700; color:" + C_PRIMARY + ";");
    tl->addWidget(tlTitle);

    QString timelineText = "2023: 1 projet terminé\n2024: 2 projets en cours\n2025: prévisions\n\nTendance: ↗️ Croissance";
    QLabel *tlInfo = new QLabel(timelineText);
    tlInfo->setStyleSheet("color:" + C_TEXT + ";");
    tl->addWidget(tlInfo);

    l->addWidget(timeline);

    l->addSpacing(8);

    QPushButton *back = styledButton("🔙 Retour à la liste", C_PRIMARY);
    l->addWidget(back, 0, Qt::AlignLeft);

    connect(back, &QPushButton::clicked, this, [=](){ stack->setCurrentIndex(0); });

    return w;
}

// ================= Page: Modifier =================
QWidget* MainWindow::pageModifierProjet()
{
    QWidget *w = new QWidget;
    w->setStyleSheet("background:" + C_BG + ";");
    QVBoxLayout *mainLayout = new QVBoxLayout(w);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    QLabel *title = new QLabel("✏️ Modifier le projet");
    title->setStyleSheet("font-size:18px; font-weight:700; color:" + C_TEXT + ";");
    mainLayout->addWidget(title);

    mainLayout->addSpacing(12);

    // Info bar that reflects selection
    if(selectedRow >= 0) {
        QFrame *infoBar = new QFrame;
        infoBar->setStyleSheet("background: " + C_DARK_GREEN + "; border-radius: 6px; padding: 8px;");
        QHBoxLayout *infoLayout = new QHBoxLayout(infoBar);
        QString infoText = QString("📋 Modification de : %1 (ID: %2)")
                               .arg(tableProjets->item(selectedRow, 1)->text())
                               .arg(tableProjets->item(selectedRow, 0)->text());
        QLabel *infoLabel = new QLabel(infoText);
        infoLabel->setStyleSheet("color: white; font-weight:600;");
        infoLayout->addWidget(infoLabel);
        mainLayout->addWidget(infoBar);
    }

    // Use the same form widgets as add page (they are members), just ensure they exist
    QFrame *form = card();
    // ensure readable text inside the form
    form->setStyleSheet("QLabel { color: " + C_TEXT + "; } QLineEdit, QComboBox, QDateEdit, QDoubleSpinBox { color: " + C_TEXT + "; background: white; }");

    QVBoxLayout *f = new QVBoxLayout(form);

    f->addWidget(new QLabel("📝 Modifier les informations"));

    // ID readonly
    QLineEdit *idRead = new QLineEdit;
    if (selectedRow >= 0) idRead->setText(tableProjets->item(selectedRow,0)->text());
    idRead->setReadOnly(true);
    idRead->setStyleSheet("background:#eaeaea; padding:6px; color:" + C_TEXT + ";");
    f->addWidget(new QLabel("ID du projet")); f->addWidget(idRead);

    // Ensure fields exist
    if (!editNomProjet) editNomProjet = new QLineEdit;
    if (!comboDomaine) comboDomaine = new QComboBox;
    if (!spinBudget) spinBudget = new QDoubleSpinBox;
    if (!editFinancement) editFinancement = new QLineEdit;
    if (!editStartDate) editStartDate = new QDateEdit;
    if (!editEndDate) editEndDate = new QDateEdit;
    if (!comboStatus) comboStatus = new QComboBox;
    if (!editEthique) editEthique = new QLineEdit;
    if (!spinPublications) spinPublications = new QSpinBox;

    // Pre-fill form with selected project
    if(selectedRow >= 0) fillFormForEdit(selectedRow);

    f->addWidget(new QLabel("Nom du projet")); f->addWidget(editNomProjet);
    f->addWidget(new QLabel("Domaine")); f->addWidget(comboDomaine);
    f->addWidget(new QLabel("Budget")); f->addWidget(spinBudget);
    f->addWidget(new QLabel("Financement")); f->addWidget(editFinancement);

    QHBoxLayout *datesLayout = new QHBoxLayout;
    datesLayout->addWidget(new QLabel("Date début")); datesLayout->addWidget(editStartDate);
    datesLayout->addWidget(new QLabel("Date fin")); datesLayout->addWidget(editEndDate);
    f->addLayout(datesLayout);

    f->addWidget(new QLabel("Statut")); f->addWidget(comboStatus);
    f->addWidget(new QLabel("N° éthique")); f->addWidget(editEthique);
    f->addWidget(new QLabel("Publications")); f->addWidget(spinPublications);

    QHBoxLayout *btns = new QHBoxLayout;
    QPushButton *update = styledButton("💾 Mettre à jour", "#FF9800");
    QPushButton *cancel = styledButton("❌ Annuler", "#999");
    QPushButton *reset = styledButton("🔄 Réinitialiser", C_BEIGE);
    btns->addWidget(update); btns->addWidget(reset); btns->addWidget(cancel);
    f->addLayout(btns);

    mainLayout->addWidget(form);

    connect(cancel, &QPushButton::clicked, this, [=](){ stack->setCurrentIndex(0); });
    connect(reset, &QPushButton::clicked, this, [=](){ if(selectedRow >= 0) fillFormForEdit(selectedRow); });
    connect(update, &QPushButton::clicked, this, [=](){
        if(selectedRow < 0) return;
        tableProjets->item(selectedRow,1)->setText(editNomProjet->text());
        tableProjets->item(selectedRow,2)->setText(comboDomaine->currentText());
        tableProjets->item(selectedRow,3)->setText(QString::number(spinBudget->value(), 'f', 2));
        tableProjets->item(selectedRow,4)->setText(editStartDate->date().toString("dd/MM/yyyy"));
        tableProjets->item(selectedRow,5)->setText(editEndDate->date().toString("dd/MM/yyyy"));
        tableProjets->item(selectedRow,6)->setText(comboStatus->currentText());
        tableProjets->item(selectedRow,7)->setText(QString::number(spinPublications->value()));
        applyRowStyle(selectedRow);
        updateStats();
        QMessageBox::information(this, "Succès", "✅ Projet mis à jour !");
        stack->setCurrentIndex(0);
    });

    return w;
}

// ================= Page: Details =================
QWidget* MainWindow::pageDetails()
{
    QWidget *w = new QWidget;
    w->setStyleSheet("background:" + C_BG + ";");
    QVBoxLayout *l = new QVBoxLayout(w);
    l->setContentsMargins(20,20,20,20);

    QLabel *title = new QLabel("👁️ Détails du projet");
    title->setStyleSheet("font-size:18px; font-weight:700; color:" + C_TEXT + ";");
    l->addWidget(title);

    l->addSpacing(8);

    QScrollArea *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);

    QWidget *content = new QWidget;
    QVBoxLayout *sc = new QVBoxLayout(content);

    QGroupBox *infoGroup = new QGroupBox("📋 Informations générales");
    // groupbox title and content readable
    infoGroup->setStyleSheet("QGroupBox { background:" + C_CARD + "; padding:10px; border-radius:8px; color:" + C_TEXT + "; } QGroupBox::title { color:" + C_TEXT + "; }");
    QVBoxLayout *infoLayout = new QVBoxLayout(infoGroup);

    // Create member labels so we can update them when user selects a different project
    detailID = new QLabel("ID : ");
    detailNom = new QLabel("Nom : ");
    detailDomaine = new QLabel("Domaine : ");
    detailBudget = new QLabel("Budget : ");
    detailStart = new QLabel("Date début : ");
    detailEnd = new QLabel("Date fin : ");
    detailStatus = new QLabel("Statut : ");
    detailPubs = new QLabel("Publications : ");

    QList<QLabel*> list = { detailID, detailNom, detailDomaine, detailBudget, detailStart, detailEnd, detailStatus, detailPubs };
    for (auto lbl : list) {
        lbl->setStyleSheet("color:" + C_TEXT + "; font-size:13px; padding:4px; background: transparent;");
        infoLayout->addWidget(lbl);
    }

    sc->addWidget(infoGroup);

    // Budget details and donut for this specific project (budget utilization simulation)
    QGroupBox *budgetGroup = new QGroupBox("💰 Informations budgétaires");
    budgetGroup->setStyleSheet("QGroupBox { background:" + C_CARD + "; padding:10px; border-radius:8px; color:" + C_TEXT + "; } QGroupBox::title { color:" + C_TEXT + "; }");
    QHBoxLayout *bgL = new QHBoxLayout(budgetGroup);

    // Left: donut showing project's budget used %
    budgetDonut = new DonutChart;
    bgL->addWidget(budgetDonut, 1);

    // Right: textual budget info
    QLabel *budgetText = new QLabel("Détail d'utilisation (simulé)");
    budgetText->setStyleSheet("color:" + C_TEXT + ";");
    bgL->addWidget(budgetText, 1);

    sc->addWidget(budgetGroup);

    // Publications
    QGroupBox *pubGroup = new QGroupBox("📚 Publications associées");
    pubGroup->setStyleSheet("QGroupBox { background:" + C_CARD + "; padding:10px; border-radius:8px; color:" + C_TEXT + "; } QGroupBox::title { color:" + C_TEXT + "; }");
    QVBoxLayout *pgL = new QVBoxLayout(pubGroup);
    QLabel *pubsLabel = new QLabel("Liste des publications liées à ce projet...");
    pubsLabel->setStyleSheet("color:" + C_TEXT + ";");
    pgL->addWidget(pubsLabel);
    sc->addWidget(pubGroup);

    scroll->setWidget(content);
    l->addWidget(scroll);

    // Buttons
    QHBoxLayout *btnLayout = new QHBoxLayout;
    QPushButton *back = styledButton("🔙 Retour", C_PRIMARY);
    QPushButton *edit = styledButton("✏️ Modifier", C_BEIGE);
    QPushButton *budget = styledButton("💰 Suivi budgétaire", C_DARK_GREEN);

    btnLayout->addWidget(back);
    btnLayout->addWidget(edit);
    btnLayout->addWidget(budget);
    btnLayout->addStretch();

    l->addLayout(btnLayout);

    connect(back, &QPushButton::clicked, this, [=](){ stack->setCurrentIndex(0); });
    connect(edit, &QPushButton::clicked, this, [=](){ if(selectedRow>=0){ fillFormForEdit(selectedRow); stack->setCurrentIndex(4); } });
    connect(budget, &QPushButton::clicked, this, [=](){ updateStats(); stack->setCurrentIndex(5); });

    return w;
}

// ================= Page: Budget Tracking =================
QWidget* MainWindow::pageSuiviBudgetaire()
{
    QWidget *w = new QWidget;
    w->setStyleSheet("background:" + C_BG + ";");
    QVBoxLayout *l = new QVBoxLayout(w);
    l->setContentsMargins(20,20,20,20);

    QLabel *title = new QLabel("💰 Suivi Budgétaire Intelligent");
    title->setStyleSheet("font-size:18px; font-weight:700; color:" + C_TEXT + ";");
    l->addWidget(title);

    l->addSpacing(8);

    // KPIs
    QHBoxLayout *kpiLayout = new QHBoxLayout;

    QFrame *k1 = card(); QVBoxLayout *k1L = new QVBoxLayout(k1);
    QLabel *k1Num = new QLabel(); k1Num->setStyleSheet("font-size:20px; font-weight:700; color:" + C_TEXT + ";");
    QLabel *k1Val = new QLabel("Budget Total"); k1Val->setStyleSheet("color:" + C_TEXT + ";");
    k1L->addWidget(k1Num, 0, Qt::AlignCenter); k1L->addWidget(k1Val, 0, Qt::AlignCenter);

    QFrame *k2 = card(); QVBoxLayout *k2L = new QVBoxLayout(k2);
    QLabel *k2Num = new QLabel(); k2Num->setStyleSheet("font-size:20px; font-weight:700; color:" + C_TEXT + ";");
    QLabel *k2Val = new QLabel("Utilisé"); k2Val->setStyleSheet("color:" + C_TEXT + ";");
    k2L->addWidget(k2Num, 0, Qt::AlignCenter); k2L->addWidget(k2Val, 0, Qt::AlignCenter);

    QFrame *k3 = card(); QVBoxLayout *k3L = new QVBoxLayout(k3);
    QLabel *k3Num = new QLabel(); k3Num->setStyleSheet("font-size:20px; font-weight:700; color:" + C_TEXT + ";");
    QLabel *k3Val = new QLabel("Restant"); k3Val->setStyleSheet("color:" + C_TEXT + ";");
    k3L->addWidget(k3Num, 0, Qt::AlignCenter); k3L->addWidget(k3Val, 0, Qt::AlignCenter);

    kpiLayout->addWidget(k1); kpiLayout->addWidget(k2); kpiLayout->addWidget(k3);
    l->addLayout(kpiLayout);

    // Fill KPI numbers from current table data
    double total = 0;
    for (int i=0;i<tableProjets->rowCount();++i) total += QLocale().toDouble(tableProjets->item(i,3)->text());
    double used = total * 0.45; // simulated
    double remain = total - used;
    k1Num->setText(QString("%1 DT").arg(QString::number(total,'f',0)));
    k2Num->setText(QString("%1 DT (%2%)").arg(QString::number(used,'f',0)).arg(QString::number((total>0?used*100.0/total:0),'f',0)));
    k3Num->setText(QString("%1 DT").arg(QString::number(remain,'f',0)));

    l->addSpacing(10);

    // Per-project bars
    QFrame *projectsCard = card();
    // ensure labels inside the project card are dark
    projectsCard->setStyleSheet("QLabel { color: " + C_TEXT + "; }");
    QVBoxLayout *pc = new QVBoxLayout(projectsCard);
    QLabel *projTitle = new QLabel("📊 Détail par projet"); projTitle->setStyleSheet("color:" + C_TEXT + "; font-weight:700;");
    pc->addWidget(projTitle);

    for (int i=0;i<tableProjets->rowCount();++i) {
        QFrame *pf = new QFrame;
        pf->setStyleSheet("background:white; padding:8px; border-radius:6px;");
        QVBoxLayout *pfl = new QVBoxLayout(pf);
        QString name = tableProjets->item(i,1)->text();
        double bud = QLocale().toDouble(tableProjets->item(i,3)->text());
        int usage = 30 + (i*20); // simulation
        QLabel *projLabel = new QLabel(QString("%1 - %2 DT").arg(name).arg(QString::number(bud,'f',0)));
        projLabel->setStyleSheet("color:" + C_TEXT + ";");
        pfl->addWidget(projLabel);
        QProgressBar *bar = new QProgressBar;
        bar->setValue(usage);
        bar->setFormat(QString("%1% utilis��").arg(usage));
        QString barColor = "#4CAF50";
        if (usage > 90) barColor = "#F44336"; else if (usage > 70) barColor = "#FF9800";
        // Ensure progress bar text is dark (C_TEXT) and chunk keeps colored background
        bar->setStyleSheet(QString("QProgressBar{height:18px;border:1px solid %1;border-radius:6px;background:white;color:%3;} QProgressBar::chunk{background:%2;border-radius:6px;color:%3;}").arg(C_BG, barColor, C_TEXT));
        pfl->addWidget(bar);
        pc->addWidget(pf);
    }

    l->addWidget(projectsCard);

    l->addSpacing(8);

    // Predictions + recommendations
    QHBoxLayout *aiLayout = new QHBoxLayout;
    QFrame *pred = card(); QVBoxLayout *predL = new QVBoxLayout(pred);
    QLabel *pt = new QLabel("🔮 Prédictions IA"); pt->setStyleSheet("color:" + C_TEXT + "; font-weight:700;");
    predL->addWidget(pt);
    QLabel *predText = new QLabel("Budget épuisé estimé: Avril 2026"); predText->setStyleSheet("color:" + C_TEXT + ";");
    predL->addWidget(predText);

    QFrame *reco = card(); QVBoxLayout *recoL = new QVBoxLayout(reco);
    QLabel *rt = new QLabel("💡 Recommandations"); rt->setStyleSheet("color:" + C_TEXT + "; font-weight:700;");
    recoL->addWidget(rt);
    QLabel *recoText = new QLabel("Optimiser équipements, mutualiser ressources"); recoText->setStyleSheet("color:" + C_TEXT + ";");
    recoL->addWidget(recoText);

    aiLayout->addWidget(pred); aiLayout->addWidget(reco);
    l->addLayout(aiLayout);

    l->addSpacing(8);

    QHBoxLayout *btns = new QHBoxLayout;
    QPushButton *back = styledButton("🔙 Retour", C_PRIMARY);
    QPushButton *report = styledButton("📄 Générer rapport", C_BEIGE);
    btns->addWidget(back); btns->addWidget(report); btns->addStretch();
    l->addLayout(btns);

    connect(back, &QPushButton::clicked, this, [=](){ stack->setCurrentIndex(0); });
    connect(report, &QPushButton::clicked, this, [=](){ QMessageBox::information(this, "Rapport", "Génération du rapport..."); });

    return w;
}

// ================= Utilities =================
void MainWindow::clearForm()
{
    if (editNomProjet) editNomProjet->clear();
    if (editFinancement) editFinancement->clear();
    if (editEthique) editEthique->clear();
    if (comboDomaine) comboDomaine->setCurrentIndex(0);
    if (comboStatus) comboStatus->setCurrentIndex(0);
    if (spinBudget) spinBudget->setValue(0);
    if (spinPublications) spinPublications->setValue(0);
    if (editStartDate) editStartDate->setDate(QDate::currentDate());
    if (editEndDate) editEndDate->setDate(QDate::currentDate().addYears(1));
}

void MainWindow::fillFormForEdit(int row)
{
    if(row < 0 || row >= tableProjets->rowCount()) return;
    editNomProjet->setText(tableProjets->item(row,1)->text());
    comboDomaine->setCurrentText(tableProjets->item(row,2)->text());
    spinBudget->setValue(QLocale().toDouble(tableProjets->item(row,3)->text()));
    editStartDate->setDate(QDate::fromString(tableProjets->item(row,4)->text(),"dd/MM/yyyy"));
    editEndDate->setDate(QDate::fromString(tableProjets->item(row,5)->text(),"dd/MM/yyyy"));
    comboStatus->setCurrentText(tableProjets->item(row,6)->text());
    spinPublications->setValue(tableProjets->item(row,7)->text().toInt());
}

void MainWindow::addProjetToTable(const QString &id, const QString &nom,
                                  const QString &domaine, const QString &budget,
                                  const QString &startDate, const QString &endDate,
                                  const QString &status, const QString &publications)
{
    int row = tableProjets->rowCount();
    tableProjets->insertRow(row);

    auto mkItem = [&](const QString &t)->QTableWidgetItem* {
        QTableWidgetItem *it = new QTableWidgetItem(t);
        it->setForeground(QBrush(Qt::black));
        return it;
    };

    tableProjets->setItem(row, 0, mkItem(id));
    tableProjets->setItem(row, 1, mkItem(nom));
    tableProjets->setItem(row, 2, mkItem(domaine));
    tableProjets->setItem(row, 3, mkItem(budget));
    tableProjets->setItem(row, 4, mkItem(startDate));
    tableProjets->setItem(row, 5, mkItem(endDate));
    tableProjets->setItem(row, 6, mkItem(status));
    tableProjets->setItem(row, 7, mkItem(publications));
    tableProjets->setItem(row, 8, mkItem("👁️ ✏️ 🗑️"));

    applyRowStyle(row);
}

void MainWindow::applyRowStyle(int row)
{
    if (row < 0 || row >= tableProjets->rowCount()) return;
    QString status = tableProjets->item(row,6)->text();
    // Reset background and text color to default readable
    for(int c=0;c<tableProjets->columnCount();++c) {
        tableProjets->item(row,c)->setBackground(QBrush(Qt::white));
        tableProjets->item(row,c)->setForeground(QBrush(Qt::black));
    }
    if (status == "Terminé" || status == "Termine") {
        QColor bg("#dff0d8"); // light green
        for(int c=0;c<tableProjets->columnCount();++c) {
            tableProjets->item(row,c)->setBackground(bg);
            tableProjets->item(row,c)->setForeground(QBrush(Qt::black));
        }
    }
}

void MainWindow::updateDetailsUI(int row)
{
    if(row < 0 || row >= tableProjets->rowCount()) {
        detailID->setText("ID : -");
        detailNom->setText("Nom : -");
        detailDomaine->setText("Domaine : -");
        detailBudget->setText("Budget : -");
        detailStart->setText("Date début : -");
        detailEnd->setText("Date fin : -");
        detailStatus->setText("Statut : -");
        detailPubs->setText("Publications : -");
        budgetDonut->setData({});
        return;
    }
    detailID->setText("ID : " + tableProjets->item(row,0)->text());
    detailNom->setText("Nom du projet : " + tableProjets->item(row,1)->text());
    detailDomaine->setText("Domaine : " + tableProjets->item(row,2)->text());
    detailBudget->setText("Budget : " + tableProjets->item(row,3)->text() + " DT");
    detailStart->setText("Date de début : " + tableProjets->item(row,4)->text());
    detailEnd->setText("Date de fin : " + tableProjets->item(row,5)->text());
    detailStatus->setText("Statut : " + tableProjets->item(row,6)->text());
    detailPubs->setText("Publications : " + tableProjets->item(row,7)->text());

    // Simulate budget usage as example: deterministic based on row
    double bud = QLocale().toDouble(tableProjets->item(row,3)->text());
    double usedPct = 30 + (row * 20) % 70; // deterministic simulated %
    QList<DonutChart::Slice> s;
    s.append({ usedPct, QColor("#FF9800"), "Used" });
    s.append({ 100.0 - usedPct, QColor("#9FBEB9"), "Remaining" });
    budgetDonut->setData(s);
}

void MainWindow::updateStats()
{
    QMap<QString,int> domaines;
    double total = 0;
    for (int i=0;i<tableProjets->rowCount();++i) {
        domaines[tableProjets->item(i,2)->text()]++;
        total += QLocale().toDouble(tableProjets->item(i,3)->text());
    }
    QList<DonutChart::Slice> s;
    QColor colors[] = { QColor("#9FBEB9"), QColor("#2E6F63"), QColor("#B5672C"), QColor("#8B2F3C"), QColor("#8FD3E8") };
    int ci=0;
    for (auto it = domaines.begin(); it != domaines.end(); ++it) {
        s.append({ (double)it.value(), colors[ci%5], it.key() });
        ++ci;
    }
    if (!statsDonut) statsDonut = new DonutChart;
    statsDonut->setData(s);

    double used = total * 0.45; // simulated consumption
    QList<DonutChart::Slice> bs;
    bs.append({ used, QColor("#FF9800"), "Used" });
    bs.append({ qMax(0.0, total - used), QColor("#9FBEB9"), "Remaining" });
    if (!budgetDonut) budgetDonut = new DonutChart;
    budgetDonut->setData(bs);

    // Re-apply row style to ensure all "Terminé" rows are colored
    for (int r = 0; r < tableProjets->rowCount(); ++r) applyRowStyle(r);
}
