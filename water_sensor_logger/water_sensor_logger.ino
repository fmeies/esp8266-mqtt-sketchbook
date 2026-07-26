// S connected to A0

const char* SKETCH_VERSION = "1.2.3";

const char* BROKER_TOPIC = "water";

const int MIN_SLEEP_TIME = 900;
int sleepTime = MIN_SLEEP_TIME;

void sketch_callback(String value) {
    sleepTime = value.toInt();
    if(sleepTime < MIN_SLEEP_TIME) {
      sleepTime = MIN_SLEEP_TIME;
    }
}

const int SENSOR_POWER_PIN = 14; // 14 (=GPIO14) for NodeMCU

#include <MyWiFi.h>
#include <AllSketches.h>

void setup(void){

  // switch on sensor
  pinMode(SENSOR_POWER_PIN, OUTPUT);
  digitalWrite(SENSOR_POWER_PIN, HIGH);

  serial_init();

  // Reference point: a reading of 188 corresponds to 100% at 3V supply.
  const float water = analogRead(A0);

  // switch off sensor
  digitalWrite(SENSOR_POWER_PIN, LOW);

  float volt = ESP.getVcc(); 

  SERIAL_PRINT(String("Read water level: ") + water);
  SERIAL_PRINT(String("Read volt: ") + volt);

  if(wifi_start()) {
    WiFiClient* client = create_wifi_client();
    PubSubClient psclient(MQTT_SERVER, MQTT_PORT, callback, *client);

    if(mqtt_connect(psclient, true)) {  
      StringSumHelper message = String("{\"water\":") + water + ",\"volt\":" + volt + ",\"version\":\"" + SKETCH_VERSION + "\", \"chip\":\"" + CHIP_ID + "\"}";
      SERIAL_PRINT(String("Publishing to ") + BROKER_TOPIC_NODE + ":" + message);
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
