/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         File Management IFS Utility functions
 * FILE:            reactos/dll/win32/fmifs/chkdsk.c
 * PURPOSE:         Disk Checker
 *
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

/* FMIFS.1 */
VOID
NTAPI
Chkdsk(
    IN PWCHAR DriveRoot,
    IN PWCHAR Format,
    IN BOOLEAN CorrectErrors,
    IN BOOLEAN Verbose,
    IN BOOLEAN CheckOnlyIfDirty,
    IN BOOLEAN ScanDrive,
    IN PVOID Unused2,
    IN PVOID Unused3,
    IN PFMIFSCALLBACK Callback)
{
    PIFS_PROVIDER Provider;
    UNICODE_STRING usDriveRoot;
    BOOLEAN Argument = FALSE;
    WCHAR VolumeName[MAX_PATH];
    //CURDIR CurDir;

    Provider = GetProvider(Format);
    if (!Provider)
    {
        /* Unknown file system */
        Callback(DONE, 0, &Argument);
        return;
    }

#if 1
    DPRINT1("Warning: use GetVolumeNameForVolumeMountPointW() instead!\n");
    swprintf(VolumeName, L"\\??\\%c:", towupper(DriveRoot[0]));
    RtlCreateUnicodeString(&usDriveRoot, VolumeName);
    /* Code disabled as long as our storage stack doesn't understand IOCTL_MOUNTDEV_QUERY_DEVICE_NAME */
#else
    if (!GetVolumeNameForVolumeMountPointW(DriveRoot, VolumeName, MAX_PATH) ||
        !RtlDosPathNameToNtPathName_U(VolumeName, &usDriveRoot, NULL, &CurDir))
    {
        /* Report an error. */
        Callback(DONE, 0, &Argument);
        return;
    }
#endif

    DPRINT("ChkdskEx - %S\n", Format);
    Provider->ChkdskEx(&usDriveRoot,
                       CorrectErrors,
                       Verbose,
                       CheckOnlyIfDirty,
                       ScanDrive,
                       Callback);

    RtlFreeUnicodeString(&usDriveRoot);
}

/* EOF */
