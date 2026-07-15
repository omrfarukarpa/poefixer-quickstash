#pragma once

#include "../config/Settings.h"
#include "sdk/PluginSDK.h"

#include <Windows.h>
#include <imgui.h>

namespace QuickStashOverlay {

inline constexpr float kButtonW = 88.f;
inline constexpr float kButtonH = 24.f;
// Default screen position above grid top-left; settings offsets are relative to this.
inline constexpr float kButtonBaseOffsetX = 0.f;
inline constexpr float kButtonBaseOffsetY = -40.f;

struct TransferButtonResult {
    bool clicked = false;
    bool clickedRelease = false;
    bool hovered = false;
    bool held = false;
    bool beginOk = false;
    bool wantCaptureMouse = false;
    ImVec2 mousePos{};
    ImVec2 btnP0{};
    ImVec2 btnP1{};
};

inline bool TransferButtonActivated(const TransferButtonResult& r) {
    return r.clicked || r.clickedRelease;
}

inline bool HitRect(const ImVec2& p, ImVec2 p0, ImVec2 p1) {
    return p.x >= p0.x && p.x < p1.x && p.y >= p0.y && p.y < p1.y;
}

// ImGui often never sees LMB on the button (game eats it). Hardware edge still works.
// Fires on the RELEASE edge only: a single real click is one press + one
// release, and returning true on both edges (pressed || released) double-fired
// the activation. Release-only also avoids triggering while the button is held.
// One tracker instance per button; Update must run every frame the button
// exists so the press/release edge state never goes stale.
struct HardwareClick {
    bool wasDown = false;
    bool pressedOnRect = false;

    bool Update(bool rectActive, ImVec2 p0, ImVec2 p1) {
        const bool down = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

        POINT pt{};
        const bool haveCursor = GetCursorPos(&pt) != 0;
        const ImVec2 mouse(static_cast<float>(pt.x), static_cast<float>(pt.y));
        const bool overRect = rectActive && haveCursor && HitRect(mouse, p0, p1);

        if (down && !wasDown)
            pressedOnRect = overRect;   // press edge: remember where it started
        const bool released = !down && wasDown;
        const bool clicked = released && pressedOnRect && overRect;
        if (released)
            pressedOnRect = false;
        wasDown = down;
        return clicked;
    }
};

inline ImVec2 TransferButtonPos(const PluginSDK::Inventory& inv,
                                const QuickStashConfig::Settings& settings) {
    return ImVec2(inv.Grid.GridScreenX + kButtonBaseOffsetX + settings.buttonOffsetX,
                  inv.Grid.GridScreenY + kButtonBaseOffsetY + settings.buttonOffsetY);
}

inline ImVec2 TransferButtonMax(const ImVec2& p0) {
    return ImVec2(p0.x + kButtonW, p0.y + kButtonH);
}

inline bool IsMouseOverTransferButton(const PluginSDK::Inventory& inv,
                                      const QuickStashConfig::Settings& settings) {
    const ImVec2 p0 = TransferButtonPos(inv, settings);
    return HitRect(ImGui::GetIO().MousePos, p0, TransferButtonMax(p0));
}

inline TransferButtonResult DrawOverlayButton(const ImVec2& pos,
                                              const char* label,
                                              const char* windowId) {
    TransferButtonResult r;
    const ImVec2 size(kButtonW, kButtonH);
    r.btnP0 = pos;
    r.btnP1 = TransferButtonMax(pos);
    r.mousePos = ImGui::GetIO().MousePos;

    ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(size, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.f);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.5f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.125f, 0.10f, 0.07f, 0.92f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.16f, 0.13f, 0.09f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.09f, 0.07f, 0.05f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.47f, 0.38f, 0.20f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.80f, 0.71f, 0.43f, 1.f));

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
                                 | ImGuiWindowFlags_NoBackground
                                 | ImGuiWindowFlags_NoSavedSettings
                                 | ImGuiWindowFlags_NoFocusOnAppearing
                                 | ImGuiWindowFlags_NoBringToFrontOnFocus
                                 | ImGuiWindowFlags_NoNav;

    r.beginOk = ImGui::Begin(windowId, nullptr, flags);
    if (r.beginOk) {
        r.clicked = ImGui::Button(label, size);  // ImGui's own click already requires press+release on the button
        r.hovered = ImGui::IsItemHovered();
        r.held = ImGui::IsItemActive();
        // No release-only fallback here: it fired on any release-while-hovered
        // even when the press began off the button (e.g. a drag from the stash
        // panel just above). The hardware-edge tracker (HardwareClick, with a
        // press-origin check) covers the case where the game eats the click and
        // ImGui never sees it.
    }
    ImGui::End();

    ImGui::PopStyleColor(7);
    ImGui::PopStyleVar(5);

    r.wantCaptureMouse = ImGui::GetIO().WantCaptureMouse;
    return r;
}

inline TransferButtonResult DrawTransferButton(const PluginSDK::Inventory& inv,
                                               const QuickStashConfig::Settings& settings) {
    return DrawOverlayButton(TransferButtonPos(inv, settings), "transfer",
                             "##quick_stash_transfer");
}

inline TransferButtonResult DrawWithdrawButtonAt(const ImVec2& pos, int count) {
    char label[32];
    snprintf(label, sizeof(label), "TAKE (%d)", count);
    return DrawOverlayButton(pos, label, "##quick_stash_withdraw");
}

inline void DrawTransferProgress(int index, int total, const char* verb = "Transferring") {
    if (total <= 0) return;
    char buf[64];
    snprintf(buf, sizeof(buf), "%s: %d / %d", verb, index, total);
    ImGui::GetForegroundDrawList()->AddText(ImVec2(12.f, 12.f), IM_COL32(205, 180, 110, 255), buf);
}

} // namespace QuickStashOverlay
