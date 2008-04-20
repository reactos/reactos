#include <windows.h>
#include <tchar.h>

#define DEBUG 1

#define LOG_FILE 1
#define LOG_EVENTLOG 2
#define LOG_ERROR 4
#define LOG_ALL (LOG_FILE | LOG_EVENTLOG | LOG_ERROR)

extern volatile BOOL bShutDown;
extern volatile BOOL bPause;

VOID
LogEvent(LPCTSTR lpMsg,
         DWORD errNum,
         DWORD exitCode,
         UINT flags);

VOID
InitLogging();
