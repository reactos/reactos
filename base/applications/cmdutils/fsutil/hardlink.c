/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS FS utility tool
 * FILE:            base/applications/cmdutils/hardlink.c
 * PURPOSE:         FSutil hard links handling
 * PROGRAMMERS:     Pierre Schweitzer <pierre@reactos.org>
 */

#include "fsutil.h"

/* Add handlers here for subcommands */
static HandlerProc CreateMain;
static HandlerItem HandlersList[] =
{
    /* Proc, name, help */
    { CreateMain, _T("create"), _T("Create a new hard link") },
};

static int
CreateMain(int argc, const TCHAR *argv[])
{
    TCHAR Source[MAX_PATH], Target[MAX_PATH];

    /* We need a source and a destination */
    if (argc < 3)
    {
        _ftprintf(stderr, _T("Usage: fsutil hardlink create <new> <existing>\n"));
        _ftprintf(stderr, _T("\tFor example: fsutil hardlink create c:\\target.txt c:\\source.txt\n"));
        return 1;
    }

    /* Get full names */
    if (GetFullPathName(argv[1], MAX_PATH, Target, NULL) == 0)
    {
        PrintErrorMessage(GetLastError());
        return 1;
    }

    if (GetFullPathName(argv[2], MAX_PATH, Source, NULL) == 0)
    {
        PrintErrorMessage(GetLastError());
        return 1;
    }

    /* Simply delegate to kernel32 */
    if (!CreateHardLink(Target, Source, NULL))
    {
        PrintErrorMessage(GetLastError());
        return 1;
    }

    /* Print the status */
    _ftprintf(stdout, _T("Hard link created for %s <=> %s\n"), Target, Source);

    return 0;
}

static void
PrintUsage(const TCHAR * Command)
{
    PrintDefaultUsage(_T(" HARDLINK "), Command, (HandlerItem *)&HandlersList,
                      (sizeof(HandlersList) / sizeof(HandlersList[0])));
}

int
HardLinkMain(int argc, const TCHAR *argv[])
{
    return FindHandler(argc, argv, (HandlerItem *)&HandlersList,
                       (sizeof(HandlersList) / sizeof(HandlersList[0])),
                       PrintUsage);
}
