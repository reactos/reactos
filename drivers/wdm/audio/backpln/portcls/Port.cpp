/*
    ReactOS Operating System
    Port Class API / IPort Implementation

    by Andrew Greenwood

    REFERENCE:
        http://www.osronline.com/ddkx/stream/audmp-routines_0tgz.htm

    NOTE: I'm not sure if this file is even needed...
*/

#if 0
#include "../private.h"
#include <stdunk.h>
#include <portcls.h>

NTSTATUS
IPort::GetDeviceProperty(
    IN  DEVICE_REGISTRY_PROPERTY DeviceRegistryProperty,
    IN  ULONG BufferLength,
    OUT PVOID PropertyBuffer,
    OUT PULONG ReturnLength)
{
    /* http://www.osronline.com/ddkx/stream/audmp-routines_93xv.htm */
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
IPort::Init(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PUNKNOWN UnknownMiniport,
    IN  PUNKNOWN UnknownAdapter OPTIONAL,
    IN  PRESOURCELIST ResourceList)
{
    /* http://www.osronline.com/ddkx/stream/audmp-routines_7qcz.htm */
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
IPort::NewRegistryKey(
    OUT PREGISTRYKEY* OutRegistryKey,
    IN  PUNKNOWN OuterUnknown OPTIONAL,
    IN  ULONG RegistryKeyType,
    IN  ACCESS_MASK DesiredAccess,
    IN  POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN  ULONG CreateOptions OPTIONAL,
    OUT PULONG Disposition OPTIONAL)
{
    /* http://www.osronline.com/ddkx/stream/audmp-routines_2jhv.htm */
    return STATUS_UNSUCCESSFUL;
}

#endif
