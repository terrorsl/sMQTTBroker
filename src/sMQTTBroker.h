/*!
Main class
*/
#ifndef SMQTTBROKER_FILE
#define SMQTTBROKER_FILE

#include"sMQTTplatform.h"
#include"sMQTTClient.h"
#include"sMQTTTopic.h"
#include"sMQTTEvent.h"

class sMQTTBroker
{
public:
	sMQTTBroker(unsigned char _version);
	//! setup broker
	bool init(unsigned short port, bool checkWifiConnection=false);
	//! call in loop function
	void update();
	//! publish message
	void publish(const std::string &topic, const std::string &payload,unsigned char qos=0,bool retain=false);
	//! restart WIFI server
	void restart();
	//! receive event from broker
	virtual bool onEvent(sMQTTEvent *event) = 0;

	// inner function
	void publish(sMQTTClient *client, sMQTTTopic *topic, sMQTTMessage *msg);

	bool subscribe(sMQTTClient *client, const char *topic);
	void unsubscribe(sMQTTClient *client, const char *topic);

	bool isTopicValidName(const char *filter);
	void updateRetainedTopic(sMQTTTopic *topic);

	bool isClientConnected(sMQTTClient *client);
protected:
	unsigned char version;
private:
	void findRetainTopic(sMQTTTopic *topic, sMQTTClient *client);

	TCPServer *_server;
	sMQTTClientList clients;
	sMQTTTopicList subscribes, retains;
	bool isCheckWifiConnection;
};

class sMQTTSimpleBroker final: public sMQTTBroker
{
public:
	sMQTTSimpleBroker(unsigned char version):sMQTTBroker(version){}
	bool onEvent(sMQTTEvent *event) override {return true;}	
};
#endif