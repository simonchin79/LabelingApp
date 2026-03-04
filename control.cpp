#include "control.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QImageReader>
#include <QSet>
#include <QUrl>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

namespace {
const QString kLastProjectFileKey = QStringLiteral("session/lastProjectFile");
const QString kLastImageIndexKey = QStringLiteral("session/lastImageIndex");
const QString kShowLabelKey = QStringLiteral("ui/showLabel");
const QString kShowPredictKey = QStringLiteral("ui/showPredict");
const QString kShowGoodKey = QStringLiteral("ui/showGood");
const QString kIoFolderPathKey = QStringLiteral("io/lastFolderPath");

enum class ProjectTask {
    Unknown,
    Create,
    Load,
    SaveAs
};

enum class ImageTask {
    Unknown,
    ImportFolder,
    ImportFiles,
    Select,
    First,
    Last,
    Next,
    Previous,
    Next10,
    Previous10
};

enum class IoTask {
    Unknown,
    SetFolder,
    SetVisibility,
    ImportClassification,
    ExportClassification
};

enum class AnnotationTask {
    Unknown,
    StartNew,
    AddClass,
    DeleteClass,
    SetCurrentClass,
    SetField,
    SetKind,
    SetSeverity,
    SetSplit,
    SetRemarks,
    AddPoint,
    UpdateDraftPoint,
    Save,
    Undo,
    Clear,
    Select,
    Delete,
    UpdateClass,
    UpdateDetails,
    UpdatePoint,
    SelectFromList
};

ProjectTask toProjectTask(const QString &task)
{
    if (task == QStringLiteral("create")) {
        return ProjectTask::Create;
    }
    if (task == QStringLiteral("load")) {
        return ProjectTask::Load;
    }
    if (task == QStringLiteral("save_as")) {
        return ProjectTask::SaveAs;
    }
    return ProjectTask::Unknown;
}

ImageTask toImageTask(const QString &task)
{
    if (task == QStringLiteral("import_folder")) {
        return ImageTask::ImportFolder;
    }
    if (task == QStringLiteral("import_files")) {
        return ImageTask::ImportFiles;
    }
    if (task == QStringLiteral("select")) {
        return ImageTask::Select;
    }
    if (task == QStringLiteral("first")) {
        return ImageTask::First;
    }
    if (task == QStringLiteral("last")) {
        return ImageTask::Last;
    }
    if (task == QStringLiteral("next")) {
        return ImageTask::Next;
    }
    if (task == QStringLiteral("prev")) {
        return ImageTask::Previous;
    }
    if (task == QStringLiteral("next10")) {
        return ImageTask::Next10;
    }
    if (task == QStringLiteral("prev10")) {
        return ImageTask::Previous10;
    }
    return ImageTask::Unknown;
}

IoTask toIoTask(const QString &task)
{
    if (task == QStringLiteral("set_folder")) {
        return IoTask::SetFolder;
    }
    if (task == QStringLiteral("set_visibility")) {
        return IoTask::SetVisibility;
    }
    if (task == QStringLiteral("import_classification")) {
        return IoTask::ImportClassification;
    }
    if (task == QStringLiteral("export_classification")) {
        return IoTask::ExportClassification;
    }
    return IoTask::Unknown;
}

AnnotationTask toAnnotationTask(const QString &task)
{
    if (task == QStringLiteral("start_new")) {
        return AnnotationTask::StartNew;
    }
    if (task == QStringLiteral("add_class")) {
        return AnnotationTask::AddClass;
    }
    if (task == QStringLiteral("delete_class")) {
        return AnnotationTask::DeleteClass;
    }
    if (task == QStringLiteral("set_current_class")) {
        return AnnotationTask::SetCurrentClass;
    }
    if (task == QStringLiteral("set_field")) {
        return AnnotationTask::SetField;
    }
    if (task == QStringLiteral("set_kind")) {
        return AnnotationTask::SetKind;
    }
    if (task == QStringLiteral("set_severity")) {
        return AnnotationTask::SetSeverity;
    }
    if (task == QStringLiteral("set_split")) {
        return AnnotationTask::SetSplit;
    }
    if (task == QStringLiteral("set_remarks")) {
        return AnnotationTask::SetRemarks;
    }
    if (task == QStringLiteral("add_point")) {
        return AnnotationTask::AddPoint;
    }
    if (task == QStringLiteral("update_draft_point")) {
        return AnnotationTask::UpdateDraftPoint;
    }
    if (task == QStringLiteral("save")) {
        return AnnotationTask::Save;
    }
    if (task == QStringLiteral("undo")) {
        return AnnotationTask::Undo;
    }
    if (task == QStringLiteral("clear")) {
        return AnnotationTask::Clear;
    }
    if (task == QStringLiteral("select")) {
        return AnnotationTask::Select;
    }
    if (task == QStringLiteral("delete")) {
        return AnnotationTask::Delete;
    }
    if (task == QStringLiteral("update_class")) {
        return AnnotationTask::UpdateClass;
    }
    if (task == QStringLiteral("update_details")) {
        return AnnotationTask::UpdateDetails;
    }
    if (task == QStringLiteral("update_point")) {
        return AnnotationTask::UpdatePoint;
    }
    if (task == QStringLiteral("select_from_list")) {
        return AnnotationTask::SelectFromList;
    }
    return AnnotationTask::Unknown;
}
}

Control::Control(QObject *parent)
    : QObject(parent)
    , m_systemService(this)
    , m_labelingService(&m_systemService, this)
    , m_trainService(this)
    , m_inferenceService(this)
{
    connect(this, &Control::statusChanged, this, &Control::uiStateChanged);
    connect(this, &Control::projectChanged, this, &Control::uiStateChanged);
    connect(this, &Control::imagePathsChanged, this, &Control::uiStateChanged);
    connect(this, &Control::currentImageChanged, this, &Control::uiStateChanged);
    connect(this, &Control::classNamesChanged, this, &Control::uiStateChanged);
    connect(this, &Control::currentClassNameChanged, this, &Control::uiStateChanged);
    connect(this, &Control::draftPointsChanged, this, &Control::uiStateChanged);
    connect(this, &Control::currentPolygonsChanged, this, &Control::uiStateChanged);
    connect(this, &Control::selectedPolygonIndexChanged, this, &Control::uiStateChanged);
    connect(this, &Control::selectedAnnotationClassNameChanged, this, &Control::uiStateChanged);
    connect(this, &Control::annotationFieldsChanged, this, &Control::uiStateChanged);
    connect(this, &Control::typeVisibilityChanged, this, &Control::uiStateChanged);
    connect(this, &Control::ioFolderPathChanged, this, &Control::uiStateChanged);

    m_status = QStringLiteral("Create a project to start labeling");
    m_showLabel = m_systemService.loadAppValue(kShowLabelKey, true).toBool();
    m_showPredict = m_systemService.loadAppValue(kShowPredictKey, true).toBool();
    m_showGood = m_systemService.loadAppValue(kShowGoodKey, true).toBool();
    m_ioFolderPath = m_systemService.loadAppValue(kIoFolderPathKey).toString();
    if (!restoreLastSession()) {
        setStatus(QStringLiteral("Create a project to start labeling"));
    }
}

QString Control::currentImagePath() const
{
    if (m_currentImageIndex < 0 || m_currentImageIndex >= m_imagePaths.size()) {
        return QString();
    }

    return m_imagePaths.at(m_currentImageIndex);
}

QVariantList Control::draftPoints() const
{
    QVariantList points;
    points.reserve(m_draftPoints.size());
    for (const QPointF &pt : m_draftPoints) {
        QVariantMap map;
        map.insert(QStringLiteral("x"), pt.x());
        map.insert(QStringLiteral("y"), pt.y());
        points.push_back(map);
    }

    return points;
}

