#ifndef TRACABILITEMANAGER_H
#define TRACABILITEMANAGER_H

#include "gestproj.h"
#include <QString>
#include <QDate>

class TracabiliteManager
{
public:
    static void setUserContext(const QString& fullName, const QString& email);
    static void clearUserContext();

    static void logAjoutProjet(const ProjetRecord& p);
    static void logModificationProjet(const ProjetRecord& ancien,
                                      const ProjetRecord& nouveau);
    static void logSuppressionProjet(const ProjetRecord& p);
    static void logConnexion(const QString& fullName,
                             const QString& email,
                             const QString& role);
    static void logDeconnexion(const QString& fullName,
                               const QString& email);
};

#endif // TRACABILITEMANAGER_H
