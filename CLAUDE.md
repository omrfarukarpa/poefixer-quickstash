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

**Release gotchas (learned the hard way):**
- **Bump the version FIRST, then rebuild, THEN hash.** `kQuickStashVersion` is compiled into the DLL, so the version bump changes the binary — the SHA-256 you publish must come from the *post-bump* rebuild, not an earlier build.
- **Never pass release notes inline via `gh release create --notes "..."`.** The notes contain backticks/`$` and shell mangles them ("bad substitution", silently empty body). Write the notes to a file and use `gh release edit <tag> --notes-file <file>` (or `create --notes-file`). Always verify afterwards: `gh release view <tag> --json body --jq '.body'`.
- **Match the existing asset shape:** past releases attach the **bare `QuickStash.dll`**, not a zip (the README's `QuickStash-*.zip` wording is aspirational — don't silently switch formats).
- **Local deploy will fail while fixer is running** ("Device or resource busy"). That's expected; the release is the deliverable. The bare version-string bump makes the DLL functionally identical to the prior build, so a running fixer already has a working binary — copy the final DLL when fixer is next closed.

## Updating the vendored SDK (after a PoeFixer host update, e.g. "update 268")

The maintainer will occasionally say "PoeFixer updated, recheck the plugin." The authoritative SDK source is the **`POEFixer/ExamplePlugin`** GitHub repo (`sdk/PluginAbi.h`, `sdk/PluginSDK.h` at HEAD) — the SDK is **not** shipped inside the fixer install (only `.dll`s live under `<fixer>\Plugins\`). Pull the headers with `gh api repos/POEFixer/ExamplePlugin/contents/sdk/<file> --jq .content | base64 -d`.

**The single most important rule — build direction:** `HostCompatible()` is `abi->version == PLUGIN_SDK_VERSION && abi->size_bytes >= sizeof(HostAbi)`. The ABI grows **append-only** (v6 has been stable across many host updates; only tail fields/functions are added). Consequences:
- Building against an SDK **older-than-or-equal-to** the running host is **always safe** — the host's `size_bytes` is `>=` the plugin's smaller `sizeof(HostAbi)`. The *existing* released DLL therefore keeps working on newer hosts with no rebuild.
- Building against an SDK **newer than** the running host is the **only failure mode**: the plugin's `sizeof(HostAbi)` exceeds the host's `size_bytes`, `HostCompatible()` returns false, and the plugin **silently self-disables** (logs `Quick Stash: incompatible PoeFixer host`). So do **not** jump to bleeding-edge upstream unless you've confirmed the installed host is at least that new.
- **Confirming what SDK a host build maps to:** the internal timestamp of `Fixer.exe` inside the distribution zip (`unzip -l <fixer>\*.zip`) vs the ExamplePlugin repo commit dates; the maintainer ships host + all bundled plugins as a matched set, so the bundled plugins' build date ≈ the SDK generation the host expects. The only ground truth is the in-game log — after any SDK bump, tell the user to enable the plugin and confirm no `incompatible PoeFixer host` line appears; the fallback is to rebuild against an earlier SDK commit.

**Rechecking after an SDK bump — what actually needs verifying:** QuickStash holds **zero raw game-memory offsets**; it only consumes host-resolved structs (`Inventory` / `InventoryGrid`: `GridScreenX/Y`, `CellSize`, `TotalBoxesX/Y`, `Items`) and a handful of service calls (`Game.IsInGame`/`GetSnapshot`, `Inventory.GetAll`/`GetName`/`Scan`/`GetItems`, `Events.OnFrame`/`Unsubscribe`, `Overlay.SetWantsOverlayInput`, `Log.Info`/`Error`) plus the `Plugin` vtable. So host-side "offset moved to core" / "GameUI/RootUI offset fixed" changes need **no plugin code change** — the plugin transparently gets the corrected coordinates. To recheck: diff those specific structs/signatures old-vs-new (they should be byte-identical bar comments); if so it's a pure compatibility rebuild.

## Work in progress — Withdraw ("pick up highlighted items from stash")

**Goal (community request):** a reverse of Transfer — Ctrl+click items in the OPEN stash back INTO the inventory, filtered (a search box emulating PoE's stash "highlight"). User chose the *filtered withdraw* variant.

**Status as of last session (2026-07-08): DIAGNOSTIC PHASE ONLY — withdraw execution NOT yet written.** The released plugin is still **v1.1.1** (Transfer only). Local test builds add a read-only diagnostics panel but were **not** released or committed as a feature.

**Key findings (why this is non-trivial):**
- `InventoryService::GetAll()` enumerates **100+ inventories** at once: `MainInventory1`, every equipment slot (`BodyArmour1`, `Weapon1/2`, `Offhand1/2`, `Helm1`, `Amulet1`, `Ring1/2`, `Gloves1`, `Boots1`, `Belt1`, `Flask1`, `Trinket`), `Cursor1`, **and every stash tab you own**. Ids 1–13 are the fixed player slots; stash tabs are id 14+.
- Opening a stash makes **many** affinity tabs "active" at once (user's real ids: Abyss 148, Breach 149, Expedition 150, Fragment/Tablet 151, Ritual 152, Waystone 154, quad 156…). So **you cannot identify the visible tab by "on-screen valid grid"** — many report valid grids simultaneously.
- **Chosen approach: item-centric, NOT tab identification.** Every `InventoryItem` has `ScreenX/Y/W/H` + `ScreenValid`. The bet is that the host only assigns a real on-screen rect to items in the **currently visible** tab. So withdraw = collect items that are `ScreenValid` + on-screen from all inventories **except** player slots (see `IsPlayerSlotName`), then Ctrl+click each at its **own item screen center** (`ScreenX + ScreenW/2`, `ScreenY + ScreenH/2`). This also sidesteps special tabs whose grid-cell math doesn't map to click points.
- Detection **tracks the visible tab** (switching tabs changes it) — good sign. **Open question still being diagnosed:** some tabs the user can see items in report `OnScr=0`. Need the Total/ScreenValid/OnScreen breakdown per tab (the diagnostic now shows it, amber-highlighting `OnScr==0` rows) to learn whether those items lack `ScreenValid` or just fall outside the on-screen test.

**Code added so far (uncommitted WIP, compiles clean, diagnostic-only):**
- `ui/InventoryDiagnostics.h` — settings-panel inspector: display size, "On-screen withdrawable items: N (from M tabs)", and a per-tab Total/Valid/OnScr table. Wired into `DrawSettings` in `QuickStash.cpp`.
- `game/PanelDetector.h` — helpers `IsPlayerSlotName`, `GridOnScreen`, `ItemOnScreen`, and `FindOpenStash` (the last is grid-based and likely to be dropped in favour of the item-centric collection).

**Next steps when resuming:** (1) read the diagnostic breakdown to confirm the item-centric filter isolates the visible tab (is M==1? why do some visible tabs show OnScr=0?); (2) add withdraw settings (filter string matched against `BaseTypeName`/`UniqueName`, empty = all) + a "withdraw" button beside "transfer"; (3) build the withdraw click queue from on-screen filter-matching non-player items using item screen centers, and drive it through the EXISTING `TransferState` machine (generalise `Start` to accept a prebuilt queue + the click target coords, or add `StartWithdraw`) so all Ctrl-hold/fail-safe/watchdog guarantees carry over; (4) before shipping, verify in-game that it never Ctrl+clicks the player's own bag.
