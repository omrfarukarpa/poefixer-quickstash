# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Quick Stash is a **C++20 plugin DLL** for **PoeFixer** (an ExileCore-style host for Path of Exile 2). It adds a "Transfer" button that Ctrl+click-dumps every non-excluded item into whatever storage panel is open, plus a **"TAKE" (withdraw)** flow that Ctrl+clicks matching items out of the open stash back into the inventory (filtered by reading PoE's native "Highlight Items" box via the UI tree). This is **v1.2.0** — no longer framed as a fork: it began from the original QuickStash 1.0 concept but is now effectively its own implementation (maintainer: Ömer Faruk ARPA, GitHub `omrfarukarpa`). It is an unofficial third-party game tool.

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

**Rechecking after an SDK bump — what actually needs verifying:** QuickStash holds **zero raw game-memory offsets**; it only consumes host-resolved structs (`Inventory` / `InventoryGrid`: `GridScreenX/Y`, `CellSize`, `TotalBoxesX/Y`, `Items` incl. per-item `ScreenX/Y/W/H`/`ScreenValid`/`StackCount`) and these service calls plus the `Plugin` vtable:
- `Game.IsInGame`/`GetSnapshot`; `Events.OnFrame`/`Unsubscribe`; `Overlay.SetWantsOverlayInput`; `Log.Info`/`Warn`/`Error`.
- `Inventory.GetAll`/`GetName`/`Scan`/`GetItems`; and for withdraw, `Inventory.ReadItemMods`/`FormatStat`/`ReadItemBaseTypeName` (mod path currently gated off — see the crash note).
- **`Ui` service** (added for withdraw): `GetGameUiRoot`, `GetChildren`, `GetText`, `GetStringId`, `ComputeScreenRect` — used by `ReadPoeHighlight` to read PoE's "Highlight Items" box. If a host update reshuffles the UI tree, the `"Highlight Items"` anchor lookup is the thing to re-verify (use the Debug-mode UI-tree dumper).

So host-side "offset moved to core" / coordinate-fix changes need **no plugin code change**. To recheck: diff those specific structs/signatures old-vs-new (byte-identical bar comments = pure compatibility rebuild).

## Withdraw / TAKE (shipped in v1.2.0)

**What it is:** the reverse of Transfer — Ctrl+click items OUT of the open stash tab back INTO the inventory, filtered live. Shipped in v1.2.0 (release `v1.2.0`, commit `98a2084`). A **TAKE (N)** button sits just above PoE's own "Highlight Items" box and drives the same `TransferState` engine as Transfer (all Ctrl-hold / watchdog / foreground-abort / cursor-restore guarantees shared).

**The filter is Path of Exile's OWN "Highlight Items" box — the plugin has NO text input of its own, by necessity.** `Overlay.SetWantsOverlayInput(true)` routes only the MOUSE to the overlay, NOT the keyboard, so an in-game ImGui `InputText` never receives typed characters (this was built, couldn't be typed into, and removed). Instead `game/PoeHighlight.h::ReadPoeHighlight` walks the host UI tree (`ctx->Ui`: `GetGameUiRoot` → `GetChildren` / `GetText` / `GetStringId` / `ComputeScreenRect`), finds the element whose text is `"Highlight Items"` (a stable anchor), and reads the text element on the same screen row just to its right as the typed filter. That anchor's rect also positions the TAKE button. This UI walk is throttled to 150 ms.

**Filter matching (`game/WithdrawPlanner.h`):** `BuildItemSearchText` builds a per-item haystack = `BaseTypeName` + `UniqueName` + `Path`; `ContainsCI` is case-insensitive substring; empty filter = take everything on the tab.
- **Mod-text matching is GATED OFF** behind `g_withdrawReadMods` (false). The SDK CAN read mods — `ctx->Inventory.ReadItemMods(entityAddr)` → `ItemMods` (each `Mod` has `AffixName`/`StatKey`/`Value0/1`), and `FormatStat(StatKey, v0, v1)` renders the in-game stat line — which would let "fire" match a Ruby Ring's fire-res mod. **BUT calling `ReadItemMods(item.Address)` CRASHED the host** (uncatchable access violation): `InventoryItemAbi.address` is apparently NOT the entity address the mod reader wants for every item. Re-enable only after verifying the address with the low-risk `ReadItemBaseTypeName(item.Address)` (right name back = safe), and guard no-mod items.

