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

} // namespace QuickStashGame
