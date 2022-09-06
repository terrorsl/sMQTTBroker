#include"sMQTTEvent.h"

sMQTTNewClientEvent::sMQTTNewClientEvent(sMQTTClient *client,
	std::string &_login, std::string &_password) :sMQTTEvent(NewClient_sMQTTEventType),_client(client),
	login(_login),password(_password)
{
};
sMQTTClient *sMQTTNewClientEvent::Client()
{
	return _client;
};
sMQTTRemoveClientEvent::sMQTTRemoveClientEvent(sMQTTClient *client):sMQTTEvent(RemoveClient_sMQTTEventType),_client(client)
{
};
sMQTTClient *sMQTTRemoveClientEvent::Client()
{
	return _client;
};
sMQTTLostConnectionEvent::sMQTTLostConnectionEvent() :sMQTTEvent(LostConnect_sMQTTEventType)
{
};
sMQTTPublicClientEvent::sMQTTPublicClientEvent(sMQTTClient *client,const std::string &topic):
sMQTTEvent(Public_sMQTTEventType),_client(client),_topic(topic)
{
};
void sMQTTPublicClientEvent::setPayload(const std::string &payload)
{
	_payload=payload;
};
sMQTTClient *sMQTTPublicClientEvent::Client()
{
	return _client;
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
sMQTTEvent(type)
{
	_client=client;
	_topic=topic;
};
sMQTTClient *sMQTTSubUnSubClientEvent::Client()
{
	return _client;
};
std::string sMQTTSubUnSubClientEvent::Topic()
{
	return _topic;
};