/*
 * PROJECT:     Authentication Package DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/msv1_0/msv1_0.c
 * PURPOSE:     Main file
 * COPYRIGHT:   Copyright 2013 Eric Kohl
 */

/* INCLUDES ****************************************************************/

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(msv1_0);

typedef struct _LOGON_LIST_ENTRY
{
    LIST_ENTRY ListEntry;
    LUID LogonId;
    ULONG EnumHandle;
    UNICODE_STRING UserName;
    UNICODE_STRING LogonDomainName;
    UNICODE_STRING LogonServer;
    SECURITY_LOGON_TYPE LogonType;
} LOGON_LIST_ENTRY, *PLOGON_LIST_ENTRY;

/* GLOBALS *****************************************************************/

BOOL PackageInitialized = FALSE;
LIST_ENTRY LogonListHead;
RTL_RESOURCE LogonListResource;
ULONG EnumCounter;

/* FUNCTIONS ***************************************************************/

static
PLOGON_LIST_ENTRY
GetLogonByLogonId(
    _In_ PLUID LogonId)
{
    PLOGON_LIST_ENTRY LogonEntry;
    PLIST_ENTRY CurrentEntry;

    CurrentEntry = LogonListHead.Flink;
    while (CurrentEntry != &LogonListHead)
    {
        LogonEntry = CONTAINING_RECORD(CurrentEntry,
                                       LOGON_LIST_ENTRY,
                                       ListEntry);

        if ((LogonEntry->LogonId.HighPart == LogonId->HighPart) &&
            (LogonEntry->LogonId.LowPart == LogonId->LowPart))
            return LogonEntry;

        CurrentEntry = CurrentEntry->Flink;
    }

    return NULL;
}