QVariantList Control::currentPolygons() const
{
    QVariantList polygons;
    const QList<AnnotationData> annList = currentImageAnnotations();
    polygons.reserve(annList.size());

    for (const AnnotationData &ann : annList) {
        const QString kind = m_labelingService.normalizedKind(ann.kind);
        if ((kind == QStringLiteral("label") && !m_showLabel)
            || (kind == QStringLiteral("predict") && !m_showPredict)
            || (kind == QStringLiteral("good") && !m_showGood)) {
            continue;
        }

        QVariantMap poly;
        poly.insert(QStringLiteral("className"), ann.className);
        poly.insert(QStringLiteral("kind"), kind);

        QVariantList points;
        points.reserve(ann.points.size());
        for (const QPointF &pt : ann.points) {
            QVariantMap pointMap;
            pointMap.insert(QStringLiteral("x"), pt.x());
            pointMap.insert(QStringLiteral("y"), pt.y());
            points.push_back(pointMap);
        }

        poly.insert(QStringLiteral("points"), points);
        polygons.push_back(poly);
    }

    return polygons;
}

QString Control::selectedAnnotationClassName() const
{
    const QList<AnnotationData> annList = currentImageAnnotations();
    if (m_selectedPolygonIndex < 0 || m_selectedPolygonIndex >= annList.size()) {
        return QString();
    }

    return annList.at(m_selectedPolygonIndex).className;
}

void Control::setIoFolderPath(const QString &path)
{
    const QString normalized = normalizePath(path);
    if (m_ioFolderPath == normalized) {
        return;
    }

    m_ioFolderPath = normalized;
    m_systemService.saveAppValue(kIoFolderPathKey, m_ioFolderPath);
    emit ioFolderPathChanged();
}

QVariantMap Control::projectState() const
{
    QVariantMap state;
    state.insert(QStringLiteral("status"), m_status);
    state.insert(QStringLiteral("projectName"), m_projectName);
    state.insert(QStringLiteral("projectFilePath"), m_systemService.projectFilePath());
    return state;
}

QVariantMap Control::imageState() const
{
    QVariantMap state;
    state.insert(QStringLiteral("imagePaths"), m_imagePaths);
    state.insert(QStringLiteral("imageCount"), m_imagePaths.size());
    state.insert(QStringLiteral("currentImageIndex"), m_currentImageIndex);
    state.insert(QStringLiteral("currentImagePath"), currentImagePath());
    return state;
}

QVariantMap Control::annotationState() const
{
    QVariantMap state;
    state.insert(QStringLiteral("classNames"), m_classNames);
    state.insert(QStringLiteral("currentClassName"), m_currentClassName);
    state.insert(QStringLiteral("draftPoints"), draftPoints());
    state.insert(QStringLiteral("currentPolygons"), currentPolygons());
    state.insert(QStringLiteral("selectedPolygonIndex"), m_selectedPolygonIndex);
    state.insert(QStringLiteral("selectedAnnotationClassName"), selectedAnnotationClassName());
    state.insert(QStringLiteral("annotationRemarks"), m_annotationRemarks);
    state.insert(QStringLiteral("annotationKind"), m_annotationKind);
    state.insert(QStringLiteral("annotationSeverity"), m_annotationSeverity);
    state.insert(QStringLiteral("annotationSplit"), m_annotationSplit);
    return state;
}

QVariantMap Control::visibilityState() const
{
    QVariantMap state;
    state.insert(QStringLiteral("showLabel"), m_showLabel);
    state.insert(QStringLiteral("showPredict"), m_showPredict);
    state.insert(QStringLiteral("showGood"), m_showGood);
    state.insert(QStringLiteral("ioFolderPath"), m_ioFolderPath);
    return state;
}

QVariantMap Control::uiState() const
{
    QVariantMap state;
    state.insert(QStringLiteral("project"), projectState());
    state.insert(QStringLiteral("image"), imageState());
    state.insert(QStringLiteral("annotation"), annotationState());
    state.insert(QStringLiteral("visibility"), visibilityState());
    return state;
}

