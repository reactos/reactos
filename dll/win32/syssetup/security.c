/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           System setup
 * FILE:              dll/win32/syssetup/security.c
 * PROGRAMER:         Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

#include <ntlsa.h>
#include <ntsecapi.h>
#include <ntsam.h>
#include <sddl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

NTSTATUS
WINAPI
SetAccountsDomainSid(
    PSID DomainSid,
    LPCWSTR DomainName)
{
    PPOLICY_ACCOUNT_DOMAIN_INFO OrigInfo = NULL;
    POLICY_ACCOUNT_DOMAIN_INFO Info;
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE PolicyHandle;

    SAM_HANDLE ServerHandle = NULL;
    SAM_HANDLE DomainHandle = NULL;
    DOMAIN_NAME_INFORMATION DomainNameInfo;

    NTSTATUS Status;

    DPRINT("SYSSETUP: SetAccountsDomainSid\n");

    memset(&ObjectAttributes, 0, sizeof(LSA_OBJECT_ATTRIBUTES));
    ObjectAttributes.Length = sizeof(LSA_OBJECT_ATTRIBUTES);

    Status = LsaOpenPolicy(NULL,
                           &ObjectAttributes,
                           POLICY_VIEW_LOCAL_INFORMATION | POLICY_TRUST_ADMIN,
                           &PolicyHandle);
    if (Status != STATUS_SUCCESS)
    {
        DPRINT("LsaOpenPolicy failed (Status: 0x%08lx)\n", Status);
        return Status;
    }

    Status = LsaQueryInformationPolicy(PolicyHandle,
                                       PolicyAccountDomainInformation,
                                       (PVOID *)&OrigInfo);
    if (Status == STATUS_SUCCESS && OrigInfo != NULL)
    {
        if (DomainName == NULL)
        {
            Info.DomainName.Buffer = OrigInfo->DomainName.Buffer;
            Info.DomainName.Length = OrigInfo->DomainName.Length;
            Info.DomainName.MaximumLength = OrigInfo->DomainName.MaximumLength;
        }
        else
        {
            Info.DomainName.Buffer = (LPWSTR)DomainName;
            Info.DomainName.Length = wcslen(DomainName) * sizeof(WCHAR);
            Info.DomainName.MaximumLength = Info.DomainName.Length + sizeof(WCHAR);
        }

        if (DomainSid == NULL)
            Info.DomainSid = OrigInfo->DomainSid;
        else
            Info.DomainSid = DomainSid;
    }
    else
    {
        Info.DomainName.Buffer = (LPWSTR)DomainName;
        Info.DomainName.Length = wcslen(DomainName) * sizeof(WCHAR);
        Info.DomainName.MaximumLength = Info.DomainName.Length + sizeof(WCHAR);
        Info.DomainSid = DomainSid;
    }

    Status = LsaSetInformationPolicy(PolicyHandle,
                                     PolicyAccountDomainInformation,
                                     (PVOID)&Info);
    if (Status != STATUS_SUCCESS)
    {
        DPRINT("LsaSetInformationPolicy failed (Status: 0x%08lx)\n", Status);
    }

    if (OrigInfo != NULL)
        LsaFreeMemory(OrigInfo);

    LsaClose(PolicyHandle);

    DomainNameInfo.DomainName.Length = wcslen(DomainName) * sizeof(WCHAR);
    DomainNameInfo.DomainName.MaximumLength = (wcslen(DomainName) + 1) * sizeof(WCHAR);
    DomainNameInfo.DomainName.Buffer = (LPWSTR)DomainName;

    Status = SamConnect(NULL,
                        &ServerHandle,
                        SAM_SERVER_CONNECT | SAM_SERVER_LOOKUP_DOMAIN,
                        NULL);
    if (NT_SUCCESS(Status))
    {
        Status = SamOpenDomain(ServerHandle,
                               DOMAIN_WRITE_OTHER_PARAMETERS,
                               Info.DomainSid,
                               &DomainHandle);
        if (NT_SUCCESS(Status))
        {
            Status = SamSetInformationDomain(DomainHandle,
                                             DomainNameInformation,
                                             (PVOID)&DomainNameInfo);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("SamSetInformationDomain failed (Status: 0x%08lx)\n", Status);
            }

            SamCloseHandle(DomainHandle);
        }
        else
        {
            DPRINT1("SamOpenDomain failed (Status: 0x%08lx)\n", Status);
        }

        SamCloseHandle(ServerHandle);
    }

    return Status;
}


