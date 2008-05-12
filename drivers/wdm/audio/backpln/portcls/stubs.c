/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS
 * FILE:            drivers/multimedia/portcls/stubs.c
 * PURPOSE:         Port Class driver / Stubs
 * PROGRAMMER:      Andrew Greenwood
 *
 * HISTORY:
 *                  27 Jan 07   Created
 */

#include "private.h"
#include <portcls.h>

/*
    Factory Stubs
*/

/*
 * @unimplemented
 */
NTSTATUS NTAPI
PcNewDmaChannel(
    OUT PDMACHANNEL* OutDmaChannel,
    IN  PUNKNOWN OuterUnknown OPTIONAL,
    IN  POOL_TYPE PoolType,
    IN  PDEVICE_DESCRIPTION DeviceDescription,
    IN  PDEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS NTAPI
PcNewInterruptSync(
    OUT PINTERRUPTSYNC* OUtInterruptSync,
    IN  PUNKNOWN OuterUnknown OPTIONAL,
    IN  PRESOURCELIST ResourceList,
    IN  ULONG ResourceIndex,
    IN  INTERRUPTSYNCMODE Mode)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS NTAPI
PcNewMiniport(
    OUT PMINIPORT* OutMiniport,
    IN  REFCLSID ClassId)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS NTAPI
PcNewRegistryKey(
    OUT PREGISTRYKEY* OutRegistryKey,
    IN  PUNKNOWN OuterUnknown OPTIONAL,
    IN  ULONG RegistryKeyType,
    IN  ACCESS_MASK DesiredAccess,
    IN  PVOID DeviceObject OPTIONAL,
    IN  PVOID SubDevice OPTIONAL,
    IN  POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN  ULONG CreateOptions OPTIONAL,
    OUT PULONG Disposition OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS NTAPI
PcNewServiceGroup(
    OUT PSERVICEGROUP* OutServiceGroup,
    IN  PUNKNOWN OuterUnknown OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}


/* ===============================================================
    Power Management
*/

/*
 * @unimplemented
 */
NTSTATUS NTAPI
PcRegisterAdapterPowerManagement(
    IN  PUNKNOWN pUnknown,
    IN  PVOID pvContext1)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS NTAPI
PcRequestNewPowerState(
    IN  PDEVICE_OBJECT pDeviceObject,
    IN  DEVICE_POWER_STATE RequestedNewState)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}


/* ===============================================================
    Properties
*/

/*
 * @unimplemented
 */
NTSTATUS NTAPI
PcGetDeviceProperty(
    IN  PVOID DeviceObject,
    IN  DEVICE_REGISTRY_PROPERTY DeviceProperty,
    IN  ULONG BufferLength,
    OUT PVOID PropertyBuffer,
    OUT PULONG ResultLength)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS NTAPI
PcCompletePendingPropertyRequest(
    IN  PPCPROPERTY_REQUEST PropertyRequest,
    IN  NTSTATUS NtStatus)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}


/* ===============================================================
    I/O Timeouts
*/

/*
 * @unimplemented
 */
NTSTATUS NTAPI
PcRegisterIoTimeout(
    IN  PDEVICE_OBJECT pDeviceObject,
    IN  PIO_TIMER_ROUTINE pTimerRoutine,
    IN  PVOID pContext)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS NTAPI
PcUnregisterIoTimeout(
    IN  PDEVICE_OBJECT pDeviceObject,
    IN  PIO_TIMER_ROUTINE pTimerRoutine,
    IN  PVOID pContext)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}


/* ===============================================================
    Physical Connections
*/

/*
 * @unimplemented
 */
NTSTATUS NTAPI
PcRegisterPhysicalConnection(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PUNKNOWN FromUnknown,
    IN  ULONG FromPin,
    IN  PUNKNOWN ToUnknown,
    IN  ULONG ToPin)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS NTAPI
PcRegisterPhysicalConnectionFromExternal(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PUNICODE_STRING FromString,
    IN  ULONG FromPin,
    IN  PUNKNOWN ToUnknown,
    IN  ULONG ToPin)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS NTAPI
PcRegisterPhysicalConnectionToExternal(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PUNKNOWN FromUnknown,
    IN  ULONG FromPin,
    IN  PUNICODE_STRING ToString,
    IN  ULONG ToPin)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}


/* ===============================================================
    Misc
*/

/*
 * @unimplemented
 */
ULONGLONG NTAPI
PcGetTimeInterval(
    IN  ULONGLONG Since)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS NTAPI
PcRegisterSubdevice(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PWCHAR Name,
    IN  PUNKNOWN Unknown)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}