bool Control::executeProjectOperation(const QString &operationKey, const QVariant &value, const QVariantMap &payload)
{
    Q_UNUSED(payload)
    const QString op = operationKey.trimmed().toLower();
    if (op == QStringLiteral("create")) {
        const QString preferredName = value.toString();
        if (!m_systemService.createProject(preferredName)) {
            setStatus(QStringLiteral("Failed to create project"));
            return false;
        }

        resetState();
        m_projectName = QFileInfo(m_systemService.projectFilePath()).completeBaseName();
        m_classNames = {QStringLiteral("default")};
        m_currentClassName = m_classNames.first();

        emit projectChanged();
        emit classNamesChanged();
        emit currentClassNameChanged();
        emit currentPolygonsChanged();
        emit selectedPolygonIndexChanged();
        emit selectedAnnotationClassNameChanged();

        if (!saveProjectData()) {
            setStatus(QStringLiteral("Project created but initial save failed"));
            return false;
        }

        saveSessionState();
        setStatus(QStringLiteral("Project created: %1").arg(m_systemService.projectFilePath()));
        return true;
    }
    if (op == QStringLiteral("load")) {
        const QString path = normalizePath(value);
        if (path.isEmpty() || !m_systemService.loadProject(path)) {
            setStatus(QStringLiteral("Failed to load project file"));
            return false;
        }

        QFile file(m_systemService.projectFilePath());
        if (!file.open(QIODevice::ReadOnly)) {
            setStatus(QStringLiteral("Failed to read project file"));
            return false;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();
        if (!doc.isObject()) {
            setStatus(QStringLiteral("Invalid project JSON"));
            return false;
        }

        resetState();

        const QJsonObject root = doc.object();
        m_projectName = root.value(QStringLiteral("projectName")).toString();

        const QJsonArray classes = root.value(QStringLiteral("classes")).toArray();
        for (const QJsonValue &classValue : classes) {
            const QString name = classValue.toString().trimmed();
            if (!name.isEmpty() && !m_classNames.contains(name)) {
                m_classNames.push_back(name);
            }
        }
        if (m_classNames.isEmpty()) {
            m_classNames = {QStringLiteral("default")};
        }
        m_currentClassName = m_classNames.first();

        const QJsonArray images = root.value(QStringLiteral("images")).toArray();
        for (const QJsonValue &imgVal : images) {
            const QJsonObject imgObj = imgVal.toObject();
            const QString pathValue = imgObj.value(QStringLiteral("path")).toString();
            if (pathValue.isEmpty()) {
                continue;
            }

            m_imagePaths.push_back(pathValue);

            QList<AnnotationData> imageAnnotations;
            const QJsonArray anns = imgObj.value(QStringLiteral("annotations")).toArray();
            for (const QJsonValue &annVal : anns) {
                const QJsonObject annObj = annVal.toObject();
                AnnotationData ann;
                ann.className = annObj.value(QStringLiteral("className")).toString();
                ann.remarks = annObj.value(QStringLiteral("remarks")).toString();
                ann.kind = m_labelingService.normalizedKind(annObj.value(QStringLiteral("kind")).toString());
                ann.severity = m_labelingService.normalizedSeverity(annObj.value(QStringLiteral("severity")).toString());
                ann.split = m_labelingService.normalizedSplit(annObj.value(QStringLiteral("split")).toString());

                const QJsonArray pts = annObj.value(QStringLiteral("points")).toArray();
                for (const QJsonValue &ptVal : pts) {
                    const QJsonObject ptObj = ptVal.toObject();
                    ann.points.push_back(
                        QPointF(ptObj.value(QStringLiteral("x")).toDouble(),
                                ptObj.value(QStringLiteral("y")).toDouble()));
                }

                if (!ann.className.isEmpty() && !ann.points.isEmpty()) {
                    imageAnnotations.push_back(ann);
                }
            }

            if (!imageAnnotations.isEmpty()) {
                m_annotations.insert(m_labelingService.annotationKeyForPath(pathValue), imageAnnotations);
            }
        }

        emit projectChanged();
        emit imagePathsChanged();
        emit classNamesChanged();
        emit currentClassNameChanged();
        emit currentPolygonsChanged();
        emit selectedPolygonIndexChanged();
        emit selectedAnnotationClassNameChanged();

        const QString rememberedProject =
            QFileInfo(m_systemService.loadAppValue(kLastProjectFileKey).toString()).absoluteFilePath();
        int targetIndex = 0;
        if (QFileInfo(m_systemService.projectFilePath()).absoluteFilePath() == rememberedProject) {
            targetIndex = m_systemService.loadAppValue(kLastImageIndexKey, 0).toInt();
        }

        if (!m_imagePaths.isEmpty()) {
            targetIndex = qBound(0, targetIndex, m_imagePaths.size() - 1);
            selectImage(targetIndex);
        } else {
            emit currentImageChanged();
        }

        saveSessionState();
        setStatus(QStringLiteral("Project loaded: %1").arg(m_systemService.projectFilePath()));
        return true;
    }
    if (op == QStringLiteral("save_as")) {
        if (m_systemService.projectFilePath().isEmpty()) {
            setStatus(QStringLiteral("Create or load a project first"));
            return false;
        }

        const QString newPath = normalizePath(value);
        if (newPath.isEmpty()) {
            setStatus(QStringLiteral("Invalid target config path"));
            return false;
        }

        if (!m_systemService.createProject(newPath)) {
            setStatus(QStringLiteral("Failed to create target config"));
            return false;
        }

        m_projectName = QFileInfo(m_systemService.projectFilePath()).completeBaseName();
        emit projectChanged();

        if (!saveProjectData()) {
            setStatus(QStringLiteral("Failed to save copied config"));
            return false;
        }

        saveSessionState();
        setStatus(QStringLiteral("Saved as: %1").arg(m_systemService.projectFilePath()));
        return true;
    }

    setStatus(QStringLiteral("Unknown project operation: %1").arg(operationKey));
    return false;
}

bool Control::projectAction(const QString &action, const QVariantMap &payload)
{
    const QString opKey = payload.value(QStringLiteral("key")).toString();
    if (!opKey.trimmed().isEmpty()) {
        return executeProjectOperation(opKey, payload.value(QStringLiteral("value")), payload);
    }

    switch (toProjectTask(m_labelingService.canonicalProjectAction(action))) {
    case ProjectTask::Create:
        return executeProjectOperation(QStringLiteral("create"),
                                       payload.value(QStringLiteral("preferredName")),
                                       payload);
    case ProjectTask::Load:
        return executeProjectOperation(QStringLiteral("load"),
                                       payload.value(QStringLiteral("projectPath")),
                                       payload);
    case ProjectTask::SaveAs:
        return executeProjectOperation(QStringLiteral("save_as"),
                                       payload.value(QStringLiteral("projectPath")),
                                       payload);
    case ProjectTask::Unknown:
        break;
    }

    setStatus(QStringLiteral("Unknown project action: %1").arg(action));
    return false;
}

void Control::clearImageWorkspaceForImport()
{
    m_imagePaths.clear();
    m_annotations.clear();
    m_currentImageIndex = -1;
    m_draftPoints.clear();
    m_selectedPolygonIndex = -1;
    m_draftModeActive = false;
    emit draftPointsChanged();
    emit currentImageChanged();
    emit currentPolygonsChanged();
    emit selectedPolygonIndexChanged();
    emit selectedAnnotationClassNameChanged();
    resetAnnotationFieldsToDefault();
}

bool Control::importImagePaths(const QStringList &paths, bool clearExisting, const QString &emptyImportStatus)
{
    if (m_systemService.projectFilePath().isEmpty()) {
        setStatus(QStringLiteral("Create or load a project first"));
        return false;
    }

    if (clearExisting) {
        clearImageWorkspaceForImport();
    }

    int importedCount = 0;
    int updatedCount = 0;
    bool changed = false;

    for (const QString &pathEntry : paths) {
        const QString path = normalizePath(pathEntry);
        if (path.isEmpty()) {
            continue;
        }

        const QString suffix = QStringLiteral(".") + QFileInfo(path).suffix().toLower();
        if (!m_labelingService.supportedExtensions().contains(suffix)) {
            continue;
        }

        if (m_labelingService.upsertImagePathByFilename(m_imagePaths, path, &importedCount, &updatedCount)) {
            changed = true;
        }
    }

    if (!changed) {
        setStatus(emptyImportStatus);
        return false;
    }

    emit imagePathsChanged();
    if (m_currentImageIndex < 0) {
        selectImage(0);
    } else {
        loadImageToView(m_imagePaths.at(m_currentImageIndex));
    }

    saveProjectData();
    saveLastImageIndex();
    setStatus(QStringLiteral("Imported %1 images, updated %2 by filename").arg(importedCount).arg(updatedCount));
    return true;
}

bool Control::selectImage(int index)
{
    if (index < 0 || index >= m_imagePaths.size()) {
        setStatus(QStringLiteral("Image index out of range"));
        return false;
    }

    m_currentImageIndex = index;
    m_draftPoints.clear();
    m_draftModeActive = false;
    m_selectedPolygonIndex = -1;
    emit draftPointsChanged();
    emit currentImageChanged();
    emit currentPolygonsChanged();
    emit selectedPolygonIndexChanged();
    emit selectedAnnotationClassNameChanged();
    resetAnnotationFieldsToDefault();

    if (!loadImageToView(m_imagePaths.at(index))) {
        return false;
    }

    saveLastImageIndex();
    return true;
}

bool Control::navigateImage(ImageNavigation navigation)
{
    if (m_imagePaths.isEmpty()) {
        return false;
    }

    int targetIndex = m_currentImageIndex;
    switch (navigation) {
    case ImageNavigation::First:
        targetIndex = 0;
        break;
    case ImageNavigation::Last:
        targetIndex = m_imagePaths.size() - 1;
        break;
    case ImageNavigation::Next:
        targetIndex = qMin(m_currentImageIndex + 1, m_imagePaths.size() - 1);
        break;
    case ImageNavigation::Previous:
        targetIndex = qMax(0, m_currentImageIndex - 1);
        break;
    case ImageNavigation::Next10:
        targetIndex = qMin(m_currentImageIndex + 10, m_imagePaths.size() - 1);
        break;
    case ImageNavigation::Previous10:
        targetIndex = qMax(0, m_currentImageIndex - 10);
        break;
    }

    return selectImage(targetIndex);
}

bool Control::imageAction(const QString &action, const QVariantMap &payload)
{
    const bool clearExisting = payload.value(QStringLiteral("clearExisting"), false).toBool();
    switch (toImageTask(m_labelingService.canonicalImageAction(action))) {
    case ImageTask::ImportFolder: {
        const QString folderPath = normalizePath(payload.value(QStringLiteral("folderPath")));
        QDir dir(folderPath);
        if (!dir.exists()) {
            setStatus(QStringLiteral("Folder does not exist"));
            return false;
        }
        QStringList paths;
        QDirIterator it(folderPath, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            paths.push_back(it.next());
        }
        return importImagePaths(paths, clearExisting, QStringLiteral("No new images imported from folder"));
    }
    case ImageTask::ImportFiles: {
        QVariantList entries;
        const QVariant fileListValue = payload.value(QStringLiteral("fileList"));
        if (fileListValue.typeId() == QMetaType::QVariantList) {
            entries = fileListValue.toList();
        } else {
            const QString maybeSingle = normalizePath(fileListValue);
            if (!maybeSingle.isEmpty()) {
                entries.push_back(maybeSingle);
            }
        }
        QStringList paths;
        paths.reserve(entries.size());
        for (const QVariant &entry : entries) {
            paths.push_back(normalizePath(entry));
        }
        return importImagePaths(paths, clearExisting, QStringLiteral("No new images imported"));
    }
    case ImageTask::Select:
        return selectImage(payload.value(QStringLiteral("index"), -1).toInt());
    case ImageTask::First:
        return navigateImage(ImageNavigation::First);
    case ImageTask::Last:
        return navigateImage(ImageNavigation::Last);
    case ImageTask::Next:
        return navigateImage(ImageNavigation::Next);
    case ImageTask::Previous:
        return navigateImage(ImageNavigation::Previous);
    case ImageTask::Next10:
        return navigateImage(ImageNavigation::Next10);
    case ImageTask::Previous10:
        return navigateImage(ImageNavigation::Previous10);
    case ImageTask::Unknown:
        break;
    }

    setStatus(QStringLiteral("Unknown image action: %1").arg(action));
    return false;
}

QVariantList Control::listAction(const QString &action, const QVariantMap &payload) const
{
    Q_UNUSED(payload)
    const QString task = m_labelingService.canonicalListAction(action);
    if (task == QStringLiteral("image_summary")) {
        return imageSummaryList();
    }
    if (task == QStringLiteral("annotation_current")) {
        return annotationListForCurrentImage();
    }
    if (task == QStringLiteral("annotation_all")) {
        return annotationListForAllImages();
    }
    return {};
}

QVariantList Control::imageSummaryList() const
{
    QVariantList list;
    list.reserve(m_imagePaths.size());

    for (int imageIndex = 0; imageIndex < m_imagePaths.size(); ++imageIndex) {
        const QString &path = m_imagePaths.at(imageIndex);
        const QList<AnnotationData> annList = m_annotations.value(m_labelingService.annotationKeyForPath(path));
        int labelCount = 0;
        int predictCount = 0;
        int goodCount = 0;
        for (const AnnotationData &ann : annList) {
            const QString kind = m_labelingService.normalizedKind(ann.kind);
            if (kind == QStringLiteral("predict")) {
                ++predictCount;
            } else if (kind == QStringLiteral("good")) {
                ++goodCount;
            } else {
                ++labelCount;
            }
        }

        QVariantMap row;
        row.insert(QStringLiteral("imageIndex"), imageIndex);
        row.insert(QStringLiteral("fileName"), QFileInfo(path).fileName());
        row.insert(QStringLiteral("path"), path);
        row.insert(QStringLiteral("labelCount"), labelCount);
        row.insert(QStringLiteral("predictCount"), predictCount);
        row.insert(QStringLiteral("goodCount"), goodCount);
        list.push_back(row);
    }

    return list;
}

QVariantList Control::annotationListForCurrentImage() const
{
    QVariantList list;
    if (m_currentImageIndex < 0 || m_currentImageIndex >= m_imagePaths.size()) {
        return list;
    }

    const QString path = m_imagePaths.at(m_currentImageIndex);
    const QString fileName = QFileInfo(path).fileName();
    const QList<AnnotationData> annList = m_annotations.value(m_labelingService.annotationKeyForPath(path));
    list.reserve(annList.size());

    for (int annIndex = 0; annIndex < annList.size(); ++annIndex) {
        const AnnotationData &ann = annList.at(annIndex);
        QVariantMap row;
        row.insert(QStringLiteral("imageIndex"), m_currentImageIndex);
        row.insert(QStringLiteral("annotationIndex"), annIndex);
        row.insert(QStringLiteral("imageFileName"), fileName);
        row.insert(QStringLiteral("className"), ann.className);
        row.insert(QStringLiteral("type"), m_labelingService.normalizedKind(ann.kind));
        row.insert(QStringLiteral("level"), m_labelingService.normalizedSeverity(ann.severity));
        row.insert(QStringLiteral("split"), m_labelingService.normalizedSplit(ann.split));
        row.insert(QStringLiteral("remarks"), ann.remarks);
        list.push_back(row);
    }

    return list;
}

QVariantList Control::annotationListForAllImages() const
{
    QVariantList list;
    for (int imageIndex = 0; imageIndex < m_imagePaths.size(); ++imageIndex) {
        const QString &path = m_imagePaths.at(imageIndex);
        const QString fileName = QFileInfo(path).fileName();
        const QList<AnnotationData> annList = m_annotations.value(m_labelingService.annotationKeyForPath(path));
        for (int annIndex = 0; annIndex < annList.size(); ++annIndex) {
            const AnnotationData &ann = annList.at(annIndex);
            QVariantMap row;
            row.insert(QStringLiteral("imageIndex"), imageIndex);
            row.insert(QStringLiteral("annotationIndex"), annIndex);
            row.insert(QStringLiteral("imageFileName"), fileName);
            row.insert(QStringLiteral("className"), ann.className);
            row.insert(QStringLiteral("type"), m_labelingService.normalizedKind(ann.kind));
            row.insert(QStringLiteral("level"), m_labelingService.normalizedSeverity(ann.severity));
            row.insert(QStringLiteral("split"), m_labelingService.normalizedSplit(ann.split));
            row.insert(QStringLiteral("remarks"), ann.remarks);
            list.push_back(row);
        }
    }

    return list;
}

bool Control::selectAnnotationFromList(int imageIndex, int annotationIndex)
{
    if (!selectImage(imageIndex)) {
        return false;
    }

    const int maxIndex = currentImageAnnotations().size() - 1;
    const int clamped = (annotationIndex < 0 || annotationIndex > maxIndex) ? -1 : annotationIndex;
    m_selectedPolygonIndex = clamped;
    m_draftModeActive = false;
    emit selectedPolygonIndexChanged();
    emit selectedAnnotationClassNameChanged();

    const QList<AnnotationData> annList = currentImageAnnotations();
    if (m_selectedPolygonIndex >= 0 && m_selectedPolygonIndex < annList.size()) {
        const AnnotationData &ann = annList.at(m_selectedPolygonIndex);
        setAnnotationFields(ann.remarks, ann.kind, ann.severity, ann.split);
    } else {
        resetAnnotationFieldsToDefault();
    }

    if (clamped != annotationIndex) {
        setStatus(QStringLiteral("Annotation index out of range"));
        return false;
    }

    return true;
}

bool Control::importClassificationFromFolder(const QVariant &folderUrlOrPath, bool clearExisting)
{
    if (m_systemService.projectFilePath().isEmpty()) {
        setStatus(QStringLiteral("Create or load a project first"));
        return false;
    }

    const QString rootFolder = normalizePath(folderUrlOrPath);
    QDir baseDir(rootFolder);
    if (!baseDir.exists()) {
        setStatus(QStringLiteral("Classification folder does not exist"));
        return false;
    }

    setIoFolderPath(rootFolder);

    if (clearExisting) {
        clearImageWorkspaceForImport();
    }

    int addedImages = 0;
    int updatedImages = 0;
    int labeledImages = 0;
    bool changed = false;
    bool classesChanged = false;

    const QFileInfoList classDirs =
        baseDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable, QDir::Name | QDir::IgnoreCase);

    QList<QPair<QString, QString>> classSources;
    for (const QFileInfo &classDirInfo : classDirs) {
        classSources.push_back(qMakePair(classDirInfo.fileName().trimmed(), classDirInfo.absoluteFilePath()));
    }

    // Also allow selecting a single class folder directly (images in selected folder root).
    const QFileInfoList rootFiles =
        baseDir.entryInfoList(QDir::Files | QDir::NoSymLinks | QDir::Readable, QDir::Name | QDir::IgnoreCase);
    bool hasSupportedRootImage = false;
    for (const QFileInfo &fileInfo : rootFiles) {
        const QString suffix = QStringLiteral(".") + fileInfo.suffix().toLower();
        if (m_labelingService.supportedExtensions().contains(suffix)) {
            hasSupportedRootImage = true;
            break;
        }
    }
    if (classSources.isEmpty() && hasSupportedRootImage) {
        const QString directClassName = baseDir.dirName().trimmed().isEmpty() ? QStringLiteral("default")
                                                                               : baseDir.dirName().trimmed();
        classSources.push_back(qMakePair(directClassName, baseDir.absolutePath()));
    }

    if (classSources.isEmpty()) {
        setStatus(QStringLiteral("No class folders found. Expected: <root>/<class_name>/*.png"));
        return false;
    }

    for (const auto &classSource : classSources) {
        const QString className = classSource.first;
        if (className.isEmpty()) {
            continue;
        }

        if (!m_classNames.contains(className)) {
            m_classNames.push_back(className);
            classesChanged = true;
        }

        QDirIterator it(classSource.second,
                        QDir::Files | QDir::NoSymLinks,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString imagePath = it.next();
            const QString suffix = QStringLiteral(".") + QFileInfo(imagePath).suffix().toLower();
            if (!m_labelingService.supportedExtensions().contains(suffix)) {
                continue;
            }

            const bool pathListChanged = m_labelingService.upsertImagePathByFilename(
                m_imagePaths, imagePath, &addedImages, &updatedImages);
            if (!pathListChanged && !m_imagePaths.contains(imagePath)) {
                continue;
            }

            const QString annotationKey = m_labelingService.annotationKeyForPath(imagePath);
            QImageReader reader(imagePath);
            const QSize imageSize = reader.size();
            if (imageSize.width() <= 0 || imageSize.height() <= 0) {
                continue;
            }

            const qreal left = 0.0;
            const qreal top = 0.0;
            const qreal right = qMax<qreal>(0.0, imageSize.width() - 1.0);
            const qreal bottom = qMax<qreal>(0.0, imageSize.height() - 1.0);

            AnnotationData ann;
            ann.className = className;
            ann.kind = QStringLiteral("label");
            ann.severity = QStringLiteral("normal");
            ann.split = QStringLiteral("none");
            ann.points = {
                QPointF(left, top),
                QPointF(right, top),
                QPointF(right, bottom),
                QPointF(left, bottom),
                QPointF(left, top)
            };

            QList<AnnotationData> &annList = m_annotations[annotationKey];
            bool alreadyExists = false;
            const QList<AnnotationData> existingAnnotations = annList;
            for (const AnnotationData &existing : existingAnnotations) {
                if (existing.className != ann.className || existing.points.size() != ann.points.size()) {
                    continue;
                }

                bool samePoints = true;
                for (int i = 0; i < existing.points.size(); ++i) {
                    if (existing.points.at(i) != ann.points.at(i)) {
                        samePoints = false;
                        break;
                    }
                }
                if (samePoints) {
                    alreadyExists = true;
                    break;
                }
            }

            if (!alreadyExists) {
                if (clearExisting) {
                    // Replace mode already cleared all annotations before import.
                    annList = {ann};
                } else {
                    // Append mode keeps existing annotations and adds imported classification.
                    annList.push_back(ann);
                }
                ++labeledImages;
                changed = true;
            }
        }
    }

    if (!changed && !classesChanged) {
        setStatus(QStringLiteral("No classification images imported"));
        return false;
    }

    if (classesChanged) {
        emit classNamesChanged();
    }
    emit imagePathsChanged();

    if (m_currentImageIndex < 0 && !m_imagePaths.isEmpty()) {
        selectImage(0);
    } else if (m_currentImageIndex >= 0 && m_currentImageIndex < m_imagePaths.size()) {
        loadImageToView(m_imagePaths.at(m_currentImageIndex));
        emit currentPolygonsChanged();
    }

    if (!saveProjectData()) {
        setStatus(QStringLiteral("Classification import completed, but save failed"));
        return false;
    }

    saveLastImageIndex();
    setStatus(QStringLiteral("Classification import: %1 added, %2 updated, %3 labeled")
                  .arg(addedImages)
                  .arg(updatedImages)
                  .arg(labeledImages));
    return true;
}

