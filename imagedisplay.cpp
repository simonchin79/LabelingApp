#include "imagedisplay.h"

#include "control.h"

#include <QLineF>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QRectF>
#include <QSize>
#include <QVariantMap>
#include <QWheelEvent>

ImageDisplay::ImageDisplay(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setAntialiasing(true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);
}

QObject *ImageDisplay::controller() const
{
    return m_controller;
}

void ImageDisplay::setController(QObject *controller)
{
    if (m_controller == controller) {
        return;
    }

    if (m_imageReadyConnection) {
        disconnect(m_imageReadyConnection);
    }

    m_controller = controller;
    const auto typedController = qobject_cast<Control *>(m_controller);
    if (typedController) {
        m_imageReadyConnection =
            connect(typedController, &Control::imageReady, this, &ImageDisplay::setImage);
    }

    emit controllerChanged();
}

bool ImageDisplay::hasImage() const
{
    return !m_image.isNull();
}

bool ImageDisplay::editMode() const
{
    return m_editMode;
}

void ImageDisplay::setEditMode(bool enabled)
{
    if (m_editMode == enabled) {
        return;
    }

    m_editMode = enabled;
    if (!m_editMode) {
        m_editingPoint = false;
        m_editingDraftPoint = false;
        m_editPolygonIndex = -1;
        m_editPointIndex = -1;
    }
    emit editModeChanged();
    update();
}

int ImageDisplay::selectedPolygonIndex() const
{
    return m_selectedPolygonIndex;
}

void ImageDisplay::setSelectedPolygonIndex(int polygonIndex)
{
    if (m_selectedPolygonIndex == polygonIndex) {
        return;
    }

    m_selectedPolygonIndex = polygonIndex;
    emit selectedPolygonIndexChanged();
    update();
}

QVariantList ImageDisplay::savedPolygons() const
{
    return m_savedPolygons;
}

void ImageDisplay::setSavedPolygons(const QVariantList &polygons)
{
    m_savedPolygons = polygons;
    emit savedPolygonsChanged();
    update();
}

QVariantList ImageDisplay::annotationPoints() const
{
    return m_annotationPoints;
}

void ImageDisplay::setAnnotationPoints(const QVariantList &points)
{
    m_annotationPoints = points;
    emit annotationPointsChanged();
    update();
}

void ImageDisplay::setImage(const QImage &image)
{
    if (image.isNull()) {
        clear();
        return;
    }

    m_image = image;
    zoomFit();
    emit imageChanged();
    update();
}

void ImageDisplay::clear()
{
    if (m_image.isNull()) {
        return;
    }

    m_image = QImage();
    m_zoom = 1.0;
    m_pan = QPointF(0.0, 0.0);
    m_autoFit = true;
    emit imageChanged();
    update();
}

