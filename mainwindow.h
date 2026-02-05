#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QTextEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLabel>

class DonutChart;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() {}

private:
    // Stack widget for pages
    QStackedWidget *stack;

    // Table for projects
    QTableWidget *tableProjets;

    // Form fields (used in add/edit)
    QLineEdit *editNomProjet;
    QLineEdit *editFinancement;
    QLineEdit *editEthique;
    QComboBox *comboDomaine;
    QComboBox *comboStatus;
    QDoubleSpinBox *spinBudget;
    QSpinBox *spinPublications;
    QDateEdit *editStartDate;
    QDateEdit *editEndDate;

    // Detail labels (updated dynamically)
    QLabel *detailID;
    QLabel *detailNom;
    QLabel *detailDomaine;
    QLabel *detailBudget;
    QLabel *detailStart;
    QLabel *detailEnd;
    QLabel *detailStatus;
    QLabel *detailPubs;

    // Donut charts
    DonutChart *statsDonut;
    DonutChart *budgetDonut;

    // Selected row
    int selectedRow;

    // UI pages
    QWidget* pageListeProjets();
    QWidget* pageAjoutProjet();
    QWidget* pageStats();
    QWidget* pageDetails();
    QWidget* pageModifierProjet();
    QWidget* pageSuiviBudgetaire();

    // Dialog
    QDialog* dialogSuppression();

    // Utils
    void clearForm();
    void fillFormForEdit(int row);
    void addProjetToTable(const QString &id, const QString &nom,
                          const QString &domaine, const QString &budget,
                          const QString &startDate, const QString &endDate,
                          const QString &status, const QString &publications);
    void applyRowStyle(int row);
    void updateStats();            // recompute counts and charts
    void updateDetailsUI(int row); // set detail widgets from a row
};
