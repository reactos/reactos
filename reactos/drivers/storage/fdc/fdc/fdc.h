/*
 * PROJECT:        ReactOS Floppy Disk Controller Driver
 * LICENSE:        GNU GPLv2 only as published by the Free Software Foundation
 * FILE:           drivers/storage/fdc/fdc/fdc.h
 * PURPOSE:        Common header file
 * PROGRAMMERS:    Eric Kohl
 */

#include <ntddk.h>
#include <debug.h>


typedef struct _COMMON_DEVICE_EXTENSION
{
    BOOLEAN IsFDO;
} COMMON_DEVICE_EXTENSION, *PCOMMON_DEVICE_EXTENSION;

typedef struct _FDO_DEVICE_EXTENSION
{
    COMMON_DEVICE_EXTENSION Common;

    PDEVICE_OBJECT LowerDevice;
    PDEVICE_OBJECT Fdo;
    PDEVICE_OBJECT Pdo;

} FDO_DEVICE_EXTENSION, *PFDO_DEVICE_EXTENSION;


/* fdo.c */

NTSTATUS
NTAPI
FdcAddDevice(IN PDRIVER_OBJECT DriverObject,
             IN PDEVICE_OBJECT Pdo);

NTSTATUS
NTAPI
FdcFdoPnp(IN PDEVICE_OBJECT DeviceObject,
          IN PIRP Irp);

/* pdo.c */

NTSTATUS
NTAPI
FdcPdoPnp(IN PDEVICE_OBJECT DeviceObject,
          IN PIRP Irp);


/* EOF */