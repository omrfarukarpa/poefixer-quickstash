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

inline bool IContainsCI(const std::string& hay, const char* needle) {
    if (!needle || needle[0] == '\0') return true;
    size_t nlen = 0;
    while (needle[nlen] != '\0') ++nlen;
    if (nlen > hay.size()) return false;
    for (size_t i = 0; i + nlen <= hay.size(); ++i) {
        size_t j = 0;
        for (; j < nlen; ++j) {
            if (LowerAscii(hay[i + j]) != LowerAscii(needle[j]))
                break;
        }
        if (j == nlen) return true;
    }
    return false;
}

inline std::string TrimAscii(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && static_cast<unsigned char>(s[start]) <= ' ')
        ++start;
    size_t end = s.size();
    while (end > start && static_cast<unsigned char>(s[end - 1]) <= ' ')
        --end;
    return s.substr(start, end - start);
}

inline bool IsHighlightAnchorText(const std::string& raw) {
    const std::string s = TrimAscii(raw);
    static const char* const kAnchors[] = {
        "Highlight Items",
        "Highlight Item",
        "\xec\x95\x84\xec\x9d\xb4\xed\x85\x9c \xea\xb0\x95\xec\xa1\xb0\xed\x95\x98\xea\xb8\xb0",
        "\xec\x95\x84\xec\x9d\xb4\xed\x85\x9c \xea\xb0\x95\xec\xa1\xb0",
        "\xe7\xaa\x81\xe9\xa1\xaf\xe9\xa0\x85\xe7\x9b\xae",
        "\xe9\x86\x92\xe7\x9b\xae\xe6\x8f\x90\xe7\xa4\xba\xe9\x81\x93\xe5\x85\xb7",
        "\xe7\xaa\x81\xe9\xa1\xaf\xe9\x81\x93\xe5\x85\xb7",
        "\xe9\x86\x92\xe7\x9b\xae\xe6\x8f\x90\xe7\xa4\xba\xe9\xa0\x85\xe7\x9b\xae",
        "\xe9\xab\x98\xe4\xba\xae\xe7\x89\xa9\xe5\x93\x81",
        "\xe7\xaa\x81\xe5\x87\xba\xe6\x98\xbe\xe7\xa4\xba\xe7\x89\xa9\xe5\x93\x81",
        "\xe9\xab\x98\xe4\xba\xae\xe6\x98\xbe\xe7\xa4\xba\xe7\x89\xa9\xe5\x93\x81",
        "\xd0\x9f\xd0\xbe\xd0\xb4\xd1\x81\xd0\xb2\xd0\xb5\xd1\x82\xd0\xb8\xd1\x82\xd1\x8c \xd0\xb2\xd0\xb5\xd1\x89\xd0\xb8",
        "\xd0\xbf\xd0\xbe\xd0\xb4\xd1\x81\xd0\xb2\xd0\xb5\xd1\x82\xd0\xb8\xd1\x82\xd1\x8c \xd0\xb2\xd0\xb5\xd1\x89\xd0\xb8",
        "\xd0\x9f\xd0\xbe\xd0\xb4\xd1\x81\xd0\xb2\xd0\xb5\xd1\x82\xd0\xba\xd0\xb0 \xd0\xbf\xd1\x80\xd0\xb5\xd0\xb4\xd0\xbc\xd0\xb5\xd1\x82\xd0\xbe\xd0\xb2",
        "\xd0\xbf\xd0\xbe\xd0\xb4\xd1\x81\xd0\xb2\xd0\xb5\xd1\x82\xd0\xba\xd0\xb0 \xd0\xbf\xd1\x80\xd0\xb5\xd0\xb4\xd0\xbc\xd0\xb5\xd1\x82\xd0\xbe\xd0\xb2",
        "\xd0\x9f\xd0\xbe\xd0\xb4\xd1\x81\xd0\xb2\xd0\xb5\xd1\x82\xd0\xb8\xd1\x82\xd1\x8c \xd0\xbf\xd1\x80\xd0\xb5\xd0\xb4\xd0\xbc\xd0\xb5\xd1\x82\xd1\x8b",
        "Gegenst\xc3\xa4nde hervorheben",
        "Gegenstand hervorheben",
        "Mettre les objets en surbrillance",
        "Mettre en surbrillance",
        "Objets en surbrillance",
        "Resaltar objetos",
        "Resaltar objeto",
        "Destacar Itens",
        "Destacar Item",
        "\xe3\x82\xa2\xe3\x82\xa4\xe3\x83\x86\xe3\x83\xa0\xe3\x82\x92\xe3\x83\x8f\xe3\x82\xa4\xe3\x83\xa9\xe3\x82\xa4\xe3\x83\x88",
        "\xe3\x82\xa2\xe3\x82\xa4\xe3\x83\x86\xe3\x83\xa0\xe3\x81\xae\xe3\x83\x8f\xe3\x82\xa4\xe3\x83\xa9\xe3\x82\xa4\xe3\x83\x88",
        "\xe0\xb9\x80\xe0\xb8\x99\xe0\xb9\x89\xe0\xb8\x99\xe0\xb9\x84\xe0\xb8\xad\xe0\xb9\x80\xe0\xb8\x97\xe0\xb8\xa1",
    };
    for (const char* k : kAnchors) {
        if (IEqualsCI(s, k)) return true;
    }
    return false;
}

