# UVC Studio 功能介绍

[English](README.md) · [台灣正體中文](README.zh-TW.md) · [Deutsch](README.de.md) · [Français](README.fr.md) · [Italiano](README.it.md) · [Español](README.es.md) · [Português](README.pt.md) · [日本語](README.ja.md) · [한국어](README.ko.md) · **简体中文** · [ไทย](README.th.md) · [Tiếng Việt](README.vi.md)

UVC Studio 1.1.0 是一款原生、完全离线的采集卡应用，支持 Linux、Windows 和 Meta Quest：通过任意 UVC 采集卡低延迟畅玩主机游戏，还能 4K 升频、叠加复古画面或 3D 景深、把摄像头画面叠加到游戏上、录像和截图，全程不联网。1.1.0 是第一个正式版，为 Meta Quest 推出“UVC Studio 白金”：4K 升频与 3D 景深每天各可免费使用 30 分钟，一次购买即可永久无限使用；Linux 与 Windows 版保持免费，并以 MIT 许可证开源。

![UVC Studio 开启 4K 升频玩游戏](docs/screenshots/02-playing-4k.png)

## 支持平台与下载

同一套代码，各平台功能和版本号完全一致。

| 平台 | 下载 | 视频采集 | 音频 | 图形 | 录像 |
| --- | --- | --- | --- | --- | --- |
| Linux（x86_64） | [`UVCStudio-v1.1.0-x86_64.AppImage`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-v1.1.0-x86_64.AppImage) | Video4Linux2 | PulseAudio / PipeWire / ALSA | OpenGL 3.3 | H.264（VA-API / NVENC / AMF 硬件编码）+ AAC，MP4，需具备其一 |
| Windows（x64） | [`UVCStudio-Setup-v1.1.0.exe`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-Setup-v1.1.0.exe)（安装版）或 [`UVCStudio-Portable-v1.1.0.zip`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-Portable-v1.1.0.zip)（便携版） | Media Foundation | WASAPI | OpenGL 3.3 | H.264（NVENC / AMF / 通过 Media Foundation 的 Intel 硬件编码，否则使用 Windows 自带编码器）+ AAC，MP4，需具备其一 |
| Meta Quest 2 / Pro / 3 / 3S | [`UVCStudio-Quest-v1.1.0-Free.apk`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-Quest-v1.1.0-Free.apk)（侧载）或 Meta Horizon Store（即将上架） | USB 主机模式，libuvc | 直接读取采集卡的 USB 音频（立体声或 5.1） | OpenGL ES 3（2D 窗口）、OpenXR（沉浸模式） | H.264 + AAC（硬件 MediaCodec），MP4 |

源代码（Linux 与 Windows 版，MIT 许可证）：本仓库，或 [`UVCStudio-v1.1.0-src.tar.gz`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-v1.1.0-src.tar.gz)。

Meta Quest 版的 4K 升频与 3D 景深每天各可免费使用 30 分钟；购买一次“UVC Studio 白金”（应用内购买）即可无限使用。Linux 与 Windows 版所有功能均免费。

## 截图

Linux 版，1920 × 1080；点击图片可查看原尺寸。Meta Quest 的 2D 窗口使用相同界面。

