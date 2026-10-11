# UVC Studio: guía de funciones

[English](README.md) · [台灣正體中文](README.zh-TW.md) · [Deutsch](README.de.md) · [Français](README.fr.md) · [Italiano](README.it.md) · **Español** · [Português](README.pt.md) · [日本語](README.ja.md) · [한국어](README.ko.md) · [简体中文](README.zh-CN.md) · [ไทย](README.th.md) · [Tiếng Việt](README.vi.md)

UVC Studio 1.1.0 es una aplicación nativa para capturadoras que funciona totalmente sin conexión en Linux, Windows y Meta Quest: juega con tus consolas a través de cualquier capturadora UVC con una latencia mínima, escala la imagen a 4K, añade un efecto retro o profundidad 3D, superpón tu cámara web sobre el juego, graba y haz capturas, todo sin conectarse nunca a la red. La 1.1.0, la primera versión oficial, trae UVC Studio Platinum a Meta Quest: el escalado 4K y la profundidad 3D siguen gratis 30 minutos al día cada uno, y una compra única los desbloquea para siempre; las versiones para Linux y Windows siguen siendo gratis y ahora son de código abierto con licencia MIT.

![UVC Studio jugando con escalado 4K](docs/screenshots/02-playing-4k.png)

## Plataformas y descargas

Un único código fuente: las mismas funciones y el mismo número de versión en todas las plataformas.

| Plataforma | Descarga | Captura de vídeo | Audio | Gráficos | Grabación |
| --- | --- | --- | --- | --- | --- |
| Linux (x86_64) | [`UVCStudio-v1.1.0-x86_64.AppImage`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-v1.1.0-x86_64.AppImage) | Video4Linux2 | PulseAudio / PipeWire / ALSA | OpenGL 3.3 | H.264 (por hardware: VA-API / NVENC / AMF) + AAC, MP4; se necesita uno de ellos |
| Windows (x64) | [`UVCStudio-Setup-v1.1.0.exe`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-Setup-v1.1.0.exe) (instalador) o [`UVCStudio-Portable-v1.1.0.zip`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-Portable-v1.1.0.zip) (portable) | Media Foundation | WASAPI | OpenGL 3.3 | H.264 (NVENC / AMF / Intel mediante Media Foundation o, si no, el codificador integrado en Windows) + AAC, MP4; se necesita uno de ellos |
| Meta Quest 2 / Pro / 3 / 3S | [`UVCStudio-Quest-v1.1.0-Free.apk`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-Quest-v1.1.0-Free.apk) (sideload) o Meta Horizon Store (próximamente) | Host USB, libuvc | Audio USB de la capturadora, leído directamente (estéreo o 5.1) | OpenGL ES 3 (ventana 2D), OpenXR (inmersivo) | H.264 + AAC (MediaCodec por hardware), MP4 |

Código fuente (Linux y Windows, licencia MIT): este repositorio o [`UVCStudio-v1.1.0-src.tar.gz`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-v1.1.0-src.tar.gz).

En Meta Quest, el escalado 4K y la profundidad 3D son gratis 30 minutos al día cada uno; UVC Studio Platinum, una compra única dentro de la app, los hace ilimitados. En Linux y Windows todas las funciones son gratis.

## Capturas de pantalla

Linux a 1920 × 1080; haz clic en una imagen para verla a tamaño completo. La ventana 2D de Meta Quest usa la misma interfaz.

