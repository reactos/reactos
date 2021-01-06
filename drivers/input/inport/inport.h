/*
 * PROJECT:     ReactOS InPort (Bus) Mouse Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Main header file
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

#ifndef _INPORT_H_
#define _INPORT_H_

#include <wdm.h>
#include <wmilib.h>
#include <wmistr.h>
#include <kbdmou.h>

#define INPORT_TAG 'tPnI'

typedef enum
{
    dsStopped,
    dsStarted,
    dsRemoved
} INPORT_DEVICE_STATE;

typedef enum
{
    NecBusMouse,
    MsInPortMouse,
    LogitechBusMouse
} INPORT_MOUSE_TYPE;

typedef struct _INPORT_RAW_DATA
{
    CHAR DeltaX;
    CHAR DeltaY;
    UCHAR Buttons;
    ULONG ButtonDiff;
} INPORT_RAW_DATA, *PINPORT_RAW_DATA;

typedef struct _INPORT_DEVICE_EXTENSION
{
    PDEVICE_OBJECT Self;
    PDEVICE_OBJECT Pdo;
    PDEVICE_OBJECT Ldo;
    INPORT_DEVICE_STATE State;
    IO_REMOVE_LOCK RemoveLock;
    WMILIB_CONTEXT WmiLibInfo;
    PUCHAR IoBase;
    INPORT_MOUSE_TYPE MouseType;

    /* Interrupt */
    PKINTERRUPT InterruptObject;
    ULONG InterruptVector;
    KIRQL InterruptLevel;
    KINTERRUPT_MODE InterruptMode;
    BOOLEAN InterruptShared;
    KAFFINITY InterruptAffinity;

    /* Movement data and state of the mouse buttons */
    INPORT_RAW_DATA RawData;

    /* Mouclass */
    CONNECT_DATA ConnectData;
    PDEVICE_OBJECT ClassDeviceObject;
    PVOID ClassService;

    /* Mouse packet */
    MOUSE_INPUT_DATA MouseInputData;

    /* Previous state */
    ULONG MouseButtonState;

    /* Mouse device attributes */
    MOUSE_ATTRIBUTES MouseAttributes;
} INPORT_DEVICE_EXTENSION, *PINPORT_DEVICE_EXTENSION;

DRIVER_INITIALIZE DriverEntry;

DRIVER_UNLOAD InPortUnload;

DRIVER_ADD_DEVICE InPortAddDevice;

_Dispatch_type_(IRP_MJ_CREATE)
_Dispatch_type_(IRP_MJ_CLOSE)
DRIVER_DISPATCH_PAGED InPortCreateClose;

_Dispatch_type_(IRP_MJ_INTERNAL_DEVICE_CONTROL)
DRIVER_DISPATCH_RAISED InPortInternalDeviceControl;

_Dispatch_type_(IRP_MJ_POWER)
DRIVER_DISPATCH_RAISED InPortPower;

_Dispatch_type_(IRP_MJ_SYSTEM_CONTROL)
DRIVER_DISPATCH_PAGED InPortWmi;

_Dispatch_type_(IRP_MJ_PNP)
DRIVER_DISPATCH_PAGED InPortPnp;

KSERVICE_ROUTINE InPortIsr;

IO_DPC_ROUTINE InPortDpcForIsr;

KSYNCHRONIZE_ROUTINE InPortStartMouse;

KSYNCHRONIZE_ROUTINE InPortStopMouse;

NTSTATUS
NTAPI
InPortStartDevice(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp);

NTSTATUS
NTAPI
InPortRemoveDevice(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp);

VOID
NTAPI
InPortInitializeMouse(
    _In_ PINPORT_DEVICE_EXTENSION DeviceExtension);

NTSTATUS
NTAPI
InPortWmiRegistration(
    _Inout_ PINPORT_DEVICE_EXTENSION DeviceExtension);

NTSTATUS
NTAPI
InPortWmiDeRegistration(
    _Inout_ PINPORT_DEVICE_EXTENSION DeviceExtension);

NTSTATUS
NTAPI
InPortQueryWmiRegInfo(
    _Inout_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PULONG RegFlags,
    _Inout_ PUNICODE_STRING InstanceName,
    _Out_opt_ PUNICODE_STRING *RegistryPath,
    _Inout_ PUNICODE_STRING MofResourceName,
    _Out_opt_ PDEVICE_OBJECT *Pdo);

NTSTATUS
NTAPI
InPortQueryWmiDataBlock(
    _Inout_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp,
    _In_ ULONG GuidIndex,
    _In_ ULONG InstanceIndex,
    _In_ ULONG InstanceCount,
    _Out_opt_ PULONG InstanceLengthArray,
    _In_ ULONG BufferAvail,
    _Out_opt_ PUCHAR Buffer);

extern UNICODE_STRING DriverRegistryPath;

#endif /* _INPORT_H_ */
