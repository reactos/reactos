/*
 * VideoPort driver
 *
 * Copyright (C) 2002, 2003, 2004 ReactOS Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; see the file COPYING.LIB.
 * If not, write to the Free Software Foundation,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id: spinlock.c,v 1.1 2004/03/09 17:28:49 navaraf Exp $
 */

#include "videoprt.h"

/*
 * @implemented
 */

VP_STATUS STDCALL
VideoPortCreateSpinLock(
   IN PVOID HwDeviceExtension,
   OUT PSPIN_LOCK *SpinLock)
{
   DPRINT("VideoPortCreateSpinLock\n");   
   *SpinLock = ExAllocatePool(NonPagedPool, sizeof(KSPIN_LOCK));
   if (*SpinLock == NULL)
      return STATUS_UNSUCCESSFUL;
   KeInitializeSpinLock((PKSPIN_LOCK)*SpinLock);
   return STATUS_SUCCESS;
}

/*
 * @implemented
 */

VP_STATUS STDCALL
VideoPortDeleteSpinLock(
   IN PVOID HwDeviceExtension,
   IN PSPIN_LOCK SpinLock)
{
   DPRINT("VideoPortDeleteSpinLock\n");
   ExFreePool(SpinLock);
   return STATUS_SUCCESS;
}

/*
 * @implemented
 */

VOID STDCALL
VideoPortAcquireSpinLock(
   IN PVOID HwDeviceExtension,
   IN PSPIN_LOCK SpinLock,
   OUT PUCHAR OldIrql)
{
   DPRINT("VideoPortAcquireSpinLock\n");
   KeAcquireSpinLock((PKSPIN_LOCK)SpinLock, OldIrql);
}

/*
 * @implemented
 */

VOID STDCALL
VideoPortAcquireSpinLockAtDpcLevel(
   IN PVOID HwDeviceExtension,
   IN PSPIN_LOCK SpinLock)
{
   DPRINT("VideoPortAcquireSpinLockAtDpcLevel\n");
   KefAcquireSpinLockAtDpcLevel((PKSPIN_LOCK)SpinLock);
}

/*
 * @implemented
 */

VOID STDCALL
VideoPortReleaseSpinLock(
   IN PVOID HwDeviceExtension,
   IN PSPIN_LOCK SpinLock,
   IN UCHAR NewIrql)
{
   DPRINT("VideoPortReleaseSpinLock\n");
   KeReleaseSpinLock((PKSPIN_LOCK)SpinLock, NewIrql);
}

/*
 * @implemented
 */

VOID STDCALL
VideoPortReleaseSpinLockFromDpcLevel(
   IN PVOID HwDeviceExtension,
   IN PSPIN_LOCK SpinLock)
{
   DPRINT("VideoPortReleaseSpinLockFromDpcLevel\n");
   KefReleaseSpinLockFromDpcLevel((PKSPIN_LOCK)SpinLock);
}