/* Hack */
static
NTSTATUS
SetPrimaryDomain(LPCWSTR DomainName,
                 PSID DomainSid)
{
    PPOLICY_PRIMARY_DOMAIN_INFO OrigInfo = NULL;
    POLICY_PRIMARY_DOMAIN_INFO Info;
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE PolicyHandle;
    NTSTATUS Status;

    DPRINT1("SYSSETUP: SetPrimaryDomain()\n");

    memset(&ObjectAttributes, 0, sizeof(LSA_OBJECT_ATTRIBUTES));
    ObjectAttributes.Length = sizeof(LSA_OBJECT_ATTRIBUTES);

    Status = LsaOpenPolicy(NULL,
                           &ObjectAttributes,
                           POLICY_VIEW_LOCAL_INFORMATION | POLICY_TRUST_ADMIN,
                           &PolicyHandle);
    if (Status != STATUS_SUCCESS)
    {
        DPRINT("LsaOpenPolicy failed (Status: 0x%08lx)\n", Status);
        return Status;
    }

    Status = LsaQueryInformationPolicy(PolicyHandle,
                                       PolicyPrimaryDomainInformation,
                                       (PVOID *)&OrigInfo);
    if (Status == STATUS_SUCCESS && OrigInfo != NULL)
    {
        if (DomainName == NULL)
        {
            Info.Name.Buffer = OrigInfo->Name.Buffer;
            Info.Name.Length = OrigInfo->Name.Length;
            Info.Name.MaximumLength = OrigInfo->Name.MaximumLength;
        }
        else
        {
            Info.Name.Buffer = (LPWSTR)DomainName;
            Info.Name.Length = wcslen(DomainName) * sizeof(WCHAR);
            Info.Name.MaximumLength = Info.Name.Length + sizeof(WCHAR);
        }

        if (DomainSid == NULL)
            Info.Sid = OrigInfo->Sid;
        else
            Info.Sid = DomainSid;
    }
    else
    {
        Info.Name.Buffer = (LPWSTR)DomainName;
        Info.Name.Length = wcslen(DomainName) * sizeof(WCHAR);
        Info.Name.MaximumLength = Info.Name.Length + sizeof(WCHAR);
        Info.Sid = DomainSid;
    }

    Status = LsaSetInformationPolicy(PolicyHandle,
                                     PolicyPrimaryDomainInformation,
                                     (PVOID)&Info);
    if (Status != STATUS_SUCCESS)
    {
        DPRINT("LsaSetInformationPolicy failed (Status: 0x%08lx)\n", Status);
    }

    if (OrigInfo != NULL)
        LsaFreeMemory(OrigInfo);

    LsaClose(PolicyHandle);

    return Status;
}


static
VOID
InstallBuiltinAccounts(VOID)
{
    LPWSTR BuiltinAccounts[] = {
        L"S-1-1-0",         /* Everyone */
        L"S-1-5-4",         /* Interactive */
        L"S-1-5-6",         /* Service */
        L"S-1-5-19",        /* Local Service */
        L"S-1-5-20",        /* Network Service */
        L"S-1-5-32-544",    /* Administrators */
        L"S-1-5-32-545",    /* Users */
        L"S-1-5-32-547",    /* Power Users */
        L"S-1-5-32-551",    /* Backup Operators */
        L"S-1-5-32-555"};   /* Remote Desktop Users */
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    LSA_HANDLE PolicyHandle = NULL;
    LSA_HANDLE AccountHandle = NULL;
    PSID AccountSid;
    ULONG i;

    DPRINT("InstallBuiltinAccounts()\n");

    memset(&ObjectAttributes, 0, sizeof(LSA_OBJECT_ATTRIBUTES));

    Status = LsaOpenPolicy(NULL,
                           &ObjectAttributes,
                           POLICY_CREATE_ACCOUNT,
                           &PolicyHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("LsaOpenPolicy failed (Status %08lx)\n", Status);
        return;
    }

    for (i = 0; i < ARRAYSIZE(BuiltinAccounts); i++)
    {
        if (!ConvertStringSidToSid(BuiltinAccounts[i], &AccountSid))
        {
            DPRINT1("ConvertStringSidToSid(%S) failed: %lu\n", BuiltinAccounts[i], GetLastError());
            continue;
        }

        Status = LsaCreateAccount(PolicyHandle,
                                  AccountSid,
                                  0,
                                  &AccountHandle);
        if (NT_SUCCESS(Status))
        {
            LsaClose(AccountHandle);
        }

        LocalFree(AccountSid);
    }

    LsaClose(PolicyHandle);
}


