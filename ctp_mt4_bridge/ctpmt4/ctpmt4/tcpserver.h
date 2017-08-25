#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <string>
#include <stdio.h>  
#include <Winsock2.h>
#pragma comment(lib,"WS2_32.lib")  

class tcpserver
{
public:
	tcpserver(int port)
	{
		WORD wVersionRequested;
		WSADATA wsaData;
		int err;

		wVersionRequested = MAKEWORD(1, 1);

		err = WSAStartup(wVersionRequested, &wsaData);

		if (err != 0)
		{
			return;
		}

		if (LOBYTE(wsaData.wVersion) != 1 ||
			HIBYTE(wsaData.wVersion) != 1)
		{
			WSACleanup();
			return;
		}

		sockSrv = socket(AF_INET, SOCK_STREAM, 0);

		SOCKADDR_IN addrSrv;
		addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
		addrSrv.sin_family = AF_INET;
		addrSrv.sin_port = htons(port);
		bind(sockSrv, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));
		listen(sockSrv, 5);
	}

	void sendmsg(const std::string& info)
	{
		SOCKADDR_IN addrClient;
		int len = sizeof(SOCKADDR);
		SOCKET sockConn = accept(sockSrv, (SOCKADDR*)&addrClient, &len);
		send(sockConn, info.c_str(), info.length() + 1, 0);
		closesocket(sockConn);
	}

	SOCKET sockSrv;
};