bool Control::exportClassificationToFolder(const QVariant &folderUrlOrPath)
{
    if (m_systemService.projectFilePath().isEmpty()) {
        setStatus(QStringLiteral("Create or load a project first"));
        return false;
    }

    const QString folderPath = normalizePath(folderUrlOrPath);
    if (folderPath.isEmpty()) {
        setStatus(QStringLiteral("Select a folder first"));
        return false;
    }

    QDir dir(folderPath);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        setStatus(QStringLiteral("Failed to create export folder"));
        return false;
    }

    setIoFolderPath(folderPath);

    int exportedFiles = 0;
    int skippedFiles = 0;

    for (const QString &imagePath : m_imagePaths) {
        const QFileInfo srcInfo(imagePath);
        if (!srcInfo.exists() || !srcInfo.isFile()) {
            ++skippedFiles;
            continue;
        }

        const QList<AnnotationData> annList = m_annotations.value(m_labelingService.annotationKeyForPath(imagePath));
        QSet<QString> classesForImage;
        for (const AnnotationData &ann : annList) {
            const QString cls = ann.className.trimmed();
            if (!cls.isEmpty()) {
                classesForImage.insert(cls);
            }
        }

        if (classesForImage.isEmpty()) {
            ++skippedFiles;
            continue;
        }

        for (const QString &className : classesForImage) {
            if (!dir.mkpath(className)) {
                ++skippedFiles;
                continue;
            }

            const QString classDirPath = dir.filePath(className);
            const QString baseName = srcInfo.completeBaseName();
            const QString suffix = srcInfo.suffix();

            QString targetFilePath = QDir(classDirPath).filePath(srcInfo.fileName());
            if (QFileInfo::exists(targetFilePath)) {
                int serial = 1;
                do {
                    const QString candidateName =
                        suffix.isEmpty()
                            ? QStringLiteral("%1_%2").arg(baseName).arg(serial)
                            : QStringLiteral("%1_%2.%3").arg(baseName).arg(serial).arg(suffix);
                    targetFilePath = QDir(classDirPath).filePath(candidateName);
                    ++serial;
                } while (QFileInfo::exists(targetFilePath));
            }

            if (QFile::copy(imagePath, targetFilePath)) {
                ++exportedFiles;
            } else {
                ++skippedFiles;
            }
        }
    }

    if (exportedFiles == 0) {
        setStatus(QStringLiteral("No classification images exported"));
        return false;
    }

    setStatus(QStringLiteral("Classification export: %1 files exported, %2 skipped")
                  .arg(exportedFiles)
                  .arg(skippedFiles));
    return true;
}

