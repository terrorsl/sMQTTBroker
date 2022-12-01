#ifndef SMQTT_ESP32_FILE
#define SMQTT_ESP32_FILE

#include <WiFi.h>

typedef WiFiClient sMQTTOSClient;

class sMQTTESP32Server : public sMQTTOSServer, public WiFiServer
{
public:
	sMQTTESP32Server(unsigned short port) :WiFiServer(port)
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

static const char *SMQTTTAG = "sMQTTBroker";
#define SMQTT_LOGD(...) ESP_LOGD(SMQTTTAG,__VA_ARGS__)

sMQTTServer *newsMQTTServer(unsigned short port) {
	return new sMQTTESP32Server(port);
}
#endif