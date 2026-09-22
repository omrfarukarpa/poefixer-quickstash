# Kod haritası — Quick Stash

> Güncellendi: 2026-09-23. Kanıt: QuickStash.cpp, config/game/input/overlay/ui başlıkları, çözüm/proje dosyaları ve SDK imzaları. Bu oturumda oyun içi çalışma gözlenmedi.

## Giriş ve sınırlar

Windows x64/C++20 DLL; `QuickStash.sln` ve `QuickStash.vcxproj`, MSVC v145. Tek eklenti derleme birimi `QuickStash.cpp`; diğer birimler vendored ImGui. Kod sürümü 1.4.2-beta.1 (prerelease). Kurallar `AGENTS.md`, yayın/kararlar `PROJECT_MEMORY.md` içindedir.

| Yol | Görev |
|---|---|
| `QuickStash.cpp` | Yaşam döngüsü, DrawUI/DrawSettings, önbellekler, tuş yakalama ve OnFrameTick |
| `config/Settings.h` | Zamanlama, hariç tutulan hücreler, readMods/debug ayarları; JSON Load/Save |
| `game/PanelDetector.h` | FindMainInventory, FindOpenStash, görünürlük yardımcıları; tek guild kimliği tanımı |
| `game/PoeHighlight.h` | UI ağacından Highlight Items metni ve düğme ankrajı |
| `game/TransferPlanner.h` | Hariç tutulan hücreleri atlayan kuyruk; slot merkezleri ve ScreenPoint tipi |
| `game/WithdrawPlanner.h` | FindOpenStashAny, görünür adaylar, eşya dikdörtgenleri, isim/mod filtresi |
| `game/TransferState.h` | Ortak BeginRun, Tick aşamaları, iptal, son bekleme ve Ctrl/imleç geri yükleme |
| `input/Win32Input.h` | Win32 SendInput, Ctrl scan code, client→screen dönüşümü |
| `overlay/TransferButtonOverlay.h` | Transfer, TAKE/TAKE G(N), ilerleme ve HardwareClick |
| `ui/ExclusionGrid.h` | Çanta hariç tutma hücreleri |
| `ui/InventoryDiagnostics.h` | Envanter, filtre metni, Agg anahtarları ve UI ağacı tanısı |
| `sdk/PluginAbi.h`, `sdk/PluginSDK.h` | C ABI v6 ve C++ sarmalayıcıları; authoritative upstream POEFixer/ExamplePlugin |

## Çalışma akışı

1. OnEnable host uyumluluğunu kontrol eder, ayarları yükler ve OnFrame olayına abone olur.
2. `RefreshInventoryIfNeeded` en az 150 ms aralıkla Scan ve çanta seçimi yapar. Transfer, bu canlı önbelleğin grid koordinatlarını her hedefte kullanır.
3. Uygun DrawUI karesinde `UpdateWithdraw` çalışır. Highlight Items UI yürüyüşü 150 ms ile sınırlıdır; stash seçimi, GetAll ve aday değerlendirmesi halen her çağrıda yapılır.
4. `FindOpenStashAny` çantayı, oyuncu slot adlarını ve Ritual önekini dışlar. Guild `10002` kimliğiyle açıkça kabul edilir ve yalnızca guild için alan <10 filtresi atlanır. Boş veya ekranda görünmeyen kaynak seçilmez; en fazla eşyaya sahip uygun kaynak kazanır.
5. Highlight alanı bulunmadan TAKE hedefi oluşturulmaz. Yakın metinler içinde ankraja X olarak en yakın geçerli değer seçilir; alan okunamıyor, aynı satırda birden fazla farklı değer görülüyor veya placeholder ile değer aynı anda bulunuyorsa TAKE gizlenir. Alan bulunmuş ve filtre boşsa bütün uygun eşyalar seçilir. TAKE sayısı tıklama/yığın sayısıdır; ayrı miktar etiketi yığın toplamını verir.
6. Transfer slot kuyruğu, TAKE başlangıçta yakalanan eşya dikdörtgen merkezleriyle aynı `BeginRun` yoluna girer. `ScreenPoint`/`m_screenQueue` isimlerine rağmen bunlar oyun-client koordinatlarıdır.
7. Tick aşamaları `Spacing → Settling → PostClick`; ilk hareket CtrlDown sonrasındaki 50 ms eşiğinden önce yapılmaz. Hareketten önce `ClientToScreenPoint` uygulanır; geçersiz pencere/dönüşüm tıklamayı durdurur.
8. Bitiş Ctrl'ü en az 60 ms tabanlı bir süre tutar; Abort Ctrl'ü bırakır ve kaydedilmiş ekran imlecini geri getirir. Ön plan kaybı ve watchdog iptal yollarıdır. TAKE kaynak kapanış kontrolü 150 ms aralıkla iki kaçırma sonrası iptal eder; panel koruması ayarına bağlıdır.

