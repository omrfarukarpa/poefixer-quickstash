#pragma once

#include "../game/PanelDetector.h"
#include "sdk/PluginSDK.h"

#include <imgui.h>

namespace QuickStashUi {

// Read-only inventory inspector shown in the settings panel. Its purpose is to
// discover, on a live game, how the currently-open storage panel (stash tab,
// vendor, trade, gamble, ...) appears in InventoryService::GetAll() — and to
// validate that FindOpenStash() picks it out from the 100+ enumerated tabs.
//
// The withdraw ("pick up from stash") feature Ctrl+clicks items in the STASH,
// so it must positively identify which enumerated inventory is the open stash
// grid. This panel surfaces the detection result and the candidate rows at the
// top so there's no need to scroll the full list.
//
// GetAll() materializes every item (and its strings) per call, which is heavy
// for wide special tabs, so it only runs while this header is expanded.
inline void DrawInventoryDiagnostics(const PluginSDK::Context* ctx) {
    if (!ctx) return;
    if (!ImGui::CollapsingHeader("Diagnostics: inventory inspector"))
        return;

    const ImVec2 disp = ImGui::GetIO().DisplaySize;
    ImGui::TextWrapped(
        "Open your inventory AND a stash tab, then check the detection below. "
        "'Detected open stash' is the row the withdraw feature will target.");
    ImGui::Text("Display: %.0f x %.0f", disp.x, disp.y);
    ImGui::Spacing();

    // Rescan so a freshly-opened panel shows up without waiting on the main loop.
    ctx->Inventory.Scan(-1);
    const auto all = ctx->Inventory.GetAll();
    ImGui::Text("Inventories enumerated: %d", static_cast<int>(all.size()));

    const auto main = QuickStashGame::FindMainInventory(ctx);
    const int mainId = main ? main->InventoryId : -1;

    // --- Item-centric detection ------------------------------------------
    // The withdraw source is every ON-SCREEN item that is NOT in the player's
    // own bag/equipment. If only the visible stash tab contributes items here,
    // we can target it purely by item screen coords (no tab identification).
    int totalOnScreen = 0;          // withdrawable items (all non-player invs)
    int invsWithOnScreen = 0;       // how many inventories contribute them
    for (const auto& inv : all) {
        if (inv.InventoryId == mainId) continue;
        const char* name = ctx->Inventory.GetName(inv.InventoryId);
        if (QuickStashGame::IsPlayerSlotName(name)) continue;
        int n = 0;
        for (const auto& it : inv.Items)
            if (QuickStashGame::ItemOnScreen(it, disp.x, disp.y)) ++n;
        if (n > 0) { ++invsWithOnScreen; totalOnScreen += n; }
    }

    if (totalOnScreen > 0) {
        ImGui::TextColored(ImVec4(0.4f, 0.85f, 0.4f, 1.f),
            "On-screen withdrawable items: %d  (from %d inventory/tabs)",
            totalOnScreen, invsWithOnScreen);
        if (invsWithOnScreen == 1)
            ImGui::TextDisabled("Good: only one tab is on-screen -> clean target.");
        else
            ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.35f, 1.f),
                "Note: %d tabs report on-screen items at once.", invsWithOnScreen);
    } else {
        ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.3f, 1.f),
            "On-screen withdrawable items: 0 (open a stash tab with items)");
    }
    ImGui::Spacing();

    // --- Per-tab breakdown: EVERY non-player inventory that has items ----
    // Three counts expose why a tab may fail: Total = items the host sees,
    // Valid = items whose ScreenValid flag is set, OnScr = valid AND inside the
    // window. A tab you can see but that shows Total>0 / OnScr=0 is the case we
    // need to understand (items without a usable screen rect).
    ImGui::TextDisabled("All non-player tabs holding items (Total / ScreenValid / OnScreen):");
    const ImGuiTableFlags flags = ImGuiTableFlags_Borders
                                | ImGuiTableFlags_RowBg
                                | ImGuiTableFlags_SizingStretchProp
                                | ImGuiTableFlags_ScrollY;
    if (ImGui::BeginTable("##qs_inv_cand", 7, flags, ImVec2(0.f, 200.f))) {
        ImGui::TableSetupColumn("Id",    ImGuiTableColumnFlags_WidthFixed, 40.f);
        ImGui::TableSetupColumn("Name",  ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Total", ImGuiTableColumnFlags_WidthFixed, 50.f);
        ImGui::TableSetupColumn("Valid", ImGuiTableColumnFlags_WidthFixed, 50.f);
        ImGui::TableSetupColumn("OnScr", ImGuiTableColumnFlags_WidthFixed, 50.f);
        ImGui::TableSetupColumn("1st item XY", ImGuiTableColumnFlags_WidthFixed, 96.f);
        ImGui::TableSetupColumn("Boxes", ImGuiTableColumnFlags_WidthFixed, 64.f);
        ImGui::TableHeadersRow();

        int shown = 0;
        for (const auto& inv : all) {
            if (inv.InventoryId == mainId) continue;
            const char* name = ctx->Inventory.GetName(inv.InventoryId);
            if (QuickStashGame::IsPlayerSlotName(name)) continue;
            if (inv.Items.empty()) continue;

            int valid = 0, onScr = 0;
            for (const auto& it : inv.Items) {
                if (it.ScreenValid) ++valid;
                if (QuickStashGame::ItemOnScreen(it, disp.x, disp.y)) ++onScr;
            }
            ++shown;
            const auto& f = inv.Items.front();
            ImGui::TableNextRow();
            // Highlight rows that have items the player can see but that we
            // fail to count as on-screen — the interesting failure case.
            if (onScr == 0)
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0,
                                       IM_COL32(70, 45, 20, 160));
            ImGui::TableNextColumn(); ImGui::Text("%d", inv.InventoryId);
            ImGui::TableNextColumn(); ImGui::TextUnformatted(name ? name : "");
            ImGui::TableNextColumn(); ImGui::Text("%d", static_cast<int>(inv.Items.size()));
            ImGui::TableNextColumn(); ImGui::Text("%d", valid);
            ImGui::TableNextColumn(); ImGui::Text("%d", onScr);
            ImGui::TableNextColumn();
            ImGui::Text("%.0f,%.0f", f.ScreenX, f.ScreenY);
            ImGui::TableNextColumn();
            ImGui::Text("%dx%d", inv.TotalBoxesX, inv.TotalBoxesY);
        }
        ImGui::EndTable();
        if (shown == 0)
            ImGui::TextDisabled("(no non-player tab holds items)");
    }
}

} // namespace QuickStashUi
