#include <PubSubClient.h>

const char* SKETCH_VERSION = "1.2.5";

const char* BROKER_TOPIC = "watermeter";
const int IN_PIN = 12;      // (=GPIO12)
const int STATUS_PIN = 13;  // (=GPIO13)
unsigned int STATUS_REPORT_FREQUENCY = 600;

int actual = 1;
int prev = 1;
long int value = -1;
unsigned int loopCount = STATUS_REPORT_FREQUENCY;

void sketch_callback(String stringValue) {
  Serial.println(String("Received configuration value ") + stringValue);
  int intValue = stringValue.toInt();
  Serial.println(String("Received configuration int value ") + intValue);
  value = intValue;
}

#include <MyWiFi.h>
#include <AllSketches.h>

ADC_MODE(ADC_VCC);

WiFiClient* client = create_wifi_client();
PubSubClient psclient(MQTT_SERVER, MQTT_PORT, callback, *client);

void reportStatus(PubSubClient& psclient, int value) {
  if(wifi_start()) {
    if (mqtt_connect(psclient, false)) {
      float volt = ESP.getVcc();
      StringSumHelper message = String("{\"counter\":") + value  + ","
              + "\"volt\":" + volt + ","
              + "\"version\":\"" + SKETCH_VERSION + "\","
              + "\"chip\":\"" + CHIP_ID  + "\","
              + "\"mac\":\"" + MAC_ADDRESS  + "\""
              + "}";
      psclient.publish(BROKER_TOPIC_NODE.c_str(), message.c_str());
      psclient.publish(BROKER_TOPIC_CONF_NODE.c_str(), String(value).c_str(), true);
      SERIAL_PRINT(String("Published value ") + value);
      psclient.disconnect();
      delay(200);
    }
  }
}

void setup() {

  pinMode(IN_PIN, INPUT_PULLUP);
  pinMode(STATUS_PIN, OUTPUT);

  serial_init();

  // give it some time to settle
  SERIAL_PRINT("Waiting 2 seconds...");
  delay(2000);

  //WiFi.setSleepMode(WIFI_LIGHT_SLEEP); // -> does not work with core 2.4.2, freezes device
  // try to read offset value from channel on startup, do not start without
  while(value == -1) {
    if(wifi_start()) {
      if(mqtt_connect(psclient, true)) {
        mqtt_loop(psclient);
      }
    }

    delay(1000);
  }

  psclient.disconnect();
  delay(200);

  SERIAL_PRINT("SETUP COMPLETE");

  delay(50);
}

void loop() {

  actual = digitalRead(IN_PIN);

  SERIAL_PRINT(String("Digital read is ") + actual);

  if (actual == 0 && actual != prev) {
    value++;
    SERIAL_PRINT(String("Tick recognized. Current value is ") + value);
    reportStatus(psclient, value);
  }

  if (actual == 0) {
    digitalWrite(STATUS_PIN, HIGH);
  } else {
    digitalWrite(STATUS_PIN, HIGH);
    delay(10);
    digitalWrite(STATUS_PIN, LOW);
  }

  // report status every now and then
  loopCount++;
  if(loopCount > STATUS_REPORT_FREQUENCY) {
    SERIAL_PRINT("sending status");
    reportStatus(psclient, value);
    loopCount = 0;
  }

  prev = actual;
  SERIAL_PRINT("going to sleep now");
  delay(980);
 }
