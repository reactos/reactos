/*
    ReactOS Operating System
    Port Class API / IPortMidi Implementation

    by Andrew Greenwood

    REFERENCE:
        http://www.osronline.com/ddkx/stream/audmp-routines_49pv.htm

    NOTES:
        IPortMidi inherits from IPort. This file contains specific
        extensions.
*/

#include <stdunk.h>
#include <portcls.h>
#include "port.h"


/*
    IPort Methods
*/

NTSTATUS
CPortMidi::GetDeviceProperty(
    IN  DEVICE_REGISTRY_PROPERTY DeviceRegistryProperty,
    IN  ULONG BufferLength,
    OUT PVOID PropertyBuffer,
    OUT PULONG ReturnLength)
{
    /* http://www.osronline.com/ddkx/stream/audmp-routines_93xv.htm */
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
CPortMidi::Init(
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
CPortMidi::NewRegistryKey(
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

/*
    IPortMidi Methods
*/

VOID
CPortMidi::Notify(IN PSERVICEGROUP ServiceGroup OPTIONAL)
{
}

NTSTATUS
CPortMidi::RegisterServiceGroup(IN PSERVICEGROUP ServiceGroup)
{
    return STATUS_UNSUCCESSFUL;
}
