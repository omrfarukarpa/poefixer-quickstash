#pragma once

#include "../game/PanelDetector.h"
#include "../game/TransferPlanner.h"
#include "../game/WithdrawPlanner.h"
#include "sdk/PluginSDK.h"

#include <imgui.h>

namespace QuickStashUi {

// Read-only inventory inspector shown in the settings panel. Its purpose is to
// discover, on a live game, how the currently-open storage panel (stash tab,
// vendor, trade, gamble, ...) appears in InventoryService::GetAll() — and to
// validate which detection path lets the withdraw feature target it.
//
// TWO detection paths are surfaced side by side because they behave DIFFERENTLY
// per tab type (learned in-game 2026-07-08):
//   * ITEM-centric  — per-item ScreenX/Y + ScreenValid. Works for special
//     affinity tabs (currency/fragment/...) whose items each carry a real
//     screen rect, but is EMPTY for normal grid tabs (Valid=0, XY 0,0).
//   * GRID-centric  — Grid.GridScreenX/Y + CellSize + item SlotX/Y, the SAME
//     math the working Transfer feature uses (see TransferPlanner::SlotCenter*).
//     This is expected to work for normal grid tabs even when per-item rects are
//     absent, and is the primary candidate for driving withdraw.
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
        "GRID-based detection is what the withdraw feature will most likely use.");
    ImGui::Text("Display: %.0f x %.0f", disp.x, disp.y);
    ImGui::Spacing();

    // Rescan so a freshly-opened panel shows up without waiting on the main loop.
    ctx->Inventory.Scan(-1);
    const auto all = ctx->Inventory.GetAll();
    ImGui::Text("Inventories enumerated: %d", static_cast<int>(all.size()));

    const auto main = QuickStashGame::FindMainInventory(ctx);
    const int mainId = main ? main->InventoryId : -1;

    // --- GRID-centric detection (primary withdraw candidate) -------------
    // FindOpenStash keeps a valid, on-screen grid that is neither the backpack
    // nor an equipment slot. If it finds a tab, withdraw can click every item
    // via SlotCenterX/Y just like Transfer does — no per-item rect required.
    const auto openStash =
        QuickStashGame::FindOpenStash(ctx, mainId, disp.x, disp.y);
    if (openStash) {
        const char* name = ctx->Inventory.GetName(openStash->InventoryId);
        ImGui::TextColored(ImVec4(0.4f, 0.85f, 0.4f, 1.f),
            "GRID-detected open stash: id %d (%s), %d items",
            openStash->InventoryId, name ? name : "",
            static_cast<int>(openStash->Items.size()));
        ImGui::TextDisabled("  grid=(%.0f,%.0f) cell=%.1f boxes=%dx%d",
            openStash->Grid.GridScreenX, openStash->Grid.GridScreenY,
            openStash->Grid.CellSize,
            openStash->TotalBoxesX, openStash->TotalBoxesY);
        if (!openStash->Items.empty()) {
            const auto& f = openStash->Items.front();
            ImGui::TextDisabled(
                "  1st item slot (%d,%d) -> grid click (%.0f,%.0f)",
                f.SlotX, f.SlotY,
                QuickStashGame::SlotCenterX(*openStash, f.SlotX),
                QuickStashGame::SlotCenterY(*openStash, f.SlotY));
        }
    } else {
        ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.3f, 1.f),
            "GRID-detected open stash: none (open a stash tab)");
    }
    ImGui::Spacing();

    // --- ITEM-centric detection (works for special affinity tabs) --------
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
    ImGui::TextDisabled("Item-centric on-screen items: %d (from %d tabs)",
                        totalOnScreen, invsWithOnScreen);
    ImGui::Spacing();

    // --- Per-tab breakdown: EVERY non-player inventory that has items ----
    // Total     = items the host sees
    // Valid/OnS = per-item ScreenValid, and valid AND inside the window
    // GV        = Grid.Valid, GOn = grid rect is on-screen (GridOnScreen)
    // ClickXY   = grid-computed click point of the 1st item (SlotCenter)
    // A row usable by GRID withdraw has GV=1 and GOn=1 with a sane ClickXY,
    // regardless of Valid/OnS.
    ImGui::TextDisabled("All non-player tabs holding items:");
    const ImGuiTableFlags flags = ImGuiTableFlags_Borders
                                | ImGuiTableFlags_RowBg
                                | ImGuiTableFlags_SizingStretchProp
                                | ImGuiTableFlags_ScrollY;
    if (ImGui::BeginTable("##qs_inv_cand", 10, flags, ImVec2(0.f, 220.f))) {
        ImGui::TableSetupColumn("Id",    ImGuiTableColumnFlags_WidthFixed, 40.f);
        ImGui::TableSetupColumn("Name",  ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Total", ImGuiTableColumnFlags_WidthFixed, 46.f);
        ImGui::TableSetupColumn("Valid", ImGuiTableColumnFlags_WidthFixed, 46.f);
        ImGui::TableSetupColumn("OnS",   ImGuiTableColumnFlags_WidthFixed, 42.f);
        ImGui::TableSetupColumn("GV",    ImGuiTableColumnFlags_WidthFixed, 34.f);
        ImGui::TableSetupColumn("GOn",   ImGuiTableColumnFlags_WidthFixed, 38.f);
        ImGui::TableSetupColumn("Grid XY",  ImGuiTableColumnFlags_WidthFixed, 90.f);
        ImGui::TableSetupColumn("ClickXY(1st)", ImGuiTableColumnFlags_WidthFixed, 100.f);
        ImGui::TableSetupColumn("Boxes", ImGuiTableColumnFlags_WidthFixed, 58.f);
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
            const bool gridOn =
                QuickStashGame::GridOnScreen(inv, disp.x, disp.y);
            ++shown;
            const auto& f = inv.Items.front();
            ImGui::TableNextRow();
            // Green-tint rows that GRID withdraw could drive (valid on-screen
            // grid) — the outcome we now care about most.
            if (gridOn)
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0,
                                       IM_COL32(25, 60, 30, 150));
            ImGui::TableNextColumn(); ImGui::Text("%d", inv.InventoryId);
            ImGui::TableNextColumn(); ImGui::TextUnformatted(name ? name : "");
            ImGui::TableNextColumn(); ImGui::Text("%d", static_cast<int>(inv.Items.size()));
            ImGui::TableNextColumn(); ImGui::Text("%d", valid);
            ImGui::TableNextColumn(); ImGui::Text("%d", onScr);
            ImGui::TableNextColumn(); ImGui::Text("%d", inv.Grid.Valid ? 1 : 0);
            ImGui::TableNextColumn(); ImGui::Text("%d", gridOn ? 1 : 0);
            ImGui::TableNextColumn();
            ImGui::Text("%.0f,%.0f", inv.Grid.GridScreenX, inv.Grid.GridScreenY);
            ImGui::TableNextColumn();
            if (inv.Grid.Valid)
                ImGui::Text("%.0f,%.0f",
                    QuickStashGame::SlotCenterX(inv, f.SlotX),
                    QuickStashGame::SlotCenterY(inv, f.SlotY));
            else
                ImGui::TextUnformatted("-");
            ImGui::TableNextColumn();
            ImGui::Text("%dx%d", inv.TotalBoxesX, inv.TotalBoxesY);
        }
        ImGui::EndTable();
        if (shown == 0)
            ImGui::TextDisabled("(no non-player tab holds items)");
    }

    ImGui::Spacing();
    if (const auto tab =
            QuickStashGame::FindOpenStashAny(ctx, mainId, disp.x, disp.y)) {
        ImGui::TextColored(ImVec4(0.6f, 0.85f, 1.f, 1.f),
            "Open tab item names (id %d) - this is ALL the filter can match:",
            tab->InventoryId);
        const ImGuiTableFlags nf = ImGuiTableFlags_Borders
                                 | ImGuiTableFlags_RowBg
                                 | ImGuiTableFlags_SizingStretchProp
                                 | ImGuiTableFlags_ScrollY;
        if (ImGui::BeginTable("##qs_names", 5, nf, ImVec2(0.f, 240.f))) {
            ImGui::TableSetupColumn("Slot", ImGuiTableColumnFlags_WidthFixed, 52.f);
            ImGui::TableSetupColumn("Rar",  ImGuiTableColumnFlags_WidthFixed, 34.f);
            ImGui::TableSetupColumn("BaseTypeName", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("UniqueName",   ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Path",         ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();
            int rows = 0;
            for (const auto& it : tab->Items) {
                if (rows++ >= 80) break;
                const bool emptyNames =
                    it.BaseTypeName.empty() && it.UniqueName.empty();
                ImGui::TableNextRow();
                if (emptyNames)
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0,
                                           IM_COL32(70, 45, 20, 160));
                ImGui::TableNextColumn(); ImGui::Text("%d,%d", it.SlotX, it.SlotY);
                ImGui::TableNextColumn(); ImGui::Text("%d", it.Rarity);
                ImGui::TableNextColumn(); ImGui::TextUnformatted(it.BaseTypeName.c_str());
                ImGui::TableNextColumn(); ImGui::TextUnformatted(it.UniqueName.c_str());
                ImGui::TableNextColumn(); ImGui::TextUnformatted(it.Path.c_str());
            }
            ImGui::EndTable();
        }
        ImGui::TextDisabled(
            "Amber rows = no BaseTypeName AND no UniqueName (nothing to match).");
    }
}

