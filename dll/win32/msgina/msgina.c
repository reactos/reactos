/*
 *  ReactOS GINA
 *  Copyright (C) 2003-2004, 2006 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * PROJECT:         ReactOS msgina.dll
 * FILE:            dll/win32/msgina/msgina.c
 * PURPOSE:         ReactOS Logon GINA DLL
 * PROGRAMMER:      Thomas Weidenmueller (w3seek@users.sourceforge.net)
 *                  Hervé Poussineau (hpoussin@reactos.org)
 */

#include "msgina.h"

#include <winsvc.h>
#include <userenv.h>
#include <ndk/sefuncs.h>

HINSTANCE hDllInstance;

extern GINA_UI GinaGraphicalUI;
extern GINA_UI GinaTextUI;
static PGINA_UI pGinaUI;
static SID_IDENTIFIER_AUTHORITY SystemAuthority = {SECURITY_NT_AUTHORITY};
static PSID AdminSid;

/*
 * @implemented
 */
BOOL WINAPI
WlxNegotiate(
    IN DWORD dwWinlogonVersion,
    OUT PDWORD pdwDllVersion)
{
    TRACE("WlxNegotiate(%lx, %p)\n", dwWinlogonVersion, pdwDllVersion);

    if(!pdwDllVersion || (dwWinlogonVersion < WLX_VERSION_1_3))
        return FALSE;

    *pdwDllVersion = WLX_VERSION_1_3;

    return TRUE;
}

static VOID
ChooseGinaUI(VOID)
{
    HKEY ControlKey = NULL;
    LPWSTR SystemStartOptions = NULL;
    LPWSTR CurrentOption, NextOption; /* Pointers into SystemStartOptions */
    BOOL ConsoleBoot = FALSE;
    LONG rc;

    rc = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Control",
        0,
        KEY_QUERY_VALUE,
        &ControlKey);

    rc = ReadRegSzValue(ControlKey, L"SystemStartOptions", &SystemStartOptions);
    if (rc != ERROR_SUCCESS)
        goto cleanup;

    /* Check for CONSOLE switch in SystemStartOptions */
    CurrentOption = SystemStartOptions;
    while (CurrentOption)
    {
        NextOption = wcschr(CurrentOption, L' ');
        if (NextOption)
            *NextOption = L'\0';
        if (_wcsicmp(CurrentOption, L"CONSOLE") == 0)
        {
            TRACE("Found %S. Switching to console boot\n", CurrentOption);
            ConsoleBoot = TRUE;
            goto cleanup;
        }
        CurrentOption = NextOption ? NextOption + 1 : NULL;
    }

cleanup:
    if (ConsoleBoot)
        pGinaUI = &GinaTextUI;
    else
        pGinaUI = &GinaGraphicalUI;

    if (ControlKey != NULL)
        RegCloseKey(ControlKey);
    HeapFree(GetProcessHeap(), 0, SystemStartOptions);
}

static BOOL
SafeGetUnicodeString(
    _In_ const LSA_UNICODE_STRING *pInput,
    _Out_ PWSTR pszOutput,
    _In_ SIZE_T cchMax)
{
    HRESULT hr;
    hr = StringCbCopyNExW(pszOutput, cchMax * sizeof(WCHAR),
                          pInput->Buffer, pInput->Length,
                          NULL, NULL,
                          STRSAFE_NO_TRUNCATION | STRSAFE_NULL_ON_FAILURE);
    return (hr == S_OK);
}

/* Reference: https://learn.microsoft.com/en-us/windows/win32/secauthn/protecting-the-automatic-logon-password */
static BOOL
GetLsaDefaultPassword(_Inout_ PGINA_CONTEXT pgContext)
{
    LSA_HANDLE hPolicy;
    LSA_UNICODE_STRING Name, *pPwd;
    LSA_OBJECT_ATTRIBUTES ObjectAttributes = { sizeof(ObjectAttributes) };

    NTSTATUS Status = LsaOpenPolicy(NULL, &ObjectAttributes, 
                                    POLICY_GET_PRIVATE_INFORMATION, &hPolicy);
    if (!NT_SUCCESS(Status))
        return FALSE;

    RtlInitUnicodeString(&Name, L"DefaultPassword");
    Status = LsaRetrievePrivateData(hPolicy, &Name, &pPwd);
    LsaClose(hPolicy);

    if (Status == STATUS_SUCCESS)
    {
        if (!SafeGetUnicodeString(pPwd, pgContext->Password,
                                  _countof(pgContext->Password)))
        {
            Status = STATUS_BUFFER_TOO_SMALL;
        }
        SecureZeroMemory(pPwd->Buffer, pPwd->Length);
        LsaFreeMemory(pPwd);
    }

    return Status == STATUS_SUCCESS;
}

