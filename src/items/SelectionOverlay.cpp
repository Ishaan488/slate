#include "SelectionOverlay.h"
#include "ShapeItem.h"
#include "TextNoteItem.h"
#include "ImageDropItem.h"
#include "FreehandLineItem.h"

#include <QPainter>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>

SelectionOverlay::SelectionOverlay(QGraphicsItem* parent)
    : QGraphicsObject(parent)
{
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setZValue(1000); // Ensure it draws above the item
    hide();
}

void SelectionOverlay::attachTo(QGraphicsItem* target)
{
    if (m_target == target) {
        updateBounds();
        return;
    }
    m_target = target;
    setParentItem(target);
    updateBounds();
    show();
}

void SelectionOverlay::detach()
{
    m_target = nullptr;
    setParentItem(nullptr);
    hide();
}

void SelectionOverlay::updateBounds()
{
    if (!m_target) return;
    prepareGeometryChange();
    m_bounds = m_target->boundingRect();
}

QRectF SelectionOverlay::boundingRect() const
{
    qreal margin = m_handleSize;
    return m_bounds.adjusted(-margin, -margin, margin, margin);
}

QRectF SelectionOverlay::handleRect(HandleIndex handle) const
{
    qreal sz = m_handleSize;
    qreal hw = sz / 2.0;

    qreal l = m_bounds.left();
    qreal r = m_bounds.right();
    qreal t = m_bounds.top();
    qreal b = m_bounds.bottom();
    qreal cx = m_bounds.center().x();
    qreal cy = m_bounds.center().y();

    switch (handle) {
        case TopLeft:      return QRectF(l - hw, t - hw, sz, sz);
        case TopCenter:    return QRectF(cx - hw, t - hw, sz, sz);
        case TopRight:     return QRectF(r - hw, t - hw, sz, sz);
        case RightCenter:  return QRectF(r - hw, cy - hw, sz, sz);
        case BottomRight:  return QRectF(r - hw, b - hw, sz, sz);
        case BottomCenter: return QRectF(cx - hw, b - hw, sz, sz);
        case BottomLeft:   return QRectF(l - hw, b - hw, sz, sz);
        case LeftCenter:   return QRectF(l - hw, cy - hw, sz, sz);
        default: return QRectF();
    }
}

SelectionOverlay::HandleIndex SelectionOverlay::handleAt(const QPointF& pos) const
{
    for (int i = 1; i <= LeftCenter; ++i) {
        if (handleRect(static_cast<HandleIndex>(i)).contains(pos)) {
            return static_cast<HandleIndex>(i);
        }
    }
    return None;
}

void SelectionOverlay::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    if (!m_target) return;

    painter->setRenderHint(QPainter::Antialiasing);

    // Draw bounds outline
    painter->setPen(QPen(QColor(0, 120, 215, 180), 1.0, Qt::DashLine));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(m_bounds);

    // Draw handles
    painter->setBrush(QColor(255, 255, 255));
    painter->setPen(QPen(QColor(0, 120, 215), 1.5));

    for (int i = 1; i <= LeftCenter; ++i) {
        painter->drawRect(handleRect(static_cast<HandleIndex>(i)));
    }
}

void SelectionOverlay::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    HandleIndex h = handleAt(event->pos());
    switch (h) {
        case TopLeft:
        case BottomRight:
            setCursor(Qt::SizeFDiagCursor); break;
        case TopRight:
        case BottomLeft:
            setCursor(Qt::SizeBDiagCursor); break;
        case TopCenter:
        case BottomCenter:
            setCursor(Qt::SizeVerCursor); break;
        case LeftCenter:
        case RightCenter:
            setCursor(Qt::SizeHorCursor); break;
        default:
            setCursor(Qt::ArrowCursor); break;
    }
    QGraphicsObject::hoverMoveEvent(event);
}

