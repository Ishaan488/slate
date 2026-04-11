#include "CanvasStore.h"
#include "items/FreehandLineItem.h"
#include "items/TextNoteItem.h"
#include "items/ImageDropItem.h"
#include "items/ShapeItem.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QUuid>
#include <QDebug>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>

CanvasStore::CanvasStore(const QString& dbPath, QObject* parent)
    : QObject(parent)
    , m_dbPath(dbPath)
{
    m_saveTimer = new QTimer(this);
    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(kSaveDebounceMs);
    connect(m_saveTimer, &QTimer::timeout, this, &CanvasStore::saveTriggered);
}

CanvasStore::~CanvasStore()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool CanvasStore::initialize()
{
    // Ensure directory exists
    QDir dir(QFileInfo(m_dbPath).absolutePath());
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    m_db = QSqlDatabase::addDatabase("QSQLITE", "slate_canvas");
    m_db.setDatabaseName(m_dbPath);

    if (!m_db.open()) {
        qWarning() << "[CanvasStore] Failed to open database:" << m_db.lastError().text();
        return false;
    }

    createTables();
    qDebug() << "[CanvasStore] Database opened at" << m_dbPath;
    return true;
}

void CanvasStore::createTables()
{
    QSqlQuery q(m_db);

    q.exec(R"(
        CREATE TABLE IF NOT EXISTS canvas_items (
            id TEXT PRIMARY KEY,
            type TEXT NOT NULL,
            x REAL NOT NULL DEFAULT 0,
            y REAL NOT NULL DEFAULT 0,
            scale REAL DEFAULT 1.0,
            z_index INTEGER DEFAULT 0,
            rotation REAL DEFAULT 0.0,
            data TEXT,
            created_at TEXT,
            updated_at TEXT
        )
    )");

    q.exec(R"(
        CREATE TABLE IF NOT EXISTS canvas_state (
            key TEXT PRIMARY KEY,
            value TEXT
        )
    )");
}

void CanvasStore::scheduleSave()
{
    m_saveTimer->start();  // Restart the debounce timer
}

// --- Save items ---

void CanvasStore::saveItem(FreehandLineItem* item)
{
    if (item->itemId().isEmpty()) {
        item->setItemId(newId());
    }

    // Serialize points to JSON
    QJsonArray pointsArray;
    for (const QPointF& pt : item->points()) {
        QJsonObject ptObj;
        ptObj["x"] = pt.x();
        ptObj["y"] = pt.y();
        pointsArray.append(ptObj);
    }

    QJsonObject data;
    data["points"] = pointsArray;
    data["color"] = item->strokeColor().name(QColor::HexArgb);
    data["width"] = item->strokeWidth();

    QSqlQuery q(m_db);
    q.prepare(R"(
        INSERT OR REPLACE INTO canvas_items
        (id, type, x, y, scale, z_index, rotation, data, created_at, updated_at)
        VALUES (?, 'freehand', ?, ?, ?, ?, ?, ?, datetime('now'), datetime('now'))
    )");
    q.addBindValue(item->itemId());
    q.addBindValue(item->pos().x());
    q.addBindValue(item->pos().y());
    q.addBindValue(item->scale());
    q.addBindValue(static_cast<int>(item->zValue()));
    q.addBindValue(item->rotation());
    q.addBindValue(QJsonDocument(data).toJson(QJsonDocument::Compact));

    if (!q.exec()) {
        qWarning() << "[CanvasStore] Failed to save freehand item:" << q.lastError().text();
    }
}

