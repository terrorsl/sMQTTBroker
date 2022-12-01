#ifndef SMQTT_SOCKET_FILE
#define SMQTT_SOCKET_FILE

class sMQTTOSClient
{
public:
	virtual int available() = 0;
	virtual unsigned char read() = 0;
	virtual void write(const char *buffer, unsigned long size) = 0;
	virtual void stop() = 0;
	virtual bool connected() = 0;
};

class sMQTTOSServer
{
public:
	virtual void begin() = 0;
	virtual void end() = 0;

	virtual sMQTTOSClient *available() = 0;
	virtual bool isConnected() = 0;
};
#endif