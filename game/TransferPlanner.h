#pragma once

#include "../config/Settings.h"
#include "sdk/PluginSDK.h"

#include <algorithm>
#include <vector>

namespace QuickStashGame {

// Only the slot is stored. Screen coordinates are recomputed from the LIVE
// grid at click time (see TransferState::Tick), so the queue stays correct
// even if the inventory panel moves, scrolls, or the UI scale changes
// mid-transfer.
struct ClickTarget {
    int slotX = 0;
    int slotY = 0;
};

struct ScreenPoint {
    int x = 0;
    int y = 0;
};

inline float SlotCenterX(const PluginSDK::Inventory& inv, int slotX) {
    return inv.Grid.GridScreenX + (static_cast<float>(slotX) + 0.5f) * inv.Grid.CellSize;
}

inline float SlotCenterY(const PluginSDK::Inventory& inv, int slotY) {
    return inv.Grid.GridScreenY + (static_cast<float>(slotY) + 0.5f) * inv.Grid.CellSize;
}

inline bool ItemTouchesIgnoredCell(const PluginSDK::InventoryItem& item,
                                   const QuickStashConfig::Settings& settings) {
    for (int dy = 0; dy < item.Height; ++dy) {
        for (int dx = 0; dx < item.Width; ++dx) {
            if (settings.IsCellIgnored(item.SlotX + dx, item.SlotY + dy))
                return true;
        }
    }
    return false;
}

inline std::vector<ClickTarget> BuildClickQueue(
    const PluginSDK::Inventory& inv,
    const QuickStashConfig::Settings& settings) {
    std::vector<ClickTarget> queue;
    if (!inv.Grid.Valid) return queue;

    for (const auto& item : inv.Items) {
        if (ItemTouchesIgnoredCell(item, settings))
            continue;
        ClickTarget t;
        t.slotX = item.SlotX;
        t.slotY = item.SlotY;
        queue.push_back(t);
    }

    // Click top-to-bottom, left-to-right. Each item already occupies a unique
    // top-left slot, so no de-duplication is needed.
    std::sort(queue.begin(), queue.end(), [](const ClickTarget& a, const ClickTarget& b) {
        if (a.slotY != b.slotY) return a.slotY < b.slotY;
        return a.slotX < b.slotX;
    });
    return queue;
}

} // namespace QuickStashGame
