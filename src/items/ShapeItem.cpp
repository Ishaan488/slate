#include "ShapeItem.h"
#include <QStyleOptionGraphicsItem>
#include <QGraphicsDropShadowEffect>

ShapeItem::ShapeItem(ShapeClass shapeClass, const QColor& color, qreal penWidth, QGraphicsItem* parent)
    : QGraphicsPathItem(parent)
    , m_shapeClass(shapeClass)
    , m_color(color)
    , m_penWidth(penWidth)
{
    setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
    
    QPen pen(m_color, m_penWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    setPen(pen);
    
    // Slight translucent fill for filled shapes look over dark canvas
    QColor fillColor = m_color;
    fillColor.setAlpha(40);
    setBrush(QBrush(fillColor));

    // Optional subtle shadow
    auto* shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(10);
    shadow->setOffset(0, 2);
    shadow->setColor(QColor(0, 0, 0, 100));
    setGraphicsEffect(shadow);
}

void ShapeItem::setRect(const QRectF& rect)
{
    m_rect = rect;
    updatePath();
}

void ShapeItem::updatePath()
{
    QPainterPath path;
    
    if (m_rect.isNull()) {
        setPath(path);
        return;
    }

    if (m_shapeClass == Square) {
        path.addRect(m_rect);
    } 
    else if (m_shapeClass == Circle) {
        path.addEllipse(m_rect);
    } 
    else if (m_shapeClass == Triangle) {
        path.moveTo(m_rect.center().x(), m_rect.top());
        path.lineTo(m_rect.right(), m_rect.bottom());
        path.lineTo(m_rect.left(), m_rect.bottom());
        path.closeSubpath();
    }
    else if (m_shapeClass == Line || m_shapeClass == Arrow) {
        path.moveTo(m_rect.topLeft());
        path.lineTo(m_rect.bottomRight());

        if (m_shapeClass == Arrow) {
            qreal dx = m_rect.bottomRight().x() - m_rect.topLeft().x();
            qreal dy = m_rect.bottomRight().y() - m_rect.topLeft().y();
            qreal angle = std::atan2(dy, dx);
            
            qreal arrowSize = 12.0 + m_penWidth * 1.5;
            QPointF p1 = m_rect.bottomRight() - QPointF(cos(angle - M_PI/6.0) * arrowSize, sin(angle - M_PI/6.0) * arrowSize);
            QPointF p2 = m_rect.bottomRight() - QPointF(cos(angle + M_PI/6.0) * arrowSize, sin(angle + M_PI/6.0) * arrowSize);
            
            QPainterPath arrowHead;
            arrowHead.moveTo(p1);
            arrowHead.lineTo(m_rect.bottomRight());
            arrowHead.lineTo(p2);
            path.addPath(arrowHead);
        }
    }

    setPath(path);
}

void ShapeItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    painter->setRenderHint(QPainter::Antialiasing);

    // Apply distinct visual state when selected
    QPen customPen = pen();
    if (option->state & QStyle::State_Selected) {
        customPen.setColor(QColor(255, 255, 255)); // Highlight border on select
        // add dashed line around it
        QPen dashed(Qt::DashLine);
        dashed.setColor(QColor(130, 130, 255));
        dashed.setWidthF(1.5);
        painter->setPen(dashed);
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(shape());
    }

    painter->setPen(customPen);
    painter->setBrush(brush());
    painter->drawPath(path());
}

QPainterPath ShapeItem::shape() const
{
    QPainterPath p;
    // Add stroker to allow selecting by clicking the stroke
    QPainterPathStroker stroker;
    stroker.setWidth(m_penWidth + 10); // Generous hit area
    p.addPath(stroker.createStroke(path()));
    p.addPath(path()); // Also hit the filled area
    return p;
}