static
NTSTATUS
BuildInteractiveProfileBuffer(IN PLSA_CLIENT_REQUEST ClientRequest,
                              IN PSAMPR_USER_INFO_BUFFER UserInfo,
                              IN PWSTR ComputerName,
                              OUT PMSV1_0_INTERACTIVE_PROFILE *ProfileBuffer,
                              OUT PULONG ProfileBufferLength)
{
    PMSV1_0_INTERACTIVE_PROFILE LocalBuffer = NULL;
    PVOID ClientBaseAddress = NULL;
    LPWSTR Ptr;
    ULONG BufferLength;
    USHORT ComputerNameLength;
    NTSTATUS Status = STATUS_SUCCESS;

    *ProfileBuffer = NULL;
    *ProfileBufferLength = 0;

    if (UIntPtrToUShort(wcslen(ComputerName), &ComputerNameLength) != S_OK)
    {
        return STATUS_INVALID_PARAMETER;
    }

    BufferLength = sizeof(MSV1_0_INTERACTIVE_PROFILE) +
                   UserInfo->All.FullName.Length + sizeof(WCHAR) +
                   UserInfo->All.HomeDirectory.Length + sizeof(WCHAR) +
                   UserInfo->All.HomeDirectoryDrive.Length + sizeof(WCHAR) +
                   UserInfo->All.ScriptPath.Length + sizeof(WCHAR) +
                   UserInfo->All.ProfilePath.Length + sizeof(WCHAR) +
                   ((ComputerNameLength + 3) * sizeof(WCHAR));

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

    LocalBuffer->LogoffTime.LowPart = UserInfo->All.AccountExpires.LowPart;
    LocalBuffer->LogoffTime.HighPart = UserInfo->All.AccountExpires.HighPart;

    LocalBuffer->KickOffTime.LowPart = UserInfo->All.AccountExpires.LowPart;
    LocalBuffer->KickOffTime.HighPart = UserInfo->All.AccountExpires.HighPart;

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

    LocalBuffer->LogonServer.Length = (ComputerNameLength + 2) * sizeof(WCHAR);
    LocalBuffer->LogonServer.MaximumLength = LocalBuffer->LogonServer.Length + sizeof(WCHAR);
    LocalBuffer->LogonServer.Buffer = (LPWSTR)((ULONG_PTR)ClientBaseAddress + (ULONG_PTR)Ptr - (ULONG_PTR)LocalBuffer);
    wcscpy(Ptr, L"\\");
    wcscat(Ptr, ComputerName);

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
NTSTATUS
BuildLm20LogonProfileBuffer(
    _In_ PLSA_CLIENT_REQUEST ClientRequest,
    _In_ PSAMPR_USER_INFO_BUFFER UserInfo,
    _In_ PLSA_SAM_PWD_DATA LogonPwdData,
    _Out_ PMSV1_0_LM20_LOGON_PROFILE *ProfileBuffer,
    _Out_ PULONG ProfileBufferLength)
{
    PMSV1_0_LM20_LOGON_PROFILE LocalBuffer;
    NTLM_CLIENT_BUFFER Buffer;
    PBYTE PtrOffset;
    ULONG BufferLength;
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING ComputerNameUCS;

    *ProfileBuffer = NULL;
    *ProfileBufferLength = 0;

    if (!NtlmUStrAlloc(&ComputerNameUCS, LogonPwdData->ComputerName->Length + sizeof(WCHAR) * 3, 0))
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }
    Status = RtlAppendUnicodeToString(&ComputerNameUCS, L"\\\\");
    if (!NT_SUCCESS(Status))
    {
        ERR("RtlAppendUnicodeToString failed 0x%lx\n", Status);
        goto done;
    }
    Status = RtlAppendUnicodeStringToString(&ComputerNameUCS, LogonPwdData->ComputerName);
    if (!NT_SUCCESS(Status))
    {
        ERR("RtlAppendUnicodeStringToString failed 0x%lx\n", Status);
        goto done;
    }

    BufferLength = sizeof(MSV1_0_LM20_LOGON_PROFILE) + ComputerNameUCS.Length + sizeof(WCHAR);

    Status = NtlmAllocateClientBuffer(ClientRequest, BufferLength, &Buffer);
    if (!NT_SUCCESS(Status))
    {
        TRACE("DispatchTable.AllocateClientBuffer failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    TRACE("ClientBaseAddress: %p\n", Buffer.ClientBaseAddress);

    LocalBuffer = (PMSV1_0_LM20_LOGON_PROFILE)Buffer.LocalBuffer;
    PtrOffset = (PBYTE)(LocalBuffer + 1);

    LocalBuffer->MessageType = MsV1_0Lm20LogonProfile;
    LocalBuffer->KickOffTime.LowPart = UserInfo->All.AccountExpires.LowPart;
    LocalBuffer->KickOffTime.HighPart = UserInfo->All.AccountExpires.HighPart;
    LocalBuffer->LogoffTime.LowPart = UserInfo->All.AccountExpires.LowPart;
    LocalBuffer->LogoffTime.HighPart = UserInfo->All.AccountExpires.HighPart;

    memcpy(LocalBuffer->UserSessionKey,
           &LogonPwdData->UserSessionKey,
           MSV1_0_USER_SESSION_KEY_LENGTH);

    //FIXME: Set Domainname if we domain joined
    //       what to do if not? WORKGROUP
    RtlInitUnicodeString(&LocalBuffer->LogonDomainName, NULL);

    memcpy(LocalBuffer->LanmanSessionKey,
           &LogonPwdData->LanmanSessionKey,
           MSV1_0_LANMAN_SESSION_KEY_LENGTH);

    if (!NtlmUStrWriteToStruct(LocalBuffer,
                               BufferLength,
                               &LocalBuffer->LogonServer,
                               &ComputerNameUCS,
                               &PtrOffset,
                               TRUE))
    {
        ERR("NtlmStructWriteUCS failed.\n");
        Status = ERROR_INTERNAL_ERROR;
        goto done;
    }
    /* not supported */
    RtlInitUnicodeString(&LocalBuffer->UserParameters, NULL);
    /* Build user flags */
    LocalBuffer->UserFlags = 0x0;
    if (LogonPwdData->LogonType == NetLogonLmKey)
        LocalBuffer->UserFlags |= LOGON_USED_LM_PASSWORD;

    /* copy data to client buffer */
    Status = NtlmCopyToClientBuffer(ClientRequest, BufferLength, &Buffer);
    if (!NT_SUCCESS(Status))
    {
        TRACE("DispatchTable.CopyToClientBuffer failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    *ProfileBuffer = (PMSV1_0_LM20_LOGON_PROFILE)Buffer.ClientBaseAddress;
    *ProfileBufferLength = BufferLength;
done:
    /* On success Buffer.ClientBaseAddress will not be free */
    NtlmFreeClientBuffer(ClientRequest, !NT_SUCCESS(Status), &Buffer);
    NtlmUStrFree(&ComputerNameUCS);
    return Status;
}


static
PSID
AppendRidToSid(PSID SrcSid,
               ULONG Rid)
{
    PSID DstSid = NULL;
    UCHAR RidCount;

    RidCount = *RtlSubAuthorityCountSid(SrcSid);
    if (RidCount >= 8)
        return NULL;

    DstSid = DispatchTable.AllocateLsaHeap(RtlLengthRequiredSid(RidCount + 1));
    if (DstSid == NULL)
        return NULL;

    RtlCopyMemory(DstSid,
                  SrcSid,
                  RtlLengthRequiredSid(RidCount));

    *RtlSubAuthorityCountSid(DstSid) = RidCount + 1;
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
    if (User->User.Sid == NULL)
    {
        ERR("Could not create the user SID\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    User->User.Attributes = 0;

    return STATUS_SUCCESS;
}


static
NTSTATUS
BuildTokenPrimaryGroup(OUT PTOKEN_PRIMARY_GROUP PrimaryGroup,
                       IN PSID AccountDomainSid,
                       IN ULONG RelativeId)
{
    PrimaryGroup->PrimaryGroup = AppendRidToSid(AccountDomainSid,
                                                RelativeId);
    if (PrimaryGroup->PrimaryGroup == NULL)
    {
        ERR("Could not create the primary group SID\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}


static
NTSTATUS
BuildTokenGroups(OUT PTOKEN_GROUPS *Groups,
                 IN PSID AccountDomainSid,
                 IN ULONG RelativeId,
                 IN BOOL SpecialAccount)
{
    SID_IDENTIFIER_AUTHORITY SystemAuthority = {SECURITY_NT_AUTHORITY};
    PTOKEN_GROUPS TokenGroups;
    DWORD GroupCount = 0;
    DWORD MaxGroups = 2;
    PSID Sid;
    NTSTATUS Status = STATUS_SUCCESS;

    if (SpecialAccount)
        MaxGroups++;

    TokenGroups = DispatchTable.AllocateLsaHeap(sizeof(TOKEN_GROUPS) +
                                                MaxGroups * sizeof(SID_AND_ATTRIBUTES));
    if (TokenGroups == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (SpecialAccount)
    {
        /* Self */
        Sid = AppendRidToSid(AccountDomainSid, RelativeId);
        if (Sid == NULL)
        {

        }

        TokenGroups->Groups[GroupCount].Sid = Sid;
        TokenGroups->Groups[GroupCount].Attributes =
            SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY;
        GroupCount++;

        /* Member of 'Users' alias */
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
    }
    else
    {
        /* Member of the domains users group */
        Sid = AppendRidToSid(AccountDomainSid, DOMAIN_GROUP_RID_USERS);
        if (Sid == NULL)
        {

        }

        TokenGroups->Groups[GroupCount].Sid = Sid;
        TokenGroups->Groups[GroupCount].Attributes =
            SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY;
        GroupCount++;
    }

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
    ASSERT(TokenGroups->GroupCount <= MaxGroups);

    *Groups = TokenGroups;

    return Status;
}


static
NTSTATUS
BuildTokenInformationBuffer(PLSA_TOKEN_INFORMATION_V1 *TokenInformation,
                            PRPC_SID AccountDomainSid,
                            PSAMPR_USER_INFO_BUFFER UserInfo,
                            BOOL SpecialAccount)
{
    PLSA_TOKEN_INFORMATION_V1 Buffer = NULL;
    ULONG i;
    NTSTATUS Status = STATUS_SUCCESS;

    Buffer = DispatchTable.AllocateLsaHeap(sizeof(LSA_TOKEN_INFORMATION_V1));
    if (Buffer == NULL)
    {
        WARN("Failed to allocate the local buffer!\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    Buffer->ExpirationTime.LowPart = UserInfo->All.AccountExpires.LowPart;
    Buffer->ExpirationTime.HighPart = UserInfo->All.AccountExpires.HighPart;

    Status = BuildTokenUser(&Buffer->User,
                            (PSID)AccountDomainSid,
                            UserInfo->All.UserId);
    if (!NT_SUCCESS(Status))
    {
        WARN("BuildTokenUser() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    Status = BuildTokenPrimaryGroup(&Buffer->PrimaryGroup,
                                    (PSID)AccountDomainSid,
                                    UserInfo->All.PrimaryGroupId);
    if (!NT_SUCCESS(Status))
    {
        WARN("BuildTokenPrimaryGroup() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    Status = BuildTokenGroups(&Buffer->Groups,
                              (PSID)AccountDomainSid,
                              UserInfo->All.UserId,
                              SpecialAccount);
    if (!NT_SUCCESS(Status))
    {
        WARN("BuildTokenGroups() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

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

            if (Buffer->DefaultDacl.DefaultDacl != NULL)
                DispatchTable.FreeLsaHeap(Buffer->DefaultDacl.DefaultDacl);

            DispatchTable.FreeLsaHeap(Buffer);
        }
    }

    return Status;
}


static
NTSTATUS
MsvpChangePassword(IN PLSA_CLIENT_REQUEST ClientRequest,
                   IN PVOID ProtocolSubmitBuffer,
                   IN PVOID ClientBufferBase,
                   IN ULONG SubmitBufferLength,
                   OUT PVOID *ProtocolReturnBuffer,
                   OUT PULONG ReturnBufferLength,
                   OUT PNTSTATUS ProtocolStatus)
{
    NTSTATUS Status;
    PMSV1_0_CHANGEPASSWORD_REQUEST RequestBuffer;
    ULONG_PTR PtrOffset;

    SAMPR_HANDLE ServerHandle = NULL;
    SAMPR_HANDLE DomainHandle = NULL;
    SAMPR_HANDLE UserHandle = NULL;
    PRPC_SID DomainSid = NULL;
    RPC_UNICODE_STRING Names[1];
    SAMPR_ULONG_ARRAY RelativeIds = {0, NULL};
    SAMPR_ULONG_ARRAY Use = {0, NULL};

    ENCRYPTED_NT_OWF_PASSWORD OldNtPassword;
    ENCRYPTED_NT_OWF_PASSWORD NewNtPassword;
    ENCRYPTED_LM_OWF_PASSWORD OldLmPassword;
    ENCRYPTED_LM_OWF_PASSWORD NewLmPassword;
    OEM_STRING LmPwdString;
    CHAR LmPwdBuffer[15];
    BOOLEAN OldLmPasswordPresent = FALSE;
    BOOLEAN NewLmPasswordPresent = FALSE;

    ENCRYPTED_LM_OWF_PASSWORD OldLmEncryptedWithNewLm;
    ENCRYPTED_LM_OWF_PASSWORD NewLmEncryptedWithOldLm;
    ENCRYPTED_LM_OWF_PASSWORD OldNtEncryptedWithNewNt;
    ENCRYPTED_LM_OWF_PASSWORD NewNtEncryptedWithOldNt;
    PENCRYPTED_LM_OWF_PASSWORD pOldLmEncryptedWithNewLm = NULL;
    PENCRYPTED_LM_OWF_PASSWORD pNewLmEncryptedWithOldLm = NULL;

    TRACE("MsvpChangePassword()\n");

    /* Parameters validation */

    if (SubmitBufferLength < sizeof(MSV1_0_CHANGEPASSWORD_REQUEST))
    {
        ERR("Invalid SubmitBufferLength %lu\n", SubmitBufferLength);
        return STATUS_INVALID_PARAMETER;
    }

    RequestBuffer = (PMSV1_0_CHANGEPASSWORD_REQUEST)ProtocolSubmitBuffer;

    /* Fix-up pointers in the request buffer info */
    PtrOffset = (ULONG_PTR)ProtocolSubmitBuffer - (ULONG_PTR)ClientBufferBase;

    Status = RtlValidateUnicodeString(0, &RequestBuffer->DomainName);
    if (!NT_SUCCESS(Status))
        return STATUS_INVALID_PARAMETER;
    // TODO: Check for Buffer limits wrt. ClientBufferBase and alignment.
    RequestBuffer->DomainName.Buffer = FIXUP_POINTER(RequestBuffer->DomainName.Buffer, PtrOffset);
    RequestBuffer->DomainName.MaximumLength = RequestBuffer->DomainName.Length;

    Status = RtlValidateUnicodeString(0, &RequestBuffer->AccountName);
    if (!NT_SUCCESS(Status))
        return STATUS_INVALID_PARAMETER;
    // TODO: Check for Buffer limits wrt. ClientBufferBase and alignment.
    RequestBuffer->AccountName.Buffer = FIXUP_POINTER(RequestBuffer->AccountName.Buffer, PtrOffset);
    RequestBuffer->AccountName.MaximumLength = RequestBuffer->AccountName.Length;

    Status = RtlValidateUnicodeString(0, &RequestBuffer->OldPassword);
    if (!NT_SUCCESS(Status))
        return STATUS_INVALID_PARAMETER;
    // TODO: Check for Buffer limits wrt. ClientBufferBase and alignment.
    RequestBuffer->OldPassword.Buffer = FIXUP_POINTER(RequestBuffer->OldPassword.Buffer, PtrOffset);
    RequestBuffer->OldPassword.MaximumLength = RequestBuffer->OldPassword.Length;

    Status = RtlValidateUnicodeString(0, &RequestBuffer->NewPassword);
    if (!NT_SUCCESS(Status))
        return STATUS_INVALID_PARAMETER;
    // TODO: Check for Buffer limits wrt. ClientBufferBase and alignment.
    RequestBuffer->NewPassword.Buffer = FIXUP_POINTER(RequestBuffer->NewPassword.Buffer, PtrOffset);
    RequestBuffer->NewPassword.MaximumLength = RequestBuffer->NewPassword.Length;

    TRACE("Domain: %S\n", RequestBuffer->DomainName.Buffer);
    TRACE("Account: %S\n", RequestBuffer->AccountName.Buffer);
    TRACE("Old Password: %S\n", RequestBuffer->OldPassword.Buffer);
    TRACE("New Password: %S\n", RequestBuffer->NewPassword.Buffer);

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

    /* Get the domain SID */
    Status = SamrLookupDomainInSamServer(ServerHandle,
                                         (PRPC_UNICODE_STRING)&RequestBuffer->DomainName,
                                         &DomainSid);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamrLookupDomainInSamServer failed (Status %08lx)\n", Status);
        goto done;
    }

    /* Open the domain */
    Status = SamrOpenDomain(ServerHandle,
                            DOMAIN_LOOKUP,
                            DomainSid,
                            &DomainHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamrOpenDomain failed (Status %08lx)\n", Status);
        goto done;
    }

    Names[0].Length = RequestBuffer->AccountName.Length;
    Names[0].MaximumLength = RequestBuffer->AccountName.MaximumLength;
    Names[0].Buffer = RequestBuffer->AccountName.Buffer;

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
                          USER_CHANGE_PASSWORD,
                          RelativeIds.Element[0],
                          &UserHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamrOpenUser failed (Status %08lx)\n", Status);
        goto done;
    }


    /* Calculate the NT hash for the old password */
    Status = SystemFunction007(&RequestBuffer->OldPassword,
                               (LPBYTE)&OldNtPassword);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SystemFunction007 failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    /* Calculate the NT hash for the new password */
    Status = SystemFunction007(&RequestBuffer->NewPassword,
                               (LPBYTE)&NewNtPassword);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SystemFunction007 failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    /* Calculate the LM password and hash for the old password */
    LmPwdString.Length = 15;
    LmPwdString.MaximumLength = 15;
    LmPwdString.Buffer = LmPwdBuffer;
    ZeroMemory(LmPwdString.Buffer, LmPwdString.MaximumLength);

    Status = RtlUpcaseUnicodeStringToOemString(&LmPwdString,
                                               &RequestBuffer->OldPassword,
                                               FALSE);
    if (NT_SUCCESS(Status))
    {
        /* Calculate the LM hash value of the password */
        Status = SystemFunction006(LmPwdString.Buffer,
                                   (LPSTR)&OldLmPassword);
        if (NT_SUCCESS(Status))
        {
            OldLmPasswordPresent = TRUE;
        }
    }

    /* Calculate the LM password and hash for the new password */
    LmPwdString.Length = 15;
    LmPwdString.MaximumLength = 15;
    LmPwdString.Buffer = LmPwdBuffer;
    ZeroMemory(LmPwdString.Buffer, LmPwdString.MaximumLength);

    Status = RtlUpcaseUnicodeStringToOemString(&LmPwdString,
                                               &RequestBuffer->NewPassword,
                                               FALSE);
    if (NT_SUCCESS(Status))
    {
        /* Calculate the LM hash value of the password */
        Status = SystemFunction006(LmPwdString.Buffer,
                                   (LPSTR)&NewLmPassword);
        if (NT_SUCCESS(Status))
        {
            NewLmPasswordPresent = TRUE;
        }
    }

    /* Encrypt the old and new LM passwords, if they exist */
    if (OldLmPasswordPresent && NewLmPasswordPresent)
    {
        /* Encrypt the old LM password */
        Status = SystemFunction012((const BYTE *)&OldLmPassword,
                                   (const BYTE *)&NewLmPassword,
                                   (LPBYTE)&OldLmEncryptedWithNewLm);
        if (!NT_SUCCESS(Status))
        {
            TRACE("SystemFunction012 failed (Status 0x%08lx)\n", Status);
            goto done;
        }

        /* Encrypt the new LM password */
        Status = SystemFunction012((const BYTE *)&NewLmPassword,
                                   (const BYTE *)&OldLmPassword,
                                   (LPBYTE)&NewLmEncryptedWithOldLm);
        if (!NT_SUCCESS(Status))
        {
            TRACE("SystemFunction012 failed (Status 0x%08lx)\n", Status);
            goto done;
        }

        pOldLmEncryptedWithNewLm = &OldLmEncryptedWithNewLm;
        pNewLmEncryptedWithOldLm = &NewLmEncryptedWithOldLm;
    }

    /* Encrypt the old NT password */
    Status = SystemFunction012((const BYTE *)&OldNtPassword,
                               (const BYTE *)&NewNtPassword,
                               (LPBYTE)&OldNtEncryptedWithNewNt);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SystemFunction012 failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    /* Encrypt the new NT password */
    Status = SystemFunction012((const BYTE *)&NewNtPassword,
                               (const BYTE *)&OldNtPassword,
                               (LPBYTE)&NewNtEncryptedWithOldNt);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SystemFunction012 failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    /* Change the password */
    Status = SamrChangePasswordUser(UserHandle,
                                    OldLmPasswordPresent && NewLmPasswordPresent,
                                    pOldLmEncryptedWithNewLm,
                                    pNewLmEncryptedWithOldLm,
                                    TRUE,
                                    &OldNtEncryptedWithNewNt,
                                    &NewNtEncryptedWithOldNt,
                                    FALSE,
                                    NULL,
                                    FALSE,
                                    NULL);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SamrChangePasswordUser failed (Status %08lx)\n", Status);
        goto done;
    }

done:
    if (UserHandle != NULL)
        SamrCloseHandle(&UserHandle);

    SamIFree_SAMPR_ULONG_ARRAY(&RelativeIds);
    SamIFree_SAMPR_ULONG_ARRAY(&Use);

    if (DomainHandle != NULL)
        SamrCloseHandle(&DomainHandle);

    if (DomainSid != NULL)
        SamIFreeVoid(DomainSid);

    if (ServerHandle != NULL)
        SamrCloseHandle(&ServerHandle);

    return Status;
}


static
NTSTATUS
MsvpEnumerateUsers(
    _In_ PLSA_CLIENT_REQUEST ClientRequest,
    _In_ PVOID ProtocolSubmitBuffer,
    _In_ PVOID ClientBufferBase,
    _In_ ULONG SubmitBufferLength,
    _Out_ PVOID *ProtocolReturnBuffer,
    _Out_ PULONG ReturnBufferLength,
    _Out_ PNTSTATUS ProtocolStatus)
{
    PMSV1_0_ENUMUSERS_RESPONSE LocalBuffer = NULL;
    PVOID ClientBaseAddress = NULL;
    ULONG BufferLength;
    PLIST_ENTRY CurrentEntry;
    PLOGON_LIST_ENTRY LogonEntry;
    ULONG LogonCount = 0;
    PLUID LuidPtr;
    PULONG EnumPtr;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("MsvpEnumerateUsers()\n");

    if (SubmitBufferLength < sizeof(MSV1_0_ENUMUSERS_REQUEST))
    {
        ERR("Invalid SubmitBufferLength %lu\n", SubmitBufferLength);
        return STATUS_INVALID_PARAMETER;
    }

    RtlAcquireResourceShared(&LogonListResource, TRUE);

    /* Count the currently logged-on users */
    CurrentEntry = LogonListHead.Flink;
    while (CurrentEntry != &LogonListHead)
    {
        LogonEntry = CONTAINING_RECORD(CurrentEntry,
                                       LOGON_LIST_ENTRY,
                                       ListEntry);

        TRACE("Logon %lu: 0x%08lx\n", LogonCount, LogonEntry->LogonId.LowPart);
        LogonCount++;

        CurrentEntry = CurrentEntry->Flink;
    }

    TRACE("LogonCount %lu\n", LogonCount);

    BufferLength = sizeof(MSV1_0_ENUMUSERS_RESPONSE) + 
                   (LogonCount * sizeof(LUID)) + 
                   (LogonCount * sizeof(ULONG));

    LocalBuffer = DispatchTable.AllocateLsaHeap(BufferLength);
    if (LocalBuffer == NULL)
    {
        ERR("Failed to allocate the local buffer!\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    Status = DispatchTable.AllocateClientBuffer(ClientRequest,
                                                BufferLength,
                                                &ClientBaseAddress);
    if (!NT_SUCCESS(Status))
    {
        ERR("DispatchTable.AllocateClientBuffer failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    TRACE("ClientBaseAddress: %p\n", ClientBaseAddress);

    /* Fill the local buffer */
    LocalBuffer->MessageType = MsV1_0EnumerateUsers;
    LocalBuffer->NumberOfLoggedOnUsers = LogonCount;

    LuidPtr = (PLUID)((ULONG_PTR)LocalBuffer + sizeof(MSV1_0_ENUMUSERS_RESPONSE));
    EnumPtr = (PULONG)((ULONG_PTR)LuidPtr + LogonCount * sizeof(LUID));

    LocalBuffer->LogonIds = (PLUID)((ULONG_PTR)ClientBaseAddress + (ULONG_PTR)LuidPtr - (ULONG_PTR)LocalBuffer);
    LocalBuffer->EnumHandles = (PULONG)((ULONG_PTR)ClientBaseAddress + (ULONG_PTR)EnumPtr - (ULONG_PTR)LocalBuffer);

    /* Copy the LogonIds and EnumHandles into the local buffer */
    CurrentEntry = LogonListHead.Flink;
    while (CurrentEntry != &LogonListHead)
    {
        LogonEntry = CONTAINING_RECORD(CurrentEntry,
                                       LOGON_LIST_ENTRY,
                                       ListEntry);

        TRACE("Logon: 0x%08lx  %lu\n", LogonEntry->LogonId.LowPart, LogonEntry->EnumHandle);
        RtlCopyMemory(LuidPtr, &LogonEntry->LogonId, sizeof(LUID));
        LuidPtr++;

        *EnumPtr = LogonEntry->EnumHandle;
        EnumPtr++;

        CurrentEntry = CurrentEntry->Flink;
    }

    Status = DispatchTable.CopyToClientBuffer(ClientRequest,
                                              BufferLength,
                                              ClientBaseAddress,
                                              LocalBuffer);
    if (!NT_SUCCESS(Status))
    {
        ERR("DispatchTable.CopyToClientBuffer failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    *ProtocolReturnBuffer = ClientBaseAddress;
    *ReturnBufferLength = BufferLength;
    *ProtocolStatus = STATUS_SUCCESS;

done:
    RtlReleaseResource(&LogonListResource);

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
NTSTATUS
MsvpGetUserInfo(
    _In_ PLSA_CLIENT_REQUEST ClientRequest,
    _In_ PVOID ProtocolSubmitBuffer,
    _In_ PVOID ClientBufferBase,
    _In_ ULONG SubmitBufferLength,
    _Out_ PVOID *ProtocolReturnBuffer,
    _Out_ PULONG ReturnBufferLength,
    _Out_ PNTSTATUS ProtocolStatus)
{
    PMSV1_0_GETUSERINFO_REQUEST RequestBuffer;
    PLOGON_LIST_ENTRY LogonEntry;
    PMSV1_0_GETUSERINFO_RESPONSE LocalBuffer = NULL;
    PVOID ClientBaseAddress = NULL;
    ULONG BufferLength;
    PWSTR BufferPtr;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("MsvpGetUserInfo()\n");

    if (SubmitBufferLength < sizeof(MSV1_0_GETUSERINFO_REQUEST))
    {
        ERR("Invalid SubmitBufferLength %lu\n", SubmitBufferLength);
        return STATUS_INVALID_PARAMETER;
    }

    RequestBuffer = (PMSV1_0_GETUSERINFO_REQUEST)ProtocolSubmitBuffer;

    TRACE("LogonId: 0x%lx\n", RequestBuffer->LogonId.LowPart);

    RtlAcquireResourceShared(&LogonListResource, TRUE);

    LogonEntry = GetLogonByLogonId(&RequestBuffer->LogonId);
    if (LogonEntry == NULL)
    {
        ERR("No logon found for LogonId %lx\n", RequestBuffer->LogonId.LowPart);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    TRACE("UserName: %wZ\n", &LogonEntry->UserName);
    TRACE("LogonDomain: %wZ\n", &LogonEntry->LogonDomainName);
    TRACE("LogonServer: %wZ\n", &LogonEntry->LogonServer);

    BufferLength = sizeof(MSV1_0_GETUSERINFO_RESPONSE) + 
                   LogonEntry->UserName.MaximumLength +
                   LogonEntry->LogonDomainName.MaximumLength +
                   LogonEntry->LogonServer.MaximumLength;

    LocalBuffer = DispatchTable.AllocateLsaHeap(BufferLength);
    if (LocalBuffer == NULL)
    {
        ERR("Failed to allocate the local buffer!\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    Status = DispatchTable.AllocateClientBuffer(ClientRequest,
                                                BufferLength,
                                                &ClientBaseAddress);
    if (!NT_SUCCESS(Status))
    {
        ERR("DispatchTable.AllocateClientBuffer failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    TRACE("ClientBaseAddress: %p\n", ClientBaseAddress);

    /* Fill the local buffer */
    LocalBuffer->MessageType = MsV1_0GetUserInfo;

    BufferPtr = (PWSTR)((ULONG_PTR)LocalBuffer + sizeof(MSV1_0_GETUSERINFO_RESPONSE));

    /* UserName */
    LocalBuffer->UserName.Length = LogonEntry->UserName.Length;
    LocalBuffer->UserName.MaximumLength = LogonEntry->UserName.MaximumLength;
    LocalBuffer->UserName.Buffer = (PWSTR)((ULONG_PTR)ClientBaseAddress + (ULONG_PTR)BufferPtr - (ULONG_PTR)LocalBuffer);

    RtlCopyMemory(BufferPtr, LogonEntry->UserName.Buffer, LogonEntry->UserName.MaximumLength);
    BufferPtr = (PWSTR)((ULONG_PTR)BufferPtr + (ULONG_PTR)LocalBuffer->UserName.MaximumLength);

    /* LogonDomainName */
    LocalBuffer->LogonDomainName.Length = LogonEntry->LogonDomainName.Length;
    LocalBuffer->LogonDomainName.MaximumLength = LogonEntry->LogonDomainName.MaximumLength;
    LocalBuffer->LogonDomainName.Buffer = (PWSTR)((ULONG_PTR)ClientBaseAddress + (ULONG_PTR)BufferPtr - (ULONG_PTR)LocalBuffer);

    RtlCopyMemory(BufferPtr, LogonEntry->LogonDomainName.Buffer, LogonEntry->LogonDomainName.MaximumLength);
    BufferPtr = (PWSTR)((ULONG_PTR)BufferPtr + (ULONG_PTR)LocalBuffer->LogonDomainName.MaximumLength);

    /* LogonServer */
    LocalBuffer->LogonServer.Length = LogonEntry->LogonServer.Length;
    LocalBuffer->LogonServer.MaximumLength = LogonEntry->LogonServer.MaximumLength;
    LocalBuffer->LogonServer.Buffer = (PWSTR)((ULONG_PTR)ClientBaseAddress + (ULONG_PTR)BufferPtr - (ULONG_PTR)LocalBuffer);

    RtlCopyMemory(BufferPtr, LogonEntry->LogonServer.Buffer, LogonEntry->LogonServer.MaximumLength);

    /* Logon Type */
    LocalBuffer->LogonType = LogonEntry->LogonType;

    Status = DispatchTable.CopyToClientBuffer(ClientRequest,
                                              BufferLength,
                                              ClientBaseAddress,
                                              LocalBuffer);
    if (!NT_SUCCESS(Status))
    {
        ERR("DispatchTable.CopyToClientBuffer failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    *ProtocolReturnBuffer = ClientBaseAddress;
    *ReturnBufferLength = BufferLength;
    *ProtocolStatus = STATUS_SUCCESS;

done:
    RtlReleaseResource(&LogonListResource);

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
    NTSTATUS Status;
    MSV1_0_PROTOCOL_MESSAGE_TYPE MessageType;

    TRACE("LsaApCallPackage()\n");

    if (SubmitBufferLength < sizeof(MSV1_0_PROTOCOL_MESSAGE_TYPE))
        return STATUS_INVALID_PARAMETER;

    MessageType = *((PMSV1_0_PROTOCOL_MESSAGE_TYPE)ProtocolSubmitBuffer);

    *ProtocolReturnBuffer = NULL;
    *ReturnBufferLength = 0;

    switch (MessageType)
    {
        case MsV1_0Lm20ChallengeRequest:
        case MsV1_0Lm20GetChallengeResponse:
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case MsV1_0EnumerateUsers:
             Status = MsvpEnumerateUsers(ClientRequest,
                                         ProtocolSubmitBuffer,
                                         ClientBufferBase,
                                         SubmitBufferLength,
                                         ProtocolReturnBuffer,
                                         ReturnBufferLength,
                                         ProtocolStatus);
             break;

        case MsV1_0GetUserInfo:
             Status = MsvpGetUserInfo(ClientRequest,
                                      ProtocolSubmitBuffer,
                                      ClientBufferBase,
                                      SubmitBufferLength,
                                      ProtocolReturnBuffer,
                                      ReturnBufferLength,
                                      ProtocolStatus);
             break;

        case MsV1_0ReLogonUsers:
            Status = STATUS_INVALID_PARAMETER;
            break;

        case MsV1_0ChangePassword:
            Status = MsvpChangePassword(ClientRequest,
                                        ProtocolSubmitBuffer,
                                        ClientBufferBase,
                                        SubmitBufferLength,
                                        ProtocolReturnBuffer,
                                        ReturnBufferLength,
                                        ProtocolStatus);
            break;

        case MsV1_0ChangeCachedPassword:
        case MsV1_0GenericPassthrough:
        case MsV1_0CacheLogon:
        case MsV1_0SubAuth:
        case MsV1_0DeriveCredential:
        case MsV1_0CacheLookup:
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        default:
            return STATUS_INVALID_PARAMETER;
    }

    return Status;
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
    TRACE("LsaApCallPackagePassthrough()\n");
    return STATUS_NOT_IMPLEMENTED;
}


/*
 * @implemented
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
    ULONG MessageType;
    NTSTATUS Status;

    TRACE("LsaApCallPackageUntrusted()\n");

    if (SubmitBufferLength < sizeof(MSV1_0_PROTOCOL_MESSAGE_TYPE))
        return STATUS_INVALID_PARAMETER;

    MessageType = (ULONG)*((PMSV1_0_PROTOCOL_MESSAGE_TYPE)ProtocolSubmitBuffer);

    *ProtocolReturnBuffer = NULL;
    *ReturnBufferLength = 0;

    if (MessageType == MsV1_0ChangePassword)
        Status = MsvpChangePassword(ClientRequest,
                                    ProtocolSubmitBuffer,
                                    ClientBufferBase,
                                    SubmitBufferLength,
                                    ProtocolReturnBuffer,
                                    ReturnBufferLength,
                                    ProtocolStatus);
    else
        Status = STATUS_ACCESS_DENIED;

    return Status;
}


/*
 * @implemented
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

    TRACE("LsaApInitializePackage(%lu %p %p %p %p)\n",
          AuthenticationPackageId, LsaDispatchTable, Database,
          Confidentiality, AuthenticationPackageName);

    if (!PackageInitialized)
    {
        InitializeListHead(&LogonListHead);
        RtlInitializeResource(&LogonListResource);
        EnumCounter = 0;
        PackageInitialized = TRUE;
    }

    /* Get the dispatch table entries */
    DispatchTable.CreateLogonSession = LsaDispatchTable->CreateLogonSession;
    DispatchTable.DeleteLogonSession = LsaDispatchTable->DeleteLogonSession;
    DispatchTable.AddCredential = LsaDispatchTable->AddCredential;
    DispatchTable.GetCredentials = LsaDispatchTable->GetCredentials;
    DispatchTable.DeleteCredential = LsaDispatchTable->DeleteCredential;
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
LsaApLogonTerminated(
    _In_ PLUID LogonId)
{
    PLOGON_LIST_ENTRY LogonEntry;

    TRACE("LsaApLogonTerminated()\n");

    /* Remove the given logon entry from the list */
    LogonEntry = GetLogonByLogonId(LogonId);
    if (LogonEntry != NULL)
    {
        RtlAcquireResourceExclusive(&LogonListResource, TRUE);
        RemoveEntryList(&LogonEntry->ListEntry);
        RtlReleaseResource(&LogonListResource);

        if (LogonEntry->UserName.Buffer)
            RtlFreeHeap(RtlGetProcessHeap(), 0, LogonEntry->UserName.Buffer);

        if (LogonEntry->LogonDomainName.Buffer)
            RtlFreeHeap(RtlGetProcessHeap(), 0, LogonEntry->LogonDomainName.Buffer);

        if (LogonEntry->LogonServer.Buffer)
            RtlFreeHeap(RtlGetProcessHeap(), 0, LogonEntry->LogonServer.Buffer);

        RtlFreeHeap(RtlGetProcessHeap(), 0, LogonEntry);
    }
}


/*
 * Handle Network logon
 */
static
NTSTATUS
LsaApLogonUserEx2_Network(
    _In_ PLSA_CLIENT_REQUEST ClientRequest,
    _In_ PVOID ProtocolSubmitBuffer,
    _In_ PVOID ClientBufferBase,
    _In_ ULONG SubmitBufferSize,
    _In_ PUNICODE_STRING ComputerName,
    _Out_ PUNICODE_STRING* LogonUserRef,
    _Out_ PUNICODE_STRING* LogonDomainRef,
    _Inout_ PLSA_SAM_PWD_DATA LogonPwdData,
    _Out_ SAMPR_HANDLE* UserHandlePtr,
    _Out_ PSAMPR_USER_INFO_BUFFER* UserInfoPtr,
    _Out_ PRPC_SID* AccountDomainSidPtr,
    _Out_ PBOOL SpecialAccount,
    _Out_ PMSV1_0_LM20_LOGON_PROFILE *LogonProfile,
    _Out_ PULONG LogonProfileSize,
    _Out_ PNTSTATUS SubStatus)
{
    NTSTATUS Status;
    PMSV1_0_LM20_LOGON LogonInfo;
    ULONG_PTR PtrOffset;

    *LogonProfile = NULL;
    *LogonProfileSize = 0;
    *UserInfoPtr = NULL;
    *AccountDomainSidPtr = NULL;
    *SpecialAccount = FALSE;
    LogonInfo = ProtocolSubmitBuffer;

    if (SubmitBufferSize < sizeof(MSV1_0_LM20_LOGON))
    {
        ERR("Invalid SubmitBufferSize %lu\n", SubmitBufferSize);
        return STATUS_INVALID_PARAMETER;
    }

    /* Fix-up pointers in the authentication info */
    PtrOffset = (ULONG_PTR)ProtocolSubmitBuffer - (ULONG_PTR)ClientBufferBase;

    if ((!NtlmFixupAndValidateUStr(&LogonInfo->LogonDomainName, PtrOffset)) ||
        (!NtlmFixupAndValidateUStr(&LogonInfo->UserName, PtrOffset)) ||
        (!NtlmFixupAndValidateUStr(&LogonInfo->Workstation, PtrOffset)) ||
        (!NtlmFixupAStr(&LogonInfo->CaseSensitiveChallengeResponse, PtrOffset)) ||
        (!NtlmFixupAStr(&LogonInfo->CaseInsensitiveChallengeResponse, PtrOffset)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    LogonPwdData->IsNetwork = TRUE;
    LogonPwdData->LogonInfo = LogonInfo;
    LogonPwdData->ComputerName = ComputerName;
    Status = SamValidateUser(Network,
                             &LogonInfo->UserName,
                             &LogonInfo->LogonDomainName,
                             LogonPwdData,
                             ComputerName,
                             SpecialAccount,
                             AccountDomainSidPtr,
                             UserHandlePtr,
                             UserInfoPtr,
                             SubStatus);
    if (!NT_SUCCESS(Status))
    {
        ERR("SamValidateUser failed with 0x%lx\n", Status);
        return Status;
    }

    if (LogonInfo->ParameterControl & MSV1_0_RETURN_PROFILE_PATH)
    {
        Status = BuildLm20LogonProfileBuffer(ClientRequest,
                                             *UserInfoPtr,
                                             LogonPwdData,
                                             LogonProfile,
                                             LogonProfileSize);
        if (!NT_SUCCESS(Status))
        {
            ERR("BuildLm20LogonProfileBuffer failed with 0x%lx\n", Status);
            return Status;
        }
    }

    *LogonUserRef = &LogonInfo->UserName;
    *LogonDomainRef = &LogonInfo->LogonDomainName;

    return Status;
}

/*
 * @implemented
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
                  OUT PSECPKG_PRIMARY_CRED PrimaryCredentials, /* Not supported yet */
                  OUT PSECPKG_SUPPLEMENTAL_CRED_ARRAY *SupplementalCredentials) /* Not supported yet */
{
    NTSTATUS Status;
    UNICODE_STRING ComputerName;
    WCHAR ComputerNameData[MAX_COMPUTERNAME_LENGTH + 1];
    PUNICODE_STRING LogonUserName = NULL;
    LSA_SAM_PWD_DATA LogonPwdData = { FALSE, NULL };
    PUNICODE_STRING LogonDomain = NULL;
    SAMPR_HANDLE UserHandle = NULL;
    PRPC_SID AccountDomainSid = NULL;
    PSAMPR_USER_INFO_BUFFER UserInfo = NULL;
    BOOLEAN SessionCreated = FALSE;
    DWORD ComputerNameSize;
    BOOL SpecialAccount = FALSE;
    UCHAR LogonPassHash;
    PUNICODE_STRING ErasePassword = NULL;
    PLOGON_LIST_ENTRY LogonEntry = NULL;

    TRACE("LsaApLogonUserEx2()\n");

    TRACE("LogonType: %lu\n", LogonType);
    TRACE("ProtocolSubmitBuffer: %p\n", ProtocolSubmitBuffer);
    TRACE("SubmitBufferSize: %lu\n", SubmitBufferSize);

    *ProfileBuffer = NULL;
    *ProfileBufferSize = 0;
    *SubStatus = STATUS_SUCCESS;
    *AccountName = NULL;
    *AuthenticatingAuthority = NULL;

    /* Get the computer name */
    ComputerNameSize = ARRAYSIZE(ComputerNameData);
    if (!GetComputerNameW(ComputerNameData, &ComputerNameSize))
    {
        ERR("Failed to get Computername.\n");
        return STATUS_INTERNAL_ERROR;
    }
    RtlInitUnicodeString(&ComputerName, ComputerNameData);

    /* Parameters validation */
    if (LogonType == Interactive ||
        LogonType == Batch ||
        LogonType == Service)
    {
        PMSV1_0_INTERACTIVE_LOGON LogonInfo;
        ULONG_PTR PtrOffset;

        if (SubmitBufferSize < sizeof(MSV1_0_INTERACTIVE_LOGON))
        {
            ERR("Invalid SubmitBufferSize %lu\n", SubmitBufferSize);
            return STATUS_INVALID_PARAMETER;
        }

        LogonInfo = (PMSV1_0_INTERACTIVE_LOGON)ProtocolSubmitBuffer;

        if (LogonInfo->MessageType != MsV1_0InteractiveLogon &&
            LogonInfo->MessageType != MsV1_0WorkstationUnlockLogon)
        {
            ERR("Invalid MessageType %lu\n", LogonInfo->MessageType);
            return STATUS_BAD_VALIDATION_CLASS;
        }

#if 0   // FIXME: These checks happen to be done on Windows. We however keep them general on ReactOS for now...
        if (LogonInfo->UserName.Length > 512) // CRED_MAX_STRING_LENGTH * sizeof(WCHAR) or (CREDUI_MAX_USERNAME_LENGTH (== CRED_MAX_USERNAME_LENGTH) - 1) * sizeof(WCHAR)
        {
            ERR("UserName too long (%lu, maximum 512)\n", LogonInfo->UserName.Length);
            return STATUS_NAME_TOO_LONG;
        }
        if (LogonInfo->Password.Length > 512) // CREDUI_MAX_PASSWORD_LENGTH * sizeof(WCHAR)
        {
            ERR("Password too long (%lu, maximum 512)\n", LogonInfo->Password.Length);
            return STATUS_NAME_TOO_LONG;
        }
#endif

        /* Fix-up pointers in the authentication info */
        PtrOffset = (ULONG_PTR)ProtocolSubmitBuffer - (ULONG_PTR)ClientBufferBase;

        /* LogonDomainName is optional and can be an empty string */
        if (LogonInfo->LogonDomainName.Length)
        {
            // TODO: Check for Buffer limits wrt. ClientBufferBase and alignment.
            LogonInfo->LogonDomainName.Buffer = FIXUP_POINTER(LogonInfo->LogonDomainName.Buffer, PtrOffset);
            LogonInfo->LogonDomainName.MaximumLength = LogonInfo->LogonDomainName.Length;
        }
        else
        {
            LogonInfo->LogonDomainName.Buffer = NULL;
            LogonInfo->LogonDomainName.MaximumLength = 0;
        }
        Status = RtlValidateUnicodeString(0, &LogonInfo->LogonDomainName);
        if (!NT_SUCCESS(Status))
            return STATUS_INVALID_PARAMETER;

        /* UserName is mandatory and cannot be an empty string */
        // TODO: Check for Buffer limits wrt. ClientBufferBase and alignment.
        LogonInfo->UserName.Buffer = FIXUP_POINTER(LogonInfo->UserName.Buffer, PtrOffset);
        LogonInfo->UserName.MaximumLength = LogonInfo->UserName.Length;

        Status = RtlValidateUnicodeString(0, &LogonInfo->UserName);
        if (!NT_SUCCESS(Status))
            return STATUS_INVALID_PARAMETER;

        /* MS docs says max length is 0xFF bytes. But thats not the full story:
         *
         * A Quote from https://groups.google.com/forum/#!topic/microsoft.public.win32.programmer.kernel/eFGcCo_ZObk:
         * "... At least on my WinXP SP2. Domain and UserName are passed
         * in clear text, but the Password is NOT. ..."
         *
         * If the higher byte of length != 0 we have to use RtlRunDecodeUnicodeString.
         */
        LogonPassHash = (LogonInfo->Password.Length >> 8) & 0xFF;
        LogonInfo->Password.Length = LogonInfo->Password.Length & 0xFF;

        /* Password is optional and can be an empty string */
        if (LogonInfo->Password.Length)
        {
            // TODO: Check for Buffer limits wrt. ClientBufferBase and alignment.
            LogonInfo->Password.Buffer = FIXUP_POINTER(LogonInfo->Password.Buffer, PtrOffset);
            LogonInfo->Password.MaximumLength = LogonInfo->Password.Length;
        }
        else
        {
            LogonInfo->Password.Buffer = NULL;
            LogonInfo->Password.MaximumLength = 0;
        }

        /* Decode password */
        if (LogonPassHash > 0)
        {
            RtlRunDecodeUnicodeString(LogonPassHash, &LogonInfo->Password);
        }

        /* ErasePassword will be "erased" before we return */
        ErasePassword = &LogonInfo->Password;

        Status = RtlValidateUnicodeString(0, &LogonInfo->Password);
        if (!NT_SUCCESS(Status))
            return STATUS_INVALID_PARAMETER;

        LogonUserName = &LogonInfo->UserName;
        LogonDomain = &LogonInfo->LogonDomainName;
        LogonPwdData.IsNetwork = FALSE;
        LogonPwdData.PlainPwd = &LogonInfo->Password;
        LogonPwdData.ComputerName = &ComputerName;

        TRACE("Domain: %wZ\n", &LogonInfo->LogonDomainName);
        TRACE("User: %wZ\n", &LogonInfo->UserName);
        TRACE("Password: %wZ\n", &LogonInfo->Password);

        // TODO: If LogonType == Service, do some extra work using LogonInfo->Password.
    }
    else if (LogonType == Network)
    {
        Status = LsaApLogonUserEx2_Network(ClientRequest,
                                           ProtocolSubmitBuffer,
                                           ClientBufferBase,
                                           SubmitBufferSize,
                                           &ComputerName,
                                           &LogonUserName,
                                           &LogonDomain,
                                           &LogonPwdData,
                                           &UserHandle,
                                           &UserInfo,
                                           &AccountDomainSid,
                                           &SpecialAccount,
                                           (PMSV1_0_LM20_LOGON_PROFILE*)ProfileBuffer,
                                           ProfileBufferSize,
                                           SubStatus);
        if (!NT_SUCCESS(Status))
            goto done;
    }
    else
    {
        FIXME("LogonType %lu is not supported yet!\n", LogonType);
        return STATUS_NOT_IMPLEMENTED;
    }
    // TODO: Add other LogonType validity checks.

    Status = SamValidateUser(LogonType,
                             LogonUserName,
                             LogonDomain,
                             &LogonPwdData,
                             &ComputerName,
                             &SpecialAccount,
                             &AccountDomainSid,
                             &UserHandle,
                             &UserInfo,
                             SubStatus);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Return logon information */

    /* Create and return a new logon id */
    Status = NtAllocateLocallyUniqueId(LogonId);
    if (!NT_SUCCESS(Status))
    {
        TRACE("NtAllocateLocallyUniqueId failed (Status %08lx)\n", Status);
        goto done;
    }

    /* Create the logon session */
    Status = DispatchTable.CreateLogonSession(LogonId);
    if (!NT_SUCCESS(Status))
    {
        TRACE("CreateLogonSession failed (Status %08lx)\n", Status);
        goto done;
    }

    SessionCreated = TRUE;

    LogonEntry = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LOGON_LIST_ENTRY));
    if (LogonEntry)
    {
        RtlCopyMemory(&LogonEntry->LogonId, LogonId, sizeof(LUID));
        LogonEntry->EnumHandle = EnumCounter;
        EnumCounter++;

        TRACE("Logon User: %wZ %wZ %lx\n", LogonUserName, LogonDomain, LogonId->LowPart);
        LogonEntry->UserName.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, LogonUserName->MaximumLength);
        if (LogonEntry->UserName.Buffer)
        {
            LogonEntry->UserName.MaximumLength = LogonUserName->MaximumLength;
            RtlCopyUnicodeString(&LogonEntry->UserName, LogonUserName);
        }

        LogonEntry->LogonDomainName.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, LogonDomain->MaximumLength);
        if (LogonEntry->LogonDomainName.Buffer)
        {
            LogonEntry->LogonDomainName.MaximumLength = LogonDomain->MaximumLength;
            RtlCopyUnicodeString(&LogonEntry->LogonDomainName, LogonDomain);
        }

        LogonEntry->LogonServer.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, ComputerName.MaximumLength);
        if (LogonEntry->LogonServer.Buffer)
        {
            LogonEntry->LogonServer.MaximumLength = ComputerName.MaximumLength;
            RtlCopyUnicodeString(&LogonEntry->LogonServer, &ComputerName);
        }

        LogonEntry->LogonType = LogonType;

        RtlAcquireResourceExclusive(&LogonListResource, TRUE);
        InsertTailList(&LogonListHead, &LogonEntry->ListEntry);
        RtlReleaseResource(&LogonListResource);
    }

    if (LogonType == Interactive || LogonType == Batch || LogonType == Service)
    {
        /* Build and fill the interactive profile buffer */
        Status = BuildInteractiveProfileBuffer(ClientRequest,
                                               UserInfo,
                                               ComputerName.Buffer,
                                               (PMSV1_0_INTERACTIVE_PROFILE*)ProfileBuffer,
                                               ProfileBufferSize);
        if (!NT_SUCCESS(Status))
        {
            TRACE("BuildInteractiveProfileBuffer failed (Status %08lx)\n", Status);
            goto done;
        }
    }
    else if (LogonType == Network)
    {
        //FIXME: no need to do anything, its already done ...
    }

    /* Return the token information type */
    *TokenInformationType = LsaTokenInformationV1;

    /* Build and fill the token information buffer */
    Status = BuildTokenInformationBuffer((PLSA_TOKEN_INFORMATION_V1*)TokenInformation,
                                         AccountDomainSid,
                                         UserInfo,
                                         SpecialAccount);
    if (!NT_SUCCESS(Status))
    {
        TRACE("BuildTokenInformationBuffer failed (Status %08lx)\n", Status);
        goto done;
    }

done:
    /* Erase password */
    if (ErasePassword)
    {
        RtlEraseUnicodeString(ErasePassword);
    }

    /* Update the logon time/count or the bad password time/count */
    if ((UserHandle != NULL) &&
        (Status == STATUS_SUCCESS || Status == STATUS_WRONG_PASSWORD))
    {
        SAMPR_USER_INFO_BUFFER InternalInfo;

        RtlZeroMemory(&InternalInfo, sizeof(InternalInfo));

        if (Status == STATUS_SUCCESS)
            InternalInfo.Internal2.Flags = USER_LOGON_SUCCESS;
        else
            InternalInfo.Internal2.Flags = USER_LOGON_BAD_PASSWORD;

        SamrSetInformationUser(UserHandle,
                               UserInternal2Information,
                               &InternalInfo);
    }

    if (NT_SUCCESS(Status))
    {
        /* Return the account name */
        *AccountName = DispatchTable.AllocateLsaHeap(sizeof(UNICODE_STRING));
        if ((LogonUserName != NULL) &&
            (*AccountName != NULL))
        {
            (*AccountName)->Buffer = DispatchTable.AllocateLsaHeap(LogonUserName->Length +
                                                                   sizeof(UNICODE_NULL));
            if ((*AccountName)->Buffer != NULL)
            {
                (*AccountName)->MaximumLength = LogonUserName->Length +
                                                sizeof(UNICODE_NULL);
                RtlCopyUnicodeString(*AccountName, LogonUserName);
            }
        }

        /* Return the authenticating authority */
        *AuthenticatingAuthority = DispatchTable.AllocateLsaHeap(sizeof(UNICODE_STRING));
        if ((LogonDomain != NULL) &&
            (*AuthenticatingAuthority != NULL))
        {
            (*AuthenticatingAuthority)->Buffer = DispatchTable.AllocateLsaHeap(LogonDomain->Length +
                                                                               sizeof(UNICODE_NULL));
            if ((*AuthenticatingAuthority)->Buffer != NULL)
            {
                (*AuthenticatingAuthority)->MaximumLength = LogonDomain->Length +
                                                            sizeof(UNICODE_NULL);
                RtlCopyUnicodeString(*AuthenticatingAuthority, LogonDomain);
            }
        }

        /* Return the machine name */
        *MachineName = DispatchTable.AllocateLsaHeap(sizeof(UNICODE_STRING));
        if (*MachineName != NULL)
        {
            (*MachineName)->Buffer = DispatchTable.AllocateLsaHeap(ComputerName.MaximumLength);
            if ((*MachineName)->Buffer != NULL)
            {
                (*MachineName)->MaximumLength = ComputerName.MaximumLength;
                (*MachineName)->Length = ComputerName.Length;
                RtlCopyMemory((*MachineName)->Buffer,
                              ComputerName.Buffer,
                              ComputerName.MaximumLength);
            }
        }
    }

    if (!NT_SUCCESS(Status))
    {
        if (SessionCreated != FALSE)
            DispatchTable.DeleteLogonSession(LogonId);

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

    if (AccountDomainSid != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, AccountDomainSid);

    if (Status == STATUS_NO_SUCH_USER ||
        Status == STATUS_WRONG_PASSWORD)
    {
        *SubStatus = Status;
        Status = STATUS_LOGON_FAILURE;
    }

    TRACE("LsaApLogonUserEx2 done (Status 0x%08lx, SubStatus 0x%08lx)\n", Status, *SubStatus);

    return Status;
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
SpLsaModeInitialize(
    _In_ ULONG LsaVersion,
    _Out_ PULONG PackageVersion,
    _Out_ PSECPKG_FUNCTION_TABLE *ppTables,
    _Out_ PULONG pcTables)
{
    TRACE("SpLsaModeInitialize(0x%lx %p %p %p)\n",
          LsaVersion, PackageVersion, ppTables, pcTables);

    if (LsaVersion != SECPKG_INTERFACE_VERSION)
        return STATUS_INVALID_PARAMETER;

    *PackageVersion = SECPKG_INTERFACE_VERSION;

    *ppTables = NtlmLsaFn;
    *pcTables = 1;

    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS
WINAPI
SpUserModeInitialize(
    _In_ ULONG LsaVersion,
    _Out_ PULONG PackageVersion,
    _Out_ PSECPKG_USER_FUNCTION_TABLE *ppTables,
    _Out_ PULONG pcTables)
{
    TRACE("SpUserModeInitialize(0x%lx %p %p %p)\n",
          LsaVersion, PackageVersion, ppTables, pcTables);

    if (LsaVersion != SECPKG_INTERFACE_VERSION)
        return STATUS_INVALID_PARAMETER;

    *PackageVersion = SECPKG_INTERFACE_VERSION;

    *ppTables = NtlmUsrFn;
    *pcTables = 1;

    return STATUS_SUCCESS;
}

/* EOF */
