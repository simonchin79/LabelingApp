#include "trainservice.h"

#include "systemservice.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QStringList>

namespace {
const QString kTrainingPythonFilePathKey = QStringLiteral("training/pythonFilePath");
const QString kTrainingPredictionJsonPathKey = QStringLiteral("training/predictionJsonPath");
const QString kTrainingBestModelPathKey = QStringLiteral("training/bestModelPath");
const QString kTrainingEpochsKey = QStringLiteral("training/epochs");
const QString kTrainingBatchSizeKey = QStringLiteral("training/batchSize");
const QString kTrainingNumWorkersKey = QStringLiteral("training/numWorkers");
const QString kTrainingInputSizeKey = QStringLiteral("training/inputSize");
const QString kTrainingLearningRateKey = QStringLiteral("training/learningRate");
const QString kTrainingWeightDecayKey = QStringLiteral("training/weightDecay");
const QString kTrainingBackboneKey = QStringLiteral("training/backbone");
const QString kTrainingSplitTrainCountKey = QStringLiteral("training/splitTrainCount");
const QString kTrainingSplitValCountKey = QStringLiteral("training/splitValCount");
const QString kTrainingSplitTestCountKey = QStringLiteral("training/splitTestCount");
const QString kTrainingSplitTrainPercentKey = QStringLiteral("training/splitTrainPercent");
const QString kTrainingSplitValPercentKey = QStringLiteral("training/splitValPercent");
const QString kTrainingSplitTestPercentKey = QStringLiteral("training/splitTestPercent");
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
                m_running = false;
                m_progress = (exitCode == 0 && exitStatus == QProcess::NormalExit) ? 100 : m_progress;
                m_status = (exitCode == 0 && exitStatus == QProcess::NormalExit)
                               ? QStringLiteral("Training complete")
                               : QStringLiteral("Training failed (code=%1)").arg(exitCode);
                appendLogLine(m_status);
                emit settingsChanged();
            });
    loadFromConfig();
}

QVariantMap TrainService::state() const
{
    QVariantMap data;
    data.insert(QStringLiteral("pythonFilePath"), m_pythonFilePath);
    data.insert(QStringLiteral("predictionJsonPath"), m_predictionJsonPath);
    data.insert(QStringLiteral("bestModelPath"), m_bestModelPath);
    data.insert(QStringLiteral("epochs"), m_epochs);
    data.insert(QStringLiteral("batchSize"), m_batchSize);
    data.insert(QStringLiteral("numWorkers"), m_numWorkers);
    data.insert(QStringLiteral("inputSize"), m_inputSize);
    data.insert(QStringLiteral("learningRate"), m_learningRate);
    data.insert(QStringLiteral("weightDecay"), m_weightDecay);
    data.insert(QStringLiteral("backbone"), m_backbone);
    data.insert(QStringLiteral("splitTrainCount"), m_splitTrainCount);
    data.insert(QStringLiteral("splitValCount"), m_splitValCount);
    data.insert(QStringLiteral("splitTestCount"), m_splitTestCount);
    data.insert(QStringLiteral("splitTrainPercent"), m_splitTrainPercent);
    data.insert(QStringLiteral("splitValPercent"), m_splitValPercent);
    data.insert(QStringLiteral("splitTestPercent"), m_splitTestPercent);
    data.insert(QStringLiteral("running"), m_running);
    data.insert(QStringLiteral("progress"), m_progress);
    data.insert(QStringLiteral("status"), m_status);
    data.insert(QStringLiteral("logText"), m_logLines.join('\n'));
    return data;
}

