/*
 * PROJECT:     FreeLoader UEFI Support
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     UEFI disk enumeration and ARC name bridge (CD + HDD, partition index)
 * COPYRIGHT:   Copyright 2025 Ahmed ARIF
 */

#include <uefildr.h>
#include <freeldr.h>
#include <arcname.h>
#include "ntldr/winldr.h"
#include <disk.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(WARNING);

/* -------------------------------------------------------------------------- */
/* Local GUIDs and protocol symbols                                           */
/* -------------------------------------------------------------------------- */

EFI_GUID gEfiBlockIoProtocolGuid     = EFI_BLOCK_IO_PROTOCOL_GUID;
EFI_GUID gEfiLoadedImageProtocolGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
EFI_GUID EfiDevicePathProtocol       = EFI_DEVICE_PATH_PROTOCOL_GUID;

/* -------------------------------------------------------------------------- */
/* External symbols                                                           */
/* -------------------------------------------------------------------------- */

extern EFI_SYSTEM_TABLE*    GlobalSystemTable;
extern EFI_HANDLE           GlobalImageHandle;
extern PLOADER_SYSTEM_BLOCK WinLdrSystemBlock;

/* From archwsup.c - existing ARC disk registry */
extern VOID  AddReactOSArcDiskInfo(IN PSTR ArcName,
                                   IN ULONG Signature,
                                   IN ULONG Checksum,
                                   IN BOOLEAN ValidPartitionTable);
extern ULONG ArcGetDiskCount(VOID);
extern PARC_DISK_SIGNATURE_EX ArcGetDiskInfo(ULONG Index);

/* -------------------------------------------------------------------------- */
/* Local types and state                                                      */
/* -------------------------------------------------------------------------- */

#define TAG_UEFI_DISK 'kDfU'  /* "UefD" */
#define TAG_DP_WORK   'pDvU'  /* "UDvp" */
#define TAG_PARTLIST  'rPvU'  /* "UVpr" */
#define TAG_SIGEX     'giSD'

typedef struct _UEFI_DISK_HANDLE_ENTRY
{
    EFI_HANDLE             Handle;
    EFI_BLOCK_IO_PROTOCOL* BlockIo;
    ULONG                  DiskNumber; /* rdisk index */
} UEFI_DISK_HANDLE_ENTRY, *PUEFI_DISK_HANDLE_ENTRY;

/* HDD/SSD physical handles (non-partition) */
static UEFI_DISK_HANDLE_ENTRY* UefiDiskHandles     = NULL;
static UINTN                   UefiDiskHandleCount = 0;

/* CD-ROM physical handles (separate index space for cdrom(#)) */
static EFI_HANDLE*             UefiCdromHandles    = NULL;
static ULONG                   UefiCdromCount      = 0;

/* -------------------------------------------------------------------------- */
/* Minimal Device Path helpers                                                */
/* -------------------------------------------------------------------------- */

static UINTN
UefiGetDevicePathSize(IN EFI_DEVICE_PATH_PROTOCOL* Dp)
{
    if (!Dp) return 0;
    UINTN sz = 0;
    EFI_DEVICE_PATH_PROTOCOL* Node = Dp;
    while (!IsDevicePathEnd(Node))
    {
        sz += DevicePathNodeLength(Node);
        Node = NextDevicePathNode(Node);
    }
    /* add end node */
    sz += sizeof(EFI_DEVICE_PATH_PROTOCOL);
    return sz;
}

static VOID
UefiSetDevicePathEndNode(OUT EFI_DEVICE_PATH_PROTOCOL* Node)
{
    if (!Node) return;
    Node->Type    = END_DEVICE_PATH_TYPE;
    Node->SubType = END_ENTIRE_DEVICE_PATH_SUBTYPE;
    /* 16-bit length = 4 */
    Node->Length[0] = 4;
    Node->Length[1] = 0;
}

static BOOLEAN
IsCdRomHandle(IN EFI_HANDLE Handle)
{
    EFI_DEVICE_PATH_PROTOCOL* Dp = NULL;
    if (EFI_ERROR(GlobalSystemTable->BootServices->HandleProtocol(
            Handle, &EfiDevicePathProtocol, (VOID**)&Dp)) || !Dp)
        return FALSE;

    EFI_DEVICE_PATH_PROTOCOL* Node = Dp;
    while (!IsDevicePathEnd(Node))
    {
        if (Node->Type == MEDIA_DEVICE_PATH && Node->SubType == MEDIA_CDROM_DP)
            return TRUE;
        Node = NextDevicePathNode(Node);
    }
    return FALSE;
}

