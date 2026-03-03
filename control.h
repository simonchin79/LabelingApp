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
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString projectName READ projectName NOTIFY projectChanged)
    Q_PROPERTY(QString projectFilePath READ projectFilePath NOTIFY projectChanged)
    Q_PROPERTY(QStringList imagePaths READ imagePaths NOTIFY imagePathsChanged)
    Q_PROPERTY(int imageCount READ imageCount NOTIFY imagePathsChanged)
    Q_PROPERTY(int currentImageIndex READ currentImageIndex NOTIFY currentImageChanged)
    Q_PROPERTY(QString currentImagePath READ currentImagePath NOTIFY currentImageChanged)
    Q_PROPERTY(QStringList classNames READ classNames NOTIFY classNamesChanged)
    Q_PROPERTY(QString currentClassName READ currentClassName WRITE setCurrentClassName NOTIFY currentClassNameChanged)
    Q_PROPERTY(QVariantList draftPoints READ draftPoints NOTIFY draftPointsChanged)
    Q_PROPERTY(QVariantList currentPolygons READ currentPolygons NOTIFY currentPolygonsChanged)
    Q_PROPERTY(int selectedPolygonIndex READ selectedPolygonIndex NOTIFY selectedPolygonIndexChanged)
    Q_PROPERTY(QString selectedAnnotationClassName READ selectedAnnotationClassName NOTIFY selectedAnnotationClassNameChanged)
    Q_PROPERTY(QString annotationRemarks READ annotationRemarks WRITE setAnnotationRemarks NOTIFY annotationFieldsChanged)
    Q_PROPERTY(QString annotationKind READ annotationKind WRITE setAnnotationKind NOTIFY annotationFieldsChanged)
    Q_PROPERTY(QString annotationSeverity READ annotationSeverity WRITE setAnnotationSeverity NOTIFY annotationFieldsChanged)
    Q_PROPERTY(QString annotationSplit READ annotationSplit WRITE setAnnotationSplit NOTIFY annotationFieldsChanged)
    Q_PROPERTY(bool showLabel READ showLabel WRITE setShowLabel NOTIFY typeVisibilityChanged)
    Q_PROPERTY(bool showPredict READ showPredict WRITE setShowPredict NOTIFY typeVisibilityChanged)
    Q_PROPERTY(bool showGood READ showGood WRITE setShowGood NOTIFY typeVisibilityChanged)
    Q_PROPERTY(QString ioFolderPath READ ioFolderPath WRITE setIoFolderPath NOTIFY ioFolderPathChanged)

public:
    explicit Control(QObject *parent = nullptr);

    QString status() const;
    QString projectName() const;
    QString projectFilePath() const;
    QStringList imagePaths() const;
    int imageCount() const;
    int currentImageIndex() const;
    QString currentImagePath() const;
    QStringList classNames() const;
    QString currentClassName() const;
    QVariantList draftPoints() const;
    QVariantList currentPolygons() const;
    int selectedPolygonIndex() const;
    QString selectedAnnotationClassName() const;
    QString annotationRemarks() const;
    QString annotationKind() const;
    QString annotationSeverity() const;
    QString annotationSplit() const;
    void setAnnotationRemarks(const QString &remarks);
    void setAnnotationKind(const QString &kind);
    void setAnnotationSeverity(const QString &severity);
    void setAnnotationSplit(const QString &split);
    bool showLabel() const;
    bool showPredict() const;
    bool showGood() const;
    void setShowLabel(bool enabled);
    void setShowPredict(bool enabled);
    void setShowGood(bool enabled);
    QString ioFolderPath() const;
    void setIoFolderPath(const QString &path);

    Q_INVOKABLE bool createNewProject(const QString &preferredName);
    Q_INVOKABLE bool loadProjectFile(const QVariant &projectFileUrlOrPath);
    Q_INVOKABLE bool saveProjectAs(const QVariant &projectFileUrlOrPath);
    Q_INVOKABLE bool importImageFolder(const QVariant &folderUrlOrPath, bool clearExisting);
    Q_INVOKABLE bool importImageFiles(const QVariant &fileUrlList, bool clearExisting);
    Q_INVOKABLE bool importImageFileUrls(const QList<QUrl> &fileUrls, bool clearExisting);
    Q_INVOKABLE bool selectImage(int index);
    Q_INVOKABLE bool firstImage();
    Q_INVOKABLE bool lastImage();
    Q_INVOKABLE bool nextImage();
    Q_INVOKABLE bool previousImage();
    Q_INVOKABLE bool next10Images();
    Q_INVOKABLE bool previous10Images();
    Q_INVOKABLE QVariantList imageSummaryList() const;
    Q_INVOKABLE QVariantList annotationListForCurrentImage() const;
    Q_INVOKABLE QVariantList annotationListForAllImages() const;
    Q_INVOKABLE bool selectAnnotationFromList(int imageIndex, int annotationIndex);
    Q_INVOKABLE bool importClassificationFromFolder(const QVariant &folderUrlOrPath, bool clearExisting);
    Q_INVOKABLE bool exportClassificationToFolder(const QVariant &folderUrlOrPath);

    Q_INVOKABLE bool addClassName(const QString &name);
    Q_INVOKABLE bool deleteClassName(const QString &name);
    Q_INVOKABLE void setCurrentClassName(const QString &name);

    Q_INVOKABLE bool addAnnotationPoint(qreal x, qreal y);
    Q_INVOKABLE bool updateDraftPoint(int pointIndex, qreal x, qreal y);
    Q_INVOKABLE void startNewAnnotation();
    Q_INVOKABLE void setSelectedPolygonIndex(int polygonIndex);
    Q_INVOKABLE bool deleteSelectedAnnotation();
    Q_INVOKABLE bool updateSelectedAnnotationClassName(const QString &className);
    Q_INVOKABLE bool updateSelectedAnnotationDetails(const QString &className);
    Q_INVOKABLE bool updateAnnotationPoint(int polygonIndex, int pointIndex, qreal x, qreal y);
    Q_INVOKABLE bool undoLastPoint();
    Q_INVOKABLE void clearDraftPoints();
    Q_INVOKABLE bool saveCurrentAnnotation();

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
    void imageReady(const QImage &image);

private:
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
    bool loadImageToView(const QString &path);
    void setStatus(const QString &status);
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
    QVector<QPointF> m_draftPoints;
    QHash<QString, QList<AnnotationData>> m_annotations;
    int m_selectedPolygonIndex = -1;
    bool m_draftModeActive = false;
};

#endif // CONTROL_H
