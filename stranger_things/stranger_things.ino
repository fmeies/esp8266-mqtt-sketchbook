#include <PubSubClient.h>

const char* SKETCH_VERSION = "1.3";

const char* BROKER_TOPIC = "stranger_things";
const int SWITCH_PIN  = 4;  // (=GPIO4, D2)
unsigned int STATUS_REPORT_FREQUENCY = 600;

int clicks = 0;
int lastCommand = 0;
unsigned int loopCount = STATUS_REPORT_FREQUENCY;

// Runs before AllSketches.h is included, so SERIAL_PRINT does not exist yet.
void sketch_callback(String stringValue) {
  Serial.println(String("Received configuration value ") + stringValue);
  int newCommand = stringValue.toInt();
  if(newCommand > 8) {
    return;
  }

  if(newCommand == 0) {
    clicks = (9 - lastCommand) % 9;
  } else if(newCommand > lastCommand) {
    clicks = newCommand - lastCommand;
  } else if (newCommand < lastCommand) {
    clicks = 9 - lastCommand + newCommand;
  }

  lastCommand = newCommand;
}

#include <MyWiFi.h>
#include <AllSketches.h>

ADC_MODE(ADC_VCC);

WiFiClient* client = create_wifi_client();
PubSubClient psclient(MQTT_SERVER, MQTT_PORT, callback, *client);

void reportStatus(PubSubClient& psclient, int value) {
  float volt = ESP.getVcc();
  StringSumHelper message = String("{\"command\":\"") + value + "\", \"volt\":" + volt + ",\"version\":\"" + SKETCH_VERSION + "\", \"chip\":\"" + CHIP_ID + "\"}";
  psclient.publish(BROKER_TOPIC_NODE.c_str(), message.c_str());
  SERIAL_PRINT(String("Published value ") + value);
}

void emulateButton(int times) {
  for(int i = 0; i < times; ++i) {
    pinMode(SWITCH_PIN, OUTPUT);
    digitalWrite(SWITCH_PIN, LOW);
    delay(100);
    pinMode(SWITCH_PIN, INPUT);
    delay(100);
  }
}

void setup(void) {
  pinMode(SWITCH_PIN, INPUT);

  serial_init();
  SERIAL_PRINT("SETUP COMPLETE");
  delay(50);
}

void loop() {

  if(wifi_start()) {
    if (mqtt_connect(psclient, true)) {
      mqtt_loop(psclient);
      delay(20);

      if(clicks != 0) {
        // send ack to server
        reportStatus(psclient, lastCommand);

        SERIAL_PRINT(String("Clicking ") + clicks + " times");
        emulateButton(clicks);
        clicks = 0;
      }
      
      loopCount++;
      if(loopCount > STATUS_REPORT_FREQUENCY) {
        SERIAL_PRINT("sending regular status message");
        reportStatus(psclient, lastCommand);
        loopCount = 0;
      }
    }
  }

  delay(250);
}
