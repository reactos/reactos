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
 * $Id: videoprt.h,v 1.6 2004/03/09 18:56:32 navaraf Exp $
 */

#ifndef VIDEOPRT_H
#define VIDEOPRT_H

#include <ddk/miniport.h>
#include <ddk/video.h>
#include <ddk/ntddvdeo.h>
#include "internal/ps.h"
#define NDEBUG
#include <debug.h>

/*
 * FIXME: Definition missing from w32api!
 */
#ifndef SYNCH_LEVEL
#define SYNCH_LEVEL	(IPI_LEVEL - 2)
#endif
VOID FASTCALL KfLowerIrql(IN KIRQL NewIrql);
#define KeRaiseIrql(a,b) *(b) = KfRaiseIrql(a)
KIRQL FASTCALL KfRaiseIrql(IN KIRQL NewIrql);
NTKERNELAPI VOID HalAcquireDisplayOwnership(IN PHAL_RESET_DISPLAY_PARAMETERS ResetDisplayParameters);

#define TAG_VIDEO_PORT  TAG('V', 'I', 'D', 'P')

extern PEPROCESS Csrss;

typedef struct _VIDEO_PORT_ADDRESS_MAPPING
{
   LIST_ENTRY List;
   PVOID MappedAddress;
   ULONG NumberOfUchars;
   PHYSICAL_ADDRESS IoAddress;
   ULONG SystemIoBusNumber;
   UINT MappingCount;
} VIDEO_PORT_ADDRESS_MAPPING, *PVIDEO_PORT_ADDRESS_MAPPING;

typedef struct _VIDEO_PORT_DEVICE_EXTENSTION
{
   PDEVICE_OBJECT DeviceObject;
   PKINTERRUPT InterruptObject;
   KSPIN_LOCK InterruptSpinLock;
   ULONG InterruptVector;
   ULONG InterruptLevel;
   PVIDEO_HW_INITIALIZE HwInitialize;
   PVIDEO_HW_RESET_HW HwResetHw;
   PVIDEO_HW_TIMER HwTimer;
   PVIDEO_HW_INTERRUPT HwInterrupt;
   LIST_ENTRY AddressMappingListHead;
   INTERFACE_TYPE AdapterInterfaceType;
   ULONG SystemIoBusNumber;
   UNICODE_STRING RegistryPath;
   KDPC DpcObject;
   UCHAR MiniPortDeviceExtension[1]; /* must be the last entry */
} VIDEO_PORT_DEVICE_EXTENSION, *PVIDEO_PORT_DEVICE_EXTENSION;

NTSTATUS STDCALL
VidDispatchOpen(IN PDEVICE_OBJECT pDO, IN PIRP Irp);

NTSTATUS STDCALL
VidDispatchClose(IN PDEVICE_OBJECT pDO, IN PIRP Irp);

NTSTATUS STDCALL
VidDispatchDeviceControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

PVOID STDCALL
InternalMapMemory(
   IN PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension,
   IN PHYSICAL_ADDRESS IoAddress,
   IN ULONG NumberOfUchars,
   IN UCHAR InIoSpace,
   OUT NTSTATUS *Status);

VOID STDCALL
InternalUnmapMemory(
   IN PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension,
   IN PVOID MappedAddress);

VP_STATUS STDCALL
IntInt10AllocateBuffer(
   IN PVOID Context,
   OUT PUSHORT Seg,
   OUT PUSHORT Off,
   IN OUT PULONG Length);

VP_STATUS STDCALL
IntInt10FreeBuffer(
   IN PVOID Context,
   IN USHORT Seg,
   IN USHORT Off);

VP_STATUS STDCALL
IntInt10ReadMemory(
   IN PVOID Context,
   IN USHORT Seg,
   IN USHORT Off,
   OUT PVOID Buffer,
   IN ULONG Length);

VP_STATUS STDCALL
IntInt10WriteMemory(
   IN PVOID Context,
   IN USHORT Seg,
   IN USHORT Off,
   IN PVOID Buffer,
   IN ULONG Length);

VP_STATUS STDCALL
IntInt10CallBios(
   IN PVOID Context,
   IN OUT PINT10_BIOS_ARGUMENTS BiosArguments);

#endif /* VIDEOPRT_H */
