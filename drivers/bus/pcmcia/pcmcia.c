/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Kernel
 * FILE:        drivers/bus/pcmcia/pcmcia.c
 * PURPOSE:     PCMCIA Bus Driver
 * PROGRAMMERS: Cameron Gutman (cameron.gutman@reactos.org)
 */

#include <pcmcia.h>

//#define NDEBUG
#include <debug.h>

BOOLEAN IoctlEnabled;

NTSTATUS
NTAPI
PcmciaCreateClose(PDEVICE_OBJECT DeviceObject,
                  PIRP Irp)
{
  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;

  DPRINT("PCMCIA: Create/Close\n");

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PcmciaDeviceControl(PDEVICE_OBJECT DeviceObject,
                    PIRP Irp)
{
  PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
  NTSTATUS Status;

  DPRINT("PCMCIA: DeviceIoControl\n");

  Irp->IoStatus.Information = 0;

  switch (IrpSp->Parameters.DeviceIoControl.IoControlCode)
  {
     default:
       DPRINT1("PCMCIA: Unknown ioctl code: %x\n", IrpSp->Parameters.DeviceIoControl.IoControlCode);
       Status = STATUS_NOT_SUPPORTED;
  }

  Irp->IoStatus.Status = Status;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return Status;
}

VOID
NTAPI
PcmciaUnload(PDRIVER_OBJECT DriverObject)
{
  DPRINT("PCMCIA: Unload\n");
}

NTSTATUS
NTAPI
PcmciaPlugPlay(PDEVICE_OBJECT DeviceObject,
               PIRP Irp)
{
  PPCMCIA_COMMON_EXTENSION Common = DeviceObject->DeviceExtension;

  DPRINT("PCMCIA: PnP\n");
  if (Common->IsFDO)
  {
     return PcmciaFdoPlugPlay((PPCMCIA_FDO_EXTENSION)Common,
                              Irp);
  }
  else
  {
     return PcmciaPdoPlugPlay((PPCMCIA_PDO_EXTENSION)Common,
                              Irp);
  }
}

NTSTATUS
NTAPI
PcmciaPower(PDEVICE_OBJECT DeviceObject,
            PIRP Irp)
{
  PPCMCIA_COMMON_EXTENSION Common = DeviceObject->DeviceExtension;
  PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
  NTSTATUS Status;

  switch (IrpSp->MinorFunction)
  {
     case IRP_MN_QUERY_POWER:
       /* I don't see any reason that we should care */
       DPRINT("PCMCIA: IRP_MN_QUERY_POWER\n");
       Status = STATUS_SUCCESS;
       break;

     case IRP_MN_POWER_SEQUENCE:
       DPRINT("PCMCIA: IRP_MN_POWER_SEQUENCE\n");
       RtlCopyMemory(IrpSp->Parameters.PowerSequence.PowerSequence,
                     &Common->PowerSequence,
                     sizeof(POWER_SEQUENCE));
       Status = STATUS_SUCCESS;
       break;

     case IRP_MN_WAIT_WAKE:
       /* Not really sure about this */
       DPRINT("PCMCIA: IRP_MN_WAIT_WAKE\n");
       Status = STATUS_NOT_SUPPORTED;
       break;

     case IRP_MN_SET_POWER:
       DPRINT("PCMCIA: IRP_MN_SET_POWER\n");
       if (IrpSp->Parameters.Power.Type == SystemPowerState)
       {
          Common->SystemPowerState = IrpSp->Parameters.Power.State.SystemState;

          Status = STATUS_SUCCESS;
       }
       else
       {
          Common->DevicePowerState = IrpSp->Parameters.Power.State.DeviceState;

          /* Update the POWER_SEQUENCE struct */
          if (Common->DevicePowerState <= PowerDeviceD1)
              Common->PowerSequence.SequenceD1++;

          if (Common->DevicePowerState <= PowerDeviceD2)
              Common->PowerSequence.SequenceD2++;

          if (Common->DevicePowerState <= PowerDeviceD3)
              Common->PowerSequence.SequenceD3++;

          /* Start the underlying device if we are handling this for a PDO */
          if (!Common->IsFDO)
              Status = PcmciaPdoSetPowerState((PPCMCIA_PDO_EXTENSION)Common);
          else
              Status = STATUS_SUCCESS;
       }

       /* Report that we changed state to the Power Manager */
       PoSetPowerState(DeviceObject, IrpSp->Parameters.Power.Type, IrpSp->Parameters.Power.State);
       break;

     default:
       DPRINT1("PCMCIA: Invalid MN code in MJ_POWER handler %x\n", IrpSp->MinorFunction);
       ASSERT(FALSE);
       Status = STATUS_INVALID_DEVICE_REQUEST;
       break;
  }

  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = 0;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return Status;
}

NTSTATUS
NTAPI
PcmciaAddDevice(PDRIVER_OBJECT DriverObject,
                PDEVICE_OBJECT PhysicalDeviceObject)
{
  PPCMCIA_FDO_EXTENSION FdoExt;
  PDEVICE_OBJECT Fdo;
  NTSTATUS Status;

  DPRINT("PCMCIA: AddDevice\n");

  Status = IoCreateDevice(DriverObject,
                          sizeof(*FdoExt),
                          NULL,
                          FILE_DEVICE_BUS_EXTENDER,
                          FILE_DEVICE_SECURE_OPEN,
                          FALSE,
                          &Fdo);
  if (!NT_SUCCESS(Status)) return Status;

  FdoExt = Fdo->DeviceExtension;

  RtlZeroMemory(FdoExt, sizeof(*FdoExt));

  InitializeListHead(&FdoExt->ChildDeviceList);
  KeInitializeSpinLock(&FdoExt->Lock);

  FdoExt->Common.Self = Fdo;
  FdoExt->Common.IsFDO = TRUE;
  FdoExt->Common.State = dsStopped;

  FdoExt->Ldo = IoAttachDeviceToDeviceStack(Fdo,
                                            PhysicalDeviceObject);

  Fdo->Flags &= ~DO_DEVICE_INITIALIZING;

  return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject,
            PUNICODE_STRING RegistryPath)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[2];
  NTSTATUS Status;

  DPRINT1("PCMCIA: DriverEntry\n");

  DriverObject->MajorFunction[IRP_MJ_CREATE] = PcmciaCreateClose;
  DriverObject->MajorFunction[IRP_MJ_CLOSE] = PcmciaCreateClose;
  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = PcmciaDeviceControl;
  DriverObject->MajorFunction[IRP_MJ_PNP] = PcmciaPlugPlay;
  DriverObject->MajorFunction[IRP_MJ_POWER] = PcmciaPower;

  DriverObject->DriverExtension->AddDevice = PcmciaAddDevice;
  DriverObject->DriverUnload = PcmciaUnload;

  RtlZeroMemory(QueryTable, sizeof(RTL_QUERY_REGISTRY_TABLE) * 2);

  QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
  QueryTable[0].Name = L"IoctlInterface";
  QueryTable[0].EntryContext = &IoctlEnabled;

  Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                                  L"Pcmcia\\Parameters",
                                  QueryTable,
                                  NULL,
                                  NULL);
  if (!NT_SUCCESS(Status))
  {
      /* Key not present so assume disabled */
      IoctlEnabled = FALSE;
  }

  DPRINT("PCMCIA: Ioctl interface %s\n",
         (IoctlEnabled ? "enabled" : "disabled"));

  return STATUS_SUCCESS;
}
