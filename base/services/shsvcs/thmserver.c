/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             base/services/thmsvc/thmserver.c
 * PURPOSE:          Themes server
 * PROGRAMMER:       Giannis Adamopoulos
 */

/*
 * NOTE:
 * ThemeWaitForServiceReady and ThemeWatchForStart are called from msgina
 * so all the functions in this file run in the context of winlogon
 */

/* INCLUDES *****************************************************************/

#include <windows.h>
#include <uxtheme.h>
#include <uxundoc.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(shsvcs);

/* GLOBALS ******************************************************************/

static WCHAR ServiceName[] = L"Themes";

HANDLE hThemeStartWaitObject, hThemeStopWaitObject, hThemeServiceWaitObject;
HANDLE hStartEvent, hStopEvent, hServiceProcess;

BOOL WINAPI ThemeWatchForStart(VOID);

/* FUNCTIONS *****************************************************************/

static
HANDLE
GetThemeServiceProcessHandle(VOID)
{
    SC_HANDLE scm, service;
    SERVICE_STATUS_PROCESS status;
    DWORD dummy;
    HANDLE ret;

    if (!(scm = OpenSCManagerW( NULL, NULL, 0 )))
    {
        ERR( "failed to open service manager\n" );
        return NULL;
    }
    if (!(service = OpenServiceW( scm, ServiceName, SERVICE_QUERY_STATUS )))
    {
        ERR( "failed to open themes service\n" );
        CloseServiceHandle( scm );
        return NULL;
    }

    if (!QueryServiceStatusEx( service, SC_STATUS_PROCESS_INFO,
                                  (BYTE *)&status, sizeof(status), &dummy ))
    {
        ERR("QueryServiceStatusEx failed\n");
        CloseServiceHandle( service );
        CloseServiceHandle( scm );
        return NULL;
    }

    ret = OpenProcess(SYNCHRONIZE, FALSE, status.dwProcessId);

    CloseServiceHandle( service );
    CloseServiceHandle( scm );

    return ret;
}

static
VOID
CALLBACK
ThemeStopCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
    CloseHandle(hServiceProcess);
    UnregisterWait(hThemeServiceWaitObject);

    ThemeWatchForStart();
    ThemeHooksRemove();
}

static
VOID
CALLBACK
ThemeServiceDiedCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
    /* The theme service died and we don't know if it could set its events properly */
    ResetEvent(hStartEvent);
    ResetEvent(hStopEvent);

    CloseHandle(hServiceProcess);
    UnregisterWait(hThemeStopWaitObject);
    ThemeWatchForStart();
    ThemeHooksRemove();
}

static
VOID
CALLBACK
ThemeStartCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
    hServiceProcess = GetThemeServiceProcessHandle();

    RegisterWaitForSingleObject(&hThemeStopWaitObject, hStopEvent, ThemeStopCallback, NULL, INFINITE, WT_EXECUTEONLYONCE);
    RegisterWaitForSingleObject(&hThemeServiceWaitObject, hServiceProcess, ThemeServiceDiedCallback, NULL, INFINITE, WT_EXECUTEONLYONCE);

    ThemeHooksInstall();
}

BOOL
WINAPI
ThemeWatchForStart(VOID)
{
    hStartEvent = CreateEventW(NULL, TRUE, FALSE, L"Global\\ThemeStartEvent");
    hStopEvent = CreateEventW(NULL, TRUE, FALSE, L"Global\\ThemeStopEvent");

    RegisterWaitForSingleObject(&hThemeStartWaitObject, hStartEvent, ThemeStartCallback, NULL, INFINITE, WT_EXECUTEONLYONCE);

    return TRUE;
}

DWORD
WINAPI
ThemeWaitForServiceReady(DWORD dwTimeout)
{
    SC_HANDLE scm, service;
    SERVICE_STATUS_PROCESS status;
    BOOL ret = FALSE;
    DWORD start_time = GetTickCount();

    TRACE("\n");

    if (!(scm = OpenSCManagerW( NULL, NULL, 0 )))
    {
        ERR( "failed to open service manager\n" );
        return FALSE;
    }
    if (!(service = OpenServiceW( scm, ServiceName, SERVICE_QUERY_STATUS )))
    {
        ERR( "failed to open themes service\n" );
        CloseServiceHandle( scm );
        return FALSE;
    }

    while(TRUE)
    {
        DWORD dummy;

        if (!QueryServiceStatusEx( service, SC_STATUS_PROCESS_INFO,
                                  (BYTE *)&status, sizeof(status), &dummy ))
            break;
        if (status.dwCurrentState == SERVICE_RUNNING)
        {
            ret = TRUE;
            break;
        }
        if (status.dwCurrentState != SERVICE_START_PENDING)
        {
            break;
        }

        if (GetTickCount() - start_time > dwTimeout)
        {
            break;
        }
        Sleep( 100 );
    };

    CloseServiceHandle( service );
    CloseServiceHandle( scm );
    return ret;
}

/* EOF */
