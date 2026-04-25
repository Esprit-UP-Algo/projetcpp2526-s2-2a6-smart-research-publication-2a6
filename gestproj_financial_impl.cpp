// ─────────────────────────────────────────────────────────────────────
//  RAPPORT FINANCIER TRIMESTRIEL (Excel Export)
//  Generated for: SmartVision Bio-Laboratory Research Management System
// ─────────────────────────────────────────────────────────────────────

// Helper function: Write financial report data to CSV file
static bool writeFinancialReportToCSV(const QString& filePath, int quarter, int year,
                                      const QList<ProjetRecord>& projects)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);

    // ── SHEET 1: Résumé Trimestriel ──────────────────────────
    out << "RESUME TRIMESTRIEL\n";
    out << "Laboratoire,SmartVision\n";
    out << "Periode du rapport,Q" << quarter << " " << year << "\n";
    out << "Date de generation," << QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm") << "\n\n";

    // Calculate summary metrics
    int activeProjectCount = 0;
    double totalEstimatedBudget = 0.0;
    int projectsOverBudget = 0;

    for (const auto& proj : projects) {
        QString statL = proj.statut.trimmed().toLower();
        if (statL == "en cours" || statL == "planifié" || statL == "planifie") {
            activeProjectCount++;
        }
        if (proj.budget > 0) {
            totalEstimatedBudget += proj.budget;
        }
    }

    // Fetch consumed budget from experiences/biosamples
    double totalBudgetUsed = 0.0;
    int newProjectsStarted = 0, projectsCompleted = 0;

    QDate qtrStart(year, (quarter-1)*3+1, 1);
    QDate qtrEnd = qtrStart.addMonths(3);

    for (const auto& proj : projects) {
        // Experiences and Biosamples costs
        if (proj.statut.trimmed().toLower() == "termine" || proj.statut.trimmed().toLower() == "terminé") {
            projectsCompleted++;
        }
        if (proj.dateDeDebut >= qtrStart && proj.dateDeDebut < qtrEnd) {
            newProjectsStarted++;
        }
    }

    // Query for actual expenses
    {
        QSqlQuery q;
        q.prepare(R"(
            SELECT NVL(COUNT(*), 0) * 500.0 FROM "Expérience"
            WHERE "date_debut" >= :startDate AND "date_debut" < :endDate
        )");
        q.bindValue(":startDate", qtrStart);
        q.bindValue(":endDate", qtrEnd);
        if (q.exec() && q.next()) {
            totalBudgetUsed += q.value(0).toDouble();
        }
    }

    // Add biosamples costs
    {
        QSqlQuery q;
        q.prepare(R"(
            SELECT NVL(COUNT(*), 0) * 150.0 FROM "Échantillon"
            WHERE "date_creation" >= :startDate AND "date_creation" < :endDate
        )");
        q.bindValue(":startDate", qtrStart);
        q.bindValue(":endDate", qtrEnd);
        if (q.exec() && q.next()) {
            totalBudgetUsed += q.value(0).toDouble();
        }
    }

    double remainingBudget = totalEstimatedBudget - totalBudgetUsed;
    double consumptionRate = (totalEstimatedBudget > 0) ? (totalBudgetUsed / totalEstimatedBudget * 100.0) : 0.0;

    out << "Nombre de projets actifs," << activeProjectCount << "\n";
    out << "Budget total estime," << QString::number(totalEstimatedBudget, 'f', 2) << " TND\n";
    out << "Budget consomme ce trimestre," << QString::number(totalBudgetUsed, 'f', 2) << " TND\n";
    out << "Budget restant," << QString::number(remainingBudget, 'f', 2) << " TND\n";
    out << "Taux de consommation," << QString::number(consumptionRate, 'f', 1) << "%\n";
    out << "Projets termines ce trimestre," << projectsCompleted << "\n";
    out << "Nouveaux projets demarre ce trimestre," << newProjectsStarted << "\n\n";

    // ── SHEET 2: Détail par Projet ──────────────────────────
    out << "DETAIL PAR PROJET\n";
    out << "Id_projet,Nom du projet,Domaine,Statut,Date debut,Date fin,Budget estime,"
        << "Budget consomme,Reste disponible,% consomme,Employes,Experiences,BioSamples,Publications,"
        << "Source financement,Numero ethique\n";

    for (const auto& proj : projects) {
        // Count related items
        int empCount = 0, expCount = 0, bioCount = 0;
        {
            QSqlQuery q;
            q.prepare("SELECT COUNT(*) FROM \"Associer\" WHERE \"Id_projet\" = :id");
            q.bindValue(":id", proj.idProjet);
            if (q.exec() && q.next()) empCount = q.value(0).toInt();
        }
        {
            QString expStr = QString::fromUtf8("Expérience");
            QSqlQuery q;
            q.prepare(QString("SELECT COUNT(*) FROM \"%1\" WHERE \"Id_projet\" = :id").arg(expStr));
            q.bindValue(":id", proj.idProjet);
            if (q.exec() && q.next()) expCount = q.value(0).toInt();
        }
        {
            QString ecStr = QString::fromUtf8("Échantillon");
            QSqlQuery q;
            q.prepare(QString("SELECT COUNT(*) FROM \"%1\" WHERE \"Id_projet\" = :id").arg(ecStr));
            q.bindValue(":id", proj.idProjet);
            if (q.exec() && q.next()) bioCount = q.value(0).toInt();
        }

        double consumed = (expCount * 500.0 + bioCount * 150.0) * 1.15;
        double remaining = proj.budget - consumed;
        double pctUsed = (proj.budget > 0) ? (consumed / proj.budget * 100.0) : 0.0;

        out << proj.idProjet << ","
            << "\"" << proj.nomDuProjet << "\","
            << "\"" << proj.domaineDeRecherche << "\","
            << "\"" << proj.statut << "\","
            << proj.dateDeDebut.toString("dd/MM/yyyy") << ","
            << proj.dateDeFin.toString("dd/MM/yyyy") << ","
            << QString::number(proj.budget, 'f', 2) << ","
            << QString::number(consumed, 'f', 2) << ","
            << QString::number(remaining, 'f', 2) << ","
            << QString::number(pctUsed, 'f', 1) << ","
            << empCount << ","
            << expCount << ","
            << bioCount << ","
            << proj.nombreDePublications << ","
            << "\"" << proj.sourceDeFinancement << "\","
            << "\"" << proj.numeroDApprobationEthique << "\"\n";
    }

    // ── SHEET 3: Répartition des Dépenses ────────────────────
    out << "\nREPARTITION DES DEPENSES\n";
    out << "Id_projet,Nom projet,Cout Employes,Cout Experiences,Cout BioSamples,"
        << "Cout Equipements,Cout Publications,Overhead (15%),Total\n";

    double grandTotal = 0.0;
    for (const auto& proj : projects) {
        double empCost = 0.0, expCost = 0.0, bioCost = 0.0, eqpCost = 0.0, pubCost = 0.0;

        // Employee counts
        {
            QSqlQuery q;
            q.prepare("SELECT COUNT(*) FROM \"Associer\" WHERE \"Id_projet\" = :id");
            q.bindValue(":id", proj.idProjet);
            if (q.exec() && q.next()) {
                int empCount = q.value(0).toInt();
                empCost = empCount * 2500.0; // Average cost per employee
            }
        }

        // Experience costs
        {
            QString expStr = QString::fromUtf8("Expérience");
            QSqlQuery q;
            q.prepare(QString("SELECT COUNT(*) FROM \"%1\" WHERE \"Id_projet\" = :id").arg(expStr));
            q.bindValue(":id", proj.idProjet);
            if (q.exec() && q.next()) {
                int expCount = q.value(0).toInt();
                expCost = expCount * 500.0;
            }
        }

        // BioSample costs
        {
            QString bioStr = QString::fromUtf8("Échantillon");
            QSqlQuery q;
            q.prepare(QString("SELECT COUNT(*) FROM \"%1\" WHERE \"Id_projet\" = :id").arg(bioStr));
            q.bindValue(":id", proj.idProjet);
            if (q.exec() && q.next()) {
                int bioCount = q.value(0).toInt();
                bioCost = bioCount * 150.0;
            }
        }

        // Equipment costs
        {
            QString expStr = QString::fromUtf8("Expérience");
            QSqlQuery q;
            q.prepare(QString("SELECT COUNT(*) FROM \"Équipement\" WHERE \"Id_exp\" IN "
                             "(SELECT \"Id_exp\" FROM \"%1\" WHERE \"Id_projet\" = :id)").arg(expStr));
            q.bindValue(":id", proj.idProjet);
            if (q.exec() && q.next()) {
                int eqpCount = q.value(0).toInt();
                eqpCost = eqpCount * 300.0;
            }
        }

        pubCost = proj.nombreDePublications * 200.0;

        double subtotal = empCost + expCost + bioCost + eqpCost + pubCost;
        double overhead = subtotal * 0.15;
        double total = subtotal + overhead;
        grandTotal += total;

        out << proj.idProjet << ","
            << "\"" << proj.nomDuProjet << "\","
            << QString::number(empCost, 'f', 2) << ","
            << QString::number(expCost, 'f', 2) << ","
            << QString::number(bioCost, 'f', 2) << ","
            << QString::number(eqpCost, 'f', 2) << ","
            << QString::number(pubCost, 'f', 2) << ","
            << QString::number(overhead, 'f', 2) << ","
            << QString::number(total, 'f', 2) << "\n";
    }
    out << "TOTAL,,,,,,,," << QString::number(grandTotal, 'f', 2) << "\n";

    // ── SHEET 4: Évolution Budgétaire (Monthly) ──────────────
    out << "\nEVOLUTION BUDGETAIRE\n";
    out << "Projet,Mois 1 consomme,Mois 2 consomme,Mois 3 consomme,Total trimestre\n";

    for (const auto& proj : projects) {
        double m1 = 0.0, m2 = 0.0, m3 = 0.0;

        // Month 1
        {
            QSqlQuery q;
            QDate m1Start(year, (quarter-1)*3+1, 1);
            QDate m1End = m1Start.addMonths(1);
            QString expStr = QString::fromUtf8("Expérience");
            q.prepare(QString(
                "SELECT NVL(COUNT(*), 0) * 500.0 FROM \"%1\" "
                "WHERE \"Id_projet\" = :id AND \"date_debut\" >= :start AND \"date_debut\" < :end"
            ).arg(expStr));
            q.bindValue(":id", proj.idProjet);
            q.bindValue(":start", m1Start);
            q.bindValue(":end", m1End);
            if (q.exec() && q.next()) m1 += q.value(0).toDouble();
        }

        // Month 2
        {
            QSqlQuery q;
            QDate m2Start(year, (quarter-1)*3+2, 1);
            QDate m2End = m2Start.addMonths(1);
            QString expStr = QString::fromUtf8("Expérience");
            q.prepare(QString(
                "SELECT NVL(COUNT(*), 0) * 500.0 FROM \"%1\" "
                "WHERE \"Id_projet\" = :id AND \"date_debut\" >= :start AND \"date_debut\" < :end"
            ).arg(expStr));
            q.bindValue(":id", proj.idProjet);
            q.bindValue(":start", m2Start);
            q.bindValue(":end", m2End);
            if (q.exec() && q.next()) m2 += q.value(0).toDouble();
        }

        // Month 3
        {
            QSqlQuery q;
            QDate m3Start(year, (quarter-1)*3+3, 1);
            QDate m3End = m3Start.addMonths(1);
            QString expStr = QString::fromUtf8("Expérience");
            q.prepare(QString(
                "SELECT NVL(COUNT(*), 0) * 500.0 FROM \"%1\" "
                "WHERE \"Id_projet\" = :id AND \"date_debut\" >= :start AND \"date_debut\" < :end"
            ).arg(expStr));
            q.bindValue(":id", proj.idProjet);
            q.bindValue(":start", m3Start);
            q.bindValue(":end", m3End);
            if (q.exec() && q.next()) m3 += q.value(0).toDouble();
        }

        double total = m1 + m2 + m3;
        out << "\"" << proj.nomDuProjet << "\","
            << QString::number(m1, 'f', 2) << ","
            << QString::number(m2, 'f', 2) << ","
            << QString::number(m3, 'f', 2) << ","
            << QString::number(total, 'f', 2) << "\n";
    }

    // ── SHEET 5: Alertes Budgétaires ────────────────────────
    out << "\nALERTES BUDGETAIRES\n";
    out << "Projet,Type d'alerte,Valeur actuelle,Seuil,Recommandation\n";

    for (const auto& proj : projects) {
        // Budget dépassé?
        if (proj.budget > 0) {
            double consumed = 0.0;
            {
                QSqlQuery q;
                QString expStr = QString::fromUtf8("Expérience");
                q.prepare(QString("SELECT COUNT(*) FROM \"%1\" WHERE \"Id_projet\" = :id").arg(expStr));
                q.bindValue(":id", proj.idProjet);
                if (q.exec() && q.next()) {
                    consumed += q.value(0).toInt() * 500.0;
                }
            }
            {
                QString bioStr = QString::fromUtf8("Échantillon");
                QSqlQuery q;
                q.prepare(QString("SELECT COUNT(*) FROM \"%1\" WHERE \"Id_projet\" = :id").arg(bioStr));
                q.bindValue(":id", proj.idProjet);
                if (q.exec() && q.next()) {
                    consumed += q.value(0).toInt() * 150.0;
                }
            }
            consumed *= 1.15;

            double pctUsed = (consumed / proj.budget * 100.0);

            if (consumed > proj.budget) {
                out << "\"" << proj.nomDuProjet << "\","
                    << "Budget depasse,"
                    << QString::number(consumed, 'f', 2) << " TND,"
                    << QString::number(proj.budget, 'f', 2) << " TND,"
                    << "Revoir les dépenses ou augmenter le budget\n";
            } else if (pctUsed > 85.0) {
                out << "\"" << proj.nomDuProjet << "\","
                    << "Risque depassement,"
                    << QString::number(pctUsed, 'f', 1) << "%,"
                    << "85%,"
                    << "Reduire les dépenses ou augmenter le budget\n";
            }
        }

        // Missing financing source
        if (proj.budget > 0 && proj.sourceDeFinancement.trimmed().isEmpty()) {
            out << "\"" << proj.nomDuProjet << "\","
                << "Source financement manquante,"
                << "(vide),"
                << "(requis),"
                << "Specifier la source de financement\n";
        }

        // Missing ethics approval
        QString statL = proj.statut.trimmed().toLower();
        if ((statL == "en cours" || statL == "en retard" || statL == "critique") &&
            proj.numeroDApprobationEthique.trimmed().isEmpty()) {
            out << "\"" << proj.nomDuProjet << "\","
                << "Approbation ethique manquante,"
                << "(vide),"
                << "(requis pour En cours),"
                << "Obtenir l'approbation ethique\n";
        }
    }

    file.close();
    return true;
}

