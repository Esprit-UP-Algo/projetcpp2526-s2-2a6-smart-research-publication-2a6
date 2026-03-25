#include "employes.h"

#include <QMetaType>

bool EmployesCrud::loadEmployes(QList<EmployeRecord>& out,
                                QString* error,
                                const QString& cin,
                                const QString& nom,
                                const QString& prenom,
                                const QString& role,
                                const QString& specialization)
{
    out.clear();

    QSqlQuery q;
    q.prepare(
        "SELECT \"employee_id\", \"CIN\", \"nom\", \"prenom\", \"ROLE\", \"specialization\" "
        "FROM \"Employés\" "
        "WHERE (:cin IS NULL OR :cin = '' OR LOWER(\"CIN\") LIKE '%' || LOWER(:cin) || '%') "
        "  AND (:nom IS NULL OR :nom = '' OR LOWER(\"nom\") LIKE '%' || LOWER(:nom) || '%') "
        "  AND (:prenom IS NULL OR :prenom = '' OR LOWER(\"prenom\") LIKE '%' || LOWER(:prenom) || '%') "
        "  AND (:role IS NULL OR :role = '' OR LOWER(\"ROLE\") LIKE '%' || LOWER(:role) || '%') "
        "  AND (:spec IS NULL OR :spec = '' OR LOWER(\"specialization\") LIKE '%' || LOWER(:spec) || '%') "
        "ORDER BY \"nom\", \"prenom\", \"employee_id\"");

    q.bindValue(":cin", cin);
    q.bindValue(":nom", nom);
    q.bindValue(":prenom", prenom);
    q.bindValue(":role", role);
    q.bindValue(":spec", specialization);

    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }

    while (q.next()) {
        EmployeRecord rec;
        rec.employeeId     = q.value(0).toInt();
        rec.cin            = q.value(1).toString();
        rec.nom            = q.value(2).toString();
        rec.prenom         = q.value(3).toString();
        rec.role           = q.value(4).toString();
        rec.specialization = q.value(5).toString();
        rec.qualification.clear();
        rec.tempsTravail.clear();
        rec.laboratoire.clear();
        out.push_back(rec);
    }

    return true;
}

bool EmployesCrud::fetchEmploye(int employeeId, EmployeRecord& out, QString* error)
{
    QSqlQuery q;
    q.prepare("SELECT \"CIN\", \"nom\", \"prenom\", \"ROLE\", \"specialization\" "
              "FROM \"Employés\" WHERE \"employee_id\" = :id");
    q.bindValue(":id", employeeId);

    if (!q.exec() || !q.next()) {
        if (error) *error = q.lastError().text();
        return false;
    }

    out.employeeId     = employeeId;
    out.cin            = q.value(0).toString();
    out.nom            = q.value(1).toString();
    out.prenom         = q.value(2).toString();
    out.role           = q.value(3).toString();
    out.specialization = q.value(4).toString();
    out.qualification.clear();
    out.tempsTravail.clear();
    out.laboratoire.clear();

    return true;
}

bool EmployesCrud::deleteEmploye(int employeeId, QString* error)
{
    QSqlQuery q;
    q.prepare("DELETE FROM \"Employés\" WHERE \"employee_id\" = :id");
    q.bindValue(":id", employeeId);

    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }

    return true;
}

int EmployesCrud::nextEmployeId(QString* error)
{
    QSqlQuery q;
    if (!q.exec("SELECT NVL(MAX(\"employee_id\"),0)+1 FROM \"Employés\"") || !q.next()) {
        if (error) *error = q.lastError().text();
        return -1;
    }

    return q.value(0).toInt();
}

bool EmployesCrud::insertEmploye(const EmployeRecord& in, QString* error)
{
    if (in.cin.trimmed().isEmpty()) {
        if (error) *error = "CIN est obligatoire.";
        return false;
    }
    if (in.nom.trimmed().isEmpty()) {
        if (error) *error = "NOM est obligatoire.";
        return false;
    }
    if (in.prenom.trimmed().isEmpty()) {
        if (error) *error = "PRENOM est obligatoire.";
        return false;
    }

    int employeeId = in.employeeId;
    if (employeeId <= 0) {
        employeeId = nextEmployeId(error);
        if (employeeId <= 0) return false;
    }

    QSqlQuery q;
    q.prepare("INSERT INTO \"Employés\" "
              "(\"employee_id\", \"CIN\", \"nom\", \"prenom\", \"ROLE\", \"specialization\") "
              "VALUES (:id, :cin, :nom, :prenom, :role, :spec)");

    auto nullStr = QVariant(QMetaType::fromType<QString>());

    q.bindValue(":id", employeeId);
    q.bindValue(":cin", in.cin.trimmed());
    q.bindValue(":nom", in.nom.trimmed());
    q.bindValue(":prenom", in.prenom.trimmed());
    q.bindValue(":role", in.role.trimmed().isEmpty() ? nullStr : QVariant(in.role.trimmed()));
    q.bindValue(":spec", in.specialization.trimmed().isEmpty() ? nullStr : QVariant(in.specialization.trimmed()));

    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }

    return true;
}

bool EmployesCrud::updateEmploye(const EmployeRecord& in, QString* error)
{
    if (in.employeeId <= 0) {
        if (error) *error = "EMPLOYEE_ID invalide.";
        return false;
    }
    if (in.cin.trimmed().isEmpty()) {
        if (error) *error = "CIN est obligatoire.";
        return false;
    }
    if (in.nom.trimmed().isEmpty()) {
        if (error) *error = "NOM est obligatoire.";
        return false;
    }
    if (in.prenom.trimmed().isEmpty()) {
        if (error) *error = "PRENOM est obligatoire.";
        return false;
    }

    QSqlQuery q;
    q.prepare("UPDATE \"Employés\" "
              "SET \"CIN\" = :cin, \"nom\" = :nom, \"prenom\" = :prenom, "
              "    \"ROLE\" = :role, \"specialization\" = :spec "
              "WHERE \"employee_id\" = :id");

    auto nullStr = QVariant(QMetaType::fromType<QString>());

    q.bindValue(":cin", in.cin.trimmed());
    q.bindValue(":nom", in.nom.trimmed());
    q.bindValue(":prenom", in.prenom.trimmed());
    q.bindValue(":role", in.role.trimmed().isEmpty() ? nullStr : QVariant(in.role.trimmed()));
    q.bindValue(":spec", in.specialization.trimmed().isEmpty() ? nullStr : QVariant(in.specialization.trimmed()));
    q.bindValue(":id", in.employeeId);

    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }

    return true;
}
