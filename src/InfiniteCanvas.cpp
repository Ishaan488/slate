#include "InfiniteCanvas.h"
#include <QGraphicsItem>
#include <QPainter>
#include <QScrollBar>
#include <QtMath>
#include <QApplication>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QUrl>
#include <QDebug>
#include "items/ImageDropItem.h"
#include "items/TextNoteItem.h"

InfiniteCanvas::InfiniteCanvas(QWidget* parent)
    : QGraphicsView(parent)
{
    m_scene = new QGraphicsScene(this);
    // Set a very large scene rect to simulate infinite canvas
    m_scene->setSceneRect(-1e6, -1e6, 2e6, 2e6);
    setScene(m_scene);

    // Rendering quality
    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::SmoothPixmapTransform, true);

    // Optimization flags
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, true);
    setCacheMode(QGraphicsView::CacheBackground);

    // No scrollbars — we handle panning ourselves
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Accept drops for image drag-and-drop
    setAcceptDrops(true);
    viewport()->setAcceptDrops(true);

    // Opaque dark background
    setFrameShape(QFrame::NoFrame);
    setStyleSheet("QGraphicsView { background-color: #14141a; border: none; }");

    // Start centered at origin
    centerOn(0, 0);
}

QPointF InfiniteCanvas::viewCenter() const
{
    return mapToScene(viewport()->rect().center());
}

void InfiniteCanvas::restoreViewState(const QPointF& center, double zoom)
{
    // Reset transform then apply saved zoom
    resetTransform();
    m_zoomFactor = zoom;
    scale(m_zoomFactor, m_zoomFactor);
    centerOn(center);
}

// --- Zoom via scroll wheel, centered on cursor ---
void InfiniteCanvas::wheelEvent(QWheelEvent* event)
{
    const double scaleFactor = 1.15;
    QPointF oldPos = mapToScene(event->position().toPoint());

    if (event->angleDelta().y() > 0) {
        // Zoom in
        if (m_zoomFactor * scaleFactor <= kMaxZoom) {
            scale(scaleFactor, scaleFactor);
            m_zoomFactor *= scaleFactor;
        }
    } else {
        // Zoom out
        if (m_zoomFactor / scaleFactor >= kMinZoom) {
            scale(1.0 / scaleFactor, 1.0 / scaleFactor);
            m_zoomFactor /= scaleFactor;
        }
    }

    // Keep the point under the cursor stationary
    QPointF newPos = mapToScene(event->position().toPoint());
    QPointF delta = newPos - oldPos;
    translate(delta.x(), delta.y());

    // Invalidate background cache so grid redraws at new scale
    resetCachedContent();
}

// --- Pan via middle mouse button or Space+Left click ---
void InfiniteCanvas::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton ||
        (event->button() == Qt::LeftButton &&
         QApplication::keyboardModifiers() & Qt::AltModifier))
    {
        m_isPanning = true;
        m_lastPanPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    // Right-click drag to move the entire window (when not embedded)
    if (event->button() == Qt::RightButton) {
        // Let the parent handle window dragging if needed
    }

    QGraphicsView::mousePressEvent(event);
}

void InfiniteCanvas::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isPanning) {
        QPoint delta = event->pos() - m_lastPanPos;
        m_lastPanPos = event->pos();

        // Move scrollbars to pan
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());

        event->accept();
        return;
    }

    QGraphicsView::mouseMoveEvent(event);
}

void InfiniteCanvas::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_isPanning) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }

    QGraphicsView::mouseReleaseEvent(event);
    emit itemModified();
}

void InfiniteCanvas::keyPressEvent(QKeyEvent* event)
{
    // Ctrl+Q to quit
    if (event->key() == Qt::Key_Q &&
        event->modifiers() & Qt::ControlModifier) {
        QApplication::quit();
    }

    // Delete selected items
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        auto* focusItem = m_scene->focusItem();
        bool isEditingText = focusItem && focusItem->type() == TextNoteItem::Type 
                             && static_cast<TextNoteItem*>(focusItem)->isEditing();
        
        if (!isEditingText) {
            auto selected = m_scene->selectedItems();
            for (auto* item : selected) {
                m_scene->removeItem(item);
                delete item;
            }
            emit itemModified();
        }
    }

    // Reset zoom with Ctrl+0
    if (event->key() == Qt::Key_0 &&
        event->modifiers() & Qt::ControlModifier)
    {
        resetTransform();
        m_zoomFactor = 1.0;
        centerOn(0, 0);
        resetCachedContent();
    }

    QGraphicsView::keyPressEvent(event);
}

// --- Draw a subtle dot grid as background ---
void InfiniteCanvas::drawBackground(QPainter* painter, const QRectF& rect)
{
    // Fill with current reactive background color
    painter->fillRect(rect, m_bgColor);

    // Calculate grid spacing based on zoom
    double gridSize = kGridSize;

    // Don't draw grid if zoomed out too far (performance)
    if (m_zoomFactor < 0.15) return;

    // Adjust opacity based on zoom
    int alpha = qBound(30, static_cast<int>(60 * m_zoomFactor), 80);
    QPen dotPen(QColor(255, 255, 255, alpha));
    dotPen.setWidth(1);
    dotPen.setCosmetic(true);  // Constant size regardless of zoom
    painter->setPen(dotPen);

    // Calculate visible area bounds snapped to grid
    double left = qFloor(rect.left() / gridSize) * gridSize;
    double top = qFloor(rect.top() / gridSize) * gridSize;
    double right = rect.right();
    double bottom = rect.bottom();

    // Draw dots
    QVector<QPointF> dots;
    for (double x = left; x <= right; x += gridSize) {
        for (double y = top; y <= bottom; y += gridSize) {
            dots.append(QPointF(x, y));
        }

        // Safety: limit dots per frame
        if (dots.size() > 50000) break;
    }

    painter->drawPoints(dots.data(), dots.size());
}

// --- Image drag and drop ---
void InfiniteCanvas::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        for (const QUrl& url : event->mimeData()->urls()) {
            QString path = url.toLocalFile().toLower();
            if (path.endsWith(".png") || path.endsWith(".jpg") ||
                path.endsWith(".jpeg") || path.endsWith(".bmp") ||
                path.endsWith(".webp") || path.endsWith(".gif"))
            {
                event->setDropAction(Qt::CopyAction);
                event->accept();
                return;
            }
        }
    }
    QGraphicsView::dragEnterEvent(event);
}

void InfiniteCanvas::dragMoveEvent(QDragMoveEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->setDropAction(Qt::CopyAction);
        event->accept();
        return;
    }
    QGraphicsView::dragMoveEvent(event);
}

void InfiniteCanvas::dropEvent(QDropEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        for (const QUrl& url : event->mimeData()->urls()) {
            QString path = url.toLocalFile();
            QString lower = path.toLower();
            if (lower.endsWith(".png") || lower.endsWith(".jpg") ||
                lower.endsWith(".jpeg") || lower.endsWith(".bmp") ||
                lower.endsWith(".webp") || lower.endsWith(".gif"))
            {
                QPointF scenePos = mapToScene(event->position().toPoint());
                auto* imgItem = new ImageDropItem(path, scenePos);
                m_scene->addItem(imgItem);
                qDebug() << "[Slate] Dropped image:" << path;
            }
        }
        event->accept();
        emit itemModified();
        return;
    }
    QGraphicsView::dropEvent(event);
}

void InfiniteCanvas::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    resetCachedContent();
}
