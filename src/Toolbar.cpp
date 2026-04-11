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
    // ── Global Stylesheet ──────────────────────────────────────────────
    setStyleSheet(R"(
        Toolbar {
            background-color: transparent;
        }
        QWidget#mainToolbar {
            background-color: rgba(18, 18, 18, 235);
            border-radius: 12px;
            border: 1px solid rgba(255, 255, 255, 6);
        }
        QWidget#subMenu {
            background-color: rgba(18, 18, 18, 225);
            border-radius: 10px;
            border: 1px solid rgba(255, 255, 255, 6);
        }
        QPushButton {
            background-color: transparent;
            color: rgba(180, 180, 200, 180);
            border: none;
            border-radius: 8px;
            padding: 6px;
            font-family: "Segoe UI";
            font-size: 15px;
            min-width: 32px;
            min-height: 32px;
        }
        QPushButton:hover {
            background-color: rgba(255, 255, 255, 12);
            color: #ffffff;
        }
        QPushButton:checked {
            background-color: rgba(255, 255, 255, 15);
            color: #ffffff;
        }
        QPushButton#clearBtn {
            color: rgba(220, 90, 90, 200);
        }
        QPushButton#clearBtn:hover {
            background-color: rgba(220, 60, 60, 30);
            color: #ff6666;
        }
    )");

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(4);
    m_mainLayout->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);

    // ── Sub Menu (contextual options) ──────────────────────────────────
    m_subMenuWidget = new QWidget(this);
    m_subMenuWidget->setObjectName("subMenu");
    m_subLayout = new QHBoxLayout(m_subMenuWidget);
    m_subLayout->setContentsMargins(6, 4, 6, 4);
    m_subLayout->setSpacing(2);
    updateSubMenu();
    m_mainLayout->addWidget(m_subMenuWidget, 0, Qt::AlignHCenter);

    // ── Primary Toolbar ────────────────────────────────────────────────
    auto* mainToolbar = new QWidget(this);
    mainToolbar->setObjectName("mainToolbar");
    auto* layout = new QHBoxLayout(mainToolbar);
    layout->setContentsMargins(6, 4, 6, 4);
    layout->setSpacing(2);

    //  Icons:  ↖ cursor  ✎ pencil  ◇ shapes  T text  ● color  ✕ clear
    m_selectBtn = createToolButton(QChar(0x2196), "Select & move (V)");   // ↖
    m_selectBtn->setCheckable(true);
    m_selectBtn->setChecked(true);
    connect(m_selectBtn, &QPushButton::clicked, this, &Toolbar::onSelectClicked);

    m_drawBtn = createToolButton(QChar(0x270E), "Freehand draw (D)");     // ✎
    m_drawBtn->setCheckable(true);
    connect(m_drawBtn, &QPushButton::clicked, this, &Toolbar::onDrawClicked);

    m_shapesBtn = createToolButton(QChar(0x25CB), "Shapes (S)");          // ○
    m_shapesBtn->setCheckable(true);
    connect(m_shapesBtn, &QPushButton::clicked, this, &Toolbar::onShapesClicked);

    m_textBtn = createToolButton("T", "Text note (T)");                    // T
    m_textBtn->setCheckable(true);
    m_textBtn->setStyleSheet(m_textBtn->styleSheet() +
        "QPushButton { font-family: 'Georgia', serif; font-weight: bold; font-size: 16px; }");
    connect(m_textBtn, &QPushButton::clicked, this, &Toolbar::onTextClicked);

    // Thin separator
    auto* sep1 = createSeparator();

    m_colorBtn = createToolButton(QChar(0x25CF), "Pick color");            // ●
    m_colorBtn->setStyleSheet(m_colorBtn->styleSheet() +
        QString("QPushButton { color: %1; font-size: 14px; }").arg(m_currentColor.name()));
    connect(m_colorBtn, &QPushButton::clicked, this, &Toolbar::onColorClicked);

    auto* sep2 = createSeparator();

    m_clearBtn = createToolButton(QChar(0x2715), "Delete selected");          // ✕
    m_clearBtn->setObjectName("clearBtn");
    connect(m_clearBtn, &QPushButton::clicked, this, &Toolbar::onClearClicked);

    layout->addWidget(m_selectBtn);
    layout->addWidget(m_drawBtn);
    layout->addWidget(m_shapesBtn);
    layout->addWidget(m_textBtn);
    layout->addWidget(sep1);
    layout->addWidget(m_colorBtn);
    layout->addWidget(sep2);
    layout->addWidget(m_clearBtn);

    m_mainLayout->addWidget(mainToolbar, 0, Qt::AlignHCenter);

    // Subtle shadow
    auto* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(20);
    shadow->setOffset(0, 2);
    shadow->setColor(QColor(0, 0, 0, 120));
    mainToolbar->setGraphicsEffect(shadow);

    setFixedHeight(90);
}

QWidget* Toolbar::createSeparator()
{
    auto* sep = new QWidget(this);
    sep->setFixedSize(1, 20);
    sep->setStyleSheet("background-color: rgba(255, 255, 255, 15);");
    return sep;
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
    return btn;
}

