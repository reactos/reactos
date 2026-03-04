/*
 * PROJECT:         Ramdisk Class Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/storage/class/ramdisk/ramdisk.c
 * PURPOSE:         Main Driver Routines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 *                  Ahmed ARIF 2025 <arif193@gmail.com>
 */

/* INCLUDES *******************************************************************/

#include <initguid.h>
#include <ntddk.h>
#include <ntifs.h>
#include <ntdddisk.h>
#include <ntddcdrm.h>
#include <ntddstor.h>
#include <scsi.h>
#include <ntddscsi.h>
#include <ntddvol.h>
#include <mountdev.h>
#include <mountmgr.h>
#include <ketypes.h>
#include <iotypes.h>
#include <rtlfuncs.h>
#include <arc/arc.h>
#include <reactos/drivers/ntddrdsk.h>
#include "../../../filesystems/fs_rec/fs_rec.h"
#include <stdio.h>
#include <limits.h>
#include <wchar.h>
#define NDEBUG
#include <debug.h>

#define DO_XIP   0x00020000

#define ISO9660_PRIMARY_VOLUME_DESCRIPTOR_OFFSET  0x8000
#define ISO9660_PRIMARY_VOLUME_DESCRIPTOR_TYPE    0x01
#define ISO9660_PRIMARY_VOLUME_DESCRIPTOR_VERSION 0x01
#define ISO9660_SIGNATURE                         "CD001"
#define ISO9660_SIGNATURE_LENGTH                  5
#define ISO9660_PROBE_LENGTH                      2048

#include <pshpack1.h>
typedef struct _RAMDISK_MBR_PARTITION_ENTRY
{
    UCHAR BootIndicator;
    UCHAR StartHead;
    UCHAR StartSector;
    UCHAR StartCylinder;
    UCHAR SystemIndicator;
    UCHAR EndHead;
    UCHAR EndSector;
    UCHAR EndCylinder;
    ULONG SectorCountBeforePartition;
    ULONG PartitionSectorCount;
} RAMDISK_MBR_PARTITION_ENTRY, *PRAMDISK_MBR_PARTITION_ENTRY;

typedef struct _RAMDISK_MASTER_BOOT_RECORD
{
    UCHAR BootCode[0x1B8];
    ULONG Signature;
    USHORT Reserved;
    RAMDISK_MBR_PARTITION_ENTRY PartitionTable[4];
    USHORT Magic;
} RAMDISK_MASTER_BOOT_RECORD, *PRAMDISK_MASTER_BOOT_RECORD;
#include <poppack.h>

_IRQL_requires_max_(DISPATCH_LEVEL)
static __inline BOOLEAN
RamdiskBootSectorHasSignature(IN PPACKED_BOOT_SECTOR BootSector)
{
    return *((PUSHORT)((PUCHAR)BootSector + 0x1FE)) == 0xAA55;
}

#ifndef IOCTL_MOUNTDEV_QUERY_DEVICE_RELATIONS
#define IOCTL_MOUNTDEV_QUERY_DEVICE_RELATIONS \
    CTL_CODE(MOUNTDEVCONTROLTYPE, 7, METHOD_BUFFERED, FILE_ANY_ACCESS)
typedef struct _MOUNTDEV_DEVICE_RELATIONS
{
    ULONG NumberOfObjects;
    PDEVICE_OBJECT Objects[1];
} MOUNTDEV_DEVICE_RELATIONS, *PMOUNTDEV_DEVICE_RELATIONS;
#endif

/* GLOBALS ********************************************************************/

#define RAMDISK_SESSION_SIZE \
    FIELD_OFFSET(CDROM_TOC, TrackData) + sizeof(TRACK_DATA)

#define RAMDISK_TOC_SIZE \
    FIELD_OFFSET(CDROM_TOC, TrackData) + 2 * sizeof(TRACK_DATA)

#define TOC_DATA_TRACK              (0x04)

typedef enum _RAMDISK_DEVICE_TYPE
{
    RamdiskBus,
    RamdiskDrive
} RAMDISK_DEVICE_TYPE;

typedef enum _RAMDISK_DEVICE_STATE
{
    RamdiskStateUninitialized,
    RamdiskStateStarted,
    RamdiskStatePaused,
    RamdiskStateStopped,
    RamdiskStateRemoved,
    RamdiskStateBusRemoved,
    RamdiskStateEnumerated,
} RAMDISK_DEVICE_STATE;

DEFINE_GUID(RamdiskBusInterface,
            0x5DC52DF0,
            0x2F8A,
            0x410F,
            0x80, 0xE4, 0x05, 0xF8, 0x10, 0xE7, 0xA8, 0x8A);

DEFINE_GUID(RamdiskDiskInterface,
            0x31D909F0,
            0x2CDF,
            0x4A20,
            0x9E, 0xD4, 0x7D, 0x65, 0x47, 0x6C, 0xA7, 0x68);

typedef struct _RAMDISK_EXTENSION
{
    RAMDISK_DEVICE_TYPE Type;
    RAMDISK_DEVICE_STATE State;
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_OBJECT PhysicalDeviceObject;
    PDEVICE_OBJECT AttachedDevice;
    IO_REMOVE_LOCK RemoveLock;
    UNICODE_STRING DriveDeviceName;
    UNICODE_STRING BusDeviceName;
    FAST_MUTEX DiskListLock;
    LIST_ENTRY DiskList;
} RAMDISK_EXTENSION, *PRAMDISK_EXTENSION;

typedef struct _RAMDISK_BUS_EXTENSION
{
    RAMDISK_EXTENSION;
} RAMDISK_BUS_EXTENSION, *PRAMDISK_BUS_EXTENSION;

typedef struct _RAMDISK_DRIVE_EXTENSION
{
    /* Inherited base class */
    RAMDISK_EXTENSION;

    /* Data we get from the creator */
    GUID DiskGuid;
    UNICODE_STRING GuidString;
    UNICODE_STRING SymbolicLinkName;
    UNICODE_STRING DeviceObjectName;
    ULONG DiskType;
    RAMDISK_CREATE_OPTIONS DiskOptions;
    LARGE_INTEGER DiskLength;
    LONG DiskOffset;
    WCHAR DriveLetter;
    ULONG BasePage;
    ULONG DiskNumber;

    /* Data we get from the disk */
    ULONG BytesPerSector;
    ULONG SectorsPerTrack;
    ULONG NumberOfHeads;
    ULONG Cylinders;
    ULONG HiddenSectors;
    BOOLEAN VolumeOffline;
    ULONG MountdevLinkCount;

    /* Boot PFN mapping state */
    PVOID BootPfnMappingBase;
    PULONG BootPfnArray;
    SIZE_T BootPfnMappingSize;
    ULONG BootPfnCount;
    ULONG BootPfnTableOffset;
    BOOLEAN BootPfnUsesList;
    BOOLEAN BootPfnInitialized;
    BOOLEAN BootPfnMappingOwned;
    PVOID BootZeroPage;

    /* Two-level PFN mapping state (map pages -> data pages) */
    BOOLEAN BootPfnIsTwoLevel;      /* TRUE if using two-level mapping */
    ULONG BootPfnMapPageCount;      /* Number of map pages */
    PULONG *BootPfnMapPages;        /* Array of pointers to mapped PFN tables */
    SIZE_T *BootPfnMapSizes;        /* Size of each mapped table for cleanup */

    /* MountMgr unique ID change notify support */
    PIRP PendingUniqueIdNotifyIrp;
} RAMDISK_DRIVE_EXTENSION, *PRAMDISK_DRIVE_EXTENSION;

ULONG MaximumViewLength;
ULONG MaximumPerDiskViewLength;
ULONG ReportDetectedDevice;
ULONG MarkRamdisksAsRemovable;
ULONG MinimumViewCount;
LONG RamdiskDiskNumberSeed;
NTSTATUS
NTAPI
RamdiskReadWriteReal(IN PIRP Irp,
                     IN PRAMDISK_DRIVE_EXTENSION DeviceExtension);

static
BOOLEAN
RamdiskEnsureBootPfnTable(IN PRAMDISK_DRIVE_EXTENSION DriveExtension);

static
VOID
RamdiskReleaseBootPfnTable(IN PRAMDISK_DRIVE_EXTENSION DriveExtension);

static
PVOID
RamdiskMapBootPfn(IN PRAMDISK_DRIVE_EXTENSION DeviceExtension,
                  IN LARGE_INTEGER Offset,
                  IN ULONG Length,
                  OUT PULONG OutputLength,
                  OUT PULONG MappingLength);

static
NTSTATUS
RamdiskBuildRegistrySubKey(IN PRAMDISK_DRIVE_EXTENSION DriveExtension,
                           _Out_writes_(BufferChars) PWSTR Buffer,
                           _In_ ULONG BufferChars);

static
VOID
RamdiskPersistDiskState(IN PRAMDISK_DRIVE_EXTENSION DriveExtension);

static
VOID
RamdiskRestoreDiskState(IN PRAMDISK_DRIVE_EXTENSION DriveExtension);

static
ULONGLONG
RamdiskQueryGptAttributes(IN PRAMDISK_DRIVE_EXTENSION DriveExtension);

static
VOID
RamdiskApplyGptAttributes(IN PRAMDISK_DRIVE_EXTENSION DriveExtension,
                          ULONGLONG Attributes);

static
VOID
NTAPI
RamdiskCancelUniqueIdNotify(IN PDEVICE_OBJECT DeviceObject,
                            IN PIRP Irp)
{
    PRAMDISK_DRIVE_EXTENSION DriveExtension = (PRAMDISK_DRIVE_EXTENSION)DeviceObject->DeviceExtension;
    KIRQL CancelIrql = Irp->CancelIrql;

    /* Cancel spin lock is held on entry; clear our pending pointer */
    if (DriveExtension && DriveExtension->PendingUniqueIdNotifyIrp == Irp)
    {
        DriveExtension->PendingUniqueIdNotifyIrp = NULL;
    }

    /* Release the remove lock while cancel spin lock is still held to prevent
     * a race with RamdiskDeleteDiskDevice freeing the extension. */
    if (DriveExtension)
    {
        IoReleaseRemoveLock(&DriveExtension->RemoveLock, Irp);
    }

    IoReleaseCancelSpinLock(CancelIrql);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

static
VOID
RamdiskEnsureRegistryPath(IN PRAMDISK_DRIVE_EXTENSION DriveExtension)
{
    WCHAR KeyBuffer[128];
    NTSTATUS Status;

    RtlCreateRegistryKey(RTL_REGISTRY_SERVICES, L"Ramdisk");
    RtlCreateRegistryKey(RTL_REGISTRY_SERVICES, L"Ramdisk\\Parameters");
    RtlCreateRegistryKey(RTL_REGISTRY_SERVICES, L"Ramdisk\\Parameters\\Disks");

    Status = RamdiskBuildRegistrySubKey(DriveExtension, KeyBuffer, RTL_NUMBER_OF(KeyBuffer));
    if (NT_SUCCESS(Status))
    {
        RtlCreateRegistryKey(RTL_REGISTRY_SERVICES, KeyBuffer);
    }
}

static
BOOLEAN
RamdiskEnsureBootPfnTable(IN PRAMDISK_DRIVE_EXTENSION DriveExtension)
{
    PHYSICAL_ADDRESS TablePhysical;
    PULONG Table;
    ULONGLONG TotalBytes;
    ULONG EntryCount;
    SIZE_T TableBytes;
    SIZE_T MapBytes;
    SIZE_T SpanBytes;
    ULONG SpanWords;
    ULONG Index;
    const ULONG RequiredRun = 4;

    if (DriveExtension->BootPfnInitialized)
    {
        return TRUE;
    }

    DriveExtension->BootPfnMappingBase = NULL;
    DriveExtension->BootPfnArray = NULL;
    DriveExtension->BootPfnMappingSize = 0;
    DriveExtension->BootPfnCount = 0;
    DriveExtension->BootPfnTableOffset = 0;
    DriveExtension->BootPfnUsesList = FALSE;
    DriveExtension->BootPfnMappingOwned = FALSE;
    DriveExtension->BootZeroPage = NULL;
    DriveExtension->BootPfnIsTwoLevel = FALSE;
    DriveExtension->BootPfnMapPageCount = 0;
    DriveExtension->BootPfnMapPages = NULL;
    DriveExtension->BootPfnMapSizes = NULL;

    if (DriveExtension->DiskType != RAMDISK_BOOT_DISK)
    {
        DriveExtension->BootPfnInitialized = TRUE;
        return TRUE;
    }

    /* Writable boot ramdisk (overlay) uses a contiguous allocation built by FreeLdr.
       Avoid PFN table heuristics and map directly from BasePage + DiskOffset. */
    if (!DriveExtension->DiskOptions.Readonly)
    {
        DriveExtension->BootPfnInitialized = TRUE;
        DriveExtension->BootPfnUsesList = FALSE;
        DriveExtension->BootPfnMappingOwned = FALSE;
        DriveExtension->BootPfnArray = NULL;
        DriveExtension->BootPfnCount = 0;
        return TRUE;
    }

    TotalBytes = DriveExtension->DiskOffset + DriveExtension->DiskLength.QuadPart;
    EntryCount = (ULONG)(TotalBytes >> PAGE_SHIFT);
    if (TotalBytes & (PAGE_SIZE - 1)) EntryCount++;
    if (EntryCount == 0) EntryCount = 1;

    TableBytes = (SIZE_T)EntryCount * sizeof(ULONG);
    MapBytes = TableBytes + 0x20000;
    if (MapBytes < TableBytes)
    {
        return FALSE;
    }
    TablePhysical.QuadPart = (ULONGLONG)DriveExtension->BasePage << PAGE_SHIFT;
    SpanBytes = ADDRESS_AND_SIZE_TO_SPAN_PAGES(TablePhysical.QuadPart, MapBytes) << PAGE_SHIFT;
    Table = MmMapIoSpace(TablePhysical, SpanBytes, MmCached);
    if (!Table)
    {
        return FALSE;
    }

    SpanWords = (ULONG)(SpanBytes / sizeof(ULONG));
    DriveExtension->BootPfnMappingBase = Table;
    DriveExtension->BootPfnMappingSize = SpanBytes;
    DriveExtension->BootPfnCount = EntryCount;
    DriveExtension->BootPfnTableOffset = 0;
    DriveExtension->BootPfnMappingOwned = TRUE;

    for (Index = 0; Index + RequiredRun < SpanWords; Index++)
    {
        ULONG Value = Table[Index];
        ULONG Run;

        if (Value == 0)
        {
            continue;
        }

        for (Run = 1; Run < RequiredRun; Run++)
        {
            if (Table[Index + Run] != Value + Run)
            {
                break;
            }
        }

        if (Run == RequiredRun)
        {
            DriveExtension->BootPfnArray = &Table[Index];
            DriveExtension->BootPfnTableOffset = Index;
            DriveExtension->BootPfnUsesList = TRUE;
            break;
        }
    }

    if (!DriveExtension->BootPfnArray)
    {
#if DBG
        DbgPrintEx(DPFLTR_DEFAULT_ID,
                   DPFLTR_ERROR_LEVEL,
                   "RamdiskEnsureBootPfnTable: failed to locate PFN table (entries=%lu)\n",
                   EntryCount);
#endif
        RamdiskReleaseBootPfnTable(DriveExtension);
        return FALSE;
    }

#if DBG
    if (DriveExtension->BootPfnTableOffset >= SpanWords)
    {
        DbgPrintEx(DPFLTR_DEFAULT_ID,
                   DPFLTR_ERROR_LEVEL,
                   "RamdiskEnsureBootPfnTable: PFN offset %lu exceeds span %lu\n",
                   DriveExtension->BootPfnTableOffset,
                   SpanWords);
    }
#endif
    if (DriveExtension->BootPfnTableOffset >= SpanWords)
    {
        RamdiskReleaseBootPfnTable(DriveExtension);
        return FALSE;
    }

    {
        ULONG AvailableEntries = SpanWords - DriveExtension->BootPfnTableOffset;
        if (AvailableEntries < EntryCount)
        {
            SIZE_T TableOffsetBytes = (SIZE_T)DriveExtension->BootPfnTableOffset * sizeof(ULONG);
            SIZE_T GuardBytes = PAGE_SIZE;
            SIZE_T RequiredBytes;
            SIZE_T RequiredSpanBytes;

            if (TableOffsetBytes > (SIZE_T)-1 - TableBytes - GuardBytes)
            {
                RamdiskReleaseBootPfnTable(DriveExtension);
                return FALSE;
            }

            RequiredBytes = TableOffsetBytes + TableBytes + GuardBytes;
            RequiredSpanBytes = ADDRESS_AND_SIZE_TO_SPAN_PAGES(TablePhysical.QuadPart,
                                                               RequiredBytes) << PAGE_SHIFT;

            if (RequiredSpanBytes > SpanBytes)
            {
                PULONG OldTable = Table;
                SIZE_T OldSpanBytes = SpanBytes;
                PULONG NewTable;

                NewTable = MmMapIoSpace(TablePhysical, RequiredSpanBytes, MmCached);
                if (!NewTable)
                {
                    RamdiskReleaseBootPfnTable(DriveExtension);
                    return FALSE;
                }

                MmUnmapIoSpace(OldTable, OldSpanBytes);

                Table = NewTable;
                SpanBytes = RequiredSpanBytes;
                SpanWords = (ULONG)(SpanBytes / sizeof(ULONG));
                DriveExtension->BootPfnMappingBase = Table;
                DriveExtension->BootPfnMappingSize = SpanBytes;
                DriveExtension->BootPfnArray = &Table[DriveExtension->BootPfnTableOffset];
                AvailableEntries = SpanWords - DriveExtension->BootPfnTableOffset;
            }

            if (AvailableEntries < EntryCount)
            {
#if DBG
                DbgPrintEx(DPFLTR_DEFAULT_ID,
                           DPFLTR_ERROR_LEVEL,
                           "RamdiskEnsureBootPfnTable: PFN table truncated (offset=%lu span=%lu entries=%lu required=%lu)\n",
                           DriveExtension->BootPfnTableOffset,
                           SpanWords,
                           AvailableEntries,
                           EntryCount);
#endif
                RamdiskReleaseBootPfnTable(DriveExtension);
                return FALSE;
            }
        }
    }

    if ((DriveExtension->BootPfnArray < Table) ||
        ((ULONG_PTR)(DriveExtension->BootPfnArray - Table) >= SpanWords))
    {
        RamdiskReleaseBootPfnTable(DriveExtension);
        return FALSE;
    }

#if DBG
    DbgPrintEx(DPFLTR_DEFAULT_ID,
               DPFLTR_ERROR_LEVEL,
               "RamdiskEnsureBootPfnTable: tableOffset=%lu span=%lu firstPFN=%lX entries=%lu\n",
               DriveExtension->BootPfnTableOffset,
               SpanWords,
               DriveExtension->BootPfnArray[0],
               DriveExtension->BootPfnCount);
#endif

    /*
     * Detect two-level PFN mapping format:
     * In the new format, BootPfnArray contains map page PFNs (e.g., 0x54, 0x55...)
     * which are much smaller than BasePage. Each map page is a 4KiB table of 1024
     * PFNs pointing to actual data pages. Entries may be separated by 0x0FFFFFFF
     * markers for zero pages.
     */
    {
        const ULONG InvalidPfnMarker = 0x0FFFFFFF;
        const ULONG PfnsPerMapPage = PAGE_SIZE / sizeof(ULONG);  /* 1024 */
        BOOLEAN IsTwoLevel = FALSE;
        ULONG MapPageCount = 0;
        ULONG i;

        /* Check if first PFN is much smaller than BasePage (heuristic for two-level) */
        if (DriveExtension->BootPfnArray[0] != 0 &&
            DriveExtension->BootPfnArray[0] < InvalidPfnMarker &&
            DriveExtension->BootPfnArray[0] < DriveExtension->BasePage)
        {
            IsTwoLevel = TRUE;
        }

        if (IsTwoLevel)
        {
            /* Count map pages (non-zero, non-marker entries) */
            for (i = 0; i < DriveExtension->BootPfnCount && i < 256; i++)
            {
                ULONG Pfn = DriveExtension->BootPfnArray[i];
                if (Pfn != 0 && Pfn < InvalidPfnMarker)
                {
                    MapPageCount++;
                }
            }

            if (MapPageCount == 0)
            {
                IsTwoLevel = FALSE;
            }
        }

        if (IsTwoLevel)
        {
#if DBG
            DbgPrintEx(DPFLTR_DEFAULT_ID,
                       DPFLTR_ERROR_LEVEL,
                       "RamdiskEnsureBootPfnTable: detected two-level mapping, mapPages=%lu\n",
                       MapPageCount);
#endif

            /* Allocate array of pointers to map pages */
            DriveExtension->BootPfnMapPages = ExAllocatePoolWithTag(
                NonPagedPool,
                MapPageCount * sizeof(PULONG),
                'fmRd');
            if (!DriveExtension->BootPfnMapPages)
            {
                RamdiskReleaseBootPfnTable(DriveExtension);
                return FALSE;
            }
            RtlZeroMemory(DriveExtension->BootPfnMapPages, MapPageCount * sizeof(PULONG));

            DriveExtension->BootPfnMapSizes = ExAllocatePoolWithTag(
                NonPagedPool,
                MapPageCount * sizeof(SIZE_T),
                'smRd');
            if (!DriveExtension->BootPfnMapSizes)
            {
                RamdiskReleaseBootPfnTable(DriveExtension);
                return FALSE;
            }
            RtlZeroMemory(DriveExtension->BootPfnMapSizes, MapPageCount * sizeof(SIZE_T));

            /* Map each map page */
            {
                ULONG MapIdx = 0;
                for (i = 0; i < DriveExtension->BootPfnCount && MapIdx < MapPageCount; i++)
                {
                    ULONG MapPfn = DriveExtension->BootPfnArray[i];
                    PHYSICAL_ADDRESS MapPhysical;
                    PULONG MapPage;

                    if (MapPfn == 0 || MapPfn >= InvalidPfnMarker)
                    {
                        continue;
                    }

                    /* Map PFNs are relative to BasePage, just like single-level PFNs */
                    MapPhysical.QuadPart = (ULONGLONG)(DriveExtension->BasePage + MapPfn) << PAGE_SHIFT;
                    MapPage = MmMapIoSpace(MapPhysical, PAGE_SIZE, MmCached);
                    if (!MapPage)
                    {
#if DBG
                        DbgPrintEx(DPFLTR_DEFAULT_ID,
                                   DPFLTR_ERROR_LEVEL,
                                   "RamdiskEnsureBootPfnTable: failed to map page %lu (PFN=%lX)\n",
                                   MapIdx,
                                   MapPfn);
#endif
                        RamdiskReleaseBootPfnTable(DriveExtension);
                        return FALSE;
                    }

                    DriveExtension->BootPfnMapPages[MapIdx] = MapPage;
                    DriveExtension->BootPfnMapSizes[MapIdx] = PAGE_SIZE;
                    MapIdx++;

#if DBG
                    if (MapIdx <= 2)
                    {
                        ULONG j;
                        DbgPrintEx(DPFLTR_DEFAULT_ID,
                                   DPFLTR_ERROR_LEVEL,
                                   "  Map[%lu] PFN=%lX -> first 8 PFNs:\n",
                                   MapIdx - 1,
                                   MapPfn);
                        for (j = 0; j < 8 && j < PAGE_SIZE/sizeof(ULONG); j++)
                        {
                            DbgPrintEx(DPFLTR_DEFAULT_ID,
                                       DPFLTR_ERROR_LEVEL,
                                       "    [%lu]=%lX",
                                       j,
                                       MapPage[j]);
                            if ((j + 1) % 4 == 0)
                                DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "\n");
                        }
                        if (j % 4 != 0)
                            DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "\n");
                    }
#endif
                }
            }

            DriveExtension->BootPfnIsTwoLevel = TRUE;
            DriveExtension->BootPfnMapPageCount = MapPageCount;

            /* Update BootPfnCount to reflect total data pages available */
            DriveExtension->BootPfnCount = MapPageCount * PfnsPerMapPage;
        }
        else
        {
            /* Single-level mapping: BootPfnArray directly contains data page PFNs */
            DriveExtension->BootPfnIsTwoLevel = FALSE;
        }
    }

    DriveExtension->BootPfnInitialized = TRUE;
    return TRUE;
}

static
VOID
RamdiskReleaseBootPfnTable(IN PRAMDISK_DRIVE_EXTENSION DriveExtension)
{
    /* Clean up two-level mapping resources */
    if (DriveExtension->BootPfnMapPages)
    {
        ULONG i;
        for (i = 0; i < DriveExtension->BootPfnMapPageCount; i++)
        {
            if (DriveExtension->BootPfnMapPages[i] &&
                DriveExtension->BootPfnMapSizes &&
                DriveExtension->BootPfnMapSizes[i])
            {
                MmUnmapIoSpace(DriveExtension->BootPfnMapPages[i],
                               DriveExtension->BootPfnMapSizes[i]);
            }
        }
        ExFreePoolWithTag(DriveExtension->BootPfnMapPages, 'fmRd');
        DriveExtension->BootPfnMapPages = NULL;
    }

    if (DriveExtension->BootPfnMapSizes)
    {
        ExFreePoolWithTag(DriveExtension->BootPfnMapSizes, 'smRd');
        DriveExtension->BootPfnMapSizes = NULL;
    }

    /* Clean up single-level mapping resources */
    if (DriveExtension->BootPfnMappingOwned &&
        DriveExtension->BootPfnMappingBase &&
        DriveExtension->BootPfnMappingSize)
    {
        MmUnmapIoSpace(DriveExtension->BootPfnMappingBase,
                       DriveExtension->BootPfnMappingSize);
    }

    if (DriveExtension->BootZeroPage)
    {
        ExFreePoolWithTag(DriveExtension->BootZeroPage, '0dma');
        DriveExtension->BootZeroPage = NULL;
    }

    DriveExtension->BootPfnMappingBase = NULL;
    DriveExtension->BootPfnArray = NULL;
    DriveExtension->BootPfnMappingSize = 0;
    DriveExtension->BootPfnCount = 0;
    DriveExtension->BootPfnTableOffset = 0;
    DriveExtension->BootPfnUsesList = FALSE;
    DriveExtension->BootPfnInitialized = FALSE;
    DriveExtension->BootPfnMappingOwned = FALSE;
    DriveExtension->BootPfnIsTwoLevel = FALSE;
    DriveExtension->BootPfnMapPageCount = 0;
}

