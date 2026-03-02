#include "config.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>

Config::Config(QObject *parent)
    : QObject(parent)
{
}

bool Config::createProject(const QString &projectName)
{
    QString filePath = projectName.trimmed();
    if (filePath.isEmpty()) {
        return false;
    }

    if (!filePath.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive)) {
        filePath += QStringLiteral(".json");
    }

    QFileInfo pathInfo(filePath);
    if (pathInfo.isRelative()) {
        filePath = QDir::current().absoluteFilePath(filePath);
        pathInfo = QFileInfo(filePath);
    }

    QDir parentDir = pathInfo.absoluteDir();
    if (!parentDir.exists() && !parentDir.mkpath(QStringLiteral("."))) {
        return false;
    }

    m_projectFilePath = pathInfo.absoluteFilePath();
    const QString projectBaseName = pathInfo.completeBaseName();
    if (projectBaseName.isEmpty()) {
        return false;
    }

    QFile file(m_projectFilePath);
    if (file.exists()) {
        return true;
    }

    const QJsonObject initial{
        {"projectName", projectBaseName},
        {"images", QJsonArray()},
        {"classes", QJsonArray()}
    };

    return saveProject(initial);
}

bool Config::loadProject(const QString &projectFilePath)
{
    QFile file(projectFilePath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject()) {
        return false;
    }

    m_projectFilePath = QFileInfo(file).absoluteFilePath();
    return true;
}

bool Config::saveProject(const QJsonObject &data)
{
    if (m_projectFilePath.isEmpty()) {
        return false;
    }

    QFile file(m_projectFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    const QJsonDocument doc(data);
    const qint64 written = file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return written > 0;
}

QString Config::projectFilePath() const
{
    return m_projectFilePath;
}

QVariant Config::loadAppValue(const QString &key, const QVariant &defaultValue) const
{
    QFile file(appConfigFilePath());
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        return defaultValue;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject()) {
        return defaultValue;
    }

    const QJsonObject root = doc.object();
    if (!root.contains(key)) {
        return defaultValue;
    }

    return root.value(key).toVariant();
}

bool Config::saveAppValue(const QString &key, const QVariant &value) const
{
    QFile file(appConfigFilePath());
    QJsonObject root;

    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        const QJsonDocument readDoc = QJsonDocument::fromJson(file.readAll());
        file.close();
        if (readDoc.isObject()) {
            root = readDoc.object();
        }
    }

    root.insert(key, QJsonValue::fromVariant(value));

    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    const QJsonDocument writeDoc(root);
    const qint64 written = file.write(writeDoc.toJson(QJsonDocument::Indented));
    file.close();
    return written > 0;
}

QString Config::appConfigFilePath() const
{
    return QDir::current().absoluteFilePath(QStringLiteral("app_config.json"));
}
