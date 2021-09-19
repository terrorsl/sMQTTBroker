#include<sMQTTBroker.h>

class MyBroker:public sMQTTBroker
{
public:
    bool onConnect(sMQTTClient *client, const std::string &username, const std::string &password)
    {
        // check username and password, if ok return true
        return true;
    };
	void onRemove(sMQTTClient*)
    {
    };
};

MyBroker broker;

void setup()
{
    broker.init(1883);
};
void loop()
{
    broker.update();
};