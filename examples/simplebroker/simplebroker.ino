#include"sMQTTBroker.h"

sMQTTBroker broker;

void setup()
{
    Serial.begin(115200);
    const char* ssid = "SSID";         // The SSID (name) of the Wi-Fi network you want to connect to
    const char* password = "PASSWORD"; // The password of the Wi-Fi network
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
        delay(1000);
    }
    Serial.println("Connection established!");  
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());
    
    const unsigned short mqttPort=1883;
    broker.init(mqttPort);
    // all done
}
void loop()
{
    broker.update();
}