static
BOOL
GetRegistrySettings(PGINA_CONTEXT pgContext)
{
    HKEY hKey = NULL;
    DWORD dwValue, dwSize;
    DWORD dwDisableCAD = 0;
    LONG rc;

    rc = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                       L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                       0,
                       KEY_QUERY_VALUE,
                       &hKey);
    if (rc != ERROR_SUCCESS)
    {
        WARN("RegOpenKeyExW() failed with error %lu\n", rc);
        return FALSE;
    }

    rc = ReadRegDwordValue(hKey, L"AutoAdminLogon", &dwValue);
    if (rc == ERROR_SUCCESS)
        pgContext->bAutoAdminLogon = !!dwValue;
    TRACE("bAutoAdminLogon: %s\n", pgContext->bAutoAdminLogon ? "TRUE" : "FALSE");

    // TODO: What to do also depends whether we are on Terminal Services.
    rc = ReadRegDwordValue(hKey, L"DisableCAD", &dwDisableCAD);
    if (rc == ERROR_SUCCESS)
    {
        if (dwDisableCAD != 0)
            pgContext->bDisableCAD = TRUE;
    }
    TRACE("bDisableCAD: %s\n", pgContext->bDisableCAD ? "TRUE" : "FALSE");

    // NOTE: The default value is always read from the registry (Workstation: TRUE; Server: FALSE).
    // TODO: Set it to TRUE always on SafeMode. Keep it FALSE for remote sessions.
    pgContext->bShutdownWithoutLogon = TRUE;
    rc = ReadRegDwordValue(hKey, L"ShutdownWithoutLogon", &dwValue);
    if (rc == ERROR_SUCCESS)
        pgContext->bShutdownWithoutLogon = !!dwValue;

    rc = ReadRegDwordValue(hKey, L"DontDisplayLastUserName", &dwValue);
    if (rc == ERROR_SUCCESS)
        pgContext->bDontDisplayLastUserName = !!dwValue;

    rc = ReadRegDwordValue(hKey, L"IgnoreShiftOverride", &dwValue);
    if (rc == ERROR_SUCCESS)
        pgContext->bIgnoreShiftOverride = !!dwValue;

    dwSize = sizeof(pgContext->UserName);
    rc = RegQueryValueExW(hKey,
                          L"DefaultUserName",
                          NULL,
                          NULL,
                          (PBYTE)&pgContext->UserName,
                          &dwSize);

    dwSize = sizeof(pgContext->DomainName);
    rc = RegQueryValueExW(hKey,
                          L"DefaultDomainName",
                          NULL,
                          NULL,
                          (PBYTE)&pgContext->DomainName,
                          &dwSize);

    dwSize = sizeof(pgContext->Password);
    rc = RegQueryValueExW(hKey,
                          L"DefaultPassword",
                          NULL,
                          NULL,
                          (PBYTE)&pgContext->Password,
                          &dwSize);
    if (rc)
        GetLsaDefaultPassword(pgContext);

    if (hKey != NULL)
        RegCloseKey(hKey);

    return TRUE;
}

typedef DWORD (WINAPI *pThemeWait)(DWORD dwTimeout);
typedef BOOL (WINAPI *pThemeWatch)(void);

static void
InitThemeSupport(VOID)
{
    HMODULE hDll = LoadLibraryW(L"shsvcs.dll");
    pThemeWait themeWait;
    pThemeWatch themeWatch;

    if(!hDll)
        return;

    themeWait = (pThemeWait) GetProcAddress(hDll, (LPCSTR)2);
    themeWatch = (pThemeWatch) GetProcAddress(hDll, (LPCSTR)1);

    if(themeWait && themeWatch)
    {
        themeWait(5000);
        themeWatch();
    }
}

/*
 * @implemented
 */
