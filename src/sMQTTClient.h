#ifndef SMQTTCLIENT_FILE
#define SMQTTCLIENT_FILE

#include "sMQTTMessage.h"

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

//!\brief Main Client class
class sMQTTClient
{
public:
	sMQTTClient(sMQTTBroker *parent, TCPClient &client);
	~sMQTTClient();

	virtual void update();
	
	//! check connection
	bool isConnected();
	virtual void write(const char* buf, size_t length);

	//! get client id
	const std::string &getClientId() {
		return clientId;
	};
protected:
	void processMessage();
	void updateLiveStatus();

	char mqtt_flags;
	bool mqtt_connected;
	std::string clientId;
	unsigned short keepAlive;
	unsigned long aliveMillis;

	sMQTTBroker *_parent;
	TCPClient _client;
	sMQTTMessage message;
};

typedef enum {
    WSC_NOT_CONNECTED,
    WSC_HEADER,
    WSC_BODY,
    WSC_CONNECTED
} WSclientsStatus_t;

typedef enum {
    WSop_continuation = 0x00,    ///< %x0 denotes a continuation frame
    WSop_text         = 0x01,    ///< %x1 denotes a text frame
    WSop_binary       = 0x02,    ///< %x2 denotes a binary frame
                                 ///< %x3-7 are reserved for further non-control frames
    WSop_close = 0x08,           ///< %x8 denotes a connection close
    WSop_ping  = 0x09,           ///< %x9 denotes a ping
    WSop_pong  = 0x0A            ///< %xA denotes a pong
                                 ///< %xB-F are reserved for further control frames
} WSopcode_t;

typedef struct {
    bool fin;
    bool rsv1;
    bool rsv2;
    bool rsv3;

    WSopcode_t opCode;
    bool mask;

    size_t payloadLen;

    uint8_t * maskKey;
} WSMessageHeader_t;

#define WEBSOCKETS_STRING(var) var
// max size of the WS Message Header
#define WEBSOCKETS_MAX_HEADER_SIZE (14)
#define WEBSOCKETS_MAX_DATA_SIZE (15*1024)
#define WEBSOCKETS_YIELD() delay(0)
#define WEBSOCKETS_YIELD_MORE() delay(1)
#define WEBSOCKETS_TCP_TIMEOUT (5000)
typedef std::function<void(bool ok)> WSreadWaitCb;

class sMQTTClientWebSocket:public sMQTTClient
{
public:
	sMQTTClientWebSocket(sMQTTBroker *parent, TCPClient &client);

	void write(const char* buf, size_t length);
	void update();
private:
	void handleHeader(String *header);
	void headerDone();
	bool sendFrame(WSopcode_t opcode, uint8_t * payload=0, size_t length=0, bool fin=true, bool headerToPayload=false);
	uint8_t createHeader(uint8_t * headerPtr, WSopcode_t opcode, size_t length, bool mask, uint8_t maskKey[4], bool fin);

	void handleWebsocketCb();
	bool handleWebsocketWaitFor(size_t size);
	bool readCb(uint8_t * out, size_t n, WSreadWaitCb cb);
	void handleWebsocketPayloadCb(bool ok, uint8_t * payload);
	void clientDisconnect(uint16_t code, char * reason = NULL, size_t reasonLen = 0){
		SMQTT_LOGD("[WS][handleWebsocket] clientDisconnect code: %u\n", code);
		if(status == WSC_CONNECTED && code)
		{
			if(reason)
			{
            	sendFrame(WSop_close, (uint8_t *)reason, reasonLen);
        	} else {
				uint8_t buffer[2];
				buffer[0] = ((code >> 8) & 0xFF);
				buffer[1] = (code & 0xFF);
				sendFrame(WSop_close, &buffer[0], 2);
        	}
    	}
		_client.stop();
	};
	void handleHBPing(){
		
	}
	void handleWebsocket();

	String acceptKey(String & clientKey);
	String base64_encode(uint8_t * data, size_t length);

	void handleAuthorizationFailed() {
        _client.write(
            "HTTP/1.1 401 Unauthorized\r\n"
            "Server: arduino-WebSocket-Server\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 45\r\n"
            "Connection: close\r\n"
            "Sec-WebSocket-Version: 13\r\n"
            "WWW-Authenticate: Basic realm=\"WebSocket Server\""
            "\r\n"
            "This Websocket server requires Authorization!");
        //clientDisconnect();
    }
	void handleNonWebsocketConnection();

	bool execHttpHeaderValidation(String headerName, String headerValue) {
        /*if(_httpHeaderValidationFunc) {
            // return the value of the custom http header validation function
            return _httpHeaderValidationFunc(headerName, headerValue);
        }*/
        // no custom http header validation so just assume all is good
        return true;
    }
	bool hasMandatoryHeader(String headerName);
	void messageReceived(WSopcode_t opcode, uint8_t * payload, size_t length, bool fin);

	unsigned char status;
	bool cIsUpgrade;
	bool cIsWebsocket;
	int cVersion;
	String base64Authorization, _base64Authorization;
	String cUrl, cKey, cProtocol, cExtensions, cSessionId;
	bool cIsClient, isSocketIO;

	int cWsRXsize; ///< State of the RX
    uint8_t cWsHeader[WEBSOCKETS_MAX_HEADER_SIZE];    ///< RX WS Message buffer
	WSMessageHeader_t cWsHeaderDecode;
	size_t cMandatoryHeadersCount, _mandatoryHttpHeaderCount;
	bool cHttpHeadersValid;
};

typedef std::vector<sMQTTClient*> sMQTTClientList;
#endif