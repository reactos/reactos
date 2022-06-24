/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Winlogon
 * FILE:            base/system/winlogon/sas.c
 * PURPOSE:         Secure Attention Sequence
 * PROGRAMMERS:     Thomas Weidenmueller (w3seek@users.sourceforge.net)
 *                  Hervé Poussineau (hpoussin@reactos.org)
 *                  Arnav Bhatt (arnavbhatt288@gmail.com)
 * UPDATE HISTORY:
 *                  Created 28/03/2004
 */

/* INCLUDES *****************************************************************/

#include "winlogon.h"

#define WIN32_LEAN_AND_MEAN
#include <aclapi.h>
#include <mmsystem.h>
#include <userenv.h>
#include <ndk/setypes.h>
#include <ndk/sefuncs.h>

/* GLOBALS ******************************************************************/

#define WINLOGON_SAS_CLASS L"SAS Window class"
#define WINLOGON_SAS_TITLE L"SAS window"

#define HK_CTRL_ALT_DEL   0
#define HK_CTRL_SHIFT_ESC 1

// #define EWX_FLAGS_MASK  0x00000014
// #define EWX_ACTION_MASK ~EWX_FLAGS_MASK

// FIXME: At the moment we use this value (select the lowbyte flags and some highbytes ones).
// It should be set such that it makes winlogon accepting only valid flags.
#define EWX_ACTION_MASK 0x5C0F

typedef struct tagLOGOFF_SHUTDOWN_DATA
{
    UINT Flags;
    PWLSESSION Session;
} LOGOFF_SHUTDOWN_DATA, *PLOGOFF_SHUTDOWN_DATA;

static BOOL ExitReactOSInProgress = FALSE;

LUID LuidNone = {0, 0};

/* FUNCTIONS ****************************************************************/

static BOOL
StartTaskManager(
    IN OUT PWLSESSION Session)
{
    LPVOID lpEnvironment;
    BOOL ret;

    if (!Session->Gina.Functions.WlxStartApplication)
        return FALSE;

    if (!CreateEnvironmentBlock(
        &lpEnvironment,
        Session->UserToken,
        TRUE))
    {
        return FALSE;
    }

    ret = Session->Gina.Functions.WlxStartApplication(
        Session->Gina.Context,
        L"Default",
        lpEnvironment,
        L"taskmgr.exe");

    DestroyEnvironmentBlock(lpEnvironment);
    return ret;
}

static BOOL
StartUserShell(
    IN OUT PWLSESSION Session)
{
    LPVOID lpEnvironment = NULL;
    BOOLEAN Old;
    BOOL ret;

    /* Create environment block for the user */
    if (!CreateEnvironmentBlock(&lpEnvironment, Session->UserToken, TRUE))
    {
        WARN("WL: CreateEnvironmentBlock() failed\n");
        return FALSE;
    }

    /* Get privilege */
    /* FIXME: who should do it? winlogon or gina? */
    /* FIXME: reverting to lower privileges after creating user shell? */
    RtlAdjustPrivilege(SE_ASSIGNPRIMARYTOKEN_PRIVILEGE, TRUE, FALSE, &Old);

    ret = Session->Gina.Functions.WlxActivateUserShell(
                Session->Gina.Context,
                L"Default",
                NULL, /* FIXME */
                lpEnvironment);

    DestroyEnvironmentBlock(lpEnvironment);
    return ret;
}


BOOL
SetDefaultLanguage(
    IN PWLSESSION Session)
{
    BOOL ret = FALSE;
    BOOL UserProfile;
    LONG rc;
    HKEY UserKey, hKey = NULL;
    LPCWSTR SubKey, ValueName;
    DWORD dwType, dwSize;
    LPWSTR Value = NULL;
    UNICODE_STRING ValueString;
    NTSTATUS Status;
    LCID Lcid;

    UserProfile = (Session && Session->UserToken);

    if (UserProfile && !ImpersonateLoggedOnUser(Session->UserToken))
    {
        ERR("WL: ImpersonateLoggedOnUser() failed with error %lu\n", GetLastError());
        return FALSE;
        // FIXME: ... or use the default language of the system??
        // UserProfile = FALSE;
    }

    if (UserProfile)
    {
        rc = RegOpenCurrentUser(MAXIMUM_ALLOWED, &UserKey);
        if (rc != ERROR_SUCCESS)
        {
            TRACE("RegOpenCurrentUser() failed with error %lu\n", rc);
            goto cleanup;
        }

        SubKey = L"Control Panel\\International";
        ValueName = L"Locale";
    }
    else
    {
        UserKey = NULL;
        SubKey = L"System\\CurrentControlSet\\Control\\Nls\\Language";
        ValueName = L"Default";
    }

    rc = RegOpenKeyExW(UserKey ? UserKey : HKEY_LOCAL_MACHINE,
                       SubKey,
                       0,
                       KEY_READ,
                       &hKey);

    if (UserKey)
        RegCloseKey(UserKey);

    if (rc != ERROR_SUCCESS)
    {
        TRACE("RegOpenKeyEx() failed with error %lu\n", rc);
        goto cleanup;
    }

    rc = RegQueryValueExW(hKey,
                          ValueName,
                          NULL,
                          &dwType,
                          NULL,
                          &dwSize);
    if (rc != ERROR_SUCCESS)
    {
        TRACE("RegQueryValueEx() failed with error %lu\n", rc);
        goto cleanup;
    }
    else if (dwType != REG_SZ)
    {
        TRACE("Wrong type for %S\\%S registry entry (got 0x%lx, expected 0x%x)\n",
            SubKey, ValueName, dwType, REG_SZ);
        goto cleanup;
    }

    Value = HeapAlloc(GetProcessHeap(), 0, dwSize);
    if (!Value)
    {
        TRACE("HeapAlloc() failed\n");
        goto cleanup;
    }
    rc = RegQueryValueExW(hKey,
                          ValueName,
                          NULL,
                          NULL,
                          (LPBYTE)Value,
                          &dwSize);
    if (rc != ERROR_SUCCESS)
    {
        TRACE("RegQueryValueEx() failed with error %lu\n", rc);
        goto cleanup;
    }

    /* Convert Value to a Lcid */
    ValueString.Length = ValueString.MaximumLength = (USHORT)dwSize;
    ValueString.Buffer = Value;
    Status = RtlUnicodeStringToInteger(&ValueString, 16, (PULONG)&Lcid);
    if (!NT_SUCCESS(Status))
    {
        TRACE("RtlUnicodeStringToInteger() failed with status 0x%08lx\n", Status);
        goto cleanup;
    }

    TRACE("%s language is 0x%08lx\n",
        UserProfile ? "User" : "System", Lcid);
    Status = NtSetDefaultLocale(UserProfile, Lcid);
    if (!NT_SUCCESS(Status))
    {
        TRACE("NtSetDefaultLocale() failed with status 0x%08lx\n", Status);
        goto cleanup;
    }

    ret = TRUE;

cleanup:
    if (Value)
        HeapFree(GetProcessHeap(), 0, Value);

    if (hKey)
        RegCloseKey(hKey);

    if (UserProfile)
        RevertToSelf();

    return ret;
}

