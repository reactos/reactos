/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/include/internal/hal.h
 * PURPOSE:         Internal header for the I/O HAL Functions (Fstub)
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Pierre Schweitzer (pierre.schweitzer@reactos.org)
 */

#pragma once

//
// Default implementations of HAL dispatch table
//
VOID
FASTCALL
xHalExamineMBR(IN PDEVICE_OBJECT DeviceObject,
               IN ULONG SectorSize,
               IN ULONG MbrTypeIdentifier,
               OUT PVOID *MbrBuffer);

VOID
FASTCALL
xHalIoAssignDriveLetters(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                         IN PSTRING NtDeviceName,
                         OUT PUCHAR NtSystemPath,
                         OUT PSTRING NtSystemPathString);

NTSTATUS
FASTCALL
xHalIoReadPartitionTable(IN PDEVICE_OBJECT DeviceObject,
                         IN ULONG SectorSize,
                         IN BOOLEAN ReturnRecognizedPartitions,
                         OUT PDRIVE_LAYOUT_INFORMATION *PartitionBuffer);

NTSTATUS
FASTCALL
xHalIoSetPartitionInformation(IN PDEVICE_OBJECT DeviceObject,
                              IN ULONG SectorSize,
                              IN ULONG PartitionNumber,
                              IN ULONG PartitionType);

NTSTATUS
FASTCALL
xHalIoWritePartitionTable(IN PDEVICE_OBJECT DeviceObject,
                          IN ULONG SectorSize,
                          IN ULONG SectorsPerTrack,
                          IN ULONG NumberOfHeads,
                          IN PDRIVE_LAYOUT_INFORMATION PartitionBuffer);

VOID
NTAPI
xHalHaltSystem(
    VOID
);

VOID
NTAPI
xHalEndOfBoot(
    VOID
);

VOID
NTAPI
xHalSetWakeEnable(
    IN BOOLEAN Enable
);

UCHAR
NTAPI
xHalVectorToIDTEntry(
    IN ULONG Vector
);

NTSTATUS
NTAPI
xHalGetInterruptTranslator(
    IN INTERFACE_TYPE ParentInterfaceType,
    IN ULONG ParentBusNumber,
    IN INTERFACE_TYPE BridgeInterfaceType,
    IN USHORT Size,
    IN USHORT Version,
    OUT PTRANSLATOR_INTERFACE Translator,
    OUT PULONG BridgeBusNumber
);

PBUS_HANDLER
FASTCALL
xHalHandlerForBus(
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber
);

VOID
FASTCALL
xHalReferenceHandler(
    IN PBUS_HANDLER BusHandler
);

NTSTATUS
NTAPI
xHalInitPnpDriver(
    VOID
);

NTSTATUS
NTAPI
xHalInitPowerManagement(
    IN PPM_DISPATCH_TABLE PmDriverDispatchTable,
    OUT PPM_DISPATCH_TABLE *PmHalDispatchTable
);

NTSTATUS
NTAPI
xHalStartMirroring(
    VOID
);

NTSTATUS
NTAPI
xHalEndMirroring(
    IN ULONG PassNumber
);

NTSTATUS
NTAPI
xHalMirrorPhysicalMemory(
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN LARGE_INTEGER NumberOfBytes
);

NTSTATUS
NTAPI
xHalQueryBusSlots(
    IN PBUS_HANDLER BusHandler,
    IN ULONG BufferSize,
    OUT PULONG SlotNumbers,
    OUT PULONG ReturnedLength
);

NTSTATUS
NTAPI
xHalSetSystemInformation(
    IN HAL_SET_INFORMATION_CLASS InformationClass,
    IN ULONG BufferSize,
    IN PVOID Buffer
);

NTSTATUS
NTAPI
xHalQuerySystemInformation(
    IN HAL_QUERY_INFORMATION_CLASS InformationClass,
    IN ULONG BufferSize,
    IN OUT PVOID Buffer,
    OUT PULONG ReturnedLength
);

VOID
NTAPI
xHalLocateHiberRanges(
    IN PVOID MemoryMap
);

NTSTATUS
NTAPI
xHalRegisterBusHandler(
    IN INTERFACE_TYPE InterfaceType,
    IN BUS_DATA_TYPE ConfigSpace,
    IN ULONG BusNumber,
    IN INTERFACE_TYPE ParentInterfaceType,
    IN ULONG ParentBusNumber,
    IN ULONG ContextSize,
    IN PINSTALL_BUS_HANDLER InstallCallback,
    OUT PBUS_HANDLER *BusHandler
);

VOID
NTAPI
xHalSetWakeAlarm(
    IN ULONGLONG AlartTime,
    IN PTIME_FIELDS TimeFields
);

BOOLEAN
NTAPI
xHalTranslateBusAddress(
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
);

NTSTATUS
NTAPI
xHalAllocateMapRegisters(
    IN PADAPTER_OBJECT AdapterObject,
    IN ULONG Unknown,
    IN ULONG Unknown2,
    PMAP_REGISTER_ENTRY Registers
);

