# System Monitor Desktop

A C++20 and Qt 6 desktop app for monitoring system performance.

## Features

- CPU, RAM, swap, and disk summary cards with live gauges
- CPU area chart, per-core bars, and separate RAM/swap charts
- Chart zoom (1m/5m/30m/All) and hover values; up to 30 minutes retained in memory
- Memory usage by app, with native macOS icons

## Requirements

macOS, Xcode Command Line Tools, CMake 3.21+, and Qt 6 Core and Widgets.

## Normal Run (Debug)

Run from the project root. Qt is installed outside the build folder; change its path if needed.

```sh
MONITOR_QT_PREFIX="$HOME/.local/share/system-monitor-qt/6.8.3/macos"

cmake -S . -B build/debug \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH="$MONITOR_QT_PREFIX"

cmake --build build/debug --config Debug --parallel
open build/debug/SystemMonitor.app
```

## Release Run

```sh
MONITOR_QT_PREFIX="$HOME/.local/share/system-monitor-qt/6.8.3/macos"

cmake -S . -B build/release \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$MONITOR_QT_PREFIX"

cmake --build build/release --config Release --parallel
open build/release/SystemMonitor.app
```

## Clean Rebuild

To remove the entire build folder, run from the project root:

```sh
rm -rf build
```

The external Qt SDK is preserved. Configure again using the commands above before rebuilding.

To rebuild without removing the SDK:

```sh
# Debug
cmake --build build/debug --config Debug --clean-first --parallel

# Release
cmake --build build/release --config Release --clean-first --parallel
```

## Create DMG

After building Release, use the same Qt path:

```sh
"$MONITOR_QT_PREFIX/bin/macdeployqt" build/release/SystemMonitor.app -dmg -codesign=-
```

Output: `build/release/SystemMonitor.dmg`. Qt is bundled; Apple notarization is not included.

## Tests

```sh
ctest --test-dir build/debug -C Debug --output-on-failure
ctest --test-dir build/release -C Release --output-on-failure
```