inline void DrawWithdrawHaystackDump(const PluginSDK::Context* ctx) {
    if (!ctx) return;
    if (!ImGui::CollapsingHeader("Diagnostics: withdraw filter haystack (mods)"))
        return;

    ImGui::TextWrapped(
        "For the open tab, this is the EXACT text the filter matches against "
        "(name + mods). If a word you type is not here, it cannot match. This "
        "reads item mods live - keep it collapsed during normal play.");

    const ImVec2 disp = ImGui::GetIO().DisplaySize;
    ctx->Inventory.Scan(-1);
    const auto main = QuickStashGame::FindMainInventory(ctx);
    const int mainId = main ? main->InventoryId : -1;
    const auto tab = QuickStashGame::FindOpenStashAny(ctx, mainId, disp.x, disp.y);
    if (!tab) {
        ImGui::TextDisabled("(open a stash tab)");
        return;
    }

    const ImGuiTableFlags flags = ImGuiTableFlags_Borders
                                | ImGuiTableFlags_RowBg
                                | ImGuiTableFlags_SizingStretchProp
                                | ImGuiTableFlags_ScrollY;
    if (ImGui::BeginTable("##qs_haystack", 4, flags, ImVec2(0.f, 320.f))) {
        ImGui::TableSetupColumn("Slot", ImGuiTableColumnFlags_WidthFixed, 52.f);
        ImGui::TableSetupColumn("BaseType", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Mod haystack", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Agg key:val", ImGuiTableColumnFlags_WidthFixed, 200.f);
        ImGui::TableHeadersRow();
        int rows = 0;
        for (const auto& it : tab->Items) {
            if (rows++ >= 40) break;
            std::string mods = QuickStashGame::BuildModText(ctx, it);
            for (auto& ch : mods)
                if (ch == '\n') ch = ' ';
            const std::string agg = QuickStashGame::DebugAggregatedPairs(ctx, it);
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%d,%d", it.SlotX, it.SlotY);
            ImGui::TableNextColumn(); ImGui::TextUnformatted(it.BaseTypeName.c_str());
            ImGui::TableNextColumn();
            if (mods.empty())
                ImGui::TextDisabled("(no mods read)");
            else
                ImGui::TextWrapped("%s", mods.c_str());
            ImGui::TableNextColumn();
            ImGui::TextWrapped("%s", agg.c_str());
        }
        ImGui::EndTable();
    }
}