bool Control::ioAction(const QString &action, const QVariantMap &payload)
{
    switch (toIoTask(m_labelingService.canonicalIoAction(action))) {
    case IoTask::SetFolder:
        setIoFolderPath(payload.value(QStringLiteral("folderPath")).toString());
        return true;
    case IoTask::SetVisibility: {
        const QString key = payload.value(QStringLiteral("key")).toString().trimmed().toLower();
        const bool enabled = payload.value(QStringLiteral("enabled"), true).toBool();
        auto applyVisibility = [this, enabled](bool &stateField, const QString &persistKey) {
            if (stateField == enabled) {
                return true;
            }
            stateField = enabled;
            m_systemService.saveAppValue(persistKey, stateField);
            emit typeVisibilityChanged();
            emit currentPolygonsChanged();
            return true;
        };
        if (key == QStringLiteral("label")) {
            return applyVisibility(m_showLabel, kShowLabelKey);
        }
        if (key == QStringLiteral("predict")) {
            return applyVisibility(m_showPredict, kShowPredictKey);
        }
        if (key == QStringLiteral("good")) {
            return applyVisibility(m_showGood, kShowGoodKey);
        }
        setStatus(QStringLiteral("Unknown visibility key: %1").arg(key));
        return false;
    }
    case IoTask::ImportClassification:
        return importClassificationFromFolder(payload.value(QStringLiteral("folderPath")),
                                              payload.value(QStringLiteral("clearExisting"), false).toBool());
    case IoTask::ExportClassification:
        return exportClassificationToFolder(payload.value(QStringLiteral("folderPath")));
    case IoTask::Unknown:
        break;
    }

    setStatus(QStringLiteral("Unknown I/O action: %1").arg(action));
    return false;
}

