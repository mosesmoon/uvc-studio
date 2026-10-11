# UVC Studio 功能說明

[English](README.md) · **台灣正體中文** · [Deutsch](README.de.md) · [Français](README.fr.md) · [Italiano](README.it.md) · [Español](README.es.md) · [Português](README.pt.md) · [日本語](README.ja.md) · [한국어](README.ko.md) · [简体中文](README.zh-CN.md) · [ไทย](README.th.md) · [Tiếng Việt](README.vi.md)

UVC Studio 1.1.0 是一套原生、完全離線的擷取卡應用程式，支援 Linux、Windows 與 Meta Quest：透過任何 UVC 擷取卡低延遲玩主機遊戲，並可 4K 升頻、加上復古畫面或 3D 景深、把網路攝影機畫面疊在遊戲上、錄影與截圖，全程不連網。1.1.0 是第一個正式版，為 Meta Quest 推出「UVC Studio 白金」：4K 升頻與 3D 景深每天各可免費使用 30 分鐘，一次購買即可永久無限使用；Linux 與 Windows 版維持免費，並以 MIT 授權開放原始碼。

![UVC Studio 開啟 4K 升頻玩遊戲](docs/screenshots/02-playing-4k.png)

## 支援平台與下載檔

同一套程式碼，各平台功能與版本號一致。

| 平台 | 下載檔 | 影像擷取 | 音訊 | 繪圖 | 錄影 |
| --- | --- | --- | --- | --- | --- |
| Linux（x86_64） | [`UVCStudio-v1.1.0-x86_64.AppImage`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-v1.1.0-x86_64.AppImage) | Video4Linux2 | PulseAudio / PipeWire / ALSA | OpenGL 3.3 | H.264（VA-API / NVENC / AMF 硬體編碼）+ AAC，MP4，須具備其一 |
| Windows（x64） | [`UVCStudio-Setup-v1.1.0.exe`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-Setup-v1.1.0.exe)（安裝版）或 [`UVCStudio-Portable-v1.1.0.zip`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-Portable-v1.1.0.zip)（免安裝版） | Media Foundation | WASAPI | OpenGL 3.3 | H.264（NVENC / AMF / 透過 Media Foundation 的 Intel 硬體編碼，否則用 Windows 內建編碼器）+ AAC，MP4，須具備其一 |
| Meta Quest 2 / Pro / 3 / 3S | [`UVCStudio-Quest-v1.1.0-Free.apk`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-Quest-v1.1.0-Free.apk)（側載）或 Meta Horizon Store（即將上架） | USB 主機模式，libuvc | 直接讀取擷取卡的 USB 音訊（立體聲或 5.1） | OpenGL ES 3（2D 視窗）、OpenXR（沉浸模式） | H.264 + AAC（硬體 MediaCodec），MP4 |

原始碼（Linux 與 Windows 版，MIT 授權）：本儲存庫，或 [`UVCStudio-v1.1.0-src.tar.gz`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-v1.1.0-src.tar.gz)。

Meta Quest 版的 4K 升頻與 3D 景深每天各可免費使用 30 分鐘；購買一次「UVC Studio 白金」（App 內購買）即可無限使用。Linux 與 Windows 版所有功能皆免費。

## 截圖

Linux 版，1920 × 1080；點圖片可看原尺寸。Meta Quest 的 2D 視窗使用相同介面。

