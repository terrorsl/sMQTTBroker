#ifndef SMQTTBROKER_FILE
#define SMQTTBROKER_FILE

#include"sMQTTplatform.h"
#include"sMQTTClient.h"
#include"sMQTTTopic.h"

class sMQTTBroker
{
public:
	bool init(unsigned short port);
	void update();

	void publish(const std::string &topic, const std::string &payload);

	// inner function
	void publish(sMQTTClient *client, sMQTTTopic *topic, sMQTTMessage *msg);

	bool subscribe(sMQTTClient *client, const char *topic);
	void unsubscribe(sMQTTClient *client, const char *topic);

	bool isTopicValidName(const char *filter);
	void updateRetainedTopic(sMQTTTopic *topic);

	bool isClientConnected(sMQTTClient *client);

	virtual bool onConnect(sMQTTClient *client, const std::string &username, const std::string &password);
	virtual void onRemove(sMQTTClient*);

	virtual void onPublish(sMQTTClient *client, const std::string &topic, const std::string &payload) {};
private:
	void findRetainTopic(sMQTTTopic *topic, sMQTTClient *client);

	TCPServer *_server;
	sMQTTClientList clients;
	sMQTTTopicList subscribes, retains;
};
#endif