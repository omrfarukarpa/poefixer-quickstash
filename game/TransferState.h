#pragma once

#include "../config/Settings.h"
#include "../input/Win32Input.h"
#include "PanelDetector.h"
#include "TransferPlanner.h"
#include "sdk/PluginSDK.h"

#include <algorithm>
#include <chrono>
#include <vector>

namespace QuickStashGame {

// Minimum time Ctrl stays held after the final click, so the game always
// registers the last Ctrl+click even at aggressive timing settings.
inline constexpr int kMinCompletionHoldMs = 60;

class TransferState {
public:
    bool IsRunning() const { return m_running || m_finishing; }
    int  ProgressIndex() const { return m_index; }
    int  ProgressTotal() const { return ActiveQueueSize(); }
    bool IsWithdrawMode() const { return m_useScreenPoints; }

    // Release Ctrl no matter how the object dies (e.g. host destroys the
    // plugin without calling OnDisable). Without this, a leaked CtrlUp leaves
    // VK_CONTROL latched DOWN system-wide.
    ~TransferState() {
        if (m_ctrlHeld)
            QuickStashInput::CtrlUp();
    }

    void Start(const PluginSDK::Context* ctx, const QuickStashConfig::Settings& settings,
               const PluginSDK::Inventory& inv) {
        if (!ctx || m_running || m_finishing) return;
        m_settings = settings;
        m_useScreenPoints = false;
        m_screenQueue.clear();
        m_queue = BuildClickQueue(inv, settings);
        m_index = 0;
        m_ctrlHeld = false;
        m_finishing = false;
        m_phase = ClickPhase::Spacing;
        m_running = !m_queue.empty();
        m_startedAt = std::chrono::steady_clock::now();
        // Backdate so the first click's Spacing gate passes immediately rather
        // than waiting one clickDelayMs after the user pressed Transfer.
        m_lastClick = m_startedAt - std::chrono::milliseconds(m_settings.clickDelayMs);

        if (!m_running) {
            // Nothing to do (everything excluded or inventory empty). Tell the
            // user instead of silently no-op'ing the button press.
            ctx->Log.Info("Quick Stash: nothing to transfer (no eligible items)");
            return;
        }

        // Remember where the cursor was so we can put it back when done.
        m_haveSavedCursor = QuickStashInput::GetCursorScreen(m_savedCursorX, m_savedCursorY);

        QuickStashInput::CtrlDown();
        m_ctrlHeld = true;
        ctx->Log.Info(("Quick Stash: transferring " + std::to_string(m_queue.size())
                       + " items").c_str());
    }

    void StartWithdraw(const PluginSDK::Context* ctx,
                       const QuickStashConfig::Settings& settings,
                       std::vector<ScreenPoint> points) {
        if (!ctx || m_running || m_finishing) return;
        m_settings = settings;
        m_useScreenPoints = true;
        m_screenQueue = std::move(points);
        m_queue.clear();
        m_index = 0;
        m_ctrlHeld = false;
        m_finishing = false;
        m_phase = ClickPhase::Spacing;
        m_running = !m_screenQueue.empty();
        m_startedAt = std::chrono::steady_clock::now();
        m_lastClick = m_startedAt - std::chrono::milliseconds(m_settings.clickDelayMs);

        if (!m_running) {
            ctx->Log.Info("Quick Stash: nothing to withdraw (no matching on-screen items)");
            return;
        }

        m_haveSavedCursor = QuickStashInput::GetCursorScreen(m_savedCursorX, m_savedCursorY);

        QuickStashInput::CtrlDown();
        m_ctrlHeld = true;
        ctx->Log.Info(("Quick Stash: withdrawing " + std::to_string(m_screenQueue.size())
                       + " items").c_str());
    }

