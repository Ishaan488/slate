#pragma once

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>

class InfiniteCanvas : public QGraphicsView {
    Q_OBJECT

public:
    explicit InfiniteCanvas(QWidget* parent = nullptr);
    ~InfiniteCanvas() override = default;

    QGraphicsScene* scene() const { return m_scene; }

    // View state for persistence
    double currentZoom() const { return m_zoomFactor; }
    QPointF viewCenter() const;
    void restoreViewState(const QPointF& center, double zoom);

signals:
    void itemModified();   // Emitted when any item is moved/changed
    void canvasClicked(const QPointF& scenePos);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void drawBackground(QPainter* painter, const QRectF& rect) override;
    void resizeEvent(QResizeEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    QGraphicsScene* m_scene;
    double m_zoomFactor = 1.0;

    // Pan state
    bool m_isPanning = false;
    QPoint m_lastPanPos;

    // Zoom limits
    static constexpr double kMinZoom = 0.05;
    static constexpr double kMaxZoom = 10.0;

    // Grid
    static constexpr double kGridSize = 40.0;
};
