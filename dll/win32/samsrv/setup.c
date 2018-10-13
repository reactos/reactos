/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Security Account Manager (SAM) Server
 * FILE:            reactos/dll/win32/samsrv/setup.c
 * PURPOSE:         Registry setup routines
 *
 * PROGRAMMERS:     Eric Kohl
 */

#include "samsrv.h"

#include <ntsecapi.h>

#include "resources.h"

/* GLOBALS *****************************************************************/

#define TICKS_PER_SECOND 10000000LL

SID_IDENTIFIER_AUTHORITY SecurityNtAuthority = {SECURITY_NT_AUTHORITY};

/* FUNCTIONS ***************************************************************/

static BOOL
SampSetupAddMemberToAlias(HKEY hDomainKey,
                          ULONG AliasId,
                          PSID MemberSid)
{
    DWORD dwDisposition;
    LPWSTR MemberSidString = NULL;
    WCHAR szKeyName[256];
    HKEY hMembersKey;

    ConvertSidToStringSidW(MemberSid, &MemberSidString);

    swprintf(szKeyName, L"Aliases\\%08lX\\Members", AliasId);

    if (!RegCreateKeyExW(hDomainKey,
                         szKeyName,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         KEY_ALL_ACCESS,
                         NULL,
                         &hMembersKey,
                         &dwDisposition))
    {
        RegSetValueEx(hMembersKey,
                      MemberSidString,
                      0,
                      REG_BINARY,
                      (LPVOID)MemberSid,
                      RtlLengthSid(MemberSid));

        RegCloseKey(hMembersKey);
    }

    swprintf(szKeyName, L"Aliases\\Members\\%s", MemberSidString);

    if (!RegCreateKeyExW(hDomainKey,
                         szKeyName,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         KEY_ALL_ACCESS,
                         NULL,
                         &hMembersKey,
                         &dwDisposition))
    {
        swprintf(szKeyName, L"%08lX", AliasId);

        RegSetValueEx(hMembersKey,
                      szKeyName,
                      0,
                      REG_BINARY,
                      (LPVOID)MemberSid,
                      RtlLengthSid(MemberSid));

        RegCloseKey(hMembersKey);
    }

    if (MemberSidString != NULL)
        LocalFree(MemberSidString);

    return TRUE;
}


