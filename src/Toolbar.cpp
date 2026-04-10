#include "Toolbar.h"
#include <QColorDialog>
#include <QGraphicsDropShadowEffect>

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
            background-color: rgba(25, 25, 35, 200);
            border-radius: 10px;
            border: 1px solid rgba(100, 100, 130, 80);
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
            min-width: 50px;
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

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(4);

    m_selectBtn = createToolButton("Select", "Select & move items (V)");
    m_selectBtn->setCheckable(true);
    m_selectBtn->setChecked(true);
    connect(m_selectBtn, &QPushButton::clicked, this, &Toolbar::onSelectClicked);

    m_drawBtn = createToolButton("Draw", "Freehand draw (D)");
    m_drawBtn->setCheckable(true);
    connect(m_drawBtn, &QPushButton::clicked, this, &Toolbar::onDrawClicked);

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
    layout->addWidget(m_textBtn);
    layout->addSpacing(6);
    layout->addWidget(m_colorBtn);
    layout->addSpacing(6);
    layout->addWidget(m_clearBtn);

    setLayout(layout);
    setFixedHeight(42);

    // Drop shadow on the toolbar itself
    auto* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(25);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 150));
    setGraphicsEffect(shadow);
}

QPushButton* Toolbar::createToolButton(const QString& text, const QString& tooltip)
{
    auto* btn = new QPushButton(text, this);
    btn->setToolTip(tooltip);
    btn->setCursor(Qt::PointingHandCursor);
    return btn;
}

void Toolbar::updateButtonStates()
{
    m_selectBtn->setChecked(m_currentTool == Select);
    m_drawBtn->setChecked(m_currentTool == Draw);
    m_textBtn->setChecked(m_currentTool == Text);
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
