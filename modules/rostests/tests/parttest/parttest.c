/*
 * PROJECT:         ReactOS partitions tests/dump
 * LICENSE:         LGPLv2+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Open disk & partition trying to get information about volumes & MBR
 * PROGRAMMER:      Pierre Schweitzer <pierre@reactos.org>
 */

#include <stdio.h>
#include <stdlib.h>
#include <winternl.h>

#ifndef NT_SUCCESS
# define NT_SUCCESS(_Status) (((NTSTATUS)(_Status)) >= 0)
#endif

#define SECTOR_SIZE 512
#define BOOT_RECORD_SIGNATURE 0xAA55

PCWSTR DiskFormat = L"\\Device\\Harddisk%lu\\Partition%lu";

#include <pshpack1.h>
typedef struct {
    unsigned char  magic0, res0, magic1;
    unsigned char  OEMName[8];
    unsigned short BytesPerSector;
    unsigned char  SectorsPerCluster;
    unsigned short ReservedSectors;
    unsigned char  FATCount;
    unsigned short RootEntries, Sectors;
    unsigned char  Media;
    unsigned short FATSectors, SectorsPerTrack, Heads;
    unsigned long  HiddenSectors, SectorsHuge;
    unsigned long  FATSectors32;
    unsigned short ExtFlag;
    unsigned short FSVersion;
    unsigned long  RootCluster;
    unsigned short FSInfoSector;
    unsigned short BootBackup;
    unsigned char  Res3[12];
    unsigned char  Drive;
    unsigned char  Res4;
    unsigned char  ExtBootSignature;
    unsigned long  VolumeID;
    unsigned char  VolumeLabel[11], SysType[8];
    unsigned char  Res2[420];
    unsigned short Signature1;
} FATBootSector, *PFATBootSector;

typedef struct {
    UCHAR     Jump[3];
    UCHAR     OEMID[8];
    USHORT    BytesPerSector;
    UCHAR     SectorsPerCluster;
    UCHAR     Unused0[7];
    UCHAR     MediaId;
    UCHAR     Unused1[2];
    USHORT    SectorsPerTrack;
    USHORT    Heads;
    UCHAR     Unused2[4];
    UCHAR     Unused3[4];
    USHORT    Unknown[2];
    ULONGLONG SectorCount;
    ULONGLONG MftLocation;
    ULONGLONG MftMirrLocation;
    CHAR      ClustersPerMftRecord;
    UCHAR     Unused4[3];
    CHAR      ClustersPerIndexRecord;
    UCHAR     Unused5[3];
    ULONGLONG SerialNumber;
    UCHAR     Checksum[4];
    UCHAR     BootStrap[426];
    USHORT    EndSector;
} NTFSBootSector, *PNTFSBootSector;

typedef struct {
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
} PARTITION_TABLE_ENTRY, *PPARTITION_TABLE_ENTRY;

typedef struct {
    UCHAR MasterBootRecordCodeAndData[0x1B8];
    ULONG Signature;
    USHORT Reserved;
    PARTITION_TABLE_ENTRY PartitionTable[4];
    USHORT MasterBootRecordMagic;
} MASTER_BOOT_RECORD, *PMASTER_BOOT_RECORD;
#include <poppack.h>

BOOL CheckAgainstFAT(PFATBootSector Sector)
{
    if (Sector->Signature1 != 0xaa55)
    {
        return FALSE;
    }

    if (Sector->BytesPerSector != 512 &&
        Sector->BytesPerSector != 1024 &&
        Sector->BytesPerSector != 2048 &&
        Sector->BytesPerSector != 4096)
    {
        return FALSE;
    }

    if (Sector->FATCount != 1 &&
        Sector->FATCount != 2)
    {
        return FALSE;
    }

    if (Sector->Media != 0xf0 &&
        Sector->Media != 0xf8 &&
        Sector->Media != 0xf9 &&
        Sector->Media != 0xfa &&
        Sector->Media != 0xfb &&
        Sector->Media != 0xfc &&
        Sector->Media != 0xfd &&
        Sector->Media != 0xfe &&
        Sector->Media != 0xff)
    {
        return FALSE;
    }

    if (Sector->SectorsPerCluster != 1 &&
        Sector->SectorsPerCluster != 2 &&
        Sector->SectorsPerCluster != 4 &&
        Sector->SectorsPerCluster != 8 &&
        Sector->SectorsPerCluster != 16 &&
        Sector->SectorsPerCluster != 32 &&
        Sector->SectorsPerCluster != 64 &&
        Sector->SectorsPerCluster != 128)
    {
        return FALSE;
    }

    if (Sector->BytesPerSector * Sector->SectorsPerCluster > 32 * 1024)
    {
        return FALSE;
    }

    return TRUE;
}

