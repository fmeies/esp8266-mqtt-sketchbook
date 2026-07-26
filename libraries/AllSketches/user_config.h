#ifndef USER_CONFIG_H
#define USER_CONFIG_H

// Central configuration for all sketches.
//
// This file holds no credentials and is version controlled. Real values belong
// in user_config_override.h, which .gitignore excludes so it cannot be
// published by accident.
//
// Setup:
//   cp user_config_override_sample.h user_config_override.h
//   then fill in your own values there.
//
// If the compiler reports that the include below cannot be found, that setup
// step is missing.

// Included unconditionally on purpose. Wrapping this in __has_include would
// read nicer but silently does the wrong thing: the ESP8266 core ships GCC
// 4.8.2, which does not know __has_include and evaluates it to false, so the
// override would be skipped and every value would fall back to its default.
//
// The override comes before the defaults so that the #ifndef blocks below only
// fill in what it left open. That way the override needs no #undef and cannot
// be overwritten by accident.
#include "user_config_override.h"

// Required values, deliberately without defaults. A missing one fails the
// build instead of letting the device silently fail to connect later.
#if !defined(WIFI_SSID) || !defined(WIFI_PASS)
	#error "WIFI_SSID and WIFI_PASS must be set in user_config_override.h."
#endif

#if !defined(MQTT_HOST) || !defined(MQTT_BROKER_USER) || !defined(MQTT_BROKER_PASS)
	#error "MQTT_HOST, MQTT_BROKER_USER and MQTT_BROKER_PASS must be set in user_config_override.h."
#endif

// Optional values with sensible defaults.
#ifndef MQTT_PORT_PLAIN
	#define MQTT_PORT_PLAIN 1883
#endif

// Host the sketches pull OTA firmware updates from.
#ifndef UPDATE_HOST
	#define UPDATE_HOST MQTT_HOST
#endif

#ifndef UPDATE_PORT
	#define UPDATE_PORT 80
#endif

#endif  // USER_CONFIG_H
