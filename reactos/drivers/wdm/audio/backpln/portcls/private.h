/*
    PortCls FDO Extension

    by Andrew Greenwood
*/

#ifndef PORTCLS_PRIVATE_H
#define PORTCLS_PRIVATE_H

/* windows.h should no be included my drivers at all */
#ifndef _MSC_VER
#include <windows.h>
#endif

#include <ntddk.h>
#include <debug.h>

#include <portcls.h>

#ifdef _MSC_VER
  #define STDCALL
  #define DDKAPI
#endif

NTSTATUS
NTAPI
PortClsCreate(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

NTSTATUS
NTAPI
PortClsPnp(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

NTSTATUS
NTAPI
PortClsPower(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

NTSTATUS
NTAPI
PortClsSysControl(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);


typedef struct
{
    PCPFNSTARTDEVICE StartDevice;

    IResourceList* resources;
} PCExtension;

#endif
