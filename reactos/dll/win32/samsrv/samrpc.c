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
SampQueryDomainPassword(PSAM_DB_OBJECT DomainObject,
                        PSAMPR_DOMAIN_INFO_BUFFER *Buffer)
{
    PSAMPR_DOMAIN_INFO_BUFFER InfoBuffer = NULL;
    SAM_DOMAIN_FIXED_DATA FixedData;
    ULONG Length = 0;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_DOMAIN_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Length = sizeof(SAM_DOMAIN_FIXED_DATA);
    Status = SampGetObjectAttribute(DomainObject,
                                    L"F",
                                    NULL,
                                    (PVOID)&FixedData,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    InfoBuffer->Password.MinPasswordLength = FixedData.MinPasswordLength;
    InfoBuffer->Password.PasswordHistoryLength = FixedData.PasswordHistoryLength;
    InfoBuffer->Password.PasswordProperties = FixedData.PasswordProperties;
    InfoBuffer->Password.MaxPasswordAge.LowPart = FixedData.MaxPasswordAge.LowPart;
    InfoBuffer->Password.MaxPasswordAge.HighPart = FixedData.MaxPasswordAge.HighPart;
    InfoBuffer->Password.MinPasswordAge.LowPart = FixedData.MinPasswordAge.LowPart;
    InfoBuffer->Password.MinPasswordAge.HighPart = FixedData.MinPasswordAge.HighPart;

    *Buffer = InfoBuffer;

done:
    if (!NT_SUCCESS(Status))
    {
        if (InfoBuffer != NULL)
        {
            midl_user_free(InfoBuffer);
        }
    }

    return Status;
}


static NTSTATUS
SampQueryDomainGeneral(PSAM_DB_OBJECT DomainObject,
                       PSAMPR_DOMAIN_INFO_BUFFER *Buffer)
{
    PSAMPR_DOMAIN_INFO_BUFFER InfoBuffer = NULL;
    SAM_DOMAIN_FIXED_DATA FixedData;
    ULONG Length = 0;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_DOMAIN_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Length = sizeof(SAM_DOMAIN_FIXED_DATA);
    Status = SampGetObjectAttribute(DomainObject,
                                    L"F",
                                    NULL,
                                    (PVOID)&FixedData,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    InfoBuffer->General.ForceLogoff.LowPart = FixedData.ForceLogoff.LowPart;
    InfoBuffer->General.ForceLogoff.HighPart = FixedData.ForceLogoff.HighPart;
    InfoBuffer->General.DomainModifiedCount.LowPart = FixedData.DomainModifiedCount.LowPart;
    InfoBuffer->General.DomainModifiedCount.HighPart = FixedData.DomainModifiedCount.HighPart;
    InfoBuffer->General.DomainServerState = FixedData.DomainServerState;
    InfoBuffer->General.DomainServerRole = FixedData.DomainServerRole;
    InfoBuffer->General.UasCompatibilityRequired = FixedData.UasCompatibilityRequired;

    /* Get the OemInformation string */
    Status = SampGetObjectAttributeString(DomainObject,
                                          L"OemInformation",
                                          &InfoBuffer->General.OemInformation);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    /* Get the Name string */
    Status = SampGetObjectAttributeString(DomainObject,
                                          L"Name",
                                          &InfoBuffer->General.DomainName);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    /* Get the ReplicaSourceNodeName string */
    Status = SampGetObjectAttributeString(DomainObject,
                                          L"ReplicaSourceNodeName",
                                          &InfoBuffer->General.ReplicaSourceNodeName);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    InfoBuffer->General.UserCount = 0;  /* FIXME */
    InfoBuffer->General.GroupCount = 0; /* FIXME */
    InfoBuffer->General.AliasCount = 0; /* FIXME */

    *Buffer = InfoBuffer;

done:
    if (!NT_SUCCESS(Status))
    {
        if (InfoBuffer != NULL)
        {
            if (InfoBuffer->General.OemInformation.Buffer != NULL)
                midl_user_free(InfoBuffer->General.OemInformation.Buffer);

            if (InfoBuffer->General.DomainName.Buffer != NULL)
                midl_user_free(InfoBuffer->General.DomainName.Buffer);

            if (InfoBuffer->General.ReplicaSourceNodeName.Buffer != NULL)
                midl_user_free(InfoBuffer->General.ReplicaSourceNodeName.Buffer);

            midl_user_free(InfoBuffer);
        }
    }

    return Status;
}


static NTSTATUS
SampQueryDomainLogoff(PSAM_DB_OBJECT DomainObject,
                      PSAMPR_DOMAIN_INFO_BUFFER *Buffer)
{
    PSAMPR_DOMAIN_INFO_BUFFER InfoBuffer = NULL;
    SAM_DOMAIN_FIXED_DATA FixedData;
    ULONG Length = 0;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_DOMAIN_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Length = sizeof(SAM_DOMAIN_FIXED_DATA);
    Status = SampGetObjectAttribute(DomainObject,
                                    L"F",
                                    NULL,
                                    (PVOID)&FixedData,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    InfoBuffer->Logoff.ForceLogoff.LowPart = FixedData.ForceLogoff.LowPart;
    InfoBuffer->Logoff.ForceLogoff.HighPart = FixedData.ForceLogoff.HighPart;

    *Buffer = InfoBuffer;

done:
    if (!NT_SUCCESS(Status))
    {
        if (InfoBuffer != NULL)
        {
            midl_user_free(InfoBuffer);
        }
    }

    return Status;
}


static NTSTATUS
SampQueryDomainOem(PSAM_DB_OBJECT DomainObject,
                   PSAMPR_DOMAIN_INFO_BUFFER *Buffer)
{
    PSAMPR_DOMAIN_INFO_BUFFER InfoBuffer = NULL;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_DOMAIN_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Get the OemInformation string */
    Status = SampGetObjectAttributeString(DomainObject,
                                          L"OemInformation",
                                          &InfoBuffer->Oem.OemInformation);
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
            if (InfoBuffer->Oem.OemInformation.Buffer != NULL)
                midl_user_free(InfoBuffer->Oem.OemInformation.Buffer);

            midl_user_free(InfoBuffer);
        }
    }

    return Status;
}


static NTSTATUS
SampQueryDomainName(PSAM_DB_OBJECT DomainObject,
                    PSAMPR_DOMAIN_INFO_BUFFER *Buffer)
{
    PSAMPR_DOMAIN_INFO_BUFFER InfoBuffer = NULL;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_DOMAIN_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Get the Name string */
    Status = SampGetObjectAttributeString(DomainObject,
                                          L"Name",
                                          &InfoBuffer->Name.DomainName);
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
            if (InfoBuffer->Name.DomainName.Buffer != NULL)
                midl_user_free(InfoBuffer->Name.DomainName.Buffer);

            midl_user_free(InfoBuffer);
        }
    }

    return Status;
}


static NTSTATUS
SampQueryDomainReplication(PSAM_DB_OBJECT DomainObject,
                           PSAMPR_DOMAIN_INFO_BUFFER *Buffer)
{
    PSAMPR_DOMAIN_INFO_BUFFER InfoBuffer = NULL;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_DOMAIN_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Get the ReplicaSourceNodeName string */
    Status = SampGetObjectAttributeString(DomainObject,
                                          L"ReplicaSourceNodeName",
                                          &InfoBuffer->Replication.ReplicaSourceNodeName);
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
            if (InfoBuffer->Replication.ReplicaSourceNodeName.Buffer != NULL)
                midl_user_free(InfoBuffer->Replication.ReplicaSourceNodeName.Buffer);

            midl_user_free(InfoBuffer);
        }
    }

    return Status;
}


