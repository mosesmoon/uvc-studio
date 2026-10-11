# UVC Studio – Funktionsübersicht

[English](README.md) · [台灣正體中文](README.zh-TW.md) · **Deutsch** · [Français](README.fr.md) · [Italiano](README.it.md) · [Español](README.es.md) · [Português](README.pt.md) · [日本語](README.ja.md) · [한국어](README.ko.md) · [简体中文](README.zh-CN.md) · [ไทย](README.th.md) · [Tiếng Việt](README.vi.md)

UVC Studio 1.1.0 ist eine native App für Capture-Karten, die komplett offline läuft – unter Linux, Windows und auf der Meta Quest. Du spielst über jede UVC-Capture-Karte mit minimaler Latenz, skalierst auf 4K hoch, legst einen Retro-Look oder 3D-Tiefe darüber, blendest deine Webcam ins Spiel ein, nimmst auf und machst Snapshots – ohne dass die App jemals ins Netz geht. 1.1.0, die erste offizielle Version, bringt UVC Studio Platinum auf die Meta Quest: 4K-Upscaling und 3D-Tiefe bleiben jeweils 30 Minuten am Tag frei, ein einmaliger Kauf schaltet sie dauerhaft frei; die Versionen für Linux und Windows bleiben kostenlos und sind jetzt Open Source unter der MIT-Lizenz.

![UVC Studio beim Spielen mit 4K-Upscaling](docs/screenshots/02-playing-4k.png)

## Plattformen und Downloads

Eine gemeinsame Codebasis: Alle Plattformen haben denselben Funktionsumfang und dieselbe Versionsnummer.

| Plattform | Download | Videoerfassung | Audio | Grafik | Aufnahme |
| --- | --- | --- | --- | --- | --- |
| Linux (x86_64) | [`UVCStudio-v1.1.0-x86_64.AppImage`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-v1.1.0-x86_64.AppImage) | Video4Linux2 | PulseAudio / PipeWire / ALSA | OpenGL 3.3 | H.264 (VA-API / NVENC / AMF in Hardware) + AAC, MP4 – einer davon ist nötig |
| Windows (x64) | [`UVCStudio-Setup-v1.1.0.exe`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-Setup-v1.1.0.exe) (Installer) oder [`UVCStudio-Portable-v1.1.0.zip`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-Portable-v1.1.0.zip) (ohne Installation) | Media Foundation | WASAPI | OpenGL 3.3 | H.264 (NVENC / AMF / Intel über Media Foundation, sonst der in Windows eingebaute Encoder) + AAC, MP4 – einer davon ist nötig |
| Meta Quest 2 / Pro / 3 / 3S | [`UVCStudio-Quest-v1.1.0-Free.apk`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-Quest-v1.1.0-Free.apk) (Sideload) oder Meta Horizon Store (demnächst) | USB-Host, libuvc | Ton der Karte, direkt per USB gelesen (Stereo oder 5.1) | OpenGL ES 3 (2D-Fenster), OpenXR (immersiv) | H.264 + AAC (Hardware-MediaCodec), MP4 |

Quellcode (Linux und Windows, MIT-Lizenz): dieses Repository oder [`UVCStudio-v1.1.0-src.tar.gz`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-v1.1.0-src.tar.gz).

Auf der Meta Quest sind 4K-Upscaling und 3D-Tiefe jeweils 30 Minuten am Tag frei; UVC Studio Platinum, ein einmaliger In-App-Kauf, hebt die Grenze auf. Unter Linux und Windows sind alle Funktionen frei.

## Screenshots

Linux in 1920 × 1080; ein Klick auf ein Bild zeigt es in voller Größe. Das 2D-Fenster auf der Meta Quest hat dieselbe Oberfläche.

