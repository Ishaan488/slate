#include "ImageDropItem.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>

ImageDropItem::ImageDropItem(const QString& filePath,
                             const QPointF& scenePos,
                             QGraphicsItem* parent)
    : QGraphicsPixmapItem(parent)
    , m_filePath(filePath)
{
    QPixmap pix(filePath);
    if (pix.isNull()) {
        // Fallback: create a placeholder
        pix = QPixmap(200, 150);
        pix.fill(QColor(60, 60, 80));
    }

    // Scale down large images
    if (pix.width() > kMaxDimension || pix.height() > kMaxDimension) {
        pix = pix.scaled(kMaxDimension, kMaxDimension,
                         Qt::KeepAspectRatio,
                         Qt::SmoothTransformation);
    }

    setPixmap(pix);
    setPos(scenePos);

    setFlags(QGraphicsItem::ItemIsSelectable |
             QGraphicsItem::ItemIsMovable);

    // Enable smooth scaling
    setTransformationMode(Qt::SmoothTransformation);

    // Drop shadow
    auto* shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(20);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 120));
    setGraphicsEffect(shadow);
}

void ImageDropItem::paint(QPainter* painter,
                          const QStyleOptionGraphicsItem* option,
                          QWidget* widget)
{
    painter->setRenderHint(QPainter::Antialiasing);

    // Draw rounded clip for the image
    QRectF imgRect = boundingRect();
    QPainterPath clip;
    clip.addRoundedRect(imgRect, 6, 6);
    painter->setClipPath(clip);

    // Draw the pixmap
    QGraphicsPixmapItem::paint(painter, option, widget);
    painter->setClipping(false);

    // Selection border
    if (option->state & QStyle::State_Selected) {
        painter->setPen(QPen(QColor(130, 130, 255, 200), 2.0));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(imgRect.adjusted(1, 1, -1, -1), 6, 6);
    }
}
