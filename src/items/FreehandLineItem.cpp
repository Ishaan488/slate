#include "FreehandLineItem.h"
#include <QPainter>

FreehandLineItem::FreehandLineItem(const QPointF& startPoint,
                                   const QColor& color,
                                   qreal width,
                                   QGraphicsItem* parent)
    : QGraphicsPathItem(parent)
    , m_color(color)
    , m_width(width)
{
    m_points.append(startPoint);

    QPen pen(m_color);
    pen.setWidthF(m_width);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    setPen(pen);
    setBrush(Qt::NoBrush);

    setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
}

void FreehandLineItem::addPoint(const QPointF& point)
{
    m_points.append(point);
    rebuildPath();
}

void FreehandLineItem::finishStroke()
{
    // Simplify: remove points that are very close together
    if (m_points.size() < 3) return;

    QVector<QPointF> simplified;
    simplified.append(m_points.first());

    const qreal minDistance = 2.0;  // Minimum pixel distance between points
    for (int i = 1; i < m_points.size(); ++i) {
        QPointF delta = m_points[i] - simplified.last();
        if (delta.manhattanLength() > minDistance) {
            simplified.append(m_points[i]);
        }
    }

    // Always keep the last point
    if (simplified.last() != m_points.last()) {
        simplified.append(m_points.last());
    }

    m_points = simplified;
    rebuildPath();
}

void FreehandLineItem::rebuildPath()
{
    if (m_points.size() < 2) return;

    QPainterPath path;
    path.moveTo(m_points[0]);

    if (m_points.size() == 2) {
        path.lineTo(m_points[1]);
    } else {
        // Smooth curve using cubic bezier through points
        // Using Catmull-Rom-like smoothing
        for (int i = 1; i < m_points.size() - 1; ++i) {
            QPointF midCurrent = (m_points[i - 1] + m_points[i]) / 2.0;
            QPointF midNext = (m_points[i] + m_points[i + 1]) / 2.0;
            path.quadTo(m_points[i], midNext);
        }
        path.lineTo(m_points.last());
    }

    setPath(path);
}

void FreehandLineItem::setTargetRect(const QRectF& newRect)
{
    if (m_points.isEmpty()) return;

    QRectF oldPathBounds = path().boundingRect();
    if (oldPathBounds.width() <= 0.1 || oldPathBounds.height() <= 0.1) return;

    // overlay gives full bounds including pen stroke. Extract math path bounds:
    qreal padding = m_width / 2.0;
    QRectF newPathBounds = newRect.adjusted(padding, padding, -padding, -padding);

    // Guard against inversion/collapse
    if (newPathBounds.width() < 1.0) newPathBounds.setWidth(1.0);
    if (newPathBounds.height() < 1.0) newPathBounds.setHeight(1.0);

    qreal sx = newPathBounds.width() / oldPathBounds.width();
    qreal sy = newPathBounds.height() / oldPathBounds.height();

    for (int i = 0; i < m_points.size(); ++i) {
        qreal nx = newPathBounds.left() + (m_points[i].x() - oldPathBounds.left()) * sx;
        qreal ny = newPathBounds.top() + (m_points[i].y() - oldPathBounds.top()) * sy;
        m_points[i] = QPointF(nx, ny);
    }

    prepareGeometryChange();
    rebuildPath();
}
