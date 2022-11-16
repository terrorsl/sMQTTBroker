#include "sMQTTBroker.h"

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
void sMQTTClient::processMessage()
{
	if (message.type() <= sMQTTMessage::Type::Disconnect)
	{
		SMQTT_LOGD("message type:%s(0x%x)", debugMessageType[message.type() / 0x10], message.type());
	}

	const char *header = message.getVHeader();
	switch (message.type())
	{
	case sMQTTMessage::Type::Connect:
		{
			if (mqtt_connected)
			{
				_client.stop();
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
				SMQTT_LOGD("message keepTime:%d", keepAlive);

				if (mqtt_flags&sMQTTWillFlag)
				{
					//topic
					message.getString(payload, len);
					//willTopic = std::string(payload, len);
					payload += len;
					//message
					message.getString(payload, len);
					//willMessage = std::string(payload, len);
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

				if (_parent->isClientConnected(this) == false)
				{
					sMQTTNewClientEvent event(this, username, password);
					if(_parent->onEvent(&event)==false || _parent->onConnect(this, username, password) == false)
						status = sMQTTConnReturnBadUsernameOrPassword;
				}
				else
					status = sMQTTConnReturnIdentifierRejected;
			}

			sMQTTMessage msg(sMQTTMessage::Type::ConnAck);
			msg.add(0);	// Session present (not implemented)
			msg.add(status); // Connection accepted
			msg.sendTo(this);

			if (status)
				_client.stop();
			else
				mqtt_connected = true;
		}
		break;
	case sMQTTMessage::Type::Publish:
		{
			unsigned char qos = message.QoS();

			unsigned short len;
			const char *payload = header;
			message.getString(payload, len);

			const char *topicName = payload;
			std::string _topicName(topicName, len);
			payload += len;

			char packeteIdent[2]={0};
			if (qos)
			{
				packeteIdent[0] = payload[0];
				packeteIdent[1] = payload[1];
				payload += 2;
			}
			len = message.end() - payload;
			std::string _payload(payload, len);

			sMQTTTopic topic(_topicName,_payload, qos);

			if (message.isRetained())
				_parent->updateRetainedTopic(&topic);

			switch (qos)
			{
			case 1:
				{
					sMQTTMessage msg(sMQTTMessage::Type::PubAck);
					msg.add(packeteIdent[0]);
					msg.add(packeteIdent[1]);
					msg.sendTo(this);
				}
				break;
			case 2:
				{
					sMQTTMessage msg(sMQTTMessage::Type::PubRec);
					msg.add(packeteIdent[0]);
					msg.add(packeteIdent[1]);
					msg.sendTo(this);
				}
				break;
			}

			_parent->publish(this,&topic, &message);
		}
		break;
	case sMQTTMessage::Type::PubAck:
		{
		}
		break;
	case sMQTTMessage::Type::PubRec:
		{
			const char *payload = header;
			sMQTTMessage msg(sMQTTMessage::Type::PubRel);
			msg.add(payload[0]);
			msg.add(payload[1]);
			msg.sendTo(this);
		}
		break;
	case sMQTTMessage::Type::PubRel:
		{
			const char *payload = header;
			sMQTTMessage msg(sMQTTMessage::Type::PubComp);
			msg.add(payload[0]);
			msg.add(payload[1]);
			msg.sendTo(this);
		}
		break;
	case sMQTTMessage::Type::PubComp:
		{

		}
		break;
	case sMQTTMessage::Type::Subscribe:
		{
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
			unsigned short msg_id = (header[0] << 8) | header[1];
			SMQTT_LOGD("message id:%d", msg_id);
#endif
			const char *payload = header + 2;
			std::vector<char> qoss;
			while (payload < message.end())
			{
				unsigned short len;
				message.getString(payload, len);	// Topic

				std::string topic(payload, len);
				SMQTT_LOGD("message topic:%s", topic.c_str());
				payload += len;
				unsigned char qos = *payload++;

				if (_parent->subscribe(this, topic.c_str()) == false)
				{
					SMQTT_LOGD("subscribe failed");
					qos = 0x80;
				}
				qoss.push_back(qos);
			}
			sMQTTMessage msg(sMQTTMessage::Type::SubAck);
			msg.add(header[0]);
			msg.add(header[1]);
			for (int i = 0; i<qoss.size(); i++)
				msg.add(qoss[i]);
			msg.sendTo(this);
		}
		break;
	case sMQTTMessage::Type::UnSubscribe:
		{
			//unsigned short msg_id = (header[0] << 8) | header[1];
			//SMQTT_LOGD("message id:%d", msg_id);
			const char *payload = header + 2;
			while (payload < message.end())
			{
				unsigned short len;
				message.getString(payload, len);	// Topic

				//SMQTT_LOGD("message topic:%s", std::string(payload, len).c_str());
				_parent->unsubscribe(this, std::string(payload, len).c_str());

				payload += len;
				//unsigned char qos = *payload++;
			}
			sMQTTMessage msg(sMQTTMessage::Type::UnSuback);
			msg.add(header[0]);
			msg.add(header[1]);
			msg.sendTo(this);
		}
		break;
	case sMQTTMessage::Type::Disconnect:
		{
			mqtt_connected = false;
			_client.stop();
		}
		break;
	case sMQTTMessage::Type::PingReq:
		{
			sMQTTMessage msg(sMQTTMessage::Type::PingResp);
			msg.sendTo(this);
		}
		break;
	default:
		{
			SMQTT_LOGD("unknown message %d", message.type());
			mqtt_connected = false;
			_client.stop();
		}
		break;
	}
	updateLiveStatus();
};
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