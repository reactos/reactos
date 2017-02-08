/*
 * VideoPort driver
 *
 * Copyright (C) 2002, 2003, 2004 ReactOS Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "videoprt.h"

#define NDEBUG
#include <debug.h>

/*
 * @implemented
 */

VP_STATUS NTAPI
VideoPortCreateSpinLock(
   IN PVOID HwDeviceExtension,
   OUT PSPIN_LOCK *SpinLock)
{
   TRACE_(VIDEOPRT, "VideoPortCreateSpinLock\n");
   *SpinLock = ExAllocatePool(NonPagedPool, sizeof(KSPIN_LOCK));
   if (*SpinLock == NULL)
      return ERROR_NOT_ENOUGH_MEMORY;
   KeInitializeSpinLock((PKSPIN_LOCK)*SpinLock);
   return NO_ERROR;
}

/*
 * @implemented
 */

VP_STATUS NTAPI
VideoPortDeleteSpinLock(
   IN PVOID HwDeviceExtension,
   IN PSPIN_LOCK SpinLock)
{
   TRACE_(VIDEOPRT, "VideoPortDeleteSpinLock\n");
   ExFreePool(SpinLock);
   return NO_ERROR;
}

/*
 * @implemented
 */

VOID NTAPI
VideoPortAcquireSpinLock(
   IN PVOID HwDeviceExtension,
   IN PSPIN_LOCK SpinLock,
   OUT PUCHAR OldIrql)
{
   TRACE_(VIDEOPRT, "VideoPortAcquireSpinLock\n");
   KeAcquireSpinLock((PKSPIN_LOCK)SpinLock, OldIrql);
}

/*
 * @implemented
 */

VOID NTAPI
VideoPortAcquireSpinLockAtDpcLevel(
   IN PVOID HwDeviceExtension,
   IN PSPIN_LOCK SpinLock)
{
   TRACE_(VIDEOPRT, "VideoPortAcquireSpinLockAtDpcLevel\n");
   KeAcquireSpinLockAtDpcLevel((PKSPIN_LOCK)SpinLock);
}

/*
 * @implemented
 */

VOID NTAPI
VideoPortReleaseSpinLock(
   IN PVOID HwDeviceExtension,
   IN PSPIN_LOCK SpinLock,
   IN UCHAR NewIrql)
{
   TRACE_(VIDEOPRT, "VideoPortReleaseSpinLock\n");
   KeReleaseSpinLock((PKSPIN_LOCK)SpinLock, NewIrql);
}

/*
 * @implemented
 */

VOID NTAPI
VideoPortReleaseSpinLockFromDpcLevel(
   IN PVOID HwDeviceExtension,
   IN PSPIN_LOCK SpinLock)
{
   TRACE_(VIDEOPRT, "VideoPortReleaseSpinLockFromDpcLevel\n");
   KeReleaseSpinLockFromDpcLevel((PKSPIN_LOCK)SpinLock);
}