static
VOID
InstallPrivileges(
    HINF hSecurityInf)
{
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    WCHAR szPrivilegeString[256];
    WCHAR szSidString[256];
    INFCONTEXT InfContext;
    DWORD i;
    PSID AccountSid = NULL;
    NTSTATUS Status;
    LSA_HANDLE PolicyHandle = NULL;
    LSA_UNICODE_STRING RightString;
    PLSA_TRANSLATED_SID2 Sids = NULL;

    DPRINT("InstallPrivileges()\n");

    memset(&ObjectAttributes, 0, sizeof(LSA_OBJECT_ATTRIBUTES));

    Status = LsaOpenPolicy(NULL,
                           &ObjectAttributes,
                           POLICY_CREATE_ACCOUNT | POLICY_LOOKUP_NAMES,
                           &PolicyHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("LsaOpenPolicy failed (Status %08lx)\n", Status);
        goto done;
    }

    if (!SetupFindFirstLineW(hSecurityInf,
                             L"Privilege Rights",
                             NULL,
                             &InfContext))
    {
        DPRINT1("SetupFindFirstLineW failed\n");
        goto done;
    }

    do
    {
        /* Retrieve the privilege name */
        if (!SetupGetStringFieldW(&InfContext,
                                  0,
                                  szPrivilegeString,
                                  ARRAYSIZE(szPrivilegeString),
                                  NULL))
        {
            DPRINT1("SetupGetStringFieldW() failed\n");
            goto done;
        }
        DPRINT("Privilege: %S\n", szPrivilegeString);

        for (i = 0; i < SetupGetFieldCount(&InfContext); i++)
        {
            if (!SetupGetStringFieldW(&InfContext,
                                      i + 1,
                                      szSidString,
                                      ARRAYSIZE(szSidString),
                                      NULL))
            {
                DPRINT1("SetupGetStringFieldW() failed\n");
                goto done;
            }
            DPRINT("SID: %S\n", szSidString);

            if (szSidString[0] == UNICODE_NULL)
                continue;

            if (szSidString[0] == L'*')
            {
                DPRINT("Account Sid: %S\n", &szSidString[1]);

                if (!ConvertStringSidToSid(&szSidString[1], &AccountSid))
                {
                    DPRINT1("ConvertStringSidToSid(%S) failed: %lu\n", szSidString, GetLastError());
                    continue;
                }
            }
            else
            {
                DPRINT("Account name: %S\n", szSidString);
                continue;
 
            }

            RtlInitUnicodeString(&RightString, szPrivilegeString);
            Status = LsaAddAccountRights(PolicyHandle,
                                         (AccountSid != NULL) ? AccountSid : Sids[0].Sid,
                                         &RightString,
                                         1);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("LsaAddAccountRights() failed (Status %08lx)\n", Status);
            }

            if (Sids != NULL)
            {
                LsaFreeMemory(Sids);
                Sids = NULL;
            }

            if (AccountSid != NULL)
            {
                LocalFree(AccountSid);
                AccountSid = NULL;
            }
        }

    }
    while (SetupFindNextLine(&InfContext, &InfContext));

done:
    if (PolicyHandle != NULL)
        LsaClose(PolicyHandle);
}


