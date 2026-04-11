#include "ImageDropItem.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>

ImageDropItem::ImageDropItem(const QString& filePath,
                             const QPointF& scenePos,
                             QGraphicsItem* parent)
    : QGraphicsObject(parent)
    , m_filePath(filePath)
{
    QPixmap pix(filePath);
    if (pix.isNull()) {
        // Fallback: create a placeholder
        pix = QPixmap(200, 150);
        pix.fill(QColor(30, 30, 30));
    }

    // Scale down large images
    if (pix.width() > kMaxDimension || pix.height() > kMaxDimension) {
        pix = pix.scaled(kMaxDimension, kMaxDimension,
                         Qt::KeepAspectRatio,
                         Qt::SmoothTransformation);
    }

    m_pixmap = pix;
    m_rect = QRectF(0, 0, pix.width(), pix.height());

    setPos(scenePos);

    setFlags(QGraphicsItem::ItemIsSelectable |
             QGraphicsItem::ItemIsMovable);


    // Drop shadow
    auto* shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(20);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 120));
    setGraphicsEffect(shadow);
}

void ImageDropItem::setTargetRect(const QRectF& rect)
{
    prepareGeometryChange();
    m_rect = rect;
    update();
}

QRectF ImageDropItem::boundingRect() const
{
    return m_rect;
}

void ImageDropItem::paint(QPainter* painter,
                          const QStyleOptionGraphicsItem* option,
                          QWidget* widget)
{
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);

    // Draw rounded clip for the image
    QRectF imgRect = m_rect;
    QPainterPath clip;
    clip.addRoundedRect(imgRect, 6, 6);
    painter->setClipPath(clip);

    // Draw the pixmap
    painter->drawPixmap(imgRect, m_pixmap, m_pixmap.rect());
    painter->setClipping(false);

    // Selection border
    if (option->state & QStyle::State_Selected) {
        painter->setPen(QPen(QColor(130, 130, 255, 200), 2.0));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(imgRect.adjusted(1, 1, -1, -1), 6, 6);
    }
}
