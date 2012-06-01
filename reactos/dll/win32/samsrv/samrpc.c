/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Security Account Manager (SAM) Server
 * FILE:            reactos/dll/win32/samsrv/samrpc.c
 * PURPOSE:         RPC interface functions
 *
 * PROGRAMMERS:     Eric Kohl
 */

/* INCLUDES ****************************************************************/

#include "samsrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(samsrv);

/* GLOBALS ********************************************************************/

static SID_IDENTIFIER_AUTHORITY NtSidAuthority = {SECURITY_NT_AUTHORITY};

/* FUNCTIONS ***************************************************************/

VOID
SampStartRpcServer(VOID)
{
    RPC_STATUS Status;

    TRACE("SampStartRpcServer() called\n");

    Status = RpcServerUseProtseqEpW(L"ncacn_np",
                                    10,
                                    L"\\pipe\\samr",
                                    NULL);
    if (Status != RPC_S_OK)
    {
        WARN("RpcServerUseProtseqEpW() failed (Status %lx)\n", Status);
        return;
    }

    Status = RpcServerRegisterIf(samr_v1_0_s_ifspec,
                                 NULL,
                                 NULL);
    if (Status != RPC_S_OK)
    {
        WARN("RpcServerRegisterIf() failed (Status %lx)\n", Status);
        return;
    }

    Status = RpcServerListen(1, 20, TRUE);
    if (Status != RPC_S_OK)
    {
        WARN("RpcServerListen() failed (Status %lx)\n", Status);
        return;
    }

    TRACE("SampStartRpcServer() done\n");
}


void __RPC_FAR * __RPC_USER midl_user_allocate(SIZE_T len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}


void __RPC_USER midl_user_free(void __RPC_FAR * ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}

void __RPC_USER SAMPR_HANDLE_rundown(SAMPR_HANDLE hHandle)
{
}

/* Function 0 */
NTSTATUS
NTAPI
SamrConnect(IN PSAMPR_SERVER_NAME ServerName,
            OUT SAMPR_HANDLE *ServerHandle,
            IN ACCESS_MASK DesiredAccess)
{
    PSAM_DB_OBJECT ServerObject;
    NTSTATUS Status;

    TRACE("SamrConnect(%p %p %lx)\n",
          ServerName, ServerHandle, DesiredAccess);

    Status = SampOpenDbObject(NULL,
                              NULL,
                              L"SAM",
                              SamDbServerObject,
                              DesiredAccess,
                              &ServerObject);
    if (NT_SUCCESS(Status))
        *ServerHandle = (SAMPR_HANDLE)ServerObject;

    TRACE("SamrConnect done (Status 0x%08lx)\n", Status);

    return Status;
}

/* Function 1 */
NTSTATUS
NTAPI
SamrCloseHandle(IN OUT SAMPR_HANDLE *SamHandle)
{
    PSAM_DB_OBJECT DbObject;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("SamrCloseHandle(%p)\n", SamHandle);

    Status = SampValidateDbObject(*SamHandle,
                                  SamDbIgnoreObject,
                                  0,
                                  &DbObject);
    if (Status == STATUS_SUCCESS)
    {
        Status = SampCloseDbObject(DbObject);
        *SamHandle = NULL;
    }

    TRACE("SamrCloseHandle done (Status 0x%08lx)\n", Status);

    return Status;
}

/* Function 2 */
NTSTATUS
NTAPI
SamrSetSecurityObject(IN SAMPR_HANDLE ObjectHandle,
                      IN SECURITY_INFORMATION SecurityInformation,
                      IN PSAMPR_SR_SECURITY_DESCRIPTOR SecurityDescriptor)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 3 */
NTSTATUS
NTAPI
SamrQuerySecurityObject(IN SAMPR_HANDLE ObjectHandle,
                        IN SECURITY_INFORMATION SecurityInformation,
                        OUT PSAMPR_SR_SECURITY_DESCRIPTOR * SecurityDescriptor)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 4 */
