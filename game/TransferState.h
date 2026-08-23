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

// Minimum time Ctrl stays held after the final click, so the game always
// registers the last Ctrl+click even at aggressive timing settings.
inline constexpr int kMinCompletionHoldMs = 60;

// Minimum time between CtrlDown and the FIRST click. The game samples key
// state per frame; a click issued in the same instant as CtrlDown can be
// processed as a PLAIN click — which picks the item up onto the cursor
// instead of transferring it.
inline constexpr int kCtrlSettleMs = 50;

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
                // Both queues carry game-CLIENT coordinates (grid math and item
                // rects alike) — convert to SCREEN pixels at the single click
                // emission point. A failed conversion means the game window is
                // gone mid-run: ABORT rather than fall back to injecting
                // client coords as screen coords (off-by-titlebar clicks into
                // whatever owns that part of the screen, with Ctrl held).
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
    // Shared tail of Start/StartWithdraw — the caller has already set the mode
    // flag and filled its queue. One copy on purpose: the Ctrl-settle timing
    // and the game-window capture are click-safety invariants, and a fix
    // applied to only one of the two entry points silently desynchronises
    // transfer vs withdraw behaviour.
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
        // Backdate so the first click's Spacing gate passes after only
        // kCtrlSettleMs (not a full clickDelayMs) — quick to start, but never
        // in the same instant as the CtrlDown below.
        m_lastClick = m_startedAt - std::chrono::milliseconds(m_settings.clickDelayMs)
                                  + std::chrono::milliseconds(kCtrlSettleMs);

        if (!m_running) {
            // Nothing to do (everything excluded / empty / no matches). Tell
            // the user instead of silently no-op'ing the button press.
            ctx->Log.Info(emptyMsg);
            return;
        }

        // Every click is aimed in game-CLIENT space and converted through this
        // window. No window — no clicks: injecting through the identity
        // fallback would repeat the exact off-by-titlebar mis-aim (tab-strip
        // clicks, missed top-row items) the conversion exists to prevent.
        m_gameWnd = ctx->Game.GetGameWindow();
        if (!m_gameWnd || !IsWindow(m_gameWnd)) {
            ctx->Log.Warn("Quick Stash: game window unavailable — not starting");
            m_running = false;
            m_queue.clear();
            m_screenQueue.clear();
            return;
        }

        // Remember where the cursor was so we can put it back when done.
        m_haveSavedCursor = QuickStashInput::GetCursorScreen(m_savedCursorX, m_savedCursorY);

        QuickStashInput::CtrlDown();
        m_ctrlHeld = true;
        char msg[64];
        snprintf(msg, sizeof(msg), "%s %d items", startVerb, ActiveQueueSize());
        ctx->Log.Info(msg);
    }

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
    HWND m_gameWnd = nullptr;   // client->screen conversion target for clicks
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
