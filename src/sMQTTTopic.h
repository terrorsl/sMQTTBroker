#ifndef SMQTTTOPIC_FILE
#define SMQTTTOPIC_FILE

#define SINGLE_LEVEL_WILDCARD "+"
#define TOPIC_LEVEL_SEPARATOR "/"
#define MULTI_LEVEL_WILDCARD "#"

class sMQTTTopic
{
public:
	sMQTTTopic(const char *name);
	sMQTTTopic(const char *name, unsigned short nameLen, const char *payload, unsigned short payloadLen);
	sMQTTTopic(sMQTTTopic *other);
	virtual ~sMQTTTopic();
	void update(sMQTTTopic *other)
	{
		if (_name)
			delete[] _name;
		if (_payload)
			delete[] _payload;
		_name = new char[strlen(other->Name()) + 1];
		strcpy(_name, other->Name());
		_payload = new char[strlen(other->Payload()) + 1];
		strcpy(_payload, other->Payload());
		qos = other->QoS();
	}
	const char *Name() {
		return _name;
	}
	const char *Payload() {
		return _payload;
	}
	const unsigned char QoS() {
		return qos;
	}
	bool match(sMQTTTopic *other);
	void subscribe(sMQTTClient *client);
	bool unsubscribe(sMQTTClient *client) {
		sMQTTClientList::iterator cli; for (cli = clients.begin(); cli != clients.end(); cli++)if (*cli == client) { clients.erase(cli); break; }
		return clients.empty();
	}
	const sMQTTClientList getSubscribeList() {
		return clients;
	}
protected:
	char *_name, *_payload;
	unsigned char qos;
	unsigned short _nameLen, _payloadLen;
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