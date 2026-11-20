# System Monitor Desktop

A C++20 and Qt 6 desktop app by SEM Bunly.

## Features

- Live CPU, RAM, and disk monitoring with charts
- Memory usage by app, with native macOS icons
- Searchable and sortable process list
- Adjustable refresh, pause, and saved preferences

## Requirements

macOS, Xcode Command Line Tools, CMake 3.21+, and Qt 6 Core and Widgets.

## Build & Run Release

Run from the project root. This workspace has Qt at the path below; change it if your Qt installation is elsewhere.

```sh
MONITOR_QT_PREFIX="$PWD/build/qt-sdk/6.8.3/macos"

cmake -S . -B build/release \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$MONITOR_QT_PREFIX"

cmake --build build/release --config Release --parallel
open build/release/SystemMonitor.app
```

## Create DMG

After building Release, use the same Qt path:

```sh
"$MONITOR_QT_PREFIX/bin/macdeployqt" build/release/SystemMonitor.app -dmg -codesign=-
```

Output: `build/release/SystemMonitor.dmg`. Qt is bundled; Apple notarization is not included.

## Tests

```sh
ctest --test-dir build/release -C Release --output-on-failure
```