<table>
<tr><td width="50%"><a href="docs/screenshots/01-ready.png"><img src="docs/screenshots/01-ready.png" alt="Pantalla de inicio: capturadora, audio del juego y cámara web comprobados en directo"></a><br><sub>Pantalla de inicio: capturadora, audio del juego y cámara web comprobados en directo</sub></td><td width="50%"><a href="docs/screenshots/03-4k-upscaling.png"><img src="docs/screenshots/03-4k-upscaling.png" alt="Escalado 4K: cuatro métodos para elegir"></a><br><sub>Escalado 4K: cuatro métodos para elegir</sub></td></tr>
<tr><td width="50%"><a href="docs/screenshots/04-settings.png"><img src="docs/screenshots/04-settings.png" alt="Ajustes: todo en un solo panel de cristal"></a><br><sub>Ajustes: todo en un solo panel de cristal</sub></td><td width="50%"><a href="docs/screenshots/05-webcam-overlay.png"><img src="docs/screenshots/05-webcam-overlay.png" alt="Cámara web superpuesta sobre el juego"></a><br><sub>Cámara web superpuesta sobre el juego</sub></td></tr>
<tr><td width="50%"><a href="docs/screenshots/06-retro-vhs.png"><img src="docs/screenshots/06-retro-vhs.png" alt="Efecto retro: VHS"></a><br><sub>Efecto retro: VHS</sub></td><td width="50%"><a href="docs/screenshots/07-3d-depth.png"><img src="docs/screenshots/07-3d-depth.png" alt="Profundidad 3D"></a><br><sub>Profundidad 3D</sub></td></tr>
<tr><td width="50%"><a href="docs/screenshots/08-shortcuts.png"><img src="docs/screenshots/08-shortcuts.png" alt="Atajos de teclado"></a><br><sub>Atajos de teclado</sub></td><td width="50%"><a href="docs/screenshots/09-light-theme.png"><img src="docs/screenshots/09-light-theme.png" alt="Tema claro"></a><br><sub>Tema claro</sub></td></tr>
<tr><td width="50%"><a href="docs/screenshots/10-zh-TW.png"><img src="docs/screenshots/10-zh-TW.png" alt="Interfaz en 台灣正體中文"></a><br><sub>Interfaz en 台灣正體中文</sub></td><td width="50%"><a href="docs/screenshots/11-languages.png"><img src="docs/screenshots/11-languages.png" alt="12 idiomas de interfaz"></a><br><sub>12 idiomas de interfaz</sub></td></tr>
</table>

## Primeros pasos

1. Conecta la capturadora por USB y abre UVC Studio. La **pantalla de inicio** comprueba en directo tres cosas: *Capturadora*, *Audio del juego* y *Cámara web (opcional)*. Si falta la capturadora verás «No encontrada: conéctala por USB»; si falta el audio, «No encontrado (solo vídeo)».
2. Pulsa **Conectar** (en la pantalla de inicio o arriba a la derecha) o la tecla `Intro`. Con **Conectar automáticamente** activado (Ajustes → General, activado de serie), UVC Studio se conecta solo en cuanto enchufas una capturadora, y también al abrirse si ya estaba enchufada.
3. A jugar. **Desconectar** (el mismo botón o `Ctrl+D`) lo cierra todo; si hay una grabación en curso, primero se termina y se guarda.

Ya no hay pausa ni reanudación: UVC Studio está conectado o no lo está. Si desenchufas la capturadora, aparece un error bajo el indicador de conexión y la aplicación vuelve a la pantalla de inicio; con la conexión automática activada, se reconecta en cuanto la capturadora vuelve.

## Captura y sonido

Cualquier capturadora UVC funciona sin controladores adicionales: ShadowCast, Elgato, AVerMedia, Magewell y las típicas capturadoras HDMI USB 3 se comportan igual.