<table>
<tr><td width="50%"><a href="docs/screenshots/01-ready.png"><img src="docs/screenshots/01-ready.png" alt="待机画面：实时检查采集卡、游戏音频与摄像头"></a><br><sub>待机画面：实时检查采集卡、游戏音频与摄像头</sub></td><td width="50%"><a href="docs/screenshots/03-4k-upscaling.png"><img src="docs/screenshots/03-4k-upscaling.png" alt="4K 升频：四种升频方式可选"></a><br><sub>4K 升频：四种升频方式可选</sub></td></tr>
<tr><td width="50%"><a href="docs/screenshots/04-settings.png"><img src="docs/screenshots/04-settings.png" alt="设置：所有选项都在一个玻璃面板上"></a><br><sub>设置：所有选项都在一个玻璃面板上</sub></td><td width="50%"><a href="docs/screenshots/05-webcam-overlay.png"><img src="docs/screenshots/05-webcam-overlay.png" alt="摄像头画面叠加在游戏上"></a><br><sub>摄像头画面叠加在游戏上</sub></td></tr>
<tr><td width="50%"><a href="docs/screenshots/06-retro-vhs.png"><img src="docs/screenshots/06-retro-vhs.png" alt="复古画面：VHS"></a><br><sub>复古画面：VHS</sub></td><td width="50%"><a href="docs/screenshots/07-3d-depth.png"><img src="docs/screenshots/07-3d-depth.png" alt="3D 景深"></a><br><sub>3D 景深</sub></td></tr>
<tr><td width="50%"><a href="docs/screenshots/08-shortcuts.png"><img src="docs/screenshots/08-shortcuts.png" alt="键盘快捷键"></a><br><sub>键盘快捷键</sub></td><td width="50%"><a href="docs/screenshots/09-light-theme.png"><img src="docs/screenshots/09-light-theme.png" alt="浅色主题"></a><br><sub>浅色主题</sub></td></tr>
<tr><td width="50%"><a href="docs/screenshots/10-zh-TW.png"><img src="docs/screenshots/10-zh-TW.png" alt="台灣正體中文界面"></a><br><sub>台灣正體中文界面</sub></td><td width="50%"><a href="docs/screenshots/11-languages.png"><img src="docs/screenshots/11-languages.png" alt="12 种界面语言"></a><br><sub>12 种界面语言</sub></td></tr>
</table>

## 快速上手

1. 把采集卡接入 USB，打开 UVC Studio。**启动画面**会实时检查三项：“采集卡”“游戏音频”和“摄像头（可选）”。找不到采集卡时显示“未找到 — 请接入 USB”，没有音频时显示“未找到（仅视频）”。
2. 点击启动画面或右上角的 **连接**，或直接按 `Enter`。“设置 → 常规 → 自动连接”默认开启：一插上采集卡就会自动连接；打开应用时如果采集卡已经接好，也会直接连接。
3. 开玩吧。点击同一个按钮（**断开连接**）或按 `Ctrl+D` 即可关闭所有设备；如果正在录像，会先结束录像并保存文件。

没有“暂停”或“继续”：UVC Studio 只有已连接和未连接两种状态。采集卡被拔出时，连接状态下方会显示错误信息并回到启动画面；开启自动连接后，重新插上采集卡就会自动连接。

## 采集与声音

任意 UVC 采集卡都无需另装驱动：ShadowCast、Elgato、AVerMedia、Magewell 以及普通的 USB 3 HDMI 采集棒用法都一样。

- **使用采集卡实际支持的模式**：分辨率和帧率直接来自采集卡上报的列表，从大到小排列，并标注 4K、QHD、Full HD、HD、SD。默认选择能达到 59 fps 以上的最高分辨率。格式可选自动、未压缩（延迟最低）或压缩（可用更高的分辨率和帧率）。
- **自动选对设备**：还没有保存过设备时，会根据名称识别采集卡、它的音频和摄像头，不会把笔记本内置摄像头或虚拟摄像头误认为采集卡。同名设备会加上 (2)、(3)… 加以区分。
- **低延迟**：始终显示最新的一帧，不会积压旧画面。
- **记住你的设置**，运行中插拔采集卡也能立即识别。
- **监听**（`L`）：通过音箱或耳机听到游戏声音。鼠标移到 Dock 按钮上会出现开关、音量和当前输出设备，点击“输出”那一行可直接跳到“设置 → 声音”。
- **实时模式显示**：“设置 → 来源”会显示实际的视频流，例如 3840 × 2160 · 60 fps · YUY2。
- **Meta Quest 音频**：应用直接从 USB 读取采集卡的声音（USB Audio Class 1 和 2，包括 MacroSilicon MS2109 采集棒）——“设置 → 声音 → 音频输入 → 采集卡音频（USB）”。“设置 → 声音 → 游戏声道”可选**立体声 2CH**（默认）或**环绕 6CH**：采集卡支持 6 声道模式（例如 CJ-16），且主机通过 HDMI 输出 5.1（例如 Switch 2 设为环绕）时，就能播放 5.1；录像仍保存为立体声。
- **Quest 自动恢复**：采集卡不再输出画面时，应用会自动重新打开（最多两次），仍然没有画面则提示检查 HDMI 信号源或重新插拔采集卡。

