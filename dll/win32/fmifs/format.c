/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         File Management IFS Utility functions
 * FILE:            reactos/dll/win32/fmifs/format.c
 * PURPOSE:         Volume format
 *
 * PROGRAMMERS:     Emanuele Aliberti
 *                  Hervé Poussineau (hpoussin@reactos.org)
 */

#include "precomp.h"
#include <ntstrsafe.h>

#define NDEBUG
#include <debug.h>

/* FMIFS.6 */
VOID NTAPI
Format(
    IN PWCHAR DriveRoot,
    IN FMIFS_MEDIA_FLAG MediaFlag,
    IN PWCHAR Format,
    IN PWCHAR Label,
    IN BOOLEAN QuickFormat,
    IN PFMIFSCALLBACK Callback)
{
    FormatEx(DriveRoot,
             MediaFlag,
             Format,
             Label,
             QuickFormat,
             0,
             Callback);
}

/* FMIFS.7 */
VOID
NTAPI
FormatEx(
    IN PWCHAR DriveRoot,
    IN FMIFS_MEDIA_FLAG MediaFlag,
    IN PWCHAR Format,
    IN PWCHAR Label,
    IN BOOLEAN QuickFormat,
    IN ULONG ClusterSize,
    IN PFMIFSCALLBACK Callback)
{
    PIFS_PROVIDER Provider;
    UNICODE_STRING usDriveRoot;
    UNICODE_STRING usLabel;
    BOOLEAN Success = FALSE;
    BOOLEAN BackwardCompatible = FALSE; // Default to latest FS versions.
    MEDIA_TYPE MediaType;
    WCHAR DriveName[MAX_PATH];
    WCHAR VolumeName[MAX_PATH];

//
// TODO: Convert filesystem Format into ULIB format string.
//

    Provider = GetProvider(Format);
    if (!Provider)
    {
        /* Unknown file system */
        goto Quit;
    }

    if (!NT_SUCCESS(RtlStringCchCopyW(DriveName, ARRAYSIZE(DriveName), DriveRoot)))
        goto Quit;

    if (DriveName[wcslen(DriveName) - 1] != L'\\')
    {
        /* Append the trailing backslash for GetVolumeNameForVolumeMountPointW */
        if (!NT_SUCCESS(RtlStringCchCatW(DriveName, ARRAYSIZE(DriveName), L"\\")))
            goto Quit;
    }

    if (!GetVolumeNameForVolumeMountPointW(DriveName, VolumeName, ARRAYSIZE(VolumeName)))
    {
        /* Couldn't get a volume GUID path, try formatting using a parameter provided path */
        DPRINT1("Couldn't get a volume GUID path for drive %S\n", DriveName);
        wcscpy(VolumeName, DriveName);
    }

    if (!RtlDosPathNameToNtPathName_U(VolumeName, &usDriveRoot, NULL, NULL))
        goto Quit;

    /* Trim the trailing backslash since we will work with a device object */
    usDriveRoot.Length -= sizeof(WCHAR);

    RtlInitUnicodeString(&usLabel, Label);

    /* Set the BackwardCompatible flag in case we format with older FAT12/16 */
    if (_wcsicmp(Format, L"FAT") == 0)
        BackwardCompatible = TRUE;
    // else if (wcsicmp(Format, L"FAT32") == 0)
        // BackwardCompatible = FALSE;

    /* Convert the FMIFS MediaFlag to a NT MediaType */
    // FIXME: Actually covert all the possible flags.
    switch (MediaFlag)
    {
    case FMIFS_FLOPPY:
        MediaType = F5_320_1024; // FIXME: This is hardfixed!
        break;
    case FMIFS_REMOVABLE:
        MediaType = RemovableMedia;
        break;
    case FMIFS_HARDDISK:
        MediaType = FixedMedia;
        break;
    default:
        DPRINT1("Unknown FMIFS MediaFlag %d, converting 1-to-1 to NT MediaType\n",
                MediaFlag);
        MediaType = (MEDIA_TYPE)MediaFlag;
        break;
    }

    DPRINT("Format() - %S\n", Format);
    Success = Provider->Format(&usDriveRoot,
                               Callback,
                               QuickFormat,
                               BackwardCompatible,
                               MediaType,
                               &usLabel,
                               ClusterSize);
    if (!Success)
        DPRINT1("Format() failed\n");

    RtlFreeUnicodeString(&usDriveRoot);

Quit:
    /* Report result */
    Callback(DONE, 0, &Success);
}

/* EOF */
