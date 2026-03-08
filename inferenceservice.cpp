#include "inferenceservice.h"

#include "systemservice.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QDebug>

namespace {
const QString kTaskTypeKey = QStringLiteral("inference/taskType");
const QString kPythonPathKey = QStringLiteral("inference/pythonFilePath");
const QString kDockerImageKey = QStringLiteral("inference/dockerImage");
const QString kDockerHostMountPathKey = QStringLiteral("inference/dockerHostMountPath");
const QString kDockerContainerWorkDirKey = QStringLiteral("inference/dockerContainerWorkDir");
const QString kBestModelPathKey = QStringLiteral("inference/bestModelPath");
const QString kUseTrainKey = QStringLiteral("inference/useTrain");
const QString kUseValKey = QStringLiteral("inference/useVal");
const QString kUseTestKey = QStringLiteral("inference/useTest");
const QString kUiSettingsExpandedKey = QStringLiteral("inference/uiSettingsExpanded");
}

InferenceService::InferenceService(SystemService *systemService, QObject *parent)
    : QObject(parent)
    , m_systemService(systemService)
{
    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        const QString text = QString::fromLocal8Bit(m_process->readAllStandardOutput());
        const QStringList lines = text.split('\n', Qt::SkipEmptyParts);
        for (const QString &raw : lines) {
            const QString line = raw.trimmed();
            if (line.isEmpty()) {
                continue;
            }
            appendLogLine(line);
            if (line.startsWith('[')) {
                const int slash = line.indexOf('/');
                const int close = line.indexOf(']');
                if (slash > 1 && close > slash) {
                    bool okCur = false;
                    bool okTot = false;
                    const int current = line.mid(1, slash - 1).toInt(&okCur);
                    const int total = line.mid(slash + 1, close - slash - 1).toInt(&okTot);
                    if (okCur && okTot && total > 0) {
                        m_progress = qBound(0, (current * 100) / total, 100);
                    }
                }
            }
        }
        emit settingsChanged();
    });
    connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
        const QString text = QString::fromLocal8Bit(m_process->readAllStandardError());
        const QStringList lines = text.split('\n', Qt::SkipEmptyParts);
        for (const QString &raw : lines) {
            const QString line = raw.trimmed();
            if (!line.isEmpty()) {
                appendLogLine(QStringLiteral("[ERR] %1").arg(line));
            }
        }
        emit settingsChanged();
    });
    connect(m_process,
            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                m_running = false;
                if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
                    m_progress = 100;
                    m_status = QStringLiteral("Inference complete");
                    const QString projectBest = readBestModelFromProject(m_lastProjectFilePath);
                    if (!projectBest.isEmpty()) {
                        m_settings.bestModelPath = projectBest;
                        if (m_systemService) {
                            m_systemService->saveAppValue(kBestModelPathKey, m_settings.bestModelPath);
                        }
                    }
                } else {
                    m_status = QStringLiteral("Inference failed (code=%1)").arg(exitCode);
                }
                appendLogLine(m_status);
                emit settingsChanged();
            });
    loadFromConfig();
}

QVariantMap InferenceService::state() const
{
    QVariantMap m;
    QString bestModelPath = m_settings.bestModelPath;
    if (bestModelPath.isEmpty() && m_systemService) {
        bestModelPath = readBestModelFromProject(m_systemService->projectFilePath());
    }
    m.insert(QStringLiteral("taskType"), m_settings.taskType);
    m.insert(QStringLiteral("pythonFilePath"), m_settings.pythonFilePath);
    m.insert(QStringLiteral("dockerImage"), m_settings.dockerImage);
    m.insert(QStringLiteral("dockerHostMountPath"), m_settings.dockerHostMountPath);
    m.insert(QStringLiteral("dockerContainerWorkDir"), m_settings.dockerContainerWorkDir);
    m.insert(QStringLiteral("bestModelPath"), bestModelPath);
    m.insert(QStringLiteral("useTrain"), m_settings.useTrain);
    m.insert(QStringLiteral("useVal"), m_settings.useVal);
    m.insert(QStringLiteral("useTest"), m_settings.useTest);
    m.insert(QStringLiteral("settingsExpanded"), m_settings.uiSettingsExpanded);
    m.insert(QStringLiteral("running"), m_running);
    m.insert(QStringLiteral("progress"), m_progress);
    m.insert(QStringLiteral("status"), m_status);
    m.insert(QStringLiteral("logText"), m_logLines.join('\n'));
    return m;
}

bool InferenceService::action(const QString &action, const QVariantMap &payload)
{
    const QString op = action.trimmed().toLower();
    qDebug() << "[InferenceService] action:" << op << "payload keys:" << payload.keys();
    if (op == QStringLiteral("set_setting")) {
        return setSetting(payload.value(QStringLiteral("setting")).toString(), payload.value(QStringLiteral("value")));
    }
    if (op == QStringLiteral("start_inference")) {
        return startInference(payload);
    }
    if (op == QStringLiteral("stop_inference")) {
        return stopInference();
    }
    return false;
}

