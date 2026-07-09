// Quick Stash — PoeFixer plugin (SDK v6)
// Ctrl+click transfer from main inventory when the backpack is open.

#include "sdk/PluginSDK.h"

#include "config/Settings.h"
#include "game/PanelDetector.h"
#include "game/PoeHighlight.h"
#include "game/TransferState.h"
#include "game/WithdrawPlanner.h"
#include "overlay/TransferButtonOverlay.h"
#include "ui/ExclusionGrid.h"
#include "ui/InventoryDiagnostics.h"

#include <imgui.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <optional>
#include <string>
#include <utility>
#include <vector>

inline constexpr const char* kQuickStashVersion    = "1.3.1";
inline constexpr const char* kQuickStashMaintainer = "Omer Faruk ARPA";

class QuickStashPlugin : public PluginSDK::Plugin {
public:
    const char* GetName() const override { return "Quick Stash"; }

    bool WantsOverlay() const override { return m_settings.enabled; }

    void OnEnable(bool /*isGameAttached*/) override {
        // If the host ABI version/size didn't match at attach time, m_ctx is
        // unpopulated (all service pointers null). Refuse to run rather than
        // silently no-op with no diagnostic. Log is safe even here — the SDK
        // wrappers null-check their function pointers.
        if (!HostCompatible()) {
            ctx()->Log.Error(
                "Quick Stash: incompatible PoeFixer host (SDK version/size mismatch) — disabled");
            return;
        }

        if (ctx()->ImGuiContext)
            ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ctx()->ImGuiContext));

        m_settings.Load(DirectoryPath());
        m_lastScan = std::chrono::steady_clock::now();

        auto& events = const_cast<PluginSDK::EventsService&>(ctx()->Events);
        m_frameToken = events.OnFrame([this] { OnFrameTick(); });

        ctx()->Log.Info("Quick Stash plugin enabled");
    }

    void OnDisable() override {
        m_transfer.Abort();
        if (m_frameToken.Valid()) {
            auto& events = const_cast<PluginSDK::EventsService&>(ctx()->Events);
            events.Unsubscribe(m_frameToken);
            m_frameToken = {};
        }
        m_overlayCapturePending = false;
        ctx()->Overlay.SetWantsOverlayInput(false);
        SaveSettings();
        ctx()->Log.Info("Quick Stash plugin disabled");
    }

    void DrawSettings() override {
        if (ctx()->ImGuiContext)
            ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ctx()->ImGuiContext));

        ImGui::TextDisabled("Quick Stash v%s  -  by %s",
                            kQuickStashVersion, kQuickStashMaintainer);
        ImGui::Checkbox("Enable Quick Stash", &m_settings.enabled);

        // How-to: collapsed by default so it doesn't crowd the settings, but
        // there for anyone who needs it.
        if (ImGui::CollapsingHeader("How to use")) {
            ImGui::TextWrapped(
                "Quick Stash dumps your backpack into whatever storage panel you "
                "have open, using Ctrl+click for each item.");
            ImGui::Spacing();
            ImGui::TextWrapped("1. Open your character inventory.");
            ImGui::TextWrapped("2. Open the target panel too (Stash, Vendor, Trade, Gamble...).");
            ImGui::TextWrapped("3. Click the \"transfer\" button that appears above your backpack.");
            ImGui::TextWrapped("4. Items move in order; excluded cells and empty slots are skipped.");
            ImGui::Spacing();
            ImGui::TextDisabled(
                "Right-click during a transfer cancels it (if enabled below). "
                "Alt-tabbing also stops it. The cursor returns to where it was when done.");
            ImGui::Spacing();
            ImGui::TextDisabled(
                "Tip: disable PoeFixer's built-in auto-stash while using this plugin "
                "to avoid conflicting Ctrl+click behaviour.");
        }
        ImGui::Separator();

        ImGui::SliderInt("Click delay (ms)", &m_settings.clickDelayMs,
                         QuickStashConfig::kClickDelayMinMs, QuickStashConfig::kClickDelayMaxMs);
        ImGui::SliderInt("Post-click delay (ms)", &m_settings.postClickDelayMs,
                         QuickStashConfig::kPostClickDelayMinMs, QuickStashConfig::kPostClickDelayMaxMs);
        ImGui::SliderInt("Cursor settle (ms)", &m_settings.cursorSettleMs,
                         QuickStashConfig::kCursorSettleMinMs, QuickStashConfig::kCursorSettleMaxMs);
        ImGui::SliderInt("Hold Ctrl after last click (ms)", &m_settings.completionHoldMs,
                         QuickStashConfig::kCompletionHoldMinMs, QuickStashConfig::kCompletionHoldMaxMs);
        ImGui::SliderFloat("Button offset X", &m_settings.buttonOffsetX, -40.f, 80.f, "%.0f");
        ImGui::SliderFloat("Button offset Y", &m_settings.buttonOffsetY, -40.f, 80.f, "%.0f");
        ImGui::Checkbox("Cancel on right-click", &m_settings.cancelOnRightClick);
        ImGui::Checkbox("Stop if inventory closes", &m_settings.verifyPanelsOpen);

        ImGui::Separator();
        ImGui::TextWrapped(
            "Exclusion grid: each cell maps to a slot in your backpack. "
            "Click a cell to toggle whether items in that slot get transferred. "
            "Any item touching a red cell stays in your bag.");

        ImGui::Separator();
        QuickStashUi::DrawExclusionGrid(m_settings);

        ImGui::Separator();
        ImGui::TextWrapped(
            "Withdraw (TAKE): type in PoE's own 'Highlight Items' box; the TAKE "
            "button above it Ctrl+clicks the matching items in the open stash back "
            "into your inventory. Caution: with a vendor open, Ctrl+click buys.");
        ImGui::Checkbox("Match item mods in filter", &m_settings.readMods);
        ImGui::TextDisabled(
            "When on, the filter also matches an item's mods (e.g. 'waystone', "
            "'rarity'), like PoE's own highlight. Turn off if reading item data "
            "ever causes a crash.");

        ImGui::Separator();
        ImGui::Checkbox("Debug mode (inventory / UI tree inspectors)", &m_settings.debugMode);
        if (m_settings.debugMode) {
            QuickStashUi::DrawInventoryDiagnostics(ctx());
            QuickStashUi::DrawWithdrawHaystackDump(ctx());
            QuickStashUi::DrawUiTreeDiagnostics(ctx());
        }
    }

    void DrawUI() override {
        if (!m_settings.enabled) return;
        if (!ctx()->Game.IsInGame()) return;
        if (ctx()->ImGuiContext)
            ImGui::SetCurrentContext(static_cast<ImGuiContext*>(ctx()->ImGuiContext));

        if (!ctx()->Game.GetSnapshot().GameWindowForeground) {
            m_overlayCapturePending = false;
            ctx()->Overlay.SetWantsOverlayInput(false);
            return;
        }

        RefreshInventoryIfNeeded();

        if (m_transfer.IsRunning()) {
            m_overlayCapturePending = false;
            ctx()->Overlay.SetWantsOverlayInput(false);
            QuickStashOverlay::DrawTransferProgress(
                m_transfer.ProgressIndex(), m_transfer.ProgressTotal(),
                m_transfer.IsWithdrawMode() ? "Withdrawing" : "Transferring");
            return;
        }

        // m_backpack is refreshed by RefreshInventoryIfNeeded(); a valid grid
        // here means the inventory is open. No extra IsInventoryOpen() scan.
        if (!m_backpack || !m_backpack->Grid.Valid) {
            m_overlayCapturePending = false;
            ctx()->Overlay.SetWantsOverlayInput(false);
            return;
        }

        UpdateWithdraw();
        DrawWithdrawHighlights();

        const bool showWithdraw = m_poeFound && m_stashOpen;
        const ImVec2 takePos(m_poeX, m_poeY - QuickStashOverlay::kButtonH - 6.f);

        const bool mouseOverTransfer =
            QuickStashOverlay::IsMouseOverTransferButton(*m_backpack, m_settings);
        const bool mouseOverWithdraw = showWithdraw && QuickStashOverlay::HitRect(
            ImGui::GetIO().MousePos, takePos,
            ImVec2(takePos.x + QuickStashOverlay::kButtonW,
                   takePos.y + QuickStashOverlay::kButtonH));
        const bool mouseOverBtn = mouseOverTransfer || mouseOverWithdraw;
        m_overlayCapturePending = mouseOverBtn;
        ctx()->Overlay.SetWantsOverlayInput(mouseOverBtn || m_overlayCaptureApplied);

        const auto tbtn = QuickStashOverlay::DrawTransferButton(*m_backpack, m_settings);

        QuickStashOverlay::TransferButtonResult wbtn;
        if (showWithdraw) {
            wbtn = QuickStashOverlay::DrawWithdrawButtonAt(takePos, m_withdrawCount);
            if (m_withdrawCount > 0 && m_withdrawQty != m_withdrawCount) {
                char q[24];
                snprintf(q, sizeof(q), "x%d", m_withdrawQty);
                ImGui::GetForegroundDrawList()->AddText(
                    ImVec2(takePos.x + QuickStashOverlay::kButtonW + 8.f, takePos.y + 5.f),
                    IM_COL32(150, 210, 255, 255), q);
            }
        }

        bool transferActivated = QuickStashOverlay::TransferButtonActivated(tbtn);
        bool withdrawActivated = showWithdraw && QuickStashOverlay::TransferButtonActivated(wbtn);

        if (!transferActivated && !withdrawActivated) {
            if (mouseOverTransfer || tbtn.hovered) {
                if (QuickStashOverlay::Win32LeftClickOnRect(tbtn.btnP0, tbtn.btnP1))
                    transferActivated = true;
            } else if (showWithdraw && (mouseOverWithdraw || wbtn.hovered)) {
                if (QuickStashOverlay::Win32LeftClickOnRect(wbtn.btnP0, wbtn.btnP1))
                    withdrawActivated = true;
            }
        }

        if (transferActivated) {
            m_overlayCapturePending = false;
            ctx()->Overlay.SetWantsOverlayInput(false);
            m_transfer.Start(ctx(), m_settings, *m_backpack);
        } else if (withdrawActivated) {
            m_overlayCapturePending = false;
            ctx()->Overlay.SetWantsOverlayInput(false);
            StartWithdraw();
        }
    }

    void StartWithdraw() {
        if (m_withdrawTargets.empty()) {
            ctx()->Log.Info("Quick Stash: nothing to withdraw (no matching items)");
            return;
        }
        std::vector<QuickStashGame::ScreenPoint> pts;
        pts.reserve(m_withdrawTargets.size());
        for (const auto& r : m_withdrawTargets)
            pts.push_back(QuickStashGame::RectCenter(r));
        m_transfer.StartWithdraw(ctx(), m_settings, std::move(pts));
    }

    void UpdateWithdraw() {
        const auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastPoeRead).count() >= 150) {
            m_lastPoeRead = now;
            const auto poe = QuickStashGame::ReadPoeHighlight(ctx());
            m_poeFilter = poe.filter;
            m_poeFound = poe.found;
            m_poeX = poe.x;
            m_poeY = poe.y;
        }
        const std::string& filter = m_poeFilter;

        const ImVec2 disp = ImGui::GetIO().DisplaySize;
        const int mainId = m_backpack ? m_backpack->InventoryId : -1;
        auto stash = QuickStashGame::FindOpenStashAny(ctx(), mainId, disp.x, disp.y);
        if (!stash) {
            m_candidates.clear();
            m_withdrawTargets.clear();
            m_withdrawCount = 0;
            m_stashOpen = false;
            return;
        }
        m_stashOpen = true;
        const bool needMods = m_settings.readMods && !filter.empty();
        m_candidates = QuickStashGame::CollectCandidates(
            ctx(), *stash, disp.x, disp.y, &m_modCache, needMods);
        auto sel = QuickStashGame::FilterCandidates(m_candidates, filter);
        m_withdrawTargets = std::move(sel.rects);
        m_withdrawCount = static_cast<int>(m_withdrawTargets.size());
        m_withdrawQty = sel.totalQty;
    }

    void DrawWithdrawHighlights() {
        if (m_withdrawTargets.empty()) return;
        auto* dl = ImGui::GetForegroundDrawList();
        for (const auto& r : m_withdrawTargets) {
            const ImVec2 p0(r.x, r.y);
            const ImVec2 p1(r.x + r.w, r.y + r.h);
            dl->AddRectFilled(p0, p1, IM_COL32(90, 200, 255, 45), 2.f);
            dl->AddRect(p0, p1, IM_COL32(90, 200, 255, 235), 2.f, 0, 2.5f);
        }
    }

    void SaveSettings() override { m_settings.Save(DirectoryPath()); }

