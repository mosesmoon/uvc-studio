# UVC Studio — interface and behaviour specification (v1.1.0)

This document is the single source for how UVC Studio's session, devices,
overlay, picture controls and interface behave. It is written from UVC
Studio's own product goals. Implementations must follow this document; they
must not be derived from any other application's code, markup, stylesheets,
strings or icons.

Unchanged and out of scope (keep as they are): capture back-ends
(V4L2 / Media Foundation / libuvc), decoders, the recorder's encoder
selection, audio engine internals, the four 4K upscalers (FSR, Anime4K,
Lanczos + CAS, NIS) and their popover, Quest immersive mode / OpenXR /
thermal guard, the Liquid Glass
`glass()` helper and backdrop blur, the language menu, theme (auto / dark /
light), interface size, toasts, the dock's magnify-on-hover and click
bounce, the About sheet with open-source licences.

## 1. Vocabulary

| Term | Meaning |
|---|---|
| Source | The capture card (main video + its audio). |
| Overlay | The webcam picture placed over the source ("webcam overlay"). |
| Connected / Disconnected | Whether UVC Studio is capturing. |
| Listen | Playing the game audio through the computer's output (monitoring). |

## 2. Session model

Two states only: **Disconnected** and **Connected**.

- Disconnected → Connected: "Connect" (top-right button, `Enter`, or the
  Connect button on the ready screen). Opens the source video, starts game
  audio capture, starts listening if Listen is on, opens the overlay camera
  if the overlay is on, starts the mic if mic recording is on.
- Connected → Disconnected: "Disconnect" (same button, or `Ctrl+D`). Stops
  recording first if needed (the file is finalised and announced), then
  closes everything. There is no paused / primed intermediate state.
- **Auto-connect** (Settings → General, default **on**): while
  Disconnected, when a new likely capture device appears in the device list,
  connect automatically. Also connect at launch if a likely capture device is
  present and auto-connect is on.
- Changing source device, resolution, frame rate or pixel format while
  Connected reopens only the video stream (audio keeps running). Changing the
  game-audio input or output device restarts only audio. Each of these is an
  explicit call made by the control that changed the value (no per-frame
  "compare previous value" polling).
- If the source disappears while Connected: show the error, go to
  Disconnected; auto-connect (if on) reconnects when it returns.
- Quest: the 2D ⇄ VR switch disconnects, switches activity, and connects
  again on the other side (keep the existing host hook, renamed
  `reconnect()`).

## 3. Devices

### 3.1 Source (video) auto-pick
When no source is saved (or the saved one is missing), score every video
device by its name (case-insensitive) and pick the highest score; ties keep
list order:
- +4 if the name contains any of: `capture`, `hdmi`, `cam link`, `game`,
  `video grabber`, `usb video`, `uvc`, `4k`.
- −6 if it contains `virtual` or `loopback`, or (Linux) the driver is
  `v4l2 loopback`.
- −3 if it contains any of: `integrated`, `built-in`, `facetime`,
  `webcam`, `face`, `front`, `rear`, `ir camera`.
No brand-specific rules and no brand-specific frame rates.

### 3.2 Overlay camera auto-pick
Exclude the current source. Score: +4 for `webcam`, `camera`, `cam`,
`facetime`, `integrated`, `built-in`; −6 for `virtual`/`loopback`; −4 for
the capture-card words above. Highest wins.

