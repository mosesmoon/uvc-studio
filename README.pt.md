# UVC Studio — guia de funcionalidades

[台灣正體中文](README.zh-TW.md) · [English](README.md) · [Deutsch](README.de.md) · [Français](README.fr.md) · [Italiano](README.it.md) · [Español](README.es.md) · **Português** · [日本語](README.ja.md) · [한국어](README.ko.md) · [简体中文](README.zh-CN.md) · [ไทย](README.th.md) · [Tiếng Việt](README.vi.md)

O UVC Studio 1.1.0 é uma aplicação nativa para placas de captura, totalmente offline, para Linux, Windows e Meta Quest: jogue nas suas consolas através de qualquer placa de captura UVC com latência mínima, aumente a imagem para 4K, acrescente um efeito retro ou profundidade 3D, sobreponha a webcam ao jogo, grave e faça capturas — sem nunca aceder à rede. A 1.1.0, a primeira versão oficial, traz o UVC Studio Platinum para o Meta Quest: o upscaling 4K e a profundidade 3D continuam grátis durante 30 minutos por dia cada um, e uma compra única desbloqueia-os para sempre; as versões para Linux e Windows continuam gratuitas e passam a ser de código aberto sob a licença MIT.

## Plataformas e transferências

Um único código-fonte: as mesmas funcionalidades e o mesmo número de versão em todas as plataformas.

| Plataforma | Transferência | Captura de vídeo | Áudio | Gráficos | Gravação |
| --- | --- | --- | --- | --- | --- |
| Linux (x86_64) | [`UVCStudio-v1.1.0-x86_64.AppImage`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-v1.1.0-x86_64.AppImage) | Video4Linux2 | PulseAudio / PipeWire / ALSA | OpenGL 3.3 | H.264 (por hardware: VA-API / NVENC / AMF) + AAC, MP4 — é necessário um deles |
| Windows (x64) | [`UVCStudio-Setup-v1.1.0.exe`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-Setup-v1.1.0.exe) (instalador) ou [`UVCStudio-Portable-v1.1.0.zip`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-Portable-v1.1.0.zip) (portátil) | Media Foundation | WASAPI | OpenGL 3.3 | H.264 (NVENC / AMF / Intel através do Media Foundation ou, em alternativa, o codificador integrado no Windows) + AAC, MP4 — é necessário um deles |
| Meta Quest 2 / Pro / 3 / 3S | [`UVCStudio-Quest-v1.1.0-Free.apk`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-Quest-v1.1.0-Free.apk) (sideload) ou Meta Horizon Store (em breve) | Anfitrião USB, libuvc | Áudio USB da placa, lido diretamente (estéreo ou 5.1) | OpenGL ES 3 (janela 2D), OpenXR (imersivo) | H.264 + AAC (MediaCodec por hardware), MP4 |

Código-fonte (Linux e Windows, licença MIT): este repositório ou [`UVCStudio-v1.1.0-src.tar.gz`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-v1.1.0-src.tar.gz).

No Meta Quest, o upscaling 4K e a profundidade 3D são grátis durante 30 minutos por dia cada um; o UVC Studio Platinum, uma compra única na aplicação, torna-os ilimitados. No Linux e no Windows todas as funções são gratuitas.

## Primeiros passos

1. Ligue a placa de captura por USB e abra o UVC Studio. O **ecrã inicial** verifica em tempo real três itens: *Placa de captura*, *Áudio do jogo* e *Webcam (opcional)*. Se faltar a placa, aparece «Não encontrada — ligue-a por USB»; se faltar o áudio, «Não encontrado (só vídeo)».
2. Clique em **Ligar** (no ecrã inicial ou no canto superior direito) ou prima `Enter`. Com **Ligar automaticamente** ativo (Definições → Geral, ativo por predefinição), o UVC Studio liga-se sozinho assim que uma placa é ligada ao computador, e também ao abrir, se ela já lá estiver.
3. Jogue. **Desligar** (o mesmo botão, ou `Ctrl+D`) fecha tudo; uma gravação em curso é primeiro concluída e guardada.

