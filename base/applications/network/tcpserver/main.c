#include <stdio.h>
#include <winsock2.h>

#define backlog 4
#define LENGTH 256

struct ClientInfo {
    int ClientID;
    SOCKET ClientSock;
};

DWORD WINAPI ServerThreadMain(LPVOID lpParam) {
    SOCKET ClientSock;

    struct ClientInfo *CInfo;

    char buff[LENGTH];

    int count;
    int ret;

    CInfo = (struct ClientInfo *)lpParam;

    count = CInfo->ClientID;
    ClientSock = CInfo->ClientSock;

    printf("Connection established to client %d\n", count);

    ret = recv(ClientSock, buff, LENGTH, 0);
    if (ret < 0) {
        printf("  Receive failure on socket %d: %d\n", ClientSock, WSAGetLastError());
        return 1;
    }
    buff[ret] = '\0';
    printf("  Received %d-byte message from client %d: %s\n", ret, count, buff);

    ret = send(ClientSock, buff, ret, 0);
    if (ret == SOCKET_ERROR) {
        printf("Send failed: %d\n", WSAGetLastError());
        closesocket(ClientSock);
        fgets(buff, LENGTH, stdin);
        return 1;
    }
    printf("Message echoed\n");

    closesocket(ClientSock);

    free(CInfo);

    return 0;
}

int wmain(int argc, LPWSTR argv[]) {
    DWORD ServerThreadID;
    WSADATA wsa;
    SOCKET ListenSock;
    SOCKET ClientSock;

    struct ClientInfo *CInfo;

    struct sockaddr_in ListenAddr;
    struct sockaddr_in ClientAddr;

    char buff[LENGTH];

    int ret;
    int addrSize;
    int count;

    printf("Server startup\n");

    ret = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (ret != 0) {
        printf("Windows Socket API Startup Failed: %d\n", WSAGetLastError());
        fgets(buff, LENGTH, stdin);
        return 1;
    }

    memset(&ListenAddr, 0, sizeof(struct sockaddr_in));
    ListenAddr.sin_family = AF_INET;
    ListenAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    ListenAddr.sin_port = htons(10000);

    ListenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ListenSock == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        WSACleanup();
        fgets(buff, LENGTH, stdin);
        return 1;
    }
    printf("Server listening socket created\n");

    ret = bind(ListenSock, (struct sockaddr *)&ListenAddr, sizeof(ListenAddr));
    if (ret == SOCKET_ERROR) {
        printf("Bind failed: %d\n", WSAGetLastError());
        closesocket(ListenSock);
        WSACleanup();
        fgets(buff, LENGTH, stdin);
        return 1;
    }
    printf("Server listening socket bound\n");

    ret = listen(ListenSock, backlog);
    if (ret == SOCKET_ERROR) {
        printf("Listen failed: %d\n", WSAGetLastError());
        closesocket(ListenSock);
        WSACleanup();
        fgets(buff, LENGTH, stdin);
        return 1;
    }
    printf("Server listening socket is now listening\n");

    count = 0;
    while (1) {
        addrSize = sizeof(struct sockaddr_in);
        memset(buff, 0, sizeof(buff));
        printf("\nWaiting to accept connection %d\n", count);
        ClientSock = accept(ListenSock, (struct sockaddr *)&ClientAddr, &addrSize);
        if (ClientSock == INVALID_SOCKET) {
            printf("Accept failed: %d\n", WSAGetLastError());
            closesocket(ListenSock);
            WSACleanup();
            fgets(buff, LENGTH, stdin);
            exit(1);
        }
        
        CInfo = malloc(sizeof(*CInfo));
        CInfo->ClientID = count;
        CInfo->ClientSock = ClientSock;

        CreateThread(
            NULL,
            0,
            ServerThreadMain,
            (LPVOID)CInfo,
            0,
            &ServerThreadID);

        count++;
    }

    printf("Server exit\n");

    fgets(buff, LENGTH, stdin);

    return 0;
}