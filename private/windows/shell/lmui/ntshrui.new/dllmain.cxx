//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       dllmain.hxx
//
//  Contents:   DLL initialization entrypoint and global variables
//
//  History:    4-Apr-95 BruceFo  Created
//
//--------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include <locale.h>

#include "resource.h"
#include "critsec.hxx"
#include "cache.hxx"
#include "strhash.hxx"
#include "dllmain.hxx"
#include "util.hxx"

//--------------------------------------------------------------------------
// Globals used elsewhere

UINT        g_NonOLEDLLRefs = 0;
HINSTANCE   g_hInstance = NULL;
WCHAR       g_szShare[50];
BOOL        g_fSharingEnabled = FALSE;   // until proven otherwise
UINT        g_uiMaxUsers = 0;   // max number of users based on product type

WCHAR       g_szAdminShare[] = L"ADMIN$";
WCHAR       g_szIpcShare[]   = L"IPC$";

//--------------------------------------------------------------------------
// Globals used only in this file

CRITICAL_SECTION    g_csOneTimeInit;
BOOL                g_fOneTimeInitDone = FALSE;

// Note: the total wait time is:
//    min( (the wait hint / WAIT_FRACTION), MAX_WAIT_PERIOD ) * MAX_WAIT_COUNT
// In the case of the server, with a 30 second hint, about 2 minutes, 30 sec.

#define WAIT_FRACTION   4       // the fraction of the hint to wait
#define MAX_WAIT_COUNT  20      // the maximum number of wait failures to tolerate before quiting
#define MAX_WAIT_PERIOD 15000L  // 15 seconds

#define WAIT_TO_BEGIN_GRANULARITY   4000    // 4 seconds
#define MAX_WAIT_TO_BEGIN           90000L  // 90 seconds: time to wait before giving up that it will ever start

//--------------------------------------------------------------------------
// Debugging

DECLARE_INFOLEVEL(Sharing)

//--------------------------------------------------------------------------

VOID
InitializeShareCache(
    VOID
    );

DWORD WINAPI
WaitForServerThread(
    IN LPVOID ThreadParameter
    );

BOOL
CheckServiceController(
    VOID
    );

BOOL
ServerConfiguredToStart(
    SC_HANDLE hScManager
    );

BOOL
WaitForServerToBeginStarting(
    SC_HANDLE hService,
    LPSERVICE_STATUS pServiceStatus // so we don't need to re-query on successful return
    );

//--------------------------------------------------------------------------

//+--------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:   Win32 DLL initialization function
//
//  Arguments:  [hInstance] - Handle to this dll
//              [dwReason]  - Reason this function was called.  Can be
//                            Process/Thread Attach/Detach.
//
//  Returns:    BOOL    - TRUE if no error.  FALSE otherwise
//
//  History:    4-Apr-95 BruceFo  Created
//
//---------------------------------------------------------------------------

extern "C"
BOOL
DllMain(
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID lpReserved
    )
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
    {
#if DBG == 1
        InitializeDebugging();
        SharingInfoLevel = DEB_ERROR | DEB_TRACE;
//         SharingInfoLevel = DEB_ERROR;
        SetWin4AssertLevel(ASSRT_BREAK | ASSRT_MESSAGE);
#endif // DBG == 1

        appDebugOut((DEB_TRACE, "ntshrui.dll: DllMain enter\n"));

        // Disable thread notification from OS
        DisableThreadLibraryCalls(hInstance);
        g_hInstance = hInstance;
        InitCommonControls();   // get up/down control
        setlocale(LC_CTYPE, ""); // set the C runtime library locale, for string operations
        InitializeCriticalSection(&g_csOneTimeInit);
        break;
    }

    case DLL_PROCESS_DETACH:
        appDebugOut((DEB_TRACE, "ntshrui.dll: DllMain leave\n"));
        break;
    }

    return TRUE;
}

extern HRESULT SharePropDummyFunction();
HRESULT Linkage()
{
    return SharePropDummyFunction();
}

//+-------------------------------------------------------------------------
//
//  Function:   OneTimeInit
//
//  Synopsis:   Initialization code: check if SMB server is running
//
//  History:    21-Apr-95 BruceFo  Created
//
//  Returns:
//  Note:       We don't want to do this in the DLL initialization code, so
//              we call it for every entrypoint, namely DllGetClassObject,
//              DllCanUnloadNow, and IsPathShared.
//
//--------------------------------------------------------------------------

