#ifdef _MSC_VER
#define _CRT_SECURE_NO_DEPRECATE 1
#endif
#include <stdio.h>
#include <winsock2.h>
#include <tchar.h>
#include <time.h>

/* default port numbers */
#define ECHO_PORT 7
#define DISCARD_PORT 9
#define DAYTIME_PORT 13
#define QOTD_PORT 17
#define CHARGEN_PORT 19

#define NUM_SERVICES 5
#define BUF_SIZE 255
#define BUF 1024
#define CS_TIMEOUT 1000

/* RFC865 states no more than 512 chars per line */
#define MAX_QUOTE_BUF 512

/* printable ASCII's characters for chargen */
#define ASCII_START 32
#define ASCII_END 126

/* number of chars to put on a line */
#define LINESIZ 74 // 72 + /r and /n

/* data structure to pass to threads */
typedef struct _Services {
    USHORT Port;
    TCHAR *Name;
    LPTHREAD_START_ROUTINE Service;
} SERVICES, *PSERVICES;

/* tcpsvcs functions */
VOID WINAPI ServerCtrlHandler(DWORD control);
INT CreateServers(VOID);
VOID LogEvent(LPCTSTR UserMessage, DWORD ExitCode, BOOL PrintErrorMsg);
void UpdateStatus(DWORD NewStatus, DWORD Check);

/* skelserver functions */
DWORD WINAPI StartServer(LPVOID lpParam);
BOOL ShutdownConnection(SOCKET Sock, BOOL bRec);

/* server thread handlers */
DWORD WINAPI ChargenHandler(VOID* Sock_);
DWORD WINAPI DaytimeHandler(VOID* Sock_);
DWORD WINAPI EchoHandler(VOID* Sock_);
DWORD WINAPI DiscardHandler(VOID* Sock_);
DWORD WINAPI QotdHandler(VOID* Sock_);
