# UVC Studio

**English** · [台灣正體中文](README.zh-TW.md) · [Deutsch](README.de.md) · [Français](README.fr.md) · [Italiano](README.it.md) · [Español](README.es.md) · [Português](README.pt.md) · [日本語](README.ja.md) · [한국어](README.ko.md) · [简体中文](README.zh-CN.md) · [ไทย](README.th.md) · [Tiếng Việt](README.vi.md)

UVC Studio 1.1.0 is a native, fully offline capture-card app for Linux, Windows and Meta Quest: play consoles through any UVC capture card with low latency, upscale to 4K, add a retro look or 3D depth, put your webcam over the game, record and take snapshots — without ever touching the network. 1.1.0, the first official release, brings UVC Studio Platinum to Meta Quest: 4K upscaling and 3D depth stay free for 30 minutes a day each, and a one-time purchase unlocks them for good; the Linux and Windows versions stay free and are now open source under the MIT License.

![UVC Studio playing a game with 4K upscaling](docs/screenshots/02-playing-4k.png)

## Platforms and downloads

One codebase, the same features and the same version number on every platform.

| Platform | Download | Video capture | Audio | Graphics | Recording |
| --- | --- | --- | --- | --- | --- |
| Linux (x86_64) | [`UVCStudio-v1.1.0-x86_64.AppImage`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-v1.1.0-x86_64.AppImage) | Video4Linux2 | PulseAudio / PipeWire / ALSA | OpenGL 3.3 | H.264 (VA-API / NVENC / AMF hardware) + AAC, MP4 — needs one of these |
| Windows (x64) | [`UVCStudio-Setup-v1.1.0.exe`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-Setup-v1.1.0.exe) (installer) or [`UVCStudio-Portable-v1.1.0.zip`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-Portable-v1.1.0.zip) (portable) | Media Foundation | WASAPI | OpenGL 3.3 | H.264 (NVENC / AMF / Intel via Media Foundation, else Windows' built-in encoder) + AAC, MP4 — needs one of these |
| Meta Quest 2 / Pro / 3 / 3S | [`UVCStudio-Quest-v1.1.0-Free.apk`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-Quest-v1.1.0-Free.apk) (sideload) or Meta Horizon Store (coming soon) | USB host, libuvc | The card's own USB audio, read directly (stereo or 5.1) | OpenGL ES 3 (2D window), OpenXR (immersive) | H.264 + AAC (hardware MediaCodec), MP4 |

Source code (Linux and Windows, MIT License): this repository, or [`UVCStudio-v1.1.0-src.tar.gz`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-v1.1.0-src.tar.gz).

On Meta Quest, 4K upscaling and 3D depth are free for 30 minutes a day each; UVC Studio Platinum, a one-time in-app purchase, makes them unlimited. On Linux and Windows every feature is free.

## Screenshots

Linux at 1920 × 1080; click a picture to see it full size. The Meta Quest 2D window uses the same interface.

<table>
<tr><td width="50%"><a href="docs/screenshots/01-ready.png"><img src="docs/screenshots/01-ready.png" alt="Ready screen: capture card, game audio and webcam checked live"></a><br><sub>Ready screen: capture card, game audio and webcam checked live</sub></td><td width="50%"><a href="docs/screenshots/03-4k-upscaling.png"><img src="docs/screenshots/03-4k-upscaling.png" alt="4K upscaling: four upscalers to choose from"></a><br><sub>4K upscaling: four upscalers to choose from</sub></td></tr>
<tr><td width="50%"><a href="docs/screenshots/04-settings.png"><img src="docs/screenshots/04-settings.png" alt="Settings: everything on one glass panel"></a><br><sub>Settings: everything on one glass panel</sub></td><td width="50%"><a href="docs/screenshots/05-webcam-overlay.png"><img src="docs/screenshots/05-webcam-overlay.png" alt="Webcam overlay over the game"></a><br><sub>Webcam overlay over the game</sub></td></tr>
<tr><td width="50%"><a href="docs/screenshots/06-retro-vhs.png"><img src="docs/screenshots/06-retro-vhs.png" alt="Retro look: VHS"></a><br><sub>Retro look: VHS</sub></td><td width="50%"><a href="docs/screenshots/07-3d-depth.png"><img src="docs/screenshots/07-3d-depth.png" alt="3D depth"></a><br><sub>3D depth</sub></td></tr>
<tr><td width="50%"><a href="docs/screenshots/08-shortcuts.png"><img src="docs/screenshots/08-shortcuts.png" alt="Keyboard shortcuts"></a><br><sub>Keyboard shortcuts</sub></td><td width="50%"><a href="docs/screenshots/09-light-theme.png"><img src="docs/screenshots/09-light-theme.png" alt="Light theme"></a><br><sub>Light theme</sub></td></tr>
<tr><td width="50%"><a href="docs/screenshots/10-zh-TW.png"><img src="docs/screenshots/10-zh-TW.png" alt="Interface in 台灣正體中文"></a><br><sub>Interface in 台灣正體中文</sub></td><td width="50%"><a href="docs/screenshots/11-languages.png"><img src="docs/screenshots/11-languages.png" alt="12 interface languages"></a><br><sub>12 interface languages</sub></td></tr>
</table>

## Getting started

1. Plug the capture card into USB and start UVC Studio. The **ready screen** checks three things live: *Capture card*, *Game audio* and *Webcam (optional)*. A missing card shows *Not found — plug it into USB*; missing audio shows *Not found (video only)*.
2. Press **Connect** (on the ready screen or at the top right) or hit `Enter`. With **Connect automatically** on (Settings → General, on by default), UVC Studio connects by itself as soon as a card is plugged in, and at launch when one is already there.
3. Play. **Disconnect** (same button, or `Ctrl+D`) closes everything; a running recording is finished and saved first.

There is no pause or resume: UVC Studio is either connected or not. If the card is unplugged, an error appears under the connection chip and the app goes back to the ready screen; with auto-connect on it reconnects when the card returns.

## Capture and sound

Any UVC capture card works without extra drivers: ShadowCast, Elgato, AVerMedia, Magewell and generic USB 3 HDMI dongles behave the same way.

- **Exact card modes**: resolutions and frame rates come from what the card reports, listed largest first and tagged 4K, QHD, Full HD, HD or SD. By default UVC Studio picks the largest resolution that reaches 59 fps or more. Format: Auto, Uncompressed (least delay) or Compressed (higher resolutions and frame rates).
- **Picks the right devices**: with nothing saved, the capture card, its audio and the webcam are chosen by name, so built-in and virtual cameras are not mistaken for the card. Devices with identical names get (2), (3)… added.
- **Low latency**: the newest frame is always shown, never a queue of old ones.
- **Remembers your setup** and detects cards plugged in or removed while the app runs.
- **Listen** (`L`): hear the game through your speakers or headphones. Hover the dock tile for an on/off switch, the volume and the current output; the output line leads to Settings → Sound.
- **Live mode readout**: the actual stream (for example 3840 × 2160 · 60 fps · YUY2) is shown in Settings → Source.
- **Meta Quest audio**: the app reads the card's sound straight from USB (USB Audio Class 1 and 2, including MacroSilicon MS2109 dongles) — Settings → Sound → Audio input → *Capture card audio (USB)*. Settings → Sound → *Game audio channels*: **Stereo 2CH** (default) or **Surround 6CH** — 5.1 from cards with a 6-channel mode (such as the CJ-16) and consoles that output 5.1 over HDMI (such as Switch 2 set to Surround); recordings stay stereo.
- **Self-recovery on Quest**: if the card stops sending pictures, the app reopens it automatically (up to twice), then asks you to check the HDMI source or replug the card.

## Picture

Four 4K upscalers, three retro looks and 3D depth all run on the GPU, frame by frame, adding no delay beyond the GPU time.

**4K upscaling** (`U`): click the dock tile or press `U` to switch it on or off, hover the tile to pick a mode, or press `Shift+U` for the next one. Settings → Picture → 4K upscaling offers *Off*, *FSR*, *Anime4K*, *Lanczos* and *NIS*: picking a mode turns upscaling on, and *Off* keeps your last mode for next time.

| Mode | How it works | Best for | Scale |
| --- | --- | --- | --- |
| FSR 4K upscaling (default) | AMD FidelityFX Super Resolution 1: edge-adaptive EASU upscaling plus RCAS sharpening | Games and any video | 720p / 1080p to 4K; SD sources up to 3× |
| Anime4K upscaling for animation | Anime4K ×2 neural network (M size on desktop, S on Quest), then FSR for any remaining step | Anime, cartoons, line art | 2× by the network, then to 4K |
| Lanczos + CAS 4K upscaling | Lanczos-3 resampling (6 taps per axis) plus AMD FidelityFX CAS sharpening | Classic, neutral scaling | To 4K |
| NIS 4K upscaling | NVIDIA Image Scaling 1.0.3: edge-directed 6-tap scaler with adaptive sharpening | Crisp detail | Up to 2×, Lanczos-3 beyond |

The upscaler only re-runs when a new frame arrives. If the Quest lacks half-float render targets, Anime4K falls back to FSR.

- **Retro look** (`C`): hover the dock tile and pick *Off*, *CRT*, *VHS* or *Arcade*; `C` steps through them in that order, and a click on the tile switches between *Off* and the last look you used. Settings → Picture → Retro look offers the same choice. The look is drawn on the finished picture, after 4K upscaling, so the two work together, and it shows in snapshots, recordings and on the Quest giant screen. Your choice is remembered.
  - **CRT**: a tube TV — a staggered slot mask, scan lines that darken with brightness, a gently curved screen and a soft vignette.
  - **VHS**: a home video tape — a softer picture, colour bleeding to the right, slightly faded warm tones with lifted blacks, faint noise, a gentle line wobble and, now and then, a faint tracking band drifting down the screen (text stays readable).
  - **Arcade**: an arcade cabinet monitor — a low-resolution raster with bold dark scanlines, a vertical RGB aperture grille, a soft glow around bright areas, punchy colour and a slight curve.
- **3D depth** (`D`): estimates what is near and far in the picture. Hover the tile, or open Settings → 3D depth, for the **Depth format** and the **Effect strength**. *Depth effect (2D enhancer only)* makes the foreground crisper and more vivid and the background softer and hazier; the stereo formats for 3D displays are described below; in Quest immersive mode, *Stereo 3D giant screen* turns the giant screen into real stereo 3D. `D` or a click on the tile switches between *No depth* and the last format you used.
- **Brightness, contrast, saturation** (Settings → Picture): −40 to +40, with 0 as neutral. **Gamma**: 0.70 to 1.40 (1.00 is neutral); above 1.00 brightens the mid-tones, below 1.00 deepens them, and black and white stay put. **Looks** presets: *Natural*, *Realism* (natural contrast with a gentle S-curve and restrained colour, keeping detail in the shadows), *Punchy*, *Film* and *Bright room* (now with a touch more gamma, so dark scenes stay visible in a bright room).
- **Mirror** (`X`): flips the whole picture left to right.

## Stereo 3D formats (Linux / Windows)

On the desktop, the depth format menu (on the 3D depth tile or in Settings → 3D depth) also offers four stereo formats that send real stereo 3D to a 3D display. Set the display to the same 3D format and go fullscreen (`F`); snapshots and recordings stay 2D. Text and HUD elements are detected and kept close to the screen plane, and the depth is smoothed, so letters move as one piece in both eyes instead of looking cut apart.

| Format | For |
| --- | --- |
| Side by side, full (3840×1080) | XR glasses in 3D mode, such as XREAL, VITURE and RayNeo |
| Side by side, half | 3D TVs |
| Top and bottom | 3D TVs and projectors |
| Red / cyan glasses | Red / cyan anaglyph glasses |

## Webcam overlay, snapshots and recording

What you see is what gets saved: effects, mirror and the webcam overlay are baked into every snapshot and recording.

- **Webcam overlay** (`W`): your webcam over the game, in one of the four corners (bottom left by default) at size S, M or L (18, 26 or 36 % of the picture width).
  - Drag it anywhere; when you let go it glides into the nearest corner. The mouse wheel over it changes the size.
  - Hover it for a small toolbar: *Mirror*, *Size* and *Remove*.
  - Hover the dock tile (or open Settings → Webcam overlay) for *Camera*, *Quality* (720p · 30 fps, 1080p · 30 fps or 1080p · 60 fps), *Size*, *Corner*, *Mirror* and **Swap pictures**, which makes the webcam the main picture and the capture card the overlay.
- **Snapshot** (`S`): a PNG of the current picture.
- **Record** (`R`): MP4 with H.264 video and 192 kbit/s AAC audio. The bitrate follows the stream (width × height × fps × 0.12, between 8 and 50 Mbit/s: about 15 Mbit/s for 1080p60, 50 Mbit/s for 4K60). The connection chip shows REC and the elapsed time.
  - **Windows:** no software encoder is bundled. Recording needs an NVIDIA (NVENC), AMD (AMF) or Intel graphics encoder, or the H.264 encoder built into Windows (Media Foundation; Windows "N" editions need the Media Feature Pack).
  - **Linux:** no software encoder is bundled either. Recording needs Intel or AMD graphics through VA-API (with the VA-API driver installed), NVIDIA NVENC (proprietary driver) or AMD AMF.
  - Without one of these, everything else works, but Record reports *No video encoder: recording needs NVIDIA, AMD or Intel graphics*.
- **Record microphone** (`M`): mixes your voice with the game audio.
- **File names and folders**: `UVC Studio 2026-10-06 21.30.15.png` / `.mp4` (date and time; a number is added if the name exists). Snapshots go to **Pictures/UVC Studio**, recordings to **Videos/UVC Studio**, with a *Saved to Pictures/UVC Studio* notice and a **Show** button; Settings → General has *Open snapshots folder* and *Open recordings folder*. On Quest, files go to the gallery (`Pictures/UVC Studio` and `Movies/UVC Studio`), visible in the Files app and over USB.

## Meta Quest

Plug the capture card into the headset's USB-C port and play on a resizable window or a giant virtual screen, with real stereo 3D.

- **Install**: from the Meta Horizon Store (coming soon), or install `UVCStudio-Quest-v1.1.0-Free.apk` from this project's Releases with the headset in developer mode (Meta Quest Developer Hub, SideQuest or `adb install`). Platinum can be bought only in the version installed from the store.
- **Connect**: a USB-C OTG adapter or hub works too. Accept "Open UVC Studio when this device is connected?" (tick Always), plus the camera and microphone permissions Android requires for USB capture cards. The headset's own cameras are never used.
- **Decoding**: H.264 and HEVC cards are decoded by the headset's built-in hardware decoder; the bundled FFmpeg is used for MJPEG only.
- **2D window** (default): the full desktop interface in a Horizon OS panel you can move and resize. Double pinch the picture to enter or leave fullscreen.
- **Immersive mode**: the headset tile at the end of the dock, after Settings, switches to a giant virtual screen and back. UVC Studio disconnects, switches and connects again on the other side by itself.
- **3D depth on Quest**: in the 2D window the depth format is *No depth* or *Depth effect*; in immersive mode it is *No depth* or *Stereo 3D giant screen*.

| In immersive mode | How |
| --- | --- |
| Click the control panel | Point with a controller and pull the trigger, or pinch with your hand |
| Move the control panel | Grab the white bar under it and drag; push or pull to change the distance. Settings → VR → *Reset panel position* puts it back |
| Move the screen | Grab the white bar under it and drag; push your hand away or pull it in to change the distance |
| Resize the screen | Point at a corner until a bracket appears, then pinch and drag |
| Closer / farther, smaller / bigger | Thumbstick up / down, left / right |
| Hide or show the control panel | Double pinch on the screen, or press B, Y or Menu |
| Re-centre | Long-press the Meta button |
| Screen presets | Settings → VR: background (passthrough or dark room), screen shape (flat or curved), screen size, screen distance, refresh rate 72 / 90 / 120 Hz (default 90) |

- **System menu**: while it is open the app keeps drawing, hides its pointers and mutes the sound.
- **Cooler running**: upscaling stops at what the giant screen can actually show (about 30 pixels per degree, at most 2560 wide), and the stereo pair is rendered at that same size.
- **Overheat guard**: watches Android's thermal status and OpenXR thermal notifications. When the headset gets warm it switches to FSR and 72 Hz; when it gets hot it pauses upscaling and 3D depth. Your settings are not changed, and a notice says when it has cooled down.
- **Load readout**: while connected, Settings → About shows the GPU time per frame, the processing resolution and the headset temperature.

## Interface

The whole interface uses a Liquid Glass style: frosted, translucent surfaces that, while connected, show a blurred and colour-boosted copy of the game behind them.

- **Top bar**: the UVC Studio wordmark on the left; the **connection chip** in the centre (*Not connected*; a green dot with device · resolution · fps, plus ▸ 4K and the upscaler, and · 3D; or a pulsing red REC with the recording time); the language menu and **Connect / Disconnect** on the right. Errors appear in red under the chip for a few seconds.
- **Dock**: four groups — *Snapshot, Record, Record microphone* · *4K upscaling, Retro look, 3D depth, Mirror* · *Webcam overlay, Listen* · *Settings*. Tiles magnify under the pointer, active features light up in the brand gradient, and every tooltip shows the shortcut key. The version sits at the bottom left, Fullscreen at the bottom right.
- **Settings**: one large glass panel with cards in up to three balanced columns, so everything is adjustable without scrolling: Source, Sound, Picture, Webcam overlay, Display, VR (Quest immersive mode), General, 3D depth and About — in the automatic layout the 3D depth card sits directly above About. About shows the version, the GPU time per frame and the processing resolution while connected (plus the headset temperature on Quest) and the *Open-source licenses* button; the privacy note is in the sheet that button opens.
  - **Collapse a card**: the round chevron button at the far right of every card header, right of the padlock, folds the card down to its header (the title, padlock and fold button stay) and opens it again. The other cards slide up, nothing changes column, and collapsed cards are remembered.
  - **Arrange the cards yourself**: every card has a padlock at the top right of its header, and all cards start locked. Click the padlock (*Unlock to move*), then drag the card by its header to another place in the three columns; a slot shows where it will land, and `Esc` cancels the drag. Click the padlock again (*Lock position*) to fix the card in place. Locked cards stay where they are, and unlocked cards move between them. The arrangement and the unlocked cards are remembered for the next launch. *Reset layout*, next to the close button and shown only while your own layout is in use, restores the automatic layout and locks every card. In a window too narrow for three columns the automatic layout is shown, and your arrangement is kept for when the window is wide again. On Quest the 2D window works the same way with the controller pointer.
- **Languages**: 台灣正體中文, English, Deutsch, Français, Italiano, Español, Português, 日本語, 한국어, 简体中文, ไทย and Tiếng Việt, picked from the glass menu at the top right. The first launch follows your system language.
- **Themes**: Auto (light from 06:00 to 18:00), Dark or Light.
- **Interface size**: follows the screen (1080p 100%, 1440p about 135%, 4K 200%); override it in Settings → Display or with `Ctrl` `+` / `−` / `0`.
- **Fonts and icons**: Manrope and IBM Plex Sans (IBM Plex Sans Thai for Thai, Noto Sans CJK TC for Chinese, Japanese and Korean); the icons are UVC Studio's own.
- **Fullscreen**: `F`, `F11`, the bottom-right button or double-click the picture; `Esc` leaves it.

## Keyboard shortcuts (Linux / Windows)

Press `?` or `H` in the app for the same list as a shortcut sheet; any key or click closes it.

| Key | Action |
| --- | --- |
| `Enter` | Connect |
| `Ctrl+D` | Disconnect |
| `S` | Snapshot |
| `R` | Start or stop recording |
| `M` | Record microphone on / off |
| `L` | Listen on / off |
| `W` | Webcam overlay on / off |
| `X` | Mirror |
| `U` | 4K upscaling on / off |
| `Shift+U` | Next upscaler (turns upscaling on) |
| `C` | Retro look: Off → CRT → VHS → Arcade |
| `D` | 3D depth: No depth ⇄ the last depth format |
| `O` or `Ctrl+,` | Settings |
| `F` or `F11` | Fullscreen |
| `Esc` | Cancel a card drag, leave fullscreen or close Settings |
| `?` or `H` | Keyboard shortcuts |
| `Ctrl` `+` / `−` / `0` | Interface bigger / smaller / reset |

## Privacy

UVC Studio works entirely on your device. Picture, sound and recordings never leave it — no account, no upload, no tracking, and the app never touches the network. On Meta Quest, buying Platinum and checking the purchase go through the headset's Meta Horizon Store service; UVC Studio itself sends nothing.

## Licences

UVC Studio for Linux and Windows is open source under the [MIT License](LICENSE); the Meta Quest version is not open source. UVC Studio contains no GPL components; FFmpeg is built LGPL-only (on Quest as replaceable shared libraries with the MJPEG decoder only). Other components: SDL2, Dear ImGui, miniaudio, stb, NanoSVG, libusb and libuvc (Quest), the OpenXR loader, AMD FidelityFX FSR 1 and CAS, Anime4K and NVIDIA Image Scaling. The fonts (Manrope, IBM Plex Sans, IBM Plex Sans Thai, Noto Sans CJK TC) are under the SIL Open Font License. Every component and its full licence text is listed in the app under Settings → About → Open-source licenses. The Meta Quest version also includes the Meta Platform SDK loader (Oculus SDK License) for the store purchase.