Já não há pausa nem retoma: o UVC Studio está ligado ou não está. Se desligar a placa, surge um erro por baixo do indicador de ligação e a aplicação volta ao ecrã inicial; com a ligação automática ativa, volta a ligar-se assim que a placa regressar.

## Captura e som

Qualquer placa de captura UVC funciona sem controladores adicionais: ShadowCast, Elgato, AVerMedia, Magewell e as habituais pens HDMI USB 3 comportam-se da mesma forma.

- **Os modos reais da placa**: resoluções e taxas de fotogramas são as que a própria placa indica, da maior para a menor, com a etiqueta 4K, QHD, Full HD, HD ou SD. Por predefinição, o UVC Studio escolhe a maior resolução que atinge 59 fps ou mais. Formato: Automático, Sem compressão (menor atraso) ou Comprimido (resoluções e taxas mais altas).
- **Escolhe os dispositivos certos**: enquanto nada estiver guardado, a placa de captura, o respetivo áudio e a webcam são escolhidos pelo nome, para que uma câmara integrada ou virtual não seja confundida com a placa. Dispositivos com o mesmo nome recebem o acréscimo (2), (3)…
- **Baixa latência**: é sempre mostrado o fotograma mais recente, nunca uma fila de fotogramas atrasados.
- **Memoriza a sua configuração** e deteta placas ligadas ou desligadas com a aplicação aberta.
- **Ouvir** (`L`): ouça o jogo nas colunas ou nos auscultadores. Ao passar o rato sobre o ícone da dock encontra um interruptor, o volume e a saída de áudio atual; a linha «Saída» leva diretamente a Definições → Som.
- **Modo real à vista**: Definições → Origem mostra o fluxo efetivo, por exemplo 3840 × 2160 · 60 fps · YUY2.
- **Áudio no Meta Quest**: a aplicação lê o som da placa diretamente por USB (USB Audio Class 1 e 2, incluindo as pens MacroSilicon MS2109) — Definições → Som → Entrada de áudio → «Áudio da placa de captura (USB)». Definições → Som → «Canais de áudio do jogo»: **Estéreo 2CH** (predefinição) ou **Surround 6CH** — 5.1 com placas que têm modo de 6 canais (como a CJ-16) e consolas que enviam 5.1 por HDMI (como a Switch 2 em Surround); as gravações continuam em estéreo.
- **Recuperação automática no Quest**: se a placa deixar de enviar imagem, a aplicação volta a abri-la sozinha (até duas vezes) e, se não resultar, pede-lhe que verifique a fonte HDMI ou que volte a ligar a placa.

## Imagem

Quatro upscalers 4K, três efeitos retro e a profundidade 3D são processados na GPU, fotograma a fotograma, sem outro atraso além do tempo de cálculo da GPU.

**Upscaling 4K** (`U`): clique no ícone da dock ou prima `U` para o ativar ou desativar, passe o rato sobre o ícone para escolher o modo e use `Shift+U` para passar ao seguinte. Em Definições → Imagem → Upscaling 4K encontra *Desativado*, *FSR*, *Anime4K*, *Lanczos* e *NIS*: escolher um modo ativa o upscaling, e *Desativado* guarda o último modo para a próxima vez.

| Modo | Como funciona | Ideal para | Fator |
| --- | --- | --- | --- |
| Upscaling 4K FSR (predefinido) | AMD FidelityFX Super Resolution 1: ampliação adaptativa às arestas (EASU) mais nitidez RCAS | Jogos e qualquer vídeo | De 720p / 1080p para 4K; fontes SD até 3× |
| Upscaling 4K Anime4K para animação | Rede neural Anime4K ×2 (tamanho M no computador, S no Quest) e FSR para o resto | Anime, desenhos animados, desenho de traço | 2× pela rede, depois até 4K |
| Upscaling 4K Lanczos + CAS | Reamostragem Lanczos-3 (6 amostras por eixo) mais nitidez AMD FidelityFX CAS | Ampliação clássica e neutra | Até 4K |
| Upscaling 4K NIS | NVIDIA Image Scaling 1.0.3: ampliação de 6 amostras orientada às arestas com nitidez adaptativa | Detalhe nítido | Até 2×; a partir daí, Lanczos-3 |

