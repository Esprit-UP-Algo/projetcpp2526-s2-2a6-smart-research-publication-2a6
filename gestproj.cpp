#include "gestproj.h"

bool GestProjCrud::loadProjets(QList<ProjetRecord>& out,
                                QString* error,
                                const QString& nom,
                                const QString& domaine,
                                const QString& statut)
{
    out.clear();

    QSqlQuery q;
    q.prepare(
        "SELECT \"Id_projet\", \"nom_du_projet\", \"domaine_de_recherche\", "
        "\"date_de_début\", \"date_de_fin\", \"budget\", \"statut\", "
        "\"source_de_financement\", \"numéro_d_approbation_éthique\", "
        "\"nombre_de_publications\" "
        "FROM \"projet\" "
        "WHERE (:nom IS NULL OR :nom = '' OR LOWER(\"nom_du_projet\") LIKE '%' || LOWER(:nom) || '%') "
        "  AND (:domaine IS NULL OR :domaine = '' OR LOWER(\"domaine_de_recherche\") LIKE '%' || LOWER(:domaine) || '%') "
        "  AND (:statut IS NULL OR :statut = '' OR LOWER(\"statut\") LIKE '%' || LOWER(:statut) || '%') "
        "ORDER BY \"nom_du_projet\", \"Id_projet\"");

    q.bindValue(":nom", nom);
    q.bindValue(":domaine", domaine);
    q.bindValue(":statut", statut);

    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }

    while (q.next()) {
        ProjetRecord rec;
        rec.idProjet                  = q.value(0).toInt();
        rec.nomDuProjet               = q.value(1).toString();
        rec.domaineDeRecherche        = q.value(2).toString();
        rec.dateDeDebut               = q.value(3).toDate();
        rec.dateDeFin                 = q.value(4).toDate();
        rec.budget                    = q.value(5).toDouble();
        rec.statut                    = q.value(6).toString();
        rec.sourceDeFinancement       = q.value(7).toString();
        rec.numeroDApprobationEthique = q.value(8).toString();
        rec.nombreDePublications      = q.value(9).toInt();
        out.push_back(rec);
    }

    return true;
}

bool GestProjCrud::fetchProjet(int idProjet, ProjetRecord& out, QString* error)
{
    QSqlQuery q;
    q.prepare("SELECT \"nom_du_projet\", \"domaine_de_recherche\", "
              "\"date_de_début\", \"date_de_fin\", \"budget\", \"statut\", "
              "\"source_de_financement\", \"numéro_d_approbation_éthique\", "
              "\"nombre_de_publications\" "
              "FROM \"projet\" WHERE \"Id_projet\" = :id");
    q.bindValue(":id", idProjet);

    if (!q.exec() || !q.next()) {
        if (error) *error = q.lastError().text();
        return false;
    }

    out.idProjet                  = idProjet;
    out.nomDuProjet               = q.value(0).toString();
    out.domaineDeRecherche        = q.value(1).toString();
    out.dateDeDebut               = q.value(2).toDate();
    out.dateDeFin                 = q.value(3).toDate();
    out.budget                    = q.value(4).toDouble();
    out.statut                    = q.value(5).toString();
    out.sourceDeFinancement       = q.value(6).toString();
    out.numeroDApprobationEthique = q.value(7).toString();
    out.nombreDePublications      = q.value(8).toInt();

    return true;
}

bool GestProjCrud::deleteProjet(int idProjet, QString* error)
{
    QSqlQuery q;
    q.prepare("DELETE FROM \"projet\" WHERE \"Id_projet\" = :id");
    q.bindValue(":id", idProjet);

    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }

    return true;
}

int GestProjCrud::nextProjetId(QString* error)
{
    QSqlQuery q;
    if (!q.exec("SELECT NVL(MAX(\"Id_projet\"),0)+1 FROM \"projet\"") || !q.next()) {
        if (error) *error = q.lastError().text();
        return -1;
    }

    return q.value(0).toInt();
}

