/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Volume list functions
 * COPYRIGHT:   Copyright 2024 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#pragma once

/* VOLUME UTILITY FUNCTIONS *************************************************/

typedef struct _VOLINFO
{
    // WCHAR VolumeName[MAX_PATH]; // Name in the DOS/Win32 namespace: "\??\Volume{GUID}\"
    WCHAR DeviceName[MAX_PATH]; // NT device name: "\Device\HarddiskVolumeN"

    WCHAR DriveLetter;
    WCHAR VolumeLabel[20];
    WCHAR FileSystem[MAX_PATH+1];

    // VOLUME_TYPE VolumeType;
    // ULARGE_INTEGER Size;
    // PVOLUME_DISK_EXTENTS Extents;
} VOLINFO, *PVOLINFO;

/* RawFS "RAW" file system name */
#define RAWFS_SIGNATURE     (*(ULONGLONG*)"R\0A\0W\0\0\0")
#define RAWFS_SIGNATURE_A   (*(ULONG*)"RAW\0")

#define IsUnknown(VolInfo) \
    (!*(VolInfo)->FileSystem)

#define IsUnformatted(VolInfo) \
    (*(ULONGLONG*)(VolInfo)->FileSystem == RAWFS_SIGNATURE)

#define IsFormatted(VolInfo) \
    (!IsUnknown(VolInfo) && !IsUnformatted(VolInfo))


// DetectFileSystem()
NTSTATUS
MountVolume(
    _Inout_ PVOLINFO Volume,
    _In_opt_ UCHAR MbrPartitionType);

NTSTATUS
DismountVolume(
    _Inout_ PVOLINFO Volume);

/* EOF */
