#ifndef EMPLOYES_H
#define EMPLOYES_H

#include <QString>
#include <QList>
#include <QSqlQuery>
#include <QSqlError>

struct EmployeRecord {
    int     employeeId = 0;
    QString cin;
    QString nom;
    QString prenom;
    QString role;
    QString specialization;
    QString qualification;
    QString tempsTravail;
    QString laboratoire;
};

class EmployesCrud
{
public:
    bool loadEmployes(QList<EmployeRecord>& out,
                      QString* error = nullptr,
                      const QString& cin = QString(),
                      const QString& nom = QString(),
                      const QString& prenom = QString(),
                      const QString& role = QString(),
                      const QString& specialization = QString());
    bool fetchEmploye(int employeeId, EmployeRecord& out, QString* error = nullptr);
    bool deleteEmploye(int employeeId, QString* error = nullptr);
    int  nextEmployeId(QString* error = nullptr);
    bool insertEmploye(const EmployeRecord& in, QString* error = nullptr);
    bool updateEmploye(const EmployeRecord& in, QString* error = nullptr);
};

#endif // EMPLOYES_H
