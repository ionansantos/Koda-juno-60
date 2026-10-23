#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
task_build_dir="${TMPDIR:-/tmp}/tal-un-no-60-check"
mkdir -p "$task_build_dir"
clang++ -std=c++17 -O1 -Wall -Wextra -Werror -fsanitize=address,undefined -I Source \
    Source/DSP/SynthDSP.cpp tests/dsp_test.cpp -o "$task_build_dir/dsp_test"
"$task_build_dir/dsp_test"
clang++ -std=c++17 -O1 -Wall -Wextra -Werror -fsanitize=address,undefined -I Source \
    Source/DSP/SynthDSP.cpp tests/sustain_test.cpp -o "$task_build_dir/sustain_test"
"$task_build_dir/sustain_test"
clang++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I Source \
    tests/preset_test.cpp -o "$task_build_dir/preset_test"
"$task_build_dir/preset_test" evidence/factory-bank.bin
python3 evidence/parameters/recover-parameter-names.py
