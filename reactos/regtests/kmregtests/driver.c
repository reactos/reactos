/*
 * PROJECT:         ReactOS kernel
 * FILE:            regtests/kmregtests/driver.c
 * PURPOSE:         Kernel-mode regression testing driver
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      06-07-2003  CSH  Created
 */
#define NTOS_MODE_KERNEL
#include <ntos.h>
#include "regtests.h"
#include "kmregtests.h"

#define NDEBUG
#include <debug.h>

PVOID
AllocateMemory(ULONG Size)
{
  return ExAllocatePool(NonPagedPool, Size);
}


VOID
FreeMemory(PVOID Base)
{
  ExFreePool(NonPagedPool);
}

VOID
ShutdownBochs()
{
  /* Shutdown bochs programmatically */
  WRITE_PORT_BUFFER_UCHAR((PUCHAR) 0x8900,
    (PUCHAR) "Shutdown",
    strlen("Shutdown"));
}

NTSTATUS
STDCALL
KMRegTestsRun(
  PIRP Irp,
  PIO_STACK_LOCATION IrpSp)
{
  InitializeTests();
  RegisterTests();
  PerformTests();
  ShutdownBochs();

  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;
  return STATUS_SUCCESS;
}

NTSTATUS
STDCALL
KMRegTestsDispatch(
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

  DPRINT("Called. DeviceObject is at (0x%X), IRP is at (0x%X), IrpSp->FileObject (0x%X).\n",
      DeviceObject, Irp, IrpSp->FileObject);

  Irp->IoStatus.Information = 0;

  switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {
  case IOCTL_KMREGTESTS_RUN:
      Status = KMRegTestsRun(Irp, IrpSp);
      break;

  default:
      DPRINT("Unknown IOCTL (0x%X).\n",
        IrpSp->Parameters.DeviceIoControl.IoControlCode);
      Status = STATUS_NOT_IMPLEMENTED;
      break;
  }

  if (Status != STATUS_PENDING) {
      Irp->IoStatus.Status = Status;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
  }

  DPRINT("Leaving. Status (0x%X).\n", Status);

	return Status;
}

NTSTATUS STDCALL
KMRegTestsOpenClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
  PIO_STACK_LOCATION piosStack = IoGetCurrentIrpStackLocation(Irp);
  NTSTATUS nErrCode;

  nErrCode = STATUS_SUCCESS;

  switch(piosStack->MajorFunction)
    {
      /* Opening and closing handles to the device */
      case IRP_MJ_CREATE:
      case IRP_MJ_CLOSE:
        break;

      /* Write data */
      case IRP_MJ_WRITE:
        /* Ignore */
        Irp->IoStatus.Information = 0;
        break;

      /* Read data */
      case IRP_MJ_READ:
        /* Ignore */
        Irp->IoStatus.Information = 0;
        nErrCode = STATUS_END_OF_FILE;
        break;

      /* Unsupported operations */
      default:
        nErrCode = STATUS_NOT_IMPLEMENTED;
    }

  Irp->IoStatus.Status = nErrCode;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return nErrCode;
}

NTSTATUS STDCALL
KMRegTestsUnload(PDRIVER_OBJECT DriverObject)
{
  return STATUS_SUCCESS;
}

NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT DriverObject,
	    PUNICODE_STRING RegistryPath)
{
  PDEVICE_OBJECT DeviceObject;
  UNICODE_STRING DeviceName;
  UNICODE_STRING DosName;
  NTSTATUS Status;

  /* Register driver routines */
  DriverObject->MajorFunction[IRP_MJ_CLOSE] = (PDRIVER_DISPATCH) KMRegTestsOpenClose;
  DriverObject->MajorFunction[IRP_MJ_CREATE] = (PDRIVER_DISPATCH) KMRegTestsOpenClose;
  DriverObject->MajorFunction[IRP_MJ_WRITE] = (PDRIVER_DISPATCH) KMRegTestsOpenClose;
  DriverObject->MajorFunction[IRP_MJ_READ] = (PDRIVER_DISPATCH) KMRegTestsOpenClose;
  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = (PDRIVER_DISPATCH) KMRegTestsDispatch;
  DriverObject->DriverUnload = (PDRIVER_UNLOAD) KMRegTestsUnload;

  /* Create device */
  RtlInitUnicodeString(&DeviceName,
    L"\\Device\\KMRegTests");

  Status = IoCreateDevice(DriverObject,
    0,
    &DeviceName,
    FILE_DEVICE_NULL,
    0,
    FALSE,
    &DeviceObject);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  DeviceObject->Flags |= DO_BUFFERED_IO;

  return Status;
}
