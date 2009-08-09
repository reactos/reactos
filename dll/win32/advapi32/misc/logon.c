/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/advapi32/misc/logon.c
 * PURPOSE:     Logon functions
 * PROGRAMMER:  Eric Kohl
 */

#include <advapi32.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(advapi);


/* FUNCTIONS ***************************************************************/

/*
 * @implemented
 */
BOOL WINAPI
CreateProcessAsUserA(HANDLE hToken,
                     LPCSTR lpApplicationName,
                     LPSTR lpCommandLine,
                     LPSECURITY_ATTRIBUTES lpProcessAttributes,
                     LPSECURITY_ATTRIBUTES lpThreadAttributes,
                     BOOL bInheritHandles,
                     DWORD dwCreationFlags,
                     LPVOID lpEnvironment,
                     LPCSTR lpCurrentDirectory,
                     LPSTARTUPINFOA lpStartupInfo,
                     LPPROCESS_INFORMATION lpProcessInformation)
{
    PROCESS_ACCESS_TOKEN AccessToken;
    NTSTATUS Status;

    /* Create the process with a suspended main thread */
    if (!CreateProcessA(lpApplicationName,
                        lpCommandLine,
                        lpProcessAttributes,
                        lpThreadAttributes,
                        bInheritHandles,
                        dwCreationFlags | CREATE_SUSPENDED,
                        lpEnvironment,
                        lpCurrentDirectory,
                        lpStartupInfo,
                        lpProcessInformation))
    {
        return FALSE;
    }

    AccessToken.Token = hToken;
    AccessToken.Thread = NULL;

    /* Set the new process token */
    Status = NtSetInformationProcess(lpProcessInformation->hProcess,
                                     ProcessAccessToken,
                                     (PVOID)&AccessToken,
                                     sizeof(AccessToken));
    if (!NT_SUCCESS (Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    /* Resume the main thread */
    if (!(dwCreationFlags & CREATE_SUSPENDED))
    {
       ResumeThread(lpProcessInformation->hThread);
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL WINAPI
CreateProcessAsUserW(HANDLE hToken,
                     LPCWSTR lpApplicationName,
                     LPWSTR lpCommandLine,
                     LPSECURITY_ATTRIBUTES lpProcessAttributes,
                     LPSECURITY_ATTRIBUTES lpThreadAttributes,
                     BOOL bInheritHandles,
                     DWORD dwCreationFlags,
                     LPVOID lpEnvironment,
                     LPCWSTR lpCurrentDirectory,
                     LPSTARTUPINFOW lpStartupInfo,
                     LPPROCESS_INFORMATION lpProcessInformation)
{
    PROCESS_ACCESS_TOKEN AccessToken;
    NTSTATUS Status;

    /* Create the process with a suspended main thread */
    if (!CreateProcessW(lpApplicationName,
                        lpCommandLine,
                        lpProcessAttributes,
                        lpThreadAttributes,
                        bInheritHandles,
                        dwCreationFlags | CREATE_SUSPENDED,
                        lpEnvironment,
                        lpCurrentDirectory,
                        lpStartupInfo,
                        lpProcessInformation))
    {
        return FALSE;
    }

    AccessToken.Token = hToken;
    AccessToken.Thread = NULL;

    /* Set the new process token */
    Status = NtSetInformationProcess(lpProcessInformation->hProcess,
                                     ProcessAccessToken,
                                     (PVOID)&AccessToken,
                                     sizeof(AccessToken));
    if (!NT_SUCCESS (Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    /* Resume the main thread */
    if (!(dwCreationFlags & CREATE_SUSPENDED))
    {
        ResumeThread(lpProcessInformation->hThread);
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL WINAPI
LogonUserA(LPSTR lpszUsername,
           LPSTR lpszDomain,
           LPSTR lpszPassword,
           DWORD dwLogonType,
           DWORD dwLogonProvider,
           PHANDLE phToken)
{
    UNICODE_STRING UserName;
    UNICODE_STRING Domain;
    UNICODE_STRING Password;
    NTSTATUS Status;
    BOOL ret = FALSE;

    UserName.Buffer = NULL;
    Domain.Buffer = NULL;
    Password.Buffer = NULL;

    Status = RtlCreateUnicodeStringFromAsciiz(&UserName,
                                              lpszUsername);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        goto UsernameDone;
    }

    Status = RtlCreateUnicodeStringFromAsciiz(&Domain,
                                              lpszDomain);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        goto DomainDone;
    }

    Status = RtlCreateUnicodeStringFromAsciiz(&Password,
                                              lpszPassword);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        goto PasswordDone;
    }

    ret = LogonUserW(UserName.Buffer,
                     Domain.Buffer,
                     Password.Buffer,
                     dwLogonType,
                     dwLogonProvider,
                     phToken);

    if (Password.Buffer != NULL)
        RtlFreeUnicodeString(&Password);

PasswordDone:
    if (Domain.Buffer != NULL)
        RtlFreeUnicodeString(&Domain);

DomainDone:
    if (UserName.Buffer != NULL)
        RtlFreeUnicodeString(&UserName);

UsernameDone:
    return ret;
}


static BOOL WINAPI
SamGetUserSid(LPCWSTR UserName,
              PSID *Sid)
{
    PSID lpSid;
    DWORD dwLength;
    HKEY hUsersKey;
    HKEY hUserKey;

    if (Sid != NULL)
        *Sid = NULL;

    /* Open the Users key */
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"SAM\\SAM\\Domains\\Account\\Users",
                      0,
                      KEY_READ,
                      &hUsersKey))
    {
        ERR("Failed to open Users key! (Error %lu)\n", GetLastError());
        return FALSE;
    }

    /* Open the user key */
    if (RegOpenKeyExW(hUsersKey,
                      UserName,
                      0,
                      KEY_READ,
                      &hUserKey))
    {
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            ERR("Invalid user name!\n");
            SetLastError(ERROR_NO_SUCH_USER);
        }
        else
        {
            ERR("Failed to open user key! (Error %lu)\n", GetLastError());
        }

        RegCloseKey(hUsersKey);
        return FALSE;
    }

    RegCloseKey (hUsersKey);

    /* Get SID size */
    dwLength = 0;
    if (RegQueryValueExW(hUserKey,
                         L"Sid",
                         NULL,
                         NULL,
                         NULL,
                         &dwLength))
    {
        ERR("Failed to read the SID size! (Error %lu)\n", GetLastError());
        RegCloseKey(hUserKey);
        return FALSE;
    }

    /* Allocate sid buffer */
    TRACE("Required SID buffer size: %lu\n", dwLength);
    lpSid = (PSID)RtlAllocateHeap(RtlGetProcessHeap(),
                                  0,
                                  dwLength);
    if (lpSid == NULL)
    {
        ERR("Failed to allocate SID buffer!\n");
        RegCloseKey(hUserKey);
        return FALSE;
    }

    /* Read sid */
    if (RegQueryValueExW(hUserKey,
                         L"Sid",
                         NULL,
                         NULL,
                         (LPBYTE)lpSid,
                         &dwLength))
    {
        ERR("Failed to read the SID! (Error %lu)\n", GetLastError());
        RtlFreeHeap(RtlGetProcessHeap(),
                    0,
                    lpSid);
        RegCloseKey(hUserKey);
        return FALSE;
    }

    RegCloseKey(hUserKey);

    *Sid = lpSid;

    return TRUE;
}


static BOOL WINAPI
SamGetDomainSid(PSID *Sid)
{
    PSID lpSid;
    DWORD dwLength;
    HKEY hDomainKey;

    TRACE("SamGetDomainSid() called\n");

    if (Sid != NULL)
        *Sid = NULL;

    /* Open the account domain key */
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"SAM\\SAM\\Domains\\Account",
                      0,
                      KEY_READ,
                      &hDomainKey))
    {
        ERR("Failed to open the account domain key! (Error %lu)\n", GetLastError());
        return FALSE;
    }

    /* Get SID size */
    dwLength = 0;
    if (RegQueryValueExW(hDomainKey,
                         L"Sid",
                         NULL,
                         NULL,
                         NULL,
                         &dwLength))
    {
        ERR("Failed to read the SID size! (Error %lu)\n", GetLastError());
        RegCloseKey(hDomainKey);
        return FALSE;
    }

    /* Allocate sid buffer */
    TRACE("Required SID buffer size: %lu\n", dwLength);
    lpSid = (PSID)RtlAllocateHeap(RtlGetProcessHeap(),
                                  0,
                                  dwLength);
    if (lpSid == NULL)
    {
        ERR("Failed to allocate SID buffer!\n");
        RegCloseKey(hDomainKey);
        return FALSE;
    }

    /* Read sid */
    if (RegQueryValueExW(hDomainKey,
                         L"Sid",
                         NULL,
                         NULL,
                         (LPBYTE)lpSid,
                         &dwLength))
    {
        ERR("Failed to read the SID! (Error %lu)\n", GetLastError());
        RtlFreeHeap(RtlGetProcessHeap(),
                    0,
                    lpSid);
        RegCloseKey(hDomainKey);
        return FALSE;
    }

    RegCloseKey(hDomainKey);

    *Sid = lpSid;

    TRACE("SamGetDomainSid() done\n");

    return TRUE;
}


