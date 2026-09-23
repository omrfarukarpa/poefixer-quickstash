#pragma once

#include "../game/PanelDetector.h"
#include "../game/PoeHighlight.h"
#include "../game/TransferPlanner.h"
#include "../game/WithdrawPlanner.h"
#include "sdk/PluginSDK.h"

#include <imgui.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

namespace QuickStashUi {

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

    ctx->Inventory.Scan(-1);
    const auto all = ctx->Inventory.GetAll();
    ImGui::Text("Inventories enumerated: %d", static_cast<int>(all.size()));

    const auto main = QuickStashGame::FindMainInventory(ctx);
    const int mainId = main ? main->InventoryId : -1;

    const auto openStash =
        QuickStashGame::FindOpenStash(ctx, mainId, disp.x, disp.y);
    if (openStash) {
        const char* name = ctx->Inventory.GetName(openStash->InventoryId);
        const bool guild = QuickStashGame::IsGuildStashInventory(*openStash);
        ImGui::TextColored(ImVec4(0.4f, 0.85f, 0.4f, 1.f),
            "GRID-detected open stash: id %d (%s)%s, %d items",
            openStash->InventoryId, name ? name : "",
            guild ? " [GUILD]" : "",
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

    int totalOnScreen = 0;
    int invsWithOnScreen = 0;
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
    if (ImGui::BeginTable("##qs_haystack", 5, flags, ImVec2(0.f, 320.f))) {
        ImGui::TableSetupColumn("Slot", ImGuiTableColumnFlags_WidthFixed, 52.f);
        ImGui::TableSetupColumn("BaseType", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Corr(i/m)", ImGuiTableColumnFlags_WidthFixed, 70.f);
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
            const bool modCorrupt =
                (it.Address
                 && !ctx->Inventory.ReadItemBaseTypeName(it.Address).empty())
                    ? ctx->Inventory.ReadItemMods(it.Address).IsCorrupted
                    : false;
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%d,%d", it.SlotX, it.SlotY);
            ImGui::TableNextColumn(); ImGui::TextUnformatted(it.BaseTypeName.c_str());
            ImGui::TableNextColumn();
            ImGui::Text("%d/%d", it.IsCorrupted ? 1 : 0, modCorrupt ? 1 : 0);
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

inline constexpr int kTextReportItems = 30;

inline bool IsValidUtf8(const std::string& s) {
    size_t i = 0;
    const size_t n = s.size();
    while (i < n) {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 0x80) {
            ++i;
            continue;
        }
        size_t len = 0;
        unsigned int cp = 0;
        if (c >= 0xC2 && c <= 0xDF)      { len = 2; cp = c & 0x1Fu; }
        else if (c >= 0xE0 && c <= 0xEF) { len = 3; cp = c & 0x0Fu; }
        else if (c >= 0xF0 && c <= 0xF4) { len = 4; cp = c & 0x07u; }
        else return false;
        if (len > n - i) return false;
        for (size_t k = 1; k < len; ++k) {
            const unsigned char cc = static_cast<unsigned char>(s[i + k]);
            if ((cc & 0xC0u) != 0x80u) return false;
            cp = (cp << 6) | (cc & 0x3Fu);
        }
        if (len == 3 && (cp < 0x800u || (cp >= 0xD800u && cp <= 0xDFFFu))) return false;
        if (len == 4 && (cp < 0x10000u || cp > 0x10FFFFu)) return false;
        i += len;
    }
    return true;
}

inline const char* TextEncodingClass(const std::string& s) {
    if (s.empty()) return "empty";
    bool high = false;
    bool question = false;
    for (const char ch : s) {
        const unsigned char c = static_cast<unsigned char>(ch);
        if (c >= 0x80) high = true;
        else if (c == '?') question = true;
    }
    if (!high) return question ? "ascii+?" : "ascii";
    if (!IsValidUtf8(s)) return "not-utf8";
    return question ? "utf8+?" : "utf8";
}

inline std::string HexBytes(const std::string& s, size_t maxBytes) {
    static const char kHex[] = "0123456789ABCDEF";
    const size_t n = s.size() < maxBytes ? s.size() : maxBytes;
    std::string out;
    out.reserve(n * 3 + 4);
    for (size_t i = 0; i < n; ++i) {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        if (i) out += ' ';
        out += kHex[c >> 4];
        out += kHex[c & 0x0F];
    }
    if (s.size() > n) out += " ...";
    return out;
}

inline std::string ReportText(const std::string& s, size_t maxBytes) {
    if (!IsValidUtf8(s)) return "(not utf8, see hex)";
    std::string out = s;
    for (auto& ch : out)
        if (ch == '\n' || ch == '\r') ch = '|';
    if (out.size() > maxBytes) {
        size_t cut = maxBytes;
        while (cut > 0 && (static_cast<unsigned char>(out[cut]) & 0xC0u) == 0x80u) --cut;
        out.resize(cut);
        out += " ...";
    }
    return out;
}

inline void AppendReportField(std::string& r, const char* label,
                              const std::string& value, size_t maxText) {
    r += "  ";
    r += label;
    r += " [";
    r += TextEncodingClass(value);
    r += "] \"";
    r += ReportText(value, maxText);
    r += '"';
    if (!value.empty()) {
        r += " hex: ";
        r += HexBytes(value, 48);
    }
    r += '\n';
}

inline std::string BuildTextReport(const PluginSDK::Context* ctx, const char* version,
                                   const QuickStashGame::PoeHighlight& poe,
                                   const PluginSDK::Inventory* tab) {
    std::string r;
    r.reserve(16384);
    r += "Quick Stash ";
    r += version ? version : "";
    r += " text report\n";
    r += "Highlight box found: ";
    r += poe.found ? "yes\n" : "no\n";
    AppendReportField(r, "filter", poe.filter, 160);
    if (!ctx || !tab) {
        r += "Open stash tab: none\n";
        return r;
    }
    const char* tabName = ctx->Inventory.GetName(tab->InventoryId);
    r += "Open stash tab: id ";
    r += std::to_string(tab->InventoryId);
    r += ", name \"";
    r += ReportText(tabName ? tabName : "", 64);
    r += "\", items ";
    r += std::to_string(tab->Items.size());
    r += '\n';

    const bool hasFilter = !poe.filter.empty();
    int rows = 0;
    int listHits = 0;
    int directHits = 0;
    int modHits = 0;
    std::string body;
    for (const auto& it : tab->Items) {
        if (rows >= kTextReportItems) break;
        ++rows;
        const std::string directBase = it.Address
            ? ctx->Inventory.ReadItemBaseTypeName(it.Address) : std::string();
        const std::string directUnique = (it.Address && !directBase.empty())
            ? ctx->Inventory.ReadItemUniqueName(it.Address) : std::string();
        const std::string mods = QuickStashGame::BuildModText(ctx, it);
        const bool listHit = hasFilter && QuickStashGame::ContainsCI(
            QuickStashGame::BuildNameSearchText(it), poe.filter);
        const bool directHit = hasFilter
            && (QuickStashGame::ContainsCI(directBase, poe.filter)
                || (!directUnique.empty()
                    && QuickStashGame::ContainsCI(directUnique, poe.filter)));
        const bool modHit = hasFilter && QuickStashGame::ContainsCI(mods, poe.filter);
        if (listHit) ++listHits;
        if (directHit) ++directHits;
        if (modHit) ++modHits;

        body += "#";
        body += std::to_string(rows);
        body += " slot ";
        body += std::to_string(it.SlotX);
        body += ',';
        body += std::to_string(it.SlotY);
        body += " rarity ";
        body += std::to_string(it.Rarity);
        body += " match list/direct/mods ";
        body += listHit ? '1' : '0';
        body += '/';
        body += directHit ? '1' : '0';
        body += '/';
        body += modHit ? '1' : '0';
        body += '\n';
        AppendReportField(body, "path", it.Path, 160);
        AppendReportField(body, "list base", it.BaseTypeName, 160);
        AppendReportField(body, "direct base", directBase, 160);
        if (!it.UniqueName.empty() || !directUnique.empty()) {
            AppendReportField(body, "list unique", it.UniqueName, 160);
            AppendReportField(body, "direct unique", directUnique, 160);
        }
        AppendReportField(body, "mods", mods, 600);
    }
    r += "Reported items: ";
    r += std::to_string(rows);
    r += ", matches list/direct/mods: ";
    r += std::to_string(listHits);
    r += '/';
    r += std::to_string(directHits);
    r += '/';
    r += std::to_string(modHits);
    r += "\n\n";
    r += body;
    return r;
}

inline void DrawTextEncodingDiagnostics(const PluginSDK::Context* ctx, const char* version,
                                        const std::filesystem::path& pluginDir) {
    if (!ctx) return;
    if (!ImGui::CollapsingHeader("Diagnostics: text encoding report"))
        return;

    ImGui::TextWrapped(
        "Open the stash tab, type a word in PoE's Highlight Items box, then press "
        "the button. It copies a report of the search text and the item names/mods "
        "exactly as the host returns them, and saves it to config/text-report.txt "
        "in the plugin folder. Attach that report to your bug report.");

    static std::string s_status;
    const auto poe = QuickStashGame::ReadPoeHighlight(ctx);
    const ImVec2 disp = ImGui::GetIO().DisplaySize;
    ctx->Inventory.Scan(-1);
    const auto main = QuickStashGame::FindMainInventory(ctx);
    const int mainId = main ? main->InventoryId : -1;
    const auto tab = QuickStashGame::FindOpenStashAny(ctx, mainId, disp.x, disp.y);

    if (ImGui::Button("Copy text report")) {
        const std::string report =
            BuildTextReport(ctx, version, poe, tab ? &*tab : nullptr);
        ImGui::SetClipboardText(report.c_str());
        bool saved = false;
        if (!pluginDir.empty()) {
            std::error_code ec;
            std::filesystem::create_directories(pluginDir / "config", ec);
            std::ofstream out(pluginDir / "config" / "text-report.txt",
                              std::ios::binary | std::ios::trunc);
            if (out) {
                out.write(report.data(), static_cast<std::streamsize>(report.size()));
                saved = static_cast<bool>(out);
            }
        }
        s_status = "Copied " + std::to_string(report.size()) + " bytes"
                 + (saved ? ", saved config/text-report.txt" : ", file not saved");
    }
    if (!s_status.empty()) {
        ImGui::SameLine();
        ImGui::TextDisabled("%s", s_status.c_str());
    }

    ImGui::Text("Filter [%s]: %s", TextEncodingClass(poe.filter),
                HexBytes(poe.filter, 24).c_str());
    if (!tab) {
        ImGui::TextDisabled("(open a stash tab)");
        return;
    }

    const ImGuiTableFlags flags = ImGuiTableFlags_Borders
                                | ImGuiTableFlags_RowBg
                                | ImGuiTableFlags_SizingStretchProp
                                | ImGuiTableFlags_ScrollY;
    if (ImGui::BeginTable("##qs_textenc", 5, flags, ImVec2(0.f, 260.f))) {
        ImGui::TableSetupColumn("Slot", ImGuiTableColumnFlags_WidthFixed, 52.f);
        ImGui::TableSetupColumn("List name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("List bytes", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Direct name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Direct bytes", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();
        int rows = 0;
        for (const auto& it : tab->Items) {
            if (rows++ >= kTextReportItems) break;
            const std::string direct = it.Address
                ? ctx->Inventory.ReadItemBaseTypeName(it.Address) : std::string();
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%d,%d", it.SlotX, it.SlotY);
            ImGui::TableNextColumn();
            ImGui::Text("[%s] %s", TextEncodingClass(it.BaseTypeName),
                        ReportText(it.BaseTypeName, 64).c_str());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(HexBytes(it.BaseTypeName, 12).c_str());
            ImGui::TableNextColumn();
            ImGui::Text("[%s] %s", TextEncodingClass(direct),
                        ReportText(direct, 64).c_str());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(HexBytes(direct, 12).c_str());
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

}
