#ifndef USER_CONFIG_OVERRIDE_H
#define USER_CONFIG_OVERRIDE_H

// Template for the local configuration.
//
//   cp user_config_override_sample.h user_config_override.h
//
// user_config_override.h is excluded by .gitignore and may hold real
// credentials. This template here is version controlled, so never put real
// values in it.
//
// No #undef needed: user_config.h reads this file first and only fills in
// afterwards what is missing here.

// --- Required ------------------------------------------------------------

#define WIFI_SSID          "<TODO_INSERT>"
#define WIFI_PASS          "<TODO_INSERT>"

#define MQTT_HOST          "<TODO_INSERT>"
#define MQTT_BROKER_USER   "<TODO_INSERT>"
#define MQTT_BROKER_PASS   "<TODO_INSERT>"

// --- Optional: only set these to deviate from the defaults ----------------

// #define MQTT_PORT_PLAIN 1883

// Defaults to MQTT_HOST.
// #define UPDATE_HOST     "<TODO_INSERT>"
// #define UPDATE_PORT     80

#endif  // USER_CONFIG_OVERRIDE_H
