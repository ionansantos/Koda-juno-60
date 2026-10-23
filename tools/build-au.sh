#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
task_cmake=".tools/cmake-4.4.4-macos-universal/CMake.app/Contents/bin/cmake"
if [ ! -x "$task_cmake" ]; then task_cmake="$(command -v cmake)"; fi
"$task_cmake" -S . -B build -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 -DBUILD_TESTING=ON
"$task_cmake" --build build --config Release --target TalUNo60Clone_AU tal_tests --parallel 4
task_ctest="$(dirname "$task_cmake")/ctest"
"$task_ctest" --test-dir build --output-on-failure
task_component="build/TalUNo60Clone_artefacts/Release/AU/KODA-JUNO-60.component"
codesign --force --sign - "$task_component"
codesign --verify --deep --strict "$task_component"
if [ "${1:-}" = "--install" ]; then
    mkdir -p "$HOME/Library/Audio/Plug-Ins/Components"
    task_previous="$HOME/Library/Audio/Plug-Ins/Components/U-No-60 Study.component"
    if [ -d "$task_previous" ]; then
        task_previous_id="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$task_previous/Contents/Info.plist")"
        if [ "$task_previous_id" = "audio.independent.taluno60clone" ]; then
            task_backup="$PWD/dist/legacy-au-$(date +%Y%m%d-%H%M%S)"
            mkdir -p "$task_backup"
            mv "$task_previous" "$task_backup/"
        fi
    fi
    ditto "$task_component" "$HOME/Library/Audio/Plug-Ins/Components/KODA-JUNO-60.component"
    printf '%s\n' "Instalado: $HOME/Library/Audio/Plug-Ins/Components/KODA-JUNO-60.component"
fi
