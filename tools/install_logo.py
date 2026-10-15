#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""Install a local PNG in BOTW's existing Wii U title-logo textures.

Requires Pillow and oead. No generated code, game archive or image is distributed.
RGBA8 macro-tile addressing follows src/wiiu_gx2_cemu_layout.c, derived from
Cemu (13d1826b4d018c2b6674f58b9b772e21ba084f61), under MPL-2.0.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import struct

import oead
from PIL import Image, ImageOps


PROJECT = Path(__file__).resolve().parent.parent
LOGO = "timg/Logo_W_00^l.bflim"
ARCHIVES = {"Bootup.pack": "Layout/BootUp.sblarc",
            "Title.pack": "Layout/Title.sblarc"}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def atomic_write(path, data):
    temporary = path.with_name(path.name + ".logo-tmp")
    with temporary.open("wb") as stream:
        stream.write(data)
        stream.flush()
        os.fsync(stream.fileno())
    os.replace(temporary, path)


def rgba8_address(x, y, pitch, swizzle):
    """Single-slice RGBA8 GX2 tile mode 4; 2 pipes, 4 banks."""
    pixel = ((x & 3) | ((y & 1) << 2) | ((x & 4) << 1)
             | ((y & 2) << 3) | ((y & 4) << 3))
    pipe = ((y >> 3) ^ (x >> 3)) & 1
    bank = (y >> 4) & 3
    bank = (((bank >> 1) | (bank << 1)) ^ (x >> 3)) & 3
    bank_pipe = (pipe + 2 * bank) ^ ((swizzle >> 8) & 7)
    pipe, bank = bank_pipe & 1, bank_pipe >> 1
    macro = ((x // 32) + (pitch // 32) * (y // 16)) * 2048
    offset = (macro >> 3) + pixel * 4
    return ((offset & ~255) << 3) | (offset & 255) | (pipe << 8) | (bank << 9)


def encode_logo(original, image):
    if original[-40:-36] != b"FLIM" or original[-36:-34] != b"\xfe\xff":
        raise ValueError("Expected a big-endian Wii U BFLIM logo.")
    magic, info_size, width, height, alignment, fmt, tiled, size = struct.unpack(
        ">4sI3H2BI", original[-20:])
    if (magic, info_size, fmt, tiled & 31) != (b"imag", 16, 0x14, 4):
        raise ValueError("Only the existing RGBA8 sRGB, tile-mode-4 logo is supported.")
    pitch = (width + 31) & ~31
    padded_height = (height + 15) & ~15
    if size != pitch * padded_height * 4 or len(original) != size + 40:
        raise ValueError("Unexpected logo surface size.")
    fitted = ImageOps.contain(image, (width, height), Image.Resampling.LANCZOS)
    canvas = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    canvas.paste(fitted, ((width - fitted.width) // 2, (height - fitted.height) // 2))
    linear = canvas.tobytes()
    swizzle = ((tiled >> 5) & 7) << 8
    pixels = bytearray(size)
    seen = set()
    for y in range(height):
        for x in range(width):
            offset = rgba8_address(x, y, pitch, swizzle)
            if offset in seen or offset + 4 > size:
                raise ValueError("Invalid or overlapping texture address.")
            seen.add(offset)
            source = (y * width + x) * 4
            pixels[offset:offset + 4] = linear[source:source + 4]
    return bytes(pixels) + original[-40:], canvas


def patch_archive(original, layout_name, image, preview):
    archive = oead.Sarc(original)
    layout = bytes(archive.get_file(layout_name).data)
    if layout[:4] != b"Yaz0":
        raise ValueError("Expected the original Yaz0-compressed layout.")
    unpacked = bytes(oead.yaz0.decompress(layout))
    inner = oead.Sarc(unpacked)
    logo = bytes(inner.get_file(LOGO).data)
    replacement, canvas = encode_logo(logo, image)
    # Keep the inner archive's offsets, size, texture metadata and heap requirement.
    if unpacked.count(logo) != 1 or len(replacement) != len(logo):
        raise ValueError("Cannot replace the logo without changing its allocation.")
    new_inner = unpacked.replace(logo, replacement, 1)
    compressed = bytes(oead.yaz0.compress(new_inner, level=7))
    compressed = layout[:16] + compressed[16:]
    writer = oead.SarcWriter.from_sarc(archive)
    writer.files[layout_name] = compressed
    _, result = writer.write()
    result = bytes(result)
    # Independently reopen both archives and compare every unaffected member.
    checked = oead.Sarc(result)
    if {f.name for f in checked.get_files()} != {f.name for f in archive.get_files()}:
        raise ValueError("Archive member list changed.")
    for member in archive.get_files():
        if member.name != layout_name and bytes(checked.get_file(member.name).data) != bytes(member.data):
            raise ValueError("An unrelated archive member changed.")
    checked_inner = bytes(oead.yaz0.decompress(checked.get_file(layout_name).data))
    if checked_inner != new_inner or len(checked_inner) != len(unpacked):
        raise ValueError("Layout verification failed.")
    canvas.save(preview)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--work-root", type=Path, default=PROJECT / "workspace")
    parser.add_argument("--image", type=Path, default=PROJECT / "custom-assets/textures/title-logo.png")
    parser.add_argument("--restore", action="store_true")
    args = parser.parse_args()
    work = args.work_root.resolve()
    if work == Path(work.anchor) or not (work / "snapshot.complete.json").is_file():
        raise ValueError("Use the isolated, prepared project workspace.")
    if not json.loads((work / "snapshot.complete.json").read_text(encoding="utf-8-sig")).get("complete"):
        raise ValueError("Game preparation is incomplete.")
    targets = work / "game/update/content/Pack"
    backup = work / "custom-assets-backup/title-logo"
    manifest_path = backup / "installed.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8")) if manifest_path.exists() else {"archives": {}}
    backup.mkdir(parents=True, exist_ok=True)
    image = None
    if not args.restore:
        image = Image.open(args.image).convert("RGBA")
        if image.getextrema()[3][1] == 0:
            raise ValueError("The supplied image is entirely transparent.")
    prepared = {}
    for name, layout in ARCHIVES.items():
        target = targets / name
        saved = backup / name
        current = target.read_bytes()
        if not saved.exists():
            if args.restore:
                raise ValueError("No original archive backup exists.")
            with saved.open("xb") as stream:
                stream.write(current)
        original = saved.read_bytes()
        record = manifest["archives"].get(name, {})
        if record and record.get("original_sha256") != digest(original):
            raise ValueError("Original backup checksum changed: " + name)
        if digest(current) not in {digest(original), record.get("installed_sha256"),
                                   record.get("previous_sha256")}:
            raise ValueError("Archive changed outside this installer; refusing to overwrite: " + name)
        prepared[name] = original if args.restore else patch_archive(
            original, layout, image, backup / "logo-preview.png")
        manifest["archives"][name] = {"original_sha256": digest(original),
                                      "previous_sha256": digest(current),
                                      "installed_sha256": digest(prepared[name])}
    manifest["enabled"] = not args.restore
    if image is not None:
        manifest["image_sha256"] = digest(args.image.read_bytes())
    # Save planned hashes first, so an interrupted installation can safely resume.
    atomic_write(manifest_path, (json.dumps(manifest, indent=2) + "\n").encode())
    for name, content in prepared.items():
        atomic_write(targets / name, content)
        if digest((targets / name).read_bytes()) != digest(content):
            raise ValueError("Installed archive checksum mismatch: " + name)
    print("Original title logos restored." if args.restore else
          "Custom logo installed in Bootup.pack and Title.pack (isolated update only).")


if __name__ == "__main__":
    main()
