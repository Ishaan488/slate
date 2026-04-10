#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsSceneMouseEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDebug>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QUrl>
#include <QTimer>
#include <QPainter>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "InfiniteCanvas.h"
#include "DesktopEmbedder.h"
#include "Toolbar.h"
#include "CanvasStore.h"
#include "items/FreehandLineItem.h"
#include "items/TextNoteItem.h"
#include "items/ImageDropItem.h"

/**
 * SlateApp — Main container that manages canvas, toolbar, tools, and persistence.
 * Supports custom resize handles and window dragging for frameless desktop embedding.
 */
class SlateApp : public QWidget {
    Q_OBJECT

public:
    SlateApp(DesktopEmbedder* embedder, QWidget* parent = nullptr)
        : QWidget(parent, Qt::FramelessWindowHint)
        , m_embedder(embedder)
    {
        setMouseTracking(true);
        setMinimumSize(300, 200);

        // --- Canvas ---
        m_canvas = new InfiniteCanvas(this);

        // --- Toolbar ---
        m_toolbar = new Toolbar(this);

        // --- Layout: toolbar pinned to bottom center ---
        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(kResizeMargin, kResizeMargin,
                                       kResizeMargin, kResizeMargin);
        mainLayout->setSpacing(0);

        mainLayout->addWidget(m_canvas, 1);

        auto* toolbarContainer = new QHBoxLayout();
        toolbarContainer->addStretch();
        toolbarContainer->addWidget(m_toolbar);
        toolbarContainer->addStretch();
        mainLayout->addLayout(toolbarContainer);

        setLayout(mainLayout);

        // Opaque dark background for the container
        setStyleSheet("SlateApp { background-color: #0e0e14; }");

        // --- Persistence ---
        QString dbDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QString dbPath = dbDir + "/slate_canvas.db";
        m_store = new CanvasStore(dbPath, this);

        if (m_store->initialize()) {
            // Restore saved items
            m_store->restoreItems(m_canvas->scene());

            // Restore view position/zoom
            QPointF center;
            double zoom;
            m_store->restoreViewState(center, zoom);
            if (zoom > 0) {
                m_canvas->restoreViewState(center, zoom);
            }
        }

        // --- Connect signals ---
        connect(m_toolbar, &Toolbar::toolChanged, this, &SlateApp::onToolChanged);
        connect(m_toolbar, &Toolbar::clearAllRequested, this, &SlateApp::onClearAll);

        // Auto-save on any modification
        connect(m_canvas, &InfiniteCanvas::itemModified, this, &SlateApp::saveAll);
        connect(m_store, &CanvasStore::saveTriggered, this, &SlateApp::saveAll);

        // Install event filter on the scene for tool interactions
        m_canvas->scene()->installEventFilter(this);

        qDebug() << "[Slate] Application initialized";
    }

    ~SlateApp() override
    {
        // Final save before exit
        saveAll();
    }

protected:
    // Handle scene events for drawing/text tools
    bool eventFilter(QObject* obj, QEvent* event) override
    {
        if (obj != m_canvas->scene())
            return QWidget::eventFilter(obj, event);

        auto* scene = m_canvas->scene();

        switch (event->type()) {
        case QEvent::GraphicsSceneMousePress: {
            auto* me = static_cast<QGraphicsSceneMouseEvent*>(event);
            if (me->button() != Qt::LeftButton) break;

            if (m_toolbar->currentTool() == Toolbar::Draw) {
                // Start a new freehand stroke
                m_activeStroke = new FreehandLineItem(
                    me->scenePos(),
                    m_toolbar->currentColor(),
                    m_toolbar->currentPenWidth()
                );
                scene->addItem(m_activeStroke);
                return true;
            }

            if (m_toolbar->currentTool() == Toolbar::Text) {
                // Place a new text note
                auto* textItem = new TextNoteItem("Double-click to edit");
                textItem->setPos(me->scenePos());
                scene->addItem(textItem);

                connect(textItem, &TextNoteItem::editFinished, this, &SlateApp::saveAll);

                m_toolbar->onSelectClicked();  // Switch back to select
                saveAll();
                return true;
            }

            break;
        }

        case QEvent::GraphicsSceneMouseMove: {
            auto* me = static_cast<QGraphicsSceneMouseEvent*>(event);

            if (m_activeStroke) {
                m_activeStroke->addPoint(me->scenePos());
                return true;
            }
            break;
        }

        case QEvent::GraphicsSceneMouseRelease: {
            if (m_activeStroke) {
                m_activeStroke->finishStroke();
                m_activeStroke = nullptr;
                saveAll();
                return true;
            }
            break;
        }

        default:
            break;
        }

        return QWidget::eventFilter(obj, event);
    }