void ImageDisplay::paint(QPainter *painter)
{
    painter->fillRect(boundingRect(), QColor(20, 20, 20));

    if (m_image.isNull()) {
        return;
    }

    const QSizeF targetSize(m_image.width() * m_zoom, m_image.height() * m_zoom);
    const QRectF targetRect(imageTopLeft(), targetSize);
    painter->drawImage(targetRect, m_image);

    if (!m_savedPolygons.isEmpty()) {
        painter->setRenderHint(QPainter::Antialiasing, true);

        for (int p = 0; p < m_savedPolygons.size(); ++p) {
            const QVariantMap polyMap = m_savedPolygons.at(p).toMap();
            const QVariantList points = polyMap.value(QStringLiteral("points")).toList();
            if (points.size() < 3) {
                continue;
            }

            const bool isSelected = (p == m_selectedPolygonIndex);
            const QString kind = polyMap.value(QStringLiteral("kind")).toString().toLower();
            QColor baseColor(70, 140, 255); // label -> blue
            if (kind == QStringLiteral("predict")) {
                baseColor = QColor(230, 80, 80); // red
            } else if (kind == QStringLiteral("good")) {
                baseColor = QColor(70, 220, 120); // green
            }

            const QColor drawColor = isSelected ? QColor(255, 196, 64) : baseColor; // selected -> yellow
            painter->setPen(QPen(drawColor, isSelected ? 3 : 2));
            painter->setBrush(QColor(drawColor.red(), drawColor.green(), drawColor.blue(), isSelected ? 64 : 48));

            QPolygonF polygon;
            for (const QVariant &pointVar : points) {
                const QVariantMap pointMap = pointVar.toMap();
                QPointF imagePoint(pointMap.value(QStringLiteral("x")).toDouble(),
                                   pointMap.value(QStringLiteral("y")).toDouble());
                const QPointF itemPoint = targetRect.topLeft() + imagePoint * m_zoom;
                polygon << itemPoint;
            }

            painter->drawPolygon(polygon);

            if (m_editMode && isSelected) {
                painter->setPen(QPen(QColor(255, 230, 120), 2));
                painter->setBrush(QColor(255, 230, 120, 180));
                for (int i = 0; i < points.size(); ++i) {
                    const QVariantMap pointMap = points.at(i).toMap();
                    const QPointF imagePoint(pointMap.value(QStringLiteral("x")).toDouble(),
                                             pointMap.value(QStringLiteral("y")).toDouble());
                    const QPointF itemPoint = targetRect.topLeft() + imagePoint * m_zoom;

                    const bool isActive = (p == m_editPolygonIndex && i == m_editPointIndex);
                    const qreal radius = isActive ? 7.0 : 5.0;
                    if (isActive) {
                        painter->setPen(QPen(QColor(255, 140, 0), 3));
                        painter->setBrush(QColor(255, 180, 80, 220));
                    } else {
                        painter->setPen(QPen(QColor(255, 230, 120), 2));
                        painter->setBrush(QColor(255, 230, 120, 180));
                    }
                    painter->drawEllipse(itemPoint, radius, radius);
                }
            }
        }
    }

    if (!m_annotationPoints.isEmpty()) {
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(QPen(QColor(255, 106, 0), 2));

        QPolygonF poly;
        for (int i = 0; i < m_annotationPoints.size(); ++i) {
            const QVariant &v = m_annotationPoints.at(i);
            const QVariantMap pointMap = v.toMap();
            QPointF imagePoint(pointMap.value(QStringLiteral("x")).toDouble(),
                               pointMap.value(QStringLiteral("y")).toDouble());
            const QPointF itemPoint = targetRect.topLeft() + imagePoint * m_zoom;
            poly << itemPoint;
            const bool isActive = (m_editingDraftPoint && i == m_editPointIndex);
            painter->setPen(QPen(isActive ? QColor(255, 220, 120) : QColor(255, 106, 0), isActive ? 3 : 2));
            painter->setBrush(isActive ? QColor(255, 220, 120, 220) : QColor(255, 106, 0, 180));
            painter->drawEllipse(itemPoint, isActive ? 6 : 4, isActive ? 6 : 4);
        }
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(QColor(255, 106, 0), 2));

        if (poly.size() >= 2) {
            painter->drawPolyline(poly);
        }
    }
}

void ImageDisplay::zoomFit()
{
    if (m_image.isNull() || width() <= 0.0 || height() <= 0.0) {
        return;
    }

    m_zoom = fitZoom();
    m_pan = QPointF(0.0, 0.0);
    m_autoFit = true;
    update();
}

qreal ImageDisplay::fitZoom() const
{
    if (m_image.isNull() || width() <= 0.0 || height() <= 0.0) {
        return 1.0;
    }

    const qreal sx = width() / static_cast<qreal>(m_image.width());
    const qreal sy = height() / static_cast<qreal>(m_image.height());
    return qMin(sx, sy);
}

QPointF ImageDisplay::imageTopLeft() const
{
    const QSizeF scaled(m_image.width() * m_zoom, m_image.height() * m_zoom);
    return QPointF((width() - scaled.width()) * 0.5, (height() - scaled.height()) * 0.5) + m_pan;
}

bool ImageDisplay::mapItemToImage(const QPointF &itemPoint, QPointF *imagePoint) const
{
    if (m_image.isNull() || !imagePoint) {
        return false;
    }

    const QPointF local = (itemPoint - imageTopLeft()) / m_zoom;
    if (local.x() < 0.0 || local.y() < 0.0 || local.x() > m_image.width() || local.y() > m_image.height()) {
        return false;
    }

    *imagePoint = local;
    return true;
}

bool ImageDisplay::findEditablePoint(const QPointF &itemPoint, int *polygonIndex, int *pointIndex) const
{
    if (!polygonIndex || !pointIndex) {
        return false;
    }

    const QSizeF targetSize(m_image.width() * m_zoom, m_image.height() * m_zoom);
    const QRectF targetRect(imageTopLeft(), targetSize);
    const qreal pickRadius = 8.0;
    qreal bestDistance = 1e12;
    int bestPoly = -1;
    int bestPoint = -1;

    if (m_selectedPolygonIndex < 0 || m_selectedPolygonIndex >= m_savedPolygons.size()) {
        return false;
    }

    const int p = m_selectedPolygonIndex;
    const QVariantMap polyMap = m_savedPolygons.at(p).toMap();
    const QVariantList points = polyMap.value(QStringLiteral("points")).toList();
    for (int i = 0; i < points.size(); ++i) {
        const QVariantMap pointMap = points.at(i).toMap();
        const QPointF imgPt(pointMap.value(QStringLiteral("x")).toDouble(),
                            pointMap.value(QStringLiteral("y")).toDouble());
        const QPointF viewPt = targetRect.topLeft() + imgPt * m_zoom;
        const qreal dist = QLineF(viewPt, itemPoint).length();
        if (dist <= pickRadius && dist < bestDistance) {
            bestDistance = dist;
            bestPoly = p;
            bestPoint = i;
        }
    }

    if (bestPoly < 0) {
        return false;
    }

    *polygonIndex = bestPoly;
    *pointIndex = bestPoint;
    return true;
}

