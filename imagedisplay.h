#ifndef IMAGEDISPLAY_H
#define IMAGEDISPLAY_H

#include <QImage>
#include <QMetaObject>
#include <QPointF>
#include <QQuickPaintedItem>
#include <QVariantList>

class ImageDisplay : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QObject* controller READ controller WRITE setController NOTIFY controllerChanged)
    Q_PROPERTY(bool hasImage READ hasImage NOTIFY imageChanged)
    Q_PROPERTY(bool editMode READ editMode WRITE setEditMode NOTIFY editModeChanged)
    Q_PROPERTY(int selectedPolygonIndex READ selectedPolygonIndex WRITE setSelectedPolygonIndex NOTIFY selectedPolygonIndexChanged)
    Q_PROPERTY(QVariantList savedPolygons READ savedPolygons WRITE setSavedPolygons NOTIFY savedPolygonsChanged)
    Q_PROPERTY(QVariantList annotationPoints READ annotationPoints WRITE setAnnotationPoints NOTIFY annotationPointsChanged)

public:
    explicit ImageDisplay(QQuickItem *parent = nullptr);

    QObject *controller() const;
    void setController(QObject *controller);

    bool hasImage() const;
    bool editMode() const;
    void setEditMode(bool enabled);
    int selectedPolygonIndex() const;
    void setSelectedPolygonIndex(int polygonIndex);
    QVariantList savedPolygons() const;
    void setSavedPolygons(const QVariantList &polygons);
    QVariantList annotationPoints() const;
    void setAnnotationPoints(const QVariantList &points);

    Q_INVOKABLE void setImage(const QImage &image);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void zoomFit();

    void paint(QPainter *painter) override;

signals:
    void controllerChanged();
    void imageChanged();
    void editModeChanged();
    void selectedPolygonIndexChanged();
    void savedPolygonsChanged();
    void annotationPointsChanged();
    void imageClicked(qreal x, qreal y);
    void annotationPointEdited(int polygonIndex, int pointIndex, qreal x, qreal y);
    void draftPointEdited(int pointIndex, qreal x, qreal y);
    void polygonSelected(int polygonIndex);

private:
    qreal fitZoom() const;
    QPointF imageTopLeft() const;
    bool mapItemToImage(const QPointF &itemPoint, QPointF *imagePoint) const;
    bool findEditablePoint(const QPointF &itemPoint, int *polygonIndex, int *pointIndex) const;
    bool findEditableDraftPoint(const QPointF &itemPoint, int *pointIndex) const;
    int findPolygonContainingItemPoint(const QPointF &itemPoint) const;
    void clampPan();

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    QObject *m_controller = nullptr;
    QMetaObject::Connection m_imageReadyConnection;
    QImage m_image;
    qreal m_zoom = 1.0;
    QPointF m_pan = QPointF(0.0, 0.0);
    bool m_autoFit = true;
    bool m_dragging = false;
    bool m_dragMoved = false;
    bool m_editMode = false;
    int m_selectedPolygonIndex = -1;
    bool m_editingPoint = false;
    bool m_editingDraftPoint = false;
    int m_editPolygonIndex = -1;
    int m_editPointIndex = -1;
    QPointF m_lastMousePos;
    QVariantList m_savedPolygons;
    QVariantList m_annotationPoints;
};

#endif // IMAGEDISPLAY_H
