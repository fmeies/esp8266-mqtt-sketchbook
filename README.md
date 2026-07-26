# ESP8266 MQTT Sketchbook

A collection of ESP8266 sketches for home sensing. Every node publishes its
readings as JSON over MQTT to an openHAB broker; the battery-powered ones deep
sleep afterwards.

The shared foundation lives in `libraries/AllSketches/` and handles the WiFi
connection, MQTT, runtime configuration over an MQTT topic and OTA firmware
updates.

## Setup

This directory is an Arduino sketchbook: point the Arduino IDE at it under
*Preferences → Sketchbook location*.

### 1. Create the credentials file

```bash
cp libraries/AllSketches/user_config_override_sample.h \
   libraries/AllSketches/user_config_override.h
```

Then fill in your own values in `user_config_override.h`. That file is excluded
by `.gitignore` and is the only place holding real credentials.

| Required value | Meaning |
|---|---|
| `WIFI_SSID` / `WIFI_PASS` | WiFi access |
| `MQTT_HOST` | Hostname or IP of the MQTT broker |
| `MQTT_BROKER_USER` / `MQTT_BROKER_PASS` | Broker credentials |

Optionally overridable: `MQTT_PORT_PLAIN` (1883), `UPDATE_HOST` (defaults to
`MQTT_HOST`), `UPDATE_PORT` (80).

If the file or any required value is missing, the build fails with a message
saying so. That is deliberate: it stops a device from being built with an
incomplete configuration.

### 2. Install the libraries

Third-party libraries are **not** part of this repository. Install them through
the Arduino IDE library manager. The versions listed are the ones last used —
the sketches have not been tested against newer APIs:

| Library | Version | Needed by |
|---|---|---|
| PubSubClient | 2.8 | every MQTT sketch |
| DallasTemperature | 3.9.0 | `bme280_ds18b20_logger` |
| OneWire | 2.3.6 | dependency of DallasTemperature |
| DHT sensor library | 1.4.3 | `dht22_logger`, `gasmeter_logger` |
| Adafruit BME280 Library | 2.2.2 | `bme280_logger`, `bme280_ds18b20_logger` |
| Adafruit Unified Sensor | 1.1.4 | dependency of Adafruit BME280 |
| Adafruit BusIO | 1.8.3 | dependency of Adafruit BME280 |

The ESP8266 board package is required on top of these.

## Sketches

### MQTT sensor nodes

| Sketch | MQTT topic | Purpose |
|---|---|---|
| `bme280_logger` | `weather` | Temperature, humidity, barometric pressure (BME280) |
| `bme280_ds18b20_logger` | `weatherplus` | BME280 plus an additional DS18B20 |
| `dht22_logger` | `humidity` | Temperature and humidity (DHT22) |
| `easymeter_logger` | `easymeter` | Electricity meter via SML over an optical probe, with deep sleep |
| `easymeter_logger_no_sleep` | `easymeter` | As above, always on, with recovery from serial failures |
| `gasmeter_logger` | `gasmeter` | Gas meter pulses (GPIO12) plus a DHT sensor |
| `watermeter_logger` | `watermeter` | Water meter pulses (GPIO12) |
| `water_sensor_logger` | `water` | Analog water detector on A0, powered only during the measurement |
| `mailbox_notifier` | `mailbox` | Mailbox delivery, reports state and battery voltage |
| `stranger_things` | `stranger_things` | Switch on GPIO4, reports command and voltage |
| `nodemcu_test` | `nodemcu_test` | Basic check for NodeMCU boards |

Every node also subscribes to `<topic>_conf` and `<topic>_update` (each with a
`/<chip-id>` suffix as well, to address a single device), which allows runtime
configuration and OTA updates respectively.

## Tests

```bash
./tests/test_user_config.sh
```

Checks the configuration override mechanism using the host compiler; the
ESP8266 toolchain is not needed for it.

```bash
./tests/compile_all.sh
```

Builds every sketch against `esp8266:esp8266:nodemcuv2`. Requires
`arduino-cli`, an installed ESP8266 core and an existing
`user_config_override.h`; if one of them is missing, the script says which.
This run is the only one that catches target toolchain problems — the host
compiler is considerably newer than the core's GCC 4.8.2 and supports
preprocessor features that are absent there.

## Security

MQTT runs unencrypted on port 1883. TLS is not supported: the nodes only ever
talk to the broker inside the home network.

OTA updates pull firmware over **unencrypted HTTP** and are triggered by an
MQTT message, without the firmware being signed. Anyone who can write to that
MQTT topic can execute arbitrary code on the nodes. That is acceptable on a
segregated home network; it would not be with a broker reachable from outside.