BOOL
PlaySoundRoutine(
    IN LPCWSTR FileName,
    IN UINT bLogon,
    IN UINT Flags)
{
    typedef BOOL (WINAPI *PLAYSOUNDW)(LPCWSTR,HMODULE,DWORD);
    typedef UINT (WINAPI *WAVEOUTGETNUMDEVS)(VOID);
    PLAYSOUNDW Play;
    WAVEOUTGETNUMDEVS waveOutGetNumDevs;
    UINT NumDevs;
    HMODULE hLibrary;
    BOOL Ret = FALSE;

    hLibrary = LoadLibraryW(L"winmm.dll");
    if (hLibrary)
    {
        waveOutGetNumDevs = (WAVEOUTGETNUMDEVS)GetProcAddress(hLibrary, "waveOutGetNumDevs");
        if (waveOutGetNumDevs)
        {
            NumDevs = waveOutGetNumDevs();
            if (!NumDevs)
            {
                if (!bLogon)
                {
                    Beep(440, 125);
                }
                FreeLibrary(hLibrary);
                return FALSE;
            }
        }

        Play = (PLAYSOUNDW)GetProcAddress(hLibrary, "PlaySoundW");
        if (Play)
        {
            Ret = Play(FileName, NULL, Flags);
        }
        FreeLibrary(hLibrary);
    }

    return Ret;
}

DWORD
WINAPI
PlayLogonSoundThread(
    IN LPVOID lpParameter)
{
    BYTE TokenUserBuffer[256];
    PTOKEN_USER pTokenUser = (TOKEN_USER*)TokenUserBuffer;
    ULONG Length;
    HKEY hKey;
    WCHAR wszBuffer[MAX_PATH] = {0};
    WCHAR wszDest[MAX_PATH];
    DWORD dwSize = sizeof(wszBuffer), dwType;
    SERVICE_STATUS_PROCESS Info;
    UNICODE_STRING SidString;
    NTSTATUS Status;
    ULONG Index = 0;
    SC_HANDLE hSCManager, hService;

    //
    // FIXME: Isn't it possible to *JUST* impersonate the current user
    // *AND* open its HKCU??
    //

    /* Get SID of current user */
    Status = NtQueryInformationToken((HANDLE)lpParameter,
                                     TokenUser,
                                     TokenUserBuffer,
                                     sizeof(TokenUserBuffer),
                                     &Length);
    if (!NT_SUCCESS(Status))
    {
        ERR("NtQueryInformationToken failed: %x!\n", Status);
        return 0;
    }

    /* Convert SID to string */
    RtlInitEmptyUnicodeString(&SidString, wszBuffer, sizeof(wszBuffer));
    Status = RtlConvertSidToUnicodeString(&SidString, pTokenUser->User.Sid, FALSE);
    if (!NT_SUCCESS(Status))
    {
        ERR("RtlConvertSidToUnicodeString failed: %x!\n", Status);
        return 0;
    }

    /* Build path to logon sound registry key.
       Note: We can't use HKCU here, because Winlogon is owned by SYSTEM user */
    if (FAILED(StringCbCopyW(wszBuffer + SidString.Length/sizeof(WCHAR),
                             sizeof(wszBuffer) - SidString.Length,
                             L"\\AppEvents\\Schemes\\Apps\\.Default\\WindowsLogon\\.Current")))
    {
        /* SID is too long. Should not happen. */
        ERR("StringCbCopyW failed!\n");
        return 0;
    }

    /* Open registry key and query sound path */
    if (RegOpenKeyExW(HKEY_USERS, wszBuffer, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW(%ls) failed!\n", wszBuffer);
        return 0;
    }

    if (RegQueryValueExW(hKey, NULL, NULL, &dwType,
                      (LPBYTE)wszBuffer, &dwSize) != ERROR_SUCCESS ||
        (dwType != REG_SZ && dwType != REG_EXPAND_SZ))
    {
        ERR("RegQueryValueExW failed!\n");
        RegCloseKey(hKey);
        return 0;
    }

    RegCloseKey(hKey);

    if (!wszBuffer[0])
    {
        /* No sound has been set */
        ERR("No sound has been set\n");
        return 0;
    }

    /* Expand environment variables */
    if (!ExpandEnvironmentStringsW(wszBuffer, wszDest, MAX_PATH))
    {
        ERR("ExpandEnvironmentStringsW failed!\n");
        return 0;
    }

    /* Open the service manager */
    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!hSCManager)
    {
        ERR("OpenSCManager failed (%x)\n", GetLastError());
        return 0;
    }

    /* Open the wdmaud service */
    hService = OpenServiceW(hSCManager, L"wdmaud", GENERIC_READ);
    if (!hService)
    {
        /* The service is not installed */
        TRACE("Failed to open wdmaud service (%x)\n", GetLastError());
        CloseServiceHandle(hSCManager);
        return 0;
    }

    /* Wait for wdmaud to start */
    do
    {
        if (!QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&Info, sizeof(SERVICE_STATUS_PROCESS), &dwSize))
        {
            TRACE("QueryServiceStatusEx failed (%x)\n", GetLastError());
            break;
        }

        if (Info.dwCurrentState == SERVICE_RUNNING)
            break;

        Sleep(1000);

    } while (Index++ < 20);

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);

    /* If wdmaud is not running exit */
    if (Info.dwCurrentState != SERVICE_RUNNING)
    {
        WARN("wdmaud has not started!\n");
        return 0;
    }

    /* Sound subsystem is running. Play logon sound. */
    TRACE("Playing logon sound: %ls\n", wszDest);
    PlaySoundRoutine(wszDest, TRUE, SND_FILENAME);
    return 0;
}

static
VOID
PlayLogonSound(
    IN OUT PWLSESSION Session)
{
    HANDLE hThread;

    hThread = CreateThread(NULL, 0, PlayLogonSoundThread, (PVOID)Session->UserToken, 0, NULL);
    if (hThread)
        CloseHandle(hThread);
}

