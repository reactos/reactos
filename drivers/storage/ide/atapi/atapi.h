/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Common header file
 * COPYRIGHT:   Copyright 2024-2025 Dmitry Borisov <di.sean@protonmail.com>
 */

#pragma once

#include <ntddk.h>
#include <ntstrsafe.h>
#include <ntintsafe.h>

#include <wmilib.h>
#include <initguid.h>
#include <wdmguid.h>
#include <wmistr.h>
#include <wmidata.h>

#include <ata.h>
#include <ide.h>
#include <scsi.h>
#include <ntddscsi.h>
#include <ntdddisk.h>

#include <sptilib.h>
#include <section_attribs.h>
#include <debug/driverdbg.h>
#include <reactos/drivers/ata/ntddata.h>

typedef struct _ATAPORT_CHANNEL_EXTENSION ATAPORT_CHANNEL_EXTENSION, *PATAPORT_CHANNEL_EXTENSION;
typedef struct _ATAPORT_DEVICE_EXTENSION ATAPORT_DEVICE_EXTENSION, *PATAPORT_DEVICE_EXTENSION;
typedef struct _ATAPORT_PORT_DATA ATAPORT_PORT_DATA, *PATAPORT_PORT_DATA;
typedef struct _ATA_DEVICE_REQUEST ATA_DEVICE_REQUEST, *PATA_DEVICE_REQUEST;
typedef struct _ATAPORT_IO_CONTEXT ATAPORT_IO_CONTEXT, *PATAPORT_IO_CONTEXT;

/** @brief Private enum between the driver and storprop.dll */
typedef enum _ATA_DEVICE_CLASS
{
    DEV_UNKNOWN = 0,
    DEV_ATA = 1,
    DEV_ATAPI = 2,
    DEV_NONE = 3
} ATA_DEVICE_CLASS;

typedef union _ATA_SCSI_ADDRESS
{
    /*
     * The ordering between Lun, TargetId, and PathId fields is important
     * with address comparison.
     */
    struct
    {
        /**
         * The lun number 0-7.
         * @sa ATA_MAX_LUN_COUNT
         */
        UCHAR Lun;

        /**
         * PATA:
         * The device number 0 - Master, 1 - Slave,
         * 2 - Master (PC-98), 3 - Slave (PC-98).
         *
         * AHCI:
         * The device number 0-15.
         */
        UCHAR TargetId;

        /**
         * PATA:
         * The PCI channel number 0-1 or 0 for legacy channels.
         *
         * AHCI:
         * The port number 0-31.
         */
        UCHAR PathId;

        UCHAR IsValid;
    };
    ULONG AsULONG;
} ATA_SCSI_ADDRESS, *PATA_SCSI_ADDRESS;

typedef VOID
(STOP_IO_CALLBACK)(
    _In_ PATAPORT_IO_CONTEXT Device);
typedef STOP_IO_CALLBACK *PSTOP_IO_CALLBACK;

typedef struct _ATAPORT_IO_CONTEXT
{
    ULONG DeviceFlags;
#define DEVICE_PIO_ONLY                          0x00000001
#define DEVICE_IS_ATAPI                          0x00000002
#define DEVICE_HAS_CDB_INTERRUPT                 0x00000004
#define DEVICE_LBA_MODE                          0x00000008
#define DEVICE_LBA48                             0x00000010
#define DEVICE_HAS_FUA                           0x00000020
#define DEVICE_NCQ                               0x00000040
#define DEVICE_IS_NEC_CDR260                     0x00000080
#define DEVICE_HAS_MEDIA_STATUS                  0x00000100
#define DEVICE_NEED_DMA_DIRECTION                0x00000200
#define DEVICE_SENSE_DATA_REPORTING              0x00000400
#define DEVICE_IS_SUPER_FLOPPY                   0x00000800
#define DEVICE_IS_PMP_DEVICE                     0x00001000
#define DEVICE_IS_PDO_REMOVABLE                  0x00002000
#define DEVICE_IS_AHCI                           0x00004000
#define DEVICE_UNINITIALIZED                     0x00008000
#define DEVICE_PNP_STARTED                       0x00010000
#define DEVICE_DESCRIPTOR_SENSE                  0x00020000
#define DEVICE_HARDWARE_ERROR                    0x00040000

    PULONG PowerIdleCounter;
    PVOID LocalBuffer;
    ULONG SectorSize;
    ATA_SCSI_ADDRESS AtaScsiAddress;
    UCHAR MultiSectorCount;
    UCHAR CdbSize;
    UCHAR DeviceSelect;
    PATAPORT_CHANNEL_EXTENSION ChanExt;
    PATAPORT_PORT_DATA PortData;
    KSPIN_LOCK QueueLock;

    ULONG QueueFlags;
#define QUEUE_FLAG_FROZEN_PORT_BUSY          0x00000001
#define QUEUE_FLAG_FROZEN_SLOT               0x00000002
#define QUEUE_FLAG_FROZEN_PNP                0x00000004
#define QUEUE_FLAG_SIGNAL_STOP               0x00000008
#define QUEUE_FLAG_FROZEN_QUEUE_FREEZE       0x00000010
#define QUEUE_FLAG_FROZEN_POWER              0x00000020
#define QUEUE_FLAG_FROZEN_REMOVED            0x00000020
#define QUEUE_FLAG_FROZEN_QUEUE_LOCK         0x00080000

#define QUEUE_FLAGS_FROZEN \
    (QUEUE_FLAG_FROZEN_PORT_BUSY | \
     QUEUE_FLAG_FROZEN_SLOT | \
     QUEUE_FLAG_FROZEN_PNP | \
     QUEUE_FLAG_FROZEN_QUEUE_FREEZE | \
     QUEUE_FLAG_FROZEN_POWER | \
     QUEUE_FLAG_FROZEN_QUEUE_LOCK)

    USHORT Cylinders;
    USHORT Heads;
    USHORT SectorsPerTrack;
    ULONG64 TotalSectors;

    ULONG MaxRequestsBitmap;
    ULONG FreeRequestsBitmap;
    ULONG MaxQueuedSlotsBitmap;
    PATA_DEVICE_REQUEST Requests;

    LIST_ENTRY DeviceQueueList;
    PLIST_ENTRY NextDeviceQueueEntry;
    KEVENT QueueStoppedEvent;
    PSCSI_REQUEST_BLOCK QuiescenceSrb;

#if DBG
    struct
    {
        ULONG RequestsStarted;
        ULONG RequestsCompleted;
    } Statistics;
#endif
} ATAPORT_IO_CONTEXT, *PATAPORT_IO_CONTEXT;

