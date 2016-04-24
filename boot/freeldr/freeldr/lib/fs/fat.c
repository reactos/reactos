/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
 *  Copyright (C) 2009       Hervé Poussineau
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <freeldr.h>

#define NDEBUG
#include <debug.h>

DBG_DEFAULT_CHANNEL(FILESYSTEM);

ULONG    FatDetermineFatType(PFAT_BOOTSECTOR FatBootSector, ULONGLONG PartitionSectorCount);
PVOID    FatBufferDirectory(PFAT_VOLUME_INFO Volume, ULONG DirectoryStartCluster, ULONG* EntryCountPointer, BOOLEAN RootDirectory);
BOOLEAN    FatSearchDirectoryBufferForFile(PFAT_VOLUME_INFO Volume, PVOID DirectoryBuffer, ULONG EntryCount, PCHAR FileName, PFAT_FILE_INFO FatFileInfoPointer);
ARC_STATUS FatLookupFile(PFAT_VOLUME_INFO Volume, PCSTR FileName, ULONG DeviceId, PFAT_FILE_INFO FatFileInfoPointer);
void    FatParseShortFileName(PCHAR Buffer, PDIRENTRY DirEntry);
BOOLEAN    FatGetFatEntry(PFAT_VOLUME_INFO Volume, ULONG Cluster, ULONG* ClusterPointer);
ULONG    FatCountClustersInChain(PFAT_VOLUME_INFO Volume, ULONG StartCluster);
ULONG*    FatGetClusterChainArray(PFAT_VOLUME_INFO Volume, ULONG StartCluster);
BOOLEAN    FatReadClusterChain(PFAT_VOLUME_INFO Volume, ULONG StartClusterNumber, ULONG NumberOfClusters, PVOID Buffer);
BOOLEAN    FatReadPartialCluster(PFAT_VOLUME_INFO Volume, ULONG ClusterNumber, ULONG StartingOffset, ULONG Length, PVOID Buffer);
BOOLEAN    FatReadVolumeSectors(PFAT_VOLUME_INFO Volume, ULONG SectorNumber, ULONG SectorCount, PVOID Buffer);

#define TAG_FAT_CHAIN 'CtaT'
#define TAG_FAT_FILE 'FtaF'
#define TAG_FAT_VOLUME 'VtaF'
#define TAG_FAT_BUFFER 'BtaF'

typedef struct _FAT_VOLUME_INFO
{
    ULONG BytesPerSector; /* Number of bytes per sector */
    ULONG SectorsPerCluster; /* Number of sectors per cluster */
    ULONG FatSectorStart; /* Starting sector of 1st FAT table */
    ULONG ActiveFatSectorStart; /* Starting sector of active FAT table */
    ULONG NumberOfFats; /* Number of FAT tables */
    ULONG SectorsPerFat; /* Sectors per FAT table */
    ULONG RootDirSectorStart; /* Starting sector of the root directory (non-fat32) */
    ULONG RootDirSectors; /* Number of sectors of the root directory (non-fat32) */
    ULONG RootDirStartCluster; /* Starting cluster number of the root directory (fat32 only) */
    ULONG DataSectorStart; /* Starting sector of the data area */
    ULONG FatType; /* FAT12, FAT16, FAT32, FATX16 or FATX32 */
    ULONG DeviceId;
} FAT_VOLUME_INFO;

PFAT_VOLUME_INFO FatVolumes[MAX_FDS];

VOID FatSwapFatBootSector(PFAT_BOOTSECTOR Obj)
{
    SW(Obj, BytesPerSector);
    SW(Obj, ReservedSectors);
    SW(Obj, RootDirEntries);
    SW(Obj, TotalSectors);
    SW(Obj, SectorsPerFat);
    SW(Obj, SectorsPerTrack);
    SW(Obj, NumberOfHeads);
    SD(Obj, HiddenSectors);
    SD(Obj, TotalSectorsBig);
    SD(Obj, VolumeSerialNumber);
    SW(Obj, BootSectorMagic);
}

VOID FatSwapFat32BootSector(PFAT32_BOOTSECTOR Obj)
{
    SW(Obj, BytesPerSector);
    SW(Obj, ReservedSectors);
    SW(Obj, RootDirEntries);
    SW(Obj, TotalSectors);
    SW(Obj, SectorsPerFat);
    SW(Obj, NumberOfHeads);
    SD(Obj, HiddenSectors);
    SD(Obj, TotalSectorsBig);
    SD(Obj, SectorsPerFatBig);
    SW(Obj, ExtendedFlags);
    SW(Obj, FileSystemVersion);
    SD(Obj, RootDirStartCluster);
    SW(Obj, FsInfo);
    SW(Obj, BackupBootSector);
    SD(Obj, VolumeSerialNumber);
    SW(Obj, BootSectorMagic);
}

VOID FatSwapFatXBootSector(PFATX_BOOTSECTOR Obj)
{
    SD(Obj, VolumeSerialNumber);
    SD(Obj, SectorsPerCluster);
    SW(Obj, NumberOfFats);
}

VOID FatSwapDirEntry(PDIRENTRY Obj)
{
    SW(Obj, CreateTime);
    SW(Obj, CreateDate);
    SW(Obj, LastAccessDate);
    SW(Obj, ClusterHigh);
    SW(Obj, Time);
    SW(Obj, Date);
    SW(Obj, ClusterLow);
    SD(Obj, Size);
}

VOID FatSwapLFNDirEntry(PLFN_DIRENTRY Obj)
{
    int i;
    SW(Obj, StartCluster);
    for(i = 0; i < 5; i++)
        Obj->Name0_4[i] = SWAPW(Obj->Name0_4[i]);
    for(i = 0; i < 6; i++)
        Obj->Name5_10[i] = SWAPW(Obj->Name5_10[i]);
    for(i = 0; i < 2; i++)
        Obj->Name11_12[i] = SWAPW(Obj->Name11_12[i]);
}

VOID FatSwapFatXDirEntry(PFATX_DIRENTRY Obj)
{
    SD(Obj, StartCluster);
    SD(Obj, Size);
    SW(Obj, Time);
    SW(Obj, Date);
    SW(Obj, CreateTime);
    SW(Obj, CreateDate);
    SW(Obj, LastAccessTime);
    SW(Obj, LastAccessDate);
}