void SelectionOverlay::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    m_activeHandle = handleAt(event->pos());
    if (m_activeHandle != None) {
        m_dragStartPos = event->pos();
        m_dragStartBounds = m_bounds;
        if (auto text = dynamic_cast<TextNoteItem*>(m_target)) {
            m_dragStartFontSize = text->font().pointSizeF();
            if (m_dragStartFontSize <= 0) m_dragStartFontSize = 11.0;
            m_dragStartTextWidth = text->textWidth();
        }
        event->accept();
    } else {
        event->ignore(); // pass through to item for dragging
    }
}

void SelectionOverlay::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (m_activeHandle == None) {
        event->ignore();
        return;
    }

    QPointF diff = event->pos() - m_dragStartPos;
    QRectF r = m_dragStartBounds;

    // Expand or shrink depending on which handle
    switch (m_activeHandle) {
        case TopLeft:      r.setTopLeft(r.topLeft() + diff); break;
        case TopCenter:    r.setTop(r.top() + diff.y()); break;
        case TopRight:     r.setTopRight(r.topRight() + diff); break;
        case RightCenter:  r.setRight(r.right() + diff.x()); break;
        case BottomRight:  r.setBottomRight(r.bottomRight() + diff); break;
        case BottomCenter: r.setBottom(r.bottom() + diff.y()); break;
        case BottomLeft:   r.setBottomLeft(r.bottomLeft() + diff); break;
        case LeftCenter:   r.setLeft(r.left() + diff.x()); break;
        default: break;
    }

    // Minimum size enforcement
    if (r.width() < 20) {
        if (m_activeHandle == LeftCenter || m_activeHandle == TopLeft || m_activeHandle == BottomLeft)
            r.setLeft(r.right() - 20);
        else
            r.setRight(r.left() + 20);
    }
    if (r.height() < 20) {
        if (m_activeHandle == TopCenter || m_activeHandle == TopLeft || m_activeHandle == TopRight)
            r.setTop(r.bottom() - 20);
        else
            r.setBottom(r.top() + 20);
    }

    applyResize(r);
    updateBounds(); // Sync our bounds with item's newly calculated bounds
}

void SelectionOverlay::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (m_activeHandle != None) {
        m_activeHandle = None;
        setCursor(Qt::ArrowCursor);
        emit resizeFinished();
        event->accept();
    } else {
        event->ignore();
    }
}

void SelectionOverlay::applyResize(const QRectF& newRect)
{
    if (!m_target) return;

    if (auto shape = dynamic_cast<ShapeItem*>(m_target)) {
        shape->setRect(newRect);
    } 
    else if (auto text = dynamic_cast<TextNoteItem*>(m_target)) {
        bool isCorner = (m_activeHandle == TopLeft || m_activeHandle == TopRight || 
                         m_activeHandle == BottomLeft || m_activeHandle == BottomRight);

        if (isCorner) {
            // Scale font size dynamically based on height resize ratio
            qreal ratio = newRect.height() / m_dragStartBounds.height();
            qreal newSize = m_dragStartFontSize * ratio;
            if (newSize < 4.0) newSize = 4.0; // Enforce minimum visual font size

            QFont f = text->font();
            f.setPointSizeF(newSize);
            text->setFont(f);

            // Proportional formatting to match new font size exactly so words don't wrap differently
            if (m_dragStartTextWidth > 0) {
                text->setTextWidth(m_dragStartTextWidth * ratio);
            }
        } else {
            // Apply dynamic width formatting (word wrapping) without changing font size
            text->setTextWidth(newRect.width());
        }
        
        // If left edge moved, shift the object's pos() to compensate
        if (m_activeHandle == TopLeft || m_activeHandle == LeftCenter || m_activeHandle == BottomLeft) {
            QPointF shift = m_target->mapToScene(QPointF(newRect.left() - m_bounds.left(), 0));
            QPointF origin = m_target->mapToScene(QPointF(0,0));
            m_target->setPos(m_target->pos() + (shift - origin));
        }
    }
    else if (auto img = dynamic_cast<ImageDropItem*>(m_target)) {
        img->setTargetRect(newRect);
    }
    else if (auto freehand = dynamic_cast<FreehandLineItem*>(m_target)) {
        freehand->setTargetRect(newRect);
    }
}