/* Extract starting LBA from a partition handle’s device path (MEDIA_HARDDRIVE_DP). */
static BOOLEAN
GetPartitionStartLbaFromHandle(IN EFI_HANDLE PartHandle, OUT UINT64* OutLbaStart)
{
    EFI_DEVICE_PATH_PROTOCOL* Dp = NULL;
    if (OutLbaStart) *OutLbaStart = 0;

    if (EFI_ERROR(GlobalSystemTable->BootServices->HandleProtocol(
            PartHandle, &EfiDevicePathProtocol, (VOID**)&Dp)) || !Dp)
        return FALSE;

    EFI_DEVICE_PATH_PROTOCOL* Node = Dp;
    while (!IsDevicePathEnd(Node))
    {
        if ((Node->Type == MEDIA_DEVICE_PATH) && (Node->SubType == MEDIA_HARDDRIVE_DP))
        {
            HARDDRIVE_DEVICE_PATH* Hd = (HARDDRIVE_DEVICE_PATH*)Node;
            if (OutLbaStart) *OutLbaStart = Hd->PartitionStart;
            return TRUE;
        }
        Node = NextDevicePathNode(Node);
    }
    return FALSE;
}

/* Find the physical (non-partition) disk handle that is parent of a partition handle. */
static EFI_HANDLE
FindParentDiskHandle(IN EFI_HANDLE PartHandle)
{
    EFI_DEVICE_PATH_PROTOCOL* Dp = NULL;
    EFI_DEVICE_PATH_PROTOCOL* Work = NULL;
    EFI_STATUS Status;

    if (EFI_ERROR(GlobalSystemTable->BootServices->HandleProtocol(
            PartHandle, &EfiDevicePathProtocol, (VOID**)&Dp)) || !Dp)
        return NULL;

    UINTN Sz = UefiGetDevicePathSize(Dp);
    Work = (EFI_DEVICE_PATH_PROTOCOL*)FrLdrHeapAlloc(Sz, TAG_DP_WORK);
    if (!Work) return NULL;
    RtlCopyMemory(Work, Dp, Sz);

    /* Walk to the end */
    EFI_DEVICE_PATH_PROTOCOL* Node = Work;
    EFI_DEVICE_PATH_PROTOCOL* LastGood = Work;
    while (!IsDevicePathEnd(Node))
    {
        LastGood = Node;
        Node = NextDevicePathNode(Node);
    }

    /* Pop nodes backwards, try locate a BlockIo parent that is not a logical partition */
    while (LastGood && LastGood != Work)
    {
        UefiSetDevicePathEndNode(LastGood);

        EFI_DEVICE_PATH_PROTOCOL* Tmp = Work;
        EFI_HANDLE Handle = NULL;

        Status = GlobalSystemTable->BootServices->LocateDevicePath(
            &gEfiBlockIoProtocolGuid, &Tmp, &Handle);
        if (!EFI_ERROR(Status) && Handle)
        {
            EFI_BLOCK_IO_PROTOCOL* Blk = NULL;
            if (!EFI_ERROR(GlobalSystemTable->BootServices->HandleProtocol(
                    Handle, &gEfiBlockIoProtocolGuid, (VOID**)&Blk)) &&
                Blk && !Blk->Media->LogicalPartition)
            {
                FrLdrHeapFree(Work, TAG_DP_WORK);
                return Handle;
            }
        }

        /* Move LastGood back one node */
        EFI_DEVICE_PATH_PROTOCOL* Prev = NULL;
        EFI_DEVICE_PATH_PROTOCOL* Cur  = Work;
        while (!IsDevicePathEnd(Cur) && Cur != LastGood)
        {
            Prev = Cur;
            Cur = NextDevicePathNode(Cur);
        }
        LastGood = Prev;
    }

    FrLdrHeapFree(Work, TAG_DP_WORK);
    return NULL;
}

/* Partition list and index computation for a given parent disk */
typedef struct _PART_ENUM_ENTRY {
    EFI_HANDLE Handle;
    UINT64     StartLba;
} PART_ENUM_ENTRY;

