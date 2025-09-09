/*
 * PROJECT:     FreeLoader UEFI Support
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     UEFI disk enumeration and ARC name bridge
 * COPYRIGHT:   Copyright 2025 Ahmed ARIF
 */

#include <uefildr.h>
#include <freeldr.h>
#include <arcname.h>
#include "ntldr/winldr.h"
#include <disk.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(WARNING);

#define TAG_UEFI_DISK 'kDfU'  /* "UefD" in reverse for UEFI Disk */


/* -------------------------------------------------------------------------- */
/* External symbols                                                           */
/* -------------------------------------------------------------------------- */

extern EFI_SYSTEM_TABLE* GlobalSystemTable;
extern EFI_HANDLE GlobalImageHandle;
extern PLOADER_SYSTEM_BLOCK WinLdrSystemBlock;

/* From archwsup.c - use existing infrastructure */
extern VOID AddReactOSArcDiskInfo(IN PSTR ArcName,
                                  IN ULONG Signature,
                                  IN ULONG Checksum,
                                  IN BOOLEAN ValidPartitionTable);
extern ULONG ArcGetDiskCount(VOID);
extern PARC_DISK_SIGNATURE_EX ArcGetDiskInfo(ULONG Index);

/* -------------------------------------------------------------------------- */
/* UEFI Disk tracking structures                                              */
/* -------------------------------------------------------------------------- */

typedef struct _UEFI_DISK_HANDLE_ENTRY
{
    EFI_HANDLE Handle;
    EFI_BLOCK_IO_PROTOCOL* BlockIo;
    ULONG DiskNumber;
} UEFI_DISK_HANDLE_ENTRY, *PUEFI_DISK_HANDLE_ENTRY;

/* Keep track of UEFI handles for disk I/O operations */
static UEFI_DISK_HANDLE_ENTRY* UefiDiskHandles = NULL;
static UINTN UefiDiskHandleCount = 0;

/* -------------------------------------------------------------------------- */
/* MBR/GPT signature reading                                                  */
/* -------------------------------------------------------------------------- */

static BOOLEAN
ReadDiskSignature(
    IN EFI_BLOCK_IO_PROTOCOL* BlockIo,
    OUT PULONG Signature,
    OUT PULONG CheckSum)
{
    EFI_STATUS Status;
    MASTER_BOOT_RECORD* Mbr;
    UINT8* Buffer;
    ULONG i, Sum = 0;

    *Signature = 0;
    *CheckSum = 0;

    /* Allocate buffer for reading */
    Status = GlobalSystemTable->BootServices->AllocatePool(
        EfiLoaderData,
        BlockIo->Media->BlockSize,
        (VOID**)&Buffer);
    
    if (EFI_ERROR(Status))
        return FALSE;

    /* Read first sector (MBR) */
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

    Mbr = (MASTER_BOOT_RECORD*)Buffer;

    /* Check for valid MBR signature */
    if (Mbr->MasterBootRecordMagic == 0xAA55)
    {
        /* Calculate checksum */
        for (i = 0; i < 512 / sizeof(ULONG); i++)
        {
            Sum += ((PULONG)Buffer)[i];
        }
        *CheckSum = ~Sum + 1;
        *Signature = Mbr->Signature;
        
        GlobalSystemTable->BootServices->FreePool(Buffer);
        return TRUE;
    }

    GlobalSystemTable->BootServices->FreePool(Buffer);
    return FALSE;
}

/* -------------------------------------------------------------------------- */
/* FreeLdr disk I/O implementation for UEFI                                   */
/* -------------------------------------------------------------------------- */

BOOLEAN
UefiDiskGetDriveGeometry(
    UCHAR DriveNumber,
    PGEOMETRY Geometry)
{
    ULONG DiskIndex = DriveNumber - 0x80; /* Convert from BIOS drive number */
    
    if (DiskIndex >= UefiDiskHandleCount)
        return FALSE;
    
    PUEFI_DISK_HANDLE_ENTRY Disk = &UefiDiskHandles[DiskIndex];
    
    Geometry->Cylinders = 1024; /* Fake values for LBA mode */
    Geometry->Heads = 255;
    Geometry->SectorsPerTrack = 63;
    Geometry->BytesPerSector = (ULONG)Disk->BlockIo->Media->BlockSize;
    Geometry->Sectors = Disk->BlockIo->Media->LastBlock + 1;
    
    return TRUE;
}

BOOLEAN
UefiDiskReadLogicalSectors(
    IN UCHAR DriveNumber,
    IN ULONGLONG SectorNumber,
    IN ULONG SectorCount,
    OUT PVOID Buffer)
{
    EFI_STATUS Status;
    ULONG DiskIndex = DriveNumber - 0x80;
    
    if (DiskIndex >= UefiDiskHandleCount)
        return FALSE;
    
    PUEFI_DISK_HANDLE_ENTRY Disk = &UefiDiskHandles[DiskIndex];
    
    Status = Disk->BlockIo->ReadBlocks(
        Disk->BlockIo,
        Disk->BlockIo->Media->MediaId,
        SectorNumber,
        SectorCount * Disk->BlockIo->Media->BlockSize,
        Buffer);
    
    return !EFI_ERROR(Status);
}

