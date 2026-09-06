# Project memory

## 2026-09-06 — Guild stash support, published v1.4.0

- Integrated guild detection, TAKE G(N), diagnostics and windowed-client click fixes. Published source commit 7c5f228 and release v1.4.0 with the bare QuickStash.dll asset after explicit publication authorization.
- Verified reserved guild inventory id 10002 against POEFixer/ExamplePlugin sdk/PluginAbi.h. Existing vendored SDK remains unchanged; host ABI compatibility requirements remain those of v1.3.2. Guild TAKE requires the GuildStash1 publisher introduced in PoeFixer v301, per the supplied host release notes.
- Excluded guild inventories from backpack dimension fallback. Small guild special tabs bypass the non-storage size heuristic, but still require visible content.
- Transfer and TAKE share BeginRun; clicks convert client coordinates through the game window. Ctrl settling is measured after CtrlDown. Hardware fallback uses valid ImGui mouse coordinates only in overlay mode.
- Foreground checks use Game.IsForeground rather than per-frame world snapshots.
- Final Release/x64 clean rebuild succeeded without compiler warnings/errors. Automated tests were not run per user preference. In-game verification remains for the maintainer. The final DLL was installed locally; the published asset hash matches the build and the download URL returned HTTP 200.
- Existing limitations: physical mouse-button mapping is unchanged; with panel-close cancellation disabled, a missing backpack can wait until the watchdog expires.

- Published release: https://github.com/omrfarukarpa/poefixer-quickstash/releases/tag/v1.4.0