- **Los modos reales de la capturadora**: las resoluciones y los fotogramas por segundo son los que anuncia la propia capturadora, de mayor a menor y con la etiqueta 4K, QHD, Full HD, HD o SD. De forma predeterminada, UVC Studio elige la mayor resolución que alcanza 59 fps o más. Formato: Automático, Sin comprimir (mínimo retardo) o Comprimido (resoluciones y frecuencias más altas).
- **Elige los dispositivos correctos**: mientras no hayas guardado nada, la capturadora, su audio y la cámara web se eligen por su nombre, así que una cámara integrada o virtual no se confunde con la capturadora. Los dispositivos con el mismo nombre se distinguen con (2), (3)…
- **Baja latencia**: siempre se muestra el fotograma más reciente, nunca una cola de fotogramas atrasados.
- **Recuerda tu configuración** y detecta las capturadoras que conectas o desconectas con la aplicación abierta.
- **Escucha** (`L`): oye el juego por los altavoces o los auriculares. Al pasar el ratón por el icono del dock tienes un interruptor, el volumen y la salida de audio actual; la línea «Salida» te lleva directamente a Ajustes → Sonido.
- **Modo real a la vista**: Ajustes → Fuente muestra el flujo efectivo, por ejemplo 3840 × 2160 · 60 fps · YUY2.
- **Audio en Meta Quest**: la aplicación lee el sonido de la capturadora directamente por USB (USB Audio Class 1 y 2, incluidas las capturadoras MacroSilicon MS2109): Ajustes → Sonido → Entrada de audio → «Audio de la capturadora (USB)». Ajustes → Sonido → «Canales de audio del juego»: **Estéreo 2CH** (predeterminado) o **Envolvente 6CH**: 5.1 con capturadoras que tienen modo de 6 canales (como la CJ-16) y consolas que envían 5.1 por HDMI (como Switch 2 en Envolvente); las grabaciones siguen en estéreo.
- **Recuperación automática en Quest**: si la capturadora deja de enviar imagen, la aplicación la vuelve a abrir sola (hasta dos veces) y, si no lo consigue, te pide que compruebes la fuente HDMI o que vuelvas a conectarla.

## Imagen

Cuatro escaladores 4K, tres efectos retro y la profundidad 3D se procesan en la GPU fotograma a fotograma, sin más retardo que el tiempo de cálculo de la GPU.

**Escalado 4K** (`U`): haz clic en el icono del dock o pulsa `U` para activarlo o desactivarlo, pasa el ratón por el icono para elegir el modo y usa `Mayús+U` para pasar al siguiente. En Ajustes → Imagen → Escalado 4K tienes *Desactivado*, *FSR*, *Anime4K*, *Lanczos* y *NIS*: al elegir un modo se activa el escalado, y *Desactivado* recuerda el último modo para la próxima vez.

| Modo | Cómo funciona | Ideal para | Factor |
| --- | --- | --- | --- |
| Escalado 4K FSR (predeterminado) | AMD FidelityFX Super Resolution 1: ampliación adaptativa a los bordes (EASU) más enfoque RCAS | Juegos y cualquier vídeo | De 720p / 1080p a 4K; fuentes SD hasta 3× |
| Escalado 4K Anime4K para animación | Red neuronal Anime4K ×2 (tamaño M en escritorio, S en Quest) y FSR para el resto | Anime, dibujos animados, dibujo a línea | 2× con la red, después hasta 4K |
| Escalado 4K Lanczos + CAS | Remuestreo Lanczos-3 (6 muestras por eje) más enfoque AMD FidelityFX CAS | Escalado clásico y neutro | Hasta 4K |
| Escalado 4K NIS | NVIDIA Image Scaling 1.0.3: escalador de 6 muestras orientado a bordes con enfoque adaptativo | Detalle nítido | Hasta 2×; a partir de ahí, Lanczos-3 |

El escalador solo vuelve a calcular cuando llega un fotograma nuevo. Si el Quest no admite destinos de renderizado de media precisión, Anime4K pasa a FSR.

- **Efecto retro** (`C`): pasa el ratón por el icono del dock para elegir *Desactivado*, *CRT*, *VHS* o *Arcade*; `C` los recorre en ese orden y un clic en el icono alterna entre *Desactivado* y el último efecto usado. La misma opción está en Ajustes → Imagen → Efecto retro. El efecto se aplica sobre la imagen final, después del escalado 4K, así que ambos se combinan, y aparece también en las capturas, las grabaciones y la pantalla gigante de Quest. Tu elección se guarda.
  - **CRT**: un televisor de tubo, con máscara de ranuras escalonadas, líneas de barrido que se oscurecen según el brillo, pantalla ligeramente abombada y viñeteado suave.
  - **VHS**: una cinta de vídeo doméstica, con imagen algo más suave, colores que se corren hacia la derecha, tonos cálidos algo desvaídos con negros levantados, ligero ruido, un leve vaivén de las líneas y, de vez en cuando, una tenue banda de tracking que baja por la pantalla (el texto sigue siendo legible).
  - **Arcade**: el monitor de una máquina recreativa, con trama de baja resolución y líneas de barrido oscuras y marcadas, rejilla RGB vertical, un ligero brillo en las zonas claras, colores intensos y una leve curvatura.
