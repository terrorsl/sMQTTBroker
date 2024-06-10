#ifndef SMQTTMESSAGE_FILE
#define SMQTTMESSAGE_FILE

#include<vector>
#include<string>

#if (__cplusplus >= 201703L)
#define MAYBE_UNUSED [[maybe_unused]]
#else
#define MAYBE_UNUSED
#endif

enum sMQTTError
{
	sMQTTOk = 0,
	sMQTTNowhereToSend = 1,
	sMQTTInvalidMessage = 2,
};

#if defined(DEBUG_ESP_PORT) || defined(CORE_DEBUG_LEVEL)
MAYBE_UNUSED static const char *debugMessageType[] = {	"Unknown",
	"Connect",
	"ConnAck",
	"Publish",
	"PubAck",
	"PubRec",
	"PubRel",
	"PubComp",
	"Subscribe",
	"SubAck",
	"UnSubscribe",
	"UnSuback",
	"PingReq",
	"PingResp",
	"Disconnect"
};
#endif

class sMQTTClient;
class sMQTTMessage
{
public:
	enum Type
	{
		Unknown = 0,
		Connect = 0x10,
		ConnAck = 0x20,
		Publish = 0x30,
		PubAck = 0x40,
		PubRec = 0x50,
		PubRel = 0x60,
		PubComp=0x70,
		Subscribe = 0x80,
		SubAck = 0x90,
		UnSubscribe = 0xA0,
		UnSuback = 0xB0,
		PingReq = 0xC0,
		PingResp = 0xD0,
		Disconnect = 0xE0,
		// version 5 only
		AUTH = 0xF0
	};
	enum State
	{
		FixedHeader = 0,
		Length = 1,
		VariableHeader = 2,
		PayLoad = 3,
		Complete = 4,
		Error = 5,
		Create = 6
	};
	sMQTTMessage();
	sMQTTMessage(Type t, unsigned char bits_d3_d0 = 0);
	void incoming(char byte);
	void add(char byte) { incoming(byte); }
	void add(const char* p, size_t len, bool addLength = true)
	{
		if (addLength)
		{
			buffer.reserve(buffer.size() + len + 2);
			incoming(len >> 8);
			incoming(len & 0xFF);
		}
		while (len--) incoming(*p++);
	}
	void add(const std::string &str) { add(str.c_str(), str.size()); }
	const char* end() const { return &buffer[0] + buffer.size(); }
	const char* getVHeader() const { return &buffer[vheader]; }
	void reset();
	Type type() const
	{
		return state == Complete ? static_cast<Type>(buffer[0] & 0XF0) : Unknown;
	}
	unsigned char QoS() {
		return (buffer[0] & 0x6)>>1;
	}
	bool isRetained() {
		return buffer[0] & 0x1;
	}
	sMQTTError sendTo(sMQTTClient *, bool needRecalc=true);
	// buff is MSB/LSB/STRING
	// output buff+=2, len=length(str)
	static void getString(const char* &buff, unsigned short &len);

	const char *decodeLength(const char *msg, unsigned long &size);

	const char *get(const char *msg, unsigned long &value);
	const char *get(const char *msg, unsigned short &value);
private:
	void create(Type type)
	{
		buffer.push_back((char)type);
		buffer.push_back('\0');		// reserved for msg length
		vheader = 2;
		size = 0;
		state = Create;
	}
	int encodeLength(char* msb, int length) const;

	std::vector<char> buffer;
	State state;
	unsigned short multiplyer;
	unsigned short size;
	unsigned char vheader;
};
#endif