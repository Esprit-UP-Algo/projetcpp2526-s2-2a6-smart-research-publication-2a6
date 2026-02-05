#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);  // QApplication, not QCoreApplication
    MainWindow w;
    w.show();
    return app.exec();
}
