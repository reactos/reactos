/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: power.c,v 1.7 2003/07/11 01:23:15 royce Exp $
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/po/power.c
 * PURPOSE:         Power Manager
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *   20/08/1999 EA  Created
 *   16/04/2001 CSH Stubs added
 */
#include <ddk/ntddk.h>
#include <roscfg.h>
#include <internal/io.h>
#include <internal/po.h>

#define NDEBUG
#include <internal/debug.h>


PDEVICE_NODE PopSystemPowerDeviceNode = NULL;


NTSTATUS
STDCALL
PoCallDriver(
  IN PDEVICE_OBJECT DeviceObject,
  IN OUT PIRP Irp)
{
  NTSTATUS Status;

  Status = IoCallDriver(DeviceObject, Irp);

  return Status;
}

/*
 * @unimplemented
 */
PULONG
STDCALL
PoRegisterDeviceForIdleDetection(
  IN PDEVICE_OBJECT DeviceObject,
  IN ULONG ConservationIdleTime,
  IN ULONG PerformanceIdleTime,
  IN DEVICE_POWER_STATE State)
{
  return NULL;
}

/*
 * @unimplemented
 */
PVOID
STDCALL
PoRegisterSystemState(
  IN PVOID StateHandle,
  IN EXECUTION_STATE Flags)
{
  return NULL;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
PoRequestPowerIrp(
  IN PDEVICE_OBJECT DeviceObject,
  IN UCHAR MinorFunction,  
  IN POWER_STATE PowerState,
  IN PREQUEST_POWER_COMPLETE CompletionFunction,
  IN PVOID Context,
  OUT PIRP *Irp   OPTIONAL)
{
  return STATUS_NOT_IMPLEMENTED;
}

VOID
STDCALL
PoSetDeviceBusy(
  PULONG IdlePointer)
{
}

/*
 * @unimplemented
 */
POWER_STATE
STDCALL
PoSetPowerState(
  IN PDEVICE_OBJECT DeviceObject,
  IN POWER_STATE_TYPE Type,
  IN POWER_STATE State)
{
  POWER_STATE ps;

  ps.SystemState = PowerSystemWorking;  // Fully on
  ps.DeviceState = PowerDeviceD0;       // Fully on

  return ps;
}

/*
 * @unimplemented
 */
VOID
STDCALL
PoSetSystemState(
  IN EXECUTION_STATE Flags)
{
}

/*
 * @unimplemented
 */
VOID
STDCALL
PoStartNextPowerIrp(
  IN PIRP Irp)
{
}

/*
 * @unimplemented
 */
VOID
STDCALL
PoUnregisterSystemState(
  IN PVOID StateHandle)
{
}

NTSTATUS
PopSetSystemPowerState(
  SYSTEM_POWER_STATE PowerState)
{

#ifdef ACPI

  IO_STATUS_BLOCK IoStatusBlock;
  PDEVICE_OBJECT DeviceObject;
  PIO_STACK_LOCATION IrpSp;
  PDEVICE_OBJECT Fdo;
  NTSTATUS Status;
  KEVENT Event;
  PIRP Irp;

  Status = IopGetSystemPowerDeviceObject(&DeviceObject);
  if (!NT_SUCCESS(Status)) {
    CPRINT("No system power driver available\n");
    return STATUS_UNSUCCESSFUL;
  }

  Fdo = IoGetAttachedDeviceReference(DeviceObject);

  if (Fdo == DeviceObject)
    {
      DPRINT("An FDO was not attached\n");
      return STATUS_UNSUCCESSFUL;
    }

  KeInitializeEvent(&Event,
	  NotificationEvent,
	  FALSE);

  Irp = IoBuildSynchronousFsdRequest(IRP_MJ_POWER,
    Fdo,
	  NULL,
	  0,
	  NULL,
	  &Event,
	  &IoStatusBlock);

  IrpSp = IoGetNextIrpStackLocation(Irp);
  IrpSp->MinorFunction = IRP_MN_SET_POWER;
  IrpSp->Parameters.Power.Type = SystemPowerState;
  IrpSp->Parameters.Power.State.SystemState = PowerState;

	Status = PoCallDriver(Fdo, Irp);
	if (Status == STATUS_PENDING)
	  {
		  KeWaitForSingleObject(&Event,
		                        Executive,
		                        KernelMode,
		                        FALSE,
		                        NULL);
      Status = IoStatusBlock.Status;
    }

  ObDereferenceObject(Fdo);

  return Status;

#endif /* ACPI */

  return STATUS_NOT_IMPLEMENTED;
}

VOID
PoInit(VOID)
{
}

/* EOF */
