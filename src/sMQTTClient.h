#ifndef SMQTTCLIENT_FILE
#define SMQTTCLIENT_FILE

#include<sMQTTMessage.h>

class sMQTTBroker;

#define sMQTTUserNameFlag 0x80
#define sMQTTPasswordFlag 0x40
#define sMQTTWillRetainFlag 0x20
#define sMQTTWillQoSFlag 0x18
#define sMQTTWillFlag 0x4

class sMQTTClient
{
public:
	sMQTTClient(sMQTTBroker *parent, TCPClient *client);

	void update();
	
	bool isConnected();
	void write(const char* buf, size_t length);

	const std::string &getClientId() {
		return clientId;
	};
private:
	void processMessage();

	char mqtt_flags;
	bool mqtt_connected;
	std::string clientId;
	unsigned short keepAlive;

	sMQTTBroker *_parent;
	TCPClient *_client;
	sMQTTMessage message;
};

typedef std::vector<sMQTTClient*> sMQTTClientList;
#endif