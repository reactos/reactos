/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Parallel Port Function Driver
 * FILE:            drivers/parallel/parport/parport.h
 * PURPOSE:         Parport driver header
 */

#ifndef _PARPORT_PCH_
#define _PARPORT_PCH_

#include <ntddk.h>
#include <ndk/haltypes.h>
#include <ntddpar.h>
#include <stdio.h>

#include "hardware.h"

//#define NDEBUG
#include <debug.h>

typedef enum
{
    dsStopped,
    dsStarted,
    dsPaused,
    dsRemoved,
    dsSurpriseRemoved
} DEVICE_STATE;

typedef struct _COMMON_DEVICE_EXTENSION
{
    BOOLEAN IsFDO;
    DEVICE_STATE PnpState;
} COMMON_DEVICE_EXTENSION, *PCOMMON_DEVICE_EXTENSION;

typedef struct _FDO_DEVICE_EXTENSION
{
    COMMON_DEVICE_EXTENSION Common;

    PDEVICE_OBJECT Pdo;
    PDEVICE_OBJECT LowerDevice;

    PDEVICE_OBJECT AttachedRawPdo;
    PDEVICE_OBJECT AttachedPdo[2];

    ULONG PortNumber;

    ULONG OpenCount;

    ULONG BaseAddress;
    PKINTERRUPT Interrupt;

} FDO_DEVICE_EXTENSION, *PFDO_DEVICE_EXTENSION;

typedef struct _PDO_DEVICE_EXTENSION
{
    COMMON_DEVICE_EXTENSION Common;

    PDEVICE_OBJECT AttachedFdo;

    ULONG PortNumber;
    ULONG LptPort;

    ULONG OpenCount;

} PDO_DEVICE_EXTENSION, *PPDO_DEVICE_EXTENSION;

#define PARPORT_TAG 'trpP'

/* fdo.c */

DRIVER_ADD_DEVICE AddDevice;

NTSTATUS
NTAPI
FdoCreate(IN PDEVICE_OBJECT DeviceObject,
          IN PIRP Irp);

NTSTATUS
NTAPI
FdoClose(IN PDEVICE_OBJECT DeviceObject,
         IN PIRP Irp);

NTSTATUS
NTAPI
FdoCleanup(IN PDEVICE_OBJECT DeviceObject,
           IN PIRP Irp);

NTSTATUS
NTAPI
FdoRead(IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp);

NTSTATUS
NTAPI
FdoWrite(IN PDEVICE_OBJECT DeviceObject,
         IN PIRP Irp);

NTSTATUS
NTAPI
FdoPnp(IN PDEVICE_OBJECT DeviceObject,
       IN PIRP Irp);

NTSTATUS
NTAPI
FdoPower(IN PDEVICE_OBJECT DeviceObject,
         IN PIRP Irp);


/* misc.c */

DRIVER_DISPATCH ForwardIrpAndForget;

PVOID
GetUserBuffer(IN PIRP Irp);

//KSERVICE_ROUTINE ParportInterruptService;


/* pdo.c */

NTSTATUS
NTAPI
PdoCreate(IN PDEVICE_OBJECT DeviceObject,
          IN PIRP Irp);

NTSTATUS
NTAPI
PdoClose(IN PDEVICE_OBJECT DeviceObject,
         IN PIRP Irp);

NTSTATUS
NTAPI
PdoCleanup(IN PDEVICE_OBJECT DeviceObject,
           IN PIRP Irp);

NTSTATUS
NTAPI
PdoRead(IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp);

NTSTATUS
NTAPI
PdoWrite(IN PDEVICE_OBJECT DeviceObject,
         IN PIRP Irp);

NTSTATUS
NTAPI
PdoPnp(IN PDEVICE_OBJECT DeviceObject,
       IN PIRP Irp);

NTSTATUS
NTAPI
PdoPower(IN PDEVICE_OBJECT DeviceObject,
         IN PIRP Irp);

#endif /* _PARPORT_PCH_ */
