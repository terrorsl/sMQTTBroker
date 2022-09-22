#ifndef SMQTTEVENT_FILE
#define SMQTTEVENT_FILE

#include<string>

/*! Enum for event*/
enum sMQTTEventType
{
	NewClient_sMQTTEventType,//< event when new client connect
	RemoveClient_sMQTTEventType,//< client disconnect
	Public_sMQTTEventType,//< some client publish message
	LostConnect_sMQTTEventType,//< broker lost connection
	Subscribe_sMQTTEventType,//< client subscribe to topic
	UnSubscribe_sMQTTEventType//< client unsubscribe from topic
};
class sMQTTClient;
class sMQTTEvent
{
public:
	sMQTTEvent(unsigned char type, sMQTTClient *cl) :_type(type),_client(cl) {};

	unsigned char Type() {
		return _type;
	}
	/*! get client
	\retval sMQTTClient
	*/
	sMQTTClient *Client();
protected:
	unsigned char _type;
	sMQTTClient *_client;
};
/*! Connect new client event*/
class sMQTTNewClientEvent :public sMQTTEvent
{
public:
	sMQTTNewClientEvent(sMQTTClient *,std::string &,std::string &);
	/*! get client login*/
	std::string Login() {
		return login;
	};
	/*! get client password*/
	std::string Password() {
		return password;
	};
private:
	std::string login, password;
};
/*! Remove client event*/
class sMQTTRemoveClientEvent:public sMQTTEvent
{
public:
	sMQTTRemoveClientEvent(sMQTTClient *);
};
/*! Broker lost connection event*/
class sMQTTLostConnectionEvent :public sMQTTEvent
{
public:
	sMQTTLostConnectionEvent();
};
/*! Client publish messsage event*/
class sMQTTPublicClientEvent:public sMQTTEvent
{
public:
	sMQTTPublicClientEvent(sMQTTClient *client,const std::string &topic);
	void setPayload(const std::string &payload);
	std::string Topic();
	std::string Payload();
private:
	std::string _topic;
	std::string _payload;
};
/*! Client subscribe/unsubscribe event*/
class sMQTTSubUnSubClientEvent:public sMQTTEvent
{
public:
	sMQTTSubUnSubClientEvent(unsigned char type,sMQTTClient *client,const std::string &topic);
	/*! Get topic name*/
	std::string Topic();
private:
	std::string _topic;
};
#endif