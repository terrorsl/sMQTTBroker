#include"sMQTTBroker.h"

sMQTTBroker broker;

void setup()
{
    broker.init();
}
void loop()
{
    broker.update();
}