BOOL WINAPI
WlxInitialize(
    LPWSTR lpWinsta,
    HANDLE hWlx,
    PVOID  pvReserved,
    PVOID  pWinlogonFunctions,
    PVOID  *pWlxContext)
{
    PGINA_CONTEXT pgContext;

    UNREFERENCED_PARAMETER(pvReserved);

    InitThemeSupport();

    pgContext = (PGINA_CONTEXT)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(GINA_CONTEXT));
    if(!pgContext)
    {
        WARN("LocalAlloc() failed\n");
        return FALSE;
    }

    if (!GetRegistrySettings(pgContext))
    {
        WARN("GetRegistrySettings() failed\n");
        LocalFree(pgContext);
        return FALSE;
    }

    /* Return the context to winlogon */
    *pWlxContext = (PVOID)pgContext;
    pgContext->hDllInstance = hDllInstance;

    /* Save pointer to dispatch table */
    pgContext->pWlxFuncs = (PWLX_DISPATCH_VERSION_1_3)pWinlogonFunctions;

    /* Save the winlogon handle used to call the dispatch functions */
    pgContext->hWlx = hWlx;

    /* Save window station */
    pgContext->station = lpWinsta;

    /* Clear status window handle */
    pgContext->hStatusWindow = NULL;

    /* Notify winlogon that we will use the default SAS */
    pgContext->pWlxFuncs->WlxUseCtrlAltDel(hWlx);

    /* Locates the authentication package */
    //LsaRegisterLogonProcess(...);

    pgContext->nShutdownAction = WLX_SAS_ACTION_SHUTDOWN_POWER_OFF;

    ChooseGinaUI();
    return pGinaUI->Initialize(pgContext);
}

/*
 * @implemented
 */
BOOL
WINAPI
WlxScreenSaverNotify(
    PVOID pWlxContext,
    BOOL  *pSecure)
{
#if 0
    PGINA_CONTEXT pgContext = (PGINA_CONTEXT)pWlxContext;
    WCHAR szBuffer[2];
    HKEY hKeyCurrentUser, hKey;
    DWORD bufferSize = sizeof(szBuffer);
    DWORD varType = REG_SZ;
    LONG rc;

    TRACE("(%p %p)\n", pWlxContext, pSecure);

    *pSecure = TRUE;

    /*
     * Policy setting:
     *    HKLM\Software\Policies\Microsoft\Windows\Control Panel\Desktop : ScreenSaverIsSecure
     * User setting:
     *    HKCU\Control Panel\Desktop : ScreenSaverIsSecure
     */

    if (!ImpersonateLoggedOnUser(pgContext->UserToken))
    {
        ERR("WL: ImpersonateLoggedOnUser() failed with error %lu\n", GetLastError());
        *pSecure = FALSE;
        return TRUE;
    }

    /* Open the current user HKCU key */
    rc = RegOpenCurrentUser(MAXIMUM_ALLOWED, &hKeyCurrentUser);
    TRACE("RegOpenCurrentUser: %ld\n", rc);
    if (rc == ERROR_SUCCESS)
    {
        /* Open the subkey */
        rc = RegOpenKeyExW(hKeyCurrentUser,
                           L"Control Panel\\Desktop",
                           0,
                           KEY_QUERY_VALUE,
                           &hKey);
        TRACE("RegOpenKeyExW: %ld\n", rc);
        RegCloseKey(hKeyCurrentUser);
    }

    /* Read the value */
    if (rc == ERROR_SUCCESS)
    {
        rc = RegQueryValueExW(hKey,
                              L"ScreenSaverIsSecure",
                              NULL,
                              &varType,
                              (LPBYTE)szBuffer,
                              &bufferSize);

        TRACE("RegQueryValueExW: %ld\n", rc);

        if (rc == ERROR_SUCCESS)
        {
            TRACE("szBuffer: \"%S\"\n", szBuffer);
            *pSecure = _wtoi(szBuffer);
        }

        RegCloseKey(hKey);
    }

    /* Revert the impersonation */
    RevertToSelf();

    TRACE("*pSecure: %ld\n", *pSecure);
#endif

    *pSecure = FALSE;

    return TRUE;
}

/*
 * @implemented
 */
