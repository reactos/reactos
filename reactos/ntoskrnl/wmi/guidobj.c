/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/wmidrv.c
 * PURPOSE:         I/O Windows Management Instrumentation (WMI) Support
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <wmistr.h>
#include "wmip.h"

#define NDEBUG
#include <debug.h>


/* GLOBALS ******************************************************************/

POBJECT_TYPE WmipGuidObjectType;
GENERIC_MAPPING WmipGenericMapping;


/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
WmipSecurityMethod(
    _In_ PVOID Object,
    _In_ SECURITY_OPERATION_CODE OperationType,
    _In_ PSECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Inout_ PULONG CapturedLength,
    _Inout_ PSECURITY_DESCRIPTOR *ObjectSecurityDescriptor,
    _In_ POOL_TYPE PoolType,
    _In_ PGENERIC_MAPPING GenericMapping)
{
    PAGED_CODE();

    ASSERT((PoolType == PagedPool) || (PoolType == NonPagedPool));
    ASSERT((OperationType == QuerySecurityDescriptor) ||
           (OperationType == SetSecurityDescriptor) ||
           (OperationType == AssignSecurityDescriptor) ||
           (OperationType == DeleteSecurityDescriptor));

    if (OperationType == QuerySecurityDescriptor)
    {
        return ObQuerySecurityDescriptorInfo(Object,
                                             SecurityInformation,
                                             SecurityDescriptor,
                                             CapturedLength,
                                             ObjectSecurityDescriptor);
    }
    else if (OperationType == SetSecurityDescriptor)
    {
        return ObSetSecurityDescriptorInfo(Object,
                                             SecurityInformation,
                                             SecurityDescriptor,
                                             ObjectSecurityDescriptor,
                                             PoolType,
                                             GenericMapping);
    }
    else if (OperationType == AssignSecurityDescriptor)
    {
        ObAssignObjectSecurityDescriptor(Object, SecurityDescriptor, PoolType);
        return STATUS_SUCCESS;
    }
    else if (OperationType == DeleteSecurityDescriptor)
    {
        return ObDeassignSecurity(ObjectSecurityDescriptor);
    }

    ASSERT(FALSE);
    return STATUS_INVALID_PARAMETER;
}

VOID
NTAPI
WmipDeleteMethod(
    _In_ PVOID Object)
{
    PWMIP_GUID_OBJECT GuidObject = Object;

    /* Check if the object is attached to an IRP */
    if (GuidObject->Irp != NULL)
    {
        /* This is not supported yet */
        ASSERT(FALSE);
    }
}

VOID
NTAPI
WmipCloseMethod(
    _In_opt_ PEPROCESS Process,
    _In_ PVOID Object,
    _In_ ACCESS_MASK GrantedAccess,
    _In_ ULONG ProcessHandleCount,
    _In_ ULONG SystemHandleCount)
{
    /* For now nothing */
}

NTSTATUS
NTAPI
WmipInitializeGuidObjectType(
    VOID)
{
    static UNICODE_STRING GuidObjectName = RTL_CONSTANT_STRING(L"WmiGuid");
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    NTSTATUS Status;

    /* Setup the object type initializer */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.InvalidAttributes = OBJ_OPENLINK;
    ObjectTypeInitializer.GenericMapping = WmipGenericMapping;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.MaintainHandleCount = FALSE;
    ObjectTypeInitializer.ValidAccessMask = STANDARD_RIGHTS_ALL | 0xFFF;
    ObjectTypeInitializer.SecurityRequired = TRUE;
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(WMIP_GUID_OBJECT);;
    ObjectTypeInitializer.SecurityProcedure = WmipSecurityMethod;
    ObjectTypeInitializer.DeleteProcedure = WmipDeleteMethod;
    ObjectTypeInitializer.CloseProcedure = WmipCloseMethod;

    /* Create the object type */
    Status = ObCreateObjectType(&GuidObjectName,
                                &ObjectTypeInitializer,
                                0,
                                &WmipGuidObjectType);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ObCreateObjectType failed: 0x%lx\n", Status);
    }

    return Status;
}