static
VOID
RestoreAllConnections(PWLSESSION Session)
{
    DWORD dRet;
    HANDLE hEnum;
    LPNETRESOURCE lpRes;
    DWORD dSize = 0x1000;
    DWORD dCount = -1;
    LPNETRESOURCE lpCur;
    BOOL UserProfile;

    UserProfile = (Session && Session->UserToken);
    if (!UserProfile)
    {
        return;
    }

    if (!ImpersonateLoggedOnUser(Session->UserToken))
    {
        ERR("WL: ImpersonateLoggedOnUser() failed with error %lu\n", GetLastError());
        return;
    }

    dRet = WNetOpenEnum(RESOURCE_REMEMBERED, RESOURCETYPE_DISK, 0, NULL, &hEnum);
    if (dRet != WN_SUCCESS)
    {
        ERR("Failed to open enumeration: %lu\n", dRet);
        goto quit;
    }

    lpRes = HeapAlloc(GetProcessHeap(), 0, dSize);
    if (!lpRes)
    {
        ERR("Failed to allocate memory\n");
        WNetCloseEnum(hEnum);
        goto quit;
    }

    do
    {
        dSize = 0x1000;
        dCount = -1;

        memset(lpRes, 0, dSize);
        dRet = WNetEnumResource(hEnum, &dCount, lpRes, &dSize);
        if (dRet == WN_SUCCESS || dRet == WN_MORE_DATA)
        {
            lpCur = lpRes;
            for (; dCount; dCount--)
            {
                WNetAddConnection(lpCur->lpRemoteName, NULL, lpCur->lpLocalName);
                lpCur++;
            }
        }
    } while (dRet != WN_NO_MORE_ENTRIES);

    HeapFree(GetProcessHeap(), 0, lpRes);
    WNetCloseEnum(hEnum);

quit:
    RevertToSelf();
}

static
BOOL
HandleLogon(
    IN OUT PWLSESSION Session)
{
    PROFILEINFOW ProfileInfo;
    BOOL ret = FALSE;

    /* Loading personal settings */
    DisplayStatusMessage(Session, Session->WinlogonDesktop, IDS_LOADINGYOURPERSONALSETTINGS);
    ProfileInfo.hProfile = INVALID_HANDLE_VALUE;
    if (0 == (Session->Options & WLX_LOGON_OPT_NO_PROFILE))
    {
        if (Session->Profile == NULL
         || (Session->Profile->dwType != WLX_PROFILE_TYPE_V1_0
          && Session->Profile->dwType != WLX_PROFILE_TYPE_V2_0))
        {
            ERR("WL: Wrong profile\n");
            goto cleanup;
        }

        /* Load the user profile */
        ZeroMemory(&ProfileInfo, sizeof(PROFILEINFOW));
        ProfileInfo.dwSize = sizeof(PROFILEINFOW);
        ProfileInfo.dwFlags = 0;
        ProfileInfo.lpUserName = Session->MprNotifyInfo.pszUserName;
        ProfileInfo.lpProfilePath = Session->Profile->pszProfile;
        if (Session->Profile->dwType >= WLX_PROFILE_TYPE_V2_0)
        {
            ProfileInfo.lpDefaultPath = Session->Profile->pszNetworkDefaultUserProfile;
            ProfileInfo.lpServerName = Session->Profile->pszServerName;
            ProfileInfo.lpPolicyPath = Session->Profile->pszPolicy;
        }

        if (!LoadUserProfileW(Session->UserToken, &ProfileInfo))
        {
            ERR("WL: LoadUserProfileW() failed\n");
            goto cleanup;
        }
    }

    /* Create environment block for the user */
    if (!CreateUserEnvironment(Session))
    {
        WARN("WL: SetUserEnvironment() failed\n");
        goto cleanup;
    }

    CallNotificationDlls(Session, LogonHandler);

    DisplayStatusMessage(Session, Session->WinlogonDesktop, IDS_APPLYINGYOURPERSONALSETTINGS);
    UpdatePerUserSystemParameters(0, TRUE);

    /* Set default user language */
    if (!SetDefaultLanguage(Session))
    {
        WARN("WL: SetDefaultLanguage() failed\n");
        goto cleanup;
    }

    /* Allow winsta and desktop access for this session */
    if (!AllowAccessOnSession(Session))
    {
        WARN("WL: AllowAccessOnSession() failed to give winsta & desktop access for this session\n");
        goto cleanup;
    }

    /* Connect remote resources */
    RestoreAllConnections(Session);

    if (!StartUserShell(Session))
    {
        //WCHAR StatusMsg[256];
        WARN("WL: WlxActivateUserShell() failed\n");
        //LoadStringW(hAppInstance, IDS_FAILEDACTIVATEUSERSHELL, StatusMsg, sizeof(StatusMsg) / sizeof(StatusMsg[0]));
        //MessageBoxW(0, StatusMsg, NULL, MB_ICONERROR);
        goto cleanup;
    }

    CallNotificationDlls(Session, StartShellHandler);

    if (!InitializeScreenSaver(Session))
        WARN("WL: Failed to initialize screen saver\n");

    Session->hProfileInfo = ProfileInfo.hProfile;

    /* Logon has succeeded. Play sound. */
    PlayLogonSound(Session);

    ret = TRUE;

cleanup:
    if (Session->Profile)
    {
        HeapFree(GetProcessHeap(), 0, Session->Profile->pszProfile);
        HeapFree(GetProcessHeap(), 0, Session->Profile);
    }
    Session->Profile = NULL;
    if (!ret && ProfileInfo.hProfile != INVALID_HANDLE_VALUE)
    {
        UnloadUserProfile(Session->UserToken, ProfileInfo.hProfile);
    }
    RemoveStatusMessage(Session);
    if (!ret)
    {
        SetWindowStationUser(Session->InteractiveWindowStation,
                             &LuidNone, NULL, 0);
        CloseHandle(Session->UserToken);
        Session->UserToken = NULL;
    }

    if (ret)
    {
        SwitchDesktop(Session->ApplicationDesktop);
        Session->LogonState = STATE_LOGGED_ON;
    }

    return ret;
}


