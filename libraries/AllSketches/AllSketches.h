#define USE_SERIAL

#ifdef USE_SERIAL
	#define SERIAL_PRINT( value ) Serial.println( value )
	#define SERIAL_BEGIN( value ) Serial.begin( value )
	#define SERIAL_PRINTF( w, x, y, z ) Serial.printf( w, x, y, z )
#else
	#define SERIAL_PRINT( value )
	#define SERIAL_BEGIN( value )
	#define SERIAL_PRINTF( w, x, y, z )
#endif

#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include <PubSubClient.h>

const String CHIP_ID(ESP.getChipId());

const char* CLIENT_ID = CHIP_ID.c_str();
const char* BROKER_USER = MQTT_BROKER_USER;
const char* BROKER_PASS = MQTT_BROKER_PASS;
const String BROKER_TOPIC_NODE = String(BROKER_TOPIC) + "/" + CHIP_ID;
const String BROKER_TOPIC_CONF = String(BROKER_TOPIC) + "_conf";
const String BROKER_TOPIC_CONF_NODE = String(BROKER_TOPIC) + "_conf/" + CHIP_ID;
const String BROKER_TOPIC_UPDATE = String(BROKER_TOPIC) + "_update";
const String BROKER_TOPIC_UPDATE_NODE = String(BROKER_TOPIC) + "_update/" + CHIP_ID;
String MAC_ADDRESS = "";

const char* UPDATE_IP = UPDATE_HOST;

boolean wifi_start() {
	if (WiFi.status() == WL_CONNECTED) {
		SERIAL_PRINT(String("WiFi already connected to ") + SSID + "...");
		return true;
	}

	SERIAL_PRINT(String("Connecting WiFi to ") + SSID + "...");

	WiFi.mode(WIFI_STA);
	WiFi.begin(SSID, PASS);

	boolean ret = false;

	// Wait for connection
	for(int i = 0; i < 15; ++i) {
		if(WiFi.status() == WL_CONNECTED)
		{
			ret = true;
			break;
		}

		delay(1000);
		SERIAL_PRINT(".");
	}

	if(ret)
		SERIAL_PRINT(String("WiFi connection to ") + SSID + " esablished.");
	else
		SERIAL_PRINT(String("WiFi connection to ") + SSID + " could not be esablished.");

	MAC_ADDRESS = WiFi.macAddress();

	return ret;
}

void wifi_stop() {
	WiFi.mode(WIFI_OFF);
}


boolean mqtt_connect(PubSubClient& psclient, boolean subscribe) {
	if (!psclient.connected()) {
		SERIAL_PRINT(String("psclient not connected. Connecting to Server ") +	MQTT_SERVER + " as client " + CLIENT_ID);
		if(psclient.connect(CLIENT_ID, BROKER_USER, BROKER_PASS)) {
			SERIAL_PRINT("Connection to MQTT-Server established!");
			if(subscribe) {
				SERIAL_PRINT(String("Subscribing to Topic ") + BROKER_TOPIC_CONF.c_str());
				SERIAL_PRINT(String("Subscribing to Topic ") + BROKER_TOPIC_CONF_NODE.c_str());
				SERIAL_PRINT(String("Subscribing to Topic ") + BROKER_TOPIC_UPDATE.c_str());
				SERIAL_PRINT(String("Subscribing to Topic ") + BROKER_TOPIC_UPDATE_NODE.c_str());
				psclient.subscribe(BROKER_TOPIC_CONF.c_str());
				psclient.subscribe(BROKER_TOPIC_CONF_NODE.c_str());
				psclient.subscribe(BROKER_TOPIC_UPDATE.c_str());
				psclient.subscribe(BROKER_TOPIC_UPDATE_NODE.c_str());
				delay(200);
			}
		} else {
			SERIAL_PRINT(String("Connection to MQTT-Server ") + MQTT_SERVER + " failed!");
			return false;
		}
	} else {
		SERIAL_PRINT(String("psclient already connected to ") +	MQTT_SERVER + " as client " + CLIENT_ID);
	}
	return true;
}

void mqtt_loop(PubSubClient& psclient) {
	for(int i = 0; i < 5; ++i) {
		psclient.loop();
		delay(50);
	}
}

void serial_init() {
	//SERIAL_BEGIN(74880);
	SERIAL_BEGIN(115200);
	SERIAL_PRINT("Starting setup ...");
	delay(200);
}

void callback(char* topic, byte* payload, unsigned int length) {
	// create character buffer with ending null terminator (string)
	char message_buff[100];
	int i;
	for(i=0; i<length; i++) {
		message_buff[i] = payload[i];
	}
	message_buff[i] = '\0';
	String value(message_buff);

	SERIAL_PRINT("Message arrived in topic: ");
	SERIAL_PRINT(topic);
	SERIAL_PRINT(String("Message: ") + message_buff);

	if(strcmp(BROKER_TOPIC_CONF.c_str(), topic) == 0 || strcmp(BROKER_TOPIC_CONF_NODE.c_str(), topic) == 0) {
		SERIAL_PRINT("Adjusting configuration.");
		SERIAL_PRINT("-----------------------");
		sketch_callback(value);
	}

	if (strcmp(BROKER_TOPIC_UPDATE.c_str(), topic) == 0 || strcmp(BROKER_TOPIC_UPDATE_NODE.c_str(), topic) == 0) {
		SERIAL_PRINT("Checking for firmware update...");
		int updateAvailable = value.length() > 0;
		if(updateAvailable > 0) {
			SERIAL_PRINT("Updating firmware.");
			SERIAL_PRINT("-----------------------");
			t_httpUpdate_return ret = ESPhttpUpdate.update(UPDATE_IP, UPDATE_PORT, value);

			switch(ret) {
				case HTTP_UPDATE_FAILED:
					SERIAL_PRINTF("HTTP_UPDATE_FAILED Error (%d): %s %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str(), value.c_str());
					break;
				case HTTP_UPDATE_NO_UPDATES:
					SERIAL_PRINT("HTTP_UPDATE_NO_UPDATES");
					break;
				case HTTP_UPDATE_OK:
					SERIAL_PRINT("HTTP_UPDATE_OK");
					break;
			}
		} else {
			SERIAL_PRINT("-----------------------");
		}
	}
}

WiFiClient* create_wifi_client() {
	SERIAL_PRINT("Creating WiFiClient.");
	return new WiFiClient();
}