bool Control::annotationAction(const QString &action, const QVariantMap &payload)
{
    const QString opKey = payload.value(QStringLiteral("key")).toString();
    if (!opKey.trimmed().isEmpty()) {
        return executeAnnotationOperation(opKey, payload.value(QStringLiteral("value")), payload);
    }

    switch (toAnnotationTask(m_labelingService.canonicalAnnotationAction(action))) {
    case AnnotationTask::StartNew:
        return executeAnnotationOperation(QStringLiteral("start_new"), QVariant(), payload);
    case AnnotationTask::AddClass:
        return executeAnnotationOperation(QStringLiteral("add_class"), payload.value(QStringLiteral("className")), payload);
    case AnnotationTask::DeleteClass:
        return executeAnnotationOperation(QStringLiteral("delete_class"), payload.value(QStringLiteral("className")), payload);
    case AnnotationTask::SetCurrentClass:
        return executeAnnotationOperation(QStringLiteral("set_current_class"),
                                          payload.value(QStringLiteral("className")),
                                          payload);
    case AnnotationTask::SetField:
        return executeAnnotationOperation(QStringLiteral("set_field"), payload, payload);
    case AnnotationTask::SetKind:
        return executeAnnotationOperation(QStringLiteral("kind"), payload.value(QStringLiteral("kind")), payload);
    case AnnotationTask::SetSeverity:
        return executeAnnotationOperation(QStringLiteral("severity"), payload.value(QStringLiteral("severity")), payload);
    case AnnotationTask::SetSplit:
        return executeAnnotationOperation(QStringLiteral("split"), payload.value(QStringLiteral("split")), payload);
    case AnnotationTask::SetRemarks:
        return executeAnnotationOperation(QStringLiteral("remarks"), payload.value(QStringLiteral("remarks")), payload);
    case AnnotationTask::AddPoint:
        return executeAnnotationOperation(QStringLiteral("add_point"), payload, payload);
    case AnnotationTask::UpdateDraftPoint:
        return executeAnnotationOperation(QStringLiteral("update_draft_point"), payload, payload);
    case AnnotationTask::Save:
        return executeAnnotationOperation(QStringLiteral("save"), QVariant(), payload);
    case AnnotationTask::Undo:
        return executeAnnotationOperation(QStringLiteral("undo"), QVariant(), payload);
    case AnnotationTask::Clear:
        return executeAnnotationOperation(QStringLiteral("clear"), QVariant(), payload);
    case AnnotationTask::Select:
        return executeAnnotationOperation(QStringLiteral("select"), payload.value(QStringLiteral("polygonIndex")), payload);
    case AnnotationTask::Delete:
        return executeAnnotationOperation(QStringLiteral("delete"), QVariant(), payload);
    case AnnotationTask::UpdateClass:
        return executeAnnotationOperation(QStringLiteral("update_class"),
                                          payload.value(QStringLiteral("className")),
                                          payload);
    case AnnotationTask::UpdateDetails:
        return executeAnnotationOperation(QStringLiteral("update_details"),
                                          payload.value(QStringLiteral("className")),
                                          payload);
    case AnnotationTask::UpdatePoint:
        return executeAnnotationOperation(QStringLiteral("update_point"), payload, payload);
    case AnnotationTask::SelectFromList:
        return executeAnnotationOperation(QStringLiteral("select_from_list"), payload, payload);
    case AnnotationTask::Unknown:
        break;
    }

    setStatus(QStringLiteral("Unknown annotation action: %1").arg(action));
    return false;
}