### 3.3 Game audio input auto-pick
Keep the existing name-token matcher that pairs the capture card's audio
with its video (it is UVC Studio's own). Remove any brand-specific fallback.


### 3.4 Quest USB audio (capture card sound)
The app reads the card's USB Audio Class stream itself. *Stereo 2CH* picks
the 48 kHz 16-bit stereo mode; *Surround 6CH* picks a 6-channel mode when
the card has one (else stereo). Every packet is decoded on its own from a
frame boundary (a partial frame at the end is dropped), so a short or lost
packet can never shift the channels; only MS2109 dongles, which send one
unaligned byte stream, join frames across packets. Playback is a plain
AAudio stream (float, 48 kHz) with the card's channel count — 6 channels
are 5.1 in FL FR FC LFE SL SR order, which the headset places around the
listener. A 5.1 card in stereo mode is downmixed (L = FL + 0.707 FC +
0.707 SL, R likewise, LFE left out); recordings always get that stereo
downmix. Changing the setting reopens the card. If the headset's output
stream dies (rerouted, reset), it is reopened.
### 3.4 Duplicate names
When two devices share a name, append ` (2)`, ` (3)` … to the later ones.

## 4. Video mode

- Resolution menu lists exactly the resolutions the device reports, largest
  first, labelled `W × H` followed by a tag: ≥ 3840 wide "4K",
  2560 × 1440 "QHD", 1920 × 1080 "Full HD", 1280 × 720 "HD", 720 × 480 /
  720 × 576 "SD"; others show dimensions only.
- Frame-rate menu lists the rates the device reports for that resolution,
  highest first, as `60 fps` etc. (rounded).
- Default (and fallback when the stored mode is unsupported): the largest
  resolution that offers ≥ 59 fps; if none, the largest resolution at its
  highest rate. Applying uses the existing `chooseMode()` for the pixel
  format.
- Pixel format: Auto / Uncompressed / Compressed (segmented control).
- Upscale target: scale = min(3840 / w, 2160 / h); no upscale when
  scale < 1.05; never more than 4×.

## 5. Webcam overlay

- Placement is one of four **corners** (top-left, top-right, bottom-left,
  bottom-right; default **bottom-left**) plus a **size** S / M / L = 18 % /
  26 % / 36 % of the video's width. Margin from the video edge: 3 % of the
  video's width. Aspect 16:9; other camera aspects are centre-cropped to
  16:9.
- One function computes the overlay rectangle in normalised video
  coordinates (0…1) from corner, size and margin; the on-screen drawing, the
  recording/snapshot composition, the depth "flat" region and VR all use it.
- Interaction: drag the overlay anywhere; on release it animates (150 ms
  ease-out) to the nearest corner. Mouse wheel over the overlay steps the
  size. Hovering shows a small glass toolbar centred along the overlay's
  inner edge with three buttons: Mirror, Size (cycles S → M → L), Remove.
  Corners are 10 px rounded; a 1.5 px gradient outline while hovered.
- Mirror default **off**.
- Overlay settings (popover from its dock tile, and Settings → Sources):
  Camera, Quality (single select: "720p · 30 fps", "1080p · 30 fps",
  "1080p · 60 fps"), Size (segmented S/M/L), Corner (2×2 picker), Mirror
  (switch), "Swap pictures" (makes the camera the source and the source
  the overlay; only when both are video devices).
- Opening the overlay camera fails → error once, overlay switched off.

## 6. Picture controls

- Brightness, contrast, saturation: internal gains 0.60 … 1.40, shown as
  −40 … +40 (integer, 0 = neutral), step 1 (0.01 gain). Shader uniforms are
  named `u_gain`, `u_contrast_gain`, `u_sat_gain`.
- **Gamma**: 0.70 … 1.40, default 1.00, step 0.01, shown with two decimals
  ("1.00"). Applied last in the colour shader as c^(1 / gamma) (uniform
  `u_gamma`): above 1.00 lifts the mid-tones, below 1.00 deepens them;
  black and white stay where they are. Order in the shader: saturation
  (around Rec. 709 luma), contrast (around mid-grey), gain, gamma.
- Presets (chips, in this order; brightness, contrast, saturation, gamma):
  **Natural** (0, 0, 0, 1.00), **Realism** (0, +6, −8, 1.04 — a gentle
  S-curve feel: a little more contrast with the mid-tones lifted a touch so
  shadows keep their detail, and restrained colour), **Punchy** (+2, +12,
  +25, 1.00), **Film** (−3, +8, −20, 1.00), **Bright room** (+8, +5, +5,
  1.10 — the mid-tones lifted so dark scenes stay legible in a bright room).
  A chip is shown selected when all four values match it (gamma to two
  decimals).
- **Retro look**: Off, **CRT**, **VHS** or **Arcade**, drawn on the
  finished picture (after the colour controls and after any upscaler, so it
  combines with 4K upscaling and is what recordings, snapshots and the Quest
  giant screen show). The patterns are laid out for the size the picture is
  shown at (the stage, or ~1600 px across the VR screen).
  - CRT: shadow-mask style — a staggered 2 × 2 slot-mask cell, mask depth
    0.80, horizontal scanlines whose darkness follows brightness, barrel
    curvature with a soft vignette, no gamma change.
  - VHS: a home video tape on a TV — luma softened a little, chroma blurred
    much wider and offset to the right (colour bleeds past edges), slightly
    desaturated with a warm cast and lifted blacks, faint grain and chroma
    noise that change every frame, a sub-pixel per-line wobble, head
    switching that bends the bottom few lines, and now and then (an 11 s
    cycle) a faint tracking band drifting down. Animated from a clock but
    always readable.
  - Arcade: an arcade cabinet's low-resolution monitor — one sample per
    scanline (120 … 270 lines) with bold dark gaps whose width follows
    brightness, a vertical R/G/B aperture grille, a mild glow around bright
    pixels, punchier colour (more saturation and contrast), light curvature,
    rounded dark corners and a vignette.
- Mirror flips the whole picture horizontally (overlay keeps its own mirror).
- GPU passes of the picture chain restore the caller's OpenGL state
  (framebuffer, program, viewport) when they finish, so the interface is
  always drawn into the window.

### 6.1 4K upscaling
- On / off with the dock tile or `U`; `Shift+U` picks the next upscaler and
  turns upscaling on. The tile's hover popover lists Off and the four
  upscalers; Settings → Picture has the same choice as a segmented control
  (Off, FSR, Anime4K, Lanczos, NIS). Choosing an upscaler (popover or
  Settings) turns upscaling on; Off keeps the chosen upscaler for next time.

### 6.2 3D depth
- **Depth format** (`depth.format`): desktop — **No depth**, **Depth effect
  (2D enhancer only)**, **Side by side (full)**, **Side by side (half)**,
  **Top and bottom**, **Red / cyan glasses**. Quest 2D window — No depth /
  Depth effect; Quest immersive mode — No depth / **Stereo 3D giant screen**
  (the existing stereo pair on the giant screen). Only the options of the
  current mode are offered. The stereo formats replace the former
  "Settings → Display → 3D output", which is removed.
- 3D depth is on whenever the format is not No depth (the dock tile is
  active, the status chip adds " · 3D"). The tile and `D` toggle between No
  depth and the last format used (`depth.lastFormat`, default Depth effect).
- Effect strength 10 … 100 % (`depth.strength`, default 60 %): the 2D
  enhancer's strength and the stereo shift (largest per-eye shift 1.4 % of
  the width on the desktop; on Quest up to 2 %, capped by screen size).
