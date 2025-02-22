/*
 * PROJECT:     ReactOS Service Control Manager
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/system/services/services.c
 * PURPOSE:     Main SCM controller
 * COPYRIGHT:   Copyright 2001-2005 Eric Kohl
 *              Copyright 2007 Ged Murphy <gedmurphy@reactos.org>
 *
 */

/* INCLUDES *****************************************************************/

#include "services.h"

#include <wincon.h>

#define NDEBUG
#include <debug.h>

int WINAPI RegisterServicesProcess(DWORD ServicesProcessId);

/* GLOBALS ******************************************************************/

/* Defined in include/reactos/services/services.h */
// #define SCM_START_EVENT             L"SvcctrlStartEvent_A3752DX"
#define SCM_AUTOSTARTCOMPLETE_EVENT L"SC_AutoStartComplete"

BOOL ScmInitialize = FALSE;
BOOL ScmShutdown = FALSE;
BOOL ScmLiveSetup = FALSE;
BOOL ScmSetupInProgress = FALSE;
static HANDLE hScmShutdownEvent = NULL;
static HANDLE hScmSecurityServicesEvent = NULL;


/* FUNCTIONS *****************************************************************/

VOID
PrintString(LPCSTR fmt, ...)
{
#if DBG
    CHAR buffer[512];
    va_list ap;

    va_start(ap, fmt);
    vsprintf(buffer, fmt, ap);
    va_end(ap);

    OutputDebugStringA(buffer);
#endif
}

DWORD
CheckForLiveCD(VOID)
{
    WCHAR CommandLine[MAX_PATH];
    HKEY hSetupKey;
    DWORD dwSetupType;
    DWORD dwSetupInProgress;
    DWORD dwType;
    DWORD dwSize;
    DWORD dwError;

    DPRINT1("CheckSetup()\n");

    /* Open the Setup key */
    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"SYSTEM\\Setup",
                            0,
                            KEY_QUERY_VALUE,
                            &hSetupKey);
    if (dwError != ERROR_SUCCESS)
        return dwError;

    /* Read the SetupType value */
    dwSize = sizeof(DWORD);
    dwError = RegQueryValueExW(hSetupKey,
                               L"SetupType",
                               NULL,
                               &dwType,
                               (LPBYTE)&dwSetupType,
                               &dwSize);

    if (dwError != ERROR_SUCCESS ||
        dwType != REG_DWORD ||
        dwSize != sizeof(DWORD) ||
        dwSetupType == 0)
        goto done;

    /* Read the CmdLine value */
    dwSize = sizeof(CommandLine);
    dwError = RegQueryValueExW(hSetupKey,
                               L"CmdLine",
                               NULL,
                               &dwType,
                               (LPBYTE)CommandLine,
                               &dwSize);

    if (dwError != ERROR_SUCCESS ||
        (dwType != REG_SZ &&
         dwType != REG_EXPAND_SZ &&
         dwType != REG_MULTI_SZ))
        goto done;

    /* Check for the '-mini' option */
    if (wcsstr(CommandLine, L" -mini") != NULL)
    {
        DPRINT1("Running on LiveCD\n");
        ScmLiveSetup = TRUE;
    }

    /* Read the SystemSetupInProgress value */
    dwSize = sizeof(DWORD);
    dwError = RegQueryValueExW(hSetupKey,
                               L"SystemSetupInProgress",
                               NULL,
                               &dwType,
                               (LPBYTE)&dwSetupInProgress,
                               &dwSize);
    if (dwError != ERROR_SUCCESS ||
        dwType != REG_DWORD ||
        dwSize != sizeof(DWORD) ||
        dwSetupType == 0)
    {
        goto done;
    }

    if (dwSetupInProgress == 1)
    {
        DPRINT1("ReactOS Setup currently in progress!\n");
        ScmSetupInProgress = TRUE;
    }

done:
    RegCloseKey(hSetupKey);

    return dwError;
}