- **Profundidad 3D** (`D`): estima qué está cerca y qué está lejos en la imagen. Pasa el ratón por el icono, o abre Ajustes → Profundidad 3D, para elegir el **Formato de profundidad** y la **Intensidad del efecto**. *Efecto de profundidad (solo realce 2D)* da más nitidez y viveza al primer plano y vuelve el fondo más suave y difuminado; los formatos estéreo para pantallas 3D se describen más abajo; en el modo inmersivo de Quest, *Pantalla gigante 3D estéreo* convierte la pantalla gigante en 3D estereoscópico de verdad. `D` o un clic en el icono alterna entre *Sin profundidad* y el último formato usado.
- **Brillo, contraste y saturación** (Ajustes → Imagen): de −40 a +40, con 0 como valor neutro. **Gamma**: de 0,70 a 1,40 (1,00 es neutro); por encima de 1,00 aclara los tonos medios y por debajo los oscurece, sin tocar el negro ni el blanco. **Estilos** predefinidos: *Natural*, *Realista* (contraste natural con una suave curva en S y color contenido, sin perder detalle en las sombras), *Intenso*, *Cine* y *Habitación luminosa* (ahora con algo más de gamma, para que las escenas oscuras se vean bien en una habitación iluminada).
- **Espejo** (`X`): invierte toda la imagen de izquierda a derecha.

## Formatos 3D estéreo (Linux / Windows)

En el ordenador, el menú de formato de profundidad (en el icono Profundidad 3D o en Ajustes → Profundidad 3D) ofrece además cuatro formatos estéreo que envían 3D estereoscópico real a una pantalla 3D. Pon la pantalla en el mismo formato 3D y pasa a pantalla completa (`F`); las capturas y las grabaciones siguen siendo 2D. El texto y los elementos del HUD se detectan y se mantienen cerca del plano de la pantalla, y la profundidad es más suave: las letras se desplazan como un bloque en cada ojo y ya no parecen cortadas.

| Formato | Para |
| --- | --- |
| Lado a lado, completo (3840×1080) | Gafas XR en modo 3D, como XREAL, VITURE o RayNeo |
| Lado a lado, mitad | Televisores 3D |
| Arriba y abajo | Televisores 3D y proyectores |
| Gafas rojo / cian | Gafas anaglifo rojo / cian |

## Superposición de cámara web, capturas y grabación

Lo que ves es lo que se guarda: los efectos, el espejo y la superposición de la cámara web quedan integrados en cada captura y cada grabación.

- **Superposición de cámara web** (`W`): tu cámara web sobre el juego, en una de las cuatro esquinas (abajo a la izquierda de forma predeterminada) y en tamaño S, M o L (18, 26 o 36 % del ancho de la imagen).
  - Arrástrala adonde quieras: al soltarla se desliza hasta la esquina más cercana. La rueda del ratón sobre ella cambia el tamaño.
  - Al pasar el ratón por encima aparece una pequeña barra con *Espejo*, *Tamaño* y *Quitar*.
  - Desde el icono del dock (o en Ajustes → Superposición de cámara web) eliges *Cámara*, *Calidad* (720p · 30 fps, 1080p · 30 fps o 1080p · 60 fps), *Tamaño*, *Esquina*, *Espejo* e **Intercambiar imágenes**: la cámara web pasa a ser la imagen principal y la capturadora, la superposición.