<table>
<tr><td width="50%"><a href="docs/screenshots/01-ready.png"><img src="docs/screenshots/01-ready.png" alt="Startbildschirm: Capture-Karte, Spielton und Webcam werden live geprüft"></a><br><sub>Startbildschirm: Capture-Karte, Spielton und Webcam werden live geprüft</sub></td><td width="50%"><a href="docs/screenshots/03-4k-upscaling.png"><img src="docs/screenshots/03-4k-upscaling.png" alt="4K-Upscaling: vier Upscaler zur Auswahl"></a><br><sub>4K-Upscaling: vier Upscaler zur Auswahl</sub></td></tr>
<tr><td width="50%"><a href="docs/screenshots/04-settings.png"><img src="docs/screenshots/04-settings.png" alt="Einstellungen: alles auf einem Glas-Panel"></a><br><sub>Einstellungen: alles auf einem Glas-Panel</sub></td><td width="50%"><a href="docs/screenshots/05-webcam-overlay.png"><img src="docs/screenshots/05-webcam-overlay.png" alt="Webcam-Einblendung über dem Spiel"></a><br><sub>Webcam-Einblendung über dem Spiel</sub></td></tr>
<tr><td width="50%"><a href="docs/screenshots/06-retro-vhs.png"><img src="docs/screenshots/06-retro-vhs.png" alt="Retro-Look: VHS"></a><br><sub>Retro-Look: VHS</sub></td><td width="50%"><a href="docs/screenshots/07-3d-depth.png"><img src="docs/screenshots/07-3d-depth.png" alt="3D-Tiefe"></a><br><sub>3D-Tiefe</sub></td></tr>
<tr><td width="50%"><a href="docs/screenshots/08-shortcuts.png"><img src="docs/screenshots/08-shortcuts.png" alt="Tastenkürzel"></a><br><sub>Tastenkürzel</sub></td><td width="50%"><a href="docs/screenshots/09-light-theme.png"><img src="docs/screenshots/09-light-theme.png" alt="Helles Design"></a><br><sub>Helles Design</sub></td></tr>
<tr><td width="50%"><a href="docs/screenshots/10-zh-TW.png"><img src="docs/screenshots/10-zh-TW.png" alt="Oberfläche in 台灣正體中文"></a><br><sub>Oberfläche in 台灣正體中文</sub></td><td width="50%"><a href="docs/screenshots/11-languages.png"><img src="docs/screenshots/11-languages.png" alt="12 Sprachen"></a><br><sub>12 Sprachen</sub></td></tr>
</table>

## Erste Schritte

1. Steck die Capture-Karte per USB an und starte UVC Studio. Der **Startbildschirm** prüft live drei Punkte: *Capture-Karte*, *Spielaudio* und *Webcam (optional)*. Fehlt die Karte, steht dort „Nicht gefunden – an USB anschließen“; fehlt der Ton, „Nicht gefunden (nur Video)“.
2. Klick auf **Verbinden** (auf dem Startbildschirm oder oben rechts) oder drück `Enter`. Ist **Automatisch verbinden** aktiv (Einstellungen → Allgemein, standardmäßig an), verbindet sich UVC Studio von selbst, sobald du eine Karte ansteckst – und beim Start, wenn schon eine dranhängt.
3. Spielen. **Trennen** (dieselbe Schaltfläche oder `Strg+D`) schließt alles; eine laufende Aufnahme wird vorher sauber beendet und gespeichert.

Pausieren und Fortsetzen gibt es nicht mehr: UVC Studio ist entweder verbunden oder nicht. Ziehst du die Karte ab, erscheint unter dem Verbindungsstatus eine Fehlermeldung und die App kehrt zum Startbildschirm zurück; mit „Automatisch verbinden“ geht es weiter, sobald die Karte wieder steckt.

## Erfassung und Ton

Jede UVC-Capture-Karte funktioniert ohne zusätzliche Treiber – ob ShadowCast, Elgato, AVerMedia, Magewell oder ein günstiger USB-3-HDMI-Stick.

