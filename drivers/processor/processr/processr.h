/*
 * PROJECT:        ReactOS Generic CPU Driver
 * LICENSE:        GNU GPLv2 only as published by the Free Software Foundation
 * FILE:           drivers/processor/processr/processr.h
 * PURPOSE:        Common header file
 * PROGRAMMERS:    Eric Kohl <eric.kohl@reactos.org>
 */

#ifndef _PROCESSR_PCH_
#define _PROCESSR_PCH_

#include <ntddk.h>

typedef struct _DEVICE_EXTENSION
{
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_OBJECT LowerDevice;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


/* misc.c */

NTSTATUS
NTAPI
ForwardIrpAndForget(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);


/* pnp.c */

NTSTATUS
NTAPI
ProcessorPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS
NTAPI
ProcessorAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT Pdo);

#endif /* _PROCESSR_PCH_ */
