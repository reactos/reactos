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

static DWORD
ScmErrorFromBoolean(BOOL Result)
{
    if (Result)
        return ERROR_SUCCESS;

    return GetLastError();
}


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
    HANDLE hEvent;
    DWORD dwError;
    DWORD dwElapsed = 0;
    DWORD dwWait;

    DPRINT1("ROSLSA scm-wait-enter\n");

    SetLastError(ERROR_SUCCESS);
    hEvent = CreateEventW(NULL, TRUE, FALSE, L"LSA_RPC_SERVER_ACTIVE");
    dwError = GetLastError();
    DPRINT1("ROSLSA scm-event-create-result handle=%p error=%lu\n",
            hEvent,
            dwError);
    if (hEvent == NULL)
    {
        DPRINT1("Failed to create or open the notification event (Error %lu)\n",
                dwError);
    }
    else
    {
        DPRINT("Wait for the LSA server\n");
        for (;;)
        {
            dwWait = WaitForSingleObject(hEvent, 1000);
            if (dwWait == WAIT_OBJECT_0)
            {
                DPRINT1("ROSLSA scm-wait-signaled elapsed=%lu\n",
                        dwElapsed);
                break;
            }

            if (dwWait == WAIT_TIMEOUT)
            {
                dwElapsed += 1000;
                if ((dwElapsed % 5000) == 0)
                {
                    DPRINT1("ROSLSA scm-wait-pending elapsed=%lu\n",
                            dwElapsed);
                }

                continue;
            }

            dwError = GetLastError();
            DPRINT1("ROSLSA scm-wait-failed wait=%lu error=%lu elapsed=%lu\n",
                    dwWait,
                    dwError,
                    dwElapsed);
            break;
        }
        DPRINT("LSA server running\n");
        CloseHandle(hEvent);
    }

    DPRINT("ScmWaitForLsa() done\n");
    DPRINT1("ROSLSA scm-wait-done\n");
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
    BOOL bResult;
    DWORD dwError;

    DPRINT("SERVICES: Service Control Manager\n");
    DPRINT1("ROSLSA scm-main-enter\n");

    dwError = CheckForLiveCD();
    DPRINT1("ROSLSA scm-livecd-result error=%lu live=%u setup=%u\n",
            dwError,
            ScmLiveSetup,
            ScmSetupInProgress);
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
    SetLastError(ERROR_SUCCESS);
    hScmStartEvent = CreateEventW(NULL, TRUE, FALSE, SCM_START_EVENT);
    dwError = GetLastError();
    DPRINT1("ROSLSA scm-start-event-result handle=%p error=%lu\n",
            hScmStartEvent,
            dwError);
    if (hScmStartEvent == NULL)
    {
        DPRINT1("SERVICES: Failed to create the start event\n");
        goto done;
    }
    DPRINT("SERVICES: Created start event with handle %p\n", hScmStartEvent);

    /* Create the auto-start complete event */
    SetLastError(ERROR_SUCCESS);
    hScmAutoStartCompleteEvent = CreateEventW(NULL, TRUE, FALSE, SCM_AUTOSTARTCOMPLETE_EVENT);
    dwError = GetLastError();
    DPRINT1("ROSLSA scm-autostart-event-result handle=%p error=%lu\n",
            hScmAutoStartCompleteEvent,
            dwError);
    if (hScmAutoStartCompleteEvent == NULL)
    {
        DPRINT1("SERVICES: Failed to create the auto-start complete event\n");
        goto done;
    }
    DPRINT("SERVICES: created auto-start complete event with handle %p\n", hScmAutoStartCompleteEvent);

    /* Create the shutdown event */
    SetLastError(ERROR_SUCCESS);
    hScmShutdownEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    dwError = GetLastError();
    DPRINT1("ROSLSA scm-shutdown-event-result handle=%p error=%lu\n",
            hScmShutdownEvent,
            dwError);
    if (hScmShutdownEvent == NULL)
    {
        DPRINT1("SERVICES: Failed to create the shutdown event\n");
        goto done;
    }

    /* Initialize our communication named pipe's critical section */
    DPRINT1("ROSLSA scm-named-pipe-cs-enter\n");
    ScmInitNamedPipeCriticalSection();
    bCanDeleteNamedPipeCriticalSection = TRUE;
    DPRINT1("ROSLSA scm-named-pipe-cs-done\n");

