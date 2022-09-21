#ifndef SMQTTCLIENT_FILE
#define SMQTTCLIENT_FILE

#include "sMQTTMessage.h"

class sMQTTBroker;

#define sMQTTUserNameFlag 0x80
#define sMQTTPasswordFlag 0x40
#define sMQTTWillRetainFlag 0x20
#define sMQTTWillQoSFlag 0x18
#define sMQTTWillFlag 0x4

#define sMQTTConnReturnAccepted 0x0
#define sMQTTConnReturnUnacceptableProtocolVersion 0x1
#define sMQTTConnReturnIdentifierRejected 0x2
#define sMQTTConnReturnServerUnavailable 0x3
#define sMQTTConnReturnBadUsernameOrPassword 0x4


class sMQTTClient
{
public:
	sMQTTClient(sMQTTBroker *parent, TCPClient &client);
	~sMQTTClient();

	void update();
	
	bool isConnected();
	void write(const char* buf, size_t length);

	const std::string &getClientId() {
		return clientId;
	};
private:
	void processMessage();
	void updateLiveStatus();

	char mqtt_flags;
	bool mqtt_connected;
	std::string clientId;
	unsigned short keepAlive;
	unsigned long aliveMillis;

	sMQTTBroker *_parent;
	TCPClient _client;
	sMQTTMessage message;
};

typedef std::vector<sMQTTClient*> sMQTTClientList;
#endif