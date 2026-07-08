#pragma once

#include "../config/Settings.h"
#include "sdk/PluginSDK.h"

#include <cstring>
#include <optional>
#include <string>

namespace QuickStashGame {

inline std::optional<PluginSDK::Inventory> FindMainInventory(
    const PluginSDK::Context* ctx) {
    if (!ctx) return std::nullopt;
    static const char* kNames[] = {
        "MainInventory1", "Main Inventory", "Backpack", "Player Inventory"};

    // GetAll() materializes every inventory and reads every item's strings, so
    // call it ONCE and do all matching over the local snapshot. (The previous
    // version called GetAll() up to five times per lookup, and this lookup runs
    // several times per frame.)
    const auto all = ctx->Inventory.GetAll();

    // Pass 1: preferred names, in priority order.
    for (const char* want : kNames) {
        for (const auto& inv : all) {
            const char* name = ctx->Inventory.GetName(inv.InventoryId);
            if (name && inv.Grid.Valid && strcmp(name, want) == 0)
                return inv;
        }
    }

    // Pass 2: fall back to any valid grid matching the backpack dimensions.
    for (const auto& inv : all) {
        const char* name = ctx->Inventory.GetName(inv.InventoryId);
        if (!inv.Grid.Valid || !name) continue;
        if (inv.TotalBoxesX == QuickStashConfig::kGridCols
            && inv.TotalBoxesY == QuickStashConfig::kGridRows)
            return inv;
    }
    return std::nullopt;
}

// The player's own always-present grid inventories (backpack, equipment slots,
// cursor). PoeFixer enumerates ALL of these plus EVERY stash tab you own
// (100+), so identifying the *open* stash means excluding these fixed slots and
// then keeping only a tab that is actually laid out on screen. Matched by name
// prefix so weapon-swap / ring pairs (Weapon1/2, Ring1/2, Offhand1/2) are all
// covered.
inline bool IsPlayerSlotName(const char* name) {
    if (!name) return true;  // no name -> not a usable stash target
    static const char* kSlotPrefixes[] = {
        "MainInventory", "BodyArmour", "Weapon", "Offhand", "Helm", "Amulet",
        "Ring", "Gloves", "Boots", "Belt", "Flask", "Cursor", "Trinket"};
    for (const char* p : kSlotPrefixes) {
        size_t n = 0;
        while (p[n] != '\0') ++n;
        if (strncmp(name, p, n) == 0) return true;
    }
    return false;
}

// True if the grid's on-screen rectangle sits inside the game window. Closed
// stash tabs are still enumerated but are not laid out where the player can
// see/click them; the open tab has a real, on-screen rect. A small negative
// margin tolerates a panel whose origin sits a hair off the top/left edge.
inline bool GridOnScreen(const PluginSDK::Inventory& inv,
                         float displayW, float displayH) {
    if (!inv.Grid.Valid || inv.Grid.CellSize <= 0.f) return false;
    if (displayW <= 0.f || displayH <= 0.f) return false;
    const float x = inv.Grid.GridScreenX;
    const float y = inv.Grid.GridScreenY;
    const float margin = 4.f;
    return x >= -margin && y >= -margin && x < displayW && y < displayH;
}

// True if a single item is actually rendered on screen (so it can be clicked).
// PoeFixer only assigns a real screen rect to items in the *visible* stash tab;
// items in the other loaded-but-hidden affinity tabs report ScreenValid=false
// or an off-screen rect. This is what lets withdraw target the open tab WITHOUT
// identifying which inventory it is — and it also handles special tabs
// (currency/fragment/etc.) whose grid-cell math doesn't map to click points.
inline bool ItemOnScreen(const PluginSDK::InventoryItem& item,
                         float displayW, float displayH) {
    if (!item.ScreenValid) return false;
    if (item.ScreenW <= 0.f || item.ScreenH <= 0.f) return false;
    if (displayW <= 0.f || displayH <= 0.f) return false;
    const float cx = item.ScreenX + item.ScreenW * 0.5f;
    const float cy = item.ScreenY + item.ScreenH * 0.5f;
    return cx >= 0.f && cy >= 0.f && cx < displayW && cy < displayH;
}

// Identify the currently-open stash tab (the withdraw source): a valid,
// on-screen grid that is neither the main backpack nor an equipment/cursor
// slot. If more than one qualifies (shouldn't normally happen — only the
// selected tab is laid out), prefer the one carrying the most items, then the
// largest grid, for a stable pick.
inline std::optional<PluginSDK::Inventory> FindOpenStash(
    const PluginSDK::Context* ctx, int mainInventoryId,
    float displayW, float displayH) {
    if (!ctx) return std::nullopt;
    const auto all = ctx->Inventory.GetAll();

    const PluginSDK::Inventory* best = nullptr;
    for (const auto& inv : all) {
        if (inv.InventoryId == mainInventoryId) continue;
        const char* name = ctx->Inventory.GetName(inv.InventoryId);
        if (IsPlayerSlotName(name)) continue;
        if (!GridOnScreen(inv, displayW, displayH)) continue;

        if (!best) { best = &inv; continue; }
        const long long invArea =
            static_cast<long long>(inv.TotalBoxesX) * inv.TotalBoxesY;
        const long long bestArea =
            static_cast<long long>(best->TotalBoxesX) * best->TotalBoxesY;
        if (inv.Items.size() > best->Items.size()
            || (inv.Items.size() == best->Items.size() && invArea > bestArea))
            best = &inv;
    }
    if (best) return *best;
    return std::nullopt;
}

} // namespace QuickStashGame
