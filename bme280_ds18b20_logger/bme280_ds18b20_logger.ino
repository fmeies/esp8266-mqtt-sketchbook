// Weather node with two sensors: a BME280 for temperature, humidity and
// pressure, plus a DS18B20 on its own bus. Publishes via MQTT, then deep
// sleeps. Sleeps indefinitely once the supply voltage drops too low.
//
// Skeleton derived from:
// http://vaasa.hacklab.fi
// https://gist.github.com/jeje/57091acf138a92c4176a
// https://github.com/milesburton/Arduino-Temperature-Control-Library


#include <DallasTemperature.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

const char* SKETCH_VERSION = "1.4";

const char* BROKER_TOPIC = "weatherplus";

const int MIN_SLEEP_TIME = 900;
int sleepTime = MIN_SLEEP_TIME;

void sketch_callback(String value) {
  sleepTime = value.toInt();
  if (sleepTime < MIN_SLEEP_TIME) {
    sleepTime = MIN_SLEEP_TIME;
  }
}

const int WIRE_BUS_SCL        = D1;  // (=GPIO5) for NodeMCU
const int WIRE_BUS_SDA        = D2;  // (=GPIO4) for NodeMCU
const int SENSOR_POWER_PIN_1  = D5;  // (=GPIO14) for NodeMCU
const int SENSOR_ADDRESS      = 0x76;

const int ONE_WIRE_BUS        = D3;  // (=GPIO0) for NodeMCU
const int SENSOR_POWER_PIN_2  = D6;  // (=GPIO12) for NodeMCU

#include <MyWiFi.h>
#include <AllSketches.h>

#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme; // I2C

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

ADC_MODE(ADC_VCC);

// Temporary variables
static char celsiusTemp[10];
static char humidityTemp[10];
static char pressureTemp[10];

void setup(void) {

  //------ SENSOR 1

  // switch on sensor 1
  pinMode(SENSOR_POWER_PIN_1, OUTPUT);
  digitalWrite(SENSOR_POWER_PIN_1, HIGH);

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

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(p)) {
    SERIAL_PRINT("Failed to read from BME sensor!");
    strcpy(celsiusTemp, "\"Failed\"");
    strcpy(humidityTemp, "\"Failed\"");
    strcpy(pressureTemp, "\"Failed\"");
  } else {
    dtostrf(t, 5, 1, celsiusTemp);
    dtostrf(h, 5, 1, humidityTemp);
    dtostrf(p, 6, 1, pressureTemp);
  }

  // switch off sensor 1
  digitalWrite(SENSOR_POWER_PIN_1, LOW);

  //------ SENSOR 2

  // switch on sensor 2
  pinMode(SENSOR_POWER_PIN_2, OUTPUT);
  digitalWrite(SENSOR_POWER_PIN_2, HIGH);

  DS18B20.begin();

  float temperature = getTemperature();

  // switch off sensor 2
  digitalWrite(SENSOR_POWER_PIN_2, LOW);

  float volt = ESP.getVcc();

  SERIAL_PRINT(String("Read temp 1: ") + celsiusTemp);
  SERIAL_PRINT(String("Read temp 2: ") + temperature);
  SERIAL_PRINT(String("Read hum: ") + humidityTemp);
  SERIAL_PRINT(String("Read press: ") + pressureTemp);
  SERIAL_PRINT(String("Read volt: ") + volt);

  if(wifi_start()) {
    WiFiClient* client = create_wifi_client();
    PubSubClient psclient(MQTT_SERVER, MQTT_PORT, callback, *client);

    if(mqtt_connect(psclient, true)) {  
      String message = String("{\"temp1\":") + celsiusTemp + ",\"temp2\":" + temperature + ",\"humidity\":" + humidityTemp + ",\"pressure\":" + pressureTemp + ",\"volt\":" + volt + ",\"version\":\"" + SKETCH_VERSION + "\", \"chip\":\"" + CHIP_ID + "\"}";
      SERIAL_PRINT(String("Publishing to ") + MQTT_SERVER + ":" + MQTT_PORT + "/" + BROKER_TOPIC_NODE + ":" + message);
      psclient.publish(BROKER_TOPIC_NODE.c_str(), message.c_str());
      mqtt_loop(psclient);
      psclient.disconnect();
      delay(200);
    }
  }

  WiFi.disconnect();

  // low voltage => sleep forever
  if(volt < 3200.0) {
    sleepTime = 0;
  }
  
  SERIAL_PRINT(String("Going to sleep for ") + sleepTime + " seconds ...");
  ESP.deepSleep(sleepTime * 1000000);

  delay(100);
}

float getTemperature() {
  float temp;
  int i = 0;
  do {
    DS18B20.requestTemperatures();
    temp = DS18B20.getTempCByIndex(0);
    delay(1000);
    ++i;
  } while (i < 10 && (temp == 85.0 || temp == (-127.0)));
  
  return temp;
}

void loop() {
}
