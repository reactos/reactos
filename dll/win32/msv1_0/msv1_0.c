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
GetAccountDomainSid(PRPC_SID *Sid)
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
GetNtAuthorityDomainSid(PRPC_SID *Sid)
{
    SID_IDENTIFIER_AUTHORITY NtAuthority = {SECURITY_NT_AUTHORITY};
    ULONG Length = 0;

    Length = RtlLengthRequiredSid(0);
    *Sid = RtlAllocateHeap(RtlGetProcessHeap(), 0, Length);
    if (*Sid == NULL)
    {
        ERR("Failed to allocate SID\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlInitializeSid(*Sid,&NtAuthority, 0);

    return STATUS_SUCCESS;
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
    NTSTATUS Status = STATUS_SUCCESS;

    *ProfileBuffer = NULL;
    *ProfileBufferLength = 0;

    BufferLength = sizeof(MSV1_0_INTERACTIVE_PROFILE) +
                   UserInfo->All.FullName.Length + sizeof(WCHAR) +
                   UserInfo->All.HomeDirectory.Length + sizeof(WCHAR) +
                   UserInfo->All.HomeDirectoryDrive.Length + sizeof(WCHAR) +
                   UserInfo->All.ScriptPath.Length + sizeof(WCHAR) +
                   UserInfo->All.ProfilePath.Length + sizeof(WCHAR) +
                   ((wcslen(ComputerName) + 3) * sizeof(WCHAR));

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

    LocalBuffer->LogonServer.Length = (wcslen(ComputerName) + 2) * sizeof(WCHAR);
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
MsvpCheckPassword(PUNICODE_STRING UserPassword,
                  PSAMPR_USER_INFO_BUFFER UserInfo)
{
    ENCRYPTED_NT_OWF_PASSWORD UserNtPassword;
    ENCRYPTED_LM_OWF_PASSWORD UserLmPassword;
    BOOLEAN UserLmPasswordPresent = FALSE;
    BOOLEAN UserNtPasswordPresent = FALSE;
    OEM_STRING LmPwdString;
    CHAR LmPwdBuffer[15];
    NTSTATUS Status;

    TRACE("(%p %p)\n", UserPassword, UserInfo);

    /* Calculate the LM password and hash for the users password */
    LmPwdString.Length = 15;
    LmPwdString.MaximumLength = 15;
    LmPwdString.Buffer = LmPwdBuffer;
    ZeroMemory(LmPwdString.Buffer, LmPwdString.MaximumLength);

    Status = RtlUpcaseUnicodeStringToOemString(&LmPwdString,
                                               UserPassword,
                                               FALSE);
    if (NT_SUCCESS(Status))
    {
        /* Calculate the LM hash value of the users password */
        Status = SystemFunction006(LmPwdString.Buffer,
                                   (LPSTR)&UserLmPassword);
        if (NT_SUCCESS(Status))
        {
            UserLmPasswordPresent = TRUE;
        }
    }

    /* Calculate the NT hash of the users password */
    Status = SystemFunction007(UserPassword,
                               (LPBYTE)&UserNtPassword);
    if (NT_SUCCESS(Status))
    {
        UserNtPasswordPresent = TRUE;
    }

    Status = STATUS_WRONG_PASSWORD;

    /* Succeed, if no password has been set */
    if (UserInfo->All.NtPasswordPresent == FALSE &&
        UserInfo->All.LmPasswordPresent == FALSE)
    {
        TRACE("No password check!\n");
        Status = STATUS_SUCCESS;
        goto done;
    }

    /* Succeed, if NT password matches */
    if (UserNtPasswordPresent && UserInfo->All.NtPasswordPresent)
    {
        TRACE("Check NT password hashes:\n");
        if (RtlEqualMemory(&UserNtPassword,
                           UserInfo->All.NtOwfPassword.Buffer,
                           sizeof(ENCRYPTED_NT_OWF_PASSWORD)))
        {
            TRACE("  success!\n");
            Status = STATUS_SUCCESS;
            goto done;
        }

        TRACE("  failed!\n");
    }

    /* Succeed, if LM password matches */
    if (UserLmPasswordPresent && UserInfo->All.LmPasswordPresent)
    {
        TRACE("Check LM password hashes:\n");
        if (RtlEqualMemory(&UserLmPassword,
                           UserInfo->All.LmOwfPassword.Buffer,
                           sizeof(ENCRYPTED_LM_OWF_PASSWORD)))
        {
            TRACE("  success!\n");
            Status = STATUS_SUCCESS;
            goto done;
        }
        TRACE("  failed!\n");
    }

done:
    return Status;
}


static
BOOL
MsvpCheckLogonHours(
    _In_ PSAMPR_LOGON_HOURS LogonHours,
    _In_ PLARGE_INTEGER LogonTime)
{
#if 0
    LARGE_INTEGER LocalLogonTime;
    TIME_FIELDS TimeFields;
    USHORT MinutesPerUnit, Offset;
    BOOL bFound;

    FIXME("MsvpCheckLogonHours(%p %p)\n", LogonHours, LogonTime);

    if (LogonHours->UnitsPerWeek == 0 || LogonHours->LogonHours == NULL)
    {
        FIXME("No logon hours!\n");
        return TRUE;
    }

    RtlSystemTimeToLocalTime(LogonTime, &LocalLogonTime);
    RtlTimeToTimeFields(&LocalLogonTime, &TimeFields);

    FIXME("UnitsPerWeek: %u\n", LogonHours->UnitsPerWeek);
    MinutesPerUnit = 10080 / LogonHours->UnitsPerWeek;

    Offset = ((TimeFields.Weekday * 24 + TimeFields.Hour) * 60 + TimeFields.Minute) / MinutesPerUnit;
    FIXME("Offset: %us\n", Offset);

    bFound = (BOOL)(LogonHours->LogonHours[Offset / 8] & (1 << (Offset % 8)));
    FIXME("Logon permitted: %s\n", bFound ? "Yes" : "No");

    return bFound;
#endif
    return TRUE;
}


static
BOOL
MsvpCheckWorkstations(
    _In_ PRPC_UNICODE_STRING WorkStations,
    _In_ PWSTR ComputerName)
{
    PWSTR pStart, pEnd;
    BOOL bFound = FALSE;

    TRACE("MsvpCheckWorkstations(%p %S)\n", WorkStations, ComputerName);

    if (WorkStations->Length == 0 || WorkStations->Buffer == NULL)
    {
        TRACE("No workstations!\n");
        return TRUE;
    }

    TRACE("Workstations: %wZ\n", WorkStations);

    pStart = WorkStations->Buffer;
    for (;;)
    {
        pEnd = wcschr(pStart, L',');
        if (pEnd != NULL)
            *pEnd = UNICODE_NULL;

        TRACE("Comparing '%S' and '%S'\n", ComputerName, pStart);
        if (_wcsicmp(ComputerName, pStart) == 0)
        {
            bFound = TRUE;
            if (pEnd != NULL)
                *pEnd = L',';
            break;
        }

        if (pEnd == NULL)
            break;

        *pEnd = L',';
        pStart = pEnd + 1;
    }

    TRACE("Found allowed workstation: %s\n", (bFound) ? "Yes" : "No");

    return bFound;
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
        case MsV1_0GetUserInfo:
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
LsaApLogonTerminated(IN PLUID LogonId)
{
    TRACE("LsaApLogonTerminated()\n");
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
    static const UNICODE_STRING NtAuthorityU = RTL_CONSTANT_STRING(L"NT AUTHORITY");
    static const UNICODE_STRING LocalServiceU = RTL_CONSTANT_STRING(L"LocalService");
    static const UNICODE_STRING NetworkServiceU = RTL_CONSTANT_STRING(L"NetworkService");

    NTSTATUS Status;
    PMSV1_0_INTERACTIVE_LOGON LogonInfo;
    WCHAR ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    SAMPR_HANDLE ServerHandle = NULL;
    SAMPR_HANDLE DomainHandle = NULL;
    SAMPR_HANDLE UserHandle = NULL;
    PRPC_SID AccountDomainSid = NULL;
    RPC_UNICODE_STRING Names[1];
    SAMPR_ULONG_ARRAY RelativeIds = {0, NULL};
    SAMPR_ULONG_ARRAY Use = {0, NULL};
    PSAMPR_USER_INFO_BUFFER UserInfo = NULL;
    BOOLEAN SessionCreated = FALSE;
    LARGE_INTEGER LogonTime;
    LARGE_INTEGER AccountExpires;
    LARGE_INTEGER PasswordMustChange;
    LARGE_INTEGER PasswordLastSet;
    DWORD ComputerNameSize;
    BOOL SpecialAccount = FALSE;

    TRACE("LsaApLogonUserEx2()\n");

    TRACE("LogonType: %lu\n", LogonType);
    TRACE("ProtocolSubmitBuffer: %p\n", ProtocolSubmitBuffer);
    TRACE("SubmitBufferSize: %lu\n", SubmitBufferSize);

    *ProfileBuffer = NULL;
    *ProfileBufferSize = 0;
    *SubStatus = STATUS_SUCCESS;
    *AccountName = NULL;
    *AuthenticatingAuthority = NULL;

    /* Parameters validation */
    if (LogonType == Interactive ||
        LogonType == Batch ||
        LogonType == Service)
    {
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

        Status = RtlValidateUnicodeString(0, &LogonInfo->LogonDomainName);
        if (!NT_SUCCESS(Status))
            return STATUS_INVALID_PARAMETER;
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

        Status = RtlValidateUnicodeString(0, &LogonInfo->UserName);
        if (!NT_SUCCESS(Status))
            return STATUS_INVALID_PARAMETER;
        /* UserName is mandatory and cannot be an empty string */
        // TODO: Check for Buffer limits wrt. ClientBufferBase and alignment.
        LogonInfo->UserName.Buffer = FIXUP_POINTER(LogonInfo->UserName.Buffer, PtrOffset);
        LogonInfo->UserName.MaximumLength = LogonInfo->UserName.Length;

        Status = RtlValidateUnicodeString(0, &LogonInfo->Password);
        if (!NT_SUCCESS(Status))
            return STATUS_INVALID_PARAMETER;
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

        TRACE("Domain: %S\n", LogonInfo->LogonDomainName.Buffer);
        TRACE("User: %S\n", LogonInfo->UserName.Buffer);
        TRACE("Password: %S\n", LogonInfo->Password.Buffer);

        // TODO: If LogonType == Service, do some extra work using LogonInfo->Password.
    }
    else
    {
        FIXME("LogonType %lu is not supported yet!\n", LogonType);
        return STATUS_NOT_IMPLEMENTED;
    }
    // TODO: Add other LogonType validity checks.

    /* Get the logon time */
    NtQuerySystemTime(&LogonTime);

    /* Get the computer name */
    ComputerNameSize = ARRAYSIZE(ComputerName);
    GetComputerNameW(ComputerName, &ComputerNameSize);

    /* Check for special accounts */
    // FIXME: Windows does not do this that way!! (msv1_0 does not contain these hardcoded values)
    if (RtlEqualUnicodeString(&LogonInfo->LogonDomainName, &NtAuthorityU, TRUE))
    {
        SpecialAccount = TRUE;

        /* Get the authority domain SID */
        Status = GetNtAuthorityDomainSid(&AccountDomainSid);
        if (!NT_SUCCESS(Status))
        {
            ERR("GetNtAuthorityDomainSid() failed (Status 0x%08lx)\n", Status);
            return Status;
        }

        if (RtlEqualUnicodeString(&LogonInfo->UserName, &LocalServiceU, TRUE))
        {
            TRACE("SpecialAccount: LocalService\n");

            if (LogonType != Service)
                return STATUS_LOGON_FAILURE;

            UserInfo = RtlAllocateHeap(RtlGetProcessHeap(),
                                       HEAP_ZERO_MEMORY,
                                       sizeof(SAMPR_USER_ALL_INFORMATION));
            if (UserInfo == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto done;
            }

            UserInfo->All.UserId = SECURITY_LOCAL_SERVICE_RID;
            UserInfo->All.PrimaryGroupId = SECURITY_LOCAL_SERVICE_RID;
        }
        else if (RtlEqualUnicodeString(&LogonInfo->UserName, &NetworkServiceU, TRUE))
        {
            TRACE("SpecialAccount: NetworkService\n");

            if (LogonType != Service)
                return STATUS_LOGON_FAILURE;

            UserInfo = RtlAllocateHeap(RtlGetProcessHeap(),
                                       HEAP_ZERO_MEMORY,
                                       sizeof(SAMPR_USER_ALL_INFORMATION));
            if (UserInfo == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto done;
            }

            UserInfo->All.UserId = SECURITY_NETWORK_SERVICE_RID;
            UserInfo->All.PrimaryGroupId = SECURITY_NETWORK_SERVICE_RID;
        }
        else
        {
            Status = STATUS_NO_SUCH_USER;
            goto done;
        }
    }
    else
    {
        TRACE("NormalAccount\n");

        /* Get the account domain SID */
        Status = GetAccountDomainSid(&AccountDomainSid);
        if (!NT_SUCCESS(Status))
        {
            ERR("GetAccountDomainSid() failed (Status 0x%08lx)\n", Status);
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
            ERR("SamrOpenDomain failed (Status %08lx)\n", Status);
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
            ERR("SamrLookupNamesInDomain failed (Status %08lx)\n", Status);
            Status = STATUS_NO_SUCH_USER;
            goto done;
        }

        /* Fail, if it is not a user account */
        if (Use.Element[0] != SidTypeUser)
        {
            ERR("Account is not a user account!\n");
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
            ERR("SamrOpenUser failed (Status %08lx)\n", Status);
            goto done;
        }

        Status = SamrQueryInformationUser(UserHandle,
                                          UserAllInformation,
                                          &UserInfo);
        if (!NT_SUCCESS(Status))
        {
            ERR("SamrQueryInformationUser failed (Status %08lx)\n", Status);
            goto done;
        }

        TRACE("UserName: %S\n", UserInfo->All.UserName.Buffer);

        /* Check the password */
        if ((UserInfo->All.UserAccountControl & USER_PASSWORD_NOT_REQUIRED) == 0)
        {
            Status = MsvpCheckPassword(&LogonInfo->Password,
                                       UserInfo);
            if (!NT_SUCCESS(Status))
            {
                ERR("MsvpCheckPassword failed (Status %08lx)\n", Status);
                goto done;
            }
        }

        /* Check account restrictions for non-administrator accounts */
        if (RelativeIds.Element[0] != DOMAIN_USER_RID_ADMIN)
        {
            /* Check if the account has been disabled */
            if (UserInfo->All.UserAccountControl & USER_ACCOUNT_DISABLED)
            {
                ERR("Account disabled!\n");
                *SubStatus = STATUS_ACCOUNT_DISABLED;
                Status = STATUS_ACCOUNT_RESTRICTION;
                goto done;
            }

            /* Check if the account has been locked */
            if (UserInfo->All.UserAccountControl & USER_ACCOUNT_AUTO_LOCKED)
            {
                ERR("Account locked!\n");
                *SubStatus = STATUS_ACCOUNT_LOCKED_OUT;
                Status = STATUS_ACCOUNT_RESTRICTION;
                goto done;
            }

            /* Check if the account expired */
            AccountExpires.LowPart = UserInfo->All.AccountExpires.LowPart;
            AccountExpires.HighPart = UserInfo->All.AccountExpires.HighPart;
            if (LogonTime.QuadPart >= AccountExpires.QuadPart)
            {
                ERR("Account expired!\n");
                *SubStatus = STATUS_ACCOUNT_EXPIRED;
                Status = STATUS_ACCOUNT_RESTRICTION;
                goto done;
            }

            /* Check if the password expired */
            PasswordMustChange.LowPart = UserInfo->All.PasswordMustChange.LowPart;
            PasswordMustChange.HighPart = UserInfo->All.PasswordMustChange.HighPart;
            PasswordLastSet.LowPart = UserInfo->All.PasswordLastSet.LowPart;
            PasswordLastSet.HighPart = UserInfo->All.PasswordLastSet.HighPart;

            if (LogonTime.QuadPart >= PasswordMustChange.QuadPart)
            {
                ERR("Password expired!\n");
                if (PasswordLastSet.QuadPart == 0)
                    *SubStatus = STATUS_PASSWORD_MUST_CHANGE;
                else
                    *SubStatus = STATUS_PASSWORD_EXPIRED;

                Status = STATUS_ACCOUNT_RESTRICTION;
                goto done;
            }

            /* Check logon hours */
            if (!MsvpCheckLogonHours(&UserInfo->All.LogonHours, &LogonTime))
            {
                ERR("Invalid logon hours!\n");
                *SubStatus = STATUS_INVALID_LOGON_HOURS;
                Status = STATUS_ACCOUNT_RESTRICTION;
                goto done;
            }

            /* Check workstations */
            if (!MsvpCheckWorkstations(&UserInfo->All.WorkStations, ComputerName))
            {
                ERR("Invalid workstation!\n");
                *SubStatus = STATUS_INVALID_WORKSTATION;
                Status = STATUS_ACCOUNT_RESTRICTION;
                goto done;
            }
        }
    }

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

    /* Build and fill the interactive profile buffer */
    Status = BuildInteractiveProfileBuffer(ClientRequest,
                                           UserInfo,
                                           ComputerName,
                                           (PMSV1_0_INTERACTIVE_PROFILE*)ProfileBuffer,
                                           ProfileBufferSize);
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
                                         UserInfo,
                                         SpecialAccount);
    if (!NT_SUCCESS(Status))
    {
        TRACE("BuildTokenInformationBuffer failed (Status %08lx)\n", Status);
        goto done;
    }

done:
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

    /* Return the authenticating authority */
    *AuthenticatingAuthority = DispatchTable.AllocateLsaHeap(sizeof(UNICODE_STRING));
    if (*AuthenticatingAuthority != NULL)
    {
        (*AuthenticatingAuthority)->Buffer = DispatchTable.AllocateLsaHeap(LogonInfo->LogonDomainName.Length +
                                                                           sizeof(UNICODE_NULL));
        if ((*AuthenticatingAuthority)->Buffer != NULL)
        {
            (*AuthenticatingAuthority)->MaximumLength = LogonInfo->LogonDomainName.Length +
                                                        sizeof(UNICODE_NULL);
            RtlCopyUnicodeString(*AuthenticatingAuthority, &LogonInfo->LogonDomainName);
        }
    }

    /* Return the machine name */
    *MachineName = DispatchTable.AllocateLsaHeap(sizeof(UNICODE_STRING));
    if (*MachineName != NULL)
    {
        (*MachineName)->Buffer = DispatchTable.AllocateLsaHeap((ComputerNameSize + 1) * sizeof(WCHAR));
        if ((*MachineName)->Buffer != NULL)
        {
            (*MachineName)->MaximumLength = (ComputerNameSize + 1) * sizeof(WCHAR);
            (*MachineName)->Length = ComputerNameSize * sizeof(WCHAR);
            RtlCopyMemory((*MachineName)->Buffer, ComputerName, (*MachineName)->MaximumLength);
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
    SamIFree_SAMPR_ULONG_ARRAY(&RelativeIds);
    SamIFree_SAMPR_ULONG_ARRAY(&Use);

    if (DomainHandle != NULL)
        SamrCloseHandle(&DomainHandle);

    if (ServerHandle != NULL)
        SamrCloseHandle(&ServerHandle);

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
    SECPKG_FUNCTION_TABLE Tables[1];

    TRACE("SpLsaModeInitialize(0x%lx %p %p %p)\n",
          LsaVersion, PackageVersion, ppTables, pcTables);

    if (LsaVersion != SECPKG_INTERFACE_VERSION)
        return STATUS_INVALID_PARAMETER;

    *PackageVersion = SECPKG_INTERFACE_VERSION;

    RtlZeroMemory(&Tables, sizeof(Tables));

    Tables[0].InitializePackage = LsaApInitializePackage;
//    Tables[0].LogonUser = NULL;
    Tables[0].CallPackage = (PLSA_AP_CALL_PACKAGE)LsaApCallPackage;
    Tables[0].LogonTerminated = LsaApLogonTerminated;
    Tables[0].CallPackageUntrusted = LsaApCallPackageUntrusted;
    Tables[0].CallPackagePassthrough = (PLSA_AP_CALL_PACKAGE_PASSTHROUGH)LsaApCallPackagePassthrough;
//    Tables[0].LogonUserEx = NULL;
    Tables[0].LogonUserEx2 = LsaApLogonUserEx2;
//    Tables[0].Initialize = SpInitialize;
//    Tables[0].Shutdown = NULL;
//    Tables[0].GetInfo = NULL;
//    Tables[0].AcceptCredentials = NULL;
//    Tables[0].SpAcquireCredentialsHandle = NULL;
//    Tables[0].SpQueryCredentialsAttributes = NULL;
//    Tables[0].FreeCredentialsHandle = NULL;
//    Tables[0].SaveCredentials = NULL;
//    Tables[0].GetCredentials = NULL;
//    Tables[0].DeleteCredentials = NULL;
//    Tables[0].InitLsaModeContext = NULL;
//    Tables[0].AcceptLsaModeContext = NULL;
//    Tables[0].DeleteContext = NULL;
//    Tables[0].ApplyControlToken = NULL;
//    Tables[0].GetUserInfo = NULL;
//    Tables[0].GetExtendedInformation = NULL;
//    Tables[0].SpQueryContextAttributes = NULL;
//    Tables[0].SpAddCredentials = NULL;
//    Tables[0].SetExtendedInformation = NULL;

    *ppTables = Tables;
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
    SECPKG_USER_FUNCTION_TABLE Tables[1];

    TRACE("SpUserModeInitialize(0x%lx %p %p %p)\n",
          LsaVersion, PackageVersion, ppTables, pcTables);

    if (LsaVersion != SECPKG_INTERFACE_VERSION)
        return STATUS_INVALID_PARAMETER;

    *PackageVersion = SECPKG_INTERFACE_VERSION;

    RtlZeroMemory(&Tables, sizeof(Tables));

//    Tables[0].InstanceInit = SpInstanceInit;
//    Tables[0].InitUserModeContext = NULL;
//    Tables[0].MakeSignature = NULL;
//    Tables[0].VerifySignature = NULL;
//    Tables[0].SealMessage = NULL;
//    Tables[0].UnsealMessage = NULL;
//    Tables[0].GetContextToken = NULL;
//    Tables[0].SpQueryContextAttributes = NULL;
//    Tables[0].CompleteAuthToken = NULL;
//    Tables[0].DeleteUserModeContext = NULL;
//    Tables[0].FormatCredentials = NULL;
//    Tables[0].MarshallSupplementalCreds = NULL;
//    Tables[0].ExportContext = NULL;
//    Tables[0].ImportContext = NULL;

    *ppTables = Tables;
    *pcTables = 1;

    return STATUS_SUCCESS;
}

/* EOF */
