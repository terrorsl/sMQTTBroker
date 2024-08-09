#include"sMQTTEvent.h"

sMQTTClient *sMQTTEvent::Client()
{
	return _client;
};

sMQTTNewClientEvent::sMQTTNewClientEvent(sMQTTClient *client,
	std::string &_login, std::string &_password) :sMQTTEvent(NewClient_sMQTTEventType,client),
	login(_login),password(_password)
{
};
sMQTTRemoveClientEvent::sMQTTRemoveClientEvent(sMQTTClient *client):
sMQTTEvent(RemoveClient_sMQTTEventType,client)
{
};
sMQTTLostConnectionEvent::sMQTTLostConnectionEvent() :sMQTTEvent(LostConnect_sMQTTEventType,0)
{
};
sMQTTPublicClientEvent::sMQTTPublicClientEvent(sMQTTClient *client,const std::string &topic):
sMQTTEvent(Public_sMQTTEventType,client),_topic(topic)
{
};
void sMQTTPublicClientEvent::setPayload(const std::string &payload)
{
	_payload=payload;
};
std::string sMQTTPublicClientEvent::Topic()
{
	return _topic;
};
std::string sMQTTPublicClientEvent::Payload()
{
	return _payload;
};
sMQTTSubUnSubClientEvent::sMQTTSubUnSubClientEvent(unsigned char type,sMQTTClient *client,const std::string &topic):
sMQTTEvent(type,client)
{
	_client=client;
	_topic=topic;
};
std::string sMQTTSubUnSubClientEvent::Topic()
{
	return _topic;
};