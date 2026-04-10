#pragma once

#include <QObject>
#include <QWidget>
#include <QTimer>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

/**
 * DesktopEmbedder — Embeds a window into the Windows desktop wallpaper layer.
 *
 * Uses the proven WorkerW technique: sends 0x052C to Progman to spawn
 * a WorkerW behind SHELLDLL_DefView, then reparents our widget into it.
 * The widget becomes part of the desktop — immune to Win+D and Show Desktop.
 */
class DesktopEmbedder : public QObject {
    Q_OBJECT

public:
    explicit DesktopEmbedder(QObject* parent = nullptr);
    ~DesktopEmbedder() override = default;

    bool embed(WId windowHandle);
    void detach(WId windowHandle);
    bool isEmbedded() const { return m_embedded; }

    void setWidgetEnabled(bool enabled);
    bool isWidgetEnabled() const { return m_enabled; }

signals:
    void enabledChanged(bool enabled);

private:
#ifdef Q_OS_WIN
    HWND m_hwnd = nullptr;
    HWND m_workerW = nullptr;
#endif
    QTimer* m_visTimer = nullptr;
    bool m_embedded = false;
    bool m_enabled = true;
};
