# Validation de cette session

La base Windows x64 se compile et lance le code recompilé de BOTW. Le contrôle visuel confirme l'écran d'ouverture, puis le texte de l'introduction. La capture finale est uniformément jaune : le rendu de Link et du décor reste incomplet et des programmes vertex sont rejetés par le renderer.

**État des outils au dernier contrôle :** le build Release et les essais ci-dessous ont bien été effectués avec MSVC 2022. L'installation `C:\Program Files\Microsoft Visual Studio\2022\Community` n'est ensuite plus présente et `vswhere` ne trouve aucune installation Visual Studio. Une nouvelle configuration CMake échoue pour ce motif. Les binaires sont conservés ; reconstruire nécessitera les outils C++ de Visual Studio 2022. Le diagnostic `doctor` signale désormais cette absence.

## Entrées et isolation

- Jeu européen : titre `00050000101C9500`, mise à jour v208 et DLC `0005000C101C9500` v80.
- RPX : SHA-256 `BA58DA5B95CE929E005D058CEB08B9B2788D1AB2BBC8A6C189BBADCA0BB34D30`, identique à l'entrée attendue par les hooks de CafeRecomp.
- Les trois titres NUS ont été copiés, puis extraits dans `E:\BOTW Compiled\workspace\game`. Les fichiers sources et leurs copies NUS avaient les mêmes tailles et dates de modification lors du contrôle.
- Seul `portable/mlc01/sys/title` a été copié pour les titres système. Aucune sauvegarde Cemu n'a été copiée ; aucun changement n'a été effectué dans le dossier Cemu actif.
- Le lanceur utilise les données de cette copie, ses chemins de sauvegarde sous `game/update/save`, ses journaux et un MLC séparé.

## Contrôles effectués

- Build Release MSVC x64 du traducteur et du runtime complet : réussi. Sources et objets PowerPC locaux ; aucune dépendance au processus Cemu pour exécuter le port.
- Suite complète CTest : **24 tests réussis sur 24**. Elle inclut le chargement du RPX, un boot de 10 000 blocs, les formats/cibles GPU, les shaders, l'audio, la mémoire, les imports et l'identité de titre américaine/européenne.
- Suite indépendante des composants : **12 tests réussis sur 12**.
- DolRecomp : **5 tests ciblés réussis** (`rpx`, `opcodes`, `c_execute`, `fpscr`, `float_semantics`).
- La présentation crée réellement une fenêtre cachée de 640×360 avec la configuration demandée. Les contrôles de présentation ne changent pas la résolution des cibles du jeu.
- Le remplacement HLSL a été vérifié avec une lecture des pixels GPU : rouge d'origine, vert après remplacement. Les sources invalides et une origine différente sont refusées ; les draws refusés préservent la cible.
- Les gardes ont refusé un RPX synthétique non pris en charge et un chevauchement des dossiers source/copie. Syntaxe PowerShell et `git diff --check` contrôlés.

La génération du v208 produit 2 259 chunks C. Un défaut réel a été corrigé : le backend générique émettait les accès UGQR vers un helper qui les traitait comme privilégiés. Le post-traitement reconstruit l'instruction brute et passe par le gestionnaire Espresso existant, avec reprise au PC suivant. 51 chunks sont concernés. Le test reproduisant l'instruction `0x03E27924` échouait avant et passe après correction, sans modification de son assertion. Les encodages lecture/écriture et l'idempotence du post-traitement ont aussi été contrôlés sur un fichier synthétique.

## Essai du jeu

Un processus du port a tourné caché et silencieux pendant environ 120 secondes, avec capture et appui Start logique de diagnostic. Il a ensuite été arrêté par le minuteur du lanceur ; ce n'est pas une sortie normale du jeu. Le compteur interne atteint 2 760 presents, ce qui ne constitue pas une mesure de FPS ou de jouabilité.

Le logo et le fond de l'écran d'ouverture apparaissent réellement, puis le texte « Breath of the Wild » a été observé pendant l'introduction. La capture de fin est un écran jaune uni. Le journal signale plusieurs rejets au stade `vertex-program`, notamment aux frames internes 1 724, 2 415 et 2 432. Il faut résoudre ces programmes, puis vérifier les personnages, le décor, les textures, la profondeur et les effets dans une scène réelle avant d'annoncer un jeu jouable ou des améliorations graphiques validées.

Écran d'ouverture obtenu :

Capture locale : `first-render.png` (exclue du dépôt de sources).

État final du diagnostic, montrant le défaut de rendu restant :

Capture locale : `runtime-capture.png` (exclue du dépôt de sources).

## Retrouver les résultats

Les journaux détaillés restent dans `E:\BOTW Compiled\workspace\logs`, notamment `native-tests-final.log`, `runtime.log` et `runtime-probe.json`. Les captures/programmes restent dans `captures` et les HLSL locaux dans `shader-overrides` sous cet espace. Les binaires sont dans `E:\BOTW Compiled\build\native\bin` ; le lanceur du projet retrouve ce chemin via `native-build.json`.

## Déplacement du 15 octobre 2026

Le projet et ses données locales sont regroupés dans `E:\BOTW Compiled` : sources à la racine, données dans `workspace`, binaires et objets dans `build`, référence WiiCompiled dans `references`. La copie de l'espace de travail comprend 65 610 fichiers (39 332 929 294 octets). La comparaison après copie ne signale aucun fichier manquant, différent ou supplémentaire. L'empreinte du RPX et celles des binaires sont vérifiées.

Les 24 tests CTest passent après le déplacement. Le lanceur exécuté depuis ce dossier utilise les nouveaux chemins et termine normalement un diagnostic caché et silencieux de 10 000 blocs, avec code de sortie 0 et sans arrêt par le minuteur. Ce contrôle confirme le lancement depuis le nouvel emplacement ; il ne valide pas le rendu complet d'une scène.

Les anciens chemins sont des jonctions vers le nouveau dossier, pour les chemins absolus conservés dans le build. Les sources et données d'avant le déplacement sont conservées avec le suffixe `.before-relocation` aux anciens emplacements. Les journaux du long essai de rendu précédent sont préservés dans `workspace/logs/before-relocation`. Les nouveaux contrôles sont dans `workspace/logs/relocation-tests.log`, `runtime.log`, `runtime-probe.json` et les journaux `relocation-*.log`. Les fichiers Cemu actifs et leur sauvegarde ne sont pas concernés par ce déplacement.
