#pragma once

#include <QGraphicsObject>
#include <QPen>
#include <QBrush>
#include <QCursor>

class SelectionOverlay : public QGraphicsObject {
    Q_OBJECT

public:
    explicit SelectionOverlay(QGraphicsItem* parent = nullptr);

    void attachTo(QGraphicsItem* target);
    void detach();
    void updateBounds();

    enum HandleIndex {
        None = 0,
        TopLeft, TopCenter, TopRight,
        RightCenter,
        BottomRight, BottomCenter, BottomLeft,
        LeftCenter
    };

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

signals:
    void resizeFinished();

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    QRectF handleRect(HandleIndex handle) const;
    HandleIndex handleAt(const QPointF& pos) const;
    void applyResize(const QRectF& newRect);

    QGraphicsItem* m_target = nullptr;
    QRectF m_bounds;
    
    HandleIndex m_activeHandle = None;
    QPointF m_dragStartPos;
    QRectF m_dragStartBounds;
    qreal m_dragStartFontSize = 11.0;
    qreal m_dragStartTextWidth = -1.0;

    qreal m_handleSize = 8.0;
};
