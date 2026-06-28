#pragma once

#include <Windows.h>

namespace QuickStashInput {

// Move the cursor to a screen pixel. We use SendInput with absolute,
// virtual-desktop-normalised coordinates rather than SetCursorPos.
//
// Why: the grid coordinates come from the host snapshot in the host's
// (DPI-aware) coordinate space. If this DLL's process is NOT per-monitor
// DPI-aware, SetCursorPos applies DPI virtualization and the cursor lands
// off-target on non-100% scaling or multi-monitor setups. SendInput with
// MOUSEEVENTF_ABSOLUTE maps 0..65535 across the whole virtual desktop and is
// not subject to that per-call virtualization, so targeting stays correct
// regardless of the process's DPI-awareness mode.
inline void MoveCursorScreen(int x, int y) {
    const int vsX = GetSystemMetrics(SM_XVIRTUALSCREEN);
    const int vsY = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int vsW = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int vsH = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    if (vsW <= 0) vsW = 1;
    if (vsH <= 0) vsH = 1;

    // Normalise to 0..65535 across the virtual desktop. The +half-span /span
    // rounding maps a pixel to the centre of its normalised cell.
    const LONG nx = static_cast<LONG>(
        ((static_cast<long long>(x - vsX) * 65535) + (vsW - 1) / 2) / (vsW - 1 > 0 ? vsW - 1 : 1));
    const LONG ny = static_cast<LONG>(
        ((static_cast<long long>(y - vsY) * 65535) + (vsH - 1) / 2) / (vsH - 1 > 0 ? vsH - 1 : 1));

    INPUT in{};
    in.type = INPUT_MOUSE;
    in.mi.dx = nx;
    in.mi.dy = ny;
    in.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;
    SendInput(1, &in, sizeof(INPUT));
}

// Current cursor position in screen pixels. Returns false if unavailable
// (so the caller can skip a later restore rather than warp to {0,0}).
inline bool GetCursorScreen(int& x, int& y) {
    POINT pt{};
    if (!GetCursorPos(&pt))
        return false;
    x = pt.x;
    y = pt.y;
    return true;
}

// PoE2 reads keyboard state via DirectInput/raw input, which keys off the
// hardware SCAN CODE, not the virtual-key. Injecting VK_CONTROL alone is
// frequently ignored by the game, turning an intended Ctrl+click into a plain
// click (the item gets picked up onto the cursor instead of transferred). We
// therefore send the scan code (with the VK still set for well-behaved
// consumers) and flag KEYEVENTF_SCANCODE.
inline WORD CtrlScanCode() {
    // The VK->scan mapping is fixed for the session; compute it once.
    static const WORD scan = static_cast<WORD>(MapVirtualKey(VK_CONTROL, MAPVK_VK_TO_VSC));
    return scan;
}

inline void CtrlDown() {
    INPUT in{};
    in.type = INPUT_KEYBOARD;
    in.ki.wVk = VK_CONTROL;
    in.ki.wScan = CtrlScanCode();
    in.ki.dwFlags = KEYEVENTF_SCANCODE;
    SendInput(1, &in, sizeof(INPUT));
}

inline void CtrlUp() {
    INPUT in{};
    in.type = INPUT_KEYBOARD;
    in.ki.wVk = VK_CONTROL;
    in.ki.wScan = CtrlScanCode();
    in.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    SendInput(1, &in, sizeof(INPUT));
}

inline void LeftClickAtCursor() {
    INPUT inputs[2]{};
    inputs[0].type = INPUT_MOUSE;
    inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    inputs[1].type = INPUT_MOUSE;
    inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(2, inputs, sizeof(INPUT));
}

inline bool IsRightMouseDown() {
    return (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
}

} // namespace QuickStashInput
