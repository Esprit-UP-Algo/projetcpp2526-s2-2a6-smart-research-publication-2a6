#include "integration.h"    // previously biosimple.h
#include "connection.h"
#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    Connection c;
    bool test = c.createconnect();

    if (test) {
        MainWindow w;
        w.show();
        QMessageBox::information(nullptr, QObject::tr("Database Status"),
                                 QObject::tr("Connection successful."));
        return a.exec();
    } else {
        QMessageBox::critical(nullptr, QObject::tr("Database Status"),
                               QObject::tr("Connection failed. Exiting application."));
        return -1;
    }
}
