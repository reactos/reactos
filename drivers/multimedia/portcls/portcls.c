/*
 *  ReactOS PortCls Driver
 *  Copyright (C) 2005 ReactOS Team
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
 *  along with this program; if not, write to the Free Software Foundation,
 *  Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA 
 *
 *  COPYRIGHT:        See COPYING in the top level directory
 *  PROJECT:          ReactOS Sound System
 *  PURPOSE:          Audio Port Class Functions
 *  FILE:             drivers/multimedia/portcls/portcls.c
 *  PROGRAMMERS:
 *
 *  REVISION HISTORY:
 *       21 November 2005   Created James Tabor
 */
#include "portcls.h"


#define NDEBUG
#include <debug.h>


NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT DriverObject,
            PUNICODE_STRING RegistryPath)
{
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
DWORD STDCALL
DllInitialize(DWORD Unknown)
{
    return 0;
}

/*
 * @implemented
 */
DWORD STDCALL
DllUnload(VOID)
{
    return 0;
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
PcAddAdapterDevice(
 DWORD DriverObject,
 DWORD PhysicalDeviceObject,
 DWORD StartDevice,
 DWORD MaxObjects,
 DWORD DeviceExtensionSize
)
{
 UNIMPLEMENTED;
 return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
PcAddContentHandlers(
 DWORD  ContentId,
 DWORD  paHandlers,
 DWORD  NumHandlers
)
{
 UNIMPLEMENTED;
 return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
PcCompleteIrp(
 DWORD  DeviceObject,
 DWORD  Irp,
 DWORD  Status
)
{
 UNIMPLEMENTED;
 return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
PcCompletePendingPropertyRequest(
 DWORD PropertyRequest,
 DWORD NtStatus
)
{
 UNIMPLEMENTED;
 return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL 
PcCreateContentMixed(
 DWORD paContentId,
 DWORD cContentId,
 DWORD pMixedContentId
)
{
 UNIMPLEMENTED;
 return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
PcDestroyContent(
 DWORD ContentId
)
{
 UNIMPLEMENTED;
 return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
PcDispatchIrp(
 DWORD DeviceObject,
 DWORD Irp
)
{
 UNIMPLEMENTED;
 return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
PcForwardContentToDeviceObject(
 DWORD ContentId,
 DWORD Reserved,
 DWORD DrmForward
)
{
 UNIMPLEMENTED;
 return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL 
PcForwardContentToFileObject(
 DWORD ContentId,
 DWORD FileObject
)
{
 UNIMPLEMENTED;
 return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL 
PcForwardContentToInterface(
 DWORD ContentId,
 DWORD Unknown,
 DWORD NumMethods
)
{
 UNIMPLEMENTED;
 return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
PcForwardIrpSynchronous(
 DWORD DeviceObject,
 DWORD Irp 
)
{
 UNIMPLEMENTED;
 return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL 
PcGetContentRights(
 DWORD ContentId,
 DWORD DrmRights
)
{
 UNIMPLEMENTED;
 return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
PcGetDeviceProperty(
 DWORD DeviceObject,
 DWORD DeviceProperty,
 DWORD BufferLength,
 DWORD PropertyBuffer,
 DWORD ResultLength
)
{
 UNIMPLEMENTED;
 return STATUS_UNSUCCESSFUL;
}

/*
 * @implemented
 */
ULONGLONG STDCALL
PcGetTimeInterval(
  ULONGLONG  Timei
)
{
    LARGE_INTEGER CurrentTime;
    
    KeQuerySystemTime( &CurrentTime);

    return (Timei - CurrentTime.QuadPart);
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
PcInitializeAdapterDriver(
 DWORD DriverObject,
 DWORD RegistryPathName,
 DWORD AddDevice
)
{
 UNIMPLEMENTED;
 return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
PcNewDmaChannel(
 DWORD OutDmaChannel,
 DWORD Unknown,
 DWORD PoolType,
 DWORD DeviceDescription,
 DWORD DeviceObject
)
{
 UNIMPLEMENTED;
 return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
PcNewInterruptSync(
 DWORD OutInterruptSync,
 DWORD Unknown,
 DWORD ResourceList,
 DWORD ResourceIndex,
 DWORD Mode
)
{
 UNIMPLEMENTED;
 return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
PcNewMiniport(
 DWORD OutMiniport,
 DWORD ClassId
)
{
 UNIMPLEMENTED;
 return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
PcNewPort(
 DWORD OutPort,
 DWORD ClassId
)
{
 UNIMPLEMENTED;
 return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
PcNewRegistryKey(
 DWORD OutRegistryKey,
 DWORD Unknown,
 DWORD RegistryKeyType,
 DWORD DesiredAccess,
 DWORD DeviceObject,
 DWORD SubDevice,
 DWORD ObjectAttributes,
 DWORD CreateOptions,
 DWORD Disposition
)
{
 UNIMPLEMENTED;
 return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
PcNewResourceList(
 DWORD OutResourceList,
 DWORD Unknown,
 DWORD PoolType,
 DWORD TranslatedResources,
 DWORD UntranslatedResources
)
{
 UNIMPLEMENTED;
 return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
PcNewResourceSublist(
 DWORD OutResourceList,
 DWORD Unknown,
 DWORD PoolType,
 DWORD ParentList,
 DWORD MaximumEntries
)
{
 UNIMPLEMENTED;
 return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
PcNewServiceGroup(
 DWORD OutServiceGroup,
 DWORD Unknown
)
{
 UNIMPLEMENTED;
 return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
PcRegisterAdapterPowerManagement(
 DWORD Unknown,
 DWORD pvContext
)
{
 UNIMPLEMENTED;
 return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
PcRegisterIoTimeout(
 DWORD pDeviceObject,
 DWORD pTimerRoutine,
 DWORD pContext
)
{
 UNIMPLEMENTED;
 return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
PcRegisterPhysicalConnection(
 DWORD DeviceObject,
 DWORD FromUnknown,
 DWORD FromPin,
 DWORD ToUnknown,
 DWORD ToPin
)
{
 UNIMPLEMENTED;
 return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
PcRegisterPhysicalConnectionFromExternal(
 DWORD DeviceObject,
 DWORD FromString,
 DWORD FromPin,
 DWORD ToUnknown,
 DWORD ToPin
)
{
 UNIMPLEMENTED;
 return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
PcRegisterPhysicalConnectionToExternal(
 DWORD DeviceObject,
 DWORD FromUnknown,
 DWORD FromPin,
 DWORD ToString,
 DWORD ToPin
)
{
 UNIMPLEMENTED;
 return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
PcRegisterSubdevice(
 DWORD DeviceObject,
 DWORD SubdevName,
 DWORD Unknown
)
{
 UNIMPLEMENTED;
 return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
PcRequestNewPowerState(
 DWORD pDeviceObject,
 DWORD RequestedNewState
)
{
 UNIMPLEMENTED;
 return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
PcUnregisterIoTimeout(
 DWORD pDeviceObject,
 DWORD pTimerRoutine,
 DWORD pContext
)
{
 UNIMPLEMENTED;
 return STATUS_UNSUCCESSFUL;
}

