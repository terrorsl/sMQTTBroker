#include<sMQTTBroker.h>

sMQTTClient::sMQTTClient(sMQTTBroker *parent, TCPClient *client):_parent(parent), mqtt_connected(false)
{
	_client = new WiFiClient(*client);
};
void sMQTTClient::update()
{
	unsigned long currentMillis;
#if defined(ESP8266) || defined(ESP32)
	currentMillis = millis();
#endif
	if (keepAlive!=0 && aliveMillis < currentMillis)
	{
		SMQTT_LOGD("aliveMillis < currentMillis");
		_client->stop();
		return;
	}
	while (_client->available()>0)
	{
		message.incoming(_client->read());
		if (message.type())
		{
			processMessage();
			message.reset();
		}
	}
};
bool sMQTTClient::isConnected()
{
	return _client->connected();
};
void sMQTTClient::write(const char* buf, size_t length)
{
	if (_client) _client->write(buf, length);
}
void sMQTTClient::processMessage()
{
	SMQTT_LOGD("message type:%d", message.type());

	const char *header = message.getVHeader();
	switch (message.type())
	{
	case sMQTTMessage::Type::Connect:
		{
			if (mqtt_connected)
			{
				_client->stop();
				break;
			}
			unsigned char status = 0;
			if (strncmp("MQTT", header + 2, 4))
			{
				//TODO: close connection
			}
			if (header[6] != 0x04)
			{
				status = sMQTTConnReturnUnacceptableProtocolVersion;
				// Level 3.1.1
			}
			else
			{
				unsigned short len;
				mqtt_flags = header[7];
				keepAlive = (header[8] << 8) | header[9];

				const char *payload = &header[10];
				message.getString(payload, len);
				clientId = std::string(payload,len);
				payload += len;

				SMQTT_LOGD("message clientId:%s", clientId.c_str());

				if (mqtt_flags&sMQTTWillFlag)
				{
					//topic
					message.getString(payload, len);
					payload += len;
					//message
					message.getString(payload, len);
					payload += len;
				}
				std::string username;
				if (mqtt_flags&sMQTTUserNameFlag)
				{
					message.getString(payload, len);

					username = std::string(payload, len);
					SMQTT_LOGD("message user:%s", username.c_str());

					payload += len;
				}
				std::string password;
				if (mqtt_flags&sMQTTPasswordFlag)
				{
					message.getString(payload, len);
					password = std::string(payload, len);
					SMQTT_LOGD("message password:%s", password.c_str());
					payload += len;
				}

				if (_parent->isClientConnected(this, clientId) == false)
					if (_parent->onConnect(this, username, password) == false)
						status = sMQTTConnReturnBadUsernameOrPassword;
				else
					status = sMQTTConnReturnIdentifierRejected;
			}

			sMQTTMessage msg(sMQTTMessage::Type::ConnAck);
			msg.add(0);	// Session present (not implemented)
			msg.add(status); // Connection accepted
			msg.sendTo(this);

			mqtt_connected = true;
		}
		break;
	case sMQTTMessage::Type::Publish:
		{
			unsigned char qos = message.QoS();//message.type() & 0x6;
			unsigned short len, topicNameLen;
			const char *payload = header;
			message.getString(payload, len);

			const char *topicName = payload;
			topicNameLen = len;
			payload += len;

			char packeteIdent[2];
			if (qos)
			{
				packeteIdent[0] = payload[0];
				packeteIdent[1] = payload[1];
				payload += 2;
			}
			len = message.end() - payload;

			sMQTTTopic topic(topicName, topicNameLen, payload, len);
			SMQTT_LOGD("message topic:%s payload:%s", topic.Name(), topic.Payload());

			_parent->publish(&topic, &message);
			if (message.isRetained())
				_parent->updateRetainedTopic(&topic);

			if (qos==1)
			{
				sMQTTMessage msg(sMQTTMessage::Type::PubAck);
				msg.add(packeteIdent[0]);
				msg.add(packeteIdent[1]);
				msg.sendTo(this);
			}
		}
		break;
	case sMQTTMessage::Type::Subscribe:
		{
			unsigned short msg_id = (header[0] << 8) | header[1];
			SMQTT_LOGD("message id:%d", msg_id);
			const char *payload = header + 2;
			int count = 0;
			while (payload < message.end())
			{
				unsigned short len;
				message.getString(payload, len);	// Topic

				SMQTT_LOGD("message topic:%s", std::string(payload, len).c_str());
				_parent->subscribe(this, std::string(payload, len).c_str());

				payload += len;
				unsigned char qos = *payload++;
				count++;
			}
			sMQTTMessage msg(sMQTTMessage::Type::SubAck);
			msg.add(header[0]);
			msg.add(header[1]);
			for (int i = 0; i<count; i++)
				msg.add(0);
			msg.sendTo(this);
		}
		break;
	case sMQTTMessage::Type::UnSubscribe:
		{
			unsigned short msg_id = (header[0] << 8) | header[1];
			SMQTT_LOGD("message id:%d", msg_id);
			const char *payload = header + 2;
			int count = 0;
			while (payload < message.end())
			{
				unsigned short len;
				message.getString(payload, len);	// Topic

				SMQTT_LOGD("message topic:%s", std::string(payload, len).c_str());
				_parent->unsubscribe(this, std::string(payload, len).c_str());

				payload += len;
				unsigned char qos = *payload++;
				count++;
			}
			sMQTTMessage msg(sMQTTMessage::Type::UnSuback);
			msg.add(header[0]);
			msg.add(header[1]);
			msg.sendTo(this);
		}
		break;
	case sMQTTMessage::Type::Disconnect:
		{
			_client->stop();
		}
		break;
	case sMQTTMessage::Type::PingReq:
		{
			sMQTTMessage msg(sMQTTMessage::Type::PingResp);
			msg.sendTo(this);
		}
		break;
	}
	updateLiveStatus();
};
void sMQTTClient::updateLiveStatus()
{
	if (keepAlive)
#if defined(ESP8266) || defined(ESP32)
		aliveMillis = keepAlive * 1000 + millis();
#endif
	else
		aliveMillis = 0;
};