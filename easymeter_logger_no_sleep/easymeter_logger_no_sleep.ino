/*
 * Electricity meter reader (D0 / SML over an optical probe), always-on variant
 *
 * Same job as easymeter_logger, but stays awake instead of deep sleeping.
 * Mains-powered nodes lose their serial port occasionally, so this version
 * re-opens it after repeated empty reads and reboots the ESP if that does not
 * help. Credentials come from user_config_override.h.
 *
 * Wiring: the probe's phototransistor pulls RxD low, so meter data arrives on
 * the hardware serial port. TxD stays free for debug output, which means a USB
 * console can stay attached while the sketch runs.
 *
 * The SML reading routine originates from Frank Carius, original version
 * dated 2016-05-01:
 * https://www.msxfaq.de/sonst/bastelbude/smartmeter_d0_sml.code.htm
 * with portions from http://www.esp8266.com/viewtopic.php?f=29&t=2222
 */

#include <PubSubClient.h>

const char* SKETCH_VERSION = "1.0.2";

const char* BROKER_TOPIC = "easymeter";

const int MIN_SLEEP_TIME = 60;
int sleepTime = MIN_SLEEP_TIME;

int errorCount = 0;
int serialResetCount = 0;

void sketch_callback(String value) {
  sleepTime = value.toInt();
  if (sleepTime < MIN_SLEEP_TIME) {
    sleepTime = MIN_SLEEP_TIME;
  }
}

#include <MyWiFi.h>
#include <AllSketches.h>

ADC_MODE(ADC_VCC);

WiFiClient* client = create_wifi_client();
PubSubClient psclient(MQTT_SERVER, MQTT_PORT, callback, *client);

void reportStatus(PubSubClient& psclient, const String &data) {
  if(wifi_start()) {
    if(mqtt_connect(psclient, true)) {  
      float volt = ESP.getVcc();
      StringSumHelper message = String("{\"data\":\"") + data + "\","
              + "\"volt\":" + volt + ","
              + "\"version\":\"" + SKETCH_VERSION + "\","
              + "\"chip\":\"" + CHIP_ID  + "\","
              + "\"mac\":\"" + MAC_ADDRESS  + "\""
              + "}";
      SERIAL_PRINT(String("Publishing to ") + MQTT_SERVER + ":" + MQTT_PORT + "/" + BROKER_TOPIC_NODE + ":" + message);
      psclient.setBufferSize(512);
      psclient.publish(BROKER_TOPIC_NODE.c_str(), message.c_str());
      psclient.disconnect();
      delay(200);
    }
  }
}

void setup() {

  SERIAL_PRINT("Init Serial ...");

  Serial.begin(9600);
  while (!Serial) {
    ;
  }

  SERIAL_PRINT("Serial init finished.");
  delay(2000);

  if(wifi_start()) {
    if(mqtt_connect(psclient, true)) {
      mqtt_loop(psclient);
    }
  
    delay(1000);
  }

  psclient.disconnect();
  delay(200);
  SERIAL_PRINT("SETUP COMPLETE");

  delay(50);
}

// A full SML datagram stays well below this; oversized reads are rejected.
byte datagram[500];

void loop() {

  // We may have started mid-datagram, so throw away whatever is still in
  // flight. The pause between bursts is what tells us a boundary was reached.
  SERIAL_PRINT("C");
  while (Serial.available()) {
    while (Serial.available()) {
      Serial.read();
    }
    delay(10);  // roughly ten byte times at 9600 baud
  }

  // The line is quiet now. The next byte to arrive starts a fresh datagram;
  // give up after a second so a dead probe cannot stall the loop.
  SERIAL_PRINT("W");
  int count = 0;
  while (!Serial.available() && count < 200) {
    delay(5);
    ++count;
  }

  // Pull in the whole burst at once. The timeout ends the read when the meter
  // stops sending rather than when the buffer happens to be full.
  SERIAL_PRINT("R");
  Serial.setTimeout(500);
  int serindex = Serial.available() ? Serial.readBytes(datagram, 500) : 0;

  StringSumHelper data = String("");

  // Anything shorter is a truncated burst, anything longer overran the buffer.
  if (serindex > 200 && serindex < 500) {
    SERIAL_PRINT(String("Datagram received. Total Bytes: ") + serindex);

    delay(1000);

    for (int i = 0; i < serindex ; i++) {
      // The probe leaves the parity bit set on some bytes; clearing it
      // recovers the actual payload value.
      int d = datagram[i];
      if (d > 128) {
        d = d - 128;
      }
      data += char(d);
    }

    errorCount = 0;
    serialResetCount = 0;
  } else {
    data += String("error count=") + errorCount + ", received bytes=" + serindex + ", free heap=" + ESP.getFreeHeap();
    errorCount++;
  }

  // A silent port usually recovers from being re-opened.
  if (errorCount > 5) {
    SERIAL_PRINT("Reinitializing Serial...");
    Serial.end();
    delay(100);
    Serial.begin(9600);
    errorCount = 0;
    serialResetCount++;
    data += String(" - resetting serial,  serial reset count=") + serialResetCount;
  }

  // Reopening did not help often enough. Report first, reboot afterwards.
  if (serialResetCount > 3) {
    SERIAL_PRINT(" - restarting esp ...");
  }

  float volt = ESP.getVcc();

  SERIAL_PRINT(String("Datagram received: ") + data);
  SERIAL_PRINT(String("Read volt: ") + volt);

  reportStatus(psclient, data);

  if (serialResetCount > 3) {
    ESP.restart();
  }

  delay(sleepTime * 1000);
}