#include "include/debug.h"
#include "include/acpi.h"
#include "include/ahci.h"
// #include "include/pata.h"
#include "include/identify_funcs.h"
#include "include/scsiex.h"
#include "include/request.h"
#include "include/devevent.h"

#if defined(_MSC_VER)
#pragma section("PAGECONS", read)
#endif

/** Pageable read-only data */
#define ATAPORT_DATA    DATA_SEG("PAGECONS")

#define ASSUME(cond) \
    do { \
        ASSERT(cond); \
        __assume(cond); \
    } while (0)

#define ATAPORT_TAG             'PedI'

#define IS_FDO(p) \
    ((((PATAPORT_COMMON_EXTENSION)(p))->Flags & DO_IS_FDO) != 0)

#define IS_PCIIDE_EXT(p) \
    (((p)->Common.Flags & DO_IS_PCIIDE) != 0)

#define IS_LEGACY_IDE_EXT(p) \
    (((p)->Common.Flags & DO_IS_LEGACY_IDE) != 0)

#define IS_AHCI_EXT(p) \
    (((p)->Common.Flags & DO_IS_AHCI) != 0)

#define IS_AHCI(p) \
    (((p)->DeviceFlags & DEVICE_IS_AHCI) != 0)

#define IS_ATAPI(Device) \
    (((Device)->DeviceFlags & DEVICE_IS_ATAPI) != 0)

/**
 * @brief
 * The maximum length of identifier strings for ATA devices excluding the terminating NULL.
 *
 * @sa
 * See MSDN note:
 * https://learn.microsoft.com/en-us/windows-hardware/drivers/install/identifiers-for-ide-devices
 *
 * and
 * IDENTIFY_DEVICE_DATA.SerialNumber
 * IDENTIFY_DEVICE_DATA.ModelNumber
 * IDENTIFY_DEVICE_DATA.FirmwareRevision
 */
/*@{*/
#define ATAPORT_FN_FIELD   40
#define ATAPORT_SN_FIELD   40
#define ATAPORT_RN_FIELD   8
/*@}*/

#define ATA_RESERVED_PAGES       4

#define ATA_MAX_LUN_COUNT        8

/* Maximum size (ATA Information VPD page) */
#define ATA_LOCAL_BUFFER_SIZE    572

#define SEARCH_FDO_DEV              0x00000001
#define SEARCH_REMOVED_DEV          0x00000002

typedef struct _ATA_ENABLE_INTERRUPTS_CONTEXT
{
    PATAPORT_CHANNEL_EXTENSION ChanExt;
    BOOLEAN Enable;
} ATA_ENABLE_INTERRUPTS_CONTEXT, *PATA_ENABLE_INTERRUPTS_CONTEXT;

typedef VOID
(SEND_REQUEST)(
    _In_ PATA_DEVICE_REQUEST Request);
typedef SEND_REQUEST *PSEND_REQUEST;

typedef VOID
(PREPARE_PRD_TABLE)(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ SCATTER_GATHER_LIST* __restrict SgList);
typedef PREPARE_PRD_TABLE *PPREPARE_PRD_TABLE;

