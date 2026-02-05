<<<<<<< HEAD
#pragma once
#include <QMainWindow>
#include <QScrollArea>
class QScrollArea;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
};
=======
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QStyle>
#include <QWidget>
#include <QDialog>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    // ====== STACK ======
    QStackedWidget* stack;

    // ====== PAGES ======
    QWidget* pageListeExperiences(QStyle* st);
    QWidget* pageAjoutExperience(QStyle* st);
    QWidget* pageStats(QStyle* st);
    QWidget* pageDetails(QStyle* st);

    // ====== DIALOG ======
    QDialog* dialogSuppression(QStyle* st);

    // ====== UTILS ======
    void setupUI();
    void demonstrateFixedCalls();
    void showMessage(const QString &message);

private slots:
    void on_ajouterExperience_clicked();
};

#endif // MAINWINDOW_H
>>>>>>> a5f2e61294e377957bdd8d01540afbd830e739b8
