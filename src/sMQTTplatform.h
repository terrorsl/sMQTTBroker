#ifndef SMQTTPLATFORM_FILE
#define SMQTTPLATFORM_FILE

#define SMQTT_DEPRECATED(msg) [[deprecated(msg)]]

#if defined(SMQTT_USER_SOCKET)
// create and write your client and server
#include"smqtt_user_socket.h"
#elif defined(WIN32)
class TCPClient {
public:
	bool available() { return false; };
	char read() { return 0; }
	bool connected() {
		return false;
	}
	void stop() {}
	void write(const char *, int) {}
};
class TCPServer {
public:
	TCPServer(short) {}
	void begin() {}
};
#define SMQTT_LOGD
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#define TCPClient WiFiClient
#define TCPServer WiFiServer
#if defined(DEBUG_ESP_PORT)
#define SMQTT_LOGD(...) DEBUG_ESP_PORT.printf(__VA_ARGS__)
#else
#define SMQTT_LOGD(...)
#endif
#elif defined(ESP32)
#include <WiFi.h>
#ifdef SMQTT_WT32_ETH01
#include <ETH.h>
#endif
#define TCPClient WiFiClient
#define TCPServer WiFiServer
static const char *SMQTTTAG = "sMQTTBroker";
#define SMQTT_LOGD(...) ESP_LOGD(SMQTTTAG,__VA_ARGS__)
#elif defined(WIO_TERMINAL)
#include <rpcWiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#define TCPClient WiFiClient
#define TCPServer WiFiServer
#define SMQTT_LOGD(...)
#else
#error "unknown platform"
#endif

#endif