//    ScmInitThreadManager();

    DPRINT1("ROSLSA scm-security-enter\n");
    dwError = ScmInitializeSecurity();
    DPRINT1("ROSLSA scm-security-result error=%lu\n", dwError);

    /* FIXME: more initialization */

    /* Create the 'Last Known Good' control set */
    DPRINT1("ROSLSA scm-lkg-enter\n");
    dwError = ScmCreateLastKnownGoodControlSet();
    DPRINT1("ROSLSA scm-lkg-result error=%lu\n", dwError);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("SERVICES: Failed to create the 'Last Known Good' control set (Error %lu)\n", dwError);
        goto done;
    }

    /* Create the services database */
    DPRINT1("ROSLSA scm-database-enter\n");
    dwError = ScmCreateServiceDatabase();
    DPRINT1("ROSLSA scm-database-result error=%lu\n", dwError);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("SERVICES: Failed to create SCM database (Error %lu)\n", dwError);
        goto done;
    }

    /* Update the services database */
    DPRINT1("ROSLSA scm-driver-state-enter\n");
    ScmGetBootAndSystemDriverState();
    DPRINT1("ROSLSA scm-driver-state-done\n");

    /* Register the Service Control Manager process with the ReactOS Subsystem */
    bResult = RegisterServicesProcess(GetCurrentProcessId());
    dwError = ScmErrorFromBoolean(bResult);
    DPRINT1("ROSLSA scm-register-result success=%u error=%lu\n",
            bResult,
            dwError);
    if (!bResult)
    {
        DPRINT1("SERVICES: Could not register SCM process\n");
        goto done;
    }

    /*
     * Acquire the user service start lock until
     * auto-start services have been started.
     */
    dwError = ScmAcquireServiceStartLock(TRUE, &Lock);
    DPRINT1("ROSLSA scm-start-lock-result error=%lu lock=%p\n",
            dwError,
            Lock);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("SERVICES: Failed to acquire service start lock (Error %lu)\n", dwError);
        goto done;
    }

    /* Start the RPC server */
    DPRINT1("ROSLSA scm-rpc-enter\n");
    ScmStartRpcServer();
    DPRINT1("ROSLSA scm-rpc-done\n");

    /* Signal start event */
    bResult = SetEvent(hScmStartEvent);
    dwError = ScmErrorFromBoolean(bResult);
    DPRINT1("ROSLSA scm-start-event-signal-result success=%u error=%lu\n",
            bResult,
            dwError);

    DPRINT("SERVICES: Initialized\n");

    /* Register event handler (used for system shutdown) */
    SetConsoleCtrlHandler(ShutdownHandlerRoutine, TRUE);

    /*
     * Set our shutdown parameters: we want to shutdown after the maintained
     * services (that inherit the default shutdown level of 640).
     */
    SetProcessShutdownParameters(480, SHUTDOWN_NORETRY);

    /* Start auto-start services */
    DPRINT1("ROSLSA scm-autostart-enter\n");
    ScmAutoStartServices();
    DPRINT1("ROSLSA scm-autostart-done\n");

    /* Signal auto-start complete event */
    bResult = SetEvent(hScmAutoStartCompleteEvent);
    dwError = ScmErrorFromBoolean(bResult);
    DPRINT1("ROSLSA scm-autostart-event-signal-result success=%u error=%lu\n",
            bResult,
            dwError);

    /* FIXME: more to do ? */

    /* Release the service start lock */
    DPRINT1("ROSLSA scm-release-start-lock-enter lock=%p\n", Lock);
    ScmReleaseServiceStartLock(&Lock);
    DPRINT1("ROSLSA scm-release-start-lock-done lock=%p\n", Lock);

    /* Initialization finished */
    ScmInitialize = FALSE;
    DPRINT1("ROSLSA scm-initialize-clear\n");

    DPRINT("SERVICES: Running\n");
    DPRINT1("ROSLSA scm-shutdown-wait-enter\n");

    /* Wait until the shutdown event gets signaled */
    dwError = WaitForSingleObject(hScmShutdownEvent, INFINITE);
    DPRINT1("ROSLSA scm-shutdown-wait-done wait=%lu\n", dwError);

done:
    DPRINT1("ROSLSA scm-main-exit\n");
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