- **Echte Kartenmodi**: Auflösungen und Bildraten stammen direkt aus dem, was die Karte meldet – die größte zuerst, gekennzeichnet mit 4K, QHD, Full HD, HD oder SD. Voreingestellt ist die größte Auflösung, die mindestens 59 fps schafft. Format: Automatisch, Unkomprimiert (geringste Verzögerung) oder Komprimiert (höhere Auflösungen und Bildraten).
- **Findet die richtigen Geräte**: Ist noch nichts gespeichert, wählt UVC Studio Capture-Karte, deren Ton und Webcam anhand der Gerätenamen – eingebaute oder virtuelle Kameras werden nicht für die Karte gehalten. Gleichnamige Geräte erhalten den Zusatz (2), (3) …
- **Geringe Latenz**: Angezeigt wird immer das neueste Bild – alte Bilder stauen sich nicht auf.
- **Merkt sich deine Einrichtung** und erkennt Karten, die du im laufenden Betrieb ein- oder aussteckst.
- **Mithören** (`L`): Spielton über Lautsprecher oder Kopfhörer hören. Fährst du mit der Maus über die Dock-Kachel, gibt es einen Schalter, die Lautstärke und den aktuellen Audioausgang; die Ausgabe-Zeile führt direkt zu Einstellungen → Ton.
- **Aktueller Modus auf einen Blick**: Einstellungen → Quelle zeigt den tatsächlichen Stream, etwa 3840 × 2160 · 60 fps · YUY2.
- **Ton auf der Quest**: Die App liest den Ton der Karte direkt über USB (USB Audio Class 1 und 2, auch MacroSilicon-MS2109-Sticks) – Einstellungen → Ton → Audioeingang → „Audio der Capture-Karte (USB)“. Einstellungen → Ton → „Spielton-Kanäle“: **Stereo 2CH** (Standard) oder **Surround 6CH** – 5.1 von Karten mit 6-Kanal-Modus (etwa der CJ-16) und Konsolen, die 5.1 über HDMI ausgeben (etwa die Switch 2 auf Surround); Aufnahmen bleiben Stereo.
- **Selbstheilung auf der Quest**: Liefert die Karte keine Bilder mehr, öffnet die App sie bis zu zweimal neu und bittet dich dann, die HDMI-Quelle zu prüfen oder die Karte neu einzustecken.

## Bild

Vier 4K-Upscaler, drei Retro-Looks und 3D-Tiefe laufen Bild für Bild auf der GPU – zusätzliche Verzögerung entsteht nur durch deren Rechenzeit.

**4K-Upscaling** (`U`): Ein Klick auf die Dock-Kachel oder `U` schaltet es ein und aus; per Mouseover über der Kachel wählst du den Upscaler, `Umschalt+U` springt zum nächsten. Unter Einstellungen → Bild → 4K-Upscaling stehen *Aus*, *FSR*, *Anime4K*, *Lanczos* und *NIS* zur Wahl: Ein Upscaler schaltet das Upscaling ein, *Aus* merkt sich den zuletzt gewählten fürs nächste Mal.

| Modus | Funktionsweise | Ideal für | Faktor |
| --- | --- | --- | --- |
| FSR-4K-Upscaling (Standard) | AMD FidelityFX Super Resolution 1: kantenadaptives Hochskalieren (EASU) plus RCAS-Schärfung | Spiele und jedes Video | 720p / 1080p auf 4K, SD-Quellen bis 3× |
| Anime4K-Upscaling für Animation | Neuronales Netz Anime4K ×2 (Größe M am Desktop, S auf der Quest), den Rest übernimmt FSR | Anime, Cartoons, Strichzeichnungen | 2× durch das Netz, dann auf 4K |
| Lanczos + CAS 4K-Upscaling | Lanczos-3-Resampling (6 Taps pro Achse) plus AMD-FidelityFX-CAS-Schärfung | Klassisches, neutrales Skalieren | Auf 4K |
| NIS-4K-Upscaling | NVIDIA Image Scaling 1.0.3: kantengerichteter 6-Tap-Skalierer mit adaptiver Schärfung | Knackige Details | Bis 2×, darüber Lanczos-3 |