NTSTATUS
NTAPI
SamrShutdownSamServer(IN SAMPR_HANDLE ServerHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 5 */
NTSTATUS
NTAPI
SamrLookupDomainInSamServer(IN SAMPR_HANDLE ServerHandle,
                            IN PRPC_UNICODE_STRING Name,
                            OUT PRPC_SID *DomainId)
{
    PSAM_DB_OBJECT ServerObject;
    HANDLE DomainsKeyHandle = NULL;
    HANDLE DomainKeyHandle = NULL;
    WCHAR DomainKeyName[64];
    ULONG Index;
    WCHAR DomainNameString[MAX_COMPUTERNAME_LENGTH + 1];
    UNICODE_STRING DomainName;
    ULONG Length;
    BOOL Found = FALSE;
    NTSTATUS Status;

    TRACE("SamrLookupDomainInSamServer(%p %p %p)\n",
          ServerHandle, Name, DomainId);

    /* Validate the server handle */
    Status = SampValidateDbObject(ServerHandle,
                                  SamDbServerObject,
                                  SAM_SERVER_LOOKUP_DOMAIN,
                                  &ServerObject);
    if (!NT_SUCCESS(Status))
        return Status;

    *DomainId = NULL;

    Status = SampRegOpenKey(ServerObject->KeyHandle,
                            L"Domains",
                            KEY_READ,
                            &DomainsKeyHandle);
    if (!NT_SUCCESS(Status))
        return Status;

    Index = 0;
    while (Found == FALSE)
    {
        Status = SampRegEnumerateSubKey(DomainsKeyHandle,
                                        Index,
                                        64,
                                        DomainKeyName);
        if (!NT_SUCCESS(Status))
        {
            if (Status == STATUS_NO_MORE_ENTRIES)
                Status = STATUS_NO_SUCH_DOMAIN;
            break;
        }

        TRACE("Domain key name: %S\n", DomainKeyName);

        Status = SampRegOpenKey(DomainsKeyHandle,
                                DomainKeyName,
                                KEY_READ,
                                &DomainKeyHandle);
        if (NT_SUCCESS(Status))
        {
            Length = (MAX_COMPUTERNAME_LENGTH + 1) * sizeof(WCHAR);
            Status = SampRegQueryValue(DomainKeyHandle,
                                       L"Name",
                                       NULL,
                                       (PVOID)&DomainNameString,
                                       &Length);
            if (NT_SUCCESS(Status))
            {
                TRACE("Domain name: %S\n", DomainNameString);

                RtlInitUnicodeString(&DomainName,
                                     DomainNameString);
                if (RtlEqualUnicodeString(&DomainName, (PUNICODE_STRING)Name, TRUE))
                {
                   TRACE("Found it!\n");
                   Found = TRUE;

                   Status = SampRegQueryValue(DomainKeyHandle,
                                              L"SID",
                                              NULL,
                                              NULL,
                                              &Length);
                   if (NT_SUCCESS(Status))
                   {
                       *DomainId = midl_user_allocate(Length);

                       SampRegQueryValue(DomainKeyHandle,
                                         L"SID",
                                         NULL,
                                         (PVOID)*DomainId,
                                         &Length);

                       Status = STATUS_SUCCESS;
                       break;
                   }
                }
            }

            NtClose(DomainKeyHandle);
        }

        Index++;
    }

    NtClose(DomainsKeyHandle);

    return Status;
}

/* Function 6 */
NTSTATUS
NTAPI
SamrEnumerateDomainsInSamServer(IN SAMPR_HANDLE ServerHandle,
                                IN OUT unsigned long *EnumerationContext,
                                OUT PSAMPR_ENUMERATION_BUFFER *Buffer,
                                IN unsigned long PreferedMaximumLength,
                                OUT unsigned long *CountReturned)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 7 */
NTSTATUS
NTAPI
SamrOpenDomain(IN SAMPR_HANDLE ServerHandle,
               IN ACCESS_MASK DesiredAccess,
               IN PRPC_SID DomainId,
               OUT SAMPR_HANDLE *DomainHandle)
{
    PSAM_DB_OBJECT ServerObject;
    PSAM_DB_OBJECT DomainObject;
    NTSTATUS Status;

    TRACE("SamrOpenDomain(%p %lx %p %p)\n",
          ServerHandle, DesiredAccess, DomainId, DomainHandle);

    /* Validate the server handle */
    Status = SampValidateDbObject(ServerHandle,
                                  SamDbServerObject,
                                  SAM_SERVER_LOOKUP_DOMAIN,
                                  &ServerObject);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Validate the Domain SID */
    if ((DomainId->Revision != SID_REVISION) ||
        (DomainId->SubAuthorityCount > SID_MAX_SUB_AUTHORITIES) ||
        (memcmp(&DomainId->IdentifierAuthority, &NtSidAuthority, sizeof(SID_IDENTIFIER_AUTHORITY)) != 0))
        return STATUS_INVALID_PARAMETER;

    /* Open the domain object */
    if ((DomainId->SubAuthorityCount == 1) &&
        (DomainId->SubAuthority[0] == SECURITY_BUILTIN_DOMAIN_RID))
    {
        /* Builtin domain object */
        TRACE("Opening the builtin domain object.\n");

        Status = SampOpenDbObject(ServerObject,
                                  L"Domains",
                                  L"Builtin",
                                  SamDbDomainObject,
                                  DesiredAccess,
                                  &DomainObject);
    }
    else if ((DomainId->SubAuthorityCount == 4) &&
             (DomainId->SubAuthority[0] == SECURITY_NT_NON_UNIQUE))
    {
        /* Account domain object */
        TRACE("Opening the account domain object.\n");

        /* FIXME: Check the account domain sub authorities!!! */

        Status = SampOpenDbObject(ServerObject,
                                  L"Domains",
                                  L"Account",
                                  SamDbDomainObject,
                                  DesiredAccess,
                                  &DomainObject);
    }
    else
    {
        /* No vaild domain SID */
        Status = STATUS_INVALID_PARAMETER;
    }

    if (NT_SUCCESS(Status))
        *DomainHandle = (SAMPR_HANDLE)DomainObject;

    TRACE("SamrOpenDomain done (Status 0x%08lx)\n", Status);

    return Status;
}

/* Function 8 */
NTSTATUS
NTAPI
SamrQueryInformationDomain(IN SAMPR_HANDLE DomainHandle,
                           IN DOMAIN_INFORMATION_CLASS DomainInformationClass,
                           OUT PSAMPR_DOMAIN_INFO_BUFFER *Buffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 9 */
NTSTATUS
NTAPI
SamrSetInformationDomain(IN SAMPR_HANDLE DomainHandle,
                         IN DOMAIN_INFORMATION_CLASS DomainInformationClass,
                         IN PSAMPR_DOMAIN_INFO_BUFFER DomainInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 10 */
NTSTATUS
NTAPI
SamrCreateGroupInDomain(IN SAMPR_HANDLE DomainHandle,
                        IN PRPC_UNICODE_STRING Name,
                        IN ACCESS_MASK DesiredAccess,
                        OUT SAMPR_HANDLE *GroupHandle,
                        OUT unsigned long *RelativeId)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 10 */
NTSTATUS
NTAPI
SamrEnumerateGroupsInDomain(IN SAMPR_HANDLE DomainHandle,
                            IN OUT unsigned long *EnumerationContext,
                            OUT PSAMPR_ENUMERATION_BUFFER *Buffer,
                            IN unsigned long PreferedMaximumLength,
                            OUT unsigned long *CountReturned)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 12 */
NTSTATUS
NTAPI
SamrCreateUserInDomain(IN SAMPR_HANDLE DomainHandle,
                       IN PRPC_UNICODE_STRING Name,
                       IN ACCESS_MASK DesiredAccess,
                       OUT SAMPR_HANDLE *UserHandle,
                       OUT unsigned long *RelativeId)
{
    PSAM_DB_OBJECT DomainObject;
    PSAM_DB_OBJECT UserObject;
    ULONG ulSize;
    ULONG ulRid;
    WCHAR szRid[9];
    BOOL bAliasExists = FALSE;
    NTSTATUS Status;

    TRACE("SamrCreateUserInDomain(%p %p %lx %p %p)\n",
          DomainHandle, Name, DesiredAccess, UserHandle, RelativeId);

    /* Validate the domain handle */
    Status = SampValidateDbObject(DomainHandle,
                                  SamDbDomainObject,
                                  DOMAIN_CREATE_USER,
                                  &DomainObject);
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

    /* Get the NextRID attribute */
    ulSize = sizeof(ULONG);
    Status = SampGetObjectAttribute(DomainObject,
                                    L"NextRID",
                                    NULL,
                                    (LPVOID)&ulRid,
                                    &ulSize);
    if (!NT_SUCCESS(Status))
        ulRid = DOMAIN_USER_RID_MAX + 1;

    TRACE("RID: %lx\n", ulRid);

    /* Convert the RID into a string (hex) */
    swprintf(szRid, L"%08lX", ulRid);

    /* Check whether the user name is already in use */
    Status = SampCheckDbObjectNameAlias(DomainObject,
                                        L"Users",
                                        Name->Buffer,
                                        &bAliasExists);
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

    if (bAliasExists)
    {
        TRACE("The user account %S already exists!\n", Name->Buffer);
        return STATUS_USER_EXISTS;
    }

    /* Create the user object */
    Status = SampCreateDbObject(DomainObject,
                                L"Users",
                                szRid,
                                SamDbUserObject,
                                DesiredAccess,
                                &UserObject);
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

    /* Add the name alias for the user object */
    Status = SampSetDbObjectNameAlias(DomainObject,
                                      L"Users",
                                      Name->Buffer,
                                      ulRid);
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

    /* Set the name attribute */
    Status = SampSetObjectAttribute(UserObject,
                                    L"Name",
                                    REG_SZ,
                                    (LPVOID)Name->Buffer,
                                    Name->MaximumLength);
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

    /* FIXME: Set default user attributes */

    if (NT_SUCCESS(Status))
    {
        *UserHandle = (SAMPR_HANDLE)UserObject;
        *RelativeId = ulRid;
    }

    /* Increment the NextRID attribute */
    ulRid++;
    ulSize = sizeof(ULONG);
    SampSetObjectAttribute(DomainObject,
                           L"NextRID",
                           REG_DWORD,
                           (LPVOID)&ulRid,
                           ulSize);

    TRACE("returns with status 0x%08lx\n", Status);

    return Status;
}

/* Function 13 */
NTSTATUS
NTAPI
SamrEnumerateUsersInDomain(IN SAMPR_HANDLE DomainHandle,
                           IN OUT unsigned long *EnumerationContext,
                           IN unsigned long UserAccountControl,
                           OUT PSAMPR_ENUMERATION_BUFFER *Buffer,
                           IN unsigned long PreferedMaximumLength,
                           OUT unsigned long *CountReturned)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 14 */
NTSTATUS
NTAPI
SamrCreateAliasInDomain(IN SAMPR_HANDLE DomainHandle,
                        IN PRPC_UNICODE_STRING AccountName,
                        IN ACCESS_MASK DesiredAccess,
                        OUT SAMPR_HANDLE *AliasHandle,
                        OUT unsigned long *RelativeId)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 15 */
NTSTATUS
NTAPI
SamrEnumerateAliasesInDomain(IN SAMPR_HANDLE DomainHandle,
                             IN OUT unsigned long *EnumerationContext,
                             OUT PSAMPR_ENUMERATION_BUFFER *Buffer,
                             IN unsigned long PreferedMaximumLength,
                             OUT unsigned long *CountReturned)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 16 */
NTSTATUS
NTAPI
SamrGetAliasMembership(IN SAMPR_HANDLE DomainHandle,
                       IN PSAMPR_PSID_ARRAY SidArray,
                       OUT PSAMPR_ULONG_ARRAY Membership)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 17 */
NTSTATUS
NTAPI
SamrLookupNamesInDomain(IN SAMPR_HANDLE DomainHandle,
                        IN unsigned long Count,
                        IN RPC_UNICODE_STRING Names[],
                        OUT PSAMPR_ULONG_ARRAY RelativeIds,
                        OUT PSAMPR_ULONG_ARRAY Use)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 18 */
NTSTATUS
NTAPI
SamrLookupIdsInDomain(IN SAMPR_HANDLE DomainHandle,
                      IN unsigned long Count,
                      IN unsigned long *RelativeIds,
                      OUT PSAMPR_RETURNED_USTRING_ARRAY Names,
                      OUT PSAMPR_ULONG_ARRAY Use)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 19 */
NTSTATUS
NTAPI
SamrOpenGroup(IN SAMPR_HANDLE DomainHandle,
              IN ACCESS_MASK DesiredAccess,
              IN unsigned long GroupId,
              OUT SAMPR_HANDLE *GroupHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 20 */
NTSTATUS
NTAPI
SamrQueryInformationGroup(IN SAMPR_HANDLE GroupHandle,
                          IN GROUP_INFORMATION_CLASS GroupInformationClass,
                          OUT PSAMPR_GROUP_INFO_BUFFER *Buffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 21 */
NTSTATUS
NTAPI
SamrSetInformationGroup(IN SAMPR_HANDLE GroupHandle,
                        IN GROUP_INFORMATION_CLASS GroupInformationClass,
                        IN PSAMPR_GROUP_INFO_BUFFER Buffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 22 */
NTSTATUS
NTAPI
SamrAddMemberToGroup(IN SAMPR_HANDLE GroupHandle,
                     IN unsigned long MemberId,
                     IN unsigned long Attributes)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 21 */
NTSTATUS
NTAPI
SamrDeleteGroup(IN OUT SAMPR_HANDLE *GroupHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 24 */
NTSTATUS
NTAPI
SamrRemoveMemberFromGroup(IN SAMPR_HANDLE GroupHandle,
                          IN unsigned long MemberId)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 25 */
NTSTATUS
NTAPI
SamrGetMembersInGroup(IN SAMPR_HANDLE GroupHandle,
                      OUT PSAMPR_GET_MEMBERS_BUFFER *Members)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 26 */
NTSTATUS
NTAPI
SamrSetMemberAttributesOfGroup(IN SAMPR_HANDLE GroupHandle,
                               IN unsigned long MemberId,
                               IN unsigned long Attributes)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 27 */
NTSTATUS
NTAPI
SamrOpenAlias(IN SAMPR_HANDLE DomainHandle,
              IN ACCESS_MASK DesiredAccess,
              IN unsigned long AliasId,
              OUT SAMPR_HANDLE *AliasHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 28 */
NTSTATUS
NTAPI
SamrQueryInformationAlias(IN SAMPR_HANDLE AliasHandle,
                          IN ALIAS_INFORMATION_CLASS AliasInformationClass,
                          OUT PSAMPR_ALIAS_INFO_BUFFER *Buffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 29 */
NTSTATUS
NTAPI
SamrSetInformationAlias(IN SAMPR_HANDLE AliasHandle,
                        IN ALIAS_INFORMATION_CLASS AliasInformationClass,
                        IN PSAMPR_ALIAS_INFO_BUFFER Buffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 30 */
NTSTATUS
NTAPI
SamrDeleteAlias(IN OUT SAMPR_HANDLE *AliasHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 31 */
NTSTATUS
NTAPI
SamrAddMemberToAlias(IN SAMPR_HANDLE AliasHandle,
                     IN PRPC_SID MemberId)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 32 */
NTSTATUS
NTAPI
SamrRemoveMemberFromAlias(IN SAMPR_HANDLE AliasHandle,
                          IN PRPC_SID MemberId)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 33 */
NTSTATUS
NTAPI
SamrGetMembersInAlias(IN SAMPR_HANDLE AliasHandle,
                      OUT PSAMPR_PSID_ARRAY_OUT Members)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 34 */
NTSTATUS
NTAPI
SamrOpenUser(IN SAMPR_HANDLE DomainHandle,
             IN ACCESS_MASK DesiredAccess,
             IN unsigned long UserId,
             OUT SAMPR_HANDLE *UserHandle)
{
    PSAM_DB_OBJECT DomainObject;
    PSAM_DB_OBJECT UserObject;
    WCHAR szRid[9];
    NTSTATUS Status;

    TRACE("SamrOpenUser(%p %lx %lx %p)\n",
          DomainHandle, DesiredAccess, UserId, UserHandle);

    /* Validate the domain handle */
    Status = SampValidateDbObject(DomainHandle,
                                  SamDbDomainObject,
                                  DOMAIN_LOOKUP,
                                  &DomainObject);
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

    /* Convert the RID into a string (hex) */
    swprintf(szRid, L"%08lX", UserId);

    /* Create the user object */
    Status = SampOpenDbObject(DomainObject,
                              L"Users",
                              szRid,
                              SamDbUserObject,
                              DesiredAccess,
                              &UserObject);
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

    *UserHandle = (SAMPR_HANDLE)UserObject;

    return STATUS_SUCCESS;
}

/* Function 35 */
NTSTATUS
NTAPI
SamrDeleteUser(IN OUT SAMPR_HANDLE *UserHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 36 */
NTSTATUS
NTAPI
SamrQueryInformationUser(IN SAMPR_HANDLE UserHandle,
                         IN USER_INFORMATION_CLASS UserInformationClass,
                         OUT PSAMPR_USER_INFO_BUFFER *Buffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


static
NTSTATUS
SampSetPasswordInformation(PSAM_DB_OBJECT UserObject,
                           PSAMPR_USER_SET_PASSWORD_INFORMATION PasswordInfo)
{
    NTSTATUS Status;

    TRACE("Password: %S\n", PasswordInfo->Password.Buffer);
    TRACE("PasswordExpired: %d\n", PasswordInfo->PasswordExpired);

    Status = SampSetObjectAttribute(UserObject,
                                    L"Password",
                                    REG_SZ,
                                    PasswordInfo->Password.Buffer,
                                    PasswordInfo->Password.MaximumLength);

    return Status;
}


/* Function 37 */
NTSTATUS
NTAPI
SamrSetInformationUser(IN SAMPR_HANDLE UserHandle,
                       IN USER_INFORMATION_CLASS UserInformationClass,
                       IN PSAMPR_USER_INFO_BUFFER Buffer)
{
    PSAM_DB_OBJECT UserObject;
    NTSTATUS Status;

    TRACE("SamrSetInformationUser(%p %lu %p)\n",
          UserHandle, UserInformationClass, Buffer);

    /* Validate the domain handle */
    Status = SampValidateDbObject(UserHandle,
                                  SamDbUserObject,
                                  USER_FORCE_PASSWORD_CHANGE,
                                  &UserObject);
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

    switch (UserInformationClass)
    {
        case UserSetPasswordInformation:
            Status = SampSetPasswordInformation(UserObject,
                                                (PSAMPR_USER_SET_PASSWORD_INFORMATION)Buffer);
            break;

        default:
            Status = STATUS_INVALID_INFO_CLASS;
            break;
    }

    return Status;
}

/* Function 38 */
NTSTATUS
NTAPI
SamrChangePasswordUser(IN SAMPR_HANDLE UserHandle,
                       IN unsigned char LmPresent,
                       IN PENCRYPTED_LM_OWF_PASSWORD OldLmEncryptedWithNewLm,
                       IN PENCRYPTED_LM_OWF_PASSWORD NewLmEncryptedWithOldLm,
                       IN unsigned char NtPresent,
                       IN PENCRYPTED_NT_OWF_PASSWORD OldNtEncryptedWithNewNt,
                       IN PENCRYPTED_NT_OWF_PASSWORD NewNtEncryptedWithOldNt,
                       IN unsigned char NtCrossEncryptionPresent,
                       IN PENCRYPTED_NT_OWF_PASSWORD NewNtEncryptedWithNewLm,
                       IN unsigned char LmCrossEncryptionPresent,
                       IN PENCRYPTED_LM_OWF_PASSWORD NewLmEncryptedWithNewNt)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 39 */
NTSTATUS
NTAPI
SamrGetGroupsForUser(IN SAMPR_HANDLE UserHandle,
                     OUT PSAMPR_GET_GROUPS_BUFFER *Groups)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 40 */
NTSTATUS
NTAPI
SamrQueryDisplayInformation(IN SAMPR_HANDLE DomainHandle,
                            IN DOMAIN_DISPLAY_INFORMATION DisplayInformationClass,
                            IN unsigned long Index,
                            IN unsigned long EntryCount,
                            IN unsigned long PreferredMaximumLength,
                            OUT unsigned long *TotalAvailable,
                            OUT unsigned long *TotalReturned,
                            OUT PSAMPR_DISPLAY_INFO_BUFFER Buffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 41 */
NTSTATUS
NTAPI
SamrGetDisplayEnumerationIndex(IN SAMPR_HANDLE DomainHandle,
                               IN DOMAIN_DISPLAY_INFORMATION DisplayInformationClass,
                               IN PRPC_UNICODE_STRING Prefix,
                               OUT unsigned long *Index)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 42 */
NTSTATUS
NTAPI
SamrTestPrivateFunctionsDomain(IN SAMPR_HANDLE DomainHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 43 */
NTSTATUS
NTAPI
SamrTestPrivateFunctionsUser(IN SAMPR_HANDLE UserHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 44 */
NTSTATUS
NTAPI
SamrGetUserDomainPasswordInformation(IN SAMPR_HANDLE UserHandle,
                                     OUT PUSER_DOMAIN_PASSWORD_INFORMATION PasswordInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 45 */
NTSTATUS
NTAPI
SamrRemoveMemberFromForeignDomain(IN SAMPR_HANDLE DomainHandle,
                                  IN PRPC_SID MemberSid)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 46 */
NTSTATUS
NTAPI
SamrQueryInformationDomain2(IN SAMPR_HANDLE DomainHandle,
                            IN DOMAIN_INFORMATION_CLASS DomainInformationClass,
                            OUT PSAMPR_DOMAIN_INFO_BUFFER *Buffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 47 */
NTSTATUS
NTAPI
SamrQueryInformationUser2(IN SAMPR_HANDLE UserHandle,
                          IN USER_INFORMATION_CLASS UserInformationClass,
                          OUT PSAMPR_USER_INFO_BUFFER *Buffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 48 */
NTSTATUS
NTAPI
SamrQueryDisplayInformation2(IN SAMPR_HANDLE DomainHandle,
                             IN DOMAIN_DISPLAY_INFORMATION DisplayInformationClass,
                             IN unsigned long Index,
                             IN unsigned long EntryCount,
                             IN unsigned long PreferredMaximumLength,
                             OUT unsigned long *TotalAvailable,
                             OUT unsigned long *TotalReturned,
                             OUT PSAMPR_DISPLAY_INFO_BUFFER Buffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 49 */
NTSTATUS
NTAPI
SamrGetDisplayEnumerationIndex2(IN SAMPR_HANDLE DomainHandle,
                                IN DOMAIN_DISPLAY_INFORMATION DisplayInformationClass,
                                IN PRPC_UNICODE_STRING Prefix,
                                OUT unsigned long *Index)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 50 */
NTSTATUS
NTAPI
SamrCreateUser2InDomain(IN SAMPR_HANDLE DomainHandle,
                        IN PRPC_UNICODE_STRING Name,
                        IN unsigned long AccountType,
                        IN ACCESS_MASK DesiredAccess,
                        OUT SAMPR_HANDLE *UserHandle,
                        OUT unsigned long *GrantedAccess,
                        OUT unsigned long *RelativeId)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 51 */
NTSTATUS
NTAPI
SamrQueryDisplayInformation3(IN SAMPR_HANDLE DomainHandle,
                             IN DOMAIN_DISPLAY_INFORMATION DisplayInformationClass,
                             IN unsigned long Index,
                             IN unsigned long EntryCount,
                             IN unsigned long PreferredMaximumLength,
                             OUT unsigned long *TotalAvailable,
                             OUT unsigned long *TotalReturned,
                             OUT PSAMPR_DISPLAY_INFO_BUFFER Buffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 52 */
NTSTATUS
NTAPI
SamrAddMultipleMembersToAlias(IN SAMPR_HANDLE AliasHandle,
                              IN PSAMPR_PSID_ARRAY MembersBuffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 53 */
NTSTATUS
NTAPI
SamrRemoveMultipleMembersFromAlias(IN SAMPR_HANDLE AliasHandle,
                                   IN PSAMPR_PSID_ARRAY MembersBuffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 54 */
NTSTATUS
NTAPI
SamrOemChangePasswordUser2(IN handle_t BindingHandle,
                           IN PRPC_STRING ServerName,
                           IN PRPC_STRING UserName,
                           IN PSAMPR_ENCRYPTED_USER_PASSWORD NewPasswordEncryptedWithOldLm,
                           IN PENCRYPTED_LM_OWF_PASSWORD OldLmOwfPasswordEncryptedWithNewLm)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 55 */
NTSTATUS
NTAPI
SamrUnicodeChangePasswordUser2(IN handle_t BindingHandle,
                               IN PRPC_UNICODE_STRING ServerName,
                               IN PRPC_UNICODE_STRING UserName,
                               IN PSAMPR_ENCRYPTED_USER_PASSWORD NewPasswordEncryptedWithOldNt,
                               IN PENCRYPTED_NT_OWF_PASSWORD OldNtOwfPasswordEncryptedWithNewNt,
                               IN unsigned char LmPresent,
                               IN PSAMPR_ENCRYPTED_USER_PASSWORD NewPasswordEncryptedWithOldLm,
                               IN PENCRYPTED_LM_OWF_PASSWORD OldLmOwfPasswordEncryptedWithNewNt)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 56 */
NTSTATUS
NTAPI
SamrGetDomainPasswordInformation(IN handle_t BindingHandle,
                                 IN PRPC_UNICODE_STRING Unused,
                                 OUT PUSER_DOMAIN_PASSWORD_INFORMATION PasswordInformation)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 57 */
NTSTATUS
NTAPI
SamrConnect2(IN PSAMPR_SERVER_NAME ServerName,
             OUT SAMPR_HANDLE *ServerHandle,
             IN ACCESS_MASK DesiredAccess)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 58 */
NTSTATUS
NTAPI
SamrSetInformationUser2(IN SAMPR_HANDLE UserHandle,
                        IN USER_INFORMATION_CLASS UserInformationClass,
                        IN PSAMPR_USER_INFO_BUFFER Buffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 59 */
NTSTATUS
NTAPI
SamrSetBootKeyInformation(IN handle_t BindingHandle) /* FIXME */
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 60 */
NTSTATUS
NTAPI
SamrGetBootKeyInformation(IN handle_t BindingHandle) /* FIXME */
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 61 */
NTSTATUS
NTAPI
SamrConnect3(IN handle_t BindingHandle) /* FIXME */
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 62 */
NTSTATUS
NTAPI
SamrConnect4(IN PSAMPR_SERVER_NAME ServerName,
             OUT SAMPR_HANDLE *ServerHandle,
             IN unsigned long ClientRevision,
             IN ACCESS_MASK DesiredAccess)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 63 */
NTSTATUS
NTAPI
SamrUnicodeChangePasswordUser3(IN handle_t BindingHandle) /* FIXME */
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 64 */
NTSTATUS
NTAPI
SamrConnect5(IN PSAMPR_SERVER_NAME ServerName,
             IN ACCESS_MASK DesiredAccess,
             IN unsigned long InVersion,
             IN SAMPR_REVISION_INFO *InRevisionInfo,
             OUT unsigned long *OutVersion,
             OUT SAMPR_REVISION_INFO *OutRevisionInfo,
             OUT SAMPR_HANDLE *ServerHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 65 */
NTSTATUS
NTAPI
SamrRidToSid(IN SAMPR_HANDLE ObjectHandle,
             IN unsigned long Rid,
             OUT PRPC_SID *Sid)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 66 */
NTSTATUS
NTAPI
SamrSetDSRMPassword(IN handle_t BindingHandle,
                    IN PRPC_UNICODE_STRING Unused,
                    IN unsigned long UserId,
                    IN PENCRYPTED_NT_OWF_PASSWORD EncryptedNtOwfPassword)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* Function 67 */
NTSTATUS
NTAPI
SamrValidatePassword(IN handle_t Handle,
                     IN PASSWORD_POLICY_VALIDATION_TYPE ValidationType,
                     IN PSAM_VALIDATE_INPUT_ARG InputArg,
                     OUT PSAM_VALIDATE_OUTPUT_ARG *OutputArg)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
