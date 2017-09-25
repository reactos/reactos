/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS FS utility tool
 * FILE:            base/applications/cmdutils/common.c
 * PURPOSE:         FSutil common functions
 * PROGRAMMERS:     Pierre Schweitzer <pierre@reactos.org>
 */

#include "fsutil.h"

int FindHandler(int argc,
                const TCHAR *argv[],
                HandlerItem * HandlersList,
                int HandlerListCount,
                void (*UsageHelper)(const TCHAR *))
{
    int i;
    int ret;
    const TCHAR * Command;

    ret = 1;
    Command = NULL;
    i = HandlerListCount;

    /* If we have a command, does it match a known one? */
    if (argc > 1)
    {
        /* Browse all the known commands finding the right one */
        Command = argv[1];
        for (i = 0; i < HandlerListCount; ++i)
        {
            if (_tcsicmp(Command, HandlersList[i].Command) == 0)
            {
                ret = HandlersList[i].Handler(argc - 1, &argv[1]);
                break;
            }
        }
    }

    /* We failed finding someone to handle the caller's needs, print out */
    if (i == HandlerListCount)
    {
        UsageHelper(Command);
    }

    return ret;
}

HANDLE OpenVolume(const TCHAR * Volume,
                  BOOLEAN AllowRemote,
                  BOOLEAN NtfsOnly)
{
    UINT Type;
    HANDLE hVolume;
    TCHAR VolumeID[PATH_MAX];

    /* Get volume type */
    if (!AllowRemote && Volume[1] == L':')
    {
        Type = GetDriveType(Volume);
        if (Type == DRIVE_REMOTE)
        {
            _ftprintf(stderr, _T("FSUTIL needs a local device\n"));
            return INVALID_HANDLE_VALUE;
        }
    }

    /* Get filesystem type */
    if (NtfsOnly)
    {
        TCHAR FileSystem[MAX_PATH + 1];

        _stprintf(VolumeID, _T("\\\\.\\%s\\"), Volume);
        if (!GetVolumeInformation(VolumeID, NULL,  0, NULL, NULL, NULL, FileSystem, MAX_PATH + 1))
        {
            PrintErrorMessage(GetLastError());
            return INVALID_HANDLE_VALUE;
        }

        if (_tcscmp(FileSystem, _T("NTFS")) != 0)
        {
            _ftprintf(stderr, _T("FSUTIL needs a NTFS device\n"));
            return INVALID_HANDLE_VALUE;
        }
    }

    /* Create full name */
    _stprintf(VolumeID, _T("\\\\.\\%s"), Volume);

    /* Open the volume */
    hVolume = CreateFile(VolumeID, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hVolume == INVALID_HANDLE_VALUE)
    {
        PrintErrorMessage(GetLastError());
        return INVALID_HANDLE_VALUE;
    }

    return hVolume;
}

void PrintDefaultUsage(const TCHAR * Command,
                       const TCHAR * SubCommand,
                       HandlerItem * HandlersList,
                       int HandlerListCount)
{
    int i;

    /* If we were given a command, print it's not supported */
    if (SubCommand != NULL)
    {
        _ftprintf(stderr, _T("Unhandled%scommand: %s\n"), Command, SubCommand);
    }

    /* And dump any available command */
    _ftprintf(stderr, _T("---- Handled%scommands ----\n\n"), Command);
    for (i = 0; i < HandlerListCount; ++i)
    {
        _ftprintf(stderr, _T("%s\t%s\n"), HandlersList[i].Command, HandlersList[i].Desc);
    }
}

int PrintErrorMessage(DWORD Error)
{
    TCHAR * String;

    /* Try to get textual error */
    if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                       NULL, Error, 0, (TCHAR *)&String, 0, NULL) != 0)
    {
        /* And print it */
        _ftprintf(stderr, _T("Error: %s\n"), String);
        LocalFree(String);
    }
    else
    {
        /* Otherwise, just print the error number */
        _ftprintf(stderr, _T("Error: %d\n"), Error);
    }

    return Error;
}
