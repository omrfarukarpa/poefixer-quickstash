# Project memory

## 2026-09-23 — Korece beta.1 geri bildirimi, 1.4.2-beta.2

- Kanıt (Korece kullanıcı ekran görüntüleri, v1.4.2-beta.1, host v346): UI-tree `아이템 강조하기`, `speed` ve `속도` satırlarını doğru Korece gösteriyor; arama kutusu okuma sorunu kapandı. `speed` → TAKE (10), `속도` → TAKE (0). Kullanıcı beyanı: Seal Relic için `인장` ve `seal` → 0. poe2db kr: Seal Relic = `인장 유물`.
- Kök neden daraltması: `ContainsCI` bayt bazlı ve UTF-8 Korece için eşleşmeyi bozmaz; liste isim alanı (`InventoryItem.BaseTypeName`, enumerate + `read_string`) ne `인장 유물` ne `Seal Relic` içeriyor. Mod eşleşmesi `FormatStat` (host .csd) İngilizce satırlarından geliyor; Korece stat metni eklentiye hiç ulaşmıyor. Host içindeki metin dönüşümü eklenti tarafından görülemiyor; upstream ExamplePlugin SDK 2026-08-23'ten beri değişmedi, v345/v346 notlarında Unicode maddesi yok.
- Hipotez (doğrulanmadı): enumerate isimleri host'ta kayıplı daraltılıyor (önceki `??? ????` görüntüsü karakter başına bir `?`); plugin buffer'a yazan `read_item_base_type_name` yolu düzeltilen `get_text` gibi UTF-8 dönebilir.
- Değişiklik: `ItemTextCache` (eski `ModTextCache`) filtre doluyken `ReadItemBaseTypeName`/`ReadItemUniqueName` sonucunu liste adından farklıysa isim havuzuna ekler (64/çağrı bütçe, adres+Path önbelleği). Debug "Diagnostics: text encoding report": filtre ve ilk 30 eşya için liste/direkt isim, mod metni, sınıf (`utf8`, `ascii+?`, `not-utf8`) ve hex; "Copy text report" panoya kopyalar ve `config/text-report.txt` yazar.
- `kQuickStashVersion` ve README 1.4.2-beta.2. Temiz Release/x64 derleme 0 uyarı; DLL SHA-256 `adb8182313783b4b674a88c60755fb071dd8565759eb5812f77d76b3f2a7e6d0`, yerel kuruluma kopyalandı. Oyun içi doğrulama yok; otomatik test çalıştırılmadı.
- Risk: İngilizce istemcide direkt isim liste adından farklı bir biçim dönerse (ör. iç kimlik) yanlış eşleşme üretebilir; beta.2 rapor/Debug tablosuyla kontrol edilmeli. Direkt yol da kayıplıysa isim ve Korece stat metni host düzeltmesi ister.

## 2026-09-23 — TAKE filtre düzeltmesi, v1.4.2-beta.1 yayını

- Kullanıcının yeni bilgisi: geliştirici host'u düzelttikten sonra Korece istemcide TAKE görünür oldu; İngilizce sorguyla ilgisiz eşyaların sonuçlarda kaldığı bildirildi. Bu gerçek oyun gözlemi yerel olarak tekrar üretilemedi; 2026-09-22 host/SDK analizi artık bu belirti için blokaj olarak ele alınmıyor.
- `ReadPoeHighlight` önce UI yürüyüşünde ilk aynı satır metnini alıyordu; artık etikete yakın kabul edilen metinler içinde en yakın X seçilir. Alan değeri de placeholder'ı da okunamıyorsa TAKE kapatılır, boş filtre sanılıp bütün sekme hedeflenmez. Bağımsız kaynak incelemesi UI satırında başka bir metnin daha yakın olabileceğini (canlı kanıt yok) işaretledi; birden fazla farklı metin veya placeholder+değer birlikte görülürse TAKE bu yüzden gizlenir. Tek başına yanlış/uzak etiket olasılığı hâlâ oyun içi kontrol gerektirir.
- `BuildModText` arama metninden oyunda görünmeyen `Mod.Id` ve `StatKey` kaldırıldı; `Name`, `AffixName`, `FormatStat` ve mevcut bozulma/aggregate etiketleri kalır. Gizli anahtarlardan gelen yanlış eşleşmeleri azaltır; başka dillerde ad/affix ve biçimlendirilmiş statların nasıl döndüğü oyun içi tanı ekranıyla karşılaştırılmalı.
- `QuickStash.cpp` ve README sürümü 1.4.2-beta.1 olarak güncellendi. Temiz Release/x64 derlemesi tamamlandı; DLL SHA-256 özeti `b34d299eaaf529a3d609cb67303210861fb4687c88cf752941cfd1ea2ccfa57b`. Kullanıcının açık "beta sürümleri ilerletip yayınlayalım" talimatıyla GitHub'da prerelease olarak yayımlandı. Oyun içi Korece doğrulama kullanıcı topluluğundan bekleniyor.

## 2026-09-22 — Host/SDK Unicode change investigation

- `POEFixer/ExamplePlugin` public `master` branch still points at 2026-08-23 commit `b2e52aad`; `UiServiceAbi::get_text/get_string_id` and C++ wrappers have unchanged signatures there. No public Unicode-specific SDK method was found. Public v345 host release (2026-09-22) mentions trade and protected-slot changes, not a Unicode SDK fix.
- Installed `D:/POE2/fixer/fixer.exe` was last modified 2026-09-18; v345 has not been installed in that folder. No separate newer `PluginAbi.h` was found in the local workspace/downloads.
- Proposed bounded follow-up awaiting the actual updated host/SDK package or developer's change notes: compare new ABI layout to installed host, expose raw UTF-8 byte diagnostics for the Korean UI label, then update only the required QuickStash call path and clean-build/deploy locally; no publication requested.

## 2026-09-19 — Korean UI detection report

- User supplied the requested Debug UI-tree capture with the game still in Korean, both inventory and stash open, and `fire` typed into the native search field. The game screenshot visibly shows the label `아이템 강조하기` and the query `fire` at the bottom-left.
- The UI-tree table renders the label row at approximately `391,1178` as `??? ????` in both Text and StringId, while the query row is `fire` at approximately `547,1173`. A screenshot of `ImGui::TextUnformatted` cannot distinguish literal `?` bytes returned by the host from missing Korean font glyphs replaced during rendering; raw UTF-8 byte diagnostics are needed before attributing the defect to the host.
- This isolates the failure to Korean Highlight Items recognition, not necessarily to host string conversion. Do not add another guessed Korean spelling or claim Unicode data loss until raw bytes or the updated host implementation confirm it; English remains a temporary workaround.

## 2026-09-19 — English-language control case
- User confirmed that the same v1.4.1 DLL shows and operates TAKE when the game language is switched to English, while Korean mode does not. The attached screenshot shows the working English anchor and placeholder (`Highlight Items`, `Type keywords here...`, `TAKE (0)`). This isolates the failure to Korean Highlight Items UI recognition; stash detection, plugin loading, button rendering, and click flow are working.
- v1.4.1 includes Korean anchor strings (`아이템 강조하기`, `아이템 강조`) and ASCII StringId fragments; the Korean Debug capture above is the next evidence. Font glyph absence and host conversion remain competing hypotheses pending raw bytes.

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