QPushButton* Toolbar::createColorDot(const QColor& color, const QString& tooltip)
{
    auto* btn = new QPushButton(this);
    btn->setToolTip(tooltip);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setFixedSize(18, 18);
    btn->setStyleSheet(QString(
        "QPushButton { background-color: %1; border: 2px solid rgba(255,255,255,25); border-radius: 9px; min-width: 0; min-height: 0; padding: 0; }"
        "QPushButton:hover { border-color: rgba(255,255,255,100); }"
    ).arg(color.name()));
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
            auto* sqBtn = createIconButton(QChar(0x25A0), "Square");  // ■
            sqBtn->setChecked(m_currentShape == Square);
            connect(sqBtn, &QPushButton::clicked, this, [this]{ setShape(Square); });

            auto* circBtn = createIconButton(QChar(0x25CF), "Circle"); // ●
            circBtn->setChecked(m_currentShape == Circle);
            connect(circBtn, &QPushButton::clicked, this, [this]{ setShape(Circle); });

            auto* triBtn = createIconButton(QChar(0x25B2), "Triangle"); // ▲
            triBtn->setChecked(m_currentShape == Triangle);
            connect(triBtn, &QPushButton::clicked, this, [this]{ setShape(Triangle); });

            m_subLayout->addWidget(sqBtn);
            m_subLayout->addWidget(circBtn);
            m_subLayout->addWidget(triBtn);
            m_subLayout->addWidget(createSeparator());
        }

        // Stroke width buttons — use real thickness lines
        auto* thinBtn = createIconButton(QChar(0x2500), "Thin");    // ─
        thinBtn->setChecked(qFuzzyCompare(m_penWidth, 1.0));
        connect(thinBtn, &QPushButton::clicked, this, [this]{ setPenWidth(1.0); });

        auto* medBtn = createIconButton(QChar(0x2501), "Medium");   // ━
        medBtn->setChecked(qFuzzyCompare(m_penWidth, 3.0));
        connect(medBtn, &QPushButton::clicked, this, [this]{ setPenWidth(3.0); });

        auto* thickBtn = createIconButton(QChar(0x2588), "Thick");   // █
        thickBtn->setChecked(qFuzzyCompare(m_penWidth, 6.0));
        connect(thickBtn, &QPushButton::clicked, this, [this]{ setPenWidth(6.0); });

        m_subLayout->addWidget(thinBtn);
        m_subLayout->addWidget(medBtn);
        m_subLayout->addWidget(thickBtn);

    } else if (m_currentTool == Select) {
        m_subMenuWidget->show();

        // Canvas background presets as colored dots
        struct Preset { QColor color; QString name; };
        const Preset presets[] = {
            { QColor(13, 13, 13),   "Midnight"      },
            { QColor(28, 28, 28),   "Charcoal"      },
            { QColor(22, 30, 42),   "Blueprint"     },
            { QColor(24, 32, 24),   "Forest"        },
            { QColor(26, 20, 38),   "Indigo"        },
            { QColor(242,242,242),  "Soft White"    },
            { QColor(250,246,238),  "Warm Cream"    },
        };

        for (const auto& p : presets) {
            auto* dot = createColorDot(p.color, p.name);
            m_subLayout->addWidget(dot);
            connect(dot, &QPushButton::clicked, this, [this, c = p.color]() {
                emit canvasColorRequested(c);
            });
        }

    } else if (m_currentTool == Text) {
        m_subMenuWidget->show();

        auto* boldBtn = createIconButton("B", "Bold");
        boldBtn->setStyleSheet("QPushButton { font-weight: bold; }");
        boldBtn->setCheckable(true);
        connect(boldBtn, &QPushButton::toggled, this, &Toolbar::textBoldToggled);

        auto* italicBtn = createIconButton("I", "Italic");
        italicBtn->setStyleSheet("QPushButton { font-style: italic; font-family: Georgia, serif; }");
        italicBtn->setCheckable(true);
        connect(italicBtn, &QPushButton::toggled, this, &Toolbar::textItalicToggled);

        auto* smallerBtn = createIconButton(QChar(0x2212), "Smaller");  // −
        smallerBtn->setCheckable(false);
        connect(smallerBtn, &QPushButton::clicked, this, [this]{ emit textFontSizeChanged(-1); });

        auto* largerBtn = createIconButton("+", "Larger");
        largerBtn->setCheckable(false);
        connect(largerBtn, &QPushButton::clicked, this, [this]{ emit textFontSizeChanged(1); });

        auto* textColBtn = createIconButton("A", "Text Color");
        textColBtn->setCheckable(false);
        textColBtn->setStyleSheet("QPushButton { font-weight: bold; border-bottom: 2px solid #e0e0e0; }");
        connect(textColBtn, &QPushButton::clicked, this, [this]() {
            QColor color = QColorDialog::getColor(Qt::white, this, "Pick Text Color");
            if (color.isValid()) emit textColorRequested(color);
        });

        m_subLayout->addWidget(boldBtn);
        m_subLayout->addWidget(italicBtn);
        m_subLayout->addWidget(createSeparator());
        m_subLayout->addWidget(smallerBtn);
        m_subLayout->addWidget(largerBtn);
        m_subLayout->addWidget(createSeparator());
        m_subLayout->addWidget(textColBtn);

    } else {
        m_subMenuWidget->hide();
    }
}

void Toolbar::setPenWidth(qreal width)
{
    m_penWidth = width;
    updateSubMenu();
    emit penWidthChanged(m_penWidth);
}

void Toolbar::setShape(ShapeType shape)
{
    m_currentShape = shape;
    updateSubMenu();
}

void Toolbar::requestCanvasColor()
{
    // Legacy slot — no longer used (replaced by lambda connections)
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
            QString("QPushButton { color: %1; font-size: 14px; }").arg(color.name()));
        emit colorChanged(color);
    }
}

void Toolbar::onClearClicked()
{
    emit clearAllRequested();
}