static
PVOID
RamdiskMapBootPfn(IN PRAMDISK_DRIVE_EXTENSION DeviceExtension,
                  IN LARGE_INTEGER Offset,
                  IN ULONG Length,
                  OUT PULONG OutputLength,
                  OUT PULONG MappingLength)
{
    const ULONG InvalidPfnMarker = 0x0FFFFFFF;
    ULONGLONG AbsoluteStart;
    ULONGLONG AbsoluteEnd;
    ULONGLONG Remaining;
    ULONG EffectiveLength;
    ULONG PageIndex;
    ULONG PageOffset;
    ULONG BytesToCopy;
    ULONG PfnEntry;
    PHYSICAL_ADDRESS PagePhysical;
    SIZE_T SpanBytes;
    PVOID MappingBase;

    *OutputLength = 0;
    if (MappingLength) *MappingLength = 0;

    AbsoluteStart = DeviceExtension->DiskOffset + Offset.QuadPart;
    AbsoluteEnd = DeviceExtension->DiskOffset + DeviceExtension->DiskLength.QuadPart;
    if (AbsoluteStart >= AbsoluteEnd)
    {
        return NULL;
    }

    Remaining = AbsoluteEnd - AbsoluteStart;
    EffectiveLength = Length;
    if ((ULONGLONG)EffectiveLength > Remaining)
    {
        EffectiveLength = (Remaining > (ULONGLONG)ULONG_MAX) ? ULONG_MAX : (ULONG)Remaining;
    }
    if (EffectiveLength == 0)
    {
        return NULL;
    }

    PageIndex = (ULONG)(AbsoluteStart >> PAGE_SHIFT);
    PageOffset = (ULONG)(AbsoluteStart & (PAGE_SIZE - 1));
    if (PageIndex >= DeviceExtension->BootPfnCount)
    {
        return NULL;
    }

    BytesToCopy = EffectiveLength;
    if (BytesToCopy > (PAGE_SIZE - PageOffset))
    {
        BytesToCopy = PAGE_SIZE - PageOffset;
    }

    /* Get the PFN entry from the table */
    BOOLEAN UseDirectMapping = FALSE;

    if (DeviceExtension->BootPfnIsTwoLevel)
    {
        /* Two-level mapping */
        const ULONG PfnsPerMapPage = PAGE_SIZE / sizeof(ULONG);  /* 1024 */
        ULONG MapIndex = PageIndex / PfnsPerMapPage;
        ULONG EntryIndex = PageIndex % PfnsPerMapPage;

        if (MapIndex >= DeviceExtension->BootPfnMapPageCount)
        {
            return NULL;
        }

        /* Read data page PFN from the appropriate map page */
        PfnEntry = DeviceExtension->BootPfnMapPages[MapIndex][EntryIndex];

#if DBG
        /* Debug: trace first few lookups */
        if (PageIndex < 4)
        {
            DbgPrintEx(DPFLTR_DEFAULT_ID,
                       DPFLTR_ERROR_LEVEL,
                       "RamdiskMapBootPfn: page[%lu] map[%lu][%lu] -> PFN=0x%lX\n",
                       PageIndex,
                       MapIndex,
                       EntryIndex,
                       PfnEntry);

            /* Dump raw bytes from map page for first access */
            if (PageIndex == 0 && MapIndex == 0 && EntryIndex == 0)
            {
                PUCHAR bytes = (PUCHAR)DeviceExtension->BootPfnMapPages[0];
                DbgPrintEx(DPFLTR_DEFAULT_ID,
                           DPFLTR_ERROR_LEVEL,
                           "  Map[0] raw bytes: %02X %02X %02X %02X %02X %02X %02X %02X\n",
                           bytes[0], bytes[1], bytes[2], bytes[3],
                           bytes[4], bytes[5], bytes[6], bytes[7]);
            }
        }
#endif

        /* Check if PFN table is empty (all zeros) - use direct mapping for writable ramdisks */
        if (PfnEntry == 0)
        {
            /* For writable ramdisks, the bootloader doesn't populate the PFN table */
            /* The data is contiguous starting at BasePage */
            UseDirectMapping = TRUE;
            PfnEntry = PageIndex;
#if DBG
            if (PageIndex < 4)
            {
                DbgPrintEx(DPFLTR_DEFAULT_ID,
                           DPFLTR_ERROR_LEVEL,
                           "RamdiskMapBootPfn: PFN=0 detected, using direct mapping for page %lu\n",
                           PageIndex);
            }
#endif
        }
    }
    else
    {
        /* Single-level: direct lookup */
        PfnEntry = DeviceExtension->BootPfnArray[PageIndex];
    }

    /* Check for invalid PFN */
    if (!UseDirectMapping && (PfnEntry == 0 || PfnEntry >= InvalidPfnMarker))
    {
        /* Invalid PFN - return zero page */
        if (!DeviceExtension->BootZeroPage)
        {
            PVOID NewPage = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, '0dma');
            if (!NewPage)
            {
                return NULL;
            }
            RtlZeroMemory(NewPage, PAGE_SIZE);
            if (InterlockedCompareExchangePointer(&DeviceExtension->BootZeroPage,
                                                   NewPage, NULL) != NULL)
            {
                /* Another thread already allocated it */
                ExFreePoolWithTag(NewPage, '0dma');
            }
        }

        *OutputLength = BytesToCopy;
        if (MappingLength) *MappingLength = 0;
        return (PUCHAR)DeviceExtension->BootZeroPage + PageOffset;
    }

    /* Convert PFN to physical address */
    /* All PFNs are relative to BasePage */
    PagePhysical.QuadPart = ((ULONGLONG)DeviceExtension->BasePage +
                              (ULONGLONG)PfnEntry) << PAGE_SHIFT;

#if DBG
    /* Debug: trace physical address calculation for first few pages */
    if (PageIndex < 4)
    {
        DbgPrintEx(DPFLTR_DEFAULT_ID,
                   DPFLTR_ERROR_LEVEL,
                   "  -> PhysAddr=%llX (BasePage=%lX + PFN=%lX)\n",
                   PagePhysical.QuadPart,
                   DeviceExtension->BasePage,
                   PfnEntry);
    }
#endif

    SpanBytes = ADDRESS_AND_SIZE_TO_SPAN_PAGES(PagePhysical.QuadPart + PageOffset,
                                               BytesToCopy) << PAGE_SHIFT;
    if (SpanBytes == 0) SpanBytes = PAGE_SIZE;

    MappingBase = MmMapIoSpace(PagePhysical, SpanBytes, MmCached);
    if (!MappingBase)
    {
        return NULL;
    }

#if DBG
    /* Debug: Check if we're getting the right data for first page */
    if (PageIndex == 0 && PageOffset == 0)
    {
        PUCHAR Data = (PUCHAR)MappingBase;
        DbgPrintEx(DPFLTR_DEFAULT_ID,
                   DPFLTR_ERROR_LEVEL,
                   "  Mapped %llX -> VA %p, first 16 bytes: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",
                   PagePhysical.QuadPart,
                   MappingBase,
                   Data[0], Data[1], Data[2], Data[3], Data[4], Data[5], Data[6], Data[7],
                   Data[8], Data[9], Data[10], Data[11], Data[12], Data[13], Data[14], Data[15]);
    }
#endif

    *OutputLength = BytesToCopy;
    if (MappingLength)
    {
        *MappingLength = (SpanBytes > (SIZE_T)ULONG_MAX) ? ULONG_MAX : (ULONG)SpanBytes;
    }

    return (PUCHAR)MappingBase + PageOffset;
}

static
NTSTATUS
RamdiskBuildPartitionInfo(IN PRAMDISK_DRIVE_EXTENSION DeviceExtension,
                          OUT PPARTITION_INFORMATION PartitionInfo);

static
ULONGLONG
RamdiskQueryGptAttributes(IN PRAMDISK_DRIVE_EXTENSION DriveExtension)
{
    ULONGLONG Attributes = 0;

    if (DriveExtension->DiskOptions.Readonly)
        Attributes |= GPT_BASIC_DATA_ATTRIBUTE_READ_ONLY;

    if (DriveExtension->DiskOptions.Hidden)
        Attributes |= GPT_BASIC_DATA_ATTRIBUTE_HIDDEN;

    if (DriveExtension->DiskOptions.NoDriveLetter)
        Attributes |= GPT_BASIC_DATA_ATTRIBUTE_NO_DRIVE_LETTER;

    return Attributes;
}

static
VOID
RamdiskApplyGptAttributes(IN PRAMDISK_DRIVE_EXTENSION DriveExtension,
                          ULONGLONG Attributes)
{
    DriveExtension->DiskOptions.Readonly = (Attributes & GPT_BASIC_DATA_ATTRIBUTE_READ_ONLY) != 0;
    DriveExtension->DiskOptions.Hidden = (Attributes & GPT_BASIC_DATA_ATTRIBUTE_HIDDEN) != 0;
    DriveExtension->DiskOptions.NoDriveLetter = (Attributes & GPT_BASIC_DATA_ATTRIBUTE_NO_DRIVE_LETTER) != 0;
}

static
VOID
RamdiskLogBufferedReply(
    _In_ PCSTR IoctlName,
    _In_ PVOID Buffer,
    _In_ ULONG OutputLength,
    _In_ ULONG HeaderLength,
    _In_ ULONG DataLength,
    _In_ ULONG RequiredLength,
    _In_ ULONG CopyLength,
    _In_ NTSTATUS Status);

#if DBG
static
VOID
RamdiskAssertIoctlCoverage(VOID);
#endif

static
PCSTR
RamdiskGetIoctlName(ULONG IoControlCode);

static
VOID
RamdiskLogBufferedReply(
    _In_ PCSTR IoctlName,
    _In_ PVOID Buffer,
    _In_ ULONG OutputLength,
    _In_ ULONG HeaderLength,
    _In_ ULONG DataLength,
    _In_ ULONG RequiredLength,
    _In_ ULONG CopyLength,
    _In_ NTSTATUS Status)
{
#if DBG
    DbgPrintEx(DPFLTR_DEFAULT_ID,
               DPFLTR_TRACE_LEVEL,
               "RamdiskDeviceControl[%s]: buf=%p out=%lu hdr=%lu data=%lu copy=%lu req=%lu status=0x%lx\n",
               IoctlName,
               Buffer,
               OutputLength,
               HeaderLength,
               DataLength,
               CopyLength,
               RequiredLength,
               Status);

    ASSERT(RequiredLength >= HeaderLength);
    if (OutputLength >= HeaderLength)
    {
        ASSERT(CopyLength <= DataLength);
    }

    if (OutputLength < HeaderLength)
    {
        ASSERT(Status == STATUS_BUFFER_TOO_SMALL);
    }
    else if (OutputLength < RequiredLength)
    {
        ASSERT(Status == STATUS_BUFFER_OVERFLOW);
    }
    else
    {
        ASSERT(NT_SUCCESS(Status));
    }
#else
    UNREFERENCED_PARAMETER(IoctlName);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(OutputLength);
    UNREFERENCED_PARAMETER(HeaderLength);
    UNREFERENCED_PARAMETER(DataLength);
    UNREFERENCED_PARAMETER(RequiredLength);
    UNREFERENCED_PARAMETER(CopyLength);
    UNREFERENCED_PARAMETER(Status);
#endif
}

#if DBG
static
VOID
RamdiskAssertIoctlCoverage(VOID)
{
    static const ULONG RequiredIoctls[] =
    {
        IOCTL_DISK_GET_DRIVE_GEOMETRY,
        IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
        IOCTL_DISK_GET_PARTITION_INFO,
        IOCTL_DISK_GET_PARTITION_INFO_EX,
        IOCTL_DISK_GET_DRIVE_LAYOUT,
        IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
        IOCTL_DISK_GET_LENGTH_INFO,
        IOCTL_DISK_SET_PARTITION_INFO,
        IOCTL_DISK_IS_WRITABLE,
        IOCTL_STORAGE_QUERY_PROPERTY,
        IOCTL_STORAGE_GET_DEVICE_NUMBER,
        IOCTL_STORAGE_GET_HOTPLUG_INFO,
        IOCTL_VOLUME_GET_GPT_ATTRIBUTES,
        IOCTL_VOLUME_SET_GPT_ATTRIBUTES,
        IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
        IOCTL_VOLUME_QUERY_FAILOVER_SET,
        IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
        IOCTL_MOUNTDEV_QUERY_UNIQUE_ID,
        IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME,
        IOCTL_MOUNTDEV_QUERY_STABLE_GUID,
        IOCTL_MOUNTDEV_LINK_CREATED,
        IOCTL_MOUNTDEV_LINK_DELETED,
        IOCTL_MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY
    };

    for (ULONG Index = 0; Index < RTL_NUMBER_OF(RequiredIoctls); ++Index)
    {
        ULONG Ioctl = RequiredIoctls[Index];
        PCSTR Name = RamdiskGetIoctlName(Ioctl);
        ASSERTMSG("Missing IOCTL coverage in RamdiskGetIoctlName", Name != NULL);
    }
}
#endif /* DBG */

#define IOCTL_ACCESS_MASK (3u << 14)
#define IOCTL_NORMALIZE_ACCESS(code) ((code) & ~IOCTL_ACCESS_MASK)

static
PCSTR
RamdiskGetIoctlName(ULONG IoControlCode)
{
    if (IoControlCode == IOCTL_STORAGE_CHECK_VERIFY2)
    {
        return "IOCTL_STORAGE_CHECK_VERIFY2";
    }

    ULONG Normalized = IOCTL_NORMALIZE_ACCESS(IoControlCode);

    switch (Normalized)
    {
        case IOCTL_NORMALIZE_ACCESS(FSCTL_CREATE_RAM_DISK):
            return "FSCTL_CREATE_RAM_DISK";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_CDROM_CHECK_VERIFY):
            return "IOCTL_CDROM_CHECK_VERIFY";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_CDROM_GET_DRIVE_GEOMETRY):
            return "IOCTL_CDROM_GET_DRIVE_GEOMETRY";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_CDROM_READ_TOC):
            return "IOCTL_CDROM_READ_TOC";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_DISK_CHECK_VERIFY):
            return "IOCTL_DISK_CHECK_VERIFY";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_DISK_GET_DRIVE_GEOMETRY):
            return "IOCTL_DISK_GET_DRIVE_GEOMETRY";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_DISK_GET_DRIVE_GEOMETRY_EX):
            return "IOCTL_DISK_GET_DRIVE_GEOMETRY_EX";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_DISK_GET_DRIVE_LAYOUT):
            return "IOCTL_DISK_GET_DRIVE_LAYOUT";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_DISK_GET_DRIVE_LAYOUT_EX):
            return "IOCTL_DISK_GET_DRIVE_LAYOUT_EX";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_DISK_GET_LENGTH_INFO):
            return "IOCTL_DISK_GET_LENGTH_INFO";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_DISK_GET_MEDIA_TYPES):
            return "IOCTL_DISK_GET_MEDIA_TYPES";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_DISK_GET_PARTITION_INFO):
            return "IOCTL_DISK_GET_PARTITION_INFO";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_DISK_GET_PARTITION_INFO_EX):
            return "IOCTL_DISK_GET_PARTITION_INFO_EX";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_DISK_IS_WRITABLE):
            return "IOCTL_DISK_IS_WRITABLE";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_DISK_SET_PARTITION_INFO):
            return "IOCTL_DISK_SET_PARTITION_INFO";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_MOUNTDEV_QUERY_STABLE_GUID):
            return "IOCTL_MOUNTDEV_QUERY_STABLE_GUID";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME):
            return "IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_MOUNTDEV_LINK_CREATED):
            return "IOCTL_MOUNTDEV_LINK_CREATED";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_MOUNTDEV_LINK_DELETED):
            return "IOCTL_MOUNTDEV_LINK_DELETED";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY):
            return "IOCTL_MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_MOUNTDEV_QUERY_DEVICE_NAME):
            return "IOCTL_MOUNTDEV_QUERY_DEVICE_NAME";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_MOUNTDEV_QUERY_DEVICE_RELATIONS):
            return "IOCTL_MOUNTDEV_QUERY_DEVICE_RELATIONS";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_MOUNTDEV_QUERY_UNIQUE_ID):
            return "IOCTL_MOUNTDEV_QUERY_UNIQUE_ID";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_SCSI_MINIPORT):
            return "IOCTL_SCSI_MINIPORT";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_STORAGE_CHECK_VERIFY):
            return "IOCTL_STORAGE_CHECK_VERIFY";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_STORAGE_GET_MEDIA_TYPES):
            return "IOCTL_STORAGE_GET_MEDIA_TYPES";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_STORAGE_GET_DEVICE_NUMBER):
            return "IOCTL_STORAGE_GET_DEVICE_NUMBER";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_STORAGE_GET_HOTPLUG_INFO):
            return "IOCTL_STORAGE_GET_HOTPLUG_INFO";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_STORAGE_QUERY_PROPERTY):
            return "IOCTL_STORAGE_QUERY_PROPERTY";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_VOLUME_GET_GPT_ATTRIBUTES):
            return "IOCTL_VOLUME_GET_GPT_ATTRIBUTES";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS):
            return "IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_VOLUME_QUERY_FAILOVER_SET):
            return "IOCTL_VOLUME_QUERY_FAILOVER_SET";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_VOLUME_OFFLINE):
            return "IOCTL_VOLUME_OFFLINE";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_VOLUME_ONLINE):
            return "IOCTL_VOLUME_ONLINE";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_VOLUME_SET_GPT_ATTRIBUTES):
            return "IOCTL_VOLUME_SET_GPT_ATTRIBUTES";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_SCSI_PASS_THROUGH):
            return "IOCTL_SCSI_PASS_THROUGH";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_SCSI_PASS_THROUGH_DIRECT):
            return "IOCTL_SCSI_PASS_THROUGH_DIRECT";
        case IOCTL_NORMALIZE_ACCESS(IOCTL_SCSI_GET_ADDRESS):
            return "IOCTL_SCSI_GET_ADDRESS";
        default:
            return NULL;
    }
}
ULONG DefaultViewCount;
ULONG MaximumViewCount;
ULONG MinimumViewLength;
ULONG DefaultViewLength;
UNICODE_STRING DriverRegistryPath;
BOOLEAN ExportBootDiskAsCd;
BOOLEAN IsWinPEBoot;
PDEVICE_OBJECT RamdiskBusFdo;

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
QueryParameters(IN PUNICODE_STRING RegistryPath)
{
    ULONG MinView, DefView, MinViewLength, DefViewLength, MaxViewLength;
    RTL_QUERY_REGISTRY_TABLE QueryTable[11];

    /* Set defaults */
    MaximumViewLength = 0x10000000u;
    MaximumPerDiskViewLength = 0x10000000u;
    ReportDetectedDevice = 0;
    MarkRamdisksAsRemovable = 0;
    MinimumViewCount = 2;
    DefaultViewCount = 16;
    MaximumViewCount = 64;
    MinimumViewLength = 0x10000u;
    DefaultViewLength = 0x100000u;

    /* Setup the query table and query the registry */
    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
    QueryTable[0].Name = L"Parameters";
    QueryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_TYPECHECK;
    QueryTable[1].Name = L"ReportDetectedDevice";
    QueryTable[1].EntryContext = &ReportDetectedDevice;
    QueryTable[1].DefaultType = REG_DWORD;
    QueryTable[2].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_TYPECHECK;
    QueryTable[2].Name = L"MarkRamdisksAsRemovable";
    QueryTable[2].EntryContext = &MarkRamdisksAsRemovable;
    QueryTable[2].DefaultType = REG_DWORD;
    QueryTable[3].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_TYPECHECK;
    QueryTable[3].Name = L"MinimumViewCount";
    QueryTable[3].EntryContext = &MinimumViewCount;
    QueryTable[3].DefaultType = REG_DWORD;
    QueryTable[4].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_TYPECHECK;
    QueryTable[4].Name = L"DefaultViewCount";
    QueryTable[4].EntryContext = &DefaultViewCount;
    QueryTable[4].DefaultType = REG_DWORD;
    QueryTable[5].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_TYPECHECK;
    QueryTable[5].Name = L"MaximumViewCount";
    QueryTable[5].EntryContext = &MaximumViewCount;
    QueryTable[5].DefaultType = REG_DWORD;
    QueryTable[6].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_TYPECHECK;
    QueryTable[6].Name = L"MinimumViewLength";
    QueryTable[6].EntryContext = &MinimumViewLength;
    QueryTable[6].DefaultType = REG_DWORD;
    QueryTable[7].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_TYPECHECK;
    QueryTable[7].Name = L"DefaultViewLength";
    QueryTable[7].EntryContext = &DefaultViewLength;
    QueryTable[7].DefaultType = REG_DWORD;
    QueryTable[8].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_TYPECHECK;
    QueryTable[8].Name = L"MaximumViewLength";
    QueryTable[8].EntryContext = &MaximumViewLength;
    QueryTable[8].DefaultType = REG_DWORD;
    QueryTable[9].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_TYPECHECK;
    QueryTable[9].Name = L"MaximumPerDiskViewLength";
    QueryTable[9].EntryContext = &MaximumPerDiskViewLength;
    QueryTable[9].DefaultType = REG_DWORD;
    RtlQueryRegistryValues(RTL_REGISTRY_OPTIONAL,
                           RegistryPath->Buffer,
                           QueryTable,
                           NULL,
                           NULL);

    /* Parse minimum view count, cannot be bigger than 256 or smaller than 2 */
    MinView = MinimumViewCount;
    if (MinimumViewCount >= 2)
    {
        if (MinimumViewCount > 256) MinView = 256;
    }
    else
    {
        MinView = 2;
    }
    MinimumViewCount = MinView;

    /* Parse default view count, cannot be bigger than 256 or smaller than minimum */
    DefView = DefaultViewCount;
    if (DefaultViewCount >= MinView)
    {
        if (DefaultViewCount > 256) DefView = 256;
    }
    else
    {
        DefView = MinView;
    }
    DefaultViewCount = DefView;

    /* Parse maximum view count, cannot be bigger than 256 or smaller than default */
    if (MaximumViewCount >= DefView)
    {
        if (MaximumViewCount > 256) MaximumViewCount = 256;
    }
    else
    {
        MaximumViewCount = DefView;
    }

    /* Parse minimum view length, cannot be bigger than 1GB or smaller than 64KB */
    MinViewLength = MinimumViewLength;
    if (MinimumViewLength >= 0x10000)
    {
        if (MinimumViewLength > 0x40000000) MinViewLength = 0x40000000u;
    }
    else
    {
        MinViewLength = 0x10000u;
    }
    MinimumViewLength = MinViewLength;

    /* Parse default view length, cannot be bigger than 1GB or smaller than minimum */
    DefViewLength = DefaultViewLength;
    if (DefaultViewLength >= MinViewLength)
    {
        if (DefaultViewLength > 0x40000000) DefViewLength = 0x40000000u;
    }
    else
    {
        DefViewLength = MinViewLength;
    }
    DefaultViewLength = DefViewLength;

    /* Parse maximum view length, cannot be bigger than 1GB or smaller than default */
    MaxViewLength = MaximumViewLength;
    if (MaximumViewLength >= DefViewLength)
    {
        if (MaximumViewLength > 0x40000000) MaxViewLength = 0x40000000u;
    }
    else
    {
        MaxViewLength = DefViewLength;
    }
    MaximumViewLength = MaxViewLength;

    /* Parse maximum view length per disk, cannot be smaller than 16MB */
    if (MaximumPerDiskViewLength >= 0x1000000)
    {
        if (MaxViewLength > 0xFFFFFFFF) MaximumPerDiskViewLength = -1;
    }
    else
    {
        MaximumPerDiskViewLength = 0x1000000u;
    }
}

