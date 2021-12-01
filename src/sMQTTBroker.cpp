#include"sMQTTBroker.h"

bool sMQTTBroker::init(unsigned short port)
{
	_server = new TCPServer(port);
	if (_server == 0)
		return false;
	_server->begin();
	return true;
};
void sMQTTBroker::update()
{
#if defined(ESP8266) || defined(ESP32)
	WiFiClient client = _server->available();
	if (client)
	{
		SMQTT_LOGD("New Client");
		clients.push_back(new sMQTTClient(this, client));
	}
#endif
	sMQTTClientList::iterator clit;
	for (clit = clients.begin(); clit != clients.end(); clit++)
	{
		sMQTTClient *c = *clit;
		if (c->isConnected())
			c->update();
		else
		{
			onRemove(c);

			for (sMQTTTopicList::iterator sub = subscribes.begin(); sub != subscribes.end(); sub++)
			{
				if ((*sub)->unsubscribe(c) == true)
				{
					delete *sub;
					sub= subscribes.erase(sub);
					if (sub == subscribes.end())
						break;
				}
			}

			delete c;
			clit=clients.erase(clit);
			SMQTT_LOGD("Clients %d", clients.size());
			if (clit == clients.end())
				break;
		}
	}
};
bool sMQTTBroker::subscribe(sMQTTClient *client, const char *topic)
{
	if (isTopicValidName(topic) == false)
		return false;
	sMQTTTopicList::iterator sub;
	for (sub = subscribes.begin(); sub != subscribes.end(); sub++)
	{
		if (strcmp((*sub)->Name(), topic) == 0)
		{
			(*sub)->subscribe(client);
			findRetainTopic(*sub, client);
			return true;
		}
	}
	sMQTTTopic *newTopic = new sMQTTTopic(topic,0);
	newTopic->subscribe(client);
	subscribes.push_back(newTopic);

	findRetainTopic(newTopic, client);
	return true;
};
void sMQTTBroker::unsubscribe(sMQTTClient *client, const char *topic)
{
	sMQTTTopicList::iterator it;
	for (it = subscribes.begin(); it != subscribes.end(); it++)
	{
		if (strcmp((*it)->Name(), topic) == 0)
		{
			if ((*it)->unsubscribe(client) == true)
			{
				delete *it;
				subscribes.erase(it);
			}
			break;
		}
	}
};
void sMQTTBroker::publish(sMQTTClient *client, sMQTTTopic *topic, sMQTTMessage *msg)
{
	if(topic->Payload())
		onPublish(client,std::string(topic->Name()),std::string(topic->Payload()));
	else
		onPublish(client,std::string(topic->Name()), std::string());

	sMQTTTopicList::iterator sub;
	for (sub = subscribes.begin(); sub != subscribes.end(); sub++)
	{
		//if ((*sub)->match(topic))
		if ((*sub)->match(topic->Name()))
		{
			sMQTTClientList subList = (*sub)->getSubscribeList();
			SMQTT_LOGD("topic %s Clients %d", topic->Name(), subList.size());
			for (auto cl : subList)
			{
				msg->sendTo(cl,false);
			}
		}
	}
};
bool sMQTTBroker::isTopicValidName(const char *filter)
{
	int length = strlen(filter);
	const char *hashpos = strchr(filter, '#');	/* '#' wildcard can be only at the beginning or the end of a topic */

	if (hashpos != NULL)
	{
		const char *second = strchr(hashpos + 1, '#');
		if ((hashpos != filter && hashpos != filter + (length - 1)) || second != NULL)
			return false;
	}
	/* '#' or '+' only next to a slash separator or end of name */
	for (const char *c = "#+"; *c != '\0'; ++c)
	{
		const char *pos = strchr(filter, *c);
		while (pos != NULL)
		{
			if (pos > filter)
			{	/* check previous char is '/' */
				if (*(pos - 1) != '/')
					return false;
			}
			if (*(pos + 1) != '\0')
			{	/* check that subsequent char is '/' */
				if (*(pos + 1) != '/')
					return false;
			}
			pos = strchr(pos + 1, *c);
		}
	}
	return true;
};
void sMQTTBroker::updateRetainedTopic(sMQTTTopic *topic)
{
	SMQTT_LOGD("updateRetainedTopic %s", topic->Name());
	sMQTTTopicList::iterator it;
	for (it = retains.begin(); it != retains.end(); it++)
	{
		if (strcmp((*it)->Name(), topic->Name()) == 0)
			break;
	}
	if (it != retains.end())
	{
		SMQTT_LOGD("updateRetainedTopic update %s", topic->Name());
		if (topic->Payload())
			(*it)->update(topic);
		else
		{
			delete *it;
			retains.erase(it);
		}
	}
	else
	{
		// append only with payload
		if (topic->Payload())
		{
			sMQTTTopic *newTopic = new sMQTTTopic(topic);
			retains.push_back(newTopic);
		}
	}
};
void sMQTTBroker::findRetainTopic(sMQTTTopic *topic, sMQTTClient *client)
{
	//SMQTT_LOGD("findRetainTopic %s %d", topic->Name(), retains.size());
	sMQTTTopicList::iterator it;
	int time = 0;
	for (it = retains.begin(); it != retains.end(); it++)
	{
		//if (topic->match(*it))
		if (topic->match((*it)->Name()))
		{
			SMQTT_LOGD("findRetainTopic %s qos:%d", (*it)->Name(), (*it)->QoS());
			SMQTT_LOGD("findRetainTopic %s", (*it)->Payload());
			sMQTTMessage msg(sMQTTMessage::Type::Publish, (*it)->QoS()<<1);
			msg.add((*it)->Name(), strlen((*it)->Name()));
			// msg_id
			if ((*it)->QoS()) {
				msg.add(time>>8);
				msg.add(time);
				time++;
			}
			msg.add((*it)->Payload(), strlen((*it)->Payload()), false);
			msg.sendTo(client);
		}
	}
};
bool sMQTTBroker::isClientConnected(sMQTTClient *client, const std::string &clientId)
{
	sMQTTClientList::iterator clit;
	for (clit = clients.begin(); clit != clients.end(); clit++)
	{
		sMQTTClient *c = *clit;
		if (c == client)
			return false;
		if (c->getClientId() == clientId)
		{
			SMQTT_LOGD("found:%s", clientId.c_str());
			return true;
		}
	}
	return false;
};
bool sMQTTBroker::onConnect(sMQTTClient *client, const std::string &username, const std::string &password) {
	// do nothing here
	return true;
};
void sMQTTBroker::onRemove(sMQTTClient*) {
};
void sMQTTBroker::publish(const std::string &topic, const std::string &payload)
{
	sMQTTTopicList::iterator sub;
	for (sub = subscribes.begin(); sub != subscribes.end(); sub++)
	{
		if ((*sub)->match(topic))
		{
			sMQTTClientList subList = (*sub)->getSubscribeList();
			SMQTT_LOGD("topic %s Clients %d", topic.c_str(), subList.size());

			sMQTTMessage msg(sMQTTMessage::Type::Publish, 0);
			msg.add(topic.c_str(), topic.size());
			// msg_id
			/*if ((*it)->QoS()) {
				msg.add(time >> 8);
				msg.add(time);
				time++;
			}*/
			msg.add(payload.c_str(), payload.size(), false);
			//msg.sendTo(client);
			for (auto cl : subList)
			{
				msg.sendTo(cl);
			}
		}
	}
};