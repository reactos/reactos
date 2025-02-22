/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS FS utility tool
 * FILE:            base/applications/cmdutils/fsutil.c
 * PURPOSE:         FSutil main
 * PROGRAMMERS:     Pierre Schweitzer <pierre@reactos.org>
 */

#include "fsutil.h"

/* Add handlers here for commands */
HandlerProc DirtyMain;
HandlerProc FsInfoMain;
HandlerProc HardLinkMain;
HandlerProc VolumeMain;
static HandlerItem HandlersList[] =
{
    /* Proc, name, help */
    { DirtyMain, _T("dirty"), _T("Manipulates the dirty bit") },
    { FsInfoMain, _T("fsinfo"), _T("Gathers informations about file systems") },
    { HardLinkMain, _T("hardlink"), _T("Handles hard links") },
    { VolumeMain, _T("volume"), _T("Manages volumes") },
};

static void
PrintUsage(const TCHAR * Command)
{
    PrintDefaultUsage(_T(" "), Command, (HandlerItem *)&HandlersList,
                      (sizeof(HandlersList) / sizeof(HandlersList[0])));
}

int
__cdecl
_tmain(int argc, const TCHAR *argv[])
{
    return FindHandler(argc, argv, (HandlerItem *)&HandlersList,
                       (sizeof(HandlersList) / sizeof(HandlersList[0])),
                       PrintUsage);
}