NTSTATUS
NTAPI
xKdSetupPciDeviceForDebugging(
    IN PVOID LoaderBlock OPTIONAL,
    IN OUT PDEBUG_DEVICE_DESCRIPTOR PciDevice
);

NTSTATUS
NTAPI
xKdReleasePciDeviceForDebugging(
    IN OUT PDEBUG_DEVICE_DESCRIPTOR PciDevice
);

PVOID
NTAPI
xKdGetAcpiTablePhase(
    IN struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
    IN ULONG Signature
);

PVOID
NTAPI
MatchAll(
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN ULONG NumberPages,
    IN BOOLEAN FlushCurrentTLB
);

VOID
NTAPI
xKdUnmapVirtualAddress(
    IN PVOID VirtualAddress,
    IN ULONG NumberPages,
    IN BOOLEAN FlushCurrentTLB
);


//
// Various offsets in the boot record
//
#define DISK_SIGNATURE_OFFSET           0x1B8
#define PARTITION_TABLE_OFFSET          0x1BE
#define BOOT_SIGNATURE_OFFSET           (0x200 - 2)

#define BOOT_RECORD_SIGNATURE           0xAA55
#define NUM_PARTITION_TABLE_ENTRIES     4

//
// Helper Macros
//
#define GET_STARTING_SECTOR(p)                  \
    ((ULONG)((p)->StartingSectorLsb0) +         \
     (ULONG)((p)->StartingSectorLsb1 << 8 ) +   \
     (ULONG)((p)->StartingSectorMsb0 << 16) +   \
     (ULONG)((p)->StartingSectorMsb1 << 24))

#define GET_ENDING_S_OF_CHS(p)  \
    ((UCHAR)((p)->EndingCylinderLsb & 0x3F))

#define GET_PARTITION_LENGTH(p)                     \
    ((ULONG)((p)->PartitionLengthLsb0) +            \
     (ULONG)((p)->PartitionLengthLsb1 << 8 ) +      \
     (ULONG)((p)->PartitionLengthMsb0 << 16) +      \
     (ULONG)((p)->PartitionLengthMsb1 << 24))

#define SET_PARTITION_LENGTH(p, l)                  \
    ((p)->PartitionLengthLsb0 =  (l) & 0xFF,        \
     (p)->PartitionLengthLsb1 = ((l) >> 8 ) & 0xFF, \
     (p)->PartitionLengthMsb0 = ((l) >> 16) & 0xFF, \
     (p)->PartitionLengthMsb1 = ((l) >> 24) & 0xFF)

//
// Structure describing a partition
//
typedef struct _PARTITION_DESCRIPTOR
{
    UCHAR ActiveFlag;
    UCHAR StartingTrack;
    UCHAR StartingCylinderLsb;
    UCHAR StartingCylinderMsb;
    UCHAR PartitionType;
    UCHAR EndingTrack;
    UCHAR EndingCylinderLsb;
    UCHAR EndingCylinderMsb;
    UCHAR StartingSectorLsb0;
    UCHAR StartingSectorLsb1;
    UCHAR StartingSectorMsb0;
    UCHAR StartingSectorMsb1;
    UCHAR PartitionLengthLsb0;
    UCHAR PartitionLengthLsb1;
    UCHAR PartitionLengthMsb0;
    UCHAR PartitionLengthMsb1;
} PARTITION_DESCRIPTOR, *PPARTITION_DESCRIPTOR;

//
// Structure describing a boot sector
//
typedef struct _BOOT_SECTOR_INFO
{
    UCHAR JumpByte[1];
    UCHAR Ignore1[2];
    UCHAR OemData[8];
    UCHAR BytesPerSector[2];
    UCHAR Ignore2[6];
    UCHAR NumberOfSectors[2];
    UCHAR MediaByte[1];
    UCHAR Ignore3[2];
    UCHAR SectorsPerTrack[2];
    UCHAR NumberOfHeads[2];
} BOOT_SECTOR_INFO, *PBOOT_SECTOR_INFO;

//
// Partition Table and Disk Layout
//
typedef struct _PARTITION_TABLE
{
    PARTITION_INFORMATION PartitionEntry[4];
} PARTITION_TABLE, *PPARTITION_TABLE;

typedef struct _DISK_LAYOUT
{
    ULONG TableCount;
    ULONG Signature;
    PARTITION_TABLE PartitionTable[1];
} DISK_LAYOUT, *PDISK_LAYOUT;

//
// Partition Table Entry
//
typedef struct _PTE
{
    UCHAR ActiveFlag;
    UCHAR StartingTrack;
    USHORT StartingCylinder;
    UCHAR PartitionType;
    UCHAR EndingTrack;
    USHORT EndingCylinder;
    ULONG StartingSector;
    ULONG PartitionLength;
} PTE, *PPTE;
