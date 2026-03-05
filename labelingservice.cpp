#include "labelingservice.h"

#include "systemservice.h"

#include <QFileInfo>

namespace {
QString canonicalize(const QString &action, const QList<QPair<QString, QStringList>> &entries)
{
    const QString key = action.trimmed().toLower();
    for (const auto &entry : entries) {
        if (entry.second.contains(key)) {
            return entry.first;
        }
    }
    return QString();
}
}

LabelingService::LabelingService(SystemService *systemService, QObject *parent)
    : QObject(parent)
    , m_systemService(systemService)
{
}

SystemService *LabelingService::systemService() const
{
    return m_systemService;
}

QStringList LabelingService::supportedExtensions() const
{
    return {
        QStringLiteral(".png"),
        QStringLiteral(".jpg"),
        QStringLiteral(".jpeg"),
        QStringLiteral(".bmp"),
        QStringLiteral(".tif"),
        QStringLiteral(".tiff")
    };
}

QString LabelingService::annotationKeyForPath(const QString &path) const
{
    return QFileInfo(path).fileName().toLower();
}

QString LabelingService::normalizedKind(const QString &kind) const
{
    const QString value = kind.trimmed().toLower();
    if (value == QStringLiteral("predict") || value == QStringLiteral("good")) {
        return value;
    }
    return QStringLiteral("label");
}

QString LabelingService::normalizedSeverity(const QString &severity) const
{
    const QString value = severity.trimmed().toLower();
    if (value == QStringLiteral("serious") || value == QStringLiteral("marginal")) {
        return value;
    }
    return QStringLiteral("normal");
}

QString LabelingService::normalizedSplit(const QString &split) const
{
    const QString value = split.trimmed().toLower();
    if (value == QStringLiteral("train") || value == QStringLiteral("val") || value == QStringLiteral("test")) {
        return value;
    }
    return QStringLiteral("none");
}

bool LabelingService::upsertImagePathByFilename(QStringList &imagePaths, const QString &path, int *addedCount, int *updatedCount) const
{
    const QString newFileName = QFileInfo(path).fileName();
    if (newFileName.isEmpty()) {
        return false;
    }

    if (imagePaths.contains(path)) {
        return false;
    }

    for (int i = 0; i < imagePaths.size(); ++i) {
        const QString existingPath = imagePaths.at(i);
        const QString existingFileName = QFileInfo(existingPath).fileName();
        if (existingFileName.compare(newFileName, Qt::CaseInsensitive) != 0) {
            continue;
        }

        imagePaths[i] = path;
        if (updatedCount) {
            ++(*updatedCount);
        }
        return true;
    }

    imagePaths.push_back(path);
    if (addedCount) {
        ++(*addedCount);
    }
    return true;
}

QString LabelingService::canonicalProjectAction(const QString &action) const
{
    return canonicalize(action,
                        {
                            {QStringLiteral("create"), {QStringLiteral("create"), QStringLiteral("new")}},
                            {QStringLiteral("load"), {QStringLiteral("load"), QStringLiteral("open")}},
                            {QStringLiteral("save_as"), {QStringLiteral("save_as"), QStringLiteral("saveas")}}
                        });
}

QString LabelingService::canonicalImageAction(const QString &action) const
{
    return canonicalize(action,
                        {
                            {QStringLiteral("import_folder"),
                             {QStringLiteral("import_folder"), QStringLiteral("importfolder"), QStringLiteral("folder")}},
                            {QStringLiteral("import_files"),
                             {QStringLiteral("import_files"), QStringLiteral("importfiles"), QStringLiteral("files")}},
                            {QStringLiteral("select"), {QStringLiteral("select"), QStringLiteral("goto")}},
                            {QStringLiteral("first"), {QStringLiteral("first")}},
                            {QStringLiteral("last"), {QStringLiteral("last")}},
                            {QStringLiteral("next"), {QStringLiteral("next")}},
                            {QStringLiteral("prev"), {QStringLiteral("prev"), QStringLiteral("previous")}},
                            {QStringLiteral("next10"), {QStringLiteral("next10"), QStringLiteral("next_10")}},
                            {QStringLiteral("prev10"), {QStringLiteral("prev10"), QStringLiteral("previous10"), QStringLiteral("prev_10")}}
                        });
}

