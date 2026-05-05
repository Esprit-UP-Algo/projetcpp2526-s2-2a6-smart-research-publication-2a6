#ifndef PDFEMPLOYE_H
#define PDFEMPLOYE_H

#include "employes.h"
#include <QString>

// Génère un rapport PDF pour un employé.
// rec  : données de l'employé
// path : chemin complet du fichier PDF de sortie
void exportEmployePdf(const EmployeRecord& rec, const QString& path);

#endif // PDFEMPLOYE_H
