#ifndef INFERENCESERVICE_H
#define INFERENCESERVICE_H

#include <QObject>
#include <QVariantMap>

class QProcess;
class SystemService;

class InferenceService : public QObject
{
    Q_OBJECT

public:
    explicit InferenceService(SystemService *systemService, QObject *parent = nullptr);

    QVariantMap state() const;
    bool action(const QString &action, const QVariantMap &payload = QVariantMap());

signals:
    void settingsChanged();

private:
    struct Settings {
        QString taskType = QStringLiteral("classification");
        QString pythonFilePath;
        QString dockerImage;
        QString dockerHostMountPath;
        QString dockerContainerWorkDir = QStringLiteral("/workspace");
        QString bestModelPath;
        bool useTrain = true;
        bool useVal = true;
        bool useTest = true;
        bool uiSettingsExpanded = true;
    };

    bool setSetting(const QString &key, const QVariant &value);
    bool startInference(const QVariantMap &payload);
    bool stopInference();
    void appendLogLine(const QString &line);
    void loadFromConfig();
    QString readBestModelFromProject(const QString &projectFilePath) const;

    SystemService *m_systemService = nullptr;
    QProcess *m_process = nullptr;
    Settings m_settings;
    bool m_running = false;
    int m_progress = 0;
    QString m_status = QStringLiteral("Idle");
    QStringList m_logLines;
    QString m_lastProjectFilePath;
};

#endif // INFERENCESERVICE_H