void GestProjCrud::generateFinancialReport(int quarter, int year, QWidget* parent)
{
    // ── Dialog: Select Quarter & Year ────────────────────────
    QDialog* dlg = new QDialog(parent, Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    dlg->setWindowTitle("Rapport Financier Trimestriel");
    dlg->setMinimumSize(400, 280);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setStyleSheet("QDialog{ background:#F5F5F5; }");

    QVBoxLayout* mainL = new QVBoxLayout(dlg);
    mainL->setContentsMargins(20, 20, 20, 20);
    mainL->setSpacing(14);

    // Header
    QLabel* header = new QLabel("Generer un rapport financier trimestriel");
    header->setStyleSheet("color:#2A649B; font-size:14px; font-weight:900;");
    mainL->addWidget(header);

    // Quarter selection
    QHBoxLayout* qtrL = new QHBoxLayout;
    QLabel* qtrLbl = new QLabel("Trimestre:");
    qtrLbl->setMinimumWidth(70);
    QComboBox* qtrCombo = new QComboBox;
    qtrCombo->addItem("Q1 (Janvier-Mars)", 1);
    qtrCombo->addItem("Q2 (Avril-Juin)", 2);
    qtrCombo->addItem("Q3 (Juillet-Septembre)", 3);
    qtrCombo->addItem("Q4 (Octobre-Decembre)", 4);
    qtrL->addWidget(qtrLbl);
    qtrL->addWidget(qtrCombo, 1);
    mainL->addLayout(qtrL);

    // Year selection
    QHBoxLayout* yrL = new QHBoxLayout;
    QLabel* yrLbl = new QLabel("Annee:");
    yrLbl->setMinimumWidth(70);
    QComboBox* yrCombo = new QComboBox;
    int currentYear = QDate::currentDate().year();
    for (int y = currentYear - 2; y <= currentYear + 1; ++y) {
        yrCombo->addItem(QString::number(y), y);
    }
    yrCombo->setCurrentIndex(yrCombo->count() - 1);
    yrL->addWidget(yrLbl);
    yrL->addWidget(yrCombo, 1);
    mainL->addLayout(yrL);

    mainL->addSpacing(10);

    // Info
    QLabel* info = new QLabel(
        "Ce rapport generera un fichier Excel contenant :\n"
        "  • Resume trimestriel\n"
        "  • Detail par projet\n"
        "  • Repartition des depenses\n"
        "  • Evolution budgetaire\n"
        "  • Alertes budgetaires");
    info->setWordWrap(true);
    info->setStyleSheet("color:#666; font-size:11px;");
    mainL->addWidget(info);

    mainL->addStretch(1);

    // Buttons
    QHBoxLayout* btnL = new QHBoxLayout;
    btnL->addStretch(1);

    QPushButton* cancelBtn = new QPushButton("Annuler");
    cancelBtn->setFixedWidth(90);
    cancelBtn->setStyleSheet(
        "QPushButton{ background:rgba(0,0,0,0.05); border:1px solid #ccc; border-radius:6px; padding:6px; }"
        "QPushButton:hover{ background:rgba(0,0,0,0.10); }");
    QObject::connect(cancelBtn, &QPushButton::clicked, dlg, &QDialog::reject);
    btnL->addWidget(cancelBtn);

    QPushButton* generateBtn = new QPushButton("Generer & Telecharger");
    generateBtn->setFixedWidth(140);
    generateBtn->setStyleSheet(
        "QPushButton{ background:#0A5F58; color:white; border:none; border-radius:6px; padding:8px; font-weight:600; }"
        "QPushButton:hover{ background:#07433D; }");
    btnL->addWidget(generateBtn);

    mainL->addLayout(btnL);

    // On Generate clicked
    QObject::connect(generateBtn, &QPushButton::clicked, dlg, [=]() {
        int selectedQuarter = qtrCombo->currentData().toInt();
        int selectedYear = yrCombo->currentData().toInt();

        // Show save dialog
        QString fileName = QString("Rapport_Financier_Q%1_%2.csv")
            .arg(selectedQuarter).arg(selectedYear);

        QString filePath = QFileDialog::getSaveFileName(dlg,
            "Enregistrer le rapport financier",
            QDir::homePath() + "/" + fileName,
            "Excel Files (*.csv);;CSV Files (*.csv);;All Files (*)");

        if (filePath.isEmpty()) return;

        // Load all projects
        GestProjCrud crud;
        QList<ProjetRecord> allProjects;
        QString err;
        if (!crud.loadProjets(allProjects, &err)) {
            QMessageBox::critical(dlg, "Erreur", "Impossible de charger les projets: " + err);
            return;
        }

        // Generate report
        if (!writeFinancialReportToCSV(filePath, selectedQuarter, selectedYear, allProjects)) {
            QMessageBox::critical(dlg, "Erreur", "Impossible de creer le fichier rapport.");
            return;
        }

        QMessageBox::information(dlg, "Succes",
            "Rapport genere avec succes!\n\n" + QFileInfo(filePath).fileName() +
            "\n\nOuvrir le fichier?");

        QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
        dlg->accept();
    });

    dlg->exec();
}
