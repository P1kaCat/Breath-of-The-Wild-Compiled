# Assets personnalisés

Dépose tes propres fichiers dans les sous-dossiers :

- `textures/` : textures et images.
- `models/` : modèles et animations.
- `shaders/` : sources de shaders personnalisés.
- `audio/` : sons et musiques.
- `ui/` : kits et références pour l'interface.

Ces fichiers restent locaux : Git publie uniquement ces instructions et les dossiers vides. Les fichiers du jeu et ses sauvegardes restent dans leur espace de travail séparé.

Le logo de l'écran d'accueil est pris en charge : place ton PNG transparent dans `textures/title-logo.png`, puis lance `python tools/install_logo.py` depuis le dossier du projet. Il remplace la texture du logo dans les copies locales de `Bootup.pack` et `Title.pack`. Le prochain lancement utilise alors ce logo. Python 3.10 ou supérieur, Pillow et oead sont nécessaires (`python -m pip install -r tools/requirements-assets.txt`).

Pour revenir au logo du jeu, utilise `python tools/install_logo.py --restore`. Les archives d'origine sont conservées dans `workspace/custom-assets-backup/title-logo`. Les archives modifiées et le PNG restent locaux.

Pour les autres assets, ce dossier sert à préparer tes fichiers. Le runtime ne charge pas encore automatiquement leur contenu ; leur intégration au jeu reste à développer.

Le kit Figma de référence peut être conservé dans `ui/zelda-botw-ui-kit.fig`. Cette copie complète reste locale ; la présentation GitHub utilise seulement un ornement SVG exporté et les couleurs du kit.

Pour le remplacement de shaders pixel déjà disponible, utilise `workspace/shader-overrides` et l'option `-ShaderOverrides` du lanceur, comme décrit dans le guide principal.
