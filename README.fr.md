# UVC Studio — présentation des fonctionnalités

[English](README.md) · [台灣正體中文](README.zh-TW.md) · [Deutsch](README.de.md) · **Français** · [Italiano](README.it.md) · [Español](README.es.md) · [Português](README.pt.md) · [日本語](README.ja.md) · [한국어](README.ko.md) · [简体中文](README.zh-CN.md) · [ไทย](README.th.md) · [Tiếng Việt](README.vi.md)

UVC Studio 1.1.0 est une application native pour cartes de capture, entièrement hors ligne, disponible sous Linux, Windows et sur Meta Quest. Jouez sur vos consoles via n'importe quelle carte de capture UVC avec une latence minimale, passez l'image en 4K, ajoutez un rendu rétro ou de la profondeur 3D, incrustez votre webcam, enregistrez et faites des captures — sans jamais accéder au réseau. La version 1.1.0, la première version officielle, apporte UVC Studio Platinum sur Meta Quest : l'upscaling 4K et la profondeur 3D restent gratuits 30 minutes par jour chacun, et un achat unique les débloque pour toujours ; les versions Linux et Windows restent gratuites et sont désormais open source sous licence MIT.

## Plateformes et téléchargements

Un seul code source : mêmes fonctionnalités et même numéro de version sur toutes les plateformes.

