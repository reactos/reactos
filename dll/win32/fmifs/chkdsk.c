/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         File Management IFS Utility functions
 * FILE:            reactos/dll/win32/fmifs/chkdsk.c
 * PURPOSE:         Disk Checker
 *
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#include "precomp.h"
#include <ntstrsafe.h>

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
    WCHAR DriveName[MAX_PATH];
    WCHAR VolumeName[MAX_PATH];

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
        /* Couldn't get a volume GUID path, try checking using a parameter provided path */
        DPRINT1("Couldn't get a volume GUID path for drive %S\n", DriveName);
        wcscpy(VolumeName, DriveName);
    }

    if (!RtlDosPathNameToNtPathName_U(VolumeName, &usDriveRoot, NULL, NULL))
        goto Quit;

    /* Trim the trailing backslash since we will work with a device object */
    usDriveRoot.Length -= sizeof(WCHAR);

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

    RtlFreeUnicodeString(&usDriveRoot);

Quit:
    /* Report result */
    Callback(DONE, 0, &Success);
}

/* EOF */