static UINTN
BuildPartitionIndexForDisk(IN EFI_HANDLE DiskHandle, OUT PART_ENUM_ENTRY** OutList)
{
    EFI_STATUS  Status;
    EFI_HANDLE* Handles = NULL;
    UINTN       Count = 0, i, n = 0;

    if (OutList) *OutList = NULL;

    Status = GlobalSystemTable->BootServices->LocateHandleBuffer(
        ByProtocol, &gEfiBlockIoProtocolGuid, NULL, &Count, &Handles);
    if (EFI_ERROR(Status) || Count == 0) return 0;

    PART_ENUM_ENTRY* entries = (PART_ENUM_ENTRY*)FrLdrHeapAlloc(sizeof(PART_ENUM_ENTRY) * Count, TAG_PARTLIST);
    if (!entries)
    {
        GlobalSystemTable->BootServices->FreePool(Handles);
        return 0;
    }

    for (i = 0; i < Count; ++i)
    {
        EFI_BLOCK_IO_PROTOCOL* Blk = NULL;
        if (EFI_ERROR(GlobalSystemTable->BootServices->HandleProtocol(
                Handles[i], &gEfiBlockIoProtocolGuid, (VOID**)&Blk)) || !Blk)
            continue;

        if (!Blk->Media->LogicalPartition) continue; /* only partitions */

        /* Must be child of DiskHandle */
        EFI_HANDLE parent = FindParentDiskHandle(Handles[i]);
        if (parent != DiskHandle) continue;

        UINT64 start = 0;
        if (!GetPartitionStartLbaFromHandle(Handles[i], &start))
            continue;

        entries[n].Handle   = Handles[i];
        entries[n].StartLba = start;
        ++n;
    }

    GlobalSystemTable->BootServices->FreePool(Handles);

    /* insertion sort by StartLba (n is tiny) */
    for (i = 1; i < n; ++i)
    {
        PART_ENUM_ENTRY key = entries[i];
        INTN j = (INTN)i - 1;
        while (j >= 0 && entries[j].StartLba > key.StartLba)
        {
            entries[j + 1] = entries[j];
            --j;
        }
        entries[j + 1] = key;
    }

    if (OutList) *OutList = entries; else FrLdrHeapFree(entries, TAG_PARTLIST);
    return n;
}

static BOOLEAN
ComputePartitionIndex(IN EFI_HANDLE PartHandle, OUT ULONG* OutIndex, OUT EFI_HANDLE* OutParentDisk)
{
    if (OutIndex) *OutIndex = 1;
    if (OutParentDisk) *OutParentDisk = NULL;

    EFI_HANDLE parent = FindParentDiskHandle(PartHandle);
    if (!parent) return FALSE;

    PART_ENUM_ENTRY* list = NULL;
    UINTN n = BuildPartitionIndexForDisk(parent, &list);
    if (n == 0 || !list) return FALSE;

    ULONG idx = 1;
    for (UINTN i = 0; i < n; ++i)
    {
        if (list[i].Handle == PartHandle)
        {
            idx = (ULONG)(i + 1);
            break;
        }
    }

    if (OutIndex) *OutIndex = idx;
    if (OutParentDisk) *OutParentDisk = parent;

    FrLdrHeapFree(list, TAG_PARTLIST);
    return TRUE;
}

/* -------------------------------------------------------------------------- */
/* MBR signature reading                                                      */
/* -------------------------------------------------------------------------- */

static BOOLEAN
ReadDiskSignature(
    IN  EFI_BLOCK_IO_PROTOCOL* BlockIo,
    OUT PULONG Signature,
    OUT PULONG CheckSum)
{
    EFI_STATUS Status;
    UINT8*     Buffer;
    ULONG      i, Sum = 0;

    *Signature = 0;
    *CheckSum  = 0;

    Status = GlobalSystemTable->BootServices->AllocatePool(
        EfiLoaderData,
        BlockIo->Media->BlockSize,
        (VOID**)&Buffer);
    if (EFI_ERROR(Status)) return FALSE;

    Status = BlockIo->ReadBlocks(
        BlockIo,
        BlockIo->Media->MediaId,
        0,
        BlockIo->Media->BlockSize,
        Buffer);
    if (EFI_ERROR(Status))
    {
        GlobalSystemTable->BootServices->FreePool(Buffer);
        return FALSE;
    }

    MASTER_BOOT_RECORD* Mbr = (MASTER_BOOT_RECORD*)Buffer;
    if (Mbr->MasterBootRecordMagic == 0xAA55)
    {
        for (i = 0; i < (512u / (UINT32)sizeof(ULONG)); i++)
            Sum += ((PULONG)Buffer)[i];
        *CheckSum = ~Sum + 1;
        *Signature = Mbr->Signature;

        GlobalSystemTable->BootServices->FreePool(Buffer);
        return TRUE;
    }

    GlobalSystemTable->BootServices->FreePool(Buffer);
    return FALSE;
}