VOID
OneTimeInit(
    IN BOOL bDialog // TRUE if for dialog API
    )
{
    // quick check; no critical section
    if (g_fOneTimeInitDone)
    {
        return;
    }

    {
        CTakeCriticalSection t(&g_csOneTimeInit);   // scope it

        // Since there wasn't a critical section on the above check, multiple
        // threads might have fallen through to the critical section taking,
        // and wait. After the one-time initialization is complete, the
        // first thread sets g_fOneTimeInitDone to TRUE and leaves the
        // critical section. At this point, the other threads will wake up
        // here. Do the check again, and return if another thread did the
        // initialization.

        if (g_fOneTimeInitDone)
        {
            return;
        }

        // Now, do the actual initialization

        if (!bDialog)
        {
            // First, determine if the server is running. If not, see if
            // we should wait for it. If we wait and it still isn't
            // started, then give up.

            InitializeShareCache();
        }
        // if it is a dialog call, then we don't load up the cache because
        // that's the first thing the dialog code does.

        // Load now, so can use later.
        LoadString(g_hInstance, IDS_MSGTITLE, g_szShare, ARRAYLEN(g_szShare));

        // Determine the maximum number of users
        g_uiMaxUsers = IsWorkstationProduct()
                            ? MAX_USERS_ON_WORKSTATION
                            : MAX_USERS_ON_SERVER
                            ;

        g_fOneTimeInitDone = TRUE;  // set this *last*
    }
}


VOID
InitializeShareCache(
    VOID
    )

/*++

Routine Description:

    This routine initializes the share cache. It determines if the LanMan
    server is running. If it is, great.
    If it isn't, determine if it is starting. If so, wait for a while. After
    a while, if it still hasn't started, then give up and assume it's hung.
    If the server isn't starting, then determine if the configuration says it
    is going to start (set to autostart). If so, wait to see if it ever goes
    into the "start pending" state. Give up after a reasonable period. If it
    does go into this state, then wait for it to finish starting, as described
    before.

    Both LanMan and Service Controller APIs are used in this endeavor.

Arguments:

    None

Return Value:

    Returns TRUE if the server service has been started; otherwise
    returns FALSE.  Any API errors return FALSE, and hence assume
    the server isn't started or won't start.

--*/

