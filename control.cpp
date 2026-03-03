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
}

Control::Control(QObject *parent)
    : QObject(parent)
    , m_systemService(this)
    , m_labelingService(&m_systemService, this)
    , m_trainService(this)
    , m_inferenceService(this)
{
    m_status = QStringLiteral("Create a project to start labeling");
    m_showLabel = m_systemService.loadAppValue(kShowLabelKey, true).toBool();
    m_showPredict = m_systemService.loadAppValue(kShowPredictKey, true).toBool();
    m_showGood = m_systemService.loadAppValue(kShowGoodKey, true).toBool();
    m_ioFolderPath = m_systemService.loadAppValue(kIoFolderPathKey).toString();
    if (!restoreLastSession()) {
        setStatus(QStringLiteral("Create a project to start labeling"));
    }
}

QString Control::status() const
{
    return m_status;
}

QString Control::projectName() const
{
    return m_projectName;
}

QString Control::projectFilePath() const
{
    return m_systemService.projectFilePath();
}

QStringList Control::imagePaths() const
{
    return m_imagePaths;
}

int Control::imageCount() const
{
    return m_imagePaths.size();
}

int Control::currentImageIndex() const
{
    return m_currentImageIndex;
}

QString Control::currentImagePath() const
{
    if (m_currentImageIndex < 0 || m_currentImageIndex >= m_imagePaths.size()) {
        return QString();
    }

    return m_imagePaths.at(m_currentImageIndex);
}

QStringList Control::classNames() const
{
    return m_classNames;
}

