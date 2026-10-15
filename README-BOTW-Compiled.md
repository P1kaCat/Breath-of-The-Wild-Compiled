# BOTW Compiled

Port natif expérimental de **Breath of the Wild Wii U** pour Windows x64, basé sur CafeRecomp. Ce dépôt contient le runtime et les outils ; les fichiers du jeu et le code généré restent dans un espace de travail local séparé.

La recompilation PowerPC, le runtime Cafe et le traducteur de shaders GX2/Latte sont hérités de CafeRecomp. Son rendu complet de Link et du décor reste incomplet. Construire le programme ou réussir les tests GPU ne prouve pas que le jeu est jouable.

Dans cette session, 24 tests du runtime passent. Le lancement montre l'écran d'ouverture et le texte d'introduction, puis un écran jaune uni avec des programmes vertex rejetés. Voir [la validation](VALIDATION.md). Les captures restent locales.

## Démarrer

Prérequis : Visual Studio 2022 avec les outils C++ et un SDK Windows, CMake et PowerShell. La compilation du traducteur télécharge zlib depuis sa distribution officielle, avec une empreinte SHA-256 épinglée.

Au dernier contrôle de cette session, Visual Studio 2022 n'est plus installé. Le build effectué et ses binaires sont conservés ; il faut réinstaller les outils C++ pour reconstruire le projet.

Depuis ce dossier :

```powershell
.\botw.ps1 doctor
.\botw.ps1 components
.\botw.ps1 translator
.\botw.ps1 generate
.\botw.ps1 build
.\botw.ps1 run -Headless -Mute -Blocks 10000
```

Le projet est regroupé dans `E:\BOTW Compiled`. Le chemin par défaut des données et journaux est le sous-dossier `workspace`, calculé depuis le dossier du projet. Utiliser `-WorkRoot` pour changer ce chemin dans chaque commande. Les titres doivent être extraits dans `game/base`, `game/update` et `game/dlc` sous ce chemin. La préparation réussie écrit `snapshot.complete.json` ; le lancement exige ce marqueur pour éviter de démarrer pendant une extraction. Le RPX v208 est vérifié avant génération et lancement ; une empreinte différente est refusée car les hooks hérités utilisent des adresses fixes.

Les commandes `components` et `translator` fonctionnent sans fichiers du jeu. `components` construit et teste les shaders, textures, cibles HDR, post-traitement, mémoire et audio avec des entrées contrôlées.

La génération écrit plusieurs gigaoctets de C dans l'espace de travail. Le build complet produit `game-build/bin/botw_recomp.exe` et ses DLL. La limite de blocs du lancement par défaut est un diagnostic ; `-Blocks 0` lance sans cette limite.

Les sorties natives sont dans `E:\BOTW Compiled\build\native`, et les copies du jeu et les sources générées dans `E:\BOTW Compiled\workspace`. Le chemin du build réussi est conservé dans `workspace/native-build.json`, puis repris par `doctor` et `run`. Des jonctions aux anciens emplacements permettent de conserver les chemins absolus du build existant. Les anciennes copies des sources et des données sont conservées à côté de ces jonctions avec le suffixe `.before-relocation`. Une reconstruction à partir de zéro doit utiliser un nouveau `-BuildRoot`. Aucun de ces dossiers n'est le dossier Cemu actif.

`-BuildRoot` permet de choisir un autre dossier de compilation native. L'option de développement `-PrecompiledObjects` peut réutiliser les objets Release déjà construits avec le même compilateur, les mêmes options et le même ABI ; les dates des sources et des en-têtes sont vérifiées. Cette session utilise ce cache pour relier la DLL sur le SSD en reconstruisant les sources modifiées.

Les sources dont les dates sont plus récentes que les objets sont recompilées normalement. `-RecompileGenerated` force la reconstruction complète, notamment après un changement de compilateur ou d'options.

`test` exécute les tests du runtime complet si le lanceur a été construit, sinon ceux des composants.

## Réglages disponibles

```powershell
.\botw.ps1 run -Width 1920 -Height 1080 -VSync -PresentationFilter linear -Blocks 0
```

`Width` et `Height` changent la taille cliente de la fenêtre et la présentation. Ils n'augmentent pas encore la résolution des cibles de rendu du jeu. La VSync agit sur le swapchain D3D11. Le filtre `nearest` ou `linear` concerne seulement la présentation finale ; les samplers de matériaux du jeu sont conservés.