- The dock tile's hover popover: heading "3D depth · D", the format
  dropdown and the strength slider (with its percentage); for a stereo
  format, the hint to set the display to the same 3D format and use
  fullscreen. Settings has the same controls in its own **3D depth** card.
- Depth map (shared by the enhancer, the desktop stereo formats and the
  Quest stereo screen): estimated at 192 texels across from cues (lower =
  nearer, salient / detailed / saturated = nearer, bright flat haze =
  farther), blurred so every texel is averaged in (no checkerboard), with
  **text and HUD** (two-tone patches with many thin high-contrast strokes)
  pulled towards the screen plane, the left / right 6 % faded to the screen
  plane, and smoothed over time. Its gradient is then limited (average of
  the upper and lower slope envelopes; flatter still over text / HUD) so
  the stereo warp never stretches or squeezes the picture by more than
  about 7 % at the largest shift, and the stereo pass reads it with a cubic
  B-spline, so the shift has no kinks: characters and HUD elements move as
  one rigid piece in both eyes, with no seams through them. Maps are half
  float where the GPU can render to it.

## 7. Snapshots and recordings

- File name: `UVC Studio YYYY-MM-DD HH.MM.SS.png` / `.mp4` in local time; if
  it exists add ` (2)` etc.
- Folders: Linux `$XDG_PICTURES_DIR/UVC Studio` (snapshots) and
  `$XDG_VIDEOS_DIR/UVC Studio` (recordings), falling back to
  `~/Pictures` / `~/Videos`; Windows the Pictures / Videos known folders +
  `\UVC Studio`; Quest keeps the current gallery publishing, with the new
  names.
- Video bitrate: `w × h × fps × 0.12` bits/s, clamped to 8…50 Mbit/s
  (1080p60 ≈ 15, 4K60 = 50). AAC 192 kbit/s.
