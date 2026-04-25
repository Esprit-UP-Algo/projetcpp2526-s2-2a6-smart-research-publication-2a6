#ifndef EMPLOYEEVERIFICATION_H
#define EMPLOYEEVERIFICATION_H

#include <QString>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDatabase>
#include <QDebug>

// ================================================================================
// EmployeeVerification - Database queries for employee verification
// ================================================================================

class EmployeeVerification
{
public:
    /**
     * Structure to hold employee data
     */
    struct EmployeeData {
        int employeeId;
        QString cin;
        QString fullName;
        QString nom;
        QString prenom;
        QString email;
        QString role;
        QString status;
        bool isActive;
        
        EmployeeData() : employeeId(0), isActive(false) {}
    };

    /**
     * Verify employee exists and is active by CIN
     * @param cin: Employee identification number
     * @param employeeData: Output parameter with employee data
     * @return true if employee found and active, false otherwise
     */
    static bool verifyEmployeeByCIN(const QString &cin, EmployeeData &employeeData) {
        if (cin.isEmpty()) {
            qWarning() << "CIN is empty";
            return false;
        }

        QSqlQuery query;
        query.prepare("SELECT \"employee_id\", \"CIN\", \"FULL_NAME\", \"nom\", \"prenom\", "
                      "\"EMAIL\", \"ROLE\", \"ACTIVE\" "
                      "FROM \"Employés\" "
                      "WHERE \"CIN\" = :cin");
        query.addBindValue(cin);

        if (!query.exec()) {
            qWarning() << "Database query failed:" << query.lastError().text();
            return false;
        }

        if (query.next()) {
            employeeData.employeeId = query.value(0).toInt();
            employeeData.cin = query.value(1).toString();
            employeeData.fullName = query.value(2).toString();
            employeeData.nom = query.value(3).toString();
            employeeData.prenom = query.value(4).toString();
            employeeData.email = query.value(5).toString();
            employeeData.role = query.value(6).toString();
            
            QString activeStatus = query.value(7).toString();
            employeeData.isActive = (activeStatus == "O" || activeStatus == "Y" || activeStatus == "1");

            if (employeeData.isActive) {
                qDebug() << "Employee verified:" << employeeData.fullName;
                return true;
            } else {
                qWarning() << "Employee found but not active:" << cin;
                return false;
            }
        }

        qWarning() << "Employee not found with CIN:" << cin;
        return false;
    }

    /**
     * Get employee name by CIN (simplified version for Arduino)
     * @param cin: Employee identification number
     * @return Full name if found and active, empty string otherwise
     */
    static QString getEmployeeNameByCIN(const QString &cin) {
        EmployeeData empData;
        if (verifyEmployeeByCIN(cin, empData)) {
            return empData.fullName;
        }
        return "";
    }

    /**
     * Verify employee by ID
     * @param employeeId: Employee ID
     * @param employeeData: Output parameter with employee data
     * @return true if employee found and active, false otherwise
     */
    static bool verifyEmployeeByID(int employeeId, EmployeeData &employeeData) {
        if (employeeId <= 0) {
            qWarning() << "Invalid employee ID";
            return false;
        }

        QSqlQuery query;
        query.prepare("SELECT \"employee_id\", \"CIN\", \"FULL_NAME\", \"nom\", \"prenom\", "
                      "\"EMAIL\", \"ROLE\", \"ACTIVE\" "
                      "FROM \"Employés\" "
                      "WHERE \"employee_id\" = :id");
        query.addBindValue(employeeId);

        if (!query.exec()) {
            qWarning() << "Database query failed:" << query.lastError().text();
            return false;
        }

        if (query.next()) {
            employeeData.employeeId = query.value(0).toInt();
            employeeData.cin = query.value(1).toString();
            employeeData.fullName = query.value(2).toString();
            employeeData.nom = query.value(3).toString();
            employeeData.prenom = query.value(4).toString();
            employeeData.email = query.value(5).toString();
            employeeData.role = query.value(6).toString();
            
            QString activeStatus = query.value(7).toString();
            employeeData.isActive = (activeStatus == "O" || activeStatus == "Y" || activeStatus == "1");

            return employeeData.isActive;
        }

        return false;
    }

    /**
     * Log access attempt (optional - for audit trail)
     * @param cin: Employee CIN
     * @param success: Whether verification was successful
     */
    static void logAccessAttempt(const QString &cin, bool success) {
        // This can be extended to log to a database table
        // or file for audit purposes
        if (success) {
            qInfo() << "[ACCESS GRANTED] CIN:" << cin;
        } else {
            qWarning() << "[ACCESS DENIED] CIN:" << cin;
        }
    }

    /**
     * Validate CIN format (optional - can be customized)
     * @param cin: CIN to validate
     * @return true if format is valid
     */
    static bool isValidCINFormat(const QString &cin) {
        if (cin.isEmpty() || cin.length() > 20) {
            return false;
        }
        // Add your CIN validation logic here
        return true;
    }
};

#endif // EMPLOYEEVERIFICATION_H
