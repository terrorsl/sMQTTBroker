#include"sMQTTBroker.h"

sMQTTBroker broker;

void setup()
{
    broker.init(1883);
}
void loop()
{
    broker.update();
}