<table>
<tr><td width="50%"><a href="docs/screenshots/01-ready.png"><img src="docs/screenshots/01-ready.png" alt="待機畫面：即時檢查擷取卡、遊戲音訊與網路攝影機"></a><br><sub>待機畫面：即時檢查擷取卡、遊戲音訊與網路攝影機</sub></td><td width="50%"><a href="docs/screenshots/03-4k-upscaling.png"><img src="docs/screenshots/03-4k-upscaling.png" alt="4K 升頻：四種升頻方式可選"></a><br><sub>4K 升頻：四種升頻方式可選</sub></td></tr>
<tr><td width="50%"><a href="docs/screenshots/04-settings.png"><img src="docs/screenshots/04-settings.png" alt="設定：所有選項都在一個玻璃面板上"></a><br><sub>設定：所有選項都在一個玻璃面板上</sub></td><td width="50%"><a href="docs/screenshots/05-webcam-overlay.png"><img src="docs/screenshots/05-webcam-overlay.png" alt="網路攝影機畫面疊在遊戲上"></a><br><sub>網路攝影機畫面疊在遊戲上</sub></td></tr>
<tr><td width="50%"><a href="docs/screenshots/06-retro-vhs.png"><img src="docs/screenshots/06-retro-vhs.png" alt="復古畫面：VHS"></a><br><sub>復古畫面：VHS</sub></td><td width="50%"><a href="docs/screenshots/07-3d-depth.png"><img src="docs/screenshots/07-3d-depth.png" alt="3D 景深"></a><br><sub>3D 景深</sub></td></tr>
<tr><td width="50%"><a href="docs/screenshots/08-shortcuts.png"><img src="docs/screenshots/08-shortcuts.png" alt="鍵盤快捷鍵"></a><br><sub>鍵盤快捷鍵</sub></td><td width="50%"><a href="docs/screenshots/09-light-theme.png"><img src="docs/screenshots/09-light-theme.png" alt="淺色主題"></a><br><sub>淺色主題</sub></td></tr>
<tr><td width="50%"><a href="docs/screenshots/10-zh-TW.png"><img src="docs/screenshots/10-zh-TW.png" alt="台灣正體中文介面"></a><br><sub>台灣正體中文介面</sub></td><td width="50%"><a href="docs/screenshots/11-languages.png"><img src="docs/screenshots/11-languages.png" alt="12 種介面語言"></a><br><sub>12 種介面語言</sub></td></tr>
</table>

## 開始使用

1. 把擷取卡接上 USB，開啟 UVC Studio。**待機畫面**會即時檢查三個項目：「擷取卡」、「遊戲音訊」與「網路攝影機（選用）」。找不到擷取卡時顯示「找不到 — 請接上 USB」，沒有聲音來源時顯示「找不到（僅視訊）」。
2. 按待機畫面或右上角的 **連線**，或直接按 `Enter`。「設定 → 一般 → 自動連線」預設為開啟：一插上擷取卡就會自動連線，開啟 App 時若擷取卡已經接好也會直接連線。
3. 開始玩。按同一個按鈕（**中斷連線**）或 `Ctrl+D` 就會關閉所有裝置；錄影中的話會先把影片存好。

沒有「暫停」或「繼續」：UVC Studio 只有已連線與未連線兩種狀態。擷取卡被拔掉時，連線狀態下方會出現錯誤訊息並回到待機畫面；開啟自動連線時，擷取卡插回去就會自動接上。

## 擷取與聲音

任何 UVC 擷取卡都不必另外安裝驅動：ShadowCast、Elgato、AVerMedia、Magewell 與一般 USB 3 HDMI 擷取棒用法都一樣。

- **使用擷取卡實際支援的模式**：解析度與影格率依擷取卡回報的清單，由大到小排列，並標示 4K、QHD、Full HD、HD、SD。預設選擇能達到 59 fps 以上的最高解析度。格式可選自動、未壓縮（延遲最低）或壓縮（可用更高的解析度與影格率）。
- **自動選對裝置**：還沒選過裝置時，會依名稱挑出擷取卡、它的聲音與網路攝影機，不會把筆電內建或虛擬攝影機誤當成擷取卡。名稱相同的裝置會加上 (2)、(3)… 區分。
- **低延遲**：永遠顯示最新的一格畫面，不會排隊累積舊畫面。
- **記住設定**，執行中插入或拔除擷取卡都能偵測。
- **監聽**（`L`）：透過喇叭或耳機聽到遊戲聲音。滑鼠指到 Dock 按鈕上會出現開關、音量與目前的輸出裝置，點輸出那一行可直接到「設定 → 聲音」。
- **即時模式顯示**：「設定 → 來源」會顯示實際的串流模式，例如 3840 × 2160 · 60 fps · YUY2。
- **Meta Quest 音訊**：App 直接從 USB 讀取擷取卡的聲音（USB Audio Class 1 與 2，包含 MacroSilicon MS2109 擷取棒）——「設定 → 聲音 → 音訊輸入 → 擷取卡音訊（USB）」。「設定 → 聲音 → 遊戲聲道」可選**立體聲 2CH**（預設）或**環繞 6CH**：擷取卡具備 6 聲道模式（例如 CJ-16），且主機透過 HDMI 輸出 5.1（例如 Switch 2 設為環繞）時，就能播放 5.1；錄影仍存成立體聲。
- **Quest 自動恢復**：擷取卡停止傳送畫面時，App 會自動重新開啟（最多兩次），仍無畫面則提示檢查 HDMI 來源或重新插拔擷取卡。