QString LabelingService::canonicalAnnotationAction(const QString &action) const
{
    return canonicalize(action,
                        {
                            {QStringLiteral("start_new"), {QStringLiteral("start_new"), QStringLiteral("new")}},
                            {QStringLiteral("add_class"), {QStringLiteral("add_class"), QStringLiteral("class_add")}},
                            {QStringLiteral("delete_class"), {QStringLiteral("delete_class"), QStringLiteral("class_delete")}},
                            {QStringLiteral("set_current_class"),
                             {QStringLiteral("set_current_class"), QStringLiteral("class_select")}},
                            {QStringLiteral("set_field"),
                             {QStringLiteral("set_field"), QStringLiteral("field"), QStringLiteral("update_field")}},
                            {QStringLiteral("set_kind"), {QStringLiteral("set_kind"), QStringLiteral("kind")}},
                            {QStringLiteral("set_severity"), {QStringLiteral("set_severity"), QStringLiteral("severity"), QStringLiteral("level")}},
                            {QStringLiteral("set_split"), {QStringLiteral("set_split"), QStringLiteral("split")}},
                            {QStringLiteral("set_remarks"), {QStringLiteral("set_remarks"), QStringLiteral("remarks")}},
                            {QStringLiteral("add_point"), {QStringLiteral("add_point"), QStringLiteral("point_add")}},
                            {QStringLiteral("update_draft_point"),
                             {QStringLiteral("update_draft_point"), QStringLiteral("point_update_draft")}},
                            {QStringLiteral("save"), {QStringLiteral("save"), QStringLiteral("save_annotation")}},
                            {QStringLiteral("undo"), {QStringLiteral("undo"), QStringLiteral("undo_point")}},
                            {QStringLiteral("clear"), {QStringLiteral("clear"), QStringLiteral("clear_draft")}},
                            {QStringLiteral("select"), {QStringLiteral("select"), QStringLiteral("select_polygon")}},
                            {QStringLiteral("delete"), {QStringLiteral("delete"), QStringLiteral("delete_selected")}},
                            {QStringLiteral("update_class"),
                             {QStringLiteral("update_class"), QStringLiteral("set_class")}},
                            {QStringLiteral("update_details"),
                             {QStringLiteral("update_details"), QStringLiteral("update_annotation")}},
                            {QStringLiteral("update_point"),
                             {QStringLiteral("update_point"), QStringLiteral("point_update")}},
                            {QStringLiteral("select_from_list"),
                             {QStringLiteral("select_from_list"), QStringLiteral("list_select")}}
                        });
}

QString LabelingService::canonicalIoAction(const QString &action) const
{
    return canonicalize(action,
                        {
                            {QStringLiteral("set_folder"), {QStringLiteral("set_folder"), QStringLiteral("browse")}},
                            {QStringLiteral("set_visibility"), {QStringLiteral("set_visibility"), QStringLiteral("visibility")}},
                            {QStringLiteral("set_tab_index"),
                             {QStringLiteral("set_tab_index"), QStringLiteral("tab"), QStringLiteral("tab_index")}},
                            {QStringLiteral("import_classification"),
                             {QStringLiteral("import_classification"), QStringLiteral("import")}},
                            {QStringLiteral("export_classification"),
                             {QStringLiteral("export_classification"), QStringLiteral("export")}}
                        });
}

QString LabelingService::canonicalListAction(const QString &action) const
{
    return canonicalize(action,
                        {
                            {QStringLiteral("image_summary"),
                             {QStringLiteral("image_summary"), QStringLiteral("images"), QStringLiteral("image")}},
                            {QStringLiteral("annotation_current"),
                             {QStringLiteral("annotation_current"), QStringLiteral("anno_current"), QStringLiteral("current")}},
                            {QStringLiteral("annotation_all"),
                             {QStringLiteral("annotation_all"), QStringLiteral("anno_all"), QStringLiteral("all")}}
                        });
}
