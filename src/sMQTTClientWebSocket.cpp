#include "sMQTTBroker.h"
#include <libb64/cencode.h>

sMQTTClientWebSocket::sMQTTClientWebSocket(sMQTTBroker *parent, TCPClient &client):sMQTTClient(parent, client), status(WSC_HEADER)
{
	_mandatoryHttpHeaderCount=0;
    isSocketIO=false;
    cIsClient=false;
};
void sMQTTClientWebSocket::write(const char* buf, size_t length)
{
	sendFrame(WSop_binary, (uint8_t*)buf, length);
};
void sMQTTClientWebSocket::update()
{
	int len = _client.available();
	if(len)
	{
		switch(status)
		{
		case WSC_HEADER:
			{
				String header = _client.readStringUntil('\n');
				handleHeader(&header);
			}
			break;
        case WSC_BODY:
            {
                char buf[256] = { 0 };
                _client.readBytes(&buf[0], std::min((size_t)len, sizeof(buf)));
                String bodyLine = buf;
                handleHeader(&bodyLine);
            }
            break;
		case WSC_CONNECTED:
			handleWebsocket();
			break;
        default:
            SMQTT_LOGD("[WS-Server][handleClientData] unknown client status %d", status);
            clientDisconnect(1002);
            break;
		}
	}
    handleHBPing();

	unsigned long currentMillis;
#if defined(ESP8266) || defined(ESP32)
	currentMillis = millis();
#endif
	if (keepAlive != 0 && aliveMillis < currentMillis)
	{
		SMQTT_LOGD("aliveMillis(%lu) < currentMillis(%lu)", aliveMillis, currentMillis);
		_client.stop();
	}
}
void sMQTTClientWebSocket::handleHeader(String *header)
{
	static const char * NEW_LINE = "\r\n";
	header->trim();

	if(header->length())
	{
        SMQTT_LOGD("[WS-Server][handleHeader] RX: %s", header->c_str());

        // websocket requests always start with GET see rfc6455
        if(header->startsWith("GET ")) {
            // cut URL out
            cUrl = header->substring(4, header->indexOf(' ', 4));

            // reset non-websocket http header validation state for this client
            cHttpHeadersValid      = true;
            cMandatoryHeadersCount = 0;

        } else if(header->indexOf(':') >= 0) {
            String headerName  = header->substring(0, header->indexOf(':'));
            String headerValue = header->substring(header->indexOf(':') + 1);

            // remove space in the beginning (RFC2616)
            if(headerValue[0] == ' ') {
                headerValue.remove(0, 1);
            }

            if(headerName.equalsIgnoreCase(WEBSOCKETS_STRING("Connection"))) {
                headerValue.toLowerCase();
                if(headerValue.indexOf(WEBSOCKETS_STRING("upgrade")) >= 0) {
                    cIsUpgrade = true;
                }
            } else if(headerName.equalsIgnoreCase(WEBSOCKETS_STRING("Upgrade"))) {
                if(headerValue.equalsIgnoreCase(WEBSOCKETS_STRING("websocket"))) {
                    cIsWebsocket = true;
                }
            } else if(headerName.equalsIgnoreCase(WEBSOCKETS_STRING("Sec-WebSocket-Version"))) {
                cVersion = headerValue.toInt();
            } else if(headerName.equalsIgnoreCase(WEBSOCKETS_STRING("Sec-WebSocket-Key"))) {
                cKey = headerValue;
                cKey.trim();    // see rfc6455
            } else if(headerName.equalsIgnoreCase(WEBSOCKETS_STRING("Sec-WebSocket-Protocol"))) {
                cProtocol = headerValue;
            } else if(headerName.equalsIgnoreCase(WEBSOCKETS_STRING("Sec-WebSocket-Extensions"))) {
                cExtensions = headerValue;
            } else if(headerName.equalsIgnoreCase(WEBSOCKETS_STRING("Authorization"))) {
                base64Authorization = headerValue;
            } else {
                cHttpHeadersValid &= execHttpHeaderValidation(headerName, headerValue);
                if(_mandatoryHttpHeaderCount > 0 && hasMandatoryHeader(headerName)) {
                    cMandatoryHeadersCount++;
                }
            }

        } else {
            SMQTT_LOGD("[WS-Server][handleHeader] Header error (%s)", header->c_str());
        }
	}
	else
	{
        SMQTT_LOGD("[WS-Server][handleHeader] Header read fin.");
        SMQTT_LOGD("[WS-Server][handleHeader]  - cURL: %s", cUrl.c_str());
        SMQTT_LOGD("[WS-Server][handleHeader]  - cIsUpgrade: %d", cIsUpgrade);
        SMQTT_LOGD("[WS-Server][handleHeader]  - cIsWebsocket: %d", cIsWebsocket);
        SMQTT_LOGD("[WS-Server][handleHeader]  - cKey: %s", cKey.c_str());
        SMQTT_LOGD("[WS-Server][handleHeader]  - cProtocol: %s", cProtocol.c_str());
        SMQTT_LOGD("[WS-Server][handleHeader]  - cExtensions: %s", cExtensions.c_str());
        SMQTT_LOGD("[WS-Server][handleHeader]  - cVersion: %d", cVersion);
        SMQTT_LOGD("[WS-Server][handleHeader]  - base64Authorization: %s", base64Authorization.c_str());
        SMQTT_LOGD("[WS-Server][handleHeader]  - cHttpHeadersValid: %d", cHttpHeadersValid);
        SMQTT_LOGD("[WS-Server][handleHeader]  - cMandatoryHeadersCount: %d", cMandatoryHeadersCount);

		bool ok = (cIsUpgrade && cIsWebsocket);
        
        if(ok) {
            if(cUrl.length() == 0) {
                ok = false;
                SMQTT_LOGD("cUrl.length()");
            }
            if(cKey.length() == 0) {
                ok = false;
                SMQTT_LOGD("cKey.length()");
            }
            if(cVersion != 13) {
                ok = false;
                SMQTT_LOGD("cVersion");
            }
            if(!cHttpHeadersValid) {
                ok = false;
                SMQTT_LOGD("cHttpHeadersValid");
            }
            if(cMandatoryHeadersCount != _mandatoryHttpHeaderCount) {
                ok = false;
                SMQTT_LOGD("cMandatoryHeadersCount %d %d",cMandatoryHeadersCount,_mandatoryHttpHeaderCount);
            }
        }
        if(_base64Authorization.length() > 0)
		{
            String auth = WEBSOCKETS_STRING("Basic ");
            auth += _base64Authorization;
            if(auth != base64Authorization) {
                SMQTT_LOGD("[WS-Server][handleHeader] HTTP Authorization failed!");
                handleAuthorizationFailed();
                return;
            }
        }

        if(ok)
		{
            SMQTT_LOGD("[WS-Server][handleHeader] Websocket connection incoming.");

            // generate Sec-WebSocket-Accept key
            String sKey = acceptKey(cKey);

            //SMQTT_LOGD("[WS-Server][handleHeader]  - sKey: %s\n", sKey.c_str());

            status = WSC_CONNECTED;

            String handshake = WEBSOCKETS_STRING(
                "HTTP/1.1 101 Switching Protocols\r\n"
                "Server: arduino-WebSocketsServer\r\n"
                "Upgrade: websocket\r\n"
                "Connection: Upgrade\r\n"
                "Sec-WebSocket-Version: 13\r\n"
                "Sec-WebSocket-Accept: ");
            handshake += sKey + NEW_LINE;

            /*if(_origin.length() > 0) {
                handshake += WEBSOCKETS_STRING("Access-Control-Allow-Origin: ");
                handshake += _origin + NEW_LINE;
            }*/

            if(cProtocol.length() > 0) {
				String _protocol("mqtt");
                handshake += WEBSOCKETS_STRING("Sec-WebSocket-Protocol: ");
                handshake += _protocol + NEW_LINE;
            }
            // header end
            handshake += NEW_LINE;

            SMQTT_LOGD("[WS-Server][handleHeader] handshake %s", (uint8_t *)handshake.c_str());

			_client.write(handshake.c_str(), handshake.length());

            headerDone();

            // send ping
            sendFrame(WSop_ping);
        }
		else
		{
            handleNonWebsocketConnection();
        }
	}
};
void sMQTTClientWebSocket::handleNonWebsocketConnection() {
        //DEBUG_WEBSOCKETS("[WS-Server][%d][handleHeader] no Websocket connection close.\n", client->num);
        _client.write(
            "HTTP/1.1 400 Bad Request\r\n"
            "Server: arduino-WebSocket-Server\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 32\r\n"
            "Connection: close\r\n"
            "Sec-WebSocket-Version: 13\r\n"
            "\r\n"
            "This is a Websocket server only!");
        _client.stop();
        //clientDisconnect(client);
    }