DWORD
SetSecurityServicesEvent(VOID)
{
    DWORD dwError;

    if (hScmSecurityServicesEvent != NULL)
        return ERROR_SUCCESS;

    /* Create or open the SECURITY_SERVICES_STARTED event */
    hScmSecurityServicesEvent = CreateEventW(NULL,
                                             TRUE,
                                             FALSE,
                                             L"SECURITY_SERVICES_STARTED");
    if (hScmSecurityServicesEvent == NULL)
    {
        dwError = GetLastError();
        if (dwError != ERROR_ALREADY_EXISTS)
            return dwError;

        hScmSecurityServicesEvent = OpenEventW(EVENT_MODIFY_STATE,
                                               FALSE,
                                               L"SECURITY_SERVICES_STARTED");
        if (hScmSecurityServicesEvent == NULL)
            return GetLastError();
    }

    SetEvent(hScmSecurityServicesEvent);

    return ERROR_SUCCESS;
}


VOID
ScmLogEvent(DWORD dwEventId,
            WORD wType,
            WORD wStrings,
            LPCWSTR *lpStrings)
{
    HANDLE hLog;

    hLog = RegisterEventSourceW(NULL,
                                L"Service Control Manager");
    if (hLog == NULL)
    {
        DPRINT1("ScmLogEvent: RegisterEventSourceW failed %lu\n", GetLastError());
        return;
    }

    if (!ReportEventW(hLog,
                      wType,
                      0,
                      dwEventId,
                      NULL,
                      wStrings,
                      0,
                      lpStrings,
                      NULL))
    {
        DPRINT1("ScmLogEvent: ReportEventW failed %lu\n", GetLastError());
    }

    DeregisterEventSource(hLog);
}


VOID
ScmWaitForLsa(VOID)
{
    HANDLE hEvent = CreateEventW(NULL, TRUE, FALSE, L"LSA_RPC_SERVER_ACTIVE");
    if (hEvent == NULL)
    {
        DPRINT1("Failed to create or open the notification event (Error %lu)\n", GetLastError());
    }
    else
    {
        DPRINT("Wait for the LSA server\n");
        WaitForSingleObject(hEvent, INFINITE);
        DPRINT("LSA server running\n");
        CloseHandle(hEvent);
    }

    DPRINT("ScmWaitForLsa() done\n");
}


BOOL WINAPI
ShutdownHandlerRoutine(DWORD dwCtrlType)
{
    DPRINT1("ShutdownHandlerRoutine() called\n");

    if (dwCtrlType & (CTRL_SHUTDOWN_EVENT | CTRL_LOGOFF_EVENT))
    {
        DPRINT1("Shutdown event received\n");
        ScmShutdown = TRUE;

        ScmAutoShutdownServices();
        ScmShutdownServiceDatabase();

        /* Set the shutdown event */
        SetEvent(hScmShutdownEvent);
    }

    return TRUE;
}


int WINAPI
wWinMain(HINSTANCE hInstance,
         HINSTANCE hPrevInstance,
         LPWSTR lpCmdLine,
         int nShowCmd)
{
    HANDLE hScmStartEvent = NULL;
    HANDLE hScmAutoStartCompleteEvent = NULL;
    SC_RPC_LOCK Lock = NULL;
    BOOL bCanDeleteNamedPipeCriticalSection = FALSE;
    DWORD dwError;

    DPRINT("SERVICES: Service Control Manager\n");

    dwError = CheckForLiveCD();
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("SERVICES: Failed to check for LiveCD (Error %lu)\n", dwError);
        goto done;
    }

    /* Make us critical */
    RtlSetProcessIsCritical(TRUE, NULL, TRUE);

    /* We are initializing ourselves */
    ScmInitialize = TRUE;

    /* Create the start event */
    hScmStartEvent = CreateEventW(NULL, TRUE, FALSE, SCM_START_EVENT);
    if (hScmStartEvent == NULL)
    {
        DPRINT1("SERVICES: Failed to create the start event\n");
        goto done;
    }
    DPRINT("SERVICES: Created start event with handle %p\n", hScmStartEvent);

    /* Create the auto-start complete event */
    hScmAutoStartCompleteEvent = CreateEventW(NULL, TRUE, FALSE, SCM_AUTOSTARTCOMPLETE_EVENT);
    if (hScmAutoStartCompleteEvent == NULL)
    {
        DPRINT1("SERVICES: Failed to create the auto-start complete event\n");
        goto done;
    }
    DPRINT("SERVICES: created auto-start complete event with handle %p\n", hScmAutoStartCompleteEvent);

    /* Create the shutdown event */
    hScmShutdownEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (hScmShutdownEvent == NULL)
    {
        DPRINT1("SERVICES: Failed to create the shutdown event\n");
        goto done;
    }

    /* Initialize our communication named pipe's critical section */
    ScmInitNamedPipeCriticalSection();
    bCanDeleteNamedPipeCriticalSection = TRUE;

