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
#define LSA_RPC_SERVER_ACTIVE       L"LSA_RPC_SERVER_ACTIVE"

BOOL ScmInitialize = FALSE;
BOOL ScmShutdown = FALSE;
static HANDLE hScmShutdownEvent = NULL;


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
    HANDLE hEvent = CreateEventW(NULL, TRUE, FALSE, LSA_RPC_SERVER_ACTIVE);
    if (hEvent == NULL)
    {
        DPRINT1("Failed to create the notification event (Error %lu)\n", GetLastError());
    }
    else
    {
        DPRINT("Wait for the LSA server!\n");
        WaitForSingleObject(hEvent, INFINITE);
        DPRINT("LSA server running!\n");
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
        DPRINT1("Shutdown event received!\n");
        ScmShutdown = TRUE;

        ScmAutoShutdownServices();
        ScmShutdownServiceDatabase();

        /* Set the shutdown event */
        SetEvent(hScmShutdownEvent);
    }

    return TRUE;
}


/*** HACK CORE-12541: Special service accounts initialization HACK ************/

#include <ndk/setypes.h>
#include <sddl.h>
#include <userenv.h>
#include <strsafe.h>

/* Inspired from userenv.dll's CreateUserProfileExW and LoadUserProfileW APIs */
static
BOOL
ScmLogAccountHack(IN LPCWSTR pszAccountName,
                  IN LPCWSTR pszSid,
                  OUT PHKEY phProfile)
{
    BOOL Success = FALSE;
    LONG Error;
    NTSTATUS Status;
    BOOLEAN WasPriv1Set = FALSE, WasPriv2Set = FALSE;
    PSID pSid;
    DWORD dwLength;
    WCHAR szUserHivePath[MAX_PATH];

    DPRINT1("ScmLogAccountsHack(%S, %S)\n", pszAccountName, pszSid);
    if (!pszAccountName || !pszSid || !phProfile)
        return ERROR_INVALID_PARAMETER;

    /* Convert the SID string into a SID. NOTE: No RTL equivalent. */
    if (!ConvertStringSidToSidW(pszSid, &pSid))
    {
        DPRINT1("ConvertStringSidToSidW() failed (error %lu)\n", GetLastError());
        return FALSE;
    }

    /* Determine a suitable profile path */
    dwLength = ARRAYSIZE(szUserHivePath);
    if (!GetProfilesDirectoryW(szUserHivePath, &dwLength))
    {
        DPRINT1("GetProfilesDirectoryW() failed (error %lu)\n", GetLastError());
        goto Quit;
    }

    /* Create user hive name */
    StringCbCatW(szUserHivePath, sizeof(szUserHivePath), L"\\");
    StringCbCatW(szUserHivePath, sizeof(szUserHivePath), pszAccountName);
    StringCbCatW(szUserHivePath, sizeof(szUserHivePath), L"\\ntuser.dat");
    DPRINT("szUserHivePath: %S\n", szUserHivePath);

    /* Magic #1: Create the special user profile if needed */
    if (GetFileAttributesW(szUserHivePath) == INVALID_FILE_ATTRIBUTES)
    {
        if (!CreateUserProfileW(pSid, pszAccountName))
        {
            DPRINT1("CreateUserProfileW() failed (error %lu)\n", GetLastError());
            goto Quit;
        }
    }

    /*
     * Now Da Magiks #2: Manually mount the user profile registry hive
     * aka. manually do what LoadUserProfile does!! But we don't require
     * a security token!
     */

    /* Acquire restore privilege */
    Status = RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE, FALSE, &WasPriv1Set);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE) failed (Error 0x%08lx)\n", Status);
        goto Quit;
    }

    /* Acquire backup privilege */
    Status = RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE, TRUE, FALSE, &WasPriv2Set);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE) failed (Error 0x%08lx)\n", Status);
        RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, WasPriv1Set, FALSE, &WasPriv1Set);
        goto Quit;
    }

    /* Load user registry hive */
    Error = RegLoadKeyW(HKEY_USERS, pszSid, szUserHivePath);

    /* Remove restore and backup privileges */
    RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE, WasPriv2Set, FALSE, &WasPriv2Set);
    RtlAdjustPrivilege(SE_RESTORE_PRIVILEGE, WasPriv1Set, FALSE, &WasPriv1Set);

    /* HACK: Do not fail if the profile has already been loaded! */
    if (Error == ERROR_SHARING_VIOLATION)
        Error = ERROR_SUCCESS;

    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("RegLoadKeyW() failed (Error %ld)\n", Error);
        goto Quit;
    }

    /* Open future HKEY_CURRENT_USER */
    Error = RegOpenKeyExW(HKEY_USERS,
                          pszSid,
                          0,
                          MAXIMUM_ALLOWED,
                          phProfile);
    if (Error != ERROR_SUCCESS)
    {
        DPRINT1("RegOpenKeyExW() failed (Error %ld)\n", Error);
        goto Quit;
    }

    Success = TRUE;

