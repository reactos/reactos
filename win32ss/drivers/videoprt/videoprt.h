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

#ifndef VIDEOPRT_H
#define VIDEOPRT_H

#include <ntifs.h>

#define __BROKEN__
#include <miniport.h>
#include <video.h>
#include <ntagp.h>
#include <dderror.h>
#include <windef.h>
#include <wdmguid.h>

/* PSEH for SEH Support */
#include <pseh/pseh2.h>

#define TAG_VIDEO_PORT          'PDIV'
#define TAG_VIDEO_PORT_BUFFER   '\0mpV'
#define TAG_REQUEST_PACKET      'qRpV'

#define GUID_STRING_LENGTH (38 * sizeof(WCHAR))

typedef struct _VIDEO_PORT_ADDRESS_MAPPING
{
   LIST_ENTRY List;
   PVOID MappedAddress;
   ULONG NumberOfUchars;
   PHYSICAL_ADDRESS IoAddress;
   ULONG SystemIoBusNumber;
   UINT MappingCount;
} VIDEO_PORT_ADDRESS_MAPPING, *PVIDEO_PORT_ADDRESS_MAPPING;

struct _VIDEO_PORT_AGP_VIRTUAL_MAPPING;

typedef struct _VIDEO_PORT_AGP_MAPPING
{
   ULONG NumberOfPages;
   PVOID MapHandle;
   PHYSICAL_ADDRESS PhysicalAddress;
} VIDEO_PORT_AGP_MAPPING, *PVIDEO_PORT_AGP_MAPPING;

typedef struct _VIDEO_PORT_AGP_VIRTUAL_MAPPING
{
   PVIDEO_PORT_AGP_MAPPING AgpMapping;
   HANDLE ProcessHandle;
   PVOID MappedAddress;
} VIDEO_PORT_AGP_VIRTUAL_MAPPING, *PVIDEO_PORT_AGP_VIRTUAL_MAPPING;

typedef struct _VIDEO_PORT_DRIVER_EXTENSION
{
   VIDEO_HW_INITIALIZATION_DATA InitializationData;
   PVOID HwContext;
   UNICODE_STRING RegistryPath;
} VIDEO_PORT_DRIVER_EXTENSION, *PVIDEO_PORT_DRIVER_EXTENSION;

typedef struct _VIDEO_PORT_COMMON_EXTENSION
{
    BOOLEAN Fdo;
} VIDEO_PORT_COMMON_EXTENSION, *PVIDEO_PORT_COMMON_EXTENSION;

typedef struct _VIDEO_PORT_DEVICE_EXTENSTION
{
   VIDEO_PORT_COMMON_EXTENSION Common;
   ULONG DeviceNumber;
   PDRIVER_OBJECT DriverObject;
   PDEVICE_OBJECT PhysicalDeviceObject;
   PDEVICE_OBJECT FunctionalDeviceObject;
   PDEVICE_OBJECT NextDeviceObject;
   UNICODE_STRING RegistryPath;
   UNICODE_STRING NewRegistryPath;
   PKINTERRUPT InterruptObject;
   KSPIN_LOCK InterruptSpinLock;
   PCM_RESOURCE_LIST AllocatedResources;
   ULONG InterruptVector;
   ULONG InterruptLevel;
   BOOLEAN InterruptShared;
   INTERFACE_TYPE AdapterInterfaceType;
   ULONG SystemIoBusNumber;
   ULONG SystemIoSlotNumber;
   LIST_ENTRY AddressMappingListHead;
   KDPC DpcObject;
   VIDEO_PORT_DRIVER_EXTENSION *DriverExtension;
   ULONG DeviceOpened;
   AGP_BUS_INTERFACE_STANDARD AgpInterface;
   KMUTEX DeviceLock;
   LIST_ENTRY DmaAdapterList, ChildDeviceList;
   LIST_ENTRY HwResetListEntry;
   ULONG SessionId;
   USHORT AdapterNumber;
   USHORT DisplayNumber;
   ULONG NumberOfSecondaryDisplays;
   CHAR POINTER_ALIGNMENT MiniPortDeviceExtension[1];
} VIDEO_PORT_DEVICE_EXTENSION, *PVIDEO_PORT_DEVICE_EXTENSION;

typedef struct _VIDEO_PORT_CHILD_EXTENSION
{
    VIDEO_PORT_COMMON_EXTENSION Common;

    ULONG ChildId;
    VIDEO_CHILD_TYPE ChildType;
    UCHAR ChildDescriptor[256];

    BOOLEAN EdidValid;

    PDRIVER_OBJECT DriverObject;
    PDEVICE_OBJECT PhysicalDeviceObject;

    LIST_ENTRY ListEntry;

    CHAR ChildDeviceExtension[1];
} VIDEO_PORT_CHILD_EXTENSION, *PVIDEO_PORT_CHILD_EXTENSION;

#define VIDEO_PORT_GET_CHILD_EXTENSION(MiniportExtension) \
   CONTAINING_RECORD( \
       MiniportExtension, \
       VIDEO_PORT_CHILD_EXTENSION, \
       ChildDeviceExtension)

#define VIDEO_PORT_GET_DEVICE_EXTENSION(MiniportExtension) \
   CONTAINING_RECORD( \
      MiniportExtension, \
      VIDEO_PORT_DEVICE_EXTENSION, \
      MiniPortDeviceExtension)

typedef struct _VIDEO_PORT_EVENT
{
    ENG_EVENT;
} VIDEO_PORT_EVENT, *PVIDEO_PORT_EVENT;

/* agp.c */

NTSTATUS
IopInitiatePnpIrp(
  PDEVICE_OBJECT DeviceObject,
  PIO_STATUS_BLOCK IoStatusBlock,
  UCHAR MinorFunction,
  PIO_STACK_LOCATION Stack OPTIONAL);