## 画面处理

四种 4K 升频、三种复古画面和 3D 景深都在 GPU 上逐帧处理，除了 GPU 运算时间外不增加任何延迟。

**4K 升频**（`U`）：点击 Dock 按钮或按 `U` 开关，鼠标移到按钮上可以选择方式，`Shift+U` 切换到下一种。“设置 → 画面 → 4K 升频”提供“关闭”、FSR、Anime4K、Lanczos 和 NIS：选择任一方式即开启升频，选“关闭”会记住上次使用的方式。

| 方式 | 原理 | 适合 | 放大倍数 |
| --- | --- | --- | --- |
| FSR 4K 升频（默认） | AMD FidelityFX Super Resolution 1：EASU 边缘自适应放大，再用 RCAS 锐化 | 游戏和各类视频 | 720p / 1080p 到 4K；标清信号源最多 3 倍 |
| Anime4K 动画专用 4K 升频 | Anime4K ×2 神经网络（桌面版用 M、Quest 版用 S），剩余倍数由 FSR 补足 | 动画、卡通、线稿 | 神经网络放大 2 倍，再到 4K |
| Lanczos + CAS 4K 升频 | Lanczos-3 重采样（每轴 6 抽头），再用 AMD FidelityFX CAS 锐化 | 经典、中性的缩放 | 到 4K |
| NIS 4K 升频 | NVIDIA Image Scaling 1.0.3：沿边缘方向的 6 抽头缩放加自适应锐化 | 清晰细节 | 最多 2 倍，超出部分由 Lanczos-3 完成 |

只有新画面到来时才会重新升频。如果 Quest 不支持半精度浮点渲染目标，Anime4K 会自动改用 FSR。

- **复古画面**（`C`）：鼠标移到 Dock 按钮上可以选择“关闭”“CRT”“VHS”或“街机”；按 `C` 依次切换，点击按钮则在“关闭”和上次使用的效果之间切换，“设置 → 画面 → 复古画面”中也有同样的选项。效果在 4K 升频之后处理，因此可以和升频同时使用，截图、录像和 Quest 巨幕上都能看到；所选效果会自动保存。
  - **CRT**：显像管电视——交错排列的槽形荫罩、随亮度加深的扫描线、微微弯曲的屏幕和柔和暗角。
  - **VHS**：家用录像带——画质略微柔和、色彩向右晕开、轻微褪色偏暖、黑位略微发灰，带有淡淡的噪点和轻微晃动，偶尔还有一条淡淡的跟踪干扰带从上往下飘过（不影响阅读）。
  - **街机**：街机机台显示器——低分辨率扫描、粗黑扫描线、竖条 RGB 光栅、亮部微微发光、色彩更鲜明，再加上轻微弧面。
- **3D 景深**（`D`）：估算画面中物体的远近。鼠标移到按钮上（或打开“设置 → 3D 景深”）可以选择**景深格式**并调节**效果强度**。“景深效果（仅 2D 增强）”让前景更清晰鲜艳、背景更柔和朦胧；桌面版的立体 3D 格式见下一节；在 Quest 沉浸模式中选择“立体 3D 巨型屏幕”，巨幕就会变成真正的立体 3D。按 `D` 或点击按钮可在“无景深”和上次使用的格式之间切换。
- **亮度、对比度、饱和度**（设置 → 画面）：−40～+40，0 为原始画面。**伽马**：0.70～1.40（1.00 为原始画面），大于 1.00 提亮中间调，小于 1.00 压暗中间调，黑白两端保持不变。**风格**预设有“自然”“写实”（略带 S 曲线的自然对比和克制的色彩，暗部细节依然保留）“鲜艳”“电影”和“明亮房间”（伽马略微调高，在明亮的房间里也能看清暗场景）。
- **镜像**（`X`）：整个画面左右翻转。