static
DWORD
WINAPI
LogoffShutdownThread(
    LPVOID Parameter)
{
    DWORD ret = 1;
    PLOGOFF_SHUTDOWN_DATA LSData = (PLOGOFF_SHUTDOWN_DATA)Parameter;
    UINT uFlags;

    if (LSData->Session->UserToken != NULL &&
        !ImpersonateLoggedOnUser(LSData->Session->UserToken))
    {
        ERR("ImpersonateLoggedOnUser() failed with error %lu\n", GetLastError());
        return 0;
    }

    // FIXME: To be really fixed: need to check what needs to be kept and what needs to be removed there.
    //
    // uFlags = EWX_INTERNAL_KILL_USER_APPS | (LSData->Flags & EWX_FLAGS_MASK) |
             // ((LSData->Flags & EWX_ACTION_MASK) == EWX_LOGOFF ? EWX_CALLER_WINLOGON_LOGOFF : 0);

    uFlags = EWX_CALLER_WINLOGON | (LSData->Flags & 0x0F);

    TRACE("In LogoffShutdownThread with uFlags == 0x%x; exit_in_progress == %s\n",
        uFlags, ExitReactOSInProgress ? "true" : "false");

    ExitReactOSInProgress = TRUE;

    /* Close processes of the interactive user */
    if (!ExitWindowsEx(uFlags, 0))
    {
        ERR("Unable to kill user apps, error %lu\n", GetLastError());
        ret = 0;
    }

    /* Cancel all the user connections */
    WNetClearConnections(NULL);

    if (LSData->Session->UserToken)
        RevertToSelf();

    return ret;
}

static
DWORD
WINAPI
KillComProcesses(
    LPVOID Parameter)
{
    DWORD ret = 1;
    PLOGOFF_SHUTDOWN_DATA LSData = (PLOGOFF_SHUTDOWN_DATA)Parameter;

    TRACE("In KillComProcesses\n");

    if (LSData->Session->UserToken != NULL &&
        !ImpersonateLoggedOnUser(LSData->Session->UserToken))
    {
        ERR("ImpersonateLoggedOnUser() failed with error %lu\n", GetLastError());
        return 0;
    }

    /* Attempt to kill remaining processes. No notifications needed. */
    if (!ExitWindowsEx(EWX_CALLER_WINLOGON | EWX_NONOTIFY | EWX_FORCE | EWX_LOGOFF, 0))
    {
        ERR("Unable to kill COM apps, error %lu\n", GetLastError());
        ret = 0;
    }

    if (LSData->Session->UserToken)
        RevertToSelf();

    return ret;
}

static
NTSTATUS
CreateLogoffSecurityAttributes(
    OUT PSECURITY_ATTRIBUTES* ppsa)
{
    /* The following code is not working yet and messy */
    /* Still, it gives some ideas about data types and functions involved and */
    /* required to set up a SECURITY_DESCRIPTOR for a SECURITY_ATTRIBUTES */
    /* instance for a thread, to allow that  thread to ImpersonateLoggedOnUser(). */
    /* Specifically THREAD_SET_THREAD_TOKEN is required. */
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    PSECURITY_ATTRIBUTES psa = 0;
    BYTE* pMem;
    PACL pACL;
    EXPLICIT_ACCESS Access;
    PSID pEveryoneSID = NULL;
    static SID_IDENTIFIER_AUTHORITY WorldAuthority = { SECURITY_WORLD_SID_AUTHORITY };

    *ppsa = NULL;

    // Let's first try to enumerate what kind of data we need for this to ever work:
    // 1.  The Winlogon SID, to be able to give it THREAD_SET_THREAD_TOKEN.
    // 2.  The users SID (the user trying to logoff, or rather shut down the system).
    // 3.  At least two EXPLICIT_ACCESS instances:
    // 3.1 One for Winlogon itself, giving it the rights
    //     required to THREAD_SET_THREAD_TOKEN (as it's needed to successfully call
    //     ImpersonateLoggedOnUser).
    // 3.2 One for the user, to allow *that* thread to perform its work.
    // 4.  An ACL to hold the these EXPLICIT_ACCESS ACE's.
    // 5.  A SECURITY_DESCRIPTOR to hold the ACL, and finally.
    // 6.  A SECURITY_ATTRIBUTES instance to pull all of this required stuff
    //     together, to hand it to CreateThread.
    //
    // However, it seems struct LOGOFF_SHUTDOWN_DATA doesn't contain
    // these required SID's, why they'd have to be added.
    // The Winlogon's own SID should probably only be created once,
    // while the user's SID obviously must be created for each new user.
    // Might as well store it when the user logs on?

    if(!AllocateAndInitializeSid(&WorldAuthority,
                                 1,
                                 SECURITY_WORLD_RID,
                                 0, 0, 0, 0, 0, 0, 0,
                                 &pEveryoneSID))
    {
        ERR("Failed to initialize security descriptor for logoff thread!\n");
        return STATUS_UNSUCCESSFUL;
    }

    /* set up the required security attributes to be able to shut down */
    /* To save space and time, allocate a single block of memory holding */
    /* both SECURITY_ATTRIBUTES and SECURITY_DESCRIPTOR */
    pMem = HeapAlloc(GetProcessHeap(),
                     0,
                     sizeof(SECURITY_ATTRIBUTES) +
                     SECURITY_DESCRIPTOR_MIN_LENGTH +
                     sizeof(ACL));
    if (!pMem)
    {
        ERR("Failed to allocate memory for logoff security descriptor!\n");
        return STATUS_NO_MEMORY;
    }

    /* Note that the security descriptor needs to be in _absolute_ format, */
    /* meaning its members must be pointers to other structures, rather */
    /* than the relative format using offsets */
    psa = (PSECURITY_ATTRIBUTES)pMem;
    SecurityDescriptor = (PSECURITY_DESCRIPTOR)(pMem + sizeof(SECURITY_ATTRIBUTES));
    pACL = (PACL)(((PBYTE)SecurityDescriptor) + SECURITY_DESCRIPTOR_MIN_LENGTH);

    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    // The ACE will allow this thread to log off (and shut down the system, currently).
    ZeroMemory(&Access, sizeof(Access));
    Access.grfAccessPermissions = THREAD_SET_THREAD_TOKEN;
    Access.grfAccessMode = SET_ACCESS; // GRANT_ACCESS?
    Access.grfInheritance = NO_INHERITANCE;
    Access.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    Access.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    Access.Trustee.ptstrName = pEveryoneSID;

    if (SetEntriesInAcl(1, &Access, NULL, &pACL) != ERROR_SUCCESS)
    {
        ERR("Failed to set Access Rights for logoff thread. Logging out will most likely fail.\n");

        HeapFree(GetProcessHeap(), 0, pMem);
        return STATUS_UNSUCCESSFUL;
    }

    if (!InitializeSecurityDescriptor(SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION))
    {
        ERR("Failed to initialize security descriptor for logoff thread!\n");
        HeapFree(GetProcessHeap(), 0, pMem);
        return STATUS_UNSUCCESSFUL;
    }

    if (!SetSecurityDescriptorDacl(SecurityDescriptor,
                                   TRUE,     // bDaclPresent flag
                                   pACL,
                                   FALSE))   // not a default DACL
    {
        ERR("SetSecurityDescriptorDacl Error %lu\n", GetLastError());
        HeapFree(GetProcessHeap(), 0, pMem);
        return STATUS_UNSUCCESSFUL;
    }

    psa->nLength = sizeof(SECURITY_ATTRIBUTES);
    psa->lpSecurityDescriptor = SecurityDescriptor;
    psa->bInheritHandle = FALSE;

    *ppsa = psa;

    return STATUS_SUCCESS;
}

