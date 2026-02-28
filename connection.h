#ifndef CONNECTION_H
#define CONNECTION_H

#include <QSqlDatabase>
#include <QString>

class Connection
{
public:
    static Connection& createInstance();
    bool createConnection();
    QString lastErrorText() const;

private:
    Connection();
    ~Connection();

    QSqlDatabase db;

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
};

#endif
