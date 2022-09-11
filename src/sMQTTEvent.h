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
class sMQTTEvent
{
public:
	sMQTTEvent(unsigned char type) :_type(type) {};

	unsigned char Type() {
		return _type;
	}
protected:
	unsigned char _type;
};
class sMQTTClient;
/*! Connect new client event*/
class sMQTTNewClientEvent :public sMQTTEvent
{
public:
	sMQTTNewClientEvent(sMQTTClient *,std::string &,std::string &);
	/*! get connected client */
	sMQTTClient *Client();
	/*! get client login*/
	std::string Login() {
		return login;
	};
	/*! get client password*/
	std::string Password() {
		return password;
	};
private:
	sMQTTClient *_client;
	std::string login, password;
};
class sMQTTRemoveClientEvent:public sMQTTEvent
{
public:
	sMQTTRemoveClientEvent(sMQTTClient *);
	sMQTTClient *Client();
private:
	sMQTTClient *_client;
};
class sMQTTLostConnectionEvent :public sMQTTEvent
{
public:
	sMQTTLostConnectionEvent();
};
class sMQTTPublicClientEvent:public sMQTTEvent
{
public:
	sMQTTPublicClientEvent(sMQTTClient *client,const std::string &topic);
	void setPayload(const std::string &payload);
	sMQTTClient *Client();
	std::string Topic();
	std::string Payload();
private:
	sMQTTClient *_client;
	std::string _topic;
	std::string _payload;
};
class sMQTTSubUnSubClientEvent:public sMQTTEvent
{
public:
	sMQTTSubUnSubClientEvent(unsigned char type,sMQTTClient *client,const std::string &topic);
	sMQTTClient *Client();
	std::string Topic();
private:
	sMQTTClient *_client;
	std::string _topic;
};
#endif