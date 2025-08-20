# Repository Guidelines

## Project Structure & Module Organization
- `main/`: app entry (`main.cpp`), `Kconfig.projbuild`, component manifest.
- `components/`: reusable modules (e.g., `ble_sync/`, `display_manager/`, `gui/`, `sensors/`, `settings/`, `nimble-nordic-uart/`, `XPowersLib/`, `bsp_extra/`). Each keeps `CMakeLists.txt`, `idf_component.yml`, `include/`, and sources. Public headers live in `components/<name>/include/`.
- `managed_components/`: auto-fetched ESP-IDF dependencies.
- Root: `CMakeLists.txt`, `sdkconfig`/`sdkconfig.defaults`, `partitions.csv`, `.devcontainer/`, `.vscode/`. Build output goes to `build/`.

## Build, Test, and Development Commands
- `idf.py set-target esp32s3`: set chip target (run once per workspace).
- `idf.py menuconfig`: configure features and board options.
- `idf.py build`: compile and fetch managed components.
- `idf.py -p COMx flash monitor`: flash and open serial (Linux/macOS: `/dev/ttyUSB0`).
- `idf.py clean` / `idf.py fullclean`: clear build cache/artifacts.
- Tip: Use the Dev Container or ESP-IDF 5.x locally and ensure `IDF_PATH` is set.

## Coding Style & Naming Conventions
- Language: C/C++; indent 4 spaces; UTF-8; LF line endings.
- Files: C modules `snake_case.c/.h`; C++ classes in `.cpp/.hpp` named `PascalCase`.
- Names: functions/vars `snake_case`; classes/types `PascalCase`; macros/consts `UPPER_SNAKE_CASE`.
- Formatting: prefer `clang-format` (project style if present). Include order: local, component, ESP-IDF, standard.
- APIs: keep small and documented; expose only via `components/<name>/include/`.

## Testing Guidelines
- Framework: ESP-IDF Unity test app. Place tests under `components/<name>/test/`.
- Run all: `idf.py test`; filter: `idf.py test -T <pattern>`.
- Prefer hardware-independent tests; gate HW-dependent cases behind Kconfig options.

## Commit & Pull Request Guidelines
- Commits: small, focused; subject ≤ ~72 chars. Prefer Conventional Commits (`feat:`, `fix:`, `docs:`, `refactor:`, `chore:`).
- Branches: `feature/...`, `fix/...`, `chore/...`.
- PRs: clear description, linked issues, key logs, and screenshots/GIFs for UI (LVGL) changes. Note any `sdkconfig`/partition impacts and migration steps.

## Security & Configuration Tips
- Never commit secrets (Wi‑Fi creds, tokens). Prefer `sdkconfig.defaults` and runtime settings via the `settings` component.
- Keep `sdkconfig.defaults` authoritative; avoid noisy local `sdkconfig` diffs unless intentional.

## Architecture Notes
- Modular ESP-IDF components; GUI via LVGL; BLE via NimBLE Nordic UART; sensors/power via `XPowersLib`; board specifics in `bsp_extra/`.

