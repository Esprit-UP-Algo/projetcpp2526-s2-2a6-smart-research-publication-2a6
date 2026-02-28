#include "crud.h"

bool ExperienceCrud::loadProjects(QList<ProjectItem>& out, QString* error)
{
	out.clear();
	QSqlQuery q;
	q.prepare("SELECT ID_PROJET, NOM_DU_PROJET FROM PROJET ORDER BY ID_PROJET");
	if (!q.exec()) {
		if (error) *error = q.lastError().text();
		return false;
	}
	while (q.next()) {
		ProjectItem item;
		item.id = q.value(0).toInt();
		item.name = q.value(1).toString();
		out.push_back(item);
	}
	return true;
}

bool ExperienceCrud::loadExperiences(QList<ExperienceRecord>& out, QString* error)
{
	out.clear();
	QSqlQuery q;
	q.prepare("SELECT ID_EXP, TITRE, HYPOTHESE, DATE_DEBUT, DATE_FIN, STATUS FROM EXPERIENCE ORDER BY ID_EXP DESC");
	if (!q.exec()) {
		if (error) *error = q.lastError().text();
		return false;
	}
	while (q.next()) {
		ExperienceRecord rec;
		rec.id = q.value(0).toInt();
		rec.titre = q.value(1).toString();
		rec.hypothese = q.value(2).toString();
		rec.dateDebut = q.value(3).toDate();
		rec.dateFin = q.value(4).toDate();
		rec.status = q.value(5).toString();
		out.push_back(rec);
	}
	return true;
}

bool ExperienceCrud::fetchExperience(int id, ExperienceRecord& out, QString* error)
{
	QSqlQuery q;
	q.prepare("SELECT TITRE, HYPOTHESE, DATE_DEBUT, DATE_FIN, STATUS, ID_PROJET FROM EXPERIENCE WHERE ID_EXP = :id");
	q.bindValue(":id", id);
	if (!q.exec() || !q.next()) {
		if (error) *error = q.lastError().text();
		return false;
	}
	out.id = id;
	out.titre = q.value(0).toString();
	out.hypothese = q.value(1).toString();
	out.dateDebut = q.value(2).toDate();
	out.dateFin = q.value(3).toDate();
	out.status = q.value(4).toString();
	out.projetId = q.value(5);
	return true;
}

bool ExperienceCrud::deleteExperience(int id, QString* error)
{
	QSqlQuery q;
	q.prepare("DELETE FROM EXPERIENCE WHERE ID_EXP = :id");
	q.bindValue(":id", id);
	if (!q.exec()) {
		if (error) *error = q.lastError().text();
		return false;
	}
	return true;
}

int ExperienceCrud::nextExperienceId(QString* error)
{
	QSqlQuery q;
	if (!q.exec("SELECT NVL(MAX(ID_EXP),0)+1 FROM EXPERIENCE") || !q.next()) {
		if (error) *error = q.lastError().text();
		return -1;
	}
	return q.value(0).toInt();
}

bool ExperienceCrud::insertExperience(const ExperienceRecord& in, QString* error)
{
	int id = in.id;
	if (id <= 0) {
		id = nextExperienceId(error);
		if (id <= 0) return false;
	}

	QSqlQuery q;
	q.prepare("INSERT INTO EXPERIENCE (ID_EXP, TITRE, HYPOTHESE, DATE_DEBUT, DATE_FIN, STATUS, ID_PROJET, EQUIPEMENT_ID) "
			  "VALUES (:id, :t, :h, :d, :df, :s, :p, NULL)");
	q.bindValue(":id", id);
	q.bindValue(":t", in.titre);
	q.bindValue(":h", in.hypothese);
	q.bindValue(":d", in.dateDebut);
	q.bindValue(":df", in.dateFin);
	q.bindValue(":s", in.status);
	q.bindValue(":p", in.projetId);
	if (!q.exec()) {
		if (error) *error = q.lastError().text();
		return false;
	}
	return true;
}

bool ExperienceCrud::updateExperience(const ExperienceRecord& in, QString* error)
{
	QSqlQuery q;
	q.prepare("UPDATE EXPERIENCE SET TITRE = :t, HYPOTHESE = :h, DATE_DEBUT = :d, DATE_FIN = :df, STATUS = :s, ID_PROJET = :p WHERE ID_EXP = :id");
	q.bindValue(":t", in.titre);
	q.bindValue(":h", in.hypothese);
	q.bindValue(":d", in.dateDebut);
	q.bindValue(":df", in.dateFin);
	q.bindValue(":s", in.status);
	q.bindValue(":p", in.projetId);
	q.bindValue(":id", in.id);
	if (!q.exec()) {
		if (error) *error = q.lastError().text();
		return false;
	}
	return true;
}
