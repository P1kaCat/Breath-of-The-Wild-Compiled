# Custom assets

Place your own files in these subfolders:

- `textures/`: textures and images.
- `models/`: models and animations.
- `shaders/`: custom shader source files.
- `audio/`: sounds and music.
- `ui/`: interface kits and references.

These files stay local: Git publishes only these instructions and the empty folder structure. Game files and saves remain in their separate workspace.

Custom opening logos are supported. Place a transparent PNG at `textures/title-logo.png`, then run `python tools/install_logo.py` from the project directory. It replaces the logo texture in the local copies of `Bootup.pack` and `Title.pack`; the next launch uses the new logo. Python 3.10 or newer, Pillow, and oead are required (`python -m pip install -r tools/requirements-assets.txt`).

To restore the game's logo, run `python tools/install_logo.py --restore`. Original archives are backed up in `workspace/custom-assets-backup/title-logo`. Modified archives and the PNG stay local.

For other assets, this folder is a preparation area. The runtime does not automatically load them yet; their game integration remains to be developed.

The reference Figma kit can be kept at `ui/zelda-botw-ui-kit.fig`. This complete copy stays local; the GitHub presentation uses only an exported SVG ornament and the kit's colors.

For the existing pixel shader override feature, use `workspace/shader-overrides` and the launcher's `-ShaderOverrides` option as described in the main guide.