Der Upscaler rechnet nur, wenn ein neues Bild ankommt. Fehlen der Quest Half-Float-Render-Targets, nutzt Anime4K stattdessen FSR.

- **Retro-Look** (`C`): Per Mouseover über der Dock-Kachel wählst du *Aus*, *CRT*, *VHS* oder *Arcade*; `C` schaltet in dieser Reihenfolge weiter, ein Klick auf die Kachel wechselt zwischen *Aus* und dem zuletzt genutzten Look. Dieselbe Auswahl gibt es unter Einstellungen → Bild → Retro-Look. Der Look liegt auf dem fertigen, hochskalierten Bild, funktioniert also zusammen mit dem 4K-Upscaling und ist auch in Snapshots, Aufnahmen und auf der Riesenleinwand der Quest zu sehen. Deine Wahl bleibt gespeichert.
  - **CRT**: Röhrenfernseher – versetzte Schlitzmaske, Scanlines, die mit der Helligkeit dunkler werden, leicht gewölbter Bildschirm und weiche Vignette.
  - **VHS**: Videokassette – etwas weicheres Bild, nach rechts auslaufende Farben, leicht verblasste, warme Töne mit angehobenem Schwarz, feines Rauschen, sanftes Zeilenzittern und ab und zu ein schwacher Tracking-Streifen, der nach unten wandert (Schrift bleibt lesbar).
  - **Arcade**: Monitor eines Arcade-Automaten – grobes Raster mit kräftigen dunklen Scanlines, senkrechte RGB-Streifenmaske, sanftes Leuchten heller Stellen, kräftige Farben und eine leichte Wölbung.
- **3D-Tiefe** (`D`): schätzt, was im Bild nah und was fern ist. Per Mouseover über der Kachel oder unter Einstellungen → 3D-Tiefe stellst du **Tiefenformat** und **Effektstärke** ein. *Tiefeneffekt (nur 2D-Verstärkung)* macht den Vordergrund schärfer und lebendiger, den Hintergrund weicher und dunstiger; die Stereo-Formate für 3D-Displays beschreibt der nächste Abschnitt; im immersiven Modus der Quest macht *Stereo-3D-Riesenleinwand* die Leinwand zu echtem Stereo-3D. `D` oder ein Klick auf die Kachel wechselt zwischen *Keine Tiefe* und dem zuletzt genutzten Format.
- **Helligkeit, Kontrast, Sättigung** (Einstellungen → Bild): −40 bis +40, 0 ist neutral. **Gamma**: 0,70 bis 1,40 (1,00 ist neutral) – Werte über 1,00 hellen die Mitteltöne auf, Werte darunter dunkeln sie ab; Schwarz und Weiß bleiben unverändert. Dazu die **Stile** *Natürlich*, *Realistisch* (natürlicher Kontrast mit sanfter S-Kurve und zurückhaltenden Farben, die Schatten behalten ihre Details), *Kräftig*, *Film* und *Heller Raum* (jetzt mit etwas mehr Gamma, damit dunkle Szenen auch in hellen Räumen gut erkennbar bleiben).
- **Spiegeln** (`X`): spiegelt das ganze Bild horizontal.

## Stereo-3D-Formate (Linux / Windows)

Am Desktop bietet das Tiefenformat-Menü (an der Kachel 3D-Tiefe oder unter Einstellungen → 3D-Tiefe) außerdem vier Stereo-Formate, die echtes Stereo-3D an ein 3D-Display schicken. Stell das Display auf dasselbe 3D-Format und wechsle in den Vollbildmodus (`F`); Snapshots und Aufnahmen bleiben 2D. Schrift und HUD-Elemente werden erkannt und nahe der Bildschirmebene gehalten, und die Tiefe verläuft glatter – Buchstaben bewegen sich für beide Augen als Ganzes und wirken nicht mehr zerschnitten.

