#ifndef SMQTT_ESP8266_FILE
#define SMQTT_ESP8266_FILE

#include <ESP8266WiFi.h>

typedef WiFiClient sMQTTOSClient;

class sMQTTESP8266Server : public sMQTTOSServer, public WiFiServer
{
public:
	sMQTTESP8266Server(unsigned short port) :WiFiServer(port)
	{
	};
	sMQTTOSClient *available() {
		return &WiFiServer::available();
	};
	bool isConnected() {
		return WiFi.isConnected();
	};
	void begin() {
		WiFiServer::begin();
	};
	void end() {
		WiFiServer::end();
	};
};

#if defined(DEBUG_ESP_PORT)
#define SMQTT_LOGD(...) DEBUG_ESP_PORT.printf(__VA_ARGS__)
#else
#define SMQTT_LOGD(...)
#endif

sMQTTServer *newsMQTTServer(unsigned short port) {
	return new sMQTTESP8266Server(port);
}
#endif