typedef VOID
(PREPARE_IO)(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATA_DEVICE_REQUEST Request);
typedef PREPARE_IO *PPREPARE_IO;

typedef BOOLEAN
(START_IO)(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATA_DEVICE_REQUEST Request);
typedef START_IO *PSTART_IO;

typedef struct _ATAPORT_PATA_PORT_DATA
{
    PCIIDE_INTERFACE PciIdeInterface;
    IDE_REGISTERS Registers;
    PUCHAR DataBuffer;
    ULONG BytesToTransfer;
    ULONG CommandFlags;
    ULONG DrqByteCount;
    PUCHAR CommandPortBase;
    PUCHAR ControlPortBase;
    UCHAR LastTargetId;
    IDE_ACPI_TIMING_MODE_BLOCK CurrentTimingMode;
    PVOID LocalBuffer;
} ATAPORT_PATA_PORT_DATA, *PATAPORT_PATA_PORT_DATA;

typedef struct _ATAPORT_AHCI_PORT_DATA
{
    PULONG IoBase;
    SCATTER_GATHER_LIST LocalSgList;
    PAHCI_RECEIVED_FIS ReceivedFis;
    PAHCI_COMMAND_LIST CommandList;
    ULONG64 ReceivedFisPhys;
    ULONG64 CommandListPhys;
    PAHCI_COMMAND_TABLE CommandTable[AHCI_MAX_COMMAND_SLOTS];
} ATAPORT_AHCI_PORT_DATA, *PATAPORT_AHCI_PORT_DATA;

typedef struct DECLSPEC_CACHEALIGN _ATAPORT_PORT_DATA
{
    KSPIN_LOCK QueueLock;
    LIST_ENTRY PortQueueList;
    ULONG ActiveTimersBitmap;
    ULONG FreeSlotsBitmap;
    ULONG LastUsedSlot;
    ULONG QueueFlags;
#define PORT_QUEUE_FLAG_LAST_TARGET_MASK          0x000000FF
#define PORT_QUEUE_FLAG_EXCLUSIVE_MODE            0x00000100
#define PORT_QUEUE_FLAG_EXCLUSIVE_PMP_MODE        0x00000200
// #define PORT_QUEUE_FLAG_FROZEN                    0x00000400
#define PORT_QUEUE_FLAG_SIGNAL_STOP               0x00000800

    /**
     * >0: we have native queued commands pending.
     * =0: the slot queue is empty.
     * <0: we have non-queued commands pending.
     */
    LONG AllocatedSlots;

    ULONG InterruptFlags;
#define PORT_INT_FLAG_IS_IO_ACTIVE       0x00000001
#define PORT_INT_FLAG_IS_DPC_ACTIVE      0x00000002
#define PORT_INT_FLAG_IGNORE_LINK_IRQ    0x00000004

    ULONG ActiveSlotsBitmap;
    ULONG ActiveQueuedSlotsBitmap;
    ULONG PortFlags;
/** FIS-based switching supported */
#define PORT_FLAG_HAS_FBS            0x00000002

/** FIS-based switching enabled */
#define PORT_FLAG_FBS_ENABLED        0x00000004

/** Port Multiplier is connected */
#define PORT_FLAG_IS_PMP             0x00000008

#define PORT_FLAG_IO32               0x00000010
#define PORT_FLAG_SIMPLEX_DMA        0x00000020
#define PORT_FLAG_PORT_MULTIPLIER    0x00000040
#define PORT_FLAG_IN_POLL_STATE      0x00000080
#define PORT_FLAG_IO_MODE_DETECTED   0x00000100

/** FIS-based switching enabled */
#define PORT_FLAG_IS_EXTERNAL        0x00000200

/** NEC PC-98 internal IDE controller */
#define PORT_FLAG_CBUS_IDE           0x80000000

    PATAPORT_CHANNEL_EXTENSION ChanExt;
    PPREPARE_PRD_TABLE PreparePrdTable;
    PPREPARE_IO PrepareIo;
    PSTART_IO StartIo;
    PSEND_REQUEST SendRequest;
    PATA_DEVICE_REQUEST Slots[AHCI_MAX_COMMAND_SLOTS];
    LONG TimerCount[AHCI_MAX_COMMAND_SLOTS];
    KDPC PollingTimerDpc;
    ULONG PortNumber;
    ULONG MaxSlotsBitmap;
    union
    {
        ATAPORT_PATA_PORT_DATA Pata;
        ATAPORT_AHCI_PORT_DATA Ahci;
    };

    ATA_WORKER_CONTEXT Worker;
} ATAPORT_PORT_DATA, *PATAPORT_PORT_DATA;

