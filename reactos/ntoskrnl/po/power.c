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
/* $Id: power.c,v 1.2 2001/04/16 00:48:04 chorns Exp $
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/po/power.c
 * PURPOSE:         Power Manager
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *   20/08/1999 EA  Created
 *   16/04/2001 CSH Stubs added
 */
#include <ddk/ntddk.h>

NTSTATUS
STDCALL
PoCallDriver(
  IN PDEVICE_OBJECT DeviceObject,
  IN OUT PIRP Irp)
{
  return STATUS_NOT_IMPLEMENTED;
}

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

PVOID
STDCALL
PoRegisterSystemState(
  IN PVOID StateHandle,
  IN EXECUTION_STATE Flags)
{
  return NULL;
}

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

VOID
STDCALL
PoSetSystemState(
  IN EXECUTION_STATE Flags)
{
}

VOID
STDCALL
PoStartNextPowerIrp(
  IN PIRP Irp)
{
}

VOID
STDCALL
PoUnregisterSystemState(
  IN PVOID StateHandle)
{
}

/* EOF */
