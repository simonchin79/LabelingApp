#ifndef SYSTEMSERVICE_H
#define SYSTEMSERVICE_H

#include <QObject>
#include <QJsonObject>
#include <QVariant>

#include "config.h"

class SystemService : public QObject
{
    Q_OBJECT

public:
    explicit SystemService(QObject *parent = nullptr);

    bool createProject(const QString &projectName);
    bool loadProject(const QString &projectFilePath);
    bool saveProject(const QJsonObject &data);

    QVariant loadAppValue(const QString &key, const QVariant &defaultValue = QVariant()) const;
    bool saveAppValue(const QString &key, const QVariant &value) const;

    QString projectFilePath() const;
    QString appConfigFilePath() const;

    static QString normalizePath(const QVariant &urlOrPath);

private:
    Config m_config;
};

#endif // SYSTEMSERVICE_H
