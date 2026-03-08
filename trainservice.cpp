#include "trainservice.h"

#include "systemservice.h"

#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QDebug>
#include <QRegularExpression>
#include <QStringList>

namespace {
const QString kTrainingSelectedTaskTypeKey = QStringLiteral("training/selectedTaskType");
const QString kTaskFieldPythonFilePath = QStringLiteral("pythonFilePath");
const QString kTaskFieldDockerImage = QStringLiteral("dockerImage");
const QString kTaskFieldDockerHostMountPath = QStringLiteral("dockerHostMountPath");
const QString kTaskFieldDockerContainerWorkDir = QStringLiteral("dockerContainerWorkDir");
const QString kTaskFieldPredictionJsonPath = QStringLiteral("predictionJsonPath");
const QString kTaskFieldBestModelPath = QStringLiteral("bestModelPath");
const QString kTaskFieldEpochs = QStringLiteral("epochs");
const QString kTaskFieldBatchSize = QStringLiteral("batchSize");
const QString kTaskFieldNumWorkers = QStringLiteral("numWorkers");
const QString kTaskFieldInputSize = QStringLiteral("inputSize");
const QString kTaskFieldLearningRate = QStringLiteral("learningRate");
const QString kTaskFieldWeightDecay = QStringLiteral("weightDecay");
const QString kTaskFieldBackbone = QStringLiteral("backbone");

const QString kLegacyTrainingPythonFilePathKey = QStringLiteral("training/pythonFilePath");
const QString kLegacyTrainingDockerImageKey = QStringLiteral("training/dockerImage");
const QString kLegacyTrainingDockerHostMountPathKey = QStringLiteral("training/dockerHostMountPath");
const QString kLegacyTrainingDockerContainerWorkDirKey = QStringLiteral("training/dockerContainerWorkDir");
const QString kLegacyTrainingPredictionJsonPathKey = QStringLiteral("training/predictionJsonPath");
const QString kLegacyTrainingBestModelPathKey = QStringLiteral("training/bestModelPath");
const QString kLegacyTrainingEpochsKey = QStringLiteral("training/epochs");
const QString kLegacyTrainingBatchSizeKey = QStringLiteral("training/batchSize");
const QString kLegacyTrainingNumWorkersKey = QStringLiteral("training/numWorkers");
const QString kLegacyTrainingInputSizeKey = QStringLiteral("training/inputSize");
const QString kLegacyTrainingLearningRateKey = QStringLiteral("training/learningRate");
const QString kLegacyTrainingWeightDecayKey = QStringLiteral("training/weightDecay");
const QString kLegacyTrainingBackboneKey = QStringLiteral("training/backbone");
const QString kTrainingSplitTrainCountKey = QStringLiteral("training/splitTrainCount");
const QString kTrainingSplitValCountKey = QStringLiteral("training/splitValCount");
const QString kTrainingSplitTestCountKey = QStringLiteral("training/splitTestCount");
const QString kTrainingSplitTrainPercentKey = QStringLiteral("training/splitTrainPercent");
const QString kTrainingSplitValPercentKey = QStringLiteral("training/splitValPercent");
const QString kTrainingSplitTestPercentKey = QStringLiteral("training/splitTestPercent");
const QString kTrainingSplitSeedKey = QStringLiteral("training/splitSeed");
const QString kTrainingUiPathsExpandedKey = QStringLiteral("training/uiPathsExpanded");
const QString kTrainingUiHyperparamsExpandedKey = QStringLiteral("training/uiHyperparamsExpanded");
const QString kTrainingUiSplitExpandedKey = QStringLiteral("training/uiSplitExpanded");
}

TrainService::TrainService(SystemService *systemService, QObject *parent)
    : QObject(parent)
    , m_systemService(systemService)
{
    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        const QString text = QString::fromLocal8Bit(m_process->readAllStandardOutput());
        const QStringList lines = text.split('\n', Qt::SkipEmptyParts);
        for (const QString &rawLine : lines) {
            const QString line = rawLine.trimmed();
            if (line.isEmpty()) {
                continue;
            }
            parseStatsFromLine(line);
            appendLogLine(line);
            parseProgressFromLine(line);
        }
        emit settingsChanged();
    });
    connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
        const QString text = QString::fromLocal8Bit(m_process->readAllStandardError());
        const QStringList lines = text.split('\n', Qt::SkipEmptyParts);
        for (const QString &rawLine : lines) {
            const QString line = rawLine.trimmed();
            if (line.isEmpty()) {
                continue;
            }
            appendLogLine(QStringLiteral("[ERR] %1").arg(line));
        }
        emit settingsChanged();
    });
    connect(m_process,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                qDebug() << "[TrainService] process finished. exitCode=" << exitCode
                         << "exitStatus=" << exitStatus;
                m_running = false;
                m_progress = (exitCode == 0 && exitStatus == QProcess::NormalExit) ? 100 : m_progress;
                m_status = (exitCode == 0 && exitStatus == QProcess::NormalExit)
                               ? QStringLiteral("Training complete")
                               : QStringLiteral("Training failed (code=%1)").arg(exitCode);
                if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
                    loadResultSummaryFromProjectConfig();
                }
                appendLogLine(m_status);
                emit settingsChanged();
            });
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError err) {
        qDebug() << "[TrainService] process error:" << err << m_process->errorString();
    });
    loadFromConfig();
}

