// Quick Stash — PoeFixer plugin (SDK v6)
// Ctrl+click transfer from main inventory when the backpack is open.

#include "sdk/PluginSDK.h"

#include "config/Settings.h"
#include "game/PanelDetector.h"
#include "game/TransferState.h"
#include "overlay/TransferButtonOverlay.h"
#include "ui/ExclusionGrid.h"
#include "ui/InventoryDiagnostics.h"

#include <imgui.h>
#include <chrono>
#include <optional>

// Version + maintainer, shown in the settings panel. This is a hardened fork
// of the original Quick Stash 1.0.
inline constexpr const char* kQuickStashVersion    = "1.1.1";
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

        ImGui::TextDisabled("Quick Stash v%s  -  fork by %s",
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
        QuickStashUi::DrawInventoryDiagnostics(ctx());
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
            QuickStashOverlay::DrawTransferProgress(m_transfer.ProgressIndex(),
                                                    m_transfer.ProgressTotal());
            return;
        }

        // m_backpack is refreshed by RefreshInventoryIfNeeded(); a valid grid
        // here means the inventory is open. No extra IsInventoryOpen() scan.
        if (!m_backpack || !m_backpack->Grid.Valid) {
            m_overlayCapturePending = false;
            ctx()->Overlay.SetWantsOverlayInput(false);
            return;
        }

        const bool mouseOverBtn =
            QuickStashOverlay::IsMouseOverTransferButton(*m_backpack, m_settings);
        m_overlayCapturePending = mouseOverBtn;
        ctx()->Overlay.SetWantsOverlayInput(mouseOverBtn || m_overlayCaptureApplied);

        const auto btn = QuickStashOverlay::DrawTransferButton(*m_backpack, m_settings);

        bool activated = QuickStashOverlay::TransferButtonActivated(btn);
        if (!activated && (mouseOverBtn || btn.hovered))
            activated = QuickStashOverlay::Win32LeftClickOnRect(btn.btnP0, btn.btnP1);

        if (activated) {
            m_overlayCapturePending = false;
            ctx()->Overlay.SetWantsOverlayInput(false);
            m_transfer.Start(ctx(), m_settings, *m_backpack);
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
