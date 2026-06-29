/*
 * PROJECT:     FreeLoader
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     GPT partitioning scheme support
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

/* GPT (GUID Partition Table) definitions */
#define EFI_PARTITION_HEADER_SIGNATURE  "EFI PART"
#define EFI_HEADER_LOCATION             1ULL
#define EFI_TABLE_REVISION              0x00010000
#define EFI_PARTITION_ENTRIES_BLOCK     2ULL
#define EFI_PARTITION_ENTRY_COUNT       128
#define EFI_PARTITION_ENTRY_SIZE        128
#define EFI_PARTITION_NAME_LENGTH       36

/* GPT Partition Type GUIDs */
#define EFI_PART_TYPE_UNUSED_GUID \
    {0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}

#define EFI_PART_TYPE_EFI_SYSTEM_PART_GUID \
    {0xc12a7328, 0xf81f, 0x11d2, {0xba, 0x4b, 0x00, 0xa0, 0xc9, 0x3e, 0xc9, 0x3b}}

#ifndef PARTITION_GPT
#define PARTITION_GPT                   0xEE      // GPT protective partition
#endif

#include <pshpack1.h>

/* GPT Table Header */
typedef struct _GPT_TABLE_HEADER
{
    CHAR    Signature[8];               /* "EFI PART" */
    UINT32  Revision;                   /* 0x00010000 */
    UINT32  HeaderSize;                 /* Size of header (usually 92) */
    UINT32  HeaderCrc32;                /* CRC32 of header */
    UINT32  Reserved;                   /* Must be 0 */
    UINT64  MyLba;                      /* LBA of this header */
    UINT64  AlternateLba;               /* LBA of alternate header */
    UINT64  FirstUsableLba;             /* First usable LBA for partitions */
    UINT64  LastUsableLba;              /* Last usable LBA for partitions */
    GUID    DiskGuid;                   /* Disk GUID */
    UINT64  PartitionEntryLba;          /* LBA of partition entries array */
    UINT32  NumberOfPartitionEntries;   /* Number of partition entries */
    UINT32  SizeOfPartitionEntry;       /* Size of each entry (usually 128) */
    UINT32  PartitionEntryArrayCrc32;   /* CRC32 of partition entries array */
} GPT_TABLE_HEADER, *PGPT_TABLE_HEADER;
C_ASSERT(sizeof(GPT_TABLE_HEADER) == 92);

/* GPT Partition Entry */
typedef struct _GPT_PARTITION_ENTRY
{
    GUID    PartitionTypeGuid;          /* Partition type GUID */
    GUID    UniquePartitionGuid;        /* Unique partition GUID */
    UINT64  StartingLba;                /* Starting LBA */
    UINT64  EndingLba;                  /* Ending LBA */
    UINT64  Attributes;                 /* Partition attributes */
    WCHAR   PartitionName[EFI_PARTITION_NAME_LENGTH]; /* Partition name (UTF-16) */
} GPT_PARTITION_ENTRY, *PGPT_PARTITION_ENTRY;
C_ASSERT(sizeof(GPT_PARTITION_ENTRY) == 128);

#include <poppack.h>