| Format | Für |
| --- | --- |
| Nebeneinander, voll (3840×1080) | XR-Brillen im 3D-Modus, etwa XREAL, VITURE und RayNeo |
| Nebeneinander, halb | 3D-Fernseher |
| Übereinander | 3D-Fernseher und Projektoren |
| Rot-Cyan-Brille | Anaglyphenbrillen in Rot/Cyan |

## Webcam-Overlay, Snapshots und Aufnahme

Gespeichert wird genau das, was du siehst: Effekte, Spiegelung und Webcam-Overlay landen in jedem Snapshot und jeder Aufnahme.

- **Webcam-Overlay** (`W`): deine Webcam über dem Spiel, in einer der vier Ecken (standardmäßig unten links) und in den Größen S, M oder L (18, 26 oder 36 % der Bildbreite).
  - Zieh es an eine beliebige Stelle; beim Loslassen gleitet es in die nächste Ecke. Mit dem Mausrad darüber änderst du die Größe.
  - Beim Darüberfahren erscheint eine kleine Leiste mit *Spiegeln*, *Größe* und *Entfernen*.
  - Über die Dock-Kachel (oder Einstellungen → Webcam-Overlay) wählst du *Kamera*, *Qualität* (720p · 30 fps, 1080p · 30 fps oder 1080p · 60 fps), *Größe*, *Ecke*, *Spiegeln* sowie **Bilder tauschen**: Die Webcam wird zum Hauptbild, die Capture-Karte zum Overlay.
- **Snapshot** (`S`): das aktuelle Bild als PNG.
- **Aufnehmen** (`R`): MP4 mit H.264-Video und AAC-Ton mit 192 kbit/s. Die Bitrate richtet sich nach dem Stream (Breite × Höhe × fps × 0,12, zwischen 8 und 50 Mbit/s: rund 15 Mbit/s bei 1080p60, 50 Mbit/s bei 4K60). Der Verbindungsstatus zeigt REC und die bisherige Dauer.
  - **Windows:** Es ist kein Software-Encoder enthalten. Für Aufnahmen brauchst du einen Grafik-Encoder von NVIDIA (NVENC), AMD (AMF) oder Intel oder den in Windows eingebauten H.264-Encoder (Media Foundation; die „N“-Editionen von Windows benötigen das Media Feature Pack).
  - **Linux:** Auch hier ist kein Software-Encoder enthalten. Für Aufnahmen brauchst du Intel- oder AMD-Grafik über VA-API (mit installiertem VA-API-Treiber), NVIDIA NVENC (proprietärer Treiber) oder AMD AMF.
  - Fehlt all das, funktioniert alles andere weiterhin; nur „Aufnehmen“ meldet „Kein Video-Encoder: Aufnahmen brauchen NVIDIA-, AMD- oder Intel-Grafik“.
- **Mikrofon aufnehmen** (`M`): mischt deine Stimme zum Spielton.
- **Dateinamen und Ordner**: `UVC Studio 2026-10-06 21.30.15.png` bzw. `.mp4` (Datum und Uhrzeit; existiert der Name schon, wird eine Nummer angehängt). Snapshots landen in **Bilder/UVC Studio**, Aufnahmen in **Videos/UVC Studio**, jeweils mit dem Hinweis „Gespeichert in Pictures/UVC Studio“ und der Schaltfläche **Anzeigen**; unter Einstellungen → Allgemein gibt es außerdem „Snapshot-Ordner öffnen“ und „Aufnahmeordner öffnen“. Auf der Quest geht alles in die Galerie (`Pictures/UVC Studio` und `Movies/UVC Studio`), sichtbar in der Dateien-App und per USB.

