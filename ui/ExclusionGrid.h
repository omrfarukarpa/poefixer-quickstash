#pragma once

#include "../config/Settings.h"
#include <imgui.h>

namespace QuickStashUi {

// Shared grid colours so the cells and the legend always match.
inline constexpr ImU32 kCellExcluded = IM_COL32(180, 60, 60, 220);   // red
inline constexpr ImU32 kCellIncluded = IM_COL32(60, 120, 60, 180);   // green
inline constexpr ImU32 kCellBorder   = IM_COL32(120, 120, 120, 255);

// Draws a small colour swatch followed by a label, used for the legend.
inline void LegendSwatch(const char* label, ImU32 color) {
    const float sz = ImGui::GetFontSize();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImVec2 p1(p0.x + sz, p0.y + sz);
    dl->AddRectFilled(p0, p1, color);
    dl->AddRect(p0, p1, kCellBorder);
    ImGui::Dummy(ImVec2(sz, sz));
    ImGui::SameLine();
    ImGui::TextUnformatted(label);
}

inline void DrawExclusionGrid(QuickStashConfig::Settings& settings) {
    ImGui::Text("Excluded cells (click to toggle):");
    ImGui::TextDisabled("Columns = X (0=left), Rows = Y (0=top)");

    // Legend: explain what the two cell colours mean.
    LegendSwatch("Green = transferred", kCellIncluded);
    ImGui::SameLine(0.f, 18.f);
    LegendSwatch("Red = skipped (kept in bag)", kCellExcluded);

    if (ImGui::Button("Clear all")) {
        settings.ClearIgnoredCells();
    }
    ImGui::SameLine();
    if (ImGui::Button("Weapon column (col 0)")) {
        settings.SetWeaponColumnExcluded();
    }

    const float cellSize = 22.f;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 origin = ImGui::GetCursorScreenPos();

    for (int y = 0; y < QuickStashConfig::kGridRows; ++y) {
        for (int x = 0; x < QuickStashConfig::kGridCols; ++x) {
            ImVec2 p0(origin.x + x * cellSize, origin.y + y * cellSize);
            ImVec2 p1(p0.x + cellSize - 2.f, p0.y + cellSize - 2.f);
            bool ignored = settings.ignoredCells[static_cast<size_t>(y)][static_cast<size_t>(x)];
            ImU32 fill = ignored ? kCellExcluded : kCellIncluded;
            dl->AddRectFilled(p0, p1, fill);
            dl->AddRect(p0, p1, kCellBorder);

            ImGui::SetCursorScreenPos(p0);
            char id[32];
            snprintf(id, sizeof(id), "##ex_%d_%d", x, y);
            ImGui::InvisibleButton(id, ImVec2(cellSize - 2.f, cellSize - 2.f));
            if (ImGui::IsItemClicked())
                settings.ignoredCells[static_cast<size_t>(y)][static_cast<size_t>(x)] = !ignored;
        }
    }

    // Reserve exactly the grid's footprint so following widgets sit just below
    // it (the previous Dummy double-counted the height and left a big gap).
    ImGui::SetCursorScreenPos(ImVec2(origin.x, origin.y));
    ImGui::Dummy(ImVec2(QuickStashConfig::kGridCols * cellSize,
                        QuickStashConfig::kGridRows * cellSize));
}

} // namespace QuickStashUi