private:
    QuickStashConfig::Settings m_settings;
    QuickStashGame::TransferState m_transfer;
    PluginSDK::EventsService::Token m_frameToken{};
    std::optional<PluginSDK::Inventory> m_backpack;
    std::chrono::steady_clock::time_point m_lastScan{};
    bool m_overlayCapturePending = false;
    bool m_overlayCaptureApplied = false;
    int m_withdrawCount = 0;
    int m_withdrawQty = 0;
    bool m_stashOpen = false;
    std::string m_poeFilter;
    bool m_poeFound = false;
    float m_poeX = 0.f;
    float m_poeY = 0.f;
    std::chrono::steady_clock::time_point m_lastPoeRead{};
    std::vector<QuickStashGame::ScreenRect> m_withdrawTargets;
    std::vector<QuickStashGame::WithdrawCandidate> m_candidates;
    QuickStashGame::ModTextCache m_modCache;

    // Refresh the cached backpack snapshot at most every 150 ms. During a
    // transfer this also keeps m_backpack's grid current so TransferState can
    // recompute click coordinates from the live grid (handles a panel that
    // moves/scrolls mid-transfer).
    void RefreshInventoryIfNeeded() {
        const auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastScan).count() < 150)
            return;
        m_lastScan = now;
        ctx()->Inventory.Scan(-1);
        m_backpack = QuickStashGame::FindMainInventory(ctx());
    }

    void OnFrameTick() {
        if (m_transfer.IsRunning()) {
            m_overlayCapturePending = false;
            m_overlayCaptureApplied = false;
            ctx()->Overlay.SetWantsOverlayInput(false);
            // If the game loses foreground mid-transfer, abort instead of
            // continuing to inject clicks (which would land in another window)
            // and, critically, leaving Ctrl held down system-wide. Abort()
            // releases Ctrl. Without this, alt-tabbing during a transfer keeps
            // SetCursorPos/SendInput firing with Ctrl stuck on.
            if (!ctx()->Game.GetSnapshot().GameWindowForeground) {
                m_transfer.Abort();
                return;
            }
            RefreshInventoryIfNeeded();
            m_transfer.Tick(ctx(), m_backpack ? &*m_backpack : nullptr);
            return;
        }

        if (!m_settings.enabled || !ctx()->Game.IsInGame()) {
            m_overlayCapturePending = false;
            m_overlayCaptureApplied = false;
            ctx()->Overlay.SetWantsOverlayInput(false);
            return;
        }

        if (!ctx()->Game.GetSnapshot().GameWindowForeground) {
            m_overlayCapturePending = false;
            m_overlayCaptureApplied = false;
            ctx()->Overlay.SetWantsOverlayInput(false);
            return;
        }

        RefreshInventoryIfNeeded();
        if (!m_backpack || !m_backpack->Grid.Valid) {
            m_overlayCapturePending = false;
            m_overlayCaptureApplied = false;
            ctx()->Overlay.SetWantsOverlayInput(false);
            return;
        }

        m_overlayCaptureApplied = m_overlayCapturePending;
        ctx()->Overlay.SetWantsOverlayInput(m_overlayCaptureApplied);
    }
};

extern "C" PLUGIN_API PluginSDK::Plugin* CreatePlugin() { return new QuickStashPlugin(); }

extern "C" PLUGIN_API void DestroyPlugin(PluginSDK::Plugin* p) { delete p; }
