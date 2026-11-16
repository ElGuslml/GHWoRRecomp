// This game's own texts (game.h): the first run's screens and the Instruments and gamepad page in the game's spirit
// (GH Van Halen's, from its mockup), the Game page's switches, where the game shows its DLC, and the guitar's and the
// drums' functions on the Keyboard and Gamepad pages, in the launcher's languages (English, German, French, Spanish,
// Italian).

#include "game.h"

#include "settings.h"

namespace launcher::game {

const GameTexts& GameTextsFor(uint32_t language) {
  static const GameTexts kEnglish{
      .welcome_title = "Welcome backstage",
      .welcome_text = "Before you hit the stage, the game has to be installed. The launcher extracts its files from "
                      "your Xbox 360 ISO; you only have to do this once.",
      .toggles = {
          "Bloom", "The glow around lights and bright surfaces.",
          "Depth of field", "The background blurred behind what is in focus.",
          "Color correction", "Each venue's look; off turns the grain off too.",
          "Film grain", "The grain over the image. Needs the color correction.",
          "Vignette", "Darker corners, where a venue turns it on.",
          "Venue at the full frame rate", "The venue and the band drawn in every frame, not every other one.",
          "Screen effects before the upscaler", "With DLSS, DLAA or FSR 3.1: faster, the glow and the blur a little softer.",
          "Even frame times with DLSS", "DLSS's work moved to the next frame; the venue shows a pair of frames later.",
          "Skip the intro videos", "The logos end once loaded; no intro movie.",
          "Game debug menu", "Neversoft's debug menu in the main menu and the pause menu.",
          "World Tour's Star Power sound", "Star Power with World Tour's echo and reverb.",
      },
      .toggle_help = {
          "The halo of light around the stage's lights, the pyrotechnics and the brightest surfaces. Off, the image is sharper and flatter.",
          "Blurs the background in some camera shots and behind the menus and the pause menu, as a camera's lens does. Off, everything stays sharp.",
          "The brightness, contrast and saturation each venue sets, the fades to black and the grey image while the rock meter is red. Off, the colors are the scene's own and the film grain goes off with it: without this pass the grain is far stronger.",
          "A fine moving grain over the image, as on film. It only shows with the color correction on.",
          "Darkens the corners of the image in the venues and moments that use it.",
          "The game draws its 3D scene over two frames: the venue and the crowd in one, the band and the screen effects in the next. So the stage moves at half the frame rate (30 times a second on the console) while the note highway and the HUD use the full rate. On, the whole scene is drawn in every frame: the stage as smooth as the highway, for about twice the work a frame for the processor and the graphics card: at 160 FPS even a fast PC can drop to about 130 in busy songs, while 120 FPS or lower leaves it room. If the frame rate drops, lower it or turn this off.",
          "With DLSS, DLAA or FSR 3.1, the glow around lights and the blurred backgrounds are drawn before the upscaler, at its input resolution, instead of after it at the screen's. They cost much less on a modest graphics card (an RTX A2000 at 4K with DLSS Performance went from 113 to 130 frames per second) and look a little softer, as the upscaler enlarges them with the rest of the image. It changes nothing with SMAA or FXAA.",
          "The game draws the venue over two frames, and DLSS falls in the second, so a long frame follows a short one. With this on, DLSS runs at the start of the next frame: both frames take about the same time and the motion looks steadier. The venue, the crowd and the band show a pair of frames later (17 ms at 120 FPS); the note highway and the HUD are not delayed. Only with DLSS upscaling (not with DLAA), and not with frame generation or with \"Venue at the full frame rate\" on.",
          "At each start the game shows its logos while it loads, then its intro movie. With this on, the logos end as soon as the game has loaded and the intro movie is not played: the autosave notice and the main menu come right away.",
          "Adds a \"(debug menu)\" item to the main menu and a Debug Menu item to the pause menu: the menu the game's developers used (Unlock All, Skip Into Song, Change Venue, Show Song Time, Play Credits, Add 100 Quickplay Stars, ...). Its texts are in English, some items do nothing without the developers' tools, and Unlock All and Add 100 Quickplay Stars change your save as playing does.",
          "In Guitar Hero World Tour, Star Power sent your instrument through an echo timed to the song, a reverb and a high pass, set for each instrument and for one player or several, until Star Power ended. Warriors of Rock kept those settings but only raises the venue's reverb. With this on, Star Power sounds as in World Tour; the band, the crowd and the vocals do not change.",
      },
      .done_title = "The stage is yours!",
      .done_text = "The game was installed. Go play now, or first review the video, graphics and instrument settings; "
                   "you can always change them in the launcher.",
      .dlc_where = "The game lists the songs in Quickplay+'s song list.",
      .extracting_title = "Tuning up the stage",
      .extracting_text = "The game is being extracted. It can take a few minutes; do not close the launcher.",
      .keyboard_tap_title = "Strum when a fret is pressed",
      .keyboard_tap_hint = "Each fret key strums as you press it; frets pressed within 20 ms of each other are one chord, one strum.",
      .part_instrument = "Guitar",
      .part_vocals = "Vocals",
  };
  static const GameTexts kGerman{
      .welcome_title = "Willkommen im Backstage",
      .welcome_text = "Bevor es auf die Bühne geht, muss das Spiel installiert werden. Der Launcher entpackt die Dateien "
                      "aus deinem Xbox-360-ISO; das musst du nur einmal tun.",
      .toggles = {
          "Bloom", "Das Leuchten um Lichter und helle Flächen.",
          "Tiefenschärfe", "Der unscharfe Hintergrund hinter dem Fokus.",
          "Farbkorrektur", "Der Look jedes Ortes; aus schaltet auch die Körnung aus.",
          "Filmkörnung", "Die Körnung über dem Bild. Braucht die Farbkorrektur.",
          "Vignette", "Dunklere Ecken, wo ein Ort sie einschaltet.",
          "Bühne mit voller Bildrate", "Bühne und Band in jedem Bild statt in jedem zweiten.",
          "Bildeffekte vor der Hochskalierung", "Mit DLSS, DLAA oder FSR 3.1: schneller, Leuchten und Unschärfe etwas weicher.",
          "Gleichmäßige Bildzeiten mit DLSS", "Die Arbeit von DLSS rückt ins nächste Bild; die Bühne erscheint ein Bildpaar später.",
          "Intro-Videos überspringen", "Die Logos enden nach dem Laden; kein Intro-Film.",
          "Debug-Menü des Spiels", "Das Debug-Menü von Neversoft im Hauptmenü und im Pausenmenü.",
          "Star-Power-Klang aus World Tour", "Star Power mit dem Echo und dem Hall aus World Tour.",
      },
      .toggle_help = {
          "Der Lichtschein um die Bühnenlichter, die Pyrotechnik und die hellsten Flächen. Aus wirkt das Bild schärfer und flacher.",
          "Macht den Hintergrund in manchen Kameraeinstellungen sowie hinter den Menüs und dem Pausenmenü unscharf, wie ein Kameraobjektiv. Aus bleibt alles scharf.",
          "Helligkeit, Kontrast und Sättigung jedes Auftrittsortes, die Abblenden ins Schwarze und das graue Bild, solange die Rock-Anzeige rot ist. Aus zeigt die eigenen Farben der Szene, und die Filmkörnung geht mit aus: ohne diesen Durchgang ist sie viel stärker.",
          "Eine feine, bewegte Körnung über dem Bild, wie bei Film. Nur mit eingeschalteter Farbkorrektur sichtbar.",
          "Dunkelt die Bildecken an den Orten und in den Momenten ab, die sie verwenden.",
          "Das Spiel zeichnet seine 3D-Szene über zwei Bilder: den Auftrittsort und das Publikum in einem, die Band und die Bildeffekte im nächsten. So bewegt sich die Bühne mit der halben Bildrate (auf der Konsole 30-mal pro Sekunde), während Notenbahn und HUD die volle nutzen. Eingeschaltet wird die ganze Szene in jedem Bild gezeichnet: die Bühne so flüssig wie die Notenbahn, für etwa die doppelte Arbeit pro Bild für Prozessor und Grafikkarte: Bei 160 FPS kann selbst ein schneller PC in vollen Songs auf etwa 130 fallen, bei 120 FPS oder weniger bleibt Luft. Sinkt die Bildrate, senke sie oder schalte es aus.",
          "Mit DLSS, DLAA oder FSR 3.1 werden der Lichtschein um die Lichter und die unscharfen Hintergründe vor der Hochskalierung gezeichnet, in deren Eingangsauflösung statt danach in der des Bildschirms. Auf einer einfacheren Grafikkarte kosten sie so viel weniger (eine RTX A2000 in 4K mit DLSS auf Leistung kam von 113 auf 130 Bilder pro Sekunde) und wirken etwas weicher, da die Hochskalierung sie mit dem Rest des Bildes vergrößert. Mit SMAA oder FXAA ändert sich nichts.",
          "Das Spiel zeichnet die Bühne über zwei Bilder, und DLSS fällt ins zweite, sodass auf ein kurzes Bild ein langes folgt. Eingeschaltet läuft DLSS zu Beginn des nächsten Bildes: Beide Bilder dauern etwa gleich lang, und die Bewegung wirkt gleichmäßiger. Auftrittsort, Publikum und Band erscheinen ein Bildpaar später (17 ms bei 120 FPS); Notenbahn und HUD werden nicht verzögert. Nur mit der Hochskalierung von DLSS (nicht mit DLAA), und nicht mit Bildgenerierung oder mit eingeschalteter \"Bühne mit voller Bildrate\".",
          "Das Spiel zeigt bei jedem Start seine Logos, während es lädt, und danach seinen Intro-Film. Eingeschaltet enden die Logos, sobald das Spiel geladen ist, und der Intro-Film wird nicht gezeigt: der Hinweis zum automatischen Speichern und das Hauptmenü kommen sofort.",
          "Fügt dem Hauptmenü den Eintrag \"(debug menu)\" und dem Pausenmenü den Eintrag Debug Menu hinzu: das Menü der Entwickler des Spiels (Unlock All, Skip Into Song, Change Venue, Show Song Time, Play Credits, Add 100 Quickplay Stars, ...). Seine Texte sind englisch, manche Einträge tun ohne die Werkzeuge der Entwickler nichts, und Unlock All und Add 100 Quickplay Stars ändern deinen Spielstand wie das Spielen.",
          "In Guitar Hero World Tour schickte Star Power dein Instrument durch ein Echo im Takt des Songs, einen Hall und einen Hochpass, eingestellt für jedes Instrument und für einen oder mehrere Spieler, bis Star Power endete. Warriors of Rock behielt diese Einstellungen, verstärkt aber nur den Hall des Auftrittsortes. Eingeschaltet klingt Star Power wie in World Tour; die Band, das Publikum und der Gesang ändern sich nicht.",
      },
      .done_title = "Die Bühne gehört dir!",
      .done_text = "Das Spiel wurde installiert. Leg gleich los oder sieh dir zuerst die Video-, Grafik- und "
                   "Instrumenteinstellungen an; du kannst sie jederzeit im Launcher ändern.",
      .dlc_where = "Das Spiel zeigt die Songs in der Songliste von Schnellspiel+.",
      .extracting_title = "Die Bühne wird gestimmt",
      .extracting_text = "Das Spiel wird entpackt. Das kann ein paar Minuten dauern; schließe den Launcher nicht.",
      .keyboard_tap_title = "Anschlag beim Drücken eines Bunds",
      .keyboard_tap_hint = "Jede Bundtaste schlägt an, wenn du sie drückst; Bünde, die du innerhalb von 20 ms drückst, sind ein Akkord, ein Anschlag.",
      .part_instrument = "Gitarre",
      .part_vocals = "Gesang",
  };
  static const GameTexts kFrench{
      .welcome_title = "Bienvenue en coulisses",
      .welcome_text = "Avant de monter sur scène, il faut installer le jeu. Le launcher extrait ses fichiers de votre ISO "
                      "Xbox 360 ; vous n'aurez à le faire qu'une fois.",
      .toggles = {
          "Bloom", "La lueur autour des lumières et des surfaces claires.",
          "Profondeur de champ", "L'arrière-plan flou derrière le sujet net.",
          "Correction des couleurs", "L'ambiance de chaque salle ; désactivée, le grain l'est aussi.",
          "Grain", "Le grain sur l'image. Nécessite la correction des couleurs.",
          "Vignettage", "Des coins plus sombres, là où une salle l'active.",
          "Scène à pleine fréquence", "La salle et le groupe dessinés à chaque image, pas une sur deux.",
          "Effets d'image avant la mise à l'échelle", "Avec DLSS, DLAA ou FSR 3.1 : plus rapide, la lueur et le flou un peu plus doux.",
          "Temps d'image réguliers avec DLSS", "Le travail de DLSS passe à l'image suivante ; la salle s'affiche une paire d'images plus tard.",
          "Passer les vidéos d'intro", "Les logos s'arrêtent une fois chargé ; pas de film d'intro.",
          "Menu de débogage du jeu", "Le menu de débogage de Neversoft dans le menu principal et le menu pause.",
          "Son du Star Power de World Tour", "Le Star Power avec l'écho et la réverbération de World Tour.",
      },
      .toggle_help = {
          "Le halo autour des lumières de la scène, de la pyrotechnie et des surfaces les plus claires. Désactivé, l'image est plus nette et plus plate.",
          "Floute l'arrière-plan dans certains plans de caméra et derrière les menus et le menu pause, comme un objectif. Désactivée, tout reste net.",
          "La luminosité, le contraste et la saturation de chaque salle, les fondus au noir et l'image grise tant que la jauge rock est dans le rouge. Désactivée, les couleurs sont celles de la scène et le grain s'éteint avec elle : sans cette passe, il est bien plus fort.",
          "Un grain fin et animé sur l'image, comme sur pellicule. Il n'apparaît qu'avec la correction des couleurs activée.",
          "Assombrit les coins de l'image dans les salles et les moments qui l'utilisent.",
          "Le jeu dessine sa scène 3D sur deux images : la salle et le public dans l'une, le groupe et les effets d'image dans la suivante. La scène bouge donc à la moitié de la fréquence d'images (30 fois par seconde sur la console), tandis que la piste de notes et l'interface utilisent la fréquence complète. Activée, toute la scène est dessinée à chaque image : la scène aussi fluide que la piste, pour environ deux fois plus de travail par image pour le processeur et la carte graphique : à 160 FPS, même un PC rapide peut descendre vers 130 dans les chansons chargées, alors qu'à 120 FPS ou moins il garde de la marge. Si la fréquence d'images baisse, réduisez-la ou désactivez cette option.",
          "Avec DLSS, DLAA ou FSR 3.1, la lueur autour des lumières et les arrière-plans flous sont dessinés avant la mise à l'échelle, à sa résolution d'entrée, au lieu de l'être après, à celle de l'écran. Ils coûtent bien moins sur une carte graphique modeste (une RTX A2000 en 4K avec DLSS en Performance est passée de 113 à 130 images par seconde) et paraissent un peu plus doux, car la mise à l'échelle les agrandit avec le reste de l'image. Cela ne change rien avec SMAA ou FXAA.",
          "Le jeu dessine la salle sur deux images, et DLSS tombe dans la seconde : une image longue suit donc une image courte. Activé, DLSS s'exécute au début de l'image suivante : les deux images durent à peu près autant et le mouvement paraît plus régulier. La salle, le public et le groupe s'affichent une paire d'images plus tard (17 ms à 120 FPS) ; la piste de notes et l'interface ne sont pas retardées. Seulement avec la mise à l'échelle de DLSS (pas avec DLAA), et pas avec la génération d'images ni avec « Scène à pleine fréquence » activée.",
          "À chaque démarrage, le jeu montre ses logos pendant qu'il charge, puis son film d'intro. Activée, les logos s'arrêtent dès que le jeu est chargé et le film d'intro n'est pas joué : l'avis de sauvegarde automatique et le menu principal arrivent aussitôt.",
          "Ajoute l'entrée « (debug menu) » au menu principal et l'entrée Debug Menu au menu pause : le menu des développeurs du jeu (Unlock All, Skip Into Song, Change Venue, Show Song Time, Play Credits, Add 100 Quickplay Stars, ...). Ses textes sont en anglais, certaines entrées ne font rien sans les outils des développeurs, et Unlock All et Add 100 Quickplay Stars modifient votre sauvegarde comme le jeu.",
          "Dans Guitar Hero World Tour, le Star Power faisait passer votre instrument par un écho calé sur le tempo de la chanson, une réverbération et un passe-haut, réglés pour chaque instrument et pour un ou plusieurs joueurs, jusqu'à la fin du Star Power. Warriors of Rock a gardé ces réglages mais ne fait que monter la réverbération de la salle. Activé, le Star Power sonne comme dans World Tour ; le groupe, le public et le chant ne changent pas.",
      },
      .done_title = "La scène est à vous !",
      .done_text = "Le jeu a été installé. Allez jouer tout de suite, ou vérifiez d'abord les réglages vidéo, "
                   "graphiques et de vos instruments ; vous pourrez toujours les changer dans le launcher.",
      .dlc_where = "Le jeu affiche les chansons dans la liste de Partie rapide+.",
      .extracting_title = "On accorde la scène",
      .extracting_text = "Le jeu est en cours d'extraction. Cela peut prendre quelques minutes ; ne fermez pas le "
                         "launcher.",
      .keyboard_tap_title = "Gratter en appuyant sur une frette",
      .keyboard_tap_hint = "Chaque touche de frette gratte quand vous l'appuyez ; les frettes appuyées à moins de 20 ms d'intervalle forment un accord, un seul grattage.",
      .part_instrument = "Guitare",
      .part_vocals = "Chant",
  };
  static const GameTexts kSpanish{
      .welcome_title = "Bienvenido al backstage",
      .welcome_text = "Antes de subir al escenario hay que instalar el juego. El launcher extrae los archivos desde tu "
                      "ISO de Xbox 360; solo tendrás que hacerlo una vez.",
      .toggles = {
          "Bloom", "El resplandor alrededor de las luces y superficies claras.",
          "Profundidad de campo", "El fondo desenfocado detrás de lo enfocado.",
          "Corrección de color", "El aspecto de cada escenario; al desactivarla se apaga el grano.",
          "Grano de película", "El grano sobre la imagen. Necesita la corrección de color.",
          "Viñeta", "Esquinas más oscuras, donde un escenario la activa.",
          "Escenario a cuadros completos", "El escenario y la banda en cada cuadro, no en uno de cada dos.",
          "Efectos de pantalla antes del escalado", "Con DLSS, DLAA o FSR 3.1: más rápido, el brillo y el desenfoque un poco más suaves.",
          "Tiempos de cuadro parejos con DLSS", "El trabajo de DLSS pasa al cuadro siguiente; el escenario se muestra un par de cuadros después.",
          "Saltar los videos de introducción", "Los logos terminan al cargar; sin película de intro.",
          "Menú de depuración del juego", "El menú de depuración de Neversoft en el menú principal y en el de pausa.",
          "Sonido del Star Power de World Tour", "El Star Power con el eco y la reverberación de World Tour.",
      },
      .toggle_help = {
          "El halo de luz alrededor de las luces del escenario, la pirotecnia y las superficies más brillantes. Desactivado, la imagen se ve más nítida y plana.",
          "Desenfoca el fondo en algunas tomas de cámara y detrás de los menús y la pausa, como el lente de una cámara. Desactivada, todo se ve nítido.",
          "El brillo, el contraste y la saturación de cada escenario, los fundidos a negro y la imagen gris mientras el medidor de rock está en rojo. Desactivada, se ven los colores propios de la escena y el grano de película se apaga con ella: sin esta pasada el grano es mucho más fuerte.",
          "Un grano fino y en movimiento sobre la imagen, como en una película. Solo se ve con la corrección de color activada.",
          "Oscurece las esquinas de la imagen en los escenarios y momentos que la usan.",
          "El juego dibuja su escena 3D en dos cuadros: el escenario y el público en uno, la banda y los efectos de imagen en el siguiente. Así el escenario se mueve a la mitad de los cuadros por segundo (30 veces por segundo en la consola), mientras el highway de notas y el HUD van a la tasa completa. Activado, toda la escena se dibuja en cada cuadro: el escenario tan fluido como el highway, a cambio de cerca del doble de trabajo por cuadro para el procesador y la tarjeta gráfica: a 160 FPS incluso un PC rápido puede bajar a unos 130 en canciones cargadas, mientras que a 120 FPS o menos le queda margen. Si bajan los cuadros por segundo, bájalos o desactívalo.",
          "Con DLSS, DLAA o FSR 3.1, el resplandor alrededor de las luces y los fondos desenfocados se dibujan antes del escalado, a su resolución de entrada, en vez de después a la de la pantalla. Cuestan mucho menos en una tarjeta gráfica modesta (una RTX A2000 en 4K con DLSS en Rendimiento pasó de 113 a 130 cuadros por segundo) y se ven un poco más suaves, porque el escalado los agranda junto con el resto de la imagen. Con SMAA o FXAA no cambia nada.",
          "El juego dibuja el escenario en dos cuadros y DLSS cae en el segundo, así que a un cuadro corto le sigue uno largo. Activado, DLSS se ejecuta al inicio del cuadro siguiente: los dos cuadros tardan más o menos lo mismo y el movimiento se ve más parejo. El escenario, el público y la banda se muestran un par de cuadros después (17 ms a 120 FPS); el highway de notas y el HUD no se retrasan. Solo con el escalado de DLSS (no con DLAA), y no con generación de cuadros ni con «Escenario a cuadros completos» activado.",
          "En cada inicio el juego muestra sus logos mientras carga y después su película de introducción. Activado, los logos terminan en cuanto el juego cargó y la película de introducción no se reproduce: el aviso de guardado automático y el menú principal llegan enseguida.",
          "Añade la opción «(debug menu)» al menú principal y Debug Menu al de pausa: el menú que usaban los desarrolladores del juego (Unlock All, Skip Into Song, Change Venue, Show Song Time, Play Credits, Add 100 Quickplay Stars, ...). Sus textos están en inglés, algunas opciones no hacen nada sin las herramientas de los desarrolladores, y Unlock All y Add 100 Quickplay Stars cambian tu partida guardada como jugar.",
          "En Guitar Hero World Tour, el Star Power pasaba tu instrumento por un eco al tempo de la canción, una reverberación y un filtro pasa altos, ajustados para cada instrumento y para uno o varios jugadores, hasta que terminaba el Star Power. Warriors of Rock conservó esos ajustes pero solo sube la reverberación del escenario. Activado, el Star Power suena como en World Tour; la banda, el público y la voz no cambian.",
      },
      .done_title = "¡El escenario es tuyo!",
      .done_text = "El juego se instaló correctamente. Puedes salir a tocar ya o revisar primero los ajustes de video, "
                   "gráficos y tus instrumentos; siempre podrás cambiarlos desde el launcher.",
      .dlc_where = "El juego muestra las canciones en la lista de canciones de Partida rápida+.",
      .extracting_title = "Afinando el escenario",
      .extracting_text = "Estamos extrayendo el juego. Puede tardar unos minutos; no cierres el launcher.",
      .keyboard_tap_title = "Activar rasgueo al presionar botón",
      .keyboard_tap_hint = "Cada tecla de traste rasguea al presionarla; los trastes que presiones con menos de 20 ms de diferencia cuentan como un acorde: un solo rasgueo.",
      .part_instrument = "Guitarra",
      .part_vocals = "Voz",
  };
  static const GameTexts kItalian{
      .welcome_title = "Benvenuto nel backstage",
      .welcome_text = "Prima di salire sul palco bisogna installare il gioco. Il launcher estrae i file dalla tua ISO "
                      "Xbox 360; dovrai farlo una sola volta.",
      .toggles = {
          "Bloom", "Il bagliore attorno alle luci e alle superfici chiare.",
          "Profondità di campo", "Lo sfondo sfocato dietro ciò che è a fuoco.",
          "Correzione del colore", "L'aspetto di ogni locale; disattivata spegne anche la grana.",
          "Grana", "La grana sull'immagine. Richiede la correzione del colore.",
          "Vignettatura", "Angoli più scuri, dove un locale la attiva.",
          "Palco a frequenza piena", "Il locale e la band disegnati in ogni fotogramma, non uno sì e uno no.",
          "Effetti dello schermo prima dell'upscaling", "Con DLSS, DLAA o FSR 3.1: più veloce, il bagliore e la sfocatura un po' più morbidi.",
          "Tempi dei fotogrammi regolari con DLSS", "Il lavoro di DLSS passa al fotogramma successivo; il locale appare una coppia di fotogrammi dopo.",
          "Salta i video introduttivi", "I loghi finiscono a caricamento finito; niente filmato.",
          "Menu di debug del gioco", "Il menu di debug di Neversoft nel menu principale e nel menu di pausa.",
          "Suono dello Star Power di World Tour", "Lo Star Power con l'eco e il riverbero di World Tour.",
      },
      .toggle_help = {
          "L'alone di luce attorno alle luci del palco, alla pirotecnica e alle superfici più luminose. Disattivato, l'immagine è più nitida e piatta.",
          "Sfoca lo sfondo in alcune inquadrature e dietro i menu e il menu di pausa, come l'obiettivo di una telecamera. Disattivata, tutto resta nitido.",
          "La luminosità, il contrasto e la saturazione di ogni locale, le dissolvenze al nero e l'immagine grigia mentre l'indicatore rock è nel rosso. Disattivata, si vedono i colori propri della scena e la grana si spegne con essa: senza questo passaggio è molto più forte.",
          "Una grana fine e in movimento sull'immagine, come su pellicola. Si vede solo con la correzione del colore attiva.",
          "Scurisce gli angoli dell'immagine nei locali e nei momenti che la usano.",
          "Il gioco disegna la sua scena 3D su due fotogrammi: il locale e il pubblico in uno, la band e gli effetti d'immagine nel successivo. Così il palco si muove alla metà della frequenza dei fotogrammi (30 volte al secondo sulla console), mentre la pista delle note e l'HUD usano la frequenza piena. Attivato, tutta la scena viene disegnata in ogni fotogramma: il palco fluido come la pista, per circa il doppio del lavoro per fotogramma per il processore e la scheda grafica: a 160 FPS anche un PC veloce può scendere a circa 130 nelle canzoni più cariche, mentre a 120 FPS o meno ha margine. Se la frequenza cala, abbassala o disattivalo.",
          "Con DLSS, DLAA o FSR 3.1 il bagliore attorno alle luci e gli sfondi sfocati vengono disegnati prima dell'upscaling, alla sua risoluzione d'ingresso, invece che dopo, a quella dello schermo. Costano molto meno su una scheda grafica modesta (una RTX A2000 in 4K con DLSS in Prestazioni è passata da 113 a 130 fotogrammi al secondo) e appaiono un po' più morbidi, perché l'upscaling li ingrandisce insieme al resto dell'immagine. Con SMAA o FXAA non cambia nulla.",
          "Il gioco disegna il locale su due fotogrammi e DLSS cade nel secondo, così a un fotogramma breve ne segue uno lungo. Attivato, DLSS viene eseguito all'inizio del fotogramma successivo: i due fotogrammi durano più o meno lo stesso e il movimento appare più regolare. Il locale, il pubblico e la band appaiono una coppia di fotogrammi dopo (17 ms a 120 FPS); la pista delle note e l'HUD non vengono ritardati. Solo con l'upscaling di DLSS (non con DLAA), e non con la generazione di fotogrammi né con «Palco a frequenza piena» attivato.",
          "A ogni avvio il gioco mostra i suoi loghi mentre carica e poi il suo filmato introduttivo. Attivato, i loghi finiscono appena il gioco è caricato e il filmato introduttivo non viene riprodotto: l'avviso di salvataggio automatico e il menu principale arrivano subito.",
          "Aggiunge la voce «(debug menu)» al menu principale e Debug Menu al menu di pausa: il menu degli sviluppatori del gioco (Unlock All, Skip Into Song, Change Venue, Show Song Time, Play Credits, Add 100 Quickplay Stars, ...). I suoi testi sono in inglese, alcune voci non fanno nulla senza gli strumenti degli sviluppatori, e Unlock All e Add 100 Quickplay Stars cambiano il tuo salvataggio come giocare.",
          "In Guitar Hero World Tour lo Star Power faceva passare il tuo strumento in un'eco a tempo con il brano, un riverbero e un filtro passa alto, regolati per ogni strumento e per uno o più giocatori, fino alla fine dello Star Power. Warriors of Rock ha conservato quelle regolazioni ma alza solo il riverbero del locale. Attivato, lo Star Power suona come in World Tour; la band, il pubblico e la voce non cambiano.",
      },
      .done_title = "Il palco è tuo!",
      .done_text = "Il gioco è stato installato. Vai a suonare subito o controlla prima le impostazioni video, grafiche e "
                   "dei tuoi strumenti; potrai sempre cambiarle nel launcher.",
      .dlc_where = "Il gioco mostra i brani nella scaletta di Partita veloce+.",
      .extracting_title = "Accordiamo il palco",
      .extracting_text = "Stiamo estraendo il gioco. Può richiedere qualche minuto; non chiudere il launcher.",
      .keyboard_tap_title = "Plettrata alla pressione di un tasto",
      .keyboard_tap_hint = "Ogni tasto della chitarra plettra quando lo premi; i tasti premuti a meno di 20 ms l'uno dall'altro sono un accordo, una sola plettrata.",
      .part_instrument = "Chitarra",
      .part_vocals = "Voce",
  };
  switch (language) {
    case 3:
      return kGerman;
    case 4:
      return kFrench;
    case 5:
      return kSpanish;
    case 6:
      return kItalian;
    default:
      return kEnglish;
  }
}


namespace {

// The frets' colors (the mockup's) and the chips' text colors.
constexpr uint32_t kGreen = 0x3FAE5F;
constexpr uint32_t kRed = 0xD9443A;
constexpr uint32_t kYellow = 0xE8C547;
constexpr uint32_t kBlue = 0x3E82D6;
constexpr uint32_t kOrange = 0xEB8A35;
constexpr uint32_t kGrey = 0x2A2A30;
constexpr uint32_t kOnFret = 0x0E0E10;
constexpr uint32_t kOnGrey = 0xC9C9D0;
constexpr uint32_t kOnStarPower = 0xC5DC3A;

// One language's words for the guitar's functions.
struct ControlWords {
  const char* frets;
  const char* strum_group;
  const char* menus;
  const char* green;
  const char* red;
  const char* yellow;
  const char* blue;
  const char* orange;
  const char* strum_up;
  const char* strum_down;
  const char* whammy;
  const char* whammy_chip;
  const char* dpad_up;
  const char* dpad_down;
  const char* right_stick;
};

// The game's guitar on the Xbox 360 controller (what the game reads from a guitar, and from a controller or the
// keyboard reported as one, src/startup.cpp's gamepad_subtype): the frets on A, B, Y, X and LB, strumming on the
// D-pad, the whammy bar on the right stick, Star Power on Back. Back is Star Power in the menus too, so it is listed
// once.
std::vector<GameControlGroup> Guitar(const ControlWords& w) {
  return {
      {w.frets,
       {
           {int(Bind::kA), "", kGreen, kOnFret, w.green},
           {int(Bind::kB), "", kRed, kOnFret, w.red},
           {int(Bind::kY), "", kYellow, kOnFret, w.yellow},
           {int(Bind::kX), "", kBlue, kOnFret, w.blue},
           {int(Bind::kLeftShoulder), "", kOrange, kOnFret, w.orange},
       }},
      {w.strum_group,
       {
           {int(Bind::kDpadUp), "\xE2\x86\x91", kGrey, kOnGrey, w.strum_up, w.dpad_up},
           {int(Bind::kDpadDown), "\xE2\x86\x93", kGrey, kOnGrey, w.strum_down, w.dpad_down},
           {int(Bind::kRStickRight), w.whammy_chip, kGrey, kOnGrey, w.whammy, w.right_stick},
           {int(Bind::kBack), "SP", kGrey, kOnStarPower, "Star Power"},
       }},
      {w.menus,
       {
           {int(Bind::kStart), "START", kGrey, kOnGrey, "Start"},
       }},
  };
}

// One language's words for the drums' functions.
struct DrumWords {
  const char* pads;
  const char* kick_group;
  const char* menus;
  const char* red;
  const char* yellow_cymbal;
  const char* yellow_pad;
  const char* blue;
  const char* orange;
  const char* green;
  const char* kick;
  const char* kick_chip;
  const char* dpad_up;
  const char* dpad_down;
};

// The game's drums on the Xbox 360 controller, in the highway's order (GH Metallica's): a Guitar Hero kit's pads on B,
// Y, X and A, its orange cymbal on RB and its kick pedal on LB; a Rock Band kit (four lanes) has no orange. Star Power on
// Back; the menus on the D-pad and Start.
std::vector<GameControlGroup> Drums(const DrumWords& w, bool four) {
  std::vector<GameControl> pads = {
      {int(Bind::kB), "", kRed, kOnFret, w.red},
      {int(Bind::kY), "", kYellow, kOnFret, four ? w.yellow_pad : w.yellow_cymbal},
      {int(Bind::kX), "", kBlue, kOnFret, w.blue},
  };
  if (!four) pads.push_back({int(Bind::kRightShoulder), "", kOrange, kOnFret, w.orange});
  pads.push_back({int(Bind::kA), "", kGreen, kOnFret, w.green});
  return {
      {w.pads, pads},
      {w.kick_group,
       {
           {int(Bind::kLeftShoulder), w.kick_chip, kGrey, kOnGrey, w.kick},
           {int(Bind::kBack), "SP", kGrey, kOnStarPower, "Star Power"},
       }},
      {w.menus,
       {
           {int(Bind::kDpadUp), "\xE2\x86\x91", kGrey, kOnGrey, w.dpad_up, w.dpad_up},
           {int(Bind::kDpadDown), "\xE2\x86\x93", kGrey, kOnGrey, w.dpad_down, w.dpad_down},
           {int(Bind::kStart), "START", kGrey, kOnGrey, "Start"},
       }},
  };
}

}  // namespace

const std::vector<GameControlGroup>& GameDrumControlsFor(uint32_t language, bool four_lanes) {
  static const DrumWords kEnglish{"Drums \xC2\xB7 Pads", "Drums \xC2\xB7 Kick and Star Power", "Menus", "Red pad",
                                  "Yellow cymbal", "Yellow pad", "Blue pad", "Orange cymbal", "Green pad",
                                  "Kick pedal", "KICK", "D-pad up", "D-pad down"};
  static const DrumWords kGerman{"Schlagzeug \xC2\xB7 Pads", "Schlagzeug \xC2\xB7 Bass und Star Power", "Menüs",
                                 "Rotes Pad", "Gelbes Becken", "Gelbes Pad", "Blaues Pad", "Oranges Becken",
                                 "Grünes Pad", "Bassdrum-Pedal", "BASS", "Steuerkreuz hoch", "Steuerkreuz runter"};
  static const DrumWords kFrench{"Batterie \xC2\xB7 Pads", "Batterie \xC2\xB7 Grosse caisse et Star Power", "Menus",
                                 "Pad rouge", "Cymbale jaune", "Pad jaune", "Pad bleu", "Cymbale orange", "Pad vert",
                                 "Pédale de grosse caisse", "GC", "Croix haut", "Croix bas"};
  static const DrumWords kSpanish{"Batería \xC2\xB7 Pads", "Batería \xC2\xB7 Bombo y Star Power", "Menús",
                                  "Pad rojo", "Platillo amarillo", "Pad amarillo", "Pad azul", "Platillo naranja",
                                  "Pad verde", "Pedal del bombo", "BOMBO", "Cruceta arriba", "Cruceta abajo"};
  static const DrumWords kItalian{"Batteria \xC2\xB7 Pad", "Batteria \xC2\xB7 Cassa e Star Power", "Menu",
                                  "Pad rosso", "Piatto giallo", "Pad giallo", "Pad blu", "Piatto arancione",
                                  "Pad verde", "Pedale della cassa", "CASSA", "Croce su", "Croce giù"};
  const DrumWords& words = language == 3   ? kGerman
                           : language == 4 ? kFrench
                           : language == 5 ? kSpanish
                           : language == 6 ? kItalian
                                           : kEnglish;
  // Built once per language and kind of kit.
  static std::vector<GameControlGroup> built[7][2];
  static bool done[7][2] = {};
  const uint32_t index = language < 7 ? language : 1;
  if (!done[index][four_lanes]) {
    built[index][four_lanes] = Drums(words, four_lanes);
    done[index][four_lanes] = true;
  }
  return built[index][four_lanes];
}

const std::vector<GameControlGroup>& GameControlsFor(uint32_t language) {
  static const std::vector<GameControlGroup> kEnglish = Guitar({
      "Guitar \xC2\xB7 Frets", "Guitar \xC2\xB7 Strum and effects", "Menus", "Green fret", "Red fret", "Yellow fret",
      "Blue fret", "Orange fret", "Strum up", "Strum down", "Whammy bar", "WHAM", "D-pad up", "D-pad down",
      "Right stick"});
  static const std::vector<GameControlGroup> kGerman = Guitar({
      "Gitarre \xC2\xB7 Bünde", "Gitarre \xC2\xB7 Anschlag und Effekte", "Menüs", "Grüner Bund", "Roter Bund",
      "Gelber Bund", "Blauer Bund", "Oranger Bund", "Anschlag hoch", "Anschlag runter", "Whammy-Hebel", "WHAM",
      "Steuerkreuz hoch", "Steuerkreuz runter", "Rechter Stick"});
  static const std::vector<GameControlGroup> kFrench = Guitar({
      "Guitare \xC2\xB7 Frettes", "Guitare \xC2\xB7 Grattage et effets", "Menus", "Frette verte", "Frette rouge",
      "Frette jaune", "Frette bleue", "Frette orange", "Grattage haut", "Grattage bas", "Levier de vibrato", "VIBR",
      "Croix haut", "Croix bas", "Stick droit"});
  static const std::vector<GameControlGroup> kSpanish = Guitar({
      "Guitarra \xC2\xB7 Trastes", "Guitarra \xC2\xB7 Rasgueo y efectos", "Menús", "Traste verde", "Traste rojo",
      "Traste amarillo", "Traste azul", "Traste naranja", "Rasgueo arriba", "Rasgueo abajo", "Palanca de trémolo",
      "TRÉM", "Cruceta arriba", "Cruceta abajo", "Stick derecho"});
  static const std::vector<GameControlGroup> kItalian = Guitar({
      "Chitarra \xC2\xB7 Tasti", "Chitarra \xC2\xB7 Plettro ed effetti", "Menu", "Tasto verde", "Tasto rosso",
      "Tasto giallo", "Tasto blu", "Tasto arancione", "Plettrata su", "Plettrata giù", "Leva del vibrato", "VIBR",
      "Croce su", "Croce giù", "Levetta destra"});
  switch (language) {
    case 3:
      return kGerman;
    case 4:
      return kFrench;
    case 5:
      return kSpanish;
    case 6:
      return kItalian;
    default:
      return kEnglish;
  }
}

}  // namespace launcher::game
