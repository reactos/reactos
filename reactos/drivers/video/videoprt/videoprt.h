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
 * $Id: videoprt.h,v 1.10 2004/04/08 09:43:55 navaraf Exp $
 */

#ifndef VIDEOPRT_H
#define VIDEOPRT_H

#include <ddk/miniport.h>
#include <ddk/video.h>
#include <ddk/ntddvdeo.h>
#include <ddk/ntapi.h>
#define NDEBUG
#include <debug.h>

int swprintf(wchar_t *buf, const wchar_t *fmt, ...);
int vsprintf(char *buf, const char *fmt, va_list args);
BOOLEAN STDCALL HalDisableSystemInterrupt(ULONG Vector, ULONG Unknown2);
BOOLEAN STDCALL HalEnableSystemInterrupt(ULONG Vector, ULONG Unknown2, ULONG Unknown3);
PIMAGE_NT_HEADERS STDCALL RtlImageNtHeader(IN PVOID BaseAddress);

#define TAG_VIDEO_PORT  TAG('V', 'I', 'D', 'P')

typedef struct _VIDEO_PORT_ADDRESS_MAPPING
{
   LIST_ENTRY List;
   PVOID MappedAddress;
   ULONG NumberOfUchars;
   PHYSICAL_ADDRESS IoAddress;
   ULONG SystemIoBusNumber;
   UINT MappingCount;
} VIDEO_PORT_ADDRESS_MAPPING, *PVIDEO_PORT_ADDRESS_MAPPING;

typedef struct _VIDEO_PORT_DRIVER_EXTENSION
{
   VIDEO_HW_INITIALIZATION_DATA InitializationData;
   PVOID HwContext;
   UNICODE_STRING RegistryPath;
} VIDEO_PORT_DRIVER_EXTENSION, *PVIDEO_PORT_DRIVER_EXTENSION;

typedef struct _VIDEO_PORT_DEVICE_EXTENSTION
{
   PDEVICE_OBJECT PhysicalDeviceObject;
   PDEVICE_OBJECT FunctionalDeviceObject;
   UNICODE_STRING RegistryPath;
   PKINTERRUPT InterruptObject;
   KSPIN_LOCK InterruptSpinLock;
   ULONG InterruptVector;
   ULONG InterruptLevel;
   ULONG AdapterInterfaceType;
   ULONG SystemIoBusNumber;
   ULONG SystemIoSlotNumber;
   LIST_ENTRY AddressMappingListHead;
   KDPC DpcObject;
   VIDEO_PORT_DRIVER_EXTENSION *DriverExtension;
   ULONG DeviceOpened;
   CHAR MiniPortDeviceExtension[1];
} VIDEO_PORT_DEVICE_EXTENSION, *PVIDEO_PORT_DEVICE_EXTENSION;

#define VIDEO_PORT_GET_DEVICE_EXTENSION(MiniportExtension) \
   CONTAINING_RECORD( \
      HwDeviceExtension, \
      VIDEO_PORT_DEVICE_EXTENSION, \
      MiniPortDeviceExtension)

/* dispatch.c */

NTSTATUS STDCALL
IntVideoPortAddDevice(
   IN PDRIVER_OBJECT DriverObject,
   IN PDEVICE_OBJECT PhysicalDeviceObject);

NTSTATUS STDCALL
IntVideoPortDispatchOpen(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp);

NTSTATUS STDCALL
IntVideoPortDispatchClose(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp);

NTSTATUS STDCALL
IntVideoPortDispatchCleanup(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp);

NTSTATUS STDCALL
IntVideoPortDispatchDeviceControl(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp);

NTSTATUS STDCALL
IntVideoPortDispatchPnp(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp);

NTSTATUS STDCALL
IntVideoPortDispatchPower(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp);

VOID STDCALL
IntVideoPortUnload(PDRIVER_OBJECT DriverObject);

/* timer.c */

BOOLEAN STDCALL
IntVideoPortSetupTimer(
   IN PDEVICE_OBJECT DeviceObject,
   IN PVIDEO_PORT_DRIVER_EXTENSION DriverExtension);

/* interrupt.c */

BOOLEAN STDCALL
IntVideoPortSetupInterrupt(
   IN PDEVICE_OBJECT DeviceObject,
   IN PVIDEO_PORT_DRIVER_EXTENSION DriverExtension,
   IN PVIDEO_PORT_CONFIG_INFO ConfigInfo);

/* videoprt.c */

extern ULONG CsrssInitialized;
extern PEPROCESS Csrss;

PVOID STDCALL
VideoPortGetProcAddress(
   IN PVOID HwDeviceExtension,
   IN PUCHAR FunctionName);

VOID FASTCALL 
IntAttachToCSRSS(PEPROCESS *CallingProcess, PEPROCESS *PrevAttachedProcess);

VOID FASTCALL 
IntDetachFromCSRSS(PEPROCESS *CallingProcess, PEPROCESS *PrevAttachedProcess);

NTSTATUS STDCALL
IntVideoPortFindAdapter(
   IN PDRIVER_OBJECT DriverObject,
   IN PVIDEO_PORT_DRIVER_EXTENSION DriverExtension,
   IN PDEVICE_OBJECT PhysicalDeviceObject);

/* int10.c */

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

VP_STATUS STDCALL
VideoPortInt10(
   IN PVOID HwDeviceExtension,
   IN PVIDEO_X86_BIOS_ARGUMENTS BiosArguments);

#endif /* VIDEOPRT_H */
