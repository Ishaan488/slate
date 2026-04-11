#include "TextNoteItem.h"
#include <QGraphicsSceneMouseEvent>
#include <QTextCursor>
#include <QAbstractTextDocumentLayout>
#include <QStyleOptionGraphicsItem>

TextNoteItem::TextNoteItem(const QString& text, QGraphicsItem* parent)
    : QGraphicsTextItem(parent)
{
    setPlainText(text);
    setDefaultTextColor(QColor(230, 230, 240));

    QFont font("Segoe UI", 11);
    setFont(font);

    setFlags(QGraphicsItem::ItemIsSelectable |
             QGraphicsItem::ItemIsMovable |
             QGraphicsItem::ItemIsFocusable);

    // Start as non-editable
    setTextInteractionFlags(Qt::NoTextInteraction);

    // Subtle drop shadow
    auto* shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(15);
    shadow->setOffset(0, 3);
    shadow->setColor(QColor(0, 0, 0, 100));
    setGraphicsEffect(shadow);
}

void TextNoteItem::setBold(bool bold)
{
    QFont f = font();
    f.setBold(bold);
    setFont(f);
    update();
}

void TextNoteItem::setItalic(bool italic)
{
    QFont f = font();
    f.setItalic(italic);
    setFont(f);
    update();
}

void TextNoteItem::increaseFontSize(int delta)
{
    QFont f = font();
    int newSize = qMax(6, f.pointSize() + delta);
    f.setPointSize(newSize);
    setFont(f);
    update();
}

void TextNoteItem::setTextColor(const QColor& color)
{
    setDefaultTextColor(color);
    update();
}

QRectF TextNoteItem::boundingRect() const
{
    QRectF textRect = QGraphicsTextItem::boundingRect();
    return textRect.adjusted(-kPadding, -kPadding, kPadding, kPadding);
}

void TextNoteItem::paint(QPainter* painter,
                         const QStyleOptionGraphicsItem* option,
                         QWidget* widget)
{
    // Draw card background
    painter->setRenderHint(QPainter::Antialiasing);

    QRectF bg = boundingRect();

    // Background fill
    QColor bgColor = m_editing ? QColor(40, 40, 40, 220) : QColor(28, 28, 28, 200);
    painter->setPen(Qt::NoPen);
    painter->setBrush(bgColor);
    painter->drawRoundedRect(bg, kCornerRadius, kCornerRadius);

    // Subtle border
    QColor borderColor = isSelected() ? QColor(130, 130, 255, 150) : QColor(80, 80, 100, 100);
    painter->setPen(QPen(borderColor, 1.0));
    painter->setBrush(Qt::NoBrush);
    painter->drawRoundedRect(bg, kCornerRadius, kCornerRadius);

    // Draw the text itself
    // Temporarily disable the selected decoration from QGraphicsTextItem
    QStyleOptionGraphicsItem opt(*option);
    opt.state &= ~QStyle::State_Selected;
    QGraphicsTextItem::paint(painter, &opt, widget);
}

void TextNoteItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    // Enter edit mode
    m_editing = true;
    setTextInteractionFlags(Qt::TextEditorInteraction);
    setFocus();

    // Place cursor at click position
    QTextCursor cursor = textCursor();
    cursor.setPosition(document()->documentLayout()->hitTest(
        event->pos(), Qt::FuzzyHit));
    setTextCursor(cursor);

    update();
}

void TextNoteItem::focusOutEvent(QFocusEvent* event)
{
    // Exit edit mode
    m_editing = false;
    setTextInteractionFlags(Qt::NoTextInteraction);

    // Clear selection cursor
    QTextCursor cursor = textCursor();
    cursor.clearSelection();
    setTextCursor(cursor);

    update();
    emit editFinished();
    QGraphicsTextItem::focusOutEvent(event);
}
