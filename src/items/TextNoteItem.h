#pragma once

#include <QGraphicsTextItem>
#include <QGraphicsDropShadowEffect>
#include <QFont>
#include <QPainter>

/**
 * TextNoteItem — An editable text note on the canvas.
 * Renders with a semi-transparent background card.
 * Double-click to edit, click away to deselect.
 */
class TextNoteItem : public QGraphicsTextItem {
    Q_OBJECT

public:
    explicit TextNoteItem(const QString& text = "Double-click to edit",
                          QGraphicsItem* parent = nullptr);

    // Serialization
    void setItemId(const QString& id) { m_id = id; }
    QString itemId() const { return m_id; }

    enum { Type = UserType + 2 };
    int type() const override { return Type; }

    bool isEditing() const { return m_editing; }

signals:
    void editFinished();

protected:
    void paint(QPainter* painter,
               const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    QRectF boundingRect() const override;

private:
    QString m_id;
    bool m_editing = false;
    static constexpr qreal kPadding = 12.0;
    static constexpr qreal kCornerRadius = 8.0;
};
