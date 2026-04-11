#include "Toolbar.h"
#include <QColorDialog>
#include <QGraphicsDropShadowEffect>
#include <QLabel>

Toolbar::Toolbar(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void Toolbar::setupUI()
{
    // Semi-transparent dark background
    setStyleSheet(R"(
        Toolbar {
            background-color: transparent;
        }
        QWidget#mainToolbar {
            background-color: rgba(25, 25, 35, 200);
            border-radius: 10px;
            border: 1px solid rgba(100, 100, 130, 80);
        }
        QWidget#subMenu {
            background-color: rgba(30, 30, 42, 180);
            border-radius: 8px;
            border: 1px solid rgba(80, 80, 110, 60);
        }
        QPushButton {
            background-color: rgba(50, 50, 70, 180);
            color: #d0d0e0;
            border: 1px solid rgba(80, 80, 110, 100);
            border-radius: 6px;
            padding: 6px 14px;
            font-family: "Segoe UI";
            font-size: 11px;
            font-weight: 500;
            min-width: 40px;
        }
        QPushButton:hover {
            background-color: rgba(70, 70, 100, 200);
            border-color: rgba(120, 120, 180, 150);
        }
        QPushButton:checked {
            background-color: rgba(80, 80, 180, 200);
            border-color: rgba(130, 130, 255, 200);
            color: #ffffff;
        }
        QPushButton#clearBtn {
            background-color: rgba(120, 40, 40, 180);
            border-color: rgba(180, 60, 60, 100);
        }
        QPushButton#clearBtn:hover {
            background-color: rgba(160, 50, 50, 220);
        }
    )");

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(6);
    m_mainLayout->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);

    // --- Sub Menu ---
    m_subMenuWidget = new QWidget(this);
    m_subMenuWidget->setObjectName("subMenu");
    m_subLayout = new QHBoxLayout(m_subMenuWidget);
    m_subLayout->setContentsMargins(6, 4, 6, 4);
    m_subLayout->setSpacing(4);
    
    // We will populate subLayout dynamically based on current tool
    updateSubMenu();
    m_mainLayout->addWidget(m_subMenuWidget, 0, Qt::AlignHCenter);

    // --- Primary Toolbar ---
    auto* mainToolbar = new QWidget(this);
    mainToolbar->setObjectName("mainToolbar");
    auto* layout = new QHBoxLayout(mainToolbar);
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(4);

    m_selectBtn = createToolButton("Select", "Select & move items (V)");
    m_selectBtn->setCheckable(true);
    m_selectBtn->setChecked(true);
    connect(m_selectBtn, &QPushButton::clicked, this, &Toolbar::onSelectClicked);

    m_drawBtn = createToolButton("Draw", "Freehand draw (D)");
    m_drawBtn->setCheckable(true);
    connect(m_drawBtn, &QPushButton::clicked, this, &Toolbar::onDrawClicked);

    m_shapesBtn = createToolButton("Shapes", "Draw geometric shapes (S)");
    m_shapesBtn->setCheckable(true);
    connect(m_shapesBtn, &QPushButton::clicked, this, &Toolbar::onShapesClicked);

    m_textBtn = createToolButton("Text", "Add text note (T)");
    m_textBtn->setCheckable(true);
    connect(m_textBtn, &QPushButton::clicked, this, &Toolbar::onTextClicked);

    m_colorBtn = createToolButton("●", "Pick color");
    m_colorBtn->setStyleSheet(m_colorBtn->styleSheet() +
        QString("QPushButton { color: %1; font-size: 16px; min-width: 30px; }")
            .arg(m_currentColor.name()));
    connect(m_colorBtn, &QPushButton::clicked, this, &Toolbar::onColorClicked);

    m_clearBtn = createToolButton("Clear", "Clear entire canvas");
    m_clearBtn->setObjectName("clearBtn");
    connect(m_clearBtn, &QPushButton::clicked, this, &Toolbar::onClearClicked);

    layout->addWidget(m_selectBtn);
    layout->addWidget(m_drawBtn);
    layout->addWidget(m_shapesBtn);
    layout->addWidget(m_textBtn);
    layout->addSpacing(6);
    layout->addWidget(m_colorBtn);
    layout->addSpacing(6);
    layout->addWidget(m_clearBtn);

    m_mainLayout->addWidget(mainToolbar, 0, Qt::AlignHCenter);

    // Drop shadow on the primary toolbar
    auto* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(25);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 150));
    mainToolbar->setGraphicsEffect(shadow);
    
    // Initial size constraints
    setFixedHeight(100);
}