BOOLEAN FatOpenVolume(PFAT_VOLUME_INFO Volume, PFAT_BOOTSECTOR BootSector, ULONGLONG PartitionSectorCount)
{
    char ErrMsg[80];
    ULONG FatSize;
    PFAT_BOOTSECTOR    FatVolumeBootSector;
    PFAT32_BOOTSECTOR Fat32VolumeBootSector;
    PFATX_BOOTSECTOR FatXVolumeBootSector;

    TRACE("FatOpenVolume() DeviceId = %d\n", Volume->DeviceId);

    //
    // Allocate the memory to hold the boot sector
    //
    FatVolumeBootSector = (PFAT_BOOTSECTOR)BootSector;
    Fat32VolumeBootSector = (PFAT32_BOOTSECTOR)BootSector;
    FatXVolumeBootSector = (PFATX_BOOTSECTOR)BootSector;

    // Get the FAT type
    Volume->FatType = FatDetermineFatType(FatVolumeBootSector, PartitionSectorCount);

    // Dump boot sector (and swap it for big endian systems)
    TRACE("Dumping boot sector:\n");
    if (ISFATX(Volume->FatType))
    {
        FatSwapFatXBootSector(FatXVolumeBootSector);
        TRACE("sizeof(FATX_BOOTSECTOR) = 0x%x.\n", sizeof(FATX_BOOTSECTOR));

        TRACE("FileSystemType: %c%c%c%c.\n", FatXVolumeBootSector->FileSystemType[0], FatXVolumeBootSector->FileSystemType[1], FatXVolumeBootSector->FileSystemType[2], FatXVolumeBootSector->FileSystemType[3]);
        TRACE("VolumeSerialNumber: 0x%x\n", FatXVolumeBootSector->VolumeSerialNumber);
        TRACE("SectorsPerCluster: %d\n", FatXVolumeBootSector->SectorsPerCluster);
        TRACE("NumberOfFats: %d\n", FatXVolumeBootSector->NumberOfFats);
        TRACE("Unknown: 0x%x\n", FatXVolumeBootSector->Unknown);

        TRACE("FatType %s\n", Volume->FatType == FATX16 ? "FATX16" : "FATX32");

    }
    else if (Volume->FatType == FAT32)
    {
        FatSwapFat32BootSector(Fat32VolumeBootSector);
        TRACE("sizeof(FAT32_BOOTSECTOR) = 0x%x.\n", sizeof(FAT32_BOOTSECTOR));

        TRACE("JumpBoot: 0x%x 0x%x 0x%x\n", Fat32VolumeBootSector->JumpBoot[0], Fat32VolumeBootSector->JumpBoot[1], Fat32VolumeBootSector->JumpBoot[2]);
        TRACE("OemName: %c%c%c%c%c%c%c%c\n", Fat32VolumeBootSector->OemName[0], Fat32VolumeBootSector->OemName[1], Fat32VolumeBootSector->OemName[2], Fat32VolumeBootSector->OemName[3], Fat32VolumeBootSector->OemName[4], Fat32VolumeBootSector->OemName[5], Fat32VolumeBootSector->OemName[6], Fat32VolumeBootSector->OemName[7]);
        TRACE("BytesPerSector: %d\n", Fat32VolumeBootSector->BytesPerSector);
        TRACE("SectorsPerCluster: %d\n", Fat32VolumeBootSector->SectorsPerCluster);
        TRACE("ReservedSectors: %d\n", Fat32VolumeBootSector->ReservedSectors);
        TRACE("NumberOfFats: %d\n", Fat32VolumeBootSector->NumberOfFats);
        TRACE("RootDirEntries: %d\n", Fat32VolumeBootSector->RootDirEntries);
        TRACE("TotalSectors: %d\n", Fat32VolumeBootSector->TotalSectors);
        TRACE("MediaDescriptor: 0x%x\n", Fat32VolumeBootSector->MediaDescriptor);
        TRACE("SectorsPerFat: %d\n", Fat32VolumeBootSector->SectorsPerFat);
        TRACE("SectorsPerTrack: %d\n", Fat32VolumeBootSector->SectorsPerTrack);
        TRACE("NumberOfHeads: %d\n", Fat32VolumeBootSector->NumberOfHeads);
        TRACE("HiddenSectors: %d\n", Fat32VolumeBootSector->HiddenSectors);
        TRACE("TotalSectorsBig: %d\n", Fat32VolumeBootSector->TotalSectorsBig);
        TRACE("SectorsPerFatBig: %d\n", Fat32VolumeBootSector->SectorsPerFatBig);
        TRACE("ExtendedFlags: 0x%x\n", Fat32VolumeBootSector->ExtendedFlags);
        TRACE("FileSystemVersion: 0x%x\n", Fat32VolumeBootSector->FileSystemVersion);
        TRACE("RootDirStartCluster: %d\n", Fat32VolumeBootSector->RootDirStartCluster);
        TRACE("FsInfo: %d\n", Fat32VolumeBootSector->FsInfo);
        TRACE("BackupBootSector: %d\n", Fat32VolumeBootSector->BackupBootSector);
        TRACE("Reserved: 0x%x\n", Fat32VolumeBootSector->Reserved);
        TRACE("DriveNumber: 0x%x\n", Fat32VolumeBootSector->DriveNumber);
        TRACE("Reserved1: 0x%x\n", Fat32VolumeBootSector->Reserved1);
        TRACE("BootSignature: 0x%x\n", Fat32VolumeBootSector->BootSignature);
        TRACE("VolumeSerialNumber: 0x%x\n", Fat32VolumeBootSector->VolumeSerialNumber);
        TRACE("VolumeLabel: %c%c%c%c%c%c%c%c%c%c%c\n", Fat32VolumeBootSector->VolumeLabel[0], Fat32VolumeBootSector->VolumeLabel[1], Fat32VolumeBootSector->VolumeLabel[2], Fat32VolumeBootSector->VolumeLabel[3], Fat32VolumeBootSector->VolumeLabel[4], Fat32VolumeBootSector->VolumeLabel[5], Fat32VolumeBootSector->VolumeLabel[6], Fat32VolumeBootSector->VolumeLabel[7], Fat32VolumeBootSector->VolumeLabel[8], Fat32VolumeBootSector->VolumeLabel[9], Fat32VolumeBootSector->VolumeLabel[10]);
        TRACE("FileSystemType: %c%c%c%c%c%c%c%c\n", Fat32VolumeBootSector->FileSystemType[0], Fat32VolumeBootSector->FileSystemType[1], Fat32VolumeBootSector->FileSystemType[2], Fat32VolumeBootSector->FileSystemType[3], Fat32VolumeBootSector->FileSystemType[4], Fat32VolumeBootSector->FileSystemType[5], Fat32VolumeBootSector->FileSystemType[6], Fat32VolumeBootSector->FileSystemType[7]);
        TRACE("BootSectorMagic: 0x%x\n", Fat32VolumeBootSector->BootSectorMagic);
    }
    else
    {
        FatSwapFatBootSector(FatVolumeBootSector);
        TRACE("sizeof(FAT_BOOTSECTOR) = 0x%x.\n", sizeof(FAT_BOOTSECTOR));

        TRACE("JumpBoot: 0x%x 0x%x 0x%x\n", FatVolumeBootSector->JumpBoot[0], FatVolumeBootSector->JumpBoot[1], FatVolumeBootSector->JumpBoot[2]);
        TRACE("OemName: %c%c%c%c%c%c%c%c\n", FatVolumeBootSector->OemName[0], FatVolumeBootSector->OemName[1], FatVolumeBootSector->OemName[2], FatVolumeBootSector->OemName[3], FatVolumeBootSector->OemName[4], FatVolumeBootSector->OemName[5], FatVolumeBootSector->OemName[6], FatVolumeBootSector->OemName[7]);
        TRACE("BytesPerSector: %d\n", FatVolumeBootSector->BytesPerSector);
        TRACE("SectorsPerCluster: %d\n", FatVolumeBootSector->SectorsPerCluster);
        TRACE("ReservedSectors: %d\n", FatVolumeBootSector->ReservedSectors);
        TRACE("NumberOfFats: %d\n", FatVolumeBootSector->NumberOfFats);
        TRACE("RootDirEntries: %d\n", FatVolumeBootSector->RootDirEntries);
        TRACE("TotalSectors: %d\n", FatVolumeBootSector->TotalSectors);
        TRACE("MediaDescriptor: 0x%x\n", FatVolumeBootSector->MediaDescriptor);
        TRACE("SectorsPerFat: %d\n", FatVolumeBootSector->SectorsPerFat);
        TRACE("SectorsPerTrack: %d\n", FatVolumeBootSector->SectorsPerTrack);
        TRACE("NumberOfHeads: %d\n", FatVolumeBootSector->NumberOfHeads);
        TRACE("HiddenSectors: %d\n", FatVolumeBootSector->HiddenSectors);
        TRACE("TotalSectorsBig: %d\n", FatVolumeBootSector->TotalSectorsBig);
        TRACE("DriveNumber: 0x%x\n", FatVolumeBootSector->DriveNumber);
        TRACE("Reserved1: 0x%x\n", FatVolumeBootSector->Reserved1);
        TRACE("BootSignature: 0x%x\n", FatVolumeBootSector->BootSignature);
        TRACE("VolumeSerialNumber: 0x%x\n", FatVolumeBootSector->VolumeSerialNumber);
        TRACE("VolumeLabel: %c%c%c%c%c%c%c%c%c%c%c\n", FatVolumeBootSector->VolumeLabel[0], FatVolumeBootSector->VolumeLabel[1], FatVolumeBootSector->VolumeLabel[2], FatVolumeBootSector->VolumeLabel[3], FatVolumeBootSector->VolumeLabel[4], FatVolumeBootSector->VolumeLabel[5], FatVolumeBootSector->VolumeLabel[6], FatVolumeBootSector->VolumeLabel[7], FatVolumeBootSector->VolumeLabel[8], FatVolumeBootSector->VolumeLabel[9], FatVolumeBootSector->VolumeLabel[10]);
        TRACE("FileSystemType: %c%c%c%c%c%c%c%c\n", FatVolumeBootSector->FileSystemType[0], FatVolumeBootSector->FileSystemType[1], FatVolumeBootSector->FileSystemType[2], FatVolumeBootSector->FileSystemType[3], FatVolumeBootSector->FileSystemType[4], FatVolumeBootSector->FileSystemType[5], FatVolumeBootSector->FileSystemType[6], FatVolumeBootSector->FileSystemType[7]);
        TRACE("BootSectorMagic: 0x%x\n", FatVolumeBootSector->BootSectorMagic);
    }

    //
    // Check the boot sector magic
    //
    if (! ISFATX(Volume->FatType) && FatVolumeBootSector->BootSectorMagic != 0xaa55)
    {
        sprintf(ErrMsg, "Invalid boot sector magic (expected 0xaa55 found 0x%x)",
                FatVolumeBootSector->BootSectorMagic);
        FileSystemError(ErrMsg);
        return FALSE;
    }

    //
    // Check the FAT cluster size
    // We do not support clusters bigger than 64k
    //
    if ((ISFATX(Volume->FatType) && 64 * 1024 < FatXVolumeBootSector->SectorsPerCluster * 512) ||
       (! ISFATX(Volume->FatType) && 64 * 1024 < FatVolumeBootSector->SectorsPerCluster * FatVolumeBootSector->BytesPerSector))
    {
        FileSystemError("This file system has cluster sizes bigger than 64k.\nFreeLoader does not support this.");
        return FALSE;
    }

    //
    // Get the sectors per FAT,
    // root directory starting sector,
    // and data sector start
    //
    if (ISFATX(Volume->FatType))
    {
        Volume->BytesPerSector = 512;
        Volume->SectorsPerCluster = SWAPD(FatXVolumeBootSector->SectorsPerCluster);
        Volume->FatSectorStart = (4096 / Volume->BytesPerSector);
        Volume->ActiveFatSectorStart = Volume->FatSectorStart;
        Volume->NumberOfFats = 1;
        FatSize = (ULONG)(PartitionSectorCount / Volume->SectorsPerCluster *
                  (Volume->FatType == FATX16 ? 2 : 4));
        Volume->SectorsPerFat = (((FatSize + 4095) / 4096) * 4096) / Volume->BytesPerSector;

        Volume->RootDirSectorStart = Volume->FatSectorStart + Volume->NumberOfFats * Volume->SectorsPerFat;
        Volume->RootDirSectors = FatXVolumeBootSector->SectorsPerCluster;

        Volume->DataSectorStart = Volume->RootDirSectorStart + Volume->RootDirSectors;
    }
    else if (Volume->FatType != FAT32)
    {
        Volume->BytesPerSector = FatVolumeBootSector->BytesPerSector;
        Volume->SectorsPerCluster = FatVolumeBootSector->SectorsPerCluster;
        Volume->FatSectorStart = FatVolumeBootSector->ReservedSectors;
        Volume->ActiveFatSectorStart = Volume->FatSectorStart;
        Volume->NumberOfFats = FatVolumeBootSector->NumberOfFats;
        Volume->SectorsPerFat = FatVolumeBootSector->SectorsPerFat;

        Volume->RootDirSectorStart = Volume->FatSectorStart + Volume->NumberOfFats * Volume->SectorsPerFat;
        Volume->RootDirSectors = ((FatVolumeBootSector->RootDirEntries * 32) + (Volume->BytesPerSector - 1)) / Volume->BytesPerSector;

        Volume->DataSectorStart = Volume->RootDirSectorStart + Volume->RootDirSectors;
    }
    else
    {
        Volume->BytesPerSector = Fat32VolumeBootSector->BytesPerSector;
        Volume->SectorsPerCluster = Fat32VolumeBootSector->SectorsPerCluster;
        Volume->FatSectorStart = Fat32VolumeBootSector->ReservedSectors;
        Volume->ActiveFatSectorStart = Volume->FatSectorStart +
                                       ((Fat32VolumeBootSector->ExtendedFlags & 0x80) ? ((Fat32VolumeBootSector->ExtendedFlags & 0x0f) * Fat32VolumeBootSector->SectorsPerFatBig) : 0);
        Volume->NumberOfFats = Fat32VolumeBootSector->NumberOfFats;
        Volume->SectorsPerFat = Fat32VolumeBootSector->SectorsPerFatBig;

        Volume->RootDirStartCluster = Fat32VolumeBootSector->RootDirStartCluster;
        Volume->DataSectorStart = Volume->FatSectorStart + Volume->NumberOfFats * Volume->SectorsPerFat;

        //
        // Check version
        // we only work with version 0
        //
        if (Fat32VolumeBootSector->FileSystemVersion != 0)
        {
            FileSystemError("FreeLoader is too old to work with this FAT32 filesystem.\nPlease update FreeLoader.");
            return FALSE;
        }
    }

    return TRUE;
}

