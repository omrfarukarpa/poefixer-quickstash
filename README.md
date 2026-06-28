# Quick Stash

A [PoeFixer](https://github.com/POEFixer/PoeFixer) plugin for **Path of Exile 2** that adds a **Transfer** button on your inventory and moves items into whatever storage panel you have open (stash, vendor, trade, gamble, etc.) using Ctrl+click — similar to ExileCore’s Highlighted Items quick-stash flow.

## Features

- **Transfer button** — Appears above your backpack grid whenever the main inventory is open.
- **One-click dump** — Ctrl+clicks every non-excluded occupied slot into the panel on the other side of the trade (stash tab, shop, player trade, etc.).
- **Exclusion grid** — Click cells in settings to skip slots (weapon column excluded by default). Presets: weapon column only, clear all, select all.
- **Timing controls** — Click delay, post-click delay, cursor settle, and hold Ctrl after the last click for reliable transfers.
- **Safety options** — Cancel on right-click; stop if inventory closes mid-transfer.
- **Button position** — Optional X/Y offset sliders; default placement is built in (offsets `0` / `0` = standard spot above the grid).

<img width="1135" height="697" alt="image" src="https://github.com/user-attachments/assets/6bc10c68-94ff-469c-a25f-4bae9ed31d2f" />

<img width="672" height="1179" alt="image" src="https://github.com/user-attachments/assets/29429aa9-5f4c-4bae-8754-33daa5018649" />


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
