#include <PubSubClient.h>

const char* SKETCH_VERSION = "1.0";

const char* BROKER_TOPIC = "nodemcu_test";
const int LED1_PIN  = D0;  // (=GPIO16)
const int LED2_PIN  = D4;  // (=GPIO2)
unsigned int STATUS_REPORT_FREQUENCY = 10;

unsigned int loopCount = STATUS_REPORT_FREQUENCY;

void sketch_callback(String stringValue) {
  Serial.println(String("Received configuration value ") + stringValue);
}

#include <MyWiFi.h>
#include <AllSketches.h>

ADC_MODE(ADC_VCC);

WiFiClient* client = create_wifi_client();
PubSubClient psclient(MQTT_SERVER, MQTT_PORT, callback, *client);

void reportStatus(PubSubClient& psclient) {
  float volt = ESP.getVcc();
  int value = 0;
  StringSumHelper message = String("{\"command\":\"") + value + "\", \"volt\":" + volt + ",\"version\":\"" + SKETCH_VERSION + "\", \"chip\":\"" + CHIP_ID + "\"}";
  psclient.publish(BROKER_TOPIC_NODE.c_str(), message.c_str());
  SERIAL_PRINT(String("Published data"));
}

void setup(void) {
  pinMode(LED1_PIN, OUTPUT);  
  pinMode(LED2_PIN, OUTPUT);  

  serial_init();
  SERIAL_PRINT("SETUP COMPLETE");
  delay(50);
}

void loop() {

  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  
  if(wifi_start()) {
    if (mqtt_connect(psclient, true)) {
      mqtt_loop(psclient);
      delay(20);        
      loopCount++;
      if(loopCount > STATUS_REPORT_FREQUENCY) {
        SERIAL_PRINT("sending regular status message");
        reportStatus(psclient);
        loopCount = 0;
      }
    }
  }

  digitalWrite(LED1_PIN, HIGH);
  digitalWrite(LED2_PIN, HIGH);

  delay(1000);

}
