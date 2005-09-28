/* default port numbers */
#define ECHO_PORT 7
#define CHARGEN_PORT 19
#define DAYTIME_PORT 13
#define DISCARD_PORT 9
#define QOTD_PORT 17

#define NUM_SERVICES 6
#define BUF_SIZE 255
#define BUF 1024

/* RFC865 states no more than 512 chars per line */
#define MAX_QUOTE_BUF 512

/* printable ASCII's characters for chargen */
#define START 32
#define END 126

/* number of chars to put on a line */
#define LINESIZ 72

/* data structure to pass to threads */
typedef struct _Services {
    INT Port;
    LPTHREAD_START_ROUTINE Service;
} SERVICES, *PSERVICES;

/* skelserver functions */
DWORD WINAPI StartServer(LPVOID lpParam);
SOCKET SetUpListener(const char* ServAddr, int Port);
VOID AcceptConnections(SOCKET ListeningSocket, LPTHREAD_START_ROUTINE Service);
BOOL EchoIncomingPackets(SOCKET sd);
BOOL ShutdownConnection(SOCKET Sock, BOOL bRec);

/* chargen functions */
DWORD WINAPI ChargenHandler(VOID* Sock_);
BOOL GenerateChars(SOCKET Sock);
BOOL SendChar(SOCKET Sock, CHAR c);

/* daytime functions */
DWORD WINAPI DaytimeHandler(VOID* Sock_);
BOOL SendTime(SOCKET Sock, TCHAR *time);

/* echo functions */
DWORD WINAPI EchoHandler(VOID* Sock_);
BOOL EchoIncomingPackets(SOCKET Sock);

/* discard functions */


/* qotd functions */
DWORD WINAPI QotdHandler(VOID* Sock_);
BOOL SendQuote(SOCKET Sock, TCHAR* Quote);
