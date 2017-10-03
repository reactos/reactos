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
    LocalBuffer->LogonServer.Buffer = (LPWSTR)((ULONG_PTR)ClientBaseAddress + (ULONG_PTR)Ptr - (ULONG_PTR)LocalBuffer);
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
                 IN PSID AccountDomainSid)
{
    SID_IDENTIFIER_AUTHORITY SystemAuthority = {SECURITY_NT_AUTHORITY};
    PTOKEN_GROUPS TokenGroups;
#define MAX_GROUPS 2
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
BuildTokenInformationBuffer(PLSA_TOKEN_INFORMATION_V1 *TokenInformation,
                            PRPC_SID AccountDomainSid,
                            PSAMPR_USER_INFO_BUFFER UserInfo)
{
    PLSA_TOKEN_INFORMATION_V1 Buffer = NULL;
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
                            UserInfo->All.UserId);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = BuildTokenPrimaryGroup(&Buffer->PrimaryGroup,
                                    (PSID)AccountDomainSid,
                                    UserInfo->All.PrimaryGroupId);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = BuildTokenGroups(&Buffer->Groups,
                              (PSID)AccountDomainSid);
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
    PMSV1_0_CHANGEPASSWORD_REQUEST RequestBuffer;
    ULONG_PTR PtrOffset;

    SAMPR_HANDLE ServerHandle = NULL;
    SAMPR_HANDLE DomainHandle = NULL;
    SAMPR_HANDLE UserHandle = NULL;
    PRPC_SID DomainSid = NULL;
    RPC_UNICODE_STRING Names[1];
    SAMPR_ULONG_ARRAY RelativeIds = {0, NULL};
    SAMPR_ULONG_ARRAY Use = {0, NULL};
    NTSTATUS Status;

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

    TRACE("()\n");

    RequestBuffer = (PMSV1_0_CHANGEPASSWORD_REQUEST)ProtocolSubmitBuffer;

    /* Fix-up pointers in the request buffer info */
    PtrOffset = (ULONG_PTR)ProtocolSubmitBuffer - (ULONG_PTR)ClientBufferBase;

    RequestBuffer->DomainName.Buffer = FIXUP_POINTER(RequestBuffer->DomainName.Buffer, PtrOffset);
    RequestBuffer->AccountName.Buffer = FIXUP_POINTER(RequestBuffer->AccountName.Buffer, PtrOffset);
    RequestBuffer->OldPassword.Buffer = FIXUP_POINTER(RequestBuffer->OldPassword.Buffer, PtrOffset);
    RequestBuffer->NewPassword.Buffer = FIXUP_POINTER(RequestBuffer->NewPassword.Buffer, PtrOffset);

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
    ULONG MessageType;
    NTSTATUS Status;

    TRACE("()\n");

    if (SubmitBufferLength < sizeof(MSV1_0_PROTOCOL_MESSAGE_TYPE))
        return STATUS_INVALID_PARAMETER;

    MessageType = (ULONG)*((PMSV1_0_PROTOCOL_MESSAGE_TYPE)ProtocolSubmitBuffer);

    *ProtocolReturnBuffer = NULL;
    *ReturnBufferLength = 0;

    switch (MessageType)
    {
        case MsV1_0Lm20ChallengeRequest:
        case MsV1_0Lm20GetChallengeResponse:
        case MsV1_0EnumerateUsers:
        case MsV1_0GetUserInfo:
        case MsV1_0ReLogonUsers:
            Status = STATUS_NOT_IMPLEMENTED;
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
    BOOLEAN SessionCreated = FALSE;
    LARGE_INTEGER LogonTime;
//    LARGE_INTEGER AccountExpires;
    LARGE_INTEGER PasswordMustChange;
    LARGE_INTEGER PasswordLastSet;
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

        LogonInfo->LogonDomainName.Buffer = FIXUP_POINTER(LogonInfo->LogonDomainName.Buffer, PtrOffset);
        LogonInfo->UserName.Buffer = FIXUP_POINTER(LogonInfo->UserName.Buffer, PtrOffset);
        LogonInfo->Password.Buffer = FIXUP_POINTER(LogonInfo->Password.Buffer, PtrOffset);

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

    /* Get the logon time */
    NtQuerySystemTime(&LogonTime);

    /* Get the domain SID */
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

    /* Check the password */
    if ((UserInfo->All.UserAccountControl & USER_PASSWORD_NOT_REQUIRED) == 0)
    {
        Status = MsvpCheckPassword(&(LogonInfo->Password),
                                   UserInfo);
        if (!NT_SUCCESS(Status))
        {
            TRACE("MsvpCheckPassword failed (Status %08lx)\n", Status);
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

#if 0
        /* Check if the account expired */
        AccountExpires.LowPart = UserInfo->All.AccountExpires.LowPart;
        AccountExpires.HighPart = UserInfo->All.AccountExpires.HighPart;

        if (AccountExpires.QuadPart != 0 &&
            LogonTime.QuadPart >= AccountExpires.QuadPart)
        {
            ERR("Account expired!\n");
            *SubStatus = STATUS_ACCOUNT_EXPIRED;
            Status = STATUS_ACCOUNT_RESTRICTION;
            goto done;
        }
#endif

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

        /* FIXME: more checks */
        // STATUS_INVALID_LOGON_HOURS;
        // STATUS_INVALID_WORKSTATION;
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
                                         UserInfo);
    if (!NT_SUCCESS(Status))
    {
        TRACE("BuildTokenInformationBuffer failed (Status %08lx)\n", Status);
        goto done;
    }

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

    TRACE("LsaApLogonUser done (Status 0x%08lx  SubStatus 0x%08lx)\n", Status, *SubStatus);

    return Status;
}


/*
 * @unimplemented
 */
#if 0
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
#endif

/* EOF */
