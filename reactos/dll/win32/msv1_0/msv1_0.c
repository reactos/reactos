/*
 * PROJECT:     Authentication Package DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/msv1_0/msv1_0.c
 * PURPOSE:     Main file
 * COPYRIGHT:   Copyright 2013 Eric Kohl
 */

/* INCLUDES ****************************************************************/

#include "msv1_0.h"

WINE_DEFAULT_DEBUG_CHANNEL(msv1_0);


/* GLOBALS *****************************************************************/

LSA_DISPATCH_TABLE DispatchTable;


/* FUNCTIONS ***************************************************************/

static
NTSTATUS
GetDomainSid(PRPC_SID *Sid)
{
    LSAPR_HANDLE PolicyHandle = NULL;
    PLSAPR_POLICY_INFORMATION PolicyInfo = NULL;
    ULONG Length = 0;
    NTSTATUS Status;

    Status = LsaIOpenPolicyTrusted(&PolicyHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("LsaIOpenPolicyTrusted() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    Status = LsarQueryInformationPolicy(PolicyHandle,
                                        PolicyAccountDomainInformation,
                                        &PolicyInfo);
    if (!NT_SUCCESS(Status))
    {
        TRACE("LsarQueryInformationPolicy() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    Length = RtlLengthSid(PolicyInfo->PolicyAccountDomainInfo.Sid);

    *Sid = RtlAllocateHeap(RtlGetProcessHeap(), 0, Length);
    if (*Sid == NULL)
    {
        ERR("Failed to allocate SID\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    memcpy(*Sid, PolicyInfo->PolicyAccountDomainInfo.Sid, Length);

done:
    if (PolicyInfo != NULL)
        LsaIFree_LSAPR_POLICY_INFORMATION(PolicyAccountDomainInformation,
                                          PolicyInfo);

    if (PolicyHandle != NULL)
        LsarClose(&PolicyHandle);

    return Status;
}


static
NTSTATUS
BuildInteractiveProfileBuffer(IN PLSA_CLIENT_REQUEST ClientRequest,
                              IN PSAMPR_USER_INFO_BUFFER UserInfo,
                              IN PUNICODE_STRING LogonServer,
                              OUT PMSV1_0_INTERACTIVE_PROFILE *ProfileBuffer,
                              OUT PULONG ProfileBufferLength)
{
    PMSV1_0_INTERACTIVE_PROFILE LocalBuffer = NULL;
    PVOID ClientBaseAddress = NULL;
    LPWSTR Ptr;
    ULONG BufferLength;
    NTSTATUS Status = STATUS_SUCCESS;

    *ProfileBuffer = NULL;
    *ProfileBufferLength = 0;

    BufferLength = sizeof(MSV1_0_INTERACTIVE_PROFILE) +
                   UserInfo->All.FullName.Length + sizeof(WCHAR) +
                   UserInfo->All.HomeDirectory.Length + sizeof(WCHAR) +
                   UserInfo->All.HomeDirectoryDrive.Length + sizeof(WCHAR) +
                   UserInfo->All.ScriptPath.Length + sizeof(WCHAR) +
                   UserInfo->All.ProfilePath.Length + sizeof(WCHAR) +
                   LogonServer->Length + sizeof(WCHAR);

    LocalBuffer = DispatchTable.AllocateLsaHeap(BufferLength);
    if (LocalBuffer == NULL)
    {
        TRACE("Failed to allocate the local buffer!\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    Status = DispatchTable.AllocateClientBuffer(ClientRequest,
                                                BufferLength,
                                                &ClientBaseAddress);
    if (!NT_SUCCESS(Status))
    {
        TRACE("DispatchTable.AllocateClientBuffer failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    TRACE("ClientBaseAddress: %p\n", ClientBaseAddress);

    Ptr = (LPWSTR)((ULONG_PTR)LocalBuffer + sizeof(MSV1_0_INTERACTIVE_PROFILE));

    LocalBuffer->MessageType = MsV1_0InteractiveProfile;
    LocalBuffer->LogonCount = UserInfo->All.LogonCount;
    LocalBuffer->BadPasswordCount = UserInfo->All.BadPasswordCount;

    LocalBuffer->LogonTime.LowPart = UserInfo->All.LastLogon.LowPart;
    LocalBuffer->LogonTime.HighPart = UserInfo->All.LastLogon.HighPart;

//    LocalBuffer->LogoffTime.LowPart =
//    LocalBuffer->LogoffTime.HighPart =

//    LocalBuffer->KickOffTime.LowPart =
//    LocalBuffer->KickOffTime.HighPart =

    LocalBuffer->PasswordLastSet.LowPart = UserInfo->All.PasswordLastSet.LowPart;
    LocalBuffer->PasswordLastSet.HighPart = UserInfo->All.PasswordLastSet.HighPart;

    LocalBuffer->PasswordCanChange.LowPart = UserInfo->All.PasswordCanChange.LowPart;
    LocalBuffer->PasswordCanChange.HighPart = UserInfo->All.PasswordCanChange.HighPart;

    LocalBuffer->PasswordMustChange.LowPart = UserInfo->All.PasswordMustChange.LowPart;
    LocalBuffer->PasswordMustChange.HighPart = UserInfo->All.PasswordMustChange.HighPart;

    LocalBuffer->LogonScript.Length = UserInfo->All.ScriptPath.Length;
    LocalBuffer->LogonScript.MaximumLength = UserInfo->All.ScriptPath.Length + sizeof(WCHAR);
    LocalBuffer->LogonScript.Buffer = (LPWSTR)((ULONG_PTR)ClientBaseAddress + (ULONG_PTR)Ptr - (ULONG_PTR)LocalBuffer);
    memcpy(Ptr,
           UserInfo->All.ScriptPath.Buffer,
           UserInfo->All.ScriptPath.Length);

    Ptr = (LPWSTR)((ULONG_PTR)Ptr + LocalBuffer->LogonScript.MaximumLength);

    LocalBuffer->HomeDirectory.Length = UserInfo->All.HomeDirectory.Length;
    LocalBuffer->HomeDirectory.MaximumLength = UserInfo->All.HomeDirectory.Length + sizeof(WCHAR);
    LocalBuffer->HomeDirectory.Buffer = (LPWSTR)((ULONG_PTR)ClientBaseAddress + (ULONG_PTR)Ptr - (ULONG_PTR)LocalBuffer);
    memcpy(Ptr,
           UserInfo->All.HomeDirectory.Buffer,
           UserInfo->All.HomeDirectory.Length);

    Ptr = (LPWSTR)((ULONG_PTR)Ptr + LocalBuffer->HomeDirectory.MaximumLength);

    LocalBuffer->FullName.Length = UserInfo->All.FullName.Length;
    LocalBuffer->FullName.MaximumLength = UserInfo->All.FullName.Length + sizeof(WCHAR);
    LocalBuffer->FullName.Buffer = (LPWSTR)((ULONG_PTR)ClientBaseAddress + (ULONG_PTR)Ptr - (ULONG_PTR)LocalBuffer);
    memcpy(Ptr,
           UserInfo->All.FullName.Buffer,
           UserInfo->All.FullName.Length);
    TRACE("FullName.Buffer: %p\n", LocalBuffer->FullName.Buffer);

    Ptr = (LPWSTR)((ULONG_PTR)Ptr + LocalBuffer->FullName.MaximumLength);

    LocalBuffer->ProfilePath.Length = UserInfo->All.ProfilePath.Length;
    LocalBuffer->ProfilePath.MaximumLength = UserInfo->All.ProfilePath.Length + sizeof(WCHAR);
    LocalBuffer->ProfilePath.Buffer = (LPWSTR)((ULONG_PTR)ClientBaseAddress + (ULONG_PTR)Ptr - (ULONG_PTR)LocalBuffer);
    memcpy(Ptr,
           UserInfo->All.ProfilePath.Buffer,
           UserInfo->All.ProfilePath.Length);

    Ptr = (LPWSTR)((ULONG_PTR)Ptr + LocalBuffer->ProfilePath.MaximumLength);

    LocalBuffer->HomeDirectoryDrive.Length = UserInfo->All.HomeDirectoryDrive.Length;
    LocalBuffer->HomeDirectoryDrive.MaximumLength = UserInfo->All.HomeDirectoryDrive.Length + sizeof(WCHAR);
    LocalBuffer->HomeDirectoryDrive.Buffer = (LPWSTR)((ULONG_PTR)ClientBaseAddress + (ULONG_PTR)Ptr - (ULONG_PTR)LocalBuffer);
    memcpy(Ptr,
           UserInfo->All.HomeDirectoryDrive.Buffer,
           UserInfo->All.HomeDirectoryDrive.Length);

    Ptr = (LPWSTR)((ULONG_PTR)Ptr + LocalBuffer->HomeDirectoryDrive.MaximumLength);

    LocalBuffer->LogonServer.Length = LogonServer->Length;
    LocalBuffer->LogonServer.MaximumLength = LogonServer->Length + sizeof(WCHAR);
    LocalBuffer->LogonServer.Buffer = (LPWSTR)((ULONG_PTR)ClientBaseAddress + (ULONG_PTR)Ptr - (ULONG_PTR)LocalBuffer);;
    memcpy(Ptr,
           LogonServer->Buffer,
           LogonServer->Length);

    LocalBuffer->UserFlags = 0;

    Status = DispatchTable.CopyToClientBuffer(ClientRequest,
                                              BufferLength,
                                              ClientBaseAddress,
                                              LocalBuffer);
    if (!NT_SUCCESS(Status))
    {
        TRACE("DispatchTable.CopyToClientBuffer failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    *ProfileBuffer = (PMSV1_0_INTERACTIVE_PROFILE)ClientBaseAddress;
    *ProfileBufferLength = BufferLength;

done:
    if (LocalBuffer != NULL)
        DispatchTable.FreeLsaHeap(LocalBuffer);

    if (!NT_SUCCESS(Status))
    {
        if (ClientBaseAddress != NULL)
            DispatchTable.FreeClientBuffer(ClientRequest,
                                           ClientBaseAddress);
    }

    return Status;
}


static
PSID
AppendRidToSid(PSID SrcSid,
               ULONG Rid)
{
    PSID DstSid = NULL;
    UCHAR RidCount;
    ULONG Size;
    ULONG i;

    RidCount = *RtlSubAuthorityCountSid(SrcSid);
    if (RidCount >= 8)
        return NULL;

    Size = RtlLengthRequiredSid(RidCount + 1);

    DstSid = DispatchTable.AllocateLsaHeap(Size);
    if (DstSid == NULL)
        return NULL;

    for (i = 0; i < RidCount; i++)
        *RtlSubAuthoritySid(DstSid, i) = *RtlSubAuthoritySid(SrcSid, i);

    *RtlSubAuthoritySid(DstSid, RidCount) = Rid;

    return DstSid;
}

static
NTSTATUS
BuildTokenUser(OUT PTOKEN_USER User,
               IN PSID AccountDomainSid,
               IN ULONG RelativeId)
{
    User->User.Sid = AppendRidToSid(AccountDomainSid,
                                    RelativeId);
    User->User.Attributes = 0;

    return STATUS_SUCCESS;
}


static
NTSTATUS
BuildTokenGroups(IN PSID AccountDomainSid,
                 IN PLUID LogonId,
                 OUT PTOKEN_GROUPS *Groups,
                 OUT PSID *PrimaryGroupSid,
                 OUT PSID *OwnerSid)
{
    SID_IDENTIFIER_AUTHORITY WorldAuthority = {SECURITY_WORLD_SID_AUTHORITY};
    SID_IDENTIFIER_AUTHORITY LocalAuthority = {SECURITY_LOCAL_SID_AUTHORITY};
    SID_IDENTIFIER_AUTHORITY SystemAuthority = {SECURITY_NT_AUTHORITY};
    PTOKEN_GROUPS TokenGroups;
#define MAX_GROUPS 8
    DWORD GroupCount = 0;
    PSID Sid;
    NTSTATUS Status = STATUS_SUCCESS;

    TokenGroups = DispatchTable.AllocateLsaHeap(sizeof(TOKEN_GROUPS) +
                                                MAX_GROUPS * sizeof(SID_AND_ATTRIBUTES));
    if (TokenGroups == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Sid = AppendRidToSid(AccountDomainSid, DOMAIN_GROUP_RID_USERS);
    if (Sid == NULL)
    {

    }

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
                                LogonId->HighPart,
                                LogonId->LowPart,
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

    *Groups = TokenGroups;

    return Status;
}


static
NTSTATUS
BuildTokenPrimaryGroup(PTOKEN_PRIMARY_GROUP PrimaryGroup,
                       PSID PrimaryGroupSid)
{
    ULONG RidCount;
    ULONG Size;

    RidCount = *RtlSubAuthorityCountSid(PrimaryGroupSid);
    Size = RtlLengthRequiredSid(RidCount);

    PrimaryGroup->PrimaryGroup = DispatchTable.AllocateLsaHeap(Size);
    if (PrimaryGroup->PrimaryGroup == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(PrimaryGroup->PrimaryGroup,
                  PrimaryGroupSid,
                  Size);

    return STATUS_SUCCESS;
}

static
NTSTATUS
BuildTokenPrivileges(PTOKEN_PRIVILEGES *TokenPrivileges)
{
    /* FIXME shouldn't use hard-coded list of privileges */
    static struct
    {
      LPCWSTR PrivName;
      DWORD Attributes;
    }
    DefaultPrivs[] =
    {
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
    PTOKEN_PRIVILEGES Privileges = NULL;
    ULONG i;
    RPC_UNICODE_STRING PrivilegeName;
    LSAPR_HANDLE PolicyHandle = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    Status = LsaIOpenPolicyTrusted(&PolicyHandle);
    if (!NT_SUCCESS(Status))
    {
        goto done;
    }

    /* Allocate and initialize token privileges */
    Privileges = DispatchTable.AllocateLsaHeap(sizeof(TOKEN_PRIVILEGES) +
                                               sizeof(DefaultPrivs) / sizeof(DefaultPrivs[0]) *
                                               sizeof(LUID_AND_ATTRIBUTES));
    if (Privileges == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    Privileges->PrivilegeCount = 0;
    for (i = 0; i < sizeof(DefaultPrivs) / sizeof(DefaultPrivs[0]); i++)
    {
        PrivilegeName.Length = wcslen(DefaultPrivs[i].PrivName) * sizeof(WCHAR);
        PrivilegeName.MaximumLength = PrivilegeName.Length + sizeof(WCHAR);
        PrivilegeName.Buffer = (LPWSTR)DefaultPrivs[i].PrivName;

        Status = LsarLookupPrivilegeValue(PolicyHandle,
                                          &PrivilegeName,
                                          &Privileges->Privileges[Privileges->PrivilegeCount].Luid);
        if (!NT_SUCCESS(Status))
        {
            WARN("Can't set privilege %S\n", DefaultPrivs[i].PrivName);
        }
        else
        {
            Privileges->Privileges[Privileges->PrivilegeCount].Attributes = DefaultPrivs[i].Attributes;
            Privileges->PrivilegeCount++;
        }
    }

    *TokenPrivileges = Privileges;

done:
    if (PolicyHandle != NULL)
        LsarClose(PolicyHandle);

    return Status;
}


static
NTSTATUS
BuildTokenOwner(PTOKEN_OWNER Owner,
                PSID OwnerSid)
{
    ULONG RidCount;
    ULONG Size;

    RidCount = *RtlSubAuthorityCountSid(OwnerSid);
    Size = RtlLengthRequiredSid(RidCount);

    Owner->Owner = DispatchTable.AllocateLsaHeap(Size);
    if (Owner->Owner == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(Owner->Owner,
                  OwnerSid,
                  Size);

    return STATUS_SUCCESS;
}


static
NTSTATUS
BuildTokenDefaultDacl(PTOKEN_DEFAULT_DACL DefaultDacl,
                      PSID OwnerSid)
{
    SID_IDENTIFIER_AUTHORITY SystemAuthority = {SECURITY_NT_AUTHORITY};
    PSID LocalSystemSid = NULL;
    PACL Dacl = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

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

    Dacl = DispatchTable.AllocateLsaHeap(1024);
    if (Dacl == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    Status = RtlCreateAcl(Dacl, 1024, ACL_REVISION);
    if (!NT_SUCCESS(Status))
        goto done;

    RtlAddAccessAllowedAce(Dacl,
                           ACL_REVISION,
                           GENERIC_ALL,
                           OwnerSid);

    /* SID: S-1-5-18 */
    RtlAddAccessAllowedAce(Dacl,
                           ACL_REVISION,
                           GENERIC_ALL,
                           LocalSystemSid);

    DefaultDacl->DefaultDacl = Dacl;

done:
    if (!NT_SUCCESS(Status))
    {
        if (Dacl != NULL)
            DispatchTable.FreeLsaHeap(Dacl);
    }

    if (LocalSystemSid != NULL)
        RtlFreeSid(LocalSystemSid);

    return Status;
}


static
NTSTATUS
BuildTokenInformationBuffer(PLSA_TOKEN_INFORMATION_V1 *TokenInformation,
                            PRPC_SID AccountDomainSid,
                            ULONG RelativeId,
                            PLUID LogonId)
{
    PLSA_TOKEN_INFORMATION_V1 Buffer = NULL;
    PSID OwnerSid = NULL;
    PSID PrimaryGroupSid = NULL;
    ULONG i;
    NTSTATUS Status = STATUS_SUCCESS;

    Buffer = DispatchTable.AllocateLsaHeap(sizeof(LSA_TOKEN_INFORMATION_V1));
    if (Buffer == NULL)
    {
        TRACE("Failed to allocate the local buffer!\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    /* FIXME: */
    Buffer->ExpirationTime.QuadPart = -1;

    Status = BuildTokenUser(&Buffer->User,
                            (PSID)AccountDomainSid,
                            RelativeId);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = BuildTokenGroups((PSID)AccountDomainSid,
                              LogonId,
                              &Buffer->Groups,
                              &PrimaryGroupSid,
                              &OwnerSid);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = BuildTokenPrimaryGroup(&Buffer->PrimaryGroup,
                                    PrimaryGroupSid);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = BuildTokenPrivileges(&Buffer->Privileges);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = BuildTokenOwner(&Buffer->Owner,
                             OwnerSid);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = BuildTokenDefaultDacl(&Buffer->DefaultDacl,
                                   OwnerSid);
    if (!NT_SUCCESS(Status))
        goto done;

    *TokenInformation = Buffer;

done:
    if (!NT_SUCCESS(Status))
    {
        if (Buffer != NULL)
        {
            if (Buffer->User.User.Sid != NULL)
                DispatchTable.FreeLsaHeap(Buffer->User.User.Sid);

            if (Buffer->Groups != NULL)
            {
                for (i = 0; i < Buffer->Groups->GroupCount; i++)
                {
                    if (Buffer->Groups->Groups[i].Sid != NULL)
                        DispatchTable.FreeLsaHeap(Buffer->Groups->Groups[i].Sid);
                }

                DispatchTable.FreeLsaHeap(Buffer->Groups);
            }

            if (Buffer->PrimaryGroup.PrimaryGroup != NULL)
                DispatchTable.FreeLsaHeap(Buffer->PrimaryGroup.PrimaryGroup);

            if (Buffer->Privileges != NULL)
                DispatchTable.FreeLsaHeap(Buffer->Privileges);

            if (Buffer->Owner.Owner != NULL)
                DispatchTable.FreeLsaHeap(Buffer->Owner.Owner);

            if (Buffer->DefaultDacl.DefaultDacl != NULL)
                DispatchTable.FreeLsaHeap(Buffer->DefaultDacl.DefaultDacl);

            DispatchTable.FreeLsaHeap(Buffer);
        }
    }

    return Status;
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
LsaApCallPackage(IN PLSA_CLIENT_REQUEST ClientRequest,
                 IN PVOID ProtocolSubmitBuffer,
                 IN PVOID ClientBufferBase,
                 IN ULONG SubmitBufferLength,
                 OUT PVOID *ProtocolReturnBuffer,
                 OUT PULONG ReturnBufferLength,
                 OUT PNTSTATUS ProtocolStatus)
{
    TRACE("()\n");
    return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
LsaApCallPackagePassthrough(IN PLSA_CLIENT_REQUEST ClientRequest,
                            IN PVOID ProtocolSubmitBuffer,
                            IN PVOID ClientBufferBase,
                            IN ULONG SubmitBufferLength,
                            OUT PVOID *ProtocolReturnBuffer,
                            OUT PULONG ReturnBufferLength,
                            OUT PNTSTATUS ProtocolStatus)
{
    TRACE("()\n");
    return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
LsaApCallPackageUntrusted(IN PLSA_CLIENT_REQUEST ClientRequest,
                          IN PVOID ProtocolSubmitBuffer,
                          IN PVOID ClientBufferBase,
                          IN ULONG SubmitBufferLength,
                          OUT PVOID *ProtocolReturnBuffer,
                          OUT PULONG ReturnBufferLength,
                          OUT PNTSTATUS ProtocolStatus)
{
    TRACE("()\n");
    return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
LsaApInitializePackage(IN ULONG AuthenticationPackageId,
                       IN PLSA_DISPATCH_TABLE LsaDispatchTable,
                       IN PLSA_STRING Database OPTIONAL,
                       IN PLSA_STRING Confidentiality OPTIONAL,
                       OUT PLSA_STRING *AuthenticationPackageName)
{
    PANSI_STRING NameString;
    PCHAR NameBuffer;

    TRACE("(%lu %p %p %p %p)\n",
          AuthenticationPackageId, LsaDispatchTable, Database,
          Confidentiality, AuthenticationPackageName);

    /* Get the dispatch table entries */
    DispatchTable.AllocateLsaHeap = LsaDispatchTable->AllocateLsaHeap;
    DispatchTable.FreeLsaHeap = LsaDispatchTable->FreeLsaHeap;
    DispatchTable.AllocateClientBuffer = LsaDispatchTable->AllocateClientBuffer;
    DispatchTable.FreeClientBuffer = LsaDispatchTable->FreeClientBuffer;
    DispatchTable.CopyToClientBuffer = LsaDispatchTable->CopyToClientBuffer;
    DispatchTable.CopyFromClientBuffer = LsaDispatchTable->CopyFromClientBuffer;


    /* Return the package name */
    NameString = DispatchTable.AllocateLsaHeap(sizeof(LSA_STRING));
    if (NameString == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    NameBuffer = DispatchTable.AllocateLsaHeap(sizeof(MSV1_0_PACKAGE_NAME));
    if (NameBuffer == NULL)
    {
        DispatchTable.FreeLsaHeap(NameString);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    strcpy(NameBuffer, MSV1_0_PACKAGE_NAME);

    RtlInitAnsiString(NameString, NameBuffer);

    *AuthenticationPackageName = (PLSA_STRING)NameString;

    return STATUS_SUCCESS;
}


/*
 * @unimplemented
 */
VOID
NTAPI
LsaApLogonTerminated(IN PLUID LogonId)
{
    TRACE("()\n");
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
LsaApLogonUser(IN PLSA_CLIENT_REQUEST ClientRequest,
               IN SECURITY_LOGON_TYPE LogonType,
               IN PVOID AuthenticationInformation,
               IN PVOID ClientAuthenticationBase,
               IN ULONG AuthenticationInformationLength,
               OUT PVOID *ProfileBuffer,
               OUT PULONG ProfileBufferLength,
               OUT PLUID LogonId,
               OUT PNTSTATUS SubStatus,
               OUT PLSA_TOKEN_INFORMATION_TYPE TokenInformationType,
               OUT PVOID *TokenInformation,
               OUT PLSA_UNICODE_STRING *AccountName,
               OUT PLSA_UNICODE_STRING *AuthenticatingAuthority)
{
    PMSV1_0_INTERACTIVE_LOGON LogonInfo;

    SAMPR_HANDLE ServerHandle = NULL;
    SAMPR_HANDLE DomainHandle = NULL;
    SAMPR_HANDLE UserHandle = NULL;
    PRPC_SID AccountDomainSid = NULL;
    RPC_UNICODE_STRING Names[1];
    SAMPR_ULONG_ARRAY RelativeIds = {0, NULL};
    SAMPR_ULONG_ARRAY Use = {0, NULL};
    PSAMPR_USER_INFO_BUFFER UserInfo = NULL;
    UNICODE_STRING LogonServer;
    NTSTATUS Status;

    TRACE("()\n");

    TRACE("LogonType: %lu\n", LogonType);
    TRACE("AuthenticationInformation: %p\n", AuthenticationInformation);
    TRACE("AuthenticationInformationLength: %lu\n", AuthenticationInformationLength);


    *ProfileBuffer = NULL;
    *ProfileBufferLength = 0;
    *SubStatus = STATUS_SUCCESS;

    if (LogonType == Interactive ||
        LogonType == Batch ||
        LogonType == Service)
    {
        ULONG_PTR PtrOffset;

        LogonInfo = (PMSV1_0_INTERACTIVE_LOGON)AuthenticationInformation;

        /* Fix-up pointers in the authentication info */
        PtrOffset = (ULONG_PTR)AuthenticationInformation - (ULONG_PTR)ClientAuthenticationBase;

        LogonInfo->LogonDomainName.Buffer = (PWSTR)((ULONG_PTR)LogonInfo->LogonDomainName.Buffer + PtrOffset);
        LogonInfo->UserName.Buffer = (PWSTR)((ULONG_PTR)LogonInfo->UserName.Buffer + PtrOffset);
        LogonInfo->Password.Buffer = (PWSTR)((ULONG_PTR)LogonInfo->Password.Buffer + PtrOffset);

        TRACE("Domain: %S\n", LogonInfo->LogonDomainName.Buffer);
        TRACE("User: %S\n", LogonInfo->UserName.Buffer);
        TRACE("Password: %S\n", LogonInfo->Password.Buffer);

        RtlInitUnicodeString(&LogonServer, L"Testserver");
    }
    else
    {
        FIXME("LogonType %lu is not supported yet!\n", LogonType);
        return STATUS_NOT_IMPLEMENTED;
    }

    Status = GetDomainSid(&AccountDomainSid);
    if (!NT_SUCCESS(Status))
    {
        TRACE("GetDomainSid() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    /* Connect to the SAM server */
    Status = SamIConnect(NULL,
                         &ServerHandle,
                         SAM_SERVER_CONNECT | SAM_SERVER_LOOKUP_DOMAIN,
                         TRUE);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamIConnect() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    /* Open the account domain */
    Status = SamrOpenDomain(ServerHandle,
                            DOMAIN_LOOKUP,
                            AccountDomainSid,
                            &DomainHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamrOpenDomain failed (Status %08lx)\n", Status);
        goto done;
    }

    Names[0].Length = LogonInfo->UserName.Length;
    Names[0].MaximumLength = LogonInfo->UserName.MaximumLength;
    Names[0].Buffer = LogonInfo->UserName.Buffer;

    /* Try to get the RID for the user name */
    Status = SamrLookupNamesInDomain(DomainHandle,
                                     1,
                                     Names,
                                     &RelativeIds,
                                     &Use);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamrLookupNamesInDomain failed (Status %08lx)\n", Status);
        Status = STATUS_NO_SUCH_USER;
        goto done;
    }

    /* Fail, if it is not a user account */
    if (Use.Element[0] != SidTypeUser)
    {
        TRACE("Account is not a user account!\n");
        Status = STATUS_NO_SUCH_USER;
        goto done;
    }

    /* Open the user object */
    Status = SamrOpenUser(DomainHandle,
                          USER_READ_GENERAL | USER_READ_LOGON |
                          USER_READ_ACCOUNT | USER_READ_PREFERENCES, /* FIXME */
                          RelativeIds.Element[0],
                          &UserHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamrOpenUser failed (Status %08lx)\n", Status);
        goto done;
    }

    Status = SamrQueryInformationUser(UserHandle,
                                      UserAllInformation,
                                      &UserInfo);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamrQueryInformationUser failed (Status %08lx)\n", Status);
        goto done;
    }


    TRACE("UserName: %S\n", UserInfo->All.UserName.Buffer);

    /* FIXME: Check restrictions */

    /* FIXME: Check the password */
    if ((UserInfo->All.UserAccountControl & USER_PASSWORD_NOT_REQUIRED) == 0)
    {
        FIXME("Must check the password!\n");

    }

    /* Return logon information */

    /* Create and return a new logon id */
    Status = NtAllocateLocallyUniqueId(LogonId);
    if (!NT_SUCCESS(Status))
    {
        TRACE("NtAllocateLocallyUniqueId failed (Status %08lx)\n", Status);
        goto done;
    }

    /* Build and fill the interactve profile buffer */
    Status = BuildInteractiveProfileBuffer(ClientRequest,
                                           UserInfo,
                                           &LogonServer,
                                           (PMSV1_0_INTERACTIVE_PROFILE*)ProfileBuffer,
                                           ProfileBufferLength);
    if (!NT_SUCCESS(Status))
    {
        TRACE("BuildInteractiveProfileBuffer failed (Status %08lx)\n", Status);
        goto done;
    }

    /* Return the token information type */
    *TokenInformationType = LsaTokenInformationV1;

    /* Build and fill the token information buffer */
    Status = BuildTokenInformationBuffer((PLSA_TOKEN_INFORMATION_V1*)TokenInformation,
                                         AccountDomainSid,
                                         RelativeIds.Element[0],
                                         LogonId);
    if (!NT_SUCCESS(Status))
    {
        TRACE("BuildTokenInformationBuffer failed (Status %08lx)\n", Status);
        goto done;
    }

    *SubStatus = STATUS_SUCCESS;

done:
    /* Return the account name */
    *AccountName = DispatchTable.AllocateLsaHeap(sizeof(UNICODE_STRING));
    if (*AccountName != NULL)
    {
        (*AccountName)->Buffer = DispatchTable.AllocateLsaHeap(LogonInfo->UserName.Length +
                                                               sizeof(UNICODE_NULL));
        if ((*AccountName)->Buffer != NULL)
        {
            (*AccountName)->MaximumLength = LogonInfo->UserName.Length +
                                            sizeof(UNICODE_NULL);
            RtlCopyUnicodeString(*AccountName, &LogonInfo->UserName);
        }
    }

    if (!NT_SUCCESS(Status))
    {
        if (*ProfileBuffer != NULL)
        {
            DispatchTable.FreeClientBuffer(ClientRequest,
                                           *ProfileBuffer);
            *ProfileBuffer = NULL;
        }
    }

    if (UserHandle != NULL)
        SamrCloseHandle(&UserHandle);

    SamIFree_SAMPR_USER_INFO_BUFFER(UserInfo,
                                    UserAllInformation);
    SamIFree_SAMPR_ULONG_ARRAY(&RelativeIds);
    SamIFree_SAMPR_ULONG_ARRAY(&Use);

    if (DomainHandle != NULL)
        SamrCloseHandle(&DomainHandle);

    if (ServerHandle != NULL)
        SamrCloseHandle(&ServerHandle);

    if (AccountDomainSid != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, AccountDomainSid);

    TRACE("LsaApLogonUser done (Status %08lx)\n", Status);

    return Status;
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
LsaApLogonUserEx(IN PLSA_CLIENT_REQUEST ClientRequest,
                 IN SECURITY_LOGON_TYPE LogonType,
                 IN PVOID AuthenticationInformation,
                 IN PVOID ClientAuthenticationBase,
                 IN ULONG AuthenticationInformationLength,
                 OUT PVOID *ProfileBuffer,
                 OUT PULONG ProfileBufferLength,
                 OUT PLUID LogonId,
                 OUT PNTSTATUS SubStatus,
                 OUT PLSA_TOKEN_INFORMATION_TYPE TokenInformationType,
                 OUT PVOID *TokenInformation,
                 OUT PUNICODE_STRING *AccountName,
                 OUT PUNICODE_STRING *AuthenticatingAuthority,
                 OUT PUNICODE_STRING *MachineName)
{
    TRACE("()\n");

    TRACE("LogonType: %lu\n", LogonType);
    TRACE("AuthenticationInformation: %p\n", AuthenticationInformation);
    TRACE("AuthenticationInformationLength: %lu\n", AuthenticationInformationLength);

    return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
LsaApLogonUserEx2(IN PLSA_CLIENT_REQUEST ClientRequest,
                  IN SECURITY_LOGON_TYPE LogonType,
                  IN PVOID ProtocolSubmitBuffer,
                  IN PVOID ClientBufferBase,
                  IN ULONG SubmitBufferSize,
                  OUT PVOID *ProfileBuffer,
                  OUT PULONG ProfileBufferSize,
                  OUT PLUID LogonId,
                  OUT PNTSTATUS SubStatus,
                  OUT PLSA_TOKEN_INFORMATION_TYPE TokenInformationType,
                  OUT PVOID *TokenInformation,
                  OUT PUNICODE_STRING *AccountName,
                  OUT PUNICODE_STRING *AuthenticatingAuthority,
                  OUT PUNICODE_STRING *MachineName,
                  OUT PSECPKG_PRIMARY_CRED PrimaryCredentials,
                  OUT PSECPKG_SUPPLEMENTAL_CRED_ARRAY *SupplementalCredentials)
{
    TRACE("()\n");

    TRACE("LogonType: %lu\n", LogonType);
    TRACE("ProtocolSubmitBuffer: %p\n", ProtocolSubmitBuffer);
    TRACE("SubmitBufferSize: %lu\n", SubmitBufferSize);


    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
