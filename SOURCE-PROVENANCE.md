# Source provenance

BOTW Compiled is an experimental foundation for a Windows port of Breath of the Wild for Wii U.

- CafeRecomp base: https://github.com/hardkiller2565123123/CafeRecomp, commit `922b852e56cd440d23ed8ede3e0b86495e68967f`.
- The bundled DolRecomp translator comes from that base. Its GPL-3.0 license and notices are preserved in `third_party/DolRecomp` and `LICENSE-DolRecomp-GPL-3.0`.
- Code derived from Cemu retains its notices and the MPL-2.0 license in `LICENSE-Cemu-MPL-2.0`; see also `CEMU_INTEGRATION`.
- WiiCompiled: https://github.com/patchzyy/wiicompiled, commit `bd219e38a3e9a1a5047ce41d8cd01210bef3af43`, consulted as a reference. No code was imported.

Project-specific changes include isolated preparation tools, presentation settings, local pixel shader overrides, a fix for generated Espresso SPR accesses, and selection of the European title identity.

Publication starts with a new history to exclude binaries tracked in the upstream repository. Licenses, notices within source files, and these references are retained. Game files, keys, saves, code generated from the RPX, and build output are not distributed here.

The [upstream presentation and credits](docs/UPSTREAM-README.md) are preserved separately from the BOTW Compiled homepage.

The GitHub presentation ornament comes from Hunter Paramore's Zelda BOTW UI Kit. Its [source and attribution](docs/assets/botw-ui/README.md) are retained with the SVG export; the complete Figma file stays local.
