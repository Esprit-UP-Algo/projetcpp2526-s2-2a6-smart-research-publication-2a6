#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Configuration de l'application
    a.setApplicationName("Smart Research Publication Management");
    a.setOrganizationName("ESPRIT");
    a.setApplicationVersion("1.0");

    MainWindow w;
    w.show();

    return a.exec();
}