## 立体 3D 格式（Linux / Windows）

桌面版的景深格式菜单（3D 景深按钮或“设置 → 3D 景深”）还提供四种立体格式，可以把真正的立体 3D 画面输出到 3D 显示设备。请把显示设备设为相同的 3D 格式并切换到全屏（`F`）；截图和录像仍是 2D。应用会自动识别文字和 HUD，让它们保持在屏幕平面附近，景深过渡也更平滑，同一个字在左右眼中整体移动，不会再像被切开一样。

| 格式 | 适用于 |
| --- | --- |
| 左右并排・全宽（3840×1080） | 3D 模式下的 XR 眼镜，例如 XREAL、VITURE、RayNeo |
| 左右并排・半宽 | 3D 电视 |
| 上下 | 3D 电视和投影机 |
| 红蓝眼镜 | 红蓝 3D 眼镜 |

## 摄像头叠加、截图与录像

所见即所得：画面效果、镜像和摄像头叠加画面都会一并写入截图和录像。

- **摄像头叠加**（`W`）：把摄像头画面叠加在游戏画面四个角落之一（默认左下角），大小可选 S、M、L（画面宽度的 18%、26%、36%）。
  - 可以拖到任意位置，松开后会自动滑到最近的角落；在叠加画面上滚动鼠标滚轮可调整大小。
  - 鼠标悬停在叠加画面上会出现小工具栏：“镜像”“大小”“移除”。
  - 鼠标移到 Dock 按钮上（或打开“设置 → 摄像头叠加”）可以设置“摄像头”“画质”（720p · 30 fps、1080p · 30 fps 或 1080p · 60 fps）“大小”“角落”“镜像”，以及 **交换画面**：摄像头变为主画面，采集卡变为叠加画面。
- **截图**（`S`）：把当前画面保存为 PNG。
- **录像**（`R`）：MP4 格式，H.264 视频加 192 kbit/s AAC 音频。码率随视频流自动计算（宽 × 高 × 帧率 × 0.12，范围 8～50 Mbit/s：1080p60 约 15 Mbit/s，4K60 为 50 Mbit/s）。录像时连接状态会显示 REC 和已录制时长。
  - **Windows 版**：不附带软件编码器。录像需要 NVIDIA（NVENC）、AMD（AMF）或 Intel 显卡的硬件编码器，或 Windows 自带的 H.264 编码器（Media Foundation；Windows“N”版需先安装 Media Feature Pack）。
  - **Linux 版**：同样不附带软件编码器。录像需要通过 VA-API 的 Intel 或 AMD 显卡（需安装 VA-API 驱动）、NVIDIA NVENC（官方闭源驱动）或 AMD AMF 其中之一。
  - 都没有的话其他功能照常使用，只是点击录像时会提示“没有视频编码器：录像需要 NVIDIA、AMD 或 Intel 显卡”。
- **录制麦克风**（`M`）：把你的声音和游戏声音混在一起录制。
- **文件名与保存位置**：文件名为 `UVC Studio 2026-10-06 21.30.15.png`／`.mp4`（日期和时间；重名时自动加编号）。截图保存到 **图片／UVC Studio**，录像保存到 **视频／UVC Studio**，并弹出“已保存到 Pictures/UVC Studio”通知和 **查看** 按钮；“设置 → 常规”里也有“打开截图文件夹”和“打开录像文件夹”。Quest 上保存到图库（`Pictures/UVC Studio` 和 `Movies/UVC Studio`），可在“文件”应用中或通过 USB 查看。

## Meta Quest

把采集卡插到头显的 USB-C 接口上，就能在可调大小的窗口或巨大的虚拟屏幕上玩游戏，还能看真正的立体 3D。

