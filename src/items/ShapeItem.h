#pragma once

#include <QGraphicsPathItem>
#include <QPen>
#include <QBrush>
#include <QPainter>

/**
 * ShapeItem — Represents a geometric shape (Square, Circle, Triangle) on the canvas.
 */
class ShapeItem : public QGraphicsPathItem {
public:
    enum ShapeClass { Square, Circle, Triangle };

    explicit ShapeItem(ShapeClass shapeClass, const QColor& color, qreal penWidth, QGraphicsItem* parent = nullptr);

    // Dynamic updating while drawing (dragging)
    void setRect(const QRectF& rect);

    void setItemId(const QString& id) { m_id = id; }
    QString itemId() const { return m_id; }

    ShapeClass shapeClass() const { return m_shapeClass; }
    QColor color() const { return m_color; }
    qreal penWidth() const { return m_penWidth; }
    QRectF rect() const { return m_rect; }

    enum { Type = UserType + 3 };
    int type() const override { return Type; }

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
    QPainterPath shape() const override;

private:
    void updatePath();

    QString m_id;
    ShapeClass m_shapeClass;
    QRectF m_rect;
    QColor m_color;
    qreal m_penWidth;
};