QPushButton* Toolbar::createToolButton(const QString& text, const QString& tooltip)
{
    auto* btn = new QPushButton(text, this);
    btn->setToolTip(tooltip);
    btn->setCursor(Qt::PointingHandCursor);
    return btn;
}

QPushButton* Toolbar::createIconButton(const QString& text, const QString& tooltip)
{
    auto* btn = new QPushButton(text, this);
    btn->setToolTip(tooltip);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setCheckable(true);
    btn->setStyleSheet("QPushButton { min-width: 30px; border-radius: 4px; padding: 4px 8px; }");
    return btn;
}

QPushButton* Toolbar::createColorDot(const QColor& color, const QString& tooltip)
{
    auto* btn = new QPushButton("●", this);
    btn->setToolTip(tooltip);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(QString("QPushButton { background: transparent; border: none; color: %1; font-size: 18px; min-width: 24px; padding: 0px; } "
                               "QPushButton:hover { color: #ffffff; }").arg(color.name()));
    return btn;
}

void Toolbar::updateButtonStates()
{
    m_selectBtn->setChecked(m_currentTool == Select);
    m_drawBtn->setChecked(m_currentTool == Draw);
    m_shapesBtn->setChecked(m_currentTool == Shapes);
    m_textBtn->setChecked(m_currentTool == Text);
    
    updateSubMenu();
}

void Toolbar::updateSubMenu()
{
    // Clear current sub-menu items
    QLayoutItem* item;
    while ((item = m_subLayout->takeAt(0)) != nullptr) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }

    if (m_currentTool == Draw || m_currentTool == Shapes) {
        m_subMenuWidget->show();

        if (m_currentTool == Shapes) {
            auto* sqBtn = createIconButton("■", "Square");
            sqBtn->setChecked(m_currentShape == Square);
            connect(sqBtn, &QPushButton::clicked, this, [this]{ setShape(Square); });

            auto* circBtn = createIconButton("●", "Circle");
            circBtn->setChecked(m_currentShape == Circle);
            connect(circBtn, &QPushButton::clicked, this, [this]{ setShape(Circle); });

            auto* triBtn = createIconButton("▲", "Triangle");
            triBtn->setChecked(m_currentShape == Triangle);
            connect(triBtn, &QPushButton::clicked, this, [this]{ setShape(Triangle); });

            m_subLayout->addWidget(sqBtn);
            m_subLayout->addWidget(circBtn);
            m_subLayout->addWidget(triBtn);
            
            // Add a separator
            auto* sep = new QWidget();
            sep->setFixedWidth(1);
            sep->setStyleSheet("background-color: rgba(100, 100, 130, 80);");
            m_subLayout->addWidget(sep);
        }

        auto* thinBtn = createIconButton("─", "Thin Stroke");
        thinBtn->setChecked(qFuzzyCompare(m_penWidth, 1.0));
        connect(thinBtn, &QPushButton::clicked, this, [this]{ setPenWidth(1.0); });

        auto* medBtn = createIconButton("▬", "Medium Stroke");
        medBtn->setChecked(qFuzzyCompare(m_penWidth, 3.0));
        connect(medBtn, &QPushButton::clicked, this, [this]{ setPenWidth(3.0); });

        auto* thickBtn = createIconButton("█", "Thick Stroke");
        thickBtn->setChecked(qFuzzyCompare(m_penWidth, 6.0));
        connect(thickBtn, &QPushButton::clicked, this, [this]{ setPenWidth(6.0); });

        m_subLayout->addWidget(thinBtn);
        m_subLayout->addWidget(medBtn);
        m_subLayout->addWidget(thickBtn);
        
    } else if (m_currentTool == Select) {
        m_subMenuWidget->show();
        
        auto* label = new QLabel("Canvas:");
        label->setStyleSheet("color: #a0a0b0; font-size: 10px; padding: 0 4px;");
        m_subLayout->addWidget(label);
        
        // Background color options
        m_subLayout->addWidget(createColorDot(QColor(14, 14, 20), "Dark Slate"));
        connect(m_subLayout->itemAt(m_subLayout->count()-1)->widget(), SIGNAL(clicked()), this, SLOT(requestCanvasColor()));
        
        m_subLayout->addWidget(createColorDot(QColor(31, 32, 41), "Soft Charcoal"));
        connect(m_subLayout->itemAt(m_subLayout->count()-1)->widget(), SIGNAL(clicked()), this, SLOT(requestCanvasColor()));

        m_subLayout->addWidget(createColorDot(QColor(26, 35, 51), "Blueprint Blue"));
        connect(m_subLayout->itemAt(m_subLayout->count()-1)->widget(), SIGNAL(clicked()), this, SLOT(requestCanvasColor()));

    } else if (m_currentTool == Text) {
        m_subMenuWidget->show();
        
        auto* boldBtn = createIconButton("B", "Bold (Ctrl+B)");
        boldBtn->setStyleSheet("QPushButton { min-width: 24px; font-weight: bold; border-radius: 4px; padding: 4px 8px; }");
        boldBtn->setCheckable(true);
        connect(boldBtn, &QPushButton::toggled, this, &Toolbar::textBoldToggled);

        auto* italicBtn = createIconButton("I", "Italic (Ctrl+I)");
        italicBtn->setStyleSheet("QPushButton { min-width: 24px; font-style: italic; border-radius: 4px; padding: 4px 8px; }");
        italicBtn->setCheckable(true);
        connect(italicBtn, &QPushButton::toggled, this, &Toolbar::textItalicToggled);

        auto* smallerBtn = createIconButton("T-", "Decrease Font Size");
        smallerBtn->setCheckable(false);
        connect(smallerBtn, &QPushButton::clicked, this, [this]{ emit textFontSizeChanged(-1); });

        auto* largerBtn = createIconButton("T+", "Increase Font Size");
        largerBtn->setCheckable(false);
        connect(largerBtn, &QPushButton::clicked, this, [this]{ emit textFontSizeChanged(1); });

        m_subLayout->addWidget(boldBtn);
        m_subLayout->addWidget(italicBtn);

        auto* sep = new QWidget();
        sep->setFixedWidth(1);
        sep->setStyleSheet("background-color: rgba(100, 100, 130, 80);");
        m_subLayout->addWidget(sep);

        m_subLayout->addWidget(smallerBtn);
        m_subLayout->addWidget(largerBtn);

        auto* sep2 = new QWidget();
        sep2->setFixedWidth(1);
        sep2->setStyleSheet("background-color: rgba(100, 100, 130, 80);");
        m_subLayout->addWidget(sep2);
        
        auto* textColBtn = createIconButton("A", "Pick Text Color");
        textColBtn->setCheckable(false);
        textColBtn->setStyleSheet("QPushButton { min-width: 24px; color: #e0e0e0; border-radius: 4px; border-bottom: 3px solid #e0e0e0; }");
        connect(textColBtn, &QPushButton::clicked, this, [this]() {
            QColor color = QColorDialog::getColor(Qt::white, this, "Pick Text Color");
            if (color.isValid()) emit textColorRequested(color);
        });
        m_subLayout->addWidget(textColBtn);

    } else {
        m_subMenuWidget->hide();
    }
}

