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
#include <wmiguid.h>
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
    UNIMPLEMENTED_DBGBREAK();
    return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
WmipDeleteMethod(
    _In_ PVOID Object)
{
    UNIMPLEMENTED_DBGBREAK();
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
    UNIMPLEMENTED_DBGBREAK();
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
WmipCreateGuidObject(
    _In_ PUNICODE_STRING GuidString,
    _Out_ PWMIP_GUID_OBJECT *OutGuidObject)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    GUID Guid;
    PWMIP_GUID_OBJECT GuidObject;
    NTSTATUS Status;

    /* Convert the string into a GUID structure */
    Status = RtlGUIDFromString(GuidString, &Guid);
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
    if (!RtlPrefixUnicodeString(ObjectAttributes->ObjectName, &Prefix, FALSE))
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