static
VOID
DestroyLogoffSecurityAttributes(
    IN PSECURITY_ATTRIBUTES psa)
{
    if (psa)
    {
        HeapFree(GetProcessHeap(), 0, psa);
    }
}


static
NTSTATUS
HandleLogoff(
    IN OUT PWLSESSION Session,
    IN UINT Flags)
{
    PLOGOFF_SHUTDOWN_DATA LSData;
    PSECURITY_ATTRIBUTES psa;
    HANDLE hThread;
    DWORD exitCode;
    NTSTATUS Status;

    /* Prepare data for logoff thread */
    LSData = HeapAlloc(GetProcessHeap(), 0, sizeof(LOGOFF_SHUTDOWN_DATA));
    if (!LSData)
    {
        ERR("Failed to allocate mem for thread data\n");
        return STATUS_NO_MEMORY;
    }
    LSData->Flags = Flags;
    LSData->Session = Session;

    Status = CreateLogoffSecurityAttributes(&psa);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to create a required security descriptor. Status 0x%08lx\n", Status);
        HeapFree(GetProcessHeap(), 0, LSData);
        return Status;
    }

    /* Run logoff thread */
    hThread = CreateThread(psa, 0, LogoffShutdownThread, (LPVOID)LSData, 0, NULL);
    if (!hThread)
    {
        ERR("Unable to create logoff thread, error %lu\n", GetLastError());
        DestroyLogoffSecurityAttributes(psa);
        HeapFree(GetProcessHeap(), 0, LSData);
        return STATUS_UNSUCCESSFUL;
    }
    WaitForSingleObject(hThread, INFINITE);
    if (!GetExitCodeThread(hThread, &exitCode))
    {
        ERR("Unable to get exit code of logoff thread (error %lu)\n", GetLastError());
        CloseHandle(hThread);
        DestroyLogoffSecurityAttributes(psa);
        HeapFree(GetProcessHeap(), 0, LSData);
        return STATUS_UNSUCCESSFUL;
    }
    CloseHandle(hThread);
    if (exitCode == 0)
    {
        ERR("Logoff thread returned failure\n");
        DestroyLogoffSecurityAttributes(psa);
        HeapFree(GetProcessHeap(), 0, LSData);
        return STATUS_UNSUCCESSFUL;
    }

    SwitchDesktop(Session->WinlogonDesktop);

    // TODO: Play logoff sound!

    SetWindowStationUser(Session->InteractiveWindowStation,
                         &LuidNone, NULL, 0);

    // DisplayStatusMessage(Session, Session->WinlogonDesktop, IDS_LOGGINGOFF);

    // FIXME: Closing network connections!
    // DisplayStatusMessage(Session, Session->WinlogonDesktop, IDS_CLOSINGNETWORKCONNECTIONS);

    /* Kill remaining COM apps. Only at logoff! */
    hThread = CreateThread(psa, 0, KillComProcesses, (LPVOID)LSData, 0, NULL);
    if (hThread)
    {
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
    }

    /* We're done with the SECURITY_DESCRIPTOR */
    DestroyLogoffSecurityAttributes(psa);
    psa = NULL;

    HeapFree(GetProcessHeap(), 0, LSData);

    DisplayStatusMessage(Session, Session->WinlogonDesktop, IDS_SAVEYOURSETTINGS);

    UnloadUserProfile(Session->UserToken, Session->hProfileInfo);

    CallNotificationDlls(Session, LogoffHandler);

    CloseHandle(Session->UserToken);
    UpdatePerUserSystemParameters(0, FALSE);
    Session->LogonState = STATE_LOGGED_OFF;
    Session->UserToken = NULL;

    return STATUS_SUCCESS;
}

static
INT_PTR
CALLBACK
ShutdownComputerWindowProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (uMsg)
    {
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_BTNSHTDOWNCOMPUTER:
                    EndDialog(hwndDlg, IDC_BTNSHTDOWNCOMPUTER);
                    return TRUE;
            }
            break;
        }
        case WM_INITDIALOG:
        {
            RemoveMenu(GetSystemMenu(hwndDlg, FALSE), SC_CLOSE, MF_BYCOMMAND);
            SetFocus(GetDlgItem(hwndDlg, IDC_BTNSHTDOWNCOMPUTER));
            return TRUE;
        }
    }
    return FALSE;
}

static
VOID
UninitializeSAS(
    IN OUT PWLSESSION Session)
{
    if (Session->SASWindow)
    {
        DestroyWindow(Session->SASWindow);
        Session->SASWindow = NULL;
    }
    if (Session->hEndOfScreenSaverThread)
        SetEvent(Session->hEndOfScreenSaverThread);
    UnregisterClassW(WINLOGON_SAS_CLASS, hAppInstance);
}