inline bool IsPlaceholderText(const std::string& raw) {
    const std::string s = TrimAscii(raw);
    static const char* const kPrefixes[] = {
        "Type keywords",
        "\xed\x82\xa4\xec\x9b\x8c\xeb\x93\x9c",
        "\xea\xb2\x80\xec\x83\x89\xec\x96\xb4",
        "\xec\x95\x84\xec\x9d\xb4\xed\x85\x9c\xec\x9d\x84 \xea\xb0\x95\xec\xa1\xb0",
        "\xec\x95\x84\xec\x9d\xb4\xed\x85\x9c \xea\xb0\x95\xec\xa1\xb0",
        "\xe9\x97\x9c\xe9\x8d\xb5\xe5\xad\x97",
        "\xe5\x85\xb3\xe9\x94\xae\xe5\xad\x97",
        "\xd0\x92\xd0\xb2\xd0\xb5\xd0\xb4\xd0\xb8\xd1\x82\xd0\xb5",
        "\xd0\xb2\xd0\xb2\xd0\xb5\xd0\xb4\xd0\xb8\xd1\x82\xd0\xb5",
        "Schlagw\xc3\xb6rt",
        "Schlagworte",
        "Saisir",
        "Saisissez",
        "Escribe",
        "Digite",
        "\xe3\x82\xad\xe3\x83\xbc\xe3\x83\xaf\xe3\x83\xbc\xe3\x83\x89",
        "\xe0\xb8\x84\xe0\xb8\xb3\xe0\xb8\x84\xe0\xb9\x89\xe0\xb8\x99\xe0\xb8\xab\xe0\xb8\xb2",
        "\xe0\xb8\x9e\xe0\xb8\xb4\xe0\xb8\xa1\xe0\xb8\x9e\xe0\xb9\x8c",
    };
    for (const char* p : kPrefixes) {
        if (IStartsWithCI(s, p)) return true;
    }
    static const char* const kSubstrings[] = {
        "mots-cl\xc3\xa9s",
        "mots-cles",
        "palabras clave",
        "palavras-chave",
        "\xd0\xba\xd0\xbb\xd1\x8e\xd1\x87\xd0\xb5\xd0\xb2\xd1\x8b\xd0\xb5 \xd1\x81\xd0\xbb\xd0\xbe\xd0\xb2\xd0\xb0",
        "\xe9\x97\x9c\xe9\x8d\xb5\xe5\xad\x97",
        "\xe5\x85\xb3\xe9\x94\xae\xe5\xad\x97",
        "\xed\x82\xa4\xec\x9b\x8c\xeb\x93\x9c",
    };
    for (const char* sub : kSubstrings) {
        if (IContainsCI(s, sub)) return true;
    }
    return false;
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

    struct TextEl { std::string text; std::string sid; float x; float y; };
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
        std::string sid = ctx->Ui.GetStringId(addr);
        if ((!t.empty() && t.size() < 128) || !sid.empty()) {
            float x = 0.f, y = 0.f, w = 0.f, h = 0.f;
            ctx->Ui.ComputeScreenRect(addr, x, y, w, h);
            texts.push_back({std::move(t), std::move(sid), x, y});
        }
        for (const uintptr_t c : ctx->Ui.GetChildren(addr))
            if (c) stack.push_back({c, depth + 1});
    }

    float anchorX = -1.f, anchorY = -1.f;
    for (const auto& e : texts) {
        if (!e.text.empty() && IsHighlightAnchorText(e.text)) {
            anchorX = e.x;
            anchorY = e.y;
            break;
        }
    }
    if (anchorY < 0.f) {
        for (const auto& e : texts) {
            if (!e.sid.empty()) {
                if (IContainsCI(e.sid, "HighlightItems") || IContainsCI(e.sid, "item_highlight") || IContainsCI(e.sid, "stash_search")) {
                    anchorX = e.x;
                    anchorY = e.y;
                    break;
                }
            }
        }
    }
    if (anchorY < 0.f) return r;
    r.found = true;
    r.x = anchorX;
    r.y = anchorY;

    for (const auto& e : texts) {
        if (e.text.empty()) continue;
        if (IsHighlightAnchorText(e.text)) continue;
        if (IsPlaceholderText(e.text)) continue;
        if (e.x <= anchorX) continue;
        float dy = e.y - anchorY;
        if (dy < 0.f) dy = -dy;
        if (dy > 20.f) continue;
        r.filter = TrimAscii(e.text);
        break;
    }
    return r;
}

}
