#pragma once

#include "../config/Settings.h"
#include "PanelDetector.h"
#include "TransferPlanner.h"
#include "sdk/PluginSDK.h"

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace QuickStashGame {

struct ScreenRect {
    float x = 0.f;
    float y = 0.f;
    float w = 0.f;
    float h = 0.f;
};

inline ScreenPoint RectCenter(const ScreenRect& r) {
    return ScreenPoint{
        static_cast<int>(r.x + r.w * 0.5f + 0.5f),
        static_cast<int>(r.y + r.h * 0.5f + 0.5f)};
}

inline bool ContainsCI(const std::string& hay, const std::string& needle) {
    if (needle.empty()) return true;
    if (needle.size() > hay.size()) return false;
    auto low = [](unsigned char c) { return static_cast<char>(std::tolower(c)); };
    for (size_t i = 0; i + needle.size() <= hay.size(); ++i) {
        size_t j = 0;
        for (; j < needle.size(); ++j)
            if (low(static_cast<unsigned char>(hay[i + j]))
                != low(static_cast<unsigned char>(needle[j])))
                break;
        if (j == needle.size()) return true;
    }
    return false;
}

inline constexpr int kMaxModReadsPerScan = 40;
inline constexpr size_t kModCacheMax = 4000;

inline constexpr int kAggItemRarity           = 8206;
inline constexpr int kAggPackSize             = 8207;
inline constexpr int kAggMonsterRarity        = 8208;
inline constexpr int kAggMonsterEffectiveness = 8209;
inline constexpr int kAggWaystoneDropChance   = 8210;

inline const char* AggregatedLabel(int key) {
    switch (key) {
        case kAggItemRarity:           return "Item Rarity";
        case kAggPackSize:             return "Monster Pack Size";
        case kAggMonsterRarity:        return "Monster Rarity";
        case kAggMonsterEffectiveness: return "Monster Effectiveness";
        case kAggWaystoneDropChance:   return "Waystone Drop Chance";
        default:                       return nullptr;
    }
}

inline std::string BuildNameSearchText(const PluginSDK::InventoryItem& item) {
    std::string s;
    s.reserve(128);
    s += item.BaseTypeName;
    s += '\n';
    s += item.UniqueName;
    if (item.IsCorrupted) s += "\nCorrupted";
    return s;
}

inline std::string BuildModText(const PluginSDK::Context* ctx,
                                const PluginSDK::InventoryItem& item) {
    std::string s;
    if (!ctx || !item.Address) return s;
    if (ctx->Inventory.ReadItemBaseTypeName(item.Address).empty()) return s;
    const auto mods = ctx->Inventory.ReadItemMods(item.Address);
    const std::vector<const std::vector<PluginSDK::Mod>*> groups = {
        &mods.ImplicitMods, &mods.ExplicitMods, &mods.EnchantMods,
        &mods.HellscapeMods, &mods.CrucibleMods};
    for (const auto* g : groups) {
        for (const auto& m : *g) {
            if (!m.AffixName.empty()) { s += '\n'; s += m.AffixName; }
            if (!m.Name.empty())      { s += '\n'; s += m.Name; }
            if (!m.Id.empty())        { s += '\n'; s += m.Id; }
            if (!m.StatKey.empty())   { s += '\n'; s += m.StatKey; }
            const auto line = ctx->Inventory.FormatStat(m.StatKey, m.Value0, m.Value1);
            if (!line.empty())        { s += '\n'; s += line; }
        }
    }
    for (const auto& kv : ctx->Inventory.ReadItemAggregatedStats(item.Address)) {
        if (kv.second == 0) continue;
        if (const char* lbl = AggregatedLabel(kv.first)) {
            s += '\n';
            s += lbl;
        }
    }
    return s;
}

inline std::string DebugAggregatedPairs(const PluginSDK::Context* ctx,
                                        const PluginSDK::InventoryItem& item) {
    std::string s;
    if (!ctx || !item.Address) return s;
    if (ctx->Inventory.ReadItemBaseTypeName(item.Address).empty()) return s;
    for (const auto& kv : ctx->Inventory.ReadItemAggregatedStats(item.Address)) {
        s += std::to_string(kv.first);
        s += ':';
        s += std::to_string(kv.second);
        s += ' ';
    }
    return s;
}

class ModTextCache {
public:
    void BeginScan() {
        m_budget = kMaxModReadsPerScan;
        if (m_map.size() > kModCacheMax) m_map.clear();
    }

    const std::string& Get(const PluginSDK::Context* ctx,
                           const PluginSDK::InventoryItem& item) {
        Entry& e = m_map[item.Address];
        if (e.sourcePath != item.Path) {
            e.sourcePath = item.Path;
            e.read = false;
            e.text.clear();
        }
        if (!e.read && m_budget > 0) {
            e.text = BuildModText(ctx, item);
            e.read = true;
            --m_budget;
        }
        return e.text;
    }

private:
    struct Entry {
        std::string sourcePath;
        std::string text;
        bool read = false;
    };
    std::unordered_map<uintptr_t, Entry> m_map;
    int m_budget = 0;
};

inline bool GridLayoutPlausible(const PluginSDK::Inventory& inv, float displayW) {
    if (inv.TotalBoxesY < 6) return false;
    if (inv.Grid.CellSize > 0.f
        && static_cast<float>(inv.TotalBoxesX) * inv.Grid.CellSize > displayW)
        return false;
    return true;
}

inline std::optional<ScreenRect> ResolveItemRect(
    const PluginSDK::Inventory& inv, const PluginSDK::InventoryItem& item,
    float displayW, float displayH) {
    if (ItemOnScreen(item, displayW, displayH)) {
        return ScreenRect{item.ScreenX, item.ScreenY, item.ScreenW, item.ScreenH};
    }
    if (inv.Grid.Valid && GridOnScreen(inv, displayW, displayH)
        && inv.Grid.CellSize > 0.f && GridLayoutPlausible(inv, displayW)) {
        const float cell = inv.Grid.CellSize;
        return ScreenRect{
            inv.Grid.GridScreenX + static_cast<float>(item.SlotX) * cell,
            inv.Grid.GridScreenY + static_cast<float>(item.SlotY) * cell,
            static_cast<float>(item.Width) * cell,
            static_cast<float>(item.Height) * cell};
    }
    return std::nullopt;
}

inline bool TabOnScreen(const PluginSDK::Inventory& inv,
                        float displayW, float displayH) {
    if (GridOnScreen(inv, displayW, displayH)) return true;
    for (const auto& it : inv.Items)
        if (ItemOnScreen(it, displayW, displayH)) return true;
    return false;
}

inline std::optional<PluginSDK::Inventory> FindOpenStashAny(
    const PluginSDK::Context* ctx, int mainInventoryId,
    float displayW, float displayH) {
    if (!ctx) return std::nullopt;
    const auto all = ctx->Inventory.GetAll();

    const PluginSDK::Inventory* best = nullptr;
    for (const auto& inv : all) {
        if (inv.InventoryId == mainInventoryId) continue;
        const char* name = ctx->Inventory.GetName(inv.InventoryId);
        if (IsPlayerSlotName(name)) continue;
        const long long area =
            static_cast<long long>(inv.TotalBoxesX) * inv.TotalBoxesY;
        if (area < 10) continue;
        if (inv.Items.empty()) continue;
        if (!TabOnScreen(inv, displayW, displayH)) continue;
        if (!best || inv.Items.size() > best->Items.size())
            best = &inv;
    }
    if (best) return *best;
    return std::nullopt;
}

struct WithdrawCandidate {
    int slotY = 0;
    int slotX = 0;
    int stack = 0;
    ScreenRect rect;
    std::string search;
};

inline std::vector<WithdrawCandidate> CollectCandidates(
    const PluginSDK::Context* ctx, const PluginSDK::Inventory& inv,
    float displayW, float displayH, ModTextCache* modCache, bool readMods) {
    if (modCache) modCache->BeginScan();
    std::vector<WithdrawCandidate> out;
    for (const auto& item : inv.Items) {
        const auto r = ResolveItemRect(inv, item, displayW, displayH);
        if (!r) continue;
        WithdrawCandidate c;
        c.slotY = item.SlotY;
        c.slotX = item.SlotX;
        c.stack = item.StackCount;
        c.rect = *r;
        c.search = BuildNameSearchText(item);
        if (readMods && modCache) {
            const std::string& mt = modCache->Get(ctx, item);
            c.search += mt;
        }
        out.push_back(std::move(c));
    }
    std::sort(out.begin(), out.end(),
        [](const WithdrawCandidate& a, const WithdrawCandidate& b) {
            if (a.slotY != b.slotY) return a.slotY < b.slotY;
            return a.slotX < b.slotX;
        });
    return out;
}

struct WithdrawSelection {
    std::vector<ScreenRect> rects;
    int totalQty = 0;
};

inline WithdrawSelection FilterCandidates(
    const std::vector<WithdrawCandidate>& cands, const std::string& filter) {
    WithdrawSelection sel;
    for (const auto& c : cands) {
        if (!filter.empty() && !ContainsCI(c.search, filter)) continue;
        sel.rects.push_back(c.rect);
        sel.totalQty += (c.stack > 0 ? c.stack : 1);
    }
    return sel;
}

} // namespace QuickStashGame
