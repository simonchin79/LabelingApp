#ifndef TRAINSERVICE_H
#define TRAINSERVICE_H

#include <QObject>
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
    void loadFromConfig();
    void appendLogLine(const QString &line);
    void parseProgressFromLine(const QString &line);
    bool startTraining(const QVariantMap &payload);
    bool stopTraining();

    SystemService *m_systemService = nullptr;
    QProcess *m_process = nullptr;
    QString m_pythonFilePath;
    QString m_dockerImage;
    QString m_dockerHostMountPath;
    QString m_dockerContainerWorkDir = QStringLiteral("/workspace");
    QString m_predictionJsonPath;
    QString m_bestModelPath;
    int m_epochs = 40;
    int m_batchSize = 64;
    int m_numWorkers = 8;
    int m_inputSize = 224;
    double m_learningRate = 1e-3;
    double m_weightDecay = 1e-4;
    QString m_backbone = QStringLiteral("efficientnet_b0");
    int m_splitTrainCount = 0;
    int m_splitValCount = 0;
    int m_splitTestCount = 0;
    int m_splitTrainPercent = 60;
    int m_splitValPercent = 30;
    int m_splitTestPercent = 10;
    bool m_uiPathsExpanded = true;
    bool m_running = false;
    int m_progress = 0;
    QString m_status = QStringLiteral("Idle");
    QStringList m_logLines;
};

#endif // TRAINSERVICE_H
