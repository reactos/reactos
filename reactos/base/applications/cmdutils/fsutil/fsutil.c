/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS FS utility tool
 * FILE:            base/applications/cmdutils/fsutil.c
 * PURPOSE:         FSutil main
 * PROGRAMMERS:     Pierre Schweitzer <pierre@reactos.org>
 */

#include "fsutil.h"

/* Add handlers here for commands */
int DirtyMain(int argc, const TCHAR *argv[]);
static HandlerItem HandlersList[] =
{
    /* Proc, name, help */
    { DirtyMain, _T("dirty"), _T("Manipulates the dirty bit") },
};

static void
PrintUsage(const TCHAR * Command)
{
    int i;

    /* If we were given a command, print it's not supported */
    if (Command != NULL)
    {
        _ftprintf(stderr, _T("Unhandled command: %s\n"), Command);
    }

    /* And dump any available command */
    _ftprintf(stderr, _T("---- Handled commands ----\n\n"));
    for (i = 0; i < (sizeof(HandlersList) / sizeof(HandlersList[0])); ++i)
    {
        _ftprintf(stderr, _T("%s\t%s\n"), HandlersList[i].Command, HandlersList[i].Desc);
    }
}

int
__cdecl
_tmain(int argc, const TCHAR *argv[])
{
    int i;
    int ret;
    const TCHAR * Command;

    ret = 1;
    Command = NULL;
    i = (sizeof(HandlersList) / sizeof(HandlersList[0]));

    /* If we have a command, does it match a known one? */
    if (argc > 1)
    {
        /* Browse all the known commands finding the right one */
        Command = argv[1];
        for (i = 0; i < (sizeof(HandlersList) / sizeof(HandlersList[0])); ++i)
        {
            if (_tcsicmp(Command, HandlersList[i].Command) == 0)
            {
                ret = HandlersList[i].Handler(argc - 1, &argv[1]);
                break;
            }
        }
    }

    /* We failed finding someone to handle the caller's needs, print out */
    if (i == (sizeof(HandlersList) / sizeof(HandlersList[0])))
    {
        PrintUsage(Command);
    }

    return ret;
}
