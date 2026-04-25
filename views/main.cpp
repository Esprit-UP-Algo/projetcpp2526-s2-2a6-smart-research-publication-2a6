#include "integration.h"    // previously biosimple.h
#include "connection.h"
#include <QApplication>
#include <QMessageBox>
int main(int argc, char *argv[])
{
    // Force software decode to avoid repeated d3d11 hwaccel failures on some GPUs/drivers.
    qputenv("QT_FFMPEG_DECODING_HW_DEVICE_TYPES", "");

    QApplication a(argc, argv);

    Connection* c = Connection::instance();
    bool test=c->createConnect();
    MainWindow w;
    if(test)
    {w.show();
        QMessageBox::information(nullptr, QObject::tr("database is open"),
                                 QObject::tr("connection successful.\n"
                                             "Click Cancel to exit."), QMessageBox::Cancel);

    }
    else
        QMessageBox::critical(nullptr, QObject::tr("database is not open"),
                              QObject::tr("connection failed.\n"
                                          "Click Cancel to exit."), QMessageBox::Cancel);



    return a.exec();
}

