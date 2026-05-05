#ifndef PDFEXP_H
#define PDFEXP_H

#include <QString>

struct ExperiencePdfInfo {
    int id = 0;
    QString titre;
    QString hypothese;
    QString dateDebut;
    QString dateFin;
    QString statut;
    QString typeExperience;
    QString disponibilite;
    QString resultat;
    QString projet;
};

void exportExperiencePdf(const ExperiencePdfInfo& info, const QString& path);

#endif // PDFEXP_H