static
VOID
ApplyRegistryValues(
    HINF hSecurityInf)
{
    WCHAR szRegistryPath[MAX_PATH];
    WCHAR szRootName[MAX_PATH];
    WCHAR szKeyName[MAX_PATH];
    WCHAR szValueName[MAX_PATH];
    INFCONTEXT InfContext;
    DWORD dwLength, dwType;
    HKEY hRootKey, hKey;
    PWSTR Ptr1, Ptr2;
    DWORD dwError;
    PVOID pBuffer;

    DPRINT("ApplyRegistryValues()\n");

    if (!SetupFindFirstLineW(hSecurityInf,
                             L"Registry Values",
                             NULL,
                             &InfContext))
    {
        DPRINT1("SetupFindFirstLineW failed\n");
        return;
    }

    do
    {
        /* Retrieve the privilege name */
        if (!SetupGetStringFieldW(&InfContext,
                                  0,
                                  szRegistryPath,
                                  ARRAYSIZE(szRegistryPath),
                                  NULL))
        {
            DPRINT1("SetupGetStringFieldW() failed\n");
            return;
        }

        DPRINT("RegistryPath: %S\n", szRegistryPath);

        Ptr1 = wcschr(szRegistryPath, L'\\');
        Ptr2 = wcsrchr(szRegistryPath, L'\\');
        if (Ptr1 != NULL && Ptr2 != NULL && Ptr1 != Ptr2)
        {
            dwLength = (DWORD)(((ULONG_PTR)Ptr1 - (ULONG_PTR)szRegistryPath) / sizeof(WCHAR));
            wcsncpy(szRootName, szRegistryPath, dwLength);
            szRootName[dwLength] = UNICODE_NULL;

            Ptr1++;
            dwLength = (DWORD)(((ULONG_PTR)Ptr2 - (ULONG_PTR)Ptr1) / sizeof(WCHAR));
            wcsncpy(szKeyName, Ptr1, dwLength);
            szKeyName[dwLength] = UNICODE_NULL;

            Ptr2++;
            wcscpy(szValueName, Ptr2);

            DPRINT("RootName: %S\n", szRootName);
            DPRINT("KeyName: %S\n", szKeyName);
            DPRINT("ValueName: %S\n", szValueName);

            if (_wcsicmp(szRootName, L"Machine") == 0)
            {
                hRootKey = HKEY_LOCAL_MACHINE;
            }
            else
            {
                DPRINT1("Unsupported root key %S\n", szRootName);
                break;
            }

            if (!SetupGetIntField(&InfContext,
                                  1,
                                  (PINT)&dwType))
            {
                DPRINT1("Failed to get key type (Error %lu)\n", GetLastError());
                break;
            }

            if (dwType != REG_SZ && dwType != REG_EXPAND_SZ && dwType != REG_BINARY &&
                dwType != REG_DWORD && dwType != REG_MULTI_SZ)
            {
                DPRINT1("Invalid value type %lu\n", dwType);
                break;
            }

            dwLength = 0;
            switch (dwType)
            {
                case REG_SZ:
                case REG_EXPAND_SZ:
                    SetupGetStringField(&InfContext,
                                        2,
                                        NULL,
                                        0,
                                        &dwLength);
                    dwLength *= sizeof(WCHAR);
                    break;

                case REG_BINARY:
                    SetupGetBinaryField(&InfContext,
                                        2,
                                        NULL,
                                        0,
                                        &dwLength);
                    break;

                case REG_DWORD:
                    dwLength = sizeof(INT);
                    break;

                case REG_MULTI_SZ:
                    SetupGetMultiSzField(&InfContext,
                                         2,
                                         NULL,
                                         0,
                                         &dwLength);
                    dwLength *= sizeof(WCHAR);
                    break;
            }

            if (dwLength == 0)
            {
                DPRINT1("Failed to determine the required buffer size!\n");
                break;
            }

            dwError = RegCreateKeyExW(hRootKey,
                                      szKeyName,
                                      0,
                                      NULL,
                                      REG_OPTION_NON_VOLATILE,
                                      KEY_WRITE,
                                      NULL,
                                      &hKey,
                                      NULL);
            if (dwError != ERROR_SUCCESS)
            {
                DPRINT1("Failed to create the key %S (Error %lu)\n", szKeyName, dwError);
                break;
            }

            pBuffer = HeapAlloc(GetProcessHeap(), 0, dwLength);
            if (pBuffer)
            {
                switch (dwType)
                {
                    case REG_SZ:
                    case REG_EXPAND_SZ:
                        SetupGetStringField(&InfContext,
                                            2,
                                            pBuffer,
                                            dwLength / sizeof(WCHAR),
                                            &dwLength);
                        dwLength *= sizeof(WCHAR);
                        break;

                    case REG_BINARY:
                        SetupGetBinaryField(&InfContext,
                                            2,
                                            pBuffer,
                                            dwLength,
                                            &dwLength);
                        break;

                    case REG_DWORD:
                        SetupGetIntField(&InfContext,
                                         2,
                                         pBuffer);
                        break;

                    case REG_MULTI_SZ:
                        SetupGetMultiSzField(&InfContext,
                                             2,
                                             pBuffer,
                                             dwLength / sizeof(WCHAR),
                                             &dwLength);
                        dwLength *= sizeof(WCHAR);
                        break;
                }

                RegSetValueEx(hKey,
                              szValueName,
                              0,
                              dwType,
                              pBuffer,
                              dwLength);

                HeapFree(GetProcessHeap(), 0, pBuffer);
            }

            RegCloseKey(hKey);
        }
    }
    while (SetupFindNextLine(&InfContext, &InfContext));
}