bool TrainService::action(const QString &action, const QVariantMap &payload)
{
    const QString task = action.trimmed().toLower();
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

    bool changed = false;
    if (setting == QStringLiteral("pythonFilePath")) {
        const QString v = SystemService::normalizePath(value);
        if (m_pythonFilePath != v) {
            m_pythonFilePath = v;
            m_systemService->saveAppValue(kTrainingPythonFilePathKey, m_pythonFilePath);
            changed = true;
        }
    } else if (setting == QStringLiteral("predictionJsonPath")) {
        const QString v = SystemService::normalizePath(value);
        if (m_predictionJsonPath != v) {
            m_predictionJsonPath = v;
            m_systemService->saveAppValue(kTrainingPredictionJsonPathKey, m_predictionJsonPath);
            changed = true;
        }
    } else if (setting == QStringLiteral("bestModelPath")) {
        const QString v = SystemService::normalizePath(value);
        if (m_bestModelPath != v) {
            m_bestModelPath = v;
            m_systemService->saveAppValue(kTrainingBestModelPathKey, m_bestModelPath);
            changed = true;
        }
    } else if (setting == QStringLiteral("epochs")) {
        const int v = qMax(1, value.toInt());
        if (m_epochs != v) {
            m_epochs = v;
            m_systemService->saveAppValue(kTrainingEpochsKey, m_epochs);
            changed = true;
        }
    } else if (setting == QStringLiteral("batchSize")) {
        const int v = qMax(1, value.toInt());
        if (m_batchSize != v) {
            m_batchSize = v;
            m_systemService->saveAppValue(kTrainingBatchSizeKey, m_batchSize);
            changed = true;
        }
    } else if (setting == QStringLiteral("numWorkers")) {
        const int v = qMax(0, value.toInt());
        if (m_numWorkers != v) {
            m_numWorkers = v;
            m_systemService->saveAppValue(kTrainingNumWorkersKey, m_numWorkers);
            changed = true;
        }
    } else if (setting == QStringLiteral("inputSize")) {
        const int v = qMax(32, value.toInt());
        if (m_inputSize != v) {
            m_inputSize = v;
            m_systemService->saveAppValue(kTrainingInputSizeKey, m_inputSize);
            changed = true;
        }
    } else if (setting == QStringLiteral("learningRate")) {
        const double v = qMax(1e-7, value.toDouble());
        if (!qFuzzyCompare(m_learningRate, v)) {
            m_learningRate = v;
            m_systemService->saveAppValue(kTrainingLearningRateKey, m_learningRate);
            changed = true;
        }
    } else if (setting == QStringLiteral("weightDecay")) {
        const double v = qMax(0.0, value.toDouble());
        if (!qFuzzyCompare(m_weightDecay, v)) {
            m_weightDecay = v;
            m_systemService->saveAppValue(kTrainingWeightDecayKey, m_weightDecay);
            changed = true;
        }
    } else if (setting == QStringLiteral("backbone")) {
        const QString v = value.toString().trimmed();
        if (!v.isEmpty() && m_backbone != v) {
            m_backbone = v;
            m_systemService->saveAppValue(kTrainingBackboneKey, m_backbone);
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
    m_pythonFilePath = m_systemService->loadAppValue(kTrainingPythonFilePathKey).toString();
    m_predictionJsonPath = m_systemService->loadAppValue(kTrainingPredictionJsonPathKey).toString();
    m_bestModelPath = m_systemService->loadAppValue(kTrainingBestModelPathKey).toString();
    m_epochs = qMax(1, m_systemService->loadAppValue(kTrainingEpochsKey, 40).toInt());
    m_batchSize = qMax(1, m_systemService->loadAppValue(kTrainingBatchSizeKey, 64).toInt());
    m_numWorkers = qMax(0, m_systemService->loadAppValue(kTrainingNumWorkersKey, 8).toInt());
    m_inputSize = qMax(32, m_systemService->loadAppValue(kTrainingInputSizeKey, 224).toInt());
    m_learningRate = qMax(1e-7, m_systemService->loadAppValue(kTrainingLearningRateKey, 1e-3).toDouble());
    m_weightDecay = qMax(0.0, m_systemService->loadAppValue(kTrainingWeightDecayKey, 1e-4).toDouble());
    m_backbone = m_systemService->loadAppValue(kTrainingBackboneKey, QStringLiteral("efficientnet_b0")).toString();
    m_splitTrainCount = qMax(0, m_systemService->loadAppValue(kTrainingSplitTrainCountKey, 0).toInt());
    m_splitValCount = qMax(0, m_systemService->loadAppValue(kTrainingSplitValCountKey, 0).toInt());
    m_splitTestCount = qMax(0, m_systemService->loadAppValue(kTrainingSplitTestCountKey, 0).toInt());
    m_splitTrainPercent = qMax(0, m_systemService->loadAppValue(kTrainingSplitTrainPercentKey, 60).toInt());
    m_splitValPercent = qMax(0, m_systemService->loadAppValue(kTrainingSplitValPercentKey, 30).toInt());
    m_splitTestPercent = qMax(0, m_systemService->loadAppValue(kTrainingSplitTestPercentKey, 10).toInt());
}

void TrainService::appendLogLine(const QString &line)
{
    m_logLines.push_back(line);
    while (m_logLines.size() > 400) {
        m_logLines.removeFirst();
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
    if (m_pythonFilePath.trimmed().isEmpty()) {
        m_status = QStringLiteral("Python script path is empty");
        emit settingsChanged();
        return false;
    }

    const QFileInfo scriptInfo(m_pythonFilePath);
    if (!scriptInfo.exists() || !scriptInfo.isFile()) {
        m_status = QStringLiteral("Python script not found");
        emit settingsChanged();
        return false;
    }

    QString program = payload.value(QStringLiteral("pythonExe")).toString().trimmed();
    if (program.isEmpty()) {
        program = QStringLiteral("python3");
    }

    QStringList args;
    args.push_back(m_pythonFilePath);
    if (!m_backbone.trimmed().isEmpty()) {
        args.push_back(QStringLiteral("--backbone"));
        args.push_back(m_backbone.trimmed());
    }

    m_logLines.clear();
    m_progress = 0;
    m_running = true;
    m_status = QStringLiteral("Training started");
    emit settingsChanged();

    m_process->setWorkingDirectory(scriptInfo.absoluteDir().absolutePath());
    m_process->start(program, args);
    if (!m_process->waitForStarted(3000)) {
        m_running = false;
        m_status = QStringLiteral("Failed to start training process");
        appendLogLine(m_status);
        emit settingsChanged();
        return false;
    }
    return true;
}

bool TrainService::stopTraining()
{
    if (!m_running) {
        return false;
    }
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
