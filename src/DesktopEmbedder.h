#pragma once

#include <QObject>
#include <QWidget>  // For WId
#include <QRect>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

/**
 * DesktopEmbedder — Embeds a window into the Windows desktop layer.
 *
 * Uses the WorkerW/Progman technique to place the window above
 * the wallpaper on the desktop layer. The window becomes part of
 * the desktop — it doesn't minimize with Win+D and persists
 * across Show Desktop actions.
 *
 * Abstracted into its own class so macOS porting only requires
 * swapping the platform-specific implementation.
 */
class DesktopEmbedder : public QObject {
    Q_OBJECT

public:
    explicit DesktopEmbedder(QObject* parent = nullptr);
    ~DesktopEmbedder() override = default;

    bool embed(WId windowHandle);
    void detach(WId windowHandle);
    bool isEmbedded() const { return m_embedded; }
    bool reattach(WId windowHandle);

private:
#ifdef Q_OS_WIN
    HWND findWorkerW();
    HWND m_workerW = nullptr;
#endif

    bool m_embedded = false;
};