NTSTATUS
HandleShutdown(
    IN OUT PWLSESSION Session,
    IN DWORD wlxAction)
{
    PLOGOFF_SHUTDOWN_DATA LSData;
    HANDLE hThread;
    DWORD exitCode;
    BOOLEAN Old;

    // SwitchDesktop(Session->WinlogonDesktop);

    /* If the system is rebooting, show the appropriate string */
    if (wlxAction == WLX_SAS_ACTION_SHUTDOWN_REBOOT)
        DisplayStatusMessage(Session, Session->WinlogonDesktop, IDS_REACTOSISRESTARTING);
    else
        DisplayStatusMessage(Session, Session->WinlogonDesktop, IDS_REACTOSISSHUTTINGDOWN);

    /* Prepare data for shutdown thread */
    LSData = HeapAlloc(GetProcessHeap(), 0, sizeof(LOGOFF_SHUTDOWN_DATA));
    if (!LSData)
    {
        ERR("Failed to allocate mem for thread data\n");
        return STATUS_NO_MEMORY;
    }
    if (wlxAction == WLX_SAS_ACTION_SHUTDOWN_POWER_OFF)
        LSData->Flags = EWX_POWEROFF;
    else if (wlxAction == WLX_SAS_ACTION_SHUTDOWN_REBOOT)
        LSData->Flags = EWX_REBOOT;
    else
        LSData->Flags = EWX_SHUTDOWN;
    LSData->Session = Session;

    // FIXME: We may need to specify this flag to really force application kill
    // (we are shutting down ReactOS, not just logging off so no hangs, etc...
    // should be allowed).
    // LSData->Flags |= EWX_FORCE;

    /* Run shutdown thread */
    hThread = CreateThread(NULL, 0, LogoffShutdownThread, (LPVOID)LSData, 0, NULL);
    if (!hThread)
    {
        ERR("Unable to create shutdown thread, error %lu\n", GetLastError());
        HeapFree(GetProcessHeap(), 0, LSData);
        return STATUS_UNSUCCESSFUL;
    }
    WaitForSingleObject(hThread, INFINITE);
    HeapFree(GetProcessHeap(), 0, LSData);
    if (!GetExitCodeThread(hThread, &exitCode))
    {
        ERR("Unable to get exit code of shutdown thread (error %lu)\n", GetLastError());
        CloseHandle(hThread);
        return STATUS_UNSUCCESSFUL;
    }
    CloseHandle(hThread);
    if (exitCode == 0)
    {
        ERR("Shutdown thread returned failure\n");
        return STATUS_UNSUCCESSFUL;
    }

    CallNotificationDlls(Session, ShutdownHandler);

    /* Destroy SAS window */
    UninitializeSAS(Session);

    /* Now we can shut down NT */
    ERR("Shutting down NT...\n");
    RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE, TRUE, FALSE, &Old);
    if (wlxAction == WLX_SAS_ACTION_SHUTDOWN_REBOOT)
    {
        NtShutdownSystem(ShutdownReboot);
    }
    else
    {
        if (FALSE)
        {
            /* FIXME - only show this dialog if it's a shutdown and the computer doesn't support APM */
            DialogBox(hAppInstance, MAKEINTRESOURCE(IDD_SHUTDOWNCOMPUTER),
                      GetDesktopWindow(), ShutdownComputerWindowProc);
        }
        NtShutdownSystem(ShutdownNoReboot);
    }
    RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE, Old, FALSE, &Old);
    return STATUS_SUCCESS;
}

static
VOID
DoGenericAction(
    IN OUT PWLSESSION Session,
    IN DWORD wlxAction)
{
    switch (wlxAction)
    {
        case WLX_SAS_ACTION_LOGON: /* 0x01 */
            if (Session->LogonState == STATE_LOGGED_OFF_SAS)
            {
                if (!HandleLogon(Session))
                {
                    Session->Gina.Functions.WlxDisplaySASNotice(Session->Gina.Context);
                    CallNotificationDlls(Session, LogonHandler);
                }
            }
            break;
        case WLX_SAS_ACTION_NONE: /* 0x02 */
            if (Session->LogonState == STATE_LOGGED_OFF_SAS)
            {
                Session->LogonState = STATE_LOGGED_OFF;
                Session->Gina.Functions.WlxDisplaySASNotice(Session->Gina.Context);
            }
            else if (Session->LogonState == STATE_LOGGED_ON_SAS)
            {
                Session->LogonState = STATE_LOGGED_ON;
            }
            else if (Session->LogonState == STATE_LOCKED_SAS)
            {
                Session->LogonState = STATE_LOCKED;
                Session->Gina.Functions.WlxDisplayLockedNotice(Session->Gina.Context);
            }
            break;
        case WLX_SAS_ACTION_LOCK_WKSTA: /* 0x03 */
            if (Session->Gina.Functions.WlxIsLockOk(Session->Gina.Context))
            {
                SwitchDesktop(Session->WinlogonDesktop);
                Session->LogonState = STATE_LOCKED;
                Session->Gina.Functions.WlxDisplayLockedNotice(Session->Gina.Context);
                CallNotificationDlls(Session, LockHandler);
            }
            break;
        case WLX_SAS_ACTION_LOGOFF: /* 0x04 */
        case WLX_SAS_ACTION_SHUTDOWN: /* 0x05 */
        case WLX_SAS_ACTION_SHUTDOWN_POWER_OFF: /* 0x0a */
        case WLX_SAS_ACTION_SHUTDOWN_REBOOT: /* 0x0b */
            if (Session->LogonState != STATE_LOGGED_OFF)
            {
                if (!Session->Gina.Functions.WlxIsLogoffOk(Session->Gina.Context))
                    break;
                if (!NT_SUCCESS(HandleLogoff(Session, EWX_LOGOFF)))
                {
                    RemoveStatusMessage(Session);
                    break;
                }
                Session->Gina.Functions.WlxLogoff(Session->Gina.Context);
            }
            if (WLX_SHUTTINGDOWN(wlxAction))
            {
                // FIXME: WlxShutdown should be done from inside HandleShutdown,
                // after having displayed "ReactOS is shutting down" message.
                Session->Gina.Functions.WlxShutdown(Session->Gina.Context, wlxAction);
                if (!NT_SUCCESS(HandleShutdown(Session, wlxAction)))
                {
                    RemoveStatusMessage(Session);
                    Session->Gina.Functions.WlxDisplaySASNotice(Session->Gina.Context);
                }
            }
            else
            {
                RemoveStatusMessage(Session);
                Session->Gina.Functions.WlxDisplaySASNotice(Session->Gina.Context);
            }
            break;
        case WLX_SAS_ACTION_TASKLIST: /* 0x07 */
            SwitchDesktop(Session->ApplicationDesktop);
            Session->LogonState = STATE_LOGGED_ON;
            StartTaskManager(Session);
            break;
        case WLX_SAS_ACTION_UNLOCK_WKSTA: /* 0x08 */
            SwitchDesktop(Session->ApplicationDesktop);
            Session->LogonState = STATE_LOGGED_ON;
            CallNotificationDlls(Session, UnlockHandler);
            break;
        default:
            WARN("Unknown SAS action 0x%lx\n", wlxAction);
    }
}

