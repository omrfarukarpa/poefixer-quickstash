#pragma once

#include "../third_party/json.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <string>

namespace QuickStashConfig {

inline constexpr int kGridCols = 12;
inline constexpr int kGridRows = 5;

// Single source of truth for timing bounds: used by BOTH the DrawSettings
// sliders and Settings::Load clamps, so a value loaded from JSON can never sit
// outside what the slider can show (which previously made it snap silently).
inline constexpr int kClickDelayMinMs      = 10;
inline constexpr int kClickDelayMaxMs      = 500;
inline constexpr int kPostClickDelayMinMs  = 0;
inline constexpr int kPostClickDelayMaxMs  = 500;
inline constexpr int kCursorSettleMinMs    = 0;
inline constexpr int kCursorSettleMaxMs    = 200;
inline constexpr int kCompletionHoldMinMs  = 0;
inline constexpr int kCompletionHoldMaxMs  = 1000;

struct Settings {
    bool enabled = true;
    std::array<std::array<bool, kGridCols>, kGridRows> ignoredCells{};
    int  clickDelayMs = 75;
    int  postClickDelayMs = 50;
    int  cursorSettleMs = 20;
    int  completionHoldMs = 125;
    bool cancelOnRightClick = true;
    bool verifyPanelsOpen = true;
    bool debugMode = false;
    float buttonOffsetX = 0.f;
    float buttonOffsetY = 0.f;

    Settings() { SetWeaponColumnExcluded(); }

    void SetWeaponColumnExcluded() {
        for (int y = 0; y < kGridRows; ++y)
            ignoredCells[static_cast<size_t>(y)][0] = true;
    }

    void ClearIgnoredCells() {
        for (auto& row : ignoredCells)
            row.fill(false);
    }

    bool IsCellIgnored(int x, int y) const {
        if (x < 0 || x >= kGridCols || y < 0 || y >= kGridRows) return true;
        return ignoredCells[static_cast<size_t>(y)][static_cast<size_t>(x)];
    }

    std::filesystem::path SettingsPath(const std::filesystem::path& pluginDir) const {
        return pluginDir / "config" / "settings.json";
    }

    // Reads settings.json into *this. A corrupt or hand-edited file (malformed
    // JSON, trailing comma, or a value with the wrong type) must NEVER throw:
    // Load is called from OnEnable, and an exception escaping across the C ABI
    // boundary into the host is undefined behaviour and crashes the whole host.
    // On any failure we silently keep the in-memory defaults.
    void Load(const std::filesystem::path& pluginDir) {
        try {
            const auto path = SettingsPath(pluginDir);
            if (!std::filesystem::exists(path)) return;
            std::ifstream in(path);
            if (!in.is_open()) return;

            // Non-throwing parse: a malformed document yields a discarded value
            // instead of raising json::parse_error.
            nlohmann::json j = nlohmann::json::parse(in, nullptr, /*allow_exceptions=*/false);
            if (j.is_discarded() || !j.is_object()) return;

            enabled = j.value("enabled", enabled);
            clickDelayMs = std::clamp(j.value("click_delay_ms", clickDelayMs),
                                      kClickDelayMinMs, kClickDelayMaxMs);
            postClickDelayMs = std::clamp(j.value("post_click_delay_ms", postClickDelayMs),
                                          kPostClickDelayMinMs, kPostClickDelayMaxMs);
            cursorSettleMs = std::clamp(j.value("cursor_settle_ms", cursorSettleMs),
                                        kCursorSettleMinMs, kCursorSettleMaxMs);
            completionHoldMs = std::clamp(j.value("completion_hold_ms", completionHoldMs),
                                          kCompletionHoldMinMs, kCompletionHoldMaxMs);
            cancelOnRightClick = j.value("cancel_on_right_click", cancelOnRightClick);
            verifyPanelsOpen = j.value("verify_panels_open", verifyPanelsOpen);
            debugMode = j.value("debug_mode", debugMode);
            buttonOffsetX = j.value("button_offset_x", buttonOffsetX);
            buttonOffsetY = j.value("button_offset_y", buttonOffsetY);

            if (j.contains("ignored_cells") && j["ignored_cells"].is_array()) {
                const auto& rows = j["ignored_cells"];
                for (int y = 0; y < kGridRows && y < static_cast<int>(rows.size()); ++y) {
                    if (!rows[y].is_array()) continue;
                    for (int x = 0; x < kGridCols && x < static_cast<int>(rows[y].size()); ++x) {
                        // Guard every cell: a non-boolean entry would otherwise
                        // throw type_error.302 out of get<bool>().
                        if (rows[y][x].is_boolean())
                            ignoredCells[static_cast<size_t>(y)][static_cast<size_t>(x)] =
                                rows[y][x].get<bool>();
                    }
                }
            }
        } catch (...) {
            // Corrupt/unexpected file shape — keep defaults, never propagate.
        }
    }

    void Save(const std::filesystem::path& pluginDir) const {
      try {
        std::error_code ec;
        std::filesystem::create_directories(pluginDir / "config", ec);

        nlohmann::json j;
        j["enabled"] = enabled;
        j["click_delay_ms"] = clickDelayMs;
        j["post_click_delay_ms"] = postClickDelayMs;
        j["cursor_settle_ms"] = cursorSettleMs;
        j["completion_hold_ms"] = completionHoldMs;
        j["cancel_on_right_click"] = cancelOnRightClick;
        j["verify_panels_open"] = verifyPanelsOpen;
        j["debug_mode"] = debugMode;
        j["button_offset_x"] = buttonOffsetX;
        j["button_offset_y"] = buttonOffsetY;

        nlohmann::json rows = nlohmann::json::array();
        for (int y = 0; y < kGridRows; ++y) {
            nlohmann::json row = nlohmann::json::array();
            for (int x = 0; x < kGridCols; ++x)
                row.push_back(ignoredCells[static_cast<size_t>(y)][static_cast<size_t>(x)]);
            rows.push_back(std::move(row));
        }
        j["ignored_cells"] = std::move(rows);

        std::ofstream out(SettingsPath(pluginDir));
        if (out.is_open())
            out << j.dump(2);
      } catch (...) {
        // Disk/serialization failure must not propagate across the C ABI.
      }
    }
};

} // namespace QuickStashConfig
