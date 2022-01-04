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
    NTSTATUS Status;
    BOOLEAN Success = FALSE;
    WCHAR VolumeName[MAX_PATH];
    //CURDIR CurDir;

    Provider = GetProvider(Format);
    if (!Provider)
    {
        /* Unknown file system */
        Callback(DONE, 0, &Success);
        return;
    }

#if 1
    DPRINT1("Warning: use GetVolumeNameForVolumeMountPointW() instead!\n");
    swprintf(VolumeName, L"\\??\\%c:", towupper(DriveRoot[0]));
    RtlCreateUnicodeString(&usDriveRoot, VolumeName);
    /* Code disabled as long as our storage stack doesn't understand IOCTL_MOUNTDEV_QUERY_DEVICE_NAME */
#else
    if (!GetVolumeNameForVolumeMountPointW(DriveRoot, VolumeName, RTL_NUMBER_OF(VolumeName)) ||
        !RtlDosPathNameToNtPathName_U(VolumeName, &usDriveRoot, NULL, &CurDir))
    {
        /* Report an error */
        Callback(DONE, 0, &Success);
        return;
    }
#endif

    DPRINT("Chkdsk() - %S\n", Format);
    Status = STATUS_SUCCESS;
    Success = Provider->Chkdsk(&usDriveRoot,
                               Callback,
                               CorrectErrors,
                               Verbose,
                               CheckOnlyIfDirty,
                               ScanDrive,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               (PULONG)&Status);
    if (!Success)
        DPRINT1("Chkdsk() failed with Status 0x%lx\n", Status);

    /* Report success */
    Callback(DONE, 0, &Success);

    RtlFreeUnicodeString(&usDriveRoot);
}

/* EOF */