static
VOID
DispatchSAS(
    IN OUT PWLSESSION Session,
    IN DWORD dwSasType)
{
    DWORD wlxAction = WLX_SAS_ACTION_NONE;
    PSID LogonSid = NULL; /* FIXME */
    BOOL bSecure = TRUE;

    switch (dwSasType)
    {
        case WLX_SAS_TYPE_CTRL_ALT_DEL:
            switch (Session->LogonState)
            {
                case STATE_INIT:
                    Session->LogonState = STATE_LOGGED_OFF;
                    Session->Gina.Functions.WlxDisplaySASNotice(Session->Gina.Context);
                    return;

                case STATE_LOGGED_OFF:
                    Session->LogonState = STATE_LOGGED_OFF_SAS;

                    CloseAllDialogWindows();

                    Session->Options = 0;

                    wlxAction = (DWORD)Session->Gina.Functions.WlxLoggedOutSAS(
                        Session->Gina.Context,
                        Session->SASAction,
                        &Session->LogonId,
                        LogonSid,
                        &Session->Options,
                        &Session->UserToken,
                        &Session->MprNotifyInfo,
                        (PVOID*)&Session->Profile);
                    break;

                case STATE_LOGGED_OFF_SAS:
                    /* Ignore SAS if we are already in an SAS state */
                    return;

                case STATE_LOGGED_ON:
                    Session->LogonState = STATE_LOGGED_ON_SAS;
                    wlxAction = (DWORD)Session->Gina.Functions.WlxLoggedOnSAS(Session->Gina.Context, dwSasType, NULL);
                    break;

                case STATE_LOGGED_ON_SAS:
                    /* Ignore SAS if we are already in an SAS state */
                    return;

                case STATE_LOCKED:
                    Session->LogonState = STATE_LOCKED_SAS;

                    CloseAllDialogWindows();

                    wlxAction = (DWORD)Session->Gina.Functions.WlxWkstaLockedSAS(Session->Gina.Context, dwSasType);
                    break;

                case STATE_LOCKED_SAS:
                    /* Ignore SAS if we are already in an SAS state */
                    return;

                default:
                    return;
            }
            break;

        case WLX_SAS_TYPE_TIMEOUT:
            return;

        case WLX_SAS_TYPE_SCRNSVR_TIMEOUT:
            if (!Session->Gina.Functions.WlxScreenSaverNotify(Session->Gina.Context, &bSecure))
            {
                /* Skip start of screen saver */
                SetEvent(Session->hEndOfScreenSaver);
            }
            else
            {
                StartScreenSaver(Session);
                if (bSecure)
                {
                    wlxAction = WLX_SAS_ACTION_LOCK_WKSTA;
//                    DoGenericAction(Session, WLX_SAS_ACTION_LOCK_WKSTA);
                }
            }
            break;

        case WLX_SAS_TYPE_SCRNSVR_ACTIVITY:
            SetEvent(Session->hUserActivity);
            break;
    }

    DoGenericAction(Session, wlxAction);
}

static
BOOL
RegisterHotKeys(
    IN PWLSESSION Session,
    IN HWND hwndSAS)
{
    /* Register Ctrl+Alt+Del Hotkey */
    if (!RegisterHotKey(hwndSAS, HK_CTRL_ALT_DEL, MOD_CONTROL | MOD_ALT, VK_DELETE))
    {
        ERR("WL: Unable to register Ctrl+Alt+Del hotkey!\n");
        return FALSE;
    }

    /* Register Ctrl+Shift+Esc (optional) */
    Session->TaskManHotkey = RegisterHotKey(hwndSAS, HK_CTRL_SHIFT_ESC, MOD_CONTROL | MOD_SHIFT, VK_ESCAPE);
    if (!Session->TaskManHotkey)
        WARN("WL: Warning: Unable to register Ctrl+Alt+Esc hotkey!\n");
    return TRUE;
}

static
BOOL
UnregisterHotKeys(
    IN PWLSESSION Session,
    IN HWND hwndSAS)
{
    /* Unregister hotkeys */
    UnregisterHotKey(hwndSAS, HK_CTRL_ALT_DEL);

    if (Session->TaskManHotkey)
        UnregisterHotKey(hwndSAS, HK_CTRL_SHIFT_ESC);

    return TRUE;
}

BOOL
WINAPI
HandleMessageBeep(UINT uType)
{
    LPWSTR EventName;

    switch(uType)
    {
    case 0xFFFFFFFF:
        EventName = NULL;
        break;
    case MB_OK:
        EventName = L"SystemDefault";
        break;
    case MB_ICONASTERISK:
        EventName = L"SystemAsterisk";
        break;
    case MB_ICONEXCLAMATION:
        EventName = L"SystemExclamation";
        break;
    case MB_ICONHAND:
        EventName = L"SystemHand";
        break;
    case MB_ICONQUESTION:
        EventName = L"SystemQuestion";
        break;
    default:
        WARN("Unhandled type %d\n", uType);
        EventName = L"SystemDefault";
    }

    return PlaySoundRoutine(EventName, FALSE, SND_ALIAS | SND_NOWAIT | SND_NOSTOP | SND_ASYNC);
}