bool Control::executeAnnotationOperation(const QString &operationKey, const QVariant &value, const QVariantMap &payload)
{
    const QString op = operationKey.trimmed().toLower();

    if (op == QStringLiteral("start_new") || op == QStringLiteral("new")) {
        m_draftModeActive = true;
        if (!m_draftPoints.isEmpty()) {
            m_draftPoints.clear();
            emit draftPointsChanged();
        }
        if (m_selectedPolygonIndex != -1) {
            m_selectedPolygonIndex = -1;
            emit selectedPolygonIndexChanged();
            emit selectedAnnotationClassNameChanged();
            resetAnnotationFieldsToDefault();
        }
        return true;
    }
    if (op == QStringLiteral("add_class")) {
        const QString className = value.toString().isEmpty() ? payload.value(QStringLiteral("className")).toString()
                                                             : value.toString();
        const QString trimmed = className.trimmed();
        if (trimmed.isEmpty()) {
            setStatus(QStringLiteral("Class name cannot be empty"));
            return false;
        }
        if (!m_classNames.contains(trimmed)) {
            m_classNames.push_back(trimmed);
            emit classNamesChanged();
        }
        if (m_currentClassName != trimmed) {
            m_currentClassName = trimmed;
            emit currentClassNameChanged();
        }
        saveProjectData();
        return true;
    }
    if (op == QStringLiteral("delete_class")) {
        const QString className = value.toString().isEmpty() ? payload.value(QStringLiteral("className")).toString()
                                                             : value.toString();
        const QString trimmed = className.trimmed();
        if (trimmed.isEmpty()) {
            setStatus(QStringLiteral("Class name cannot be empty"));
            return false;
        }
        if (!m_classNames.contains(trimmed)) {
            setStatus(QStringLiteral("Class not found"));
            return false;
        }
        const QString defaultClass = QStringLiteral("default");
        if (trimmed == defaultClass) {
            setStatus(QStringLiteral("Default class cannot be deleted"));
            return false;
        }
        if (!m_classNames.contains(defaultClass)) {
            m_classNames.prepend(defaultClass);
            emit classNamesChanged();
        }
        for (auto it = m_annotations.begin(); it != m_annotations.end(); ++it) {
            QList<AnnotationData> &annList = it.value();
            for (AnnotationData &ann : annList) {
                if (ann.className == trimmed) {
                    ann.className = defaultClass;
                }
            }
        }
        m_classNames.removeAll(trimmed);
        emit classNamesChanged();
        if (m_currentClassName == trimmed) {
            m_currentClassName = defaultClass;
            emit currentClassNameChanged();
        }
        emit currentPolygonsChanged();
        emit selectedAnnotationClassNameChanged();
        if (!saveProjectData()) {
            setStatus(QStringLiteral("Failed to delete class"));
            return false;
        }
        setStatus(QStringLiteral("Class deleted, linked annotations moved to default"));
        return true;
    }
    if (op == QStringLiteral("set_current_class")) {
        const QString className = value.toString().isEmpty() ? payload.value(QStringLiteral("className")).toString()
                                                             : value.toString();
        const QString trimmed = className.trimmed();
        if (trimmed.isEmpty() || m_currentClassName == trimmed) {
            return true;
        }
        m_currentClassName = trimmed;
        emit currentClassNameChanged();
        return true;
    }
    if (op == QStringLiteral("set_field")) {
        const QVariantMap fieldMap = value.toMap().isEmpty() ? payload : value.toMap();
        const QString fieldKey = fieldMap.value(QStringLiteral("key")).toString();
        const QVariant fieldValue = fieldMap.value(QStringLiteral("value"));
        return executeAnnotationOperation(fieldKey, fieldValue, payload);
    }
    if (op == QStringLiteral("kind") || op == QStringLiteral("severity") || op == QStringLiteral("level")
        || op == QStringLiteral("split") || op == QStringLiteral("remarks")) {
        const QVariant fieldValue = value.isValid() ? value : payload.value(op);
        if (op == QStringLiteral("remarks")) {
            setAnnotationFields(fieldValue.toString(), m_annotationKind, m_annotationSeverity, m_annotationSplit);
            return true;
        }
        if (op == QStringLiteral("kind")) {
            setAnnotationFields(m_annotationRemarks, fieldValue.toString(), m_annotationSeverity, m_annotationSplit);
            return true;
        }
        if (op == QStringLiteral("severity") || op == QStringLiteral("level")) {
            setAnnotationFields(m_annotationRemarks, m_annotationKind, fieldValue.toString(), m_annotationSplit);
            return true;
        }
        setAnnotationFields(m_annotationRemarks, m_annotationKind, m_annotationSeverity, fieldValue.toString());
        return true;
    }
    if (op == QStringLiteral("add_point")) {
        const QVariantMap pointMap = value.toMap().isEmpty() ? payload : value.toMap();
        if (m_currentImageIndex < 0) {
            setStatus(QStringLiteral("Select an image first"));
            return false;
        }
        if (!m_draftModeActive) {
            setStatus(QStringLiteral("Click New before adding points"));
            return false;
        }
        m_draftPoints.push_back(QPointF(pointMap.value(QStringLiteral("x")).toDouble(),
                                        pointMap.value(QStringLiteral("y")).toDouble()));
        if (m_selectedPolygonIndex != -1) {
            m_selectedPolygonIndex = -1;
            emit selectedPolygonIndexChanged();
            emit selectedAnnotationClassNameChanged();
            resetAnnotationFieldsToDefault();
        }
        emit draftPointsChanged();
        return true;
    }
    if (op == QStringLiteral("update_draft_point")) {
        const QVariantMap pointMap = value.toMap().isEmpty() ? payload : value.toMap();
        if (!m_draftModeActive) {
            return false;
        }
        const int pointIndex = pointMap.value(QStringLiteral("pointIndex"), -1).toInt();
        if (pointIndex < 0 || pointIndex >= m_draftPoints.size()) {
            return false;
        }
        m_draftPoints[pointIndex] =
            QPointF(pointMap.value(QStringLiteral("x")).toDouble(), pointMap.value(QStringLiteral("y")).toDouble());
        emit draftPointsChanged();
        return true;
    }
    if (op == QStringLiteral("save")) {
        if (m_currentImageIndex < 0 || m_currentImageIndex >= m_imagePaths.size()) {
            setStatus(QStringLiteral("Select an image first"));
            return false;
        }
        if (m_currentClassName.trimmed().isEmpty()) {
            setStatus(QStringLiteral("Set a class name before saving"));
            return false;
        }
        if (m_draftPoints.size() < 3) {
            setStatus(QStringLiteral("At least 3 points are required"));
            return false;
        }

        AnnotationData ann;
        ann.className = m_currentClassName;
        ann.remarks = m_annotationRemarks;
        ann.kind = m_labelingService.normalizedKind(m_annotationKind);
        ann.severity = m_labelingService.normalizedSeverity(m_annotationSeverity);
        ann.split = m_labelingService.normalizedSplit(m_annotationSplit);
        ann.points = m_draftPoints;
        if (!ann.points.isEmpty() && ann.points.first() != ann.points.last()) {
            ann.points.push_back(ann.points.first());
        }

        const QString key = m_labelingService.annotationKeyForPath(m_imagePaths.at(m_currentImageIndex));
        m_annotations[key].push_back(ann);
        m_selectedPolygonIndex = m_annotations[key].size() - 1;
        emit selectedPolygonIndexChanged();
        emit selectedAnnotationClassNameChanged();
        emit currentPolygonsChanged();

        if (!m_draftPoints.isEmpty()) {
            m_draftPoints.clear();
            emit draftPointsChanged();
        }
        m_draftModeActive = false;

        if (!saveProjectData()) {
            setStatus(QStringLiteral("Failed to save annotation"));
            return false;
        }
        setStatus(QStringLiteral("Annotation saved"));
        return true;
    }
    if (op == QStringLiteral("undo")) {
        if (m_draftPoints.isEmpty()) {
            return false;
        }
        m_draftPoints.removeLast();
        emit draftPointsChanged();
        return true;
    }
    if (op == QStringLiteral("clear")) {
        if (!m_draftPoints.isEmpty()) {
            m_draftPoints.clear();
            emit draftPointsChanged();
        }
        return true;
    }
    if (op == QStringLiteral("select")) {
        const int polygonIndex = value.isValid() ? value.toInt() : payload.value(QStringLiteral("polygonIndex"), -1).toInt();
        const int maxIndex = currentImageAnnotations().size() - 1;
        const int clamped = (polygonIndex < 0 || polygonIndex > maxIndex) ? -1 : polygonIndex;
        if (m_selectedPolygonIndex == clamped) {
            return true;
        }
        m_selectedPolygonIndex = clamped;
        m_draftModeActive = false;
        emit selectedPolygonIndexChanged();
        emit selectedAnnotationClassNameChanged();
        const QList<AnnotationData> annList = currentImageAnnotations();
        if (m_selectedPolygonIndex >= 0 && m_selectedPolygonIndex < annList.size()) {
            const AnnotationData &ann = annList.at(m_selectedPolygonIndex);
            setAnnotationFields(ann.remarks, ann.kind, ann.severity, ann.split);
        } else {
            resetAnnotationFieldsToDefault();
        }
        return true;
    }
    if (op == QStringLiteral("delete")) {
        if (m_currentImageIndex < 0 || m_currentImageIndex >= m_imagePaths.size()) {
            setStatus(QStringLiteral("Select an image first"));
            return false;
        }
        if (m_selectedPolygonIndex < 0) {
            setStatus(QStringLiteral("No annotation selected"));
            return false;
        }
        const QString key = m_labelingService.annotationKeyForPath(m_imagePaths.at(m_currentImageIndex));
        auto it = m_annotations.find(key);
        if (it == m_annotations.end()) {
            setStatus(QStringLiteral("No annotation selected"));
            return false;
        }
        QList<AnnotationData> &annList = it.value();
        if (m_selectedPolygonIndex >= annList.size()) {
            m_selectedPolygonIndex = -1;
            emit selectedPolygonIndexChanged();
            emit selectedAnnotationClassNameChanged();
            resetAnnotationFieldsToDefault();
            setStatus(QStringLiteral("No annotation selected"));
            return false;
        }
        annList.removeAt(m_selectedPolygonIndex);
        if (annList.isEmpty()) {
            m_annotations.remove(key);
        }
        m_selectedPolygonIndex = -1;
        emit selectedPolygonIndexChanged();
        emit selectedAnnotationClassNameChanged();
        emit currentPolygonsChanged();
        resetAnnotationFieldsToDefault();
        if (!saveProjectData()) {
            setStatus(QStringLiteral("Failed to delete annotation"));
            return false;
        }
        setStatus(QStringLiteral("Annotation deleted"));
        return true;
    }
    if (op == QStringLiteral("update_class")) {
        const QString className = value.toString().isEmpty() ? payload.value(QStringLiteral("className")).toString()
                                                             : value.toString();
        if (m_currentImageIndex < 0 || m_currentImageIndex >= m_imagePaths.size()) {
            setStatus(QStringLiteral("Select an image first"));
            return false;
        }
        if (m_selectedPolygonIndex < 0) {
            setStatus(QStringLiteral("No annotation selected"));
            return false;
        }
        const QString trimmed = className.trimmed();
        if (trimmed.isEmpty()) {
            setStatus(QStringLiteral("Class name cannot be empty"));
            return false;
        }
        if (!m_classNames.contains(trimmed)) {
            setStatus(QStringLiteral("Class not found. Add it first."));
            return false;
        }
        const QString key = m_labelingService.annotationKeyForPath(m_imagePaths.at(m_currentImageIndex));
        auto it = m_annotations.find(key);
        if (it == m_annotations.end() || m_selectedPolygonIndex >= it.value().size()) {
            setStatus(QStringLiteral("No annotation selected"));
            return false;
        }
        it.value()[m_selectedPolygonIndex].className = trimmed;
        emit currentPolygonsChanged();
        emit selectedAnnotationClassNameChanged();
        if (!saveProjectData()) {
            setStatus(QStringLiteral("Failed to update annotation class"));
            return false;
        }
        setStatus(QStringLiteral("Annotation class updated"));
        return true;
    }
    if (op == QStringLiteral("update_details")) {
        const QString className = value.toString().isEmpty() ? payload.value(QStringLiteral("className")).toString()
                                                             : value.toString();
        if (m_currentImageIndex < 0 || m_currentImageIndex >= m_imagePaths.size()) {
            setStatus(QStringLiteral("Select an image first"));
            return false;
        }
        if (m_selectedPolygonIndex < 0) {
            setStatus(QStringLiteral("No annotation selected"));
            return false;
        }
        const QString trimmed = className.trimmed();
        if (trimmed.isEmpty()) {
            setStatus(QStringLiteral("Class name cannot be empty"));
            return false;
        }
        if (!m_classNames.contains(trimmed)) {
            setStatus(QStringLiteral("Class not found. Add it first."));
            return false;
        }
        const QString key = m_labelingService.annotationKeyForPath(m_imagePaths.at(m_currentImageIndex));
        auto it = m_annotations.find(key);
        if (it == m_annotations.end() || m_selectedPolygonIndex >= it.value().size()) {
            setStatus(QStringLiteral("No annotation selected"));
            return false;
        }
        AnnotationData &ann = it.value()[m_selectedPolygonIndex];
        ann.className = trimmed;
        ann.remarks = m_annotationRemarks;
        ann.kind = m_labelingService.normalizedKind(m_annotationKind);
        ann.severity = m_labelingService.normalizedSeverity(m_annotationSeverity);
        ann.split = m_labelingService.normalizedSplit(m_annotationSplit);
        emit currentPolygonsChanged();
        emit selectedAnnotationClassNameChanged();
        emit annotationFieldsChanged();
        if (!saveProjectData()) {
            setStatus(QStringLiteral("Failed to update annotation"));
            return false;
        }
        setStatus(QStringLiteral("Annotation updated"));
        return true;
    }
    if (op == QStringLiteral("update_point")) {
        const QVariantMap pointMap = value.toMap().isEmpty() ? payload : value.toMap();
        const int polygonIndex = pointMap.value(QStringLiteral("polygonIndex"), -1).toInt();
        const int pointIndex = pointMap.value(QStringLiteral("pointIndex"), -1).toInt();
        if (m_currentImageIndex < 0 || m_currentImageIndex >= m_imagePaths.size()) {
            setStatus(QStringLiteral("Select an image first"));
            return false;
        }
        if (m_selectedPolygonIndex < 0 || polygonIndex != m_selectedPolygonIndex) {
            setStatus(QStringLiteral("Select a polygon to edit"));
            return false;
        }
        const QString key = m_labelingService.annotationKeyForPath(m_imagePaths.at(m_currentImageIndex));
        auto it = m_annotations.find(key);
        if (it == m_annotations.end()) {
            return false;
        }
        QList<AnnotationData> &annList = it.value();
        if (polygonIndex < 0 || polygonIndex >= annList.size()) {
            return false;
        }
        AnnotationData &ann = annList[polygonIndex];
        if (pointIndex < 0 || pointIndex >= ann.points.size()) {
            return false;
        }
        ann.points[pointIndex] = QPointF(pointMap.value(QStringLiteral("x")).toDouble(),
                                         pointMap.value(QStringLiteral("y")).toDouble());
        emit currentPolygonsChanged();
        if (!saveProjectData()) {
            setStatus(QStringLiteral("Failed to save edited annotation"));
            return false;
        }
        setStatus(QStringLiteral("Annotation point updated"));
        return true;
    }
    if (op == QStringLiteral("select_from_list")) {
        const QVariantMap listMap = value.toMap().isEmpty() ? payload : value.toMap();
        return selectAnnotationFromList(listMap.value(QStringLiteral("imageIndex"), -1).toInt(),
                                        listMap.value(QStringLiteral("annotationIndex"), -1).toInt());
    }

    setStatus(QStringLiteral("Unknown annotation operation: %1").arg(operationKey));
    return false;
}