QStringList TrainService::supportedTaskTypes()
{
    return {QStringLiteral("classification"),
            QStringLiteral("segmentation"),
            QStringLiteral("detection"),
            QStringLiteral("ocr")};
}

TrainService::TaskSettings TrainService::taskSettingsFor(const QString &taskType) const
{
    const QString normalized = taskType.trimmed().toLower();
    if (m_taskSettings.contains(normalized)) {
        return m_taskSettings.value(normalized);
    }
    return TaskSettings();
}

TrainService::TaskSettings &TrainService::editableTaskSettingsFor(const QString &taskType)
{
    const QString normalized = taskType.trimmed().toLower();
    return m_taskSettings[normalized.isEmpty() ? QStringLiteral("classification") : normalized];
}

QString TrainService::activeTaskType() const
{
    const QString normalized = m_selectedTaskType.trimmed().toLower();
    if (supportedTaskTypes().contains(normalized)) {
        return normalized;
    }
    return QStringLiteral("classification");
}

QVariantMap TrainService::taskSettingsToVariantMap(const TaskSettings &settings) const
{
    QVariantMap data;
    data.insert(kTaskFieldPythonFilePath, settings.pythonFilePath);
    data.insert(kTaskFieldDockerImage, settings.dockerImage);
    data.insert(kTaskFieldDockerHostMountPath, settings.dockerHostMountPath);
    data.insert(kTaskFieldDockerContainerWorkDir, settings.dockerContainerWorkDir);
    data.insert(kTaskFieldPredictionJsonPath, settings.predictionJsonPath);
    data.insert(kTaskFieldBestModelPath, settings.bestModelPath);
    data.insert(kTaskFieldEpochs, settings.epochs);
    data.insert(kTaskFieldBatchSize, settings.batchSize);
    data.insert(kTaskFieldNumWorkers, settings.numWorkers);
    data.insert(kTaskFieldInputSize, settings.inputSize);
    data.insert(kTaskFieldLearningRate, settings.learningRate);
    data.insert(kTaskFieldWeightDecay, settings.weightDecay);
    data.insert(kTaskFieldBackbone, settings.backbone);
    return data;
}

QString TrainService::taskConfigKey(const QString &taskType, const QString &field) const
{
    return QStringLiteral("training/tasks/%1/%2").arg(taskType, field);
}

QVariantMap TrainService::state() const
{
    const QString taskType = activeTaskType();
    const TaskSettings task = taskSettingsFor(taskType);
    QVariantMap data;
    data.insert(QStringLiteral("taskType"), taskType);
    data.insert(QStringLiteral("taskTypes"), supportedTaskTypes());
    data.insert(QStringLiteral("taskSettings"), taskSettingsToVariantMap(task));
    QVariantMap allTaskSettings;
    for (const QString &supported : supportedTaskTypes()) {
        allTaskSettings.insert(supported, taskSettingsToVariantMap(taskSettingsFor(supported)));
    }
    data.insert(QStringLiteral("allTaskSettings"), allTaskSettings);

    // Flat keys retained for existing QML bindings.
    data.insert(QStringLiteral("pythonFilePath"), task.pythonFilePath);
    data.insert(QStringLiteral("dockerImage"), task.dockerImage);
    data.insert(QStringLiteral("dockerHostMountPath"), task.dockerHostMountPath);
    data.insert(QStringLiteral("dockerContainerWorkDir"), task.dockerContainerWorkDir);
    data.insert(QStringLiteral("predictionJsonPath"), task.predictionJsonPath);
    data.insert(QStringLiteral("bestModelPath"), task.bestModelPath);
    data.insert(QStringLiteral("epochs"), task.epochs);
    data.insert(QStringLiteral("batchSize"), task.batchSize);
    data.insert(QStringLiteral("numWorkers"), task.numWorkers);
    data.insert(QStringLiteral("inputSize"), task.inputSize);
    data.insert(QStringLiteral("learningRate"), task.learningRate);
    data.insert(QStringLiteral("weightDecay"), task.weightDecay);
    data.insert(QStringLiteral("backbone"), task.backbone);
    data.insert(QStringLiteral("splitTrainCount"), m_splitTrainCount);
    data.insert(QStringLiteral("splitValCount"), m_splitValCount);
    data.insert(QStringLiteral("splitTestCount"), m_splitTestCount);
    data.insert(QStringLiteral("splitTrainPercent"), m_splitTrainPercent);
    data.insert(QStringLiteral("splitValPercent"), m_splitValPercent);
    data.insert(QStringLiteral("splitTestPercent"), m_splitTestPercent);
    data.insert(QStringLiteral("splitSeed"), m_splitSeed);
    data.insert(QStringLiteral("pathsExpanded"), m_uiPathsExpanded);
    data.insert(QStringLiteral("hyperparamsExpanded"), m_uiHyperparamsExpanded);
    data.insert(QStringLiteral("splitExpanded"), m_uiSplitExpanded);
    data.insert(QStringLiteral("running"), m_running);
    data.insert(QStringLiteral("progress"), m_progress);
    data.insert(QStringLiteral("status"), m_status);
    data.insert(QStringLiteral("stats"), m_stats);
    data.insert(QStringLiteral("confusionMatrix"), m_confusionMatrix);
    data.insert(QStringLiteral("confusionClassNames"), m_confusionClassNames);
    data.insert(QStringLiteral("logText"), m_logLines.join('\n'));
    return data;
}