static PSID
AppendRidToSid(PSID SrcSid,
               ULONG Rid)
{
    ULONG Rids[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    UCHAR RidCount;
    PSID DstSid;
    ULONG i;

    RidCount = *RtlSubAuthorityCountSid(SrcSid);
    if (RidCount >= 8)
        return NULL;

    for (i = 0; i < RidCount; i++)
        Rids[i] = *RtlSubAuthoritySid(SrcSid, i);

    Rids[RidCount] = Rid;
    RidCount++;

    RtlAllocateAndInitializeSid(RtlIdentifierAuthoritySid(SrcSid),
                                RidCount,
                                Rids[0],
                                Rids[1],
                                Rids[2],
                                Rids[3],
                                Rids[4],
                                Rids[5],
                                Rids[6],
                                Rids[7],
                                &DstSid);

    return DstSid;
}


static PTOKEN_GROUPS
AllocateGroupSids(OUT PSID *PrimaryGroupSid,
                  OUT PSID *OwnerSid)
{
    SID_IDENTIFIER_AUTHORITY WorldAuthority = {SECURITY_WORLD_SID_AUTHORITY};
    SID_IDENTIFIER_AUTHORITY LocalAuthority = {SECURITY_LOCAL_SID_AUTHORITY};
    SID_IDENTIFIER_AUTHORITY SystemAuthority = {SECURITY_NT_AUTHORITY};
    PTOKEN_GROUPS TokenGroups;
#define MAX_GROUPS 8
    DWORD GroupCount = 0;
    PSID DomainSid;
    PSID Sid;
    LUID Luid;
    NTSTATUS Status;

    Status = NtAllocateLocallyUniqueId(&Luid);
    if (!NT_SUCCESS(Status))
        return NULL;

    if (!SamGetDomainSid(&DomainSid))
        return NULL;

    TokenGroups = RtlAllocateHeap(
        GetProcessHeap(), 0,
        sizeof(TOKEN_GROUPS) +
        MAX_GROUPS * sizeof(SID_AND_ATTRIBUTES));
    if (TokenGroups == NULL)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, DomainSid);
        return NULL;
    }

    Sid = AppendRidToSid(DomainSid, DOMAIN_GROUP_RID_USERS);
    RtlFreeHeap(RtlGetProcessHeap(), 0, DomainSid);

    /* Member of the domain */
    TokenGroups->Groups[GroupCount].Sid = Sid;
    TokenGroups->Groups[GroupCount].Attributes =
        SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY;
    *PrimaryGroupSid = Sid;
    GroupCount++;

    /* Member of 'Everyone' */
    RtlAllocateAndInitializeSid(&WorldAuthority,
                                1,
                                SECURITY_WORLD_RID,
                                SECURITY_NULL_RID,
                                SECURITY_NULL_RID,
                                SECURITY_NULL_RID,
                                SECURITY_NULL_RID,
                                SECURITY_NULL_RID,
                                SECURITY_NULL_RID,
                                SECURITY_NULL_RID,
                                &Sid);
    TokenGroups->Groups[GroupCount].Sid = Sid;
    TokenGroups->Groups[GroupCount].Attributes =
        SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY;
    GroupCount++;

