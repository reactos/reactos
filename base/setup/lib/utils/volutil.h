/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Volume utility functions
 * COPYRIGHT:   Copyright 2024 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#pragma once

/* The maximum volume label length is 32 characters */
#ifndef MAXIMUM_VOLUME_LABEL_LENGTH // Defined in wdm.h only
#define MAXIMUM_VOLUME_LABEL_LENGTH (32 * sizeof(WCHAR))
#endif

typedef struct _VOLINFO
{
    // WCHAR VolumeName[MAX_PATH]; ///< Name in the DOS/Win32 namespace: "\??\Volume{GUID}\"
    WCHAR DeviceName[MAX_PATH]; ///< NT device name: "\Device\HarddiskVolumeN"

    WCHAR DriveLetter;
    WCHAR VolumeLabel[MAXIMUM_VOLUME_LABEL_LENGTH / sizeof(WCHAR) + 1]; ///< Volume label, NUL-terminated
    WCHAR FileSystem[MAX_PATH+1];

    // VOLUME_TYPE VolumeType;
    // ULARGE_INTEGER Size;
    // PVOLUME_DISK_EXTENTS Extents;
} VOLINFO, *PVOLINFO;

/* RawFS "RAW" file system name */
#define IS_RAWFS(fs) \
    ((fs)[0] == 'R' && (fs)[1] == 'A' && (fs)[2] == 'W' && (fs)[3] == 0)

#define IsUnknown(VolInfo) \
    (!*(VolInfo)->FileSystem)

#define IsUnformatted(VolInfo) \
    IS_RAWFS((VolInfo)->FileSystem)

#define IsFormatted(VolInfo) \
    (!IsUnknown(VolInfo) && !IsUnformatted(VolInfo))


NTSTATUS
MountVolume(
    _Inout_ PVOLINFO Volume,
    _In_opt_ UCHAR MbrPartitionType);

NTSTATUS
DismountVolume(
    _Inout_ PVOLINFO Volume,
    _In_ BOOLEAN Force);

/* EOF */
