#ifndef EQUIPEMENT_H
#define EQUIPEMENT_H

#include <QString>
#include <QDate>
#include <QVariant>
#include <QList>

struct ResponsableItem {
    int id = 0;
    QString fullName;
};

struct EquipementRecord {
    int      id = 0;
    QString  nomEquipement;
    QString  fabricant;
    QString  numeroModele;
    QDate    dateAchat;
    QDate    dateDerniereMaintenance;
    QDate    dateProchaineMaintenance;
    QString  statut;
    QString  localisation;
    QDate    dateLimiteCalibration;
    QVariant idExp;      // FK → EXPERIENCE.ID_EXP (nullable)
};

class EquipementCrud
{
public:
    bool loadResponsables(QList<ResponsableItem>& out,      QString* error = nullptr);
    bool loadEquipements (QList<EquipementRecord>& out,     QString* error = nullptr,
                          const QString& fabricant = QString(),
                          const QString& nom = QString(),
                          const QString& statut = QString(),
                          const QString& localisation = QString());
    bool fetchEquipement (int id, EquipementRecord& out,    QString* error = nullptr);
    bool deleteEquipement(int id,                           QString* error = nullptr);
    int  nextEquipementId(                                  QString* error = nullptr);
    bool insertEquipement(const EquipementRecord& in,       QString* error = nullptr);
    bool updateEquipement(const EquipementRecord& in,       QString* error = nullptr);
};

#endif // EQUIPEMENT_H
