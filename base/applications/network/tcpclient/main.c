#include <stdlib.h>
#include <stdio.h>
#include <winsock2.h>

#define LENGTH 255
#define NUM_CLIENTS 1

DWORD WINAPI ClientThreadMain(LPVOID lpParam) {
    SOCKET Sock;

    struct sockaddr_in ServerAddr;

    char buff[LENGTH];

    int self;
    int ret;
    int len;

    self = (int)lpParam;

    printf("Client %d startup\n", self);

    Sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (Sock == INVALID_SOCKET) {
        printf("Client %d failed to create socket: %d\n", self, WSAGetLastError());
        return 1;
    }

    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    ServerAddr.sin_port = htons(10000);

	printf("Client %d attempting to connect\n", self);
    ret = connect(Sock, (struct sockaddr *)&ServerAddr, sizeof(struct sockaddr_in));
    if (ret == SOCKET_ERROR) {
        printf("Client %d failed to connect: %d\n", self, WSAGetLastError());
        closesocket(Sock);
        return 1;
    }

    sprintf(buff, "Client %d pinging server", self);
    len = strlen(buff);
	printf("Client %d attempting to send\n", self);
    ret = send(Sock, buff, len, 0);
    if (ret != len) {
        printf("Client %d failed to send properly. Should send %d bytes, send() returned %d\n  WSA Error: %d\n",
            self, len, ret, WSAGetLastError());
        closesocket(Sock);
        return 1;
    }

	printf("Client %d attempting to receive\n", self);
    ret = recv(Sock, buff, LENGTH, 0);
    buff[LENGTH - 1] = '\0';
    if (ret <= 0) {
        printf("Client %d received no response from server: %d\n", self, WSAGetLastError());
    }
    else {
        printf("Client %d received %d-byte response from server:\n  %s\n",
            self, ret, buff);
    }

    closesocket(Sock);

    printf("Client %d exit\n", self);

    return 0;
}

int wmain(int argc, LPWSTR argv[]) {
    WSADATA wsa;
    HANDLE ClientThreadHandles[NUM_CLIENTS];
    DWORD ClientThreadIDs[NUM_CLIENTS];

    char buff[LENGTH];

    int ret;
    int i;

    ret = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (ret != 0) {
        printf("Windows Socket API Startup Failed: %d\n", WSAGetLastError());
        return 1;
    }

    for (i = 0; i < NUM_CLIENTS; i++) {
        ClientThreadHandles[i] = CreateThread(
            NULL,
            0,
            ClientThreadMain,
            (LPVOID)i,
            0,
            &ClientThreadIDs[i]);

        if (ClientThreadHandles[i] == NULL) {
            printf("Thread %d failed to spawn\n", i);
            return 1;
        }
    }

    WaitForMultipleObjects(NUM_CLIENTS, ClientThreadHandles, TRUE, INFINITE);

    for (i = 0; i < NUM_CLIENTS; i++) {
        CloseHandle(ClientThreadHandles[i]);
    }

    fgets(buff, LENGTH, stdin);

    return 0;
}