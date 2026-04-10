#include "DesktopEmbedder.h"
#include <QDebug>

DesktopEmbedder::DesktopEmbedder(QObject* parent)
    : QObject(parent)
{
}

#ifdef Q_OS_WIN

static HWND g_foundWorkerW = nullptr;

static BOOL CALLBACK EnumWindowsCallback(HWND hwnd, LPARAM /* lParam */)
{
    HWND shellDefView = FindWindowExW(hwnd, nullptr, L"SHELLDLL_DefView", nullptr);
    if (shellDefView) {
        // The WorkerW we need is the NEXT sibling after the window
        // that contains SHELLDLL_DefView
        g_foundWorkerW = FindWindowExW(nullptr, hwnd, L"WorkerW", nullptr);
        return FALSE;  // Stop enumerating
    }
    return TRUE;
}

HWND DesktopEmbedder::findWorkerW()
{
    // Step 1: Find the Program Manager window
    HWND hProgman = FindWindowW(L"Progman", nullptr);
    if (!hProgman) {
        qWarning() << "[DesktopEmbedder] Could not find Progman window";
        return nullptr;
    }

    // Step 2: Send the undocumented message to spawn WorkerW
    SendMessageTimeoutW(
        hProgman,
        0x052C,
        0xD,
        0x1,
        SMTO_NORMAL,
        1000,
        nullptr
    );

    // Step 3: Find the WorkerW behind SHELLDLL_DefView
    g_foundWorkerW = nullptr;
    EnumWindows(EnumWindowsCallback, 0);

    if (!g_foundWorkerW) {
        qWarning() << "[DesktopEmbedder] Could not find WorkerW";
    }

    return g_foundWorkerW;
}

bool DesktopEmbedder::embed(WId windowHandle)
{
    HWND hwnd = reinterpret_cast<HWND>(windowHandle);

    m_workerW = findWorkerW();
    if (!m_workerW) {
        qWarning() << "[DesktopEmbedder] Failed — running as normal window";
        return false;
    }

    // Reparent into the WorkerW (desktop layer)
    SetParent(hwnd, m_workerW);

    // Set child window styles
    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    style &= ~WS_POPUP;
    style |= WS_CHILD | WS_VISIBLE;
    SetWindowLong(hwnd, GWL_STYLE, style);

    // Clean extended styles
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    exStyle &= ~(WS_EX_TOOLWINDOW | WS_EX_APPWINDOW);
    SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);

    // Get desktop size to center the widget
    RECT desktopRect;
    GetClientRect(m_workerW, &desktopRect);

    int width = 900;
    int height = 600;
    int x = (desktopRect.right - width) / 2;
    int y = (desktopRect.bottom - height) / 2;

    SetWindowPos(hwnd, HWND_TOP, x, y, width, height,
                 SWP_SHOWWINDOW | SWP_FRAMECHANGED);

    // Force repaint
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    m_embedded = true;
    qDebug() << "[DesktopEmbedder] Embedded into desktop at"
             << x << y << width << "x" << height;
    return true;
}

void DesktopEmbedder::detach(WId windowHandle)
{
    if (!m_embedded) return;

    HWND hwnd = reinterpret_cast<HWND>(windowHandle);
    SetParent(hwnd, nullptr);

    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    style &= ~WS_CHILD;
    style |= WS_POPUP;
    SetWindowLong(hwnd, GWL_STYLE, style);

    m_embedded = false;
    qDebug() << "[DesktopEmbedder] Detached";
}

bool DesktopEmbedder::reattach(WId windowHandle)
{
    detach(windowHandle);
    return embed(windowHandle);
}

#else
bool DesktopEmbedder::embed(WId) { return false; }
void DesktopEmbedder::detach(WId) {}
bool DesktopEmbedder::reattach(WId) { return false; }
#endif