## Meta Quest

Steck die Capture-Karte in den USB-C-Anschluss des Headsets und spiel in einem frei skalierbaren Fenster oder auf einer riesigen virtuellen Leinwand – mit echtem Stereo-3D.

- **Installation**: über den Meta Horizon Store (demnächst) oder mit der Brille im Entwicklermodus `UVCStudio-Quest-v1.1.0-Free.apk` aus den Releases dieses Projekts installieren (Meta Quest Developer Hub, SideQuest oder `adb install`). Platinum lässt sich nur in der Version aus dem Store kaufen.
- **Anschließen**: Ein USB-C-OTG-Adapter oder Hub funktioniert ebenfalls. Bestätige „UVC Studio öffnen, wenn dieses Gerät verbunden wird?“ (mit „Immer“) sowie die Kamera- und Mikrofonberechtigung, die Android für USB-Capture-Karten verlangt. Die Kameras des Headsets selbst werden nie verwendet.
- **Dekodierung**: H.264- und HEVC-Karten dekodiert der eingebaute Hardware-Decoder des Headsets; das mitgelieferte FFmpeg kümmert sich nur um MJPEG.
- **2D-Fenster** (Standard): die komplette Desktop-Oberfläche in einem Horizon-OS-Panel, das du verschieben und skalieren kannst. Ein doppeltes Pinch auf das Bild schaltet den Vollbildmodus ein und aus.
- **Immersiver Modus**: Die Headset-Kachel am Ende des Docks, hinter „Einstellungen“, wechselt zur riesigen virtuellen Leinwand und zurück. UVC Studio trennt dabei, wechselt und verbindet sich auf der anderen Seite von selbst wieder.
- **3D-Tiefe auf der Quest**: Im 2D-Fenster stehen *Keine Tiefe* und *Tiefeneffekt* zur Wahl, im immersiven Modus *Keine Tiefe* und *Stereo-3D-Riesenleinwand*.

| Im immersiven Modus | So geht's |
| --- | --- |
| Bedienfeld anklicken | Mit dem Controller zeigen und den Trigger ziehen oder mit der Hand pinchen |
| Bedienfeld verschieben | Den weißen Balken darunter greifen und ziehen; Wegdrücken oder Heranziehen ändert den Abstand. Einstellungen → VR → „Bedienfeldposition zurücksetzen“ stellt es zurück |
| Leinwand verschieben | Den weißen Balken darunter greifen und ziehen; Hand wegdrücken oder heranziehen ändert den Abstand |
| Leinwandgröße ändern | Auf eine Ecke zeigen, bis eine Klammer erscheint, dann pinchen und ziehen |
| Näher / weiter, kleiner / größer | Thumbstick hoch / runter, links / rechts |
| Bedienfeld aus- oder einblenden | Doppeltes Pinch auf die Leinwand oder B, Y bzw. Menü drücken |
| Neu zentrieren | Meta-Taste lange drücken |
| Leinwand-Voreinstellungen | Einstellungen → VR: Hintergrund (Passthrough oder Dunkler Raum), Bildschirmform (flach oder gebogen), Bildschirmgröße, Bildschirmabstand, Bildwiederholrate 72 / 90 / 120 Hz (Standard 90) |

- **Systemmenü**: Solange es offen ist, zeichnet die App weiter, blendet ihre Zeiger aus und schaltet den Ton stumm.
- **Kühler Betrieb**: Das Upscaling endet dort, wo die Riesenleinwand tatsächlich noch Details zeigen kann (etwa 30 Pixel pro Grad, höchstens 2560 breit), und das Stereo-Bildpaar wird in derselben Größe gerendert.
- **Überhitzungsschutz**: Überwacht den Temperaturstatus von Android und die Temperaturmeldungen von OpenXR. Wird das Headset warm, schaltet die App auf FSR und 72 Hz um; wird es heiß, pausiert sie Upscaling und 3D-Tiefe. Deine Einstellungen bleiben unverändert, und ein Hinweis meldet, wenn es wieder abgekühlt ist.
- **Lastanzeige**: Bei bestehender Verbindung zeigt Einstellungen → Über die GPU-Zeit pro Bild, die Verarbeitungsauflösung und die Headset-Temperatur.