bool TrainService::action(const QString &action, const QVariantMap &payload)
{
    const QString task = action.trimmed().toLower();
    qDebug() << "[TrainService] action:" << task << "payload keys:" << payload.keys();
    if (task == QStringLiteral("set_setting")) {
        return setSetting(payload.value(QStringLiteral("setting")).toString(),
                          payload.value(QStringLiteral("value")));
    }
    if (task == QStringLiteral("set_many")) {
        return setMany(payload.value(QStringLiteral("settings")).toMap());
    }
    if (task == QStringLiteral("split_dataset")) {
        const int total = qMax(0, payload.value(QStringLiteral("totalImages")).toInt());
        const int pTrain = qMax(0, m_splitTrainPercent);
        const int pVal = qMax(0, m_splitValPercent);
        const int pTest = qMax(0, m_splitTestPercent);
        const int pSum = pTrain + pVal + pTest;
        if (pSum <= 0) {
            m_status = QStringLiteral("Split percentages must be greater than 0");
            emit settingsChanged();
            return false;
        }

        if (total <= 0) {
            m_splitTrainCount = 0;
            m_splitValCount = 0;
            m_splitTestCount = 0;
        } else {
            m_splitTrainCount = (total * pTrain) / pSum;
            m_splitValCount = (total * pVal) / pSum;
            m_splitTestCount = qMax(0, total - m_splitTrainCount - m_splitValCount);
        }
        m_systemService->saveAppValue(kTrainingSplitTrainCountKey, m_splitTrainCount);
        m_systemService->saveAppValue(kTrainingSplitValCountKey, m_splitValCount);
        m_systemService->saveAppValue(kTrainingSplitTestCountKey, m_splitTestCount);

        m_status = QStringLiteral("Split set: train=%1 val=%2 test=%3")
                       .arg(m_splitTrainCount)
                       .arg(m_splitValCount)
                       .arg(m_splitTestCount);
        emit settingsChanged();
        return true;
    }
    if (task == QStringLiteral("start_train")) {
        return startTraining(payload);
    }
    if (task == QStringLiteral("stop_train")) {
        return stopTraining();
    }
    return false;
}

