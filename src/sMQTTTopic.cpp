#include "sMQTTBroker.h"

sMQTTTopic::sMQTTTopic(const char *name, char QoS) :_payload(0), qos(QoS)
{
	_name = name;
}
sMQTTTopic::sMQTTTopic(const char *name, unsigned short nameLen, const char *payload, unsigned short payloadLen) :qos(0)
{
	_name=name;

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
sMQTTTopic::sMQTTTopic(std::string &name, std::string &payload, char QoS): qos(QoS)
{
	_name = name;
	if (payload.size())
	{
		_payload = new char[payload.size() + 1];
		_payload[payload.size()] = 0;
		memcpy(_payload, payload.c_str(), payload.size());
	}
	else
		_payload = 0;
};
sMQTTTopic::sMQTTTopic(sMQTTTopic *other) : _payload(0)
{
	update(other);
};
sMQTTTopic::~sMQTTTopic()
{
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
#if !defined(WIN32)
	if (strcmp(_name.c_str(), other->Name()) == 0)
		return true;
	if (strcmp(_name.c_str(), MULTI_LEVEL_WILDCARD) == 0)
		return true;

	char *ptr, *savePtr;
	char *pmatch, *savePtr2, *topic = (char*)other->Name();
	char *name = (char*)_name.c_str();
	ptr = strtok_r(name, TOPIC_LEVEL_SEPARATOR, &savePtr);
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
#endif
	return false;
};
bool sMQTTTopic::match(const std::string &other)
{
	if (_name == other)
		return true;
	if (_name == MULTI_LEVEL_WILDCARD)
		return true;

	int first_pos=0,first_end;
	int second_pos=0,second_end;
	for (;;)
	{
		first_end = _name.find_first_of(TOPIC_LEVEL_SEPARATOR, first_pos);
		second_end = other.find_first_of(TOPIC_LEVEL_SEPARATOR, second_pos);
		std::string fsub = _name.substr(first_pos, first_end- first_pos);
		std::string ssub = other.substr(second_pos, second_end- second_pos);
		if (fsub == ssub)
		{
			if (first_end == std::string::npos && second_end == std::string::npos)
				return true;
			else
			{
				if (first_end == std::string::npos || second_end == std::string::npos)
					return false;
			}
			first_pos = first_end+1;
			second_pos = second_end+1;
			continue;
		}
		
		if (_name.substr(first_pos, first_end- first_pos) == MULTI_LEVEL_WILDCARD)
			return true;
		if (_name.substr(first_pos, first_end- first_pos) == SINGLE_LEVEL_WILDCARD)
		{
			if (second_end == std::string::npos)
			{
				if (first_end == std::string::npos)
					return true;
			}
			second_pos = second_end + 1;
			first_pos = first_end + 1;
			continue;
		}
		break;
	}
	return false;
};