QString Control::currentClassName() const
{
    return m_currentClassName;
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

int Control::selectedPolygonIndex() const
{
    return m_selectedPolygonIndex;
}

QString Control::selectedAnnotationClassName() const
{
    const QList<AnnotationData> annList = currentImageAnnotations();
    if (m_selectedPolygonIndex < 0 || m_selectedPolygonIndex >= annList.size()) {
        return QString();
    }

    return annList.at(m_selectedPolygonIndex).className;
}

QString Control::annotationRemarks() const
{
    return m_annotationRemarks;
}

QString Control::annotationKind() const
{
    return m_annotationKind;
}

QString Control::annotationSeverity() const
{
    return m_annotationSeverity;
}

QString Control::annotationSplit() const
{
    return m_annotationSplit;
}

bool Control::showLabel() const
{
    return m_showLabel;
}

bool Control::showPredict() const
{
    return m_showPredict;
}

bool Control::showGood() const
{
    return m_showGood;
}

void Control::setAnnotationRemarks(const QString &remarks)
{
    if (m_annotationRemarks == remarks) {
        return;
    }

    m_annotationRemarks = remarks;
    emit annotationFieldsChanged();
}

void Control::setAnnotationKind(const QString &kind)
{
    const QString normalized = m_labelingService.normalizedKind(kind);
    if (m_annotationKind == normalized) {
        return;
    }

    m_annotationKind = normalized;
    emit annotationFieldsChanged();
}

void Control::setAnnotationSeverity(const QString &severity)
{
    const QString normalized = m_labelingService.normalizedSeverity(severity);
    if (m_annotationSeverity == normalized) {
        return;
    }

    m_annotationSeverity = normalized;
    emit annotationFieldsChanged();
}

void Control::setAnnotationSplit(const QString &split)
{
    const QString normalized = m_labelingService.normalizedSplit(split);
    if (m_annotationSplit == normalized) {
        return;
    }

    m_annotationSplit = normalized;
    emit annotationFieldsChanged();
}

void Control::setShowLabel(bool enabled)
{
    if (m_showLabel == enabled) {
        return;
    }

    m_showLabel = enabled;
    m_systemService.saveAppValue(kShowLabelKey, m_showLabel);
    emit typeVisibilityChanged();
    emit currentPolygonsChanged();
}

void Control::setShowPredict(bool enabled)
{
    if (m_showPredict == enabled) {
        return;
    }

    m_showPredict = enabled;
    m_systemService.saveAppValue(kShowPredictKey, m_showPredict);
    emit typeVisibilityChanged();
    emit currentPolygonsChanged();
}

void Control::setShowGood(bool enabled)
{
    if (m_showGood == enabled) {
        return;
    }

    m_showGood = enabled;
    m_systemService.saveAppValue(kShowGoodKey, m_showGood);
    emit typeVisibilityChanged();
    emit currentPolygonsChanged();
}

QString Control::ioFolderPath() const
{
    return m_ioFolderPath;
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

bool Control::createNewProject(const QString &preferredName)
{
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

bool Control::loadProjectFile(const QVariant &projectFileUrlOrPath)
{
    const QString path = normalizePath(projectFileUrlOrPath);
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
    for (const QJsonValue &value : classes) {
        const QString name = value.toString().trimmed();
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

bool Control::saveProjectAs(const QVariant &projectFileUrlOrPath)
{
    if (m_systemService.projectFilePath().isEmpty()) {
        setStatus(QStringLiteral("Create or load a project first"));
        return false;
    }

    const QString newPath = normalizePath(projectFileUrlOrPath);
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

bool Control::importImageFolder(const QVariant &folderUrlOrPath, bool clearExisting)
{
    if (m_systemService.projectFilePath().isEmpty()) {
        setStatus(QStringLiteral("Create or load a project first"));
        return false;
    }

    const QString folderPath = normalizePath(folderUrlOrPath);
    QDir dir(folderPath);
    if (!dir.exists()) {
        setStatus(QStringLiteral("Folder does not exist"));
        return false;
    }

    if (clearExisting) {
        m_imagePaths.clear();
        m_annotations.clear();
        m_currentImageIndex = -1;
        m_draftPoints.clear();
        m_selectedPolygonIndex = -1;
        emit draftPointsChanged();
        emit currentImageChanged();
        emit currentPolygonsChanged();
        emit selectedPolygonIndexChanged();
        emit selectedAnnotationClassNameChanged();
        resetAnnotationFieldsToDefault();
    }

    int importedCount = 0;
    int updatedCount = 0;
    bool changed = false;

    QDirIterator it(folderPath, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = it.next();
        const QString suffix = QStringLiteral(".") + QFileInfo(path).suffix().toLower();
        if (!m_labelingService.supportedExtensions().contains(suffix)) {
            continue;
        }

        if (m_labelingService.upsertImagePathByFilename(m_imagePaths, path, &importedCount, &updatedCount)) {
            changed = true;
        }
    }

    if (!changed) {
        setStatus(QStringLiteral("No new images imported from folder"));
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

bool Control::importImageFiles(const QVariant &fileUrlList, bool clearExisting)
{
    if (m_systemService.projectFilePath().isEmpty()) {
        setStatus(QStringLiteral("Create or load a project first"));
        return false;
    }

    if (clearExisting) {
        m_imagePaths.clear();
        m_annotations.clear();
        m_currentImageIndex = -1;
        m_draftPoints.clear();
        m_selectedPolygonIndex = -1;
        emit draftPointsChanged();
        emit currentImageChanged();
        emit currentPolygonsChanged();
        emit selectedPolygonIndexChanged();
        emit selectedAnnotationClassNameChanged();
        resetAnnotationFieldsToDefault();
    }

    int importedCount = 0;
    int updatedCount = 0;
    bool changed = false;

    QVariantList entries;
    if (fileUrlList.typeId() == QMetaType::QVariantList) {
        entries = fileUrlList.toList();
    } else {
        const QString maybeSingle = normalizePath(fileUrlList);
        if (!maybeSingle.isEmpty()) {
            entries.push_back(maybeSingle);
        }
    }

    for (const QVariant &entry : entries) {
        const QString path = normalizePath(entry);
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
        setStatus(QStringLiteral("No new images imported"));
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

bool Control::importImageFileUrls(const QList<QUrl> &fileUrls, bool clearExisting)
{
    QVariantList list;
    list.reserve(fileUrls.size());
    for (const QUrl &url : fileUrls) {
        list.push_back(url);
    }

    return importImageFiles(list, clearExisting);
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

bool Control::firstImage()
{
    return selectImage(0);
}

bool Control::lastImage()
{
    if (m_imagePaths.isEmpty()) {
        return false;
    }

    return selectImage(m_imagePaths.size() - 1);
}

bool Control::nextImage()
{
    if (m_imagePaths.isEmpty()) {
        return false;
    }

    const int next = qMin(m_currentImageIndex + 1, m_imagePaths.size() - 1);
    return selectImage(next);
}

bool Control::previousImage()
{
    if (m_imagePaths.isEmpty()) {
        return false;
    }

    const int prev = qMax(0, m_currentImageIndex - 1);
    return selectImage(prev);
}

bool Control::next10Images()
{
    if (m_imagePaths.isEmpty()) {
        return false;
    }

    const int next = qMin(m_currentImageIndex + 10, m_imagePaths.size() - 1);
    return selectImage(next);
}

bool Control::previous10Images()
{
    if (m_imagePaths.isEmpty()) {
        return false;
    }

    const int prev = qMax(0, m_currentImageIndex - 10);
    return selectImage(prev);
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

    setSelectedPolygonIndex(annotationIndex);
    if (m_selectedPolygonIndex != annotationIndex) {
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
        m_imagePaths.clear();
        m_annotations.clear();
        m_currentImageIndex = -1;
        m_draftPoints.clear();
        m_selectedPolygonIndex = -1;
        emit draftPointsChanged();
        emit currentImageChanged();
        emit currentPolygonsChanged();
        emit selectedPolygonIndexChanged();
        emit selectedAnnotationClassNameChanged();
        resetAnnotationFieldsToDefault();
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

bool Control::addClassName(const QString &name)
{
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) {
        setStatus(QStringLiteral("Class name cannot be empty"));
        return false;
    }

    if (!m_classNames.contains(trimmed)) {
        m_classNames.push_back(trimmed);
        emit classNamesChanged();
    }

    setCurrentClassName(trimmed);
    saveProjectData();
    return true;
}

bool Control::deleteClassName(const QString &name)
{
    const QString trimmed = name.trimmed();
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

    // Reassign annotations using deleted class to default.
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

void Control::setCurrentClassName(const QString &name)
{
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty() || m_currentClassName == trimmed) {
        return;
    }

    m_currentClassName = trimmed;
    emit currentClassNameChanged();
}

bool Control::addAnnotationPoint(qreal x, qreal y)
{
    if (m_currentImageIndex < 0) {
        setStatus(QStringLiteral("Select an image first"));
        return false;
    }
    if (!m_draftModeActive) {
        setStatus(QStringLiteral("Click New before adding points"));
        return false;
    }

    m_draftPoints.push_back(QPointF(x, y));
    if (m_selectedPolygonIndex != -1) {
        m_selectedPolygonIndex = -1;
        emit selectedPolygonIndexChanged();
        emit selectedAnnotationClassNameChanged();
        resetAnnotationFieldsToDefault();
    }
    emit draftPointsChanged();
    return true;
}

bool Control::updateDraftPoint(int pointIndex, qreal x, qreal y)
{
    if (!m_draftModeActive) {
        return false;
    }
    if (pointIndex < 0 || pointIndex >= m_draftPoints.size()) {
        return false;
    }

    m_draftPoints[pointIndex] = QPointF(x, y);
    emit draftPointsChanged();
    return true;
}

void Control::startNewAnnotation()
{
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
}

void Control::setSelectedPolygonIndex(int polygonIndex)
{
    const int maxIndex = currentImageAnnotations().size() - 1;
    const int clamped = (polygonIndex < 0 || polygonIndex > maxIndex) ? -1 : polygonIndex;
    if (m_selectedPolygonIndex == clamped) {
        return;
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
}

bool Control::deleteSelectedAnnotation()
{
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

bool Control::updateAnnotationPoint(int polygonIndex, int pointIndex, qreal x, qreal y)
{
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

    ann.points[pointIndex] = QPointF(x, y);
    emit currentPolygonsChanged();

    if (!saveProjectData()) {
        setStatus(QStringLiteral("Failed to save edited annotation"));
        return false;
    }

    setStatus(QStringLiteral("Annotation point updated"));
    return true;
}

bool Control::updateSelectedAnnotationClassName(const QString &className)
{
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

    const QString key = m_labelingService.annotationKeyForPath(m_imagePaths.at(m_currentImageIndex));
    auto it = m_annotations.find(key);
    if (it == m_annotations.end() || m_selectedPolygonIndex >= it.value().size()) {
        setStatus(QStringLiteral("No annotation selected"));
        return false;
    }

    if (!m_classNames.contains(trimmed)) {
        setStatus(QStringLiteral("Class not found. Add it first."));
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

bool Control::updateSelectedAnnotationDetails(const QString &className)
{
    if (m_currentImageIndex < 0 || m_currentImageIndex >= m_imagePaths.size()) {
        setStatus(QStringLiteral("Select an image first"));
        return false;
    }

    if (m_selectedPolygonIndex < 0) {
        setStatus(QStringLiteral("No annotation selected"));
        return false;
    }

    const QString trimmedClass = className.trimmed();
    if (trimmedClass.isEmpty()) {
        setStatus(QStringLiteral("Class name cannot be empty"));
        return false;
    }

    if (!m_classNames.contains(trimmedClass)) {
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
    ann.className = trimmedClass;
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

bool Control::undoLastPoint()
{
    if (m_draftPoints.isEmpty()) {
        return false;
    }

    m_draftPoints.removeLast();
    emit draftPointsChanged();
    return true;
}

void Control::clearDraftPoints()
{
    if (m_draftPoints.isEmpty()) {
        return;
    }

    m_draftPoints.clear();
    emit draftPointsChanged();
}

bool Control::saveCurrentAnnotation()
{
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

    // Ensure the polygon is explicitly closed when saved.
    if (!ann.points.isEmpty() && ann.points.first() != ann.points.last()) {
        ann.points.push_back(ann.points.first());
    }

    const QString key = m_labelingService.annotationKeyForPath(m_imagePaths.at(m_currentImageIndex));
    m_annotations[key].push_back(ann);
    m_selectedPolygonIndex = m_annotations[key].size() - 1;
    emit selectedPolygonIndexChanged();
    emit selectedAnnotationClassNameChanged();
    emit currentPolygonsChanged();

    clearDraftPoints();
    m_draftModeActive = false;

    if (!saveProjectData()) {
        setStatus(QStringLiteral("Failed to save annotation"));
        return false;
    }

    setStatus(QStringLiteral("Annotation saved"));
    return true;
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

    return loadProjectFile(lastProject);
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