- The composed picture (effects, mirror, overlay) is what gets saved.
- Toast after saving: "Saved to Pictures/UVC Studio" (or Videos) with a
  "Show" action.

## 8. Keyboard

| Key | Action |
|---|---|
| `Enter` | Connect (when disconnected) |
| `Ctrl+D` | Disconnect |
| `S` | Snapshot |
| `R` | Start / stop recording |
| `M` | Record microphone on/off |
| `L` | Listen (game audio monitor) on/off |
| `W` | Webcam overlay on/off |
| `X` | Mirror picture |
| `U` / `Shift+U` | 4K upscaling on/off / next upscaler |
| `C` | Retro look: Off → CRT → VHS → Arcade → Off |
| `D` | 3D depth: No depth ⇄ the last depth format |
| `O` or `Ctrl+,` | Settings |
| `F`, `F11`, double-click picture | Fullscreen (Esc leaves) |
| `Esc` | Cancel a Settings card drag / close About / leave fullscreen / close Settings |
| `?` or `H` | Shortcut sheet |
| `Ctrl +` / `Ctrl −` / `Ctrl 0` | Interface size |

Keys are ignored while a text field or menu has focus. The **shortcut
sheet** is a centred glass card listing these keys in two columns; any key
or click closes it.

## 9. Layout and visual design

### 9.1 Tokens
- Fonts: **Manrope** (display: wordmark, headings, buttons; 400–800) and
  **IBM Plex Sans** (body, labels, values; 400–600) — both include Latin,
  Latin Extended and Vietnamese; **IBM Plex Sans Thai** merged in for Thai;
  Noto Sans CJK TC for CJK. All SIL OFL 1.1. Files in assets/fonts/
  (Onest and Inter are removed).
- Type scale (px at 100 %): wordmark 17/800; heading 28/700 (ready screen
  title); section title 13/700 with +0.02 em; body 14/500; label 12/500
  (sentence case, not upper-case); value 12/600 tabular; chip 12/600;
  keycap 11/600.
- Dark palette: background #0F1115, panel #161A21, raised #1D222B, text
  #ECEFF4, secondary text #ECEFF4 at 62 %, hairline #FFFFFF at 10 %, stage
  #07080A. Light palette: background #F3F4F7, panel #FFFFFF, raised
  #EEF0F4, text #14161B, secondary 60 %, hairline #000000 at 9 %, stage
  #E6E8EC. Brand gradient unchanged (UVC Studio's green → blue → purple →
  red).
- Radii: tiles 14, panels 18, chips/pills 999, fields 10.

### 9.2 Top bar
Left: wordmark "UVC Studio" (gradient). Centre: **connection chip** —
Disconnected: hollow dot + "Not connected"; Connected: solid green dot +
"Device · 1920 × 1080 · 60 fps" (+ " ▸ 4K FSR" when upscaling, + " · 3D"
when depth is on); Recording: red dot + "REC 03:12" (dot pulses 0.8 s).
Right: language pill (existing glass pill, moved here) then the
**Connect / Disconnect** button (plug icon; brand gradient when it says
Connect, neutral glass when it says Disconnect). Errors appear as a small
red line under the chip, auto-hiding after 6 s.

### 9.3 Ready screen (Disconnected)
Centred glass card, max width 560:
1. App icon (48 px) and title "Ready when you are".
2. Sub line: "UVC Studio shows your console through any USB capture card —
   offline, nothing leaves this device."
3. **Checklist** (three rows, each: status dot, label, value):
   - Capture card — device name, or "Not found — plug it into USB".
   - Game audio — device name, or "Not found (video only)".
   - Webcam (optional) — name or "None".
   Rows update live as devices come and go.
4. Primary button "Connect" (Enter) — disabled with "Waiting for a capture
   card…" when no source exists.
5. Footer hint: "Press ? for shortcuts · double-click the picture for
   fullscreen".

### 9.4 Dock (bottom centre; existing tile/magnify/bounce mechanics)
Groups separated by thin dividers, in this order:
1. Capture: Snapshot, Record (red when active), Microphone.
2. Picture: 4K upscaling (hover popover), Retro look (hover popover:
   Off / CRT / VHS / Arcade as cards with a one-line description; a click
   on the tile toggles Off ⇄ the last look), 3D depth (hover popover),
   Mirror.
