#ifndef CONFIG_H
#define CONFIG_H

#include <QObject>
#include <QJsonObject>
#include <QString>
#include <QVariant>

class Config : public QObject
{
    Q_OBJECT

public:
    explicit Config(QObject *parent = nullptr);

    bool createProject(const QString &projectName);
    bool loadProject(const QString &projectFilePath);
    bool saveProject(const QJsonObject &data);
    QVariant loadAppValue(const QString &key, const QVariant &defaultValue = QVariant()) const;
    bool saveAppValue(const QString &key, const QVariant &value) const;

    QString projectFilePath() const;
    QString appConfigFilePath() const;

    QString m_projectFilePath;
};

#endif // CONFIG_H
