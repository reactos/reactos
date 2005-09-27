#define BUF 1024

DWORD WINAPI StartServer(LPVOID lpParam);
SOCKET SetUpListener(const char* ServAddr, int Port);
VOID AcceptConnections(SOCKET ListeningSocket, LPTHREAD_START_ROUTINE Service);
BOOL EchoIncomingPackets(SOCKET sd);
BOOL ShutdownConnection(SOCKET Sock);
