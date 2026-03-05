#ifndef CONTROL_H
#define CONTROL_H

#include <QHash>
#include <QImage>
#include <QObject>
#include <QPointF>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariant>
#include <QVariantList>
#include <QVector>

#include "inferenceservice.h"
#include "labelingservice.h"
#include "systemservice.h"
#include "trainservice.h"

namespace cv {
class Mat;
}

class Control : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap uiState READ uiState NOTIFY uiStateChanged)

public:
    explicit Control(QObject *parent = nullptr);

    QVariantMap uiState() const;
    Q_INVOKABLE bool projectAction(const QString &action, const QVariantMap &payload = QVariantMap());
    Q_INVOKABLE bool imageAction(const QString &action, const QVariantMap &payload = QVariantMap());
    Q_INVOKABLE QVariantList listAction(const QString &action, const QVariantMap &payload = QVariantMap()) const;
    Q_INVOKABLE bool ioAction(const QString &action, const QVariantMap &payload = QVariantMap());
    Q_INVOKABLE bool annotationAction(const QString &action, const QVariantMap &payload = QVariantMap());
    Q_INVOKABLE bool trainAction(const QString &action, const QVariantMap &payload = QVariantMap());

signals:
    void statusChanged();
    void projectChanged();
    void imagePathsChanged();
    void currentImageChanged();
    void classNamesChanged();
    void currentClassNameChanged();
    void draftPointsChanged();
    void currentPolygonsChanged();
    void selectedPolygonIndexChanged();
    void selectedAnnotationClassNameChanged();
    void annotationFieldsChanged();
    void typeVisibilityChanged();
    void ioFolderPathChanged();
    void trainingSettingsChanged();
    void uiStateChanged();
    void imageReady(const QImage &image);

private:
    enum class ImageNavigation {
        First,
        Last,
        Next,
        Previous,
        Next10,
        Previous10
    };

    struct AnnotationData {
        QString className;
        QString remarks;
        QString kind;
        QString severity;
        QString split;
        QVector<QPointF> points;
    };

    static QImage matToQImage(const cv::Mat &mat);
    static QString normalizePath(const QVariant &urlOrPath);

    QString currentImagePath() const;
    QVariantList draftPoints() const;
    QVariantList currentPolygons() const;
    QString selectedAnnotationClassName() const;
    void setIoFolderPath(const QString &path);
    QVariantMap projectState() const;
    QVariantMap imageState() const;
    QVariantMap annotationState() const;
    QVariantMap visibilityState() const;
    QVariantMap trainingState() const;

    bool executeProjectOperation(const QString &operationKey, const QVariant &value, const QVariantMap &payload);
    void clearImageWorkspaceForImport();
    bool importImagePaths(const QStringList &paths, bool clearExisting, const QString &emptyImportStatus);
    bool selectImage(int index);
    bool navigateImage(ImageNavigation navigation);
    QVariantList imageSummaryList() const;
    QVariantList annotationListForCurrentImage() const;
    QVariantList annotationListForAllImages() const;
    bool selectAnnotationFromList(int imageIndex, int annotationIndex);
    bool importClassificationFromFolder(const QVariant &folderUrlOrPath, bool clearExisting);
    bool exportClassificationToFolder(const QVariant &folderUrlOrPath);
    bool loadImageToView(const QString &path);
    void setStatus(const QString &status);
    bool executeAnnotationOperation(const QString &operationKey, const QVariant &value, const QVariantMap &payload);
    bool applyDatasetSplitToAnnotations();
    bool saveProjectData();
    void setAnnotationFields(const QString &remarks, const QString &kind, const QString &severity, const QString &split);
    void resetAnnotationFieldsToDefault();
    void saveSessionState() const;
    void saveLastImageIndex() const;
    bool restoreLastSession();
    QList<AnnotationData> currentImageAnnotations() const;
    void resetState();

    SystemService m_systemService;
    LabelingService m_labelingService;
    TrainService m_trainService;
    InferenceService m_inferenceService;
    QString m_status;
    QString m_projectName;
    QStringList m_imagePaths;
    int m_currentImageIndex = -1;
    QStringList m_classNames;
    QString m_currentClassName;
    QString m_annotationRemarks;
    QString m_annotationKind = QStringLiteral("label");
    QString m_annotationSeverity = QStringLiteral("normal");
    QString m_annotationSplit = QStringLiteral("none");
    bool m_showLabel = true;
    bool m_showPredict = true;
    bool m_showGood = true;
    QString m_ioFolderPath;
    int m_lastTabIndex = 0;
    QVector<QPointF> m_draftPoints;
    QHash<QString, QList<AnnotationData>> m_annotations;
    int m_selectedPolygonIndex = -1;
    bool m_draftModeActive = false;
};

#endif // CONTROL_H
