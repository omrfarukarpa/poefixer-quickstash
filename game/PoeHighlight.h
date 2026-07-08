#pragma once

#include "sdk/PluginSDK.h"

#include <string>
#include <utility>
#include <vector>

namespace QuickStashGame {

inline char LowerAscii(char c) {
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
}

inline bool IEqualsCI(const std::string& a, const char* b) {
    size_t i = 0;
    for (; i < a.size(); ++i) {
        if (b[i] == '\0') return false;
        if (LowerAscii(a[i]) != LowerAscii(b[i])) return false;
    }
    return b[i] == '\0';
}

inline bool IStartsWithCI(const std::string& s, const char* p) {
    for (size_t i = 0; p[i] != '\0'; ++i) {
        if (i >= s.size()) return false;
        if (LowerAscii(s[i]) != LowerAscii(p[i])) return false;
    }
    return true;
}

struct PoeHighlight {
    std::string filter;
    bool  found = false;
    float x = 0.f;
    float y = 0.f;
};

inline PoeHighlight ReadPoeHighlight(const PluginSDK::Context* ctx) {
    PoeHighlight r;
    if (!ctx) return r;
    const uintptr_t root = ctx->Ui.GetGameUiRoot();
    if (!root) return r;

    struct TextEl { std::string text; float x; float y; };
    std::vector<TextEl> texts;
    std::vector<std::pair<uintptr_t, int>> stack;
    stack.push_back({root, 0});
    int visited = 0;
    while (!stack.empty() && visited < 6000) {
        const auto node = stack.back();
        stack.pop_back();
        ++visited;
        const uintptr_t addr = node.first;
        const int depth = node.second;
        if (!addr || depth > 32) continue;
        if (!ctx->Ui.IsVisible(addr)) continue;
        std::string t = ctx->Ui.GetText(addr);
        if (!t.empty() && t.size() < 64) {
            float x = 0.f, y = 0.f, w = 0.f, h = 0.f;
            ctx->Ui.ComputeScreenRect(addr, x, y, w, h);
            texts.push_back({std::move(t), x, y});
        }
        for (const uintptr_t c : ctx->Ui.GetChildren(addr))
            if (c) stack.push_back({c, depth + 1});
    }

    float anchorX = -1.f, anchorY = -1.f;
    for (const auto& e : texts) {
        if (IEqualsCI(e.text, "Highlight Items")) {
            anchorX = e.x;
            anchorY = e.y;
            break;
        }
    }
    if (anchorY < 0.f) return r;
    r.found = true;
    r.x = anchorX;
    r.y = anchorY;

    for (const auto& e : texts) {
        if (IEqualsCI(e.text, "Highlight Items")) continue;
        if (IStartsWithCI(e.text, "Type keywords")) continue;
        if (e.x <= anchorX) continue;
        float dy = e.y - anchorY;
        if (dy < 0.f) dy = -dy;
        if (dy > 20.f) continue;
        r.filter = e.text;
        break;
    }
    return r;
}

} // namespace QuickStashGame
