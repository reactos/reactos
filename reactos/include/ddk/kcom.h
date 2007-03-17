/*
    ReactOS
    Kernel-Mode COM for Kernel Streaming

    Author:
        Andrew Greenwood

    Notes:
        This is untested, and is for internal use by Kernel Streaming. The
        functions here are documented on MSDN. Does this even compile??
        Implementation should be in KS.SYS
*/

#ifndef KCOM_H
#define KCOM_H

#include <ntddk.h>

COMDDKAPI NTSTATUS NTAPI
KoCreateInstance(
    IN  REFCLSID ClassId,
    IN  IUnknown* UnkOuter OPTIONAL,
    IN  ULONG ClsContext,
    IN  REFIID InterfaceId,
    OUT PVOID* Interface);

/* Add a kernel COM Create-item entry to a device object */
COMDDKAPI NTSTATUS NTAPI
KoDeviceInitialize(
    IN  PDEVICE_OBJECT DeviceObject);

COMDDKAPI NTSTATUS NTAPI
KoDriverInitialize(
    IN  PDRIVER_OBJECT DriverObject,
    IN  PUNICODE_STRING RegistryPathName,
    IN  KoCreateObjectHandler CreateObjectHandler);

/* Decrements refcount for calling interface on an object */
COMDDKAPI NTSTATUS NTAPI
KoRelease(
    IN  REFCLSID ClassId);

#endif