PVOID
NTAPI
RamdiskMapPages(IN PRAMDISK_DRIVE_EXTENSION DeviceExtension,
                IN LARGE_INTEGER Offset,
                IN ULONG Length,
                OUT PULONG OutputLength,
                OUT PULONG MappingLength)
{
    PHYSICAL_ADDRESS PhysicalAddress;
    PVOID MappedBase;
    ULONG PageOffset;
    SIZE_T ActualLength;
    SIZE_T SpanLength;
    LARGE_INTEGER ActualOffset;
    LARGE_INTEGER ActualPages;
    ULONGLONG DiskLength;
    ULONGLONG RequestOffset;
    ULONG OriginalLength;

    /* For non-boot disks, we need different implementation */
    if (DeviceExtension->DiskType != RAMDISK_BOOT_DISK)
    {
        /* TODO: Implement memory-mapped and registry disk support */
        DPRINT1("RamdiskMapPages: Non-boot disk type %d not yet implemented\n", DeviceExtension->DiskType);
        return NULL;
    }

    /* Default to zero bytes mapped */
    *OutputLength = 0;
    if (MappingLength) *MappingLength = 0;
    SpanLength = 0;

    if (DeviceExtension->DiskType == RAMDISK_BOOT_DISK)
    {
        if (!RamdiskEnsureBootPfnTable(DeviceExtension))
        {
            return NULL;
        }

        if (DeviceExtension->BootPfnUsesList)
        {
            return RamdiskMapBootPfn(DeviceExtension,
                                     Offset,
                                     Length,
                                     OutputLength,
                                     MappingLength);
        }
    }

    if (Offset.QuadPart < 0)
    {
        return NULL;
    }

    DiskLength = DeviceExtension->DiskLength.QuadPart;
    RequestOffset = (ULONGLONG)Offset.QuadPart;

    if (RequestOffset >= DiskLength)
    {
        return NULL;
    }

    OriginalLength = Length;
    if ((ULONGLONG)Length > (DiskLength - RequestOffset))
    {
        ULONGLONG BytesAvailable = DiskLength - RequestOffset;

        if (BytesAvailable > (ULONGLONG)ULONG_MAX)
        {
            Length = ULONG_MAX;
        }
        else
        {
            Length = (ULONG)BytesAvailable;
        }

        if (Length == 0)
        {
            return NULL;
        }

#if DBG
        DbgPrintEx(DPFLTR_DEFAULT_ID,
                   DPFLTR_WARNING_LEVEL,
                   "RamdiskMapPages: trimming request len=%lu to=%lu at offset=%I64u (disk=%I64u)\n",
                   OriginalLength,
                   Length,
                   RequestOffset,
                   DiskLength);
#endif
    }

    /* Calculate the actual offset in the drive */
    ActualOffset.QuadPart = DeviceExtension->DiskOffset + Offset.QuadPart;

    /* Convert to pages */
    ActualPages.QuadPart = ActualOffset.QuadPart >> PAGE_SHIFT;

    /* Now add the base page */
    ActualPages.QuadPart = DeviceExtension->BasePage + ActualPages.QuadPart;

    /* Calculate final amount of bytes */
    PhysicalAddress.QuadPart = ActualPages.QuadPart << PAGE_SHIFT;

    /* Calculate pages spanned for the mapping */
    ActualLength = ADDRESS_AND_SIZE_TO_SPAN_PAGES(ActualOffset.QuadPart, Length);

    /* And convert this back to bytes */
    ActualLength <<= PAGE_SHIFT;
    SpanLength = ActualLength;

    /* Get the offset within the page */
    PageOffset = BYTE_OFFSET(ActualOffset.QuadPart);

    DbgPrintEx(DPFLTR_DEFAULT_ID,
               DPFLTR_TRACE_LEVEL,
               "RamdiskMapPages: basePage=%lu offset=%I64x length=%lu phys=%I64x spanBytes=%Ix\n",
               DeviceExtension->BasePage,
               Offset.QuadPart,
               Length,
               PhysicalAddress.QuadPart,
               ActualLength);

    /* Map the I/O Space from the loader */
#if DBG
    DbgPrintEx(DPFLTR_DEFAULT_ID,
               DPFLTR_TRACE_LEVEL,
               "RamdiskMapPages: len=%lu spanBytes=%Ix phys=%I64x offset=%I64x basePage=%lu\n",
               Length,
               ActualLength,
               PhysicalAddress.QuadPart,
               ActualOffset.QuadPart,
               DeviceExtension->BasePage);
#endif
    MappedBase = MmMapIoSpace(PhysicalAddress, ActualLength, MmCached);

    /* Return actual offset within the page as well as the length */
    if (MappedBase) MappedBase = (PVOID)((ULONG_PTR)MappedBase + PageOffset);

    if (ActualLength > Length)
    {
        ActualLength = Length;
    }

    ASSERT(ActualLength <= Length);
    ASSERT(SpanLength <= MAXULONG);
    *OutputLength = (ULONG)ActualLength;
    if (MappingLength) *MappingLength = (ULONG)SpanLength;
    return MappedBase;
}

VOID
NTAPI
RamdiskUnmapPages(IN PRAMDISK_DRIVE_EXTENSION DeviceExtension,
                  IN PVOID BaseAddress,
                  IN LARGE_INTEGER Offset,
                  IN ULONG MappingLength)
{
    LARGE_INTEGER ActualOffset;
    SIZE_T ActualLength;
    ULONG PageOffset;

    if (DeviceExtension->DiskType == RAMDISK_BOOT_DISK &&
        DeviceExtension->BootPfnUsesList)
    {
        if (!MappingLength)
        {
            return;
        }

        if (!BaseAddress)
        {
            return;
        }

        ActualOffset.QuadPart = DeviceExtension->DiskOffset + Offset.QuadPart;
        PageOffset = BYTE_OFFSET(ActualOffset.QuadPart);

        BaseAddress = (PVOID)((ULONG_PTR)BaseAddress - PageOffset);
        MmUnmapIoSpace(BaseAddress, MappingLength);
        return;
    }

    /* For non-boot disks, we need different implementation */
    if (DeviceExtension->DiskType != RAMDISK_BOOT_DISK)
    {
        /* TODO: Implement memory-mapped and registry disk support */
        DPRINT1("RamdiskUnmapPages: Non-boot disk type %d not yet implemented\n", DeviceExtension->DiskType);
        return;
    }

    /* Calculate the actual offset in the drive */
    ActualOffset.QuadPart = DeviceExtension->DiskOffset + Offset.QuadPart;

    /* Use the original span length supplied by the caller */
    ActualLength = MappingLength;
    if (ActualLength == 0) return;
    ASSERT((ActualLength & (PAGE_SIZE - 1)) == 0);

    /* Get the offset within the page */
    PageOffset = BYTE_OFFSET(ActualOffset.QuadPart);

    /* Calculate actual base address where we mapped this */
    BaseAddress = (PVOID)((ULONG_PTR)BaseAddress - PageOffset);

    /* Unmap the I/O space we got from the loader */
    DbgPrintEx(DPFLTR_DEFAULT_ID,
               DPFLTR_TRACE_LEVEL,
               "RamdiskUnmapPages: basePage=%lu offset=%I64x length=%lu spanBytes=%Ix\n",
               DeviceExtension->BasePage,
               Offset.QuadPart,
               MappingLength,
               ActualLength);
    MmUnmapIoSpace(BaseAddress, ActualLength);
}

static
BOOLEAN
RamdiskBootImageLooksLikeIso(
    _In_ const RAMDISK_CREATE_INPUT *Input)
{
    RAMDISK_DRIVE_EXTENSION ProbeExtension;
    LARGE_INTEGER Offset;
    PVOID BaseAddress = NULL;
    ULONG BytesRead = 0;
    ULONG MapSpan = 0;
    BOOLEAN Result = FALSE;

    DbgPrintEx(DPFLTR_DEFAULT_ID,
               DPFLTR_TRACE_LEVEL,
               "RamdiskBootImageLooksLikeIso: BasePage=%lu Offset=%ld Length=%I64u\n",
               Input->BasePage,
               Input->DiskOffset,
               Input->DiskLength.QuadPart);

    if (!Input->BasePage)
    {
        return FALSE;
    }

    if (Input->DiskLength.QuadPart <= ISO9660_PRIMARY_VOLUME_DESCRIPTOR_OFFSET)
    {
        return FALSE;
    }

    RtlZeroMemory(&ProbeExtension, sizeof(ProbeExtension));
    ProbeExtension.DiskType = Input->DiskType;
    ProbeExtension.BasePage = Input->BasePage;
    ProbeExtension.DiskOffset = Input->DiskOffset;
    ProbeExtension.DiskLength = Input->DiskLength;

    Offset.QuadPart = ISO9660_PRIMARY_VOLUME_DESCRIPTOR_OFFSET;

    BaseAddress = RamdiskMapPages(&ProbeExtension,
                                  Offset,
                                  ISO9660_PROBE_LENGTH,
                                  &BytesRead,
                                  &MapSpan);
    if (BaseAddress && BytesRead >= (ISO9660_SIGNATURE_LENGTH + 2))
    {
        const UCHAR *VolumeDescriptor = BaseAddress;

        if (VolumeDescriptor[0] == ISO9660_PRIMARY_VOLUME_DESCRIPTOR_TYPE &&
            VolumeDescriptor[6] == ISO9660_PRIMARY_VOLUME_DESCRIPTOR_VERSION &&
            RtlCompareMemory(&VolumeDescriptor[1],
                             ISO9660_SIGNATURE,
                             ISO9660_SIGNATURE_LENGTH) == ISO9660_SIGNATURE_LENGTH)
        {
            Result = TRUE;
        }
        else
        {
            DbgPrintEx(DPFLTR_DEFAULT_ID,
                       DPFLTR_TRACE_LEVEL,
                       "RamdiskBootImageLooksLikeIso: signature mismatch type=%02X sig='%c%c%c%c%c' ver=%02X\n",
                       VolumeDescriptor[0],
                       VolumeDescriptor[1],
                       VolumeDescriptor[2],
                       VolumeDescriptor[3],
                       VolumeDescriptor[4],
                       VolumeDescriptor[5],
                       VolumeDescriptor[6]);
        }
    }
    else
    {
        DbgPrintEx(DPFLTR_DEFAULT_ID,
                   DPFLTR_TRACE_LEVEL,
                   "RamdiskBootImageLooksLikeIso: probe failed (mapped=%p bytes=%lu)\n",
                   BaseAddress,
                   BytesRead);
    }

    if (BaseAddress)
    {
        RamdiskUnmapPages(&ProbeExtension, BaseAddress, Offset, MapSpan);
    }

    RamdiskReleaseBootPfnTable(&ProbeExtension);

    return Result;
}

NTSTATUS
NTAPI
RamdiskCreateDiskDevice(IN PRAMDISK_BUS_EXTENSION DeviceExtension,
                        IN PRAMDISK_CREATE_INPUT Input,
                        IN BOOLEAN ValidateOnly,
                        OUT PRAMDISK_DRIVE_EXTENSION *NewDriveExtension)
{
    ULONG BasePage, DiskType, Length;
    //ULONG ViewCount;
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICE_OBJECT DeviceObject;
    PRAMDISK_DRIVE_EXTENSION DriveExtension;
    PVOID Buffer;
    UNICODE_STRING SymbolicLinkName, GuidString, DeviceName;
    PPACKED_BOOT_SECTOR BootSector;
    BIOS_PARAMETER_BLOCK BiosBlock;
    ULONG BytesRead;
    ULONG MapSpan = 0;
    PVOID BaseAddress = NULL;
    LARGE_INTEGER CurrentOffset, CylinderSize, DiskLength;
    ULONG CylinderCount, SizeByCylinders;
    BOOLEAN IsoImage = FALSE;

    CurrentOffset.QuadPart = 0;
    RtlZeroMemory(&SymbolicLinkName, sizeof(SymbolicLinkName));
    RtlZeroMemory(&GuidString, sizeof(GuidString));
    RtlZeroMemory(&DeviceName, sizeof(DeviceName));

    DbgPrintEx(DPFLTR_DEFAULT_ID,
               DPFLTR_INFO_LEVEL,
               "RamdiskCreateDiskDevice: type %lu base %lu length %I64u letter %wc options 0x%08lx basePage=%lu offset=%ld\n",
               Input->DiskType,
               Input->BasePage,
               Input->DiskLength.QuadPart,
               Input->DriveLetter ? Input->DriveLetter : L'-',
               *(PULONG)&Input->Options,
               Input->BasePage,
               Input->DiskOffset);

    /* Check if we're a boot RAM disk */
    DiskType = Input->DiskType;
    DPRINT1("RamdiskCreateDiskDevice: DiskType=%lu ExportAsCd=%u\n",
            DiskType,
            Input->Options.ExportAsCd);
    if (DiskType >= RAMDISK_BOOT_DISK)
    {
        /* Check if we're an ISO */
        if (DiskType == RAMDISK_BOOT_DISK)
        {
            IsoImage = RamdiskBootImageLooksLikeIso(Input);

            DbgPrintEx(DPFLTR_DEFAULT_ID,
                       DPFLTR_TRACE_LEVEL,
                       "RamdiskCreateDiskDevice: boot disk incoming ExportAsCd=%u length=%I64u offset=%ld iso=%u\n",
                       Input->Options.ExportAsCd,
                       Input->DiskLength.QuadPart,
                       Input->DiskOffset,
                       IsoImage);

            if (!Input->Options.ExportAsCd && IsoImage)
            {
                DbgPrintEx(DPFLTR_DEFAULT_ID,
                           DPFLTR_INFO_LEVEL,
                           "RamdiskCreateDiskDevice: detected ISO-9660 image, exporting as CD\n");
                Input->Options.ExportAsCd = TRUE;
            }

            /* NTLDR mounted us somewhere */
            BasePage = Input->BasePage;
            if (!BasePage) return STATUS_INVALID_PARAMETER;

            /* Sanitize disk options */
            Input->Options.Fixed = !Input->Options.ExportAsCd;
            Input->Options.Readonly = Input->Options.ExportAsCd |
                                      Input->Options.Readonly;
            Input->Options.Hidden = FALSE;
            Input->Options.NoDosDevice = FALSE;
            if (Input->DriveLetter == 0 && IsWinPEBoot)
            {
                Input->Options.NoDriveLetter = TRUE;
            }
        }
        else
        {
            /* The only other possibility is a WIM disk */
            if (DiskType != RAMDISK_WIM_DISK)
            {
                /* Fail */
                return STATUS_INVALID_PARAMETER;
            }

            /* Read the view count instead */
            // ViewCount = Input->ViewCount;

            /* Sanitize disk options */
            Input->Options.Hidden = FALSE;
            Input->Options.NoDosDevice = FALSE;
            Input->Options.Readonly = FALSE;
            Input->Options.NoDriveLetter = TRUE;
            Input->Options.Fixed = TRUE;
        }

        /* Are we just validating and returning to the user? */
        if (ValidateOnly) return STATUS_SUCCESS;

        /* Build the GUID string */
        Status = RtlStringFromGUID(&Input->DiskGuid, &GuidString);
        if (!(NT_SUCCESS(Status)) || !(GuidString.Buffer))
        {
            /* Fail */
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto FailCreate;
        }

        /* Allocate our device name */
        Length = GuidString.Length + 32;
        Buffer = ExAllocatePoolWithTag(NonPagedPool, Length, 'dmaR');
        if (!Buffer)
        {
            /* Fail */
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto FailCreate;
        }

        /* Build the device name string */
        DeviceName.Buffer = Buffer;
        DeviceName.Length = Length - 2;
        DeviceName.MaximumLength = Length;
        PWSTR DeviceNameBuffer = Buffer;
        wcsncpy(DeviceNameBuffer, L"\\Device\\Ramdisk", Length / sizeof(WCHAR));
        wcsncat(DeviceNameBuffer, GuidString.Buffer, Length / sizeof(WCHAR));
        DeviceNameBuffer[(Length / sizeof(WCHAR)) - 1] = UNICODE_NULL;

        /* Create the drive device */
        Status = IoCreateDevice(DeviceExtension->DeviceObject->DriverObject,
                                sizeof(RAMDISK_DRIVE_EXTENSION),
                                &DeviceName,
                                (Input->Options.ExportAsCd) ?
                                FILE_DEVICE_CD_ROM : FILE_DEVICE_DISK,
                                0,
                                0,
                                &DeviceObject);
        if (!NT_SUCCESS(Status)) goto FailCreate;

        /* Grab the drive extension */
        DriveExtension = DeviceObject->DeviceExtension;

        if (Input->Options.ExportAsCd)
        {
            DeviceObject->Characteristics |= FILE_READ_ONLY_DEVICE | FILE_REMOVABLE_MEDIA;
        }
        else if (!Input->Options.Fixed)
        {
            DeviceObject->Characteristics |= FILE_REMOVABLE_MEDIA;
        }

        /* Check if we need a DOS device */
        if (!Input->Options.NoDosDevice)
        {
            /* Build the symbolic link name */
            SymbolicLinkName.MaximumLength = GuidString.Length + 36;
            SymbolicLinkName.Length = GuidString.Length + 34;
            Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                           SymbolicLinkName.MaximumLength,
                                           'dmaR');
            SymbolicLinkName.Buffer = Buffer;
            if (Buffer)
            {
                /* Create it */
                wcsncpy(Buffer,
                        L"\\GLOBAL??\\Ramdisk",
                        SymbolicLinkName.MaximumLength / sizeof(WCHAR));
                wcsncat(Buffer,
                        GuidString.Buffer,
                        SymbolicLinkName.MaximumLength / sizeof(WCHAR));
                Status = IoCreateSymbolicLink(&SymbolicLinkName, &DeviceName);
                if (!NT_SUCCESS(Status))
                {
                    /* Nevermind... */
                    Input->Options.NoDosDevice = TRUE;
                    ExFreePool(Buffer);
                    SymbolicLinkName.Buffer = NULL;
                }
            }
            else
            {
                /* No DOS device */
                Input->Options.NoDosDevice = TRUE;
            }

            /* For boot ramdisks, do NOT create a DOS drive-letter link here.
               Let mountmgr assign letters based on our suggested link name.
               We still keep DriveLetter in the extension so the suggestion can use it. */

        }

        /* Setup the device object flags */
        DeviceObject->Flags |= (DO_XIP | DO_POWER_PAGABLE | DO_DIRECT_IO);
        DeviceObject->AlignmentRequirement = 1;

        /* Build the drive FDO */
        *NewDriveExtension = DriveExtension;
        DriveExtension->Type = RamdiskDrive;
        DiskLength = Input->DiskLength;
        ExInitializeFastMutex(&DriveExtension->DiskListLock);
        IoInitializeRemoveLock(&DriveExtension->RemoveLock, 'dmaR', 1, 0);
        DriveExtension->DeviceObjectName = DeviceName;
        RtlZeroMemory(&DriveExtension->DriveDeviceName,
                      sizeof(DriveExtension->DriveDeviceName));
        DriveExtension->SymbolicLinkName = SymbolicLinkName;
        DriveExtension->GuidString = GuidString;
        DriveExtension->DiskGuid = Input->DiskGuid;
        DriveExtension->PhysicalDeviceObject = DeviceObject;
        DriveExtension->DeviceObject = RamdiskBusFdo;
        DriveExtension->AttachedDevice = RamdiskBusFdo;
        DriveExtension->DiskType = Input->DiskType;
        DriveExtension->DiskOptions = Input->Options;
        DriveExtension->DiskLength = DiskLength;
        DriveExtension->DiskOffset = Input->DiskOffset;
        DriveExtension->BasePage = Input->BasePage;
        DriveExtension->DriveLetter = Input->DriveLetter;
        DriveExtension->BytesPerSector = 0;
        DriveExtension->SectorsPerTrack = 0;
        DriveExtension->NumberOfHeads = 0;
        DriveExtension->DiskNumber = (ULONG)(InterlockedIncrement(&RamdiskDiskNumberSeed) - 1);
        DriveExtension->HiddenSectors = 0;
        DriveExtension->VolumeOffline = FALSE;
        DriveExtension->MountdevLinkCount = 0;
        DriveExtension->PendingUniqueIdNotifyIrp = NULL;

        /* Make sure we don't free it later */
        DeviceName.Buffer = NULL;
        SymbolicLinkName.Buffer = NULL;
        GuidString.Buffer = NULL;

        /* Do not create the \ArcName\ramdisk(0) alias here; kernel boot
           code sets it up in IopStartRamdisk and tolerates collisions.
           Creating it here can race and cause STATUS_OBJECT_NAME_COLLISION
           to be raised as fatal there. */

        /* Check if this is a boot disk, or a registry ram drive */
        if (Input->DiskType == RAMDISK_BOOT_DISK)
        {
            if (IsoImage && !DriveExtension->DiskOptions.ExportAsCd)
            {
                DbgPrintEx(DPFLTR_DEFAULT_ID,
                           DPFLTR_ERROR_LEVEL,
                           "RamdiskCreateDiskDevice: enabling ExportAsCd for ISO boot ramdisk\n");
                DriveExtension->DiskOptions.ExportAsCd = TRUE;
                Input->Options.ExportAsCd = TRUE;
            }

            ULONGLONG PartitionStartLba = 0;
            UCHAR PartitionType = 0;
            ULONG PartitionSectorCount = 0;
            BOOLEAN BootSectorValid = FALSE;

            if (!DriveExtension->DiskOptions.ExportAsCd && BaseAddress)
            {
                /* Nothing more to do */
                RamdiskUnmapPages(DriveExtension,
                                  BaseAddress,
                                  CurrentOffset,
                                  MapSpan);
                BaseAddress = NULL;
            }

            if (DriveExtension->DiskOptions.ExportAsCd)
            {
                /* No need to parse FAT boot data when we know it's a CD image */
                goto SkipBootSectorProbe;
            }

            DriveExtension->HiddenSectors = 0;
            CurrentOffset.QuadPart = 0;
            BaseAddress = RamdiskMapPages(DriveExtension,
                                          CurrentOffset,
                                          PAGE_SIZE,
                                          &BytesRead,
                                          &MapSpan);
            if (!BaseAddress)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto FailCreate;
            }

            BootSector = (PPACKED_BOOT_SECTOR)BaseAddress;
            FatUnpackBios(&BiosBlock, &BootSector->PackedBpb);

            if (RamdiskBootSectorHasSignature(BootSector) &&
                (BiosBlock.BytesPerSector >= 512) &&
                (BiosBlock.BytesPerSector <= 4096) &&
                (BiosBlock.SectorsPerTrack != 0) &&
                (BiosBlock.Heads != 0))
            {
                BootSectorValid = TRUE;
            }
            else
            {
                PRAMDISK_MASTER_BOOT_RECORD MasterBootRecord = (PRAMDISK_MASTER_BOOT_RECORD)BaseAddress;
                PUCHAR MbrBytes = (PUCHAR)BaseAddress;

                DbgPrintEx(DPFLTR_DEFAULT_ID,
                           DPFLTR_ERROR_LEVEL,
                           "RamdiskCreateDiskDevice: MBR check - Magic=0x%04X, bytes at 0x1FE: %02X %02X\n",
                           MasterBootRecord->Magic,
                           MbrBytes[0x1FE], MbrBytes[0x1FF]);

                if (MasterBootRecord->Magic == 0xAA55)
                {
                    DbgPrintEx(DPFLTR_DEFAULT_ID,
                               DPFLTR_ERROR_LEVEL,
                               "RamdiskCreateDiskDevice: MBR magic matched, examining partitions\n");
                    for (ULONG PartitionIndex = 0;
                         PartitionIndex < RTL_NUMBER_OF(MasterBootRecord->PartitionTable);
                         ++PartitionIndex)
                    {
                        const RAMDISK_MBR_PARTITION_ENTRY *Entry = &MasterBootRecord->PartitionTable[PartitionIndex];

                        if (Entry->SystemIndicator != 0)
                        {
                            DbgPrintEx(DPFLTR_DEFAULT_ID,
                                       DPFLTR_ERROR_LEVEL,
                                       "  MBR[%lu]: type=0x%02X start=%lu count=%lu\n",
                                       PartitionIndex,
                                       Entry->SystemIndicator,
                                       Entry->SectorCountBeforePartition,
                                       Entry->PartitionSectorCount);
                        }

                        if ((Entry->PartitionSectorCount != 0) &&
                            (Entry->SystemIndicator != 0))
                        {
                            PartitionStartLba = Entry->SectorCountBeforePartition;
                            PartitionType = Entry->SystemIndicator;
                            PartitionSectorCount = Entry->PartitionSectorCount;
                            break;
                        }
                    }
                }
                else
                {
                    DbgPrintEx(DPFLTR_DEFAULT_ID,
                               DPFLTR_ERROR_LEVEL,
                               "RamdiskCreateDiskDevice: MBR magic mismatch (value=0x%04X)\n",
                               MasterBootRecord->Magic);
                }

                                if ((PartitionType == PARTITION_ISO9660) &&
                    !DriveExtension->DiskOptions.ExportAsCd)
                {
                    /* Probe for a true ISO9660 volume by checking for the
                     * Primary Volume Descriptor signature "CD001" at LBA 16.
                     * If not present, avoid forcing ExportAsCd=TRUE so FAT-based
                     * live images with a nonstandard type aren't misclassified. */
                    BOOLEAN IsIso = FALSE;
                    ULONGLONG ProbeOffset = (ULONGLONG)PartitionStartLba * 512ULL + (ULONGLONG)16 * 2048ULL;
                    ULONG ProbeSpan = 0;
                    PVOID ProbeBase = RamdiskMapPages(DriveExtension,
                                                     *(PLARGE_INTEGER)&ProbeOffset,
                                                     2048,
                                                     &BytesRead,
                                                     &ProbeSpan);
                    if (ProbeBase && BytesRead >= 6)
                    {
                        const UCHAR *p = (const UCHAR *)ProbeBase;
                        if (p[1] == 'C' && p[2] == 'D' && p[3] == '0' && p[4] == '0' && p[5] == '1')
                        {
                            IsIso = TRUE;
                        }
                    }
                    if (ProbeBase)
                    {
                        RamdiskUnmapPages(DriveExtension,
                                          ProbeBase,
                                          *(PLARGE_INTEGER)&ProbeOffset,
                                          ProbeSpan);
                    }

                    if (IsIso)
                    {
                        DbgPrintEx(DPFLTR_DEFAULT_ID,
                                   DPFLTR_ERROR_LEVEL,
                                   "RamdiskCreateDiskDevice: detected El-Torito ISO9660 (0x96) -> exporting as CD (startLba=%I64u len=%lu)\n",
                                   PartitionStartLba,
                                   PartitionSectorCount);
                        DriveExtension->DiskOptions.ExportAsCd = TRUE;
                        Input->Options.ExportAsCd = TRUE;
                    }
                    else
                    {
                        DbgPrintEx(DPFLTR_DEFAULT_ID,
                                   DPFLTR_ERROR_LEVEL,
                                   "RamdiskCreateDiskDevice: 0x96 partition does not look like ISO (no CD001 at LBA16); keeping disk semantics\n");
                    }
                }

                if (PartitionStartLba != 0)
                {
                    LARGE_INTEGER BootOffset;

                    BootOffset.QuadPart = (ULONGLONG)PartitionStartLba * 512ULL;

                    RamdiskUnmapPages(DriveExtension,
                                      BaseAddress,
                                      CurrentOffset,
                                      MapSpan);

                    CurrentOffset = BootOffset;
                    BaseAddress = RamdiskMapPages(DriveExtension,
                                                  CurrentOffset,
                                                  PAGE_SIZE,
                                                  &BytesRead,
                                                  &MapSpan);
                    if (!BaseAddress)
                    {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto FailCreate;
                    }

                    BootSector = (PPACKED_BOOT_SECTOR)BaseAddress;
                    FatUnpackBios(&BiosBlock, &BootSector->PackedBpb);

                    if (RamdiskBootSectorHasSignature(BootSector) &&
                        (BiosBlock.BytesPerSector >= 512) &&
                        (BiosBlock.BytesPerSector <= 4096) &&
                        (BiosBlock.SectorsPerTrack != 0) &&
                        (BiosBlock.Heads != 0))
                    {
                        BootSectorValid = TRUE;
                        DriveExtension->HiddenSectors = (ULONG)PartitionStartLba;
                    }
                }
            }

            if (BootSectorValid)
            {
                DriveExtension->BytesPerSector = BiosBlock.BytesPerSector;
                DriveExtension->SectorsPerTrack = BiosBlock.SectorsPerTrack;
                DriveExtension->NumberOfHeads = BiosBlock.Heads;

                /* Only overwrite HiddenSectors from BPB if it matches the MBR-derived value,
                   or if we didn't find an MBR partition table */
                if (PartitionStartLba == 0 || BiosBlock.HiddenSectors == (ULONG)PartitionStartLba)
                {
                    DriveExtension->HiddenSectors = BiosBlock.HiddenSectors;
                }
                /* Otherwise keep the MBR-derived value already set in DriveExtension->HiddenSectors */
            }

SkipBootSectorProbe:
            if (BaseAddress)
            {
                RamdiskUnmapPages(DriveExtension,
                                  BaseAddress,
                                  CurrentOffset,
                                  MapSpan);
            }
        }

        if (DriveExtension->DiskOptions.ExportAsCd)
        {
            /* Force ISO defaults regardless of what the boot sector reported */
            DriveExtension->BytesPerSector = 2048;
            DriveExtension->SectorsPerTrack = 32;
            DriveExtension->NumberOfHeads = 64;
        }
        else if ((DriveExtension->BytesPerSector == 0) ||
                 (DriveExtension->SectorsPerTrack == 0) ||
                 (DriveExtension->NumberOfHeads == 0))
        {
            /* Setup partition parameters default for FAT */
            DriveExtension->BytesPerSector = 512;
            DriveExtension->SectorsPerTrack = 128;
            DriveExtension->NumberOfHeads = 16;
        }

        DPRINT1("RamdiskCreateDiskDevice: geometry BPS=%lu SPT=%lu Heads=%lu ExportAsCd=%u HiddenSectors=%lu\n",
                DriveExtension->BytesPerSector,
                DriveExtension->SectorsPerTrack,
                DriveExtension->NumberOfHeads,
                Input->Options.ExportAsCd,
                DriveExtension->HiddenSectors);

        if (DriveExtension->BytesPerSector > 0)
        {
            DeviceObject->AlignmentRequirement = (ULONG)(DriveExtension->BytesPerSector - 1);
        }

        /* Calculate the cylinder size */
        CylinderSize.QuadPart = DriveExtension->BytesPerSector *
                                DriveExtension->SectorsPerTrack *
                                DriveExtension->NumberOfHeads;
        CylinderCount = DiskLength.QuadPart / CylinderSize.QuadPart;
        SizeByCylinders = CylinderSize.QuadPart * CylinderCount;
        DriveExtension->Cylinders = CylinderCount;
        if ((DiskLength.HighPart > 0) || (SizeByCylinders < DiskLength.LowPart))
        {
            /* Align cylinder size up */
            DriveExtension->Cylinders++;
        }

        /* Acquire the disk lock */
        KeEnterCriticalRegion();
        ExAcquireFastMutex(&DeviceExtension->DiskListLock);

        /* Insert us */
        InsertTailList(&DeviceExtension->DiskList, &DriveExtension->DiskList);

        /* Release the lock */
        ExReleaseFastMutex(&DeviceExtension->DiskListLock);
        KeLeaveCriticalRegion();

        if (DriveExtension->DiskOptions.ExportAsCd)
        {
            PARTITION_INFORMATION DebugPartition;
            NTSTATUS partStatus = RamdiskBuildPartitionInfo(DriveExtension, &DebugPartition);
            if (NT_SUCCESS(partStatus))
            {
                DbgPrintEx(DPFLTR_DEFAULT_ID,
                           DPFLTR_ERROR_LEVEL,
                           "RamdiskCreateDiskDevice: partition start=%I64u length=%I64u type=0x%02X\n",
                           DebugPartition.StartingOffset.QuadPart,
                           DebugPartition.PartitionLength.QuadPart,
                           DebugPartition.PartitionType);
            }
            else
            {
                DbgPrintEx(DPFLTR_DEFAULT_ID,
                           DPFLTR_ERROR_LEVEL,
                           "RamdiskCreateDiskDevice: partition info unavailable status=0x%08X\n",
                           partStatus);
            }
        }

        /* Load any persisted state (must occur after list insertion) */
        RamdiskRestoreDiskState(DriveExtension);
        RamdiskPersistDiskState(DriveExtension);

        /* Clear init flag */
        DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
        DbgPrintEx(DPFLTR_DEFAULT_ID,
                   DPFLTR_TRACE_LEVEL,
                   "RamdiskCreateDiskDevice: GUID %wZ assigned drive %wc\n",
                   &DriveExtension->GuidString,
                   DriveExtension->DriveLetter ? DriveExtension->DriveLetter : L'-');
        return STATUS_SUCCESS;
    }

