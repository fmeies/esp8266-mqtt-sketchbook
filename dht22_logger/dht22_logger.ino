// Humidity node: reads temperature and humidity from a DHT22, publishes them
// via MQTT and deep sleeps until the next reading. The sensor is only powered
// for the duration of the measurement.
//
// Skeleton derived from:
// http://vaasa.hacklab.fi
// https://gist.github.com/jeje/57091acf138a92c4176a

// WARNING: 3V is not enough for the DHT22, humidity reads far too high.

#include <DHT.h>

const char* SKETCH_VERSION = "1.6";

const char* BROKER_TOPIC = "humidity";

const int MIN_SLEEP_TIME = 900;
int sleepTime = MIN_SLEEP_TIME;

void sketch_callback(String value) {
  sleepTime = value.toInt();
  if (sleepTime < MIN_SLEEP_TIME) {
    sleepTime = MIN_SLEEP_TIME;
  }
}

const int DHT_PIN = D1;           // D1 (=GPIO5) for NodeMCU,  2 (=GPIO2) for ESP-01
const int SENSOR_POWER_PIN = 14;  // 14 (=GPIO14) for NodeMCU, 0 (=GPIO0) for ESP-01

#include <MyWiFi.h>
#include <AllSketches.h>

// Initialize DHT sensor.
const int DHT_TYPE = DHT22;
DHT dht(DHT_PIN, DHT_TYPE);

ADC_MODE(ADC_VCC);

// Temporary variables
static char celsiusTemp[10];
static char humidityTemp[10];

void setup(void) {

  // switch on sensor
  pinMode(SENSOR_POWER_PIN, OUTPUT);
  digitalWrite(SENSOR_POWER_PIN, HIGH);

  serial_init();

  dht.begin();

  // give it some time to settle
  SERIAL_PRINT("Waiting 2 seconds...");
  delay(2000);

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // switch off sensor
  digitalWrite(SENSOR_POWER_PIN, LOW);

  float volt = ESP.getVcc();

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    SERIAL_PRINT("Failed to read from DHT sensor!");
    strcpy(celsiusTemp, "\"Failed\"");
    strcpy(humidityTemp, "\"Failed\"");
  } else {
    // Computes temperature values in Celsius + Fahrenheit and Humidity
    float hic = dht.computeHeatIndex(t, h, false);
    dtostrf(hic, 6, 2, celsiusTemp);
    dtostrf(h, 6, 2, humidityTemp);
  }

  SERIAL_PRINT(String("Read temp: ") + celsiusTemp);
  SERIAL_PRINT(String("Read hum: ") + humidityTemp);
  SERIAL_PRINT(String("Read volt: ") + volt);

  if(wifi_start()) {
    WiFiClient* client = create_wifi_client();
    PubSubClient psclient(MQTT_SERVER, MQTT_PORT, callback, *client);

    if (mqtt_connect(psclient, true)) {
      StringSumHelper message = String("{\"temp\":") + celsiusTemp + ",\"humidity\":" + humidityTemp + ",\"volt\":" + volt + ",\"version\":\"" + SKETCH_VERSION + "\", \"chip\":\"" + CHIP_ID + "\"}";
      SERIAL_PRINT(String("Publishing to ") + MQTT_SERVER + ":" + MQTT_PORT + "/" + BROKER_TOPIC_NODE + ":" + message);
      psclient.publish(BROKER_TOPIC_NODE.c_str(), message.c_str());
      mqtt_loop(psclient);
      psclient.disconnect();
      delay(200);
    }

    WiFi.disconnect();
  }
  
  SERIAL_PRINT(String("Going to sleep for ") + sleepTime + " seconds ...");
  ESP.deepSleep(sleepTime * 1000000);
  delay(100);
}

void loop() {
}
