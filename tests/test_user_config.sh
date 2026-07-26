#!/usr/bin/env bash
#
# Tests the override mechanism of libraries/AllSketches/user_config.h.
#
# This is pure preprocessor logic, so a host compiler is enough -- the
# ESP8266 toolchain is not needed.
#
#   ./tests/test_user_config.sh

set -u

readonly REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
readonly CONFIG_HEADER="$REPO_ROOT/libraries/AllSketches/user_config.h"
readonly SAMPLE_OVERRIDE="$REPO_ROOT/libraries/AllSketches/user_config_override_sample.h"
readonly CXX_STANDARD="c++11"

tests_run=0
tests_failed=0

# Builds an isolated directory holding user_config.h and an optional override,
# so that __has_include cannot pick up the user's real local configuration.
make_sandbox() {
	local sandbox
	sandbox="$(mktemp -d)"
	cp "$CONFIG_HEADER" "$sandbox/"
	echo "$sandbox"
}

compile_sandbox() {
	local sandbox="$1"
	cat > "$sandbox/main.cpp" <<-'EOF'
		#include "user_config.h"
		int main() { return 0; }
	EOF
	g++ -std=$CXX_STANDARD -fsyntax-only "$sandbox/main.cpp" 2>&1
}

report() {
	local name="$1" passed="$2" detail="${3:-}"
	tests_run=$((tests_run + 1))
	if [ "$passed" = "yes" ]; then
		printf '  ok   %s\n' "$name"
	else
		tests_failed=$((tests_failed + 1))
		printf '  FAIL %s\n' "$name"
		[ -n "$detail" ] && printf '       %s\n' "$detail"
	fi
}

test_missing_override_fails_loudly() {
	local sandbox output
	sandbox="$(make_sandbox)"
	output="$(compile_sandbox "$sandbox")"

	if echo "$output" | grep -q "user_config_override.h"; then
		report "missing override fails, naming the missing file" yes
	else
		report "missing override fails, naming the missing file" no "$output"
	fi
	rm -rf "$sandbox"
}

test_incomplete_override_fails_loudly() {
	local sandbox output
	sandbox="$(make_sandbox)"
	cat > "$sandbox/user_config_override.h" <<-'EOF'
		#define WIFI_SSID "TestNet"
		#define MQTT_HOST "broker.test"
		#define MQTT_BROKER_USER "tester"
		#define MQTT_BROKER_PASS "brokersecret"
	EOF
	output="$(compile_sandbox "$sandbox")"

	if echo "$output" | grep -q "WIFI_SSID and WIFI_PASS"; then
		report "incomplete override fails (WIFI_PASS missing)" yes
	else
		report "incomplete override fails (WIFI_PASS missing)" no "$output"
	fi
	rm -rf "$sandbox"
}

test_missing_broker_credentials_fail_loudly() {
	local sandbox output
	sandbox="$(make_sandbox)"
	cat > "$sandbox/user_config_override.h" <<-'EOF'
		#define WIFI_SSID "TestNet"
		#define WIFI_PASS "wifipassword"
		#define MQTT_HOST "broker.test"
	EOF
	output="$(compile_sandbox "$sandbox")"

	if echo "$output" | grep -q "MQTT_BROKER_USER"; then
		report "missing broker credentials fail the build" yes
	else
		report "missing broker credentials fail the build" no "$output"
	fi
	rm -rf "$sandbox"
}

test_complete_override_wins_and_defaults_fill_gaps() {
	local sandbox output
	sandbox="$(make_sandbox)"
	cat > "$sandbox/user_config_override.h" <<-'EOF'
		#define WIFI_SSID "TestNet"
		#define WIFI_PASS "wifipassword"
		#define MQTT_HOST "broker.test"
		#define MQTT_BROKER_USER "tester"
		#define MQTT_BROKER_PASS "brokersecret"
		#define MQTT_PORT_PLAIN 1884
	EOF
	cat > "$sandbox/main.cpp" <<-'EOF'
		#include "user_config.h"

		constexpr bool str_eq(const char* a, const char* b) {
			return *a == *b && (*a == '\0' || str_eq(a + 1, b + 1));
		}

		static_assert(str_eq(WIFI_SSID, "TestNet"), "override sets WIFI_SSID");
		static_assert(str_eq(WIFI_PASS, "wifipassword"), "override sets WIFI_PASS");
		static_assert(str_eq(MQTT_BROKER_PASS, "brokersecret"), "override sets MQTT_BROKER_PASS");
		static_assert(MQTT_PORT_PLAIN == 1884, "override beats default");
		static_assert(UPDATE_PORT == 80, "default fills the gap");
		static_assert(str_eq(UPDATE_HOST, MQTT_HOST), "UPDATE_HOST falls back to MQTT_HOST");

		int main() { return 0; }
	EOF
	output="$(g++ -std=$CXX_STANDARD -fsyntax-only "$sandbox/main.cpp" 2>&1)"

	if [ -z "$output" ]; then
		report "complete override wins, defaults fill the gaps" yes
	else
		report "complete override wins, defaults fill the gaps" no "$output"
	fi
	rm -rf "$sandbox"
}

test_sample_override_is_complete() {
	local sandbox output
	sandbox="$(make_sandbox)"
	cp "$SAMPLE_OVERRIDE" "$sandbox/user_config_override.h"
	output="$(compile_sandbox "$sandbox")"

	if [ -z "$output" ]; then
		report "sample template covers every required value" yes
	else
		report "sample template covers every required value" no "$output"
	fi
	rm -rf "$sandbox"
}

test_sample_override_has_no_real_credentials() {
	local leaked
	leaked="$(grep -E '^\s*#define\s+(WIFI_PASS|MQTT_BROKER_PASS|WIFI_SSID|MQTT_BROKER_USER|MQTT_HOST)\s' \
		"$SAMPLE_OVERRIDE" | grep -v '<TODO_INSERT>')"

	if [ -z "$leaked" ]; then
		report "sample template holds placeholders only" yes
	else
		report "sample template holds placeholders only" no "$leaked"
	fi
}

echo "user_config override mechanism"
test_missing_override_fails_loudly
test_incomplete_override_fails_loudly
test_missing_broker_credentials_fail_loudly
test_complete_override_wins_and_defaults_fill_gaps
test_sample_override_is_complete
test_sample_override_has_no_real_credentials

echo
if [ "$tests_failed" -eq 0 ]; then
	echo "$tests_run tests, all passed."
	exit 0
fi
echo "$tests_run tests, $tests_failed failed."
exit 1