ULONG FatDetermineFatType(PFAT_BOOTSECTOR FatBootSector, ULONGLONG PartitionSectorCount)
{
    ULONG            RootDirSectors;
    ULONG            DataSectorCount;
    ULONG            SectorsPerFat;
    ULONG            TotalSectors;
    ULONG            CountOfClusters;
    PFAT32_BOOTSECTOR    Fat32BootSector = (PFAT32_BOOTSECTOR)FatBootSector;
    PFATX_BOOTSECTOR    FatXBootSector = (PFATX_BOOTSECTOR)FatBootSector;

    if (0 == strncmp(FatXBootSector->FileSystemType, "FATX", 4))
    {
        CountOfClusters = (ULONG)(PartitionSectorCount / FatXBootSector->SectorsPerCluster);
        if (CountOfClusters < 65525)
        {
            /* Volume is FATX16 */
            return FATX16;
        }
        else
        {
            /* Volume is FAT32 */
            return FATX32;
        }
    }
    else
    {
        RootDirSectors = ((SWAPW(FatBootSector->RootDirEntries) * 32) + (SWAPW(FatBootSector->BytesPerSector) - 1)) / SWAPW(FatBootSector->BytesPerSector);
        SectorsPerFat = SWAPW(FatBootSector->SectorsPerFat) ? SWAPW(FatBootSector->SectorsPerFat) : SWAPD(Fat32BootSector->SectorsPerFatBig);
        TotalSectors = SWAPW(FatBootSector->TotalSectors) ? SWAPW(FatBootSector->TotalSectors) : SWAPD(FatBootSector->TotalSectorsBig);
        DataSectorCount = TotalSectors - (SWAPW(FatBootSector->ReservedSectors) + (FatBootSector->NumberOfFats * SectorsPerFat) + RootDirSectors);

//mjl
        if (FatBootSector->SectorsPerCluster == 0)
            CountOfClusters = 0;
        else
            CountOfClusters = DataSectorCount / FatBootSector->SectorsPerCluster;

        if (CountOfClusters < 4085)
        {
            /* Volume is FAT12 */
            return FAT12;
        }
        else if (CountOfClusters < 65525)
        {
            /* Volume is FAT16 */
            return FAT16;
        }
        else
        {
            /* Volume is FAT32 */
            return FAT32;
        }
    }
}

typedef struct _DIRECTORY_BUFFER
{
    LIST_ENTRY Link;
    PVOID Volume;
    ULONG DirectoryStartCluster;
    ULONG DirectorySize;
    UCHAR Data[];
} DIRECTORY_BUFFER, *PDIRECTORY_BUFFER;

LIST_ENTRY DirectoryBufferListHead = {&DirectoryBufferListHead, &DirectoryBufferListHead};