VOID
InstallSecurity(VOID)
{
    HINF hSecurityInf;
    PWSTR pszSecurityInf;

//    if (IsServer())
//        pszSecurityInf = L"defltsv.inf";
//    else
        pszSecurityInf = L"defltws.inf";

    InstallBuiltinAccounts();

    hSecurityInf = SetupOpenInfFileW(pszSecurityInf,
                                     NULL,
                                     INF_STYLE_WIN4,
                                     NULL);
    if (hSecurityInf != INVALID_HANDLE_VALUE)
    {
        InstallPrivileges(hSecurityInf);
        ApplyRegistryValues(hSecurityInf);

        SetupCloseInfFile(hSecurityInf);
    }

    /* Hack */
    SetPrimaryDomain(L"WORKGROUP", NULL);
}


NTSTATUS
SetAdministratorPassword(LPCWSTR Password)
{
    PPOLICY_ACCOUNT_DOMAIN_INFO OrigInfo = NULL;
    PUSER_ACCOUNT_NAME_INFORMATION AccountNameInfo = NULL;
    USER_SET_PASSWORD_INFORMATION PasswordInfo;
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE PolicyHandle = NULL;
    SAM_HANDLE ServerHandle = NULL;
    SAM_HANDLE DomainHandle = NULL;
    SAM_HANDLE UserHandle = NULL;
    NTSTATUS Status;

    DPRINT("SYSSETUP: SetAdministratorPassword(%p)\n", Password);

    memset(&ObjectAttributes, 0, sizeof(LSA_OBJECT_ATTRIBUTES));
    ObjectAttributes.Length = sizeof(LSA_OBJECT_ATTRIBUTES);

    Status = LsaOpenPolicy(NULL,
                           &ObjectAttributes,
                           POLICY_VIEW_LOCAL_INFORMATION | POLICY_TRUST_ADMIN,
                           &PolicyHandle);
    if (Status != STATUS_SUCCESS)
    {
        DPRINT1("LsaOpenPolicy() failed (Status: 0x%08lx)\n", Status);
        return Status;
    }

    Status = LsaQueryInformationPolicy(PolicyHandle,
                                       PolicyAccountDomainInformation,
                                       (PVOID *)&OrigInfo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("LsaQueryInformationPolicy() failed (Status: 0x%08lx)\n", Status);
        goto done;
    }

    Status = SamConnect(NULL,
                        &ServerHandle,
                        SAM_SERVER_CONNECT | SAM_SERVER_LOOKUP_DOMAIN,
                        NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SamConnect() failed (Status: 0x%08lx)\n", Status);
        goto done;
    }

    Status = SamOpenDomain(ServerHandle,
                           DOMAIN_LOOKUP,
                           OrigInfo->DomainSid,
                           &DomainHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SamOpenDomain() failed (Status: 0x%08lx)\n", Status);
        goto done;
    }

    Status = SamOpenUser(DomainHandle,
                         USER_FORCE_PASSWORD_CHANGE | USER_READ_GENERAL,
                         DOMAIN_USER_RID_ADMIN,
                         &UserHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SamOpenUser() failed (Status %08lx)\n", Status);
        goto done;
    }

    RtlInitUnicodeString(&PasswordInfo.Password, Password);
    PasswordInfo.PasswordExpired = FALSE;

    Status = SamSetInformationUser(UserHandle,
                                   UserSetPasswordInformation,
                                   (PVOID)&PasswordInfo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SamSetInformationUser() failed (Status %08lx)\n", Status);
        goto done;
    }

    Status = SamQueryInformationUser(UserHandle,
                                     UserAccountNameInformation,
                                     (PVOID*)&AccountNameInfo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SamSetInformationUser() failed (Status %08lx)\n", Status);
        goto done;
    }

    AdminInfo.Name = RtlAllocateHeap(RtlGetProcessHeap(),
                                     HEAP_ZERO_MEMORY,
                                     AccountNameInfo->UserName.Length + sizeof(WCHAR));
    if (AdminInfo.Name != NULL)
        RtlCopyMemory(AdminInfo.Name,
                      AccountNameInfo->UserName.Buffer,
                      AccountNameInfo->UserName.Length);

    AdminInfo.Domain = RtlAllocateHeap(RtlGetProcessHeap(),
                                       HEAP_ZERO_MEMORY,
                                       OrigInfo->DomainName.Length + sizeof(WCHAR));
    if (AdminInfo.Domain != NULL)
        RtlCopyMemory(AdminInfo.Domain,
                      OrigInfo->DomainName.Buffer,
                      OrigInfo->DomainName.Length);

    AdminInfo.Password = RtlAllocateHeap(RtlGetProcessHeap(),
                                         0,
                                         (wcslen(Password) + 1) * sizeof(WCHAR));
    if (AdminInfo.Password != NULL)
        wcscpy(AdminInfo.Password, Password);

    DPRINT("Administrator Name: %S\n", AdminInfo.Name);
    DPRINT("Administrator Domain: %S\n", AdminInfo.Domain);
    DPRINT("Administrator Password: %S\n", AdminInfo.Password);

done:
    if (AccountNameInfo != NULL)
        SamFreeMemory(AccountNameInfo);

    if (OrigInfo != NULL)
        LsaFreeMemory(OrigInfo);

    if (PolicyHandle != NULL)
        LsaClose(PolicyHandle);

    if (UserHandle != NULL)
        SamCloseHandle(UserHandle);

    if (DomainHandle != NULL)
        SamCloseHandle(DomainHandle);

    if (ServerHandle != NULL)
        SamCloseHandle(ServerHandle);

    DPRINT1("SYSSETUP: SetAdministratorPassword() done (Status %08lx)\n", Status);

    return Status;
}