bool TrainService::setSetting(const QString &key, const QVariant &value)
{
    if (!m_systemService) {
        return false;
    }

    const QString setting = key.trimmed();
    if (setting.isEmpty()) {
        return false;
    }
    qDebug() << "[TrainService] setSetting:" << setting << "value:" << value;

    if (setting == QStringLiteral("taskType")) {
        const QString v = value.toString().trimmed().toLower();
        if (!supportedTaskTypes().contains(v)) {
            return false;
        }
        if (m_selectedTaskType != v) {
            m_selectedTaskType = v;
            m_systemService->saveAppValue(kTrainingSelectedTaskTypeKey, m_selectedTaskType);
            emit settingsChanged();
        }
        return true;
    }

    bool changed = false;
    TaskSettings &task = editableTaskSettingsFor(activeTaskType());
    if (setting == kTaskFieldPythonFilePath) {
        const QString v = SystemService::normalizePath(value);
        if (task.pythonFilePath != v) {
            task.pythonFilePath = v;
            m_systemService->saveAppValue(taskConfigKey(activeTaskType(), setting), task.pythonFilePath);
            changed = true;
        }
    } else if (setting == kTaskFieldDockerImage) {
        const QString v = value.toString().trimmed();
        if (task.dockerImage != v) {
            task.dockerImage = v;
            m_systemService->saveAppValue(taskConfigKey(activeTaskType(), setting), task.dockerImage);
            changed = true;
        }
    } else if (setting == kTaskFieldDockerHostMountPath) {
        const QString v = SystemService::normalizePath(value);
        if (task.dockerHostMountPath != v) {
            task.dockerHostMountPath = v;
            m_systemService->saveAppValue(taskConfigKey(activeTaskType(), setting), task.dockerHostMountPath);
            changed = true;
        }
    } else if (setting == kTaskFieldDockerContainerWorkDir) {
        const QString v = value.toString().trimmed();
        const QString normalized = v.isEmpty() ? QStringLiteral("/workspace") : v;
        if (task.dockerContainerWorkDir != normalized) {
            task.dockerContainerWorkDir = normalized;
            m_systemService->saveAppValue(taskConfigKey(activeTaskType(), setting), task.dockerContainerWorkDir);
            changed = true;
        }
    } else if (setting == kTaskFieldPredictionJsonPath) {
        const QString v = SystemService::normalizePath(value);
        if (task.predictionJsonPath != v) {
            task.predictionJsonPath = v;
            m_systemService->saveAppValue(taskConfigKey(activeTaskType(), setting), task.predictionJsonPath);
            changed = true;
        }
    } else if (setting == kTaskFieldBestModelPath) {
        const QString v = SystemService::normalizePath(value);
        if (task.bestModelPath != v) {
            task.bestModelPath = v;
            m_systemService->saveAppValue(taskConfigKey(activeTaskType(), setting), task.bestModelPath);
            changed = true;
        }
    } else if (setting == kTaskFieldEpochs) {
        const int v = qMax(1, value.toInt());
        if (task.epochs != v) {
            task.epochs = v;
            m_systemService->saveAppValue(taskConfigKey(activeTaskType(), setting), task.epochs);
            changed = true;
        }
    } else if (setting == kTaskFieldBatchSize) {
        const int v = qMax(1, value.toInt());
        if (task.batchSize != v) {
            task.batchSize = v;
            m_systemService->saveAppValue(taskConfigKey(activeTaskType(), setting), task.batchSize);
            changed = true;
        }
    } else if (setting == kTaskFieldNumWorkers) {
        const int v = qMax(0, value.toInt());
        if (task.numWorkers != v) {
            task.numWorkers = v;
            m_systemService->saveAppValue(taskConfigKey(activeTaskType(), setting), task.numWorkers);
            changed = true;
        }
    } else if (setting == kTaskFieldInputSize) {
        const int v = qMax(32, value.toInt());
        if (task.inputSize != v) {
            task.inputSize = v;
            m_systemService->saveAppValue(taskConfigKey(activeTaskType(), setting), task.inputSize);
            changed = true;
        }
    } else if (setting == kTaskFieldLearningRate) {
        const double v = qMax(1e-7, value.toDouble());
        if (!qFuzzyCompare(task.learningRate, v)) {
            task.learningRate = v;
            m_systemService->saveAppValue(taskConfigKey(activeTaskType(), setting), task.learningRate);
            changed = true;
        }
    } else if (setting == kTaskFieldWeightDecay) {
        const double v = qMax(0.0, value.toDouble());
        if (!qFuzzyCompare(task.weightDecay, v)) {
            task.weightDecay = v;
            m_systemService->saveAppValue(taskConfigKey(activeTaskType(), setting), task.weightDecay);
            changed = true;
        }
    } else if (setting == kTaskFieldBackbone) {
        const QString v = value.toString().trimmed();
        if (!v.isEmpty() && task.backbone != v) {
            task.backbone = v;
            m_systemService->saveAppValue(taskConfigKey(activeTaskType(), setting), task.backbone);
            changed = true;
        }
    } else if (setting == QStringLiteral("splitTrainCount")) {
        const int v = qMax(0, value.toInt());
        if (m_splitTrainCount != v) {
            m_splitTrainCount = v;
            m_systemService->saveAppValue(kTrainingSplitTrainCountKey, m_splitTrainCount);
            changed = true;
        }
    } else if (setting == QStringLiteral("splitValCount")) {
        const int v = qMax(0, value.toInt());
        if (m_splitValCount != v) {
            m_splitValCount = v;
            m_systemService->saveAppValue(kTrainingSplitValCountKey, m_splitValCount);
            changed = true;
        }
    } else if (setting == QStringLiteral("splitTestCount")) {
        const int v = qMax(0, value.toInt());
        if (m_splitTestCount != v) {
            m_splitTestCount = v;
            m_systemService->saveAppValue(kTrainingSplitTestCountKey, m_splitTestCount);
            changed = true;
        }
    } else if (setting == QStringLiteral("splitTrainPercent")) {
        const int v = qMax(0, value.toInt());
        if (m_splitTrainPercent != v) {
            m_splitTrainPercent = v;
            m_systemService->saveAppValue(kTrainingSplitTrainPercentKey, m_splitTrainPercent);
            changed = true;
        }
    } else if (setting == QStringLiteral("splitValPercent")) {
        const int v = qMax(0, value.toInt());
        if (m_splitValPercent != v) {
            m_splitValPercent = v;
            m_systemService->saveAppValue(kTrainingSplitValPercentKey, m_splitValPercent);
            changed = true;
        }
    } else if (setting == QStringLiteral("splitTestPercent")) {
        const int v = qMax(0, value.toInt());
        if (m_splitTestPercent != v) {
            m_splitTestPercent = v;
            m_systemService->saveAppValue(kTrainingSplitTestPercentKey, m_splitTestPercent);
            changed = true;
        }
    } else if (setting == QStringLiteral("splitSeed")) {
        const int v = value.toInt();
        if (m_splitSeed != v) {
            m_splitSeed = v;
            m_systemService->saveAppValue(kTrainingSplitSeedKey, m_splitSeed);
            changed = true;
        }
    } else if (setting == QStringLiteral("uiPathsExpanded")) {
        const bool v = value.toBool();
        if (m_uiPathsExpanded != v) {
            m_uiPathsExpanded = v;
            m_systemService->saveAppValue(kTrainingUiPathsExpandedKey, m_uiPathsExpanded);
            changed = true;
        }
    } else if (setting == QStringLiteral("uiHyperparamsExpanded")) {
        const bool v = value.toBool();
        if (m_uiHyperparamsExpanded != v) {
            m_uiHyperparamsExpanded = v;
            m_systemService->saveAppValue(kTrainingUiHyperparamsExpandedKey, m_uiHyperparamsExpanded);
            changed = true;
        }
    } else if (setting == QStringLiteral("uiSplitExpanded")) {
        const bool v = value.toBool();
        if (m_uiSplitExpanded != v) {
            m_uiSplitExpanded = v;
            m_systemService->saveAppValue(kTrainingUiSplitExpandedKey, m_uiSplitExpanded);
            changed = true;
        }
    } else {
        return false;
    }

    if (changed) {
        emit settingsChanged();
    }
    return true;
}

