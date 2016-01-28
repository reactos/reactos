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
SetAccountDomain(LPCWSTR DomainName,
                 PSID DomainSid)
{
    PPOLICY_ACCOUNT_DOMAIN_INFO OrigInfo = NULL;
    POLICY_ACCOUNT_DOMAIN_INFO Info;
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE PolicyHandle;

    SAM_HANDLE ServerHandle = NULL;
    SAM_HANDLE DomainHandle = NULL;
    DOMAIN_NAME_INFORMATION DomainNameInfo;

    NTSTATUS Status;

    DPRINT("SYSSETUP: SetAccountDomain\n");

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

    for (i = 0; i < 10; i++)
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
InstallPrivileges(VOID)
{
    HINF hSecurityInf = INVALID_HANDLE_VALUE;
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    WCHAR szPrivilegeString[256];
    WCHAR szSidString[256];
    INFCONTEXT InfContext;
    DWORD i;
    PRIVILEGE_SET PrivilegeSet;
    PSID AccountSid;
    NTSTATUS Status;
    LSA_HANDLE PolicyHandle = NULL;
    LSA_HANDLE AccountHandle;

    DPRINT("InstallPrivileges()\n");

    hSecurityInf = SetupOpenInfFileW(L"defltws.inf", //szNameBuffer,
                                     NULL,
                                     INF_STYLE_WIN4,
                                     NULL);
    if (hSecurityInf == INVALID_HANDLE_VALUE)
    {
        DPRINT1("SetupOpenInfFileW failed\n");
        return;
    }

    memset(&ObjectAttributes, 0, sizeof(LSA_OBJECT_ATTRIBUTES));

    Status = LsaOpenPolicy(NULL,
                           &ObjectAttributes,
                           POLICY_CREATE_ACCOUNT,
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
        DPRINT1("SetupFindfirstLineW failed\n");
        goto done;
    }

    PrivilegeSet.PrivilegeCount = 1;
    PrivilegeSet.Control = 0;

    do
    {
        /* Retrieve the privilege name */
        if (!SetupGetStringFieldW(&InfContext,
                                  0,
                                  szPrivilegeString,
                                  256,
                                  NULL))
        {
            DPRINT1("SetupGetStringFieldW() failed\n");
            goto done;
        }
        DPRINT("Privilege: %S\n", szPrivilegeString);

        if (!LookupPrivilegeValueW(NULL,
                                   szPrivilegeString,
                                   &(PrivilegeSet.Privilege[0].Luid)))
        {
            DPRINT1("LookupPrivilegeNameW() failed\n");
            goto done;
        }

        PrivilegeSet.Privilege[0].Attributes = 0;

        for (i = 0; i < SetupGetFieldCount(&InfContext); i++)
        {
            if (!SetupGetStringFieldW(&InfContext,
                                      i + 1,
                                      szSidString,
                                      256,
                                      NULL))
            {
                DPRINT1("SetupGetStringFieldW() failed\n");
                goto done;
            }
            DPRINT("SID: %S\n", szSidString);

            if (!ConvertStringSidToSid(szSidString, &AccountSid))
            {
                DPRINT1("ConvertStringSidToSid(%S) failed: %lu\n", szSidString, GetLastError());
                continue;
            }

            Status = LsaOpenAccount(PolicyHandle,
                                    AccountSid,
                                    ACCOUNT_VIEW | ACCOUNT_ADJUST_PRIVILEGES,
                                    &AccountHandle);
            if (NT_SUCCESS(Status))
            {
                Status = LsaAddPrivilegesToAccount(AccountHandle,
                                                   &PrivilegeSet);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("LsaAddPrivilegesToAccount() failed (Status %08lx)\n", Status);
                }

                LsaClose(AccountHandle);
            }

            LocalFree(AccountSid);
        }

    }
    while (SetupFindNextLine(&InfContext, &InfContext));

done:
    if (PolicyHandle != NULL)
        LsaClose(PolicyHandle);

    if (hSecurityInf != INVALID_HANDLE_VALUE)
        SetupCloseInfFile(hSecurityInf);
}


VOID
InstallSecurity(VOID)
{
    InstallBuiltinAccounts();
    InstallPrivileges();

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
                       L"DefaultDomain",
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