- **安装**：从 Meta Horizon Store 安装（即将上架），或在头显开启开发者模式后，从本项目的 Releases 下载 `UVCStudio-Quest-v1.1.0-Free.apk` 安装（Meta Quest Developer Hub、SideQuest 或 `adb install`）。白金只能在从商店安装的版本中购买。
- **连接**：也可以通过 USB-C OTG 转接头或扩展坞连接。接入采集卡时如果询问是否打开 UVC Studio，请允许（勾选“始终”），并授予 Android 使用 USB 采集卡所需的相机和麦克风权限。应用从不使用头显自身的摄像头。
- **解码**：H.264 和 HEVC 采集卡由头显内置的硬件解码器解码；应用自带的 FFmpeg 只用于 MJPEG。
- **2D 窗口**（默认）：与桌面版相同的完整界面，显示在可移动、可调大小的 Horizon OS 面板中。在画面上快速捏合两次可进入或退出全屏。
- **沉浸模式**：Dock 末尾（设置之后）的头显按钮可在巨幕和窗口之间切换。切换时 UVC Studio 会自动断开连接，到另一侧后再自动连接。
- **Quest 上的 3D 景深**：2D 窗口可选“无景深”或“景深效果”；沉浸模式可选“无景深”或“立体 3D 巨型屏幕”。

| 沉浸模式操作 | 方法 |
| --- | --- |
| 点击控制面板 | 用手柄指向并扣动扳机，或用手捏合 |
| 移动控制面板 | 抓住面板下方的白色横条拖动，向前推或向后拉可调整距离；“设置 → VR → 重置面板位置”可恢复原位 |
| 移动屏幕 | 抓住屏幕下方的白色横条拖动；手向前推或向后拉可调整距离 |
| 调整屏幕大小 | 指向角落直到出现角框，然后捏住拖动 |
| 拉近 / 推远、缩小 / 放大 | 摇杆上 / 下、左 / 右 |
| 隐藏或显示控制面板 | 在屏幕上捏合两次，或按 B、Y、菜单键 |
| 重新居中 | 长按 Meta 键 |
| 屏幕预设 | 设置 → VR：背景（透视或暗室）、屏幕形状（平面或曲面）、屏幕大小、屏幕距离、刷新率 72 / 90 / 120 Hz（默认 90） |

- **系统菜单**：打开时应用会继续绘制，同时隐藏指针并静音。
- **降低发热**：升频只做到巨幕实际能显示的分辨率（每度约 30 像素，最宽 2560），左右眼画面也按同样大小渲染。
- **过热保护**：同时监控 Android 温度状态和 OpenXR 温度通知。头显变热时改用 FSR 和 72 Hz；过热时暂停升频和 3D 景深。不会改动你的设置，降温后会弹出通知。
- **负载显示**：连接时，“设置 → 关于”显示每帧 GPU 耗时、处理分辨率和头显温度。

## 界面

整个界面采用 Liquid Glass 风格：半透明的磨砂玻璃面板，连接时会透出背后游戏画面经过模糊、增强饱和度后的效果。

- **顶栏**：左侧是 UVC Studio 字标；中间是**连接状态**（“未连接”；绿点加设备 · 分辨率 · 帧率，升频时附加 ▸ 4K 和升频方式，开启 3D 景深时附加 · 3D；录像时是闪烁的红色 REC 和录制时长）；右侧是语言菜单和 **连接／断开连接** 按钮。错误信息会以红字在连接状态下方显示几秒钟。
- **Dock**：分为四组——“截图、录像、录制麦克风”·“4K 升频、复古画面、3D 景深、镜像”·“摄像头叠加、监听”·“设置”。鼠标指向的按钮会放大，已开启的功能以品牌渐变色点亮，每个提示都会显示快捷键。版本号在左下角，全屏按钮在右下角。
- **设置**：一块大玻璃面板，卡片最多分三列均衡排布，所有选项无需滚动即可调整：来源、声音、画面、摄像头叠加、显示、VR（Quest 沉浸模式）、常规、3D 景深、关于——自动布局时，“3D 景深”卡片紧挨在“关于”正上方。“关于”显示版本号，连接时还显示每帧 GPU 耗时和处理分辨率（Quest 上另有头显温度），以及“开源许可”按钮；隐私说明位于该按钮打开的页面中。
  - **收起卡片**：每张卡片标题栏最右边、锁形图标右侧有一个圆形箭头按钮，点一下卡片就收起成只剩标题栏（名称、锁形图标和折叠按钮保留），再点一下展开。下方卡片会上移，卡片不会换列，收起状态也会记住。
  - **自己排列卡片**：每张卡片标题栏的右上角都有一个锁形图标，所有卡片默认锁定。点击锁形图标（“解锁以移动”），再按住卡片标题栏，就能把它拖到三列中的其他位置；界面上会出现一个空位，标示松开后的落点，按 `Esc` 取消拖动。再次点击锁形图标（“锁定位置”）即可固定。锁定的卡片保持原位，解锁的卡片在它们之间移动。排列和解锁的卡片都会被记住，下次启动依然有效。使用自定义布局时，关闭按钮旁会出现“重置布局”，点击即可恢复自动布局并锁定所有卡片。窗口太窄、放不下三列时会显示自动布局，你的排列仍会保留，窗口拉宽后即恢复。Quest 的 2D 窗口用手柄指针也能同样操作。
