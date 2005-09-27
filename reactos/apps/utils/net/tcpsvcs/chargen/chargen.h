#define START 32
#define END 126
#define LINESIZ 72
#define BUF 1024

DWORD WINAPI ChargenHandler(VOID* Sock_);
BOOL GenerateChars(SOCKET Sock);
BOOL SendChar(SOCKET Sock, CHAR c);
