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
#include <wmiioctl.h>
#include "wmip.h"

#define NDEBUG
#include <debug.h>

// FIXME: these should go to a shared header
typedef struct _WMIP_REGISTER_GUIDS
{
    POBJECT_ATTRIBUTES ObjectAttributes;
    ULONG Unknown04;
    ULONG Unknown08;
    ULONG Unknown0C;
    ULONG Unknown10;
    ULONG Unknown14;

    WMIREGINFOW RegInfo;
} WMIP_REGISTER_GUIDS, *PWMIP_REGISTER_GUIDS;

typedef struct _WMIP_RESULT
{
    HANDLE Handle;
    ULONG Unknown04;
    ULONG Unknown08;
    ULONG Unknown0C;
    BOOLEAN Unknown10;
} WMIP_RESULT, *PWMIP_RESULT;


PDEVICE_OBJECT WmipServiceDeviceObject;
PDEVICE_OBJECT WmipAdminDeviceObject;
FAST_IO_DISPATCH WmipFastIoDispatch;


/* FUNCTIONS *****************************************************************/

DRIVER_DISPATCH WmipOpenCloseCleanup;
DRIVER_DISPATCH WmipIoControl;
DRIVER_DISPATCH WmipSystemControl;
DRIVER_DISPATCH WmipShutdown;

