#pragma once

#include <QWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QColor>

/**
 * Toolbar — Floating semi-transparent toolbar for tool selection.
 * Modes: Select, Draw, Text
 * Also has a color indicator and clear button.
 */
class Toolbar : public QWidget {
    Q_OBJECT

public:
    enum Tool {
        Select,
        Draw,
        Text
    };

    explicit Toolbar(QWidget* parent = nullptr);

    Tool currentTool() const { return m_currentTool; }
    QColor currentColor() const { return m_currentColor; }
    qreal currentPenWidth() const { return m_penWidth; }

signals:
    void toolChanged(Toolbar::Tool tool);
    void colorChanged(const QColor& color);
    void clearAllRequested();

public slots:
    void onSelectClicked();
    void onDrawClicked();
    void onTextClicked();

private slots:
    void onColorClicked();
    void onClearClicked();

private:
    void setupUI();
    void updateButtonStates();
    QPushButton* createToolButton(const QString& text, const QString& tooltip);

    Tool m_currentTool = Select;
    QColor m_currentColor{200, 200, 255};
    qreal m_penWidth = 2.5;

    QPushButton* m_selectBtn = nullptr;
    QPushButton* m_drawBtn = nullptr;
    QPushButton* m_textBtn = nullptr;
    QPushButton* m_colorBtn = nullptr;
    QPushButton* m_clearBtn = nullptr;
};