/* -------------------------------------------------------------------------- */
/* Small helpers to map handles to indices                                    */
/* -------------------------------------------------------------------------- */

static ULONG
MapToRdiskIndex(IN EFI_HANDLE DiskHandle)
{
    for (ULONG i = 0; i < (ULONG)UefiDiskHandleCount; ++i)
        if (UefiDiskHandles[i].Handle == DiskHandle)
            return i;
    return 0; /* fallback */
}

static ULONG
MapToCdromIndex(IN EFI_HANDLE CdHandle)
{
    for (ULONG i = 0; i < UefiCdromCount; ++i)
        if (UefiCdromHandles[i] == CdHandle)
            return i;
    return 0; /* fallback */
}

/* -------------------------------------------------------------------------- */
/* Enumerate disks and register ARC names                                      */
/* -------------------------------------------------------------------------- */

BOOLEAN
UefiEnumerateArcDisks(VOID)
{
    EFI_STATUS  Status;
    EFI_HANDLE* Handles     = NULL;
    UINTN       HandleCount = 0;
    UINTN       i;
    ULONG       ValidDiskCount = 0;

    TRACE("UefiEnumerateArcDisks: Starting UEFI disk enumeration\n");

    Status = GlobalSystemTable->BootServices->LocateHandleBuffer(
        ByProtocol, &gEfiBlockIoProtocolGuid, NULL, &HandleCount, &Handles);
    if (EFI_ERROR(Status) || HandleCount == 0)
    {
        ERR("UEFI ARC: No block devices found (Status=%lx)\n", (ULONG_PTR)Status);
        return FALSE;
    }

    /* Allocate worst-case storage for HDDs and CDs (we’ll fill actual counts) */
    UefiDiskHandles  = (UEFI_DISK_HANDLE_ENTRY*)FrLdrHeapAlloc(sizeof(UEFI_DISK_HANDLE_ENTRY) * HandleCount, TAG_UEFI_DISK);
    UefiCdromHandles = (EFI_HANDLE*)FrLdrHeapAlloc(sizeof(EFI_HANDLE) * HandleCount, TAG_UEFI_DISK);
    if (!UefiDiskHandles || !UefiCdromHandles)
    {
        if (Handles) GlobalSystemTable->BootServices->FreePool(Handles);
        return FALSE;
    }
    RtlZeroMemory(UefiDiskHandles,  sizeof(UEFI_DISK_HANDLE_ENTRY) * HandleCount);
    RtlZeroMemory(UefiCdromHandles, sizeof(EFI_HANDLE) * HandleCount);

    for (i = 0; i < HandleCount; i++)
    {
        EFI_BLOCK_IO_PROTOCOL* BlockIo = NULL;
        EFI_HANDLE H = Handles[i];

        if (EFI_ERROR(GlobalSystemTable->BootServices->HandleProtocol(
                H, &gEfiBlockIoProtocolGuid, (VOID**)&BlockIo)) || !BlockIo)
            continue;

        if (!BlockIo->Media->MediaPresent)
            continue;

        /* Identify if this is the boot handle (keep removable if it's the boot device) */
        BOOLEAN isBootHandle = FALSE;
        EFI_LOADED_IMAGE_PROTOCOL* LoadedImage = NULL;
        if (!EFI_ERROR(GlobalSystemTable->BootServices->HandleProtocol(
                GlobalImageHandle, &gEfiLoadedImageProtocolGuid, (VOID**)&LoadedImage)) &&
            LoadedImage && LoadedImage->DeviceHandle == H)
        {
            isBootHandle = TRUE;
        }

        BOOLEAN isPartition = BlockIo->Media->LogicalPartition;
        BOOLEAN isRemovable = BlockIo->Media->RemovableMedia;
        BOOLEAN isCd        = IsCdRomHandle(H);

        /* We only index physical handles (non-partitions) */
        if (isPartition) continue;

        /* Skip non-boot removable devices except CD-ROMs (we still index CDs) */
        if (isRemovable && !isBootHandle && !isCd)
            continue;

        if (isCd)
        {
            /* Map cdrom(k) with a sequential index */
            CHAR ArcName[64];
            ULONG idx = UefiCdromCount;

            RtlStringCbPrintfA(ArcName, sizeof(ArcName), "multi(0)disk(0)cdrom(%lu)", (ULONG)idx);
            /* CDs have no MBR; pass ValidPartitionTable = FALSE */
            AddReactOSArcDiskInfo(ArcName, 0, 0, FALSE);
            UefiCdromHandles[UefiCdromCount++] = H;

            TRACE("UEFI ARC: Found CD-ROM %lu: %s\n", (ULONG)idx, ArcName);
        }
        else
        {
            /* HDD/SSD physical disk */
            CHAR  ArcName[64];
            ULONG Signature = 0, CheckSum = 0;
            BOOLEAN HasMbr  = ReadDiskSignature(BlockIo, &Signature, &CheckSum);

            UefiDiskHandles[ValidDiskCount].Handle     = H;
            UefiDiskHandles[ValidDiskCount].BlockIo    = BlockIo;
            UefiDiskHandles[ValidDiskCount].DiskNumber = ValidDiskCount;

            RtlStringCbPrintfA(ArcName, sizeof(ArcName), "multi(0)disk(0)rdisk(%lu)", ValidDiskCount);

            AddReactOSArcDiskInfo(ArcName, Signature, CheckSum, HasMbr);

            TRACE("UEFI ARC: Disk %lu -> %s, Sig=0x%08X, Ck=0x%08X, MBR=%d\n",
                  ValidDiskCount, ArcName, Signature, CheckSum, HasMbr);

            ++ValidDiskCount;
        }
    }

    UefiDiskHandleCount = ValidDiskCount;

    GlobalSystemTable->BootServices->FreePool(Handles);

    TRACE("UefiEnumerateArcDisks: Found %lu HDD/SSD, %lu CD-ROM\n",
          (ULONG)UefiDiskHandleCount, (ULONG)UefiCdromCount);

    return (UefiDiskHandleCount + UefiCdromCount) > 0;
}

