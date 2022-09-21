#include"sMQTTBroker.h"

sMQTTClient::sMQTTClient(sMQTTBroker *parent, TCPClient &client):mqtt_connected(false), _parent(parent)
{
	_client = client;
	keepAlive = 25;
	updateLiveStatus();
};
sMQTTClient::~sMQTTClient()
{
	//SMQTT_LOGD("free _client");
	//delete _client;
};
void sMQTTClient::update()
{
	while (_client.available()>0)
	{
		message.incoming(_client.read());
		if (message.type())
		{
			processMessage();
			message.reset();
			break;
		}
	}
	unsigned long currentMillis;
#if defined(ESP8266) || defined(ESP32)
	currentMillis = millis();
#endif
	if (keepAlive != 0 && aliveMillis < currentMillis)
	{
		SMQTT_LOGD("aliveMillis(%d) < currentMillis(%d)", aliveMillis, currentMillis);
		_client.stop();
	}
	//else
	//	SMQTT_LOGD("time %d", aliveMillis - currentMillis);
};
bool sMQTTClient::isConnected()
{
	return _client.connected();
};
void sMQTTClient::write(const char* buf, size_t length)
{
	_client.write(buf, length);
}
void sMQTTClient::updateLiveStatus()
{
	if (keepAlive)
#if defined(ESP8266) || defined(ESP32)
		//aliveMillis = (keepAlive*1.5) * 1000 + millis();
		aliveMillis = keepAlive*1500 + millis();
#else
		aliveMillis = 0;
#endif
	else
		aliveMillis = 0;
};
void sMQTTClient::sendWillMessage()
{
	if(willTopic.empty()==false)
		_parent->publish(willTopic,willMessage);
};