{
    appDebugOut((DEB_TRACE, "InitializeShareCache: enter\n"));

    g_ShareCache.Refresh(); // sets g_fSharingEnabled
    if (g_fSharingEnabled)
    {
        // well, we've got a cache so no need to start up a thread and wait
        // for the server to start
        return;
    }

    // The server isn't currently started. Create a thread that waits for it
    // to start, and if it does, refreshes the shell.

    DWORD threadId;
    HANDLE hThread = CreateThread(
                            NULL,
                            0,
                            WaitForServerThread,
                            NULL,
                            0,
                            &threadId);
    if (NULL == hThread)
    {
        appDebugOut((DEB_ERROR, "Error creating thread\n"));
    }
    else
    {
        CloseHandle(hThread); // No reason to keep handle around
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   WaitForServerThread
//
//  Synopsis:   Thread procedure for the thread that waits for the server
//              to start.
//
//  History:    25-Apr-95 BruceFo  Created
//
//--------------------------------------------------------------------------

DWORD WINAPI
WaitForServerThread(
    IN LPVOID ThreadParameter
    )
{
    appDebugOut((DEB_TRACE, "Created thread to wait for server to start\n"));

    if (CheckServiceController())
    {
        // the server has started

        appDebugOut((DEB_TRACE, "The server finally started, after waiting in a thread. Refresh all!\n"));

        g_ShareCache.Refresh(); // sets g_fSharingEnabled

        // This magic line refreshes *all* the explorer windows. Unfortunately,
        // it causes a share cache refresh for each one as well...
        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
    }

    return 0;
}


//+-------------------------------------------------------------------------
//
//  Function:   CheckServiceController
//
//  Synopsis:   Returns TRUE if the server starts, based on consulting the
//              service controller and waiting for the server to start.
//
//  History:    25-Apr-95 BruceFo  Created
//
//--------------------------------------------------------------------------

BOOL
CheckServiceController(
    VOID
    )
{
    // See if it is currently starting.

    SC_HANDLE hScManager;
    SC_HANDLE hService;
    SERVICE_STATUS serviceStatus;

    hScManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (hScManager == NULL)
    {
        appDebugOut((DEB_ERROR,
                "CheckServiceController: OpenSCManager failed: 0x%08lx\n",
                GetLastError()));

        return FALSE;
    }

    appDebugOut((DEB_TRACE, "CheckServiceController: opening server service\n"));

    hService = OpenService(hScManager, SERVICE_SERVER, SERVICE_QUERY_STATUS);
    if (hService == NULL)
    {
        appDebugOut((DEB_ERROR,
                "CheckServiceController: OpenService failed: 0x%08lx\n",
                GetLastError()));

        CloseServiceHandle(hScManager);
        return FALSE;
    }

    // Now we've got a handle to the server service. See if it's started.

    appDebugOut((DEB_TRACE, "CheckServiceController: querying server service\n"));

    if (!QueryServiceStatus(hService, &serviceStatus))
    {
        appDebugOut((DEB_ERROR,
                "CheckServiceController: QueryServiceStatus failed: 0x%08lx\n",
                GetLastError()));

        CloseServiceHandle(hScManager);
        CloseServiceHandle(hService);
        return FALSE;
    }

    if (serviceStatus.dwCurrentState == SERVICE_RUNNING)
    {
        appDebugOut((DEB_TRACE, "CheckServiceController: Server is running!\n"));

        // we've off to the races!

        CloseServiceHandle(hScManager);
        CloseServiceHandle(hService);
        return TRUE;
    }

    if (serviceStatus.dwCurrentState != SERVICE_START_PENDING)
    {
        appDebugOut((DEB_TRACE, "CheckServiceController: Server is not running nor is it starting! State = %d\n", serviceStatus.dwCurrentState));

        // The server is not in the process of starting. Go check its
        // configuration and see if it is even configured to start.

        if (!ServerConfiguredToStart(hScManager))
        {
            // the service is not in the process of starting, nor is it
            // configured to start, so we give up.

            CloseServiceHandle(hScManager);
            CloseServiceHandle(hService);
            return FALSE;
        }

        if (!WaitForServerToBeginStarting(hService, &serviceStatus))
        {
            // The server is configured to start, but we already waited for
            // it to commence starting, and it never did. So give up on
            // it.

            CloseServiceHandle(hScManager);
            CloseServiceHandle(hService);
            return FALSE;
        }

        // the server is configured to start, we waited for it to commence
        // its startup sequence, and it actually did commence starting!
    }

    // In this case, the service is trying to start. Wait until it either
    // starts or we think it's hung.

    appDebugOut((DEB_TRACE, "CheckServiceController: Server is starting\n"));

    //
    // record the current check point. The service should "periodically
    // increment" this if it is making progress.
    //

    DWORD lastCheckPoint = serviceStatus.dwCheckPoint;
    DWORD waitCount = 0;

    while (serviceStatus.dwCurrentState == SERVICE_START_PENDING)
    {
        if (lastCheckPoint == serviceStatus.dwCheckPoint)
        {
            ++waitCount;
            if (waitCount > MAX_WAIT_COUNT)
            {
                appDebugOut((DEB_TRACE,
                    "CheckServiceController: Server service is HUNG\n"));

                CloseServiceHandle(hScManager);
                CloseServiceHandle(hService);
                return FALSE;
            }
        }
        else
        {
            waitCount = 0;
            lastCheckPoint = serviceStatus.dwCheckPoint;
        }

        // Ideally, we would wait the wait hint and be done with it. However,
        // We don't want to be waiting if the service gives us an overly
        // generous wait hint and finishes while we're still waiting. So,
        // wait a fraction of the wait hint. The exact fraction is
        // 1/WAIT_FRACTION. The effect is that we wait no more than
        // MAX_WAIT_COUNT / WAIT_FRACTION times the wait hint before
        // giving up. We make sure we wait at least 1 second between checks.
        // Finally, cap the wait hint in case it is far to large, possibly
        // in error.

        DWORD dwWait = serviceStatus.dwWaitHint / WAIT_FRACTION;
        dwWait = (dwWait > MAX_WAIT_PERIOD) ? MAX_WAIT_PERIOD : dwWait;
        dwWait = (dwWait < 1000) ? 1000 : dwWait; // at least 1 second

        appDebugOut((DEB_TRACE,
            "CheckServiceController: sleeping. hint = %d, actually waiting %d\n",
            serviceStatus.dwWaitHint, dwWait));

        Sleep(dwWait);

        if (!QueryServiceStatus(hService, &serviceStatus))
        {
            appDebugOut((DEB_ERROR,
                    "CheckServiceController: QueryServiceStatus failed: 0x%08lx\n",
                    GetLastError()));

            CloseServiceHandle(hScManager);
            CloseServiceHandle(hService);
            return FALSE;
        }
    }

    CloseServiceHandle(hScManager);
    CloseServiceHandle(hService);

    if (serviceStatus.dwCurrentState == SERVICE_RUNNING)
    {
        appDebugOut((DEB_TRACE, "CheckServiceController: service finally started\n"));
        return TRUE;    // Finally! It's running!
    }
    else
    {
        appDebugOut((DEB_TRACE, "CheckServiceController: service never started\n"));
        return FALSE;
    }
}


BOOL
ServerConfiguredToStart(
    SC_HANDLE hScManager
    )
{
    // We re-open the service because we want a different type of permission

    SC_HANDLE hService = OpenService(hScManager, SERVICE_SERVER, SERVICE_QUERY_CONFIG);
    if (hService == NULL)
    {
        appDebugOut((DEB_ERROR,
                "ServerConfiguredToStart: OpenService failed: 0x%08lx\n",
                GetLastError()));

        return FALSE;
    }

    BOOL b;
    DWORD cbBytesNeeded;
    BYTE buffer[1024];  // a large buffer...
    LPQUERY_SERVICE_CONFIG lpqscServConfig = (LPQUERY_SERVICE_CONFIG)buffer;
    b = QueryServiceConfig(
                    hService,
                    lpqscServConfig,
                    sizeof(buffer),
                    &cbBytesNeeded);
    if (!b)
    {
        appDebugOut((DEB_ERROR,
                "ServerConfiguredToStart: QueryServiceConfig failed: 0x%08lx\n",
                GetLastError()));

        return FALSE;
    }

    b = (lpqscServConfig->dwStartType == SERVICE_AUTO_START);
    CloseServiceHandle(hService);

    appDebugOut((DEB_TRACE,
        "ServerConfiguredToStart: configured to start? %s\n",
        b ? "yes" : "no"));

    return b;
}


BOOL
WaitForServerToBeginStarting(
    SC_HANDLE hService,
    LPSERVICE_STATUS pServiceStatus // so we don't need to re-query on successful return
    )
{
    // Here's the algorithm:
    //      wait WAIT_TO_BEGIN_GRANULARITY milliseconds
    //      query status
    //      if the service is running then return TRUE
    //      if we've waited MAX_WAIT_TO_BEGIN ms, return FALSE
    //      go back and wait again...

    DWORD dwWaitedMilliseconds;

    for (dwWaitedMilliseconds = 0;
         dwWaitedMilliseconds < MAX_WAIT_TO_BEGIN;
         dwWaitedMilliseconds += WAIT_TO_BEGIN_GRANULARITY)
    {
        appDebugOut((DEB_TRACE,
            "WaitForServerToBeginStarting: sleeping\n"));

        Sleep(WAIT_TO_BEGIN_GRANULARITY);

        if (!QueryServiceStatus(hService, pServiceStatus))
        {
            appDebugOut((DEB_ERROR,
                    "WaitForServerToBeginStarting: QueryServiceStatus failed: 0x%08lx\n",
                    GetLastError()));

            return FALSE;
        }

        if (   pServiceStatus->dwCurrentState == SERVICE_RUNNING
            || pServiceStatus->dwCurrentState == SERVICE_START_PENDING
            )
        {
            appDebugOut((DEB_TRACE,
                "WaitForServerToBeginStarting: server commenced startup\n"));

            return TRUE;
        }
    }

    appDebugOut((DEB_TRACE,
        "WaitForServerToBeginStarting: waited %d milliseconds for server to commence startup, then gave up\n",
         MAX_WAIT_TO_BEGIN));

    return FALSE;
}