BOOL WINAPI
WlxStartApplication(
    PVOID pWlxContext,
    PWSTR pszDesktopName,
    PVOID pEnvironment,
    PWSTR pszCmdLine)
{
    PGINA_CONTEXT pgContext = (PGINA_CONTEXT)pWlxContext;
    STARTUPINFOW StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    WCHAR CurrentDirectory[MAX_PATH];
    HANDLE hAppToken;
    UINT len;
    BOOL ret;

    len = GetWindowsDirectoryW(CurrentDirectory, MAX_PATH);
    if (len == 0 || len > MAX_PATH)
    {
        ERR("GetWindowsDirectoryW() failed\n");
        return FALSE;
    }

    ret = DuplicateTokenEx(pgContext->UserToken, MAXIMUM_ALLOWED, NULL, SecurityImpersonation, TokenPrimary, &hAppToken);
    if (!ret)
    {
        ERR("DuplicateTokenEx() failed with error %lu\n", GetLastError());
        return FALSE;
    }

    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
    ZeroMemory(&ProcessInformation, sizeof(ProcessInformation));
    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.lpTitle = pszCmdLine;
    StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    StartupInfo.wShowWindow = SW_SHOW;
    StartupInfo.lpDesktop = pszDesktopName;

    len = GetWindowsDirectoryW(CurrentDirectory, MAX_PATH);
    if (len == 0 || len > MAX_PATH)
    {
        ERR("GetWindowsDirectoryW() failed\n");
        return FALSE;
    }
    ret = CreateProcessAsUserW(
        hAppToken,
        pszCmdLine,
        NULL,
        NULL,
        NULL,
        FALSE,
        CREATE_UNICODE_ENVIRONMENT,
        pEnvironment,
        CurrentDirectory,
        &StartupInfo,
        &ProcessInformation);
    CloseHandle(ProcessInformation.hProcess);
    CloseHandle(ProcessInformation.hThread);
    CloseHandle(hAppToken);
    if (!ret)
        ERR("CreateProcessAsUserW() failed with error %lu\n", GetLastError());
    return ret;
}

/*
 * @implemented
 */
BOOL
WINAPI
WlxActivateUserShell(
    _In_ PVOID pWlxContext,
    _In_ PWSTR pszDesktopName,
    _In_ PWSTR pszMprLogonScript,
    _In_ PVOID pEnvironment)
{
    PGINA_CONTEXT pgContext = (PGINA_CONTEXT)pWlxContext;
    HKEY hKey, hKeyCurrentUser;
    DWORD BufSize, ValueType;
    DWORD len;
    LONG rc;
    BOOL ret;
    WCHAR pszUserInitApp[MAX_PATH + 1];
    WCHAR pszExpUserInitApp[MAX_PATH];

    TRACE("WlxActivateUserShell()\n");

    UNREFERENCED_PARAMETER(pszMprLogonScript);

    /* Get the path of Userinit */
    rc = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                       L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                       0,
                       KEY_QUERY_VALUE,
                       &hKey);
    if (rc != ERROR_SUCCESS)
    {
        WARN("RegOpenKeyExW() failed with error %lu\n", rc);
        return FALSE;
    }

    BufSize = sizeof(pszUserInitApp) - sizeof(UNICODE_NULL);
    rc = RegQueryValueExW(hKey,
                          L"Userinit",
                          NULL,
                          &ValueType,
                          (PBYTE)pszUserInitApp,
                          &BufSize);
    RegCloseKey(hKey);
    if (rc != ERROR_SUCCESS || (ValueType != REG_SZ && ValueType != REG_EXPAND_SZ))
    {
        WARN("RegQueryValueExW() failed with error %lu\n", rc);
        return FALSE;
    }
    pszUserInitApp[MAX_PATH] = UNICODE_NULL;

    len = ExpandEnvironmentStringsW(pszUserInitApp, pszExpUserInitApp, _countof(pszExpUserInitApp));
    if (len > _countof(pszExpUserInitApp))
    {
        WARN("ExpandEnvironmentStringsW() failed. Required size %lu\n", len);
        return FALSE;
    }

    /* Start the Userinit application */
    ret = WlxStartApplication(pWlxContext, pszDesktopName, pEnvironment, pszExpUserInitApp);
    if (!ret)
        return ret;

    /* For convenience, store in the logged-in user's Explorer key, the user name
     * that was entered verbatim in the "Log On" dialog to log into the system.
     * This name may differ from the resulting user name used during authentication. */

    /* Open the per-user registry key */
    rc = RegOpenLoggedOnHKCU(pgContext->UserToken,
                             KEY_SET_VALUE,
                             &hKeyCurrentUser);
    if (rc != ERROR_SUCCESS)
    {
        ERR("RegOpenLoggedOnHKCU() failed with error %ld\n", rc);
        return ret;
    }

    /* Open the subkey and write the value */
    rc = RegOpenKeyExW(hKeyCurrentUser,
                       L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer",
                       0,
                       KEY_SET_VALUE,
                       &hKey);
    if (rc == ERROR_SUCCESS)
    {
        len = wcslen(pgContext->UserName) + 1;
        RegSetValueExW(hKey,
                       L"Logon User Name",
                       0,
                       REG_SZ,
                       (PBYTE)pgContext->UserName,
                       len * sizeof(WCHAR));
        RegCloseKey(hKey);
    }
    RegCloseKey(hKeyCurrentUser);

    return ret;
}

