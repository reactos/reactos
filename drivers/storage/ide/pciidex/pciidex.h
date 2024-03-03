/*
 * PROJECT:     PCI IDE bus driver extension
 * LICENSE:     See COPYING in the top level directory
 * PURPOSE:     Common header file
 * COPYRIGHT:   Copyright 2005 Hervé Poussineau <hpoussin@reactos.org>
 *              Copyright 2023 Dmitry Borisov <di.sean@protonmail.com>
 */

#ifndef _PCIIDEX_PCH_
#define _PCIIDEX_PCH_

#include <ntddk.h>
#include <ntstrsafe.h>
#include <ntintsafe.h>
#include <initguid.h>
#include <wdmguid.h>
#include <ide.h>

#include <reactos/drivers/ntddata.h>

#define TAG_PCIIDEX    'XedI'

#define IS_FDO(p)    (((PCOMMON_DEVICE_EXTENSION)(p))->IsFDO)

#define IS_PRIMARY_CHANNEL(PdoExtension)    (PdoExtension->Channel == 0)

/*
 * Legacy ranges and interrupts
 */
#define PCIIDE_LEGACY_RESOURCE_COUNT                3
#define PCIIDE_LEGACY_COMMAND_IO_RANGE_LENGTH       8
#define PCIIDE_LEGACY_CONTROL_IO_RANGE_LENGTH       1
#define PCIIDE_LEGACY_PRIMARY_COMMAND_BASE      0x1F0
#define PCIIDE_LEGACY_PRIMARY_CONTROL_BASE      0x3F6
#define PCIIDE_LEGACY_PRIMARY_IRQ                  14
#define PCIIDE_LEGACY_SECONDARY_COMMAND_BASE    0x170
#define PCIIDE_LEGACY_SECONDARY_CONTROL_BASE    0x376
#define PCIIDE_LEGACY_SECONDARY_IRQ                15

/*
 * Programming Interface Register
 */
#define PCIIDE_PROGIF_PRIMARY_CHANNEL_NATIVE_MODE              0x01
#define PCIIDE_PROGIF_PRIMARY_CHANNEL_NATIVE_MODE_CAPABLE      0x02
#define PCIIDE_PROGIF_SECONDARY_CHANNEL_NATIVE_MODE            0x04
#define PCIIDE_PROGIF_SECONDARY_CHANNEL_NATIVE_MODE_CAPABLE    0x08
#define PCIIDE_PROGIF_DMA_CAPABLE                              0x80

typedef struct _PDO_DEVICE_EXTENSION    PDO_DEVICE_EXTENSION, *PPDO_DEVICE_EXTENSION;

typedef struct _PCIIDEX_DRIVER_EXTENSION
{
    PCONTROLLER_PROPERTIES HwGetControllerProperties;
    ULONG MiniControllerExtensionSize;
} PCIIDEX_DRIVER_EXTENSION, *PPCIIDEX_DRIVER_EXTENSION;

typedef struct _COMMON_DEVICE_EXTENSION
{
    BOOLEAN IsFDO;
    PDEVICE_OBJECT Self;

    _Write_guarded_by_(_Global_interlock_)
    volatile LONG PageFiles;

    _Write_guarded_by_(_Global_interlock_)
    volatile LONG HibernateFiles;

    _Write_guarded_by_(_Global_interlock_)
    volatile LONG DumpFiles;
} COMMON_DEVICE_EXTENSION, *PCOMMON_DEVICE_EXTENSION;

typedef struct _FDO_DEVICE_EXTENSION
{
    COMMON_DEVICE_EXTENSION Common;
    PDEVICE_OBJECT Ldo;

    ULONG ControllerNumber;
    ULONG Flags;
#define FDO_IN_NATIVE_MODE     0x00000001
#define FDO_DMA_CAPABLE        0x00000002
#define FDO_IO_BASE_MAPPED     0x00000004

    BOOLEAN MiniportStarted; // TODO move to flags

    FAST_MUTEX DeviceSyncMutex;
    _Guarded_by_(DeviceSyncMutex)
    PPDO_DEVICE_EXTENSION Channels[MAX_IDE_CHANNEL];

    USHORT VendorId;
    USHORT DeviceId;
    PDRIVER_OBJECT DriverObject;
    PUCHAR BusMasterPortBase;

    KSPIN_LOCK BusDataLock;
    BUS_INTERFACE_STANDARD BusInterface;

    IDE_CONTROLLER_PROPERTIES Properties;

    /* Must be the last entry */
    PUCHAR MiniControllerExtension[0];
} FDO_DEVICE_EXTENSION, *PFDO_DEVICE_EXTENSION;

typedef struct _PDO_DEVICE_EXTENSION
{
    COMMON_DEVICE_EXTENSION Common;
    ULONG Channel;
    PFDO_DEVICE_EXTENSION ParentController;
    BOOLEAN ReportedMissing;
    PUCHAR IoBase;
    ULONG Flags;
#define PDO_PIO_ONLY              0x00000001
#define PDO_DRIVE0_DMA_CAPABLE    0x00000002
#define PDO_DRIVE1_DMA_CAPABLE    0x00000004

    PVOID PrdTable;
    ULONG PrdTablePhysicalAddress;
    ULONG MapRegisterCount;
    PDMA_ADAPTER AdapterObject;
} PDO_DEVICE_EXTENSION, *PPDO_DEVICE_EXTENSION;

CODE_SEG("PAGE")
DRIVER_INITIALIZE DriverEntry;

CODE_SEG("PAGE")
DRIVER_UNLOAD PciIdeXUnload;

CODE_SEG("PAGE")
DRIVER_ADD_DEVICE PciIdeXAddDevice;

_Dispatch_type_(IRP_MJ_PNP)
CODE_SEG("PAGE")
DRIVER_DISPATCH_PAGED PciIdeXDispatchPnp;

_Dispatch_type_(IRP_MJ_SYSTEM_CONTROL)
CODE_SEG("PAGE")
DRIVER_DISPATCH_PAGED PciIdeXDispatchWmi;

_Dispatch_type_(IRP_MJ_POWER)
DRIVER_DISPATCH_RAISED PciIdeXDispatchPower;

CODE_SEG("PAGE")
NTSTATUS
PciIdeXFdoDispatchPnp(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension,
    _Inout_ PIRP Irp);

CODE_SEG("PAGE")
NTSTATUS
PciIdeXStartMiniport(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension);

CODE_SEG("PAGE")
IDE_CHANNEL_STATE
PciIdeXChannelState(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension,
    _In_ ULONG Channel);

#endif /* _PCIIDEX_PCH_ */