    // --- Custom resize and drag for frameless window ---

    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            m_resizeEdge = hitTestEdge(event->pos());

            if (m_resizeEdge != Edge::None) {
                // Start resize
                m_isResizing = true;
                m_dragStartPos = event->globalPosition().toPoint();
                m_dragStartGeometry = geometry();
                event->accept();
                return;
            }
        }

        // Right-click drag to move the window
        if (event->button() == Qt::RightButton) {
            m_isDragging = true;
            m_dragStartPos = event->globalPosition().toPoint();
            m_dragStartGeometry = geometry();
            setCursor(Qt::SizeAllCursor);
            event->accept();
            return;
        }

        QWidget::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        if (m_isResizing) {
            QPoint delta = event->globalPosition().toPoint() - m_dragStartPos;
            QRect newGeo = m_dragStartGeometry;

            if (m_resizeEdge == Edge::Right || m_resizeEdge == Edge::TopRight ||
                m_resizeEdge == Edge::BottomRight) {
                newGeo.setRight(newGeo.right() + delta.x());
            }
            if (m_resizeEdge == Edge::Left || m_resizeEdge == Edge::TopLeft ||
                m_resizeEdge == Edge::BottomLeft) {
                newGeo.setLeft(newGeo.left() + delta.x());
            }
            if (m_resizeEdge == Edge::Bottom || m_resizeEdge == Edge::BottomLeft ||
                m_resizeEdge == Edge::BottomRight) {
                newGeo.setBottom(newGeo.bottom() + delta.y());
            }
            if (m_resizeEdge == Edge::Top || m_resizeEdge == Edge::TopLeft ||
                m_resizeEdge == Edge::TopRight) {
                newGeo.setTop(newGeo.top() + delta.y());
            }

            // Enforce minimum size
            if (newGeo.width() >= minimumWidth() && newGeo.height() >= minimumHeight()) {
                setGeometry(newGeo);
            }
            event->accept();
            return;
        }

        if (m_isDragging) {
            QPoint delta = event->globalPosition().toPoint() - m_dragStartPos;
            move(m_dragStartGeometry.topLeft() + delta);
            event->accept();
            return;
        }

        // Update cursor based on edge hover
        Edge edge = hitTestEdge(event->pos());
        switch (edge) {
        case Edge::Left: case Edge::Right:
            setCursor(Qt::SizeHorCursor); break;
        case Edge::Top: case Edge::Bottom:
            setCursor(Qt::SizeVerCursor); break;
        case Edge::TopLeft: case Edge::BottomRight:
            setCursor(Qt::SizeFDiagCursor); break;
        case Edge::TopRight: case Edge::BottomLeft:
            setCursor(Qt::SizeBDiagCursor); break;
        default:
            setCursor(Qt::ArrowCursor); break;
        }

        QWidget::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (m_isResizing) {
            m_isResizing = false;
            setCursor(Qt::ArrowCursor);
            event->accept();
            return;
        }

        if (m_isDragging) {
            m_isDragging = false;
            setCursor(Qt::ArrowCursor);
            event->accept();
            return;
        }

        QWidget::mouseReleaseEvent(event);
    }

    void keyPressEvent(QKeyEvent* event) override
    {
        // Tool shortcuts
        if (event->key() == Qt::Key_V) m_toolbar->onSelectClicked();
        if (event->key() == Qt::Key_D) m_toolbar->onDrawClicked();
        if (event->key() == Qt::Key_T) m_toolbar->onTextClicked();

        QWidget::keyPressEvent(event);
    }

    // Paint the resize border
    // Block minimize via Win32 messages
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override
    {
#ifdef Q_OS_WIN
        MSG* msg = static_cast<MSG*>(message);
        if (msg->message == WM_SYSCOMMAND) {
            // Block minimize and close from system menu
            WPARAM cmd = msg->wParam & 0xFFF0;
            if (cmd == SC_MINIMIZE) {
                *result = 0;
                return true;  // Block it
            }
        }
#endif
        return QWidget::nativeEvent(eventType, message, result);
    }

    void paintEvent(QPaintEvent* event) override
    {
        QWidget::paintEvent(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // Draw subtle border around the widget
        QRect borderRect = rect().adjusted(1, 1, -1, -1);
        painter.setPen(QPen(QColor(60, 60, 80), 1.5));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(borderRect, 4, 4);
    }

private slots:
    void onToolChanged(Toolbar::Tool tool)
    {
        // Update cursor based on tool
        switch (tool) {
        case Toolbar::Select:
            m_canvas->setCursor(Qt::ArrowCursor);
            m_canvas->setDragMode(QGraphicsView::RubberBandDrag);
            break;
        case Toolbar::Draw:
            m_canvas->setCursor(Qt::CrossCursor);
            m_canvas->setDragMode(QGraphicsView::NoDrag);
            break;
        case Toolbar::Text:
            m_canvas->setCursor(Qt::IBeamCursor);
            m_canvas->setDragMode(QGraphicsView::NoDrag);
            break;
        }
    }

    void onClearAll()
    {
        m_canvas->scene()->clear();
        m_store->clearAll();
        qDebug() << "[Slate] Canvas cleared";
    }

    void saveAll()
    {
        auto* scene = m_canvas->scene();

        // Save view state
        m_store->saveViewState(m_canvas->viewCenter(), m_canvas->currentZoom());

        // Save each item
        for (auto* item : scene->items()) {
            if (auto* freehand = dynamic_cast<FreehandLineItem*>(item)) {
                m_store->saveItem(freehand);
            } else if (auto* text = dynamic_cast<TextNoteItem*>(item)) {
                m_store->saveItem(text);
            } else if (auto* img = dynamic_cast<ImageDropItem*>(item)) {
                m_store->saveItem(img);
            }
        }
    }

private:
    // Edge detection for resize
    enum class Edge {
        None, Left, Right, Top, Bottom,
        TopLeft, TopRight, BottomLeft, BottomRight
    };

    Edge hitTestEdge(const QPoint& pos) const
    {
        bool left = pos.x() <= kResizeMargin;
        bool right = pos.x() >= width() - kResizeMargin;
        bool top = pos.y() <= kResizeMargin;
        bool bottom = pos.y() >= height() - kResizeMargin;

        if (top && left) return Edge::TopLeft;
        if (top && right) return Edge::TopRight;
        if (bottom && left) return Edge::BottomLeft;
        if (bottom && right) return Edge::BottomRight;
        if (left) return Edge::Left;
        if (right) return Edge::Right;
        if (top) return Edge::Top;
        if (bottom) return Edge::Bottom;
        return Edge::None;
    }

    InfiniteCanvas* m_canvas = nullptr;
    Toolbar* m_toolbar = nullptr;
    CanvasStore* m_store = nullptr;
    DesktopEmbedder* m_embedder = nullptr;

    // Active drawing stroke (null when not drawing)
    FreehandLineItem* m_activeStroke = nullptr;

    // Resize/drag state
    bool m_isResizing = false;
    bool m_isDragging = false;
    Edge m_resizeEdge = Edge::None;
    QPoint m_dragStartPos;
    QRect m_dragStartGeometry;

    static constexpr int kResizeMargin = 6;
};


// =============================================================
//  Entry Point
// =============================================================
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Slate");
    app.setOrganizationName("Slate");

    // --- Desktop Embedding ---
    DesktopEmbedder embedder;

    // Create the main widget
    SlateApp slate(&embedder);
    slate.resize(900, 600);

    // Show the window first so it gets a valid HWND
    slate.show();

    // Attempt to embed into the desktop layer (at icon level)
    bool embedded = embedder.embed(slate.winId());
    if (!embedded) {
        qWarning() << "[Slate] Desktop embedding failed — running as floating window";
        slate.setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnBottomHint);
        slate.show();
    }

    qDebug() << "[Slate] Running. Press Escape to quit.";
    qDebug() << "[Slate] Right-click drag to move. Drag edges to resize.";
    return app.exec();
}

#include "main.moc"
