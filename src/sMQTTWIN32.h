#ifndef SMQTT_WIN32_FILE
#define SMQTT_WIN32_FILE

#include<string>
#include<list>
#include<map>
#include <winsock2.h>
#include<WS2tcpip.h>
#include<sysinfoapi.h>

#pragma comment(lib, "ws2_32.lib")

#if defined(DEBUG) || defined(_DEBUG)
#define SMQTT_LOGD(...) fprintf(stdout,__VA_ARGS__)
#else
#define SMQTT_LOGD
#endif

DWORD WINAPI sMQTTAcceptThread(LPVOID lpParam);

class sMQTTWIN32Client : public sMQTTOSClient
{
public:
	sMQTTWIN32Client(SOCKET _s) :_socket(_s) { is_connected=true; }
	sMQTTWIN32Client(sMQTTWIN32Client &other) { _socket = other._socket; is_connected=true; }

	int available() { update(); return bytes.size(); }
	unsigned char read() { unsigned char byte = bytes.front(); bytes.pop_front(); return byte; }
	void write(const char *buffer, unsigned long size) { send(_socket, buffer, size, 0); }
	void stop() {}
	bool connected() { return is_connected; }

	void update() {
		FD_SET read,excep; FD_ZERO(&read); FD_ZERO(&excep); FD_SET(_socket, &read); FD_SET(_socket, &excep); TIMEVAL time; time.tv_sec = 0; time.tv_usec = 0;
		select(0, &read, 0, &excep, &time);
		if (FD_ISSET(_socket, &excep))
		{
			is_connected = false;
		}
		if (FD_ISSET(_socket, &read))
		{
			char buf[1024];
			int size = recv(_socket, buf, sizeof(buf), 0);
			for (int i = 0; i < size; i++)
				bytes.push_back(buf[i]);
		}
	}
private:
	bool is_connected;
	SOCKET _socket;
	std::list<unsigned char> bytes;
};

class sMQTTWIN32Server :public sMQTTOSServer
{
public:
	sMQTTWIN32Server(unsigned short port) {
		WSADATA wsaData;
		WSAStartup(MAKEWORD(2, 2), &wsaData);

		_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		struct sockaddr_in saServer;

		saServer.sin_family = AF_INET;
		saServer.sin_addr.s_addr = inet_addr("0.0.0.0");
		saServer.sin_port = htons(port);
		int status = bind(_socket, (SOCKADDR*)&saServer, sizeof(saServer));
		listen(_socket, SOMAXCONN);

		addrinfo adr,*res;
		memset(&adr, 0, sizeof(adr));
		adr.ai_family = AF_INET;
		adr.ai_socktype = SOCK_STREAM;
		adr.ai_protocol = IPPROTO_TCP;
		if (getaddrinfo(NULL, "1883", &adr, &res) == 0)
		{
			switch (res->ai_family)
			{
			case AF_INET:
				{
					struct sockaddr_in  *sockaddr_ipv4;
					SMQTT_LOGD("Family: AF_INET (IPv4)\n");
					sockaddr_ipv4 = (struct sockaddr_in *) res->ai_addr;
					SMQTT_LOGD("IPv4 address %s\n",
						inet_ntoa(sockaddr_ipv4->sin_addr));
				}
				break;
			}
			freeaddrinfo(res);
		}

		is_connected = true;

		DWORD id;
		CreateThread(0, 0, sMQTTAcceptThread, this, 0, &id);
	};
	~sMQTTWIN32Server() {
		WSACleanup();
	}

	SOCKET getSocket() { return _socket; }
	void accept(SOCKET _s) { accepts.push_back(_s); }

	void begin() {}
	void end() {
		is_connected = false; shutdown(_socket, SD_BOTH); closesocket(_socket);
	}

	sMQTTOSClient *available() {
		if(accepts.empty())
			return 0;
		SOCKET _s = accepts.front();
		accepts.pop_front();
		std::pair<std::map<SOCKET, sMQTTWIN32Client>::iterator, bool> ret;
		ret=clients.insert(std::make_pair(_s, sMQTTWIN32Client(_s)));
		sMQTTWIN32Client *client = &ret.first->second;
		return client;
	}
	bool isConnected() { return is_connected; }
private:
	SOCKET _socket;
	bool is_connected;
	std::list<SOCKET> accepts;
	std::map<SOCKET, sMQTTWIN32Client> clients;
};

DWORD WINAPI sMQTTAcceptThread(LPVOID lpParam)
{
	sMQTTWIN32Server *server = (sMQTTWIN32Server*)lpParam;
	for (;;)
	{
		sockaddr *addr;
		SOCKET _s = accept(server->getSocket(), 0, 0);
		if (_s == INVALID_SOCKET)
			ExitThread(0);
		server->accept(_s);
	}
	return 0;
};

sMQTTOSServer *newsMQTTServer(unsigned short port) {
	return new sMQTTWIN32Server(port);
}
#endif