String sMQTTClientWebSocket::base64_encode(uint8_t * data, size_t length)
{
    size_t size   = ((length * 1.6f) + 1);
    size          = std::max(size, (size_t)5);    // minimum buffer size
    char * buffer = (char *)malloc(size);
    if(buffer) {
        base64_encodestate _state;
        base64_init_encodestate(&_state);
        int len = base64_encode_block((const char *)&data[0], length, &buffer[0], &_state);
        len     = base64_encode_blockend((buffer + len), &_state);

        String base64 = String(buffer);
        free(buffer);
        return base64;
    }
    return String("-FAIL-");
}
String sMQTTClientWebSocket::acceptKey(String & clientKey)
{
    uint8_t sha1HashBin[20] = { 0 };
#ifdef ESP8266
    sha1(clientKey + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", &sha1HashBin[0]);
#elif defined(ESP32)
    String data = clientKey + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    esp_sha(SHA1, (unsigned char *)data.c_str(), data.length(), &sha1HashBin[0]);
#else
    clientKey += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    SHA1_CTX ctx;
    SHA1Init(&ctx);
    SHA1Update(&ctx, (const unsigned char *)clientKey.c_str(), clientKey.length());
    SHA1Final(&sha1HashBin[0], &ctx);
#endif

    String key = base64_encode(sha1HashBin, 20);
    key.trim();

    return key;
}
void sMQTTClientWebSocket::headerDone()
{
    status = WSC_CONNECTED;
    cWsRXsize = 0;
    SMQTT_LOGD("[WS][headerDone] Header Handling Done.");
}
bool sMQTTClientWebSocket::sendFrame(WSopcode_t opcode, uint8_t * payload, size_t length, bool fin, bool headerToPayload)
{
    if(status != WSC_CONNECTED)
	{
        SMQTT_LOGD("[WS][sendFrame] not in WSC_CONNECTED state!?");
        return false;
    }

    SMQTT_LOGD("[WS][sendFrame] ------- send message frame -------");
    SMQTT_LOGD("[WS][sendFrame] fin: %u opCode: %u mask: %u length: %u headerToPayload: %u", fin, opcode, cIsClient, length, headerToPayload);

    if(opcode == WSop_text) {
        SMQTT_LOGD("[WS][sendFrame] text: %s", (payload + (headerToPayload ? 14 : 0)));
    }

    uint8_t buffer[WEBSOCKETS_MAX_HEADER_SIZE] = { 0 };

    uint8_t headerSize;
    uint8_t * headerPtr;
    uint8_t * payloadPtr = payload;
    bool useInternBuffer = false;
    bool ret             = true;

    // calculate header Size
    if(length < 126) {
        headerSize = 2;
    } else if(length < 0xFFFF) {
        headerSize = 4;
    } else {
        headerSize = 10;
    }

    // set Header Pointer
    if(headerToPayload) {
        // calculate offset in payload
        headerPtr = (payloadPtr + (WEBSOCKETS_MAX_HEADER_SIZE - headerSize));
    } else {
        headerPtr = &buffer[0];
    }

    createHeader(headerPtr, opcode, length, fin);

#ifndef NODEBUG_WEBSOCKETS
    unsigned long start = micros();
#endif

    if(headerToPayload) {
        // header has be added to payload
        // payload is forced to reserved 14 Byte but we may not need all based on the length and mask settings
        // offset in payload is calculatetd 14 - headerSize
        _client.write((const char*)&payloadPtr[(WEBSOCKETS_MAX_HEADER_SIZE - headerSize)], (length + headerSize));
    } else {
        // send header
        _client.write(buffer, headerSize);

        if(payloadPtr && length > 0) {
            // send payload
            _client.write(payloadPtr, length);
        }
    }

    SMQTT_LOGD("[WS][sendFrame] sending Frame Done (%luus).", (micros() - start));
    return ret;
}
uint8_t sMQTTClientWebSocket::createHeader(uint8_t * headerPtr, WSopcode_t opcode, size_t length, bool fin)
{
    uint8_t headerSize;
    // calculate header Size
    if(length < 126) {
        headerSize = 2;
    } else if(length < 0xFFFF) {
        headerSize = 4;
    } else {
        headerSize = 10;
    }

    // create header

    // byte 0
    *headerPtr = 0x00;
    if(fin) {
        *headerPtr |= bit(7);    ///< set Fin
    }
    *headerPtr |= opcode;    ///< set opcode
    headerPtr++;

    // byte 1
    *headerPtr = 0x00;
    
    if(length < 126) {
        *headerPtr |= length;
        headerPtr++;
    } else if(length < 0xFFFF) {
        *headerPtr |= 126;
        headerPtr++;
        *headerPtr = ((length >> 8) & 0xFF);
        headerPtr++;
        *headerPtr = (length & 0xFF);
        headerPtr++;
    } else {
        // Normally we never get here (to less memory)
        *headerPtr |= 127;
        headerPtr++;
        *headerPtr = 0x00;
        headerPtr++;
        *headerPtr = 0x00;
        headerPtr++;
        *headerPtr = 0x00;
        headerPtr++;
        *headerPtr = 0x00;
        headerPtr++;
        *headerPtr = ((length >> 24) & 0xFF);
        headerPtr++;
        *headerPtr = ((length >> 16) & 0xFF);
        headerPtr++;
        *headerPtr = ((length >> 8) & 0xFF);
        headerPtr++;
        *headerPtr = (length & 0xFF);
        headerPtr++;
    }
    return headerSize;
}
void sMQTTClientWebSocket::handleWebsocketCb()
{
    uint8_t * buffer = cWsHeader;

    WSMessageHeader_t * header = &cWsHeaderDecode;
    uint8_t * payload          = NULL;

    uint8_t headerLen = 2;

    if(!handleWebsocketWaitFor(headerLen))
	{
        return;
    }

    // split first 2 bytes in the data
    header->fin    = ((*buffer >> 7) & 0x01);
    header->rsv1   = ((*buffer >> 6) & 0x01);
    header->rsv2   = ((*buffer >> 5) & 0x01);
    header->rsv3   = ((*buffer >> 4) & 0x01);
    header->opCode = (WSopcode_t)(*buffer & 0x0F);
    buffer++;

    header->mask       = ((*buffer >> 7) & 0x01);
    header->payloadLen = (WSopcode_t)(*buffer & 0x7F);
    buffer++;

    if(header->payloadLen == 126) {
        headerLen += 2;
        if(!handleWebsocketWaitFor(headerLen)) {
            return;
        }
        header->payloadLen = buffer[0] << 8 | buffer[1];
        buffer += 2;
    } else if(header->payloadLen == 127) {
        headerLen += 8;
        // read 64bit integer as length
        if(!handleWebsocketWaitFor(headerLen)) {
            return;
        }

        if(buffer[0] != 0 || buffer[1] != 0 || buffer[2] != 0 || buffer[3] != 0) {
            // really too big!
            header->payloadLen = 0xFFFFFFFF;
        } else {
            header->payloadLen = buffer[4] << 24 | buffer[5] << 16 | buffer[6] << 8 | buffer[7];
        }
        buffer += 8;
    }

    SMQTT_LOGD("[WS][handleWebsocket] ------- read massage frame -------");
    SMQTT_LOGD("[WS][handleWebsocket] fin: %u rsv1: %u rsv2: %u rsv3 %u  opCode: %u", header->fin, header->rsv1, header->rsv2, header->rsv3, header->opCode);
    SMQTT_LOGD("[WS][handleWebsocket] mask: %u payloadLen: %u", header->mask, header->payloadLen);

    if(header->payloadLen > WEBSOCKETS_MAX_DATA_SIZE)
	{
        SMQTT_LOGD("[WS][handleWebsocket] payload too big! (%u)", header->payloadLen);
        clientDisconnect(1009);
        return;
    }

    if(header->mask)
	{
        headerLen += 4;
        if(!handleWebsocketWaitFor(headerLen))
		{
            return;
        }
        header->maskKey = buffer;
        buffer += 4;
    }

    if(header->payloadLen > 0) {
        // if text data we need one more
        payload = (uint8_t *)malloc(header->payloadLen + 1);

        if(!payload) {
            SMQTT_LOGD("[WS][handleWebsocket] to less memory to handle payload %d!", header->payloadLen);
            clientDisconnect(1011);
            return;
        }
        //readCb(payload, header->payloadLen, std::bind(&sMQTTClientWebSocket::handleWebsocketPayloadCb, std::placeholders::_1, payload));
		if(readCb(payload, header->payloadLen,0))
        {
            handleWebsocketPayloadCb(true, payload);
        }
    }
	else
	{
        handleWebsocketPayloadCb(true, NULL);
    }
}
bool sMQTTClientWebSocket::handleWebsocketWaitFor(size_t size)
{
    if(size > WEBSOCKETS_MAX_HEADER_SIZE)
	{
        SMQTT_LOGD("[WS][handleWebsocketWaitFor] size: %d too big!", size);
        return false;
    }

    if(cWsRXsize >= size)
	{
        return true;
    }

    SMQTT_LOGD("[WS][handleWebsocketWaitFor] size: %d cWsRXsize: %d", size, cWsRXsize);
    /*readCb(&cWsHeader[cWsRXsize], (size - cWsRXsize), std::bind([](WebSockets * server, size_t size, WSclient_t * client, bool ok) {
        //DEBUG_WEBSOCKETS("[WS][%d][handleWebsocketWaitFor][readCb] size: %d ok: %d\n", client->num, size, ok);
        if(ok) {
            cWsRXsize = size;
            server->handleWebsocketCb(client);
        } else {
            //DEBUG_WEBSOCKETS("[WS][%d][readCb] failed.\n", client->num);
            cWsRXsize = 0;
            // timeout or error
            server->clientDisconnect(client, 1002);
        }
    },
                                                                                          this, size, std::placeholders::_1, std::placeholders::_2));*/
	if(readCb(&cWsHeader[cWsRXsize], (size - cWsRXsize),0))
    {
        cWsRXsize = size;
        handleWebsocketCb();
    }
    return false;
}
bool sMQTTClientWebSocket::readCb(uint8_t * out, size_t n, WSreadWaitCb cb)
{
    unsigned long t = millis();
    ssize_t len;
    SMQTT_LOGD("[readCb] n: %zu t: %lu", n, t);
    while(n > 0) {
        /*if(client->tcp == NULL) {
            DEBUG_WEBSOCKETS("[readCb] tcp is null!\n");
            if(cb) {
                cb(client, false);
            }
            return false;
        }*/

        /*if(!_client.connected()) {
            SMQTT_LOGD("[readCb] not connected!");
            if(cb) {
                cb(false);
            }
            return false;
        }*/

        if((millis() - t) > WEBSOCKETS_TCP_TIMEOUT) {
            SMQTT_LOGD("[readCb] receive TIMEOUT! %lu", (millis() - t));
            if(cb) {
                cb(false);
            }
            return false;
        }

        if(!_client.available())
		{
            WEBSOCKETS_YIELD_MORE();
            continue;
        }

        len = _client.read((uint8_t *)out, n);
        if(len > 0) {
            t = millis();
            out += len;
            n -= len;
           // SMQTT_LOGD("Receive %d left %d!\n", len, n);
        } else {
			//SMQTT_LOGD("Receive %d left %d!\n", len, n);
        }
        if(n > 0) {
            WEBSOCKETS_YIELD();
        }
    }
    if(cb) {
        //cb(true);

    }
    WEBSOCKETS_YIELD();
//#endif
    return true;
}
void sMQTTClientWebSocket::handleWebsocketPayloadCb(bool ok, uint8_t * payload)
{
    WSMessageHeader_t * header = &cWsHeaderDecode;
    if(ok)
	{
        if(header->payloadLen > 0) {
            payload[header->payloadLen] = 0x00;

            if(header->mask) {
                // decode XOR
                for(size_t i = 0; i < header->payloadLen; i++) {
                    payload[i] = (payload[i] ^ header->maskKey[i % 4]);
                }
            }
        }

        switch(header->opCode) {
            case WSop_text:
                SMQTT_LOGD("[WS][handleWebsocket] text: %s", payload);
                // fallthrough
            case WSop_binary:
                SMQTT_LOGD("[WS][handleWebsocket] binary");
                messageReceived(header->opCode, payload, header->payloadLen, header->fin);
                break;
            case WSop_continuation:
                SMQTT_LOGD("[WS][handleWebsocket] continuation");
                messageReceived(header->opCode, payload, header->payloadLen, header->fin);
                break;
            case WSop_ping:
                // send pong back
                SMQTT_LOGD("[WS][handleWebsocket] ping received (%s)", payload ? (const char *)payload : "");
                sendFrame(WSop_pong, payload, header->payloadLen);
                messageReceived(header->opCode, payload, header->payloadLen, header->fin);
                break;
            case WSop_pong:
                SMQTT_LOGD("[WS][handleWebsocket] get pong (%s)", payload ? (const char *)payload : "");
                //pongReceived = true;
                //messageReceived(header->opCode, payload, header->payloadLen, header->fin);
                break;
            case WSop_close: {
#ifndef NODEBUG_WEBSOCKETS
                uint16_t reasonCode = 1000;
                if(header->payloadLen >= 2) {
                    reasonCode = payload[0] << 8 | payload[1];
                }
#endif
                SMQTT_LOGD("[WS][handleWebsocket] get ask for close. Code: %d", reasonCode);
                if(header->payloadLen > 2) {
                    SMQTT_LOGD(" (%s)\n", (payload + 2));
                } else {
                    SMQTT_LOGD("\n");
                }
                clientDisconnect(1000);
            } break;
            default:
                SMQTT_LOGD("[WS][handleWebsocket] got unknown opcode: %d", header->opCode);
                clientDisconnect(1002);
                break;
        }

        if(payload) {
            free(payload);
        }

        // reset input
        cWsRXsize = 0;
    }
	else
	{
        SMQTT_LOGD("[WS][handleWebsocket] missing data!");
        free(payload);
        clientDisconnect(1002);
    }
}
void sMQTTClientWebSocket::handleWebsocket()
{
    if(cWsRXsize == 0)
	{
        handleWebsocketCb();
    }
}
bool sMQTTClientWebSocket::hasMandatoryHeader(String headerName) {
    for(size_t i = 0; i < _mandatoryHttpHeaderCount; i++) {
        //if(_mandatoryHttpHeaders[i].equalsIgnoreCase(headerName))
        //    return true;
    }
    return false;
}
void sMQTTClientWebSocket::messageReceived(WSopcode_t opcode, uint8_t * payload, size_t length, bool fin)
{
    SMQTT_LOGD("opcode:%d length:%d", opcode, length);
	switch(opcode)
	{
    case WSop_text:
    case WSop_binary:
    case WSop_continuation:
		for(int index=0;index<length;index++)
		{
			message.incoming(payload[index]);
            SMQTT_LOGD("payload:%c", payload[index]);
			if (message.type())
			{
				processMessage();
				message.reset();
				break;
			}
		}
		break;
	}
};