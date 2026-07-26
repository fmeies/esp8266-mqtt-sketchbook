#!/usr/bin/env bash
#
# Compiles every sketch against the real ESP8266 toolchain.
#
# This catches what tests/test_user_config.sh cannot: the host compiler is far
# newer than the one the ESP8266 core ships, so preprocessor features the
# target does not have still work there.
#
#   ./tests/compile_all.sh

set -u

readonly REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly FQBN="esp8266:esp8266:nodemcuv2"
readonly LOCAL_CONFIG="$REPO_ROOT/libraries/AllSketches/user_config_override.h"
readonly SAMPLE_CONFIG="$REPO_ROOT/libraries/AllSketches/user_config_override_sample.h"

# The sketches include <AllSketches.h> and the vendored third-party libraries
# from this repo, so arduino-cli has to treat it as the sketchbook rather than
# whatever directories.user points at.
export ARDUINO_DIRECTORIES_USER="$REPO_ROOT"

sketches_ok=0
sketches_failed=0
failed_names=""

require() {
	local what="$1" hint="$2"
	printf 'Missing: %s\n' "$what" >&2
	printf '  %s\n' "$hint" >&2
	exit 1
}

command -v arduino-cli >/dev/null 2>&1 ||
	require "arduino-cli" "Install it, e.g.: brew install arduino-cli"

arduino-cli core list 2>/dev/null | grep -q "^esp8266:esp8266" ||
	require "the esp8266 core" "Install it with: arduino-cli core install esp8266:esp8266"

# Without it every sketch stops at the #include in user_config.h. CI should
# copy the sample over before calling this script.
[ -f "$LOCAL_CONFIG" ] ||
	require "user_config_override.h" "Create it with: cp '$SAMPLE_CONFIG' '$LOCAL_CONFIG'"

echo "Compiling for $FQBN"

for sketch_dir in "$REPO_ROOT"/*/; do
	sketch_name="$(basename "$sketch_dir")"
	[ -f "$sketch_dir/$sketch_name.ino" ] || continue

	if output="$(arduino-cli compile --fqbn "$FQBN" "$sketch_dir" 2>&1)"; then
		printf '  ok    %s\n' "$sketch_name"
		sketches_ok=$((sketches_ok + 1))
	else
		printf '  FAIL  %s\n' "$sketch_name"
		echo "$output" | grep -E "error:" | sed 's/^/          /'
		sketches_failed=$((sketches_failed + 1))
		failed_names="$failed_names $sketch_name"
	fi
done

echo
if [ "$sketches_failed" -eq 0 ]; then
	echo "$sketches_ok sketches, all compiled."
	exit 0
fi
echo "$((sketches_ok + sketches_failed)) sketches, $sketches_failed failed:$failed_names"
exit 1
