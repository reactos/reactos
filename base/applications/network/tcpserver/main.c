#include <stdio.h>
#include <WinSock2.h>

int wmain(int argc, LPWSTR argv[]) {
	WSADATA wsa;
	SOCKET sock;
	SOCKET accepted;
	struct sockaddr_in server;
	struct sockaddr_in client;

	int c;
	int ret;
	
	char buff[256];

	memset(&server, 0, sizeof(struct sockaddr_in));
	memset(&client, 0, sizeof(struct sockaddr_in));

	ret = WSAStartup(MAKEWORD(2, 2), &wsa);
	if (ret != 0) {
		printf("Windows Socket API Startup Failed: %d", WSAGetLastError());
		exit(1);
	}

	printf("Attempt socket creation\n");
	fgets(&buff[0], 255, stdin);
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) {
		printf("Socket Creation Failed: %d", WSAGetLastError());
		exit(1);
	}

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_port = htons(10000);
	
	printf("Attempt bind\n");
	fgets(&buff[0], 255, stdin);
	ret = bind(sock, (struct sockaddr *)&server, sizeof(server));
	if (ret == SOCKET_ERROR) {
		printf("Socket Bind Failed: %d", WSAGetLastError());
		exit(1);
	}

	printf("Attempt listen\n");
	fgets(&buff[0], 255, stdin);
	ret = listen(sock, 3);
	if (ret != 0) {
		printf("Socket Listen Failed: %d", WSAGetLastError());
		exit(1);
	}

	printf("Listening for connections on LOOPBACK port 10000\n");
	
	c = sizeof(struct sockaddr_in);

	printf("Enter accept loop\n");
	fgets(&buff[0], 255, stdin);
	while(1) {
		accepted = accept(sock, (struct sockaddr *)&client, &c);
		if (accepted == INVALID_SOCKET) {
			printf("Socket Connection Acceptance Failed: %d", WSAGetLastError());
			exit(1);
		} else {
			printf("Socket connection accepted\n");
			ret = recv(accepted, &buff[0], 256, MSG_OOB);
			printf("Received %d bytes\n", ret);
			printf("Message: %s", &buff[0]);
		}
	}

	exit(1);
}