static
NTSTATUS
SampSetupCreateAliasAccount(HANDLE hDomainKey,
                            LPCWSTR lpAccountName,
                            LPCWSTR lpDescription,
                            ULONG ulRelativeId)
{
    WCHAR szAccountKeyName[32];
    HANDLE hAccountKey = NULL;
    HANDLE hNamesKey = NULL;
    PSECURITY_DESCRIPTOR Sd = NULL;
    ULONG SdSize = 0;
    NTSTATUS Status;

    swprintf(szAccountKeyName, L"Aliases\\%08lX", ulRelativeId);

    Status = SampRegCreateKey(hDomainKey,
                              szAccountKeyName,
                              KEY_ALL_ACCESS,
                              &hAccountKey);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = SampRegSetValue(hAccountKey,
                             L"Name",
                             REG_SZ,
                             (LPVOID)lpAccountName,
                             (wcslen(lpAccountName) + 1) * sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampRegSetValue(hAccountKey,
                             L"Description",
                             REG_SZ,
                             (LPVOID)lpDescription,
                             (wcslen(lpDescription) + 1) * sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
        goto done;

    /* Create the server SD */
    Status = SampCreateAliasSD(&Sd,
                               &SdSize);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set SecDesc attribute*/
    Status = SampRegSetValue(hAccountKey,
                             L"SecDesc",
                             REG_BINARY,
                             Sd,
                             SdSize);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampRegOpenKey(hDomainKey,
                            L"Aliases\\Names",
                            KEY_ALL_ACCESS,
                            &hNamesKey);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampRegSetValue(hNamesKey,
                            lpAccountName,
                            REG_DWORD,
                            (LPVOID)&ulRelativeId,
                            sizeof(ULONG));

done:
    SampRegCloseKey(&hNamesKey);

    if (Sd != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, Sd);

    if (hAccountKey != NULL)
    {
        SampRegCloseKey(&hAccountKey);

        if (!NT_SUCCESS(Status))
            SampRegDeleteKey(hDomainKey,
                             szAccountKeyName);
    }

    return Status;
}


static
NTSTATUS
SampSetupAddMemberToGroup(IN HANDLE hDomainKey,
                          IN ULONG GroupId,
                          IN ULONG MemberId)
{
    WCHAR szKeyName[256];
    HANDLE hGroupKey = NULL;
    PULONG MembersBuffer = NULL;
    ULONG MembersCount = 0;
    ULONG Length = 0;
    ULONG i;
    NTSTATUS Status;

    swprintf(szKeyName, L"Groups\\%08lX", GroupId);

    Status = SampRegOpenKey(hDomainKey,
                            szKeyName,
                            KEY_ALL_ACCESS,
                            &hGroupKey);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = SampRegQueryValue(hGroupKey,
                               L"Members",
                               NULL,
                               NULL,
                               &Length);
    if (!NT_SUCCESS(Status) && Status != STATUS_OBJECT_NAME_NOT_FOUND)
        goto done;

    MembersBuffer = midl_user_allocate(Length + sizeof(ULONG));
    if (MembersBuffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
    {
        Status = SampRegQueryValue(hGroupKey,
                                   L"Members",
                                   NULL,
                                   MembersBuffer,
                                   &Length);
        if (!NT_SUCCESS(Status))
            goto done;

        MembersCount = Length / sizeof(ULONG);
    }

    for (i = 0; i < MembersCount; i++)
    {
        if (MembersBuffer[i] == MemberId)
        {
            Status = STATUS_MEMBER_IN_GROUP;
            goto done;
        }
    }

    MembersBuffer[MembersCount] = MemberId;
    Length += sizeof(ULONG);

    Status = SampRegSetValue(hGroupKey,
                             L"Members",
                             REG_BINARY,
                             MembersBuffer,
                             Length);

done:
    if (MembersBuffer != NULL)
        midl_user_free(MembersBuffer);

    SampRegCloseKey(&hGroupKey);

    return Status;
}


static
NTSTATUS
SampSetupCreateGroupAccount(HANDLE hDomainKey,
                            LPCWSTR lpAccountName,
                            LPCWSTR lpComment,
                            ULONG ulRelativeId)
{
    SAM_GROUP_FIXED_DATA FixedGroupData;
    WCHAR szAccountKeyName[32];
    HANDLE hAccountKey = NULL;
    HANDLE hNamesKey = NULL;
    PSECURITY_DESCRIPTOR Sd = NULL;
    ULONG SdSize = 0;
    NTSTATUS Status;

    /* Initialize fixed group data */
    FixedGroupData.Version = 1;
    FixedGroupData.Reserved = 0;
    FixedGroupData.GroupId = ulRelativeId;
    FixedGroupData.Attributes = 0;

    swprintf(szAccountKeyName, L"Groups\\%08lX", ulRelativeId);

    Status = SampRegCreateKey(hDomainKey,
                              szAccountKeyName,
                              KEY_ALL_ACCESS,
                              &hAccountKey);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = SampRegSetValue(hAccountKey,
                             L"F",
                             REG_BINARY,
                             (LPVOID)&FixedGroupData,
                             sizeof(SAM_GROUP_FIXED_DATA));
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampRegSetValue(hAccountKey,
                             L"Name",
                             REG_SZ,
                             (LPVOID)lpAccountName,
                             (wcslen(lpAccountName) + 1) * sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampRegSetValue(hAccountKey,
                             L"AdminComment",
                             REG_SZ,
                             (LPVOID)lpComment,
                             (wcslen(lpComment) + 1) * sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
        goto done;

    /* Create the security descriptor */
    Status = SampCreateGroupSD(&Sd,
                               &SdSize);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set the SecDesc attribute*/
    Status = SampRegSetValue(hAccountKey,
                             L"SecDesc",
                             REG_BINARY,
                             Sd,
                             SdSize);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampRegOpenKey(hDomainKey,
                            L"Groups\\Names",
                            KEY_ALL_ACCESS,
                            &hNamesKey);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampRegSetValue(hNamesKey,
                            lpAccountName,
                            REG_DWORD,
                            (LPVOID)&ulRelativeId,
                            sizeof(ULONG));

done:
    SampRegCloseKey(&hNamesKey);

    if (Sd != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, Sd);

    if (hAccountKey != NULL)
    {
        SampRegCloseKey(&hAccountKey);

        if (!NT_SUCCESS(Status))
            SampRegDeleteKey(hDomainKey,
                             szAccountKeyName);
    }

    return Status;
}


static
NTSTATUS
SampSetupCreateUserAccount(HANDLE hDomainKey,
                           LPCWSTR lpAccountName,
                           LPCWSTR lpComment,
                           PSID lpDomainSid,
                           ULONG ulRelativeId,
                           ULONG UserAccountControl)
{
    SAM_USER_FIXED_DATA FixedUserData;
    GROUP_MEMBERSHIP GroupMembership;
    UCHAR LogonHours[23];
    LPWSTR lpEmptyString = L"";
    WCHAR szAccountKeyName[32];
    HANDLE hAccountKey = NULL;
    HANDLE hNamesKey = NULL;
    PSECURITY_DESCRIPTOR Sd = NULL;
    ULONG SdSize = 0;
    PSID UserSid = NULL;
    NTSTATUS Status;

    UserSid = AppendRidToSid(lpDomainSid,
                             ulRelativeId);

    /* Create the security descriptor */
    Status = SampCreateUserSD(UserSid,
                              &Sd,
                              &SdSize);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Initialize fixed user data */
    FixedUserData.Version = 1;
    FixedUserData.Reserved = 0;
    FixedUserData.LastLogon.QuadPart = 0;
    FixedUserData.LastLogoff.QuadPart = 0;
    FixedUserData.PasswordLastSet.QuadPart = 0;
    FixedUserData.AccountExpires.QuadPart = MAXLONGLONG;
    FixedUserData.LastBadPasswordTime.QuadPart = 0;
    FixedUserData.UserId = ulRelativeId;
    FixedUserData.PrimaryGroupId = DOMAIN_GROUP_RID_USERS;
    FixedUserData.UserAccountControl = UserAccountControl;
    FixedUserData.CountryCode = 0;
    FixedUserData.CodePage = 0;
    FixedUserData.BadPasswordCount = 0;
    FixedUserData.LogonCount = 0;
    FixedUserData.AdminCount = 0;
    FixedUserData.OperatorCount = 0;

    swprintf(szAccountKeyName, L"Users\\%08lX", ulRelativeId);

    Status = SampRegCreateKey(hDomainKey,
                              szAccountKeyName,
                              KEY_ALL_ACCESS,
                              &hAccountKey);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = SampRegSetValue(hAccountKey,
                             L"F",
                             REG_BINARY,
                             (LPVOID)&FixedUserData,
                             sizeof(SAM_USER_FIXED_DATA));
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampRegSetValue(hAccountKey,
                             L"Name",
                             REG_SZ,
                             (LPVOID)lpAccountName,
                             (wcslen(lpAccountName) + 1) * sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampRegSetValue(hAccountKey,
                             L"FullName",
                             REG_SZ,
                             (LPVOID)lpEmptyString,
                             sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampRegSetValue(hAccountKey,
                             L"HomeDirectory",
                             REG_SZ,
                             (LPVOID)lpEmptyString,
                             sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampRegSetValue(hAccountKey,
                             L"HomeDirectoryDrive",
                             REG_SZ,
                             (LPVOID)lpEmptyString,
                             sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampRegSetValue(hAccountKey,
                             L"ScriptPath",
                             REG_SZ,
                             (LPVOID)lpEmptyString,
                             sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampRegSetValue(hAccountKey,
                             L"ProfilePath",
                             REG_SZ,
                             (LPVOID)lpEmptyString,
                             sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampRegSetValue(hAccountKey,
                             L"AdminComment",
                             REG_SZ,
                             (LPVOID)lpComment,
                             (wcslen(lpComment) + 1) * sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampRegSetValue(hAccountKey,
                             L"UserComment",
                             REG_SZ,
                             (LPVOID)lpEmptyString,
                             sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampRegSetValue(hAccountKey,
                             L"WorkStations",
                             REG_SZ,
                             (LPVOID)lpEmptyString,
                             sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampRegSetValue(hAccountKey,
                             L"Parameters",
                             REG_SZ,
                             (LPVOID)lpEmptyString,
                             sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set LogonHours attribute*/
    *((PUSHORT)LogonHours) = 168;
    memset(&(LogonHours[2]), 0xff, 21);

    Status = SampRegSetValue(hAccountKey,
                             L"LogonHours",
                             REG_BINARY,
                             (LPVOID)LogonHours,
                             sizeof(LogonHours));
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set Groups attribute*/
    GroupMembership.RelativeId = DOMAIN_GROUP_RID_USERS;
    GroupMembership.Attributes = SE_GROUP_MANDATORY |
                                 SE_GROUP_ENABLED |
                                 SE_GROUP_ENABLED_BY_DEFAULT;

    Status = SampRegSetValue(hAccountKey,
                             L"Groups",
                             REG_BINARY,
                             (LPVOID)&GroupMembership,
                             sizeof(GROUP_MEMBERSHIP));
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set LMPwd attribute*/
    Status = SampRegSetValue(hAccountKey,
                             L"LMPwd",
                             REG_BINARY,
                             (LPVOID)&EmptyLmHash,
                             sizeof(ENCRYPTED_LM_OWF_PASSWORD));
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set NTPwd attribute*/
    Status = SampRegSetValue(hAccountKey,
                             L"NTPwd",
                             REG_BINARY,
                             (LPVOID)&EmptyNtHash,
                             sizeof(ENCRYPTED_NT_OWF_PASSWORD));
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set LMPwdHistory attribute*/
    Status = SampRegSetValue(hAccountKey,
                             L"LMPwdHistory",
                             REG_BINARY,
                             NULL,
                             0);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set NTPwdHistory attribute*/
    Status = SampRegSetValue(hAccountKey,
                             L"NTPwdHistory",
                             REG_BINARY,
                             NULL,
                             0);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set PrivateData attribute*/
    Status = SampRegSetValue(hAccountKey,
                             L"PrivateData",
                             REG_SZ,
                             (LPVOID)lpEmptyString,
                             sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set the SecDesc attribute*/
    Status = SampRegSetValue(hAccountKey,
                             L"SecDesc",
                             REG_BINARY,
                             Sd,
                             SdSize);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampRegOpenKey(hDomainKey,
                            L"Users\\Names",
                            KEY_ALL_ACCESS,
                            &hNamesKey);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampRegSetValue(hNamesKey,
                            lpAccountName,
                            REG_DWORD,
                            (LPVOID)&ulRelativeId,
                            sizeof(ULONG));

done:
    SampRegCloseKey(&hNamesKey);

    if (Sd != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, Sd);

    if (UserSid != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, UserSid);

    if (hAccountKey != NULL)
    {
        SampRegCloseKey(&hAccountKey);

        if (!NT_SUCCESS(Status))
            SampRegDeleteKey(hDomainKey,
                             szAccountKeyName);
    }

    return Status;
}


static
NTSTATUS
SampSetupCreateDomain(IN HANDLE hServerKey,
                      IN LPCWSTR lpKeyName,
                      IN LPCWSTR lpDomainName,
                      IN PSID lpDomainSid,
                      IN BOOLEAN bBuiltinDomain,
                      OUT HANDLE *lpDomainKey)
{
    SAM_DOMAIN_FIXED_DATA FixedData;
    WCHAR szDomainKeyName[32];
    LPWSTR lpEmptyString = L"";
    HANDLE hDomainKey = NULL;
    HANDLE hAliasesKey = NULL;
    HANDLE hGroupsKey = NULL;
    HANDLE hUsersKey = NULL;
    HANDLE hNamesKey = NULL;
    PSECURITY_DESCRIPTOR Sd = NULL;
    ULONG SdSize = 0;
    NTSTATUS Status;

    if (lpDomainKey != NULL)
        *lpDomainKey = NULL;

    /* Initialize the fixed domain data */
    memset(&FixedData, 0, sizeof(SAM_DOMAIN_FIXED_DATA));
    FixedData.Version = 1;
    NtQuerySystemTime(&FixedData.CreationTime);
    FixedData.DomainModifiedCount.QuadPart = 0;
    FixedData.MaxPasswordAge.QuadPart = -(6LL * 7LL * 24LL * 60LL * 60LL * TICKS_PER_SECOND); /* 6 weeks */
    FixedData.MinPasswordAge.QuadPart = 0;                                                    /* right now */
    FixedData.ForceLogoff.QuadPart = LLONG_MAX;                                               /* very far in the future aka never */
    FixedData.LockoutDuration.QuadPart = -(30LL * 60LL * TICKS_PER_SECOND);                   /* 30 minutes */
    FixedData.LockoutObservationWindow.QuadPart = -(30LL * 60LL * TICKS_PER_SECOND);          /* 30 minutes */
    FixedData.ModifiedCountAtLastPromotion.QuadPart = 0;
    FixedData.NextRid = 1000;
    FixedData.PasswordProperties = 0;
    FixedData.MinPasswordLength = 0;
    FixedData.PasswordHistoryLength = 0;
    FixedData.LockoutThreshold = 0;
    FixedData.DomainServerState = DomainServerEnabled;
    FixedData.DomainServerRole = DomainServerRolePrimary;
    FixedData.UasCompatibilityRequired = TRUE;

    wcscpy(szDomainKeyName, L"Domains\\");
    wcscat(szDomainKeyName, lpKeyName);

    Status = SampRegCreateKey(hServerKey,
                              szDomainKeyName,
                              KEY_ALL_ACCESS,
                              &hDomainKey);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Set the fixed data value */
    Status = SampRegSetValue(hDomainKey,
                             L"F",
                             REG_BINARY,
                             (LPVOID)&FixedData,
                             sizeof(SAM_DOMAIN_FIXED_DATA));
    if (!NT_SUCCESS(Status))
        goto done;

    if (lpDomainSid != NULL)
    {
        Status = SampRegSetValue(hDomainKey,
                                 L"Name",
                                 REG_SZ,
                                 (LPVOID)lpDomainName,
                                 (wcslen(lpDomainName) + 1) * sizeof(WCHAR));
        if (!NT_SUCCESS(Status))
            goto done;

        Status = SampRegSetValue(hDomainKey,
                                 L"SID",
                                 REG_BINARY,
                                 (LPVOID)lpDomainSid,
                                 RtlLengthSid(lpDomainSid));
        if (!NT_SUCCESS(Status))
            goto done;
    }

    Status = SampRegSetValue(hDomainKey,
                             L"OemInformation",
                             REG_SZ,
                             (LPVOID)lpEmptyString,
                             sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampRegSetValue(hDomainKey,
                             L"ReplicaSourceNodeName",
                             REG_SZ,
                             (LPVOID)lpEmptyString,
                             sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
        goto done;

    /* Create the Alias container */
    Status = SampRegCreateKey(hDomainKey,
                              L"Aliases",
                              KEY_ALL_ACCESS,
                              &hAliasesKey);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampRegCreateKey(hAliasesKey,
                              L"Names",
                              KEY_ALL_ACCESS,
                              &hNamesKey);
    if (!NT_SUCCESS(Status))
        goto done;

    SampRegCloseKey(&hNamesKey);

    /* Create the Groups container */
    Status = SampRegCreateKey(hDomainKey,
                              L"Groups",
                              KEY_ALL_ACCESS,
                              &hGroupsKey);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampRegCreateKey(hGroupsKey,
                              L"Names",
                              KEY_ALL_ACCESS,
                              &hNamesKey);
    if (!NT_SUCCESS(Status))
        goto done;

    SampRegCloseKey(&hNamesKey);

    /* Create the Users container */
    Status = SampRegCreateKey(hDomainKey,
                              L"Users",
                              KEY_ALL_ACCESS,
                              &hUsersKey);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampRegCreateKey(hUsersKey,
                              L"Names",
                              KEY_ALL_ACCESS,
                              &hNamesKey);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Create the server SD */
    if (bBuiltinDomain != FALSE)
        Status = SampCreateBuiltinDomainSD(&Sd,
                                           &SdSize);
    else
        Status = SampCreateAccountDomainSD(&Sd,
                                           &SdSize);

    if (!NT_SUCCESS(Status))
        goto done;

    /* Set SecDesc attribute*/
    Status = SampRegSetValue(hServerKey,
                             L"SecDesc",
                             REG_BINARY,
                             Sd,
                             SdSize);
    if (!NT_SUCCESS(Status))
        goto done;

    SampRegCloseKey(&hNamesKey);

    if (lpDomainKey != NULL)
        *lpDomainKey = hDomainKey;

done:
    if (Sd != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, Sd);

    SampRegCloseKey(&hAliasesKey);
    SampRegCloseKey(&hGroupsKey);
    SampRegCloseKey(&hUsersKey);

    if (!NT_SUCCESS(Status))
        SampRegCloseKey(&hDomainKey);

    return Status;
}


static
NTSTATUS
SampSetupCreateServer(IN HANDLE hSamKey,
                      OUT HANDLE *lpServerKey)
{
    HANDLE hServerKey = NULL;
    HANDLE hDomainsKey = NULL;
    PSECURITY_DESCRIPTOR Sd = NULL;
    ULONG SdSize = 0;
    NTSTATUS Status;

    Status = SampRegCreateKey(hSamKey,
                              L"SAM",
                              KEY_ALL_ACCESS,
                              &hServerKey);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = SampRegCreateKey(hServerKey,
                              L"Domains",
                              KEY_ALL_ACCESS,
                              &hDomainsKey);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Create the server SD */
    Status = SampCreateServerSD(&Sd,
                                &SdSize);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set SecDesc attribute*/
    Status = SampRegSetValue(hServerKey,
                             L"SecDesc",
                             REG_BINARY,
                             Sd,
                             SdSize);
    if (!NT_SUCCESS(Status))
        goto done;

    SampRegCloseKey(&hDomainsKey);

    *lpServerKey = hServerKey;

done:
    if (Sd != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, Sd);

    return Status;
}


NTSTATUS
SampGetAccountDomainInfo(PPOLICY_ACCOUNT_DOMAIN_INFO *AccountDomainInfo)
{
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE PolicyHandle;
    NTSTATUS Status;

    TRACE("SampGetAccountDomainInfo\n");

    memset(&ObjectAttributes, 0, sizeof(LSA_OBJECT_ATTRIBUTES));
    ObjectAttributes.Length = sizeof(LSA_OBJECT_ATTRIBUTES);

    Status = LsaOpenPolicy(NULL,
                           &ObjectAttributes,
                           POLICY_VIEW_LOCAL_INFORMATION,
                           &PolicyHandle);
    if (Status != STATUS_SUCCESS)
    {
        ERR("LsaOpenPolicy failed (Status: 0x%08lx)\n", Status);
        return Status;
    }

    Status = LsaQueryInformationPolicy(PolicyHandle,
                                       PolicyAccountDomainInformation,
                                       (PVOID *)AccountDomainInfo);

    LsaClose(PolicyHandle);

    return Status;
}


BOOL
SampInitializeSAM(VOID)
{
    PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomainInfo = NULL;
    HANDLE hSamKey = NULL;
    HANDLE hServerKey = NULL;
    HANDLE hBuiltinDomainKey = NULL;
    HANDLE hAccountDomainKey = NULL;
    PSID pBuiltinSid = NULL;
    PSID pInteractiveSid = NULL;
    PSID pAuthenticatedUserSid = NULL;
    BOOL bResult = TRUE;
    PSID pSid;
    HINSTANCE hInstance;
    WCHAR szComment[256];
    WCHAR szName[80];
    NTSTATUS Status;

    TRACE("SampInitializeSAM() called\n");

    hInstance = GetModuleHandleW(L"samsrv.dll");

    /* Open the SAM key */
    Status = SampRegOpenKey(NULL,
                            L"\\Registry\\Machine\\SAM",
                            KEY_READ | KEY_CREATE_SUB_KEY | KEY_ENUMERATE_SUB_KEYS,
                            &hSamKey);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to open the SAM key (Status: 0x%08lx)\n", Status);
        return FALSE;
    }

    /* Create the SAM Server object */
    Status = SampSetupCreateServer(hSamKey,
                                   &hServerKey);
    if (!NT_SUCCESS(Status))
    {
        bResult = FALSE;
        goto done;
    }

    /* Create and initialize the Builtin Domain SID */
    pBuiltinSid = RtlAllocateHeap(RtlGetProcessHeap(), 0, RtlLengthRequiredSid(1));
    if (pBuiltinSid == NULL)
    {
        ERR("Failed to allocate the Builtin Domain SID\n");
        bResult = FALSE;
        goto done;
    }

    RtlInitializeSid(pBuiltinSid, &SecurityNtAuthority, 1);
    *(RtlSubAuthoritySid(pBuiltinSid, 0)) = SECURITY_BUILTIN_DOMAIN_RID;

    /* Create and initialize the Interactive SID */
    pInteractiveSid = RtlAllocateHeap(RtlGetProcessHeap(), 0, RtlLengthRequiredSid(1));
    if (pInteractiveSid == NULL)
    {
        ERR("Failed to allocate the Interactive SID\n");
        bResult = FALSE;
        goto done;
    }

    RtlInitializeSid(pInteractiveSid, &SecurityNtAuthority, 1);
    *(RtlSubAuthoritySid(pInteractiveSid, 0)) = SECURITY_INTERACTIVE_RID;

    /* Create and initialize the Authenticated User SID */
    pAuthenticatedUserSid = RtlAllocateHeap(RtlGetProcessHeap(), 0, RtlLengthRequiredSid(1));
    if (pAuthenticatedUserSid == NULL)
    {
        ERR("Failed to allocate the Authenticated User SID\n");
        bResult = FALSE;
        goto done;
    }

    RtlInitializeSid(pAuthenticatedUserSid, &SecurityNtAuthority, 1);
    *(RtlSubAuthoritySid(pAuthenticatedUserSid, 0)) = SECURITY_AUTHENTICATED_USER_RID;

    /* Get account domain information */
    Status = SampGetAccountDomainInfo(&AccountDomainInfo);
    if (!NT_SUCCESS(Status))
    {
        ERR("SampGetAccountDomainInfo failed (Status %08lx)\n", Status);
        bResult = FALSE;
        goto done;
    }

    SampLoadString(hInstance, IDS_DOMAIN_BUILTIN_NAME, szName, 80);

    /* Create the Builtin domain */
    Status = SampSetupCreateDomain(hServerKey,
                                   L"Builtin",
                                   szName,
                                   pBuiltinSid,
                                   TRUE,
                                   &hBuiltinDomainKey);
    if (!NT_SUCCESS(Status))
    {
        bResult = FALSE;
        goto done;
    }

    SampLoadString(hInstance, IDS_ALIAS_ADMINISTRATORS_NAME, szName, 80);
    SampLoadString(hInstance, IDS_ALIAS_ADMINISTRATORS_COMMENT, szComment, 256);

        SampSetupCreateAliasAccount(hBuiltinDomainKey,
                                    szName,
                                    szComment,
                                    DOMAIN_ALIAS_RID_ADMINS);

        SampLoadString(hInstance, IDS_ALIAS_USERS_NAME, szName, 80);
        SampLoadString(hInstance, IDS_ALIAS_USERS_COMMENT, szComment, 256);

        SampSetupCreateAliasAccount(hBuiltinDomainKey,
                                    szName,
                                    szComment,
                                    DOMAIN_ALIAS_RID_USERS);

        SampLoadString(hInstance, IDS_ALIAS_GUESTS_NAME, szName, 80);
        SampLoadString(hInstance, IDS_ALIAS_GUESTS_COMMENT, szComment, 256);

        SampSetupCreateAliasAccount(hBuiltinDomainKey,
                                    szName,
                                    szComment,
                                    DOMAIN_ALIAS_RID_GUESTS);

        SampLoadString(hInstance, IDS_ALIAS_POWER_USERS_NAME, szName, 80);
        SampLoadString(hInstance, IDS_ALIAS_POWER_USERS_COMMENT, szComment, 256);

        SampSetupCreateAliasAccount(hBuiltinDomainKey,
                                    szName,
                                    szComment,
                                    DOMAIN_ALIAS_RID_POWER_USERS);

        /* Add the Administrator user to the Administrators alias */
        pSid = AppendRidToSid(AccountDomainInfo->DomainSid,
                              DOMAIN_USER_RID_ADMIN);
        if (pSid != NULL)
        {
            SampSetupAddMemberToAlias(hBuiltinDomainKey,
                                      DOMAIN_ALIAS_RID_ADMINS,
                                      pSid);

            RtlFreeHeap(RtlGetProcessHeap(), 0, pSid);
        }

        /* Add the Guest user to the Guests alias */
        pSid = AppendRidToSid(AccountDomainInfo->DomainSid,
                              DOMAIN_USER_RID_GUEST);
        if (pSid != NULL)
        {
            SampSetupAddMemberToAlias(hBuiltinDomainKey,
                                      DOMAIN_ALIAS_RID_GUESTS,
                                      pSid);

            RtlFreeHeap(RtlGetProcessHeap(), 0, pSid);
        }

    /* Add the Interactive SID to the Users alias */
    SampSetupAddMemberToAlias(hBuiltinDomainKey,
                              DOMAIN_ALIAS_RID_USERS,
                              pInteractiveSid);

    /* Add the Authenticated User SID to the Users alias */
    SampSetupAddMemberToAlias(hBuiltinDomainKey,
                              DOMAIN_ALIAS_RID_USERS,
                              pAuthenticatedUserSid);

    /* Create the Account domain */
    Status = SampSetupCreateDomain(hServerKey,
                                   L"Account",
                                   L"",
                                   AccountDomainInfo->DomainSid,
                                   FALSE,
                                   &hAccountDomainKey);
    if (!NT_SUCCESS(Status))
    {
        bResult = FALSE;
        goto done;
    }

        SampLoadString(hInstance, IDS_GROUP_NONE_NAME, szName, 80);
        SampLoadString(hInstance, IDS_GROUP_NONE_COMMENT, szComment, 256);

        SampSetupCreateGroupAccount(hAccountDomainKey,
                                    szName,
                                    szComment,
                                    DOMAIN_GROUP_RID_USERS);

        SampLoadString(hInstance, IDS_USER_ADMINISTRATOR_NAME, szName, 80);
        SampLoadString(hInstance, IDS_USER_ADMINISTRATOR_COMMENT, szComment, 256);

        SampSetupCreateUserAccount(hAccountDomainKey,
                                   szName,
                                   szComment,
                                   AccountDomainInfo->DomainSid,
                                   DOMAIN_USER_RID_ADMIN,
                                   USER_DONT_EXPIRE_PASSWORD | USER_NORMAL_ACCOUNT);

        SampSetupAddMemberToGroup(hAccountDomainKey,
                                  DOMAIN_GROUP_RID_USERS,
                                  DOMAIN_USER_RID_ADMIN);

        SampLoadString(hInstance, IDS_USER_GUEST_NAME, szName, 80);
        SampLoadString(hInstance, IDS_USER_GUEST_COMMENT, szComment, 256);

        SampSetupCreateUserAccount(hAccountDomainKey,
                                   szName,
                                   szComment,
                                   AccountDomainInfo->DomainSid,
                                   DOMAIN_USER_RID_GUEST,
                                   USER_ACCOUNT_DISABLED | USER_DONT_EXPIRE_PASSWORD | USER_NORMAL_ACCOUNT);

        SampSetupAddMemberToGroup(hAccountDomainKey,
                                  DOMAIN_GROUP_RID_USERS,
                                  DOMAIN_USER_RID_GUEST);

done:
    if (AccountDomainInfo)
        LsaFreeMemory(AccountDomainInfo);

    if (pAuthenticatedUserSid)
        RtlFreeHeap(RtlGetProcessHeap(), 0, pAuthenticatedUserSid);

    if (pInteractiveSid)
        RtlFreeHeap(RtlGetProcessHeap(), 0, pInteractiveSid);

    if (pBuiltinSid)
        RtlFreeHeap(RtlGetProcessHeap(), 0, pBuiltinSid);

    SampRegCloseKey(&hAccountDomainKey);
    SampRegCloseKey(&hBuiltinDomainKey);
    SampRegCloseKey(&hServerKey);
    SampRegCloseKey(&hSamKey);

    TRACE("SampInitializeSAM() done\n");

    return bResult;
}