O upscaler só volta a calcular quando chega um fotograma novo. Se o Quest não suportar alvos de renderização de meia precisão, o Anime4K passa a usar FSR.

- **Efeito retro** (`C`): passe o rato sobre o ícone da dock para escolher *Desativado*, *CRT*, *VHS* ou *Arcada*; `C` percorre-os por esta ordem e um clique no ícone alterna entre *Desativado* e o último efeito usado. A mesma escolha está em Definições → Imagem → Efeito retro. O efeito é aplicado à imagem final, depois do upscaling 4K, por isso os dois combinam-se, e aparece também nas capturas, nas gravações e no ecrã gigante do Quest. A escolha fica guardada.
  - **CRT**: um televisor de tubo — máscara de fendas desencontradas, linhas de varrimento que escurecem consoante o brilho, ecrã ligeiramente abaulado e vinheta suave.
  - **VHS**: uma cassete de vídeo caseira — imagem um pouco mais suave, cores a escorrer para a direita, tons quentes ligeiramente desbotados com pretos levantados, ligeiro ruído, uma leve ondulação das linhas e, de vez em quando, uma faixa de tracking ténue a descer pelo ecrã (o texto continua legível).
  - **Arcada**: o monitor de uma máquina de arcada — trama de baixa resolução com linhas de varrimento escuras e marcadas, grelha RGB vertical, um ligeiro brilho nas zonas claras, cores vibrantes e uma leve curvatura.
- **Profundidade 3D** (`D`): estima o que está perto e o que está longe na imagem. Passe o rato sobre o ícone, ou abra Definições → Profundidade 3D, para escolher o **Formato de profundidade** e a **Intensidade do efeito**. *Efeito de profundidade (só realce 2D)* torna o primeiro plano mais nítido e vivo e o fundo mais suave e difuso; os formatos estéreo para ecrãs 3D são descritos mais abaixo; no modo imersivo do Quest, *Ecrã gigante 3D estéreo* transforma o ecrã gigante em 3D estereoscópico verdadeiro. `D` ou um clique no ícone alterna entre *Sem profundidade* e o último formato usado.
- **Brilho, contraste e saturação** (Definições → Imagem): de −40 a +40, sendo 0 o valor neutro. **Gama**: de 0,70 a 1,40 (1,00 é neutro); acima de 1,00 clareia os meios-tons, abaixo escurece-os, sem alterar o preto nem o branco. **Estilos** predefinidos: *Natural*, *Realista* (contraste natural com uma suave curva em S e cor contida, mantendo o detalhe nas sombras), *Vibrante*, *Cinema* e *Sala iluminada* (agora com um pouco mais de gama, para as cenas escuras se verem bem numa sala com luz).
- **Espelhar** (`X`): inverte toda a imagem da esquerda para a direita.

## Formatos 3D estéreo (Linux / Windows)

No computador, o menu de formato de profundidade (no ícone Profundidade 3D ou em Definições → Profundidade 3D) oferece ainda quatro formatos estéreo que enviam 3D estereoscópico verdadeiro para um ecrã 3D. Configure o ecrã para o mesmo formato 3D e passe a ecrã inteiro (`F`); as capturas e as gravações continuam em 2D. O texto e os elementos do HUD são detetados e mantidos perto do plano do ecrã, e a profundidade é mais suave: as letras deslocam-se como um bloco em cada olho e já não parecem cortadas.

| Formato | Para |
| --- | --- |
| Lado a lado, completo (3840×1080) | Óculos XR em modo 3D, como XREAL, VITURE e RayNeo |
| Lado a lado, metade | Televisores 3D |
| Cima e baixo | Televisores 3D e projetores |
| Óculos vermelho / ciano | Óculos anáglifos vermelho / ciano |

## Sobreposição da webcam, capturas e gravação

O que vê é o que fica guardado: efeitos, espelho e sobreposição da webcam ficam integrados em todas as capturas e gravações.