NTSTATUS NTAPI
IntAgpGetInterface(
   IN PVOID HwDeviceExtension,
   IN OUT PINTERFACE Interface);

/* child.c */

NTSTATUS NTAPI
IntVideoPortDispatchPdoPnp(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp);

/* dispatch.c */

NTSTATUS NTAPI
IntVideoPortAddDevice(
   IN PDRIVER_OBJECT DriverObject,
   IN PDEVICE_OBJECT PhysicalDeviceObject);

NTSTATUS NTAPI
IntVideoPortDispatchOpen(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp);

NTSTATUS NTAPI
IntVideoPortDispatchClose(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp);

NTSTATUS NTAPI
IntVideoPortDispatchCleanup(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp);

NTSTATUS NTAPI
IntVideoPortDispatchDeviceControl(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp);

NTSTATUS NTAPI
IntVideoPortDispatchPnp(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp);

NTSTATUS NTAPI
IntVideoPortDispatchPower(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp);

NTSTATUS NTAPI
IntVideoPortDispatchSystemControl(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp);

VOID NTAPI
IntVideoPortUnload(PDRIVER_OBJECT DriverObject);

/* timer.c */

BOOLEAN NTAPI
IntVideoPortSetupTimer(
   IN PDEVICE_OBJECT DeviceObject,
   IN PVIDEO_PORT_DRIVER_EXTENSION DriverExtension);

/* interrupt.c */

BOOLEAN NTAPI
IntVideoPortSetupInterrupt(
   IN PDEVICE_OBJECT DeviceObject,
   IN PVIDEO_PORT_DRIVER_EXTENSION DriverExtension,
   IN PVIDEO_PORT_CONFIG_INFO ConfigInfo);

/* resource.c */

NTSTATUS NTAPI
IntVideoPortFilterResourceRequirements(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIO_STACK_LOCATION IrpStack,
   IN PIRP Irp);

NTSTATUS NTAPI
IntVideoPortMapPhysicalMemory(
   IN HANDLE Process,
   IN PHYSICAL_ADDRESS PhysicalAddress,
   IN ULONG SizeInBytes,
   IN ULONG Protect,
   IN OUT PVOID *VirtualAddress  OPTIONAL);

/* videoprt.c */

extern PKPROCESS CsrProcess;
extern ULONG VideoPortDeviceNumber;
extern KMUTEX VideoPortInt10Mutex;
extern KSPIN_LOCK HwResetAdaptersLock;
extern LIST_ENTRY HwResetAdaptersList;

VOID FASTCALL
IntAttachToCSRSS(PKPROCESS *CallingProcess, PKAPC_STATE ApcState);

VOID FASTCALL
IntDetachFromCSRSS(PKPROCESS *CallingProcess, PKAPC_STATE ApcState);

NTSTATUS NTAPI
IntVideoPortCreateAdapterDeviceObject(
   _In_ PDRIVER_OBJECT DriverObject,
   _In_ PVIDEO_PORT_DRIVER_EXTENSION DriverExtension,
   _In_opt_ PDEVICE_OBJECT PhysicalDeviceObject,
   _In_ USHORT AdapterNumber,
   _In_ USHORT DisplayNumber,
   _Out_opt_ PDEVICE_OBJECT *DeviceObject);

NTSTATUS NTAPI
IntVideoPortFindAdapter(
   IN PDRIVER_OBJECT DriverObject,
   IN PVIDEO_PORT_DRIVER_EXTENSION DriverExtension,
   IN PDEVICE_OBJECT DeviceObject);

PVOID NTAPI
IntVideoPortGetProcAddress(
   IN PVOID HwDeviceExtension,
   IN PUCHAR FunctionName);

NTSTATUS NTAPI
IntVideoPortEnumerateChildren(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

/* int10.c */

NTSTATUS
NTAPI
IntInitializeVideoAddressSpace(VOID);

VP_STATUS NTAPI
IntInt10AllocateBuffer(
   IN PVOID Context,
   OUT PUSHORT Seg,
   OUT PUSHORT Off,
   IN OUT PULONG Length);

VP_STATUS NTAPI
IntInt10FreeBuffer(
   IN PVOID Context,
   IN USHORT Seg,
   IN USHORT Off);

VP_STATUS NTAPI
IntInt10ReadMemory(
   IN PVOID Context,
   IN USHORT Seg,
   IN USHORT Off,
   OUT PVOID Buffer,
   IN ULONG Length);

VP_STATUS NTAPI
IntInt10WriteMemory(
   IN PVOID Context,
   IN USHORT Seg,
   IN USHORT Off,
   IN PVOID Buffer,
   IN ULONG Length);

VP_STATUS NTAPI
IntInt10CallBios(
   IN PVOID Context,
   IN OUT PINT10_BIOS_ARGUMENTS BiosArguments);

/* registry.c */

NTSTATUS
NTAPI
IntCopyRegistryKey(
    _In_ HANDLE SourceKeyHandle,
    _In_ HANDLE DestKeyHandle);

NTSTATUS
NTAPI
IntCopyRegistryValue(
    HANDLE SourceKeyHandle,
    HANDLE DestKeyHandle,
    PWSTR ValueName);

NTSTATUS
NTAPI
IntSetupDeviceSettingsKey(
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension);

NTSTATUS
NTAPI
IntCreateNewRegistryPath(
    PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension);

NTSTATUS
NTAPI
IntCreateRegistryPath(
    IN PCUNICODE_STRING DriverRegistryPath,
    IN ULONG DeviceNumber,
    OUT PUNICODE_STRING DeviceRegistryPath);


#endif /* VIDEOPRT_H */