NTSTATUS
NTAPI
WmipOpenCloseCleanup(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    PAGED_CODE();

    /* No work to do, just return success */
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static
NTSTATUS
WmiTraceEvent(
    PVOID InputBuffer,
    KPROCESSOR_MODE PreviousMode)
{
    UNIMPLEMENTED_DBGBREAK();
    return STATUS_SUCCESS;
}

static
NTSTATUS
WmiTraceUserMessage(
    PVOID InputBuffer,
    ULONG InputBufferLength)
{
    UNIMPLEMENTED_DBGBREAK();
    return STATUS_SUCCESS;
}

static
NTSTATUS
WmipRegisterGuids(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PVOID Buffer,
    _In_ ULONG InputLength,
    _Inout_ PULONG OutputLength)
{
    PWMIP_REGISTER_GUIDS RegisterGuids = (PWMIP_REGISTER_GUIDS)Buffer;
    PWMIP_RESULT Result = (PWMIP_RESULT)Buffer;
    OBJECT_ATTRIBUTES LocalObjectAttributes;
    UNICODE_STRING LocalObjectName;
    WCHAR LocalObjectNameBuffer[45 + 1];
    KPROCESSOR_MODE PreviousMode;
    HANDLE GuidObjectHandle;
    PVOID GuidObject;
    NTSTATUS Status;

    /* Make sure the input buffer is large enough */
    if ((InputLength < sizeof(WMIP_REGISTER_GUIDS)) ||
        (RegisterGuids->RegInfo.BufferSize > 
         (InputLength - FIELD_OFFSET(WMIP_REGISTER_GUIDS, RegInfo))))
    {
        return STATUS_UNSUCCESSFUL;
    }

    /* Make sure we have a resonable GUID count */
    if ((RegisterGuids->RegInfo.GuidCount == 0) ||
        (RegisterGuids->RegInfo.GuidCount > 0x10000))
    {
        return STATUS_UNSUCCESSFUL;
    }

    _SEH2_TRY
    {
        /* Probe and copy the object attributes structure */
        ProbeForRead(RegisterGuids->ObjectAttributes,
                     sizeof(OBJECT_ATTRIBUTES),
                     sizeof(PVOID));
        LocalObjectAttributes = *RegisterGuids->ObjectAttributes;

        /* Probe and copy the object name UNICODE_STRING */
        ProbeForRead(LocalObjectAttributes.ObjectName,
                     sizeof(UNICODE_STRING),
                     sizeof(PVOID));
        LocalObjectName = *LocalObjectAttributes.ObjectName;

        /* Check if the object name has the expected length */
        if (LocalObjectName.Length != 45 * sizeof(WCHAR))
        {
            return STATUS_INVALID_PARAMETER;
        }

        /* Probe and copy the object name buffer */
        ProbeForRead(LocalObjectName.Buffer, LocalObjectName.Length, sizeof(WCHAR));
        RtlCopyMemory(LocalObjectNameBuffer,
                      LocalObjectName.Buffer,
                      LocalObjectName.Length);

        /* Fix pointers */
        LocalObjectName.Buffer = LocalObjectNameBuffer;
        LocalObjectAttributes.ObjectName = &LocalObjectName;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT1("Got exception!\n");
        return _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    /* Open a new GUID object */
    PreviousMode = ExGetPreviousMode();
    Status = WmipOpenGuidObject(&LocalObjectAttributes,
                                SPECIFIC_RIGHTS_ALL,
                                PreviousMode,
                                &GuidObjectHandle,
                                &GuidObject);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Derefernce the GUID object */
    ObDereferenceObject(GuidObject);

    /* Return the handle */
    Result->Handle = GuidObjectHandle;
    *OutputLength = 24;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
WmipIoControl(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStackLocation;
    ULONG IoControlCode;
    PVOID Buffer;
    ULONG InputLength, OutputLength;
    NTSTATUS Status;
    PAGED_CODE();

    /* Get the current stack location */
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    /* Get the io control parameters */
    IoControlCode = IoStackLocation->Parameters.DeviceIoControl.IoControlCode;
    Buffer = Irp->AssociatedIrp.SystemBuffer;
    InputLength = IoStackLocation->Parameters.DeviceIoControl.InputBufferLength;
    OutputLength = IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength;

    switch (IoControlCode)
    {

        case IOCTL_WMI_REGISTER_GUIDS:
        {
            Status = WmipRegisterGuids(DeviceObject,
                                       Buffer,
                                       InputLength,
                                       &OutputLength);
            break;
        }

        default:
            DPRINT1("Unsupported yet IOCTL: 0x%lx\n", IoControlCode);
            Status = STATUS_INVALID_DEVICE_REQUEST;
            __debugbreak();
            break;
    }

    if (Status == STATUS_PENDING)
        return Status;

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = NT_SUCCESS(Status) ? OutputLength : 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
NTAPI
WmipSystemControl(
  _Inout_  PDEVICE_OBJECT DeviceObject,
  _Inout_  PIRP Irp)
{
    UNIMPLEMENTED_DBGBREAK();
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
WmipShutdown(
  _Inout_  PDEVICE_OBJECT DeviceObject,
  _Inout_  PIRP Irp)
{
    UNIMPLEMENTED_DBGBREAK();
    return STATUS_NOT_IMPLEMENTED;
}

_Function_class_(FAST_IO_DEVICE_CONTROL)
_IRQL_requires_same_
BOOLEAN
NTAPI
WmipFastIoDeviceControl(
    _In_ PFILE_OBJECT FileObject,
    _In_ BOOLEAN Wait,
    _In_opt_ PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_opt_ PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _In_ ULONG IoControlCode,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject)
{
    PAGED_CODE();

    if (IoControlCode == IOCTL_WMI_TRACE_EVENT)
    {
        if (InputBufferLength < 0x30)
        {
            DPRINT1("Buffer too small\n");
            return FALSE;
        }

        IoStatus->Status = WmiTraceEvent(InputBuffer, ExGetPreviousMode());
        return TRUE;
    }
    else if (IoControlCode == IOCTL_WMI_TRACE_USER_MESSAGE)
    {
        if (InputBufferLength < 0x30)
        {
            DPRINT1("Buffer too small\n");
            return FALSE;
        }

        IoStatus->Status = WmiTraceUserMessage(InputBuffer, InputBufferLength);
        return TRUE;
    }

    DPRINT1("Invalid io control code for fast dispatch: 0x%lx\n", IoControlCode);
    return FALSE;
}

NTSTATUS
NTAPI
WmipDockUndockEventCallback(
    _In_ PVOID NotificationStructure,
    _Inout_opt_ PVOID Context)
{
    UNIMPLEMENTED_DBGBREAK();
    return STATUS_NOT_IMPLEMENTED;
}

_Function_class_(DRIVER_INITIALIZE)
_IRQL_requires_same_
NTSTATUS
NTAPI
WmipDriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    static UNICODE_STRING ServiceDeviceName = RTL_CONSTANT_STRING(L"\\Device\\WMIDataDevice");
    static UNICODE_STRING ServiceDosDeviceName = RTL_CONSTANT_STRING(L"\\DosDevices\\WMIDataDevice");
    static UNICODE_STRING AdminDeviceName = RTL_CONSTANT_STRING(L"\\Device\\WMIAdminDevice");
    static UNICODE_STRING AdminDosDeviceName = RTL_CONSTANT_STRING(L"\\DosDevices\\WMIAdminDevice");
    NTSTATUS Status;
    PAGED_CODE();

    /* Create the service device object */
    Status = IoCreateDevice(DriverObject,
                            0,
                            &ServiceDeviceName,
                            FILE_DEVICE_UNKNOWN,
                            FILE_DEVICE_SECURE_OPEN,
                            0,
                            &WmipServiceDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create service device: 0x%lx\n", Status);
        return Status;
    }

    /* Create a symbolic link for the service device */
    Status = IoCreateSymbolicLink(&ServiceDosDeviceName, &ServiceDeviceName);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoCreateSymbolicLink() failed: 0x%lx\n", Status);
        IoDeleteDevice(WmipServiceDeviceObject);
        return Status;
    }

    /* Create the admin device object */
    Status = IoCreateDevice(DriverObject,
                            0,
                            &AdminDeviceName,
                            FILE_DEVICE_UNKNOWN,
                            FILE_DEVICE_SECURE_OPEN,
                            0,
                            &WmipAdminDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create admin device: 0x%lx\n", Status);
        IoDeleteDevice(WmipServiceDeviceObject);
        IoDeleteSymbolicLink(&ServiceDosDeviceName);
        return Status;
    }

    /* Create a symbolic link for the admin device */
    Status = IoCreateSymbolicLink(&AdminDosDeviceName, &AdminDeviceName);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoCreateSymbolicLink() failed: 0x%lx\n", Status);
        IoDeleteSymbolicLink(&ServiceDosDeviceName);
        IoDeleteDevice(WmipServiceDeviceObject);
        IoDeleteDevice(WmipAdminDeviceObject);
        return Status;
    }

    /* Initialize dispatch routines */
    DriverObject->MajorFunction[IRP_MJ_CREATE] = WmipOpenCloseCleanup;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = WmipOpenCloseCleanup;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = WmipIoControl;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = WmipOpenCloseCleanup;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = WmipSystemControl;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = WmipShutdown;

    /* Initialize fast dispatch */
    RtlZeroMemory(&WmipFastIoDispatch, sizeof(WmipFastIoDispatch));
    WmipFastIoDispatch.SizeOfFastIoDispatch = sizeof(WmipFastIoDispatch);
    WmipFastIoDispatch.FastIoDeviceControl = WmipFastIoDeviceControl;
    DriverObject->FastIoDispatch = &WmipFastIoDispatch;

    /* Register the WMI service device */
    IoWMIRegistrationControl(WmipServiceDeviceObject, WMIREG_ACTION_REGISTER);

    /* Register a shutdown notification */
    IoRegisterShutdownNotification(WmipServiceDeviceObject);

    /* Initialization is done */
    WmipServiceDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    WmipAdminDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}


