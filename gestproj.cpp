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
        "SELECT ID_PROJET, NOM_DU_PROJET, DOMAINE_DE_RECHERCHE, "
        "DATE_DE_DEBUT, DATE_DE_FIN, BUDGET, STATUT, "
        "SOURCE_DE_FINANCEMENT, NUMERO_D_APPROBATION_ETHIQUE, "
        "NOMBRE_DE_PUBLICATIONS "
        "FROM PROJET "
        "WHERE (:nom IS NULL OR :nom = '' OR LOWER(NOM_DU_PROJET) LIKE '%' || LOWER(:nom) || '%') "
        "  AND (:domaine IS NULL OR :domaine = '' OR LOWER(DOMAINE_DE_RECHERCHE) LIKE '%' || LOWER(:domaine) || '%') "
        "  AND (:statut IS NULL OR :statut = '' OR LOWER(STATUT) LIKE '%' || LOWER(:statut) || '%') "
        "ORDER BY NOM_DU_PROJET, ID_PROJET");

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
    q.prepare("SELECT NOM_DU_PROJET, DOMAINE_DE_RECHERCHE, "
              "DATE_DE_DEBUT, DATE_DE_FIN, BUDGET, STATUT, "
              "SOURCE_DE_FINANCEMENT, NUMERO_D_APPROBATION_ETHIQUE, "
              "NOMBRE_DE_PUBLICATIONS "
              "FROM PROJET WHERE ID_PROJET = :id");
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
    q.prepare("DELETE FROM PROJET WHERE ID_PROJET = :id");
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
    if (!q.exec("SELECT NVL(MAX(ID_PROJET),0)+1 FROM PROJET") || !q.next()) {
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
    q.prepare("INSERT INTO PROJET "
              "(ID_PROJET, NOM_DU_PROJET, DOMAINE_DE_RECHERCHE, "
              "DATE_DE_DEBUT, DATE_DE_FIN, BUDGET, STATUT, "
              "SOURCE_DE_FINANCEMENT, NUMERO_D_APPROBATION_ETHIQUE, "
              "NOMBRE_DE_PUBLICATIONS) "
              "VALUES (:id, :nom, :domaine, :debut, :fin, :budget, "
              ":statut, :financement, :ethique, :pubs)");

    auto nullStr = QVariant(QMetaType::fromType<QString>());
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
        if (error) *error = "ID_PROJET invalide.";
        return false;
    }
    if (in.nomDuProjet.trimmed().isEmpty()) {
        if (error) *error = "Le nom du projet est obligatoire.";
        return false;
    }

    QSqlQuery q;
    q.prepare("UPDATE PROJET SET "
              "NOM_DU_PROJET = :nom, "
              "DOMAINE_DE_RECHERCHE = :domaine, "
              "DATE_DE_DEBUT = :debut, "
              "DATE_DE_FIN = :fin, "
              "BUDGET = :budget, "
              "STATUT = :statut, "
              "SOURCE_DE_FINANCEMENT = :financement, "
              "NUMERO_D_APPROBATION_ETHIQUE = :ethique, "
              "NOMBRE_DE_PUBLICATIONS = :pubs "
              "WHERE ID_PROJET = :id");

    auto nullStr = QVariant(QMetaType::fromType<QString>());
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
