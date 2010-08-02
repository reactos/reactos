/*
 * PROJECT:         ReactOS ISA PnP Bus driver
 * FILE:            isapnp.c
 * PURPOSE:         Driver entry
 * PROGRAMMERS:     Cameron Gutman (cameron.gutman@reactos.org)
 */
#include <isapnp.h>

#define NDEBUG
#include <debug.h>

static
NTSTATUS
NTAPI
ForwardIrpCompletion(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	IN PVOID Context)
{
  if (Irp->PendingReturned)
    KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);

  return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
IsaForwardIrpSynchronous(
	IN PISAPNP_FDO_EXTENSION FdoExt,
	IN PIRP Irp)
{
  KEVENT Event;
  NTSTATUS Status;

  KeInitializeEvent(&Event, NotificationEvent, FALSE);
  IoCopyCurrentIrpStackLocationToNext(Irp);

  IoSetCompletionRoutine(Irp, ForwardIrpCompletion, &Event, TRUE, TRUE, TRUE);

  Status = IoCallDriver(FdoExt->Ldo, Irp);
  if (Status == STATUS_PENDING)
  {
      Status = KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
      if (NT_SUCCESS(Status))
	  Status = Irp->IoStatus.Status;
  }

  return Status;
}


static
NTSTATUS
NTAPI
IsaCreateClose(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp)
{
  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = FILE_OPENED;

  DPRINT("%s(%p, %p)\n", __FUNCTION__, DeviceObject, Irp);

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
IsaIoctl(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp)
{
  PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
  NTSTATUS Status;

  DPRINT("%s(%p, %p)\n", __FUNCTION__, DeviceObject, Irp);

  switch (IrpSp->Parameters.DeviceIoControl.IoControlCode)
  {
     default:
        DPRINT1("Unknown ioctl code: %x\n", IrpSp->Parameters.DeviceIoControl.IoControlCode);
        Status = STATUS_NOT_SUPPORTED;
        break;
  }

  Irp->IoStatus.Status = Status;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return Status;
}

static
NTSTATUS
NTAPI
IsaReadWrite(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp)
{
  DPRINT("%s(%p, %p)\n", __FUNCTION__, DeviceObject, Irp);

  Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
  Irp->IoStatus.Information = 0;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return STATUS_NOT_SUPPORTED;
}

static
NTSTATUS
NTAPI
IsaAddDevice(
  IN PDRIVER_OBJECT DriverObject,
  IN PDEVICE_OBJECT PhysicalDeviceObject)
{
  PDEVICE_OBJECT Fdo;
  PISAPNP_FDO_EXTENSION FdoExt;
  NTSTATUS Status;

  DPRINT("%s(%p, %p)\n", __FUNCTION__, DriverObject, PhysicalDeviceObject);

  Status = IoCreateDevice(DriverObject,
                          sizeof(*FdoExt),
                          NULL,
                          FILE_DEVICE_BUS_EXTENDER,
                          FILE_DEVICE_SECURE_OPEN,
                          TRUE,
                          &Fdo);
  if (!NT_SUCCESS(Status))
  {
      DPRINT1("Failed to create FDO (0x%x)\n", Status);
      return Status;
  }

  FdoExt = Fdo->DeviceExtension;
  RtlZeroMemory(FdoExt, sizeof(*FdoExt));

  FdoExt->Common.Self = Fdo;
  FdoExt->Common.IsFdo = TRUE;
  FdoExt->Common.State = dsStopped;
  FdoExt->Pdo = PhysicalDeviceObject;
  FdoExt->Ldo = IoAttachDeviceToDeviceStack(Fdo,
                                            PhysicalDeviceObject);

  InitializeListHead(&FdoExt->DeviceListHead);
  KeInitializeSpinLock(&FdoExt->Lock);

  Fdo->Flags &= ~DO_DEVICE_INITIALIZING;

  return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
IsaPnp(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp)
{
  PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
  PISAPNP_COMMON_EXTENSION DevExt = DeviceObject->DeviceExtension;

  DPRINT("%s(%p, %p)\n", __FUNCTION__, DeviceObject, Irp);

  if (DevExt->IsFdo)
  {
     return IsaFdoPnp((PISAPNP_FDO_EXTENSION)DevExt,
                      Irp,
                      IrpSp);
  }
  else
  {
     return IsaPdoPnp((PISAPNP_LOGICAL_DEVICE)DevExt,
                      Irp,
                      IrpSp);
  }
}

NTSTATUS
NTAPI
DriverEntry(
  IN PDRIVER_OBJECT DriverObject,
  IN PUNICODE_STRING RegistryPath)
{
  DPRINT("%s(%p, %wZ)\n", __FUNCTION__, DriverObject, RegistryPath);

  DriverObject->MajorFunction[IRP_MJ_CREATE] = IsaCreateClose;
  DriverObject->MajorFunction[IRP_MJ_CLOSE] = IsaCreateClose;
  DriverObject->MajorFunction[IRP_MJ_READ] = IsaReadWrite;
  DriverObject->MajorFunction[IRP_MJ_WRITE] = IsaReadWrite;
  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IsaIoctl;
  DriverObject->MajorFunction[IRP_MJ_PNP] = IsaPnp;
  DriverObject->DriverExtension->AddDevice = IsaAddDevice;

  return STATUS_SUCCESS;
}

/* EOF */
