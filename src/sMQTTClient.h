#ifndef SMQTTCLIENT_FILE
#define SMQTTCLIENT_FILE

#include"sMQTTMessage.h"

class sMQTTBroker;

#define sMQTTUserNameFlag 0x80
#define sMQTTPasswordFlag 0x40
#define sMQTTWillRetainFlag 0x20
#define sMQTTWillQoSFlag 0x18
#define sMQTTWillFlag 0x4

#define sMQTTConnReturnAccepted 0x0
#define sMQTTConnReturnUnacceptableProtocolVersion 0x1
#define sMQTTConnReturnIdentifierRejected 0x2
#define sMQTTConnReturnServerUnavailable 0x3
#define sMQTTConnReturnBadUsernameOrPassword 0x4

#define sMQTTConnReturn5Success 0x00 //The Connection is accepted.
#define sMQTTConnReturn5UnspecifiedError 0x80 // Unspecified error
											  //The Server does not wish to reveal the reason for the failure, or none of the other Reason Codes apply.
#define sMQTTConnReturn5MalformedPacket 0x81 //Malformed Packet 
											 //Data within the CONNECT packet could not be correctly parsed.
#define sMQTTConnReturn5ProtocolError 0x82 //Protocol Error Data in the CONNECT packet does not conform to this specification.
#define sMQTTConnReturn5ImplementationSpecificError 0x83 //Implementation specific error The CONNECT is valid but is not accepted by this Server.
#define sMQTTConnReturn5UnsupportedProtocolVersion 0x84 //Unsupported Protocol Version The Server does not support the version of the MQTT protocol requested by the Client.
#define sMQTTConnReturn5ClientIdentifierNotValid 0x85 //Client Identifier not valid The Client Identifier is a valid string but is not allowed by the Server.
#define sMQTTConnReturn5BadUserNameOrPassword 0x86 //Bad User Name or Password The Server does not accept the User Name or Password specified by the Client
#define sMQTTConnReturn5NotAuthorized 0x87 //Not authorized The Client is not authorized to connect.
#define sMQTTConnReturn5ServerUnavailable 0x88 //The MQTT Server is not available.
#define sMQTTConnReturn5ServerBusy 0x89 // The Server is busy. Try again later.
#define sMQTTConnReturn5Banned 0x8A // This Client has been banned by administrative action. Contact the server administrator.
#define sMQTTConnReturn5BadAuthenticationMethod 0x8C //The authentication method is not supported or does not match the authentication method currently in use.
#define sMQTTConnReturn5TopicNameInvalid 0x90 //The Will Topic Name is not malformed, but is not accepted by this Server.
#define sMQTTConnReturn5PacketTooLarge 0x95 //The CONNECT packet exceeded the maximum permissible size.
#define sMQTTConnReturn5QuotaExceeded 0x97 //An implementation or administrative imposed limit has been exceeded.
#define sMQTTConnReturn5PayloadFormatInvalid 0x99 //The Will Payload does not match the specified Payload Format Indicator.
#define sMQTTConnReturn5RetainNotSupported 0x9A //The Server does not support retained messages, and Will Retain was set to 1.
#define sMQTTConnReturn5QoSNotSupported 0x9B //The Server does not support the QoS set in Will QoS.
#define sMQTTConnReturn5UseAnotherServer 0x9C //The Client should temporarily use another server.
#define sMQTTConnReturn5ServerMoved 0x9D //Server moved The Client should permanently use another server.
#define sMQTTConnReturn5ConnectionRateExceeded 0x9F //Connection rate exceeded The connection rate limit has been exceeded.

class sMQTTClient
{
public:
	sMQTTClient(sMQTTBroker *parent, TCPClient &client);
	virtual ~sMQTTClient();

	void update();
	
	bool isConnected();
	void write(const char* buf, size_t length);

	const std::string &getClientId() {
		return clientId;
	};
	void sendWillMessage();
	virtual void processMessage()=0;
protected:
	void updateLiveStatus();

	char mqtt_flags;
	bool mqtt_connected;
	std::string clientId;
	unsigned short keepAlive;
	unsigned long aliveMillis;
	std::string willTopic, willMessage;

	sMQTTBroker *_parent;
	TCPClient _client;
	sMQTTMessage message;
};

typedef std::vector<sMQTTClient*> sMQTTClientList;

class sMQTTClient3:public sMQTTClient
{
public:
	sMQTTClient3(sMQTTBroker *parent, TCPClient &client):sMQTTClient(parent, client){};
	void processMessage() override;
};
class sMQTTClient5:public sMQTTClient
{
public:
	sMQTTClient5(sMQTTBroker *parent, TCPClient &client):sMQTTClient(parent, client){};
	void processMessage() override;
};
#endif