#if 1
    /* Member of 'Administrators' */
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
                                &Sid);
    TokenGroups->Groups[GroupCount].Sid = Sid;
    TokenGroups->Groups[GroupCount].Attributes =
        SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY;
    GroupCount++;
#else
    TRACE("Not adding user to Administrators group\n");
#endif

    /* Member of 'Users' */
    RtlAllocateAndInitializeSid(&SystemAuthority,
                                2,
                                SECURITY_BUILTIN_DOMAIN_RID,
                                DOMAIN_ALIAS_RID_USERS,
                                SECURITY_NULL_RID,
                                SECURITY_NULL_RID,
                                SECURITY_NULL_RID,
                                SECURITY_NULL_RID,
                                SECURITY_NULL_RID,
                                SECURITY_NULL_RID,
                                &Sid);
    TokenGroups->Groups[GroupCount].Sid = Sid;
    TokenGroups->Groups[GroupCount].Attributes =
        SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY;
    GroupCount++;

    /* Logon SID */
    RtlAllocateAndInitializeSid(&SystemAuthority,
                                SECURITY_LOGON_IDS_RID_COUNT,
                                SECURITY_LOGON_IDS_RID,
                                Luid.HighPart,
                                Luid.LowPart,
                                SECURITY_NULL_RID,
                                SECURITY_NULL_RID,
                                SECURITY_NULL_RID,
                                SECURITY_NULL_RID,
                                SECURITY_NULL_RID,
                                &Sid);
    TokenGroups->Groups[GroupCount].Sid = Sid;
    TokenGroups->Groups[GroupCount].Attributes =
        SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY | SE_GROUP_LOGON_ID;
    GroupCount++;
    *OwnerSid = Sid;

    /* Member of 'Local users */
    RtlAllocateAndInitializeSid(&LocalAuthority,
                                1,
                                SECURITY_LOCAL_RID,
                                SECURITY_NULL_RID,
                                SECURITY_NULL_RID,
                                SECURITY_NULL_RID,
                                SECURITY_NULL_RID,
                                SECURITY_NULL_RID,
                                SECURITY_NULL_RID,
                                SECURITY_NULL_RID,
                                &Sid);
    TokenGroups->Groups[GroupCount].Sid = Sid;
    TokenGroups->Groups[GroupCount].Attributes =
        SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY;
    GroupCount++;

    /* Member of 'Interactive users' */
    RtlAllocateAndInitializeSid(&SystemAuthority,
                                1,
                                SECURITY_INTERACTIVE_RID,
                                SECURITY_NULL_RID,
                                SECURITY_NULL_RID,
                                SECURITY_NULL_RID,
                                SECURITY_NULL_RID,
                                SECURITY_NULL_RID,
                                SECURITY_NULL_RID,
                                SECURITY_NULL_RID,
                                &Sid);
    TokenGroups->Groups[GroupCount].Sid = Sid;
    TokenGroups->Groups[GroupCount].Attributes =
        SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY;
    GroupCount++;

    /* Member of 'Authenticated users' */
    RtlAllocateAndInitializeSid(&SystemAuthority,
                                1,
                                SECURITY_AUTHENTICATED_USER_RID,
                                SECURITY_NULL_RID,
                                SECURITY_NULL_RID,
                                SECURITY_NULL_RID,
                                SECURITY_NULL_RID,
                                SECURITY_NULL_RID,
                                SECURITY_NULL_RID,
                                SECURITY_NULL_RID,
                                &Sid);
    TokenGroups->Groups[GroupCount].Sid = Sid;
    TokenGroups->Groups[GroupCount].Attributes =
        SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY;
    GroupCount++;

    TokenGroups->GroupCount = GroupCount;
    ASSERT(TokenGroups->GroupCount <= MAX_GROUPS);

    return TokenGroups;
}


