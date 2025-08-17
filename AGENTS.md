# Repository Guidelines

## Project Structure & Module Organization
- `main/`: app entry (`main.cpp`), `Kconfig.projbuild`, component manifest.
- `components/`: reusable modules (e.g., `ble_sync/`, `display_manager/`, `gui/`, `sensors/`, `settings/`, `nimble-nordic-uart/`, `XPowersLib/`, `bsp_extra/`). Each keeps `CMakeLists.txt`, `idf_component.yml`, `include/`, and sources.
- `managed_components/`: auto-fetched ESP-IDF dependencies.
- Root: `CMakeLists.txt`, `sdkconfig`/`sdkconfig.defaults`, `partitions.csv`, `.devcontainer/`, `.vscode/`. Build output lives in `build/`.

## Build, Test, and Development Commands
- `idf.py set-target esp32s3`: set target (run once per workspace).
- `idf.py menuconfig`: configure features and board options.
- `idf.py build`: compile and fetch managed components.
- `idf.py -p COMx flash monitor` (Linux/macOS: `/dev/ttyUSB0`): flash firmware and open serial.
- `idf.py clean` / `idf.py fullclean`: clean build cache/artifacts.
- Tip: Use the provided Dev Container or install ESP-IDF 5.x; ensure `IDF_PATH` is set.

## Coding Style & Naming Conventions
- Language: C/C++. Indent 4 spaces; UTF-8; LF line endings.
- Files: C modules `snake_case.c/.h`; C++ classes `PascalCase` in `.cpp/.hpp` where used.
- Names: functions/vars `snake_case`; classes/types `PascalCase`; macros/consts `UPPER_SNAKE_CASE`.
- Components expose public headers in `components/<name>/include/`; keep APIs small and documented.
- Prefer `clang-format` (project style if present). Group includes: local, component, ESP-IDF, standard.

## Testing Guidelines
- Current tree has no unit tests. Add Unity-based tests under `components/<name>/test/` following ESP-IDF’s unit test app.
- Run tests with `idf.py test` (from project root) or filter with `-T <pattern>`.
- Aim for small, hardware-isolated tests; gate hardware-dependent cases behind Kconfig options.

## Commit & Pull Request Guidelines
- Commits: small, focused, imperative subject (max ~72 chars). Prefer Conventional Commits (`feat:`, `fix:`, `docs:`, `refactor:`, `chore:`) for clarity.
- Branches: `feature/...`, `fix/...`, `chore/...`.
- PRs: clear description, linked issues, key logs, and screenshots/gifs for UI changes (LVGL screens). Note any `sdkconfig`/partition impacts and how to migrate.

## Security & Configuration Tips
- Do not commit secrets (Wi‑Fi creds, tokens). Prefer `sdkconfig.defaults` and runtime settings (`settings` component).
- Keep `sdkconfig.defaults` authoritative; avoid noisy local `sdkconfig` diffs unless intentional.