## 畫面處理

四種 4K 升頻、三種復古畫面與 3D 景深都在 GPU 上逐格處理，除了 GPU 運算時間外不增加延遲。

**4K 升頻**（`U`）：點 Dock 按鈕或按 `U` 開關，滑鼠指到按鈕上可選擇方式，`Shift+U` 切換到下一種。「設定 → 畫面 → 4K 升頻」提供「關閉」、FSR、Anime4K、Lanczos 與 NIS：選任一方式就會開啟升頻，選「關閉」則會記住上次用的方式。

| 方式 | 原理 | 適合 | 放大倍率 |
| --- | --- | --- | --- |
| FSR 4K 升頻（預設） | AMD FidelityFX Super Resolution 1：EASU 邊緣自適應放大，再以 RCAS 銳化 | 遊戲與各種影像 | 720p / 1080p 到 4K；標準畫質來源最多 3 倍 |
| Anime4K 動畫專用 4K 升頻 | Anime4K ×2 神經網路（桌面版 M、Quest 版 S），剩下的倍率由 FSR 補上 | 動畫、卡通、線稿 | 網路放大 2 倍，再到 4K |
| Lanczos + CAS 4K 升頻 | Lanczos-3 重取樣（每軸 6 點），再以 AMD FidelityFX CAS 銳化 | 經典、中性的縮放 | 到 4K |
| NIS 4K 升頻 | NVIDIA Image Scaling 1.0.3：依邊緣方向的 6 點縮放與自適應銳化 | 清晰細節 | 最多 2 倍，超過由 Lanczos-3 完成 |

只有新畫面進來時才重新升頻。Quest 若不支援半精度浮點渲染目標，Anime4K 會改用 FSR。

- **復古畫面**（`C`）：滑鼠指到 Dock 按鈕上可選擇「關閉」、「CRT」、「VHS」或「街機」；按 `C` 依序切換，點按鈕則在「關閉」與上次用的效果之間切換，「設定 → 畫面 → 復古畫面」也有相同的選項。效果在 4K 升頻之後處理，可以和升頻同時使用，截圖、錄影與 Quest 巨型螢幕都看得到；選擇會自動記住。
  - **CRT**：映像管電視——交錯排列的槽狀遮罩、隨亮度變深淺的掃描線、微彎的螢幕弧面與柔和暗角。
  - **VHS**：家用錄影帶——畫質略為柔和、色彩向右暈開、略微褪色偏暖、黑色略浮，加上淡淡的雜訊與輕微晃動，偶爾還有一條淡淡的循軌帶由上往下飄過（不影響閱讀）。
  - **街機**：大型電玩機台螢幕——低解析度掃描、粗黑掃描線、直條 RGB 光柵、亮部微微發光、色彩更鮮明，再加上些微弧面。
- **3D 景深**（`D`）：估算畫面中的遠近。滑鼠指到按鈕上（或開啟「設定 → 3D 景深」）可選擇**景深格式**並調整**效果強度**。「景深效果（僅 2D 強化）」讓前景更清晰鮮豔、背景更柔和朦朧；桌面版的立體 3D 格式請見下一節；在 Quest 沉浸模式選「立體 3D 巨型螢幕」，巨型螢幕就會變成真正的立體 3D。按 `D` 或點按鈕可在「無景深」與上次用的格式之間切換。
- **亮度、對比、飽和度**（設定 → 畫面）：−40～+40，0 為原始畫面。**伽瑪**：0.70～1.40（1.00 為原始畫面），大於 1.00 提亮中間調、小於 1.00 讓中間調更深，黑白兩端不變。**風格**預設有「自然」、「寫實」（略帶 S 曲線的自然對比與收斂的色彩，暗部仍保留細節）、「鮮豔」、「電影」與「明亮房間」（伽瑪略為調高，在明亮的房間裡也看得清暗場景）。
- **鏡像**（`X`）：整個畫面左右翻轉。

## 立體 3D 格式（Linux / Windows）