- **Captura** (`S`): la imagen actual en PNG.
- **Grabar** (`R`): MP4 con vídeo H.264 y audio AAC a 192 kbit/s. La tasa de bits se ajusta al flujo (ancho × alto × fps × 0,12, entre 8 y 50 Mbit/s: unos 15 Mbit/s en 1080p60 y 50 Mbit/s en 4K60). El indicador de conexión muestra REC y el tiempo transcurrido.
  - **Windows:** no se incluye ningún codificador por software. Para grabar hace falta un codificador gráfico de NVIDIA (NVENC), AMD (AMF) o Intel, o el codificador H.264 integrado en Windows (Media Foundation; las ediciones «N» de Windows necesitan el Media Feature Pack).
  - **Linux:** tampoco se incluye ningún codificador por software. Para grabar hace falta una gráfica Intel o AMD mediante VA-API (con el controlador VA-API instalado), NVIDIA NVENC (controlador propietario) o AMD AMF.
  - Sin ninguno de ellos todo lo demás funciona, pero «Grabar» avisa: «No hay codificador de vídeo: para grabar se necesita una gráfica NVIDIA, AMD o Intel».
- **Grabar micrófono** (`M`): mezcla tu voz con el audio del juego.
- **Nombres y carpetas**: `UVC Studio 2026-10-06 21.30.15.png` o `.mp4` (fecha y hora; si el nombre ya existe se añade un número). Las capturas van a **Imágenes/UVC Studio** y las grabaciones a **Vídeos/UVC Studio**, con el aviso «Guardado en Pictures/UVC Studio» y un botón **Mostrar**; en Ajustes → General también tienes «Abrir carpeta de capturas» y «Abrir carpeta de grabaciones». En Quest, los archivos van a la galería (`Pictures/UVC Studio` y `Movies/UVC Studio`) y se ven en la aplicación Archivos y por USB.

## Meta Quest

Conecta la capturadora al puerto USB-C del visor y juega en una ventana redimensionable o en una pantalla virtual gigante, con 3D estereoscópico real.

- **Instalación**: desde la Meta Horizon Store (próximamente), o instala `UVCStudio-Quest-v1.1.0-Free.apk` desde las Releases de este proyecto con el visor en modo desarrollador (Meta Quest Developer Hub, SideQuest o `adb install`). Platinum solo se puede comprar en la versión instalada desde la tienda.
- **Conexión**: también sirve un adaptador USB-C OTG o un hub. Acepta que UVC Studio se abra al conectar la capturadora (mejor con «Siempre») y concede los permisos de cámara y micrófono que Android exige para las capturadoras USB. Las cámaras del propio visor no se usan nunca.
- **Decodificación**: las capturadoras H.264 y HEVC se decodifican con el decodificador por hardware integrado en el visor; el FFmpeg incluido solo se usa para MJPEG.
- **Ventana 2D** (predeterminada): toda la interfaz de escritorio en un panel de Horizon OS que puedes mover y redimensionar. Pellizca dos veces la imagen para entrar o salir de la pantalla completa.
- **Modo inmersivo**: el icono del visor, al final del dock tras Ajustes, cambia a la pantalla virtual gigante y viceversa. UVC Studio se desconecta, cambia de modo y se vuelve a conectar solo al otro lado.
- **Profundidad 3D en Quest**: en la ventana 2D puedes elegir *Sin profundidad* o *Efecto de profundidad*; en el modo inmersivo, *Sin profundidad* o *Pantalla gigante 3D estéreo*.

| En modo inmersivo | Cómo |
| --- | --- |
| Pulsar en el panel de control | Apunta con el mando y aprieta el gatillo, o pellizca con la mano |
| Mover el panel de control | Agarra la barra blanca que hay debajo y arrástrala; empuja o tira para cambiar la distancia. Ajustes → VR → «Restablecer la posición del panel» lo devuelve a su sitio |
| Mover la pantalla | Agarra la barra blanca que hay debajo y arrástrala; empuja o tira con la mano para cambiar la distancia |
| Cambiar el tamaño | Apunta a una esquina hasta que aparezca un corchete, pellizca y arrastra |
| Acercar / alejar, reducir / ampliar | Joystick arriba / abajo, izquierda / derecha |
| Ocultar o mostrar el panel | Pellizca dos veces sobre la pantalla, o pulsa B, Y o Menú |
| Recentrar | Mantén pulsado el botón Meta |
| Ajustes de pantalla | Ajustes → VR: fondo (passthrough o sala oscura), forma de pantalla (plana o curva), tamaño y distancia de la pantalla, frecuencia de actualización 72 / 90 / 120 Hz (90 de forma predeterminada) |