Quit:
    LocalFree(pSid);

    DPRINT1("ScmLogAccountsHack(%S) returned %s\n",
            pszAccountName, Success ? "success" : "failure");

    return Success;
}

static struct
{
    LPCWSTR pszAccountName;
    LPCWSTR pszSid;
    HKEY    hProfile;
} AccountHandles[] = {
//  {L"LocalSystem"   , L"S-1-5-18", NULL},
    {L"LocalService"  , L"S-1-5-19", NULL}, // L"NT AUTHORITY\\LocalService"
    {L"NetworkService", L"S-1-5-20", NULL}, // L"NT AUTHORITY\\NetworkService"
};

static VOID
ScmCleanupServiceAccountsHack(VOID)
{
    UINT i;

    DPRINT1("ScmCleanupServiceAccountsHack()\n");

    for (i = 0; i < ARRAYSIZE(AccountHandles); ++i)
    {
        if (AccountHandles[i].hProfile)
        {
            RegCloseKey(AccountHandles[i].hProfile);
            AccountHandles[i].hProfile = NULL;
        }
    }
}

static BOOL
ScmApplyServiceAccountsHack(VOID)
{
    UINT i;

    DPRINT1("ScmApplyServiceAccountsHack()\n");

    for (i = 0; i < ARRAYSIZE(AccountHandles); ++i)
    {
        if (!ScmLogAccountHack( AccountHandles[i].pszAccountName,
                                AccountHandles[i].pszSid,
                               &AccountHandles[i].hProfile))
        {
            ScmCleanupServiceAccountsHack();
            return FALSE;
        }
    }

    return TRUE;
}

/*************************** END OF HACK CORE-12541 ***************************/


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
    DPRINT("SERVICES: Created start event with handle %p.\n", hScmStartEvent);

    /* Create the auto-start complete event */
    hScmAutoStartCompleteEvent = CreateEventW(NULL, TRUE, FALSE, SCM_AUTOSTARTCOMPLETE_EVENT);
    if (hScmAutoStartCompleteEvent == NULL)
    {
        DPRINT1("SERVICES: Failed to create the auto-start complete event\n");
        goto done;
    }
    DPRINT("SERVICES: created auto-start complete event with handle %p.\n", hScmAutoStartCompleteEvent);

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

    /* Read the control set values */
    if (!ScmGetControlSetValues())
    {
        DPRINT1("SERVICES: Failed to read the control set values\n");
        goto done;
    }

    /* Create the services database */
    dwError = ScmCreateServiceDatabase();
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("SERVICES: Failed to create SCM database (Error %lu)\n", dwError);
        goto done;
    }

    /* Wait for the LSA server */
    ScmWaitForLsa();

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
        DPRINT1("SERVICES: Failed to acquire the service start lock (Error %lu)\n", dwError);
        goto done;
    }

    /* Start the RPC server */
    ScmStartRpcServer();

    /* Signal start event */
    SetEvent(hScmStartEvent);

    DPRINT("SERVICES: Initialized.\n");

    /* Register event handler (used for system shutdown) */
    SetConsoleCtrlHandler(ShutdownHandlerRoutine, TRUE);

    /*
     * Set our shutdown parameters: we want to shutdown after the maintained
     * services (that inherit the default shutdown level of 640).
     */
    SetProcessShutdownParameters(480, SHUTDOWN_NORETRY);

    /*** HACK CORE-12541: Apply service accounts HACK ***/
    ScmApplyServiceAccountsHack();

    /* Start auto-start services */
    ScmAutoStartServices();

    /* Signal auto-start complete event */
    SetEvent(hScmAutoStartCompleteEvent);

    /* FIXME: more to do ? */

    /* Release the service start lock */
    ScmReleaseServiceStartLock(&Lock);

    /* Initialization finished */
    ScmInitialize = FALSE;

    DPRINT("SERVICES: Running.\n");

    /* Wait until the shutdown event gets signaled */
    WaitForSingleObject(hScmShutdownEvent, INFINITE);

    /*** HACK CORE-12541: Cleanup service accounts HACK ***/
    ScmCleanupServiceAccountsHack();

done:
    ScmShutdownSecurity();

    /* Delete our communication named pipe's critical section */
    if (bCanDeleteNamedPipeCriticalSection == TRUE)
        ScmDeleteNamedPipeCriticalSection();

    /* Close the shutdown event */
    if (hScmShutdownEvent != NULL)
        CloseHandle(hScmShutdownEvent);

    /* Close the auto-start complete event */
    if (hScmAutoStartCompleteEvent != NULL)
        CloseHandle(hScmAutoStartCompleteEvent);

    /* Close the start event */
    if (hScmStartEvent != NULL)
        CloseHandle(hScmStartEvent);

    DPRINT("SERVICES: Finished.\n");

    ExitThread(0);
    return 0;
}

/* EOF */