/*
 * @implemented
 */
int WINAPI
WlxLoggedOnSAS(
    PVOID pWlxContext,
    DWORD dwSasType,
    PVOID pReserved)
{
    PGINA_CONTEXT pgContext = (PGINA_CONTEXT)pWlxContext;
    INT SasAction = WLX_SAS_ACTION_NONE;

    TRACE("WlxLoggedOnSAS(0x%lx)\n", dwSasType);

    UNREFERENCED_PARAMETER(pReserved);

    switch (dwSasType)
    {
        case WLX_SAS_TYPE_CTRL_ALT_DEL:
        case WLX_SAS_TYPE_TIMEOUT:
        {
            SasAction = pGinaUI->LoggedOnSAS(pgContext, dwSasType);
            break;
        }
        case WLX_SAS_TYPE_SC_INSERT:
        {
            FIXME("WlxLoggedOnSAS: SasType WLX_SAS_TYPE_SC_INSERT not supported!\n");
            break;
        }
        case WLX_SAS_TYPE_SC_REMOVE:
        {
            FIXME("WlxLoggedOnSAS: SasType WLX_SAS_TYPE_SC_REMOVE not supported!\n");
            break;
        }
        default:
        {
            WARN("WlxLoggedOnSAS: Unknown SasType: 0x%x\n", dwSasType);
            break;
        }
    }

    return SasAction;
}

/*
 * @implemented
 */
BOOL WINAPI
WlxDisplayStatusMessage(
    IN PVOID pWlxContext,
    IN HDESK hDesktop,
    IN DWORD dwOptions,
    IN PWSTR pTitle,
    IN PWSTR pMessage)
{
    PGINA_CONTEXT pgContext = (PGINA_CONTEXT)pWlxContext;

    TRACE("WlxDisplayStatusMessage(\"%S\")\n", pMessage);

    return pGinaUI->DisplayStatusMessage(pgContext, hDesktop, dwOptions, pTitle, pMessage);
}

/*
 * @implemented
 */
BOOL WINAPI
WlxRemoveStatusMessage(
    IN PVOID pWlxContext)
{
    PGINA_CONTEXT pgContext = (PGINA_CONTEXT)pWlxContext;

    TRACE("WlxRemoveStatusMessage()\n");

    return pGinaUI->RemoveStatusMessage(pgContext);
}