PVOID FatBufferDirectory(PFAT_VOLUME_INFO Volume, ULONG DirectoryStartCluster, ULONG *DirectorySize, BOOLEAN RootDirectory)
{
    PDIRECTORY_BUFFER DirectoryBuffer;
    PLIST_ENTRY Entry;

    TRACE("FatBufferDirectory() DirectoryStartCluster = %d RootDirectory = %s\n", DirectoryStartCluster, (RootDirectory ? "TRUE" : "FALSE"));

    /*
     * For FAT32, the root directory is nothing special. We can treat it the same
     * as a subdirectory.
     */
    if (RootDirectory && Volume->FatType == FAT32)
    {
        DirectoryStartCluster = Volume->RootDirStartCluster;
        RootDirectory = FALSE;
    }

    /* Search the list for a match */
    for (Entry = DirectoryBufferListHead.Flink;
         Entry != &DirectoryBufferListHead;
         Entry = Entry->Flink)
    {
        DirectoryBuffer = CONTAINING_RECORD(Entry, DIRECTORY_BUFFER, Link);

        /* Check if it matches */
        if ((DirectoryBuffer->Volume == Volume) &&
            (DirectoryBuffer->DirectoryStartCluster == DirectoryStartCluster))
        {
            TRACE("Found cached buffer\n");
            *DirectorySize = DirectoryBuffer->DirectorySize;
            return DirectoryBuffer->Data;
        }
    }

    //
    // Calculate the size of the directory
    //
    if (RootDirectory)
    {
        *DirectorySize = Volume->RootDirSectors * Volume->BytesPerSector;
    }
    else
    {
        *DirectorySize = FatCountClustersInChain(Volume, DirectoryStartCluster) * Volume->SectorsPerCluster * Volume->BytesPerSector;
    }

    //
    // Attempt to allocate memory for directory buffer
    //
    TRACE("Trying to allocate (DirectorySize) %d bytes.\n", *DirectorySize);
    DirectoryBuffer = FrLdrTempAlloc(*DirectorySize + sizeof(DIRECTORY_BUFFER),
                                     TAG_FAT_BUFFER);

    if (DirectoryBuffer == NULL)
    {
        return NULL;
    }

    //
    // Now read directory contents into DirectoryBuffer
    //
    if (RootDirectory)
    {
        if (!FatReadVolumeSectors(Volume, Volume->RootDirSectorStart, Volume->RootDirSectors, DirectoryBuffer->Data))
        {
            FrLdrTempFree(DirectoryBuffer, TAG_FAT_BUFFER);
            return NULL;
        }
    }
    else
    {
        if (!FatReadClusterChain(Volume, DirectoryStartCluster, 0xFFFFFFFF, DirectoryBuffer->Data))
        {
            FrLdrTempFree(DirectoryBuffer, TAG_FAT_BUFFER);
            return NULL;
        }
    }

    /* Enqueue it in the list */
    DirectoryBuffer->Volume = Volume;
    DirectoryBuffer->DirectoryStartCluster = DirectoryStartCluster;
    DirectoryBuffer->DirectorySize = *DirectorySize;
    InsertTailList(&DirectoryBufferListHead, &DirectoryBuffer->Link);

    return DirectoryBuffer->Data;
}