- **Sobreposição da webcam** (`W`): a sua webcam por cima do jogo, num dos quatro cantos (inferior esquerdo por predefinição) e no tamanho S, M ou L (18, 26 ou 36 % da largura da imagem).
  - Arraste-a para onde quiser: ao largar, desliza para o canto mais próximo. A roda do rato por cima dela altera o tamanho.
  - Ao passar o rato por cima aparece uma pequena barra com *Espelhar*, *Tamanho* e *Remover*.
  - No ícone da dock (ou em Definições → Sobreposição da webcam) escolhe *Câmara*, *Qualidade* (720p · 30 fps, 1080p · 30 fps ou 1080p · 60 fps), *Tamanho*, *Canto*, *Espelhar* e **Trocar imagens**: a webcam passa a imagem principal e a placa de captura a sobreposição.
- **Captura** (`S`): a imagem atual em PNG.
- **Gravar** (`R`): MP4 com vídeo H.264 e áudio AAC a 192 kbit/s. A taxa de bits acompanha o fluxo (largura × altura × fps × 0,12, entre 8 e 50 Mbit/s: cerca de 15 Mbit/s em 1080p60, 50 Mbit/s em 4K60). O indicador de ligação mostra REC e o tempo decorrido.
  - **Windows:** não é incluído nenhum codificador por software. Para gravar é necessário um codificador gráfico NVIDIA (NVENC), AMD (AMF) ou Intel, ou o codificador H.264 integrado no Windows (Media Foundation; as edições «N» do Windows precisam do Media Feature Pack).
  - **Linux:** também não é incluído nenhum codificador por software. Para gravar são necessários gráficos Intel ou AMD através de VA-API (com o controlador VA-API instalado), NVIDIA NVENC (controlador proprietário) ou AMD AMF.
  - Sem nenhum deles, tudo o resto funciona, mas «Gravar» indica «Sem codificador de vídeo: a gravação requer gráficos NVIDIA, AMD ou Intel».
- **Gravar microfone** (`M`): mistura a sua voz com o áudio do jogo.
- **Nomes e pastas**: `UVC Studio 2026-10-06 21.30.15.png` ou `.mp4` (data e hora; se o nome já existir, é acrescentado um número). As capturas vão para **Imagens/UVC Studio** e as gravações para **Vídeos/UVC Studio**, com o aviso «Guardado em Pictures/UVC Studio» e um botão **Mostrar**; em Definições → Geral encontra ainda «Abrir pasta de capturas» e «Abrir pasta de gravações». No Quest, os ficheiros vão para a Galeria (`Pictures/UVC Studio` e `Movies/UVC Studio`), visíveis na aplicação Ficheiros e por USB.

## Meta Quest

Ligue a placa de captura à porta USB-C do headset e jogue numa janela redimensionável ou num ecrã virtual gigante, com 3D estereoscópico verdadeiro.

- **Instalação**: a partir da Meta Horizon Store (em breve), ou instale `UVCStudio-Quest-v1.1.0-Free.apk` a partir das Releases deste projeto com o headset em modo de programador (Meta Quest Developer Hub, SideQuest ou `adb install`). O Platinum só pode ser comprado na versão instalada a partir da loja.
- **Ligação**: também funciona com um adaptador USB-C OTG ou um hub. Aceite que o UVC Studio abra quando a placa for ligada (de preferência com «Sempre») e conceda as permissões de câmara e microfone que o Android exige para as placas de captura USB. As câmaras do próprio headset nunca são usadas.
- **Descodificação**: as placas H.264 e HEVC são descodificadas pelo descodificador por hardware integrado no headset; o FFmpeg incluído serve apenas para MJPEG.
- **Janela 2D** (predefinida): toda a interface do computador num painel do Horizon OS que pode mover e redimensionar. Faça duplo pinch na imagem para entrar ou sair do ecrã inteiro.
- **Modo imersivo**: o ícone do headset, no fim da dock a seguir a Definições, muda para o ecrã virtual gigante e de volta. O UVC Studio desliga-se, muda de modo e volta a ligar-se sozinho do outro lado.
- **Profundidade 3D no Quest**: na janela 2D pode escolher *Sem profundidade* ou *Efeito de profundidade*; no modo imersivo, *Sem profundidade* ou *Ecrã gigante 3D estéreo*.

