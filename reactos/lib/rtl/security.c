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

/* FUNCTIONS ***************************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlDeleteSecurityObject(IN PSECURITY_DESCRIPTOR *ObjectDescriptor)
{
    DPRINT("RtlDeleteSecurityObject(%p)\n", ObjectDescriptor);

    RtlFreeHeap(RtlGetProcessHeap(),
                0,
                *ObjectDescriptor);

    return STATUS_SUCCESS;
}


/*
 * @unimplemented
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
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
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
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
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
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
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
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
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
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
RtlNewInstanceSecurityObject(IN BOOLEAN ParentDescriptorChanged,
                             IN BOOLEAN CreatorDescriptorChanged,
                             IN PLUID OldClientTokenModifiedI,
                             OUT PLUID NewClientTokenModifiedId,
                             IN PSECURITY_DESCRIPTOR ParentDescriptor,
                             IN PSECURITY_DESCRIPTOR CreatorDescriptor,
                             OUT PSECURITY_DESCRIPTOR *NewDescriptor,
                             IN BOOLEAN IsDirectoryObject,
                             IN HANDLE Token,
                             IN PGENERIC_MAPPING GenericMapping)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
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
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
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
 * @unimplemented
 */
NTSTATUS
NTAPI
RtlSetSecurityObject(IN SECURITY_INFORMATION SecurityInformation,
                     IN PSECURITY_DESCRIPTOR ModificationDescriptor,
                     OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
                     IN PGENERIC_MAPPING GenericMapping,
                     IN HANDLE Token)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
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
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
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
