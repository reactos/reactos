/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS FS utility tool
 * FILE:            base/applications/cmdutils/dirty.c
 * PURPOSE:         FSutil dirty bit handling
 * PROGRAMMERS:     Pierre Schweitzer <pierre@reactos.org>
 */

#include "fsutil.h"
#include <winioctl.h>

/* Add handlers here for subcommands */
static HandlerProc QueryMain;
static HandlerProc SetMain;
static HandlerItem HandlersList[] =
{
    /* Proc, name, help */
    { QueryMain, _T("query"), _T("Show the dirty bit") },
    { SetMain, _T("set"), _T("Set the dirty bit") },
};

static int
QueryMain(int argc, const TCHAR *argv[])
{
    HANDLE Volume;
    ULONG VolumeStatus, BytesRead;

    /* We need a volume (letter or GUID) */
    if (argc < 2)
    {
        _ftprintf(stderr, _T("Usage: fsutil dirty query <volume>\n"));
        _ftprintf(stderr, _T("\tFor example: fsutil dirty query c:\n"));
        return 1;
    }

    /* Get a handle for the volume */
    Volume = OpenVolume(argv[1], FALSE, FALSE);
    if (Volume == INVALID_HANDLE_VALUE)
    {
        return 1;
    }

    /* And query the dirty status */
    if (DeviceIoControl(Volume, FSCTL_IS_VOLUME_DIRTY, NULL, 0, &VolumeStatus,
                        sizeof(ULONG), &BytesRead, NULL) == FALSE)
    {
        PrintErrorMessage(GetLastError());
        CloseHandle(Volume);
        return 1;
    }

    CloseHandle(Volume);

    /* Print the status */
    _ftprintf(stdout, _T("The %s volume is %s\n"), argv[1], ((VolumeStatus & VOLUME_IS_DIRTY) ? _T("dirty") : _T("clean")));

    return 0;
}

static int
SetMain(int argc, const TCHAR *argv[])
{
    HANDLE Volume;
    DWORD BytesRead;

    /* We need a volume (letter or GUID) */
    if (argc < 2)
    {
        _ftprintf(stderr, _T("Usage: fsutil dirty set <volume>\n"));
        _ftprintf(stderr, _T("\tFor example: fsutil dirty set c:\n"));
        return 1;
    }

    /* Get a handle for the volume */
    Volume = OpenVolume(argv[1], FALSE, FALSE);
    if (Volume == INVALID_HANDLE_VALUE)
    {
        return 1;
    }

    /* And set the dirty bit */
    if (DeviceIoControl(Volume, FSCTL_MARK_VOLUME_DIRTY, NULL, 0, NULL, 0, &BytesRead, NULL) == FALSE)
    {
        PrintErrorMessage(GetLastError());
        CloseHandle(Volume);
        return 1;
    }

    CloseHandle(Volume);

    /* Print the status */
    _ftprintf(stdout, _T("The %s volume is now marked as dirty\n"), argv[1]);

    return 0;
}

static void
PrintUsage(const TCHAR * Command)
{
    PrintDefaultUsage(_T(" DIRTY "), Command, (HandlerItem *)&HandlersList,
                      (sizeof(HandlersList) / sizeof(HandlersList[0])));
}

int
DirtyMain(int argc, const TCHAR *argv[])
{
    return FindHandler(argc, argv, (HandlerItem *)&HandlersList,
                       (sizeof(HandlersList) / sizeof(HandlersList[0])),
                       PrintUsage);
}