## Dikdörtgen ve filtre sözleşmesi

- Önce ScreenValid eşyanın gerçek dikdörtgeni, ardından yalnızca `GridLayoutPlausible` geçen grid hesabı kullanılır. Düz/aşırı geniş özel tab ızgarasına normal slot hesabı uygulanmaz.
- İsim filtresi BaseTypeName + UniqueName içerir; internal Path isim aramasına katılmaz.
- Mod metni yalnızca `readMods` açık ve filtre doluyken okunur. Liveness probe `ReadItemBaseTypeName`; sonra ReadItemMods/ReadItemAggregatedStats. Corrupted bilgisi mod sonucundan gelir. Gizli `Id`/ham `StatKey` metinleri arama havuzundan çıkarıldı; görünen mod/affix adları ile FormatStat açıklaması kalır.
- Mod önbelleği adresle anahtarlanır; kaynak Path değişince yenilenir. Bir CollectCandidates çağrısının bütçesi 40 okuma; harita 4000 girdiyi aşınca temizlenir. Bu bütçe zamanlayıcı başına değil çağrı başınadır.
- Aggregate etiketleri: 8206 Item Rarity, 8207 Monster Pack Size, 8208 Monster Rarity, 8209 Monster Effectiveness, 8210 Waystone Drop Chance. Sıfır dışı değer etiket ekler; sayısal eşik karşılaştırması değildir. Host değişirse debug Agg sütunuyla doğrula.
- HardwareClick, ImGui MousePos kullanır; yalnızca host overlay modunda güvenilir sayılır. DrawUI boşluklarında basış takibi sıfırlanır.
- Guild yayıncısı PoeFixer v301 gerektirir. Bu sürümde vendored SDK değişmemiştir; ABI boyut gereksinimi 1.3.2 ile aynıdır.

## Veri, komut ve kurulum

Kalıcı veri `config/settings.json`; mevcut kullanıcı dosyasını dağıtımda koru. Ağ isteği, veritabanı veya zorunlu uygulama ortam değişkeni bu eklenti akışında yoktur.

```powershell
& 'C:/Program Files/Microsoft Visual Studio/18/Enterprise/MSBuild/Current/Bin/MSBuild.exe' QuickStash.sln -p:Configuration=Release -p:Platform=x64 -nologo -v:minimal -m
```

Temiz derleme için `-t:Rebuild`. Çıktı `bin/Release/QuickStash.dll`; kurulum `D:/POE2/fixer/Plugins/QuickStash/QuickStash.dll`, host kapalıyken veya AGENTS.md içindeki kapat/kopyala/yönetici olarak yeniden başlat akışı tamamlandıktan sonra. Bare DLL GitHub release varlığıdır; bin/obj ve yerel AGENTS Git dışında kalır. Takip edilen dosyalarda otomatik test/linter/CI görülmedi; kullanıcı istemeden test çalıştırılmaz.

## Sınırlamalar

TAKE koordinatları başlangıçta sabitlenir; aktarım sırasında sekme düzeni değişimi canlı doğrulanmalıdır. Per-frame aday taraması, fiziksel fare düğmesi eşlemesi ve panel kapatma seçeneği devre dışıyken watchdog bekleme davranışı için PROJECT_MEMORY.md içindeki riskleri oku. SDK API mevcudiyeti oyun belleğindeki alanların anlamını tek başına kanıtlamaz.

## 2026-09-19 canlı kullanıcı kanıtı

- İngilizce istemcide v1.4.1 TAKE görünür ve çalışır; Korece istemcide oyun label'ı `아이템 강조하기`, UI-tree **ImGui görünümü** ise `??? ????` gösterir; `fire` arama değeri okunur. Bu görünüm host'un gerçekten `?` baytları gönderdiğini kanıtlamaz: ImGui atlasındaki eksik Korece glifler de aynı görüntüyü üretebilir. `PoeHighlight.h` içindeki ham bayt eşleşmesi ile ekran çizimini ayıracak ASCII/hex tanı gereklidir; yeni Korece metin tahminiyle release yapılmamalıdır.
