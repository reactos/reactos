/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver
 * FILE:        afd/afd.c
 * PURPOSE:     MSAFD kernel mode module
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/09-2000 Created
 */
#include <afd.h>

#ifdef DBG

/* See debug.h for debug/trace constants */
DWORD DebugTraceLevel = MIN_TRACE;
//DWORD DebugTraceLevel = DEBUG_ULTRA;

#endif /* DBG */


NPAGED_LOOKASIDE_LIST BufferLookasideList;
NPAGED_LOOKASIDE_LIST ReadRequestLookasideList;


NTSTATUS
STDCALL
AfdFileSystemControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    UNIMPLEMENTED

    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    Irp->IoStatus.Information = 0;
    return STATUS_UNSUCCESSFUL;
}


NTSTATUS
STDCALL
AfdDispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
/*
 * FUNCTION: IOCTL dispatch routine
 * ARGUMENTS:
 *     DeviceObject = Pointer to a device object for this driver
 *     Irp          = Pointer to a I/O request packet
 * RETURNS:
 *     Status of the operation
 */
{
	NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    AFD_DbgPrint(MAX_TRACE, ("Called. DeviceObject is at (0x%X), IRP is at (0x%X), IrpSp->FileObject (0x%X).\n",
        DeviceObject, Irp, IrpSp->FileObject));

    Irp->IoStatus.Information = 0;

    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_AFD_BIND:
        Status = AfdDispBind(Irp, IrpSp);
        break;

    case IOCTL_AFD_LISTEN:
        Status = AfdDispListen(Irp, IrpSp);
        break;

    case IOCTL_AFD_SENDTO:
        Status = AfdDispSendTo(Irp, IrpSp);
        break;

    case IOCTL_AFD_RECVFROM:
        Status = AfdDispRecvFrom(Irp, IrpSp);
        break;

    case IOCTL_AFD_SELECT:
        Status = AfdDispSelect(Irp, IrpSp);
        break;

    default:
        AFD_DbgPrint(MIN_TRACE, ("Unknown IOCTL (0x%X).\n",
            IrpSp->Parameters.DeviceIoControl.IoControlCode));
        Status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    if (Status != STATUS_PENDING) {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    }

    AFD_DbgPrint(MAX_TRACE, ("Leaving. Status (0x%X).\n", Status));

	return Status;
}


VOID AfdUnload(
    PDRIVER_OBJECT DriverObject)
/*
 * FUNCTION: Unloads the driver
 * ARGUMENTS:
 *     DriverObject = Pointer to driver object created by the system
 */
{
	DbgPrint("Unloading Ancillary Function Driver\n");
}


NTSTATUS
STDCALL
DriverEntry(
    PDRIVER_OBJECT DriverObject,
	PUNICODE_STRING RegistryPath)
/*
 * FUNCTION: Called by the system to initialize the driver
 * ARGUMENTS:
 *     DriverObject = object describing this driver
 *     RegistryPath = path to our configuration entries
 * RETURNS:
 *     Status of operation
 */
{
    PDEVICE_EXTENSION DeviceExt;
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING DeviceName;
    NTSTATUS Status;

    DbgPrint("Ancillary Function Driver\n");

    RtlInitUnicodeString(&DeviceName, L"\\Device\\Afd");
    Status = IoCreateDevice(DriverObject,
                            sizeof(DEVICE_EXTENSION),
                            &DeviceName,
                            FILE_DEVICE_NAMED_PIPE,
                            0,
                            FALSE,
                            &DeviceObject);
    if (!NT_SUCCESS(Status)) {
      AFD_DbgPrint(MIN_TRACE, ("Could not create device (0x%X).\n", Status));
	    return Status;
    }

    DeviceObject->Flags |= DO_DIRECT_IO;

    DeviceExt = DeviceObject->DeviceExtension;
    KeInitializeSpinLock(&DeviceExt->FCBListLock);
    InitializeListHead(&DeviceExt->FCBListHead);

    DriverObject->MajorFunction[IRP_MJ_CREATE] = AfdCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = AfdClose;
    DriverObject->MajorFunction[IRP_MJ_READ] = AfdRead;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = AfdWrite;
    DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = AfdFileSystemControl;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = AfdDispatch;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = AfdClose;

    DriverObject->DriverUnload = (PDRIVER_UNLOAD)AfdUnload;

/*    ExInitializeNPagedLookasideList(
      &BufferLookasideList,
      NULL,
      NULL,
      0,
      sizeof(AFD_BUFFER),
      TAG('A', 'F', 'D', 'B'),
      0);*/

/*    ExInitializeNPagedLookasideList(
      &ReadRequestLookasideList,
      NULL,
      NULL,
      0,
      sizeof(AFD_READ_REQUEST),
      TAG('A', 'F', 'D', 'R'),
      0);*/

    
    return STATUS_SUCCESS;
}

/* EOF */