void CanvasStore::saveItem(TextNoteItem* item)
{
    if (item->itemId().isEmpty()) {
        item->setItemId(newId());
    }

    QJsonObject data;
    data["text"] = item->toPlainText();

    QSqlQuery q(m_db);
    q.prepare(R"(
        INSERT OR REPLACE INTO canvas_items
        (id, type, x, y, scale, z_index, rotation, data, created_at, updated_at)
        VALUES (?, 'text', ?, ?, ?, ?, ?, ?, datetime('now'), datetime('now'))
    )");
    q.addBindValue(item->itemId());
    q.addBindValue(item->pos().x());
    q.addBindValue(item->pos().y());
    q.addBindValue(item->scale());
    q.addBindValue(static_cast<int>(item->zValue()));
    q.addBindValue(item->rotation());
    q.addBindValue(QJsonDocument(data).toJson(QJsonDocument::Compact));

    if (!q.exec()) {
        qWarning() << "[CanvasStore] Failed to save text item:" << q.lastError().text();
    }
}

void CanvasStore::saveItem(ImageDropItem* item)
{
    if (item->itemId().isEmpty()) {
        item->setItemId(newId());
    }

    QJsonObject data;
    data["filePath"] = item->filePath();

    QSqlQuery q(m_db);
    q.prepare(R"(
        INSERT OR REPLACE INTO canvas_items
        (id, type, x, y, scale, z_index, rotation, data, created_at, updated_at)
        VALUES (?, 'image', ?, ?, ?, ?, ?, ?, datetime('now'), datetime('now'))
    )");
    q.addBindValue(item->itemId());
    q.addBindValue(item->pos().x());
    q.addBindValue(item->pos().y());
    q.addBindValue(item->scale());
    q.addBindValue(static_cast<int>(item->zValue()));
    q.addBindValue(item->rotation());
    q.addBindValue(QJsonDocument(data).toJson(QJsonDocument::Compact));

    if (!q.exec()) {
        qWarning() << "[CanvasStore] Failed to save image item:" << q.lastError().text();
    }
}

void CanvasStore::saveItem(ShapeItem* item)
{
    if (item->itemId().isEmpty()) {
        item->setItemId(newId());
    }

    QJsonObject data;
    data["shapeClass"] = static_cast<int>(item->shapeClass());
    data["color"] = item->color().name(QColor::HexArgb);
    data["width"] = item->penWidth();
    
    QRectF r = item->rect();
    data["rect_x"] = r.x();
    data["rect_y"] = r.y();
    data["rect_w"] = r.width();
    data["rect_h"] = r.height();

    QSqlQuery q(m_db);
    q.prepare(R"(
        INSERT OR REPLACE INTO canvas_items
        (id, type, x, y, scale, z_index, rotation, data, created_at, updated_at)
        VALUES (?, 'shape', ?, ?, ?, ?, ?, ?, datetime('now'), datetime('now'))
    )");
    q.addBindValue(item->itemId());
    q.addBindValue(item->pos().x());
    q.addBindValue(item->pos().y());
    q.addBindValue(item->scale());
    q.addBindValue(static_cast<int>(item->zValue()));
    q.addBindValue(item->rotation());
    q.addBindValue(QJsonDocument(data).toJson(QJsonDocument::Compact));

    if (!q.exec()) {
        qWarning() << "[CanvasStore] Failed to save shape item:" << q.lastError().text();
    }
}

void CanvasStore::deleteItem(const QString& itemId)
{
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM canvas_items WHERE id = ?");
    q.addBindValue(itemId);
    q.exec();
}

void CanvasStore::clearAll()
{
    QSqlQuery q(m_db);
    q.exec("DELETE FROM canvas_items");
    q.exec("DELETE FROM canvas_state");
    qDebug() << "[CanvasStore] Cleared all data";
}

// --- Restore ---

