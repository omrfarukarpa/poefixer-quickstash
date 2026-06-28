# Quick Stash

**Version 1.1.0** — a hardened fork of the original Quick Stash plugin,
re-worked and maintained by **Ömer Faruk ARPA**.

A [PoeFixer](https://github.com/POEFixer/PoeFixer) plugin for **Path of Exile 2** that adds a **Transfer** button on your inventory and moves items into whatever storage panel you have open (stash, vendor, trade, gamble, etc.) using Ctrl+click — similar to ExileCore’s Highlighted Items quick-stash flow.

> **About this fork.** This is a community fork derived from the original
> Quick Stash plugin (original author unknown / unlinked). Version 1.1.0 adds
> substantial reliability and safety fixes over the original 1.0 — see
> [Changes in 1.1.0](#changes-in-110). Fork maintained by Ömer Faruk ARPA.

## Features

- **Transfer button** — Appears above your backpack grid whenever the main inventory is open.
- **One-click dump** — Ctrl+clicks every non-excluded occupied slot into the panel on the other side of the trade (stash tab, shop, player trade, etc.).
- **Exclusion grid** — Click cells in settings to skip slots (weapon column excluded by default). Presets: weapon column only, clear all, select all.
- **Timing controls** — Click delay, post-click delay, cursor settle, and hold Ctrl after the last click for reliable transfers.
- **Safety options** — Cancel on right-click; stop if inventory closes mid-transfer.
- **Button position** — Optional X/Y offset sliders; default placement is built in (offsets `0` / `0` = standard spot above the grid).

### Settings panel

![Quick Stash settings panel](docs/1.png)

### Transfer button in-game

![The transfer button above the inventory grid](docs/2.png)


## Requirements

- [PoeFixer](https://github.com/POEFixer/PoeFixer) with plugin SDK v6 support
- Path of Exile 2 (Windows)
- PoeFixer’s built-in auto-stash should be **disabled** if you use this plugin, to avoid conflicting Ctrl+click behavior.

## Install (release build)

Pre-built binaries are published on GitHub:

1. On GitHub, open the **Releases** section for this repository and download the latest **`QuickStash-*.zip`**.
3. Extract the archive. You should get a `QuickStash` folder containing at least:
   - `QuickStash.dll`
4. Copy that folder into your PoeFixer install:

   ```
   <PoeFixer>\Plugins\QuickStash\
   ```

   Example:

   ```
   C:\Games\PoeFixer\Plugins\QuickStash\QuickStash.dll
   ```

5. Start (or restart) PoeFixer and enable **Quick Stash** under **Plugins**.
6. Open the plugin settings to adjust exclusions and timings. Settings are saved to:

   ```
   Plugins\QuickStash\config\settings.json
   ```

## Usage

1. Open your character inventory.
2. Open the target inventory (Stash, Shop, Trade...).
3. Click **transfer** on the button above the grid.
4. Items move in slot order; excluded cells and empty slots are skipped.

Right-click during a transfer cancels it (if enabled in settings).

## Build from source

**Requirements:** Visual Studio 2022 (MSVC v143), Windows SDK 10.0, C++20

```powershell
& "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe" `
  QuickStash.sln -p:Configuration=Release -p:Platform=x64
```

Output: `bin\Release\QuickStash.dll`

Copy the DLL to `Plugins\QuickStash\` in your PoeFixer directory, same as the release install step.

## Project layout

```
QuickStash.cpp              Plugin entry (lifecycle, settings UI, overlay)
config/Settings.h           JSON settings (delays, exclusions, offsets)
game/                       Inventory detection, transfer queue, click planner
input/Win32Input.h          Cursor move + Ctrl+LMB via SendInput
overlay/TransferButtonOverlay.h   Transfer button UI + click handling
ui/ExclusionGrid.h          12×5 exclusion editor in settings
sdk/                        PoeFixer Plugin SDK headers
```

## Changes in 1.1.0

This fork hardens the original 1.0 with reliability and safety fixes:

- **No more render-thread stalls.** The transfer is now a non-blocking,
  frame-paced state machine — the old version slept on the render thread on
  every click, hitching the game and overlay. Full framerate during transfers.
- **Crash-safe settings.** A corrupt or hand-edited `settings.json` no longer
  takes down the host; load/save never let an exception cross the C ABI.
- **Reliable Ctrl+click.** Ctrl is injected by scan code (PoE2 reads raw input)
  so items transfer instead of being picked onto the cursor.
- **DPI-correct targeting.** Cursor moves use absolute virtual-desktop
  coordinates, so clicks land correctly on non-100% scaling / multi-monitor.
- **Ctrl can't get stuck.** Fail-safe release via destructor, foreground-abort
  (alt-tab cancels), and a watchdog timeout.
- **Live click coordinates** (handles a panel that moves/scrolls mid-transfer),
  **cursor restore** after a transfer, an **exclusion-grid colour legend**, a
  collapsible **How to use** guide, and an empty-queue notice.

## Disclaimer

This is an unofficial, third-party plugin. It reads game memory and injects
synthetic input. Use of third-party tools may violate the Path of Exile Terms
of Service and could put your account at risk. **Use at your own risk.** This
project is provided for educational purposes and is not affiliated with,
endorsed by, or supported by Grinding Gear Games or the PoeFixer authors.
PoeFixer and Path of Exile are trademarks of their respective owners.

## License

This project's own source code is licensed under the **GNU General Public
License v3.0** — see [LICENSE](LICENSE).

Bundled third-party components keep their own licenses:

- **Dear ImGui** (`imgui/`) — MIT License
- **nlohmann/json** (`third_party/json.hpp`) — MIT License
- **PoeFixer Plugin SDK** (`sdk/PluginAbi.h`, `sdk/PluginSDK.h`) — provided by
  the PoeFixer project; included here so the plugin can be built. These headers
  are the property of the PoeFixer authors and are redistributed only for
  build convenience. If the PoeFixer project objects, they will be removed and
  you will need to supply them yourself from your PoeFixer install.

MIT is GPLv3-compatible, so the combined work may be distributed under GPLv3.