## Oberfläche

Die gesamte Oberfläche ist im Liquid-Glass-Stil gehalten: mattierte, durchscheinende Flächen, hinter denen – solange verbunden ist – eine weichgezeichnete, farbverstärkte Kopie des Spiels zu sehen ist.

- **Obere Leiste**: links der Schriftzug UVC Studio; in der Mitte der **Verbindungsstatus** („Nicht verbunden“; ein grüner Punkt mit Gerät · Auflösung · fps, beim Upscaling ergänzt um ▸ 4K und den Upscaler sowie · 3D; oder ein pulsierendes rotes REC mit der Aufnahmezeit); rechts das Sprachmenü und **Verbinden / Trennen**. Fehler erscheinen für ein paar Sekunden in Rot unter dem Status.
- **Dock**: vier Gruppen – *Snapshot, Aufnehmen, Mikrofon aufnehmen* · *4K-Upscaling, Retro-Look, 3D-Tiefe, Spiegeln* · *Webcam-Overlay, Mithören* · *Einstellungen*. Kacheln vergrößern sich unter dem Mauszeiger, aktive Funktionen leuchten im Markenverlauf, und jeder Tooltip zeigt das passende Tastenkürzel. Die Version steht unten links, der Vollbild-Knopf unten rechts.
- **Einstellungen**: ein großes Glas-Panel mit Karten in höchstens drei gleichmäßig gefüllten Spalten, sodass sich alles ohne Scrollen einstellen lässt: Quelle, Ton, Bild, Webcam-Overlay, Anzeige, VR (immersiver Modus der Quest), Allgemein, 3D-Tiefe und Über – in der automatischen Anordnung sitzt die Karte „3D-Tiefe“ direkt oberhalb von „Über“. „Über“ zeigt die Version, bei bestehender Verbindung die GPU-Zeit pro Bild und die Verarbeitungsauflösung (auf der Quest zusätzlich die Headset-Temperatur) sowie die Schaltfläche „Open-Source-Lizenzen“; der Datenschutzhinweis steht in dem Fenster, das sie öffnet.
  - **Karten einklappen**: Der runde Pfeil-Button ganz rechts im Kartenkopf, rechts neben dem Schloss, klappt die Karte auf ihren Kopf zusammen (Titel, Schloss und Klapp-Button bleiben) und wieder auf. Die anderen Karten rutschen nach, keine wechselt die Spalte, und eingeklappte Karten bleiben gespeichert.
  - **Karten selbst anordnen**: Jede Karte hat oben rechts in ihrer Kopfzeile ein Schloss; anfangs sind alle Karten gesperrt. Klick auf das Schloss („Zum Verschieben entsperren“) und zieh die Karte an ihrer Kopfzeile an eine andere Stelle der drei Spalten; eine Lücke zeigt, wo sie landet, und `Esc` bricht das Ziehen ab. Ein weiterer Klick aufs Schloss („Position sperren“) hält die Karte fest. Gesperrte Karten bleiben, wo sie sind, entsperrte bewegen sich zwischen ihnen. Die Anordnung und die entsperrten Karten bleiben auch beim nächsten Start erhalten. „Anordnung zurücksetzen“ neben dem Schließen-Knopf – nur sichtbar, solange deine eigene Anordnung aktiv ist – stellt die automatische Anordnung wieder her und sperrt alle Karten. Ist das Fenster für drei Spalten zu schmal, erscheint die automatische Anordnung; deine eigene bleibt gespeichert, bis das Fenster wieder breit genug ist. Im 2D-Fenster der Quest funktioniert das genauso mit dem Controller-Zeiger.
