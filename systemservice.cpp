#include "systemservice.h"

#include <QUrl>

SystemService::SystemService(QObject *parent)
    : QObject(parent)
    , m_config(this)
{
}

bool SystemService::createProject(const QString &projectName)
{
    return m_config.createProject(projectName);
}

bool SystemService::loadProject(const QString &projectFilePath)
{
    return m_config.loadProject(projectFilePath);
}

bool SystemService::saveProject(const QJsonObject &data)
{
    return m_config.saveProject(data);
}

QVariant SystemService::loadAppValue(const QString &key, const QVariant &defaultValue) const
{
    return m_config.loadAppValue(key, defaultValue);
}

bool SystemService::saveAppValue(const QString &key, const QVariant &value) const
{
    return m_config.saveAppValue(key, value);
}

QString SystemService::projectFilePath() const
{
    return m_config.projectFilePath();
}

QString SystemService::appConfigFilePath() const
{
    return m_config.appConfigFilePath();
}

QString SystemService::normalizePath(const QVariant &urlOrPath)
{
    if (!urlOrPath.isValid()) {
        return QString();
    }

    if (urlOrPath.canConvert<QUrl>()) {
        const QUrl url = urlOrPath.toUrl();
        if (url.isValid() && url.isLocalFile()) {
            return url.toLocalFile();
        }
    }

    const QString asString = urlOrPath.toString().trimmed();
    if (asString.isEmpty()) {
        return QString();
    }

    const QUrl url(asString);
    if (url.isValid() && url.isLocalFile()) {
        return url.toLocalFile();
    }

    return asString;
}
