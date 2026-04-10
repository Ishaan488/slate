#pragma once

#include <QGraphicsPixmapItem>
#include <QGraphicsDropShadowEffect>
#include <QPixmap>

/**
 * ImageDropItem — An image placed on the canvas via drag-and-drop.
 * Stores the original file path for persistence.
 */
class ImageDropItem : public QGraphicsPixmapItem {
public:
    explicit ImageDropItem(const QString& filePath,
                           const QPointF& scenePos,
                           QGraphicsItem* parent = nullptr);

    // Serialization
    void setItemId(const QString& id) { m_id = id; }
    QString itemId() const { return m_id; }
    QString filePath() const { return m_filePath; }

    enum { Type = UserType + 3 };
    int type() const override { return Type; }

protected:
    void paint(QPainter* painter,
               const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

private:
    QString m_id;
    QString m_filePath;

    static constexpr int kMaxDimension = 500;  // Max width/height on drop
};