bool TrainService::setMany(const QVariantMap &settings)
{
    bool ok = true;
    for (auto it = settings.constBegin(); it != settings.constEnd(); ++it) {
        ok = setSetting(it.key(), it.value()) && ok;
    }
    return ok;
}

void TrainService::loadFromConfig()
{
    if (!m_systemService) {
        return;
    }

    m_selectedTaskType =
        m_systemService->loadAppValue(kTrainingSelectedTaskTypeKey, QStringLiteral("classification")).toString().trimmed().toLower();
    if (!supportedTaskTypes().contains(m_selectedTaskType)) {
        m_selectedTaskType = QStringLiteral("classification");
    }

    for (const QString &taskType : supportedTaskTypes()) {
        TaskSettings settings;
        settings.pythonFilePath = m_systemService->loadAppValue(taskConfigKey(taskType, kTaskFieldPythonFilePath)).toString();
        settings.dockerImage = m_systemService->loadAppValue(taskConfigKey(taskType, kTaskFieldDockerImage)).toString();
        settings.dockerHostMountPath =
            m_systemService->loadAppValue(taskConfigKey(taskType, kTaskFieldDockerHostMountPath)).toString();
        settings.dockerContainerWorkDir =
            m_systemService->loadAppValue(taskConfigKey(taskType, kTaskFieldDockerContainerWorkDir), QStringLiteral("/workspace")).toString();
        if (settings.dockerContainerWorkDir.trimmed().isEmpty()) {
            settings.dockerContainerWorkDir = QStringLiteral("/workspace");
        }
        settings.predictionJsonPath =
            m_systemService->loadAppValue(taskConfigKey(taskType, kTaskFieldPredictionJsonPath)).toString();
        settings.bestModelPath = m_systemService->loadAppValue(taskConfigKey(taskType, kTaskFieldBestModelPath)).toString();
        settings.epochs = qMax(1, m_systemService->loadAppValue(taskConfigKey(taskType, kTaskFieldEpochs), 40).toInt());
        settings.batchSize = qMax(1, m_systemService->loadAppValue(taskConfigKey(taskType, kTaskFieldBatchSize), 64).toInt());
        settings.numWorkers =
            qMax(0, m_systemService->loadAppValue(taskConfigKey(taskType, kTaskFieldNumWorkers), 8).toInt());
        settings.inputSize = qMax(32, m_systemService->loadAppValue(taskConfigKey(taskType, kTaskFieldInputSize), 224).toInt());
        settings.learningRate =
            qMax(1e-7, m_systemService->loadAppValue(taskConfigKey(taskType, kTaskFieldLearningRate), 1e-3).toDouble());
        settings.weightDecay =
            qMax(0.0, m_systemService->loadAppValue(taskConfigKey(taskType, kTaskFieldWeightDecay), 1e-4).toDouble());
        settings.backbone =
            m_systemService->loadAppValue(taskConfigKey(taskType, kTaskFieldBackbone), QStringLiteral("efficientnet_b0")).toString();
        m_taskSettings.insert(taskType, settings);
    }

    // Backward compatibility: migrate old single-task settings into classification once.
    TaskSettings &cls = editableTaskSettingsFor(QStringLiteral("classification"));
    if (!m_systemService->loadAppValue(taskConfigKey(QStringLiteral("classification"), kTaskFieldPythonFilePath)).isValid()) {
        cls.pythonFilePath = m_systemService->loadAppValue(kLegacyTrainingPythonFilePathKey).toString();
    }
    if (!m_systemService->loadAppValue(taskConfigKey(QStringLiteral("classification"), kTaskFieldDockerImage)).isValid()) {
        cls.dockerImage = m_systemService->loadAppValue(kLegacyTrainingDockerImageKey).toString();
    }
    if (!m_systemService->loadAppValue(taskConfigKey(QStringLiteral("classification"), kTaskFieldDockerHostMountPath)).isValid()) {
        cls.dockerHostMountPath = m_systemService->loadAppValue(kLegacyTrainingDockerHostMountPathKey).toString();
    }
    if (!m_systemService->loadAppValue(taskConfigKey(QStringLiteral("classification"), kTaskFieldDockerContainerWorkDir)).isValid()) {
        const QString legacyWd =
            m_systemService->loadAppValue(kLegacyTrainingDockerContainerWorkDirKey, QStringLiteral("/workspace")).toString();
        if (!legacyWd.trimmed().isEmpty()) {
            cls.dockerContainerWorkDir = legacyWd;
        }
    }
    if (!m_systemService->loadAppValue(taskConfigKey(QStringLiteral("classification"), kTaskFieldPredictionJsonPath)).isValid()) {
        cls.predictionJsonPath = m_systemService->loadAppValue(kLegacyTrainingPredictionJsonPathKey).toString();
    }
    if (!m_systemService->loadAppValue(taskConfigKey(QStringLiteral("classification"), kTaskFieldBestModelPath)).isValid()) {
        cls.bestModelPath = m_systemService->loadAppValue(kLegacyTrainingBestModelPathKey).toString();
    }
    if (!m_systemService->loadAppValue(taskConfigKey(QStringLiteral("classification"), kTaskFieldEpochs)).isValid()) {
        cls.epochs = qMax(1, m_systemService->loadAppValue(kLegacyTrainingEpochsKey, cls.epochs).toInt());
    }
    if (!m_systemService->loadAppValue(taskConfigKey(QStringLiteral("classification"), kTaskFieldBatchSize)).isValid()) {
        cls.batchSize = qMax(1, m_systemService->loadAppValue(kLegacyTrainingBatchSizeKey, cls.batchSize).toInt());
    }
    if (!m_systemService->loadAppValue(taskConfigKey(QStringLiteral("classification"), kTaskFieldNumWorkers)).isValid()) {
        cls.numWorkers = qMax(0, m_systemService->loadAppValue(kLegacyTrainingNumWorkersKey, cls.numWorkers).toInt());
    }
    if (!m_systemService->loadAppValue(taskConfigKey(QStringLiteral("classification"), kTaskFieldInputSize)).isValid()) {
        cls.inputSize = qMax(32, m_systemService->loadAppValue(kLegacyTrainingInputSizeKey, cls.inputSize).toInt());
    }
    if (!m_systemService->loadAppValue(taskConfigKey(QStringLiteral("classification"), kTaskFieldLearningRate)).isValid()) {
        cls.learningRate = qMax(1e-7, m_systemService->loadAppValue(kLegacyTrainingLearningRateKey, cls.learningRate).toDouble());
    }
    if (!m_systemService->loadAppValue(taskConfigKey(QStringLiteral("classification"), kTaskFieldWeightDecay)).isValid()) {
        cls.weightDecay = qMax(0.0, m_systemService->loadAppValue(kLegacyTrainingWeightDecayKey, cls.weightDecay).toDouble());
    }
    if (!m_systemService->loadAppValue(taskConfigKey(QStringLiteral("classification"), kTaskFieldBackbone)).isValid()) {
        const QString legacyBackbone =
            m_systemService->loadAppValue(kLegacyTrainingBackboneKey, cls.backbone).toString().trimmed();
        if (!legacyBackbone.isEmpty()) {
            cls.backbone = legacyBackbone;
        }
    }

    m_splitTrainCount = qMax(0, m_systemService->loadAppValue(kTrainingSplitTrainCountKey, 0).toInt());
    m_splitValCount = qMax(0, m_systemService->loadAppValue(kTrainingSplitValCountKey, 0).toInt());
    m_splitTestCount = qMax(0, m_systemService->loadAppValue(kTrainingSplitTestCountKey, 0).toInt());
    m_splitTrainPercent = qMax(0, m_systemService->loadAppValue(kTrainingSplitTrainPercentKey, 60).toInt());
    m_splitValPercent = qMax(0, m_systemService->loadAppValue(kTrainingSplitValPercentKey, 30).toInt());
    m_splitTestPercent = qMax(0, m_systemService->loadAppValue(kTrainingSplitTestPercentKey, 10).toInt());
    m_splitSeed = m_systemService->loadAppValue(kTrainingSplitSeedKey, 42).toInt();
    m_uiPathsExpanded = m_systemService->loadAppValue(kTrainingUiPathsExpandedKey, true).toBool();
    m_uiHyperparamsExpanded = m_systemService->loadAppValue(kTrainingUiHyperparamsExpandedKey, true).toBool();
    m_uiSplitExpanded = m_systemService->loadAppValue(kTrainingUiSplitExpandedKey, true).toBool();
}

