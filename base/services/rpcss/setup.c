/*
 * PROJECT:     ReactOS RPC Subsystem Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     One-time service setup configuration.
 * COPYRIGHT:   Copyright 2018 Hermes Belusca-Maito
 */

/* INCLUDES *****************************************************************/

/* PSDK/NDK Headers */
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winsvc.h>

#include <ndk/rtlfuncs.h>
#include <ntsecapi.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(rpcss);

/* FUNCTIONS ****************************************************************/

static BOOL
SetupIsActive(VOID)
{
    LONG lResult;
    HKEY hKey;
    DWORD dwData = 0;
    DWORD cbData = sizeof(dwData);
    DWORD dwType = REG_NONE;

    lResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\Setup", 0, KEY_QUERY_VALUE, &hKey);
    if (lResult != ERROR_SUCCESS)
        return FALSE;

    lResult = RegQueryValueExW(hKey, L"SystemSetupInProgress", NULL,
                               &dwType, (LPBYTE)&dwData, &cbData);
    RegCloseKey(hKey);

    if ((lResult == ERROR_SUCCESS) && (dwType == REG_DWORD) &&
        (cbData == sizeof(dwData)) && (dwData == 1))
    {
        return TRUE;
    }

    return FALSE;
}

static BOOL
RunningAsSYSTEM(VOID)
{
    /* S-1-5-18 -- Local System */
    static SID SystemSid = { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_LOCAL_SYSTEM_RID } };

    BOOL bRet = FALSE;
    PTOKEN_USER pTokenUser;
    HANDLE hToken;
    DWORD cbTokenBuffer = 0;

    /* Get the process token */
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        return FALSE;

    /* Retrieve token's information */
    if (!GetTokenInformation(hToken, TokenUser, NULL, 0, &cbTokenBuffer) &&
        GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        goto Quit;
    }

    pTokenUser = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbTokenBuffer);
    if (!pTokenUser)
        goto Quit;

    if (GetTokenInformation(hToken, TokenUser, pTokenUser, cbTokenBuffer, &cbTokenBuffer))
    {
        /* Compare with SYSTEM SID */
        bRet = EqualSid(pTokenUser->User.Sid, &SystemSid);
    }

    HeapFree(GetProcessHeap(), 0, pTokenUser);

Quit:
    CloseHandle(hToken);
    return bRet;
}

static VOID
RpcSsConfigureAsNetworkService(VOID)
{
    SC_HANDLE hSCManager, hService;

    /* Open the service controller */
    hSCManager = OpenSCManagerW(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
    if (!hSCManager)
    {
        ERR("OpenSCManager() failed with error 0x%lx\n", GetLastError());
        return;
    }

    /* Open the RPCSS service */
    hService = OpenServiceW(hSCManager, L"RPCSS", SERVICE_CHANGE_CONFIG);
    if (!hService)
        ERR("OpenService(\"RPCSS\") failed with error 0x%lx\n", GetLastError());
    if (hService)
    {
        /* Use the NetworkService account */
        if (!ChangeServiceConfigW(hService,
                                  SERVICE_NO_CHANGE,
                                  SERVICE_NO_CHANGE,
                                  SERVICE_NO_CHANGE,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  L"NT AUTHORITY\\NetworkService",
                                  L"",
                                  NULL))
        {
            ERR("ChangeServiceConfig(\"RPCSS\") failed with error 0x%lx\n", GetLastError());
        }

        CloseServiceHandle(hService);
    }

    CloseServiceHandle(hSCManager);
}

static VOID
AddImpersonatePrivilege(VOID)
{
    /* S-1-5-6 -- "Service" group */
    static SID ServiceSid = { SID_REVISION, 1, { SECURITY_NT_AUTHORITY }, { SECURITY_SERVICE_RID } };

    NTSTATUS Status;
    LSA_HANDLE PolicyHandle;
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_UNICODE_STRING RightString;

    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
    Status = LsaOpenPolicy(NULL, &ObjectAttributes,
                           POLICY_CREATE_ACCOUNT | POLICY_LOOKUP_NAMES,
                           &PolicyHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsaOpenPolicy() failed with Status 0x%08lx\n", Status);
        return;
    }

    RtlInitUnicodeString(&RightString, L"SeImpersonatePrivilege");
    Status = LsaAddAccountRights(PolicyHandle, &ServiceSid, &RightString, 1);
    if (!NT_SUCCESS(Status))
    {
        ERR("LsaAddAccountRights(\"S-1-5-6\", \"%wZ\") failed with Status 0x%08lx\n", Status, &RightString);
    }

    LsaClose(PolicyHandle);
}

VOID DoRpcSsSetupConfiguration(VOID)
{
    /*
     * On first run during the setup phase, the RPCSS service runs under
     * the LocalSystem account. RPCSS then re-configures itself to run
     * under the NetworkService account and adds the Impersonate privilege
     * to the "Service" group.
     * This is done in this way, because the NetworkService account does not
     * initially exist when the setup phase is running and the RPCSS service
     * is started, but this account is created later during the setup phase.
     */
    if (SetupIsActive() && RunningAsSYSTEM())
    {
        RpcSsConfigureAsNetworkService();
        AddImpersonatePrivilege();
    }
}