- **语言**：台灣正體中文、English、Deutsch、Français、Italiano、Español、Português、日本語、한국어、简体中文、ไทย、Tiếng Việt，可在右上角的玻璃菜单中选择；首次启动时跟随系统语言。
- **主题**：自动（06:00～18:00 为浅色）、深色或浅色。
- **界面大小**：根据屏幕自动调整（1080p 100%、1440p 约 135%、4K 200%）；也可以在“设置 → 显示”中或用 `Ctrl` `+` / `−` / `0` 手动调整。
- **字体与图标**：Manrope 和 IBM Plex Sans（泰文使用 IBM Plex Sans Thai，中日韩文字使用 Noto Sans CJK TC）；图标全部由 UVC Studio 原创绘制。
- **全屏**：`F`、`F11`、右下角按钮或双击画面；按 `Esc` 退出。

## 键盘快捷键（Linux / Windows）

在应用中按 `?` 或 `H` 会显示同样的快捷键一览，按任意键或点击即可关闭。

| 按键 | 功能 |
| --- | --- |
| `Enter` | 连接 |
| `Ctrl+D` | 断开连接 |
| `S` | 截图 |
| `R` | 开始或停止录像 |
| `M` | 开关录制麦克风 |
| `L` | 开关监听 |
| `W` | 开关摄像头叠加 |
| `X` | 镜像 |
| `U` | 开关 4K 升频 |
| `Shift+U` | 下一个升频方式（同时开启升频） |
| `C` | 复古画面：关闭 → CRT → VHS → 街机 |
| `D` | 3D 景深：在“无景深”和上次使用的格式之间切换 |
| `O` 或 `Ctrl+,` | 设置 |
| `F` 或 `F11` | 全屏 |
| `Esc` | 取消拖动卡片、退出全屏或关闭设置 |
| `?` 或 `H` | 键盘快捷键 |
| `Ctrl` `+` / `−` / `0` | 界面放大 / 缩小 / 还原 |

## 隐私

UVC Studio 完全在你的设备上运行。画面、声音和录像都不会离开这台设备——无需账号、不上传、不跟踪，应用也从不联网。Meta Quest 版购买白金与确认购买记录，都通过头显的 Meta Horizon Store 服务处理；UVC Studio 本身不发送任何数据。

## 开源许可

UVC Studio 的 Linux 与 Windows 版以 [MIT 许可证](LICENSE)开源；Meta Quest 版不开源。UVC Studio 不包含任何 GPL 组件；FFmpeg 采用纯 LGPL 配置编译（Quest 版只包含 MJPEG 解码器，并以可替换的动态库形式提供）。其他组件：SDL2、Dear ImGui、miniaudio、stb、NanoSVG、libusb 和 libuvc（Quest）、OpenXR 加载器、AMD FidelityFX FSR 1 和 CAS、Anime4K、NVIDIA Image Scaling。字体（Manrope、IBM Plex Sans、IBM Plex Sans Thai、Noto Sans CJK TC）采用 SIL Open Font License。所有组件及完整许可证文本都列在应用的“设置 → 关于 → 开源许可”中。Meta Quest 版另含 Meta Platform SDK 加载程序（Oculus SDK License），用于商店购买。