void TrainService::appendLogLine(const QString &line)
{
    m_logLines.push_back(line);
    while (m_logLines.size() > 400) {
        m_logLines.removeFirst();
    }
}

void TrainService::parseStatsFromLine(const QString &line)
{
    if (!line.startsWith(QStringLiteral("STAT|"), Qt::CaseInsensitive)) {
        return;
    }
    const QString payload = line.mid(5);
    const QStringList parts = payload.split('|', Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        return;
    }

    QVariantMap updated = m_stats;
    for (const QString &part : parts) {
        const int eq = part.indexOf('=');
        if (eq <= 0) {
            continue;
        }
        const QString key = part.left(eq).trimmed();
        const QString valueRaw = part.mid(eq + 1).trimmed();
        if (key.isEmpty()) {
            continue;
        }

        bool okInt = false;
        const int asInt = valueRaw.toInt(&okInt);
        if (okInt) {
            updated.insert(key, asInt);
            continue;
        }

        bool okDouble = false;
        const double asDouble = valueRaw.toDouble(&okDouble);
        if (okDouble) {
            updated.insert(key, asDouble);
            continue;
        }
        updated.insert(key, valueRaw);
    }

    m_stats = updated;

    const int epoch = m_stats.value(QStringLiteral("epoch")).toInt();
    const int total = m_stats.value(QStringLiteral("total_epochs")).toInt();
    if (epoch > 0 && total > 0) {
        m_progress = qBound(0, (epoch * 100) / total, 100);
    }
}

