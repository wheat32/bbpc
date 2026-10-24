# Mods, texture packs, patches and DLC

## Loose-file mods

The launcher's **Mods and patches** page lists the mods in the mods folder: a switch for all of
them, a switch for each, and arrows for the load order. Changes apply the next time the game starts.
When two mods replace the same file, the one lower in the list wins, and the log says which mod
replaced which.

The mods folder is `mods/` in the data folder: the checkout itself when you run from source,
`~/.local/share/bbpc/mods` with the AppImage or the release tarball. Another folder can be chosen
on the page. Extract each mod into a folder of its own:

```text
mods/
  Some Mod/
    dvdroot_ps4/
      chr/...
      param/...
      menu/...
```

These layouts work as well:

- `Some Mod/app0/dvdroot_ps4/...` and `Some Mod/CUSA03173/dvdroot_ps4/...` (any retail serial);
- one extra wrapper folder, as an archive extracted into a folder of its own name gives:
  `Some Mod/Some Mod v1.2/dvdroot_ps4/...` (readme files and pictures may sit next to it);
- the game's own folders without `dvdroot_ps4`: `Some Mod/chr/...`, `Some Mod/parts/...`
  (`chr`, `parts`, `map`, `menu`, `msg`, `param`, `sfx`, `sound`, `font` and the other
  `dvdroot_ps4` folders).

File name case does not matter: `DVDROOT_PS4/Chr/C0000.chrbnd.dcx` from a mod made on Windows
replaces the game's `dvdroot_ps4/chr/c0000.chrbnd.dcx`. The start-up log counts what each mod did,
for example `Mods: 12 game files replaced, 0 added`; a mod meant to replace files that reports
`0 game files replaced` has its files in the wrong place. Extract ZIP and 7z archives first.

Ready-made replacements of `dvdroot_ps4` files are supported: textures, models, parameters, the
interface, sound and fonts. Two mods that change the same `.dcx` or `.bnd` are not merged: the
later one replaces the whole file. Mods that replace `eboot.bin`, `sce_module` or `sce_sys`, DLL
injectors and script plugins are not supported (patches to the executable go through the patch
system below), and symbolic links inside a mod are rejected.

A `CUSA03173-mods/dvdroot_ps4/...` folder next to `CUSA03173/` (shadPS4's convention) is
applied too, before the mods of the list; the switch for all mods turns it off as well. A folder
that holds `dvdroot_ps4` itself can be chosen as the mods folder: it is then applied as one mod.

The game folder is never written to. For each launch, `scripts/mods.py` builds a merged tree of
links in the data folder's `out/mod-game-*`, which the game reads instead; `/app0` and `/hostapp`
are read-only to the game, saves stay where they always are. The tree is removed when the game ends
or the launcher stops it; one left over after a crash is not reused.

From the command line:

```bash
BB_GAME_DIR=/path/to/CUSA03173 BB_MODS_DIR=/path/to/mods bash run.sh
BB_GAME_DIR=/path/to/CUSA03173 BB_MODS_ENABLED=0 bash run.sh
```

The order and the disabled mods are kept in the data folder's `mods.json`:

```json
{"order": ["First", "Second"], "disabled": ["First"]}
```

New folders are enabled and appended in alphabetical order. `BB_MODS_CONFIG` names another
profile file; `BB_MOD_TRACE=1` (with `BB_MODS_DIR`) logs the first reads of the files mods
provide.

## Texture packs

Packs with upscaled map textures do not fit in the game's graphics heaps, which were sized for the
PS4 (2287 and 886 MiB). Once a heap is full, the game's GPU allocations fail: textures flicker and
then the game crashes (exit code 139 at Iosefka's Clinic, where the Forbidden Woods start
loading). The start-up check finds such a pack by itself, from the mods folder or already copied
into the game folder: a map texture larger than 4096 pixels on a side, which the retail 1.09 has
none of. The game then gets the *Increased Graphics Heap Sizes* patch (3952 and 2560 MiB, as the
resolution patches above 1080p use) and 9152 MiB of direct memory, about 3.3 GB more RAM. The log
says so:

```text
Graphics heaps: larger (a texture pack: dvdroot_ps4/map/m24/m24_0000.tpfbhd has 8192-pixel textures, ...)
```

The first check reads about 8600 archive entries (a fraction of a second on an SSD); the answer is
kept in `out/texture-pack.json` until the texture files change. `BB_LARGE_HEAPS=1` always gives the
larger heaps (for a pack the check does not recognize, such as one that stays at 4096 pixels),
`BB_LARGE_HEAPS=0` never.

## Third-party patches (shadPS4 XML)

The `patches/` folder in the data folder holds patch files in the shadPS4 / GoldHEN XML format
(the **Third-party patches** group of the launcher; another folder can be chosen there). The
`Metadata` entries for `AppVer="01.09"` and `eboot.bin` are used; files whose `TitleID` names other
games are skipped. Each patch starts with the file's own `isEnabled`; the launcher's choices are
kept in the data folder's `patches.json`:

```json
{"enabled": ["file.xml/Patch name"], "disabled": ["file.xml/Other patch"]}
```

The line types `bytes`, `bytes16/32/64`, `float32/64`, `utf8` and `utf16` are supported. A patch
with `mask` lines (pattern search) or with an address outside the eboot is skipped as a whole, with
a message in the log. Third-party patches are applied after the built-in ones
(`patches/Bloodborne.xml`), so they win where both touch the same bytes. Patches that replace
pointers in tables (as *60 FPS++* and *90 FPS++* do) are moved to the image's real address.

From the command line: `BB_PATCHES_DIR=/path BB_PATCHES_CONFIG=/path/patches.json bash run.sh`.

## DLC

Put your add-on dumps in `addcont/<title id>/<entitlement label>/` inside the saves folder (`user/`
by default; shadPS4's layout), for example `user/addcont/CUSA03173/SPEXPANSIONDLC03` for The Old
Hunters. Each folder there is reported to the game as an installed add-on.
