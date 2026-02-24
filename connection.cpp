#include "connection.h"

Connection::Connection()
{
}

bool Connection::createconnect()
{
    bool test = false;
    QSqlDatabase db = QSqlDatabase::addDatabase("QODBC");
    db.setDatabaseName("Smartvision"); // insert the data source name
    db.setUserName("Aminebouzidi");        // insert user name
    db.setPassword("123");            // insert password

    if (db.open())
        test = true;

    return test;
}