- **Sprachen**: 台灣正體中文, English, Deutsch, Français, Italiano, Español, Português, 日本語, 한국어, 简体中文, ไทย und Tiếng Việt, auswählbar im Glasmenü oben rechts. Beim ersten Start folgt die App der Systemsprache.
- **Designs**: Automatisch (hell von 06:00 bis 18:00 Uhr), Dunkel oder Hell.
- **Oberflächengröße**: passt sich dem Bildschirm an (1080p 100 %, 1440p etwa 135 %, 4K 200 %); ändern lässt sie sich unter Einstellungen → Anzeige oder mit `Strg` `+` / `−` / `0`.
- **Schriften und Symbole**: Manrope und IBM Plex Sans (für Thai IBM Plex Sans Thai, für Chinesisch, Japanisch und Koreanisch Noto Sans CJK TC); die Symbole sind eigene Entwürfe von UVC Studio.
- **Vollbild**: `F`, `F11`, der Knopf unten rechts oder ein Doppelklick aufs Bild; `Esc` beendet ihn.

## Tastenkürzel (Linux / Windows)

Mit `?` oder `H` zeigt die App dieselbe Liste als Übersicht an; jede Taste oder ein Klick schließt sie wieder.

| Taste | Funktion |
| --- | --- |
| `Enter` | Verbinden |
| `Strg+D` | Trennen |
| `S` | Snapshot |
| `R` | Aufnahme starten oder stoppen |
| `M` | Mikrofon aufnehmen an / aus |
| `L` | Mithören an / aus |
| `W` | Webcam-Overlay an / aus |
| `X` | Spiegeln |
| `U` | 4K-Upscaling an / aus |
| `Umschalt+U` | Nächster Upscaler (schaltet Upscaling ein) |
| `C` | Retro-Look: Aus → CRT → VHS → Arcade |
| `D` | 3D-Tiefe: zwischen *Keine Tiefe* und dem letzten Format wechseln |
| `O` oder `Strg+,` | Einstellungen |
| `F` oder `F11` | Vollbild |
| `Esc` | Ziehen einer Karte abbrechen, Vollbild beenden oder Einstellungen schließen |
| `?` oder `H` | Tastenkürzel |
| `Strg` `+` / `−` / `0` | Oberfläche größer / kleiner / zurücksetzen |

## Datenschutz

UVC Studio arbeitet vollständig auf deinem Gerät. Bild, Ton und Aufnahmen verlassen es nie – kein Konto, kein Upload, kein Tracking, und die App geht niemals ins Netz. Auf der Meta Quest laufen der Kauf von Platinum und die Prüfung des Kaufs über den Meta-Horizon-Store-Dienst der Brille; UVC Studio selbst sendet nichts.

## Lizenzen

UVC Studio für Linux und Windows ist Open Source unter der [MIT-Lizenz](LICENSE); die Version für Meta Quest ist nicht quelloffen. UVC Studio enthält keine GPL-Komponenten; FFmpeg ist rein unter LGPL gebaut (auf der Quest nur mit dem MJPEG-Decoder und als austauschbare Shared Libraries). Weitere Komponenten: SDL2, Dear ImGui, miniaudio, stb, NanoSVG, libusb und libuvc (Quest), der OpenXR-Loader, AMD FidelityFX FSR 1 und CAS, Anime4K sowie NVIDIA Image Scaling. Die Schriften (Manrope, IBM Plex Sans, IBM Plex Sans Thai, Noto Sans CJK TC) stehen unter der SIL Open Font License. Alle Komponenten samt vollständigem Lizenztext findest du in der App unter Einstellungen → Über → Open-Source-Lizenzen. Die Version für Meta Quest enthält außerdem den Loader des Meta Platform SDK (Oculus SDK License) für den Kauf im Store.
