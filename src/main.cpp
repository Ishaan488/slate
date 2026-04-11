#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsSceneMouseEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDebug>
#include <QSqlQuery>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QUrl>
#include <QTimer>
#include <QPainter>
#include <QScreen>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "InfiniteCanvas.h"
#include "DesktopEmbedder.h"
#include "Toolbar.h"
#include "CanvasStore.h"
#include "items/FreehandLineItem.h"
#include "items/TextNoteItem.h"
#include "items/ShapeItem.h"
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

        connect(m_toolbar, &Toolbar::canvasColorRequested, this, [this](const QColor& color) {
            m_canvas->setCanvasColor(color);
        });

        // Text formatting
        auto applyToTextNodes = [this](auto func) {
            for (auto* item : m_canvas->scene()->selectedItems()) {
                if (auto* textItem = dynamic_cast<TextNoteItem*>(item)) {
                    func(textItem);
                }
            }
            saveAll();
        };

        connect(m_toolbar, &Toolbar::textBoldToggled, this, [=](bool active) {
            applyToTextNodes([=](TextNoteItem* t) { t->setBold(active); });
        });
        connect(m_toolbar, &Toolbar::textItalicToggled, this, [=](bool active) {
            applyToTextNodes([=](TextNoteItem* t) { t->setItalic(active); });
        });
        connect(m_toolbar, &Toolbar::textFontSizeChanged, this, [=](int delta) {
            applyToTextNodes([=](TextNoteItem* t) { t->increaseFontSize(delta * 2); });
        });
        connect(m_toolbar, &Toolbar::textColorRequested, this, [=](const QColor& color) {
            applyToTextNodes([=](TextNoteItem* t) { t->setTextColor(color); });
        });

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

        // Dark background
        setStyleSheet("background-color: #0d0d0d;");

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
            QColor canvasColor;
            m_store->restoreViewState(center, zoom, canvasColor);
            if (zoom > 0) {
                m_canvas->restoreViewState(center, zoom);
            }
            if (canvasColor.isValid()) {
                m_canvas->setCanvasColor(canvasColor);
            }
        }

        // --- Connect signals ---
        connect(m_toolbar, &Toolbar::toolChanged, this, &SlateApp::onToolChanged);
        connect(m_toolbar, &Toolbar::clearAllRequested, this, &SlateApp::onClearAll);

        // Auto-save on any modification
        connect(m_canvas, &InfiniteCanvas::itemModified, this, &SlateApp::saveAll);
        connect(m_store, &CanvasStore::saveTriggered, this, &SlateApp::saveAll);

        // Reliable save on application quit (fires before any destruction)
        connect(qApp, &QApplication::aboutToQuit, this, &SlateApp::saveAll);

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
    // When Win+D natively minimizes us, Qt marks the window as Minimized and STOPS rendering.
    // If our native Win32 timer brings the window back, Qt still thinks it's minimized!
    // We MUST intercept the state change and tell Qt it's no longer minimized.
    void changeEvent(QEvent* event) override
    {
        if (event->type() == QEvent::WindowStateChange) {
            if (windowState() & Qt::WindowMinimized) {
                // Instantly tell Qt we are NOT minimized
                QTimer::singleShot(50, this, [this]() {
                    setWindowState(windowState() & ~Qt::WindowMinimized);
                    show(); // force a re-render
                });
            }
        }
        QWidget::changeEvent(event);
    }

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

            if (m_toolbar->currentTool() == Toolbar::Shapes) {
                // Determine ShapeClass from Toolbar
                ShapeItem::ShapeClass sClass = ShapeItem::Square;
                if (m_toolbar->currentShape() == Toolbar::Circle) sClass = ShapeItem::Circle;
                if (m_toolbar->currentShape() == Toolbar::Triangle) sClass = ShapeItem::Triangle;

                m_shapeStartPos = me->scenePos();
                m_activeShape = new ShapeItem(sClass, m_toolbar->currentColor(), m_toolbar->currentPenWidth());
                m_activeShape->setRect(QRectF(m_shapeStartPos, QSizeF(0, 0)));
                scene->addItem(m_activeShape);
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

            if (m_activeShape) {
                // Build rectangle from start to current
                QRectF rect(m_shapeStartPos, me->scenePos());
                m_activeShape->setRect(rect.normalized());
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

            if (m_activeShape) {
                // Finalize
                m_activeShape = nullptr;
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
            QPoint newPos = m_dragStartGeometry.topLeft() + delta;

            // Clamp to screen bounds so widget stays fully visible
            QRect screen = QApplication::primaryScreen()->geometry();
            int maxX = screen.width() - width();
            int maxY = screen.height() - height();
            newPos.setX(qBound(0, newPos.x(), maxX));
            newPos.setY(qBound(0, newPos.y(), maxY));

            move(newPos);
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
        case Toolbar::Shapes:
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
        auto selected = m_canvas->scene()->selectedItems();
        if (selected.isEmpty()) return;
        for (auto* item : selected) {
            m_canvas->scene()->removeItem(item);
            delete item;
        }
        saveAll();
        qDebug() << "[Slate] Deleted" << selected.size() << "selected items";
    }

    void saveAll()
    {
        if (!m_store || !m_canvas || !m_canvas->scene()) return;

        auto* scene = m_canvas->scene();

        // Save view state (including canvas background color)
        m_store->saveViewState(m_canvas->viewCenter(), m_canvas->currentZoom(), m_canvas->canvasColor());

        // Wipe old items, then re-insert only what's still on the scene
        {
            QSqlQuery q(m_store->database());
            q.exec("DELETE FROM canvas_items");
        }

        // Save each item
        int count = 0;
        for (auto* item : scene->items()) {
            if (auto* freehand = dynamic_cast<FreehandLineItem*>(item)) {
                m_store->saveItem(freehand); ++count;
            } else if (auto* text = dynamic_cast<TextNoteItem*>(item)) {
                m_store->saveItem(text); ++count;
            } else if (auto* img = dynamic_cast<ImageDropItem*>(item)) {
                m_store->saveItem(img); ++count;
            } else if (auto* shape = dynamic_cast<ShapeItem*>(item)) {
                m_store->saveItem(shape); ++count;
            }
        }
        qDebug() << "[Slate] Saved" << count << "items to database";
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

    // Active components
    ShapeItem* m_activeShape = nullptr;
    QPointF m_shapeStartPos;

    static constexpr int kResizeMargin = 6;
};


// =============================================================
//  Entry Point
// =============================================================
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QStyle>
#include <QSettings>
#include <QDir>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Slate");
    app.setOrganizationName("Slate");

    // Don't quit when the last window is hidden (we live in the tray)
    app.setQuitOnLastWindowClosed(false);

    // --- Widget Setup ---
    DesktopEmbedder embedder;

    SlateApp slate(&embedder);
    slate.resize(900, 600);
    slate.show();

    // Apply widget behavior: hide from taskbar + start anti-minimize timer
    embedder.embed(slate.winId());

    QIcon appIcon(":/logo.png");
    app.setWindowIcon(appIcon);

    // --- System Tray Icon ---
    QSystemTrayIcon trayIcon;
    trayIcon.setIcon(appIcon);
    trayIcon.setToolTip("Slate — Desktop Canvas Widget");

    QMenu trayMenu;

    QAction* toggleAction = trayMenu.addAction("Disable Widget");
    QObject::connect(toggleAction, &QAction::triggered, [&]() {
        if (embedder.isWidgetEnabled()) {
            embedder.setWidgetEnabled(false);
            toggleAction->setText("Enable Widget");
            trayIcon.showMessage("Slate", "Widget disabled. Right-click tray icon to re-enable.",
                                 QSystemTrayIcon::Information, 2000);
        } else {
            embedder.setWidgetEnabled(true);
            toggleAction->setText("Disable Widget");
        }
    });

    QSettings bootUpSettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    QString appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    
    QAction* autoStartAction = trayMenu.addAction("Start with Windows");
    autoStartAction->setCheckable(true);
    autoStartAction->setChecked(bootUpSettings.contains("SlateWidget"));
    
    QObject::connect(autoStartAction, &QAction::toggled, [=](bool checked) {
        QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
        if (checked) {
            settings.setValue("SlateWidget", appPath);
        } else {
            settings.remove("SlateWidget");
        }
    });

    trayMenu.addSeparator();

    QAction* quitAction = trayMenu.addAction("Quit Slate");
    QObject::connect(quitAction, &QAction::triggered, &app, &QApplication::quit);

    trayIcon.setContextMenu(&trayMenu);
    trayIcon.show();

    // Double-click tray icon to toggle
    QObject::connect(&trayIcon, &QSystemTrayIcon::activated,
                     [&](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick) {
            toggleAction->trigger();
        }
    });

    qDebug() << "[Slate] Widget running. Use system tray icon to control.";
    qDebug() << "[Slate] Right-click drag to move. Drag edges to resize. Ctrl+Q to quit.";
    return app.exec();
}

#include "main.moc"

