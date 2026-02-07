#pragma once
#include <QMainWindow>
#include <QScrollArea>
class QScrollArea;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
};