bool ImageDisplay::findEditableDraftPoint(const QPointF &itemPoint, int *pointIndex) const
{
    if (!pointIndex || m_annotationPoints.isEmpty() || m_image.isNull()) {
        return false;
    }

    const QSizeF targetSize(m_image.width() * m_zoom, m_image.height() * m_zoom);
    const QRectF targetRect(imageTopLeft(), targetSize);
    const qreal pickRadius = 8.0;
    qreal bestDistance = 1e12;
    int bestPoint = -1;

    for (int i = 0; i < m_annotationPoints.size(); ++i) {
        const QVariantMap pointMap = m_annotationPoints.at(i).toMap();
        const QPointF imgPt(pointMap.value(QStringLiteral("x")).toDouble(),
                            pointMap.value(QStringLiteral("y")).toDouble());
        const QPointF viewPt = targetRect.topLeft() + imgPt * m_zoom;
        const qreal dist = QLineF(viewPt, itemPoint).length();
        if (dist <= pickRadius && dist < bestDistance) {
            bestDistance = dist;
            bestPoint = i;
        }
    }

    if (bestPoint < 0) {
        return false;
    }

    *pointIndex = bestPoint;
    return true;
}

int ImageDisplay::findPolygonContainingItemPoint(const QPointF &itemPoint) const
{
    if (m_image.isNull()) {
        return -1;
    }

    const QSizeF targetSize(m_image.width() * m_zoom, m_image.height() * m_zoom);
    const QRectF targetRect(imageTopLeft(), targetSize);

    for (int p = m_savedPolygons.size() - 1; p >= 0; --p) {
        const QVariantMap polyMap = m_savedPolygons.at(p).toMap();
        const QVariantList points = polyMap.value(QStringLiteral("points")).toList();
        if (points.size() < 3) {
            continue;
        }

        QPolygonF polygon;
        for (const QVariant &pointVar : points) {
            const QVariantMap pointMap = pointVar.toMap();
            QPointF imagePoint(pointMap.value(QStringLiteral("x")).toDouble(),
                               pointMap.value(QStringLiteral("y")).toDouble());
            polygon << (targetRect.topLeft() + imagePoint * m_zoom);
        }

        if (polygon.containsPoint(itemPoint, Qt::OddEvenFill)) {
            return p;
        }
    }

    return -1;
}

void ImageDisplay::clampPan()
{
    if (m_image.isNull()) {
        m_pan = QPointF(0.0, 0.0);
        return;
    }

    const qreal scaledW = m_image.width() * m_zoom;
    const qreal scaledH = m_image.height() * m_zoom;
    const qreal maxPanX = qMax(0.0, (scaledW - width()) * 0.5);
    const qreal maxPanY = qMax(0.0, (scaledH - height()) * 0.5);
    m_pan.setX(qBound(-maxPanX, m_pan.x(), maxPanX));
    m_pan.setY(qBound(-maxPanY, m_pan.y(), maxPanY));
}

void ImageDisplay::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);

    if (m_autoFit) {
        zoomFit();
    } else {
        m_zoom = qMax(m_zoom, fitZoom());
        clampPan();
        update();
    }
}

void ImageDisplay::wheelEvent(QWheelEvent *event)
{
    if (m_image.isNull()) {
        event->ignore();
        return;
    }

    const qreal step = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
    const qreal oldZoom = m_zoom;
    const qreal minZoom = fitZoom();
    const qreal newZoom = qBound(minZoom, oldZoom * step, 20.0);
    if (qFuzzyCompare(oldZoom, newZoom)) {
        event->accept();
        return;
    }

    const QPointF cursor = event->position();
    const QPointF oldTopLeft = imageTopLeft();
    const QPointF imageCoord = (cursor - oldTopLeft) / oldZoom;

    m_zoom = newZoom;
    m_autoFit = false;

    const QSizeF scaled(m_image.width() * m_zoom, m_image.height() * m_zoom);
    const QPointF center(width() * 0.5, height() * 0.5);
    const QPointF newTopLeft = cursor - imageCoord * m_zoom;
    m_pan = newTopLeft - QPointF((width() - scaled.width()) * 0.5, (height() - scaled.height()) * 0.5);
    clampPan();
    update();
    event->accept();
}