Les deux axes d'amélioration souhaités seront développés séparément : résolution interne, ombres et distances d'affichage d'une part ; éclairage, effets et textures d'autre part. Ils restent à implémenter après la validation des scènes d'origine. Aucun réglage fictif de ray tracing, textures HD, FPS ou distance d'affichage n'est exposé.

### Remplacement local de shaders pixel

L'option `-ShaderOverrides` exporte les shaders pixel traduits et compilés pour la première fois dans `shader-overrides` sous l'espace de travail : `XXXXXXXX.original.hlsl`. Pour expérimenter, copier un fichier en `XXXXXXXX.override.hlsl`, modifier cette copie, puis relancer le programme. Les shaders déjà en cache ne sont pas rechargés immédiatement.

Le contenu complet du fichier d'origine est comparé avant d'appliquer un remplacement, pour éviter qu'une collision de l'identifiant sélectionne un autre shader. Le fichier d'origine n'est jamais écrasé. Une source invalide ou un fichier d'origine modifié bloque ce shader et produit un diagnostic. Les remplacements doivent conserver les interfaces de ressources et de sorties du shader. Cette fonctionnalité couvre uniquement les shaders pixel que le traducteur sait déjà traiter ; elle ne complète pas les instructions Latte manquantes et ne constitue pas une amélioration d'éclairage du jeu déjà validée.

`-Capture` enregistre les captures de diagnostic dans le même espace de travail. Les programmes/shaders du jeu et les fichiers générés restent locaux et ne sont pas ajoutés au dépôt.

Un essai limité dans le temps peut être lancé avec `run -Blocks 0 -Seconds 120 -Headless -Mute -Capture`. Seul le processus créé par cette commande est arrêté à la limite ; cela ne ferme pas Cemu. Un arrêt par cette limite est une fin de diagnostic, pas la preuve d'un démarrage réussi.

## Isolation des données

La préparation de cette session utilise une copie des trois titres NUS, puis une extraction vers l'espace de travail. Elle n'inclut pas les sauvegardes Cemu. Le lanceur utilise uniquement les titres extraits de cette copie. Les chemins de sauvegarde hérités du runtime aboutissent dans `game/update/save` sous l'espace de travail ; les logs sont dans `logs/runtime.log`.

`BOTW_WIIU_MLC_ROOT` est fixé à `system/mlc01` dans cet espace : le lanceur ne recherche pas automatiquement le MLC de Cemu. Des fichiers système manquants peuvent encore empêcher un lancement complet ; ils devront provenir d'une copie séparée du dump de la console.

La préparation de cette session inclut seulement une copie de `portable/mlc01/sys/title` depuis Cemu, sans `usr/save`. Le script `tools/prepare-game.ps1` permet de reproduire la copie/extraction des trois titres européens avec un exécutable CDecrypt fourni par l'utilisateur et un nouvel espace de travail ; il refuse une extraction déjà existante.

Le dossier Cemu actif et sa sauvegarde ne sont pas modifiés. Ne rediriger aucun de ces chemins vers la session Cemu active.

La recompilation du RPX européen v208 utilise la même empreinte que l'amont. Le lanceur construit par `botw.ps1 build` prend le titre, le groupe et la région dans les métadonnées copiées. Les services de titre et de DLC du runtime utilisent cette identité, au lieu de déclarer systématiquement un titre américain.

## Provenance

- [CafeRecomp](https://github.com/hardkiller2565123123/CafeRecomp), commit `922b852e56cd440d23ed8ede3e0b86495e68967f` : base du code, licences et notices conservées.
- [WiiCompiled](https://github.com/patchzyy/wiicompiled), commit `bd219e38a3e9a1a5047ce41d8cd01210bef3af43` : référence pour le pipeline et les contrôles de recompilation PowerPC. Son runtime Wii/GX ne constitue pas un backend Wii U/GX2. Aucun code de ce dépôt n'a été importé.
- [CDecrypt](https://github.com/VitaSmith/cdecrypt) : utilisé localement pour extraire la copie NUS dans un dossier distinct ; il n'est pas livré avec ce projet.

Les historiques `README` et `RELEASE-NOTES` décrivent les essais de l'amont, pas des essais effectués sur ce nouveau projet. `VALIDATION.md` décrit les contrôles de cette session.
