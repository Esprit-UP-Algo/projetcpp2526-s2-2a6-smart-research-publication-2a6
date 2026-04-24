#include "saisiebio.h"

namespace SaisieBio
{

QVariant normalizeStorageTemperature(const QString& raw)
{
    const QString t = raw.trimmed();
    if (t.isEmpty() || t == "-" || t == "--") return QVariant(QMetaType::fromType<double>());
    const QString lower = t.toLower();
    if (lower.contains("-80")) return QVariant(-80.0);
    if (lower.contains("-20")) return QVariant(-20.0);
    if (lower.contains("+4") || lower == "4" || lower.contains("4c")) return QVariant(4.0);
    if (lower.contains("amb")) return QVariant(QMetaType::fromType<double>());
    bool ok = false;
    double v = t.toDouble(&ok);
    if (ok) return QVariant(v);
    return QVariant(QMetaType::fromType<double>());
}

QString normalizeSampleType(const QString& raw)
{
    const QString t = raw.trimmed();
    if (t.isEmpty()) return QString();

    const QString lower = t.toLower();
    if (lower == "adn" || lower == "dna") return "ADN";
    if (lower == "arn" || lower == "rna") return "ARN";
    if (lower.contains("prot")) return "Protéine";
    if (lower.contains("cell")) return "Cellule";
    if (lower.contains("tiss")) return "Tissu";
    if (lower.contains("organ")) return "Organisme";

    return QString();
}

QString normalizeDangerLevel(const QString& raw)
{
    const QString t = raw.trimmed();
    if (t.isEmpty()) return "BSL-1";

    if (t == "BSL-1" || t == "BSL-2" || t == "BSL-3") return t;
    return QString();
}

}