/* -------------------------------------------------------------------------- */
/* Populate LoaderBlock->ArcDiskInformation from global ARC disk table        */
/* -------------------------------------------------------------------------- */

BOOLEAN
UefiInitializeArcDisks(PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    ULONG i, DiskCount;
    PARC_DISK_SIGNATURE_EX ArcDiskInfo;
    PARC_DISK_SIGNATURE_EX ArcDiskSig;

    TRACE("UefiInitializeArcDisks: Populating loader block ARC disk information\n");

    if (!LoaderBlock || !LoaderBlock->ArcDiskInformation)
    {
        ERR("UEFI ARC: Invalid loader block or ArcDiskInformation\n");
        return FALSE;
    }

    /* Initialize list head (idempotent) */
    InitializeListHead(&LoaderBlock->ArcDiskInformation->DiskSignatureListHead);

    DiskCount = ArcGetDiskCount();

    for (i = 0; i < DiskCount; i++)
    {
        ArcDiskInfo = ArcGetDiskInfo(i);
        if (!ArcDiskInfo)
            continue;

        ArcDiskSig = (PARC_DISK_SIGNATURE_EX)FrLdrHeapAlloc(sizeof(ARC_DISK_SIGNATURE_EX), TAG_SIGEX);
        if (!ArcDiskSig)
        {
            ERR("UEFI ARC: Failed to allocate ARC disk signature (idx=%lu)\n", i);
            continue;
        }

        RtlCopyMemory(ArcDiskSig, ArcDiskInfo, sizeof(ARC_DISK_SIGNATURE_EX));

        /* ReactOS free loader expects VA pointers; ArcName is inline in the copied struct */
        ArcDiskSig->DiskSignature.ArcName = PaToVa(ArcDiskSig->ArcName);

        InsertTailList(&LoaderBlock->ArcDiskInformation->DiskSignatureListHead,
                       &ArcDiskSig->DiskSignature.ListEntry);

        TRACE("UEFI ARC: Added disk to loader list: %s (Sig=0x%08X, Ck=0x%08X)\n",
              ArcDiskSig->ArcName,
              ArcDiskSig->DiskSignature.Signature,
              ArcDiskSig->DiskSignature.CheckSum);
    }

    TRACE("UefiInitializeArcDisks: Added %lu disks to loader block\n", DiskCount);
    return TRUE;
}