void Toolbar::setPenWidth(qreal width)
{
    m_penWidth = width;
    updateSubMenu(); // Update checked states
    emit penWidthChanged(m_penWidth);
}

void Toolbar::setShape(ShapeType shape)
{
    m_currentShape = shape;
    updateSubMenu(); // Update checked states
}

void Toolbar::requestCanvasColor()
{
    // Retrieve color from sender's property/stylesheet context
    if (QPushButton* btn = qobject_cast<QPushButton*>(sender())) {
        // Simple extraction since we know the shape of the stylesheet
        QString ss = btn->styleSheet();
        int idx = ss.indexOf("color: #");
        if (idx != -1) {
            QString hex = ss.mid(idx + 7, 7);
            emit canvasColorRequested(QColor(hex));
        }
    }
}

void Toolbar::onSelectClicked()
{
    m_currentTool = Select;
    updateButtonStates();
    emit toolChanged(m_currentTool);
}

void Toolbar::onDrawClicked()
{
    m_currentTool = Draw;
    updateButtonStates();
    emit toolChanged(m_currentTool);
}

void Toolbar::onShapesClicked()
{
    m_currentTool = Shapes;
    updateButtonStates();
    emit toolChanged(m_currentTool);
}

void Toolbar::onTextClicked()
{
    m_currentTool = Text;
    updateButtonStates();
    emit toolChanged(m_currentTool);
}

void Toolbar::onColorClicked()
{
    QColor color = QColorDialog::getColor(m_currentColor, this, "Pick Stroke Color",
                                          QColorDialog::ShowAlphaChannel);
    if (color.isValid()) {
        m_currentColor = color;
        m_colorBtn->setStyleSheet(
            QString("QPushButton { color: %1; font-size: 16px; min-width: 30px; }")
                .arg(color.name()));
        emit colorChanged(color);
    }
}

void Toolbar::onClearClicked()
{
    emit clearAllRequested();
}
