#include"sMQTTBroker.h"
//Use my custom socket
#define sMQTT_SOCKET
#include <Ethernet.h>
//my own socket
#define TCPClient EthernetClient
#define TCPServer EthernetServer

#define ETH_SPI+_CS 10

byte mac[] = { 0xBE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

sMQTTBroker broker;

void setup()
{
    Serial.begin(115200);
    Ethernet.begin(mac);
        
    const unsigned short mqttPort=1883;
    broker.init(mqttPort);
    // all done
}
void loop()
{
    broker.update();
}