typedef struct _ATAPORT_AHCI_PORT_INFO
{
    PVOID ReceivedFisOriginal;
    PVOID CommandListOriginal;
    PVOID CommandTableOriginal[AHCI_MAX_COMMAND_SLOTS];
    ULONG CommandTableSize[AHCI_MAX_COMMAND_SLOTS];
    ULONG CommandListSize;
    PHYSICAL_ADDRESS LocalBufferPa;
    PHYSICAL_ADDRESS ReceivedFisPhysOriginal;
    PHYSICAL_ADDRESS CommandListPhysOriginal;
    PHYSICAL_ADDRESS CommandTablePhysOriginal[AHCI_MAX_COMMAND_SLOTS];
    PVOID LocalBuffer;
} ATAPORT_PORT_INFO, *PATAPORT_PORT_INFO;

typedef struct _ATAPORT_COMMON_EXTENSION
{
    _Write_guarded_by_(_Global_interlock_)
    volatile LONG PageFiles;

    _Write_guarded_by_(_Global_interlock_)
    volatile LONG HibernateFiles;

    _Write_guarded_by_(_Global_interlock_)
    volatile LONG DumpFiles;

    /** PNP device power state. */
    DEVICE_POWER_STATE DevicePowerState;

    /** PNP system power state. */
    SYSTEM_POWER_STATE SystemPowerState;

    ULONG Flags;
#define DO_IS_AHCI       0x00000001
#define DO_IS_PCIIDE     0x00000002
#define DO_IS_LEGACY_IDE 0x00000004
#define DO_IS_FDO        0x80000000

    PDEVICE_OBJECT Self;

    IO_REMOVE_LOCK RemoveLock;
} ATAPORT_COMMON_EXTENSION, *PATAPORT_COMMON_EXTENSION;

typedef struct _ATAPORT_CHANNEL_EXTENSION
{
    /** Common data, must be the first member. */
    ATAPORT_COMMON_EXTENSION Common;

    PULONG IoBase;
    ULONG PortBitmap;
    ULONG NumberOfPorts;
    PATAPORT_PORT_DATA PortData;
    PDMA_ADAPTER AdapterObject;
    PDEVICE_OBJECT AdapterDeviceObject;
    PKINTERRUPT InterruptObject;
    SLIST_HEADER CompletionQueueList;
    ULONG MaximumTransferLength;
    ULONG MaximumPhysicalPages;
    ULONG AhciCapabilities;
    ULONG AhciCapabilitiesEx;
    ULONG MapRegisterCount;
    PDEVICE_OBJECT Ldo;
    KDPC CompletionDpc;
    PVOID ReservedVaSpace;
    volatile LONG ReservedMappingLock;

    FAST_MUTEX DeviceSyncMutex;

    KSPIN_LOCK PdoListLock;

    SINGLE_LIST_ENTRY PdoList;

    /** Maximum possible Target ID per port for this FDO */
    ULONG MaxTargetId;

    ULONG Flags;
#define CHANNEL_CONTROL_PORT_BASE_MAPPED       0x00000001
#define CHANNEL_COMMAND_PORT_BASE_MAPPED       0x00000002
#define CHANNEL_INTERRUPT_SHARED               0x00000004
#define CHANNEL_PRIMARY_ADDRESS_CLAIMED        0x00000008
#define CHANNEL_SECONDARY_ADDRESS_CLAIMED      0x00000010
#define CHANNEL_SYMLINK_CREATED                0x00000020
#define CHANNEL_HAS_GTM                        0x00000040
#define CHANNEL_IO_TIMER_ACTIVE                0x00000080
#define CHANNEL_PIO_ONLY                       0x00000100

    ULONG DeviceObjectNumber;

    ULONG ScsiPortNumber;
    KIRQL InterruptLevel;
    ULONG InterruptVector;
    ULONG IoLength;
    USHORT DeviceID;
    USHORT VendorID;
    KINTERRUPT_MODE InterruptMode;
    KAFFINITY InterruptAffinity;
    UNICODE_STRING StorageInterfaceName;
    PDEVICE_OBJECT Pdo;
    PATAPORT_PORT_INFO PortInfo;
    BUS_INTERFACE_STANDARD BusInterface;
} ATAPORT_CHANNEL_EXTENSION, *PATAPORT_CHANNEL_EXTENSION;

