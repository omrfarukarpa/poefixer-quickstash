# Quick Stash codebase map

- QuickStash.cpp: plugin lifecycle, settings, overlay and OnFrame transfer driver; current version 1.4.0.
- game/PanelDetector.h: backpack and stash detection; shared guild inventory id 10002 helper.
- game/WithdrawPlanner.h: visible stash selection, item rectangles, cached mod text and TAKE filtering.
- game/PoeHighlight.h: reads the native Highlight Items field.
- game/TransferState.h: shared Transfer/TAKE initialization, Ctrl settling, non-blocking click phases, cancellation and cursor restoration.
- input/Win32Input.h: SendInput and game-client to screen-coordinate conversion.
- overlay/TransferButtonOverlay.h: Transfer and TAKE G(N) labels; overlay-only hardware fallback is gated by the caller.
- ui/InventoryDiagnostics.h: inventory inspector with guild label.
- config/Settings.h: persisted timing, exclusions, filtering and cancellation preferences.
- sdk/: unchanged PoeFixer C ABI v6 headers, authoritative upstream POEFixer/ExamplePlugin. Guild publishing is a host data contract, not a new ABI dependency.
- Build: QuickStash.sln, VS 18 Enterprise MSBuild, Release/x64, C++20/v145. Output bin/Release/QuickStash.dll.
- Local deployment: D:/POE2/fixer/Plugins/QuickStash/QuickStash.dll; close fixer before copying.
- Runtime: backpack scan cached for 150 ms; highlight-field reads every 150 ms; withdraw candidate evaluation currently runs per DrawUI frame. Foreground checks are O(1).
