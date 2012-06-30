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
                                IN ULONG PreferedMaximumLength,
                                OUT PULONG CountReturned)
{
    PSAM_DB_OBJECT ServerObject;
    WCHAR DomainKeyName[64];
    HANDLE DomainsKeyHandle;
    HANDLE DomainKeyHandle;
    ULONG EnumIndex;
    ULONG EnumCount;
    ULONG RequiredLength;
    ULONG DataLength;
    ULONG i;
    PSAMPR_ENUMERATION_BUFFER EnumBuffer = NULL;
    NTSTATUS Status;

    TRACE("SamrEnumerateDomainsInSamServer(%p %p %p %lu %p)\n",
          ServerHandle, EnumerationContext, Buffer, PreferedMaximumLength,
          CountReturned);

    /* Validate the server handle */
    Status = SampValidateDbObject(ServerHandle,
                                  SamDbServerObject,
                                  SAM_SERVER_ENUMERATE_DOMAINS,
                                  &ServerObject);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = SampRegOpenKey(ServerObject->KeyHandle,
                            L"Domains",
                            KEY_READ,
                            &DomainsKeyHandle);
    if (!NT_SUCCESS(Status))
        return Status;

    EnumIndex = *EnumerationContext;
    EnumCount = 0;
    RequiredLength = 0;

    while (TRUE)
    {
        Status = SampRegEnumerateSubKey(DomainsKeyHandle,
                                        EnumIndex,
                                        64 * sizeof(WCHAR),
                                        DomainKeyName);
        if (!NT_SUCCESS(Status))
            break;

        TRACE("EnumIndex: %lu\n", EnumIndex);
        TRACE("Domain key name: %S\n", DomainKeyName);

        Status = SampRegOpenKey(DomainsKeyHandle,
                                DomainKeyName,
                                KEY_READ,
                                &DomainKeyHandle);
        TRACE("SampRegOpenKey returned %08lX\n", Status);
        if (NT_SUCCESS(Status))
        {
            DataLength = 0;
            Status = SampRegQueryValue(DomainKeyHandle,
                                       L"Name",
                                       NULL,
                                       NULL,
                                       &DataLength);
            TRACE("SampRegQueryValue returned %08lX\n", Status);
            if (NT_SUCCESS(Status))
            {
                TRACE("Data length: %lu\n", DataLength);

                if ((RequiredLength + DataLength + sizeof(UNICODE_STRING)) > PreferedMaximumLength)
                    break;

                RequiredLength += (DataLength + sizeof(UNICODE_STRING));
                EnumCount++;
            }

            NtClose(DomainKeyHandle);
        }

        EnumIndex++;
    }

    TRACE("EnumCount: %lu\n", EnumCount);
    TRACE("RequiredLength: %lu\n", RequiredLength);

    EnumBuffer = midl_user_allocate(sizeof(SAMPR_ENUMERATION_BUFFER));
    if (EnumBuffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    EnumBuffer->EntriesRead = EnumCount;
    EnumBuffer->Buffer = midl_user_allocate(EnumCount * sizeof(SAMPR_RID_ENUMERATION));
    if (EnumBuffer->Buffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    EnumIndex = *EnumerationContext;
    for (i = 0; i < EnumCount; i++, EnumIndex++)
    {
        Status = SampRegEnumerateSubKey(DomainsKeyHandle,
                                        EnumIndex,
                                        64 * sizeof(WCHAR),
                                        DomainKeyName);
        if (!NT_SUCCESS(Status))
            break;

        TRACE("EnumIndex: %lu\n", EnumIndex);
        TRACE("Domain key name: %S\n", DomainKeyName);

        Status = SampRegOpenKey(DomainsKeyHandle,
                                DomainKeyName,
                                KEY_READ,
                                &DomainKeyHandle);
        TRACE("SampRegOpenKey returned %08lX\n", Status);
        if (NT_SUCCESS(Status))
        {
            DataLength = 0;
            Status = SampRegQueryValue(DomainKeyHandle,
                                       L"Name",
                                       NULL,
                                       NULL,
                                       &DataLength);
            TRACE("SampRegQueryValue returned %08lX\n", Status);
            if (NT_SUCCESS(Status))
            {
                EnumBuffer->Buffer[i].RelativeId = 0;
                EnumBuffer->Buffer[i].Name.Length = (USHORT)DataLength - sizeof(WCHAR);
                EnumBuffer->Buffer[i].Name.MaximumLength = (USHORT)DataLength;
                EnumBuffer->Buffer[i].Name.Buffer = midl_user_allocate(DataLength);
                if (EnumBuffer->Buffer[i].Name.Buffer == NULL)
                {
                    NtClose(DomainKeyHandle);
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto done;
                }

                Status = SampRegQueryValue(DomainKeyHandle,
                                           L"Name",
                                           NULL,
                                           EnumBuffer->Buffer[i].Name.Buffer,
                                           &DataLength);
                TRACE("SampRegQueryValue returned %08lX\n", Status);
                if (NT_SUCCESS(Status))
                {
                    TRACE("Domain name: %S\n", EnumBuffer->Buffer[i].Name.Buffer);
                }
            }

            NtClose(DomainKeyHandle);

            if (!NT_SUCCESS(Status))
                goto done;
        }
    }

    if (NT_SUCCESS(Status))
    {
        *EnumerationContext += EnumCount;
        *Buffer = EnumBuffer;
        *CountReturned = EnumCount;
    }

done:
    if (!NT_SUCCESS(Status))
    {
        *EnumerationContext = 0;
        *Buffer = NULL;
        *CountReturned = 0;

        if (EnumBuffer != NULL)
        {
            if (EnumBuffer->Buffer != NULL)
            {
                if (EnumBuffer->EntriesRead != 0)
                {
                    for (i = 0; i < EnumBuffer->EntriesRead; i++)
                    {
                        if (EnumBuffer->Buffer[i].Name.Buffer != NULL)
                            midl_user_free(EnumBuffer->Buffer[i].Name.Buffer);
                    }
                }

                midl_user_free(EnumBuffer->Buffer);
            }

            midl_user_free(EnumBuffer);
        }
    }

    NtClose(DomainsKeyHandle);

    return Status;
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


static NTSTATUS
SampQueryDomainName(PSAM_DB_OBJECT DomainObject,
                    PSAMPR_DOMAIN_INFO_BUFFER *Buffer)
{
    PSAMPR_DOMAIN_INFO_BUFFER InfoBuffer = NULL;
    ULONG Length = 0;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_DOMAIN_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = SampGetObjectAttribute(DomainObject,
                                    L"Name",
                                    NULL,
                                    NULL,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    InfoBuffer->Name.DomainName.Length = Length - sizeof(WCHAR);
    InfoBuffer->Name.DomainName.MaximumLength = Length;
    InfoBuffer->Name.DomainName.Buffer = midl_user_allocate(Length);
    if (InfoBuffer->Name.DomainName.Buffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    Status = SampGetObjectAttribute(DomainObject,
                                    L"Name",
                                    NULL,
                                    (PVOID)InfoBuffer->Name.DomainName.Buffer,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    *Buffer = InfoBuffer;

done:
    if (!NT_SUCCESS(Status))
    {
        if (InfoBuffer != NULL)
        {
            if (InfoBuffer->Name.DomainName.Buffer != NULL)
                midl_user_free(InfoBuffer->Name.DomainName.Buffer);

            midl_user_free(InfoBuffer);
        }
    }

    return Status;
}


/* Function 8 */
NTSTATUS
NTAPI
SamrQueryInformationDomain(IN SAMPR_HANDLE DomainHandle,
                           IN DOMAIN_INFORMATION_CLASS DomainInformationClass,
                           OUT PSAMPR_DOMAIN_INFO_BUFFER *Buffer)
{
    PSAM_DB_OBJECT DomainObject;
    NTSTATUS Status;

    TRACE("SamrQueryInformationDomain(%p %lu %p)\n",
          DomainHandle, DomainInformationClass, Buffer);

    /* Validate the server handle */
    Status = SampValidateDbObject(DomainHandle,
                                  SamDbDomainObject,
                                  DOMAIN_READ_OTHER_PARAMETERS,
                                  &DomainObject);
    if (!NT_SUCCESS(Status))
        return Status;

    switch (DomainInformationClass)
    {
        case DomainNameInformation:
            Status = SampQueryDomainName(DomainObject,
                                         Buffer);
            break;

        default:
            Status = STATUS_NOT_IMPLEMENTED;
    }

    return Status;
}

static NTSTATUS
SampSetDomainName(PSAM_DB_OBJECT DomainObject,
                  PSAMPR_DOMAIN_NAME_INFORMATION DomainNameInfo)
{
    NTSTATUS Status;

    Status = SampSetObjectAttribute(DomainObject,
                                    L"Name",
                                    REG_SZ,
                                    DomainNameInfo->DomainName.Buffer,
                                    DomainNameInfo->DomainName.Length + sizeof(WCHAR));

    return Status;
}

/* Function 9 */
NTSTATUS
NTAPI
SamrSetInformationDomain(IN SAMPR_HANDLE DomainHandle,
                         IN DOMAIN_INFORMATION_CLASS DomainInformationClass,
                         IN PSAMPR_DOMAIN_INFO_BUFFER DomainInformation)
{
    PSAM_DB_OBJECT DomainObject;
    NTSTATUS Status;

    TRACE("SamrSetInformationDomain(%p %lu %p)\n",
          DomainHandle, DomainInformationClass, DomainInformation);

    /* Validate the server handle */
    Status = SampValidateDbObject(DomainHandle,
                                  SamDbDomainObject,
                                  DOMAIN_WRITE_OTHER_PARAMETERS,
                                  &DomainObject);
    if (!NT_SUCCESS(Status))
        return Status;

    switch (DomainInformationClass)
    {
        case DomainNameInformation:
            Status = SampSetDomainName(DomainObject,
                                       &DomainInformation->Name);
            break;

        default:
            Status = STATUS_NOT_IMPLEMENTED;
    }

    return Status;
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
    PSAM_DB_OBJECT DomainObject;
    PSAM_DB_OBJECT AliasObject;
    UNICODE_STRING EmptyString = RTL_CONSTANT_STRING(L"");
    ULONG ulSize;
    ULONG ulRid;
    WCHAR szRid[9];
    BOOL bAliasExists = FALSE;
    NTSTATUS Status;

    TRACE("SamrCreateAliasInDomain(%p %p %lx %p %p)\n",
          DomainHandle, AccountName, DesiredAccess, AliasHandle, RelativeId);

    /* Validate the domain handle */
    Status = SampValidateDbObject(DomainHandle,
                                  SamDbDomainObject,
                                  DOMAIN_CREATE_ALIAS,
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
                                        L"Aliases",
                                        AccountName->Buffer,
                                        &bAliasExists);
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

    if (bAliasExists)
    {
        TRACE("The alias account %S already exists!\n", AccountName->Buffer);
        return STATUS_ALIAS_EXISTS;
    }

    /* Create the user object */
    Status = SampCreateDbObject(DomainObject,
                                L"Aliases",
                                szRid,
                                SamDbAliasObject,
                                DesiredAccess,
                                &AliasObject);
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

    /* Add the name alias for the user object */
    Status = SampSetDbObjectNameAlias(DomainObject,
                                      L"Aliases",
                                      AccountName->Buffer,
                                      ulRid);
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

    /* Set the Name attribute */
    Status = SampSetObjectAttribute(AliasObject,
                                    L"Name",
                                    REG_SZ,
                                    (LPVOID)AccountName->Buffer,
                                    AccountName->MaximumLength);
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

    /* Set the Description attribute */
    Status = SampSetObjectAttribute(AliasObject,
                                    L"Description",
                                    REG_SZ,
                                    EmptyString.Buffer,
                                    EmptyString.MaximumLength);
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

    if (NT_SUCCESS(Status))
    {
        *AliasHandle = (SAMPR_HANDLE)AliasObject;
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

/* Function 15 */
NTSTATUS
NTAPI
SamrEnumerateAliasesInDomain(IN SAMPR_HANDLE DomainHandle,
                             IN OUT unsigned long *EnumerationContext,
                             OUT PSAMPR_ENUMERATION_BUFFER *Buffer,
                             IN unsigned long PreferedMaximumLength,
                             OUT unsigned long *CountReturned)
{
    PSAMPR_ENUMERATION_BUFFER EnumBuffer = NULL;
    PSAM_DB_OBJECT DomainObject;
    HANDLE AliasesKeyHandle;
    WCHAR AliasKeyName[64];
    HANDLE AliasKeyHandle;
    ULONG EnumIndex;
    ULONG EnumCount;
    ULONG RequiredLength;
    ULONG DataLength;
    ULONG i;
    BOOLEAN MoreEntries = FALSE;
    NTSTATUS Status;

    TRACE("SamrEnumerateAliasesInDomain(%p %p %p %lu %p)\n",
          DomainHandle, EnumerationContext, Buffer, PreferedMaximumLength,
          CountReturned);

    /* Validate the domain handle */
    Status = SampValidateDbObject(DomainHandle,
                                  SamDbDomainObject,
                                  DOMAIN_LIST_ACCOUNTS,
                                  &DomainObject);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = SampRegOpenKey(DomainObject->KeyHandle,
                            L"Aliases",
                            KEY_READ,
                            &AliasesKeyHandle);
    if (!NT_SUCCESS(Status))
        return Status;

    TRACE("Part 1\n");

    EnumIndex = *EnumerationContext;
    EnumCount = 0;
    RequiredLength = 0;

    while (TRUE)
    {
        Status = SampRegEnumerateSubKey(AliasesKeyHandle,
                                        EnumIndex,
                                        64 * sizeof(WCHAR),
                                        AliasKeyName);
        if (!NT_SUCCESS(Status))
        {
            if (Status == STATUS_NO_MORE_ENTRIES)
                Status = STATUS_SUCCESS;
            break;
        }

        TRACE("EnumIndex: %lu\n", EnumIndex);
        TRACE("Alias key name: %S\n", AliasKeyName);

        Status = SampRegOpenKey(AliasesKeyHandle,
                                AliasKeyName,
                                KEY_READ,
                                &AliasKeyHandle);
        TRACE("SampRegOpenKey returned %08lX\n", Status);
        if (NT_SUCCESS(Status))
        {
            DataLength = 0;
            Status = SampRegQueryValue(AliasKeyHandle,
                                       L"Name",
                                       NULL,
                                       NULL,
                                       &DataLength);

            NtClose(AliasKeyHandle);

            TRACE("SampRegQueryValue returned %08lX\n", Status);

            if (NT_SUCCESS(Status))
            {
                TRACE("Data length: %lu\n", DataLength);

                if ((RequiredLength + DataLength + sizeof(SAMPR_RID_ENUMERATION)) > PreferedMaximumLength)
                {
                    MoreEntries = TRUE;
                    break;
                }

                RequiredLength += (DataLength + sizeof(SAMPR_RID_ENUMERATION));
                EnumCount++;
            }
        }

        EnumIndex++;
    }

    TRACE("EnumCount: %lu\n", EnumCount);
    TRACE("RequiredLength: %lu\n", RequiredLength);

    EnumBuffer = midl_user_allocate(sizeof(SAMPR_ENUMERATION_BUFFER));
    if (EnumBuffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    EnumBuffer->EntriesRead = EnumCount;
    if (EnumCount == 0)
        goto done;

    EnumBuffer->Buffer = midl_user_allocate(EnumCount * sizeof(SAMPR_RID_ENUMERATION));
    if (EnumBuffer->Buffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    TRACE("Part 2\n");

    EnumIndex = *EnumerationContext;
    for (i = 0; i < EnumCount; i++, EnumIndex++)
    {
        Status = SampRegEnumerateSubKey(AliasesKeyHandle,
                                        EnumIndex,
                                        64 * sizeof(WCHAR),
                                        AliasKeyName);
        if (!NT_SUCCESS(Status))
        {
            if (Status == STATUS_NO_MORE_ENTRIES)
                Status = STATUS_SUCCESS;
            break;
        }

        TRACE("EnumIndex: %lu\n", EnumIndex);
        TRACE("Alias key name: %S\n", AliasKeyName);

        Status = SampRegOpenKey(AliasesKeyHandle,
                                AliasKeyName,
                                KEY_READ,
                                &AliasKeyHandle);
        TRACE("SampRegOpenKey returned %08lX\n", Status);
        if (NT_SUCCESS(Status))
        {
            DataLength = 0;
            Status = SampRegQueryValue(AliasKeyHandle,
                                       L"Name",
                                       NULL,
                                       NULL,
                                       &DataLength);
            TRACE("SampRegQueryValue returned %08lX\n", Status);
            if (NT_SUCCESS(Status))
            {
                EnumBuffer->Buffer[i].RelativeId = wcstoul(AliasKeyName, NULL, 16);

                EnumBuffer->Buffer[i].Name.Length = (USHORT)DataLength - sizeof(WCHAR);
                EnumBuffer->Buffer[i].Name.MaximumLength = (USHORT)DataLength;
                EnumBuffer->Buffer[i].Name.Buffer = midl_user_allocate(DataLength);
                if (EnumBuffer->Buffer[i].Name.Buffer == NULL)
                {
                    NtClose(AliasKeyHandle);
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto done;
                }

                Status = SampRegQueryValue(AliasKeyHandle,
                                           L"Name",
                                           NULL,
                                           EnumBuffer->Buffer[i].Name.Buffer,
                                           &DataLength);
                TRACE("SampRegQueryValue returned %08lX\n", Status);
                if (NT_SUCCESS(Status))
                {
                    TRACE("Alias name: %S\n", EnumBuffer->Buffer[i].Name.Buffer);
                }
            }

            NtClose(AliasKeyHandle);

            if (!NT_SUCCESS(Status))
                goto done;
        }
    }

done:
    if (NT_SUCCESS(Status))
    {
        *EnumerationContext += EnumCount;
        *Buffer = EnumBuffer;
        *CountReturned = EnumCount;
    }

    if (!NT_SUCCESS(Status))
    {
        *EnumerationContext = 0;
        *Buffer = NULL;
        *CountReturned = 0;

        if (EnumBuffer != NULL)
        {
            if (EnumBuffer->Buffer != NULL)
            {
                if (EnumBuffer->EntriesRead != 0)
                {
                    for (i = 0; i < EnumBuffer->EntriesRead; i++)
                    {
                        if (EnumBuffer->Buffer[i].Name.Buffer != NULL)
                            midl_user_free(EnumBuffer->Buffer[i].Name.Buffer);
                    }
                }

                midl_user_free(EnumBuffer->Buffer);
            }

            midl_user_free(EnumBuffer);
        }
    }

    NtClose(AliasesKeyHandle);

    if ((Status == STATUS_SUCCESS) && (MoreEntries == TRUE))
        Status = STATUS_MORE_ENTRIES;

    return Status;
}

/* Function 16 */
NTSTATUS
NTAPI
SamrGetAliasMembership(IN SAMPR_HANDLE DomainHandle,
                       IN PSAMPR_PSID_ARRAY SidArray,
                       OUT PSAMPR_ULONG_ARRAY Membership)
{
    PSAM_DB_OBJECT DomainObject;
    HANDLE AliasesKeyHandle = NULL;
    HANDLE MembersKeyHandle = NULL;
    HANDLE MemberKeyHandle = NULL;
    LPWSTR MemberSidString = NULL;
    PULONG RidArray = NULL;
    ULONG MaxSidCount = 0;
    ULONG ValueCount;
    ULONG DataLength;
    ULONG i, j;
    NTSTATUS Status;

    TRACE("SamrGetAliasMembership(%p %p %p)\n",
          DomainHandle, SidArray, Membership);

    /* Validate the domain handle */
    Status = SampValidateDbObject(DomainHandle,
                                  SamDbDomainObject,
                                  DOMAIN_LOOKUP,
                                  &DomainObject);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = SampRegOpenKey(DomainObject->KeyHandle,
                            L"Aliases",
                            KEY_READ,
                            &AliasesKeyHandle);
    TRACE("SampRegOpenKey returned %08lX\n", Status);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampRegOpenKey(AliasesKeyHandle,
                            L"Members",
                            KEY_READ,
                            &MembersKeyHandle);
    TRACE("SampRegOpenKey returned %08lX\n", Status);
    if (!NT_SUCCESS(Status))
        goto done;

    for (i = 0; i < SidArray->Count; i++)
    {
        ConvertSidToStringSid(SidArray->Sids[i].SidPointer, &MemberSidString);
TRACE("Open %S\n", MemberSidString);

        Status = SampRegOpenKey(MembersKeyHandle,
                                MemberSidString,
                                KEY_READ,
                                &MemberKeyHandle);
        TRACE("SampRegOpenKey returned %08lX\n", Status);
        if (NT_SUCCESS(Status))
        {
            Status = SampRegQueryKeyInfo(MemberKeyHandle,
                                         NULL,
                                         &ValueCount);
            if (NT_SUCCESS(Status))
            {
                TRACE("Found %lu values\n", ValueCount);
                MaxSidCount += ValueCount;
            }


            NtClose(MemberKeyHandle);
        }

        LocalFree(MemberSidString);
    }

    TRACE("Maximum sid count: %lu\n", MaxSidCount);
    RidArray = midl_user_allocate(MaxSidCount * sizeof(ULONG));
    if (RidArray == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    for (i = 0; i < SidArray->Count; i++)
    {
        ConvertSidToStringSid(SidArray->Sids[i].SidPointer, &MemberSidString);
TRACE("Open %S\n", MemberSidString);

        Status = SampRegOpenKey(MembersKeyHandle,
                                MemberSidString,
                                KEY_READ,
                                &MemberKeyHandle);
        TRACE("SampRegOpenKey returned %08lX\n", Status);
        if (NT_SUCCESS(Status))
        {
            Status = SampRegQueryKeyInfo(MemberKeyHandle,
                                         NULL,
                                         &ValueCount);
            if (NT_SUCCESS(Status))
            {
                TRACE("Found %lu values\n", ValueCount);

                for (j = 0; j < ValueCount; j++)
                {
                    DataLength = sizeof(ULONG);
                    Status = SampRegEnumerateValue(MemberKeyHandle,
                                                   j,
                      NULL,
                      NULL,
                      NULL,
                      (PVOID)&RidArray[j],
                      &DataLength);
                }
            }

            NtClose(MemberKeyHandle);
        }

        LocalFree(MemberSidString);
    }


done:
    if (NT_SUCCESS(Status))
    {
        Membership->Count = MaxSidCount;
        Membership->Element = RidArray;
    }
    else
    {
        if (RidArray != NULL)
            midl_user_free(RidArray);
    }

    if (MembersKeyHandle != NULL)
        NtClose(MembersKeyHandle);

    if (MembersKeyHandle != NULL)
        NtClose(MembersKeyHandle);

    if (AliasesKeyHandle != NULL)
        NtClose(AliasesKeyHandle);

    return Status;
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
              IN ULONG AliasId,
              OUT SAMPR_HANDLE *AliasHandle)
{
    PSAM_DB_OBJECT DomainObject;
    PSAM_DB_OBJECT AliasObject;
    WCHAR szRid[9];
    NTSTATUS Status;

    TRACE("SamrOpenAlias(%p %lx %lx %p)\n",
          DomainHandle, DesiredAccess, AliasId, AliasHandle);

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
    swprintf(szRid, L"%08lX", AliasId);

    /* Create the alias object */
    Status = SampOpenDbObject(DomainObject,
                              L"Aliases",
                              szRid,
                              SamDbAliasObject,
                              DesiredAccess,
                              &AliasObject);
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

    *AliasHandle = (SAMPR_HANDLE)AliasObject;

    return STATUS_SUCCESS;
}


static NTSTATUS
SampQueryAliasGeneral(PSAM_DB_OBJECT AliasObject,
                      PSAMPR_ALIAS_INFO_BUFFER *Buffer)
{
    PSAMPR_ALIAS_INFO_BUFFER InfoBuffer = NULL;
    HANDLE MembersKeyHandle = NULL;
    ULONG NameLength = 0;
    ULONG DescriptionLength = 0;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_ALIAS_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = SampGetObjectAttribute(AliasObject,
                                    L"Name",
                                    NULL,
                                    NULL,
                                    &NameLength);
    TRACE("Status 0x%08lx\n", Status);
    if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW)
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    InfoBuffer->General.Name.Length = NameLength - sizeof(WCHAR);
    InfoBuffer->General.Name.MaximumLength = NameLength;
    InfoBuffer->General.Name.Buffer = midl_user_allocate(NameLength);
    if (InfoBuffer->General.Name.Buffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    TRACE("Name Length: %lu\n", NameLength);
    Status = SampGetObjectAttribute(AliasObject,
                                    L"Name",
                                    NULL,
                                    (PVOID)InfoBuffer->General.Name.Buffer,
                                    &NameLength);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    Status = SampGetObjectAttribute(AliasObject,
                                    L"Description",
                                    NULL,
                                    NULL,
                                    &DescriptionLength);
    if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW)
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    InfoBuffer->General.AdminComment.Length = DescriptionLength - sizeof(WCHAR);
    InfoBuffer->General.AdminComment.MaximumLength = DescriptionLength;
    InfoBuffer->General.AdminComment.Buffer = midl_user_allocate(DescriptionLength);
    if (InfoBuffer->General.AdminComment.Buffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    TRACE("Description Length: %lu\n", DescriptionLength);
    Status = SampGetObjectAttribute(AliasObject,
                                    L"Description",
                                    NULL,
                                    (PVOID)InfoBuffer->General.AdminComment.Buffer,
                                    &DescriptionLength);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    /* Open the Members subkey */
    Status = SampRegOpenKey(AliasObject->KeyHandle,
                            L"Members",
                            KEY_READ,
                            &MembersKeyHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    /* Retrieve the number of members of the alias */
    Status = SampRegQueryKeyInfo(MembersKeyHandle,
                                 NULL,
                                 &InfoBuffer->General.MemberCount);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    *Buffer = InfoBuffer;

done:
    if (MembersKeyHandle != NULL)
        SampRegCloseKey(MembersKeyHandle);

    if (!NT_SUCCESS(Status))
    {
        if (InfoBuffer != NULL)
        {
            if (InfoBuffer->General.Name.Buffer != NULL)
                midl_user_free(InfoBuffer->General.Name.Buffer);

            if (InfoBuffer->General.AdminComment.Buffer != NULL)
                midl_user_free(InfoBuffer->General.AdminComment.Buffer);

            midl_user_free(InfoBuffer);
        }
    }

    return Status;
}


static NTSTATUS
SampQueryAliasName(PSAM_DB_OBJECT AliasObject,
                   PSAMPR_ALIAS_INFO_BUFFER *Buffer)
{
    PSAMPR_ALIAS_INFO_BUFFER InfoBuffer = NULL;
    ULONG Length = 0;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_ALIAS_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = SampGetObjectAttribute(AliasObject,
                                    L"Name",
                                    NULL,
                                    NULL,
                                    &Length);
    if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW)
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    InfoBuffer->Name.Name.Length = Length - sizeof(WCHAR);
    InfoBuffer->Name.Name.MaximumLength = Length;
    InfoBuffer->Name.Name.Buffer = midl_user_allocate(Length);
    if (InfoBuffer->Name.Name.Buffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    TRACE("Length: %lu\n", Length);
    Status = SampGetObjectAttribute(AliasObject,
                                    L"Name",
                                    NULL,
                                    (PVOID)InfoBuffer->Name.Name.Buffer,
                                    &Length);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    *Buffer = InfoBuffer;

done:
    if (!NT_SUCCESS(Status))
    {
        if (InfoBuffer != NULL)
        {
            if (InfoBuffer->Name.Name.Buffer != NULL)
                midl_user_free(InfoBuffer->Name.Name.Buffer);

            midl_user_free(InfoBuffer);
        }
    }

    return Status;
}


static NTSTATUS
SampQueryAliasAdminComment(PSAM_DB_OBJECT AliasObject,
                           PSAMPR_ALIAS_INFO_BUFFER *Buffer)
{
    PSAMPR_ALIAS_INFO_BUFFER InfoBuffer = NULL;
    ULONG Length = 0;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_ALIAS_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = SampGetObjectAttribute(AliasObject,
                                    L"Description",
                                    NULL,
                                    NULL,
                                    &Length);
    if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW)
        goto done;

    InfoBuffer->AdminComment.AdminComment.Length = Length - sizeof(WCHAR);
    InfoBuffer->AdminComment.AdminComment.MaximumLength = Length;
    InfoBuffer->AdminComment.AdminComment.Buffer = midl_user_allocate(Length);
    if (InfoBuffer->AdminComment.AdminComment.Buffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    Status = SampGetObjectAttribute(AliasObject,
                                    L"Description",
                                    NULL,
                                    (PVOID)InfoBuffer->AdminComment.AdminComment.Buffer,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    *Buffer = InfoBuffer;

done:
    if (!NT_SUCCESS(Status))
    {
        if (InfoBuffer != NULL)
        {
            if (InfoBuffer->AdminComment.AdminComment.Buffer != NULL)
                midl_user_free(InfoBuffer->AdminComment.AdminComment.Buffer);

            midl_user_free(InfoBuffer);
        }
    }

    return Status;
}


/* Function 28 */
NTSTATUS
NTAPI
SamrQueryInformationAlias(IN SAMPR_HANDLE AliasHandle,
                          IN ALIAS_INFORMATION_CLASS AliasInformationClass,
                          OUT PSAMPR_ALIAS_INFO_BUFFER *Buffer)
{
    PSAM_DB_OBJECT AliasObject;
    NTSTATUS Status;

    TRACE("SamrQueryInformationAlias(%p %lu %p)\n",
          AliasHandle, AliasInformationClass, Buffer);

    /* Validate the alias handle */
    Status = SampValidateDbObject(AliasHandle,
                                  SamDbAliasObject,
                                  ALIAS_READ_INFORMATION,
                                  &AliasObject);
    if (!NT_SUCCESS(Status))
        return Status;

    switch (AliasInformationClass)
    {
        case AliasGeneralInformation:
            Status = SampQueryAliasGeneral(AliasObject,
                                           Buffer);
            break;

        case AliasNameInformation:
            Status = SampQueryAliasName(AliasObject,
                                        Buffer);
            break;

        case AliasAdminCommentInformation:
            Status = SampQueryAliasAdminComment(AliasObject,
                                                Buffer);
            break;

        default:
            Status = STATUS_INVALID_INFO_CLASS;
            break;
    }

    return Status;
}


static NTSTATUS
SampSetAliasName(PSAM_DB_OBJECT AliasObject,
                 PSAMPR_ALIAS_INFO_BUFFER Buffer)
{
    NTSTATUS Status;

    Status = SampSetObjectAttribute(AliasObject,
                                    L"Name",
                                    REG_SZ,
                                    Buffer->Name.Name.Buffer,
                                    Buffer->Name.Name.Length + sizeof(WCHAR));

    return Status;
}

static NTSTATUS
SampSetAliasAdminComment(PSAM_DB_OBJECT AliasObject,
                         PSAMPR_ALIAS_INFO_BUFFER Buffer)
{
    NTSTATUS Status;

    Status = SampSetObjectAttribute(AliasObject,
                                    L"Description",
                                    REG_SZ,
                                    Buffer->AdminComment.AdminComment.Buffer,
                                    Buffer->AdminComment.AdminComment.Length + sizeof(WCHAR));

    return Status;
}

/* Function 29 */
NTSTATUS
NTAPI
SamrSetInformationAlias(IN SAMPR_HANDLE AliasHandle,
                        IN ALIAS_INFORMATION_CLASS AliasInformationClass,
                        IN PSAMPR_ALIAS_INFO_BUFFER Buffer)
{
    PSAM_DB_OBJECT AliasObject;
    NTSTATUS Status;

    TRACE("SamrSetInformationAlias(%p %lu %p)\n",
          AliasHandle, AliasInformationClass, Buffer);

    /* Validate the alias handle */
    Status = SampValidateDbObject(AliasHandle,
                                  SamDbAliasObject,
                                  ALIAS_WRITE_ACCOUNT,
                                  &AliasObject);
    if (!NT_SUCCESS(Status))
        return Status;

    switch (AliasInformationClass)
    {
        case AliasNameInformation:
            Status = SampSetAliasName(AliasObject,
                                      Buffer);
            break;

        case AliasAdminCommentInformation:
            Status = SampSetAliasAdminComment(AliasObject,
                                              Buffer);
            break;

        default:
            Status = STATUS_INVALID_INFO_CLASS;
            break;
    }

    return Status;
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
    PSAM_DB_OBJECT AliasObject;
    LPWSTR MemberIdString = NULL;
    HANDLE MembersKeyHandle = NULL;
    HANDLE MemberKeyHandle = NULL;
    ULONG MemberIdLength;
    NTSTATUS Status;

    TRACE("SamrAddMemberToAlias(%p %p)\n",
          AliasHandle, MemberId);

    /* Validate the domain handle */
    Status = SampValidateDbObject(AliasHandle,
                                  SamDbAliasObject,
                                  ALIAS_ADD_MEMBER,
                                  &AliasObject);
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

    ConvertSidToStringSidW(MemberId, &MemberIdString);
    TRACE("Member SID: %S\n", MemberIdString);

    MemberIdLength = RtlLengthSid(MemberId);

    Status = SampRegCreateKey(AliasObject->KeyHandle,
                              L"Members",
                              KEY_WRITE,
                              &MembersKeyHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SampRegCreateKey failed with status 0x%08lx\n", Status);
        goto done;
    }

    Status = SampRegSetValue(MembersKeyHandle,
                             MemberIdString,
                             REG_BINARY,
                             MemberId,
                             MemberIdLength);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SampRegSetValue failed with status 0x%08lx\n", Status);
        goto done;
    }

    Status = SampRegCreateKey(AliasObject->MembersKeyHandle,
                              MemberIdString,
                              KEY_WRITE,
                              &MemberKeyHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SampRegCreateKey failed with status 0x%08lx\n", Status);
        goto done;
    }

    Status = SampRegSetValue(MemberKeyHandle,
                             AliasObject->Name,
                             REG_BINARY,
                             MemberId,
                             MemberIdLength);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SampRegSetValue failed with status 0x%08lx\n", Status);
        goto done;
    }

done:
    if (MemberKeyHandle != NULL)
        SampRegCloseKey(MemberKeyHandle);

    if (MembersKeyHandle != NULL)
        SampRegCloseKey(MembersKeyHandle);

    if (MemberIdString != NULL)
        LocalFree(MemberIdString);

    return Status;
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
    PSAM_DB_OBJECT AliasObject;
    HANDLE MembersKeyHandle = NULL;
    PSAMPR_SID_INFORMATION MemberArray = NULL;
    ULONG ValueCount = 0;
    ULONG DataLength;
    ULONG Index;
    NTSTATUS Status;

    TRACE("SamrGetMembersInAlias(%p %p %p)\n",
          AliasHandle, Members);

    /* Validate the alias handle */
    Status = SampValidateDbObject(AliasHandle,
                                  SamDbAliasObject,
                                  ALIAS_LIST_MEMBERS,
                                  &AliasObject);
    if (!NT_SUCCESS(Status))
    {
        ERR("failed with status 0x%08lx\n", Status);
        return Status;
    }

    /* Open the members key of the alias objct */
    Status = SampRegOpenKey(AliasObject->KeyHandle,
                            L"Members",
                            KEY_READ,
                            &MembersKeyHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("SampRegOpenKey failed with status 0x%08lx\n", Status);
        return Status;
    }

    /* Get the number of members */
    Status = SampRegQueryKeyInfo(MembersKeyHandle,
                                 NULL,
                                 &ValueCount);
    if (!NT_SUCCESS(Status))
    {
        ERR("SampRegQueryKeyInfo failed with status 0x%08lx\n", Status);
        goto done;
    }

    /* Allocate the member array */
    MemberArray = midl_user_allocate(ValueCount * sizeof(SAMPR_SID_INFORMATION));
    if (MemberArray == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    /* Enumerate the members */
    Index = 0;
    while (TRUE)
    {
        /* Get the size of the next SID */
        DataLength = 0;
        Status = SampRegEnumerateValue(MembersKeyHandle,
                                       Index,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &DataLength);
        if (!NT_SUCCESS(Status))
        {
            if (Status == STATUS_NO_MORE_ENTRIES)
                Status = STATUS_SUCCESS;
            break;
        }

        /* Allocate a buffer for the SID */
        MemberArray[Index].SidPointer = midl_user_allocate(DataLength);
        if (MemberArray[Index].SidPointer == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto done;
        }

        /* Read the SID into the buffer */
        Status = SampRegEnumerateValue(MembersKeyHandle,
                                       Index,
                                       NULL,
                                       NULL,
                                       NULL,
                                       (PVOID)MemberArray[Index].SidPointer,
                                       &DataLength);
        if (!NT_SUCCESS(Status))
        {
            goto done;
        }

        Index++;
    }

    /* Return the number of members and the member array */
    if (NT_SUCCESS(Status))
    {
        Members->Count = ValueCount;
        Members->Sids = MemberArray;
    }

done:
    /* Clean up the members array and the SID buffers if something failed */
    if (!NT_SUCCESS(Status))
    {
        if (MemberArray != NULL)
        {
            for (Index = 0; Index < ValueCount; Index++)
            {
                if (MemberArray[Index].SidPointer != NULL)
                    midl_user_free(MemberArray[Index].SidPointer);
            }

            midl_user_free(MemberArray);
        }
    }

    /* Close the members key */
    if (MembersKeyHandle != NULL)
        SampRegCloseKey(MembersKeyHandle);

    return Status;
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

/* Function 37 */
NTSTATUS
NTAPI
SamrSetInformationUser(IN SAMPR_HANDLE UserHandle,
                       IN USER_INFORMATION_CLASS UserInformationClass,
                       IN PSAMPR_USER_INFO_BUFFER Buffer)
{
    PSAM_DB_OBJECT UserObject;
    ACCESS_MASK DesiredAccess;
    NTSTATUS Status;

    TRACE("SamrSetInformationUser(%p %lu %p)\n",
          UserHandle, UserInformationClass, Buffer);

    switch (UserInformationClass)
    {
        case UserNameInformation:
        case UserAccountNameInformation:
        case UserFullNameInformation:
        case UserPrimaryGroupInformation:
        case UserHomeInformation:
        case UserScriptInformation:
        case UserProfileInformation:
        case UserAdminCommentInformation:
        case UserWorkStationsInformation:
        case UserControlInformation:
        case UserExpiresInformation:
            DesiredAccess = USER_WRITE_ACCOUNT;
            break;

        case UserSetPasswordInformation:
            DesiredAccess = USER_FORCE_PASSWORD_CHANGE;
            break;

        default:
            return STATUS_INVALID_INFO_CLASS;
    }

    /* Validate the domain handle */
    Status = SampValidateDbObject(UserHandle,
                                  SamDbUserObject,
                                  DesiredAccess,
                                  &UserObject);
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

    switch (UserInformationClass)
    {
//        case UserGeneralInformation:
//        case UserPreferencesInformation:
//        case UserLogonHoursInformation:

        case UserNameInformation:
            Status = SampSetObjectAttribute(UserObject,
                                            L"Name",
                                            REG_SZ,
                                            Buffer->Name.UserName.Buffer,
                                            Buffer->Name.UserName.MaximumLength);
            if (!NT_SUCCESS(Status))
                break;

            Status = SampSetObjectAttribute(UserObject,
                                            L"FullName",
                                            REG_SZ,
                                            Buffer->Name.FullName.Buffer,
                                            Buffer->Name.FullName.MaximumLength);
            break;

        case UserAccountNameInformation:
            Status = SampSetObjectAttribute(UserObject,
                                            L"Name",
                                            REG_SZ,
                                            Buffer->AccountName.UserName.Buffer,
                                            Buffer->AccountName.UserName.MaximumLength);
            break;

        case UserFullNameInformation:
            Status = SampSetObjectAttribute(UserObject,
                                            L"FullName",
                                            REG_SZ,
                                            Buffer->FullName.FullName.Buffer,
                                            Buffer->FullName.FullName.MaximumLength);
            break;

        case UserPrimaryGroupInformation:
            Status = SampSetObjectAttribute(UserObject,
                                            L"PrimaryGroupId",
                                            REG_DWORD,
                                            &Buffer->PrimaryGroup.PrimaryGroupId,
                                            sizeof(ULONG));
            break;

        case UserHomeInformation:
            Status = SampSetObjectAttribute(UserObject,
                                            L"HomeDirectory",
                                            REG_SZ,
                                            Buffer->Home.HomeDirectory.Buffer,
                                            Buffer->Home.HomeDirectory.MaximumLength);
            if (!NT_SUCCESS(Status))
                break;

            Status = SampSetObjectAttribute(UserObject,
                                            L"HomeDirectoryDrive",
                                            REG_SZ,
                                            Buffer->Home.HomeDirectoryDrive.Buffer,
                                            Buffer->Home.HomeDirectoryDrive.MaximumLength);
            break;

        case UserScriptInformation:
            Status = SampSetObjectAttribute(UserObject,
                                            L"ScriptPath",
                                            REG_SZ,
                                            Buffer->Script.ScriptPath.Buffer,
                                            Buffer->Script.ScriptPath.MaximumLength);
            break;

        case UserProfileInformation:
            Status = SampSetObjectAttribute(UserObject,
                                            L"ProfilePath",
                                            REG_SZ,
                                            Buffer->Profile.ProfilePath.Buffer,
                                            Buffer->Profile.ProfilePath.MaximumLength);
            break;

        case UserAdminCommentInformation:
            Status = SampSetObjectAttribute(UserObject,
                                            L"AdminComment",
                                            REG_SZ,
                                            Buffer->AdminComment.AdminComment.Buffer,
                                            Buffer->AdminComment.AdminComment.MaximumLength);
            break;

        case UserWorkStationsInformation:
            Status = SampSetObjectAttribute(UserObject,
                                            L"WorkStations",
                                            REG_SZ,
                                            Buffer->WorkStations.WorkStations.Buffer,
                                            Buffer->WorkStations.WorkStations.MaximumLength);
            break;

        case UserSetPasswordInformation:
            TRACE("Password: %S\n", Buffer->SetPassword.Password.Buffer);
            TRACE("PasswordExpired: %d\n", Buffer->SetPassword.PasswordExpired);

            Status = SampSetObjectAttribute(UserObject,
                                            L"Password",
                                            REG_SZ,
                                            Buffer->SetPassword.Password.Buffer,
                                            Buffer->SetPassword.Password.MaximumLength);
            break;

        case UserControlInformation:
            Status = SampSetObjectAttribute(UserObject,
                                            L"UserAccountControl",
                                            REG_DWORD,
                                            &Buffer->Control.UserAccountControl,
                                            sizeof(ULONG));
            break;

        case UserExpiresInformation:
            Status = SampSetObjectAttribute(UserObject,
                                            L"AccountExpires",
                                            REG_BINARY,
                                            &Buffer->Expires.AccountExpires,
                                            sizeof(OLD_LARGE_INTEGER));
            break;

//        case UserInternal1Information:
//        case UserParametersInformation:
//        case UserAllInformation:
//        case UserInternal4Information:
//        case UserInternal5Information:
//        case UserInternal4InformationNew:
//        case UserInternal5InformationNew:

        default:
            Status = STATUS_INVALID_INFO_CLASS;
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
