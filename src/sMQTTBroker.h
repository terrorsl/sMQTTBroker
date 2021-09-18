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

	void publish(sMQTTTopic *topic, sMQTTMessage *msg);

	bool subscribe_topic(sMQTTClient *client, const char *topic);
	void unsubscribe_topic(sMQTTClient *client, const char *topic);

	bool isTopicValidName(const char *filter);
	void updateRetainedTopic(sMQTTTopic *topic);
private:
	void findRetainTopic(sMQTTTopic *topic, sMQTTClient *client);

	TCPServer *_server;
	sMQTTClientList clients;
	sMQTTTopicList subscribes, retains;
};
#endif