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
        Text,
        Shapes
    };

    enum ShapeType {
        Square,
        Circle,
        Triangle
    };

    explicit Toolbar(QWidget* parent = nullptr);

    Tool currentTool() const { return m_currentTool; }
    QColor currentColor() const { return m_currentColor; }
    qreal currentPenWidth() const { return m_penWidth; }
    ShapeType currentShape() const { return m_currentShape; }

signals:
    void toolChanged(Toolbar::Tool tool);
    void colorChanged(const QColor& color);
    void penWidthChanged(qreal width);
    void clearAllRequested();
    void canvasColorRequested(const QColor& color);
    
    // Text formatting
    void textBoldToggled(bool active);
    void textItalicToggled(bool active);
    void textFontSizeChanged(int delta);
    void textColorRequested(const QColor& color);

public slots:
    void onSelectClicked();
    void onDrawClicked();
    void onTextClicked();
    void onShapesClicked();

private slots:
    void onColorClicked();
    void onClearClicked();

    void setPenWidth(qreal width);
    void setShape(ShapeType shape);
    void requestCanvasColor();

private:
    void setupUI();
    void updateButtonStates();
    void updateSubMenu();
    QPushButton* createToolButton(const QString& text, const QString& tooltip);
    QPushButton* createIconButton(const QString& text, const QString& tooltip);
    QPushButton* createColorDot(const QColor& color, const QString& tooltip);
    QWidget* createSeparator();

    Tool m_currentTool = Select;
    ShapeType m_currentShape = Square;
    QColor m_currentColor{200, 200, 255};
    qreal m_penWidth = 2.5;

    // Layouts
    QVBoxLayout* m_mainLayout = nullptr;
    QWidget* m_subMenuWidget = nullptr;
    QHBoxLayout* m_subLayout = nullptr;

    // Primary Buttons
    QPushButton* m_selectBtn = nullptr;
    QPushButton* m_drawBtn = nullptr;
    QPushButton* m_textBtn = nullptr;
    QPushButton* m_shapesBtn = nullptr;
    QPushButton* m_colorBtn = nullptr;
    QPushButton* m_clearBtn = nullptr;
};