bool InferenceService::setSetting(const QString &key, const QVariant &value)
{
    const QString k = key.trimmed();
    if (k.isEmpty() || !m_systemService) {
        return false;
    }

    bool changed = false;
    if (k == QStringLiteral("taskType")) {
        const QString v = value.toString().trimmed().toLower();
        if (!v.isEmpty() && m_settings.taskType != v) {
            m_settings.taskType = v;
            m_systemService->saveAppValue(kTaskTypeKey, m_settings.taskType);
            changed = true;
        }
    } else if (k == QStringLiteral("pythonFilePath")) {
        const QString v = SystemService::normalizePath(value);
        if (m_settings.pythonFilePath != v) {
            m_settings.pythonFilePath = v;
            m_systemService->saveAppValue(kPythonPathKey, m_settings.pythonFilePath);
            changed = true;
        }
    } else if (k == QStringLiteral("dockerImage")) {
        const QString v = value.toString().trimmed();
        if (m_settings.dockerImage != v) {
            m_settings.dockerImage = v;
            m_systemService->saveAppValue(kDockerImageKey, m_settings.dockerImage);
            changed = true;
        }
    } else if (k == QStringLiteral("dockerHostMountPath")) {
        const QString v = SystemService::normalizePath(value);
        if (m_settings.dockerHostMountPath != v) {
            m_settings.dockerHostMountPath = v;
            m_systemService->saveAppValue(kDockerHostMountPathKey, m_settings.dockerHostMountPath);
            changed = true;
        }
    } else if (k == QStringLiteral("dockerContainerWorkDir")) {
        const QString v = value.toString().trimmed().isEmpty() ? QStringLiteral("/workspace")
                                                                : value.toString().trimmed();
        if (m_settings.dockerContainerWorkDir != v) {
            m_settings.dockerContainerWorkDir = v;
            m_systemService->saveAppValue(kDockerContainerWorkDirKey, m_settings.dockerContainerWorkDir);
            changed = true;
        }
    } else if (k == QStringLiteral("bestModelPath")) {
        const QString v = SystemService::normalizePath(value);
        if (m_settings.bestModelPath != v) {
            m_settings.bestModelPath = v;
            m_systemService->saveAppValue(kBestModelPathKey, m_settings.bestModelPath);
            changed = true;
        }
    } else if (k == QStringLiteral("useTrain")) {
        const bool v = value.toBool();
        if (m_settings.useTrain != v) {
            m_settings.useTrain = v;
            m_systemService->saveAppValue(kUseTrainKey, m_settings.useTrain);
            changed = true;
        }
    } else if (k == QStringLiteral("useVal")) {
        const bool v = value.toBool();
        if (m_settings.useVal != v) {
            m_settings.useVal = v;
            m_systemService->saveAppValue(kUseValKey, m_settings.useVal);
            changed = true;
        }
    } else if (k == QStringLiteral("useTest")) {
        const bool v = value.toBool();
        if (m_settings.useTest != v) {
            m_settings.useTest = v;
            m_systemService->saveAppValue(kUseTestKey, m_settings.useTest);
            changed = true;
        }
    } else if (k == QStringLiteral("uiSettingsExpanded")) {
        const bool v = value.toBool();
        if (m_settings.uiSettingsExpanded != v) {
            m_settings.uiSettingsExpanded = v;
            m_systemService->saveAppValue(kUiSettingsExpandedKey, m_settings.uiSettingsExpanded);
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

void InferenceService::appendLogLine(const QString &line)
{
    m_logLines.push_back(line);
    while (m_logLines.size() > 500) {
        m_logLines.removeFirst();
    }
}

void InferenceService::loadFromConfig()
{
    if (!m_systemService) {
        return;
    }
    m_settings.taskType = m_systemService->loadAppValue(kTaskTypeKey, QStringLiteral("classification")).toString();
    m_settings.pythonFilePath = m_systemService->loadAppValue(kPythonPathKey).toString();
    m_settings.dockerImage = m_systemService->loadAppValue(kDockerImageKey).toString();
    m_settings.dockerHostMountPath = m_systemService->loadAppValue(kDockerHostMountPathKey).toString();
    m_settings.dockerContainerWorkDir = m_systemService->loadAppValue(kDockerContainerWorkDirKey, QStringLiteral("/workspace")).toString();
    if (m_settings.dockerContainerWorkDir.trimmed().isEmpty()) {
        m_settings.dockerContainerWorkDir = QStringLiteral("/workspace");
    }
    m_settings.bestModelPath = m_systemService->loadAppValue(kBestModelPathKey).toString();
    m_settings.useTrain = m_systemService->loadAppValue(kUseTrainKey, true).toBool();
    m_settings.useVal = m_systemService->loadAppValue(kUseValKey, true).toBool();
    m_settings.useTest = m_systemService->loadAppValue(kUseTestKey, true).toBool();
    m_settings.uiSettingsExpanded = m_systemService->loadAppValue(kUiSettingsExpandedKey, true).toBool();

    // Fallback to training(classification) settings so Inference works out-of-box.
    if (m_settings.pythonFilePath.trimmed().isEmpty()) {
        m_settings.pythonFilePath =
            m_systemService->loadAppValue(QStringLiteral("training/tasks/classification/pythonFilePath")).toString();
    }
    if (m_settings.dockerImage.trimmed().isEmpty()) {
        m_settings.dockerImage =
            m_systemService->loadAppValue(QStringLiteral("training/tasks/classification/dockerImage")).toString();
    }
    if (m_settings.dockerHostMountPath.trimmed().isEmpty()) {
        m_settings.dockerHostMountPath =
            m_systemService->loadAppValue(QStringLiteral("training/tasks/classification/dockerHostMountPath")).toString();
    }
    if (m_settings.dockerContainerWorkDir.trimmed().isEmpty()
        || m_settings.dockerContainerWorkDir == QStringLiteral("/workspace")) {
        const QString trainWd =
            m_systemService->loadAppValue(QStringLiteral("training/tasks/classification/dockerContainerWorkDir"),
                                          QStringLiteral("/workspace"))
                .toString();
        if (!trainWd.trimmed().isEmpty()) {
            m_settings.dockerContainerWorkDir = trainWd;
        }
    }
    if (m_settings.bestModelPath.trimmed().isEmpty()) {
        m_settings.bestModelPath =
            m_systemService->loadAppValue(QStringLiteral("training/tasks/classification/bestModelPath")).toString();
    }
}

QString InferenceService::readBestModelFromProject(const QString &projectFilePath) const
{
    const QString path = SystemService::normalizePath(projectFilePath);
    if (path.isEmpty()) {
        return QString();
    }
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        return QString();
    }
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isObject()) {
        return QString();
    }
    return SystemService::normalizePath(doc.object().value(QStringLiteral("bestModelPath")).toString());
}

bool InferenceService::startInference(const QVariantMap &payload)
{
    if (m_running) {
        return false;
    }
    const QString projectFilePath = SystemService::normalizePath(payload.value(QStringLiteral("projectFilePath")));
    m_lastProjectFilePath = projectFilePath;
    if (projectFilePath.isEmpty()) {
        m_status = QStringLiteral("Project file path is empty");
        appendLogLine(m_status);
        emit settingsChanged();
        return false;
    }

    QString bestModelPath = m_settings.bestModelPath;
    if (bestModelPath.trimmed().isEmpty()) {
        bestModelPath = readBestModelFromProject(projectFilePath);
    }
    if (bestModelPath.trimmed().isEmpty()) {
        m_status = QStringLiteral("Best model path is empty");
        appendLogLine(m_status);
        emit settingsChanged();
        return false;
    }

    QStringList splits;
    if (m_settings.useTrain) {
        splits.push_back(QStringLiteral("train"));
    }
    if (m_settings.useVal) {
        splits.push_back(QStringLiteral("val"));
    }
    if (m_settings.useTest) {
        splits.push_back(QStringLiteral("test"));
    }
    if (splits.isEmpty()) {
        m_status = QStringLiteral("Select at least one split");
        appendLogLine(m_status);
        emit settingsChanged();
        return false;
    }

    const QString rawScriptPath = m_settings.pythonFilePath.trimmed();
    if (rawScriptPath.isEmpty()) {
        m_status = QStringLiteral("Python script path is empty");
        appendLogLine(m_status);
        emit settingsChanged();
        return false;
    }

    QString resolvedScriptPath = rawScriptPath;
    QString hostMountForResolve = m_settings.dockerHostMountPath.trimmed();
    if (hostMountForResolve.isEmpty()) {
        hostMountForResolve = QDir::currentPath();
    }
    if (QDir::isRelativePath(resolvedScriptPath)) {
        resolvedScriptPath = QDir(hostMountForResolve).absoluteFilePath(resolvedScriptPath);
    }
    const QFileInfo scriptInfo(resolvedScriptPath);
    if (!scriptInfo.exists() || !scriptInfo.isFile()) {
        m_status = QStringLiteral("Python script not found: %1").arg(resolvedScriptPath);
        appendLogLine(m_status);
        emit settingsChanged();
        return false;
    }

    QStringList args;
    QString program;
    QString workingDir = scriptInfo.absoluteDir().absolutePath();

    if (!m_settings.dockerImage.trimmed().isEmpty()) {
        program = QStringLiteral("docker");
        QString hostMount = m_settings.dockerHostMountPath.trimmed();
        if (hostMount.isEmpty()) {
            hostMount = scriptInfo.absoluteDir().absolutePath();
        }
        QString containerWorkDir = m_settings.dockerContainerWorkDir.trimmed();
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
                return QDir(containerWorkDir).filePath(cleanHostPath.mid(cleanHostMount.size() + 1));
            }
            return cleanHostPath;
        };

        QString extraProjectHostDir;
        QString extraProjectContainerDir = QStringLiteral("/workspace_config");
        QString containerProjectPath;
        {
            const QString cleanConfigPath = QDir::cleanPath(projectFilePath);
            const QString cleanHostMount = QDir::cleanPath(hostMount);
            const bool underMainMount = (cleanConfigPath == cleanHostMount)
                || cleanConfigPath.startsWith(cleanHostMount + QDir::separator());
            if (underMainMount) {
                containerProjectPath = hostToContainerPath(cleanConfigPath);
            } else {
                QFileInfo cfi(cleanConfigPath);
                extraProjectHostDir = cfi.absolutePath();
                containerProjectPath = QDir(extraProjectContainerDir).filePath(cfi.fileName());
            }
        }

        QString extraModelHostDir;
        QString extraModelContainerDir = QStringLiteral("/workspace_model");
        QString containerModelPath;
        {
            const QString cleanModelPath = QDir::cleanPath(bestModelPath);
            const QString cleanHostMount = QDir::cleanPath(hostMount);
            const bool underMainMount = (cleanModelPath == cleanHostMount)
                || cleanModelPath.startsWith(cleanHostMount + QDir::separator());
            if (underMainMount) {
                containerModelPath = hostToContainerPath(cleanModelPath);
            } else {
                QFileInfo mfi(cleanModelPath);
                extraModelHostDir = mfi.absolutePath();
                containerModelPath = QDir(extraModelContainerDir).filePath(mfi.fileName());
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
             << QStringLiteral("-w") << containerWorkDir;
        if (!extraProjectHostDir.isEmpty()) {
            args << QStringLiteral("-v") << QStringLiteral("%1:%2").arg(extraProjectHostDir, extraProjectContainerDir);
        }
        if (!extraModelHostDir.isEmpty()) {
            args << QStringLiteral("-v") << QStringLiteral("%1:%2").arg(extraModelHostDir, extraModelContainerDir);
        }
        args << m_settings.dockerImage.trimmed()
             << QStringLiteral("python3")
             << containerScriptPath
             << QStringLiteral("--infer-only")
             << QStringLiteral("--config-json") << containerProjectPath
             << QStringLiteral("--host-mount-path") << hostMount
             << QStringLiteral("--container-workdir") << containerWorkDir
             << QStringLiteral("--checkpoint-path") << containerModelPath
             << QStringLiteral("--infer-splits") << splits.join(',');
    } else {
        program = QStringLiteral("python3");
        args << resolvedScriptPath
             << QStringLiteral("--infer-only")
             << QStringLiteral("--config-json") << projectFilePath
             << QStringLiteral("--checkpoint-path") << bestModelPath
             << QStringLiteral("--infer-splits") << splits.join(',');
    }

    m_logLines.clear();
    m_progress = 0;
    m_running = true;
    m_status = QStringLiteral("Inference started");
    appendLogLine(QStringLiteral("Selected splits: %1").arg(splits.join(',')));
    appendLogLine(QStringLiteral("Script: %1").arg(resolvedScriptPath));
    appendLogLine(QStringLiteral("Model: %1").arg(bestModelPath));
    qDebug() << "[InferenceService] startInference script:" << resolvedScriptPath;
    qDebug() << "[InferenceService] startInference model:" << bestModelPath;
    qDebug() << "[InferenceService] startInference project:" << projectFilePath;
    emit settingsChanged();

    m_process->setWorkingDirectory(workingDir);
    m_process->start(program, args);
    if (!m_process->waitForStarted(3000)) {
        m_running = false;
        m_status = QStringLiteral("Failed to start inference process");
        appendLogLine(m_status);
        qDebug() << "[InferenceService] failed to start process:" << m_process->errorString();
        emit settingsChanged();
        return false;
    }
    qDebug() << "[InferenceService] inference process started, pid:" << m_process->processId();
    return true;
}

bool InferenceService::stopInference()
{
    if (!m_running) {
        return false;
    }
    m_process->terminate();
    if (!m_process->waitForFinished(1500)) {
        m_process->kill();
    }
    m_running = false;
    m_status = QStringLiteral("Inference stopped");
    appendLogLine(m_status);
    emit settingsChanged();
    return true;
}
