#pragma once

#include "../config/Settings.h"
#include "sdk/PluginSDK.h"

#include <cstring>
#include <optional>
#include <string>

namespace QuickStashGame {

inline constexpr int kGuildStashInventoryId = 10002;

inline bool IsGuildStashInventory(const PluginSDK::Inventory& inv) {
    return inv.InventoryId == kGuildStashInventoryId;
}

inline std::optional<PluginSDK::Inventory> FindMainInventory(
    const PluginSDK::Context* ctx) {
    if (!ctx) return std::nullopt;
    static const char* kNames[] = {
        "MainInventory1", "Main Inventory", "Backpack", "Player Inventory"};

    const auto all = ctx->Inventory.GetAll();

    for (const char* want : kNames) {
        for (const auto& inv : all) {
            const char* name = ctx->Inventory.GetName(inv.InventoryId);
            if (name && inv.Grid.Valid && strcmp(name, want) == 0)
                return inv;
        }
    }

    for (const auto& inv : all) {
        const char* name = ctx->Inventory.GetName(inv.InventoryId);
        if (!inv.Grid.Valid || !name || IsGuildStashInventory(inv)) continue;
        if (inv.TotalBoxesX == QuickStashConfig::kGridCols
            && inv.TotalBoxesY == QuickStashConfig::kGridRows)
            return inv;
    }
    return std::nullopt;
}

inline bool IsPlayerSlotName(const char* name) {
    if (!name) return true;
    static const char* kSlotPrefixes[] = {
        "MainInventory", "BodyArmour", "Weapon", "Offhand", "Helm", "Amulet",
        "Ring", "Gloves", "Boots", "Belt", "Flask", "Cursor", "Trinket",
        "Charm"};
    for (const char* p : kSlotPrefixes) {
        size_t n = 0;
        while (p[n] != '\0') ++n;
        if (strncmp(name, p, n) == 0) return true;
    }
    return false;
}

inline bool GridOnScreen(const PluginSDK::Inventory& inv,
                         float displayW, float displayH) {
    if (!inv.Grid.Valid || inv.Grid.CellSize <= 0.f) return false;
    if (displayW <= 0.f || displayH <= 0.f) return false;
    const float x = inv.Grid.GridScreenX;
    const float y = inv.Grid.GridScreenY;
    const float margin = 4.f;
    return x >= -margin && y >= -margin && x < displayW && y < displayH;
}

inline bool ItemOnScreen(const PluginSDK::InventoryItem& item,
                         float displayW, float displayH) {
    if (!item.ScreenValid) return false;
    if (item.ScreenW <= 0.f || item.ScreenH <= 0.f) return false;
    if (displayW <= 0.f || displayH <= 0.f) return false;
    const float cx = item.ScreenX + item.ScreenW * 0.5f;
    const float cy = item.ScreenY + item.ScreenH * 0.5f;
    return cx >= 0.f && cy >= 0.f && cx < displayW && cy < displayH;
}

inline std::optional<PluginSDK::Inventory> FindOpenStash(
    const PluginSDK::Context* ctx, int mainInventoryId,
    float displayW, float displayH) {
    if (!ctx) return std::nullopt;
    const auto all = ctx->Inventory.GetAll();

    const PluginSDK::Inventory* best = nullptr;
    for (const auto& inv : all) {
        if (inv.InventoryId == mainInventoryId) continue;

        if (!IsGuildStashInventory(inv)) {
            const char* name = ctx->Inventory.GetName(inv.InventoryId);
            if (IsPlayerSlotName(name)) continue;
        }
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

}