/* -------------------------------------------------------------------------- */
/* Resolve boot device -> (rdisk, partition) or cdrom(#) + ARC device string  */
/* -------------------------------------------------------------------------- */

BOOLEAN
UefiGetBootPartitionInfo(
    OUT PULONG RDiskNumber,
    OUT PULONG PartitionNumber,
    OUT PCHAR  BootDevice,
    IN  ULONG  BootDeviceSize)
{
    EFI_STATUS Status;
    EFI_LOADED_IMAGE_PROTOCOL* LoadedImage = NULL;
    EFI_HANDLE BootHandle = NULL;
    EFI_BLOCK_IO_PROTOCOL* BootBlockIo = NULL;

    if (RDiskNumber)     *RDiskNumber     = 0;
    if (PartitionNumber) *PartitionNumber = 1;
    if (BootDevice && BootDeviceSize) BootDevice[0] = '\0';

    Status = GlobalSystemTable->BootServices->HandleProtocol(
        GlobalImageHandle, &gEfiLoadedImageProtocolGuid, (VOID**)&LoadedImage);
    if (EFI_ERROR(Status) || !LoadedImage)
        goto fallback;

    BootHandle = LoadedImage->DeviceHandle;

    Status = GlobalSystemTable->BootServices->HandleProtocol(
        BootHandle, &gEfiBlockIoProtocolGuid, (VOID**)&BootBlockIo);
    if (EFI_ERROR(Status) || !BootBlockIo)
        goto fallback;

    /* CD-ROM boot? Emit cdrom(#) and no partition */
    if (IsCdRomHandle(BootHandle))
    {
        ULONG cdIndex = MapToCdromIndex(BootHandle);
        if (RDiskNumber)     *RDiskNumber     = cdIndex;
        if (PartitionNumber) *PartitionNumber = 0;
        if (BootDevice)
            RtlStringCbPrintfA(BootDevice, BootDeviceSize, "multi(0)disk(0)cdrom(%lu)", cdIndex);
        TRACE("UEFI ARC: Boot device -> cdrom(%lu)\n", cdIndex);
        return TRUE;
    }

    /* HDD/SSD boot */
    if (!BootBlockIo->Media->LogicalPartition)
    {
        /* Booted from whole disk: find rdisk, assume partition 1 */
        ULONG rdisk = MapToRdiskIndex(BootHandle);
        if (RDiskNumber)     *RDiskNumber     = rdisk;
        if (PartitionNumber) *PartitionNumber = 1;
        if (BootDevice)
            RtlStringCbPrintfA(BootDevice, BootDeviceSize,
                               "multi(0)disk(0)rdisk(%lu)partition(1)", rdisk);
        TRACE("UEFI ARC: Boot device -> rdisk(%lu) partition(1)\n", rdisk);
        return TRUE;
    }
    else
    {
        /* Booted from a partition: compute true partition index and parent disk */
        ULONG part = 1, rdisk = 0;
        EFI_HANDLE parentDisk = NULL;

        if (ComputePartitionIndex(BootHandle, &part, &parentDisk))
            rdisk = MapToRdiskIndex(parentDisk);

        if (RDiskNumber)     *RDiskNumber     = rdisk;
        if (PartitionNumber) *PartitionNumber = part;
        if (BootDevice)
            RtlStringCbPrintfA(BootDevice, BootDeviceSize,
                               "multi(0)disk(0)rdisk(%lu)partition(%lu)", rdisk, part);
        TRACE("UEFI ARC: Boot device -> rdisk(%lu) partition(%lu)\n", rdisk, part);
        return TRUE;
    }

fallback:
    if (BootDevice)
        RtlStringCbPrintfA(BootDevice, BootDeviceSize,
                           "multi(0)disk(0)rdisk(0)partition(1)");
    TRACE("UEFI ARC: Boot device -> fallback rdisk(0) partition(1)\n");
    return FALSE;
}
