/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/wmi.c
 * PURPOSE:         I/O Windows Management Instrumentation (WMI) Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define INITGUID
#include <wmiguid.h>
#include <wmidata.h>
#include <wmistr.h>

#include "wmip.h"

#define NDEBUG
#include <debug.h>

typedef PVOID PWMI_LOGGER_INFORMATION; // FIXME

typedef enum _WMI_CLOCK_TYPE
{
    WMICT_DEFAULT,
    WMICT_SYSTEMTIME,
    WMICT_PERFCOUNTER,
    WMICT_PROCESS,
    WMICT_THREAD,
    WMICT_CPUCYCLE
} WMI_CLOCK_TYPE;

/* FUNCTIONS *****************************************************************/

BOOLEAN
NTAPI
WmiInitialize(
    VOID)
{
    UNICODE_STRING DriverName = RTL_CONSTANT_STRING(L"\\Driver\\WMIxWDM");
    NTSTATUS Status;

    /* Initialize the GUID object type */
    Status = WmipInitializeGuidObjectType();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("WmipInitializeGuidObjectType() failed: 0x%lx\n", Status);
        return FALSE;
    }

    /* Create the WMI driver */
    Status = IoCreateDriver(&DriverName, WmipDriverEntry);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create WMI driver: 0x%lx\n", Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoWMIRegistrationControl(IN PDEVICE_OBJECT DeviceObject,
                         IN ULONG Action)
{
    DPRINT1("IoWMIRegistrationControl() called for DO %p, requesting %lu action, returning success\n",
        DeviceObject, Action);

    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoWMIAllocateInstanceIds(IN GUID *Guid,
                         IN ULONG InstanceCount,
                         OUT ULONG *FirstInstanceId)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoWMISuggestInstanceName(IN PDEVICE_OBJECT PhysicalDeviceObject OPTIONAL,
                         IN PUNICODE_STRING SymbolicLinkName OPTIONAL,
                         IN BOOLEAN CombineNames,
                         OUT PUNICODE_STRING SuggestedInstanceName)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoWMIWriteEvent(IN PVOID WnodeEventItem)
{
    DPRINT1("IoWMIWriteEvent() called for WnodeEventItem %p, returning success\n",
        WnodeEventItem);

    /* Free the buffer if we are returning success */
    if (WnodeEventItem != NULL)
        ExFreePool(WnodeEventItem);

    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoWMIOpenBlock(
    _In_ LPCGUID DataBlockGuid,
    _In_ ULONG DesiredAccess,
    _Out_ PVOID *DataBlockObject)
{
    HANDLE GuidObjectHandle;
    NTSTATUS Status;

    /* Open the GIOD object */
    Status = WmipOpenGuidObject(DataBlockGuid,
                                DesiredAccess,
                                KernelMode,
                                &GuidObjectHandle,
                                DataBlockObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("WmipOpenGuidObject failed: 0x%lx\n", Status);
        return Status;
    }


    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoWMIQueryAllData(
    IN PVOID DataBlockObject,
    IN OUT ULONG *InOutBufferSize,
    OUT PVOID OutBuffer)
{
    PWMIP_GUID_OBJECT GuidObject;
    NTSTATUS Status;


    Status = ObReferenceObjectByPointer(DataBlockObject,
                                        WMIGUID_QUERY,
                                        WmipGuidObjectType,
                                        KernelMode);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    GuidObject = DataBlockObject;

    /* Huge HACK! */
    if (IsEqualGUID(&GuidObject->Guid, &MSSmBios_RawSMBiosTables_GUID))
    {
        Status = WmipQueryRawSMBiosTables(InOutBufferSize, OutBuffer);
    }
    else
    {
        Status = STATUS_NOT_SUPPORTED;
    }

    ObDereferenceObject(DataBlockObject);

    return Status;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoWMIQueryAllDataMultiple(IN PVOID *DataBlockObjectList,
                          IN ULONG ObjectCount,
                          IN OUT ULONG *InOutBufferSize,
                          OUT PVOID OutBuffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoWMIQuerySingleInstance(IN PVOID DataBlockObject,
                         IN PUNICODE_STRING InstanceName,
                         IN OUT ULONG *InOutBufferSize,
                         OUT PVOID OutBuffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoWMIQuerySingleInstanceMultiple(IN PVOID *DataBlockObjectList,
                                 IN PUNICODE_STRING InstanceNames,
                                 IN ULONG ObjectCount,
                                 IN OUT ULONG *InOutBufferSize,
                                 OUT PVOID OutBuffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoWMISetSingleInstance(IN PVOID DataBlockObject,
                       IN PUNICODE_STRING InstanceName,
                       IN ULONG Version,
                       IN ULONG ValueBufferSize,
                       IN PVOID ValueBuffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoWMISetSingleItem(IN PVOID DataBlockObject,
                   IN PUNICODE_STRING InstanceName,
                   IN ULONG DataItemId,
                   IN ULONG Version,
                   IN ULONG ValueBufferSize,
                   IN PVOID ValueBuffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoWMIExecuteMethod(IN PVOID DataBlockObject,
                   IN PUNICODE_STRING InstanceName,
                   IN ULONG MethodId,
                   IN ULONG InBufferSize,
                   IN OUT PULONG OutBufferSize,
                   IN OUT PUCHAR InOutBuffer)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoWMISetNotificationCallback(IN PVOID Object,
                             IN WMI_NOTIFICATION_CALLBACK Callback,
                             IN PVOID Context)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoWMIHandleToInstanceName(IN PVOID DataBlockObject,
                          IN HANDLE FileHandle,
                          OUT PUNICODE_STRING InstanceName)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoWMIDeviceObjectToInstanceName(IN PVOID DataBlockObject,
                                IN PDEVICE_OBJECT DeviceObject,
                                OUT PUNICODE_STRING InstanceName)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
WmiQueryTraceInformation(IN TRACE_INFORMATION_CLASS TraceInformationClass,
                         OUT PVOID TraceInformation,
                         IN ULONG TraceInformationLength,
                         OUT PULONG RequiredLength OPTIONAL,
                         IN PVOID Buffer OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
__cdecl
WmiTraceMessage(IN TRACEHANDLE LoggerHandle,
                IN ULONG MessageFlags,
                IN LPGUID MessageGuid,
                IN USHORT MessageNumber,
                IN ...)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
WmiTraceMessageVa(IN TRACEHANDLE LoggerHandle,
                  IN ULONG MessageFlags,
                  IN LPGUID MessageGuid,
                  IN USHORT MessageNumber,
                  IN va_list MessageArgList)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
WmiFlushTrace(IN OUT PWMI_LOGGER_INFORMATION LoggerInfo)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

LONG64
FASTCALL
WmiGetClock(IN WMI_CLOCK_TYPE ClockType,
            IN PVOID Context)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
WmiQueryTrace(IN OUT PWMI_LOGGER_INFORMATION LoggerInfo)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
WmiStartTrace(IN OUT PWMI_LOGGER_INFORMATION LoggerInfo)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}
    
NTSTATUS
NTAPI
WmiStopTrace(IN PWMI_LOGGER_INFORMATION LoggerInfo)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
FASTCALL
WmiTraceFastEvent(IN PWNODE_HEADER Wnode)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
WmiUpdateTrace(IN OUT PWMI_LOGGER_INFORMATION LoggerInfo)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtTraceEvent(IN ULONG TraceHandle,
             IN ULONG Flags,
             IN ULONG TraceHeaderLength,
             IN struct _EVENT_TRACE_HEADER* TraceHeader)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*Eof*/
