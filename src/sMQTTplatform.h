#ifndef SMQTTPLATFORM_FILE
#define SMQTTPLATFORM_FILE

static const char *SMQTTTAG = "sMQTTBroker";

#if defined(WIN32)
#elif defined(ESP32)
#include<WiFi.h>
#define TCPClient WiFiClient
#define TCPServer WiFiServer
#define MSQTT_LOGD(...) ESP_LOGD(SMQTTTAG,__VA_ARGS__)
#else
#error "unknown platform"
#endif

#endif