ULONG
UefiDiskGetCacheableBlockCount(UCHAR DriveNumber)
{
    /* Return a reasonable cache size */
    return 64; /* 64 sectors */
}

/* -------------------------------------------------------------------------- */
/* Main initialization function                                               */
/* -------------------------------------------------------------------------- */

BOOLEAN
UefiInitializeBootDevices(VOID)
{
    EFI_STATUS Status;
    EFI_HANDLE* Handles = NULL;
    UINTN HandleCount = 0;
    UINTN i, ValidDiskCount = 0;
    CHAR ArcName[256];
    ULONG Signature, CheckSum;

    TRACE("UefiInitializeBootDevices: Starting UEFI disk enumeration\n");

    /* Get all block I/O handles */
    Status = GlobalSystemTable->BootServices->LocateHandleBuffer(
        ByProtocol,
        &gEfiBlockIoProtocolGuid,
        NULL,
        &HandleCount,
        &Handles);
    
    if (EFI_ERROR(Status) || HandleCount == 0)
    {
        ERR("No block devices found\n");
        return FALSE;
    }

    /* Allocate handle tracking array */
    UefiDiskHandles = (UEFI_DISK_HANDLE_ENTRY*)FrLdrHeapAlloc(
        sizeof(UEFI_DISK_HANDLE_ENTRY) * HandleCount,
        TAG_UEFI_DISK);
    
    if (!UefiDiskHandles)
    {
        GlobalSystemTable->BootServices->FreePool(Handles);
        return FALSE;
    }

    RtlZeroMemory(UefiDiskHandles, sizeof(UEFI_DISK_HANDLE_ENTRY) * HandleCount);

    /* Enumerate physical disks only (not partitions) */
    for (i = 0; i < HandleCount; i++)
    {
        EFI_BLOCK_IO_PROTOCOL* BlockIo;
        
        Status = GlobalSystemTable->BootServices->HandleProtocol(
            Handles[i],
            &gEfiBlockIoProtocolGuid,
            (VOID**)&BlockIo);
        
        if (EFI_ERROR(Status) || !BlockIo->Media->MediaPresent)
            continue;
        
        /* Skip logical partitions - we want physical disks only */
        if (BlockIo->Media->LogicalPartition)
            continue;
        
        /* Skip removable media for now (USB, CD-ROM) */
        /* You may want to handle these separately */
        if (BlockIo->Media->RemovableMedia)
            continue;
        
        /* Store disk handle for I/O operations */
        UefiDiskHandles[ValidDiskCount].Handle = Handles[i];
        UefiDiskHandles[ValidDiskCount].BlockIo = BlockIo;
        UefiDiskHandles[ValidDiskCount].DiskNumber = ValidDiskCount;
        
        /* Create ARC name */
        RtlStringCbPrintfA(ArcName, sizeof(ArcName),
                          "multi(0)disk(0)rdisk(%lu)", ValidDiskCount);
        
        /* Read disk signature and checksum */
        if (!ReadDiskSignature(BlockIo, &Signature, &CheckSum))
        {
            Signature = 0;
            CheckSum = 0;
        }
        
        /* Add to the global ARC disk list using existing infrastructure */
        AddReactOSArcDiskInfo(ArcName, Signature, CheckSum, TRUE);
        
        TRACE("Found disk %lu: %s, Signature=0x%08X, CheckSum=0x%08X\n",
              ValidDiskCount, ArcName, Signature, CheckSum);
        
        ValidDiskCount++;
    }
    
    UefiDiskHandleCount = ValidDiskCount;
    GlobalSystemTable->BootServices->FreePool(Handles);
    
    TRACE("UefiInitializeBootDevices: Found %lu valid disks\n", ValidDiskCount);
    
    /* Register disk access functions with MachVtbl if not already done */
    if (!MachVtbl.DiskReadLogicalSectors)
    {
        MachVtbl.DiskReadLogicalSectors = UefiDiskReadLogicalSectors;
        MachVtbl.DiskGetDriveGeometry = UefiDiskGetDriveGeometry;
        MachVtbl.DiskGetCacheableBlockCount = UefiDiskGetCacheableBlockCount;
    }
    
    return (ValidDiskCount > 0);
}

