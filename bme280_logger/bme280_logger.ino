// Weather node: reads temperature, humidity and barometric pressure from a
// BME280, publishes them via MQTT and deep sleeps until the next reading.
//
// Skeleton derived from:
// http://vaasa.hacklab.fi
// https://gist.github.com/jeje/57091acf138a92c4176a

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

const char* SKETCH_VERSION = "1.2.4";

const char* BROKER_TOPIC = "weather";

const int MIN_SLEEP_TIME = 900;
int sleepTime = MIN_SLEEP_TIME;

void sketch_callback(String value) {
  sleepTime = value.toInt();
  if (sleepTime < MIN_SLEEP_TIME) {
    sleepTime = MIN_SLEEP_TIME;
  }
}

const int WIRE_BUS_SDA = D2;      // D2 (=GPIO4) for NodeMCU
const int WIRE_BUS_SCL = D1;      // D1 (=GPIO5) for NodeMCU
const int SENSOR_POWER_PIN = 14;  // 14 (=GPIO14) for NodeMCU
const int SENSOR_ADDRESS = 0x76;

#include <MyWiFi.h>
#include <AllSketches.h>

#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme; // I2C

ADC_MODE(ADC_VCC);

// Temporary variables
static char celsiusTemp[10];
static char humidityTemp[10];
static char pressureTemp[10];

void setup(void) {

  // switch on sensor
  pinMode(SENSOR_POWER_PIN, OUTPUT);
  digitalWrite(SENSOR_POWER_PIN, HIGH);

  serial_init();

  Wire.begin(WIRE_BUS_SDA, WIRE_BUS_SCL);
  Wire.setClock(100000);

  if (bme.begin(SENSOR_ADDRESS)) {
    SERIAL_PRINT("BME280 sensor ready.");
  } else {
    // switch off sensor
    SERIAL_PRINT("Could not find BME280 sensor!");
  }

  // give it some time to settle
  SERIAL_PRINT("Waiting 2 seconds...");
  delay(2000);

  float h = bme.readHumidity(); // %
  float t = bme.readTemperature(); // °C
  float p = bme.readPressure() / 100.0F; // hPa

  // switch off sensor
  digitalWrite(SENSOR_POWER_PIN, LOW);

  float volt = ESP.getVcc();

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(p)) {
    SERIAL_PRINT("Failed to read from BME280 sensor!");
    strcpy(celsiusTemp, "\"Failed\"");
    strcpy(humidityTemp, "\"Failed\"");
    strcpy(pressureTemp, "\"Failed\"");
  } else {
    dtostrf(t, 5, 1, celsiusTemp);
    dtostrf(h, 5, 1, humidityTemp);
    dtostrf(p, 6, 1, pressureTemp);
  }

  SERIAL_PRINT(String("Read temp: ") + celsiusTemp);
  SERIAL_PRINT(String("Read hum: ") + humidityTemp);
  SERIAL_PRINT(String("Read press: ") + pressureTemp);
  SERIAL_PRINT(String("Read volt: ") + volt);

  if(wifi_start()) {
    WiFiClient* client = create_wifi_client();
    PubSubClient psclient(MQTT_SERVER, MQTT_PORT, callback, *client);

    if(mqtt_connect(psclient, true)) {  
      StringSumHelper message = String("{\"temp\":") + celsiusTemp  + ","
              + "\"humidity\":" + humidityTemp + ","
              + "\"pressure\":" + pressureTemp + ","
              + "\"volt\":" + volt + ","
              + "\"version\":\"" + SKETCH_VERSION + "\","
              + "\"chip\":\"" + CHIP_ID  + "\","
              + "\"mac\":\"" + MAC_ADDRESS  + "\""
              + "}";
      SERIAL_PRINT(String("Publishing to ") + MQTT_SERVER + ":" + MQTT_PORT + "/" + BROKER_TOPIC_NODE + ":" + message);
      psclient.publish(BROKER_TOPIC_NODE.c_str(), message.c_str());
      mqtt_loop(psclient);
      psclient.disconnect();
      delay(200);
    }
  }

  WiFi.disconnect();

  SERIAL_PRINT(String("Going to sleep for ") + sleepTime + " seconds ...");
  ESP.deepSleep(sleepTime * 1000000);
  delay(100);
}

void loop() {
}
