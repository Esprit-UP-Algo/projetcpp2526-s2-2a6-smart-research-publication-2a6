#ifndef PDFBIOSAMPLE_H
#define PDFBIOSAMPLE_H

#include "basicbio.h"
#include <QString>

// Génère un rapport PDF pour un échantillon biologique.
// bi   : informations de l'échantillon
// path : chemin complet du fichier PDF de sortie
void exportBioSamplePdf(const BasicBioInfo& bi, const QString& path);

#endif // PDFBIOSAMPLE_H