static
LRESULT
CALLBACK
SASWindowProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    PWLSESSION Session = (PWLSESSION)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_HOTKEY:
        {
            switch (lParam)
            {
                case MAKELONG(MOD_CONTROL | MOD_ALT, VK_DELETE):
                {
                    TRACE("SAS: CONTROL+ALT+DELETE\n");
                    if (!Session->Gina.UseCtrlAltDelete)
                        break;
                    PostMessageW(Session->SASWindow, WLX_WM_SAS, WLX_SAS_TYPE_CTRL_ALT_DEL, 0);
                    return TRUE;
                }
                case MAKELONG(MOD_CONTROL | MOD_SHIFT, VK_ESCAPE):
                {
                    TRACE("SAS: CONTROL+SHIFT+ESCAPE\n");
                    if (Session->LogonState == STATE_LOGGED_ON)
                        DoGenericAction(Session, WLX_SAS_ACTION_TASKLIST);
                    return TRUE;
                }
            }
            break;
        }
        case WM_CREATE:
        {
            /* Get the session pointer from the create data */
            Session = (PWLSESSION)((LPCREATESTRUCT)lParam)->lpCreateParams;

            /* Save the Session pointer */
            SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (LONG_PTR)Session);
            if (GetSetupType())
                return TRUE;
            return RegisterHotKeys(Session, hwndDlg);
        }
        case WM_DESTROY:
        {
            if (!GetSetupType())
                UnregisterHotKeys(Session, hwndDlg);
            return TRUE;
        }
        case WM_SETTINGCHANGE:
        {
            UINT uiAction = (UINT)wParam;
            if (uiAction == SPI_SETSCREENSAVETIMEOUT
             || uiAction == SPI_SETSCREENSAVEACTIVE)
            {
                SetEvent(Session->hScreenSaverParametersChanged);
            }
            return TRUE;
        }
        case WM_LOGONNOTIFY:
        {
            switch(wParam)
            {
                case LN_MESSAGE_BEEP:
                {
                    return HandleMessageBeep(lParam);
                }
                case LN_SHELL_EXITED:
                {
                    /* lParam is the exit code */
                    if (lParam != 1 &&
                        Session->LogonState != STATE_LOGGED_OFF &&
                        Session->LogonState != STATE_LOGGED_OFF_SAS)
                    {
                        SetTimer(hwndDlg, 1, 1000, NULL);
                    }
                    break;
                }
                case LN_START_SCREENSAVE:
                {
                    DispatchSAS(Session, WLX_SAS_TYPE_SCRNSVR_TIMEOUT);
                    break;
                }
                case LN_LOCK_WORKSTATION:
                {
                    DoGenericAction(Session, WLX_SAS_ACTION_LOCK_WKSTA);
                    break;
                }
                case LN_LOGOFF:
                {
                    UINT Flags = (UINT)lParam;
                    UINT Action = Flags & EWX_ACTION_MASK;
                    DWORD wlxAction;

                    TRACE("\tFlags : 0x%lx\n", lParam);

                    /*
                     * Our caller (USERSRV) should have added the shutdown flag
                     * when setting also poweroff or reboot.
                     */
                    if (Action & (EWX_POWEROFF | EWX_REBOOT))
                    {
                        if ((Action & EWX_SHUTDOWN) == 0)
                        {
                            ERR("Missing EWX_SHUTDOWN flag for poweroff or reboot; action 0x%x\n", Action);
                            return STATUS_INVALID_PARAMETER;
                        }

                        /* Now we can locally remove it for performing checks */
                        Action &= ~EWX_SHUTDOWN;
                    }

                    /* Check parameters */
                    if (Action & EWX_FORCE)
                    {
                        // FIXME!
                        ERR("FIXME: EWX_FORCE present for Winlogon, what to do?\n");
                        Action &= ~EWX_FORCE;
                    }
                    switch (Action)
                    {
                        case EWX_LOGOFF:
                            wlxAction = WLX_SAS_ACTION_LOGOFF;
                            break;
                        case EWX_SHUTDOWN:
                            wlxAction = WLX_SAS_ACTION_SHUTDOWN;
                            break;
                        case EWX_REBOOT:
                            wlxAction = WLX_SAS_ACTION_SHUTDOWN_REBOOT;
                            break;
                        case EWX_POWEROFF:
                            wlxAction = WLX_SAS_ACTION_SHUTDOWN_POWER_OFF;
                            break;

                        default:
                        {
                            ERR("Invalid ExitWindows action 0x%x\n", Action);
                            return STATUS_INVALID_PARAMETER;
                        }
                    }

                    TRACE("In LN_LOGOFF, exit_in_progress == %s\n",
                        ExitReactOSInProgress ? "true" : "false");

                    /*
                     * In case a parallel shutdown request is done (while we are
                     * being to shut down) and it was not done by Winlogon itself,
                     * then just stop here.
                     */
#if 0
// This code is commented at the moment (even if it's correct) because
// our log-offs do not really work: the shell is restarted, no app is killed
// etc... and as a result you just get explorer opening "My Documents". And
// if you try now a shut down, it won't work because winlogon thinks it is
// still in the middle of a shutdown.
// Maybe we also need to reset ExitReactOSInProgress somewhere else??
                    if (ExitReactOSInProgress && (lParam & EWX_CALLER_WINLOGON) == 0)
                    {
                        break;
                    }
#endif
                    /* Now do the shutdown action proper */
                    DoGenericAction(Session, wlxAction);
                    return 1;
                }
                case LN_LOGOFF_CANCELED:
                {
                    ERR("Logoff canceled!!, before: exit_in_progress == %s, after will be false\n",
                        ExitReactOSInProgress ? "true" : "false");

                    ExitReactOSInProgress = FALSE;
                    return 1;
                }
                default:
                {
                    ERR("WM_LOGONNOTIFY case %d is unimplemented\n", wParam);
                }
            }
            return 0;
        }
        case WM_TIMER:
        {
            if (wParam == 1)
            {
                KillTimer(hwndDlg, 1);
                StartUserShell(Session);
            }
            break;
        }
        case WLX_WM_SAS:
        {
            DispatchSAS(Session, (DWORD)wParam);
            return TRUE;
        }
    }

    return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
}

BOOL
InitializeSAS(
    IN OUT PWLSESSION Session)
{
    WNDCLASSEXW swc;
    BOOL ret = FALSE;

    if (!SwitchDesktop(Session->WinlogonDesktop))
    {
        ERR("WL: Failed to switch to winlogon desktop\n");
        goto cleanup;
    }

    /* Register SAS window class */
    swc.cbSize = sizeof(WNDCLASSEXW);
    swc.style = CS_SAVEBITS;
    swc.lpfnWndProc = SASWindowProc;
    swc.cbClsExtra = 0;
    swc.cbWndExtra = 0;
    swc.hInstance = hAppInstance;
    swc.hIcon = NULL;
    swc.hCursor = NULL;
    swc.hbrBackground = NULL;
    swc.lpszMenuName = NULL;
    swc.lpszClassName = WINLOGON_SAS_CLASS;
    swc.hIconSm = NULL;
    if (RegisterClassExW(&swc) == 0)
    {
        ERR("WL: Failed to register SAS window class\n");
        goto cleanup;
    }

    /* Create invisible SAS window */
    Session->SASWindow = CreateWindowExW(
        0,
        WINLOGON_SAS_CLASS,
        WINLOGON_SAS_TITLE,
        WS_POPUP,
        0, 0, 0, 0, 0, 0,
        hAppInstance, Session);
    if (!Session->SASWindow)
    {
        ERR("WL: Failed to create SAS window\n");
        goto cleanup;
    }

    /* Register SAS window to receive SAS notifications */
    if (!SetLogonNotifyWindow(Session->SASWindow))
    {
        ERR("WL: Failed to register SAS window\n");
        goto cleanup;
    }

    if (!SetDefaultLanguage(NULL))
        return FALSE;

    ret = TRUE;

cleanup:
    if (!ret)
        UninitializeSAS(Session);
    return ret;
}