- **Menú del sistema**: mientras está abierto, la aplicación sigue dibujando, oculta sus punteros y silencia el sonido.
- **Menos calor**: el escalado se detiene en lo que la pantalla gigante puede mostrar de verdad (unos 30 píxeles por grado, 2560 de ancho como máximo) y el par estéreo se renderiza a ese mismo tamaño.
- **Protección contra el sobrecalentamiento**: vigila el estado térmico de Android y los avisos térmicos de OpenXR. Cuando el visor se calienta pasa a FSR y 72 Hz; cuando se calienta demasiado, pausa el escalado y la profundidad 3D. Tus ajustes no cambian, y un aviso te indica cuándo se ha enfriado.
- **Indicador de carga**: durante la conexión, Ajustes → Acerca de muestra el tiempo de GPU por fotograma, la resolución de procesado y la temperatura del visor.

## Interfaz

Toda la interfaz sigue el estilo Liquid Glass: superficies translúcidas y esmeriladas que, mientras estás conectado, dejan ver detrás una copia desenfocada y más saturada del juego.

- **Barra superior**: a la izquierda, el logotipo de UVC Studio; en el centro, el **indicador de conexión** («No conectado»; un punto verde con dispositivo · resolución · fps, más ▸ 4K y el escalador, y · 3D; o un REC rojo que palpita junto al tiempo de grabación); a la derecha, el menú de idioma y **Conectar / Desconectar**. Los errores aparecen en rojo bajo el indicador durante unos segundos.
- **Dock**: cuatro grupos: *Captura, Grabar, Grabar micrófono* · *Escalado 4K, Efecto retro, Profundidad 3D, Espejo* · *Superposición de cámara web, Escucha* · *Ajustes*. Los iconos se amplían bajo el puntero, las funciones activas se iluminan con el degradado de la marca y cada descripción emergente muestra su atajo. La versión está abajo a la izquierda y el botón de pantalla completa, abajo a la derecha.
- **Ajustes**: un gran panel de cristal con tarjetas en un máximo de tres columnas equilibradas, para ajustarlo todo sin desplazarte: Fuente, Sonido, Imagen, Superposición de cámara web, Pantalla, VR (modo inmersivo de Quest), General, Profundidad 3D y Acerca de. En la disposición automática, la tarjeta «Profundidad 3D» queda justo encima de «Acerca de», que muestra la versión, el tiempo de GPU por fotograma y la resolución de procesado durante la conexión (y la temperatura del visor en Quest) y el botón «Licencias de código abierto»; la nota de privacidad aparece en la ventana que abre ese botón.
  - **Contraer una tarjeta**: el botón redondo con flecha en el extremo derecho del encabezado, a la derecha del candado, reduce la tarjeta a su encabezado (el título, el candado y el botón se quedan) y la vuelve a abrir. Las demás tarjetas suben, ninguna cambia de columna y las tarjetas contraídas se recuerdan.
  - **Organiza tú mismo las tarjetas**: cada tarjeta tiene un candado arriba a la derecha de su cabecera, y al principio todas están bloqueadas. Haz clic en el candado («Desbloquear para mover») y arrastra la tarjeta por su cabecera a otro lugar de las tres columnas; un hueco indica dónde quedará, y `Esc` cancela el arrastre. Otro clic en el candado («Bloquear posición») la deja fija. Las tarjetas bloqueadas se quedan en su sitio y las desbloqueadas se mueven entre ellas. La disposición y las tarjetas desbloqueadas se recuerdan para el próximo inicio. «Restablecer disposición», junto al botón de cerrar y visible solo mientras usas tu propia disposición, vuelve a la disposición automática y bloquea todas las tarjetas. Si la ventana es demasiado estrecha para tres columnas, se muestra la disposición automática y la tuya se conserva para cuando la ventana vuelva a ser lo bastante ancha. En la ventana 2D de Quest funciona igual con el puntero del mando.
