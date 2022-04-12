#ifndef SMQTTEVENT_FILE
#define SMQTTEVENT_FILE

#include<string>

enum sMQTTEventType
{
	NewClient_sMQTTEventType,
	RemoveClient_sMQTTEventType,
	Public_sMQTTEventType,
	LostConnect_sMQTTEventType
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
class sMQTTNewClientEvent :public sMQTTEvent
{
public:
	sMQTTNewClientEvent(sMQTTClient *,std::string &,std::string &);
	sMQTTClient *Client();
	std::string Login() {
		return login;
	};
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
#endif