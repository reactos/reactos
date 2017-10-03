/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/help.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way.
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

/*
 * help_cmdlist():
 * shows all the available commands and basic descriptions for diskpart
 */
VOID help_cmdlist(VOID)
{
    PCOMMAND cmdptr;

    /* Print the header information */
    ConResPuts(StdOut, IDS_APP_HEADER);
    ConPuts(StdOut, L"\n");

    /* List all the commands and the basic descriptions */
    for (cmdptr = cmds; cmdptr->name; cmdptr++)
        ConResPuts(StdOut, cmdptr->help_desc);

    ConPuts(StdOut, L"\n");
}

/* help_main(char *arg):
 * main entry point for the help command. Gives help to users who needs it.
 */
BOOL help_main(INT argc, LPWSTR *argv)
{
    PCOMMAND cmdptr;

    if (argc == 1)
    {
        help_cmdlist();
        return TRUE;
    }

    /* Scan internal command table */
    for (cmdptr = cmds; cmdptr->name; cmdptr++)
    {
        if (_wcsicmp(argv[1], cmdptr->name) == 0)
        {
            ConResPuts(StdOut, cmdptr->help);
            return TRUE;
        }
    }

    help_cmdlist();

    return TRUE;
}
