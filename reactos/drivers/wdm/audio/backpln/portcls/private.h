/*
    PortCls FDO Extension

    by Andrew Greenwood
*/

#ifndef PORTCLS_PRIVATE_H
#define PORTCLS_PRIVATE_H

#include <ntddk.h>
#include <debug.h>

#include <portcls.h>

NTAPI
NTSTATUS
PortClsCreate(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

NTAPI
NTSTATUS
PortClsPnp(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

NTAPI
NTSTATUS
PortClsPower(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

NTAPI
NTSTATUS
PortClsSysControl(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);


typedef struct
{
    PCPFNSTARTDEVICE StartDevice;

    IResourceList* resources;
} PCExtension;

#endif
