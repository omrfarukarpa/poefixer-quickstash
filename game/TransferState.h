#pragma once

#include "../config/Settings.h"
#include "../input/Win32Input.h"
#include "PanelDetector.h"
#include "TransferPlanner.h"
#include "sdk/PluginSDK.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <vector>

namespace QuickStashGame {

inline constexpr int kMinCompletionHoldMs = 60;

inline constexpr int kCtrlSettleMs = 50;

class TransferState {
public:
    bool IsRunning() const { return m_running || m_finishing; }
    int  ProgressIndex() const { return m_index; }
    int  ProgressTotal() const { return ActiveQueueSize(); }
    bool IsWithdrawMode() const { return m_useScreenPoints; }

    ~TransferState() {
        if (m_ctrlHeld)
            QuickStashInput::CtrlUp();
    }

    void Start(const PluginSDK::Context* ctx, const QuickStashConfig::Settings& settings,
               const PluginSDK::Inventory& inv) {
        if (!ctx || m_running || m_finishing) return;
        m_useScreenPoints = false;
        m_screenQueue.clear();
        m_queue = BuildClickQueue(inv, settings);
        BeginRun(ctx, settings,
                 "Quick Stash: nothing to transfer (no eligible items)",
                 "Quick Stash: transferring");
    }

    void StartWithdraw(const PluginSDK::Context* ctx,
                       const QuickStashConfig::Settings& settings,
                       std::vector<ScreenPoint> points) {
        if (!ctx || m_running || m_finishing) return;
        m_useScreenPoints = true;
        m_screenQueue = std::move(points);
        m_queue.clear();
        BeginRun(ctx, settings,
                 "Quick Stash: nothing to withdraw (no matching on-screen items)",
                 "Quick Stash: withdrawing");
    }

    void Abort() {
        if (m_ctrlHeld) {
            QuickStashInput::CtrlUp();
            m_ctrlHeld = false;
        }

        if (m_haveSavedCursor) {
            QuickStashInput::MoveCursorScreen(m_savedCursorX, m_savedCursorY);
            m_haveSavedCursor = false;
        }
        m_running = false;
        m_finishing = false;
        m_phase = ClickPhase::Spacing;
        m_queue.clear();
        m_screenQueue.clear();
        m_useScreenPoints = false;
        m_index = 0;
    }

    void Tick(const PluginSDK::Context* ctx, const PluginSDK::Inventory* live) {
        if (!ctx) return;

        if (m_finishing) {
            TickFinishing(ctx);
            return;
        }
        if (!m_running) return;

        if (Elapsed(std::chrono::steady_clock::now(), m_startedAt) > WatchdogBudgetMs()) {
            ctx->Log.Warn("Quick Stash: aborted (watchdog timeout)");
            Abort();
            return;
        }

        if (m_settings.cancelOnRightClick && QuickStashInput::IsRightMouseDown()) {
            ctx->Log.Info("Quick Stash: cancelled (right mouse)");
            Abort();
            return;
        }

        const bool inventoryOpen = live && live->Grid.Valid;
        if (m_settings.verifyPanelsOpen && !inventoryOpen) {
            ctx->Log.Info("Quick Stash: cancelled (inventory closed)");
            Abort();
            return;
        }

        const auto now = std::chrono::steady_clock::now();

        switch (m_phase) {
            case ClickPhase::Spacing: {

                if (Elapsed(now, m_lastClick) < m_settings.clickDelayMs)
                    return;
                if (m_index >= ActiveQueueSize()) {
                    BeginFinishing(now);
                    return;
                }
                int clickX, clickY;
                if (m_useScreenPoints) {
                    const auto& p = m_screenQueue[static_cast<size_t>(m_index)];
                    clickX = p.x;
                    clickY = p.y;
                } else {
                    if (!inventoryOpen)
                        return;
                    const auto& target = m_queue[static_cast<size_t>(m_index)];
                    clickX = static_cast<int>(SlotCenterX(*live, target.slotX) + 0.5f);
                    clickY = static_cast<int>(SlotCenterY(*live, target.slotY) + 0.5f);
                }

                if (!QuickStashInput::ClientToScreenPoint(m_gameWnd, clickX, clickY)) {
                    ctx->Log.Warn("Quick Stash: game window lost — aborting");
                    Abort();
                    return;
                }
                QuickStashInput::MoveCursorScreen(clickX, clickY);
                m_phaseSince = now;
                m_phase = ClickPhase::Settling;
                return;
            }
            case ClickPhase::Settling: {

                if (Elapsed(now, m_phaseSince) < m_settings.cursorSettleMs)
                    return;
                QuickStashInput::LeftClickAtCursor();
                m_phaseSince = now;
                m_phase = ClickPhase::PostClick;
                return;
            }
            case ClickPhase::PostClick: {

                if (Elapsed(now, m_phaseSince) < m_settings.postClickDelayMs)
                    return;
                ++m_index;
                m_lastClick = now;
                m_phase = ClickPhase::Spacing;
                if (m_index >= ActiveQueueSize())
                    BeginFinishing(now);
                return;
            }
        }
    }

private:

