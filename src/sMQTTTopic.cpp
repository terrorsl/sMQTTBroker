#include<sMQTTBroker.h>

sMQTTTopic::sMQTTTopic(const char *name) :_payload(0), qos(0)
{
	_nameLen = strlen(name);
	_name = new char[_nameLen + 1];
	_name[_nameLen] = 0;
	memcpy(_name, name, _nameLen);
	_nameLen += 1;
}
sMQTTTopic::sMQTTTopic(const char *name, unsigned short nameLen, const char *payload, unsigned short payloadLen) :qos(0)
{
	_name = new char[nameLen + 1];
	_name[nameLen] = 0;
	_nameLen = nameLen + 1;
	memcpy(_name, name, nameLen);

	_payloadLen = payloadLen;
	if (payloadLen)
	{
		_payload = new char[payloadLen + 1];
		_payload[payloadLen] = 0;
		_payloadLen = payloadLen + 1;
		memcpy(_payload, payload, payloadLen);
	}
	else
		_payload = 0;
};
sMQTTTopic::sMQTTTopic(sMQTTTopic *other) :_name(0), _payload(0)
{
	update(other);
};
sMQTTTopic::~sMQTTTopic()
{
	delete[] _name;
	if (_payload)
		delete[] _payload;
};
void sMQTTTopic::subscribe(sMQTTClient *client)
{
	for (auto cli : clients)
		if (cli == client)return;
	clients.push_back(client);
};
bool sMQTTTopic::match(sMQTTTopic *other)
{
	if (strcmp(_name, other->Name()) == 0)
		return true;
	if (strcmp(_name, MULTI_LEVEL_WILDCARD) == 0)
		return true;

	char *ptr, *savePtr;
	char *pmatch, *savePtr2, *topic = (char*)other->Name();
	ptr = strtok_r(_name, TOPIC_LEVEL_SEPARATOR, &savePtr);
	pmatch = strtok_r(topic, TOPIC_LEVEL_SEPARATOR, &savePtr2);
	while (ptr != NULL)
	{
		if (strcmp(ptr, MULTI_LEVEL_WILDCARD) == 0)
			return true;
		/* Nope - check for matches... */
		if (pmatch != NULL)
		{
			if (strcmp(ptr, SINGLE_LEVEL_WILDCARD) != 0 && strcmp(ptr, pmatch) != 0)
				/* The two levels simply don't match... */
				break;
		}
		else
			break;		/* No more tokens to match against further tokens in the wildcard stream... */
		ptr = strtok_r(NULL, TOPIC_LEVEL_SEPARATOR, &savePtr);
		pmatch = strtok_r(NULL, TOPIC_LEVEL_SEPARATOR, &savePtr2);
	}
	if (pmatch == NULL && ptr == NULL)
		return true;
	return false;
};