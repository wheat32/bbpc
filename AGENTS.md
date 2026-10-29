# AGENTS.md: bbpc

## Project Overview

**bbpc** is a native Linux port of *Bloodborne* (PS4, game version 1.09), a fork of
[bloodborne_pc](https://github.com/deadinside28/bloodborne_pc) ("bbport"). It has two halves:

- **The port** (inherited from the original project): the loader and PS4 runtime (`src/`, C23),
  the renderer, shader translator and online module (`gpu/`, C++23), the offline preparation of the
  game image and other helpers (`scripts/`, Python 3.10+), and `run.sh`, which prepares the image,
  applies the patches and starts `bb-probe`.
- **The launcher** (this fork's own): a **Qt6/C++23 desktop GUI** (`launcher/`). It has **no game
  logic of its own**: it edits settings and drives the port's tools through `QProcess`: `run.sh`
  (the game), `bb-gpu-capabilities` (GPUs, monitors, gamepads, key reading) and
  `scripts/launcher_query.py` (the game check, mods, patches, the FSR 4.1.1 DLL check, the co-op
  servers: the same Python code `run.sh` uses, answered as JSON).

## Launcher Architecture

```
MainWindow (sidebar + QStackedWidget, Play button)
  ├── pages/  (GamePage, DisplayPage, GraphicsPage, ControlsPage, ModsPage, OnlinePage, AdvancedPage, LogPage)
  │     each LauncherPage builds its rows from LauncherSettings + PortIni and writes them back in store()
  ├── core/   settings and logic without widgets (unit-tested)
  │     AppConfig/LauncherSettings ─ ~/.config/bbpc/launcher.json
  │     PortIni ─ bbport.ini, the port's settings (the in-game menu edits it too)
  │     GameEnvironment ─ the BB_* environment run.sh starts the game with
  └── process/
        GroupProcess ─ the game and the FSR 4.1.1 build, in their own process group
        ProcessRunner / PortQueries ─ short queries (launcher_query.py, bb-gpu-capabilities)
```

**Key data flows:**
- UI → page → `MainWindow::store()` → `launcher.json` + `bbport.ini` (only the keys changed in the launcher, so in-game changes survive)
- Play → `GameEnvironment::build()` → `bash run.sh` in a `GroupProcess` → output lines → `LogPage`
- Pages → `PortQueries::query()` → `python3 scripts/launcher_query.py <command>` → JSON → page
- Pages → `PortQueries::capabilities()` → `bb-gpu-capabilities --gpus|--displays|--gamepads|--read-input` → `Capabilities::parse*()`

## Build & Run

```bash
bash packaging/arch/install-deps.sh   # Arch packages (packaging/arch/packages.txt)
bash build.sh                         # release preset into out/ (what run.sh plays)
bash build.sh --test                  # the same with every test, then ctest
bash launcher/bb-launcher.sh          # the launcher (out/launcher/bbpc)
```

Presets (`CMakePresets.json`): `release` (`out/`), `debug` (`out-debug/`, with the tests) and
`launcher` (`out-launcher/`: the launcher alone, without the game's dependencies):

```bash
cmake --preset launcher && cmake --build --preset launcher && ctest --preset launcher
```

CLion: `bash clion.sh`; the presets are its CMake profiles and `.run/` holds the shared run
configurations. `run.sh` rebuilds before playing; `BB_BUILD_DIR=out-debug bash run.sh` plays the
debug build.

## Subprocesses and the AppImage

Every program the launcher starts runs with `HostEnvironment::host()` (`core/hostEnvironment.h`):
inside the AppImage it removes the bundle's entries from `LD_LIBRARY_PATH`, `QT_PLUGIN_PATH` and
similar variables, so host programs (bash, Python, Proton, the file manager) never load the bundled
libraries. Use `GroupProcess` for long-running programs (stoppable as a group), `ProcessRunner` or
`PortQueries` for short ones. Never call `QProcess::start()` with the launcher's own environment.

The port's binaries in the AppImage (`usr/share/bbpc/bin`) find the bundled libraries through
their RUNPATH (`BBPC_PACKAGE_RPATH`), never through `LD_LIBRARY_PATH`, and must not be patched
with patchelf: `bb-probe` is not position independent.

## Key Files

| File | Purpose |
|---|---|
| `launcher/mainWindow.h/cpp` | Sidebar, pages, Play/Stop, saving, starting the game |
| `launcher/core/appConfig.h/cpp` | Launcher settings → `~/.config/bbpc/launcher.json` (owner-only: it holds the online password and token); imports the original launcher's settings once |
| `launcher/core/launcherSettings.h/cpp` | The settings struct and its JSON keys (the original launcher's, unknown keys kept) |
| `launcher/core/portIni.h/cpp` | `bbport.ini`: edits in place, `saveChanges()` writes only the launcher's own changes |
| `launcher/core/gameEnvironment.h/cpp` | Settings → the `BB_*` environment for `run.sh` |
| `launcher/core/portPaths.h/cpp` | Where the port, its binaries and its data folder are (source tree or installed `share/bbpc`) |
| `launcher/core/choices.h/cpp` | The values the drop-downs offer, the game's effects and the control bindings |
| `launcher/process/portQueries.h/cpp` | `launcher_query.py` and `bb-gpu-capabilities` queries |
| `launcher/themeManager.h/cpp` | Dark and light palettes + `style_dark.qss` / `style_light.qss` |
| `scripts/launcher_query.py` | The launcher's questions to the port's Python helpers, answered as JSON |
| `packaging/arch/packages.txt` | Every Arch package the build and the game need |
| `packaging/appimage/build-appimage.sh` | The AppImage (linuxdeploy) |
| `version.json` | The version (`app_version`) and the bbport release bbpc is based on (`upstream_version`): the launcher, CMake, `run.sh` and the release tag read it |
| `gpu/shim/bb_env_vars.def` | Every `BB_*` variable the port reads, with what it does (`docs/ENVIRONMENT.md` is made from it) |

## Conventions

The conventions below apply to all of the project's own C and C++ code: the launcher
(`launcher/`), the port (`src/`, `gpu/`), `tests/` and `tools/`. Vendored code keeps its upstream
style: `third_party/`, `gpu/third_party/`, the online module's `gpu/bbnet/shadps4/`,
`gpu/bbnet20/vendor/`, `gpu/bbnet20/bbsc/` and `gpu/bbnet20/thd/`, and `tools/fsr4cap/ffx/`. Their
own `.clang-format` and `.clang-tidy` files turn the checks off there.

- **Formatting**: the root `.clang-format` lays out the port's code (run `clang-format -i` on the
  files you change, or let CLion do it); the launcher is formatted by hand, as the author's other
  Qt apps are. `.clang-tidy` flags the rules below that clang-tidy can check.
- **Hardware values in the emulation code**: register offsets, packet and bit-field layouts,
  PS4 error codes and NIDs may stay literal in `src/` and `gpu/` where the code around them says
  what they are (a register name, an `ASSERT`, a comment). Named constants are still preferred for
  anything that is a choice of the port (sizes, limits, timeouts, thresholds).

- **C++23**, strict conformance (`CMAKE_CXX_EXTENSIONS OFF` in the launcher), `#pragma once` everywhere. The port's C code is **C23** (the rules below apply to it too, with `nullptr` and `bool` as C23 has them).
- **No `.ui` files**: all layouts built programmatically in constructors
- **Singletons** via `static T& instance()`: `AppConfig`
- **Logging**: use `DBG_APP(msg)`, `DBG_GAME(msg)`, `DBG_HELPER(msg)`, `DBG_SETTINGS(msg)` (`launcher/debug.h`; stdout, tagged and timestamped). Never use `qDebug()`. The game's own output goes to the Log page.
- **Versioning**: single source of truth is `version.json` (key: `app_version`; `upstream_version` names the bbport release bbpc is based on); the launcher reads it at runtime via the embedded resource `:/version.json`, CMake reads it for the project version, `run.sh` logs both, and the release workflow tags `v<app_version>`. Never hardcode a version, and keep no other version file.
- **Environment variables**: the runtime and the GPU library read `BB_*` variables through `gpu/shim/bb_env.h` (`bb_env`, `bb_env_on`, `bb_env_off`, ...), not with `getenv` directly (the DLSS bridge and the online module, libraries of their own, use `getenv`); every variable, wherever it is read, is listed with a description in `gpu/shim/bb_env_vars.def`. After adding or changing one, run `python3 scripts/env_vars.py --doc` to regenerate `docs/ENVIRONMENT.md`; `tests/test_env_vars.py` fails on a variable read but not listed, listed but read nowhere, or a stale document.
- **Themes**: dark and light palettes are set in `themeManager.cpp`; all widget-specific styles go in `style_dark.qss` / `style_light.qss`. Never hardcode style strings in widget code.
- **Translations**: UI strings use `tr()` or `QCoreApplication::translate()`; static tables mark theirs with `QT_TRANSLATE_NOOP`.
- **Dependencies**: a new Arch package goes into `packaging/arch/packages.txt` (the CI installs from it). Header-only libraries get a pinned download fallback in CMake when they are not in Arch's official repositories.
- **Language**: American English only: variable names, comments, documentation and UI strings (e.g. `color` not `colour`, `canceled` not `cancelled`, `initialize` not `initialise`)
- **No em dashes**: never write the em dash (U+2014, including the `\u2014` escape) in UI strings, code comments, or documentation. Use a comma, colon, semicolon, parentheses, or a separate sentence instead; in code comments a spaced hyphen (` - `) is also fine.

### Code Style

- **No if-init syntax**: do not use `if (init; condition)`; declare the variable on a separate line before the `if`:
  ```cpp
  // OK
  QHBoxLayout* hl = qobject_cast<QHBoxLayout*>(layout());
  if (hl != nullptr) { ... }

  // Not OK
  if (auto* hl = qobject_cast<QHBoxLayout*>(layout()); hl != nullptr) { ... }
  ```
- **Constant placement**: declare `constexpr` constants above the function or class that uses them, not inside function bodies. For `.cpp` files use an anonymous namespace; for class-scope constants use `static constexpr` members. For header-only free functions where an anonymous namespace is inappropriate (Clang-Tidy warns), use a named inner namespace (e.g. `namespace Detail`) or promote them to class-scope `static constexpr` members if a class is nearby.
- **Magic numbers**: never use numeric literals inline; define named constants using `constexpr` (or `static constexpr` at class scope) with `UPPER_SNAKE_CASE` names:
  ```cpp
  // OK
  constexpr int SIDEBAR_WIDTH = 190;
  side->setFixedWidth(SIDEBAR_WIDTH);

  // Not OK
  side->setFixedWidth(190);
  ```
  String and boolean literals are exempt. Enumerators (which already have names) are also exempt. The literal `0` is also generally exempt when used as a neutral zero (e.g. empty margins, start indices, zero spacing). Only name it when `0` carries domain-specific meaning (e.g. "feature disabled" sentinel). Plain index arithmetic (`i + 1`, `size() - 1`, halving with `/ 2`) is fine.
- **Brace style**: GNU/Allman (opening brace on its own line for functions, classes, and control structures)
- **QSS brace style**: `.qss` files use GNU/Allman too: the opening brace goes on its own line after the selector (or the last selector of a list)
- **`auto`**: avoid for simple/obvious types; use explicit types (e.g. `int count = 0;`, `QString name = ...`). `auto` is acceptable where the type is verbose or deduced from a template (e.g. lambdas, iterators, structured bindings)
- **Loop bodies**: ALL loops (`for`, `while`) must use curly braces. No single-line unbraced loops, no exceptions
- **No `do`/`while` loops**: use a `while` loop instead
- **`switch` case bodies**: `case` labels are indented one level inside the `switch` block; the body always starts on the next line after the label:
  ```cpp
  // OK
  switch (kind)
  {
      case Icons::Kind::Refresh:
          return { "view-refresh", QStyle::SP_BrowserReload };

      default:
          break;
  }

  // Not OK
  case Icons::Kind::Refresh: return { "view-refresh", QStyle::SP_BrowserReload };
  ```
- **Boolean negation**: use `== false` instead of `!` in conditions: `if (ok == false)` not `if (!ok)`. Likewise prefer `== true` when it improves clarity over a bare identifier.
- **Pointer null checks**: always use `== nullptr` or `!= nullptr` explicitly; never rely on implicit pointer-to-bool conversion (`if (ptr)` or `if (!ptr)`). The same goes for `std::function`.
- **Condition bodies**: `if`/`else` bodies must use curly braces **unless** the body is a bare `return;` (void), `return true;`/`return false;` (boolean), `break;`, or `continue;`, in which case the body may appear on the same line as the condition without braces. Everything else (including assignments, function calls, and any other return expression) must use curly braces:
  ```cpp
  // OK: bare void/boolean return, break, or continue, same line
  if (ok == false) return;
  if (found == false) return false;
  if (done) break;
  if (skip) continue;

  // Not OK: must use braces
  if (x) doSomething();                  // function call
  if (x) return m_value;                 // non-boolean return expression
  ```

## Testing

Launcher tests live in `launcher/tests/` and use **Qt Test** (`QtTest/QtTest`). Each test file maps to one logical unit; the naming convention is `tst_<unit>.cpp`. They link the launcher's core library (`bbpc_launcher_core`) and are built with the `debug` and `launcher` presets (or `BBPC_BUILD_TESTS=ON`); run them with `ctest --preset launcher` or `ctest --test-dir out --output-on-failure`.

**When to write tests:** write a test for any class/function that has pure or near-pure logic: parsers, settings, `bbport.ini` handling, the game's environment, utility functions. Do **not** try to test `QWidget` subclasses or process-driven code; those are integration-level and are not tested here.

**How to register a new test:**

1. Create `launcher/tests/tst_<unit>.cpp`.
2. Add it to `launcher/tests/CMakeLists.txt` with the existing `add_qt_test` macro (the core library is linked by default):
   ```cmake
   add_qt_test(tst_myunit tst_myunit.cpp)
   ```
3. If the unit needs extra Qt modules (e.g. `Qt6::Gui`), add them with a separate `target_link_libraries` call after `add_qt_test`.

**Test structure**: one `QObject` subclass per file, test slots in `private slots:`, `QTEST_MAIN` + `.moc` include at the bottom:

```cpp
#include <QtTest/QtTest>

#include "myunit.h"

class TstMyUnit : public QObject
{
    Q_OBJECT

private slots:
    void methodName_condition_expectedResult()
    {
        QCOMPARE(MyUnit::doThing("input"), QStringLiteral("expected"));
    }
};

QTEST_MAIN(TstMyUnit)
#include "tst_myunit.moc"
```

**Naming:** `methodName_condition_expectedResult` (e.g. `saveChanges_inGameEdit_isKeptWhenTheLauncherDidNotTouchTheKey`).

The port's own tests stay as they are: the runtime's C tests (`tests/test_*.c`), the GPU library's tests (`tests/test_*.cpp`) and the Python tests (`tests/test_*.py`, `python3 -m unittest discover -s tests`). CTest runs all of them. When the launcher relies on a new answer of `scripts/launcher_query.py`, cover it in `tests/test_launcher_query.py`.

## CI and Releases

`.github/workflows/ci.yml` builds and tests every pull request to `main` or `dev` in an `archlinux` container (with `vulkan-swrast` for the tests that create a Vulkan instance) and builds the AppImage. `.github/workflows/release.yml` runs on every push to `main`: it builds the native tarball and the AppImage and publishes them as release `v<app_version>`, so bump `version.json` before merging a release into `main`.
