#ifndef LABELINGSERVICE_H
#define LABELINGSERVICE_H

#include <QObject>
#include <QStringList>
#include <QString>

class SystemService;

class LabelingService : public QObject
{
    Q_OBJECT

public:
    explicit LabelingService(SystemService *systemService, QObject *parent = nullptr);

    SystemService *systemService() const;
    QStringList supportedExtensions() const;
    QString annotationKeyForPath(const QString &path) const;
    QString normalizedKind(const QString &kind) const;
    QString normalizedSeverity(const QString &severity) const;
    QString normalizedSplit(const QString &split) const;
    bool upsertImagePathByFilename(QStringList &imagePaths, const QString &path, int *addedCount, int *updatedCount) const;
    QString canonicalProjectAction(const QString &action) const;
    QString canonicalImageAction(const QString &action) const;
    QString canonicalAnnotationAction(const QString &action) const;
    QString canonicalIoAction(const QString &action) const;
    QString canonicalListAction(const QString &action) const;

private:
    SystemService *m_systemService = nullptr;
};

#endif // LABELINGSERVICE_H
