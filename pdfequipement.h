#ifndef PDFEQUIPEMENT_H
#define PDFEQUIPEMENT_H

#include "crudEquipement.h"
#include <QString>

void exportEquipementPdf(const EquipementRecord& info, const QString& path);

#endif // PDFEQUIPEMENT_H
