#pragma once

#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QGraphicsSceneMouseEvent>
#include <QPen>

/**
 * FreehandLineItem — A smooth freehand stroke drawn on the canvas.
 * Built from mouse drag events, simplified after stroke completion.
 */
class FreehandLineItem : public QGraphicsPathItem {
public:
    FreehandLineItem(const QPointF& startPoint,
                     const QColor& color = QColor(200, 200, 255),
                     qreal width = 2.5,
                     QGraphicsItem* parent = nullptr);

    // Serialization
    QVector<QPointF> points() const { return m_points; }
    QColor strokeColor() const { return m_color; }
    qreal strokeWidth() const { return m_width; }

    void addPoint(const QPointF& point);
    void finishStroke();

    // ID for persistence
    void setItemId(const QString& id) { m_id = id; }
    QString itemId() const { return m_id; }

    enum { Type = UserType + 1 };
    int type() const override { return Type; }

private:
    QVector<QPointF> m_points;
    QColor m_color;
    qreal m_width;
    QString m_id;

    void rebuildPath();
};
