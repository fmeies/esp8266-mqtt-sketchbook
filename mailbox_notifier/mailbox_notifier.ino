// Mailbox sensor: a delivery pulls the reset line, waking the node, which then
// reports via MQTT. The reset reason tells a real delivery (external reset)
// apart from the routine keep-alive after deep sleep.
//
// Skeleton derived from:
// http://vaasa.hacklab.fi
// https://gist.github.com/jeje/57091acf138a92c4176a

const char* SKETCH_VERSION = "1.0";

const char* BROKER_TOPIC = "mailbox";

const int MIN_SLEEP_TIME = 900;
int sleepTime = MIN_SLEEP_TIME;

void sketch_callback(String value) {
}

#include <MyWiFi.h>
#include <AllSketches.h>
 
ADC_MODE(ADC_VCC);

void setup(void) {

  serial_init();
  
  if (wifi_start()) {
    WiFiClient* client = create_wifi_client();
    PubSubClient psclient(MQTT_SERVER, MQTT_PORT, callback, *client);

    if (mqtt_connect(psclient, true)) {
      // 5 = DeepSleep
      // 6 = OnOff
      rst_info* rstInfo = ESP.getResetInfoPtr();
      int resetReasonInt = rstInfo->reason;
      int state = resetReasonInt == 5 ? 0 : 1;
      float volt = ESP.getVcc();
      StringSumHelper message = String("{\"mailbox\":") + state + ",\"reason\":\"" + resetReasonInt + "\"" + ",\"volt\":" + volt + ",\"version\":\"" + SKETCH_VERSION + "\", \"chip\":\"" + CHIP_ID + "\"}";
      SERIAL_PRINT(String("Publishing to ") + MQTT_SERVER + ":" + MQTT_PORT + "/" + BROKER_TOPIC_NODE + ":" + message);
      psclient.publish(BROKER_TOPIC_NODE.c_str(), message.c_str());
      mqtt_loop(psclient);
      psclient.disconnect();
      delay(500);
    }
  }

  WiFi.disconnect();

  SERIAL_PRINT(String("Going to sleep for ") + sleepTime + " seconds ...");
  ESP.deepSleep(sleepTime * 1000000);
  delay(100);
}

void loop() {
}
