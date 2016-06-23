#include <stdlib.h>
#include <stdio.h>
#include <winsock2.h>

int wmain(int argc, LPWSTR argv[]) {
	WSADATA wsa;
	SOCKET sock;
	struct sockaddr_in server;

	int ret;
	
	char buff[256];

	ret = WSAStartup(MAKEWORD(2, 2), &wsa);
	if (ret != 0) {
		printf("Windows Socket API Startup Failed: %d", WSAGetLastError());
		return 1;
	}

	printf("Attempt socket creation\n");
	fgets(&buff[0], 255, stdin);
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) {
		printf("Socket Creation Failed: %d", WSAGetLastError());
		return 1;
	}
	
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_family = AF_INET;
	server.sin_port = htons(10000);

	printf("Attempt connect\n");
	fgets(&buff[0], 255, stdin);
	ret = connect(sock, (struct sockaddr *)&server, sizeof(server));
	if (ret < 0) {
		printf("Socket Connection Failed: %d", WSAGetLastError());
		return 1;
	}

	printf("Connected\n");

	fgets(&buff[0], 255, stdin);
	ret = send(sock, &buff[0], 256, MSG_OOB);
	printf("Sent %d bytes", ret);
	
	fgets(&buff[0], 255, stdin);
	
	return 0;
}