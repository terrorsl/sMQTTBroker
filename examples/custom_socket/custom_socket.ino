#include"sMQTTBroker.h"

sMQTTBroker broker;

class MyServer: public sMQTTOSServer
{
public:
    MyServer(unsigned short port){}
    void begin() { /*init server, lister port etc*/}
    void end(){/*stop server*/}
    sMQTTOSClient *available(){return 0;}
    bool isConnected(){return true;}
};
class MyClient: public sMQTTOSClient
{
public:
    int available(){return 0;}
	unsigned char read(){return 0;}
	void write(const char *buffer, unsigned long size){}
	void stop(){}
	bool connected(){return true;}
};
sMQTTOSServer *newsMQTTServer(unsigned short port) {
	return new MyServer(port);
}
void setup()
{
    const unsigned short mqttPort=1883;
    sMQTTOSServer *server = newsMQTTServer(mqttPort);
    broker.init(server);
    // all done*/
}
void loop()
{
    broker.update();
}