QImage Control::matToQImage(const cv::Mat &mat)
{
    if (mat.empty()) {
        return QImage();
    }

    cv::Mat rgb;
    if (mat.type() == CV_8UC1) {
        cv::cvtColor(mat, rgb, cv::COLOR_GRAY2RGB);
    } else if (mat.type() == CV_8UC3) {
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
    } else if (mat.type() == CV_8UC4) {
        cv::cvtColor(mat, rgb, cv::COLOR_BGRA2RGBA);
    } else {
        return QImage();
    }

    return QImage(
               rgb.data,
               rgb.cols,
               rgb.rows,
               static_cast<int>(rgb.step),
               rgb.type() == CV_8UC4 ? QImage::Format_RGBA8888 : QImage::Format_RGB888)
        .copy();
}

QString Control::normalizePath(const QVariant &urlOrPath)
{
    return SystemService::normalizePath(urlOrPath);
}

bool Control::loadImageToView(const QString &path)
{
    const cv::Mat image = cv::imread(path.toStdString(), cv::IMREAD_UNCHANGED);
    if (image.empty()) {
        setStatus(QStringLiteral("Failed to read image: %1").arg(path));
        return false;
    }

    const QImage qimage = matToQImage(image);
    if (qimage.isNull()) {
        setStatus(QStringLiteral("Unsupported image format: %1").arg(path));
        return false;
    }

    emit imageReady(qimage);
    setStatus(QStringLiteral("Loaded image %1/%2").arg(m_currentImageIndex + 1).arg(m_imagePaths.size()));
    return true;
}

void Control::setStatus(const QString &status)
{
    if (m_status == status) {
        return;
    }

    m_status = status;
    emit statusChanged();
}

bool Control::saveProjectData()
{
    if (m_systemService.projectFilePath().isEmpty()) {
        return false;
    }

    QJsonObject root;
    root.insert(QStringLiteral("projectName"), m_projectName);

    QJsonArray classes;
    for (const QString &name : m_classNames) {
        classes.push_back(name);
    }
    root.insert(QStringLiteral("classes"), classes);

    QJsonArray images;
    for (const QString &path : m_imagePaths) {
        QJsonObject imgObj;
        imgObj.insert(QStringLiteral("path"), path);

        QJsonArray anns;
        const QList<AnnotationData> annList = m_annotations.value(m_labelingService.annotationKeyForPath(path));
        for (const AnnotationData &ann : annList) {
            QJsonObject annObj;
            annObj.insert(QStringLiteral("className"), ann.className);
            annObj.insert(QStringLiteral("remarks"), ann.remarks);
            annObj.insert(QStringLiteral("kind"), ann.kind);
            annObj.insert(QStringLiteral("severity"), ann.severity);
            annObj.insert(QStringLiteral("split"), ann.split);

            QJsonArray pts;
            for (const QPointF &pt : ann.points) {
                QJsonObject ptObj;
                ptObj.insert(QStringLiteral("x"), pt.x());
                ptObj.insert(QStringLiteral("y"), pt.y());
                pts.push_back(ptObj);
            }

            annObj.insert(QStringLiteral("points"), pts);
            anns.push_back(annObj);
        }

        imgObj.insert(QStringLiteral("annotations"), anns);
        images.push_back(imgObj);
    }

    root.insert(QStringLiteral("images"), images);

    return m_systemService.saveProject(root);
}

void Control::setAnnotationFields(const QString &remarks, const QString &kind, const QString &severity, const QString &split)
{
    const QString normalizedK = m_labelingService.normalizedKind(kind);
    const QString normalizedS = m_labelingService.normalizedSeverity(severity);
    const QString normalizedSp = m_labelingService.normalizedSplit(split);
    if (m_annotationRemarks == remarks && m_annotationKind == normalizedK && m_annotationSeverity == normalizedS
        && m_annotationSplit == normalizedSp) {
        return;
    }

    m_annotationRemarks = remarks;
    m_annotationKind = normalizedK;
    m_annotationSeverity = normalizedS;
    m_annotationSplit = normalizedSp;
    emit annotationFieldsChanged();
}

void Control::resetAnnotationFieldsToDefault()
{
    setAnnotationFields(QString(), QStringLiteral("label"), QStringLiteral("normal"), QStringLiteral("none"));
}

void Control::saveSessionState() const
{
    m_systemService.saveAppValue(kLastProjectFileKey, m_systemService.projectFilePath());
    m_systemService.saveAppValue(kLastImageIndexKey, m_currentImageIndex);
}

void Control::saveLastImageIndex() const
{
    m_systemService.saveAppValue(kLastImageIndexKey, m_currentImageIndex);
}

bool Control::restoreLastSession()
{
    const QString lastProject = m_systemService.loadAppValue(kLastProjectFileKey).toString();
    if (lastProject.trimmed().isEmpty()) {
        return false;
    }

    return executeProjectOperation(QStringLiteral("load"), lastProject, QVariantMap());
}

QList<Control::AnnotationData> Control::currentImageAnnotations() const
{
    if (m_currentImageIndex < 0 || m_currentImageIndex >= m_imagePaths.size()) {
        return {};
    }

    return m_annotations.value(m_labelingService.annotationKeyForPath(m_imagePaths.at(m_currentImageIndex)));
}

void Control::resetState()
{
    m_projectName.clear();
    m_imagePaths.clear();
    m_currentImageIndex = -1;
    m_classNames.clear();
    m_currentClassName.clear();
    m_annotationRemarks.clear();
    m_annotationKind = QStringLiteral("label");
    m_annotationSeverity = QStringLiteral("normal");
    m_annotationSplit = QStringLiteral("none");
    m_draftPoints.clear();
    m_annotations.clear();
    m_selectedPolygonIndex = -1;
    m_draftModeActive = false;

    emit imagePathsChanged();
    emit currentImageChanged();
    emit classNamesChanged();
    emit currentClassNameChanged();
    emit draftPointsChanged();
    emit currentPolygonsChanged();
    emit selectedPolygonIndexChanged();
    emit selectedAnnotationClassNameChanged();
    emit annotationFieldsChanged();
}