void CanvasStore::restoreItems(QGraphicsScene* scene)
{
    QSqlQuery q(m_db);
    q.exec("SELECT id, type, x, y, scale, z_index, rotation, data FROM canvas_items");

    int count = 0;
    while (q.next()) {
        QString id = q.value(0).toString();
        QString type = q.value(1).toString();
        qreal x = q.value(2).toDouble();
        qreal y = q.value(3).toDouble();
        qreal itemScale = q.value(4).toDouble();
        int zIndex = q.value(5).toInt();
        qreal rotation = q.value(6).toDouble();
        QJsonDocument dataDoc = QJsonDocument::fromJson(q.value(7).toString().toUtf8());
        QJsonObject data = dataDoc.object();

        if (type == "freehand") {
            QJsonArray pointsArr = data["points"].toArray();
            if (pointsArr.isEmpty()) continue;

            QColor color(data["color"].toString());
            qreal width = data["width"].toDouble();

            QPointF firstPt(pointsArr[0].toObject()["x"].toDouble(),
                            pointsArr[0].toObject()["y"].toDouble());
            auto* item = new FreehandLineItem(firstPt, color, width);

            for (int i = 1; i < pointsArr.size(); ++i) {
                QJsonObject ptObj = pointsArr[i].toObject();
                item->addPoint(QPointF(ptObj["x"].toDouble(), ptObj["y"].toDouble()));
            }

            item->setItemId(id);
            item->setPos(x, y);
            item->setScale(itemScale);
            item->setZValue(zIndex);
            item->setRotation(rotation);
            scene->addItem(item);
            ++count;

        } else if (type == "text") {
            auto* item = new TextNoteItem(data["text"].toString());
            item->setItemId(id);
            item->setPos(x, y);
            item->setScale(itemScale);
            item->setZValue(zIndex);
            item->setRotation(rotation);
            scene->addItem(item);
            ++count;

        } else if (type == "image") {
            QString filePath = data["filePath"].toString();
            auto* item = new ImageDropItem(filePath, QPointF(x, y));
            item->setItemId(id);
            item->setScale(itemScale);
            item->setZValue(zIndex);
            item->setRotation(rotation);
            scene->addItem(item);
            ++count;

        } else if (type == "shape") {
            ShapeItem::ShapeClass sClass = static_cast<ShapeItem::ShapeClass>(data["shapeClass"].toInt());
            QColor color(data["color"].toString());
            qreal width = data["width"].toDouble();
            QRectF rect(data["rect_x"].toDouble(), data["rect_y"].toDouble(),
                        data["rect_w"].toDouble(), data["rect_h"].toDouble());
            
            auto* item = new ShapeItem(sClass, color, width);
            item->setRect(rect);
            item->setItemId(id);
            item->setPos(x, y);
            item->setScale(itemScale);
            item->setZValue(zIndex);
            item->setRotation(rotation);
            scene->addItem(item);
            ++count;
        }
    }

    qDebug() << "[CanvasStore] Restored" << count << "items";
}

void CanvasStore::saveViewState(const QPointF& center, double zoom)
{
    QSqlQuery q(m_db);

    q.prepare("INSERT OR REPLACE INTO canvas_state (key, value) VALUES ('view_x', ?)");
    q.addBindValue(QString::number(center.x(), 'f', 2));
    q.exec();

    q.prepare("INSERT OR REPLACE INTO canvas_state (key, value) VALUES ('view_y', ?)");
    q.addBindValue(QString::number(center.y(), 'f', 2));
    q.exec();

    q.prepare("INSERT OR REPLACE INTO canvas_state (key, value) VALUES ('view_zoom', ?)");
    q.addBindValue(QString::number(zoom, 'f', 4));
    q.exec();
}

void CanvasStore::restoreViewState(QPointF& center, double& zoom)
{
    QSqlQuery q(m_db);

    q.exec("SELECT value FROM canvas_state WHERE key = 'view_x'");
    double vx = q.next() ? q.value(0).toDouble() : 0.0;

    q.exec("SELECT value FROM canvas_state WHERE key = 'view_y'");
    double vy = q.next() ? q.value(0).toDouble() : 0.0;

    q.exec("SELECT value FROM canvas_state WHERE key = 'view_zoom'");
    zoom = q.next() ? q.value(0).toDouble() : 1.0;

    center = QPointF(vx, vy);
}

QString CanvasStore::newId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}
