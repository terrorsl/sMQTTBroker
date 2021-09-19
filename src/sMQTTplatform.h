#ifndef SMQTTPLATFORM_FILE
#define SMQTTPLATFORM_FILE

#if defined(WIN32)
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#if defined(DEBUG_ESP_PORT)
#define SMQTT_LOGD(...) DEBUG_ESP_PORT.printf(__VA_ARGS__)
#else
#define SMQTT_LOGD(...)
#endif
#elif defined(ESP32)
#include<WiFi.h>
#define TCPClient WiFiClient
#define TCPServer WiFiServer
static const char *SMQTTTAG = "sMQTTBroker";
#define SMQTT_LOGD(...) ESP_LOGD(SMQTTTAG,__VA_ARGS__)
#else
#error "unknown platform"
#endif

#endif