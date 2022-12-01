#ifndef SMQTTPLATFORM_FILE
#define SMQTTPLATFORM_FILE

#include<sMQTTSocket.h>

#if defined(WIN32)
	#include<sMQTTWIN32.h>
#elif defined(__linux__)
#elif defined(ESP8266)
	#include <sMQTTESP8266.h>
#elif defined(ESP32)
	#include<sMQTTESP32.h>
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