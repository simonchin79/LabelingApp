#ifndef TRAINSERVICE_H
#define TRAINSERVICE_H

#include <QObject>
#include <QHash>
#include <QVariant>
#include <QVariantMap>

class SystemService;
class QProcess;

class TrainService : public QObject
{
    Q_OBJECT

public:
    explicit TrainService(SystemService *systemService, QObject *parent = nullptr);

    QVariantMap state() const;
    bool action(const QString &action, const QVariantMap &payload = QVariantMap());
    bool setSetting(const QString &key, const QVariant &value);
    bool setMany(const QVariantMap &settings);

signals:
    void settingsChanged();

private:
    struct TaskSettings {
        QString pythonFilePath;
        QString dockerImage;
        QString dockerHostMountPath;
        QString dockerContainerWorkDir = QStringLiteral("/workspace");
        QString predictionJsonPath;
        QString bestModelPath;
        int epochs = 40;
        int batchSize = 64;
        int numWorkers = 8;
        int inputSize = 224;
        double learningRate = 1e-3;
        double weightDecay = 1e-4;
        QString backbone = QStringLiteral("efficientnet_b0");
    };

    static QStringList supportedTaskTypes();
    TaskSettings taskSettingsFor(const QString &taskType) const;
    TaskSettings &editableTaskSettingsFor(const QString &taskType);
    QString activeTaskType() const;
    QVariantMap taskSettingsToVariantMap(const TaskSettings &settings) const;
    QString taskConfigKey(const QString &taskType, const QString &field) const;

    void loadFromConfig();
    void appendLogLine(const QString &line);
    void parseStatsFromLine(const QString &line);
    void parseProgressFromLine(const QString &line);
    void loadResultSummaryFromProjectConfig();
    bool startTraining(const QVariantMap &payload);
    bool stopTraining();

    SystemService *m_systemService = nullptr;
    QProcess *m_process = nullptr;
    QHash<QString, TaskSettings> m_taskSettings;
    QString m_selectedTaskType = QStringLiteral("classification");
    int m_splitTrainCount = 0;
    int m_splitValCount = 0;
    int m_splitTestCount = 0;
    int m_splitTrainPercent = 60;
    int m_splitValPercent = 30;
    int m_splitTestPercent = 10;
    int m_splitSeed = 42;
    bool m_uiPathsExpanded = true;
    bool m_uiHyperparamsExpanded = true;
    bool m_uiSplitExpanded = true;
    bool m_running = false;
    int m_progress = 0;
    QString m_status = QStringLiteral("Idle");
    QVariantMap m_stats;
    QVariantList m_confusionMatrix;
    QStringList m_confusionClassNames;
    QStringList m_logLines;
    QString m_lastProjectFilePath;
};

#endif // TRAINSERVICE_H
