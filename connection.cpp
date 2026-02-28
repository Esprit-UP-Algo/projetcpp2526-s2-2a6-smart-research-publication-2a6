#include "connection.h"
#include <QDebug>
#include <QSqlError>

Connection::Connection()
{
    db = QSqlDatabase::addDatabase("QODBC");
}

Connection::~Connection()
{
    if (db.isOpen()) {
        db.close();
    }
}

Connection& Connection::createInstance()
{
    static Connection instance;
    return instance;
}

bool Connection::createConnection()
{
    db.setDatabaseName("smartvision");
    db.setUserName("AmineBouzidi");
    db.setPassword("123");

    if (db.open()) {
        return true;
    }

    qDebug() << "Database connection failed:" << db.lastError().text();
    return false;
}

QString Connection::lastErrorText() const
{
    return db.lastError().text();
}