桌面版的景深格式選單（3D 景深按鈕或「設定 → 3D 景深」）另有四種立體格式，可把真正的立體 3D 送到 3D 顯示器。顯示器需設成相同的 3D 格式並切換全螢幕（`F`）；截圖與錄影仍是 2D。App 會自動偵測文字與 HUD，讓它們留在螢幕平面附近，景深的變化也更平滑，同一個字在左右眼都整塊一起移動，不再像被切開。

| 格式 | 適用 |
| --- | --- |
| 左右並排・全寬（3840×1080） | 3D 模式下的 XR 眼鏡，例如 XREAL、VITURE、RayNeo |
| 左右並排・半寬 | 3D 電視 |
| 上下 | 3D 電視與投影機 |
| 紅藍眼鏡 | 紅藍 3D 眼鏡 |

## 網路攝影機疊加、截圖與錄影

看到什麼就存什麼：畫面效果、鏡像與網路攝影機疊加畫面都會一起寫進截圖與錄影。

- **網路攝影機疊加**（`W`）：把網路攝影機畫面疊在遊戲的四個角落之一（預設左下），大小可選 S、M、L（畫面寬度的 18%、26%、36%）。
  - 可以拖到任何位置，放開後會自動滑到最近的角落；在疊加畫面上滾動滑鼠滾輪可調整大小。
  - 滑鼠指著疊加畫面會出現小工具列：「鏡像」、「大小」、「移除」。
  - 滑鼠指到 Dock 按鈕上（或打開「設定 → 網路攝影機疊加」）可設定「攝影機」、「畫質」（720p · 30 fps、1080p · 30 fps 或 1080p · 60 fps）、「大小」、「角落」、「鏡像」，以及 **交換畫面**：網路攝影機變成主畫面，擷取卡改為疊加畫面。
- **截圖**（`S`）：將目前畫面存成 PNG。
- **錄影**（`R`）：MP4 格式，H.264 影像與 192 kbit/s AAC 音訊。位元率隨串流調整（寬 × 高 × fps × 0.12，介於 8～50 Mbit/s：1080p60 約 15 Mbit/s，4K60 為 50 Mbit/s）。錄影時連線狀態會顯示 REC 與已錄製時間。
  - **Windows 版**：不附帶軟體編碼器。必須具備 NVIDIA（NVENC）、AMD（AMF）或 Intel 顯示晶片的硬體編碼器，或 Windows 內建的 H.264 編碼器（Media Foundation；Windows「N」版需先安裝 Media Feature Pack）才能錄影。
  - **Linux 版**：同樣不附帶軟體編碼器。必須具備 Intel 或 AMD 顯示晶片（透過 VA-API，需安裝 VA-API 驅動）、NVIDIA NVENC（官方驅動）或 AMD AMF 其中之一才能錄影。
  - 都沒有的話其他功能照常，只有按下錄影時會提示「沒有視訊編碼器：錄影需要 NVIDIA、AMD 或 Intel 顯示晶片」。
- **錄製麥克風**（`M`）：把你的聲音與遊戲聲音混在一起。
- **檔名與存檔位置**：檔名為 `UVC Studio 2026-10-06 21.30.15.png`／`.mp4`（日期與時間；同名時自動加上編號）。截圖存到 **圖片／UVC Studio**，錄影存到 **影片／UVC Studio**，並跳出「已儲存到 Pictures/UVC Studio」通知與 **查看** 按鈕；「設定 → 一般」也有「開啟截圖資料夾」與「開啟錄影資料夾」。Quest 存進相簿（`Pictures/UVC Studio` 與 `Movies/UVC Studio`），可在「檔案」App 或透過 USB 查看。

## Meta Quest

把擷取卡插在頭顯的 USB-C 孔，就能在可縮放的視窗或巨型虛擬螢幕上玩遊戲，還能看真正的立體 3D。