| Plateforme | Téléchargement | Capture vidéo | Audio | Graphismes | Enregistrement |
| --- | --- | --- | --- | --- | --- |
| Linux (x86_64) | [`UVCStudio-v1.1.0-x86_64.AppImage`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-v1.1.0-x86_64.AppImage) | Video4Linux2 | PulseAudio / PipeWire / ALSA | OpenGL 3.3 | H.264 (matériel VA-API / NVENC / AMF) + AAC, MP4 — l'un d'eux est requis |
| Windows (x64) | [`UVCStudio-Setup-v1.1.0.exe`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-Setup-v1.1.0.exe) (installateur) ou [`UVCStudio-Portable-v1.1.0.zip`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-Portable-v1.1.0.zip) (portable) | Media Foundation | WASAPI | OpenGL 3.3 | H.264 (NVENC / AMF / Intel via Media Foundation, sinon l'encodeur intégré à Windows) + AAC, MP4 — l'un d'eux est requis |
| Meta Quest 2 / Pro / 3 / 3S | [`UVCStudio-Quest-v1.1.0-Free.apk`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-Quest-v1.1.0-Free.apk) (sideload) ou Meta Horizon Store (bientôt) | Hôte USB, libuvc | Son USB de la carte, lu directement (stéréo ou 5.1) | OpenGL ES 3 (fenêtre 2D), OpenXR (immersif) | H.264 + AAC (MediaCodec matériel), MP4 |

Code source (Linux et Windows, licence MIT) : ce dépôt, ou [`UVCStudio-v1.1.0-src.tar.gz`](https://github.com/mosesmoon/uvc-studio/releases/download/v1.1.0/UVCStudio-v1.1.0-src.tar.gz).

Sur Meta Quest, l'upscaling 4K et la profondeur 3D sont gratuits 30 minutes par jour chacun ; UVC Studio Platinum, un achat intégré unique, les rend illimités. Sous Linux et Windows, toutes les fonctions sont gratuites.

## Prise en main

1. Branchez la carte de capture en USB et lancez UVC Studio. L'**écran d'accueil** vérifie en direct trois éléments : *Carte de capture*, *Son du jeu* et *Webcam (facultative)*. Si la carte manque, il affiche « Introuvable — branchez-la en USB » ; si le son manque, « Introuvable (vidéo seule) ».
2. Cliquez sur **Connecter** (sur l'écran d'accueil ou en haut à droite) ou appuyez sur `Entrée`. Avec **Connexion automatique** activée (Réglages → Général, activée par défaut), UVC Studio se connecte tout seul dès qu'une carte est branchée, ainsi qu'au lancement si elle l'est déjà.
3. Jouez. **Déconnecter** (le même bouton, ou `Ctrl+D`) ferme tout ; un enregistrement en cours est d'abord terminé et sauvegardé.

Plus de pause ni de reprise : UVC Studio est connecté ou il ne l'est pas. Si la carte est débranchée, un message d'erreur s'affiche sous l'indicateur de connexion et l'application revient à l'écran d'accueil ; avec la connexion automatique, elle se reconnecte dès que la carte revient.

## Capture et son

Toutes les cartes de capture UVC fonctionnent sans pilote supplémentaire : ShadowCast, Elgato, AVerMedia, Magewell ou simple dongle HDMI USB 3, tout se passe de la même façon.

- **Les modes réels de la carte** : résolutions et fréquences d'images proviennent de ce que la carte annonce, de la plus grande à la plus petite, avec une étiquette 4K, QHD, Full HD, HD ou SD. Par défaut, UVC Studio choisit la plus grande résolution qui atteint au moins 59 i/s. Format : Auto, Non compressé (latence minimale) ou Compressé (résolutions et fréquences plus élevées).
- **Les bons périphériques** : tant que rien n'est enregistré, la carte de capture, son audio et la webcam sont choisis d'après leur nom ; une caméra intégrée ou virtuelle n'est donc pas prise pour la carte. Les périphériques portant le même nom reçoivent un suffixe (2), (3)…
- **Faible latence** : c'est toujours l'image la plus récente qui s'affiche, jamais une file d'images en retard.
- **Mémorise votre configuration** et détecte les cartes branchées ou débranchées pendant l'utilisation.
- **Écoute** (`L`) : entendez le jeu sur vos haut-parleurs ou votre casque. En survolant la tuile du dock, vous trouvez un interrupteur, le volume et la sortie audio actuelle ; la ligne « Sortie » mène directement à Réglages → Son.
- **Mode réel affiché** : Réglages → Source indique le flux effectif, par exemple 3840 × 2160 · 60 fps · YUY2.
- **Son sur Meta Quest** : l'application lit le son de la carte directement par USB (USB Audio Class 1 et 2, y compris les dongles MacroSilicon MS2109) — Réglages → Son → Entrée audio → « Audio de la carte de capture (USB) ». Réglages → Son → « Canaux audio du jeu » : **Stéréo 2CH** (par défaut) ou **Surround 6CH** — du 5.1 avec les cartes dotées d'un mode 6 canaux (comme la CJ-16) et les consoles qui sortent du 5.1 en HDMI (comme la Switch 2 réglée sur Surround) ; les enregistrements restent en stéréo.
- **Reprise automatique sur Quest** : si la carte n'envoie plus d'image, l'application la rouvre d'elle-même (deux fois au maximum), puis vous invite à vérifier la source HDMI ou à rebrancher la carte.

## Image

Quatre upscalers 4K, trois rendus rétro et la profondeur 3D tournent sur le GPU, image par image, sans autre délai que le temps de calcul du GPU.

**Upscaling 4K** (`U`) : cliquez sur la tuile du dock ou appuyez sur `U` pour l'activer ou le désactiver, survolez la tuile pour choisir le mode, et `Maj+U` passe au suivant. Réglages → Image → Upscaling 4K propose *Désactivé*, *FSR*, *Anime4K*, *Lanczos* et *NIS* : choisir un mode active l'upscaling, et *Désactivé* garde le dernier mode en mémoire pour la prochaine fois.

| Mode | Principe | Idéal pour | Facteur |
| --- | --- | --- | --- |
| Upscaling 4K FSR (par défaut) | AMD FidelityFX Super Resolution 1 : agrandissement adaptatif aux contours (EASU) puis netteté RCAS | Jeux et toute vidéo | 720p / 1080p vers 4K ; sources SD jusqu'à 3× |
| Upscaling 4K Anime4K pour l'animation | Réseau neuronal Anime4K ×2 (taille M sur ordinateur, S sur Quest), puis FSR pour le reste | Animés, dessins animés, dessin au trait | 2× par le réseau, puis jusqu'à la 4K |
| Upscaling 4K Lanczos + CAS | Rééchantillonnage Lanczos-3 (6 points par axe) puis netteté AMD FidelityFX CAS | Mise à l'échelle classique et neutre | Jusqu'à la 4K |
| Upscaling 4K NIS | NVIDIA Image Scaling 1.0.3 : agrandissement 6 points orienté contours avec netteté adaptative | Détails nets | Jusqu'à 2×, puis Lanczos-3 |

L'upscaler ne recalcule que lorsqu'une nouvelle image arrive. Si le Quest ne prend pas en charge les cibles de rendu en demi-flottant, Anime4K bascule sur FSR.

- **Rendu rétro** (`C`) : survolez la tuile du dock pour choisir *Désactivé*, *CRT*, *VHS* ou *Arcade* ; `C` les fait défiler dans cet ordre, et un clic sur la tuile alterne entre *Désactivé* et le dernier rendu utilisé. Le même choix se trouve dans Réglages → Image → Rendu rétro. Le rendu s'applique à l'image finale, après l'upscaling 4K : les deux se combinent, et il apparaît aussi dans les captures, les enregistrements et sur l'écran géant du Quest. Votre choix est mémorisé.
  - **CRT** : un téléviseur à tube — masque à fentes en quinconce, lignes de balayage qui s'assombrissent selon la luminosité, écran légèrement bombé et vignettage doux.
  - **VHS** : une cassette vidéo familiale — image un peu plus douce, couleurs qui bavent vers la droite, teintes chaudes légèrement passées et noirs relevés, léger bruit, ondulation discrète des lignes et, de temps à autre, une pâle bande de tracking qui descend l'écran (le texte reste lisible).
  - **Arcade** : l'écran d'une borne d'arcade — trame basse résolution aux lignes de balayage sombres et marquées, grille RVB verticale, léger halo autour des zones claires, couleurs éclatantes et faible courbure.
- **Profondeur 3D** (`D`) : estime ce qui est proche et ce qui est lointain dans l'image. Survolez la tuile, ou ouvrez Réglages → Profondeur 3D, pour régler le **Format de profondeur** et l'**Intensité de l'effet**. *Effet de profondeur (amélioration 2D seule)* rend le premier plan plus net et plus éclatant, l'arrière-plan plus doux et vaporeux ; les formats stéréo pour écrans 3D sont décrits plus bas ; en mode immersif sur Quest, *Écran géant en 3D stéréo* fait passer l'écran géant en véritable 3D stéréoscopique. `D` ou un clic sur la tuile alterne entre *Sans profondeur* et le dernier format utilisé.
- **Luminosité, contraste, saturation** (Réglages → Image) : de −40 à +40, 0 correspondant à l'image d'origine. **Gamma** : de 0,70 à 1,40 (1,00 = neutre) ; au-dessus de 1,00 les tons moyens s'éclaircissent, en dessous ils s'assombrissent, tandis que le noir et le blanc restent inchangés. **Styles** prédéfinis : *Naturel*, *Réaliste* (contraste naturel en légère courbe en S, couleurs sobres, détails préservés dans les ombres), *Éclatant*, *Cinéma* et *Pièce lumineuse* (désormais avec un gamma un peu plus élevé, pour bien distinguer les scènes sombres dans une pièce éclairée).
- **Miroir** (`X`) : retourne toute l'image horizontalement.

## Formats 3D stéréo (Linux / Windows)

Sur ordinateur, le menu de format de profondeur (sur la tuile Profondeur 3D ou dans Réglages → Profondeur 3D) propose aussi quatre formats stéréo qui envoient une véritable 3D stéréoscopique vers un écran 3D. Réglez l'écran sur le même format 3D et passez en plein écran (`F`) ; captures et enregistrements restent en 2D. Le texte et les éléments du HUD sont détectés et maintenus près du plan de l'écran, et la profondeur est lissée : les lettres se déplacent d'un seul bloc pour chaque œil et ne paraissent plus coupées en deux.

| Format | Pour |
| --- | --- |
| Côte à côte, complet (3840×1080) | Lunettes XR en mode 3D, comme XREAL, VITURE ou RayNeo |
| Côte à côte, moitié | Téléviseurs 3D |
| Haut et bas | Téléviseurs 3D et projecteurs |
| Lunettes rouge / cyan | Lunettes anaglyphes rouge / cyan |

## Incrustation webcam, captures et enregistrement

Ce que vous voyez est ce qui est enregistré : effets, miroir et incrustation webcam sont intégrés à chaque capture et à chaque enregistrement.

- **Incrustation webcam** (`W`) : votre webcam par-dessus le jeu, dans l'un des quatre coins (en bas à gauche par défaut), en taille S, M ou L (18, 26 ou 36 % de la largeur de l'image).
  - Faites-la glisser où vous voulez : au relâchement, elle rejoint en douceur le coin le plus proche. La molette de la souris, au-dessus d'elle, change sa taille.
  - Au survol, une petite barre d'outils propose *Miroir*, *Taille* et *Retirer*.
  - En survolant la tuile du dock (ou dans Réglages → Incrustation webcam), réglez *Caméra*, *Qualité* (720p · 30 fps, 1080p · 30 fps ou 1080p · 60 fps), *Taille*, *Coin*, *Miroir* et **Inverser les images** : la webcam devient l'image principale et la carte de capture, l'incrustation.
- **Capture** (`S`) : l'image affichée au format PNG.
- **Enregistrer** (`R`) : MP4 avec vidéo H.264 et son AAC à 192 kbit/s. Le débit s'adapte au flux (largeur × hauteur × i/s × 0,12, entre 8 et 50 Mbit/s : environ 15 Mbit/s en 1080p60, 50 Mbit/s en 4K60). L'indicateur de connexion affiche REC et la durée écoulée.
  - **Windows :** aucun encodeur logiciel n'est fourni. L'enregistrement nécessite un encodeur graphique NVIDIA (NVENC), AMD (AMF) ou Intel, ou l'encodeur H.264 intégré à Windows (Media Foundation ; les éditions « N » de Windows doivent installer le Media Feature Pack).
  - **Linux :** aucun encodeur logiciel n'est fourni non plus. L'enregistrement nécessite une puce graphique Intel ou AMD via VA-API (avec le pilote VA-API installé), NVIDIA NVENC (pilote propriétaire) ou AMD AMF.
  - Sans aucun d'eux, tout le reste fonctionne, mais « Enregistrer » affiche « Aucun encodeur vidéo : l'enregistrement nécessite une puce graphique NVIDIA, AMD ou Intel ».
- **Enregistrer le micro** (`M`) : mélange votre voix au son du jeu.
- **Noms et dossiers** : `UVC Studio 2026-10-06 21.30.15.png` ou `.mp4` (date et heure ; un numéro est ajouté si le nom existe déjà). Les captures vont dans **Images/UVC Studio**, les enregistrements dans **Vidéos/UVC Studio**, avec la notification « Enregistré dans Pictures/UVC Studio » et un bouton **Afficher** ; Réglages → Général propose aussi « Ouvrir le dossier des captures » et « Ouvrir le dossier des enregistrements ». Sur Quest, les fichiers vont dans la galerie (`Pictures/UVC Studio` et `Movies/UVC Studio`), visibles dans l'application Fichiers et par USB.

## Meta Quest

Branchez la carte de capture sur le port USB-C du casque et jouez dans une fenêtre redimensionnable ou sur un écran virtuel géant, en véritable 3D stéréoscopique si vous le souhaitez.

- **Installation** : depuis le Meta Horizon Store (bientôt), ou installez `UVCStudio-Quest-v1.1.0-Free.apk` depuis les Releases de ce projet, casque en mode développeur (Meta Quest Developer Hub, SideQuest ou `adb install`). Platinum ne s'achète que dans la version installée depuis le store.
- **Branchement** : un adaptateur USB-C OTG ou un hub fonctionne aussi. Autorisez l'ouverture d'UVC Studio au branchement de la carte (de préférence avec « Toujours ») ainsi que l'accès à la caméra et au micro, sans lesquels Android ne donne pas accès aux cartes de capture USB. Les caméras du casque lui-même ne sont jamais utilisées.
- **Décodage** : les cartes H.264 et HEVC sont décodées par le décodeur matériel intégré au casque ; le FFmpeg fourni ne sert qu'au MJPEG.
- **Fenêtre 2D** (par défaut) : toute l'interface de la version ordinateur dans un panneau Horizon OS déplaçable et redimensionnable. Pincez deux fois sur l'image pour entrer en plein écran ou en sortir.
- **Mode immersif** : la tuile casque, en fin de dock après Réglages, bascule vers l'écran virtuel géant et inversement. UVC Studio se déconnecte, change de mode et se reconnecte tout seul de l'autre côté.
- **Profondeur 3D sur Quest** : dans la fenêtre 2D, le choix se fait entre *Sans profondeur* et *Effet de profondeur* ; en mode immersif, entre *Sans profondeur* et *Écran géant en 3D stéréo*.

| En mode immersif | Comment |
| --- | --- |
| Cliquer dans le panneau de commande | Visez avec la manette et appuyez sur la gâchette, ou pincez avec la main |
| Déplacer le panneau de commande | Saisissez la barre blanche située dessous et faites glisser ; poussez ou tirez pour changer la distance. Réglages → VR → « Réinitialiser la position du panneau » le remet en place |
| Déplacer l'écran | Saisissez la barre blanche située dessous et faites glisser ; poussez ou tirez la main pour changer la distance |
| Redimensionner l'écran | Visez un coin jusqu'à voir apparaître un crochet, puis pincez et faites glisser |
| Rapprocher / éloigner, réduire / agrandir | Joystick haut / bas, gauche / droite |
| Masquer ou afficher le panneau | Pincez deux fois sur l'écran, ou appuyez sur B, Y ou Menu |
| Recentrer | Appui long sur le bouton Meta |
| Préréglages d'écran | Réglages → VR : arrière-plan (passthrough ou salle obscure), forme de l'écran (plat ou incurvé), taille et distance de l'écran, fréquence de rafraîchissement 72 / 90 / 120 Hz (90 par défaut) |

- **Menu système** : tant qu'il est ouvert, l'application continue d'afficher l'image, masque ses pointeurs et coupe le son.
- **Fonctionnement plus frais** : l'upscaling s'arrête à ce que l'écran géant peut réellement afficher (environ 30 pixels par degré, 2560 de large au plus), et la paire stéréo est rendue à cette même taille.
- **Protection contre la surchauffe** : surveille l'état thermique d'Android et les notifications thermiques d'OpenXR. Quand le casque chauffe, l'application passe en FSR et en 72 Hz ; quand il devient trop chaud, elle met en pause l'upscaling et la profondeur 3D. Vos réglages restent inchangés et une notification vous prévient une fois le casque refroidi.
- **Indicateur de charge** : pendant la connexion, Réglages → À propos affiche le temps GPU par image, la résolution de traitement et la température du casque.

## Interface

Toute l'interface adopte le style Liquid Glass : des surfaces translucides et dépolies qui, une fois connecté, laissent deviner une copie floutée et ravivée du jeu.

- **Barre du haut** : à gauche le logo UVC Studio ; au centre l'**indicateur de connexion** (« Non connecté » ; un point vert avec périphérique · résolution · fps, complété par ▸ 4K et l'upscaler ainsi que · 3D ; ou un REC rouge pulsant avec la durée d'enregistrement) ; à droite le menu des langues et **Connecter / Déconnecter**. Les erreurs s'affichent en rouge sous l'indicateur pendant quelques secondes.
- **Dock** : quatre groupes — *Capture, Enregistrer, Enregistrer le micro* · *Upscaling 4K, Rendu rétro, Profondeur 3D, Miroir* · *Incrustation webcam, Écoute* · *Réglages*. Les tuiles grossissent sous le pointeur, les fonctions actives s'illuminent du dégradé de la marque et chaque info-bulle indique la touche de raccourci. La version figure en bas à gauche, le bouton Plein écran en bas à droite.
- **Réglages** : un grand panneau de verre aux cartes réparties sur trois colonnes équilibrées au plus, pour tout régler sans faire défiler : Source, Son, Image, Incrustation webcam, Affichage, VR (mode immersif du Quest), Général, Profondeur 3D et À propos — dans la disposition automatique, la carte « Profondeur 3D » se trouve juste au-dessus de « À propos ». La carte « À propos » affiche la version, le temps GPU par image et la résolution de traitement pendant la connexion (ainsi que la température du casque sur Quest) et le bouton « Licences open source » ; la note de confidentialité figure dans la fenêtre qu'il ouvre.
  - **Réduire une carte** : le bouton rond à chevron tout à droite de l'en-tête, à droite du cadenas, réduit la carte à son en-tête (le titre, le cadenas et le bouton restent) et la rouvre. Les autres cartes remontent, aucune ne change de colonne, et les cartes réduites sont mémorisées.
  - **Organisez les cartes vous-même** : chaque carte porte un cadenas en haut à droite de son en-tête, et toutes les cartes sont verrouillées au départ. Cliquez sur le cadenas (« Déverrouiller pour déplacer »), puis faites glisser la carte par son en-tête vers un autre endroit des trois colonnes ; un emplacement vide indique où elle va se placer, et `Échap` annule le déplacement. Un nouveau clic sur le cadenas (« Verrouiller la position ») fixe la carte. Les cartes verrouillées restent en place, les cartes déverrouillées se déplacent entre elles. La disposition et les cartes déverrouillées sont mémorisées pour le prochain lancement. « Réinitialiser la disposition », à côté du bouton de fermeture et affiché uniquement lorsque votre propre disposition est utilisée, rétablit la disposition automatique et verrouille toutes les cartes. Si la fenêtre est trop étroite pour trois colonnes, la disposition automatique s'affiche et la vôtre est conservée pour le moment où la fenêtre sera de nouveau assez large. Dans la fenêtre 2D du Quest, tout fonctionne de la même façon avec le pointeur de la manette.
- **Langues** : 台灣正體中文, English, Deutsch, Français, Italiano, Español, Português, 日本語, 한국어, 简体中文, ไทย et Tiếng Việt, au choix dans le menu de verre en haut à droite. Au premier lancement, l'application suit la langue du système.
- **Thèmes** : Auto (clair de 06:00 à 18:00), Sombre ou Clair.
- **Taille de l'interface** : suit l'écran (1080p 100 %, 1440p environ 135 %, 4K 200 %) ; modifiable dans Réglages → Affichage ou avec `Ctrl` `+` / `−` / `0`.
- **Polices et icônes** : Manrope et IBM Plex Sans (IBM Plex Sans Thai pour le thaï, Noto Sans CJK TC pour le chinois, le japonais et le coréen) ; les icônes ont été dessinées pour UVC Studio.
- **Plein écran** : `F`, `F11`, le bouton en bas à droite ou un double-clic sur l'image ; `Échap` pour en sortir.

## Raccourcis clavier (Linux / Windows)

Appuyez sur `?` ou `H` dans l'application pour afficher la même liste ; une touche ou un clic la referme.

| Touche | Action |
| --- | --- |
| `Entrée` | Connecter |
| `Ctrl+D` | Déconnecter |
| `S` | Capture |
| `R` | Démarrer ou arrêter l'enregistrement |
| `M` | Activer / désactiver l'enregistrement du micro |
| `L` | Activer / désactiver l'écoute |
| `W` | Activer / désactiver l'incrustation webcam |
| `X` | Miroir |
| `U` | Activer / désactiver l'upscaling 4K |
| `Maj+U` | Upscaler suivant (active l'upscaling) |
| `C` | Rendu rétro : Désactivé → CRT → VHS → Arcade |
| `D` | Profondeur 3D : alterner entre *Sans profondeur* et le dernier format |
| `O` ou `Ctrl+,` | Réglages |
| `F` ou `F11` | Plein écran |
| `Échap` | Annuler le déplacement d'une carte, quitter le plein écran ou fermer les réglages |
| `?` ou `H` | Raccourcis clavier |
| `Ctrl` `+` / `−` / `0` | Agrandir / réduire / réinitialiser l'interface |

## Confidentialité

UVC Studio fonctionne entièrement sur votre appareil. L'image, le son et les enregistrements ne le quittent jamais — aucun compte, aucun envoi, aucun suivi, et l'application n'accède jamais au réseau. Sur Meta Quest, l'achat de Platinum et sa vérification passent par le service Meta Horizon Store du casque ; UVC Studio lui-même n'envoie rien.

## Licences

UVC Studio pour Linux et Windows est open source sous [licence MIT](LICENSE) ; la version Meta Quest n'est pas open source. UVC Studio ne contient aucun composant sous GPL ; FFmpeg est compilé en LGPL uniquement (sur Quest, avec le seul décodeur MJPEG et sous forme de bibliothèques partagées remplaçables). Autres composants : SDL2, Dear ImGui, miniaudio, stb, NanoSVG, libusb et libuvc (Quest), le chargeur OpenXR, AMD FidelityFX FSR 1 et CAS, Anime4K et NVIDIA Image Scaling. Les polices (Manrope, IBM Plex Sans, IBM Plex Sans Thai, Noto Sans CJK TC) sont sous SIL Open Font License. Tous les composants et le texte complet de leurs licences figurent dans l'application, sous Réglages → À propos → Licences open source. La version Meta Quest inclut aussi le chargeur du Meta Platform SDK (Oculus SDK License) pour l'achat dans le store.
