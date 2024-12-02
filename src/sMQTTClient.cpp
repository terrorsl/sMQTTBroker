#include "sMQTTBroker.h"
#include <libb64/cencode.h>

#include <functional>

sMQTTClient::sMQTTClient(sMQTTBroker *parent, TCPClient &client):mqtt_connected(false), _parent(parent)
{
	_client = client;
	keepAlive = 25;
	updateLiveStatus();
};
sMQTTClient::~sMQTTClient()
{
	//SMQTT_LOGD("free _client");
	//delete _client;
};
void sMQTTClient::update()
{
	while (_client.available()>0)
	{
		message.incoming(_client.read());
		if (message.type())
		{
			processMessage();
			message.reset();
			break;
		}
	}
	unsigned long currentMillis;
#if defined(ESP8266) || defined(ESP32)
	currentMillis = millis();
#endif
	if (keepAlive != 0 && aliveMillis < currentMillis)
	{
		SMQTT_LOGD("aliveMillis(%lu) < currentMillis(%lu)", aliveMillis, currentMillis);
		_client.stop();
	}
	//else
	//	SMQTT_LOGD("time %d", aliveMillis - currentMillis);
};
bool sMQTTClient::isConnected()
{
	return _client.connected();
};
void sMQTTClient::write(const char* buf, size_t length)
{
	_client.write(buf, length);
}
void sMQTTClient::processMessage()
{
	if (message.type() <= sMQTTMessage::Type::Disconnect)
	{
		SMQTT_LOGD("message type:%s(0x%x)", debugMessageType[message.type() / 0x10], message.type());
	}

	const char *header = message.getVHeader();
	switch (message.type())
	{
	case sMQTTMessage::Type::Connect:
		{
			if (mqtt_connected)
			{
				_client.stop();
				break;
			}
			unsigned char status = 0;
			if (strncmp("MQTT", header + 2, 4))
			{
				//TODO: close connection
			}
			if (header[6] != 0x04)
			{
				status = sMQTTConnReturnUnacceptableProtocolVersion;
				// Level 3.1.1
			}
			else
			{
				unsigned short len;
				mqtt_flags = header[7];
				keepAlive = (header[8] << 8) | header[9];

				const char *payload = &header[10];
				message.getString(payload, len);
				clientId = std::string(payload,len);
				payload += len;

				SMQTT_LOGD("message clientId:%s", clientId.c_str());
				SMQTT_LOGD("message keepTime:%d", keepAlive);

				if (mqtt_flags&sMQTTWillFlag)
				{
					//topic
					message.getString(payload, len);
					//willTopic = std::string(payload, len);
					payload += len;
					//message
					message.getString(payload, len);
					//willMessage = std::string(payload, len);
					payload += len;
				}
				std::string username;
				if (mqtt_flags&sMQTTUserNameFlag)
				{
					message.getString(payload, len);

					username = std::string(payload, len);
					SMQTT_LOGD("message user:%s", username.c_str());

					payload += len;
				}
				std::string password;
				if (mqtt_flags&sMQTTPasswordFlag)
				{
					message.getString(payload, len);
					password = std::string(payload, len);
					SMQTT_LOGD("message password:%s", password.c_str());
					payload += len;
				}

				if (_parent->isClientConnected(this) == false)
				{
					sMQTTNewClientEvent event(this, username, password);
					if(_parent->onEvent(&event)==false)
						status = sMQTTConnReturnBadUsernameOrPassword;
				}
				else
					status = sMQTTConnReturnIdentifierRejected;
			}

			sMQTTMessage msg(sMQTTMessage::Type::ConnAck);
			msg.add(0);	// Session present (not implemented)
			msg.add(status); // Connection accepted
			msg.sendTo(this);

			if (status)
				_client.stop();
			else
				mqtt_connected = true;
		}
		break;
	case sMQTTMessage::Type::Publish:
		{
			unsigned char qos = message.QoS();

			unsigned short len;
			const char *payload = header;
			message.getString(payload, len);

			const char *topicName = payload;
			std::string _topicName(topicName, len);
			payload += len;

			char packeteIdent[2]={0};
			if (qos)
			{
				packeteIdent[0] = payload[0];
				packeteIdent[1] = payload[1];
				payload += 2;
			}
			len = message.end() - payload;
			std::string _payload(payload, len);

			sMQTTTopic topic(_topicName,_payload, qos);

			if (message.isRetained())
				_parent->updateRetainedTopic(&topic);

			switch (qos)
			{
			case 1:
				{
					sMQTTMessage msg(sMQTTMessage::Type::PubAck);
					msg.add(packeteIdent[0]);
					msg.add(packeteIdent[1]);
					msg.sendTo(this);
				}
				break;
			case 2:
				{
					sMQTTMessage msg(sMQTTMessage::Type::PubRec);
					msg.add(packeteIdent[0]);
					msg.add(packeteIdent[1]);
					msg.sendTo(this);
				}
				break;
			}

			_parent->publish(this,&topic, &message);
		}
		break;
	case sMQTTMessage::Type::PubAck:
		{
		}
		break;
	case sMQTTMessage::Type::PubRec:
		{
			const char *payload = header;
			sMQTTMessage msg(sMQTTMessage::Type::PubRel);
			msg.add(payload[0]);
			msg.add(payload[1]);
			msg.sendTo(this);
		}
		break;
	case sMQTTMessage::Type::PubRel:
		{
			const char *payload = header;
			sMQTTMessage msg(sMQTTMessage::Type::PubComp);
			msg.add(payload[0]);
			msg.add(payload[1]);
			msg.sendTo(this);
		}
		break;
	case sMQTTMessage::Type::PubComp:
		{

		}
		break;
	case sMQTTMessage::Type::Subscribe:
		{
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
			unsigned short msg_id = (header[0] << 8) | header[1];
			SMQTT_LOGD("message id:%d", msg_id);
#endif
			const char *payload = header + 2;
			std::vector<char> qoss;
			while (payload < message.end())
			{
				unsigned short len;
				message.getString(payload, len);	// Topic

				std::string topic(payload, len);
				SMQTT_LOGD("message topic:%s", topic.c_str());
				payload += len;
				unsigned char qos = *payload++;

				if (_parent->subscribe(this, topic.c_str()) == false)
				{
					SMQTT_LOGD("subscribe failed");
					qos = 0x80;
				}
				qoss.push_back(qos);
			}
			sMQTTMessage msg(sMQTTMessage::Type::SubAck);
			msg.add(header[0]);
			msg.add(header[1]);
			for (int i = 0; i<qoss.size(); i++)
				msg.add(qoss[i]);
			msg.sendTo(this);
		}
		break;
	case sMQTTMessage::Type::UnSubscribe:
		{
			//unsigned short msg_id = (header[0] << 8) | header[1];
			//SMQTT_LOGD("message id:%d", msg_id);
			const char *payload = header + 2;
			while (payload < message.end())
			{
				unsigned short len;
				message.getString(payload, len);	// Topic

				//SMQTT_LOGD("message topic:%s", std::string(payload, len).c_str());
				_parent->unsubscribe(this, std::string(payload, len).c_str());

				payload += len;
				//unsigned char qos = *payload++;
			}
			sMQTTMessage msg(sMQTTMessage::Type::UnSuback);
			msg.add(header[0]);
			msg.add(header[1]);
			msg.sendTo(this);
		}
		break;
	case sMQTTMessage::Type::Disconnect:
		{
			mqtt_connected = false;
			_client.stop();
		}
		break;
	case sMQTTMessage::Type::PingReq:
		{
			sMQTTMessage msg(sMQTTMessage::Type::PingResp);
			msg.sendTo(this);
		}
		break;
	default:
		{
			SMQTT_LOGD("unknown message %d", message.type());
			mqtt_connected = false;
			_client.stop();
		}
		break;
	}
	updateLiveStatus();
};
void sMQTTClient::updateLiveStatus()
{
	if (keepAlive)
#if defined(ESP8266) || defined(ESP32)
		//aliveMillis = (keepAlive*1.5) * 1000 + millis();
		aliveMillis = keepAlive*1500 + millis();
#else
		aliveMillis = 0;
#endif
	else
		aliveMillis = 0;
};

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
        //SMQTT_LOGD("%d",len);
		switch(status)
		{
		case WSC_HEADER:
			{
				String header = _client.readStringUntil('\n');
                //SMQTT_LOGD("%s", header.c_str());
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

            //write(client, (uint8_t *)handshake.c_str(), handshake.length());
			_client.write(handshake.c_str(), handshake.length());

            headerDone();

            // send ping
            sendFrame(WSop_ping);

            //runCbEvent(client->num, WStype_CONNECTED, (uint8_t *)client->cUrl.c_str(), client->cUrl.length());

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
/*#if(WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP8266_ASYNC)
    client->cHttpLine = "";
    handleWebsocket(client);
#endif*/
}
bool sMQTTClientWebSocket::sendFrame(WSopcode_t opcode, uint8_t * payload, size_t length, bool fin, bool headerToPayload)
{
    /*if(client->tcp && !client->tcp->connected()) {
        DEBUG_WEBSOCKETS("[WS][%d][sendFrame] not Connected!?\n", client->num);
        return false;
    }*/

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

    uint8_t maskKey[4]                         = { 0x00, 0x00, 0x00, 0x00 };
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

    if(cIsClient)
	{
        headerSize += 4;
    }

#ifdef WEBSOCKETS_USE_BIG_MEM
    // only for ESP since AVR has less HEAP
    // try to send data in one TCP package (only if some free Heap is there)
    if(!headerToPayload && ((length > 0) && (length < 1400)) && (GET_FREE_HEAP > 6000)) {
        DEBUG_WEBSOCKETS("[WS][%d][sendFrame] pack to one TCP package...\n", client->num);
        uint8_t * dataPtr = (uint8_t *)malloc(length + WEBSOCKETS_MAX_HEADER_SIZE);
        if(dataPtr) {
            memcpy((dataPtr + WEBSOCKETS_MAX_HEADER_SIZE), payload, length);
            headerToPayload = true;
            useInternBuffer = true;
            payloadPtr      = dataPtr;
        }
    }
#endif

    // set Header Pointer
    if(headerToPayload) {
        // calculate offset in payload
        headerPtr = (payloadPtr + (WEBSOCKETS_MAX_HEADER_SIZE - headerSize));
    } else {
        headerPtr = &buffer[0];
    }

    if(cIsClient && useInternBuffer)
	{
        // if we use a Intern Buffer we can modify the data
        // by this fact its possible the do the masking
        for(uint8_t x = 0; x < sizeof(maskKey); x++) {
            maskKey[x] = random(0xFF);
        }
    }

    createHeader(headerPtr, opcode, length, cIsClient, maskKey, fin);

    if(cIsClient && useInternBuffer)
	{
        uint8_t * dataMaskPtr;

        if(headerToPayload) {
            dataMaskPtr = (payloadPtr + WEBSOCKETS_MAX_HEADER_SIZE);
        } else {
            dataMaskPtr = payloadPtr;
        }

        for(size_t x = 0; x < length; x++) {
            dataMaskPtr[x] = (dataMaskPtr[x] ^ maskKey[x % 4]);
        }
    }

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
        //_client.write((const char*)&buffer[0], headerSize);
        _client.write(buffer, headerSize);

        if(payloadPtr && length > 0) {
            // send payload
            //_client.write((const char*)&payloadPtr[0], length);
            _client.write(payloadPtr, length);
        }
    }

    SMQTT_LOGD("[WS][sendFrame] sending Frame Done (%luus).", (micros() - start));

#ifdef WEBSOCKETS_USE_BIG_MEM
    if(useInternBuffer && payloadPtr) {
        free(payloadPtr);
    }
#endif

    return ret;
}
uint8_t sMQTTClientWebSocket::createHeader(uint8_t * headerPtr, WSopcode_t opcode, size_t length, bool mask, uint8_t maskKey[4], bool fin)
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

    if(mask) {
        headerSize += 4;
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
    if(mask) {
        *headerPtr |= bit(7);    ///< set mask
    }

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

    if(mask) {
        *headerPtr = maskKey[0];
        headerPtr++;
        *headerPtr = maskKey[1];
        headerPtr++;
        *headerPtr = maskKey[2];
        headerPtr++;
        *headerPtr = maskKey[3];
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
/*#if(WEBSOCKETS_NETWORK_TYPE == NETWORK_ESP8266_ASYNC)
        // register callback for next message
        handleWebsocketWaitFor(2);
#endif*/
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