| No modo imersivo | Como |
| --- | --- |
| Clicar no painel de controlo | Aponte com o comando e prima o gatilho, ou faça pinch com a mão |
| Mover o painel de controlo | Agarre a barra branca por baixo e arraste; empurre ou puxe para mudar a distância. Definições → VR → «Repor a posição do painel» volta a colocá-lo no sítio |
| Mover o ecrã | Agarre a barra branca por baixo e arraste; empurre ou puxe a mão para mudar a distância |
| Redimensionar o ecrã | Aponte para um canto até aparecer um parêntese, depois faça pinch e arraste |
| Aproximar / afastar, reduzir / aumentar | Joystick para cima / baixo, esquerda / direita |
| Ocultar ou mostrar o painel | Duplo pinch no ecrã, ou prima B, Y ou Menu |
| Recentrar | Mantenha premido o botão Meta |
| Predefinições do ecrã | Definições → VR: fundo (passthrough ou sala escura), forma do ecrã (plano ou curvo), tamanho e distância do ecrã, taxa de atualização 72 / 90 / 120 Hz (predefinição 90) |

- **Menu do sistema**: enquanto está aberto, a aplicação continua a desenhar, oculta os ponteiros e silencia o som.
- **Menos calor**: o upscaling para no que o ecrã gigante consegue realmente mostrar (cerca de 30 píxeis por grau, no máximo 2560 de largura) e o par estéreo é renderizado com esse mesmo tamanho.
- **Proteção contra sobreaquecimento**: acompanha o estado térmico do Android e as notificações térmicas do OpenXR. Quando o headset aquece, muda para FSR e 72 Hz; quando fica demasiado quente, coloca em pausa o upscaling e a profundidade 3D. As suas definições não são alteradas e um aviso indica quando arrefeceu.
- **Indicador de carga**: enquanto está ligado, Definições → Acerca de mostra o tempo de GPU por fotograma, a resolução de processamento e a temperatura do headset.

## Interface

Toda a interface segue o estilo Liquid Glass: superfícies translúcidas e foscas que, enquanto está ligado, deixam ver por trás uma cópia desfocada e mais saturada do jogo.

- **Barra superior**: à esquerda, o logótipo UVC Studio; ao centro, o **indicador de ligação** («Não ligado»; um ponto verde com dispositivo · resolução · fps, mais ▸ 4K e o upscaler, e · 3D; ou um REC vermelho a pulsar com o tempo de gravação); à direita, o menu de idioma e **Ligar / Desligar**. Os erros aparecem a vermelho por baixo do indicador durante alguns segundos.
- **Dock**: quatro grupos — *Captura, Gravar, Gravar microfone* · *Upscaling 4K, Efeito retro, Profundidade 3D, Espelhar* · *Sobreposição da webcam, Ouvir* · *Definições*. Os ícones aumentam sob o ponteiro, as funções ativas acendem-se com o gradiente da marca e cada dica mostra o respetivo atalho. A versão fica no canto inferior esquerdo e o botão de ecrã inteiro no canto inferior direito.
- **Definições**: um grande painel de vidro com cartões em, no máximo, três colunas equilibradas, para ajustar tudo sem deslocar: Origem, Som, Imagem, Sobreposição da webcam, Visualização, VR (modo imersivo do Quest), Geral, Profundidade 3D e Acerca de — na disposição automática, o cartão «Profundidade 3D» fica logo acima de «Acerca de». O cartão «Acerca de» mostra a versão, o tempo de GPU por fotograma e a resolução de processamento enquanto está ligado (e a temperatura do headset no Quest) e o botão «Licenças de código aberto»; a nota de privacidade aparece na janela que esse botão abre.
  - **Recolher um cartão**: o botão redondo com seta no extremo direito do cabeçalho, à direita do cadeado, reduz o cartão ao cabeçalho (o título, o cadeado e o botão ficam) e volta a abri-lo. Os outros cartões sobem, nenhum muda de coluna e os cartões recolhidos são memorizados.
  - **Organize os cartões à sua maneira**: cada cartão tem um cadeado no canto superior direito do cabeçalho e, de início, todos os cartões estão bloqueados. Clique no cadeado («Desbloquear para mover») e arraste o cartão pelo cabeçalho para outro lugar das três colunas; um espaço vazio mostra onde vai ficar, e `Esc` cancela o arrasto. Um novo clique no cadeado («Bloquear posição») fixa o cartão. Os cartões bloqueados ficam no lugar e os desbloqueados deslocam-se entre eles. A disposição e os cartões desbloqueados ficam memorizados para o próximo arranque. «Repor disposição», junto ao botão de fechar e visível apenas enquanto a sua própria disposição está em uso, repõe a disposição automática e bloqueia todos os cartões. Se a janela for demasiado estreita para três colunas, é mostrada a disposição automática e a sua fica guardada para quando a janela voltar a ser suficientemente larga. Na janela 2D do Quest funciona da mesma forma com o ponteiro do comando.