BOOL
DoAdminUnlock(
    IN PGINA_CONTEXT pgContext,
    IN PWSTR UserName,
    IN PWSTR Domain,
    IN PWSTR Password)
{
    HANDLE hToken = NULL;
    PTOKEN_GROUPS Groups = NULL;
    BOOL bIsAdmin = FALSE;
    ULONG Size;
    ULONG i;
    NTSTATUS Status;
    NTSTATUS SubStatus = STATUS_SUCCESS;

    TRACE("(%S %S %S)\n", UserName, Domain, Password);

    Status = ConnectToLsa(pgContext);
    if (!NT_SUCCESS(Status))
    {
        WARN("ConnectToLsa() failed\n");
        return FALSE;
    }

    Status = MyLogonUser(pgContext->LsaHandle,
                         pgContext->AuthenticationPackage,
                         UserName,
                         Domain,
                         Password,
                         &pgContext->UserToken,
                         &SubStatus);
    if (!NT_SUCCESS(Status))
    {
        WARN("MyLogonUser() failed\n");
        return FALSE;
    }

    Status = NtQueryInformationToken(hToken,
                                     TokenGroups,
                                     NULL,
                                     0,
                                     &Size);
    if ((Status != STATUS_SUCCESS) && (Status != STATUS_BUFFER_TOO_SMALL))
    {
        TRACE("NtQueryInformationToken() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    Groups = HeapAlloc(GetProcessHeap(), 0, Size);
    if (Groups == NULL)
    {
        TRACE("HeapAlloc() failed\n");
        goto done;
    }

    Status = NtQueryInformationToken(hToken,
                                     TokenGroups,
                                     Groups,
                                     Size,
                                     &Size);
    if (!NT_SUCCESS(Status))
    {
        TRACE("NtQueryInformationToken() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    for (i = 0; i < Groups->GroupCount; i++)
    {
        if (RtlEqualSid(Groups->Groups[i].Sid, AdminSid))
        {
            TRACE("Member of Admins group\n");
            bIsAdmin = TRUE;
            break;
        }
    }

done:
    if (Groups != NULL)
        HeapFree(GetProcessHeap(), 0, Groups);

    if (hToken != NULL)
        CloseHandle(hToken);

    return bIsAdmin;
}


NTSTATUS
DoLoginTasks(
    IN OUT PGINA_CONTEXT pgContext,
    IN PWSTR UserName,
    IN PWSTR Domain,
    IN PWSTR Password,
    OUT PNTSTATUS SubStatus)
{
    NTSTATUS Status;

    Status = ConnectToLsa(pgContext);
    if (!NT_SUCCESS(Status))
    {
        WARN("ConnectToLsa() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    Status = MyLogonUser(pgContext->LsaHandle,
                         pgContext->AuthenticationPackage,
                         UserName,
                         Domain,
                         Password,
                         &pgContext->UserToken,
                         SubStatus);
    if (!NT_SUCCESS(Status))
    {
        WARN("MyLogonUser() failed (Status 0x%08lx)\n", Status);
    }

    return Status;
}


BOOL
CreateProfile(
    IN OUT PGINA_CONTEXT pgContext,
    IN PWSTR UserName,
    IN PWSTR Domain,
    IN PWSTR Password)
{
    PWLX_PROFILE_V2_0 pProfile = NULL;
    PWSTR pEnvironment = NULL;
    TOKEN_STATISTICS Stats;
    DWORD cbStats, cbSize;
    DWORD dwLength;
#if 0
    BOOL bIsDomainLogon;
    WCHAR ComputerName[MAX_COMPUTERNAME_LENGTH+1];
#endif

    /* Store the logon time in the context */
    GetLocalTime(&pgContext->LogonTime);

    /* Store user and domain in the context */
    wcscpy(pgContext->UserName, UserName);
    if (Domain == NULL || !Domain[0])
    {
        dwLength = _countof(pgContext->DomainName);
        GetComputerNameW(pgContext->DomainName, &dwLength);
    }
    else
    {
        wcscpy(pgContext->DomainName, Domain);
    }
    /* From now on we use in UserName and Domain the captured values from pgContext */
    UserName = pgContext->UserName;
    Domain = pgContext->DomainName;

#if 0
    /* Determine whether this is really a domain logon, by verifying
     * that the specified domain is different from the local computer */
    dwLength = _countof(ComputerName);
    GetComputerNameW(ComputerName, &dwLength);
    bIsDomainLogon = (_wcsicmp(ComputerName, Domain) != 0);
#endif

    /* Allocate memory for profile */
    pProfile = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(*pProfile));
    if (!pProfile)
    {
        WARN("HeapAlloc() failed\n");
        goto cleanup;
    }
    pProfile->dwType = WLX_PROFILE_TYPE_V2_0;

    /*
     * TODO: For domain logon support:
     *
     * - pszProfile: Specifies the path to a *roaming* user profile on a
     *   domain server, if any. It is then used to create a local image
     *   (copy) of the profile on the local computer.
     *   ** This data should be retrieved from the LsaLogonUser() call
     *      made by MyLogonUser()! **
     *
     * - pszPolicy (for domain logon): Path to a policy file.
     *   Windows' msgina.dll hardcodes it as:
     *   "\\<domain_controller>\netlogon\ntconfig.pol"
     *
     * - pszNetworkDefaultUserProfile (for domain logon): Path to the
     *   default user profile. Windows' msgina.dll hardcodes it as:
     *   "\\<domain_controller>\netlogon\Default User"
     *
     * - pszServerName (for domain logon): Name ("domain_controller") of
     *   the server (local computer; Active Directory domain controller...)
     *   that validated the logon.
     *   ** This data should be retrieved from the LsaLogonUser() call
     *      made by MyLogonUser()! **
     *
     * NOTES:
     * - The paths use the domain controllers' "netlogon" share.
     * - These strings are LocalAlloc'd here, and LocalFree'd by Winlogon.
     */
    pProfile->pszProfile = NULL;
    pProfile->pszPolicy = NULL;
    pProfile->pszNetworkDefaultUserProfile = NULL;
    pProfile->pszServerName = NULL;
#if 0
    if (bIsDomainLogon)
    {
        PWSTR pServerName;
        cbSize = sizeof(L"\\\\") + wcslen(Domain) * sizeof(WCHAR);
        pServerName = LocalAlloc(LMEM_FIXED, cbSize);
        if (!pServerName)
            WARN("HeapAlloc() failed\n"); // Consider this optional, so no failure.
        else
            StringCbPrintfW(pServerName, cbSize, L"\\\\%ws", Domain); // See LogonServer below.
        pProfile->pszServerName = pServerName;
    }
#endif

    /* Build the minimal environment string block */
    // FIXME: LogonServer is the name of the server that processed the logon
    // request ("domain_controller"). It can be different from the selected
    // user's logon domain.
    // See e.g.:
    // - https://learn.microsoft.com/en-us/windows/win32/api/ntsecapi/ns-ntsecapi-msv1_0_interactive_profile
    // - https://learn.microsoft.com/en-us/windows/win32/api/winwlx/ns-winwlx-wlx_consoleswitch_credentials_info_v1_0
    cbSize = sizeof(L"LOGONSERVER=\\\\") +
             wcslen(Domain) * sizeof(WCHAR) +
             sizeof(UNICODE_NULL);
    pEnvironment = LocalAlloc(LMEM_FIXED, cbSize);
    if (!pEnvironment)
    {
        WARN("LocalAlloc() failed\n");
        goto cleanup;
    }

    StringCbPrintfW(pEnvironment, cbSize, L"LOGONSERVER=\\\\%ws", Domain);
    ASSERT(wcslen(pEnvironment) == cbSize / sizeof(WCHAR) - 2);
    pEnvironment[cbSize / sizeof(WCHAR) - 1] = UNICODE_NULL;

    pProfile->pszEnvironment = pEnvironment;

    /* Return the other info */
    if (!GetTokenInformation(pgContext->UserToken,
                             TokenStatistics,
                             &Stats,
                             sizeof(Stats),
                             &cbStats))
    {
        WARN("Couldn't get Authentication Id from user token!\n");
        goto cleanup;
    }

    *pgContext->pAuthenticationId = Stats.AuthenticationId;
    pgContext->pMprNotifyInfo->pszUserName = DuplicateString(UserName);
    pgContext->pMprNotifyInfo->pszDomain = DuplicateString(Domain);
    pgContext->pMprNotifyInfo->pszPassword = DuplicateString(Password);
    pgContext->pMprNotifyInfo->pszOldPassword = NULL;
    *pgContext->pdwOptions = 0;
    *pgContext->pProfile = pProfile;
    return TRUE;

cleanup:
    if (pEnvironment)
        LocalFree(pEnvironment);
    if (pProfile)
        LocalFree(pProfile);
    return FALSE;
}


/*
 * @implemented
 */
VOID WINAPI
WlxDisplaySASNotice(
    IN PVOID pWlxContext)
{
    PGINA_CONTEXT pgContext = (PGINA_CONTEXT)pWlxContext;

    TRACE("WlxDisplaySASNotice(%p)\n", pWlxContext);

    if (GetSystemMetrics(SM_REMOTESESSION))
    {
        /* User is remotely logged on. Don't display a notice */
        pgContext->pWlxFuncs->WlxSasNotify(pgContext->hWlx, WLX_SAS_TYPE_CTRL_ALT_DEL);
        return;
    }

    if (pgContext->bAutoAdminLogon)
    {
        if (pgContext->bIgnoreShiftOverride ||
            (GetKeyState(VK_SHIFT) >= 0))
        {
            /* Don't display the window, we want to do an automatic logon */
            pgContext->pWlxFuncs->WlxSasNotify(pgContext->hWlx, WLX_SAS_TYPE_CTRL_ALT_DEL);
            return;
        }

        pgContext->bAutoAdminLogon = FALSE;
    }

    if (pgContext->bDisableCAD)
    {
        pgContext->pWlxFuncs->WlxSasNotify(pgContext->hWlx, WLX_SAS_TYPE_CTRL_ALT_DEL);
        return;
    }

    pGinaUI->DisplaySASNotice(pgContext);

    TRACE("WlxDisplaySASNotice() done\n");
}

/*
 * @implemented
 */
INT WINAPI
WlxLoggedOutSAS(
    IN PVOID pWlxContext,
    IN DWORD dwSasType,
    OUT PLUID pAuthenticationId,
    IN OUT PSID pLogonSid,
    OUT PDWORD pdwOptions,
    OUT PHANDLE phToken,
    OUT PWLX_MPR_NOTIFY_INFO pMprNotifyInfo,
    OUT PVOID *pProfile)
{
    PGINA_CONTEXT pgContext = (PGINA_CONTEXT)pWlxContext;
    INT res;

    TRACE("WlxLoggedOutSAS()\n");

    UNREFERENCED_PARAMETER(dwSasType);
    UNREFERENCED_PARAMETER(pLogonSid);

    pgContext->pAuthenticationId = pAuthenticationId;
    pgContext->pdwOptions = pdwOptions;
    pgContext->pMprNotifyInfo = pMprNotifyInfo;
    pgContext->pProfile = pProfile;

    res = pGinaUI->LoggedOutSAS(pgContext);

    /* Return the logon information only if necessary */
    if (res == WLX_SAS_ACTION_LOGON)
        *phToken = pgContext->UserToken;

    return res;
}

/*
 * @implemented
 */
int WINAPI
WlxWkstaLockedSAS(
    PVOID pWlxContext,
    DWORD dwSasType)
{
    PGINA_CONTEXT pgContext = (PGINA_CONTEXT)pWlxContext;

    TRACE("WlxWkstaLockedSAS()\n");

    UNREFERENCED_PARAMETER(dwSasType);

    return pGinaUI->LockedSAS(pgContext);
}


/*
 * @implemented
 */
VOID
WINAPI
WlxDisplayLockedNotice(PVOID pWlxContext)
{
    PGINA_CONTEXT pgContext = (PGINA_CONTEXT)pWlxContext;

    TRACE("WlxDisplayLockedNotice()\n");

    if (pgContext->bDisableCAD)
    {
        pgContext->pWlxFuncs->WlxSasNotify(pgContext->hWlx, WLX_SAS_TYPE_CTRL_ALT_DEL);
        return;
    }

    pGinaUI->DisplayLockedNotice(pgContext);
}


/*
 * @implemented
 */
BOOL WINAPI
WlxIsLogoffOk(
    PVOID pWlxContext)
{
    TRACE("WlxIsLogoffOk()\n");
    UNREFERENCED_PARAMETER(pWlxContext);
    return TRUE;
}


/*
 * @implemented
 */
VOID WINAPI
WlxLogoff(
    PVOID pWlxContext)
{
    PGINA_CONTEXT pgContext = (PGINA_CONTEXT)pWlxContext;

    TRACE("WlxLogoff(%p)\n", pWlxContext);

    /* Reset the captured Winlogon pointers */
    pgContext->pAuthenticationId = NULL;
    pgContext->pdwOptions = NULL;
    pgContext->pMprNotifyInfo = NULL;
    pgContext->pProfile = NULL;

    /*
     * Reset user login information.
     * Keep pgContext->UserName and pgContext->DomainName around
     * if we want to show them as default (last logged user) in
     * the Log-On dialog.
     */
    ZeroMemory(&pgContext->LogonTime, sizeof(pgContext->LogonTime));

    /* Delete the password */
    SecureZeroMemory(pgContext->Password, sizeof(pgContext->Password));

    /* Close the user token */
    CloseHandle(pgContext->UserToken);
    pgContext->UserToken = NULL;
}


/*
 * @implemented
 */
VOID WINAPI
WlxShutdown(
    PVOID pWlxContext,
    DWORD ShutdownType)
{
    PGINA_CONTEXT pgContext = (PGINA_CONTEXT)pWlxContext;
    NTSTATUS Status;

    TRACE("WlxShutdown(%p %lx)\n", pWlxContext, ShutdownType);

    /* Close the LSA handle */
    pgContext->AuthenticationPackage = 0;
    Status = LsaDeregisterLogonProcess(pgContext->LsaHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsaDeregisterLogonProcess failed (Status 0x%08lx)\n", Status);
    }
}


BOOL WINAPI
DllMain(
    IN HINSTANCE hinstDLL,
    IN DWORD dwReason,
    IN LPVOID lpvReserved)
{
    UNREFERENCED_PARAMETER(lpvReserved);

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        hDllInstance = hinstDLL;

        RtlAllocateAndInitializeSid(&SystemAuthority,
                                    2,
                                    SECURITY_BUILTIN_DOMAIN_RID,
                                    DOMAIN_ALIAS_RID_ADMINS,
                                    SECURITY_NULL_RID,
                                    SECURITY_NULL_RID,
                                    SECURITY_NULL_RID,
                                    SECURITY_NULL_RID,
                                    SECURITY_NULL_RID,
                                    SECURITY_NULL_RID,
                                    &AdminSid);

    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        if (AdminSid != NULL)
            RtlFreeSid(AdminSid);
    }

    return TRUE;
}