static
NTSTATUS
WmipGUIDFromString(
    _In_ PUNICODE_STRING GuidString,
    _Out_ PGUID Guid)
{
    WCHAR Buffer[GUID_STRING_LENGTH + 2];
    UNICODE_STRING String;

    /* Validate string length */
    if (GuidString->Length != GUID_STRING_LENGTH * sizeof(WCHAR))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Copy the string and wrap it in {} */
    RtlCopyMemory(&Buffer[1], GuidString->Buffer, GuidString->Length);
    Buffer[0] = L'{';
    Buffer[GUID_STRING_LENGTH + 1] = L'}';

    String.Buffer = Buffer;
    String.Length = String.MaximumLength = sizeof(Buffer);

    return RtlGUIDFromString(&String, Guid);
}

static
NTSTATUS
WmipCreateGuidObject(
    _In_ PUNICODE_STRING GuidString,
    _Out_ PWMIP_GUID_OBJECT *OutGuidObject)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    GUID Guid;
    PWMIP_GUID_OBJECT GuidObject;
    NTSTATUS Status;

    /* Convert the string into a GUID structure */
    Status = WmipGUIDFromString(GuidString, &Guid);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("WMI: Invalid uuid format for guid '%wZ'\n", GuidString);
        return Status;
    }

    /* Initialize object attributes for an unnamed object */
    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               0,
                               NULL,
                               NULL); // FIXME: security descriptor!

    /* Create the GUID object */
    Status = ObCreateObject(KernelMode,
                            WmipGuidObjectType,
                            &ObjectAttributes,
                            KernelMode,
                            NULL,
                            sizeof(WMIP_GUID_OBJECT),
                            0,
                            0,
                            (PVOID*)&GuidObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("WMI: failed to create GUID object: 0x%lx\n", Status);
        return Status;
    }

    RtlZeroMemory(GuidObject, sizeof(*GuidObject));
    GuidObject->Guid = Guid;

    *OutGuidObject = GuidObject;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
WmipOpenGuidObject(
    POBJECT_ATTRIBUTES ObjectAttributes,
    ACCESS_MASK DesiredAccess,
    KPROCESSOR_MODE AccessMode,
    PHANDLE OutGuidObjectHandle,
    PVOID *OutGuidObject)
{
    static UNICODE_STRING Prefix = RTL_CONSTANT_STRING(L"\\WmiGuid\\");
    UNICODE_STRING GuidString;
    ULONG HandleAttributes;
    PWMIP_GUID_OBJECT GuidObject;
    NTSTATUS Status;
    PAGED_CODE();

    /* Check if we have the expected prefix */
    if (!RtlPrefixUnicodeString(&Prefix, ObjectAttributes->ObjectName, FALSE))
    {
        DPRINT1("WMI: Invalid prefix for guid object '%wZ'\n",
                ObjectAttributes->ObjectName);
        return STATUS_INVALID_PARAMETER;
    }

    /* Extract the GUID string */
    GuidString = *ObjectAttributes->ObjectName;
    GuidString.Buffer += Prefix.Length / sizeof(WCHAR);
    GuidString.Length -= Prefix.Length;

    /* Create the GUID object */
    Status = WmipCreateGuidObject(&GuidString, &GuidObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create GUID object: 0x%lx\n", Status);
        return Status;
    }

    /* Set handle attributes */
    HandleAttributes = (AccessMode == KernelMode) ? OBJ_KERNEL_HANDLE : 0;

    /* Get a handle for the object */
    Status = ObOpenObjectByPointer(GuidObject,
                                   HandleAttributes,
                                   0,
                                   DesiredAccess,
                                   WmipGuidObjectType,
                                   AccessMode,
                                   OutGuidObjectHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ObOpenObjectByPointer failed: 0x%lx\n", Status);
        ObfDereferenceObject(GuidObject);
    }

    *OutGuidObject = GuidObject;

    return Status;
}

