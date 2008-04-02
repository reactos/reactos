#include <stdio.h>
#include <winsock2.h>
#include <tchar.h>
#include <time.h>

#define LOG_FILE 1
#define LOG_EVENTLOG 2
#define LOG_ERROR 4
#define LOG_ALL (LOG_FILE | LOG_EVENTLOG | LOG_ERROR)

/* default port numbers */
#define ECHO_PORT 7
#define DISCARD_PORT 9
#define DAYTIME_PORT 13
#define QOTD_PORT 17
#define CHARGEN_PORT 19

#define NUM_SERVICES 5
#define CS_TIMEOUT 1000


/* data structure to pass to threads */
typedef struct _Services {
    USHORT Port;
    TCHAR *Name;
    LPTHREAD_START_ROUTINE Service;
} SERVICES, *PSERVICES;

extern volatile BOOL bShutdown;
extern volatile BOOL bPause;

/* logging functions */
VOID InitLogging();
VOID UninitLogging();
VOID LogEvent(LPCTSTR lpMsg, DWORD errNum, DWORD exitCode, UINT flags);

/* skelserver functions */
DWORD WINAPI StartServer(LPVOID lpParam);
BOOL ShutdownConnection(SOCKET Sock, BOOL bRec);

/* server thread handlers */
DWORD WINAPI ChargenHandler(VOID* sock_);
DWORD WINAPI DaytimeHandler(VOID* sock_);
DWORD WINAPI EchoHandler(VOID* sock_);
DWORD WINAPI DiscardHandler(VOID* sock_);
DWORD WINAPI QotdHandler(VOID* sock_);