static NTSTATUS
SampQueryDomainServerRole(PSAM_DB_OBJECT DomainObject,
                          PSAMPR_DOMAIN_INFO_BUFFER *Buffer)
{
    PSAMPR_DOMAIN_INFO_BUFFER InfoBuffer = NULL;
    SAM_DOMAIN_FIXED_DATA FixedData;
    ULONG Length = 0;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_DOMAIN_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Length = sizeof(SAM_DOMAIN_FIXED_DATA);
    Status = SampGetObjectAttribute(DomainObject,
                                    L"F",
                                    NULL,
                                    (PVOID)&FixedData,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    InfoBuffer->Role.DomainServerRole = FixedData.DomainServerRole;

    *Buffer = InfoBuffer;

done:
    if (!NT_SUCCESS(Status))
    {
        if (InfoBuffer != NULL)
        {
            midl_user_free(InfoBuffer);
        }
    }

    return Status;
}


static NTSTATUS
SampQueryDomainModified(PSAM_DB_OBJECT DomainObject,
                        PSAMPR_DOMAIN_INFO_BUFFER *Buffer)
{
    PSAMPR_DOMAIN_INFO_BUFFER InfoBuffer = NULL;
    SAM_DOMAIN_FIXED_DATA FixedData;
    ULONG Length = 0;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_DOMAIN_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Length = sizeof(SAM_DOMAIN_FIXED_DATA);
    Status = SampGetObjectAttribute(DomainObject,
                                    L"F",
                                    NULL,
                                    (PVOID)&FixedData,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    InfoBuffer->Modified.DomainModifiedCount.LowPart = FixedData.DomainModifiedCount.LowPart;
    InfoBuffer->Modified.DomainModifiedCount.HighPart = FixedData.DomainModifiedCount.HighPart;
    InfoBuffer->Modified.CreationTime.LowPart = FixedData.CreationTime.LowPart;
    InfoBuffer->Modified.CreationTime.HighPart = FixedData.CreationTime.HighPart;

    *Buffer = InfoBuffer;

done:
    if (!NT_SUCCESS(Status))
    {
        if (InfoBuffer != NULL)
        {
            midl_user_free(InfoBuffer);
        }
    }

    return Status;
}


static NTSTATUS
SampQueryDomainState(PSAM_DB_OBJECT DomainObject,
                     PSAMPR_DOMAIN_INFO_BUFFER *Buffer)
{
    PSAMPR_DOMAIN_INFO_BUFFER InfoBuffer = NULL;
    SAM_DOMAIN_FIXED_DATA FixedData;
    ULONG Length = 0;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_DOMAIN_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Length = sizeof(SAM_DOMAIN_FIXED_DATA);
    Status = SampGetObjectAttribute(DomainObject,
                                    L"F",
                                    NULL,
                                    (PVOID)&FixedData,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    InfoBuffer->State.DomainServerState = FixedData.DomainServerState;

    *Buffer = InfoBuffer;

done:
    if (!NT_SUCCESS(Status))
    {
        if (InfoBuffer != NULL)
        {
            midl_user_free(InfoBuffer);
        }
    }

    return Status;
}


static NTSTATUS
SampQueryDomainGeneral2(PSAM_DB_OBJECT DomainObject,
                        PSAMPR_DOMAIN_INFO_BUFFER *Buffer)
{
    PSAMPR_DOMAIN_INFO_BUFFER InfoBuffer = NULL;
    SAM_DOMAIN_FIXED_DATA FixedData;
    ULONG Length = 0;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_DOMAIN_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Length = sizeof(SAM_DOMAIN_FIXED_DATA);
    Status = SampGetObjectAttribute(DomainObject,
                                    L"F",
                                    NULL,
                                    (PVOID)&FixedData,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    InfoBuffer->General2.I1.ForceLogoff.LowPart = FixedData.ForceLogoff.LowPart;
    InfoBuffer->General2.I1.ForceLogoff.HighPart = FixedData.ForceLogoff.HighPart;
    InfoBuffer->General2.I1.DomainModifiedCount.LowPart = FixedData.DomainModifiedCount.LowPart;
    InfoBuffer->General2.I1.DomainModifiedCount.HighPart = FixedData.DomainModifiedCount.HighPart;
    InfoBuffer->General2.I1.DomainServerState = FixedData.DomainServerState;
    InfoBuffer->General2.I1.DomainServerRole = FixedData.DomainServerRole;
    InfoBuffer->General2.I1.UasCompatibilityRequired = FixedData.UasCompatibilityRequired;

    InfoBuffer->General2.LockoutDuration = FixedData.LockoutDuration;
    InfoBuffer->General2.LockoutObservationWindow = FixedData.LockoutObservationWindow;
    InfoBuffer->General2.LockoutThreshold = FixedData.LockoutThreshold;

    /* Get the OemInformation string */
    Status = SampGetObjectAttributeString(DomainObject,
                                          L"OemInformation",
                                          &InfoBuffer->General2.I1.OemInformation);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    /* Get the Name string */
    Status = SampGetObjectAttributeString(DomainObject,
                                          L"Name",
                                          &InfoBuffer->General2.I1.DomainName);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    /* Get the ReplicaSourceNodeName string */
    Status = SampGetObjectAttributeString(DomainObject,
                                          L"ReplicaSourceNodeName",
                                          &InfoBuffer->General2.I1.ReplicaSourceNodeName);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    InfoBuffer->General2.I1.UserCount = 0;  /* FIXME */
    InfoBuffer->General2.I1.GroupCount = 0; /* FIXME */
    InfoBuffer->General2.I1.AliasCount = 0; /* FIXME */

    *Buffer = InfoBuffer;

done:
    if (!NT_SUCCESS(Status))
    {
        if (InfoBuffer != NULL)
        {
            if (InfoBuffer->General2.I1.OemInformation.Buffer != NULL)
                midl_user_free(InfoBuffer->General2.I1.OemInformation.Buffer);

            if (InfoBuffer->General2.I1.DomainName.Buffer != NULL)
                midl_user_free(InfoBuffer->General2.I1.DomainName.Buffer);

            if (InfoBuffer->General2.I1.ReplicaSourceNodeName.Buffer != NULL)
                midl_user_free(InfoBuffer->General2.I1.ReplicaSourceNodeName.Buffer);

            midl_user_free(InfoBuffer);
        }
    }

    return Status;
}


static NTSTATUS
SampQueryDomainLockout(PSAM_DB_OBJECT DomainObject,
                       PSAMPR_DOMAIN_INFO_BUFFER *Buffer)
{
    PSAMPR_DOMAIN_INFO_BUFFER InfoBuffer = NULL;
    SAM_DOMAIN_FIXED_DATA FixedData;
    ULONG Length = 0;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_DOMAIN_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Length = sizeof(SAM_DOMAIN_FIXED_DATA);
    Status = SampGetObjectAttribute(DomainObject,
                                    L"F",
                                    NULL,
                                    (PVOID)&FixedData,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    InfoBuffer->Lockout.LockoutDuration = FixedData.LockoutDuration;
    InfoBuffer->Lockout.LockoutObservationWindow = FixedData.LockoutObservationWindow;
    InfoBuffer->Lockout.LockoutThreshold = FixedData.LockoutThreshold;

    *Buffer = InfoBuffer;

done:
    if (!NT_SUCCESS(Status))
    {
        if (InfoBuffer != NULL)
        {
            midl_user_free(InfoBuffer);
        }
    }

    return Status;
}


static NTSTATUS
SampQueryDomainModified2(PSAM_DB_OBJECT DomainObject,
                        PSAMPR_DOMAIN_INFO_BUFFER *Buffer)
{
    PSAMPR_DOMAIN_INFO_BUFFER InfoBuffer = NULL;
    SAM_DOMAIN_FIXED_DATA FixedData;
    ULONG Length = 0;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_DOMAIN_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Length = sizeof(SAM_DOMAIN_FIXED_DATA);
    Status = SampGetObjectAttribute(DomainObject,
                                    L"F",
                                    NULL,
                                    (PVOID)&FixedData,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    InfoBuffer->Modified2.DomainModifiedCount.LowPart = FixedData.DomainModifiedCount.LowPart;
    InfoBuffer->Modified2.DomainModifiedCount.HighPart = FixedData.DomainModifiedCount.HighPart;
    InfoBuffer->Modified2.CreationTime.LowPart = FixedData.CreationTime.LowPart;
    InfoBuffer->Modified2.CreationTime.HighPart = FixedData.CreationTime.HighPart;
    InfoBuffer->Modified2.ModifiedCountAtLastPromotion.LowPart = FixedData.ModifiedCountAtLastPromotion.LowPart;
    InfoBuffer->Modified2.ModifiedCountAtLastPromotion.HighPart = FixedData.ModifiedCountAtLastPromotion.HighPart;

    *Buffer = InfoBuffer;

done:
    if (!NT_SUCCESS(Status))
    {
        if (InfoBuffer != NULL)
        {
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
    ACCESS_MASK DesiredAccess;
    NTSTATUS Status;

    TRACE("SamrQueryInformationDomain(%p %lu %p)\n",
          DomainHandle, DomainInformationClass, Buffer);

    switch (DomainInformationClass)
    {
        case DomainPasswordInformation:
        case DomainLockoutInformation:
            DesiredAccess = DOMAIN_READ_PASSWORD_PARAMETERS;
            break;

        case DomainGeneralInformation:
        case DomainLogoffInformation:
        case DomainOemInformation:
        case DomainNameInformation:
        case DomainReplicationInformation:
        case DomainServerRoleInformation:
        case DomainModifiedInformation:
        case DomainStateInformation:
        case DomainModifiedInformation2:
            DesiredAccess = DOMAIN_READ_OTHER_PARAMETERS;
            break;

        case DomainGeneralInformation2:
            DesiredAccess = DOMAIN_READ_PASSWORD_PARAMETERS |
                            DOMAIN_READ_OTHER_PARAMETERS;
            break;

        default:
            return STATUS_INVALID_INFO_CLASS;
    }

    /* Validate the server handle */
    Status = SampValidateDbObject(DomainHandle,
                                  SamDbDomainObject,
                                  DesiredAccess,
                                  &DomainObject);
    if (!NT_SUCCESS(Status))
        return Status;

    switch (DomainInformationClass)
    {
        case DomainPasswordInformation:
            Status = SampQueryDomainPassword(DomainObject,
                                             Buffer);
            break;

        case DomainGeneralInformation:
            Status = SampQueryDomainGeneral(DomainObject,
                                            Buffer);
            break;

        case DomainLogoffInformation:
            Status = SampQueryDomainLogoff(DomainObject,
                                           Buffer);
            break;

        case DomainOemInformation:
            Status = SampQueryDomainOem(DomainObject,
                                        Buffer);
            break;

        case DomainNameInformation:
            Status = SampQueryDomainName(DomainObject,
                                         Buffer);
            break;

        case DomainReplicationInformation:
            Status = SampQueryDomainReplication(DomainObject,
                                                Buffer);
            break;

        case DomainServerRoleInformation:
            Status = SampQueryDomainServerRole(DomainObject,
                                               Buffer);
            break;

        case DomainModifiedInformation:
            Status = SampQueryDomainModified(DomainObject,
                                             Buffer);
            break;

        case DomainStateInformation:
            Status = SampQueryDomainState(DomainObject,
                                          Buffer);
            break;

        case DomainGeneralInformation2:
            Status = SampQueryDomainGeneral2(DomainObject,
                                             Buffer);
            break;

        case DomainLockoutInformation:
            Status = SampQueryDomainLockout(DomainObject,
                                            Buffer);
            break;

        case DomainModifiedInformation2:
            Status = SampQueryDomainModified2(DomainObject,
                                              Buffer);
            break;

        default:
            Status = STATUS_NOT_IMPLEMENTED;
    }

    return Status;
}


static NTSTATUS
SampSetDomainPassword(PSAM_DB_OBJECT DomainObject,
                      PSAMPR_DOMAIN_INFO_BUFFER Buffer)
{
    SAM_DOMAIN_FIXED_DATA FixedData;
    ULONG Length = 0;
    NTSTATUS Status;

    Length = sizeof(SAM_DOMAIN_FIXED_DATA);
    Status = SampGetObjectAttribute(DomainObject,
                                    L"F",
                                    NULL,
                                    (PVOID)&FixedData,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    FixedData.MinPasswordLength = Buffer->Password.MinPasswordLength;
    FixedData.PasswordHistoryLength = Buffer->Password.PasswordHistoryLength;
    FixedData.PasswordProperties = Buffer->Password.PasswordProperties;
    FixedData.MaxPasswordAge.LowPart = Buffer->Password.MaxPasswordAge.LowPart;
    FixedData.MaxPasswordAge.HighPart = Buffer->Password.MaxPasswordAge.HighPart;
    FixedData.MinPasswordAge.LowPart = Buffer->Password.MinPasswordAge.LowPart;
    FixedData.MinPasswordAge.HighPart = Buffer->Password.MinPasswordAge.HighPart;

    Status = SampSetObjectAttribute(DomainObject,
                                    L"F",
                                    REG_BINARY,
                                    &FixedData,
                                    Length);

done:
    return Status;
}


static NTSTATUS
SampSetDomainLogoff(PSAM_DB_OBJECT DomainObject,
                    PSAMPR_DOMAIN_INFO_BUFFER Buffer)
{
    SAM_DOMAIN_FIXED_DATA FixedData;
    ULONG Length = 0;
    NTSTATUS Status;

    Length = sizeof(SAM_DOMAIN_FIXED_DATA);
    Status = SampGetObjectAttribute(DomainObject,
                                    L"F",
                                    NULL,
                                    (PVOID)&FixedData,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    FixedData.ForceLogoff.LowPart = Buffer->Logoff.ForceLogoff.LowPart;
    FixedData.ForceLogoff.HighPart = Buffer->Logoff.ForceLogoff.HighPart;

    Status = SampSetObjectAttribute(DomainObject,
                                    L"F",
                                    REG_BINARY,
                                    &FixedData,
                                    Length);

done:
    return Status;
}


static NTSTATUS
SampSetDomainServerRole(PSAM_DB_OBJECT DomainObject,
                        PSAMPR_DOMAIN_INFO_BUFFER Buffer)
{
    SAM_DOMAIN_FIXED_DATA FixedData;
    ULONG Length = 0;
    NTSTATUS Status;

    Length = sizeof(SAM_DOMAIN_FIXED_DATA);
    Status = SampGetObjectAttribute(DomainObject,
                                    L"F",
                                    NULL,
                                    (PVOID)&FixedData,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    FixedData.DomainServerRole = Buffer->Role.DomainServerRole;

    Status = SampSetObjectAttribute(DomainObject,
                                    L"F",
                                    REG_BINARY,
                                    &FixedData,
                                    Length);

done:
    return Status;
}


static NTSTATUS
SampSetDomainState(PSAM_DB_OBJECT DomainObject,
                   PSAMPR_DOMAIN_INFO_BUFFER Buffer)
{
    SAM_DOMAIN_FIXED_DATA FixedData;
    ULONG Length = 0;
    NTSTATUS Status;

    Length = sizeof(SAM_DOMAIN_FIXED_DATA);
    Status = SampGetObjectAttribute(DomainObject,
                                    L"F",
                                    NULL,
                                    (PVOID)&FixedData,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    FixedData.DomainServerState = Buffer->State.DomainServerState;

    Status = SampSetObjectAttribute(DomainObject,
                                    L"F",
                                    REG_BINARY,
                                    &FixedData,
                                    Length);

done:
    return Status;
}


static NTSTATUS
SampSetDomainLockout(PSAM_DB_OBJECT DomainObject,
                     PSAMPR_DOMAIN_INFO_BUFFER Buffer)
{
    SAM_DOMAIN_FIXED_DATA FixedData;
    ULONG Length = 0;
    NTSTATUS Status;

    Length = sizeof(SAM_DOMAIN_FIXED_DATA);
    Status = SampGetObjectAttribute(DomainObject,
                                    L"F",
                                    NULL,
                                    (PVOID)&FixedData,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    FixedData.LockoutDuration = Buffer->Lockout.LockoutDuration;
    FixedData.LockoutObservationWindow = Buffer->Lockout.LockoutObservationWindow;
    FixedData.LockoutThreshold = Buffer->Lockout.LockoutThreshold;

    Status = SampSetObjectAttribute(DomainObject,
                                    L"F",
                                    REG_BINARY,
                                    &FixedData,
                                    Length);

done:
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
    ACCESS_MASK DesiredAccess;
    NTSTATUS Status;

    TRACE("SamrSetInformationDomain(%p %lu %p)\n",
          DomainHandle, DomainInformationClass, DomainInformation);

    switch (DomainInformationClass)
    {
        case DomainPasswordInformation:
        case DomainLockoutInformation:
            DesiredAccess = DOMAIN_WRITE_PASSWORD_PARAMS;
            break;

        case DomainLogoffInformation:
        case DomainOemInformation:
        case DomainNameInformation:
            DesiredAccess = DOMAIN_WRITE_OTHER_PARAMETERS;
            break;

        case DomainReplicationInformation:
        case DomainServerRoleInformation:
        case DomainStateInformation:
            DesiredAccess = DOMAIN_ADMINISTER_SERVER;
            break;

        default:
            return STATUS_INVALID_INFO_CLASS;
    }

    /* Validate the server handle */
    Status = SampValidateDbObject(DomainHandle,
                                  SamDbDomainObject,
                                  DesiredAccess,
                                  &DomainObject);
    if (!NT_SUCCESS(Status))
        return Status;

    switch (DomainInformationClass)
    {
        case DomainPasswordInformation:
            Status = SampSetDomainPassword(DomainObject,
                                           DomainInformation);
            break;

        case DomainLogoffInformation:
            Status = SampSetDomainLogoff(DomainObject,
                                         DomainInformation);
            break;

        case DomainOemInformation:
            Status = SampSetObjectAttribute(DomainObject,
                                            L"OemInformation",
                                            REG_SZ,
                                            DomainInformation->Oem.OemInformation.Buffer,
                                            DomainInformation->Oem.OemInformation.Length + sizeof(WCHAR));
            break;

        case DomainNameInformation:
            Status = SampSetObjectAttribute(DomainObject,
                                            L"Name",
                                            REG_SZ,
                                            DomainInformation->Name.DomainName.Buffer,
                                            DomainInformation->Name.DomainName.Length + sizeof(WCHAR));
            break;

        case DomainReplicationInformation:
            Status = SampSetObjectAttribute(DomainObject,
                                            L"ReplicaSourceNodeName",
                                            REG_SZ,
                                            DomainInformation->Replication.ReplicaSourceNodeName.Buffer,
                                            DomainInformation->Replication.ReplicaSourceNodeName.Length + sizeof(WCHAR));
            break;

        case DomainServerRoleInformation:
            Status = SampSetDomainServerRole(DomainObject,
                                             DomainInformation);
            break;

        case DomainStateInformation:
            Status = SampSetDomainState(DomainObject,
                                        DomainInformation);
            break;

        case DomainLockoutInformation:
            Status = SampSetDomainLockout(DomainObject,
                                          DomainInformation);
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
    UNICODE_STRING EmptyString = RTL_CONSTANT_STRING(L"");
    SAM_DOMAIN_FIXED_DATA FixedDomainData;
    SAM_GROUP_FIXED_DATA FixedGroupData;
    PSAM_DB_OBJECT DomainObject;
    PSAM_DB_OBJECT GroupObject;
    ULONG ulSize;
    ULONG ulRid;
    WCHAR szRid[9];
    NTSTATUS Status;

    TRACE("SamrCreateGroupInDomain(%p %p %lx %p %p)\n",
          DomainHandle, Name, DesiredAccess, GroupHandle, RelativeId);

    /* Validate the domain handle */
    Status = SampValidateDbObject(DomainHandle,
                                  SamDbDomainObject,
                                  DOMAIN_CREATE_GROUP,
                                  &DomainObject);
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

    /* Get the fixed domain attributes */
    ulSize = sizeof(SAM_DOMAIN_FIXED_DATA);
    Status = SampGetObjectAttribute(DomainObject,
                                    L"F",
                                    NULL,
                                    (PVOID)&FixedDomainData,
                                    &ulSize);
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

    /* Increment the NextRid attribute */
    ulRid = FixedDomainData.NextRid;
    FixedDomainData.NextRid++;

    /* Store the fixed domain attributes */
    Status = SampSetObjectAttribute(DomainObject,
                           L"F",
                           REG_BINARY,
                           &FixedDomainData,
                           ulSize);
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

    TRACE("RID: %lx\n", ulRid);

    /* Convert the RID into a string (hex) */
    swprintf(szRid, L"%08lX", ulRid);

    /* FIXME: Check whether the group name is already in use */

    /* Create the group object */
    Status = SampCreateDbObject(DomainObject,
                                L"Groups",
                                szRid,
                                SamDbGroupObject,
                                DesiredAccess,
                                &GroupObject);
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

    /* Add the name alias for the user object */
    Status = SampSetDbObjectNameAlias(DomainObject,
                                      L"Groups",
                                      Name->Buffer,
                                      ulRid);
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

    /* Initialize fixed user data */
    memset(&FixedGroupData, 0, sizeof(SAM_GROUP_FIXED_DATA));
    FixedGroupData.Version = 1;

    FixedGroupData.GroupId = ulRid;

    /* Set fixed user data attribute */
    Status = SampSetObjectAttribute(GroupObject,
                                    L"F",
                                    REG_BINARY,
                                    (LPVOID)&FixedGroupData,
                                    sizeof(SAM_GROUP_FIXED_DATA));
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

    /* Set the Name attribute */
    Status = SampSetObjectAttribute(GroupObject,
                                    L"Name",
                                    REG_SZ,
                                    (LPVOID)Name->Buffer,
                                    Name->MaximumLength);
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

    /* Set the AdminComment attribute */
    Status = SampSetObjectAttribute(GroupObject,
                                    L"AdminComment",
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
        *GroupHandle = (SAMPR_HANDLE)GroupObject;
        *RelativeId = ulRid;
    }

    TRACE("returns with status 0x%08lx\n", Status);

    return Status;
}


/* Function 11 */
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
    UNICODE_STRING EmptyString = RTL_CONSTANT_STRING(L"");
    SAM_DOMAIN_FIXED_DATA FixedDomainData;
    SAM_USER_FIXED_DATA FixedUserData;
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

    /* Get the fixed domain attributes */
    ulSize = sizeof(SAM_DOMAIN_FIXED_DATA);
    Status = SampGetObjectAttribute(DomainObject,
                                    L"F",
                                    NULL,
                                    (PVOID)&FixedDomainData,
                                    &ulSize);
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

    /* Increment the NextRid attribute */
    ulRid = FixedDomainData.NextRid;
    FixedDomainData.NextRid++;

    /* Store the fixed domain attributes */
    Status = SampSetObjectAttribute(DomainObject,
                           L"F",
                           REG_BINARY,
                           &FixedDomainData,
                           ulSize);
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

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

    /* Initialize fixed user data */
    memset(&FixedUserData, 0, sizeof(SAM_USER_FIXED_DATA));
    FixedUserData.Version = 1;

    FixedUserData.UserId = ulRid;

    /* Set fixed user data attribute */
    Status = SampSetObjectAttribute(UserObject,
                                    L"F",
                                    REG_BINARY,
                                    (LPVOID)&FixedUserData,
                                    sizeof(SAM_USER_FIXED_DATA));
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

    /* Set the Name attribute */
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

    /* Set the FullName attribute */
    Status = SampSetObjectAttribute(UserObject,
                                    L"FullName",
                                    REG_SZ,
                                    EmptyString.Buffer,
                                    EmptyString.MaximumLength);
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

    /* Set the HomeDirectory attribute */
    Status = SampSetObjectAttribute(UserObject,
                                    L"HomeDirectory",
                                    REG_SZ,
                                    EmptyString.Buffer,
                                    EmptyString.MaximumLength);
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

    /* Set the HomeDirectoryDrive attribute */
    Status = SampSetObjectAttribute(UserObject,
                                    L"HomeDirectoryDrive",
                                    REG_SZ,
                                    EmptyString.Buffer,
                                    EmptyString.MaximumLength);
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

    /* Set the ScriptPath attribute */
    Status = SampSetObjectAttribute(UserObject,
                                    L"ScriptPath",
                                    REG_SZ,
                                    EmptyString.Buffer,
                                    EmptyString.MaximumLength);
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

    /* Set the ProfilePath attribute */
    Status = SampSetObjectAttribute(UserObject,
                                    L"ProfilePath",
                                    REG_SZ,
                                    EmptyString.Buffer,
                                    EmptyString.MaximumLength);
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

    /* Set the AdminComment attribute */
    Status = SampSetObjectAttribute(UserObject,
                                    L"AdminComment",
                                    REG_SZ,
                                    EmptyString.Buffer,
                                    EmptyString.MaximumLength);
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

    /* Set the UserComment attribute */
    Status = SampSetObjectAttribute(UserObject,
                                    L"UserComment",
                                    REG_SZ,
                                    EmptyString.Buffer,
                                    EmptyString.MaximumLength);
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

    /* Set the WorkStations attribute */
    Status = SampSetObjectAttribute(UserObject,
                                    L"WorkStations",
                                    REG_SZ,
                                    EmptyString.Buffer,
                                    EmptyString.MaximumLength);
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
    SAM_DOMAIN_FIXED_DATA FixedDomainData;
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

    /* Get the fixed domain attributes */
    ulSize = sizeof(SAM_DOMAIN_FIXED_DATA);
    Status = SampGetObjectAttribute(DomainObject,
                                    L"F",
                                    NULL,
                                    (PVOID)&FixedDomainData,
                                    &ulSize);
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

    /* Increment the NextRid attribute */
    ulRid = FixedDomainData.NextRid;
    FixedDomainData.NextRid++;

    /* Store the fixed domain attributes */
    Status = SampSetObjectAttribute(DomainObject,
                           L"F",
                           REG_BINARY,
                           &FixedDomainData,
                           ulSize);
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

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

    /* Create the alias object */
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
    PSAM_DB_OBJECT DomainObject;
    PSAM_DB_OBJECT GroupObject;
    WCHAR szRid[9];
    NTSTATUS Status;

    TRACE("SamrOpenGroup(%p %lx %lx %p)\n",
          DomainHandle, DesiredAccess, GroupId, GroupHandle);

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
    swprintf(szRid, L"%08lX", GroupId);

    /* Create the group object */
    Status = SampOpenDbObject(DomainObject,
                              L"Groups",
                              szRid,
                              SamDbGroupObject,
                              DesiredAccess,
                              &GroupObject);
    if (!NT_SUCCESS(Status))
    {
        TRACE("failed with status 0x%08lx\n", Status);
        return Status;
    }

    *GroupHandle = (SAMPR_HANDLE)GroupObject;

    return STATUS_SUCCESS;
}


static NTSTATUS
SampQueryGroupGeneral(PSAM_DB_OBJECT GroupObject,
                      PSAMPR_GROUP_INFO_BUFFER *Buffer)
{
    PSAMPR_GROUP_INFO_BUFFER InfoBuffer = NULL;
    HANDLE MembersKeyHandle = NULL;
    SAM_GROUP_FIXED_DATA FixedData;
    ULONG Length = 0;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_GROUP_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = SampGetObjectAttributeString(GroupObject,
                                          L"Name",
                                          &InfoBuffer->General.Name);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    Status = SampGetObjectAttributeString(GroupObject,
                                          L"Description",
                                          &InfoBuffer->General.AdminComment);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    Length = sizeof(SAM_GROUP_FIXED_DATA);
    Status = SampGetObjectAttribute(GroupObject,
                                    L"F",
                                    NULL,
                                    (PVOID)&FixedData,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    InfoBuffer->General.Attributes = FixedData.Attributes;

    /* Open the Members subkey */
    Status = SampRegOpenKey(GroupObject->KeyHandle,
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
SampQueryGroupName(PSAM_DB_OBJECT GroupObject,
                   PSAMPR_GROUP_INFO_BUFFER *Buffer)
{
    PSAMPR_GROUP_INFO_BUFFER InfoBuffer = NULL;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_GROUP_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = SampGetObjectAttributeString(GroupObject,
                                          L"Name",
                                          &InfoBuffer->Name.Name);
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
SampQueryGroupAttribute(PSAM_DB_OBJECT GroupObject,
                        PSAMPR_GROUP_INFO_BUFFER *Buffer)
{
    PSAMPR_GROUP_INFO_BUFFER InfoBuffer = NULL;
    SAM_GROUP_FIXED_DATA FixedData;
    ULONG Length = 0;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_GROUP_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Length = sizeof(SAM_GROUP_FIXED_DATA);
    Status = SampGetObjectAttribute(GroupObject,
                                    L"F",
                                    NULL,
                                    (PVOID)&FixedData,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    InfoBuffer->Attribute.Attributes = FixedData.Attributes;

    *Buffer = InfoBuffer;

done:
    if (!NT_SUCCESS(Status))
    {
        if (InfoBuffer != NULL)
        {
            midl_user_free(InfoBuffer);
        }
    }

    return Status;
}


static NTSTATUS
SampQueryGroupAdminComment(PSAM_DB_OBJECT GroupObject,
                           PSAMPR_GROUP_INFO_BUFFER *Buffer)
{
    PSAMPR_GROUP_INFO_BUFFER InfoBuffer = NULL;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_GROUP_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = SampGetObjectAttributeString(GroupObject,
                                          L"Description",
                                          &InfoBuffer->AdminComment.AdminComment);
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
            if (InfoBuffer->AdminComment.AdminComment.Buffer != NULL)
                midl_user_free(InfoBuffer->AdminComment.AdminComment.Buffer);

            midl_user_free(InfoBuffer);
        }
    }

    return Status;
}


/* Function 20 */
NTSTATUS
NTAPI
SamrQueryInformationGroup(IN SAMPR_HANDLE GroupHandle,
                          IN GROUP_INFORMATION_CLASS GroupInformationClass,
                          OUT PSAMPR_GROUP_INFO_BUFFER *Buffer)
{
    PSAM_DB_OBJECT GroupObject;
    NTSTATUS Status;

    TRACE("SamrQueryInformationGroup(%p %lu %p)\n",
          GroupHandle, GroupInformationClass, Buffer);

    /* Validate the group handle */
    Status = SampValidateDbObject(GroupHandle,
                                  SamDbGroupObject,
                                  GROUP_READ_INFORMATION,
                                  &GroupObject);
    if (!NT_SUCCESS(Status))
        return Status;

    switch (GroupInformationClass)
    {
        case GroupGeneralInformation:
            Status = SampQueryGroupGeneral(GroupObject,
                                           Buffer);
            break;

        case GroupNameInformation:
            Status = SampQueryGroupName(GroupObject,
                                        Buffer);
            break;

        case GroupAttributeInformation:
            Status = SampQueryGroupAttribute(GroupObject,
                                             Buffer);
            break;

        case GroupAdminCommentInformation:
            Status = SampQueryGroupAdminComment(GroupObject,
                                                Buffer);
            break;

        default:
            Status = STATUS_INVALID_INFO_CLASS;
            break;
    }

    return Status;
}


static NTSTATUS
SampSetGroupAttribute(PSAM_DB_OBJECT GroupObject,
                      PSAMPR_GROUP_INFO_BUFFER Buffer)
{
    SAM_GROUP_FIXED_DATA FixedData;
    ULONG Length = 0;
    NTSTATUS Status;

    Length = sizeof(SAM_GROUP_FIXED_DATA);
    Status = SampGetObjectAttribute(GroupObject,
                                    L"F",
                                    NULL,
                                    (PVOID)&FixedData,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    FixedData.Attributes = Buffer->Attribute.Attributes;

    Status = SampSetObjectAttribute(GroupObject,
                                    L"F",
                                    REG_BINARY,
                                    &FixedData,
                                    Length);

done:
    return Status;
}


/* Function 21 */
NTSTATUS
NTAPI
SamrSetInformationGroup(IN SAMPR_HANDLE GroupHandle,
                        IN GROUP_INFORMATION_CLASS GroupInformationClass,
                        IN PSAMPR_GROUP_INFO_BUFFER Buffer)
{
    PSAM_DB_OBJECT GroupObject;
    NTSTATUS Status;

    TRACE("SamrSetInformationGroup(%p %lu %p)\n",
          GroupHandle, GroupInformationClass, Buffer);

    /* Validate the group handle */
    Status = SampValidateDbObject(GroupHandle,
                                  SamDbGroupObject,
                                  GROUP_WRITE_ACCOUNT,
                                  &GroupObject);
    if (!NT_SUCCESS(Status))
        return Status;

    switch (GroupInformationClass)
    {
        case GroupNameInformation:
            Status = SampSetObjectAttribute(GroupObject,
                                            L"Name",
                                            REG_SZ,
                                            Buffer->Name.Name.Buffer,
                                            Buffer->Name.Name.Length + sizeof(WCHAR));
            break;

        case GroupAttributeInformation:
            Status = SampSetGroupAttribute(GroupObject,
                                           Buffer);
            break;

        case GroupAdminCommentInformation:
            Status = SampSetObjectAttribute(GroupObject,
                                            L"Description",
                                            REG_SZ,
                                            Buffer->AdminComment.AdminComment.Buffer,
                                            Buffer->AdminComment.AdminComment.Length + sizeof(WCHAR));
            break;

        default:
            Status = STATUS_INVALID_INFO_CLASS;
            break;
    }

    return Status;
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
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_ALIAS_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = SampGetObjectAttributeString(AliasObject,
                                          L"Name",
                                          &InfoBuffer->General.Name);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    Status = SampGetObjectAttributeString(AliasObject,
                                          L"Description",
                                          &InfoBuffer->General.AdminComment);
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
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_ALIAS_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = SampGetObjectAttributeString(AliasObject,
                                          L"Name",
                                          &InfoBuffer->Name.Name);
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
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_ALIAS_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = SampGetObjectAttributeString(AliasObject,
                                          L"Description",
                                          &InfoBuffer->AdminComment.AdminComment);
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
            Status = SampSetObjectAttribute(AliasObject,
                                            L"Name",
                                            REG_SZ,
                                            Buffer->Name.Name.Buffer,
                                            Buffer->Name.Name.Length + sizeof(WCHAR));
            break;

        case AliasAdminCommentInformation:
            Status = SampSetObjectAttribute(AliasObject,
                                            L"Description",
                                            REG_SZ,
                                            Buffer->AdminComment.AdminComment.Buffer,
                                            Buffer->AdminComment.AdminComment.Length + sizeof(WCHAR));
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

    /* Validate the alias handle */
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


static
NTSTATUS
SampQueryUserGeneral(PSAM_DB_OBJECT UserObject,
                     PSAMPR_USER_INFO_BUFFER *Buffer)
{
    PSAMPR_USER_INFO_BUFFER InfoBuffer = NULL;
    SAM_USER_FIXED_DATA FixedData;
    ULONG Length = 0;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_USER_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Length = sizeof(SAM_USER_FIXED_DATA);
    Status = SampGetObjectAttribute(UserObject,
                                    L"F",
                                    NULL,
                                    (PVOID)&FixedData,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    InfoBuffer->General.PrimaryGroupId = FixedData.PrimaryGroupId;

    /* Get the Name string */
    Status = SampGetObjectAttributeString(UserObject,
                                          L"Name",
                                          &InfoBuffer->General.UserName);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    /* Get the FullName string */
    Status = SampGetObjectAttributeString(UserObject,
                                          L"FullName",
                                          &InfoBuffer->General.FullName);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    /* Get the AdminComment string */
    Status = SampGetObjectAttributeString(UserObject,
                                          L"AdminComment",
                                          &InfoBuffer->General.AdminComment);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    /* Get the UserComment string */
    Status = SampGetObjectAttributeString(UserObject,
                                          L"UserComment",
                                          &InfoBuffer->General.UserComment);
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
            if (InfoBuffer->General.UserName.Buffer != NULL)
                midl_user_free(InfoBuffer->General.UserName.Buffer);

            if (InfoBuffer->General.FullName.Buffer != NULL)
                midl_user_free(InfoBuffer->General.FullName.Buffer);

            if (InfoBuffer->General.AdminComment.Buffer != NULL)
                midl_user_free(InfoBuffer->General.AdminComment.Buffer);

            if (InfoBuffer->General.UserComment.Buffer != NULL)
                midl_user_free(InfoBuffer->General.UserComment.Buffer);

            midl_user_free(InfoBuffer);
        }
    }

    return Status;
}


static
NTSTATUS
SampQueryUserPreferences(PSAM_DB_OBJECT UserObject,
                         PSAMPR_USER_INFO_BUFFER *Buffer)
{
    PSAMPR_USER_INFO_BUFFER InfoBuffer = NULL;
    SAM_USER_FIXED_DATA FixedData;
    ULONG Length = 0;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_USER_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Length = sizeof(SAM_USER_FIXED_DATA);
    Status = SampGetObjectAttribute(UserObject,
                                    L"F",
                                    NULL,
                                    (PVOID)&FixedData,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    InfoBuffer->Preferences.CountryCode = FixedData.CountryCode;
    InfoBuffer->Preferences.CodePage = FixedData.CodePage;

    /* Get the UserComment string */
    Status = SampGetObjectAttributeString(UserObject,
                                          L"UserComment",
                                          &InfoBuffer->Preferences.UserComment);
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
            if (InfoBuffer->Preferences.UserComment.Buffer != NULL)
                midl_user_free(InfoBuffer->Preferences.UserComment.Buffer);

            midl_user_free(InfoBuffer);
        }
    }

    return Status;
}


static
NTSTATUS
SampQueryUserLogon(PSAM_DB_OBJECT UserObject,
                   PSAMPR_USER_INFO_BUFFER *Buffer)
{
    PSAMPR_USER_INFO_BUFFER InfoBuffer = NULL;
    SAM_USER_FIXED_DATA FixedData;
    ULONG Length = 0;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_USER_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Length = sizeof(SAM_USER_FIXED_DATA);
    Status = SampGetObjectAttribute(UserObject,
                                    L"F",
                                    NULL,
                                    (PVOID)&FixedData,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    InfoBuffer->Logon.UserId = FixedData.UserId;
    InfoBuffer->Logon.PrimaryGroupId = FixedData.PrimaryGroupId;
    InfoBuffer->Logon.LastLogon.LowPart = FixedData.LastLogon.LowPart;
    InfoBuffer->Logon.LastLogon.HighPart = FixedData.LastLogon.HighPart;
    InfoBuffer->Logon.LastLogoff.LowPart = FixedData.LastLogoff.LowPart;
    InfoBuffer->Logon.LastLogoff.HighPart = FixedData.LastLogoff.HighPart;
    InfoBuffer->Logon.PasswordLastSet.LowPart = FixedData.PasswordLastSet.LowPart;
    InfoBuffer->Logon.PasswordLastSet.HighPart = FixedData.PasswordLastSet.HighPart;
    InfoBuffer->Logon.BadPasswordCount = FixedData.BadPasswordCount;
    InfoBuffer->Logon.LogonCount = FixedData.LogonCount;
    InfoBuffer->Logon.UserAccountControl = FixedData.UserAccountControl;

//  OLD_LARGE_INTEGER PasswordCanChange;
//  OLD_LARGE_INTEGER PasswordMustChange;

    /* Get the Name string */
    Status = SampGetObjectAttributeString(UserObject,
                                          L"Name",
                                          &InfoBuffer->Logon.UserName);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    /* Get the FullName string */
    Status = SampGetObjectAttributeString(UserObject,
                                          L"FullName",
                                          &InfoBuffer->Logon.FullName);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    /* Get the HomeDirectory string */
    Status = SampGetObjectAttributeString(UserObject,
                                          L"HomeDirectory",
                                          &InfoBuffer->Logon.HomeDirectory);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    /* Get the HomeDirectoryDrive string */
    Status = SampGetObjectAttributeString(UserObject,
                                          L"HomeDirectoryDrive",
                                          &InfoBuffer->Logon.HomeDirectoryDrive);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    /* Get the ScriptPath string */
    Status = SampGetObjectAttributeString(UserObject,
                                          L"ScriptPath",
                                          &InfoBuffer->Logon.ScriptPath);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    /* Get the ProfilePath string */
    Status = SampGetObjectAttributeString(UserObject,
                                          L"ProfilePath",
                                          &InfoBuffer->Logon.ProfilePath);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    /* Get the WorkStations string */
    Status = SampGetObjectAttributeString(UserObject,
                                          L"WorkStations",
                                          &InfoBuffer->Logon.WorkStations);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    /* FIXME: LogonHours */

    *Buffer = InfoBuffer;

done:
    if (!NT_SUCCESS(Status))
    {
        if (InfoBuffer != NULL)
        {
            if (InfoBuffer->Logon.UserName.Buffer != NULL)
                midl_user_free(InfoBuffer->Logon.UserName.Buffer);

            if (InfoBuffer->Logon.FullName.Buffer != NULL)
                midl_user_free(InfoBuffer->Logon.FullName.Buffer);

            if (InfoBuffer->Logon.HomeDirectory.Buffer != NULL)
                midl_user_free(InfoBuffer->Logon.HomeDirectory.Buffer);

            if (InfoBuffer->Logon.HomeDirectoryDrive.Buffer != NULL)
                midl_user_free(InfoBuffer->Logon.HomeDirectoryDrive.Buffer);

            if (InfoBuffer->Logon.ScriptPath.Buffer != NULL)
                midl_user_free(InfoBuffer->Logon.ScriptPath.Buffer);

            if (InfoBuffer->Logon.ProfilePath.Buffer != NULL)
                midl_user_free(InfoBuffer->Logon.ProfilePath.Buffer);

            if (InfoBuffer->Logon.WorkStations.Buffer != NULL)
                midl_user_free(InfoBuffer->Logon.WorkStations.Buffer);

            if (InfoBuffer->Logon.LogonHours.LogonHours != NULL)
                midl_user_free(InfoBuffer->Logon.LogonHours.LogonHours);

            midl_user_free(InfoBuffer);
        }
    }

    return Status;
}




static
NTSTATUS
SampQueryUserAccount(PSAM_DB_OBJECT UserObject,
                     PSAMPR_USER_INFO_BUFFER *Buffer)
{
    PSAMPR_USER_INFO_BUFFER InfoBuffer = NULL;
    SAM_USER_FIXED_DATA FixedData;
    ULONG Length = 0;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_USER_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Length = sizeof(SAM_USER_FIXED_DATA);
    Status = SampGetObjectAttribute(UserObject,
                                    L"F",
                                    NULL,
                                    (PVOID)&FixedData,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    InfoBuffer->Account.UserId = FixedData.UserId;
    InfoBuffer->Account.PrimaryGroupId = FixedData.PrimaryGroupId;
    InfoBuffer->Account.LastLogon.LowPart = FixedData.LastLogon.LowPart;
    InfoBuffer->Account.LastLogon.HighPart = FixedData.LastLogon.HighPart;
    InfoBuffer->Account.LastLogoff.LowPart = FixedData.LastLogoff.LowPart;
    InfoBuffer->Account.LastLogoff.HighPart = FixedData.LastLogoff.HighPart;
    InfoBuffer->Account.PasswordLastSet.LowPart = FixedData.PasswordLastSet.LowPart;
    InfoBuffer->Account.PasswordLastSet.HighPart = FixedData.PasswordLastSet.HighPart;
    InfoBuffer->Account.AccountExpires.LowPart = FixedData.AccountExpires.LowPart;
    InfoBuffer->Account.AccountExpires.HighPart = FixedData.AccountExpires.HighPart;
    InfoBuffer->Account.BadPasswordCount = FixedData.BadPasswordCount;
    InfoBuffer->Account.LogonCount = FixedData.LogonCount;
    InfoBuffer->Account.UserAccountControl = FixedData.UserAccountControl;

    /* Get the Name string */
    Status = SampGetObjectAttributeString(UserObject,
                                          L"Name",
                                          &InfoBuffer->Account.UserName);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    /* Get the FullName string */
    Status = SampGetObjectAttributeString(UserObject,
                                          L"FullName",
                                          &InfoBuffer->Account.FullName);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    /* Get the HomeDirectory string */
    Status = SampGetObjectAttributeString(UserObject,
                                          L"HomeDirectory",
                                          &InfoBuffer->Account.HomeDirectory);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    /* Get the HomeDirectoryDrive string */
    Status = SampGetObjectAttributeString(UserObject,
                                          L"HomeDirectoryDrive",
                                          &InfoBuffer->Account.HomeDirectoryDrive);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    /* Get the ScriptPath string */
    Status = SampGetObjectAttributeString(UserObject,
                                          L"ScriptPath",
                                          &InfoBuffer->Account.ScriptPath);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    /* Get the ProfilePath string */
    Status = SampGetObjectAttributeString(UserObject,
                                          L"ProfilePath",
                                          &InfoBuffer->Account.ProfilePath);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    /* Get the AdminComment string */
    Status = SampGetObjectAttributeString(UserObject,
                                          L"AdminComment",
                                          &InfoBuffer->Account.AdminComment);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    /* Get the WorkStations string */
    Status = SampGetObjectAttributeString(UserObject,
                                          L"WorkStations",
                                          &InfoBuffer->Account.WorkStations);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    /* FIXME: LogonHours */

    *Buffer = InfoBuffer;

done:
    if (!NT_SUCCESS(Status))
    {
        if (InfoBuffer != NULL)
        {
            if (InfoBuffer->Account.UserName.Buffer != NULL)
                midl_user_free(InfoBuffer->Account.UserName.Buffer);

            if (InfoBuffer->Account.FullName.Buffer != NULL)
                midl_user_free(InfoBuffer->Account.FullName.Buffer);

            if (InfoBuffer->Account.HomeDirectory.Buffer != NULL)
                midl_user_free(InfoBuffer->Account.HomeDirectory.Buffer);

            if (InfoBuffer->Account.HomeDirectoryDrive.Buffer != NULL)
                midl_user_free(InfoBuffer->Account.HomeDirectoryDrive.Buffer);

            if (InfoBuffer->Account.ScriptPath.Buffer != NULL)
                midl_user_free(InfoBuffer->Account.ScriptPath.Buffer);

            if (InfoBuffer->Account.ProfilePath.Buffer != NULL)
                midl_user_free(InfoBuffer->Account.ProfilePath.Buffer);

            if (InfoBuffer->Account.AdminComment.Buffer != NULL)
                midl_user_free(InfoBuffer->Account.AdminComment.Buffer);

            if (InfoBuffer->Account.WorkStations.Buffer != NULL)
                midl_user_free(InfoBuffer->Account.WorkStations.Buffer);

            if (InfoBuffer->Account.LogonHours.LogonHours != NULL)
                midl_user_free(InfoBuffer->Account.LogonHours.LogonHours);

            midl_user_free(InfoBuffer);
        }
    }

    return Status;
}

/* FIXME: SampQueryUserLogonHours */

static
NTSTATUS
SampQueryUserName(PSAM_DB_OBJECT UserObject,
                  PSAMPR_USER_INFO_BUFFER *Buffer)
{
    PSAMPR_USER_INFO_BUFFER InfoBuffer = NULL;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_USER_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Get the Name string */
    Status = SampGetObjectAttributeString(UserObject,
                                          L"Name",
                                          &InfoBuffer->Name.UserName);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    /* Get the FullName string */
    Status = SampGetObjectAttributeString(UserObject,
                                          L"FullName",
                                          &InfoBuffer->Name.FullName);
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
            if (InfoBuffer->Name.UserName.Buffer != NULL)
                midl_user_free(InfoBuffer->Name.UserName.Buffer);

            if (InfoBuffer->Name.FullName.Buffer != NULL)
                midl_user_free(InfoBuffer->Name.FullName.Buffer);

            midl_user_free(InfoBuffer);
        }
    }

    return Status;
}


static NTSTATUS
SampQueryUserAccountName(PSAM_DB_OBJECT UserObject,
                         PSAMPR_USER_INFO_BUFFER *Buffer)
{
    PSAMPR_USER_INFO_BUFFER InfoBuffer = NULL;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_USER_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Get the Name string */
    Status = SampGetObjectAttributeString(UserObject,
                                          L"Name",
                                          &InfoBuffer->AccountName.UserName);
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
            if (InfoBuffer->AccountName.UserName.Buffer != NULL)
                midl_user_free(InfoBuffer->AccountName.UserName.Buffer);

            midl_user_free(InfoBuffer);
        }
    }

    return Status;
}


static NTSTATUS
SampQueryUserFullName(PSAM_DB_OBJECT UserObject,
                      PSAMPR_USER_INFO_BUFFER *Buffer)
{
    PSAMPR_USER_INFO_BUFFER InfoBuffer = NULL;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_USER_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Get the FullName string */
    Status = SampGetObjectAttributeString(UserObject,
                                          L"FullName",
                                          &InfoBuffer->FullName.FullName);
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
            if (InfoBuffer->FullName.FullName.Buffer != NULL)
                midl_user_free(InfoBuffer->FullName.FullName.Buffer);

            midl_user_free(InfoBuffer);
        }
    }

    return Status;
}


static
NTSTATUS
SampQueryUserPrimaryGroup(PSAM_DB_OBJECT UserObject,
                          PSAMPR_USER_INFO_BUFFER *Buffer)
{
    PSAMPR_USER_INFO_BUFFER InfoBuffer = NULL;
    SAM_USER_FIXED_DATA FixedData;
    ULONG Length = 0;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_USER_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Length = sizeof(SAM_USER_FIXED_DATA);
    Status = SampGetObjectAttribute(UserObject,
                                    L"F",
                                    NULL,
                                    (PVOID)&FixedData,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    InfoBuffer->PrimaryGroup.PrimaryGroupId = FixedData.PrimaryGroupId;

    *Buffer = InfoBuffer;

done:
    if (!NT_SUCCESS(Status))
    {
        if (InfoBuffer != NULL)
        {
            midl_user_free(InfoBuffer);
        }
    }

    return Status;
}


static NTSTATUS
SampQueryUserHome(PSAM_DB_OBJECT UserObject,
                  PSAMPR_USER_INFO_BUFFER *Buffer)
{
    PSAMPR_USER_INFO_BUFFER InfoBuffer = NULL;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_USER_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Get the HomeDirectory string */
    Status = SampGetObjectAttributeString(UserObject,
                                          L"HomeDirectory",
                                          &InfoBuffer->Home.HomeDirectory);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    /* Get the HomeDirectoryDrive string */
    Status = SampGetObjectAttributeString(UserObject,
                                          L"HomeDirectoryDrive",
                                          &InfoBuffer->Home.HomeDirectoryDrive);
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
            if (InfoBuffer->Home.HomeDirectory.Buffer != NULL)
                midl_user_free(InfoBuffer->Home.HomeDirectory.Buffer);

            if (InfoBuffer->Home.HomeDirectoryDrive.Buffer != NULL)
                midl_user_free(InfoBuffer->Home.HomeDirectoryDrive.Buffer);

            midl_user_free(InfoBuffer);
        }
    }

    return Status;
}


static NTSTATUS
SampQueryUserScript(PSAM_DB_OBJECT UserObject,
                    PSAMPR_USER_INFO_BUFFER *Buffer)
{
    PSAMPR_USER_INFO_BUFFER InfoBuffer = NULL;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_USER_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Get the ScriptPath string */
    Status = SampGetObjectAttributeString(UserObject,
                                          L"ScriptPath",
                                          &InfoBuffer->Script.ScriptPath);
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
            if (InfoBuffer->Script.ScriptPath.Buffer != NULL)
                midl_user_free(InfoBuffer->Script.ScriptPath.Buffer);

            midl_user_free(InfoBuffer);
        }
    }

    return Status;
}


static NTSTATUS
SampQueryUserProfile(PSAM_DB_OBJECT UserObject,
                     PSAMPR_USER_INFO_BUFFER *Buffer)
{
    PSAMPR_USER_INFO_BUFFER InfoBuffer = NULL;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_USER_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Get the ProfilePath string */
    Status = SampGetObjectAttributeString(UserObject,
                                          L"ProfilePath",
                                          &InfoBuffer->Profile.ProfilePath);
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
            if (InfoBuffer->Profile.ProfilePath.Buffer != NULL)
                midl_user_free(InfoBuffer->Profile.ProfilePath.Buffer);

            midl_user_free(InfoBuffer);
        }
    }

    return Status;
}


static NTSTATUS
SampQueryUserAdminComment(PSAM_DB_OBJECT UserObject,
                          PSAMPR_USER_INFO_BUFFER *Buffer)
{
    PSAMPR_USER_INFO_BUFFER InfoBuffer = NULL;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_USER_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Get the AdminComment string */
    Status = SampGetObjectAttributeString(UserObject,
                                          L"AdminComment",
                                          &InfoBuffer->AdminComment.AdminComment);
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
            if (InfoBuffer->AdminComment.AdminComment.Buffer != NULL)
                midl_user_free(InfoBuffer->AdminComment.AdminComment.Buffer);

            midl_user_free(InfoBuffer);
        }
    }

    return Status;
}


static NTSTATUS
SampQueryUserWorkStations(PSAM_DB_OBJECT UserObject,
                          PSAMPR_USER_INFO_BUFFER *Buffer)
{
    PSAMPR_USER_INFO_BUFFER InfoBuffer = NULL;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_USER_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Get the WorkStations string */
    Status = SampGetObjectAttributeString(UserObject,
                                          L"WorkStations",
                                          &InfoBuffer->WorkStations.WorkStations);
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
            if (InfoBuffer->WorkStations.WorkStations.Buffer != NULL)
                midl_user_free(InfoBuffer->WorkStations.WorkStations.Buffer);

            midl_user_free(InfoBuffer);
        }
    }

    return Status;
}


static
NTSTATUS
SampQueryUserControl(PSAM_DB_OBJECT UserObject,
                     PSAMPR_USER_INFO_BUFFER *Buffer)
{
    PSAMPR_USER_INFO_BUFFER InfoBuffer = NULL;
    SAM_USER_FIXED_DATA FixedData;
    ULONG Length = 0;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_USER_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Length = sizeof(SAM_USER_FIXED_DATA);
    Status = SampGetObjectAttribute(UserObject,
                                    L"F",
                                    NULL,
                                    (PVOID)&FixedData,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    InfoBuffer->Control.UserAccountControl = FixedData.UserAccountControl;

    *Buffer = InfoBuffer;

done:
    if (!NT_SUCCESS(Status))
    {
        if (InfoBuffer != NULL)
        {
            midl_user_free(InfoBuffer);
        }
    }

    return Status;
}


static
NTSTATUS
SampQueryUserExpires(PSAM_DB_OBJECT UserObject,
                     PSAMPR_USER_INFO_BUFFER *Buffer)
{
    PSAMPR_USER_INFO_BUFFER InfoBuffer = NULL;
    SAM_USER_FIXED_DATA FixedData;
    ULONG Length = 0;
    NTSTATUS Status;

    *Buffer = NULL;

    InfoBuffer = midl_user_allocate(sizeof(SAMPR_USER_INFO_BUFFER));
    if (InfoBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Length = sizeof(SAM_USER_FIXED_DATA);
    Status = SampGetObjectAttribute(UserObject,
                                    L"F",
                                    NULL,
                                    (PVOID)&FixedData,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    InfoBuffer->Expires.AccountExpires.LowPart = FixedData.AccountExpires.LowPart;
    InfoBuffer->Expires.AccountExpires.HighPart = FixedData.AccountExpires.HighPart;

    *Buffer = InfoBuffer;

done:
    if (!NT_SUCCESS(Status))
    {
        if (InfoBuffer != NULL)
        {
            midl_user_free(InfoBuffer);
        }
    }

    return Status;
}


/* Function 36 */
NTSTATUS
NTAPI
SamrQueryInformationUser(IN SAMPR_HANDLE UserHandle,
                         IN USER_INFORMATION_CLASS UserInformationClass,
                         OUT PSAMPR_USER_INFO_BUFFER *Buffer)
{
    PSAM_DB_OBJECT UserObject;
    ACCESS_MASK DesiredAccess;
    NTSTATUS Status;

    TRACE("SamrQueryInformationUser(%p %lu %p)\n",
          UserHandle, UserInformationClass, Buffer);

    switch (UserInformationClass)
    {
        case UserGeneralInformation:
        case UserNameInformation:
        case UserAccountNameInformation:
        case UserFullNameInformation:
        case UserPrimaryGroupInformation:
        case UserAdminCommentInformation:
            DesiredAccess = USER_READ_GENERAL;
            break;

        case UserLogonHoursInformation:
        case UserHomeInformation:
        case UserScriptInformation:
        case UserProfileInformation:
        case UserWorkStationsInformation:
            DesiredAccess = USER_READ_LOGON;
            break;

        case UserControlInformation:
        case UserExpiresInformation:
            DesiredAccess = USER_READ_ACCOUNT;
            break;

        case UserPreferencesInformation:
            DesiredAccess = USER_READ_GENERAL |
                            USER_READ_PREFERENCES;
            break;

        case UserLogonInformation:
        case UserAccountInformation:
            DesiredAccess = USER_READ_GENERAL |
                            USER_READ_PREFERENCES |
                            USER_READ_LOGON |
                            USER_READ_ACCOUNT;
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
        case UserGeneralInformation:
            Status = SampQueryUserGeneral(UserObject,
                                          Buffer);
            break;

        case UserPreferencesInformation:
            Status = SampQueryUserPreferences(UserObject,
                                              Buffer);
            break;

        case UserLogonInformation:
            Status = SampQueryUserLogon(UserObject,
                                        Buffer);
            break;

//        case UserLogonHoursInformation:
//            Status = SampQueryUserLogonHours(UserObject,
//                                             Buffer);
//            break;

        case UserAccountInformation:
            Status = SampQueryUserAccount(UserObject,
                                          Buffer);
            break;

        case UserNameInformation:
            Status = SampQueryUserName(UserObject,
                                       Buffer);
            break;

        case UserAccountNameInformation:
            Status = SampQueryUserAccountName(UserObject,
                                              Buffer);
            break;

        case UserFullNameInformation:
            Status = SampQueryUserFullName(UserObject,
                                           Buffer);
            break;

        case UserPrimaryGroupInformation:
            Status = SampQueryUserPrimaryGroup(UserObject,
                                               Buffer);
            break;

        case UserHomeInformation:
            Status = SampQueryUserHome(UserObject,
                                       Buffer);

        case UserScriptInformation:
            Status = SampQueryUserScript(UserObject,
                                         Buffer);
            break;

        case UserProfileInformation:
            Status = SampQueryUserProfile(UserObject,
                                          Buffer);
            break;

        case UserAdminCommentInformation:
            Status = SampQueryUserAdminComment(UserObject,
                                               Buffer);
            break;

        case UserWorkStationsInformation:
            Status = SampQueryUserWorkStations(UserObject,
                                               Buffer);
            break;

        case UserControlInformation:
            Status = SampQueryUserControl(UserObject,
                                          Buffer);
            break;

        case UserExpiresInformation:
            Status = SampQueryUserExpires(UserObject,
                                          Buffer);
            break;

        default:
            Status = STATUS_INVALID_INFO_CLASS;
    }

    return Status;
}


static NTSTATUS
SampSetUserGeneral(PSAM_DB_OBJECT UserObject,
                   PSAMPR_USER_INFO_BUFFER Buffer)
{
    SAM_USER_FIXED_DATA FixedData;
    ULONG Length = 0;
    NTSTATUS Status;

    Length = sizeof(SAM_USER_FIXED_DATA);
    Status = SampGetObjectAttribute(UserObject,
                                    L"F",
                                    NULL,
                                    (PVOID)&FixedData,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    FixedData.PrimaryGroupId = Buffer->General.PrimaryGroupId;

    Status = SampSetObjectAttribute(UserObject,
                                    L"F",
                                    REG_BINARY,
                                    &FixedData,
                                    Length);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampSetObjectAttribute(UserObject,
                                    L"Name",
                                    REG_SZ,
                                    Buffer->General.UserName.Buffer,
                                    Buffer->General.UserName.MaximumLength);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampSetObjectAttribute(UserObject,
                                    L"FullName",
                                    REG_SZ,
                                    Buffer->General.FullName.Buffer,
                                    Buffer->General.FullName.MaximumLength);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampSetObjectAttribute(UserObject,
                                    L"AdminComment",
                                    REG_SZ,
                                    Buffer->General.AdminComment.Buffer,
                                    Buffer->General.AdminComment.MaximumLength);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampSetObjectAttribute(UserObject,
                                    L"UserComment",
                                    REG_SZ,
                                    Buffer->General.UserComment.Buffer,
                                    Buffer->General.UserComment.MaximumLength);

done:
    return Status;
}


static NTSTATUS
SampSetUserPreferences(PSAM_DB_OBJECT UserObject,
                       PSAMPR_USER_INFO_BUFFER Buffer)
{
    SAM_USER_FIXED_DATA FixedData;
    ULONG Length = 0;
    NTSTATUS Status;

    Length = sizeof(SAM_USER_FIXED_DATA);
    Status = SampGetObjectAttribute(UserObject,
                                    L"F",
                                    NULL,
                                    (PVOID)&FixedData,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    FixedData.CountryCode = Buffer->Preferences.CountryCode;
    FixedData.CodePage = Buffer->Preferences.CodePage;

    Status = SampSetObjectAttribute(UserObject,
                                    L"F",
                                    REG_BINARY,
                                    &FixedData,
                                    Length);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampSetObjectAttribute(UserObject,
                                    L"UserComment",
                                    REG_SZ,
                                    Buffer->Preferences.UserComment.Buffer,
                                    Buffer->Preferences.UserComment.MaximumLength);

done:
    return Status;
}


static NTSTATUS
SampSetUserPrimaryGroup(PSAM_DB_OBJECT UserObject,
                        PSAMPR_USER_INFO_BUFFER Buffer)
{
    SAM_USER_FIXED_DATA FixedData;
    ULONG Length = 0;
    NTSTATUS Status;

    Length = sizeof(SAM_USER_FIXED_DATA);
    Status = SampGetObjectAttribute(UserObject,
                                    L"F",
                                    NULL,
                                    (PVOID)&FixedData,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    FixedData.PrimaryGroupId = Buffer->PrimaryGroup.PrimaryGroupId;

    Status = SampSetObjectAttribute(UserObject,
                                    L"F",
                                    REG_BINARY,
                                    &FixedData,
                                    Length);

done:
    return Status;
}


static NTSTATUS
SampSetUserControl(PSAM_DB_OBJECT UserObject,
                   PSAMPR_USER_INFO_BUFFER Buffer)
{
    SAM_USER_FIXED_DATA FixedData;
    ULONG Length = 0;
    NTSTATUS Status;

    Length = sizeof(SAM_USER_FIXED_DATA);
    Status = SampGetObjectAttribute(UserObject,
                                    L"F",
                                    NULL,
                                    (PVOID)&FixedData,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    FixedData.UserAccountControl = Buffer->Control.UserAccountControl;

    Status = SampSetObjectAttribute(UserObject,
                                    L"F",
                                    REG_BINARY,
                                    &FixedData,
                                    Length);

done:
    return Status;
}


static NTSTATUS
SampSetUserExpires(PSAM_DB_OBJECT UserObject,
                   PSAMPR_USER_INFO_BUFFER Buffer)
{
    SAM_USER_FIXED_DATA FixedData;
    ULONG Length = 0;
    NTSTATUS Status;

    Length = sizeof(SAM_USER_FIXED_DATA);
    Status = SampGetObjectAttribute(UserObject,
                                    L"F",
                                    NULL,
                                    (PVOID)&FixedData,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    FixedData.AccountExpires.LowPart = Buffer->Expires.AccountExpires.LowPart;
    FixedData.AccountExpires.HighPart = Buffer->Expires.AccountExpires.HighPart;

    Status = SampSetObjectAttribute(UserObject,
                                    L"F",
                                    REG_BINARY,
                                    &FixedData,
                                    Length);

done:
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
    ACCESS_MASK DesiredAccess;
    NTSTATUS Status;

    TRACE("SamrSetInformationUser(%p %lu %p)\n",
          UserHandle, UserInformationClass, Buffer);

    switch (UserInformationClass)
    {
        case UserLogonHoursInformation:
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

        case UserGeneralInformation:
            DesiredAccess = USER_WRITE_ACCOUNT |
                            USER_WRITE_PREFERENCES;
            break;

        case UserPreferencesInformation:
            DesiredAccess = USER_WRITE_PREFERENCES;
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
        case UserGeneralInformation:
            Status = SampSetUserGeneral(UserObject,
                                        Buffer);
            break;

        case UserPreferencesInformation:
            Status = SampSetUserPreferences(UserObject,
                                            Buffer);
            break;
/*
        case UserLogonHoursInformation:
            Status = SampSetUserLogonHours(UserObject,
                                           Buffer);
            break;
*/
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
            Status = SampSetUserPrimaryGroup(UserObject,
                                             Buffer);
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
            Status = SampSetUserControl(UserObject,
                                        Buffer);
            break;

        case UserExpiresInformation:
            Status = SampSetUserExpires(UserObject,
                                        Buffer);
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