FailCreate:
    DbgPrintEx(DPFLTR_DEFAULT_ID,
               DPFLTR_WARNING_LEVEL,
               "RamdiskCreateDiskDevice: failing create with status 0x%lx\n",
               Status);
    return Status;
}

NTSTATUS
NTAPI
RamdiskCreateRamdisk(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp,
                     IN BOOLEAN ValidateOnly)
{
    PRAMDISK_CREATE_INPUT Input;
    ULONG Length;
    PRAMDISK_BUS_EXTENSION DeviceExtension;
    PRAMDISK_DRIVE_EXTENSION DriveExtension;
    ULONG DiskType;
    PWCHAR FileNameStart, FileNameEnd;
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    /* Get the device extension and our input data */
    DeviceExtension = DeviceObject->DeviceExtension;
    Length = IoStackLocation->Parameters.DeviceIoControl.InputBufferLength;
    Input = (PRAMDISK_CREATE_INPUT)Irp->AssociatedIrp.SystemBuffer;

    /* Validate input parameters */
    if ((Length < sizeof(RAMDISK_CREATE_INPUT)) ||
        (Input->Version != sizeof(RAMDISK_CREATE_INPUT)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    DbgPrintEx(DPFLTR_DEFAULT_ID,
               DPFLTR_TRACE_LEVEL,
               "RamdiskCreateRamdisk: create request type %lu length %lu letter %wc options 0x%08lx\n",
               Input->DiskType,
               Length,
               Input->DriveLetter ? Input->DriveLetter : L'-',
               *(PULONG)&Input->Options);

    /* Validate the disk type */
    DiskType = Input->DiskType;
    if (DiskType == RAMDISK_WIM_DISK) return STATUS_INVALID_PARAMETER;

    /* Look at the disk type */
    if (DiskType == RAMDISK_BOOT_DISK)
    {
        /* We only allow this as an early-init boot */
        if (!KeLoaderBlock) return STATUS_INVALID_PARAMETER;

        /* Save command-line flags */
        if (ExportBootDiskAsCd) Input->Options.ExportAsCd = TRUE;
        if (Input->DriveLetter == 0 && IsWinPEBoot)
            Input->Options.NoDriveLetter = TRUE;
    }

    /* Validate the disk type */
    if ((Input->Options.ExportAsCd) && (DiskType != RAMDISK_BOOT_DISK))
    {
        /* If the type isn't CDFS, it has to at least be raw CD */
        if (DiskType != RAMDISK_MEMORY_MAPPED_DISK) return STATUS_INVALID_PARAMETER;
    }

    /* Check if this is an actual file */
    if (DiskType <= RAMDISK_MEMORY_MAPPED_DISK)
    {
        /* Validate the file name */
        FileNameStart = (PWCHAR)((ULONG_PTR)Input + Length);
        FileNameEnd = Input->FileName + 1;
        while ((FileNameEnd < FileNameStart) && *(FileNameEnd)) FileNameEnd++;
        if (FileNameEnd == FileNameStart) return STATUS_INVALID_PARAMETER;
    }

    /* Create the actual device */
    Status = RamdiskCreateDiskDevice(DeviceExtension,
                                     Input,
                                     ValidateOnly,
                                     &DriveExtension);
    if (NT_SUCCESS(Status))
    {
        /* Invalidate and set success */
        IoInvalidateDeviceRelations(DeviceExtension->PhysicalDeviceObject, 0);
        Irp->IoStatus.Information = 0;
        DbgPrintEx(DPFLTR_DEFAULT_ID,
                   DPFLTR_TRACE_LEVEL,
                   "RamdiskCreateRamdisk: device creation succeeded\n");
    }

    /* We are done */
    return Status;
}

static
NTSTATUS
RamdiskBuildPartitionInfo(IN PRAMDISK_DRIVE_EXTENSION DeviceExtension,
                          OUT PPARTITION_INFORMATION PartitionInfo)
{
    LARGE_INTEGER Zero = {{0, 0}};
    PVOID BaseAddress;
    ULONG BytesRead;
    ULONG MapSpan;

    RtlZeroMemory(PartitionInfo, sizeof(*PartitionInfo));

    BaseAddress = RamdiskMapPages(DeviceExtension, Zero, PAGE_SIZE, &BytesRead, &MapSpan);
    if (BaseAddress == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    PartitionInfo->PartitionNumber = 1;
    PartitionInfo->RewritePartition = FALSE;

    if (BytesRead >= 0x200 && DeviceExtension->BytesPerSector != 0)
    {
        USHORT Signature = *((PUSHORT)((PUCHAR)BaseAddress + 0x1FE));
        if (Signature == 0xAA55)
        {
            PUCHAR Entry = (PUCHAR)BaseAddress + 0x1BE;
            UCHAR BootIndicator = Entry[0];
            UCHAR Type = Entry[4];
            ULONG StartLba = *(PULONG)(Entry + 8);
            ULONG SectorCount = *(PULONG)(Entry + 12);

            if (Type != 0 && SectorCount != 0)
            {
                ULONG effectiveSectorSize = DeviceExtension->BytesPerSector;

                if (DeviceExtension->DiskOptions.ExportAsCd && Type == 0x96)
                {
                    effectiveSectorSize = 512;
                }

                PartitionInfo->StartingOffset.QuadPart = ((ULONGLONG)StartLba) * effectiveSectorSize;
                PartitionInfo->PartitionLength.QuadPart = ((ULONGLONG)SectorCount) * effectiveSectorSize;
                PartitionInfo->HiddenSectors = StartLba;
                PartitionInfo->PartitionType = Type;
                PartitionInfo->BootIndicator = (BootIndicator == 0x80) ? TRUE : FALSE;
                PartitionInfo->RecognizedPartition = IsRecognizedPartition(Type);

                if (DeviceExtension->DiskOptions.ExportAsCd)
                {
                    PartitionInfo->StartingOffset.QuadPart = 0;
                    PartitionInfo->PartitionLength = DeviceExtension->DiskLength;
                    PartitionInfo->HiddenSectors = 0;
                    PartitionInfo->PartitionType = PARTITION_ISO9660;
                    PartitionInfo->BootIndicator = TRUE;
                    PartitionInfo->RecognizedPartition = TRUE;
                }

                DPRINT1("RamdiskBuildPartitionInfo: type=0x%02X LBA=%lu sectors=%lu effBPS=%lu start=%I64u len=%I64u\n",
                        Type,
                        StartLba,
                        SectorCount,
                        effectiveSectorSize,
                        PartitionInfo->StartingOffset.QuadPart,
                        PartitionInfo->PartitionLength.QuadPart);

                RamdiskUnmapPages(DeviceExtension, BaseAddress, Zero, MapSpan);
                return STATUS_SUCCESS;
            }
        }
    }

    PartitionInfo->StartingOffset.QuadPart = 0;
    PartitionInfo->PartitionLength = DeviceExtension->DiskLength;
    PartitionInfo->HiddenSectors = 0;
    if (DeviceExtension->DiskOptions.ExportAsCd)
    {
        PartitionInfo->PartitionType = PARTITION_ISO9660;
        PartitionInfo->BootIndicator = TRUE;
        PartitionInfo->RecognizedPartition = TRUE;
    }
    else
    {
        PartitionInfo->PartitionType = PARTITION_IFS;
        PartitionInfo->BootIndicator = (DeviceExtension->DiskType == RAMDISK_BOOT_DISK) ? TRUE : FALSE;
        PartitionInfo->RecognizedPartition = IsRecognizedPartition(PartitionInfo->PartitionType);
    }

    RamdiskUnmapPages(DeviceExtension, BaseAddress, Zero, MapSpan);
    return STATUS_SUCCESS;
}

static
NTSTATUS
RamdiskBuildPartitionInfoEx(IN PRAMDISK_DRIVE_EXTENSION DeviceExtension,
                            OUT PPARTITION_INFORMATION_EX PartitionInfoEx)
{
    PARTITION_INFORMATION LegacyInfo;
    NTSTATUS Status;

    Status = RamdiskBuildPartitionInfo(DeviceExtension, &LegacyInfo);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    RtlZeroMemory(PartitionInfoEx, sizeof(*PartitionInfoEx));
    PartitionInfoEx->PartitionStyle = PARTITION_STYLE_MBR;
    PartitionInfoEx->StartingOffset = LegacyInfo.StartingOffset;
    PartitionInfoEx->PartitionLength = LegacyInfo.PartitionLength;
    PartitionInfoEx->PartitionNumber = LegacyInfo.PartitionNumber;
    PartitionInfoEx->RewritePartition = LegacyInfo.RewritePartition;
    if (DeviceExtension->DiskOptions.ExportAsCd)
    {
        PartitionInfoEx->Mbr.PartitionType = PARTITION_ISO9660;
        PartitionInfoEx->Mbr.BootIndicator = TRUE;
        PartitionInfoEx->Mbr.RecognizedPartition = TRUE;
        PartitionInfoEx->Mbr.HiddenSectors = 0;
    }
    else
    {
        PartitionInfoEx->Mbr.PartitionType = LegacyInfo.PartitionType;
        PartitionInfoEx->Mbr.BootIndicator = LegacyInfo.BootIndicator;
        PartitionInfoEx->Mbr.RecognizedPartition = LegacyInfo.RecognizedPartition;
        PartitionInfoEx->Mbr.HiddenSectors = LegacyInfo.HiddenSectors;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RamdiskGetPartitionInfo(IN PIRP Irp,
                        IN PRAMDISK_DRIVE_EXTENSION DeviceExtension)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStackLocation;
    PPARTITION_INFORMATION PartitionInfo;

    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    if (IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength < sizeof(PARTITION_INFORMATION))
    {
        Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
        Irp->IoStatus.Information = 0;
        return STATUS_BUFFER_TOO_SMALL;
    }

    PartitionInfo = Irp->AssociatedIrp.SystemBuffer;
    Status = RamdiskBuildPartitionInfo(DeviceExtension, PartitionInfo);
    if (NT_SUCCESS(Status))
    {
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = sizeof(PARTITION_INFORMATION);
    }
    else
    {
        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = 0;
    }

    return Status;
}

NTSTATUS
NTAPI
RamdiskGetPartitionInfoEx(IN PIRP Irp,
                          IN PRAMDISK_DRIVE_EXTENSION DeviceExtension)
{
    PIO_STACK_LOCATION IoStackLocation;
    PPARTITION_INFORMATION_EX PartitionInfo;
    NTSTATUS Status;

    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    if (IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength < sizeof(PARTITION_INFORMATION_EX))
    {
        Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
        Irp->IoStatus.Information = 0;
        return STATUS_BUFFER_TOO_SMALL;
    }

    PartitionInfo = Irp->AssociatedIrp.SystemBuffer;
    Status = RamdiskBuildPartitionInfoEx(DeviceExtension, PartitionInfo);
    if (NT_SUCCESS(Status))
    {
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = sizeof(PARTITION_INFORMATION_EX);
    }
    else
    {
        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = 0;
    }

    return Status;
}

NTSTATUS
NTAPI
RamdiskSetPartitionInfo(IN PIRP Irp,
                        IN PRAMDISK_DRIVE_EXTENSION DeviceExtension)
{
    ULONG BytesRead;
    ULONG MapSpan;
    NTSTATUS Status;
    PVOID BaseAddress;
    PIO_STACK_LOCATION Stack;
    LARGE_INTEGER Zero = {{0, 0}};
    PPARTITION_INFORMATION PartitionInfo;

    /* First validate input */
    Stack = IoGetCurrentIrpStackLocation(Irp);
    if (Stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(PARTITION_INFORMATION))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto SetAndQuit;
    }

    /* Map to get MBR */
    BaseAddress = RamdiskMapPages(DeviceExtension, Zero, PAGE_SIZE, &BytesRead, &MapSpan);
    if (BaseAddress == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto SetAndQuit;
    }

    /* Set the new partition type on partition 0, field system indicator */
    PartitionInfo = (PPARTITION_INFORMATION)Irp->AssociatedIrp.SystemBuffer;
    *((PCHAR)BaseAddress + 450) = PartitionInfo->PartitionType;

    /* And unmap */
    RamdiskUnmapPages(DeviceExtension, BaseAddress, Zero, MapSpan);
    Status = STATUS_SUCCESS;

SetAndQuit:
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    return Status;
}

static
NTSTATUS
RamdiskHandleStorageQueryProperty(IN PRAMDISK_DRIVE_EXTENSION DeviceExtension,
                                  IN PIRP Irp,
                                  IN PIO_STACK_LOCATION IoStackLocation,
                                  OUT PULONG Information)
{
    PSTORAGE_PROPERTY_QUERY Query;
    ULONG OutputLength;

    if (IoStackLocation->Parameters.DeviceIoControl.InputBufferLength < sizeof(STORAGE_PROPERTY_QUERY))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Query = Irp->AssociatedIrp.SystemBuffer;
    if (!Query)
    {
        return STATUS_INVALID_PARAMETER;
    }
    OutputLength = IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength;

    switch (Query->PropertyId)
    {
        case StorageDeviceProperty:
        {
            switch (Query->QueryType)
            {
                case PropertyStandardQuery:
                {
                    PSTORAGE_DEVICE_DESCRIPTOR Descriptor;

                    if (OutputLength < sizeof(STORAGE_DESCRIPTOR_HEADER))
                    {
                        *Information = sizeof(STORAGE_DESCRIPTOR_HEADER);
                        return STATUS_BUFFER_TOO_SMALL;
                    }

                    if (OutputLength < sizeof(STORAGE_DEVICE_DESCRIPTOR))
                    {
                        *Information = sizeof(STORAGE_DEVICE_DESCRIPTOR);
                        return STATUS_BUFFER_TOO_SMALL;
                    }

                    Descriptor = Irp->AssociatedIrp.SystemBuffer;
                    RtlZeroMemory(Descriptor, sizeof(STORAGE_DEVICE_DESCRIPTOR));
                    Descriptor->Version = sizeof(STORAGE_DEVICE_DESCRIPTOR);
                    Descriptor->Size = sizeof(STORAGE_DEVICE_DESCRIPTOR);
                    Descriptor->DeviceType = DeviceExtension->DiskOptions.ExportAsCd ? READ_ONLY_DIRECT_ACCESS_DEVICE : DIRECT_ACCESS_DEVICE;
                    Descriptor->RemovableMedia = !DeviceExtension->DiskOptions.Fixed;
                    Descriptor->CommandQueueing = FALSE;
                    Descriptor->BusType = BusTypeVirtual;
                    Descriptor->RawPropertiesLength = 0;

                    *Information = sizeof(STORAGE_DEVICE_DESCRIPTOR);
                    return STATUS_SUCCESS;
                }

                case PropertyExistsQuery:
                {
                    *Information = 0;
                    return STATUS_SUCCESS;
                }

                default:
                    return STATUS_NOT_SUPPORTED;
            }
        }

        case StorageAccessAlignmentProperty:
        {
            switch (Query->QueryType)
            {
                case PropertyStandardQuery:
                {
                    PSTORAGE_ACCESS_ALIGNMENT_DESCRIPTOR Alignment;

                    if (OutputLength < sizeof(STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR))
                    {
                        *Information = sizeof(STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR);
                        return STATUS_BUFFER_TOO_SMALL;
                    }

                    Alignment = Irp->AssociatedIrp.SystemBuffer;
                    RtlZeroMemory(Alignment, sizeof(STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR));
                    Alignment->Version = sizeof(STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR);
                    Alignment->Size = sizeof(STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR);
                    Alignment->BytesPerCacheLine = 0;
                    Alignment->BytesOffsetForCacheAlignment = 0;
                    Alignment->BytesPerLogicalSector = DeviceExtension->BytesPerSector;
                    Alignment->BytesPerPhysicalSector = DeviceExtension->BytesPerSector;
                    Alignment->BytesOffsetForSectorAlignment = 0;

                    *Information = sizeof(STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR);
                    return STATUS_SUCCESS;
                }

                case PropertyExistsQuery:
                {
                    *Information = 0;
                    return STATUS_SUCCESS;
                }

                default:
                    return STATUS_NOT_SUPPORTED;
            }
        }

        default:
            return STATUS_NOT_SUPPORTED;
    }
}

static
NTSTATUS
RamdiskBuildRegistrySubKey(IN PRAMDISK_DRIVE_EXTENSION DriveExtension,
                           _Out_writes_(BufferChars) PWSTR Buffer,
                           _In_ ULONG BufferChars)
{
    INT Count;

    if (BufferChars == 0)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    Count = _snwprintf(Buffer,
                       BufferChars,
                       L"Ramdisk\\Parameters\\Disks\\%wZ",
                       &DriveExtension->GuidString);

    if (Count < 0 || (ULONG)Count >= BufferChars)
    {
        Buffer[BufferChars - 1] = UNICODE_NULL;
        return STATUS_BUFFER_TOO_SMALL;
    }

    Buffer[Count] = UNICODE_NULL;
    return STATUS_SUCCESS;
}

static
VOID
RamdiskPersistDiskState(IN PRAMDISK_DRIVE_EXTENSION DriveExtension)
{
    WCHAR SubKey[128];
    NTSTATUS Status;
    ULONG LinkCount;

    RamdiskEnsureRegistryPath(DriveExtension);

    Status = RamdiskBuildRegistrySubKey(DriveExtension, SubKey, RTL_NUMBER_OF(SubKey));
    if (!NT_SUCCESS(Status))
    {
        return;
    }

    LinkCount = DriveExtension->MountdevLinkCount;
    RtlWriteRegistryValue(RTL_REGISTRY_SERVICES,
                          SubKey,
                          L"MountdevLinkCount",
                          REG_DWORD,
                          &LinkCount,
                          sizeof(LinkCount));

    if (DriveExtension->GuidString.Buffer != NULL)
    {
        RtlWriteRegistryValue(RTL_REGISTRY_SERVICES,
                              SubKey,
                              L"DiskGuid",
                              REG_BINARY,
                              &DriveExtension->DiskGuid,
                              sizeof(GUID));
    }

    if (DriveExtension->DriveLetter != 0)
    {
        ULONG Letter = (ULONG)DriveExtension->DriveLetter;
        RtlWriteRegistryValue(RTL_REGISTRY_SERVICES,
                              SubKey,
                              L"DriveLetter",
                              REG_DWORD,
                              &Letter,
                              sizeof(Letter));
    }

    {
        ULONGLONG Attributes = RamdiskQueryGptAttributes(DriveExtension);
        RtlWriteRegistryValue(RTL_REGISTRY_SERVICES,
                              SubKey,
                              L"GptAttributes",
                              REG_QWORD,
                              &Attributes,
                              sizeof(Attributes));
    }

    {
        ULONG OfflineValue = DriveExtension->VolumeOffline ? 1u : 0u;
        RtlWriteRegistryValue(RTL_REGISTRY_SERVICES,
                              SubKey,
                              L"VolumeOffline",
                              REG_DWORD,
                              &OfflineValue,
                              sizeof(OfflineValue));
    }
}

static
VOID
RamdiskRestoreDiskState(IN PRAMDISK_DRIVE_EXTENSION DriveExtension)
{
    WCHAR SubKey[128];
    NTSTATUS Status;
    RTL_QUERY_REGISTRY_TABLE QueryTable[6];
    ULONG LinkCount = DriveExtension->MountdevLinkCount;
    ULONG Letter = 0;
    GUID GuidValue = DriveExtension->DiskGuid;
    ULONGLONG Attributes = RamdiskQueryGptAttributes(DriveExtension);
    ULONG OfflineValue = DriveExtension->VolumeOffline ? 1u : 0u;
    ULONG DefaultOffline = OfflineValue;

    RamdiskEnsureRegistryPath(DriveExtension);

    Status = RamdiskBuildRegistrySubKey(DriveExtension, SubKey, RTL_NUMBER_OF(SubKey));
    if (!NT_SUCCESS(Status))
    {
        return;
    }

    RtlZeroMemory(QueryTable, sizeof(QueryTable));

    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_TYPECHECK;
    QueryTable[0].Name = L"MountdevLinkCount";
    QueryTable[0].EntryContext = &LinkCount;
    QueryTable[0].DefaultType = REG_DWORD;
    QueryTable[0].DefaultData = &DriveExtension->MountdevLinkCount;
    QueryTable[0].DefaultLength = sizeof(DriveExtension->MountdevLinkCount);

    QueryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_TYPECHECK;
    QueryTable[1].Name = L"DriveLetter";
    QueryTable[1].EntryContext = &Letter;
    QueryTable[1].DefaultType = REG_NONE;

    QueryTable[2].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_TYPECHECK;
    QueryTable[2].Name = L"DiskGuid";
    QueryTable[2].EntryContext = &GuidValue;
    QueryTable[2].DefaultType = REG_BINARY;
    QueryTable[2].DefaultData = &DriveExtension->DiskGuid;
    QueryTable[2].DefaultLength = sizeof(GUID);

    QueryTable[3].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_TYPECHECK;
    QueryTable[3].Name = L"GptAttributes";
    QueryTable[3].EntryContext = &Attributes;
    QueryTable[3].DefaultType = REG_QWORD;
    QueryTable[3].DefaultData = &Attributes;
    QueryTable[3].DefaultLength = sizeof(Attributes);

    QueryTable[4].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_TYPECHECK;
    QueryTable[4].Name = L"VolumeOffline";
    QueryTable[4].EntryContext = &OfflineValue;
    QueryTable[4].DefaultType = REG_DWORD;
    QueryTable[4].DefaultData = &DefaultOffline;
    QueryTable[4].DefaultLength = sizeof(DefaultOffline);

    RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                           SubKey,
                           QueryTable,
                           NULL,
                           NULL);

    DriveExtension->MountdevLinkCount = LinkCount;

    if (Letter != 0)
    {
        DriveExtension->DriveLetter = (WCHAR)Letter;
    }

    RamdiskApplyGptAttributes(DriveExtension, Attributes);
    DriveExtension->VolumeOffline = (OfflineValue != 0);

    if (RtlCompareMemory(&GuidValue, &DriveExtension->DiskGuid, sizeof(GUID)) != sizeof(GUID))
    {
        NTSTATUS GuidStatus;

        DriveExtension->DiskGuid = GuidValue;

        if (DriveExtension->GuidString.Buffer)
        {
            RtlFreeUnicodeString(&DriveExtension->GuidString);
            DriveExtension->GuidString.Buffer = NULL;
            DriveExtension->GuidString.Length = 0;
            DriveExtension->GuidString.MaximumLength = 0;
        }

        GuidStatus = RtlStringFromGUID(&DriveExtension->DiskGuid, &DriveExtension->GuidString);
        if (!NT_SUCCESS(GuidStatus))
        {
            DriveExtension->GuidString.Buffer = NULL;
            DriveExtension->GuidString.Length = 0;
            DriveExtension->GuidString.MaximumLength = 0;
        }
    }
}

VOID
NTAPI
RamdiskWorkerThread(IN PDEVICE_OBJECT DeviceObject,
                    IN PVOID Context)
{
    PRAMDISK_BUS_EXTENSION DeviceExtension;
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStackLocation;
    PIRP Irp = Context;

    /* Get the stack location */
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    /* Free the work item */
    IoFreeWorkItem(Irp->Tail.Overlay.DriverContext[0]);

    /* Grab the device extension and lock it */
    DeviceExtension = DeviceObject->DeviceExtension;
    Status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, Irp);
    if (NT_SUCCESS(Status))
    {
        /* Discriminate by major code */
        switch (IoStackLocation->MajorFunction)
        {
            /* Device control */
            case IRP_MJ_DEVICE_CONTROL:
            {
                /* Let's take a look at the IOCTL */
                ULONG NormalizedIoctl = IOCTL_NORMALIZE_ACCESS(IoStackLocation->Parameters.DeviceIoControl.IoControlCode);
                switch (NormalizedIoctl)
                {
                    /* Ramdisk create request */
                    case IOCTL_NORMALIZE_ACCESS(FSCTL_CREATE_RAM_DISK):
                    {
                        /* This time we'll do it for real */
                        Status = RamdiskCreateRamdisk(DeviceObject, Irp, FALSE);
                        break;
                    }

                    case IOCTL_NORMALIZE_ACCESS(IOCTL_DISK_SET_PARTITION_INFO):
                    {
                        Status = RamdiskSetPartitionInfo(Irp, (PRAMDISK_DRIVE_EXTENSION)DeviceExtension);
                        break;
                    }

                    case IOCTL_NORMALIZE_ACCESS(IOCTL_DISK_GET_DRIVE_LAYOUT):
                    {
                        Status = STATUS_NOT_SUPPORTED;
                        break;
                    }

                    case IOCTL_NORMALIZE_ACCESS(IOCTL_DISK_GET_PARTITION_INFO):
                    {
                        Status = RamdiskGetPartitionInfo(Irp, (PRAMDISK_DRIVE_EXTENSION)DeviceExtension);
                        break;
                    }

                    case IOCTL_NORMALIZE_ACCESS(IOCTL_DISK_GET_PARTITION_INFO_EX):
                    {
                        Status = RamdiskGetPartitionInfoEx(Irp, (PRAMDISK_DRIVE_EXTENSION)DeviceExtension);
                        break;
                    }

                    default:
                    {
                        DbgPrintEx(DPFLTR_DEFAULT_ID,
                                   DPFLTR_WARNING_LEVEL,
                                   "RamdiskWorkerThread: unsupported IOCTL 0x%lx\n",
                                   IoStackLocation->Parameters.DeviceIoControl.IoControlCode);
                        Status = STATUS_INVALID_DEVICE_REQUEST;
                        break;
                    }
                }

                /* We're here */
                break;
            }

            /* Read or write request */
            case IRP_MJ_READ:
            case IRP_MJ_WRITE:
            {
                PRAMDISK_DRIVE_EXTENSION DriveExtension = (PRAMDISK_DRIVE_EXTENSION)DeviceExtension;

                if (DriveExtension->Type != RamdiskDrive)
                {
                    Status = STATUS_INVALID_DEVICE_REQUEST;
                    break;
                }

                Status = RamdiskReadWriteReal(Irp, DriveExtension);
                break;
            }

            /* Internal request (SCSI?) */
            case IRP_MJ_INTERNAL_DEVICE_CONTROL:
            {
                Status = STATUS_NOT_SUPPORTED;
                break;
            }

            /* Flush request */
            case IRP_MJ_FLUSH_BUFFERS:
            {
                Status = STATUS_SUCCESS;
                break;
            }

            /* Anything else */
            default:
            {
                DbgPrintEx(DPFLTR_DEFAULT_ID,
                           DPFLTR_WARNING_LEVEL,
                           "RamdiskWorkerThread: unsupported major function 0x%lx\n",
                           IoStackLocation->MajorFunction);
                Status = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }
        }

        if (IoStackLocation->MajorFunction == IRP_MJ_DEVICE_CONTROL)
        {
            ULONG IoControlCode = IoStackLocation->Parameters.DeviceIoControl.IoControlCode;
            PCSTR IoctlName = RamdiskGetIoctlName(IoControlCode);

            DbgPrintEx(DPFLTR_DEFAULT_ID,
                       NT_SUCCESS(Status) ? DPFLTR_TRACE_LEVEL : DPFLTR_WARNING_LEVEL,
                       "RamdiskWorkerThread: completed %s (0x%lx) -> 0x%lx info=%lu\n",
                       IoctlName ? IoctlName : "IOCTL",
                       IoControlCode,
                       Status,
                       Irp->IoStatus.Information);
        }
        else
        {
            DbgPrintEx(DPFLTR_DEFAULT_ID,
                       NT_SUCCESS(Status) ? DPFLTR_TRACE_LEVEL : DPFLTR_WARNING_LEVEL,
                       "RamdiskWorkerThread: completed major 0x%lx -> 0x%lx info=%lu\n",
                       IoStackLocation->MajorFunction,
                       Status,
                       Irp->IoStatus.Information);
        }

        /* Complete the I/O */
        IoReleaseRemoveLock(&DeviceExtension->RemoveLock, Irp);
        Irp->IoStatus.Status = Status;
        if (!NT_SUCCESS(Status))
        {
            Irp->IoStatus.Information = 0;
        }
        IoCompleteRequest(Irp, IO_DISK_INCREMENT);
        return;
    }

    /* Fail the I/O */
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

NTSTATUS
NTAPI
SendIrpToThread(IN PDEVICE_OBJECT DeviceObject,
                IN PIRP Irp)
{
    PIO_WORKITEM WorkItem;

    /* Mark the IRP pending */
    IoMarkIrpPending(Irp);

    /* Allocate a work item */
    WorkItem = IoAllocateWorkItem(DeviceObject);
    if (WorkItem)
    {
        /* Queue it up */
        Irp->Tail.Overlay.DriverContext[0] = WorkItem;
        IoQueueWorkItem(WorkItem, RamdiskWorkerThread, DelayedWorkQueue, Irp);
        return STATUS_PENDING;
    }
    else
    {
        /* Fail */
        return STATUS_INSUFFICIENT_RESOURCES;
    }
}

NTSTATUS
NTAPI
RamdiskReadWriteReal(IN PIRP Irp,
                     IN PRAMDISK_DRIVE_EXTENSION DeviceExtension)
{
    PMDL Mdl;
    PVOID CurrentBase, SystemVa, BaseAddress;
    PIO_STACK_LOCATION IoStackLocation;
    LARGE_INTEGER CurrentOffset;
    ULONG TransferLength;
    ULONG BytesRead, BytesLeft, CopyLength, MapSpan, BufferLength;
    PVOID Source, Destination;
    NTSTATUS Status;
    ULONGLONG DiskLength;
    ULONGLONG RequestOffset;
    BOOLEAN Truncated = FALSE;
    NTSTATUS TerminalStatus = STATUS_SUCCESS;

    /* Get the MDL and map it into system space */
    Mdl = Irp->MdlAddress;
    if (!Mdl)
    {
        return STATUS_INVALID_PARAMETER;
    }
    SystemVa = MmGetSystemAddressForMdlSafe(Mdl, NormalPagePriority);
    if (!SystemVa) return STATUS_INSUFFICIENT_RESOURCES;
    CurrentBase = SystemVa;

    /* Initialize default */
    Irp->IoStatus.Information = 0;

    /* Get the I/O Stack Location and capture the data */
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    CurrentOffset = IoStackLocation->Parameters.Read.ByteOffset;
    TransferLength = IoStackLocation->Parameters.Read.Length;
    BytesLeft = TransferLength;

    /* Add the MBR partition offset so filesystem I/O lands on the
     * partition data (BPB at LBA 0 of the partition) rather than
     * the raw disk MBR. */
    if (DeviceExtension->HiddenSectors > 0 && DeviceExtension->BytesPerSector > 0)
    {
        ULONGLONG PartitionOffset = (ULONGLONG)DeviceExtension->HiddenSectors * DeviceExtension->BytesPerSector;
        CurrentOffset.QuadPart += PartitionOffset;
    }
    if (IoStackLocation->MajorFunction == IRP_MJ_WRITE &&
        (DeviceExtension->DiskOptions.Readonly || DeviceExtension->DiskOptions.ExportAsCd))
    {
        return STATUS_MEDIA_WRITE_PROTECTED;
    }

    if (DeviceExtension->DiskType != RAMDISK_BOOT_DISK)
    {
        /* TODO: implement backing store for non-boot RAM disks */
        return STATUS_NOT_SUPPORTED;
    }

    if (DeviceExtension->BytesPerSector == 0)
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    /* Validate sector alignment */
    if ((CurrentOffset.QuadPart % DeviceExtension->BytesPerSector) ||
        (TransferLength % DeviceExtension->BytesPerSector))
    {
        DPRINT1("RamdiskReadWriteReal: Unaligned I/O - offset=%I64x, length=%lu, sector=%lu\n",
                CurrentOffset.QuadPart, TransferLength, DeviceExtension->BytesPerSector);
        return STATUS_INVALID_PARAMETER;
    }

    BufferLength = MmGetMdlByteCount(Mdl);
    if (BytesLeft > BufferLength)
    {
        DPRINT1("RamdiskReadWriteReal: truncating transfer len=%lu to MDL=%lu\n",
                BytesLeft, BufferLength);
        BytesLeft = BufferLength;
        TransferLength = BufferLength;
    }

    if (!BytesLeft) return STATUS_INVALID_PARAMETER;

    if (CurrentOffset.QuadPart < 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    DiskLength = DeviceExtension->DiskLength.QuadPart;
    RequestOffset = (ULONGLONG)CurrentOffset.QuadPart;

    if (RequestOffset >= DiskLength)
    {
        return (IoStackLocation->MajorFunction == IRP_MJ_READ) ? STATUS_END_OF_FILE : STATUS_DISK_FULL;
    }

    if ((ULONGLONG)BytesLeft > (DiskLength - RequestOffset))
    {
        ULONGLONG BytesAvailable = DiskLength - RequestOffset;

        if (BytesAvailable > (ULONGLONG)ULONG_MAX)
        {
            BytesLeft = ULONG_MAX;
        }
        else
        {
            BytesLeft = (ULONG)BytesAvailable;
        }

        Truncated = TRUE;
        TerminalStatus = (IoStackLocation->MajorFunction == IRP_MJ_READ) ? STATUS_END_OF_FILE : STATUS_DISK_FULL;

#if DBG
        DbgPrintEx(DPFLTR_DEFAULT_ID,
                   DPFLTR_WARNING_LEVEL,
                   "RamdiskReadWriteReal: trimming request len=%lu to=%lu at offset=%I64u (disk=%I64u)\n",
                   TransferLength,
                   BytesLeft,
                   RequestOffset,
                   DiskLength);
#endif

        if (BytesLeft == 0)
        {
            return TerminalStatus;
        }
    }

    /* Do the copy loop */
    while (BytesLeft > 0)
    {
        /* Map the pages */
        BaseAddress = RamdiskMapPages(DeviceExtension,
                                      CurrentOffset,
                                      BytesLeft,
                                      &BytesRead,
                                      &MapSpan);
        if (!BaseAddress)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        CopyLength = BytesRead;
        if (CopyLength > BytesLeft)
        {
            CopyLength = BytesLeft;
        }
        Status = STATUS_SUCCESS;

        /* MapSpan==0 means the mapping returned a shared zero-fill page
         * (BootZeroPage) for an invalid PFN. Reads return zeroes; writes
         * must be skipped to avoid corrupting the shared page. */
        if (MapSpan == 0 && IoStackLocation->MajorFunction == IRP_MJ_WRITE)
        {
            RamdiskUnmapPages(DeviceExtension, BaseAddress, CurrentOffset, MapSpan);
            Irp->IoStatus.Information += CopyLength;
            BytesLeft -= CopyLength;
            CurrentOffset.QuadPart += CopyLength;
            CurrentBase = (PVOID)((ULONG_PTR)CurrentBase + CopyLength);
            continue;
        }

        /* Check if this was a read or write */
        if (IoStackLocation->MajorFunction == IRP_MJ_READ)
        {
            Destination = CurrentBase;
            Source = BaseAddress;
            RtlCopyMemory(Destination, Source, CopyLength);
        }
        else if (IoStackLocation->MajorFunction == IRP_MJ_WRITE)
        {
            Destination = BaseAddress;
            Source = CurrentBase;
            RtlCopyMemory(Destination, Source, CopyLength);

        }
        else
        {
            Status = STATUS_INVALID_PARAMETER;
        }

        /* Unmap the pages */
        RamdiskUnmapPages(DeviceExtension, BaseAddress, CurrentOffset, MapSpan);

        if (!NT_SUCCESS(Status))
        {
            break;
        }

        /* Update offset and bytes left */
        Irp->IoStatus.Information += CopyLength;
        BytesLeft -= CopyLength;
        CurrentOffset.QuadPart += CopyLength;
        CurrentBase = (PVOID)((ULONG_PTR)CurrentBase + CopyLength);
    }

    if (!NT_SUCCESS(Status))
    {
        if (Irp->IoStatus.Information == 0)
        {
            return Status;
        }

        Status = STATUS_SUCCESS;
    }

    if (Truncated)
    {
        if (IoStackLocation->MajorFunction == IRP_MJ_READ)
        {
            return (Irp->IoStatus.Information > 0) ? STATUS_SUCCESS : STATUS_END_OF_FILE;
        }

        return TerminalStatus;
    }

    return Status;
}

NTSTATUS
NTAPI
RamdiskOpenClose(IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP Irp)
{
    PRAMDISK_DRIVE_EXTENSION DeviceExtension;
    NTSTATUS Status;

    /* Get the device extension */
    DeviceExtension = DeviceObject->DeviceExtension;

    /* Acquire the remove lock if this is a drive */
    if (DeviceExtension->Type == RamdiskDrive)
    {
        Status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, Irp);
        if (!NT_SUCCESS(Status))
        {
            /* Fail the IRP */
            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
        }
    }

    /* Complete the IRP */
    Irp->IoStatus.Information = 0;  /* Conventionally return 0 for create/close */
    Irp->IoStatus.Status = STATUS_SUCCESS;

    /* Release the remove lock if this is a drive before completing */
    if (DeviceExtension->Type == RamdiskDrive)
    {
        IoReleaseRemoveLock(&DeviceExtension->RemoveLock, Irp);
    }

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RamdiskReadWrite(IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP Irp)
{
    PRAMDISK_DRIVE_EXTENSION DeviceExtension;
    // ULONG Length;
    // LARGE_INTEGER ByteOffset;
    PIO_STACK_LOCATION IoStackLocation;
    NTSTATUS Status, ReturnStatus;

    /* Get the device extension and make sure this isn't a bus */
    DeviceExtension = DeviceObject->DeviceExtension;
    if (DeviceExtension->Type == RamdiskBus)
    {
        /* Fail */
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto Complete;
    }

    /* Capture parameters */
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    // Length = IoStackLocation->Parameters.Read.Length;
    // ByteOffset = IoStackLocation->Parameters.Read.ByteOffset;

    /* FIXME: Validate offset */

    /* FIXME: Validate sector */

    /* Validate write */
    if ((IoStackLocation->MajorFunction == IRP_MJ_WRITE) &&
        (DeviceExtension->DiskOptions.Readonly))
    {
        /* Fail, this is read-only */
        Status = STATUS_MEDIA_WRITE_PROTECTED;
        goto Complete;
    }

    /* See if we want to do this sync or async */
    if (DeviceExtension->DiskType > RAMDISK_MEMORY_MAPPED_DISK)
    {
        /* Do it sync */
        Status = RamdiskReadWriteReal(Irp, DeviceExtension);
        goto Complete;
    }

    /* Queue it to the worker */
    Status = SendIrpToThread(DeviceObject, Irp);
    ReturnStatus = STATUS_PENDING;

    /* Check if we're pending or not */
    if (Status != STATUS_PENDING)
    {
Complete:
        /* Complete the IRP */
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_DISK_INCREMENT);
        ReturnStatus = Status;
    }

    /* Return to caller */
    return ReturnStatus;
}

NTSTATUS
NTAPI
RamdiskDeviceControl(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    PRAMDISK_BUS_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    PRAMDISK_DRIVE_EXTENSION DriveExtension = (PVOID)DeviceExtension;
    ULONG Information;
    PCDROM_TOC Toc;
    PDISK_GEOMETRY DiskGeometry;
    ULONG IoControlCode;
    PCSTR IoctlName;
    PCSTR DeviceTypeName;

    /* Grab the remove lock */
    Status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS(Status))
    {
        /* Fail the IRP */
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    /* Setup some defaults */
    Status = STATUS_INVALID_DEVICE_REQUEST;
    Information = 0;

    IoControlCode = IoStackLocation->Parameters.DeviceIoControl.IoControlCode;
    IoctlName = RamdiskGetIoctlName(IoControlCode);
    DeviceTypeName = (DeviceExtension->Type == RamdiskBus) ? "Bus" : "Drive";
    ULONG NormalizedIoctl = IOCTL_NORMALIZE_ACCESS(IoControlCode);

    if (IoctlName)
    {
        DPRINT("RamdiskDeviceControl[%s]: request %s (0x%lx) in=%lu out=%lu\n",
               DeviceTypeName,
               IoctlName,
               IoControlCode,
               IoStackLocation->Parameters.DeviceIoControl.InputBufferLength,
               IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength);
    }
    else
    {
        DPRINT("RamdiskDeviceControl[%s]: request 0x%lx in=%lu out=%lu\n",
               DeviceTypeName,
               IoControlCode,
               IoStackLocation->Parameters.DeviceIoControl.InputBufferLength,
               IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength);
    }

    /* Check if this is an bus device or the drive */
    if (DeviceExtension->Type == RamdiskBus)
    {
        /* Check what the request is */
        switch (NormalizedIoctl)
        {
            /* Request to create a ramdisk */
            case IOCTL_NORMALIZE_ACCESS(FSCTL_CREATE_RAM_DISK):
            {
                /* Do it */
                Status = RamdiskCreateRamdisk(DeviceObject, Irp, TRUE);
                if (!NT_SUCCESS(Status)) goto CompleteRequest;
                break;
            }

            default:
            {
                /* We don't handle anything else yet */
                DbgPrintEx(DPFLTR_DEFAULT_ID,
                           DPFLTR_WARNING_LEVEL,
                           "RamdiskDeviceControl[%s]: unsupported FSCTL 0x%lx\n",
                           DeviceTypeName,
                           IoControlCode);
                Status = STATUS_INVALID_DEVICE_REQUEST;
                goto CompleteRequest;
            }
        }
    }
    else
    {
        /* Check what the request is */
        switch (NormalizedIoctl)
        {
            case IOCTL_NORMALIZE_ACCESS(IOCTL_STORAGE_CHECK_VERIFY):
            case IOCTL_NORMALIZE_ACCESS(IOCTL_DISK_CHECK_VERIFY):
            case IOCTL_NORMALIZE_ACCESS(IOCTL_CDROM_CHECK_VERIFY):
            {
                /* Just pretend it's OK, don't do more */
                Status = STATUS_SUCCESS;
                break;
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_STORAGE_GET_MEDIA_TYPES):
            case IOCTL_NORMALIZE_ACCESS(IOCTL_DISK_GET_MEDIA_TYPES):
            case IOCTL_NORMALIZE_ACCESS(IOCTL_DISK_GET_DRIVE_GEOMETRY):
            case IOCTL_NORMALIZE_ACCESS(IOCTL_CDROM_GET_DRIVE_GEOMETRY):
            {
                /* Validate the length */
                if (IoStackLocation->Parameters.DeviceIoControl.
                    OutputBufferLength < sizeof(DISK_GEOMETRY))
                {
                    /* Invalid length */
                    Status = STATUS_BUFFER_TOO_SMALL;
                    break;
                }

                /* Fill it out */
                DiskGeometry = Irp->AssociatedIrp.SystemBuffer;
                DiskGeometry->Cylinders.QuadPart = DriveExtension->Cylinders;
                DiskGeometry->BytesPerSector = DriveExtension->BytesPerSector;
                DiskGeometry->SectorsPerTrack = DriveExtension->SectorsPerTrack;
                DiskGeometry->TracksPerCylinder = DriveExtension->NumberOfHeads;
                DiskGeometry->MediaType = DriveExtension->DiskOptions.ExportAsCd ?
                                          RemovableMedia :
                                          (DriveExtension->DiskOptions.Fixed ? FixedMedia : RemovableMedia);

                /* We are done */
                Status = STATUS_SUCCESS;
                Information = sizeof(DISK_GEOMETRY);
                break;
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_DISK_GET_DRIVE_GEOMETRY_EX):
            {
                PDISK_GEOMETRY_EX GeometryEx;

                if (IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength < sizeof(DISK_GEOMETRY_EX))
                {
                    Status = STATUS_BUFFER_TOO_SMALL;
                    Information = sizeof(DISK_GEOMETRY_EX);
                    break;
                }

                GeometryEx = Irp->AssociatedIrp.SystemBuffer;
                RtlZeroMemory(GeometryEx, sizeof(DISK_GEOMETRY_EX));
                GeometryEx->Geometry.Cylinders.QuadPart = DriveExtension->Cylinders;
                GeometryEx->Geometry.BytesPerSector = DriveExtension->BytesPerSector;
                GeometryEx->Geometry.SectorsPerTrack = DriveExtension->SectorsPerTrack;
                GeometryEx->Geometry.TracksPerCylinder = DriveExtension->NumberOfHeads;
                GeometryEx->Geometry.MediaType = DriveExtension->DiskOptions.ExportAsCd ?
                                                   RemovableMedia :
                                                   (DriveExtension->DiskOptions.Fixed ? FixedMedia : RemovableMedia);
                GeometryEx->DiskSize = DriveExtension->DiskLength;

                Status = STATUS_SUCCESS;
                Information = sizeof(DISK_GEOMETRY_EX);
                break;
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_CDROM_READ_TOC):
            {
                /* Validate the length */
                if (IoStackLocation->Parameters.DeviceIoControl.
                    OutputBufferLength < sizeof(CDROM_TOC))
                {
                    /* Invalid length */
                    Status = STATUS_BUFFER_TOO_SMALL;
                    Information = RAMDISK_TOC_SIZE;
                    break;
                }

                /* Clear the TOC */
                Toc = Irp->AssociatedIrp.SystemBuffer;
                RtlZeroMemory(Toc, sizeof(CDROM_TOC));

                /* Fill it out */
                Toc->Length[0] = 0;
                Toc->Length[1] = RAMDISK_TOC_SIZE - sizeof(Toc->Length);
                Toc->FirstTrack = 1;
                Toc->LastTrack = 1;
                Toc->TrackData[0].Adr = 1;
                Toc->TrackData[0].Control = TOC_DATA_TRACK;
                Toc->TrackData[0].TrackNumber = 1;

                /* We are done */
                Status = STATUS_SUCCESS;
                Information = RAMDISK_TOC_SIZE;
                break;
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_DISK_SET_PARTITION_INFO):
            {
                Status = RamdiskSetPartitionInfo(Irp, DriveExtension);
                break;
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_DISK_GET_PARTITION_INFO):
            {
                /* Validate the length */
                if (IoStackLocation->Parameters.DeviceIoControl.
                    OutputBufferLength < sizeof(PARTITION_INFORMATION))
                {
                    /* Invalid length */
                    Status = STATUS_BUFFER_TOO_SMALL;
                    Information = sizeof(PARTITION_INFORMATION);
                    break;
                }

                /* Check if we need to do this sync or async */
                if (DriveExtension->DiskType > RAMDISK_MEMORY_MAPPED_DISK)
                {
                    /* Call the helper function */
                    Status = RamdiskGetPartitionInfo(Irp, DriveExtension);
                }
                else
                {
                    /* Do it asynchronously later */
                    goto CallWorker;
                }

                /* We are done */
                Information = Irp->IoStatus.Information;
                break;
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_DISK_GET_PARTITION_INFO_EX):
            {
                if (DriveExtension->DiskType > RAMDISK_MEMORY_MAPPED_DISK)
                {
                    Status = RamdiskGetPartitionInfoEx(Irp, DriveExtension);
                    Information = Irp->IoStatus.Information;
                }
                else
                {
                    goto CallWorker;
                }
                break;
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_DISK_GET_LENGTH_INFO):
            {
                PGET_LENGTH_INFORMATION LengthInformation = Irp->AssociatedIrp.SystemBuffer;

                /* Validate the length */
                if (IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength < sizeof(GET_LENGTH_INFORMATION))
                {
                    /* Invalid length */
                    Status = STATUS_BUFFER_TOO_SMALL;
                    Information = sizeof(GET_LENGTH_INFORMATION);
                    break;
                }

                /* Fill it out */
                LengthInformation->Length = DriveExtension->DiskLength;

                /* We are done */
                Status = STATUS_SUCCESS;
                Information = sizeof(GET_LENGTH_INFORMATION);
                break;
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_DISK_GET_DRIVE_LAYOUT):
            {
                ULONG RequiredLength;
                PDRIVE_LAYOUT_INFORMATION LayoutInformation;

                RequiredLength = FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION, PartitionEntry) + sizeof(PARTITION_INFORMATION);
                if (IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength < RequiredLength)
                {
                    Status = STATUS_BUFFER_OVERFLOW;
                    Information = RequiredLength;
                    break;
                }

                LayoutInformation = Irp->AssociatedIrp.SystemBuffer;
                RtlZeroMemory(LayoutInformation, RequiredLength);
                LayoutInformation->PartitionCount = 1;
                Status = RamdiskBuildPartitionInfo(DriveExtension, &LayoutInformation->PartitionEntry[0]);
                if (!NT_SUCCESS(Status))
                {
                    Information = 0;
                    break;
                }

                Information = RequiredLength;
                Status = STATUS_SUCCESS;
                break;
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_DISK_GET_DRIVE_LAYOUT_EX):
            {
                ULONG RequiredLength;
                PDRIVE_LAYOUT_INFORMATION_EX LayoutInformation;
                PARTITION_INFORMATION_EX PartitionEntry;
                ULONG Signature;

                RequiredLength = FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION_EX, PartitionEntry) + sizeof(PARTITION_INFORMATION_EX);
                if (IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength < RequiredLength)
                {
                    Status = STATUS_BUFFER_OVERFLOW;
                    Information = RequiredLength;
                    break;
                }

                Status = RamdiskBuildPartitionInfoEx(DriveExtension, &PartitionEntry);
                if (!NT_SUCCESS(Status))
                {
                    Information = 0;
                    break;
                }

                LayoutInformation = Irp->AssociatedIrp.SystemBuffer;
                RtlZeroMemory(LayoutInformation, RequiredLength);
                LayoutInformation->PartitionStyle = PARTITION_STYLE_MBR;
                LayoutInformation->PartitionCount = 1;
                Signature = DriveExtension->DiskGuid.Data1;
                if (Signature == 0)
                {
                    Signature = 'ROSD'; /* Provide a stable non-zero fallback */
                }
                LayoutInformation->Mbr.Signature = Signature;
                LayoutInformation->PartitionEntry[0] = PartitionEntry;

                Information = RequiredLength;
                Status = STATUS_SUCCESS;
                break;
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_DISK_IS_WRITABLE):
            {
                Status = DriveExtension->DiskOptions.Readonly ? STATUS_MEDIA_WRITE_PROTECTED : STATUS_SUCCESS;
                Information = 0;
                break;
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_VOLUME_GET_GPT_ATTRIBUTES):
            {
                PVOLUME_GET_GPT_ATTRIBUTES_INFORMATION GptInformation;

                /* Validate the length */
                if (IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength < sizeof(VOLUME_GET_GPT_ATTRIBUTES_INFORMATION))
                {
                    /* Invalid length */
                    Status = STATUS_BUFFER_TOO_SMALL;
                    Information = sizeof(VOLUME_GET_GPT_ATTRIBUTES_INFORMATION);
                    break;
                }

                /* Fill it out */
                GptInformation = Irp->AssociatedIrp.SystemBuffer;
                GptInformation->GptAttributes = RamdiskQueryGptAttributes(DriveExtension);

                /* We are done */
                Status = STATUS_SUCCESS;
                Information = sizeof(VOLUME_GET_GPT_ATTRIBUTES_INFORMATION);
                break;
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_STORAGE_GET_DEVICE_NUMBER):
            {
                PSTORAGE_DEVICE_NUMBER DeviceNumber;

                if (IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength < sizeof(STORAGE_DEVICE_NUMBER))
                {
                    Status = STATUS_BUFFER_TOO_SMALL;
                    Information = sizeof(STORAGE_DEVICE_NUMBER);
                    break;
                }

                DeviceNumber = Irp->AssociatedIrp.SystemBuffer;
                DeviceNumber->DeviceType = DriveExtension->DiskOptions.ExportAsCd ? FILE_DEVICE_CD_ROM : FILE_DEVICE_DISK;
                DeviceNumber->DeviceNumber = DriveExtension->DiskNumber;
                DeviceNumber->PartitionNumber = 0;
                Status = STATUS_SUCCESS;
                Information = sizeof(STORAGE_DEVICE_NUMBER);
                break;
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_STORAGE_GET_HOTPLUG_INFO):
            {
                PSTORAGE_HOTPLUG_INFO HotplugInfo;

                if (IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength < sizeof(STORAGE_HOTPLUG_INFO))
                {
                    Status = STATUS_BUFFER_TOO_SMALL;
                    Information = sizeof(STORAGE_HOTPLUG_INFO);
                    break;
                }

                HotplugInfo = Irp->AssociatedIrp.SystemBuffer;
                RtlZeroMemory(HotplugInfo, sizeof(*HotplugInfo));
                HotplugInfo->Size = sizeof(STORAGE_HOTPLUG_INFO);
                HotplugInfo->MediaRemovable = !DriveExtension->DiskOptions.Fixed;
                HotplugInfo->MediaHotplug = FALSE;
                HotplugInfo->DeviceHotplug = FALSE;
                HotplugInfo->WriteCacheEnableOverride = 0;
                Status = STATUS_SUCCESS;
                Information = sizeof(STORAGE_HOTPLUG_INFO);
                break;
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_STORAGE_QUERY_PROPERTY):
            {
                Status = RamdiskHandleStorageQueryProperty(DriveExtension, Irp, IoStackLocation, &Information);
                break;
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS):
            {
                PVOLUME_DISK_EXTENTS Extents;
                ULONG RequiredLength;

                RequiredLength = FIELD_OFFSET(VOLUME_DISK_EXTENTS, Extents) + sizeof(DISK_EXTENT);
                if (IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength < RequiredLength)
                {
                    Status = STATUS_BUFFER_TOO_SMALL;
                    Information = RequiredLength;
                    break;
                }

                Extents = Irp->AssociatedIrp.SystemBuffer;
                Extents->NumberOfDiskExtents = 1;
                Extents->Extents[0].DiskNumber = DriveExtension->DiskNumber;
                Extents->Extents[0].StartingOffset.QuadPart = 0;
                Extents->Extents[0].ExtentLength = DriveExtension->DiskLength;
                Status = STATUS_SUCCESS;
                Information = RequiredLength;
                break;
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_VOLUME_QUERY_FAILOVER_SET):
            {
                PVOLUME_FAILOVER_SET FailoverSet;
                ULONG RequiredLength;

                RequiredLength = FIELD_OFFSET(VOLUME_FAILOVER_SET, DiskNumbers) + sizeof(ULONG);
                if (IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength < RequiredLength)
                {
                    Status = STATUS_BUFFER_TOO_SMALL;
                    Information = RequiredLength;
                    break;
                }

                FailoverSet = Irp->AssociatedIrp.SystemBuffer;
                FailoverSet->NumberOfDisks = 1;
                FailoverSet->DiskNumbers[0] = DriveExtension->DiskNumber;
                Status = STATUS_SUCCESS;
                Information = RequiredLength;
                break;
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_VOLUME_IS_OFFLINE):
            {
                if (IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength < sizeof(BOOLEAN))
                {
                    Status = STATUS_BUFFER_TOO_SMALL;
                    Information = sizeof(BOOLEAN);
                    break;
                }

                *(PBOOLEAN)Irp->AssociatedIrp.SystemBuffer = DriveExtension->VolumeOffline;
                Status = STATUS_SUCCESS;
                Information = sizeof(BOOLEAN);
                break;
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_VOLUME_IS_IO_CAPABLE):
            {
                if (IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength < sizeof(BOOLEAN))
                {
                    Status = STATUS_BUFFER_TOO_SMALL;
                    Information = sizeof(BOOLEAN);
                    break;
                }

                *(PBOOLEAN)Irp->AssociatedIrp.SystemBuffer = TRUE;
                Status = STATUS_SUCCESS;
                Information = sizeof(BOOLEAN);
                break;
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_VOLUME_QUERY_VOLUME_NUMBER):
            {
                PVOLUME_NUMBER VolumeNumber;

                if (IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength < sizeof(VOLUME_NUMBER))
                {
                    Status = STATUS_BUFFER_TOO_SMALL;
                    Information = sizeof(VOLUME_NUMBER);
                    break;
                }

                VolumeNumber = Irp->AssociatedIrp.SystemBuffer;
                VolumeNumber->VolumeNumber = DriveExtension->DiskNumber;
                RtlZeroMemory(VolumeNumber->VolumeManagerName, sizeof(VolumeNumber->VolumeManagerName));
                {
                    static const WCHAR ManagerName[] = L"RAMDISK";
                    SIZE_T CopyChars = RTL_NUMBER_OF(ManagerName) - 1;
                    SIZE_T MaxChars = RTL_NUMBER_OF(VolumeNumber->VolumeManagerName) - 1;
                    if (CopyChars > MaxChars)
                    {
                        CopyChars = MaxChars;
                    }
                    RtlCopyMemory(VolumeNumber->VolumeManagerName,
                                  ManagerName,
                                  CopyChars * sizeof(WCHAR));
                    VolumeNumber->VolumeManagerName[CopyChars] = L'\0';
                }
                Status = STATUS_SUCCESS;
                Information = sizeof(VOLUME_NUMBER);
                break;
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_VOLUME_LOGICAL_TO_PHYSICAL):
            {
                PVOLUME_LOGICAL_OFFSET LogicalOffset;
                PVOLUME_PHYSICAL_OFFSETS PhysicalOffsets;
                ULONG RequiredLength;

                if (IoStackLocation->Parameters.DeviceIoControl.InputBufferLength < sizeof(VOLUME_LOGICAL_OFFSET))
                {
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                RequiredLength = FIELD_OFFSET(VOLUME_PHYSICAL_OFFSETS, PhysicalOffset) + sizeof(VOLUME_PHYSICAL_OFFSET);
                if (IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength < RequiredLength)
                {
                    Status = STATUS_BUFFER_TOO_SMALL;
                    Information = RequiredLength;
                    break;
                }

                LogicalOffset = Irp->AssociatedIrp.SystemBuffer;
                PhysicalOffsets = Irp->AssociatedIrp.SystemBuffer;
                LONGLONG LogicalPosition = LogicalOffset->LogicalOffset;
                PhysicalOffsets->NumberOfPhysicalOffsets = 1;
                PhysicalOffsets->PhysicalOffset[0].DiskNumber = DriveExtension->DiskNumber;
                PhysicalOffsets->PhysicalOffset[0].Offset = LogicalPosition;
                Status = STATUS_SUCCESS;
                Information = RequiredLength;
                break;
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_VOLUME_PHYSICAL_TO_LOGICAL):
            {
                PVOLUME_PHYSICAL_OFFSET PhysicalOffset;
                PVOLUME_LOGICAL_OFFSET LogicalOffset;

                if (IoStackLocation->Parameters.DeviceIoControl.InputBufferLength < sizeof(VOLUME_PHYSICAL_OFFSET) ||
                    IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength < sizeof(VOLUME_LOGICAL_OFFSET))
                {
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                PhysicalOffset = Irp->AssociatedIrp.SystemBuffer;
                LogicalOffset = Irp->AssociatedIrp.SystemBuffer;

                if (PhysicalOffset->DiskNumber != DriveExtension->DiskNumber)
                {
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                LogicalOffset->LogicalOffset = PhysicalOffset->Offset;
                Status = STATUS_SUCCESS;
                Information = sizeof(VOLUME_LOGICAL_OFFSET);
                break;
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_VOLUME_IS_PARTITION):
            {
                if (IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength < sizeof(BOOLEAN))
                {
                    Status = STATUS_BUFFER_TOO_SMALL;
                    Information = sizeof(BOOLEAN);
                    break;
                }

                *(PBOOLEAN)Irp->AssociatedIrp.SystemBuffer = TRUE;
                Status = STATUS_SUCCESS;
                Information = sizeof(BOOLEAN);
                break;
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_MOUNTDEV_QUERY_DEVICE_NAME):
            {
                PMOUNTDEV_NAME Name;
                USHORT NameLength;
                ULONG RequiredLength;
                ULONG OutputLength;
                ULONG CopyLength;

                if (DriveExtension->DeviceObjectName.Buffer == NULL)
                {
                    Status = STATUS_INVALID_DEVICE_STATE;
                    Information = 0;
                    break;
                }

                DbgPrintEx(DPFLTR_DEFAULT_ID,
                           DPFLTR_TRACE_LEVEL,
                           "RamdiskDeviceControl: IOCTL_MOUNTDEV_QUERY_DEVICE_NAME -> %wZ\n",
                           &DriveExtension->DeviceObjectName);

                NameLength = DriveExtension->DeviceObjectName.Length;
                RequiredLength = FIELD_OFFSET(MOUNTDEV_NAME, Name) + NameLength;
                OutputLength = IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength;

                if (OutputLength < sizeof(USHORT))
                {
                    Status = STATUS_BUFFER_TOO_SMALL;
                    Information = sizeof(USHORT);
                RamdiskLogBufferedReply("IOCTL_MOUNTDEV_QUERY_DEVICE_NAME",
                                         Irp->AssociatedIrp.SystemBuffer,
                                         OutputLength,
                                         FIELD_OFFSET(MOUNTDEV_NAME, Name),
                                         NameLength,
                                         RequiredLength,
                                         0,
                                         Status);
                    break;
                }

                Name = Irp->AssociatedIrp.SystemBuffer;
                Name->NameLength = NameLength;
                CopyLength = 0;

                if (OutputLength > FIELD_OFFSET(MOUNTDEV_NAME, Name))
                {
                    CopyLength = OutputLength - FIELD_OFFSET(MOUNTDEV_NAME, Name);
                    if (CopyLength > NameLength) CopyLength = NameLength;
                    if (CopyLength)
                    {
#if DBG
                        ASSERT(FIELD_OFFSET(MOUNTDEV_NAME, Name) + CopyLength <= OutputLength);
#endif
                        RtlCopyMemory(Name->Name,
                                      DriveExtension->DeviceObjectName.Buffer,
                                      CopyLength);
                    }
                }

                if (OutputLength >= RequiredLength)
                {
                    Status = STATUS_SUCCESS;
                    Information = RequiredLength;
                }
                else
                {
                    Status = STATUS_BUFFER_OVERFLOW;
                    Information = FIELD_OFFSET(MOUNTDEV_NAME, Name) + CopyLength;
                }
                RamdiskLogBufferedReply("IOCTL_MOUNTDEV_QUERY_DEVICE_NAME",
                                         Irp->AssociatedIrp.SystemBuffer,
                                         OutputLength,
                                         FIELD_OFFSET(MOUNTDEV_NAME, Name),
                                         NameLength,
                                         RequiredLength,
                                         CopyLength,
                                         Status);
                break;
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_MOUNTDEV_QUERY_DEVICE_RELATIONS):
            {
                PMOUNTDEV_DEVICE_RELATIONS Relations;
                ULONG RequiredLength;

                if (DriveExtension->PhysicalDeviceObject == NULL)
                {
                    Status = STATUS_INVALID_DEVICE_STATE;
                    Information = 0;
                    break;
                }

                RequiredLength = FIELD_OFFSET(MOUNTDEV_DEVICE_RELATIONS, Objects) + sizeof(PDEVICE_OBJECT);
                if (IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength < RequiredLength)
                {
                    Status = STATUS_BUFFER_TOO_SMALL;
                    Information = RequiredLength;
                    break;
                }

                Relations = Irp->AssociatedIrp.SystemBuffer;
                Relations->NumberOfObjects = 1;
                ObReferenceObject(DriveExtension->PhysicalDeviceObject);
                Relations->Objects[0] = DriveExtension->PhysicalDeviceObject;
                Status = STATUS_SUCCESS;
                Information = RequiredLength;
                break;
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_MOUNTDEV_QUERY_UNIQUE_ID):
            {
                PMOUNTDEV_UNIQUE_ID UniqueId;
                USHORT IdLength;
                ULONG RequiredLength;
                ULONG OutputLength;
                ULONG CopyLength;

                IdLength = sizeof(GUID);
                RequiredLength = FIELD_OFFSET(MOUNTDEV_UNIQUE_ID, UniqueId) + IdLength;
                OutputLength = IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength;

                DbgPrintEx(DPFLTR_DEFAULT_ID,
                           DPFLTR_TRACE_LEVEL,
                           "RamdiskDeviceControl: IOCTL_MOUNTDEV_QUERY_UNIQUE_ID\n");

                if (OutputLength < sizeof(USHORT))
                {
                    Status = STATUS_BUFFER_TOO_SMALL;
                    Information = sizeof(USHORT);
                RamdiskLogBufferedReply("IOCTL_MOUNTDEV_QUERY_UNIQUE_ID",
                                         Irp->AssociatedIrp.SystemBuffer,
                                         OutputLength,
                                         FIELD_OFFSET(MOUNTDEV_UNIQUE_ID, UniqueId),
                                         IdLength,
                                         RequiredLength,
                                         0,
                                         Status);
                    break;
                }

                UniqueId = Irp->AssociatedIrp.SystemBuffer;
                UniqueId->UniqueIdLength = IdLength;
                CopyLength = 0;

                if (OutputLength > FIELD_OFFSET(MOUNTDEV_UNIQUE_ID, UniqueId))
                {
                    CopyLength = OutputLength - FIELD_OFFSET(MOUNTDEV_UNIQUE_ID, UniqueId);
                    if (CopyLength > IdLength) CopyLength = IdLength;
                    if (CopyLength)
                    {
#if DBG
                        ASSERT(FIELD_OFFSET(MOUNTDEV_UNIQUE_ID, UniqueId) + CopyLength <= OutputLength);
#endif
                        RtlCopyMemory(UniqueId->UniqueId, &DriveExtension->DiskGuid, CopyLength);
                    }
                }

                if (OutputLength >= RequiredLength)
                {
                    Status = STATUS_SUCCESS;
                    Information = RequiredLength;
                }
                else
                {
                    Status = STATUS_BUFFER_OVERFLOW;
                    Information = FIELD_OFFSET(MOUNTDEV_UNIQUE_ID, UniqueId) + CopyLength;
                }
                RamdiskLogBufferedReply("IOCTL_MOUNTDEV_QUERY_UNIQUE_ID",
                                         Irp->AssociatedIrp.SystemBuffer,
                                         OutputLength,
                                         FIELD_OFFSET(MOUNTDEV_UNIQUE_ID, UniqueId),
                                         IdLength,
                                         RequiredLength,
                                         CopyLength,
                                         Status);
                break;
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_MOUNTDEV_QUERY_STABLE_GUID):
            {
                PMOUNTDEV_STABLE_GUID StableGuid;
                ULONG OutputLength = IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength;

                Information = sizeof(MOUNTDEV_STABLE_GUID);

                if (OutputLength < sizeof(MOUNTDEV_STABLE_GUID))
                {
                    Status = STATUS_BUFFER_TOO_SMALL;
                RamdiskLogBufferedReply("IOCTL_MOUNTDEV_QUERY_STABLE_GUID",
                                         Irp->AssociatedIrp.SystemBuffer,
                                         OutputLength,
                                         0,
                                         sizeof(MOUNTDEV_STABLE_GUID),
                                         sizeof(MOUNTDEV_STABLE_GUID),
                                         0,
                                         Status);
                    break;
                }

                DbgPrintEx(DPFLTR_DEFAULT_ID,
                           DPFLTR_TRACE_LEVEL,
                           "RamdiskDeviceControl: IOCTL_MOUNTDEV_QUERY_STABLE_GUID\n");

                StableGuid = Irp->AssociatedIrp.SystemBuffer;
#if DBG
                ASSERT(OutputLength >= sizeof(MOUNTDEV_STABLE_GUID));
#endif
                StableGuid->StableGuid = DriveExtension->DiskGuid;
                Status = STATUS_SUCCESS;
                RamdiskLogBufferedReply("IOCTL_MOUNTDEV_QUERY_STABLE_GUID",
                                         Irp->AssociatedIrp.SystemBuffer,
                                         OutputLength,
                                         0,
                                         sizeof(MOUNTDEV_STABLE_GUID),
                                         sizeof(MOUNTDEV_STABLE_GUID),
                                         sizeof(MOUNTDEV_STABLE_GUID),
                                         Status);
                break;
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME):
            {
                PMOUNTDEV_SUGGESTED_LINK_NAME LinkName;
                WCHAR SuggestedName[16];
                WCHAR Letter;
                USHORT NameLength;
                ULONG RequiredLength;
                ULONG OutputLength;
                ULONG CopyLength;
                ULONG HeaderLength;

                Letter = DriveExtension->DriveLetter ? DriveExtension->DriveLetter : L'X';
                _snwprintf(SuggestedName,
                           RTL_NUMBER_OF(SuggestedName),
                           L"\\DosDevices\\%wc:",
                           Letter);
                NameLength = (USHORT)(wcslen(SuggestedName) * sizeof(WCHAR));
                RequiredLength = FIELD_OFFSET(MOUNTDEV_SUGGESTED_LINK_NAME, Name) + NameLength;
                OutputLength = IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength;
                HeaderLength = FIELD_OFFSET(MOUNTDEV_SUGGESTED_LINK_NAME, Name);

                DbgPrintEx(DPFLTR_DEFAULT_ID,
                           DPFLTR_TRACE_LEVEL,
                           "RamdiskDeviceControl: IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME -> %S\n",
                           SuggestedName);

                if (OutputLength < HeaderLength)
                {
                    Status = STATUS_BUFFER_TOO_SMALL;
                    Information = HeaderLength;
                RamdiskLogBufferedReply("IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME",
                                         Irp->AssociatedIrp.SystemBuffer,
                                         OutputLength,
                                         HeaderLength,
                                         NameLength,
                                         RequiredLength,
                                         0,
                                         Status);
                    break;
                }

                LinkName = Irp->AssociatedIrp.SystemBuffer;
                RtlZeroMemory(LinkName, OutputLength);
                LinkName->UseOnlyIfThereAreNoOtherLinks = FALSE;
                LinkName->NameLength = NameLength;
                CopyLength = 0;

                if (OutputLength > HeaderLength)
                {
                    CopyLength = OutputLength - HeaderLength;
                    if (CopyLength > NameLength) CopyLength = NameLength;
                    CopyLength &= ~1UL;
                    if (CopyLength)
                    {
#if DBG
                        ASSERT(HeaderLength + CopyLength <= OutputLength);
#endif
                        RtlCopyMemory(LinkName->Name, SuggestedName, CopyLength);
                    }
                }

                if (OutputLength >= RequiredLength)
                {
                    LinkName->Name[NameLength / sizeof(WCHAR)] = UNICODE_NULL;
                    Status = STATUS_SUCCESS;
                    Information = RequiredLength;
                }
                else
                {
                    if (OutputLength > HeaderLength)
                    {
                        ULONG DestChars = (OutputLength - HeaderLength) / sizeof(WCHAR);
                        if (DestChars > 0)
                        {
                            LinkName->Name[DestChars - 1] = UNICODE_NULL;
                        }
                    }
                    Status = STATUS_BUFFER_OVERFLOW;
                    Information = HeaderLength + CopyLength;
                }
                RamdiskLogBufferedReply("IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME",
                                         Irp->AssociatedIrp.SystemBuffer,
                                         OutputLength,
                                         FIELD_OFFSET(MOUNTDEV_SUGGESTED_LINK_NAME, Name),
                                         NameLength,
                                         RequiredLength,
                                         CopyLength,
                                         Status);
                break;
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_SCSI_PASS_THROUGH):
            case IOCTL_NORMALIZE_ACCESS(IOCTL_SCSI_PASS_THROUGH_DIRECT):
            case IOCTL_NORMALIZE_ACCESS(IOCTL_SCSI_GET_ADDRESS):
            {
                Status = STATUS_INVALID_DEVICE_REQUEST;
                Information = 0;
                break;
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_MOUNTDEV_LINK_CREATED):
            {
                InterlockedIncrement((volatile LONG*)&DriveExtension->MountdevLinkCount);
                RamdiskPersistDiskState(DriveExtension);
                Status = STATUS_SUCCESS;
                Information = 0;
                break;
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_MOUNTDEV_LINK_DELETED):
            {
                if (DriveExtension->MountdevLinkCount > 0)
                {
                    InterlockedDecrement((volatile LONG*)&DriveExtension->MountdevLinkCount);
                }

                RamdiskPersistDiskState(DriveExtension);
                Status = STATUS_SUCCESS;
                Information = 0;
                break;
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY):
            {
                PMOUNTDEV_UNIQUE_ID InputUniqueId;
                GUID InputGuid;
                ULONG OutputLength = IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength;
                ULONG InputLength = IoStackLocation->Parameters.DeviceIoControl.InputBufferLength;

                Information = 0;

                /* Validate the input unique ID buffer */
                if (InputLength < FIELD_OFFSET(MOUNTDEV_UNIQUE_ID, UniqueId) + sizeof(GUID))
                {
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                InputUniqueId = Irp->AssociatedIrp.SystemBuffer;
                if (InputUniqueId->UniqueIdLength != sizeof(GUID) ||
                    InputLength < FIELD_OFFSET(MOUNTDEV_UNIQUE_ID, UniqueId) + InputUniqueId->UniqueIdLength)
                {
                    Status = STATUS_NOT_SUPPORTED;
                    break;
                }

                RtlCopyMemory(&InputGuid, InputUniqueId->UniqueId, sizeof(GUID));

                /* If the caller's idea of the ID differs from ours, complete immediately
                   with old/new. Otherwise, pend this IRP until an ID change occurs. */
                if (RtlCompareMemory(&InputGuid, &DriveExtension->DiskGuid, sizeof(GUID)) != sizeof(GUID))
                {
                    PMOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY_OUTPUT Output;
                    ULONG RequiredLength = sizeof(MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY_OUTPUT) + (2 * sizeof(GUID));
                    ULONG Offset;

                    if (OutputLength < RequiredLength)
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                        Information = RequiredLength;
                        break;
                    }

                    Output = Irp->AssociatedIrp.SystemBuffer;
                    RtlZeroMemory(Output, RequiredLength);
                    Output->Size = sizeof(*Output);
                    Offset = sizeof(*Output);
                    Output->OldUniqueIdOffset = (USHORT)Offset;
                    Output->OldUniqueIdLength = sizeof(GUID);
                    RtlCopyMemory((PUCHAR)Output + Offset, &InputGuid, sizeof(GUID));
                    Offset += sizeof(GUID);
                    Output->NewUniqueIdOffset = (USHORT)Offset;
                    Output->NewUniqueIdLength = sizeof(GUID);
                    RtlCopyMemory((PUCHAR)Output + Offset, &DriveExtension->DiskGuid, sizeof(GUID));
                    Offset += sizeof(GUID);

                    Status = STATUS_SUCCESS;
                    Information = Offset;
                    break;
                }
                else
                {
                    KIRQL oldIrql;

                    /* Only one pending notify at a time; cancel any previous */
                    IoAcquireCancelSpinLock(&oldIrql);
                    if (DriveExtension->PendingUniqueIdNotifyIrp)
                    {
                        PIRP OldIrp = DriveExtension->PendingUniqueIdNotifyIrp;
                        DriveExtension->PendingUniqueIdNotifyIrp = NULL;
                        if (IoSetCancelRoutine(OldIrp, NULL) != NULL)
                        {
                            IoReleaseCancelSpinLock(oldIrql);
                            /* Release remove lock and complete old */
                            IoReleaseRemoveLock(&DriveExtension->RemoveLock, OldIrp);
                            OldIrp->IoStatus.Status = STATUS_CANCELLED;
                            OldIrp->IoStatus.Information = 0;
                            IoCompleteRequest(OldIrp, IO_NO_INCREMENT);
                            IoAcquireCancelSpinLock(&oldIrql);
                        }
                        /* else: cancel routine is running; let it complete OldIrp */
                    }

                    /* Pend this IRP */
                    IoMarkIrpPending(Irp);
                    if (IoSetCancelRoutine(Irp, RamdiskCancelUniqueIdNotify) == NULL && Irp->Cancel)
                    {
                        /* Already canceled */
                        IoReleaseCancelSpinLock(oldIrql);
                        Status = STATUS_CANCELLED;
                        Information = 0;
                        break;
                    }

                    DriveExtension->PendingUniqueIdNotifyIrp = Irp;
                    IoReleaseCancelSpinLock(oldIrql);

                    Status = STATUS_PENDING;
                    Information = 0;
                    goto CompleteRequest; /* Will skip completion because PENDING */
                }
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_VOLUME_OFFLINE):
            {
                DriveExtension->VolumeOffline = TRUE;
                RamdiskPersistDiskState(DriveExtension);
                Status = STATUS_SUCCESS;
                Information = 0;
                break;
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_VOLUME_ONLINE):
            {
                DriveExtension->VolumeOffline = FALSE;
                RamdiskPersistDiskState(DriveExtension);
                Status = STATUS_SUCCESS;
                Information = 0;
                break;
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_SCSI_MINIPORT):
            {
                DbgPrintEx(DPFLTR_DEFAULT_ID,
                           DPFLTR_WARNING_LEVEL,
                           "RamdiskDeviceControl[%s]: IOCTL 0x%lx not yet supported\n",
                           DeviceTypeName,
                           IoControlCode);
                Status = STATUS_NOT_SUPPORTED;
                break;
            }

            case IOCTL_NORMALIZE_ACCESS(IOCTL_VOLUME_SET_GPT_ATTRIBUTES):
            {
                PVOLUME_SET_GPT_ATTRIBUTES_INFORMATION SetInformation;
                ULONGLONG SupportedMask;

                if (IoStackLocation->Parameters.DeviceIoControl.InputBufferLength < sizeof(VOLUME_SET_GPT_ATTRIBUTES_INFORMATION))
                {
                    Status = STATUS_BUFFER_TOO_SMALL;
                    break;
                }

                SetInformation = Irp->AssociatedIrp.SystemBuffer;

                SupportedMask = GPT_BASIC_DATA_ATTRIBUTE_READ_ONLY |
                                GPT_BASIC_DATA_ATTRIBUTE_HIDDEN |
                                GPT_BASIC_DATA_ATTRIBUTE_NO_DRIVE_LETTER;

                if ((SetInformation->GptAttributes & ~SupportedMask) != 0)
                {
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                RamdiskApplyGptAttributes(DriveExtension, SetInformation->GptAttributes);

                if (!SetInformation->RevertOnClose)
                {
                    RamdiskPersistDiskState(DriveExtension);
                }

                Status = STATUS_SUCCESS;
                Information = 0;
                break;
            }

            default:
            {
                /* Drive code not emulated */
                DbgPrintEx(DPFLTR_DEFAULT_ID,
                           DPFLTR_WARNING_LEVEL,
                           "RamdiskDeviceControl[%s]: unknown IOCTL 0x%lx\n",
                           DeviceTypeName,
                           IoControlCode);
                Status = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }
        }

        /* If requests drop down here, we just return them complete them */
        goto CompleteRequest;
    }

    /* Queue the request to our worker thread */
CallWorker:
    DbgPrintEx(DPFLTR_DEFAULT_ID,
               DPFLTR_INFO_LEVEL,
               "RamdiskDeviceControl[%s]: queueing IOCTL 0x%lx\n",
               DeviceTypeName,
               IoControlCode);
    Status = SendIrpToThread(DeviceObject, Irp);

CompleteRequest:
    if (Status != STATUS_PENDING)
    {
        /* Release the lock before completing non-pended IRPs */
        IoReleaseRemoveLock(&DeviceExtension->RemoveLock, Irp);
        /* Complete the request */
        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = Information;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
RamdiskQueryDeviceRelations(IN DEVICE_RELATION_TYPE Type,
                            IN PDEVICE_OBJECT DeviceObject,
                            IN PIRP Irp)
{
    PRAMDISK_BUS_EXTENSION DeviceExtension;
    PRAMDISK_DRIVE_EXTENSION DriveExtension;
    PDEVICE_RELATIONS DeviceRelations, OurDeviceRelations;
    ULONG Count, DiskCount, FinalCount;
    PLIST_ENTRY ListHead, NextEntry;
    RAMDISK_DEVICE_STATE State;

    /* Get the device extension and check if this is a drive */
    DeviceExtension = DeviceObject->DeviceExtension;
    if (DeviceExtension->Type == RamdiskDrive)
    {
        NTSTATUS Status;
        PDEVICE_RELATIONS DeviceRelations;

        /* We're a child device, only handle target device relations */
        if (Type != TargetDeviceRelation)
        {
            Status = Irp->IoStatus.Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
        }

        /* Allocate a buffer big enough to contain only one DO */
        DeviceRelations = ExAllocatePoolWithTag(PagedPool,
                                                sizeof(*DeviceRelations),
                                                'dmaR');
        if (DeviceRelations != NULL)
        {
            /* Reference the DO and add it to the buffer */
            ObReferenceObject(DeviceObject);
            DeviceRelations->Objects[0] = DeviceObject;
            DeviceRelations->Count = 1;
            Status = STATUS_SUCCESS;
        }
        else
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Return our processing & complete */
        Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    /* We don't handle anything but bus relations */
    if (Type != BusRelations) goto PassToNext;

    /* Acquire the disk list lock */
    KeEnterCriticalRegion();
    ExAcquireFastMutex(&DeviceExtension->DiskListLock);

    /* Did a device already fill relations? */
    DeviceRelations = (PDEVICE_RELATIONS)Irp->IoStatus.Information;
    if (DeviceRelations)
    {
        /* Use the data */
        Count = DeviceRelations->Count;
    }
    else
    {
        /* We're the first */
        Count = 0;
    }

    /* Now loop our drives */
    DiskCount = 0;
    ListHead = &DeviceExtension->DiskList;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* As long as it wasn't removed, count it in */
        DriveExtension = CONTAINING_RECORD(NextEntry,
                                           RAMDISK_DRIVE_EXTENSION,
                                           DiskList);
        if (DriveExtension->State < RamdiskStateBusRemoved) DiskCount++;

        /* Move to the next one */
        NextEntry = NextEntry->Flink;
    }

    /* Now we know our final count */
    FinalCount = Count + DiskCount;

    /* Allocate the structure */
    OurDeviceRelations = ExAllocatePoolWithTag(PagedPool,
                                               FIELD_OFFSET(DEVICE_RELATIONS,
                                                            Objects) +
                                               FinalCount *
                                               sizeof(PDEVICE_OBJECT),
                                               'dmaR');
    if (!OurDeviceRelations)
    {
        /* Fail */
        ExReleaseFastMutex(&DeviceExtension->DiskListLock);
        KeLeaveCriticalRegion();
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Check if we already had some relations */
    if (Count)
    {
        /* Copy them in */
        RtlCopyMemory(OurDeviceRelations->Objects,
                      DeviceRelations->Objects,
                      Count * sizeof(PDEVICE_OBJECT));
    }

    /* Save the count */
    OurDeviceRelations->Count = FinalCount;

    /* Now loop our drives again */
    ListHead = &DeviceExtension->DiskList;
    NextEntry = ListHead->Flink;
    /* Initialize output index */
    ULONG OutputIndex = Count;

    while (NextEntry != ListHead)
    {
        /* Get the drive state */
        DriveExtension = CONTAINING_RECORD(NextEntry,
                                           RAMDISK_DRIVE_EXTENSION,
                                           DiskList);
        State = DriveExtension->State;

        /* If it was removed or enumerated, we don't touch the device object */
        if (State >= RamdiskStateBusRemoved)
        {
            /* If it was removed, we still have to keep track of this though */
            if (State == RamdiskStateBusRemoved)
            {
                /* Mark it as enumerated now, but don't actually reference it */
                DriveExtension->State = RamdiskStateEnumerated;
            }
        }
        else
        {
            /* First time it's enumerated, reference and return the PDO */
            ObReferenceObject(DriveExtension->PhysicalDeviceObject);
            OurDeviceRelations->Objects[OutputIndex++] = DriveExtension->PhysicalDeviceObject;
        }

        if (DriveExtension->State < RamdiskStateBusRemoved) DiskCount++;

        /* Move to the next one */
        NextEntry = NextEntry->Flink;
    }

    /* Update the final count with the actual number of objects added */
    OurDeviceRelations->Count = OutputIndex;

    /* Release the lock */
    ExReleaseFastMutex(&DeviceExtension->DiskListLock);
    KeLeaveCriticalRegion();

    /* Cleanup old relations */
    if (DeviceRelations) ExFreePool(DeviceRelations);

    /* Complete our IRP */
    Irp->IoStatus.Information = (ULONG_PTR)OurDeviceRelations;
    Irp->IoStatus.Status = STATUS_SUCCESS;

    /* Pass to the next driver */
PassToNext:
    IoCopyCurrentIrpStackLocationToNext(Irp);
    return IoCallDriver(DeviceExtension->AttachedDevice, Irp);
}

NTSTATUS
NTAPI
RamdiskDeleteDiskDevice(IN PDEVICE_OBJECT DeviceObject,
                        IN PIRP Irp)
{
    PRAMDISK_DRIVE_EXTENSION DriveExtension;
    PRAMDISK_BUS_EXTENSION BusExtension = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID RemoveContext;

    /* Validate the device object */
    if (DeviceObject == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    DriveExtension = DeviceObject->DeviceExtension;
    if (DriveExtension->Type != RamdiskDrive)
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if (DriveExtension->DeviceObject && DriveExtension->DeviceObject->DeviceExtension)
    {
        BusExtension = DriveExtension->DeviceObject->DeviceExtension;
    }

    /* Remove from the bus list */
    if (BusExtension)
    {
        KeEnterCriticalRegion();
        ExAcquireFastMutex(&BusExtension->DiskListLock);
        if (!IsListEmpty(&DriveExtension->DiskList))
        {
            RemoveEntryList(&DriveExtension->DiskList);
            InitializeListHead(&DriveExtension->DiskList);
        }
        ExReleaseFastMutex(&BusExtension->DiskListLock);
        KeLeaveCriticalRegion();
    }

    /* Disable any published interfaces */
    if (DriveExtension->DriveDeviceName.Buffer)
    {
        NTSTATUS interfaceStatus = IoSetDeviceInterfaceState(&DriveExtension->DriveDeviceName, FALSE);
        UNREFERENCED_PARAMETER(interfaceStatus);
        RtlFreeUnicodeString(&DriveExtension->DriveDeviceName);
    }

    /* Drop the DOS symbolic link if one was created */
    if (DriveExtension->SymbolicLinkName.Buffer)
    {
        IoDeleteSymbolicLink(&DriveExtension->SymbolicLinkName);
        ExFreePoolWithTag(DriveExtension->SymbolicLinkName.Buffer, 'dmaR');
        DriveExtension->SymbolicLinkName.Buffer = NULL;
    }

    /* Release the device name that backs IOCTL_MOUNTDEV_QUERY_DEVICE_NAME */
    if (DriveExtension->DeviceObjectName.Buffer)
    {
        ExFreePoolWithTag(DriveExtension->DeviceObjectName.Buffer, 'dmaR');
        DriveExtension->DeviceObjectName.Buffer = NULL;
    }

    /* Free the persistent GUID string */
    if (DriveExtension->GuidString.Buffer)
    {
        RtlFreeUnicodeString(&DriveExtension->GuidString);
    }

    /* Cancel any pending unique ID notify IRP before releasing the remove lock */
    {
        KIRQL oldIrql;
        IoAcquireCancelSpinLock(&oldIrql);
        if (DriveExtension->PendingUniqueIdNotifyIrp)
        {
            PIRP PendingIrp = DriveExtension->PendingUniqueIdNotifyIrp;
            DriveExtension->PendingUniqueIdNotifyIrp = NULL;
            if (IoSetCancelRoutine(PendingIrp, NULL) != NULL)
            {
                IoReleaseCancelSpinLock(oldIrql);
                IoReleaseRemoveLock(&DriveExtension->RemoveLock, PendingIrp);
                PendingIrp->IoStatus.Status = STATUS_CANCELLED;
                PendingIrp->IoStatus.Information = 0;
                IoCompleteRequest(PendingIrp, IO_NO_INCREMENT);
            }
            else
            {
                /* Cancel routine will complete it */
                IoReleaseCancelSpinLock(oldIrql);
            }
        }
        else
        {
            IoReleaseCancelSpinLock(oldIrql);
        }
    }

    RamdiskReleaseBootPfnTable(DriveExtension);

    DriveExtension->VolumeOffline = FALSE;
    DriveExtension->MountdevLinkCount = 0;
    DriveExtension->State = RamdiskStateRemoved;

    RamdiskPersistDiskState(DriveExtension);

    /* Release the remove lock now that no more IRPs will arrive */
    RemoveContext = Irp ? (PVOID)Irp : (PVOID)DeviceObject;
    IoReleaseRemoveLockAndWait(&DriveExtension->RemoveLock, RemoveContext);

    /* Delete the device object */
    IoDeleteDevice(DeviceObject);

    return Status;
}

NTSTATUS
NTAPI
RamdiskRemoveBusDevice(IN PDEVICE_OBJECT DeviceObject,
                       IN PIRP Irp)
{
    NTSTATUS Status;
    PLIST_ENTRY ListHead, NextEntry;
    PRAMDISK_BUS_EXTENSION DeviceExtension;
    PRAMDISK_DRIVE_EXTENSION DriveExtension;

    DeviceExtension = DeviceObject->DeviceExtension;

    /* Acquire disks list lock */
    KeEnterCriticalRegion();
    ExAcquireFastMutex(&DeviceExtension->DiskListLock);

    /* Loop over drives -- restart from list head after each deletion.
     * Release the list lock before calling RamdiskDeleteDiskDevice,
     * because it also acquires DiskListLock internally. */
    ListHead = &DeviceExtension->DiskList;
    while (!IsListEmpty(ListHead))
    {
        NextEntry = ListHead->Flink;
        DriveExtension = CONTAINING_RECORD(NextEntry,
                                           RAMDISK_DRIVE_EXTENSION,
                                           DiskList);

        /* Take a reference, then release the lock before deletion */
        IoAcquireRemoveLock(&DriveExtension->RemoveLock, NULL);
        ExReleaseFastMutex(&DeviceExtension->DiskListLock);
        KeLeaveCriticalRegion();

        /* Delete the disk (acquires DiskListLock internally) */
        RamdiskDeleteDiskDevice(DriveExtension->PhysicalDeviceObject, NULL);

        /* Reacquire the lock for the next iteration */
        KeEnterCriticalRegion();
        ExAcquireFastMutex(&DeviceExtension->DiskListLock);
    }

    /* Release disks list lock */
    ExReleaseFastMutex(&DeviceExtension->DiskListLock);
    KeLeaveCriticalRegion();

    /* Prepare to pass to the lower driver */
    IoSkipCurrentIrpStackLocation(Irp);
    /* Here everything went fine */
    Irp->IoStatus.Status = STATUS_SUCCESS;

    /* Call lower driver */
    Status = IoCallDriver(DeviceExtension->AttachedDevice, Irp);

    /* Update state */
    DeviceExtension->State = RamdiskStateBusRemoved;

    /* Release the lock and ensure that everyone has finished its job before
     * we continue. The lock has been acquired by the dispatcher */
    IoReleaseRemoveLockAndWait(&DeviceExtension->RemoveLock, Irp);

    /* If there's a drive name */
    if (DeviceExtension->DriveDeviceName.Buffer)
    {
        /* Inform it's going to be disabled and free the drive name */
        NTSTATUS status = IoSetDeviceInterfaceState(&DeviceExtension->DriveDeviceName, FALSE);
        UNREFERENCED_PARAMETER(status);
        RtlFreeUnicodeString(&DeviceExtension->DriveDeviceName);
    }

    /* Part from the stack, detach from lower device */
    IoDetachDevice(DeviceExtension->AttachedDevice);

    /* Finally, delete device */
    RamdiskBusFdo = NULL;
    IoDeleteDevice(DeviceObject);

    /* Return status from lower driver */
    return Status;
}

NTSTATUS
NTAPI
RamdiskQueryId(IN PRAMDISK_DRIVE_EXTENSION DriveExtension,
               IN PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStackLocation;
    PWSTR OutputString = NULL;
    ULONG StringLength;

    Status = STATUS_SUCCESS;
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    /* Get what is being queried */
    switch (IoStackLocation->Parameters.QueryId.IdType)
    {
        case BusQueryDeviceID:
        {
            /* Allocate a buffer long enough to receive Ramdisk\RamDisk in any case
             * In case we don't have RAMDISK_REGISTRY_DISK, we then need two more
             * chars to store Ramdisk\RamVolume instead */
            StringLength = 4 * (DriveExtension->DiskType != RAMDISK_REGISTRY_DISK) + sizeof(L"Ramdisk\\RamDisk");
            OutputString = ExAllocatePoolWithTag(PagedPool, StringLength, 'dmaR');
            if (OutputString == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            wcsncpy(OutputString, L"Ramdisk\\", StringLength / sizeof(WCHAR));
            if (DriveExtension->DiskType != RAMDISK_REGISTRY_DISK)
            {
                wcsncat(OutputString, L"RamVolume", StringLength / sizeof(WCHAR));
            }
            else
            {
                wcsncat(OutputString, L"RamDisk", StringLength / sizeof(WCHAR));
            }

            break;
        }

        case BusQueryHardwareIDs:
        {
            /* Allocate a buffer long enough to receive Ramdisk\RamDisk in any case
             * In case we don't have RAMDISK_REGISTRY_DISK, we then need two more
             * chars to store Ramdisk\RamVolume instead
             * We also need an extra char, because it is required that the string
             * is null-terminated twice */
            StringLength = 4 * (DriveExtension->DiskType != RAMDISK_REGISTRY_DISK) +
                           sizeof(UNICODE_NULL) + sizeof(L"Ramdisk\\RamDisk");
            OutputString = ExAllocatePoolWithTag(PagedPool, StringLength, 'dmaR');
            if (OutputString == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            wcsncpy(OutputString, L"Ramdisk\\", StringLength / sizeof(WCHAR));
            if (DriveExtension->DiskType != RAMDISK_REGISTRY_DISK)
            {
                wcsncat(OutputString, L"RamVolume", StringLength / sizeof(WCHAR));
            }
            else
            {
                wcsncat(OutputString, L"RamDisk", StringLength / sizeof(WCHAR));
            }
            OutputString[(StringLength / sizeof(WCHAR)) - 1] = UNICODE_NULL;

            break;
        }

        case BusQueryCompatibleIDs:
        {
            if (DriveExtension->DiskType != RAMDISK_REGISTRY_DISK)
            {
                Status = STATUS_INVALID_DEVICE_REQUEST;
                break;
            }

            StringLength = sizeof(L"GenDisk");
            OutputString = ExAllocatePoolWithTag(PagedPool, StringLength, 'dmaR');
            if (OutputString == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            wcsncpy(OutputString, L"GenDisk", StringLength / sizeof(WCHAR));
            OutputString[(StringLength / sizeof(WCHAR)) - 1] = UNICODE_NULL;

            break;
        }

        case BusQueryInstanceID:
        {
            OutputString = ExAllocatePoolWithTag(PagedPool,
                                                 DriveExtension->GuidString.MaximumLength,
                                                 'dmaR');
            if (OutputString == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            wcsncpy(OutputString,
                    DriveExtension->GuidString.Buffer,
                    DriveExtension->GuidString.MaximumLength / sizeof(WCHAR));

            break;
        }

        case BusQueryDeviceSerialNumber:
        case BusQueryContainerID:
        {
            /* Nothing to do */
            break;
        }
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = (ULONG_PTR)OutputString;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
NTAPI
RamdiskQueryCapabilities(IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStackLocation;
    PDEVICE_CAPABILITIES DeviceCapabilities;
    PRAMDISK_DRIVE_EXTENSION DriveExtension;

    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    DeviceCapabilities = IoStackLocation->Parameters.DeviceCapabilities.Capabilities;
    DriveExtension = DeviceObject->DeviceExtension;

    /* Validate our input buffer */
    if (DeviceCapabilities->Version != 1 ||
        DeviceCapabilities->Size < sizeof(DEVICE_CAPABILITIES))
    {
        Status = STATUS_UNSUCCESSFUL;
    }
    else
    {
        /* And set everything we know about our capabilities */
        DeviceCapabilities->Removable = MarkRamdisksAsRemovable;
        DeviceCapabilities->UniqueID = TRUE;
        DeviceCapabilities->SilentInstall = TRUE;
        DeviceCapabilities->RawDeviceOK = TRUE;
        DeviceCapabilities->SurpriseRemovalOK = (DriveExtension->DiskType != RAMDISK_REGISTRY_DISK);
        DeviceCapabilities->NoDisplayInUI = TRUE;
        Status = STATUS_SUCCESS;
    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
NTAPI
RamdiskQueryDeviceText(IN PRAMDISK_DRIVE_EXTENSION DriveExtension,
                       IN PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStackLocation;
    DEVICE_TEXT_TYPE DeviceTextType;
    PWSTR OutputString = NULL;

    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    DeviceTextType = IoStackLocation->Parameters.QueryDeviceText.DeviceTextType;
    Status = STATUS_SUCCESS;

    /* Just copy our constants, according to the input */
    switch (DeviceTextType)
    {
        case DeviceTextDescription:
        {
            OutputString = ExAllocatePoolWithTag(PagedPool, sizeof(L"RamDisk"), 'dmaR');
            if (OutputString == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            wcsncpy(OutputString, L"RamDisk", sizeof(L"RamDisk") / sizeof(WCHAR));

            break;
        }

        case DeviceTextLocationInformation:
        {
            OutputString = ExAllocatePoolWithTag(PagedPool, sizeof(L"RamDisk\\0"), 'dmaR');
            if (OutputString == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            wcsncpy(OutputString, L"RamDisk\\0", sizeof(L"RamDisk\\0") / sizeof(WCHAR));

            break;
        }
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = (ULONG_PTR)OutputString;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
NTAPI
RamdiskQueryBusInformation(IN PDEVICE_OBJECT DeviceObject,
                           IN PIRP Irp)
{
    PPNP_BUS_INFORMATION PnpBusInfo;
    NTSTATUS Status = STATUS_SUCCESS;

    /* Allocate output memory */
    PnpBusInfo = ExAllocatePoolWithTag(PagedPool, sizeof(*PnpBusInfo), 'dmaR');
    if (PnpBusInfo == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        /* Copy our bus GUID and set our legacy type */
        RtlCopyMemory(&PnpBusInfo->BusTypeGuid, &GUID_BUS_TYPE_RAMDISK, sizeof(GUID));
        PnpBusInfo->LegacyBusType = PNPBus;
        PnpBusInfo->BusNumber = 0;
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = (ULONG_PTR)PnpBusInfo;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
NTAPI
RamdiskIoCompletionRoutine(IN PDEVICE_OBJECT DeviceObject,
                           IN PIRP Irp,
                           IN PVOID Context)

{
    /* Just set the event to unlock caller */
    KeSetEvent((PKEVENT)Context, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
RamdiskPnp(IN PDEVICE_OBJECT DeviceObject,
           IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStackLocation;
    PRAMDISK_BUS_EXTENSION DeviceExtension;
    NTSTATUS Status;
    UCHAR Minor;
    KEVENT Event;

    /* Get the device extension and stack location */
    DeviceExtension = DeviceObject->DeviceExtension;
    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    Minor = IoStackLocation->MinorFunction;
    Irp->IoStatus.Information = 0;

    /* Check if the bus is removed */
    if (DeviceExtension->State == RamdiskStateBusRemoved)
    {
        /* Only remove-device and query-id are allowed */
        if ((Minor != IRP_MN_REMOVE_DEVICE) && (Minor != IRP_MN_QUERY_ID))
        {
            /* Fail anything else */
            Status = STATUS_NO_SUCH_DEVICE;
            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
        }
    }

    /* Acquire the remove lock */
    Status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS(Status))
    {
        /* Fail the IRP */
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    /* Default to existing status */
    Status = Irp->IoStatus.Status;

    /* Query the IRP type */
    switch (Minor)
    {
        case IRP_MN_START_DEVICE:
        {
            if (DeviceExtension->Type == RamdiskDrive)
            {
                ULONG ResultLength;
                DEVICE_INSTALL_STATE InstallState;
                PRAMDISK_DRIVE_EXTENSION DriveExtension = (PRAMDISK_DRIVE_EXTENSION)DeviceExtension;

                /* Free any bootstrap name (e.g. \Device\Ramdisk{GUID}) and register the interface */
                if (DriveExtension->DriveDeviceName.Buffer)
                {
                    ExFreePool(DriveExtension->DriveDeviceName.Buffer);
                    RtlZeroMemory(&DriveExtension->DriveDeviceName, sizeof(DriveExtension->DriveDeviceName));
                }

                if (DriveExtension->DiskType == RAMDISK_BOOT_DISK)
                {
                    /* Expose a dedicated boot-ramdisk device interface for kernel binding. */
                    Status = IoRegisterDeviceInterface(DeviceObject,
                                                       &GUID_DEVINTERFACE_REACTOS_BOOT_RAMDISK,
                                                       NULL,
                                                       &DriveExtension->DriveDeviceName);
                }
                else if (DriveExtension->DiskType != RAMDISK_REGISTRY_DISK)
                {
                    Status = IoRegisterDeviceInterface(DeviceObject,
                                                       &GUID_DEVINTERFACE_VOLUME,
                                                       NULL,
                                                       &DriveExtension->DriveDeviceName);
                }
                else
                {
                    Status = IoRegisterDeviceInterface(DeviceObject,
                                                       &RamdiskDiskInterface,
                                                       NULL,
                                                       &DriveExtension->DriveDeviceName);
                }

                /* If we were asked not to assign a drive letter or if getting
                 * a name failed, just return saying we're now started */
                if (DriveExtension->DiskOptions.NoDriveLetter ||
                    DriveExtension->DriveDeviceName.Buffer == NULL)
                {
                    DriveExtension->State = RamdiskStateStarted;
                    Irp->IoStatus.Status = Status;
                    break;
                }

                /* Now get our installation state */
                Status = IoGetDeviceProperty(DeviceObject,
                                             DevicePropertyInstallState,
                                             sizeof(InstallState),
                                             &InstallState,
                                             &ResultLength);
                /* If querying the information failed, assume success */
                if (!NT_SUCCESS(Status))
                {
                    InstallState = InstallStateInstalled;
                }

                /* If we were properly installed, then, enable the interface */
                if (InstallState == InstallStateInstalled)
                {
                    Status = IoSetDeviceInterfaceState(&DriveExtension->DriveDeviceName, TRUE);
                }

                /* We're fine & up */
                DriveExtension->State = RamdiskStateStarted;
                Irp->IoStatus.Status = Status;
                break;
            }

            /* Prepare next stack to pass it down */
            IoCopyCurrentIrpStackLocationToNext(Irp);

            /* Initialize our notification event & our completion routine */
            KeInitializeEvent(&Event, NotificationEvent, FALSE);
            IoSetCompletionRoutine(Irp, RamdiskIoCompletionRoutine, &Event, TRUE, TRUE, TRUE);

            /* Call lower driver */
            Status = IoCallDriver(DeviceExtension->AttachedDevice, Irp);
            if (Status == STATUS_PENDING)
            {
                KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
                Status = Irp->IoStatus.Status;
            }

            /* If it succeed to start then enable ourselves and we're up! */
            if (NT_SUCCESS(Status))
            {
                Status = IoSetDeviceInterfaceState(&DeviceExtension->DriveDeviceName, TRUE);
                DeviceExtension->State = RamdiskStateStarted;
            }

            Irp->IoStatus.Status = Status;
            break;
        }

        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE:
        {
            /* Flag that we are preparing for a pause */
            if (DeviceExtension->State == RamdiskStateStarted)
            {
                DeviceExtension->State = RamdiskStatePaused;
            }

            Status = STATUS_SUCCESS;
            Irp->IoStatus.Status = Status;
            break;
        }

        case IRP_MN_CANCEL_STOP_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:
        {
            /* Undo any pending pause */
            DeviceExtension->State = RamdiskStateStarted;

            Status = STATUS_SUCCESS;
            Irp->IoStatus.Status = Status;
            break;
        }

        case IRP_MN_STOP_DEVICE:
        {
            /* Disable the interface while we are stopped */
            if (DeviceExtension->Type == RamdiskDrive)
            {
                PRAMDISK_DRIVE_EXTENSION DriveExtension = (PRAMDISK_DRIVE_EXTENSION)DeviceExtension;

                if (!DriveExtension->DiskOptions.NoDriveLetter &&
                    DriveExtension->DriveDeviceName.Buffer != NULL)
                {
                    NTSTATUS setStatus = IoSetDeviceInterfaceState(&DriveExtension->DriveDeviceName, FALSE);
                    UNREFERENCED_PARAMETER(setStatus);
                }
            }
            else if (DeviceExtension->DriveDeviceName.Buffer != NULL)
            {
                NTSTATUS setStatus = IoSetDeviceInterfaceState(&DeviceExtension->DriveDeviceName, FALSE);
                UNREFERENCED_PARAMETER(setStatus);
            }

            DeviceExtension->State = RamdiskStateStopped;

            Status = STATUS_SUCCESS;
            Irp->IoStatus.Status = Status;
            break;
        }

        case IRP_MN_REMOVE_DEVICE:
        {
            /* Remove the proper device */
            if (DeviceExtension->Type == RamdiskBus)
            {
                Status = RamdiskRemoveBusDevice(DeviceObject, Irp);

                /* Return here, lower device has already been called
                 * And remove lock released. This is needed by the function. */
                return Status;
            }
            else
            {
                Status = RamdiskDeleteDiskDevice(DeviceObject, Irp);

                /* Complete the IRP here and return
                 * Here again we don't have to release remove lock
                 * This has already been done by the function. */
                Irp->IoStatus.Status = Status;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return Status;
            }
        }

        case IRP_MN_SURPRISE_REMOVAL:
        {
            if (DeviceExtension->Type == RamdiskDrive)
            {
                PRAMDISK_DRIVE_EXTENSION DriveExtension = (PRAMDISK_DRIVE_EXTENSION)DeviceExtension;

                if (!DriveExtension->DiskOptions.NoDriveLetter &&
                    DriveExtension->DriveDeviceName.Buffer != NULL)
                {
                    NTSTATUS setStatus = IoSetDeviceInterfaceState(&DriveExtension->DriveDeviceName, FALSE);
                    UNREFERENCED_PARAMETER(setStatus);
                }
            }
            else if (DeviceExtension->DriveDeviceName.Buffer != NULL)
            {
                NTSTATUS setStatus = IoSetDeviceInterfaceState(&DeviceExtension->DriveDeviceName, FALSE);
                UNREFERENCED_PARAMETER(setStatus);
            }

            DeviceExtension->State = RamdiskStateRemoved;

            Status = STATUS_SUCCESS;
            Irp->IoStatus.Status = Status;
            break;
        }

        case IRP_MN_QUERY_ID:
        {
            /* Are we a drive? */
            if (DeviceExtension->Type == RamdiskDrive)
            {
                /* Handler completes the IRP internally */
                Status = RamdiskQueryId((PRAMDISK_DRIVE_EXTENSION)DeviceExtension, Irp);
                goto ReleaseAndReturn;
            }
            break;
        }

        case IRP_MN_QUERY_BUS_INFORMATION:
        {
            /* Are we a drive? */
            if (DeviceExtension->Type == RamdiskDrive)
            {
                /* Handler completes the IRP internally */
                Status = RamdiskQueryBusInformation(DeviceObject, Irp);
                goto ReleaseAndReturn;
            }
            break;
        }

        case IRP_MN_EJECT:
        {
            Status = STATUS_NOT_SUPPORTED;
            Irp->IoStatus.Status = Status;
            break;
        }

        case IRP_MN_QUERY_DEVICE_TEXT:
        {
            /* Are we a drive? */
            if (DeviceExtension->Type == RamdiskDrive)
            {
                /* Handler completes the IRP internally */
                Status = RamdiskQueryDeviceText((PRAMDISK_DRIVE_EXTENSION)DeviceExtension, Irp);
                goto ReleaseAndReturn;
            }
            break;
        }

        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            /* Call our main routine */
            Status = RamdiskQueryDeviceRelations(IoStackLocation->
                                                 Parameters.
                                                 QueryDeviceRelations.Type,
                                                 DeviceObject,
                                                 Irp);
            goto ReleaseAndReturn;
        }

        case IRP_MN_QUERY_CAPABILITIES:
        {
            /* Are we a drive? */
            if (DeviceExtension->Type == RamdiskDrive)
            {
                Status = RamdiskQueryCapabilities(DeviceObject, Irp);
            }
            break;
        }

        case IRP_MN_QUERY_RESOURCES:
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
        {
            /* Complete immediately without touching it */
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            goto ReleaseAndReturn;
        }

        case IRP_MN_QUERY_PNP_DEVICE_STATE:
        {
            Irp->IoStatus.Information = 0;
            Status = STATUS_SUCCESS;
            Irp->IoStatus.Status = Status;
            break;
        }

        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
        {
            /* We do not need to react; just report success */
            Status = STATUS_SUCCESS;
            Irp->IoStatus.Status = Status;
            break;
        }

        case IRP_MN_QUERY_INTERFACE:
        case IRP_MN_READ_CONFIG:
        case IRP_MN_WRITE_CONFIG:
        case IRP_MN_SET_LOCK:
        {
            Status = STATUS_NOT_SUPPORTED;
            Irp->IoStatus.Status = Status;
            break;
        }

        default:
            if (DeviceExtension->Type != RamdiskBus)
            {
                DPRINT1("Illegal IRP: %lx\n", Minor);
            }
            break;
    }

    /* Are we the bus? */
    if (DeviceExtension->Type == RamdiskBus)
    {
        /* Do we have an attached device? */
        if (DeviceExtension->AttachedDevice)
        {
            /* Forward the IRP */
            IoSkipCurrentIrpStackLocation(Irp);
            Status = IoCallDriver(DeviceExtension->AttachedDevice, Irp);
        }
    }

    /* Release the lock and return status */
ReleaseAndReturn:
    IoReleaseRemoveLock(&DeviceExtension->RemoveLock, Irp);
    return Status;
}

NTSTATUS
NTAPI
RamdiskPower(IN PDEVICE_OBJECT DeviceObject,
             IN PIRP Irp)
{
    NTSTATUS Status;
    PRAMDISK_BUS_EXTENSION DeviceExtension;

    DeviceExtension = DeviceObject->DeviceExtension;

    /* Check if this is a bus FDO or a disk PDO */
    if (DeviceExtension != NULL && DeviceExtension->Type == RamdiskBus)
    {
        /* FDO: allow the next power IRP to proceed before passing down */
        PoStartNextPowerIrp(Irp);

        /* Bus FDO: forward to lower driver */
        /* Device has not been removed yet, so pass to the attached/lower driver */
        if (DeviceExtension->State < RamdiskStateBusRemoved)
        {
            IoSkipCurrentIrpStackLocation(Irp);
            return PoCallDriver(DeviceExtension->AttachedDevice, Irp);
        }
        /* Otherwise, simply complete the IRP notifying that deletion is pending */
        else
        {
            Irp->IoStatus.Status = STATUS_DELETE_PENDING;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_DELETE_PENDING;
        }
    }
    else
    {
        /* PDO (disk device): handle power locally */
        PoStartNextPowerIrp(Irp);
        /* We don't need to do anything special for RAM disks - just succeed all power requests */
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Status = STATUS_SUCCESS;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }
}

NTSTATUS
NTAPI
RamdiskSystemControl(IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp)
{
    NTSTATUS Status;
    PRAMDISK_BUS_EXTENSION DeviceExtension;

    DeviceExtension = DeviceObject->DeviceExtension;

    /* If we have a device extension, forward the IRP to the attached device */
    if (DeviceExtension != NULL)
    {
        IoSkipCurrentIrpStackLocation(Irp);
        Status = IoCallDriver(DeviceExtension->AttachedDevice, Irp);
    }
    /* Otherwise just complete the request
     * And return the status with which we complete it */
    else
    {
        Status = Irp->IoStatus.Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return Status;
}

NTSTATUS
NTAPI
RamdiskScsi(IN PDEVICE_OBJECT DeviceObject,
            IN PIRP Irp)
{
    NTSTATUS Status;
    PRAMDISK_BUS_EXTENSION DeviceExtension;

    DeviceExtension = DeviceObject->DeviceExtension;

    /* Having a proper device is mandatory */
    if (DeviceExtension->State > RamdiskStateStopped)
    {
        Status = STATUS_DEVICE_DOES_NOT_EXIST;
        goto CompleteIRP;
    }

    /* Acquire the remove lock */
    Status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS(Status))
    {
        goto CompleteIRP;
    }

    /* Queue the IRP for worker */
    Status = SendIrpToThread(DeviceObject, Irp);
    if (Status != STATUS_PENDING)
    {
        goto CompleteIRP;
    }

    /* Release the remove lock */
    IoReleaseRemoveLock(&DeviceExtension->RemoveLock, Irp);
    goto Quit;

CompleteIRP:
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

Quit:
    return Status;
}

NTSTATUS
NTAPI
RamdiskFlushBuffers(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp)
{
    NTSTATUS Status;
    PRAMDISK_DRIVE_EXTENSION DeviceExtension;

    DeviceExtension = DeviceObject->DeviceExtension;

    /* Ensure we have drive extension
     * Only perform flush on disks that have been created
     * from registry entries */
    if (DeviceExtension->Type != RamdiskDrive ||
        DeviceExtension->DiskType > RAMDISK_MEMORY_MAPPED_DISK)
    {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
    }

    /* Queue the IRP for worker */
    Status = SendIrpToThread(DeviceObject, Irp);
    if (Status != STATUS_PENDING)
    {
        /* Queuing failed - complete the IRP and return failure */
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return Status;
}

VOID
NTAPI
RamdiskUnload(IN PDRIVER_OBJECT DriverObject)
{
    /* Just release registry path if previously allocated */
    if (DriverRegistryPath.Buffer)
    {
        ExFreePoolWithTag(DriverRegistryPath.Buffer, 'dmaR');
    }
}

NTSTATUS
NTAPI
RamdiskAddDevice(IN PDRIVER_OBJECT DriverObject,
                 IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    PRAMDISK_BUS_EXTENSION DeviceExtension;
    PDEVICE_OBJECT AttachedDevice;
    NTSTATUS Status;
    UNICODE_STRING DeviceName;
    PDEVICE_OBJECT DeviceObject;

    /* Only create the bus FDO once */
    if (RamdiskBusFdo) return STATUS_DEVICE_ALREADY_ATTACHED;

    /* Create the bus FDO */
    RtlInitUnicodeString(&DeviceName, L"\\Device\\Ramdisk");
    Status = IoCreateDevice(DriverObject,
                            sizeof(RAMDISK_BUS_EXTENSION),
                            &DeviceName,
                            FILE_DEVICE_BUS_EXTENDER,
                            FILE_DEVICE_SECURE_OPEN,
                            0,
                            &DeviceObject);
    if (NT_SUCCESS(Status))
    {
        /* Initialize the bus FDO extension */
        DeviceExtension = DeviceObject->DeviceExtension;
        RtlZeroMemory(DeviceExtension, sizeof(*DeviceExtension));

        /* Set bus FDO flags */
        DeviceObject->Flags |= DO_POWER_PAGABLE | DO_DIRECT_IO;

        /* Setup the bus FDO extension */
        DeviceExtension->Type = RamdiskBus;
        ExInitializeFastMutex(&DeviceExtension->DiskListLock);
        IoInitializeRemoveLock(&DeviceExtension->RemoveLock, 'dmaR', 1, 0);
        InitializeListHead(&DeviceExtension->DiskList);
        DeviceExtension->PhysicalDeviceObject = PhysicalDeviceObject;
        DeviceExtension->DeviceObject = DeviceObject;

        /* Register the RAM disk device interface */
        Status = IoRegisterDeviceInterface(PhysicalDeviceObject,
                                           &RamdiskBusInterface,
                                           NULL,
                                           &DeviceExtension->BusDeviceName);
        if (!NT_SUCCESS(Status))
        {
            /* Fail */
            IoDeleteDevice(DeviceObject);
            return Status;
        }

        /* Attach us to the device stack */
        AttachedDevice = IoAttachDeviceToDeviceStack(DeviceObject,
                                                     PhysicalDeviceObject);
        DeviceExtension->AttachedDevice = AttachedDevice;
        if (!AttachedDevice)
        {
            /* Fail */
            NTSTATUS interfaceStatus = IoSetDeviceInterfaceState(&DeviceExtension->BusDeviceName, 0);
            UNREFERENCED_PARAMETER(interfaceStatus);
            RtlFreeUnicodeString(&DeviceExtension->BusDeviceName);
            IoDeleteDevice(DeviceObject);
            return STATUS_NO_SUCH_DEVICE;
        }

        /* Bus FDO is initialized */
        RamdiskBusFdo = DeviceObject;

        /* Loop for loader block */
        if (KeLoaderBlock)
        {
            /* Are we being booted from setup? Not yet supported */
            if (KeLoaderBlock->SetupLdrBlock)
                DPRINT1("FIXME: RamdiskAddDevice is UNSUPPORTED when being started from SETUPLDR!\n");
            // ASSERT(!KeLoaderBlock->SetupLdrBlock);
        }

        /* All done */
        DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
        Status = STATUS_SUCCESS;
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    PCHAR BootDeviceName, CommandLine;
    PDEVICE_OBJECT PhysicalDeviceObject = NULL;
    NTSTATUS Status;
    DPRINT("RAM Disk Driver Initialized\n");
    DPRINT1("RAMDISK DriverEntry: DriverObject=%p RegistryPath=%wZ\n",
            DriverObject,
            RegistryPath);

    /* Save the registry path */
    DriverRegistryPath.MaximumLength = RegistryPath->Length + sizeof(UNICODE_NULL);
    DriverRegistryPath.Buffer = ExAllocatePoolWithTag(PagedPool,
                                                      DriverRegistryPath.MaximumLength,
                                                      'dmaR');
    if (!DriverRegistryPath.Buffer)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlCopyUnicodeString(&DriverRegistryPath, RegistryPath);

    /* Query ramdisk parameters */
    QueryParameters(&DriverRegistryPath);

#if DBG
    RamdiskAssertIoctlCoverage();
#endif

    /* Set device routines */
    DriverObject->MajorFunction[IRP_MJ_CREATE] = RamdiskOpenClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = RamdiskOpenClose;
    DriverObject->MajorFunction[IRP_MJ_READ] = RamdiskReadWrite;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = RamdiskReadWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = RamdiskDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_PNP] = RamdiskPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = RamdiskPower;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = RamdiskSystemControl;
    DriverObject->MajorFunction[IRP_MJ_SCSI] = RamdiskScsi;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = RamdiskFlushBuffers;
    DriverObject->DriverExtension->AddDevice = RamdiskAddDevice;
    DriverObject->DriverUnload = RamdiskUnload;

    /* Check for a loader block */
    if (KeLoaderBlock)
    {
        /* Get the boot device name */
        BootDeviceName = KeLoaderBlock->ArcBootDeviceName;
        if (BootDeviceName)
        {
            /* Check if we're booting from ramdisk */
            if ((strlen(BootDeviceName) >= 10) &&
                !(_strnicmp(BootDeviceName, "ramdisk(0)", 10)))
            {
                /* We'll have to tell the PnP Manager */
                ReportDetectedDevice = TRUE;

                /* Check for a command line */
                CommandLine = KeLoaderBlock->LoadOptions;
                if (CommandLine)
                {
                    /* Check if this is an ISO boot */
                    if (strstr(CommandLine, "RDEXPORTASCD"))
                    {
                        /* Remember for later */
                        ExportBootDiskAsCd = TRUE;
                    }

                    /* Check if this is PE boot */
                    if (strstr(CommandLine, "MININT"))
                    {
                        /* Remember for later */
                        IsWinPEBoot = TRUE;
                    }
                }
            }

        }
    }

    /* Installing from Ramdisk isn't supported yet */
    if (KeLoaderBlock->SetupLdrBlock)
        DPRINT1("FIXME: Installing from RamDisk is UNSUPPORTED!\n");
    // ASSERT(!KeLoaderBlock->SetupLdrBlock);

    /* Are we reporting the device */
    if (ReportDetectedDevice)
    {
        /* Do it */
        Status = IoReportDetectedDevice(DriverObject,
                                        InterfaceTypeUndefined,
                                        0xFFFFFFFF,
                                        0xFFFFFFFF,
                                        NULL,
                                        NULL,
                                        0,
                                        &PhysicalDeviceObject);
        if (NT_SUCCESS(Status))
        {
            /* Create the device object */
            Status = RamdiskAddDevice(DriverObject, PhysicalDeviceObject);
            if (NT_SUCCESS(Status))
            {
                /* We are done */
                PhysicalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
                Status = STATUS_SUCCESS;
            }
        }
    }
    else
    {
        /* Done */
        Status = STATUS_SUCCESS;
    }

    /* Done */
    return Status;
}
