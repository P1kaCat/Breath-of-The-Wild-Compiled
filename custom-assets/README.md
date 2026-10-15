# Assets personnalisés

Dépose tes propres fichiers dans les sous-dossiers :

- `textures/` : textures et images.
- `models/` : modèles et animations.
- `shaders/` : sources de shaders personnalisés.
- `audio/` : sons et musiques.

Ces fichiers restent locaux : Git publie uniquement ces instructions et les dossiers vides. Les fichiers du jeu et ses sauvegardes restent dans leur espace de travail séparé.

Ce dossier sert pour l'instant à préparer tes assets. Le runtime ne charge pas encore automatiquement son contenu ; leur intégration au jeu reste à développer.

Pour le remplacement de shaders pixel déjà disponible, utilise `workspace/shader-overrides` et l'option `-ShaderOverrides` du lanceur, comme décrit dans le guide principal.
