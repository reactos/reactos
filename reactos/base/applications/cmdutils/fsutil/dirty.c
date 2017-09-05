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
static int QueryMain(int argc, const TCHAR *argv[]);
static int SetMain(int argc, const TCHAR *argv[]);
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
    TCHAR VolumeID[PATH_MAX];
    ULONG VolumeStatus, BytesRead;

    /* We need a volume (letter or GUID) */
    if (argc < 2)
    {
        _ftprintf(stderr, _T("Usage: fsutil dirty query <volume>\n"));
        _ftprintf(stderr, _T("\tFor example: fsutil dirty query c:\n"));
        return 1;
    }

    /* Create full name */
    _stprintf(VolumeID, _T("\\\\.\\%s"), argv[1]);

    /* Open the volume */
    Volume = CreateFile(VolumeID, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (Volume == INVALID_HANDLE_VALUE)
    {
        _ftprintf(stderr, _T("Error: %d\n"), GetLastError());
        return 1;
    }

    /* And query the dirty status */
    if (DeviceIoControl(Volume, FSCTL_IS_VOLUME_DIRTY, NULL, 0, &VolumeStatus,
                        sizeof(ULONG), &BytesRead, NULL) == FALSE)
    {
        _ftprintf(stderr, _T("Error: %d\n"), GetLastError());
        CloseHandle(Volume);
        return 1;
    }

    CloseHandle(Volume);

    /* Print the status */
    _ftprintf(stdout, _T("The %s volume is %s\n"), argv[1], (VolumeStatus & VOLUME_IS_DIRTY ? _T("dirty") : _T("clean")));

    return 0;
}

static int
SetMain(int argc, const TCHAR *argv[])
{
    HANDLE Volume;
    DWORD BytesRead;
    TCHAR VolumeID[PATH_MAX];

    /* We need a volume (letter or GUID) */
    if (argc < 2)
    {
        _ftprintf(stderr, _T("Usage: fsutil dirty set <volume>\n"));
        _ftprintf(stderr, _T("\tFor example: fsutil dirty set c:\n"));
        return 1;
    }

    /* Create full name */
    _stprintf(VolumeID, _T("\\\\.\\%s"), argv[1]);

    /* Open the volume */
    Volume = CreateFile(VolumeID, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (Volume == INVALID_HANDLE_VALUE)
    {
        _ftprintf(stderr, _T("Error: %d\n"), GetLastError());
        return 1;
    }

    /* And set the dirty bit */
    if (DeviceIoControl(Volume, FSCTL_MARK_VOLUME_DIRTY, NULL, 0, NULL, 0, &BytesRead, NULL) == FALSE)
    {
        _ftprintf(stderr, _T("Error: %d\n"), GetLastError());
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