    void BeginRun(const PluginSDK::Context* ctx,
                  const QuickStashConfig::Settings& settings,
                  const char* emptyMsg, const char* startVerb) {
        m_settings = settings;
        m_index = 0;
        m_ctrlHeld = false;
        m_finishing = false;
        m_phase = ClickPhase::Spacing;
        m_running = ActiveQueueSize() > 0;
        m_startedAt = std::chrono::steady_clock::now();

        if (!m_running) {

            ctx->Log.Info(emptyMsg);
            return;
        }

        m_gameWnd = ctx->Game.GetGameWindow();
        if (!m_gameWnd || !IsWindow(m_gameWnd)) {
            ctx->Log.Warn("Quick Stash: game window unavailable — not starting");
            m_running = false;
            m_queue.clear();
            m_screenQueue.clear();
            return;
        }

        m_haveSavedCursor = QuickStashInput::GetCursorScreen(m_savedCursorX, m_savedCursorY);

        QuickStashInput::CtrlDown();
        m_ctrlHeld = true;
        m_lastClick = std::chrono::steady_clock::now()
                      - std::chrono::milliseconds(m_settings.clickDelayMs)
                      + std::chrono::milliseconds(kCtrlSettleMs);
        char msg[64];
        snprintf(msg, sizeof(msg), "%s %d items", startVerb, ActiveQueueSize());
        ctx->Log.Info(msg);
    }

    enum class ClickPhase {
        Spacing,
        Settling,
        PostClick,
    };

    static long long Elapsed(std::chrono::steady_clock::time_point now,
                             std::chrono::steady_clock::time_point since) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - since).count();
    }

    int ActiveQueueSize() const {
        return static_cast<int>(m_useScreenPoints ? m_screenQueue.size()
                                                   : m_queue.size());
    }

    long long WatchdogBudgetMs() const {
        const long long perClick = static_cast<long long>(m_settings.clickDelayMs)
                                  + m_settings.cursorSettleMs
                                  + m_settings.postClickDelayMs;
        const long long items = static_cast<long long>(ActiveQueueSize());
        return 5000 + perClick * items * 3 + m_settings.completionHoldMs;
    }

    void BeginFinishing(std::chrono::steady_clock::time_point from) {
        m_running = false;
        m_finishing = true;

        int hold = m_settings.completionHoldMs > 0
                       ? m_settings.completionHoldMs
                       : m_settings.postClickDelayMs + m_settings.clickDelayMs;

        const int floor = (m_settings.postClickDelayMs > kMinCompletionHoldMs)
                              ? m_settings.postClickDelayMs : kMinCompletionHoldMs;
        if (hold < floor)
            hold = floor;
        m_finishAt = from + std::chrono::milliseconds(hold);
    }

    void TickFinishing(const PluginSDK::Context* ctx) {
        if (std::chrono::steady_clock::now() < m_finishAt)
            return;
        ctx->Log.Info("Quick Stash: transfer complete");
        Abort();
    }

    bool m_running = false;
    bool m_finishing = false;
    bool m_ctrlHeld = false;
    bool m_haveSavedCursor = false;
    bool m_useScreenPoints = false;
    int  m_index = 0;
    int  m_savedCursorX = 0;
    int  m_savedCursorY = 0;
    HWND m_gameWnd = nullptr;
    ClickPhase m_phase = ClickPhase::Spacing;
    QuickStashConfig::Settings m_settings{};
    std::vector<ClickTarget> m_queue;
    std::vector<ScreenPoint> m_screenQueue;
    std::chrono::steady_clock::time_point m_startedAt{};
    std::chrono::steady_clock::time_point m_lastClick{};
    std::chrono::steady_clock::time_point m_phaseSince{};
    std::chrono::steady_clock::time_point m_finishAt{};
};

}