typedef struct _ATAPORT_DEVICE_EXTENSION
{
    /** Common data, must be the first member. */
    ATAPORT_COMMON_EXTENSION Common;

    ATAPORT_IO_CONTEXT Device;
    ATA_WORKER_DEVICE_CONTEXT Worker;

    BOOLEAN ReportedMissing;
    BOOLEAN RemovalPending;
    BOOLEAN NotPresent;
    BOOLEAN DeviceClaimed;

    /** Power IRP queue list. */
    LIST_ENTRY PowerIrpQueueList;

    /** PDO list entry. */
    SINGLE_LIST_ENTRY ListEntry;

    /** 512-byte block of device identify data. */
    union
    {
        IDENTIFY_DEVICE_DATA IdentifyDeviceData;
        IDENTIFY_PACKET_DATA IdentifyPacketData;
    };

    /** Standard inquiry data. */
    INQUIRYDATA InquiryData;

    /** Device strings for PNP and IOCTL operations. */
    /*@{*/
    _Field_z_ CHAR FriendlyName[ATAPORT_FN_FIELD + sizeof(ANSI_NULL)];
    _Field_z_ CHAR RevisionNumber[ATAPORT_RN_FIELD + sizeof(ANSI_NULL)];
    _Field_z_ CHAR SerialNumber[ATAPORT_SN_FIELD + sizeof(ANSI_NULL)];
    /*@}*/

    /**
     * Current device class.
     * @sa ATA_DEVICE_CLASS
     *
     * Passed to storprop.dll through registry.
     */
    ULONG DeviceClass;

    /**
     * Bit map for enabled/active transfer modes on the device.
     * Holds the value based off of identify data.
     */
    ULONG TransferModeCurrentBitmap;

    /**
     * Bit map for supported transfer modes on the device.
     * Holds the value based off of identify data.
     *
     * Passed to storprop.dll through registry.
     */
    ULONG TransferModeSupportedBitmap;

    /**
     * Selected transfer modes on the device as a bit map
     * with 1-2 bits set (+1 for DMA mode (may be disabled) and +1 for PIO mode).
     *
     * Passed to storprop.dll through registry.
     */
    ULONG TransferModeSelectedBitmap;

    /**
     * Bit map for allowed transfer modes on the device.
     * DMA errors cause the upper bits to be cleared until PIO mode is reached.
     */
    ULONG TransferModeAllowedMask;

    /** Cycle time in nanoseconds for PIO or DMA transfer mode. -1 if not supported. */
    /*@{*/
    ULONG PioCycleTime;
    ULONG SingleWordDmaCycleTime;
    ULONG MultiWordDmaCycleTime;
    ULONG UltraDmaCycleTime;
    /*@}*/

    /** List of ATA commands to the drive created by the ACPI firmware. */
    PVOID GtfDataBuffer;

    /** Tracks the last time a DMA error was encountered. */
    LARGE_INTEGER LastDmaErrorTime;

    /** WMI data provider interface. */
    WMILIB_CONTEXT WmiLibInfo;
} ATAPORT_DEVICE_EXTENSION, *PATAPORT_DEVICE_EXTENSION;

typedef struct _ATAPORT_ENUM_CONTEXT
{
    PKEVENT WaitEvents[AHCI_MAX_PORTS];
    KWAIT_BLOCK WaitBlocks[ANYSIZE_ARRAY];
} ATAPORT_ENUM_CONTEXT, *PATAPORT_ENUM_CONTEXT;

C_ASSERT(QUEUE_FLAG_FROZEN_QUEUE_FREEZE == SRB_FLAGS_BYPASS_FROZEN_QUEUE);
C_ASSERT(QUEUE_FLAG_FROZEN_QUEUE_LOCK == SRB_FLAGS_BYPASS_LOCKED_QUEUE);

#include "include/pata.h"
#include "include/atahw.h"

extern UNICODE_STRING AtapDriverRegistryPath;
extern BOOLEAN AtapInPEMode;

typedef enum
{
    GetDeviceType,
    GetGenericType,
    GetPeripheralId
} DEVICE_TYPE_NAME;

FORCEINLINE
BOOLEAN
AtaPortQueueEmpty(
    _In_ PATAPORT_PORT_DATA PortData)
{
    ULONG SlotsBitmap;

    SlotsBitmap = PortData->Worker.PausedSlotsBitmap | PortData->FreeSlotsBitmap;

    return (SlotsBitmap == PortData->MaxSlotsBitmap);
}

FORCEINLINE
ATA_SCSI_ADDRESS
AtaMarshallScsiAddress(
    _In_ ULONG PathId,
    _In_ ULONG TargetId,
    _In_ ULONG Lun)
{
    ATA_SCSI_ADDRESS AtaScsiAddress;

    AtaScsiAddress.Lun = Lun;
    AtaScsiAddress.TargetId = TargetId;
    AtaScsiAddress.PathId = PathId;

    /* This is used for address comparison. See AtaFdoFindNextDeviceByPath() */
    AtaScsiAddress.IsValid = 0xAA;

    return AtaScsiAddress;
}

FORCEINLINE
ULONG
CountSetBits(
    _In_ ULONG x)
{
    x -= x >> 1 & 0x55555555;
    x = (x & 0x33333333) + (x >> 2 & 0x33333333);

    return ((x + (x >> 4)) & 0x0F0F0F0F) * 0x01010101 >> 24;
}

