#include "labelingservice.h"

#include "systemservice.h"

#include <QFileInfo>

LabelingService::LabelingService(SystemService *systemService, QObject *parent)
    : QObject(parent)
    , m_systemService(systemService)
{
}

SystemService *LabelingService::systemService() const
{
    return m_systemService;
}

QStringList LabelingService::supportedExtensions() const
{
    return {
        QStringLiteral(".png"),
        QStringLiteral(".jpg"),
        QStringLiteral(".jpeg"),
        QStringLiteral(".bmp"),
        QStringLiteral(".tif"),
        QStringLiteral(".tiff")
    };
}

QString LabelingService::annotationKeyForPath(const QString &path) const
{
    return QFileInfo(path).fileName().toLower();
}

QString LabelingService::normalizedKind(const QString &kind) const
{
    const QString value = kind.trimmed().toLower();
    if (value == QStringLiteral("predict") || value == QStringLiteral("good")) {
        return value;
    }
    return QStringLiteral("label");
}

QString LabelingService::normalizedSeverity(const QString &severity) const
{
    const QString value = severity.trimmed().toLower();
    if (value == QStringLiteral("serious") || value == QStringLiteral("marginal")) {
        return value;
    }
    return QStringLiteral("normal");
}

QString LabelingService::normalizedSplit(const QString &split) const
{
    const QString value = split.trimmed().toLower();
    if (value == QStringLiteral("train") || value == QStringLiteral("val") || value == QStringLiteral("test")) {
        return value;
    }
    return QStringLiteral("none");
}

bool LabelingService::upsertImagePathByFilename(QStringList &imagePaths, const QString &path, int *addedCount, int *updatedCount) const
{
    const QString newFileName = QFileInfo(path).fileName();
    if (newFileName.isEmpty()) {
        return false;
    }

    if (imagePaths.contains(path)) {
        return false;
    }

    for (int i = 0; i < imagePaths.size(); ++i) {
        const QString existingPath = imagePaths.at(i);
        const QString existingFileName = QFileInfo(existingPath).fileName();
        if (existingFileName.compare(newFileName, Qt::CaseInsensitive) != 0) {
            continue;
        }

        imagePaths[i] = path;
        if (updatedCount) {
            ++(*updatedCount);
        }
        return true;
    }

    imagePaths.push_back(path);
    if (addedCount) {
        ++(*addedCount);
    }
    return true;
}