void ImageDisplay::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && !m_image.isNull()) {
        int draftPointIndex = -1;
        if (findEditableDraftPoint(event->position(), &draftPointIndex)) {
            m_editingDraftPoint = true;
            m_editPointIndex = draftPointIndex;
            m_lastMousePos = event->position();
            event->accept();
            return;
        }

        if (m_editMode) {
            int polyIndex = -1;
            int pointIndex = -1;
            if (findEditablePoint(event->position(), &polyIndex, &pointIndex)) {
                m_editingPoint = true;
                m_editPolygonIndex = polyIndex;
                m_editPointIndex = pointIndex;
                m_lastMousePos = event->position();
                event->accept();
                return;
            }
        }

        m_dragging = true;
        m_dragMoved = false;
        m_lastMousePos = event->position();
        m_autoFit = false;
        event->accept();
        return;
    }

    QQuickPaintedItem::mousePressEvent(event);
}

void ImageDisplay::mouseMoveEvent(QMouseEvent *event)
{
    if (m_editingDraftPoint) {
        QPointF imagePoint;
        if (mapItemToImage(event->position(), &imagePoint)) {
            if (m_editPointIndex >= 0 && m_editPointIndex < m_annotationPoints.size()) {
                QVariantMap pointMap = m_annotationPoints.at(m_editPointIndex).toMap();
                pointMap.insert(QStringLiteral("x"), imagePoint.x());
                pointMap.insert(QStringLiteral("y"), imagePoint.y());
                m_annotationPoints[m_editPointIndex] = pointMap;
                update();
            }
        }
        event->accept();
        return;
    }

    if (m_editingPoint) {
        QPointF imagePoint;
        if (mapItemToImage(event->position(), &imagePoint)) {
            if (m_editPolygonIndex >= 0 && m_editPolygonIndex < m_savedPolygons.size()) {
                QVariantMap polyMap = m_savedPolygons.at(m_editPolygonIndex).toMap();
                QVariantList points = polyMap.value(QStringLiteral("points")).toList();
                if (m_editPointIndex >= 0 && m_editPointIndex < points.size()) {
                    QVariantMap pointMap = points.at(m_editPointIndex).toMap();
                    pointMap.insert(QStringLiteral("x"), imagePoint.x());
                    pointMap.insert(QStringLiteral("y"), imagePoint.y());
                    points[m_editPointIndex] = pointMap;
                    polyMap.insert(QStringLiteral("points"), points);
                    m_savedPolygons[m_editPolygonIndex] = polyMap;
                    update();
                }
            }
        }
        event->accept();
        return;
    }

    if (m_dragging) {
        const QPointF delta = event->position() - m_lastMousePos;
        m_lastMousePos = event->position();
        if (delta.manhattanLength() > 0.5) {
            m_dragMoved = true;
        }
        m_pan += delta;
        clampPan();
        update();
        event->accept();
        return;
    }

    QQuickPaintedItem::mouseMoveEvent(event);
}

void ImageDisplay::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_editingDraftPoint) {
        if (m_editPointIndex >= 0 && m_editPointIndex < m_annotationPoints.size()) {
            const QVariantMap pointMap = m_annotationPoints.at(m_editPointIndex).toMap();
            emit draftPointEdited(
                m_editPointIndex,
                pointMap.value(QStringLiteral("x")).toDouble(),
                pointMap.value(QStringLiteral("y")).toDouble());
        }

        m_editingDraftPoint = false;
        m_editPointIndex = -1;
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton && m_editingPoint) {
        if (m_editPolygonIndex >= 0 && m_editPolygonIndex < m_savedPolygons.size()) {
            const QVariantMap polyMap = m_savedPolygons.at(m_editPolygonIndex).toMap();
            const QVariantList points = polyMap.value(QStringLiteral("points")).toList();
            if (m_editPointIndex >= 0 && m_editPointIndex < points.size()) {
                const QVariantMap pointMap = points.at(m_editPointIndex).toMap();
                emit annotationPointEdited(
                    m_editPolygonIndex,
                    m_editPointIndex,
                    pointMap.value(QStringLiteral("x")).toDouble(),
                    pointMap.value(QStringLiteral("y")).toDouble());
            }
        }

        m_editingPoint = false;
        m_editPolygonIndex = -1;
        m_editPointIndex = -1;
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton && m_dragging) {
        if (!m_dragMoved) {
            const int polygonIndex = findPolygonContainingItemPoint(event->position());
            if (polygonIndex >= 0) {
                emit polygonSelected(polygonIndex);
            } else if (!m_editMode) {
                QPointF imagePoint;
                if (mapItemToImage(event->position(), &imagePoint)) {
                    emit imageClicked(imagePoint.x(), imagePoint.y());
                }
            }
        }
        m_dragging = false;
        event->accept();
        return;
    }

    QQuickPaintedItem::mouseReleaseEvent(event);
}

void ImageDisplay::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        zoomFit();
        event->accept();
        return;
    }

    QQuickPaintedItem::mouseDoubleClickEvent(event);
}
