/***************************************************************************
 *  VER.C
 *
 *	Code specific to the DLL version of VER which contains the Windows
 *	procedures necessary to make it work
 *
 ***************************************************************************/

#include "verpriv.h"
#include <diamondd.h>
#include "mydiam.h"

/*  LibMain
 *		Called by DLL startup code.
 *		Initializes VER.DLL.
 */

#ifdef LOG_DATA
typedef struct {
    DWORD idThd;
    char  szMsg[4];
    DWORD dwLine;
    DWORD dwData;
} LOGLINE;

typedef LOGLINE *PLOGLINE;

#define CLOG_MAX    16384

LOGLINE llLogBuff[CLOG_MAX];
int illLogPtr = 0;
CRITICAL_SECTION csLog;

void LogThisData( DWORD id, char *szMsg, DWORD dwLine, DWORD dwData ) {
    PLOGLINE pll;

    EnterCriticalSection(&csLog);
    pll = &llLogBuff[illLogPtr++];
    if (illLogPtr >= CLOG_MAX)
        illLogPtr = 0;
    LeaveCriticalSection(&csLog);


    pll->idThd = id;
    pll->dwData = dwData;
    pll->dwLine = dwLine;
    CopyMemory( pll->szMsg, szMsg, sizeof(pll->szMsg) );
}
#endif





HANDLE  hInst;
extern HINSTANCE hLz32;
extern HINSTANCE hCabinet;

DWORD itlsDiamondContext = ITLS_ERROR;  // Init to Error Condition

INT
APIENTRY
LibMain(
       HANDLE  hInstance,
       DWORD   dwReason,
       LPVOID  lp
       )
{
    PDIAMOND_CONTEXT pdcx;

    UNREFERENCED_PARAMETER(lp);

    hInst = hInstance;

#ifdef LOG_DATA
    {
        TCHAR szBuffer[80];
        wsprintf( szBuffer, TEXT("thd[0x%08ld]: Attached to version.dll for %ld\n"), GetCurrentThreadId(), dwReason );
        OutputDebugString(szBuffer);
    }
#endif

    switch (dwReason) {

        case DLL_PROCESS_ATTACH:

#ifdef LOG_DATA
            InitializeCriticalSection(&csLog);
#endif

#ifdef LOG_DATA

            {
                TCHAR szBuffer[MAX_PATH];
                OutputDebugString(TEXT(">>>version.c: compiled ") TEXT(__DATE__) TEXT(" ") TEXT(__TIME__) TEXT("\n"));
                OutputDebugString(TEXT("\tProcess "));
                GetModuleFileName(NULL, szBuffer, sizeof(szBuffer) / sizeof(TCHAR));
                OutputDebugString(szBuffer);
                OutputDebugString(TEXT(" attached\n"));
            }
#endif
            itlsDiamondContext = TlsAlloc();

            // Fall through to Thread Attach, since for some stupid reason
            // the DLL_PROCESS_ATTACH thread does not call DLL_THREAD_ATTACH

        case DLL_THREAD_ATTACH:
            if (GotDmdTlsSlot())
                TlsSetValue(itlsDiamondContext, NULL);

            break;

        case DLL_PROCESS_DETACH:

            if (GotDmdTlsSlot()) {
                TlsFree(itlsDiamondContext);
            }
            if (hLz32) {
                FreeLibrary(hLz32);
            }

            if (hCabinet) {
                FreeLibrary(hCabinet);
            }
            break;
    }

    /* Return success */
    return 1;
}