- **Idiomas**: 台灣正體中文, English, Deutsch, Français, Italiano, Español, Português, 日本語, 한국어, 简体中文, ไทย y Tiếng Việt, desde el menú de cristal de arriba a la derecha. La primera vez, la aplicación usa el idioma del sistema.
- **Temas**: Automático (claro de 06:00 a 18:00), Oscuro o Claro.
- **Tamaño de la interfaz**: se adapta a la pantalla (1080p 100 %, 1440p unos 135 %, 4K 200 %); puedes cambiarlo en Ajustes → Pantalla o con `Ctrl` `+` / `−` / `0`.
- **Tipografías e iconos**: Manrope e IBM Plex Sans (IBM Plex Sans Thai para el tailandés y Noto Sans CJK TC para chino, japonés y coreano); los iconos son diseño propio de UVC Studio.
- **Pantalla completa**: `F`, `F11`, el botón de abajo a la derecha o doble clic en la imagen; `Esc` para salir.

## Atajos de teclado (Linux / Windows)

Pulsa `?` o `H` en la aplicación para ver esta misma lista; cualquier tecla o un clic la cierra.

| Tecla | Acción |
| --- | --- |
| `Intro` | Conectar |
| `Ctrl+D` | Desconectar |
| `S` | Captura |
| `R` | Iniciar o detener la grabación |
| `M` | Activar / desactivar la grabación del micrófono |
| `L` | Activar / desactivar la escucha |
| `W` | Activar / desactivar la superposición de cámara web |
| `X` | Espejo |
| `U` | Activar / desactivar el escalado 4K |
| `Mayús+U` | Siguiente escalador (activa el escalado) |
| `C` | Efecto retro: Desactivado → CRT → VHS → Arcade |
| `D` | Profundidad 3D: alterna entre *Sin profundidad* y el último formato |
| `O` o `Ctrl+,` | Ajustes |
| `F` o `F11` | Pantalla completa |
| `Esc` | Cancelar el arrastre de una tarjeta, salir de pantalla completa o cerrar los ajustes |
| `?` o `H` | Atajos de teclado |
| `Ctrl` `+` / `−` / `0` | Interfaz más grande / más pequeña / restablecer |

## Privacidad

UVC Studio funciona íntegramente en tu dispositivo. La imagen, el sonido y las grabaciones nunca salen de él: sin cuenta, sin subidas, sin rastreo, y la aplicación no se conecta nunca a la red. En Meta Quest, la compra de Platinum y su comprobación pasan por el servicio Meta Horizon Store del visor; UVC Studio no envía nada. La política de privacidad completa está en [PRIVACY.md](PRIVACY.md).

## Licencias

UVC Studio para Linux y Windows es de código abierto con la [licencia MIT](LICENSE); la versión para Meta Quest no es de código abierto. UVC Studio no contiene componentes GPL; FFmpeg se compila solo con LGPL (en Quest, únicamente con el decodificador MJPEG y como bibliotecas compartidas reemplazables). Otros componentes: SDL2, Dear ImGui, miniaudio, stb, NanoSVG, libusb y libuvc (Quest), el cargador de OpenXR, AMD FidelityFX FSR 1 y CAS, Anime4K y NVIDIA Image Scaling. Las tipografías (Manrope, IBM Plex Sans, IBM Plex Sans Thai, Noto Sans CJK TC) se distribuyen con la SIL Open Font License. Todos los componentes y el texto completo de sus licencias están en la aplicación, en Ajustes → Acerca de → Licencias de código abierto. La versión para Meta Quest también incluye el cargador del Meta Platform SDK (Oculus SDK License) para la compra en la tienda.