    void Abort() {
        if (m_ctrlHeld) {
            QuickStashInput::CtrlUp();
            m_ctrlHeld = false;
        }
        // Restore the cursor to where the user left it before the transfer.
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

    // `live` is the caller's current cached main-inventory snapshot (may be
    // null if the inventory just closed). Passing it in lets Tick reuse the
    // host scan the caller already did instead of re-enumerating every frame.
    void Tick(const PluginSDK::Context* ctx, const PluginSDK::Inventory* live) {
        if (!ctx) return;

        if (m_finishing) {
            TickFinishing(ctx);
            return;
        }
        if (!m_running) return;

        // Watchdog: a transfer should never run far longer than the worst-case
        // time its own queue + timings imply. If it does (host stalled the
        // frame loop, an item refuses to move, something wedged a phase), force
        // an abort so Ctrl is released rather than held indefinitely.
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

        // Per-click sub-state machine. Each Tick does at most ONE non-blocking
        // action and returns immediately — no Sleep on the render thread. The
        // real-world delays the SetCursorPos/SendInput sequence needs are now
        // realised as steady_clock deadlines between frames, the same way
        // BeginFinishing/m_finishAt already works.
        switch (m_phase) {
            case ClickPhase::Spacing: {
                // Inter-click spacing: wait clickDelayMs since the previous
                // click completed before starting the next one.
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
                QuickStashInput::MoveCursorScreen(clickX, clickY);
                m_phaseSince = now;
                m_phase = ClickPhase::Settling;
                return;
            }
            case ClickPhase::Settling: {
                // Let the cursor settle at the slot before clicking.
                if (Elapsed(now, m_phaseSince) < m_settings.cursorSettleMs)
                    return;
                QuickStashInput::LeftClickAtCursor();
                m_phaseSince = now;
                m_phase = ClickPhase::PostClick;
                return;
            }
            case ClickPhase::PostClick: {
                // Hold after the click so the game registers the Ctrl+click.
                if (Elapsed(now, m_phaseSince) < m_settings.postClickDelayMs)
                    return;
                ++m_index;
                m_lastClick = now;          // spacing is measured from here
                m_phase = ClickPhase::Spacing;
                if (m_index >= ActiveQueueSize())
                    BeginFinishing(now);
                return;
            }
        }
    }

private:
    // Sub-state within a single click, advanced one step per Tick.
    enum class ClickPhase {
        Spacing,    // waiting clickDelayMs before moving to the next slot
        Settling,   // cursor moved; waiting cursorSettleMs before clicking
        PostClick,  // clicked; waiting postClickDelayMs before advancing
    };

    static long long Elapsed(std::chrono::steady_clock::time_point now,
                             std::chrono::steady_clock::time_point since) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - since).count();
    }

    int ActiveQueueSize() const {
        return static_cast<int>(m_useScreenPoints ? m_screenQueue.size()
                                                   : m_queue.size());
    }

    // Generous upper bound on how long the whole transfer may take: the sum of
    // per-click worst-case timings, tripled for frame-pacing/jitter slack, plus
    // a flat floor. Past this we assume something wedged and bail.
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
        // Keep Ctrl held briefly after the last click so the game registers the
        // final Ctrl+click before we release. Enforce a floor (at least the
        // post-click delay, and never below kMinCompletionHoldMs) so a tiny or
        // zero completionHoldMs can't release Ctrl before the last click lands.
        int hold = m_settings.completionHoldMs > 0
                       ? m_settings.completionHoldMs
                       : m_settings.postClickDelayMs + m_settings.clickDelayMs;
        // Parenthesized to dodge the <Windows.h> max() macro (no NOMINMAX here).
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
    ClickPhase m_phase = ClickPhase::Spacing;
    QuickStashConfig::Settings m_settings{};
    std::vector<ClickTarget> m_queue;
    std::vector<ScreenPoint> m_screenQueue;
    std::chrono::steady_clock::time_point m_startedAt{};
    std::chrono::steady_clock::time_point m_lastClick{};
    std::chrono::steady_clock::time_point m_phaseSince{};
    std::chrono::steady_clock::time_point m_finishAt{};
};

} // namespace QuickStashGame