VOID
SetAutoAdminLogon(VOID)
{
    WCHAR szAutoAdminLogon[2];
    HKEY hKey = NULL;
    DWORD dwType;
    DWORD dwSize;
    LONG lError;

    lError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                           0,
                           KEY_READ | KEY_WRITE,
                           &hKey);
    if (lError != ERROR_SUCCESS)
        return;

    dwSize = 2 * sizeof(WCHAR);
    lError = RegQueryValueExW(hKey,
                              L"AutoAdminLogon",
                              NULL,
                              &dwType,
                              (LPBYTE)szAutoAdminLogon,
                              &dwSize);
    if (lError != ERROR_SUCCESS)
        goto done;

    if (wcscmp(szAutoAdminLogon, L"1") == 0)
    {
        RegSetValueExW(hKey,
                       L"DefaultDomainName",
                       0,
                       REG_SZ,
                       (LPBYTE)AdminInfo.Domain,
                       (wcslen(AdminInfo.Domain) + 1) * sizeof(WCHAR));

        RegSetValueExW(hKey,
                       L"DefaultUserName",
                       0,
                       REG_SZ,
                       (LPBYTE)AdminInfo.Name,
                       (wcslen(AdminInfo.Name) + 1) * sizeof(WCHAR));

        RegSetValueExW(hKey,
                       L"DefaultPassword",
                       0,
                       REG_SZ,
                       (LPBYTE)AdminInfo.Password,
                       (wcslen(AdminInfo.Password) + 1) * sizeof(WCHAR));
    }

done:
    if (hKey != NULL)
        RegCloseKey(hKey);
}


/* EOF */

