#ifndef SMQTT_WEBSOCKET_CLIENT_FILE
#define SMQTT_WEBSOCKET_CLIENT_FILE

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

typedef enum{
    WSerror_close_normal = 1000,
    //1001 indicates that an endpoint is "going away", such as a server going down or a browser having navigated away from a page.
    WSerror_close_going_away,
    //1002 indicates that an endpoint is terminating the connection due to a protocol error.
    WSerror_close_protocol_error,
    //1003 indicates that an endpoint is terminating the connection because it has received a type of data it cannot accept (e.g., an endpoint that understands only text data MAY send this if it receives a binary message).
    WSerror_close_unsupported,
    //1005 is a reserved value and MUST NOT be set as a status code in a Close control frame by an endpoint. It is designated for use in applications expecting a status code to indicate that no status code was actually present.
    WSerror_closed_no_status=1005,
    //1006 is a reserved value and MUST NOT be set as a status code in a Close control frame by an endpoint. It is designated for use in applications expecting a status code to indicate that the connection was closed abnormally, e.g., without sending or receiving a Close control frame.
    WSerror_close_abnormal,
    //1007 indicates that an endpoint is terminating the connection because it has received data within a message that was not consistent with the type of the message (e.g., non-UTF-8 [RFC3629] data within a text message).
    WSerror_unsupported_payload,
    //1008 indicates that an endpoint is terminating the connection because it has received a message that violates its policy. This is a generic status code that can be returned when there is no other more suitable status code (e.g., 1003 or 1009) or if there is a need to hide specific details about the policy.
    WSerror_violation,
    //1009 indicates that an endpoint is terminating the connection because it has received a message that is too big for it to process.
    WSerror_messsage_too_big,
    //1010 indicates that an endpoint (client) is terminating the connection because it has expected the server to negotiate one or more extension, but the server didn't return them in the response message of the WebSocket handshake. The list of extensions that are needed SHOULD appear in the /reason/ part of the Close frame. Note that this status code is not used by the server, because it can fail the WebSocket handshake instead.
    WSerror_mandatory_extension,
    //1011 indicates that a server is terminating the connection because it encountered an unexpected condition that prevented it from fulfilling the request.
    WSerror_server_error,
    //1012 indicates that the server / service is restarting.
    WSerror_service_restart,
    //1013 indicates that a temporary server condition forced blocking the client's request.
    WSerror_try_again_later,
    //1014 indicates that the server acting as gateway received an invalid response
    WSerror_bad_gateway,
    //1015 is a reserved value and MUST NOT be set as a status code in a Close control frame by an endpoint. It is designated for use in applications expecting a status code to indicate that the connection was closed due to a failure to perform a TLS handshake (e.g., the server certificate can't be verified).
    WSerror_TLS_handshake_fail
}WSerror_t;

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
	uint8_t createHeader(uint8_t * headerPtr, WSopcode_t opcode, size_t length, bool fin);

	void handleWebsocketCb();
	bool handleWebsocketWaitFor(size_t size);
	bool readCb(uint8_t * out, size_t n);
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

	int cWsRXsize; ///< State of the RX
    uint8_t cWsHeader[WEBSOCKETS_MAX_HEADER_SIZE];    ///< RX WS Message buffer
	WSMessageHeader_t cWsHeaderDecode;
	size_t cMandatoryHeadersCount, _mandatoryHttpHeaderCount;
	bool cHttpHeadersValid;
};
#endif