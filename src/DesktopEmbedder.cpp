#include "DesktopEmbedder.h"
#include <QDebug>

DesktopEmbedder::DesktopEmbedder(QObject* parent)
    : QObject(parent)
{
}

#ifdef Q_OS_WIN

#include <dwmapi.h>

bool DesktopEmbedder::embed(WId windowHandle)
{
    m_hwnd = reinterpret_cast<HWND>(windowHandle);

    // --- NO EMBEDDING ---
    // Instead of parenting into WorkerW (which makes us subject to
    // Win+D hiding the parent), we stay as a standalone top-level window.

    // Hide from taskbar and Alt-Tab
    LONG exStyle = GetWindowLong(m_hwnd, GWL_EXSTYLE);
    exStyle |= WS_EX_TOOLWINDOW;
    exStyle &= ~WS_EX_APPWINDOW;
    SetWindowLong(m_hwnd, GWL_EXSTYLE, exStyle);

    // Remove minimize/maximize capability
    LONG style = GetWindowLong(m_hwnd, GWL_STYLE);
    style &= ~(WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
    SetWindowLong(m_hwnd, GWL_STYLE, style);

    // Apply style changes
    SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                 SWP_FRAMECHANGED | SWP_SHOWWINDOW);

    // --- THE KEY: Aggressive restore timer ---
    // Win+D will minimize us (we can't prevent that).
    // But we detect it within 100ms and IMMEDIATELY restore.
    // The widget pops back so fast that it appears to never have left.
    m_visTimer = new QTimer(this);
    connect(m_visTimer, &QTimer::timeout, this, [this]() {
        if (!m_hwnd || !m_enabled) return;

        bool needsRestore = false;

        // 1. Minimized natively?
        if (IsIconic(m_hwnd)) needsRestore = true;

        // 2. Hidden natively?
        if (!IsWindowVisible(m_hwnd)) needsRestore = true;

        // 3. Cloaked by DWM?
        BOOL cloaked = FALSE;
        DwmGetWindowAttribute(m_hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
        if (cloaked) needsRestore = true;

        if (needsRestore) {
            ShowWindow(m_hwnd, SW_RESTORE);
            ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
        }

        // Win+D reorganizes the Z-order. Repeatedly pin it to the absolute bottom 
        // (but above the desktop).
        SetWindowPos(m_hwnd, HWND_BOTTOM, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    });
    m_visTimer->start(100);

    m_embedded = true;
    m_enabled = true;
    qDebug() << "[Slate] Desktop widget active (standalone mode, 100ms restore)";
    return true;
}

void DesktopEmbedder::setWidgetEnabled(bool enabled)
{
    if (m_enabled == enabled) return;
    m_enabled = enabled;
    if (!m_hwnd) return;

    if (enabled) {
        ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
        if (m_visTimer) m_visTimer->start(100);
    } else {
        if (m_visTimer) m_visTimer->stop();
        ShowWindow(m_hwnd, SW_HIDE);
    }
    emit enabledChanged(enabled);
}

void DesktopEmbedder::detach(WId windowHandle)
{
    if (!m_embedded) return;
    if (m_visTimer) { m_visTimer->stop(); delete m_visTimer; m_visTimer = nullptr; }

    HWND hwnd = reinterpret_cast<HWND>(windowHandle);

    // Restore normal styles
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    exStyle &= ~WS_EX_TOOLWINDOW;
    exStyle |= WS_EX_APPWINDOW;
    SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);

    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    style |= WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
    SetWindowLong(hwnd, GWL_STYLE, style);

    m_embedded = false;
    m_hwnd = nullptr;
}

#else
bool DesktopEmbedder::embed(WId) { return false; }
void DesktopEmbedder::detach(WId) {}
void DesktopEmbedder::setWidgetEnabled(bool) {}
#endif