BOOL CheckAgainstNTFS(PNTFSBootSector Sector)
{
    ULONG k;
    ULONG ClusterSize;

    /* OEMID: this field must be NTFS */
    if (RtlCompareMemory(Sector->OEMID, "NTFS    ", 8) != 8)
    {
        return FALSE;
    }

    /* Unused0: this field must be COMPLETELY null */
    for (k = 0; k < 7; k++)
    {
        if (Sector->Unused0[k] != 0)
        {
            return FALSE;
        }
    }

    /* Unused3: this field must be COMPLETELY null */
    for (k = 0; k < 4; k++)
    {
        if (Sector->Unused3[k] != 0)
        {
            return FALSE;
        }
    }

    /* Check cluster size */
    ClusterSize = Sector->BytesPerSector * Sector->SectorsPerCluster;
    if (ClusterSize != 512 && ClusterSize != 1024 &&
        ClusterSize != 2048 && ClusterSize != 4096 &&
        ClusterSize != 8192 && ClusterSize != 16384 &&
        ClusterSize != 32768 && ClusterSize != 65536)
    {
        return FALSE;
    }

    return TRUE;
}

BOOL CheckAgainstMBR(PMASTER_BOOT_RECORD Sector)
{
    if (Sector->MasterBootRecordMagic != BOOT_RECORD_SIGNATURE)
    {
        return FALSE;
    }

    return TRUE;
}

int main(int argc, char ** argv)
{
    HANDLE FileHandle;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    WCHAR Buffer[MAX_PATH];
    UNICODE_STRING Name;
    PVOID Sector;

    Sector = malloc(SECTOR_SIZE);
    if (Sector == NULL)
    {
        fprintf(stderr, "Failed allocating memory!\n");
        return 0;
    }

    /* We first open disk */
    swprintf(Buffer, DiskFormat, 0, 0);
    RtlInitUnicodeString(&Name, Buffer);
    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        0,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        free(Sector);
        fprintf(stderr, "Failed opening disk! %lx\n", Status);
        return 0;
    }

    /* Read first sector of the disk */
    Status = NtReadFile(FileHandle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        Sector,
                        SECTOR_SIZE,
                        NULL,
                        NULL);
    NtClose(FileHandle);
    if (!NT_SUCCESS(Status))
    {
        free(Sector);
        fprintf(stderr, "Failed reading sector 0! %lx\n", Status);
        return 0;
    }

    /* Is it FAT? */
    if (CheckAgainstFAT(Sector))
    {
        printf("Sector 0 seems to be FAT boot sector\n");
    }
    /* Is it NTFS? */
    else if (CheckAgainstNTFS(Sector))
    {
        printf("Sector 0 seems to be NTFS boot sector\n");
    }
    /* Is it MBR? */
    else if (CheckAgainstMBR(Sector))
    {
        printf("Sector 0 might be MBR\n");
    }
    /* We don't support anything else */
    else
    {
        printf("Sector 0 not recognized\n");
    }

    /* Redo it with first partition */
    swprintf(Buffer, DiskFormat, 0, 1);
    RtlInitUnicodeString(&Name, Buffer);
    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        free(Sector);
        fprintf(stderr, "Failed opening partition! %lx\n", Status);
        return 0;
    }

    /* Read first sector of the partition */
    Status = NtReadFile(FileHandle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        Sector,
                        SECTOR_SIZE,
                        NULL,
                        NULL);
    if (!NT_SUCCESS(Status))
    {
        free(Sector);
        fprintf(stderr, "Failed reading first sector of the partition! %lx\n", Status);
        return 0;
    }

    /* Is it FAT? */
    if (CheckAgainstFAT(Sector))
    {
        printf("Seems to be a FAT partittion\n");
    }
    /* Is it NTFS? */
    else if (CheckAgainstNTFS(Sector))
    {
        printf("Seems to be a NTFS partition\n");
    }
    /* Is it MBR? */
    else if (CheckAgainstMBR(Sector))
    {
        printf("Seems to be MBR\n");
    }
    /* We don't support anything else */
    else
    {
        printf("Not recognized\n");
    }

    free(Sector);

    return 0;
}
