#ifndef SMQTTTOPIC_FILE
#define SMQTTTOPIC_FILE

#define SINGLE_LEVEL_WILDCARD "+"
#define TOPIC_LEVEL_SEPARATOR "/"
#define MULTI_LEVEL_WILDCARD "#"

#include<string>

class sMQTTTopic
{
public:
	sMQTTTopic(const char *name,char QoS=0);
	sMQTTTopic(const char *name, unsigned short nameLen, const char *payload, unsigned short payloadLen);
	sMQTTTopic(std::string &name,std::string &payload, char QoS=0);
	sMQTTTopic(sMQTTTopic *other);
	virtual ~sMQTTTopic();
	void update(sMQTTTopic *other)
	{
		if (_payload)
			delete[] _payload;
		_name = other->Name();

		if (other->Payload())
		{
			_payload = new char[strlen(other->Payload()) + 1];
			strcpy(_payload, other->Payload());
		}
		else
			_payload = 0;
		qos = other->QoS();
	}
	const char *Name() {
		return _name.c_str();
	}
	const char *Payload() {
		return _payload;
	}
	const unsigned char QoS() {
		return qos;
	}
	bool match(sMQTTTopic *other);
	bool match(const std::string &other);
	void subscribe(sMQTTClient *client);
	bool unsubscribe(sMQTTClient *client) {
		sMQTTClientList::iterator cli; for (cli = clients.begin(); cli != clients.end(); cli++)if (*cli == client) { clients.erase(cli); break; }
		return clients.empty();
	}
	const sMQTTClientList getSubscribeList() {
		return clients;
	}
protected:
	std::string _name;
	char *_payload;
	unsigned char qos;
	unsigned short _payloadLen;
	// subscribe to topic
	sMQTTClientList clients;
};

typedef std::vector<sMQTTTopic*> sMQTTTopicList;

class sMQTTTopicRetain :public sMQTTTopic
{
public:
	sMQTTTopicRetain(const char *name, const char *payload);
};
#endif