/* -------------------------------------------------------------------------- */
/* ARC disk information population for loader block                           */
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
        ERR("Invalid loader block or ArcDiskInformation\n");
        return FALSE;
    }
    
    /* Initialize the list head if needed */
    if (IsListEmpty(&LoaderBlock->ArcDiskInformation->DiskSignatureListHead))
    {
        InitializeListHead(&LoaderBlock->ArcDiskInformation->DiskSignatureListHead);
    }
    
    /* Get the disk count from the existing infrastructure */
    DiskCount = ArcGetDiskCount();
    
    /* Add each disk to the loader block's ARC disk list */
    for (i = 0; i < DiskCount; i++)
    {
        /* Get disk info from existing infrastructure */
        ArcDiskInfo = ArcGetDiskInfo(i);
        if (!ArcDiskInfo)
            continue;
        
        /* Allocate ARC disk signature structure for loader block */
        ArcDiskSig = FrLdrHeapAlloc(sizeof(ARC_DISK_SIGNATURE_EX), 'giSD');
        if (!ArcDiskSig)
        {
            ERR("Failed to allocate ARC disk signature\n");
            continue;
        }
        
        /* Copy the disk information */
        RtlCopyMemory(ArcDiskSig, ArcDiskInfo, sizeof(ARC_DISK_SIGNATURE_EX));
        
        /* Fix up the ArcName pointer to point to the copied string */
        ArcDiskSig->DiskSignature.ArcName = ArcDiskSig->ArcName;
        
        /* Insert into the loader block list */
        InsertTailList(&LoaderBlock->ArcDiskInformation->DiskSignatureListHead,
                      &ArcDiskSig->DiskSignature.ListEntry);
        
        TRACE("Added ARC disk to loader block: %s (Sig=0x%08X, ChkSum=0x%08X)\n",
              ArcDiskSig->ArcName, 
              ArcDiskSig->DiskSignature.Signature,
              ArcDiskSig->DiskSignature.CheckSum);
    }
    
    TRACE("UefiInitializeArcDisks: Added %lu disks to loader block\n", DiskCount);
    
    return TRUE;
}

/* -------------------------------------------------------------------------- */
/* Boot device resolution                                                     */
/* -------------------------------------------------------------------------- */

BOOLEAN
UefiGetBootPartitionInfo(
    OUT PULONG RDiskNumber,
    OUT PULONG PartitionNumber,
    OUT PCHAR BootDevice,
    IN ULONG BootDeviceSize)
{
    EFI_STATUS Status;
    EFI_LOADED_IMAGE_PROTOCOL* LoadedImage;
    EFI_HANDLE BootHandle;
    EFI_BLOCK_IO_PROTOCOL* BootBlockIo;
    UINTN i;
    
    /* Get the loaded image protocol */
    Status = GlobalSystemTable->BootServices->HandleProtocol(
        GlobalImageHandle,
        &gEfiLoadedImageProtocolGuid,
        (VOID**)&LoadedImage);
    
    if (EFI_ERROR(Status))
    {
        WARN("Failed to get LoadedImageProtocol\n");
        /* Default to first disk, first partition */
        *RDiskNumber = 0;
        *PartitionNumber = 1;
        if (BootDevice)
            RtlStringCbPrintfA(BootDevice, BootDeviceSize,
                              "multi(0)disk(0)rdisk(0)partition(1)");
        return FALSE;
    }
    
    BootHandle = LoadedImage->DeviceHandle;
    
    /* Try to get block I/O protocol from boot device */
    Status = GlobalSystemTable->BootServices->HandleProtocol(
        BootHandle,
        &gEfiBlockIoProtocolGuid,
        (VOID**)&BootBlockIo);
    
    if (EFI_ERROR(Status))
    {
        WARN("Boot device has no BlockIo protocol\n");
        *RDiskNumber = 0;
        *PartitionNumber = 1;
        if (BootDevice)
            RtlStringCbPrintfA(BootDevice, BootDeviceSize,
                              "multi(0)disk(0)rdisk(0)partition(1)");
        return FALSE;
    }
    
    /* Find the parent disk if this is a partition */
    if (BootBlockIo->Media->LogicalPartition)
    {
        /* For now, assume partition 1 on the first matching disk */
        /* TODO: Implement proper partition enumeration and matching */
        *RDiskNumber = 0;
        *PartitionNumber = 1;
        
        /* Try to match against enumerated disks */
        for (i = 0; i < UefiDiskHandleCount; i++)
        {
            /* Simple heuristic: could be improved with proper parent detection */
            *RDiskNumber = i;
            break;
        }
    }
    else
    {
        /* Boot device is a whole disk */
        for (i = 0; i < UefiDiskHandleCount; i++)
        {
            if (UefiDiskHandles[i].Handle == BootHandle)
            {
                *RDiskNumber = i;
                *PartitionNumber = 1; /* Assume first partition */
                break;
            }
        }
    }
    
    if (BootDevice)
    {
        RtlStringCbPrintfA(BootDevice, BootDeviceSize,
                          "multi(0)disk(0)rdisk(%lu)partition(%lu)",
                          *RDiskNumber, *PartitionNumber);
    }
    
    TRACE("Boot device: rdisk(%lu) partition(%lu)\n",
          *RDiskNumber, *PartitionNumber);
    
    return TRUE;
}