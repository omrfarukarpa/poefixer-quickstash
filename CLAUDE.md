# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Quick Stash is a **C++20 plugin DLL** for **PoeFixer** (an ExileCore-style host for Path of Exile 2). It adds a "Transfer" button over the inventory that Ctrl+click-dumps every non-excluded item into whatever storage panel is open. This is **v1.1.0, a hardened fork** of the original 1.0 (maintainer: Ömer Faruk ARPA, GitHub `omrfarukarpa`). It is an unofficial third-party game tool.

## Build

No `cl`/`msbuild` on PATH; invoke MSBuild by full path (VS 18 Enterprise is installed):

```bash
"/c/Program Files/Microsoft Visual Studio/18/Enterprise/MSBuild/Current/Bin/MSBuild.exe" \
  QuickStash.sln -p:Configuration=Release -p:Platform=x64 -nologo -v:minimal -m
```

- Output: `bin/Release/QuickStash.dll`. Clean rebuild: add `-t:Rebuild`.
- If the VS path changes, find it with `vswhere`: write a `.ps1` and run via `pwsh -NoProfile -File <file>` (the PowerShell tool is blocked by a hook on this machine — use the Bash tool with `pwsh`, or write `.ps1` files).
- There are **no tests** and no linter. "Verify" means: build clean (zero warnings from the plugin's own files; ignore `imgui/` noise), then adversarially review the diff against the real code (a compiler is the only thing that catches Windows-macro interactions like the `std::max` vs `<Windows.h>` `max()` clash — `NOMINMAX` is NOT defined).
- To deploy locally, copy the DLL to `D:\POE2\fixer\Plugins\QuickStash\QuickStash.dll`. **This fails with "Device or resource busy" while fixer is running** — fixer must be closed to overwrite its loaded DLL.

## Architecture & the constraints that shape it

The plugin's own code is small (~8 headers + `QuickStash.cpp`). `imgui/` and `third_party/json.hpp` are vendored — **never review or modify them**. `sdk/PluginAbi.h` (C ABI v6) and `sdk/PluginSDK.h` (C++ wrappers) are the **authoritative host contract** — read them to check conformance, but do not propose changing them (they belong to PoeFixer; they're redistributed here only so the plugin builds).

**The single most important fact: `DrawUI()`, `DrawSettings()`, and the `OnFrame` event callback all run on the host's RENDER thread, synchronously and inline.** The SDK's `EventsService::Subscribe` installs a captureless trampoline that calls the `std::function` directly — there is no host-side queue or thread hop. Therefore **anything blocking on this thread (e.g. `Sleep()`) freezes the game and overlay.** This is why the transfer is a non-blocking, frame-paced state machine, not a loop with sleeps.

**Two hard rules that come from being a DLL loaded over a C ABI:**
1. **No C++ exception may escape `OnEnable`/`OnDisable`/callbacks across the `extern "C"` boundary** — it's UB and crashes the whole host. This is why `Settings::Load`/`Save` are fully wrapped in `try/catch(...)` and use `nlohmann::json::parse(in, nullptr, false)` (non-throwing). Build is `ExceptionHandling=Sync`.
2. **`HostCompatible()` must be checked at the top of `OnEnable`** — on an ABI version/size mismatch `m_ctx` is unpopulated (all service pointers null). The SDK wrappers null-check their function pointers, so `ctx()->Log` is safe even then.

**Control flow (`QuickStash.cpp`):**
- `OnFrameTick` (the `OnFrame` subscriber) drives the running transfer via `m_transfer.Tick(ctx, live)`. `DrawUI` only draws the button and starts transfers; it early-returns while a transfer runs.
- `RefreshInventoryIfNeeded()` rescans at most every 150 ms and caches `m_backpack`. This cache is passed into `Tick` as the `live` inventory so the state machine never re-enumerates and recomputes click coordinates from the **live** grid (handles a panel that moves/scrolls mid-transfer).
- A running transfer is aborted if the game loses foreground (alt-tab) — this check sits in the `IsRunning()` branch *before* `Tick`, so losing focus can't keep Ctrl held.

**The transfer state machine (`game/TransferState.h`):** per-click sub-states `Spacing → Settling → PostClick`, each gated on a `steady_clock` deadline (mirrors the existing `m_finishAt` mechanism). `Ctrl` is held DOWN for the entire multi-second run (correct Ctrl+click semantics) and released via three independent fail-safes: the destructor (`~TransferState` sends `CtrlUp` if held — covers force-unload without `OnDisable`), the foreground-abort, and a watchdog timeout (`WatchdogBudgetMs`). When editing this file, **never reintroduce a blocking wait in `Tick`.**

**Input (`input/Win32Input.h`):** Ctrl is injected by **scan code** (`KEYEVENTF_SCANCODE`) because PoE2 reads raw input and ignores VK-only injection (it would turn a Ctrl+click into a plain pickup). Cursor moves use **absolute virtual-desktop `SendInput`** (not `SetCursorPos`) so targeting is correct regardless of the process's DPI-awareness.

**Settings (`config/Settings.h`):** timing bounds are `k*MinMs`/`k*MaxMs` constants — the single source of truth shared by both the `DrawSettings` sliders and the `Load` clamps. Adjust both together by editing the constant.

## Turkish / encoding note

UI strings are **English** (the SDK requires `GetName()` to be ASCII, and the plugin's whole UI is English — keep it consistent). The default ImGui font atlas only covers basic Latin, so **strings shown in-game via ImGui must stay ASCII** (e.g. the maintainer name is `"Omer Faruk ARPA"` in `QuickStash.cpp`). Documents (README, this file) use full Turkish characters (`Ömer Faruk ARPA`) — no font constraint there.

## Publishing workflow

The repo is public at `https://github.com/omrfarukarpa/poefixer-quickstash`, licensed GPLv3, `origin` is configured. **When the user says "yayınla" / "publish", do this without re-asking:**
1. Build Release, confirm it's clean.
2. Commit with author `Ömer Faruk ARPA <omrfarukarpa@gmail.com>` (repo-local git identity is already set to this — verify before committing).
3. `git push` to `origin main`.
4. For a new version: bump the version in `README.md` and `kQuickStashVersion` in `QuickStash.cpp` together, then create a GitHub Release with `gh` and **attach `bin/Release/QuickStash.dll` as an asset** (the DLL is gitignored; it ships via Releases, not the repo). Include its SHA-256 in the notes.

`gh` is installed at `"/c/Program Files/GitHub CLI/gh.exe"` (logged in as `omrfarukarpa`). `.gitignore` excludes `bin/`, `obj/`, `*.user` — keep build artifacts out of the repo.
