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
	/*! setup broker
	\param[in] port mqtt connect port
	\param[in] checkWifiConnection - if True send event if connection lost
	\return True if no error
	*/
	bool init(unsigned short port, bool checkWifiConnection=false);
	//! call in loop function
	void update();
	/*! publish message
	\param[in] topic name topic
	\param[in] payload data
	\param[in] qos default 0
	\param[in] retain default False
	\return Nothing
	*/
	void publish(const std::string &topic, const std::string &payload,unsigned char qos=0,bool retain=false);
	//! restart WIFI server
	void restart();
	//! receive event from broker
	//! \param[in] event Some event from broker
	//! \return True - process, False - error
	virtual bool onEvent(sMQTTEvent *event) = 0;

	//! \internal
	void publish(sMQTTClient *client, sMQTTTopic *topic, sMQTTMessage *msg);

	bool subscribe(sMQTTClient *client, const char *topic);
	void unsubscribe(sMQTTClient *client, const char *topic);

	bool isTopicValidName(const char *filter);
	void updateRetainedTopic(sMQTTTopic *topic);

	bool isClientConnected(sMQTTClient *client);
	//! \endinternal
protected:
	unsigned char version;
private:
	void findRetainTopic(sMQTTTopic *topic, sMQTTClient *client);

	TCPServer *_server;
	sMQTTClientList clients;
	sMQTTTopicList subscribes, retains;
	bool isCheckWifiConnection;
};
//! Simple mqtt broker if you don't need receive event
class sMQTTSimpleBroker final: public sMQTTBroker
{
public:
	sMQTTSimpleBroker(unsigned char version):sMQTTBroker(version){}
	bool onEvent(sMQTTEvent *event) override {return true;}	
};
#endif