bool GestProjCrud::insertProjet(const ProjetRecord& in, QString* error)
{
    if (in.nomDuProjet.trimmed().isEmpty()) {
        if (error) *error = "Le nom du projet est obligatoire.";
        return false;
    }

    int idProjet = in.idProjet;
    if (idProjet <= 0) {
        idProjet = nextProjetId(error);
        if (idProjet <= 0) return false;
    }

    QSqlQuery q;
    q.prepare("INSERT INTO \"projet\" "
              "(\"Id_projet\", \"nom_du_projet\", \"domaine_de_recherche\", "
              "\"date_de_début\", \"date_de_fin\", \"budget\", \"statut\", "
              "\"source_de_financement\", \"numéro_d_approbation_éthique\", "
              "\"nombre_de_publications\") "
              "VALUES (:id, :nom, :domaine, :debut, :fin, :budget, "
              ":statut, :financement, :ethique, :pubs)");

    auto nullStr  = QVariant(QMetaType::fromType<QString>());
    auto nullDate = QVariant(QMetaType::fromType<QDate>());

    q.bindValue(":id", idProjet);
    q.bindValue(":nom", in.nomDuProjet.trimmed());
    q.bindValue(":domaine", in.domaineDeRecherche.trimmed().isEmpty() ? nullStr : QVariant(in.domaineDeRecherche.trimmed()));
    q.bindValue(":debut", in.dateDeDebut.isValid() ? QVariant(in.dateDeDebut) : nullDate);
    q.bindValue(":fin", in.dateDeFin.isValid() ? QVariant(in.dateDeFin) : nullDate);
    q.bindValue(":budget", in.budget);
    q.bindValue(":statut", in.statut.trimmed().isEmpty() ? nullStr : QVariant(in.statut.trimmed()));
    q.bindValue(":financement", in.sourceDeFinancement.trimmed().isEmpty() ? nullStr : QVariant(in.sourceDeFinancement.trimmed()));
    q.bindValue(":ethique", in.numeroDApprobationEthique.trimmed().isEmpty() ? nullStr : QVariant(in.numeroDApprobationEthique.trimmed()));
    q.bindValue(":pubs", in.nombreDePublications);

    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }

    return true;
}

bool GestProjCrud::updateProjet(const ProjetRecord& in, QString* error)
{
    if (in.idProjet <= 0) {
        if (error) *error = "Id_projet invalide.";
        return false;
    }
    if (in.nomDuProjet.trimmed().isEmpty()) {
        if (error) *error = "Le nom du projet est obligatoire.";
        return false;
    }

    QSqlQuery q;
    q.prepare("UPDATE \"projet\" SET "
              "\"nom_du_projet\" = :nom, "
              "\"domaine_de_recherche\" = :domaine, "
              "\"date_de_début\" = :debut, "
              "\"date_de_fin\" = :fin, "
              "\"budget\" = :budget, "
              "\"statut\" = :statut, "
              "\"source_de_financement\" = :financement, "
              "\"numéro_d_approbation_éthique\" = :ethique, "
              "\"nombre_de_publications\" = :pubs "
              "WHERE \"Id_projet\" = :id");

    auto nullStr  = QVariant(QMetaType::fromType<QString>());
    auto nullDate = QVariant(QMetaType::fromType<QDate>());

    q.bindValue(":nom", in.nomDuProjet.trimmed());
    q.bindValue(":domaine", in.domaineDeRecherche.trimmed().isEmpty() ? nullStr : QVariant(in.domaineDeRecherche.trimmed()));
    q.bindValue(":debut", in.dateDeDebut.isValid() ? QVariant(in.dateDeDebut) : nullDate);
    q.bindValue(":fin", in.dateDeFin.isValid() ? QVariant(in.dateDeFin) : nullDate);
    q.bindValue(":budget", in.budget);
    q.bindValue(":statut", in.statut.trimmed().isEmpty() ? nullStr : QVariant(in.statut.trimmed()));
    q.bindValue(":financement", in.sourceDeFinancement.trimmed().isEmpty() ? nullStr : QVariant(in.sourceDeFinancement.trimmed()));
    q.bindValue(":ethique", in.numeroDApprobationEthique.trimmed().isEmpty() ? nullStr : QVariant(in.numeroDApprobationEthique.trimmed()));
    q.bindValue(":pubs", in.nombreDePublications);
    q.bindValue(":id", in.idProjet);

    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return false;
    }

    return true;
}