inline void DrawUiTreeDiagnostics(const PluginSDK::Context* ctx) {
    if (!ctx) return;
    if (!ImGui::CollapsingHeader("Diagnostics: UI tree text (find PoE's search box)"))
        return;

    ImGui::TextWrapped(
        "Type something (e.g. 'fire') in PoE's own 'Highlight Items' box at the "
        "bottom, then find the row below whose Text is exactly that. Tell me its "
        "StringId - then the plugin can read PoE's box directly.");

    const uintptr_t root = ctx->Ui.GetGameUiRoot();
    ImGui::Text("GameUiRoot: 0x%llx", static_cast<unsigned long long>(root));
    if (!root) {
        ImGui::TextDisabled("(no UI root - open the game)");
        return;
    }

    struct Found { int depth; std::string sid; std::string text; float x; float y; };
    std::vector<Found> found;
    struct Node { uintptr_t addr; int depth; };
    std::vector<Node> stack;
    stack.push_back({root, 0});
    int visited = 0;
    const int kMaxNodes = 5000;
    const int kMaxDepth = 30;
    while (!stack.empty() && visited < kMaxNodes && found.size() < 250) {
        const Node n = stack.back();
        stack.pop_back();
        ++visited;
        if (n.depth > kMaxDepth || !n.addr) continue;
        if (!ctx->Ui.IsVisible(n.addr)) continue;
        const std::string text = ctx->Ui.GetText(n.addr);
        if (!text.empty() && text.size() < 64) {
            Found f;
            f.depth = n.depth;
            f.sid = ctx->Ui.GetStringId(n.addr);
            f.text = text;
            f.x = 0.f; f.y = 0.f;
            float w = 0.f, h = 0.f;
            ctx->Ui.ComputeScreenRect(n.addr, f.x, f.y, w, h);
            found.push_back(std::move(f));
        }
        for (const uintptr_t c : ctx->Ui.GetChildren(n.addr))
            if (c) stack.push_back({c, n.depth + 1});
    }

    ImGui::Text("Visited %d nodes, %d have text:", visited,
                static_cast<int>(found.size()));
    const ImGuiTableFlags flags = ImGuiTableFlags_Borders
                                | ImGuiTableFlags_RowBg
                                | ImGuiTableFlags_SizingStretchProp
                                | ImGuiTableFlags_ScrollY;
    if (ImGui::BeginTable("##qs_uitree", 4, flags, ImVec2(0.f, 320.f))) {
        ImGui::TableSetupColumn("Dep", ImGuiTableColumnFlags_WidthFixed, 34.f);
        ImGui::TableSetupColumn("StringId", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Text", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("XY", ImGuiTableColumnFlags_WidthFixed, 96.f);
        ImGui::TableHeadersRow();
        for (const auto& f : found) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%d", f.depth);
            ImGui::TableNextColumn(); ImGui::TextUnformatted(f.sid.c_str());
            ImGui::TableNextColumn(); ImGui::TextUnformatted(f.text.c_str());
            ImGui::TableNextColumn(); ImGui::Text("%.0f,%.0f", f.x, f.y);
        }
        ImGui::EndTable();
    }
}

} // namespace QuickStashUi
