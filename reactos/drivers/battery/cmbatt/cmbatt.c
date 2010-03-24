/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/battery/cmbatt/cmbatt.c
 * PURPOSE:         Control Method Battery Miniclass Driver
 * PROGRAMMERS:     Cameron Gutman (cameron.gutman@reactos.org)
 */

#include <cmbatt.h>

#define NDEBUG
#include <debug.h>

LIST_ENTRY BatteryList;
KSPIN_LOCK BatteryListLock;

VOID
NTAPI
CmBattUnload(PDRIVER_OBJECT DriverObject)
{
  DPRINT("Control method battery miniclass driver unloaded\n");
}

NTSTATUS
NTAPI
CmBattDeviceControl(PDEVICE_OBJECT DeviceObject,
                    PIRP Irp)
{
  PCMBATT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
  NTSTATUS Status;

  Status = BatteryClassIoctl(DeviceExtension->BattClassHandle,
                             Irp);

  if (Status == STATUS_NOT_SUPPORTED)
  {
      Irp->IoStatus.Status = Status;
      Irp->IoStatus.Information = 0;

      IoCompleteRequest(Irp, IO_NO_INCREMENT);
  }

  return Status;
}

NTSTATUS
NTAPI
CmBattPnP(PDEVICE_OBJECT DeviceObject,
          PIRP Irp)
{
  PCMBATT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;

  UNIMPLEMENTED

  IoSkipCurrentIrpStackLocation(Irp);

  return IoCallDriver(DeviceExtension->Ldo, Irp);
}

NTSTATUS
NTAPI
CmBattSystemControl(PDEVICE_OBJECT DeviceObject,
                    PIRP Irp)
{
  UNIMPLEMENTED

  Irp->IoStatus.Status = STATUS_WMI_GUID_NOT_FOUND;
  Irp->IoStatus.Information = 0;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return STATUS_WMI_GUID_NOT_FOUND;
}

NTSTATUS
NTAPI
CmBattPower(PDEVICE_OBJECT DeviceObject,
            PIRP Irp)
{
  PCMBATT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;

  UNIMPLEMENTED

  IoSkipCurrentIrpStackLocation(Irp);

  PoStartNextPowerIrp(Irp);

  return PoCallDriver(DeviceExtension->Ldo, Irp);
}

NTSTATUS
NTAPI
CmBattCreateClose(PDEVICE_OBJECT DeviceObject,
                  PIRP Irp)
{
  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CmBattAddDevice(PDRIVER_OBJECT DriverObject,
                PDEVICE_OBJECT PhysicalDeviceObject)
{
  NTSTATUS Status;
  PDEVICE_OBJECT DeviceObject;
  PCMBATT_DEVICE_EXTENSION DeviceExtension;
  BATTERY_MINIPORT_INFO BattInfo;

  Status = IoCreateDevice(DriverObject,
                          sizeof(CMBATT_DEVICE_EXTENSION),
                          NULL,
                          FILE_DEVICE_BATTERY,
                          0,
                          FALSE,
                          &DeviceObject);
  if (!NT_SUCCESS(Status))
      return Status;

  DeviceExtension = DeviceObject->DeviceExtension;

  DeviceExtension->Pdo = PhysicalDeviceObject;
  DeviceExtension->Fdo = DeviceObject;
  DeviceExtension->Ldo = IoAttachDeviceToDeviceStack(DeviceObject,
                                                     PhysicalDeviceObject);

  DeviceObject->Flags |= DO_BUFFERED_IO | DO_POWER_PAGABLE;

  /* We require an extra stack entry */
  DeviceObject->StackSize = PhysicalDeviceObject->StackSize + 2;

  BattInfo.MajorVersion = BATTERY_CLASS_MAJOR_VERSION;
  BattInfo.MinorVersion = BATTERY_CLASS_MINOR_VERSION;
  BattInfo.Context = DeviceExtension;
  BattInfo.QueryTag = CmBattQueryTag;
  BattInfo.QueryInformation = CmBattQueryInformation;
  BattInfo.SetInformation = CmBattSetInformation;
  BattInfo.QueryStatus = CmBattQueryStatus;
  BattInfo.SetStatusNotify = CmBattSetStatusNotify;
  BattInfo.DisableStatusNotify = CmBattDisableStatusNotify;
  BattInfo.Pdo = PhysicalDeviceObject;
  BattInfo.DeviceName = NULL;

  Status = BatteryClassInitializeDevice(&BattInfo,
                                        &DeviceExtension->BattClassHandle);
  if (!NT_SUCCESS(Status))
  {
     IoDetachDevice(DeviceExtension->Ldo);
     IoDeleteDevice(DeviceObject);
     return Status;
  }

  ExInterlockedInsertTailList(&BatteryList,
                              &DeviceExtension->ListEntry,
                              &BatteryListLock);

  DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

  DPRINT("Successfully registered battery with battc (0x%x)\n", DeviceExtension->BattClassHandle);

  return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject,
            PUNICODE_STRING RegistryPath)
{
  DPRINT("Control method battery miniclass driver initialized\n");

  DriverObject->DriverUnload = CmBattUnload;
  DriverObject->DriverExtension->AddDevice = CmBattAddDevice;
  DriverObject->MajorFunction[IRP_MJ_POWER] = CmBattPower;
  DriverObject->MajorFunction[IRP_MJ_PNP] = CmBattPnP;
  DriverObject->MajorFunction[IRP_MJ_CREATE] = CmBattCreateClose;
  DriverObject->MajorFunction[IRP_MJ_CLOSE] = CmBattCreateClose;
  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = CmBattDeviceControl;
  DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = CmBattSystemControl;

  KeInitializeSpinLock(&BatteryListLock);
  InitializeListHead(&BatteryList);

  return STATUS_SUCCESS;
}