static VOID
FreeGroupSids(PTOKEN_GROUPS TokenGroups)
{
    ULONG i;

    for (i = 0; i < TokenGroups->GroupCount; i++)
    {
        if (TokenGroups->Groups[i].Sid != NULL)
            RtlFreeHeap(GetProcessHeap(), 0, TokenGroups->Groups[i].Sid);
    }

    RtlFreeHeap(GetProcessHeap(), 0, TokenGroups);
}


/*
 * @unimplemented
 */
BOOL WINAPI
LogonUserW(LPWSTR lpszUsername,
           LPWSTR lpszDomain,
           LPWSTR lpszPassword,
           DWORD dwLogonType,
           DWORD dwLogonProvider,
           PHANDLE phToken)
{
    /* FIXME shouldn't use hard-coded list of privileges */
    static struct
    {
      LPCWSTR PrivName;
      DWORD Attributes;
    }
    DefaultPrivs[] =
    {
      { L"SeUnsolicitedInputPrivilege", 0 },
      { L"SeMachineAccountPrivilege", 0 },
      { L"SeSecurityPrivilege", 0 },
      { L"SeTakeOwnershipPrivilege", 0 },
      { L"SeLoadDriverPrivilege", 0 },
      { L"SeSystemProfilePrivilege", 0 },
      { L"SeSystemtimePrivilege", 0 },
      { L"SeProfileSingleProcessPrivilege", 0 },
      { L"SeIncreaseBasePriorityPrivilege", 0 },
      { L"SeCreatePagefilePrivilege", 0 },
      { L"SeBackupPrivilege", 0 },
      { L"SeRestorePrivilege", 0 },
      { L"SeShutdownPrivilege", 0 },
      { L"SeDebugPrivilege", 0 },
      { L"SeSystemEnvironmentPrivilege", 0 },
      { L"SeChangeNotifyPrivilege", SE_PRIVILEGE_ENABLED | SE_PRIVILEGE_ENABLED_BY_DEFAULT },
      { L"SeRemoteShutdownPrivilege", 0 },
      { L"SeUndockPrivilege", 0 },
      { L"SeEnableDelegationPrivilege", 0 },
      { L"SeImpersonatePrivilege", SE_PRIVILEGE_ENABLED | SE_PRIVILEGE_ENABLED_BY_DEFAULT },
      { L"SeCreateGlobalPrivilege", SE_PRIVILEGE_ENABLED | SE_PRIVILEGE_ENABLED_BY_DEFAULT }
    };
    OBJECT_ATTRIBUTES ObjectAttributes;
    SECURITY_QUALITY_OF_SERVICE Qos;
    TOKEN_USER TokenUser;
    TOKEN_OWNER TokenOwner;
    TOKEN_PRIMARY_GROUP TokenPrimaryGroup;
    PTOKEN_GROUPS TokenGroups;
    PTOKEN_PRIVILEGES TokenPrivileges;
    TOKEN_DEFAULT_DACL TokenDefaultDacl;
    LARGE_INTEGER ExpirationTime;
    LUID AuthenticationId;
    TOKEN_SOURCE TokenSource;
    PSID UserSid = NULL;
    PSID PrimaryGroupSid = NULL;
    PSID OwnerSid = NULL;
    PSID LocalSystemSid;
    PACL Dacl;
    NTSTATUS Status;
    SID_IDENTIFIER_AUTHORITY SystemAuthority = {SECURITY_NT_AUTHORITY};
    unsigned i;

    Qos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    Qos.ImpersonationLevel = SecurityAnonymous;
    Qos.ContextTrackingMode = SECURITY_STATIC_TRACKING;
    Qos.EffectiveOnly = FALSE;

    ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
    ObjectAttributes.RootDirectory = NULL;
    ObjectAttributes.ObjectName = NULL;
    ObjectAttributes.Attributes = 0;
    ObjectAttributes.SecurityDescriptor = NULL;
    ObjectAttributes.SecurityQualityOfService = &Qos;

    Status = NtAllocateLocallyUniqueId(&AuthenticationId);
    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    ExpirationTime.QuadPart = -1;

    /* Get the user SID from the registry */
    if (!SamGetUserSid (lpszUsername, &UserSid))
    {
        ERR("SamGetUserSid() failed\n");
        return FALSE;
    }

    TokenUser.User.Sid = UserSid;
    TokenUser.User.Attributes = 0;

    /* Allocate and initialize token groups */
    TokenGroups = AllocateGroupSids(&PrimaryGroupSid,
                                    &OwnerSid);
    if (NULL == TokenGroups)
    {
        RtlFreeSid(UserSid);
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    /* Allocate and initialize token privileges */
    TokenPrivileges = RtlAllocateHeap(GetProcessHeap(), 0,
                                      sizeof(TOKEN_PRIVILEGES)
                                    + sizeof(DefaultPrivs) / sizeof(DefaultPrivs[0])
                                      * sizeof(LUID_AND_ATTRIBUTES));
    if (NULL == TokenPrivileges)
    {
        FreeGroupSids(TokenGroups);
        RtlFreeSid(UserSid);
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    TokenPrivileges->PrivilegeCount = 0;
    for (i = 0; i < sizeof(DefaultPrivs) / sizeof(DefaultPrivs[0]); i++)
    {
        if (! LookupPrivilegeValueW(NULL,
                                    DefaultPrivs[i].PrivName,
                                    &TokenPrivileges->Privileges[TokenPrivileges->PrivilegeCount].Luid))
        {
            WARN("Can't set privilege %S\n", DefaultPrivs[i].PrivName);
        }
        else
        {
            TokenPrivileges->Privileges[TokenPrivileges->PrivilegeCount].Attributes = DefaultPrivs[i].Attributes;
            TokenPrivileges->PrivilegeCount++;
        }
    }

    TokenOwner.Owner = OwnerSid;
    TokenPrimaryGroup.PrimaryGroup = PrimaryGroupSid;

    Dacl = RtlAllocateHeap(GetProcessHeap(), 0, 1024);
    if (Dacl == NULL)
    {
        FreeGroupSids(TokenGroups);
        RtlFreeSid(UserSid);
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    Status = RtlCreateAcl(Dacl, 1024, ACL_REVISION);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(GetProcessHeap(), 0, Dacl);
        FreeGroupSids(TokenGroups);
        RtlFreeHeap(GetProcessHeap(), 0, TokenPrivileges);
        RtlFreeSid(UserSid);
        return FALSE;
    }

    RtlAddAccessAllowedAce(Dacl,
                           ACL_REVISION,
                           GENERIC_ALL,
                           OwnerSid);

  RtlAllocateAndInitializeSid(&SystemAuthority,
                              1,
                              SECURITY_LOCAL_SYSTEM_RID,
                              SECURITY_NULL_RID,
                              SECURITY_NULL_RID,
                              SECURITY_NULL_RID,
                              SECURITY_NULL_RID,
                              SECURITY_NULL_RID,
                              SECURITY_NULL_RID,
                              SECURITY_NULL_RID,
                              &LocalSystemSid);

    /* SID: S-1-5-18 */
    RtlAddAccessAllowedAce(Dacl,
                           ACL_REVISION,
                           GENERIC_ALL,
                           LocalSystemSid);

    RtlFreeSid(LocalSystemSid);

    TokenDefaultDacl.DefaultDacl = Dacl;

    memcpy(TokenSource.SourceName,
           "User32  ",
           8);

    Status = NtAllocateLocallyUniqueId(&TokenSource.SourceIdentifier);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(GetProcessHeap(), 0, Dacl);
        FreeGroupSids(TokenGroups);
        RtlFreeHeap(GetProcessHeap(), 0, TokenPrivileges);
        RtlFreeSid(UserSid);
       return FALSE;
    }

    Status = NtCreateToken(phToken,
                           TOKEN_ALL_ACCESS,
                           &ObjectAttributes,
                           TokenPrimary,
                           &AuthenticationId,
                           &ExpirationTime,
                           &TokenUser,
                           TokenGroups,
                           TokenPrivileges,
                           &TokenOwner,
                           &TokenPrimaryGroup,
                           &TokenDefaultDacl,
                           &TokenSource);

    RtlFreeHeap(GetProcessHeap(), 0, Dacl);
    FreeGroupSids(TokenGroups);
    RtlFreeHeap(GetProcessHeap(), 0, TokenPrivileges);
    RtlFreeSid(UserSid);

    return NT_SUCCESS(Status);
}

/* EOF */
