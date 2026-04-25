#ifndef SAISIEBIO_H
#define SAISIEBIO_H

#include <QString>
#include <QVariant>

namespace SaisieBio
{
QVariant normalizeStorageTemperature(const QString& raw);
QString  normalizeSampleType(const QString& raw);
QString  normalizeDangerLevel(const QString& raw);
}

#endif // SAISIEBIO_H
