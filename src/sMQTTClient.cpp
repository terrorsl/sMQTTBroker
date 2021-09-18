#include<sMQTTBroker.h>

sMQTTClient::sMQTTClient(sMQTTBroker *parent, TCPClient *client):_parent(parent)
{
	_client = new WiFiClient(*client);
};
void sMQTTClient::update()
{
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
	ESP_LOGD(SMQTTTAG, "message type:%d", message.type());

	const char *header = message.getVHeader();
	switch (message.type())
	{
	case sMQTTMessage::Type::Connect:
		{
			mqtt_flags = header[7];

			sMQTTMessage msg(sMQTTMessage::Type::ConnAck);
			msg.add(0);	// Session present (not implemented)
			msg.add(0); // Connection accepted
			msg.sendTo(this);
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
			if (qos)
				payload += 2;
			len = message.end() - payload;

			sMQTTTopic topic(topicName, topicNameLen, payload, len);
			ESP_LOGD(SMQTTTAG, "message topic:%s payload:%s", topic.Name(), topic.Payload());

			_parent->publish(&topic, &message);
			if (message.isRetained())
				_parent->updateRetainedTopic(&topic);
		}
		break;
	case sMQTTMessage::Type::Subscribe:
		{
			unsigned short msg_id = (header[0] << 8) | header[1];
			ESP_LOGD(SMQTTTAG, "message id:%d", msg_id);
			const char *payload = header + 2;
			int count = 0;
			while (payload < message.end())
			{
				unsigned short len;
				message.getString(payload, len);	// Topic

				ESP_LOGD(SMQTTTAG, "message topic:%s", std::string(payload, len).c_str());
				_parent->subscribe_topic(this, std::string(payload, len).c_str());

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
			ESP_LOGD(SMQTTTAG, "message id:%d", msg_id);
			const char *payload = header + 2;
			int count = 0;
			while (payload < message.end())
			{
				unsigned short len;
				message.getString(payload, len);	// Topic

				ESP_LOGD(SMQTTTAG, "message topic:%s", std::string(payload, len).c_str());
				_parent->unsubscribe_topic(this, std::string(payload, len).c_str());

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
};