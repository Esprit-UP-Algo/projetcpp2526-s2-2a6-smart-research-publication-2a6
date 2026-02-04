/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.7.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFontComboBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QTextEdit *textEdit;
    QLabel *label;
    QTextEdit *textEdit_2;
    QGroupBox *groupBox;
    QPushButton *pushButton_3;
    QPushButton *pushButton_8;
    QPushButton *pushButton_9;
    QPushButton *pushButton_10;
    QPushButton *pushButton_11;
    QPushButton *pushButton_12;
    QPushButton *pushButton_13;
    QTableWidget *tableWidget;
    QLineEdit *lineEdit;
    QPushButton *pushButton_14;
    QFontComboBox *fontComboBox;
    QStackedWidget *stackedWidget;
    QWidget *page;
    QStackedWidget *stackedWidget_2;
    QWidget *page_3;
    QWidget *page_4;
    QWidget *page_2;
    QStackedWidget *stackedWidget_3;
    QWidget *page_5;
    QStackedWidget *stackedWidget_4;
    QWidget *page_7;
    QWidget *page_8;
    QWidget *page_6;
    QStackedWidget *stackedWidget_5;
    QWidget *page_9;
    QWidget *page_10;
    QStackedWidget *stackedWidget_6;
    QWidget *page_11;
    QWidget *page_12;
    QStackedWidget *stackedWidget_7;
    QWidget *page_13;
    QWidget *page_14;
    QStackedWidget *stackedWidget_8;
    QWidget *page_15;
    QWidget *page_16;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1311, 692);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        textEdit = new QTextEdit(centralwidget);
        textEdit->setObjectName("textEdit");
        textEdit->setGeometry(QRect(0, 0, 1311, 651));
        textEdit->setStyleSheet(QString::fromUtf8("background-color: rgb(247, 247, 255);"));
        label = new QLabel(centralwidget);
        label->setObjectName("label");
        label->setGeometry(QRect(490, 20, 581, 81));
        label->setStyleSheet(QString::fromUtf8("font: 36pt \"Trebuchet MS\";\n"
"color: rgb(0, 103, 76);\n"
"font: 18pt \"Segoe UI\";"));
        textEdit_2 = new QTextEdit(centralwidget);
        textEdit_2->setObjectName("textEdit_2");
        textEdit_2->setGeometry(QRect(0, 0, 231, 651));
        textEdit_2->setStyleSheet(QString::fromUtf8("background-color: rgb(170, 149, 113);"));
        groupBox = new QGroupBox(centralwidget);
        groupBox->setObjectName("groupBox");
        groupBox->setGeometry(QRect(350, 90, 841, 101));
        groupBox->setStyleSheet(QString::fromUtf8("background-color: rgb(248, 255, 234);"));
        pushButton_3 = new QPushButton(groupBox);
        pushButton_3->setObjectName("pushButton_3");
        pushButton_3->setGeometry(QRect(650, 40, 171, 41));
        pushButton_3->setStyleSheet(QString::fromUtf8("\n"
"color: rgb(64, 127, 0);"));
        pushButton_8 = new QPushButton(groupBox);
        pushButton_8->setObjectName("pushButton_8");
        pushButton_8->setGeometry(QRect(440, 40, 171, 41));
        pushButton_8->setStyleSheet(QString::fromUtf8("\n"
"color: rgb(64, 127, 0);"));
        pushButton_9 = new QPushButton(groupBox);
        pushButton_9->setObjectName("pushButton_9");
        pushButton_9->setGeometry(QRect(230, 40, 171, 41));
        pushButton_9->setStyleSheet(QString::fromUtf8("\n"
"color: rgb(64, 127, 0);"));
        pushButton_10 = new QPushButton(groupBox);
        pushButton_10->setObjectName("pushButton_10");
        pushButton_10->setGeometry(QRect(30, 40, 171, 41));
        pushButton_10->setStyleSheet(QString::fromUtf8("\n"
"color: rgb(64, 127, 0);"));
        pushButton_11 = new QPushButton(centralwidget);
        pushButton_11->setObjectName("pushButton_11");
        pushButton_11->setGeometry(QRect(1010, 590, 121, 31));
        pushButton_11->setStyleSheet(QString::fromUtf8("\n"
"color: rgb(64, 127, 0);"));
        pushButton_12 = new QPushButton(centralwidget);
        pushButton_12->setObjectName("pushButton_12");
        pushButton_12->setGeometry(QRect(870, 590, 121, 31));
        pushButton_12->setStyleSheet(QString::fromUtf8("\n"
"color: rgb(64, 127, 0);"));
        pushButton_13 = new QPushButton(centralwidget);
        pushButton_13->setObjectName("pushButton_13");
        pushButton_13->setGeometry(QRect(1140, 590, 121, 31));
        pushButton_13->setStyleSheet(QString::fromUtf8("\n"
"color: rgb(64, 127, 0);"));
        tableWidget = new QTableWidget(centralwidget);
        if (tableWidget->columnCount() < 8)
            tableWidget->setColumnCount(8);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        tableWidget->setHorizontalHeaderItem(0, __qtablewidgetitem);
        QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
        tableWidget->setHorizontalHeaderItem(1, __qtablewidgetitem1);
        QTableWidgetItem *__qtablewidgetitem2 = new QTableWidgetItem();
        tableWidget->setHorizontalHeaderItem(2, __qtablewidgetitem2);
        QTableWidgetItem *__qtablewidgetitem3 = new QTableWidgetItem();
        tableWidget->setHorizontalHeaderItem(3, __qtablewidgetitem3);
        QTableWidgetItem *__qtablewidgetitem4 = new QTableWidgetItem();
        tableWidget->setHorizontalHeaderItem(4, __qtablewidgetitem4);
        QTableWidgetItem *__qtablewidgetitem5 = new QTableWidgetItem();
        tableWidget->setHorizontalHeaderItem(5, __qtablewidgetitem5);
        QTableWidgetItem *__qtablewidgetitem6 = new QTableWidgetItem();
        tableWidget->setHorizontalHeaderItem(6, __qtablewidgetitem6);
        QTableWidgetItem *__qtablewidgetitem7 = new QTableWidgetItem();
        tableWidget->setHorizontalHeaderItem(7, __qtablewidgetitem7);
        if (tableWidget->rowCount() < 4)
            tableWidget->setRowCount(4);
        tableWidget->setObjectName("tableWidget");
        tableWidget->setGeometry(QRect(260, 270, 1031, 181));
        lineEdit = new QLineEdit(centralwidget);
        lineEdit->setObjectName("lineEdit");
        lineEdit->setGeometry(QRect(400, 210, 761, 41));
        pushButton_14 = new QPushButton(centralwidget);
        pushButton_14->setObjectName("pushButton_14");
        pushButton_14->setGeometry(QRect(1180, 210, 91, 41));
        pushButton_14->setStyleSheet(QString::fromUtf8("\n"
"color: rgb(64, 127, 0);"));
        fontComboBox = new QFontComboBox(centralwidget);
        fontComboBox->setObjectName("fontComboBox");
        fontComboBox->setGeometry(QRect(1160, 470, 111, 31));
        stackedWidget = new QStackedWidget(centralwidget);
        stackedWidget->setObjectName("stackedWidget");
        stackedWidget->setGeometry(QRect(50, 100, 131, 51));
        page = new QWidget();
        page->setObjectName("page");
        stackedWidget_2 = new QStackedWidget(page);
        stackedWidget_2->setObjectName("stackedWidget_2");
        stackedWidget_2->setGeometry(QRect(110, 20, 131, 51));
        page_3 = new QWidget();
        page_3->setObjectName("page_3");
        stackedWidget_2->addWidget(page_3);
        page_4 = new QWidget();
        page_4->setObjectName("page_4");
        stackedWidget_2->addWidget(page_4);
        stackedWidget->addWidget(page);
        page_2 = new QWidget();
        page_2->setObjectName("page_2");
        stackedWidget->addWidget(page_2);
        stackedWidget_3 = new QStackedWidget(centralwidget);
        stackedWidget_3->setObjectName("stackedWidget_3");
        stackedWidget_3->setGeometry(QRect(50, 180, 131, 51));
        page_5 = new QWidget();
        page_5->setObjectName("page_5");
        stackedWidget_4 = new QStackedWidget(page_5);
        stackedWidget_4->setObjectName("stackedWidget_4");
        stackedWidget_4->setGeometry(QRect(60, 40, 131, 51));
        page_7 = new QWidget();
        page_7->setObjectName("page_7");
        stackedWidget_4->addWidget(page_7);
        page_8 = new QWidget();
        page_8->setObjectName("page_8");
        stackedWidget_4->addWidget(page_8);
        stackedWidget_3->addWidget(page_5);
        page_6 = new QWidget();
        page_6->setObjectName("page_6");
        stackedWidget_3->addWidget(page_6);
        stackedWidget_5 = new QStackedWidget(centralwidget);
        stackedWidget_5->setObjectName("stackedWidget_5");
        stackedWidget_5->setGeometry(QRect(50, 260, 131, 51));
        page_9 = new QWidget();
        page_9->setObjectName("page_9");
        stackedWidget_5->addWidget(page_9);
        page_10 = new QWidget();
        page_10->setObjectName("page_10");
        stackedWidget_5->addWidget(page_10);
        stackedWidget_6 = new QStackedWidget(centralwidget);
        stackedWidget_6->setObjectName("stackedWidget_6");
        stackedWidget_6->setGeometry(QRect(50, 340, 131, 51));
        page_11 = new QWidget();
        page_11->setObjectName("page_11");
        stackedWidget_6->addWidget(page_11);
        page_12 = new QWidget();
        page_12->setObjectName("page_12");
        stackedWidget_6->addWidget(page_12);
        stackedWidget_7 = new QStackedWidget(centralwidget);
        stackedWidget_7->setObjectName("stackedWidget_7");
        stackedWidget_7->setGeometry(QRect(50, 420, 131, 51));
        page_13 = new QWidget();
        page_13->setObjectName("page_13");
        stackedWidget_7->addWidget(page_13);
        page_14 = new QWidget();
        page_14->setObjectName("page_14");
        stackedWidget_7->addWidget(page_14);
        stackedWidget_8 = new QStackedWidget(centralwidget);
        stackedWidget_8->setObjectName("stackedWidget_8");
        stackedWidget_8->setGeometry(QRect(50, 500, 131, 51));
        page_15 = new QWidget();
        page_15->setObjectName("page_15");
        stackedWidget_8->addWidget(page_15);
        page_16 = new QWidget();
        page_16->setObjectName("page_16");
        stackedWidget_8->addWidget(page_16);
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 1311, 26));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "Gestion des exp\303\251rienes et de protocoles", nullptr));
        groupBox->setTitle(QCoreApplication::translate("MainWindow", "GroupBox", nullptr));
        pushButton_3->setText(QCoreApplication::translate("MainWindow", "Afficher", nullptr));
        pushButton_8->setText(QCoreApplication::translate("MainWindow", "Modifier", nullptr));
        pushButton_9->setText(QCoreApplication::translate("MainWindow", "Supprimer", nullptr));
        pushButton_10->setText(QCoreApplication::translate("MainWindow", "Ajouter", nullptr));
        pushButton_11->setText(QCoreApplication::translate("MainWindow", "statistique", nullptr));
        pushButton_12->setText(QCoreApplication::translate("MainWindow", "Trier", nullptr));
        pushButton_13->setText(QCoreApplication::translate("MainWindow", "Export", nullptr));
        QTableWidgetItem *___qtablewidgetitem = tableWidget->horizontalHeaderItem(0);
        ___qtablewidgetitem->setText(QCoreApplication::translate("MainWindow", "ID", nullptr));
        QTableWidgetItem *___qtablewidgetitem1 = tableWidget->horizontalHeaderItem(1);
        ___qtablewidgetitem1->setText(QCoreApplication::translate("MainWindow", "Titre", nullptr));
        QTableWidgetItem *___qtablewidgetitem2 = tableWidget->horizontalHeaderItem(2);
        ___qtablewidgetitem2->setText(QCoreApplication::translate("MainWindow", "Protocole", nullptr));
        QTableWidgetItem *___qtablewidgetitem3 = tableWidget->horizontalHeaderItem(3);
        ___qtablewidgetitem3->setText(QCoreApplication::translate("MainWindow", "Chercheur", nullptr));
        QTableWidgetItem *___qtablewidgetitem4 = tableWidget->horizontalHeaderItem(4);
        ___qtablewidgetitem4->setText(QCoreApplication::translate("MainWindow", "Projet", nullptr));
        QTableWidgetItem *___qtablewidgetitem5 = tableWidget->horizontalHeaderItem(5);
        ___qtablewidgetitem5->setText(QCoreApplication::translate("MainWindow", "Statut", nullptr));
        QTableWidgetItem *___qtablewidgetitem6 = tableWidget->horizontalHeaderItem(6);
        ___qtablewidgetitem6->setText(QCoreApplication::translate("MainWindow", "DateDebut", nullptr));
        QTableWidgetItem *___qtablewidgetitem7 = tableWidget->horizontalHeaderItem(7);
        ___qtablewidgetitem7->setText(QCoreApplication::translate("MainWindow", "DateFin", nullptr));
        pushButton_14->setText(QCoreApplication::translate("MainWindow", "Recherche", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
