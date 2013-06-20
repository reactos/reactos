/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * FILE:              lib/rtl/security.c
 * PURPOSE:           Security related functions and Security Objects
 * PROGRAMMER:        Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

NTSTATUS
NTAPI
RtlpSetSecurityObject(IN PVOID Object,
                      IN SECURITY_INFORMATION SecurityInformation,
                      IN PSECURITY_DESCRIPTOR ModificationDescriptor,
                      OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
                      IN ULONG AutoInheritFlags,
                      IN ULONG PoolType,
                      IN PGENERIC_MAPPING GenericMapping,
                      IN HANDLE Token OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
RtlpNewSecurityObject(IN PSECURITY_DESCRIPTOR ParentDescriptor,
                      IN PSECURITY_DESCRIPTOR CreatorDescriptor,
                      OUT PSECURITY_DESCRIPTOR *NewDescriptor,
                      IN LPGUID *ObjectTypes,
                      IN ULONG GuidCount,
                      IN BOOLEAN IsDirectoryObject,
                      IN ULONG AutoInheritFlags,
                      IN HANDLE Token,
                      IN PGENERIC_MAPPING GenericMapping)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
RtlpConvertToAutoInheritSecurityObject(IN PSECURITY_DESCRIPTOR ParentDescriptor,
                                       IN PSECURITY_DESCRIPTOR CreatorDescriptor,
                                       OUT PSECURITY_DESCRIPTOR *NewDescriptor,
                                       IN LPGUID ObjectType,
                                       IN BOOLEAN IsDirectoryObject,
                                       IN PGENERIC_MAPPING GenericMapping)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
RtlCreateAndSetSD(IN PVOID AceData,
                  IN ULONG AceCount,
                  IN PSID OwnerSid OPTIONAL,
                  IN PSID GroupSid OPTIONAL,
                  OUT PSECURITY_DESCRIPTOR *NewDescriptor)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlDeleteSecurityObject(IN PSECURITY_DESCRIPTOR *ObjectDescriptor)
{
    DPRINT1("RtlDeleteSecurityObject(%p)\n", ObjectDescriptor);

    /* Free the object from the heap */
    RtlFreeHeap(RtlGetProcessHeap(), 0, *ObjectDescriptor);
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlNewSecurityObject(IN PSECURITY_DESCRIPTOR ParentDescriptor,
                     IN PSECURITY_DESCRIPTOR CreatorDescriptor,
                     OUT PSECURITY_DESCRIPTOR *NewDescriptor,
                     IN BOOLEAN IsDirectoryObject,
                     IN HANDLE Token,
                     IN PGENERIC_MAPPING GenericMapping)
{
    DPRINT1("RtlNewSecurityObject(%p)\n", ParentDescriptor);

    /* Call the internal API */
    return RtlpNewSecurityObject(ParentDescriptor,
                                 CreatorDescriptor,
                                 NewDescriptor,
                                 NULL,
                                 0,
                                 IsDirectoryObject,
                                 0,
                                 Token,
                                 GenericMapping);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlNewSecurityObjectEx(IN PSECURITY_DESCRIPTOR ParentDescriptor,
                       IN PSECURITY_DESCRIPTOR CreatorDescriptor,
                       OUT PSECURITY_DESCRIPTOR *NewDescriptor,
                       IN LPGUID ObjectType,
                       IN BOOLEAN IsDirectoryObject,
                       IN ULONG AutoInheritFlags,
                       IN HANDLE Token,
                       IN PGENERIC_MAPPING GenericMapping)
{
    DPRINT1("RtlNewSecurityObjectEx(%p)\n", ParentDescriptor);

    /* Call the internal API */
    return RtlpNewSecurityObject(ParentDescriptor,
                                 CreatorDescriptor,
                                 NewDescriptor,
                                 ObjectType ? &ObjectType : NULL,
                                 ObjectType ? 1 : 0,
                                 IsDirectoryObject,
                                 AutoInheritFlags,
                                 Token,
                                 GenericMapping);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlNewSecurityObjectWithMultipleInheritance(IN PSECURITY_DESCRIPTOR ParentDescriptor,
                                            IN PSECURITY_DESCRIPTOR CreatorDescriptor,
                                            OUT PSECURITY_DESCRIPTOR *NewDescriptor,
                                            IN LPGUID *ObjectTypes,
                                            IN ULONG GuidCount,
                                            IN BOOLEAN IsDirectoryObject,
                                            IN ULONG AutoInheritFlags,
                                            IN HANDLE Token,
                                            IN PGENERIC_MAPPING GenericMapping)
{
    DPRINT1("RtlNewSecurityObjectWithMultipleInheritance(%p)\n", ParentDescriptor);

    /* Call the internal API */
    return RtlpNewSecurityObject(ParentDescriptor,
                                 CreatorDescriptor,
                                 NewDescriptor,
                                 ObjectTypes,
                                 GuidCount,
                                 IsDirectoryObject,
                                 AutoInheritFlags,
                                 Token,
                                 GenericMapping);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlNewInstanceSecurityObject(IN BOOLEAN ParentDescriptorChanged,
                             IN BOOLEAN CreatorDescriptorChanged,
                             IN PLUID OldClientTokenModifiedId,
                             OUT PLUID NewClientTokenModifiedId,
                             IN PSECURITY_DESCRIPTOR ParentDescriptor,
                             IN PSECURITY_DESCRIPTOR CreatorDescriptor,
                             OUT PSECURITY_DESCRIPTOR *NewDescriptor,
                             IN BOOLEAN IsDirectoryObject,
                             IN HANDLE Token,
                             IN PGENERIC_MAPPING GenericMapping)
{
    TOKEN_STATISTICS TokenStats;
    ULONG Size;
    NTSTATUS Status;
    DPRINT1("RtlNewInstanceSecurityObject(%p)\n", ParentDescriptor);

    /* Query the token statistics */
    Status = NtQueryInformationToken(Token,
                                     TokenStatistics,
                                     &TokenStats,
                                     sizeof(TokenStats),
                                     &Size);
    if (!NT_SUCCESS(Status)) return Status;

    /* Return the LUID */
    *NewClientTokenModifiedId = TokenStats.ModifiedId;

    /* Check if the LUID changed */
    if (RtlEqualLuid(NewClientTokenModifiedId, OldClientTokenModifiedId))
    {
        /* Did nothing change? */
        if (!(ParentDescriptorChanged) && !(CreatorDescriptorChanged))
        {
            /* There's no new descriptor, we're done */
            *NewDescriptor = NULL;
            return STATUS_SUCCESS;
        }
    }

    /* Call the standard API */
    return RtlNewSecurityObject(ParentDescriptor,
                                CreatorDescriptor,
                                NewDescriptor,
                                IsDirectoryObject,
                                Token,
                                GenericMapping);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlCreateUserSecurityObject(IN PVOID AceData,
                            IN ULONG AceCount,
                            IN PSID OwnerSid,
                            IN PSID GroupSid,
                            IN BOOLEAN IsDirectoryObject,
                            IN PGENERIC_MAPPING GenericMapping,
                            OUT PSECURITY_DESCRIPTOR *NewDescriptor)
{
    NTSTATUS Status;
    PSECURITY_DESCRIPTOR Sd;
    HANDLE TokenHandle;
    DPRINT1("RtlCreateUserSecurityObject(%p)\n", AceData);

    /* Create the security descriptor based on the ACE Data */
    Status = RtlCreateAndSetSD(AceData,
                               AceCount,
                               OwnerSid,
                               GroupSid,
                               &Sd);
    if (!NT_SUCCESS(Status)) return Status;

    /* Open the process token */
    Status = NtOpenProcessToken(NtCurrentProcess(), TOKEN_QUERY, &TokenHandle);
    if (!NT_SUCCESS(Status)) goto Quickie;

    /* Create the security object */
    Status = RtlNewSecurityObject(NULL,
                                  Sd,
                                  NewDescriptor,
                                  IsDirectoryObject,
                                  TokenHandle,
                                  GenericMapping);

    /* We're done, close the token handle */
    NtClose(TokenHandle);

Quickie:
    /* Free the SD and return status */
    RtlFreeHeap(RtlGetProcessHeap(), 0, Sd);
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlNewSecurityGrantedAccess(IN ACCESS_MASK DesiredAccess,
                            OUT PPRIVILEGE_SET Privileges,
                            IN OUT PULONG Length,
                            IN HANDLE Token,
                            IN PGENERIC_MAPPING GenericMapping,
                            OUT PACCESS_MASK RemainingDesiredAccess)
{
    NTSTATUS Status;
    BOOLEAN Granted, CallerToken;
    TOKEN_STATISTICS TokenStats;
    ULONG Size;
    DPRINT1("RtlNewSecurityGrantedAccess(%lx)\n", DesiredAccess);

    /* Has the caller passed a token? */
    if (!Token)
    {
        /* Remember that we'll have to close the handle */
        CallerToken = FALSE;

        /* Nope, open it */
        Status = NtOpenThreadToken(NtCurrentThread(), TOKEN_QUERY, TRUE, &Token);
        if (!NT_SUCCESS(Status)) return Status;
    }
    else
    {
        /* Yep, use it */
        CallerToken = TRUE;
    }

    /* Get information on the token */
    Status = NtQueryInformationToken(Token,
                                     TokenStatistics,
                                     &TokenStats,
                                     sizeof(TokenStats),
                                     &Size);
    ASSERT(NT_SUCCESS(Status));

    /* Windows doesn't do anything with the token statistics! */

    /* Map the access and return it back decoded */
    RtlMapGenericMask(&DesiredAccess, GenericMapping);
    *RemainingDesiredAccess = DesiredAccess;

    /* Check if one of the rights requested was the SACL right */
    if (DesiredAccess & ACCESS_SYSTEM_SECURITY)
    {
        /* Pretend that it's allowed FIXME: Do privilege check */
        DPRINT1("Missing privilege check for SE_SECURITY_PRIVILEGE");
        Granted = TRUE;
        *RemainingDesiredAccess &= ~ACCESS_SYSTEM_SECURITY;
    }
    else
    {
        /* Nothing to grant */
        Granted = FALSE;
    }

    /* If the caller did not pass in a token, close the handle to ours */
    if (!CallerToken) NtClose(Token);

    /* We need space to return only 1 privilege -- already part of the struct */
    Size = sizeof(PRIVILEGE_SET);
    if (Size > *Length)
    {
        /* Tell the caller how much space we need and fail */
        *Length = Size;
        return STATUS_BUFFER_TOO_SMALL;
    }

    /* Check if the SACL right was granted... */
    RtlZeroMemory(&Privileges, Size);
    if (Granted)
    {
        /* Yes, return it in the structure */
        Privileges->PrivilegeCount = 1;
        Privileges->Privilege[0].Luid.LowPart = SE_SECURITY_PRIVILEGE;
        Privileges->Privilege[0].Luid.HighPart = 0;
        Privileges->Privilege[0].Attributes = SE_PRIVILEGE_USED_FOR_ACCESS;
    }

    /* All done */
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
RtlQuerySecurityObject(IN PSECURITY_DESCRIPTOR ObjectDescriptor,
                       IN SECURITY_INFORMATION SecurityInformation,
                       OUT PSECURITY_DESCRIPTOR ResultantDescriptor,
                       IN ULONG DescriptorLength,
                       OUT PULONG ReturnLength)
{
    NTSTATUS Status;
    SECURITY_DESCRIPTOR desc;
    BOOLEAN defaulted, present;
    PACL pacl;
    PSID psid;

    Status = RtlCreateSecurityDescriptor(&desc, SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status)) return Status;

    if (SecurityInformation & OWNER_SECURITY_INFORMATION)
    {
        Status = RtlGetOwnerSecurityDescriptor(ObjectDescriptor, &psid, &defaulted);
        if (!NT_SUCCESS(Status)) return Status;
        Status = RtlSetOwnerSecurityDescriptor(&desc, psid, defaulted);
        if (!NT_SUCCESS(Status)) return Status;
    }

    if (SecurityInformation & GROUP_SECURITY_INFORMATION)
    {
        Status = RtlGetGroupSecurityDescriptor(ObjectDescriptor, &psid, &defaulted);
        if (!NT_SUCCESS(Status)) return Status;
        Status = RtlSetGroupSecurityDescriptor(&desc, psid, defaulted);
        if (!NT_SUCCESS(Status)) return Status;
    }

    if (SecurityInformation & DACL_SECURITY_INFORMATION)
    {
        Status = RtlGetDaclSecurityDescriptor(ObjectDescriptor, &present, &pacl, &defaulted);
        if (!NT_SUCCESS(Status)) return Status;
        Status = RtlSetDaclSecurityDescriptor(&desc, present, pacl, defaulted);
        if (!NT_SUCCESS(Status)) return Status;
    }

    if (SecurityInformation & SACL_SECURITY_INFORMATION)
    {
        Status = RtlGetSaclSecurityDescriptor(ObjectDescriptor, &present, &pacl, &defaulted);
        if (!NT_SUCCESS(Status)) return Status;
        Status = RtlSetSaclSecurityDescriptor(&desc, present, pacl, defaulted);
        if (!NT_SUCCESS(Status)) return Status;
    }

    *ReturnLength = DescriptorLength;
    return RtlAbsoluteToSelfRelativeSD(&desc, ResultantDescriptor, ReturnLength);
}


/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlSetSecurityObject(IN SECURITY_INFORMATION SecurityInformation,
                     IN PSECURITY_DESCRIPTOR ModificationDescriptor,
                     OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
                     IN PGENERIC_MAPPING GenericMapping,
                     IN HANDLE Token)
{
    /* Call the internal API */
    return RtlpSetSecurityObject(NULL,
                                 SecurityInformation,
                                 ModificationDescriptor,
                                 ObjectsSecurityDescriptor,
                                 0,
                                 PagedPool,
                                 GenericMapping,
                                 Token);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlSetSecurityObjectEx(IN SECURITY_INFORMATION SecurityInformation,
                       IN PSECURITY_DESCRIPTOR ModificationDescriptor,
                       OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
                       IN ULONG AutoInheritFlags,
                       IN PGENERIC_MAPPING GenericMapping,
                       IN HANDLE Token)
{
    /* Call the internal API */
    return RtlpSetSecurityObject(NULL,
                                 SecurityInformation,
                                 ModificationDescriptor,
                                 ObjectsSecurityDescriptor,
                                 AutoInheritFlags,
                                 PagedPool,
                                 GenericMapping,
                                 Token);

}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlConvertToAutoInheritSecurityObject(IN PSECURITY_DESCRIPTOR ParentDescriptor,
                                      IN PSECURITY_DESCRIPTOR CreatorDescriptor,
                                      OUT PSECURITY_DESCRIPTOR *NewDescriptor,
                                      IN LPGUID ObjectType,
                                      IN BOOLEAN IsDirectoryObject,
                                      IN PGENERIC_MAPPING GenericMapping)
{
    /* Call the internal API */
    return RtlpConvertToAutoInheritSecurityObject(ParentDescriptor,
                                                  CreatorDescriptor,
                                                  NewDescriptor,
                                                  ObjectType,
                                                  IsDirectoryObject,
                                                  GenericMapping);
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
RtlRegisterSecureMemoryCacheCallback(IN PRTL_SECURE_MEMORY_CACHE_CALLBACK Callback)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
RtlFlushSecureMemoryCache(IN PVOID MemoryCache,
                          IN OPTIONAL SIZE_T MemoryLength)
{
    UNIMPLEMENTED;
    return FALSE;
}
