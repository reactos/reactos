/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Volume list functions
 * COPYRIGHT:   Copyright 2018-2019 Hermes Belusca-Maito
 */

#pragma once

/* EXTRA HANDFUL MACROS *****************************************************/

// NOTE: They should be moved into some global header.

// /* We have to define it there, because it is not in the MS DDK */
// #define PARTITION_LINUX 0x83

/* OEM MBR partition types recognized by NT (see [MS-DMRP] Appendix B) */
#define PARTITION_EISA          0x12    // EISA partition
#define PARTITION_HIBERNATION   0x84    // Hibernation partition for laptops
#define PARTITION_DIAGNOSTIC    0xA0    // Diagnostic partition on some Hewlett-Packard (HP) notebooks
#define PARTITION_DELL          0xDE    // Dell partition
#define PARTITION_IBM           0xFE    // IBM Initial Microprogram Load (IML) partition

#define IsOEMPartition(PartitionType) \
    ( ((PartitionType) == PARTITION_EISA)        || \
      ((PartitionType) == PARTITION_HIBERNATION) || \
      ((PartitionType) == PARTITION_DIAGNOSTIC)  || \
      ((PartitionType) == PARTITION_DELL)        || \
      ((PartitionType) == PARTITION_IBM) )


/* VOLUME UTILITY FUNCTIONS *************************************************/

// FORMATSTATE

typedef enum _VOLUME_TYPE
{
    VOLUME_TYPE_CDROM,
    VOLUME_TYPE_PARTITION,
    VOLUME_TYPE_REMOVABLE,
    VOLUME_TYPE_UNKNOWN
} VOLUME_TYPE, *PVOLUME_TYPE;

#if 0
//
// This is the structure from diskpart
//
typedef struct _VOLENTRY
{
    LIST_ENTRY ListEntry;

    ULONG VolumeNumber;
    WCHAR VolumeName[MAX_PATH];
    WCHAR DeviceName[MAX_PATH];

    WCHAR DriveLetter;

    PWSTR pszLabel;
    PWSTR pszFilesystem;
    VOLUME_TYPE VolumeType;
    ULARGE_INTEGER Size;

    PVOLUME_DISK_EXTENTS pExtents;

} VOLENTRY, *PVOLENTRY;
#else

typedef struct _VOLENTRY_TEMP
{
    LIST_ENTRY ListEntry;

    // ULONG PartitionNumber;       /* Current partition number, only valid for the currently running NTOS instance */

    WCHAR DriveLetter;
    WCHAR VolumeLabel[20];
    WCHAR FileSystem[MAX_PATH+1];
    FORMATSTATE FormatState;

/** The following three properties may be replaced by flags **/

    /* Volume is new and has not yet been actually formatted and mounted */
    BOOLEAN New;

    /* Volume must be checked */
    BOOLEAN NeedsCheck;

} VOLENTRY_TEMP, *PVOLENTRY_TEMP;

#define VOLENTRY  VOLENTRY_TEMP
#define PVOLENTRY PVOLENTRY_TEMP

#endif

// CreatePartition


// static
VOID
AssignDriveLetters(
    IN PPARTLIST List);

NTSTATUS
DetectFileSystem(
    _Inout_ PPARTENTRY PartEntry) // FIXME: Replace by volume entry
;

NTSTATUS
DismountVolume(
    _In_ PPARTENTRY PartEntry) // FIXME: Replace by volume entry
;

// BOOLEAN
// SetMountedDeviceValue(
//     IN WCHAR Letter,
//     IN ULONG Signature,
//     IN LARGE_INTEGER StartingOffset);

// BOOLEAN
// SetMountedDeviceValues(
//     IN PPARTLIST List);

/* EOF */