//    ScmInitThreadManager();

    ScmInitializeSecurity();

    /* FIXME: more initialization */

    /* Create the 'Last Known Good' control set */
    dwError = ScmCreateLastKnownGoodControlSet();
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("SERVICES: Failed to create the 'Last Known Good' control set (Error %lu)\n", dwError);
        goto done;
    }

    /* Create the services database */
    dwError = ScmCreateServiceDatabase();
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("SERVICES: Failed to create SCM database (Error %lu)\n", dwError);
        goto done;
    }

    /* Update the services database */
    ScmGetBootAndSystemDriverState();

    /* Register the Service Control Manager process with the ReactOS Subsystem */
    if (!RegisterServicesProcess(GetCurrentProcessId()))
    {
        DPRINT1("SERVICES: Could not register SCM process\n");
        goto done;
    }

    /*
     * Acquire the user service start lock until
     * auto-start services have been started.
     */
    dwError = ScmAcquireServiceStartLock(TRUE, &Lock);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("SERVICES: Failed to acquire service start lock (Error %lu)\n", dwError);
        goto done;
    }

    /* Start the RPC server */
    ScmStartRpcServer();

    /* Signal start event */
    SetEvent(hScmStartEvent);

    DPRINT("SERVICES: Initialized\n");

    /* Register event handler (used for system shutdown) */
    SetConsoleCtrlHandler(ShutdownHandlerRoutine, TRUE);

    /*
     * Set our shutdown parameters: we want to shutdown after the maintained
     * services (that inherit the default shutdown level of 640).
     */
    SetProcessShutdownParameters(480, SHUTDOWN_NORETRY);

    /* Start auto-start services */
    ScmAutoStartServices();

    /* Signal auto-start complete event */
    SetEvent(hScmAutoStartCompleteEvent);

    /* FIXME: more to do ? */

    /* Release the service start lock */
    ScmReleaseServiceStartLock(&Lock);

    /* Initialization finished */
    ScmInitialize = FALSE;

    DPRINT("SERVICES: Running\n");

    /* Wait until the shutdown event gets signaled */
    WaitForSingleObject(hScmShutdownEvent, INFINITE);

done:
    ScmShutdownSecurity();

    /* Delete our communication named pipe's critical section */
    if (bCanDeleteNamedPipeCriticalSection != FALSE)
        ScmDeleteNamedPipeCriticalSection();

    if (hScmSecurityServicesEvent != NULL)
        CloseHandle(hScmSecurityServicesEvent);

    /* Close the shutdown event */
    if (hScmShutdownEvent != NULL)
        CloseHandle(hScmShutdownEvent);

    /* Close the auto-start complete event */
    if (hScmAutoStartCompleteEvent != NULL)
        CloseHandle(hScmAutoStartCompleteEvent);

    /* Close the start event */
    if (hScmStartEvent != NULL)
        CloseHandle(hScmStartEvent);

    DPRINT("SERVICES: Finished\n");

    ExitThread(0);
    return 0;
}

/* EOF */
