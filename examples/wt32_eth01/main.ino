#include<sMQTTBroker.h>
#include <ETH.h>

class MyBroker:public sMQTTBroker
{
public:
    bool onEvent(sMQTTEvent *event)
    {
        switch(event->Type())
        {
        case NewClient_sMQTTEventType:
            {
                sMQTTNewClientEvent *e=(sMQTTNewClientEvent*)event;
                e->Login();
                e->Password();
            }
            break;
        case Public_sMQTTEventType:
            {
                sMQTTPublicClientEvent *e=(sMQTTPublicClientEvent*)event;
                e->Topic();
                e->Payload();
            }
            break;
        case RemoveClient_sMQTTEventType:
            {
                sMQTTRemoveClientEvent *e=(sMQTTRemoveClientEvent*)event;
                e->Client();
            }
            break;
        case LostConnect_sMQTTEventType:
            WiFi.reconnect();
            break;
        }
        return true;
    }
};

#define ETH_ADDR 1
#define ETH_POWER_PIN 16//-1 //16 // Do not use it, it can cause conflict during the software reset.
#define ETH_POWER_PIN_ALTERNATIVE 16 //17
#define ETH_MDC_PIN 23
#define ETH_MDIO_PIN 18
#define ETH_TYPE ETH_PHY_LAN8720
#define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT // ETH_CLOCK_GPIO0_IN

MyBroker broker;
unsigned long Time;
unsigned long freeRam;

void setup()
{
    Serial.begin(115200);
    
    pinMode(ETH_POWER_PIN_ALTERNATIVE, OUTPUT);
    digitalWrite(ETH_POWER_PIN_ALTERNATIVE, HIGH);

    ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE); // Enable ETH DHCP
    while(!((uint32_t)ETH.localIP())){}; // Waiting for IP (leave this line group to get IP via DHCP)

    Serial.println("Connection established!");  
    Serial.print("IP address:\t");
    Serial.println(ETH.localIP());
    
    const unsigned short mqttPort=1883;
    broker.init(mqttPort);
    // all done
    // your magic code
    Time=millis();
    freeRam=ESP.getFreeHeap();
};
void loop()
{
    broker.update();
    // your magic code
    if(millis()-Time>1000)
    {
      Time=millis();
      if(ESP.getFreeHeap()!=freeRam)
      {
        freeRam=ESP.getFreeHeap();
      Serial.print("RAM:");
      Serial.println(freeRam);
      }
    }
};