- **安裝**：從 Meta Horizon Store 安裝（即將上架），或在頭顯開啟開發人員模式後，從本專案的 Releases 下載 `UVCStudio-Quest-v1.1.0-Free.apk` 安裝（Meta Quest Developer Hub、SideQuest 或 `adb install`）。白金只能在從商店安裝的版本中購買。
- **連接**：也可以透過 USB-C OTG 轉接頭或集線器。同意「連接此裝置時開啟 UVC Studio？」（勾選「永遠」），以及 Android 使用 USB 擷取卡所需的相機與麥克風權限。頭顯本身的鏡頭從不使用。
- **解碼**：H.264 與 HEVC 擷取卡交給頭顯內建的硬體解碼器；App 內附的 FFmpeg 只用來處理 MJPEG。
- **2D 視窗**（預設）：與桌面版相同的完整介面，放在可移動、可縮放的 Horizon OS 面板中。對畫面捏兩下可進入或退出全螢幕。
- **沉浸模式**：Dock 最後面（設定之後）的頭顯按鈕可切換到巨型虛擬螢幕再切回來。切換時 App 會先中斷連線，到另一邊再自動連線。
- **Quest 的 3D 景深**：2D 視窗可選「無景深」或「景深效果」；沉浸模式可選「無景深」或「立體 3D 巨型螢幕」。

| 沉浸模式操作 | 方法 |
| --- | --- |
| 點選控制面板 | 用控制器指向並扣下扳機，或用手捏一下 |
| 移動控制面板 | 抓住面板下方的白色橫條拖曳，往前推或往後拉可調整距離；「設定 → VR → 重設面板位置」可恢復原位 |
| 移動螢幕 | 抓住螢幕下方的白色橫條拖曳；手往前推或往後拉可調整距離 |
| 縮放螢幕 | 指向角落到出現括弧後，捏住拖曳 |
| 拉近 / 推遠、縮小 / 放大 | 搖桿上 / 下、左 / 右 |
| 隱藏或顯示控制面板 | 在螢幕上捏兩下，或按 B、Y、選單鍵 |
| 重新置中 | 長按 Meta 鍵 |
| 螢幕預設 | 設定 → VR：背景（透視或暗室）、螢幕形狀（平面或曲面）、螢幕大小、螢幕距離、更新率 72 / 90 / 120 Hz（預設 90） |

- **系統選單**：打開時 App 持續繪製、隱藏指標並靜音。
- **降低發熱**：升頻只做到巨型螢幕實際看得出的解析度（每度約 30 像素，最寬 2560），左右眼畫面也以同樣大小繪製。
- **過熱保護**：同時監看 Android 溫度狀態與 OpenXR 溫度通知。頭顯偏熱時改用 FSR 與 72 Hz；過熱時暫停升頻與 3D 景深。不會更改你的設定，降溫後會跳出通知。
- **負擔顯示**：連線時，「設定 → 關於」顯示每格 GPU 耗時、處理解析度與頭顯溫度。

## 介面

整套介面採用 Liquid Glass 風格：半透明的毛玻璃面板，連線時會透出後方遊戲畫面的模糊、加強飽和度版本。

- **上方列**：左側是 UVC Studio 字樣；正中間是**連線狀態**（「未連線」；綠點加上裝置 · 解析度 · fps，升頻時加上 ▸ 4K 與升頻方式、開 3D 景深時加上 · 3D；錄影時是閃爍的紅色 REC 與錄影時間）；右側是語言選單與 **連線／中斷連線** 按鈕。錯誤訊息會以紅字顯示在連線狀態下方幾秒鐘。
- **Dock**：分成四組——「截圖、錄影、錄製麥克風」·「4K 升頻、復古畫面、3D 景深、鏡像」·「網路攝影機疊加、監聽」·「設定」。游標指到的按鈕會放大，開啟中的功能以品牌漸層顯示，每個提示都附上快捷鍵。版本號在左下角，全螢幕按鈕在右下角。
- **設定**：一個大玻璃面板，卡片最多分成三欄並平均分配，所有選項不必捲動就能調整：來源、聲音、畫面、網路攝影機疊加、顯示、VR（Quest 沉浸模式）、一般、3D 景深、關於——自動排列時，「3D 景深」卡片就在「關於」正上方。「關於」顯示版本，連線時另有每格 GPU 耗時與處理解析度（Quest 還有頭顯溫度），以及「開放原始碼授權」按鈕；隱私說明就在這個按鈕開啟的頁面裡。
  - **收合卡片**：每張卡片標題列最右邊、鎖頭右側有一個圓形箭頭按鈕，點一下卡片就收合成只剩標題列（名稱、鎖頭與折疊按鈕保留），再點一下展開。下方卡片會滑上來，卡片不會換欄，收合狀態也會記住。
  - **自己排列卡片**：每張卡片標題列的右上角都有一個鎖頭，所有卡片預設都是鎖定的。點一下鎖頭（「解鎖以移動」），再按住卡片的標題列，就能把它拖到三欄中的其他位置；畫面上會出現一個空位，標示放開後的位置，按 `Esc` 取消拖曳。再點一下鎖頭（「鎖定位置」）就會固定。鎖定的卡片留在原處，解鎖的卡片在它們之間移動。排列與解鎖的卡片都會記住，下次開啟還是一樣。使用自訂排列時，關閉按鈕旁會出現「重設版面配置」，點一下就恢復自動排列，並鎖定所有卡片。視窗太窄、放不下三欄時會改用自動排列，你的排列仍會保留，視窗拉寬後就恢復。Quest 的 2D 視窗用控制器的指標也能同樣操作。