- **Idiomas**: 台灣正體中文, English, Deutsch, Français, Italiano, Español, Português, 日本語, 한국어, 简体中文, ไทย e Tiếng Việt, no menu de vidro do canto superior direito. Na primeira utilização, a aplicação segue o idioma do sistema.
- **Temas**: Automático (claro das 06:00 às 18:00), Escuro ou Claro.
- **Tamanho da interface**: acompanha o ecrã (1080p 100 %, 1440p cerca de 135 %, 4K 200 %); pode alterá-lo em Definições → Visualização ou com `Ctrl` `+` / `−` / `0`.
- **Tipos de letra e ícones**: Manrope e IBM Plex Sans (IBM Plex Sans Thai para tailandês e Noto Sans CJK TC para chinês, japonês e coreano); os ícones foram desenhados de raiz para o UVC Studio.
- **Ecrã inteiro**: `F`, `F11`, o botão do canto inferior direito ou duplo clique na imagem; `Esc` para sair.

## Atalhos de teclado (Linux / Windows)

Prima `?` ou `H` na aplicação para ver esta mesma lista; qualquer tecla ou um clique fecha-a.

| Tecla | Ação |
| --- | --- |
| `Enter` | Ligar |
| `Ctrl+D` | Desligar |
| `S` | Captura |
| `R` | Iniciar ou parar a gravação |
| `M` | Ativar / desativar a gravação do microfone |
| `L` | Ativar / desativar Ouvir |
| `W` | Ativar / desativar a sobreposição da webcam |
| `X` | Espelhar |
| `U` | Ativar / desativar o upscaling 4K |
| `Shift+U` | Upscaler seguinte (ativa o upscaling) |
| `C` | Efeito retro: Desativado → CRT → VHS → Arcada |
| `D` | Profundidade 3D: alterna entre *Sem profundidade* e o último formato |
| `O` ou `Ctrl+,` | Definições |
| `F` ou `F11` | Ecrã inteiro |
| `Esc` | Cancelar o arrasto de um cartão, sair do ecrã inteiro ou fechar as definições |
| `?` ou `H` | Atalhos de teclado |
| `Ctrl` `+` / `−` / `0` | Interface maior / menor / repor |

## Privacidade

O UVC Studio funciona inteiramente no seu dispositivo. A imagem, o som e as gravações nunca saem dele — sem conta, sem envios, sem rastreio, e a aplicação nunca acede à rede. No Meta Quest, a compra do Platinum e a sua verificação passam pelo serviço Meta Horizon Store do headset; o UVC Studio não envia nada.

## Licenças

O UVC Studio para Linux e Windows é de código aberto sob a [licença MIT](LICENSE); a versão para Meta Quest não é de código aberto. O UVC Studio não contém componentes GPL; o FFmpeg é compilado apenas com LGPL (no Quest, só com o descodificador MJPEG e como bibliotecas partilhadas substituíveis). Outros componentes: SDL2, Dear ImGui, miniaudio, stb, NanoSVG, libusb e libuvc (Quest), o carregador OpenXR, AMD FidelityFX FSR 1 e CAS, Anime4K e NVIDIA Image Scaling. Os tipos de letra (Manrope, IBM Plex Sans, IBM Plex Sans Thai, Noto Sans CJK TC) estão sob a SIL Open Font License. Todos os componentes e o texto integral das respetivas licenças estão na aplicação, em Definições → Acerca de → Licenças de código aberto. A versão para Meta Quest inclui também o carregador do Meta Platform SDK (Oculus SDK License) para a compra na loja.