**Coordinate resolution — `ResolveItemRect` (ORDER MATTERS):** per item, PER-ITEM screen rect FIRST (`ScreenValid` + on-screen → `ScreenX/Y/W/H`), grid math ONLY as fallback (`GridScreenX/Y + slot*CellSize`), and the grid branch is further gated by `GridLayoutPlausible`. Getting the order backwards sprayed a special tab's highlights as a stretched off-screen band. Why: normal grid tabs report `ScreenValid=0` but a valid uniform on-screen grid → grid math is correct; special/affinity tabs (currency, gem/skill) expose real per-item rects, but their *logical* grid (currency 53×4; the skill tab is a flat wide row) does NOT match the visual layout, so grid math sprays boxes across the screen. `GridLayoutPlausible` rejects flat/wide grids (`TotalBoxesY < 6`, or grid wider than the display) so they use per-item rects; a special tab with neither usable rects nor a plausible grid has its items skipped (nothing drawn) rather than mis-placed.

**Key findings (why this is non-trivial):**
- `InventoryService::GetAll()` enumerates **100+ inventories** at once: `MainInventory1`, every equipment slot (`BodyArmour1`, `Weapon1/2`, `Offhand1/2`, `Helm1`, `Amulet1`, `Ring1/2`, `Gloves1`, `Boots1`, `Belt1`, `Flask1`, `Trinket`, `Charm*`), `Cursor1`, **and every stash tab you own**. Ids 1–13 are fixed player slots; stash tabs are id 14+. This is why detection must exclude player slots by name AND by tiny grid size (see `FindOpenStashAny`).
- **Visible-tab isolation is clean:** with one tab open, only that tab reports on-screen (grid-on-screen OR on-screen items), so the item-/grid-centric detection picks it out — the "many affinity tabs active at once" fear did not reproduce for non-empty tabs. Keep verifying if multiple non-empty tabs could ever be laid out simultaneously.
- **The TabletHelper (ExileCore2) approach does NOT transfer.** ExileCore2 reads UI-element colors / walks `StashElement.IndexVisibleStash`; PoeFixer's model is different (host-resolved item rects + a separate UI-tree API). Don't port ExileCore2 stash code directly.

**Open-tab detection & safety — `FindOpenStashAny`:** picks the single on-screen, non-player, non-empty inventory (most items wins). Excludes the backpack, every `IsPlayerSlotName` prefix (now incl. `Charm`), and any inventory whose grid area is `< 10` boxes — so charm/flask/equipment slots can never be a withdraw source and TAKE can never click the player's own gear. (Charms slipped through name-only exclusion once; the min-area guard is the backstop.)

**Execution & UI (`QuickStash.cpp`):** `UpdateWithdraw` runs EVERY frame (no throttle) so highlights track the live tab with zero lag and no ghost boxes on tab-switch; only the PoE-box read (150 ms) is throttled. `DrawWithdrawHighlights` outlines matches in cyan. `DrawWithdrawButtonAt` shows **TAKE (N)** = number of Ctrl+clicks (stacks); stacked currency also draws `x{total}` (sum of `StackCount`). `StartWithdraw` feeds the matched rect centres as absolute `ScreenPoint`s into `TransferState::StartWithdraw` — withdraw points are FIXED at launch (the stash panel is static during the click burst), unlike Transfer which recomputes from the live backpack grid each Tick.

**Code map (all shipped):**
- `game/PoeHighlight.h` — `ReadPoeHighlight` (UI-tree walk → box text + anchor rect) + `IEqualsCI`/`IStartsWithCI`.
- `game/WithdrawPlanner.h` — `ContainsCI`, `BuildItemSearchText` (+ gated `g_withdrawReadMods` mod path), `ResolveItemRect`, `GridLayoutPlausible`, `TabOnScreen`, `FindOpenStashAny`, `CollectCandidates` (builds per-item search text once per tab), `FilterCandidates` (→ `WithdrawSelection{rects,totalQty}`), `RectCenter`, `ScreenRect`, `WithdrawCandidate`.
- `game/TransferState.h` — `StartWithdraw(...)`, `IsWithdrawMode()`, `m_useScreenPoints`/`m_screenQueue`, `ActiveQueueSize()`.
- `game/TransferPlanner.h` — `ScreenPoint` (+ existing `SlotCenterX/Y`, `BuildClickQueue`).
- `overlay/TransferButtonOverlay.h` — `DrawOverlayButton` shared body; `DrawWithdrawButtonAt(pos, count)`; `DrawTransferProgress(verb)`.
- `config/Settings.h` — `debugMode` (persisted). The old `withdrawFilter` setting was REMOVED (PoE's box is the only filter).
- `ui/InventoryDiagnostics.h` — `DrawInventoryDiagnostics` + `DrawUiTreeDiagnostics`, both behind the **Debug mode** checkbox (default off → no scan overhead in normal play). The UI-tree dumper is how the "Highlight Items" element was located.

**Open follow-up:** re-enable mod-text matching (the one parity gap vs PoE's own highlight, which matches mods). Blocked on the `ReadItemMods` entity-address crash above — verify the address first; do NOT just retry `item.Address`.