FORCEINLINE
BOOLEAN
IsPowerOfTwo(
    _In_ ULONG x)
{
    /* Also exclude zero numbers */
    return (x != 0) && ((x & (x - 1)) == 0);
}

/* acpi.c *********************************************************************/

BOOLEAN
AtaAcpiGetTimingMode(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _Out_ PIDE_ACPI_TIMING_MODE_BLOCK TimingMode);

VOID
AtaAcpiSetTimingMode(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PIDE_ACPI_TIMING_MODE_BLOCK TimingMode,
    _In_opt_ PIDENTIFY_DEVICE_DATA IdBlock1,
    _In_opt_ PIDENTIFY_DEVICE_DATA IdBlock2);

CODE_SEG("PAGE")
PVOID
AtaAcpiGetTaskFile(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

CODE_SEG("PAGE")
VOID
AtaAcpiSetDeviceData(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PIDENTIFY_DEVICE_DATA IdBlock);

BOOLEAN
AtaAcpiSetupNextGtfRequest(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PULONG Index);

/* ahci_hw.c *******************************************************************/

CODE_SEG("PAGE")
NTSTATUS
AtaAhciInitHba(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt);

CODE_SEG("PAGE")
BOOLEAN
AtaAhciPortInit(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _Out_ PATAPORT_PORT_DATA PortData,
    _Out_ PATAPORT_PORT_INFO PortInfo);

VOID
AtaAhciRunStateMachine(
    _In_ PATAPORT_PORT_DATA PortData);

VOID
AtaAhciEnterPhyListenMode(
    _In_ PATAPORT_PORT_DATA PortData);

VOID
AtaAhciEnableInterrupts(
    _In_ PATAPORT_PORT_DATA PortData);

VOID
AtaAhciSaveTaskFile(
    _In_ PATAPORT_PORT_DATA PortData,
    _Inout_ PATA_DEVICE_REQUEST Request,
    _In_ BOOLEAN ProcessErrorStatus);

VOID
AtaAhciHandlePortStateChange(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ ULONG InterruptStatus);

VOID
AtaAhciHandleFatalError(
    _In_ PATAPORT_PORT_DATA PortData);

BOOLEAN
AtaAhciDowngradeInterfaceSpeed(
    _In_ PATAPORT_PORT_DATA PortData);

/* ahci_io.c ******************************************************************/

PREPARE_PRD_TABLE AtaAhciPreparePrdTable;
PREPARE_IO AtaAhciPrepareIo;
START_IO AtaAhciStartIo;
KSERVICE_ROUTINE AtaHbaIsr;

VOID
AtaAhciPortHandleInterrupt(
    _In_ PATAPORT_PORT_DATA PortData);

/* ahci_mem.c *****************************************************************/

CODE_SEG("PAGE")
BOOLEAN
AtaAhciPortAllocateMemory(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _Out_ PATAPORT_PORT_DATA PortData,
    _Out_ PATAPORT_PORT_INFO PortInfo);

CODE_SEG("PAGE")
VOID
AtaFdoFreePortMemory(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt);

/* atapi.c ********************************************************************/

_Dispatch_type_(IRP_MJ_CREATE)
_Dispatch_type_(IRP_MJ_CLOSE)
CODE_SEG("PAGE")
DRIVER_DISPATCH_PAGED AtaDispatchCreateClose;

CODE_SEG("PAGE")
DRIVER_ADD_DEVICE AtaAddDevice;

CODE_SEG("PAGE")
DRIVER_UNLOAD AtaUnload;

CODE_SEG("INIT")
DRIVER_INITIALIZE DriverEntry;

CODE_SEG("PAGE")
NTSTATUS
NTAPI
AtaAddChannel(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _Out_ PATAPORT_CHANNEL_EXTENSION *FdoExtension);

CODE_SEG("PAGE")
NTSTATUS
AtaFdoQueryInterface(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ const GUID* Guid,
    _Out_ PVOID Interface,
    _In_ ULONG Size);

CODE_SEG("PAGE")
NTSTATUS
AtaOpenRegistryKey(
    _In_ PHANDLE KeyHandle,
    _In_ HANDLE RootKey,
    _In_ PUNICODE_STRING KeyName,
    _In_ BOOLEAN Create);

CODE_SEG("PAGE")
VOID
AtaGetRegistryKey(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ UCHAR TargetId,
    _In_ PCWSTR KeyName,
    _Out_ PULONG KeyValue,
    _In_ ULONG DefaultValue);

CODE_SEG("PAGE")
VOID
AtaSetRegistryKey(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ UCHAR TargetId,
    _In_ PCWSTR KeyName,
    _In_ ULONG KeyValue);

VOID
AtaSetPortRegistryKey(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PCWSTR KeyName,
    _In_ ULONG KeyValue);

DECLSPEC_NOINLINE_FROM_PAGED
PATAPORT_DEVICE_EXTENSION
AtaFdoFindNextDeviceByPath(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _Inout_ PATA_SCSI_ADDRESS AtaScsiAddress,
    _In_ ULONG MatchFlags,
    _In_ PVOID ReferenceTag);

DECLSPEC_NOINLINE_FROM_PAGED
VOID
AtaFdoDeviceListInsert(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ BOOLEAN DoInsert);

VOID
AtaDeviceFlushPowerIrpQueue(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

/* data.c *********************************************************************/

CODE_SEG("PAGE")
PCSTR
AtaTypeCodeToName(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ DEVICE_TYPE_NAME Type);

/* enum.c *********************************************************************/

VOID
AtaDeviceSetAddressingMode(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

PUCHAR
AtaCopyIdStringUnsafe(
    _Out_writes_bytes_all_(Length) PUCHAR Destination,
    _In_reads_bytes_(Length) PUCHAR Source,
    _In_ ULONG Length);

VOID
AtaSwapIdString(
    _Inout_updates_bytes_(Length * sizeof(USHORT)) PVOID Buffer,
    _In_range_(>, 0) ULONG Length);

PCHAR
AtaCopyIdStringSafe(
    _Out_writes_bytes_all_(MaxLength) PCHAR Destination,
    _In_reads_bytes_(MaxLength) PUCHAR Source,
    _In_ ULONG MaxLength,
    _In_ CHAR DefaultCharacter);

CODE_SEG("PAGE")
NTSTATUS
AtaFdoQueryBusRelations(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PIRP Irp);

/* fdo.c **********************************************************************/

CODE_SEG("PAGE")
NTSTATUS
AtaFdoStartDevice(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PCM_RESOURCE_LIST ResourcesTranslated);

CODE_SEG("PAGE")
NTSTATUS
AtaFdoRemoveDevice(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PIRP Irp,
    _In_ BOOLEAN FinalRemove);

DECLSPEC_NOINLINE_FROM_PAGED
PATAPORT_DEVICE_EXTENSION
AtaFdoFindDeviceByPath(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ ATA_SCSI_ADDRESS AtaScsiAddress,
    _In_ PVOID ReferenceTag);

CODE_SEG("PAGE")
NTSTATUS
AtaFdoPnp(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _Inout_ PIRP Irp);

/* pata_hw.c ******************************************************************/

BOOLEAN
AtaPataResetDevice(
    _In_ PATAPORT_PORT_DATA PortData);

ATA_CONNECTION_STATUS
AtaPataIdentifyTargetDevice(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

ULONG
AtaPataIdentifyPortDevice(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

VOID
AtaPataRunStateMachine(
    _In_ PATAPORT_PORT_DATA PortData);

/* pata_io.c ******************************************************************/

PREPARE_PRD_TABLE AtaPciIdePreparePrdTable;
PREPARE_IO AtaPataPrepareIo;
START_IO AtaPataStartIo;
KSERVICE_ROUTINE AtaPataChannelIsr;
KSERVICE_ROUTINE AtaPciIdeChannelIsr;

VOID
AtaPataPoll(
    _In_ PATAPORT_PORT_DATA PortData);

BOOLEAN
AtaPataDmaTransferToPioTransfer(
    _In_ PATA_DEVICE_REQUEST Request);

/* pata_legacy.c **************************************************************/

#if defined(ATA_DETECT_LEGACY_DEVICES)
CODE_SEG("INIT")
VOID
AtaDetectLegacyChannels(
    _In_ PDRIVER_OBJECT DriverObject);
#endif

/* ioctl.c ********************************************************************/

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH_RAISED AtaDispatchDeviceControl;

/* pdo.c **********************************************************************/

IO_COMPLETION_ROUTINE AtaPdoCompletionRoutine;

_Dispatch_type_(IRP_MJ_PNP)
CODE_SEG("PAGE")
DRIVER_DISPATCH_PAGED AtaDispatchPnp;

CODE_SEG("PAGE")
VOID
AtaPdoFreeDevice(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

CODE_SEG("PAGE")
PATAPORT_DEVICE_EXTENSION
AtaPdoCreateDevice(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ ATA_SCSI_ADDRESS AtaScsiAddress);

/* portstate.c ****************************************************************/

KDEFERRED_ROUTINE AtaPortWorkerDpc;
REQUEST_COMPLETION_ROUTINE AtaPortCompleteInternalRequest;

DECLSPEC_NOINLINE_FROM_PAGED
VOID
AtaDeviceQueueEvent(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_opt_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ATA_PORT_ACTION Action);

VOID
AtaPortRecoveryFromError(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ ATA_PORT_ACTION Action,
    _In_ PATA_DEVICE_REQUEST Request);

VOID
AtaPortTimeout(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ ULONG Slot);

VOID
AtaPortQueueEmptyEvent(
    _In_ PATA_WORKER_CONTEXT Worker);

VOID
AtaFsmResetPort(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ ULONG TargetId);

VOID
AtaFsmCompletePortEnumEvent(
    _In_ PATAPORT_PORT_DATA PortData,
    _In_ ULONG TargetBitmap);

VOID
AtaFsmCompleteDeviceEnumEvent(
    _In_ PATA_WORKER_CONTEXT Context,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ATA_DEVICE_STATUS Status);

VOID
AtaFsmCompleteDeviceErrorEvent(
    _In_ PATAPORT_PORT_DATA PortData);

VOID
AtaFsmCompleteDeviceConfigEvent(
    _In_ PATA_WORKER_CONTEXT Context,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

VOID
AtaFsmCompleteDevicePowerEvent(
    _In_ PATA_WORKER_CONTEXT Context,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

VOID
AtaDeviceRecoveryRunStateMachine(
    _In_ PATAPORT_PORT_DATA PortData);

UCHAR
AtaDeviceGetFlushCacheCommand(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

/* satl.c *********************************************************************/

CODE_SEG("PAGE")
VOID
AtaCreateStandardInquiryData(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt);

UCHAR
AtaReqExecuteScsi(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb);

UCHAR
AtaReqSetFixedAtaSenseData(
    _In_ PATA_DEVICE_REQUEST Request);

VOID
AtaReqBuildReadLogTaskFile(
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ UCHAR LogAddress,
    _In_ UCHAR PageNumber,
    _In_ USHORT LogPageCount);

/* scsi.c *********************************************************************/

_Dispatch_type_(IRP_MJ_SCSI)
DRIVER_DISPATCH_RAISED AtaDispatchScsi;

SEND_REQUEST AtaReqSendRequest;
IO_TIMER_ROUTINE AtaReqPortIoTimer;
KDEFERRED_ROUTINE AtaReqCompletionDpc;
KDEFERRED_ROUTINE AtaReqPollingTimerDpc;

VOID
AtaReqCompleteFailedRequest(
    _In_ PATA_DEVICE_REQUEST Request);

UCHAR
AtaReqSetFixedSenseData(
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _In_ SCSI_SENSE_CODE SenseCode);

VOID
AtaReqSetLbaInformation(
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _In_ ULONG64 Lba);

BOOLEAN
AtaReqAllocateMdl(
    _In_ PATA_DEVICE_REQUEST Request);

VOID
AtaReqStartCompletionDpc(
    _In_ PATA_DEVICE_REQUEST Request);

DECLSPEC_NOINLINE_FROM_PAGED
VOID
AtaReqFreezeQueue(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ULONG ReasonFlags);

DECLSPEC_NOINLINE_FROM_PAGED
VOID
AtaReqThawQueue(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ ULONG ReasonFlags);

DECLSPEC_NOINLINE_FROM_PAGED
VOID
AtaReqWaitForOutstandingIoToComplete(
    _In_ PATAPORT_IO_CONTEXT Device,
    _In_ PSCSI_REQUEST_BLOCK Srb);

DECLSPEC_NOINLINE_FROM_PAGED
VOID
AtaReqFlushDeviceQueue(
    _In_ PATAPORT_IO_CONTEXT Device);

/* smart.c ********************************************************************/

UCHAR
AtaReqSmartIoControl(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PATA_DEVICE_REQUEST Request,
    _In_ PSCSI_REQUEST_BLOCK Srb);

CODE_SEG("PAGE")
NTSTATUS
AtaPdoHandleMiniportSmartVersion(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PSCSI_REQUEST_BLOCK Srb);

CODE_SEG("PAGE")
NTSTATUS
AtaPdoHandleMiniportIdentify(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PSCSI_REQUEST_BLOCK Srb);

/* timings.c ******************************************************************/

VOID
AtaPortUpdateTimingInformation(
    _In_ PATAPORT_PORT_DATA PortData);

/* wmi.c **********************************************************************/

_Dispatch_type_(IRP_MJ_SYSTEM_CONTROL)
CODE_SEG("PAGE")
DRIVER_DISPATCH_PAGED AtaDispatchWmi;

CODE_SEG("PAGE")
NTSTATUS
AtaPdoWmiRegistration(
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ BOOLEAN Register);

/* dev_power.c ********************************************************************/

_Dispatch_type_(IRP_MJ_POWER)
DRIVER_DISPATCH_RAISED AtaDispatchPower;

VOID
AtaDeviceIdRunStateMachine(
    _In_ PATA_WORKER_CONTEXT Context);

VOID
AtaDeviceSendIdentify(
    _In_ PATA_WORKER_CONTEXT Context,
    _In_ UCHAR Command);

VOID
AtaDeviceConfigRunStateMachine(
    _In_ PATA_WORKER_CONTEXT Context);

VOID
AtaPowerRunStateMachine(
    _In_ PATA_WORKER_CONTEXT Context);
