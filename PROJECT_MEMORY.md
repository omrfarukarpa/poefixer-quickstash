# Project memory

## 2026-09-18 — Multi-language Highlight Items support, published v1.4.1

- Added multi-language recognition for the native "Highlight Items" search bar across all supported PoE client languages (Korean 아이템 강조하기, Traditional/Simplified Chinese, Russian, German, French, Spanish, Portuguese, Japanese, Thai).
- Added localized placeholder text filtering to ensure empty search box correctly selects all items without capturing help text as filter keywords.
- Added fallback UI string identifier checks (HighlightItems, item_highlight, stash_search).
- Clean rebuild Release/x64 succeeded with 0 errors/warnings.
- Published release: https://github.com/omrfarukarpa/poefixer-quickstash/releases/tag/v1.4.1

## 2026-09-06 — Guild stash support, published v1.4.0

- Integrated guild detection, TAKE G(N), diagnostics and windowed-client click fixes. Published source commit 7c5f228 and release v1.4.0 with the bare QuickStash.dll asset after explicit publication authorization.
- Verified reserved guild inventory id 10002 against POEFixer/ExamplePlugin sdk/PluginAbi.h. Existing vendored SDK remains unchanged; host ABI compatibility requirements remain those of v1.3.2. Guild TAKE requires the GuildStash1 publisher introduced in PoeFixer v301, per the supplied host release notes.
- Excluded guild inventories from backpack dimension fallback. Small guild special tabs bypass the non-storage size heuristic, but still require visible content.
- Transfer and TAKE share BeginRun; clicks convert client coordinates through the game window. Ctrl settling is measured after CtrlDown. Hardware fallback uses valid ImGui mouse coordinates only in overlay mode.
- Foreground checks use Game.IsForeground rather than per-frame world snapshots.
- Final Release/x64 clean rebuild succeeded without compiler warnings/errors. Automated tests were not run per user preference. In-game verification remains for the maintainer. The final DLL was installed locally; the published asset hash matches the build and the download URL returned HTTP 200.
- Existing limitations: physical mouse-button mapping is unchanged; with panel-close cancellation disabled, a missing backpack can wait until the watchdog expires.

- Published release: https://github.com/omrfarukarpa/poefixer-quickstash/releases/tag/v1.4.0

## Context refresh — 2026-09-06

- CODEBASE_MAP.md now documents the actual per-frame versus 150 ms work, client-coordinate queues, mod-cache limits, aggregate labels and current build commands. Local AGENTS.md contains operational rules instead of stale v1.2.0 descriptions.
- Heavy TAKE candidate work is still per eligible DrawUI frame; the 40-read mod budget is per CollectCandidates call. This is a performance follow-up, not a completed optimization.
- TAKE points are fixed at launch. The source guard checks that a stash remains detectable; live tab/layout changes during a run are not proven safe by that check alone.
- Published v1.4.0 remains unchanged. This documentation refresh did not run builds/tests or create commits/releases. Local DLL deployment must follow the current shared AGENTS.md close → copy → elevated restart procedure.
- Completion preference: prepare English and Turkish update text only after the user explicitly requests release/publication; use a full, plain release URL on its own line after verifying publication. Preparing copy does not authorize publishing.