void TrainService::parseProgressFromLine(const QString &line)
{
    static const QRegularExpression re(QStringLiteral("Epoch\\s*\\[(\\d+)\\s*/\\s*(\\d+)\\]"),
                                       QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = re.match(line);
    if (!match.hasMatch()) {
        return;
    }

    const int current = match.captured(1).toInt();
    const int total = match.captured(2).toInt();
    if (total <= 0) {
        return;
    }
    m_progress = qBound(0, (current * 100) / total, 100);
    m_status = QStringLiteral("Training epoch %1/%2").arg(current).arg(total);
}

bool TrainService::startTraining(const QVariantMap &payload)
{
    if (m_running) {
        return false;
    }

    const QString taskType = activeTaskType();
    const TaskSettings task = taskSettingsFor(taskType);
    const QString rawScriptPath = task.pythonFilePath.trimmed();
    if (rawScriptPath.isEmpty()) {
        m_status = QStringLiteral("Python script path is empty");
        qDebug() << "[TrainService] startTraining failed:" << m_status;
        emit settingsChanged();
        return false;
    }

    QString resolvedScriptPath = rawScriptPath;
    QString hostMountForResolve = task.dockerHostMountPath.trimmed();
    if (hostMountForResolve.isEmpty()) {
        hostMountForResolve = QDir::currentPath();
    }
    if (QDir::isRelativePath(resolvedScriptPath)) {
        resolvedScriptPath = QDir(hostMountForResolve).absoluteFilePath(resolvedScriptPath);
    }

    const QFileInfo scriptInfo(resolvedScriptPath);
    if (!scriptInfo.exists() || !scriptInfo.isFile()) {
        m_status = QStringLiteral("Python script not found: %1").arg(resolvedScriptPath);
        qDebug() << "[TrainService] startTraining failed:" << m_status;
        emit settingsChanged();
        return false;
    }

    QStringList args;
    QString program;
    QString workingDir = scriptInfo.absoluteDir().absolutePath();
    const QString projectFilePath = SystemService::normalizePath(payload.value(QStringLiteral("projectFilePath")));
    m_lastProjectFilePath = projectFilePath;

    if (!task.dockerImage.trimmed().isEmpty()) {
        program = payload.value(QStringLiteral("dockerExe")).toString().trimmed();
        if (program.isEmpty()) {
            program = QStringLiteral("docker");
        }

        QString hostMount = task.dockerHostMountPath.trimmed();
        if (hostMount.isEmpty()) {
            hostMount = scriptInfo.absoluteDir().absolutePath();
        }

        QString containerWorkDir = task.dockerContainerWorkDir.trimmed();
        if (containerWorkDir.isEmpty()) {
            containerWorkDir = QStringLiteral("/workspace");
        }

        auto hostToContainerPath = [&](const QString &hostPath) -> QString {
            const QString cleanHostPath = QDir::cleanPath(hostPath);
            const QString cleanHostMount = QDir::cleanPath(hostMount);
            if (cleanHostPath == cleanHostMount) {
                return containerWorkDir;
            }
            if (cleanHostPath.startsWith(cleanHostMount + QDir::separator())) {
                const QString rel = cleanHostPath.mid(cleanHostMount.size() + 1);
                return QDir(containerWorkDir).filePath(rel);
            }
            return cleanHostPath;
        };

        QString containerConfigPath;
        QString extraConfigHostDir;
        QString extraConfigContainerDir = QStringLiteral("/workspace_config");
        if (!projectFilePath.isEmpty()) {
            const QString absConfigPath = QFileInfo(projectFilePath).absoluteFilePath();
            const QString cleanConfigPath = QDir::cleanPath(absConfigPath);
            const QString cleanHostMount = QDir::cleanPath(hostMount);
            const bool underMainMount = (cleanConfigPath == cleanHostMount)
                || cleanConfigPath.startsWith(cleanHostMount + QDir::separator());
            if (underMainMount) {
                containerConfigPath = hostToContainerPath(cleanConfigPath);
            } else {
                QFileInfo cfi(cleanConfigPath);
                extraConfigHostDir = cfi.absolutePath();
                containerConfigPath = QDir(extraConfigContainerDir).filePath(cfi.fileName());
            }
        }

        QString relativeScript = QDir(hostMount).relativeFilePath(resolvedScriptPath);
        if (relativeScript.startsWith(QStringLiteral(".."))) {
            relativeScript = QFileInfo(resolvedScriptPath).fileName();
        }
        const QString containerScriptPath = QDir(containerWorkDir).filePath(relativeScript);

        args << QStringLiteral("run")
             << QStringLiteral("--rm")
             << QStringLiteral("--ipc=host")
             << QStringLiteral("-v") << QStringLiteral("%1:%2").arg(hostMount, containerWorkDir)
             << QStringLiteral("-w") << containerWorkDir
             ;
        if (!extraConfigHostDir.isEmpty()) {
            args << QStringLiteral("-v") << QStringLiteral("%1:%2").arg(extraConfigHostDir, extraConfigContainerDir);
        }
        args << task.dockerImage.trimmed()
             << QStringLiteral("python3")
             << containerScriptPath;

        if (!containerConfigPath.isEmpty()) {
            args << QStringLiteral("--config-json")
                 << containerConfigPath
                 << QStringLiteral("--host-mount-path")
                 << hostMount
                 << QStringLiteral("--container-workdir")
                 << containerWorkDir;
        }
    } else {
        program = payload.value(QStringLiteral("pythonExe")).toString().trimmed();
        if (program.isEmpty()) {
            program = QStringLiteral("python3");
        }
        args.push_back(resolvedScriptPath);
        if (!projectFilePath.isEmpty()) {
            args.push_back(QStringLiteral("--config-json"));
            args.push_back(projectFilePath);
        }
    }

    if (!task.backbone.trimmed().isEmpty()) {
        args.push_back(QStringLiteral("--backbone"));
        args.push_back(task.backbone.trimmed());
    }
    args.push_back(QStringLiteral("--epochs"));
    args.push_back(QString::number(qMax(1, task.epochs)));
    args.push_back(QStringLiteral("--batch-size"));
    args.push_back(QString::number(qMax(1, task.batchSize)));
    args.push_back(QStringLiteral("--num-workers"));
    args.push_back(QString::number(qMax(0, task.numWorkers)));
    args.push_back(QStringLiteral("--input-size"));
    args.push_back(QString::number(qMax(32, task.inputSize)));
    args.push_back(QStringLiteral("--learning-rate"));
    args.push_back(QString::number(qMax(1e-7, task.learningRate), 'g', 16));
    args.push_back(QStringLiteral("--weight-decay"));
    args.push_back(QString::number(qMax(0.0, task.weightDecay), 'g', 16));

    m_logLines.clear();
    m_progress = 0;
    m_stats.clear();
    m_confusionMatrix.clear();
    m_confusionClassNames.clear();
    m_running = true;
    m_status = QStringLiteral("Training started");
    qDebug() << "[TrainService] startTraining program:" << program;
    qDebug() << "[TrainService] startTraining args:" << args;
    qDebug() << "[TrainService] startTraining workingDir:" << workingDir;
    qDebug() << "[TrainService] startTraining selectedTaskType:" << taskType;
    emit settingsChanged();

    m_process->setWorkingDirectory(workingDir);
    m_process->start(program, args);
    if (!m_process->waitForStarted(3000)) {
        m_running = false;
        m_status = QStringLiteral("Failed to start training process");
        appendLogLine(m_status);
        qDebug() << "[TrainService] startTraining failed to launch:" << m_process->errorString();
        emit settingsChanged();
        return false;
    }
    qDebug() << "[TrainService] training process started, pid:" << m_process->processId();
    return true;
}

void TrainService::loadResultSummaryFromProjectConfig()
{
    const QString projectFilePath = SystemService::normalizePath(m_lastProjectFilePath);
    if (projectFilePath.isEmpty()) {
        return;
    }
    QFile file(projectFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "[TrainService] loadResultSummaryFromProjectConfig: failed to open" << projectFilePath;
        return;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject()) {
        qDebug() << "[TrainService] loadResultSummaryFromProjectConfig: invalid json" << projectFilePath;
        return;
    }

    const QJsonObject root = doc.object();
    const QString bestModelPath = SystemService::normalizePath(root.value(QStringLiteral("bestModelPath")).toString());
    if (!bestModelPath.isEmpty()) {
        TaskSettings &task = editableTaskSettingsFor(activeTaskType());
        if (task.bestModelPath != bestModelPath) {
            task.bestModelPath = bestModelPath;
            if (m_systemService) {
                m_systemService->saveAppValue(taskConfigKey(activeTaskType(), kTaskFieldBestModelPath), task.bestModelPath);
            }
        }
    }

    QJsonArray matrixJson;
    QJsonArray classNamesJson;

    const QJsonObject modelResults = root.value(QStringLiteral("modelResults")).toObject();
    const QJsonObject inference = modelResults.value(QStringLiteral("inference")).toObject();
    if (inference.contains(QStringLiteral("confusionMatrix"))) {
        matrixJson = inference.value(QStringLiteral("confusionMatrix")).toArray();
    } else {
        matrixJson = root.value(QStringLiteral("confusionMatrix")).toArray();
    }

    if (inference.contains(QStringLiteral("classNames"))) {
        classNamesJson = inference.value(QStringLiteral("classNames")).toArray();
    } else if (root.contains(QStringLiteral("classNames"))) {
        classNamesJson = root.value(QStringLiteral("classNames")).toArray();
    } else {
        classNamesJson = root.value(QStringLiteral("classes")).toArray();
    }

    QVariantList matrix;
    for (const QJsonValue &rowVal : matrixJson) {
        const QJsonArray rowArr = rowVal.toArray();
        QVariantList row;
        row.reserve(rowArr.size());
        for (const QJsonValue &v : rowArr) {
            row.push_back(v.toVariant());
        }
        matrix.push_back(row);
    }

    QStringList classNames;
    for (const QJsonValue &v : classNamesJson) {
        const QString name = v.toString().trimmed();
        if (!name.isEmpty()) {
            classNames.push_back(name);
        }
    }

    m_confusionMatrix = matrix;
    m_confusionClassNames = classNames;
    qDebug() << "[TrainService] loaded confusion matrix rows:" << m_confusionMatrix.size()
             << "class names:" << m_confusionClassNames;
}

bool TrainService::stopTraining()
{
    if (!m_running) {
        qDebug() << "[TrainService] stopTraining ignored: not running";
        return false;
    }
    qDebug() << "[TrainService] stopTraining requested";
    m_process->terminate();
    if (!m_process->waitForFinished(1500)) {
        m_process->kill();
    }
    m_running = false;
    m_status = QStringLiteral("Training stopped");
    appendLogLine(m_status);
    emit settingsChanged();
    return true;
}
