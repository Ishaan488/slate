#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QGraphicsScene>
#include <QPointF>
#include <QTimer>

class FreehandLineItem;
class TextNoteItem;
class ImageDropItem;
class ShapeItem;

/**
 * CanvasStore — SQLite persistence layer.
 * Auto-saves canvas items and view state.
 * Restores everything on startup.
 */
class CanvasStore : public QObject {
    Q_OBJECT

public:
    explicit CanvasStore(const QString& dbPath, QObject* parent = nullptr);
    ~CanvasStore() override;

    bool initialize();

    // Save individual items
    void saveItem(FreehandLineItem* item);
    void saveItem(TextNoteItem* item);
    void saveItem(ImageDropItem* item);
    void saveItem(ShapeItem* item);

    // Delete
    void deleteItem(const QString& itemId);

    // Clear all
    void clearAll();

    // Restore all items into a scene
    void restoreItems(QGraphicsScene* scene);

    // View state
    void saveViewState(const QPointF& center, double zoom);
    void restoreViewState(QPointF& center, double& zoom);

    // Generate unique IDs
    static QString newId();

public slots:
    /** Call this to trigger a debounced save of the full scene. */
    void scheduleSave();

signals:
    void saveTriggered();

private:
    void createTables();

    QSqlDatabase m_db;
    QString m_dbPath;
    QTimer* m_saveTimer;

    static constexpr int kSaveDebounceMs = 500;
};
