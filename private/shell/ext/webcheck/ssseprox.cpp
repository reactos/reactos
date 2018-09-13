#include "private.h"
#include "ssseprox.h"
#include <shdocvw.h>

#include <mluisupp.h>

#define TF_THISMODULE TF_SEPROX

/////////////////////////////////////////////////////////////////////////////
// Local functions
/////////////////////////////////////////////////////////////////////////////
HANDLE          OpenDeskSwitchEvent(DWORD dwDesiredAccess, BOOL bInheritHandle);
BOOL            ExecURL();
BOOL            ReloadChannelScreenSaver();
DWORD WINAPI    SSExecThread(LPVOID pVoid);

/////////////////////////////////////////////////////////////////////////////
// Constants
/////////////////////////////////////////////////////////////////////////////
#define TIME_WAIT_DESKTOPSWITCH     10000   // ms (10 seconds)
#define TIME_WAIT_BEFORE_RESTART    5000    // ms (5 seconds)

/////////////////////////////////////////////////////////////////////////////
// Module variables
/////////////////////////////////////////////////////////////////////////////
static HANDLE s_hSEThread = NULL;
static HANDLE s_hTermEvent = NULL;    // Thread Terminate event.

#pragma data_seg(".text")
static TCHAR s_szRegSubKey[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\IE4 Screen Saver");
static TCHAR s_szRegLastNavURL[] = TEXT("LastNavURL");

static TCHAR s_szExecuteEvent[] = TEXT("ActSaverSEEvent");
static TCHAR s_szThreadTermEvent[] = TEXT("SETTermEvent");
#pragma data_seg()

/////////////////////////////////////////////////////////////////////////////
// InitSEProx
/////////////////////////////////////////////////////////////////////////////
BOOL InitSEProx
(
)
{
    BOOL bResult = FALSE;

    ASSERT((s_hSEThread == NULL) && (s_hTermEvent == NULL));

    for (;;)
    {
        // Create the Thread End event.
        if ((s_hTermEvent = CreateEvent(NULL,
                                        FALSE,
                                        FALSE,
                                        s_szThreadTermEvent)) == NULL)
        {
            ASSERT(FALSE);
            break;
        }

        // Create the Thread.
        DWORD dwThreadId;
        if ((s_hSEThread = CreateThread(NULL,
                                        1024,
                                        SSExecThread,
                                        NULL,
                                        0,
                                        &dwThreadId)) == NULL)
        {
            ASSERT(FALSE);
            break;
        }

        // Lets run really low.
        SetThreadPriority(s_hSEThread, THREAD_PRIORITY_IDLE);

        bResult = TRUE;
        break;
    }

    // Cleanup on error.
    if (!bResult)
    {
        CloseHandle(s_hTermEvent);
        s_hTermEvent = NULL;

        ASSERT(s_hSEThread == NULL);
    }

    return bResult;
}

/////////////////////////////////////////////////////////////////////////////
// FreeSEProx
/////////////////////////////////////////////////////////////////////////////
void FreeSEProx
(
)
{
    if (s_hTermEvent != NULL)
    {
        SetEvent(s_hTermEvent);

        CloseHandle(s_hSEThread);
        s_hSEThread = NULL;

        CloseHandle(s_hTermEvent);
        s_hTermEvent = NULL;
    }

    DBG("FreeSEProx: Quitting.");
}

/////////////////////////////////////////////////////////////////////////////
// SSExecThread
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI SSExecThread
(
    LPVOID pVoid
)
{
    // Prevent webcheck from disappearing while this thread is still
    // running
    DllAddRef();

    #define MAX_EVENTS  3
    HANDLE  arghEvents[MAX_EVENTS] = { NULL, NULL, NULL };

    DBG("SSExecThread: Thread starting.");

#ifdef DEBUG
    if(!g_fInitTable)
    {
        if(GetLeakDetectionFunctionTable(&LeakDetFunctionTable))
            g_fInitTable = TRUE;
    }
    if(g_fInitTable)
        LeakDetFunctionTable.pfnDebugMemLeak(DML_TYPE_THREAD | DML_BEGIN, TEXT(__FILE__), __LINE__);
#endif 

    for (;;)
    {
        // Open the thread termination event and the screen
        // saver restart event for all platforms.
        if  (
            ((arghEvents[0] = OpenEvent(SYNCHRONIZE, FALSE, s_szThreadTermEvent)) == NULL)
            ||
            ((arghEvents[1] = OpenDeskSwitchEvent(SYNCHRONIZE, FALSE)) == NULL)
            ||
            ((arghEvents[2] = CreateEvent(NULL, FALSE, FALSE, s_szExecuteEvent)) == NULL)
            )
        {
            TraceMsg(TF_ERROR, "Unable to initialize basic SSExec events");
            break;
        }

        // NOTE: Assume we are always started in the Default desktop.
        BOOL bInDefaultDesktop = TRUE;
        DWORD       dwResult;

        do
        {
            DBG("SSExecThread: Waiting for signal...");

            dwResult = WaitForMultipleObjects(  MAX_EVENTS,
                                                arghEvents,
                                                FALSE,
                                                INFINITE);

            DBG("SSExecThread: Signal received!");

            switch (dwResult)
            {
                case WAIT_OBJECT_0:     // Thread Terminate Event
                    break;

                case WAIT_OBJECT_0+1:   // Desktop Switch Event
                {
                    bInDefaultDesktop = !bInDefaultDesktop;
#ifdef _DEBUG
                    if (bInDefaultDesktop)
                        DBG("SSExecThread: Now in Default desktop.");
                    else
                        DBG("SSExecThread: Now in another desktop.");
#endif  // _DEBUG
                    break;
                }

                case WAIT_OBJECT_0+2:   // Execute Event
                {
                    if (!bInDefaultDesktop)
                    {
                        DBG("SSExecThread: Waiting for default desktop switch...");
                        WaitForSingleObject(arghEvents[1], TIME_WAIT_DESKTOPSWITCH);
                    }

                    DBG("SSExecThread: Starting ExecURL().");
                    EVAL(ExecURL());

                    break;
                }

                default:
                {
                    TraceMsg(TF_ERROR, "SSExecThread: Unknown signal!");

                    ASSERT(FALSE);
                    break;
                }
            } 
        } while (dwResult != WAIT_OBJECT_0);

        break;
    }

    // Cleanup
    for (int i = 0; i < MAX_EVENTS; i++)
    {
        if (arghEvents[i] != NULL)
            CloseHandle(arghEvents[i]);
    }
    
#ifdef DEBUG
    if(g_fInitTable)
        LeakDetFunctionTable.pfnDebugMemLeak(DML_TYPE_THREAD | DML_END, TEXT(__FILE__), __LINE__);
#endif

    DBG("SSExecThread: Thread ending.");

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// OpenDeskSwitchEvent
/////////////////////////////////////////////////////////////////////////////
HANDLE OpenDeskSwitchEvent
(
    DWORD   dwDesiredAccess,
    BOOL    bInheritHandle
)
{
    HWINSTA hDefaultWinSta;
    TCHAR   szDefaultWinSta[32];
    TCHAR   szSwitchName[64];
    DWORD   dwNameLen;
    HANDLE  hResult = NULL;

    for (;;)
    {
        if  (
            ((hDefaultWinSta = GetProcessWindowStation()) == NULL)
            ||
            (GetUserObjectInformation(  hDefaultWinSta,
                                        UOI_NAME,
                                        szDefaultWinSta,
                                        sizeof(szDefaultWinSta),
                                        &dwNameLen) == 0)
            )
        {
            break;
        }

        wnsprintf(szSwitchName, ARRAYSIZE(szSwitchName), TEXT("%s_DesktopSwitch"), 
                  szDefaultWinSta);
        hResult = OpenEvent(dwDesiredAccess, bInheritHandle, szSwitchName);

        break;
    }

    return hResult;
}

/////////////////////////////////////////////////////////////////////////////
// ExecURL
/////////////////////////////////////////////////////////////////////////////
BOOL ExecURL
(
)
{
    HKEY    hKey = NULL;
    DWORD   dwType;
    TCHAR   szURL[INTERNET_MAX_URL_LENGTH];
    DWORD   cbURLLength = sizeof(szURL);
    BOOL    bResult;

    DBG("ExecURL()");

    // Get the URL to exec from the Registry.
    if  (
        (RegOpenKey(HKEY_CURRENT_USER, s_szRegSubKey, &hKey) == ERROR_SUCCESS)
        &&
        (RegQueryValueEx(hKey, s_szRegLastNavURL, NULL, &dwType, (LPBYTE)szURL, &cbURLLength) == ERROR_SUCCESS)
        &&
        (dwType == REG_SZ)
        )
    {
        WCHAR wszPath[INTERNET_MAX_URL_LENGTH];

        TraceMsg(TF_THISMODULE, "ExecURL('%s')", szURL);

        SHTCharToUnicode(szURL, wszPath, SIZECHARS(wszPath));
        bResult = SUCCEEDED(HlinkNavigateString(NULL, wszPath));

        TraceMsg(TF_THISMODULE, "ExecURL: HlinkNavigateString() = %d", bResult);
    }
    else
        bResult = FALSE;

    if (hKey != NULL)
        RegCloseKey(hKey);

    return bResult;
}

/////////////////////////////////////////////////////////////////////////////
// ReloadChannelScreenSaver
/////////////////////////////////////////////////////////////////////////////
BOOL ReloadChannelScreenSaver
(
)
{
    DBG("Reloading Channel Screen Saver...");

    Sleep(TIME_WAIT_BEFORE_RESTART);

    TCHAR szScreenSaverEXE[MAX_PATH];
    EVAL(MLLoadString(
                    IDS_SCREENSAVEREXE,
                    szScreenSaverEXE,
                    ARRAYSIZE(szScreenSaverEXE)) != 0);

    return (ShellExecute(   NULL,
                            NULL,
                            szScreenSaverEXE,
                            NULL,
                            NULL,
                            SW_SHOWNORMAL) > (HINSTANCE)32);
}