- **語言**：台灣正體中文、English、Deutsch、Français、Italiano、Español、Português、日本語、한국어、简体中文、ไทย、Tiếng Việt，從右上角的玻璃選單中選擇；第一次開啟時依系統語言自動選擇。
- **主題**：自動（06:00～18:00 為淺色）、深色或淺色。
- **介面大小**：依螢幕自動調整（1080p 100%、1440p 約 135%、4K 200%）；可在「設定 → 顯示」或用 `Ctrl` `+` / `−` / `0` 手動調整。
- **字型與圖示**：Manrope 與 IBM Plex Sans（泰文用 IBM Plex Sans Thai，中日韓文用 Noto Sans CJK TC）；圖示全部是 UVC Studio 自己繪製的。
- **全螢幕**：`F`、`F11`、右下角按鈕或雙擊畫面；`Esc` 離開。

## 鍵盤快捷鍵（Linux / Windows）

在 App 裡按 `?` 或 `H` 會顯示同樣的快捷鍵一覽表，按任意鍵或點一下即可關閉。

| 按鍵 | 功能 |
| --- | --- |
| `Enter` | 連線 |
| `Ctrl+D` | 中斷連線 |
| `S` | 截圖 |
| `R` | 開始或停止錄影 |
| `M` | 開關錄製麥克風 |
| `L` | 開關監聽 |
| `W` | 開關網路攝影機疊加 |
| `X` | 鏡像 |
| `U` | 開關 4K 升頻 |
| `Shift+U` | 下一個升頻方式（並開啟升頻） |
| `C` | 復古畫面：關閉 → CRT → VHS → 街機 |
| `D` | 3D 景深：在「無景深」與上次用的格式之間切換 |
| `O` 或 `Ctrl+,` | 設定 |
| `F` 或 `F11` | 全螢幕 |
| `Esc` | 取消拖曳卡片、離開全螢幕或關閉設定 |
| `?` 或 `H` | 鍵盤快捷鍵 |
| `Ctrl` `+` / `−` / `0` | 介面放大 / 縮小 / 還原 |

## 隱私

UVC Studio 完全在你的裝置上運作。畫面、聲音與錄影都不會離開這台裝置——無需帳號、不上傳、不追蹤，App 也從不連網。Meta Quest 版購買白金與確認購買紀錄，都經由頭顯的 Meta Horizon Store 服務處理；UVC Studio 本身不傳送任何資料。

## 開放原始碼授權

UVC Studio 的 Linux 與 Windows 版以 [MIT 授權](LICENSE)開放原始碼；Meta Quest 版不開放原始碼。UVC Studio 不含任何 GPL 元件；FFmpeg 以純 LGPL 方式建置（Quest 版只含 MJPEG 解碼器，並以可替換的動態函式庫提供）。其他元件：SDL2、Dear ImGui、miniaudio、stb、NanoSVG、libusb 與 libuvc（Quest）、OpenXR loader、AMD FidelityFX FSR 1 與 CAS、Anime4K、NVIDIA Image Scaling。字型（Manrope、IBM Plex Sans、IBM Plex Sans Thai、Noto Sans CJK TC）採用 SIL Open Font License。所有元件與完整授權條文都列在 App 的「設定 → 關於 → 開放原始碼授權」。Meta Quest 版另含 Meta Platform SDK 載入程式（Oculus SDK License），用於商店購買。