BOOLEAN FatSearchDirectoryBufferForFile(PFAT_VOLUME_INFO Volume, PVOID DirectoryBuffer, ULONG DirectorySize, PCHAR FileName, PFAT_FILE_INFO FatFileInfoPointer)
{
    ULONG        EntryCount;
    ULONG        CurrentEntry;
    CHAR        LfnNameBuffer[265];
    CHAR        ShortNameBuffer[20];
    ULONG        StartCluster;
    DIRENTRY        OurDirEntry;
    LFN_DIRENTRY    OurLfnDirEntry;
    PDIRENTRY    DirEntry = &OurDirEntry;
    PLFN_DIRENTRY    LfnDirEntry = &OurLfnDirEntry;

    EntryCount = DirectorySize / sizeof(DIRENTRY);

    TRACE("FatSearchDirectoryBufferForFile() DirectoryBuffer = 0x%x EntryCount = %d FileName = %s\n", DirectoryBuffer, EntryCount, FileName);

    memset(ShortNameBuffer, 0, 13 * sizeof(CHAR));
    memset(LfnNameBuffer, 0, 261 * sizeof(CHAR));

    for (CurrentEntry=0; CurrentEntry<EntryCount; CurrentEntry++, DirectoryBuffer = ((PDIRENTRY)DirectoryBuffer)+1)
    {
        OurLfnDirEntry = *((PLFN_DIRENTRY) DirectoryBuffer);
        FatSwapLFNDirEntry(LfnDirEntry);
        OurDirEntry = *((PDIRENTRY) DirectoryBuffer);
        FatSwapDirEntry(DirEntry);

        //TRACE("Dumping directory entry %d:\n", CurrentEntry);
        //DbgDumpBuffer(DPRINT_FILESYSTEM, DirEntry, sizeof(DIRENTRY));

        //
        // Check if this is the last file in the directory
        // If DirEntry[0] == 0x00 then that means all the
        // entries after this one are unused. If this is the
        // last entry then we didn't find the file in this directory.
        //
        if (DirEntry->FileName[0] == '\0')
        {
            return FALSE;
        }

        //
        // Check if this is a deleted entry or not
        //
        if (DirEntry->FileName[0] == '\xE5')
        {
            memset(ShortNameBuffer, 0, 13 * sizeof(CHAR));
            memset(LfnNameBuffer, 0, 261 * sizeof(CHAR));
            continue;
        }

        //
        // Check if this is a LFN entry
        // If so it needs special handling
        //
        if (DirEntry->Attr == ATTR_LONG_NAME)
        {
            //
            // Check to see if this is a deleted LFN entry, if so continue
            //
            if (LfnDirEntry->SequenceNumber & 0x80)
            {
                continue;
            }

            //
            // Mask off high two bits of sequence number
            // and make the sequence number zero-based
            //
            LfnDirEntry->SequenceNumber &= 0x3F;
            LfnDirEntry->SequenceNumber--;

            //
            // Get all 13 LFN entry characters
            //
            if (LfnDirEntry->Name0_4[0] != 0xFFFF)
            {
                LfnNameBuffer[0 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name0_4[0];
            }
            if (LfnDirEntry->Name0_4[1] != 0xFFFF)
            {
                LfnNameBuffer[1 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name0_4[1];
            }
            if (LfnDirEntry->Name0_4[2] != 0xFFFF)
            {
                LfnNameBuffer[2 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name0_4[2];
            }
            if (LfnDirEntry->Name0_4[3] != 0xFFFF)
            {
                LfnNameBuffer[3 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name0_4[3];
            }
            if (LfnDirEntry->Name0_4[4] != 0xFFFF)
            {
                LfnNameBuffer[4 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name0_4[4];
            }
            if (LfnDirEntry->Name5_10[0] != 0xFFFF)
            {
                LfnNameBuffer[5 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name5_10[0];
            }
            if (LfnDirEntry->Name5_10[1] != 0xFFFF)
            {
                LfnNameBuffer[6 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name5_10[1];
            }
            if (LfnDirEntry->Name5_10[2] != 0xFFFF)
            {
                LfnNameBuffer[7 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name5_10[2];
            }
            if (LfnDirEntry->Name5_10[3] != 0xFFFF)
            {
                LfnNameBuffer[8 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name5_10[3];
            }
            if (LfnDirEntry->Name5_10[4] != 0xFFFF)
            {
                LfnNameBuffer[9 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name5_10[4];
            }
            if (LfnDirEntry->Name5_10[5] != 0xFFFF)
            {
                LfnNameBuffer[10 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name5_10[5];
            }
            if (LfnDirEntry->Name11_12[0] != 0xFFFF)
            {
                LfnNameBuffer[11 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name11_12[0];
            }
            if (LfnDirEntry->Name11_12[1] != 0xFFFF)
            {
                LfnNameBuffer[12 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name11_12[1];
            }

            //TRACE("Dumping long name buffer:\n");
            //DbgDumpBuffer(DPRINT_FILESYSTEM, LfnNameBuffer, 260);

            continue;
        }

        //
        // Check for the volume label attribute
        // and skip over this entry if found
        //
        if (DirEntry->Attr & ATTR_VOLUMENAME)
        {
            memset(ShortNameBuffer, 0, 13 * sizeof(UCHAR));
            memset(LfnNameBuffer, 0, 261 * sizeof(UCHAR));
            continue;
        }

        //
        // If we get here then we've found a short file name
        // entry and LfnNameBuffer contains the long file
        // name or zeroes. All we have to do now is see if the
        // file name matches either the short or long file name
        // and fill in the FAT_FILE_INFO structure if it does
        // or zero our buffers and continue looking.
        //

        //
        // Get short file name
        //
        FatParseShortFileName(ShortNameBuffer, DirEntry);

        //TRACE("Entry: %d LFN = %s\n", CurrentEntry, LfnNameBuffer);
        //TRACE("Entry: %d DOS name = %s\n", CurrentEntry, ShortNameBuffer);

        //
        // See if the file name matches either the short or long name
        //
        if (((strlen(FileName) == strlen(LfnNameBuffer)) && (_stricmp(FileName, LfnNameBuffer) == 0)) ||
            ((strlen(FileName) == strlen(ShortNameBuffer)) && (_stricmp(FileName, ShortNameBuffer) == 0)))        {
            //
            // We found the entry, now fill in the FAT_FILE_INFO struct
            //
            FatFileInfoPointer->Attributes = DirEntry->Attr;
            FatFileInfoPointer->FileSize = DirEntry->Size;
            FatFileInfoPointer->FilePointer = 0;

            TRACE("MSDOS Directory Entry:\n");
            TRACE("FileName[11] = %c%c%c%c%c%c%c%c%c%c%c\n", DirEntry->FileName[0], DirEntry->FileName[1], DirEntry->FileName[2], DirEntry->FileName[3], DirEntry->FileName[4], DirEntry->FileName[5], DirEntry->FileName[6], DirEntry->FileName[7], DirEntry->FileName[8], DirEntry->FileName[9], DirEntry->FileName[10]);
            TRACE("Attr = 0x%x\n", DirEntry->Attr);
            TRACE("ReservedNT = 0x%x\n", DirEntry->ReservedNT);
            TRACE("TimeInTenths = %d\n", DirEntry->TimeInTenths);
            TRACE("CreateTime = %d\n", DirEntry->CreateTime);
            TRACE("CreateDate = %d\n", DirEntry->CreateDate);
            TRACE("LastAccessDate = %d\n", DirEntry->LastAccessDate);
            TRACE("ClusterHigh = 0x%x\n", DirEntry->ClusterHigh);
            TRACE("Time = %d\n", DirEntry->Time);
            TRACE("Date = %d\n", DirEntry->Date);
            TRACE("ClusterLow = 0x%x\n", DirEntry->ClusterLow);
            TRACE("Size = %d\n", DirEntry->Size);

            //
            // Get the cluster chain
            //
            StartCluster = ((ULONG)DirEntry->ClusterHigh << 16) + DirEntry->ClusterLow;
            TRACE("StartCluster = 0x%x\n", StartCluster);
            FatFileInfoPointer->FileFatChain = FatGetClusterChainArray(Volume, StartCluster);

            //
            // See if memory allocation failed
            //
            if (FatFileInfoPointer->FileFatChain == NULL)
            {
                return FALSE;
            }

            return TRUE;
        }

        //
        // Nope, no match - zero buffers and continue looking
        //
        memset(ShortNameBuffer, 0, 13 * sizeof(UCHAR));
        memset(LfnNameBuffer, 0, 261 * sizeof(UCHAR));
        continue;
    }

    return FALSE;
}

static BOOLEAN FatXSearchDirectoryBufferForFile(PFAT_VOLUME_INFO Volume, PVOID DirectoryBuffer, ULONG DirectorySize, PCHAR FileName, PFAT_FILE_INFO FatFileInfoPointer)
{
    ULONG        EntryCount;
    ULONG        CurrentEntry;
    SIZE_T        FileNameLen;
    FATX_DIRENTRY    OurDirEntry;
    PFATX_DIRENTRY    DirEntry = &OurDirEntry;

    EntryCount = DirectorySize / sizeof(FATX_DIRENTRY);

    TRACE("FatXSearchDirectoryBufferForFile() DirectoryBuffer = 0x%x EntryCount = %d FileName = %s\n", DirectoryBuffer, EntryCount, FileName);

    FileNameLen = strlen(FileName);

    for (CurrentEntry = 0; CurrentEntry < EntryCount; CurrentEntry++, DirectoryBuffer = ((PFATX_DIRENTRY)DirectoryBuffer)+1)
    {
        OurDirEntry = *(PFATX_DIRENTRY) DirectoryBuffer;
        FatSwapFatXDirEntry(&OurDirEntry);
        if (0xff == DirEntry->FileNameSize)
        {
            break;
        }
        if (0xe5 == DirEntry->FileNameSize)
        {
            continue;
        }
        if (FileNameLen == DirEntry->FileNameSize &&
            0 == _strnicmp(FileName, DirEntry->FileName, FileNameLen))
        {
            /*
             * We found the entry, now fill in the FAT_FILE_INFO struct
             */
            FatFileInfoPointer->FileSize = DirEntry->Size;
            FatFileInfoPointer->FilePointer = 0;

            TRACE("FATX Directory Entry:\n");
            TRACE("FileNameSize = %d\n", DirEntry->FileNameSize);
            TRACE("Attr = 0x%x\n", DirEntry->Attr);
            TRACE("StartCluster = 0x%x\n", DirEntry->StartCluster);
            TRACE("Size = %d\n", DirEntry->Size);
            TRACE("Time = %d\n", DirEntry->Time);
            TRACE("Date = %d\n", DirEntry->Date);
            TRACE("CreateTime = %d\n", DirEntry->CreateTime);
            TRACE("CreateDate = %d\n", DirEntry->CreateDate);
            TRACE("LastAccessTime = %d\n", DirEntry->LastAccessTime);
            TRACE("LastAccessDate = %d\n", DirEntry->LastAccessDate);

            /*
             * Get the cluster chain
             */
            FatFileInfoPointer->FileFatChain = FatGetClusterChainArray(Volume, DirEntry->StartCluster);

            /*
             * See if memory allocation failed
             */
            if (NULL == FatFileInfoPointer->FileFatChain)
            {
                return FALSE;
            }

            return TRUE;
        }
    }

    return FALSE;
}

/*
 * FatLookupFile()
 * This function searches the file system for the
 * specified filename and fills in an FAT_FILE_INFO structure
 * with info describing the file, etc. returns ARC error code
 */
ARC_STATUS FatLookupFile(PFAT_VOLUME_INFO Volume, PCSTR FileName, ULONG DeviceId, PFAT_FILE_INFO FatFileInfoPointer)
{
    UINT32        i;
    ULONG        NumberOfPathParts;
    CHAR        PathPart[261];
    PVOID        DirectoryBuffer;
    ULONG        DirectoryStartCluster = 0;
    ULONG        DirectorySize;
    FAT_FILE_INFO    FatFileInfo;

    TRACE("FatLookupFile() FileName = %s\n", FileName);

    memset(FatFileInfoPointer, 0, sizeof(FAT_FILE_INFO));

    //
    // Figure out how many sub-directories we are nested in
    //
    NumberOfPathParts = FsGetNumPathParts(FileName);

    //
    // Loop once for each part
    //
    for (i=0; i<NumberOfPathParts; i++)
    {
        //
        // Get first path part
        //
        FsGetFirstNameFromPath(PathPart, FileName);

        //
        // Advance to the next part of the path
        //
        for (; (*FileName != '\\') && (*FileName != '/') && (*FileName != '\0'); FileName++)
        {
        }
        FileName++;

        //
        // Buffer the directory contents
        //
        DirectoryBuffer = FatBufferDirectory(Volume, DirectoryStartCluster, &DirectorySize, (i == 0) );
        if (DirectoryBuffer == NULL)
        {
            return ENOMEM;
        }

        //
        // Search for file name in directory
        //
        if (ISFATX(Volume->FatType))
        {
            if (!FatXSearchDirectoryBufferForFile(Volume, DirectoryBuffer, DirectorySize, PathPart, &FatFileInfo))
            {
                return ENOENT;
            }
        }
        else
        {
            if (!FatSearchDirectoryBufferForFile(Volume, DirectoryBuffer, DirectorySize, PathPart, &FatFileInfo))
            {
                return ENOENT;
            }
        }

        //
        // If we have another sub-directory to go then
        // grab the start cluster and free the fat chain array
        //
        if ((i+1) < NumberOfPathParts)
        {
            //
            // Check if current entry is a directory
            //
            if (!(FatFileInfo.Attributes & ATTR_DIRECTORY))
            {
                FrLdrTempFree(FatFileInfo.FileFatChain, TAG_FAT_CHAIN);
                return ENOTDIR;
            }
            DirectoryStartCluster = FatFileInfo.FileFatChain[0];
            FrLdrTempFree(FatFileInfo.FileFatChain, TAG_FAT_CHAIN);
            FatFileInfo.FileFatChain = NULL;
        }
    }

    memcpy(FatFileInfoPointer, &FatFileInfo, sizeof(FAT_FILE_INFO));

    return ESUCCESS;
}

/*
 * FatParseFileName()
 * This function parses a directory entry name which
 * is in the form of "FILE   EXT" and puts it in Buffer
 * in the form of "file.ext"
 */
void FatParseShortFileName(PCHAR Buffer, PDIRENTRY DirEntry)
{
    ULONG        Idx;

    Idx = 0;
    RtlZeroMemory(Buffer, 13);

    //
    // Fixup first character
    //
    if (DirEntry->FileName[0] == 0x05)
    {
        DirEntry->FileName[0] = 0xE5;
    }

    //
    // Get the file name
    //
    while (Idx < 8)
    {
        if (DirEntry->FileName[Idx] == ' ')
        {
            break;
        }

        Buffer[Idx] = DirEntry->FileName[Idx];
        Idx++;
    }

    //
    // Get extension
    //
    if ((DirEntry->FileName[8] != ' '))
    {
        Buffer[Idx++] = '.';
        Buffer[Idx++] = (DirEntry->FileName[8] == ' ') ? '\0' : DirEntry->FileName[8];
        Buffer[Idx++] = (DirEntry->FileName[9] == ' ') ? '\0' : DirEntry->FileName[9];
        Buffer[Idx++] = (DirEntry->FileName[10] == ' ') ? '\0' : DirEntry->FileName[10];
    }

    //TRACE("FatParseShortFileName() ShortName = %s\n", Buffer);
}

/*
 * FatGetFatEntry()
 * returns the Fat entry for a given cluster number
 */
BOOLEAN FatGetFatEntry(PFAT_VOLUME_INFO Volume, ULONG Cluster, ULONG* ClusterPointer)
{
    ULONG        fat = 0;
    UINT32        FatOffset;
    UINT32        ThisFatSecNum;
    UINT32        ThisFatEntOffset;
    ULONG SectorCount;
    PUCHAR ReadBuffer;
    BOOLEAN Success = TRUE;

    //TRACE("FatGetFatEntry() Retrieving FAT entry for cluster %d.\n", Cluster);

    // We need a buffer for 2 secors
    ReadBuffer = FrLdrTempAlloc(2 * Volume->BytesPerSector, TAG_FAT_BUFFER);
    if (!ReadBuffer)
    {
        return FALSE;
    }

    switch(Volume->FatType)
    {
    case FAT12:

        FatOffset = Cluster + (Cluster / 2);
        ThisFatSecNum = Volume->ActiveFatSectorStart + (FatOffset / Volume->BytesPerSector);
        ThisFatEntOffset = (FatOffset % Volume->BytesPerSector);

        TRACE("FatOffset: %d\n", FatOffset);
        TRACE("ThisFatSecNum: %d\n", ThisFatSecNum);
        TRACE("ThisFatEntOffset: %d\n", ThisFatEntOffset);

        if (ThisFatEntOffset == (Volume->BytesPerSector - 1))
        {
            SectorCount = 2;
        }
        else
        {
            SectorCount = 1;
        }

        if (!FatReadVolumeSectors(Volume, ThisFatSecNum, SectorCount, ReadBuffer))
        {
            Success = FALSE;
            break;
        }

        fat = *((USHORT *) (ReadBuffer + ThisFatEntOffset));
        fat = SWAPW(fat);
        if (Cluster & 0x0001)
            fat = fat >> 4;    /* Cluster number is ODD */
        else
            fat = fat & 0x0FFF;    /* Cluster number is EVEN */

        break;

    case FAT16:
    case FATX16:

        FatOffset = (Cluster * 2);
        ThisFatSecNum = Volume->ActiveFatSectorStart + (FatOffset / Volume->BytesPerSector);
        ThisFatEntOffset = (FatOffset % Volume->BytesPerSector);

        if (!FatReadVolumeSectors(Volume, ThisFatSecNum, 1, ReadBuffer))
        {
            Success = FALSE;
            break;
        }

        fat = *((USHORT *) (ReadBuffer + ThisFatEntOffset));
        fat = SWAPW(fat);

        break;

    case FAT32:
    case FATX32:

        FatOffset = (Cluster * 4);
        ThisFatSecNum = Volume->ActiveFatSectorStart + (FatOffset / Volume->BytesPerSector);
        ThisFatEntOffset = (FatOffset % Volume->BytesPerSector);

        if (!FatReadVolumeSectors(Volume, ThisFatSecNum, 1, ReadBuffer))
        {
            return FALSE;
        }

        // Get the fat entry
        fat = (*((ULONG *) (ReadBuffer + ThisFatEntOffset))) & 0x0FFFFFFF;
        fat = SWAPD(fat);

        break;

    default:
        ERR("Unknown FAT type %d\n", Volume->FatType);
        Success = FALSE;
        break;
    }

    //TRACE("FAT entry is 0x%x.\n", fat);

    FrLdrTempFree(ReadBuffer, TAG_FAT_BUFFER);

    *ClusterPointer = fat;

    return Success;
}

ULONG FatCountClustersInChain(PFAT_VOLUME_INFO Volume, ULONG StartCluster)
{
    ULONG    ClusterCount = 0;

    TRACE("FatCountClustersInChain() StartCluster = %d\n", StartCluster);

    while (1)
    {
        //
        // If end of chain then break out of our cluster counting loop
        //
        if (((Volume->FatType == FAT12) && (StartCluster >= 0xff8)) ||
            ((Volume->FatType == FAT16 || Volume->FatType == FATX16) && (StartCluster >= 0xfff8)) ||
            ((Volume->FatType == FAT32 || Volume->FatType == FATX32) && (StartCluster >= 0x0ffffff8)))
        {
            break;
        }

        //
        // Increment count
        //
        ClusterCount++;

        //
        // Get next cluster
        //
        if (!FatGetFatEntry(Volume, StartCluster, &StartCluster))
        {
            return 0;
        }
    }

    TRACE("FatCountClustersInChain() ClusterCount = %d\n", ClusterCount);

    return ClusterCount;
}

ULONG* FatGetClusterChainArray(PFAT_VOLUME_INFO Volume, ULONG StartCluster)
{
    ULONG    ClusterCount;
    ULONG        ArraySize;
    ULONG*    ArrayPointer;
    ULONG        Idx;

    TRACE("FatGetClusterChainArray() StartCluster = %d\n", StartCluster);

    ClusterCount = FatCountClustersInChain(Volume, StartCluster) + 1; // Lets get the 0x0ffffff8 on the end of the array
    ArraySize = ClusterCount * sizeof(ULONG);

    //
    // Allocate array memory
    //
    ArrayPointer = FrLdrTempAlloc(ArraySize, TAG_FAT_CHAIN);

    if (ArrayPointer == NULL)
    {
        return NULL;
    }

    //
    // Loop through and set array values
    //
    for (Idx=0; Idx<ClusterCount; Idx++)
    {
        //
        // Set current cluster
        //
        ArrayPointer[Idx] = StartCluster;

        //
        // Don't try to get next cluster for last cluster
        //
        if (((Volume->FatType == FAT12) && (StartCluster >= 0xff8)) ||
            ((Volume->FatType == FAT16 || Volume->FatType == FATX16) && (StartCluster >= 0xfff8)) ||
            ((Volume->FatType == FAT32 || Volume->FatType == FATX32) && (StartCluster >= 0x0ffffff8)))
        {
            Idx++;
            break;
        }

        //
        // Get next cluster
        //
        if (!FatGetFatEntry(Volume, StartCluster, &StartCluster))
        {
            FrLdrTempFree(ArrayPointer, TAG_FAT_CHAIN);
            return NULL;
        }
    }

    return ArrayPointer;
}

/*
 * FatReadClusterChain()
 * Reads the specified clusters into memory
 */
BOOLEAN FatReadClusterChain(PFAT_VOLUME_INFO Volume, ULONG StartClusterNumber, ULONG NumberOfClusters, PVOID Buffer)
{
    ULONG        ClusterStartSector;

    TRACE("FatReadClusterChain() StartClusterNumber = %d NumberOfClusters = %d Buffer = 0x%x\n", StartClusterNumber, NumberOfClusters, Buffer);

    while (NumberOfClusters > 0)
    {

        //TRACE("FatReadClusterChain() StartClusterNumber = %d NumberOfClusters = %d Buffer = 0x%x\n", StartClusterNumber, NumberOfClusters, Buffer);
        //
        // Calculate starting sector for cluster
        //
        ClusterStartSector = ((StartClusterNumber - 2) * Volume->SectorsPerCluster) + Volume->DataSectorStart;

        //
        // Read cluster into memory
        //
        if (!FatReadVolumeSectors(Volume, ClusterStartSector, Volume->SectorsPerCluster, Buffer))
        {
            return FALSE;
        }

        //
        // Decrement count of clusters left to read
        //
        NumberOfClusters--;

        //
        // Increment buffer address by cluster size
        //
        Buffer = (PVOID)((ULONG_PTR)Buffer + (Volume->SectorsPerCluster * Volume->BytesPerSector));

        //
        // Get next cluster
        //
        if (!FatGetFatEntry(Volume, StartClusterNumber, &StartClusterNumber))
        {
            return FALSE;
        }

        //
        // If end of chain then break out of our cluster reading loop
        //
        if (((Volume->FatType == FAT12) && (StartClusterNumber >= 0xff8)) ||
            ((Volume->FatType == FAT16 || Volume->FatType == FATX16) && (StartClusterNumber >= 0xfff8)) ||
            ((Volume->FatType == FAT32 || Volume->FatType == FATX32) && (StartClusterNumber >= 0x0ffffff8)))
        {
            break;
        }
    }

    return TRUE;
}

/*
 * FatReadPartialCluster()
 * Reads part of a cluster into memory
 */
BOOLEAN FatReadPartialCluster(PFAT_VOLUME_INFO Volume, ULONG ClusterNumber, ULONG StartingOffset, ULONG Length, PVOID Buffer)
{
    ULONG        ClusterStartSector;
    ULONG SectorOffset, ReadSize, SectorCount;
    PUCHAR ReadBuffer;
    BOOLEAN Success = FALSE;

    //TRACE("FatReadPartialCluster() ClusterNumber = %d StartingOffset = %d Length = %d Buffer = 0x%x\n", ClusterNumber, StartingOffset, Length, Buffer);

    ClusterStartSector = ((ClusterNumber - 2) * Volume->SectorsPerCluster) + Volume->DataSectorStart;

    // This is the offset of the data in sectors
    SectorOffset = (StartingOffset / Volume->BytesPerSector);
    StartingOffset %= Volume->BytesPerSector;

    // Calculate how many sectors we need to read
    SectorCount = (StartingOffset + Length + Volume->BytesPerSector - 1) / Volume->BytesPerSector;

    // Calculate rounded up read size
    ReadSize = SectorCount * Volume->BytesPerSector;

    ReadBuffer = FrLdrTempAlloc(ReadSize, TAG_FAT_BUFFER);
    if (!ReadBuffer)
    {
        return FALSE;
    }

    if (FatReadVolumeSectors(Volume, ClusterStartSector + SectorOffset, SectorCount, ReadBuffer))
    {
        memcpy(Buffer, ReadBuffer + StartingOffset, Length);
        Success = TRUE;
    }

    FrLdrTempFree(ReadBuffer, TAG_FAT_BUFFER);

    return Success;
}

/*
 * FatReadFile()
 * Reads BytesToRead from open file and
 * returns the number of bytes read in BytesRead
 */
BOOLEAN FatReadFile(PFAT_FILE_INFO FatFileInfo, ULONG BytesToRead, ULONG* BytesRead, PVOID Buffer)
{
    PFAT_VOLUME_INFO Volume = FatFileInfo->Volume;
    ULONG            ClusterNumber;
    ULONG            OffsetInCluster;
    ULONG            LengthInCluster;
    ULONG            NumberOfClusters;
    ULONG            BytesPerCluster;

    TRACE("FatReadFile() BytesToRead = %d Buffer = 0x%x\n", BytesToRead, Buffer);

    if (BytesRead != NULL)
    {
        *BytesRead = 0;
    }

    //
    // If they are trying to read past the
    // end of the file then return success
    // with BytesRead == 0
    //
    if (FatFileInfo->FilePointer >= FatFileInfo->FileSize)
    {
        return TRUE;
    }

    //
    // If they are trying to read more than there is to read
    // then adjust the amount to read
    //
    if ((FatFileInfo->FilePointer + BytesToRead) > FatFileInfo->FileSize)
    {
        BytesToRead = (FatFileInfo->FileSize - FatFileInfo->FilePointer);
    }

    //
    // Ok, now we have to perform at most 3 calculations
    // I'll draw you a picture (using nifty ASCII art):
    //
    // CurrentFilePointer -+
    //                     |
    //    +----------------+
    //    |
    // +-----------+-----------+-----------+-----------+
    // | Cluster 1 | Cluster 2 | Cluster 3 | Cluster 4 |
    // +-----------+-----------+-----------+-----------+
    //    |                                    |
    //    +---------------+--------------------+
    //                    |
    // BytesToRead -------+
    //
    // 1 - The first calculation (and read) will align
    //     the file pointer with the next cluster.
    //     boundary (if we are supposed to read that much)
    // 2 - The next calculation (and read) will read
    //     in all the full clusters that the requested
    //     amount of data would cover (in this case
    //     clusters 2 & 3).
    // 3 - The last calculation (and read) would read
    //     in the remainder of the data requested out of
    //     the last cluster.
    //

    BytesPerCluster = Volume->SectorsPerCluster * Volume->BytesPerSector;

    //
    // Only do the first read if we
    // aren't aligned on a cluster boundary
    //
    if (FatFileInfo->FilePointer % BytesPerCluster)
    {
        //
        // Do the math for our first read
        //
        ClusterNumber = (FatFileInfo->FilePointer / BytesPerCluster);
        ClusterNumber = FatFileInfo->FileFatChain[ClusterNumber];
        OffsetInCluster = (FatFileInfo->FilePointer % BytesPerCluster);
        LengthInCluster = (BytesToRead > (BytesPerCluster - OffsetInCluster)) ? (BytesPerCluster - OffsetInCluster) : BytesToRead;

        //
        // Now do the read and update BytesRead, BytesToRead, FilePointer, & Buffer
        //
        if (!FatReadPartialCluster(Volume, ClusterNumber, OffsetInCluster, LengthInCluster, Buffer))
        {
            return FALSE;
        }
        if (BytesRead != NULL)
        {
            *BytesRead += LengthInCluster;
        }
        BytesToRead -= LengthInCluster;
        FatFileInfo->FilePointer += LengthInCluster;
        Buffer = (PVOID)((ULONG_PTR)Buffer + LengthInCluster);
    }

    //
    // Do the math for our second read (if any data left)
    //
    if (BytesToRead > 0)
    {
        //
        // Determine how many full clusters we need to read
        //
        NumberOfClusters = (BytesToRead / BytesPerCluster);

        if (NumberOfClusters > 0)
        {
            ClusterNumber = (FatFileInfo->FilePointer / BytesPerCluster);
            ClusterNumber = FatFileInfo->FileFatChain[ClusterNumber];

            //
            // Now do the read and update BytesRead, BytesToRead, FilePointer, & Buffer
            //
            if (!FatReadClusterChain(Volume, ClusterNumber, NumberOfClusters, Buffer))
            {
                return FALSE;
            }
            if (BytesRead != NULL)
            {
                *BytesRead += (NumberOfClusters * BytesPerCluster);
            }
            BytesToRead -= (NumberOfClusters * BytesPerCluster);
            FatFileInfo->FilePointer += (NumberOfClusters * BytesPerCluster);
            Buffer = (PVOID)((ULONG_PTR)Buffer + (NumberOfClusters * BytesPerCluster));
        }
    }

    //
    // Do the math for our third read (if any data left)
    //
    if (BytesToRead > 0)
    {
        ClusterNumber = (FatFileInfo->FilePointer / BytesPerCluster);
        ClusterNumber = FatFileInfo->FileFatChain[ClusterNumber];

        //
        // Now do the read and update BytesRead, BytesToRead, FilePointer, & Buffer
        //
        if (!FatReadPartialCluster(Volume, ClusterNumber, 0, BytesToRead, Buffer))
        {
            return FALSE;
        }
        if (BytesRead != NULL)
        {
            *BytesRead += BytesToRead;
        }
        FatFileInfo->FilePointer += BytesToRead;
        BytesToRead -= BytesToRead;
        Buffer = (PVOID)((ULONG_PTR)Buffer + BytesToRead);
    }

    return TRUE;
}

BOOLEAN FatReadVolumeSectors(PFAT_VOLUME_INFO Volume, ULONG SectorNumber, ULONG SectorCount, PVOID Buffer)
{
    LARGE_INTEGER Position;
    ULONG Count;
    ARC_STATUS Status;

    //TRACE("FatReadVolumeSectors(): SectorNumber %d, SectorCount %d, Buffer %p\n",
    //    SectorNumber, SectorCount, Buffer);

    //
    // Seek to right position
    //
    Position.QuadPart = (ULONGLONG)SectorNumber * 512;
    Status = ArcSeek(Volume->DeviceId, &Position, SeekAbsolute);
    if (Status != ESUCCESS)
    {
        TRACE("FatReadVolumeSectors() Failed to seek\n");
        return FALSE;
    }

    //
    // Read data
    //
    Status = ArcRead(Volume->DeviceId, Buffer, SectorCount * 512, &Count);
    if (Status != ESUCCESS || Count != SectorCount * 512)
    {
        TRACE("FatReadVolumeSectors() Failed to read\n");
        return FALSE;
    }

    // Return success
    return TRUE;
}

ARC_STATUS FatClose(ULONG FileId)
{
    PFAT_FILE_INFO FileHandle = FsGetDeviceSpecific(FileId);

    if (FileHandle->FileFatChain) FrLdrTempFree(FileHandle->FileFatChain, TAG_FAT_CHAIN);
    FrLdrTempFree(FileHandle, TAG_FAT_FILE);

    return ESUCCESS;
}

ARC_STATUS FatGetFileInformation(ULONG FileId, FILEINFORMATION* Information)
{
    PFAT_FILE_INFO FileHandle = FsGetDeviceSpecific(FileId);

    RtlZeroMemory(Information, sizeof(FILEINFORMATION));
    Information->EndingAddress.LowPart = FileHandle->FileSize;
    Information->CurrentAddress.LowPart = FileHandle->FilePointer;

    TRACE("FatGetFileInformation() FileSize = %d\n",
        Information->EndingAddress.LowPart);
    TRACE("FatGetFileInformation() FilePointer = %d\n",
        Information->CurrentAddress.LowPart);

    return ESUCCESS;
}

ARC_STATUS FatOpen(CHAR* Path, OPENMODE OpenMode, ULONG* FileId)
{
    PFAT_VOLUME_INFO FatVolume;
    FAT_FILE_INFO TempFileInfo;
    PFAT_FILE_INFO FileHandle;
    ULONG DeviceId;
    BOOLEAN IsDirectory;
    ARC_STATUS Status;

    if (OpenMode != OpenReadOnly && OpenMode != OpenDirectory)
        return EACCES;

    DeviceId = FsGetDeviceId(*FileId);
    FatVolume = FatVolumes[DeviceId];

    TRACE("FatOpen() FileName = %s\n", Path);

    RtlZeroMemory(&TempFileInfo, sizeof(TempFileInfo));
    Status = FatLookupFile(FatVolume, Path, DeviceId, &TempFileInfo);
    if (Status != ESUCCESS)
        return ENOENT;

    //
    // Check if caller opened what he expected (dir vs file)
    //
    IsDirectory = (TempFileInfo.Attributes & ATTR_DIRECTORY) != 0;
    if (IsDirectory && OpenMode != OpenDirectory)
        return EISDIR;
    else if (!IsDirectory && OpenMode != OpenReadOnly)
        return ENOTDIR;

    FileHandle = FrLdrTempAlloc(sizeof(FAT_FILE_INFO), TAG_FAT_FILE);
    if (!FileHandle)
        return ENOMEM;

    RtlCopyMemory(FileHandle, &TempFileInfo, sizeof(FAT_FILE_INFO));
    FileHandle->Volume = FatVolume;

    FsSetDeviceSpecific(*FileId, FileHandle);
    return ESUCCESS;
}

ARC_STATUS FatRead(ULONG FileId, VOID* Buffer, ULONG N, ULONG* Count)
{
    PFAT_FILE_INFO FileHandle = FsGetDeviceSpecific(FileId);
    BOOLEAN Success;

    //
    // Call old read method
    //
    Success = FatReadFile(FileHandle, N, Count, Buffer);

    //
    // Check for success
    //
    if (Success)
        return ESUCCESS;
    else
        return EIO;
}

ARC_STATUS FatSeek(ULONG FileId, LARGE_INTEGER* Position, SEEKMODE SeekMode)
{
    PFAT_FILE_INFO FileHandle = FsGetDeviceSpecific(FileId);

    TRACE("FatSeek() NewFilePointer = %lu\n", Position->LowPart);

    if (SeekMode != SeekAbsolute)
        return EINVAL;
    if (Position->HighPart != 0)
        return EINVAL;
    if (Position->LowPart >= FileHandle->FileSize)
        return EINVAL;

    FileHandle->FilePointer = Position->LowPart;
    return ESUCCESS;
}

const DEVVTBL FatFuncTable =
{
    FatClose,
    FatGetFileInformation,
    FatOpen,
    FatRead,
    FatSeek,
    L"fastfat",
};

const DEVVTBL* FatMount(ULONG DeviceId)
{
    PFAT_VOLUME_INFO Volume;
    UCHAR Buffer[512];
    PFAT_BOOTSECTOR BootSector = (PFAT_BOOTSECTOR)Buffer;
    PFAT32_BOOTSECTOR BootSector32 = (PFAT32_BOOTSECTOR)Buffer;
    PFATX_BOOTSECTOR BootSectorX = (PFATX_BOOTSECTOR)Buffer;
    FILEINFORMATION FileInformation;
    LARGE_INTEGER Position;
    ULONG Count;
    ULARGE_INTEGER SectorCount;
    ARC_STATUS Status;

    //
    // Allocate data for volume information
    //
    Volume = FrLdrTempAlloc(sizeof(FAT_VOLUME_INFO), TAG_FAT_VOLUME);
    if (!Volume)
        return NULL;
    RtlZeroMemory(Volume, sizeof(FAT_VOLUME_INFO));

    //
    // Read the BootSector
    //
    Position.HighPart = 0;
    Position.LowPart = 0;
    Status = ArcSeek(DeviceId, &Position, SeekAbsolute);
    if (Status != ESUCCESS)
    {
        FrLdrTempFree(Volume, TAG_FAT_VOLUME);
        return NULL;
    }
    Status = ArcRead(DeviceId, Buffer, sizeof(Buffer), &Count);
    if (Status != ESUCCESS || Count != sizeof(Buffer))
    {
        FrLdrTempFree(Volume, TAG_FAT_VOLUME);
        return NULL;
    }

    //
    // Check if BootSector is valid. If no, return early
    //
    if (!RtlEqualMemory(BootSector->FileSystemType, "FAT12   ", 8) &&
        !RtlEqualMemory(BootSector->FileSystemType, "FAT16   ", 8) &&
        !RtlEqualMemory(BootSector32->FileSystemType, "FAT32   ", 8) &&
        !RtlEqualMemory(BootSectorX->FileSystemType, "FATX", 4))
    {
        FrLdrTempFree(Volume, TAG_FAT_VOLUME);
        return NULL;
    }

    //
    // Determine sector count
    //
    Status = ArcGetFileInformation(DeviceId, &FileInformation);
    if (Status != ESUCCESS)
    {
        FrLdrTempFree(Volume, TAG_FAT_VOLUME);
        return NULL;
    }
    SectorCount.HighPart = FileInformation.EndingAddress.HighPart;
    SectorCount.LowPart = FileInformation.EndingAddress.LowPart;
    SectorCount.QuadPart /= SECTOR_SIZE;

    //
    // Keep device id
    //
    Volume->DeviceId = DeviceId;

    //
    // Really open the volume
    //
    if (!FatOpenVolume(Volume, BootSector, SectorCount.QuadPart))
    {
        FrLdrTempFree(Volume, TAG_FAT_VOLUME);
        return NULL;
    }

    //
    // Remember FAT volume information
    //
    FatVolumes[DeviceId] = Volume;

    //
    // Return success
    //
    return &FatFuncTable;
}
