#ifndef SMQTTCLIENT_FILE
#define SMQTTCLIENT_FILE

#include<sMQTTMessage.h>

class sMQTTBroker;

class sMQTTClient
{
public:
	sMQTTClient(sMQTTBroker *parent, TCPClient *client);

	void update();
	
	bool isConnected();
	void write(const char* buf, size_t length);
private:
	void processMessage();

	char mqtt_flags;
	sMQTTBroker *_parent;
	TCPClient *_client;
	sMQTTMessage message;
};

typedef std::vector<sMQTTClient*> sMQTTClientList;
#endif