3. Sources: Webcam overlay (hover popover), Listen (hover popover).
4. Settings. (Quest: the VR switch after Settings.)
Disabled tiles at 35 % opacity. Tooltip: glass label above the tile with
the action name and a keycap chip for the shortcut (e.g. `Snapshot  [S]`).
A tile's hover and click area is the tile plus half the gap on each side
and 8 px above and below (so the top of a magnified tile still takes the
click); hover popovers end above that area, so they never cover a tile.
A hover popover stays open for 0.12 s (at least 2.5 frames) after the
pointer leaves the tile and the popover.
Bottom-left: version "UVC Studio v1.1.0". Bottom-right: Fullscreen
button only.

### 9.5 Listen popover
Title "Listen"; switch (on/off); horizontal volume slider 0–100 % with the
value at the right; "Output: <device>" line linking to Settings → Sound.

### 9.6 Settings (existing large glass panel and card columns)
Every card is exactly as tall as its content (cards are never stretched to
line up column bottoms), so columns may end at different heights.
Three balanced columns at most (a too-small window scrolls). Automatic card
order: **Source** (capture device, resolution, frame rate, pixel format),
**Sound** (game audio input; on Quest, with the card's USB audio selected,
*Game audio channels* segmented Stereo 2CH / Surround 6CH plus a one-line
hint; output device, microphone, listen volume),
**Picture** (brightness/contrast/saturation sliders
with −40…+40 values, gamma slider with 0.70…1.40, preset chips, Retro look
segmented Off / CRT / VHS / Arcade, 4K upscaling segmented Off / FSR /
Anime4K / Lanczos / NIS with the chosen one's description), **Webcam
overlay** (same fields as §5), **Display** (theme, interface size),
**VR** (Quest only, existing), **General** (auto-connect switch, "Open
snapshots folder", "Open recordings folder"), **3D depth** (depth format
dropdown, effect strength slider, the stereo hint for stereo formats; in
the automatic layout always in the same column, directly above About),
**About** (version in the header, GPU time per frame and processing
resolution while connected, and the open-source licences button; the
privacy note is shown in the sheet that button opens).

**Collapsible cards.**
- *Fold button*: every card header ends with a 24 px round glass button
  (same style as the padlock), always shown. Icon `chevron` (muted, foreground
  on hover) pointing up while the card is open (tooltip "Collapse") and down
  while it is collapsed (tooltip "Expand"); it turns over 220 ms.
- *Collapsed card*: only the header row (icon, title, header value or
  switch, padlock, fold button), card height = header + paddings. Collapsing
  and expanding ease (cubic out, 220 ms): the card is clipped to a height
  between its full height and its header, and the cards below follow.
  Clicking again mid-way reverses from where it is. The automatic layout
  balances the columns by the cards' expanded heights, so collapsing never
  moves a card to another column. Saved in `ui.settingsFolded`; *Reset
  layout* leaves it alone. A dragged collapsed card stays collapsed.

**User-arranged cards.**
- *Padlock*: every card header has a 24 px round glass button (same style as
  the panel's close button: solid 0.2, hover ink tint) left of the fold
  button, right of the header switch (Webcam overlay) and of the header value
  (Source stream, About version), which is ellipsised before it. Locked (default for
  every card): closed padlock in the muted colour (foreground on hover),
  tooltip "Unlock to move". Unlocked: open padlock in the brand gradient with
  a gradient rim, tooltip "Lock position". Icons `lock` / `unlock` (line set,
  24 × 24, stroke 1.75, round caps). Padlocks are shown only when the panel
  has three columns.
- *Unlocked card*: a faint dashed brand-gradient outline (60 %; 95 % while
  its header is hovered) instead of the 1 px border; the move cursor over
  the header strip (card top edge to just under the title, left of the
  padlock).
- *Drag*: a press on the header strip of an unlocked card becomes a drag
  after 4 px of movement (a click does nothing; the card's controls never
  start a drag). The card lifts off as a glass ghost that follows the
  pointer inside the grid: near-opaque glass, deeper shadow, gradient rim,
  scaled to 103 % (eased in over 150 ms). The drop target is the column under
  the pointer, before the first card whose middle is below the pointer; a
  drop slot as tall as the card opens there (10 % gradient wash, dashed
  gradient outline) and the other cards glide (150 ms cubic ease-out). The
  rest of the panel is inert during the drag; near the top / bottom edge the
  panel scrolls along. Release drops the card into the slot (it settles from
  the ghost's position); `Esc` cancels (it flies back). Keys other than Esc
  wait until the drag ends.
- *Layout model*: three columns, each an ordered list of card ids. The first
  drop copies the automatic layout and moves the card; from then on the
  saved arrangement is used instead of the balanced split, and the
  "3D depth above About" rule no longer applies. Dropping a card where it
  was changes nothing. Locked cards never change column or their order
  relative to other locked cards; unlocked cards flow around them.
  Cards missing from a saved arrangement are added once and saved: VR
  (Quest immersive mode) right after Display, any other card at the bottom
  of the shortest column. VR outside immersive mode keeps its saved place
  while hidden. A column taller than the panel scrolls (existing behaviour).
  A window too narrow for three columns shows the automatic layout without
  padlocks and keeps the saved arrangement untouched.
- *Reset layout*: a glass text button (button-small type) left of the close
  button, shown only while a saved arrangement is in use; it restores the
  automatic layout (cards glide into place) and locks every card.
- Quest's 2D window uses the same code; the controller pointer drags the
  same way (trigger down, move, release).

## 10. Errors (all translated)

| Situation | Message |
|---|---|
| Source cannot open | "Couldn't open the capture card: {detail}" |
| Source vanished | "The capture card was disconnected." |
| Game audio fails | "No game audio: {detail}" |
| Mic fails | "Microphone unavailable: {detail}" |
| Overlay camera fails | "Webcam unavailable: {detail}" |
| No encoder | "No video encoder: recording needs NVIDIA, AMD or Intel graphics" |
| Recording fails | "Recording stopped: {detail}" |

## 11. Settings storage
Keys use the `section.name` form, e.g. `source.device`, `source.mode`,
`source.format`, `sound.input`, `sound.output`, `sound.listen`,
`sound.volume`, `sound.micSource`, `sound.channels` (Quest: 2 stereo, 6
for 5.1 surround), `picture.brightness`,
`picture.contrast`, `picture.saturation`, `picture.gamma` (0.70 … 1.40),
`picture.upscaler`, `picture.retro` (0 off, 1 CRT, 2 VHS, 3 Arcade; the
look shown, restored at launch), `depth.format` (0 No depth, 1 Depth
effect, 2 Side by side full, 3 Side by side half, 4 Top and bottom, 5 Red /
cyan; Quest 0–1), `depth.lastFormat`, `depth.strength` (0.1 … 1.0),
`overlay.camera`, `overlay.quality`, `overlay.size`, `overlay.corner`,
`overlay.mirror`, `general.autoConnect`, `ui.lang`, `ui.theme`,
`ui.scale`, `ui.settingsLayout` (Settings card arrangement: three columns
separated by `|`, card ids separated by `,`, top to bottom — ids `source`,
`sound`, `picture`, `overlay`, `display`, `vr`, `general`, `depth`, `about`;
e.g. `source,sound,depth|picture,overlay|display,general,about`; empty =
automatic layout), `ui.settingsUnlocked` (comma-separated ids of the unlocked
cards; empty = all locked), `ui.settingsFolded` (comma-separated ids of the
collapsed cards; empty = all open), plus the existing VR keys. Old keys are not migrated
(unreleased betas; `depth.on` and `stereo.output` are gone).

## 12. Languages
Interface languages (12), listed in the menu in this order: 台灣正體中文
(zh-TW), English (en), Deutsch (de), Français (fr), Italiano (it), Español
(es), Português (pt, European Portuguese), 日本語 (ja), 한국어 (ko),
简体中文 (zh-CN), ไทย (th), Tiếng Việt (vi). European languages are ordered
by their country's population. The translation table uses the same column
order. Language
detection: saved choice → the OS locale mapped by language and script /
region (Traditional Chinese for Hant / TW / HK / MO, Simplified for other
Chinese) → English. String keys use the `area.name` form. Common interface
vocabulary (Settings, Record, Resolution …) keeps its existing translations
under the new keys; new sentences (ready screen, checklist, privacy note,
errors, tooltips, shortcut sheet) are written fresh in English and
translated from that English source.
