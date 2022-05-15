/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/help.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way.
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

/*
 * HelpCommandList():
 * shows all the available commands and basic descriptions for diskpart
 */
VOID
HelpCommandList(VOID)
{
    PCOMMAND cmdptr;

    /* Print the header information */
    ConResPuts(StdOut, IDS_APP_HEADER);
    ConPuts(StdOut, L"\n");

    /* List all the commands and the basic descriptions */
    for (cmdptr = cmds; cmdptr->cmd1; cmdptr++)
        if (cmdptr->help_desc != IDS_NONE)
            ConResPuts(StdOut, cmdptr->help_desc);

    ConPuts(StdOut, L"\n");
}


BOOL
HelpCommand(
    PCOMMAND pCommand)
{
    if (pCommand->help != IDS_NONE)
    {
        ConResPuts(StdOut, pCommand->help);
//        ConPuts(StdOut, L"\n");
    }

    return TRUE;
}


/* help_main(char *arg):
 * main entry point for the help command. Gives help to users who needs it.
 */
BOOL help_main(INT argc, LPWSTR *argv)
{
    PCOMMAND cmdptr;
    PCOMMAND cmdptr1 = NULL;
    PCOMMAND cmdptr2 = NULL;
    PCOMMAND cmdptr3 = NULL;

    if (argc == 1)
    {
        HelpCommandList();
        return TRUE;
    }

    /* Scan internal command table */
    for (cmdptr = cmds; cmdptr->cmd1; cmdptr++)
    {
        if ((cmdptr1 == NULL) &&
            (wcsicmp(argv[1], cmdptr->cmd1) == 0))
            cmdptr1 = cmdptr;

        if ((cmdptr2 == NULL) &&
            (argc >= 3) &&
            (wcsicmp(argv[1], cmdptr->cmd1) == 0) &&
            (wcsicmp(argv[2], cmdptr->cmd2) == 0))
            cmdptr2 = cmdptr;

        if ((cmdptr3 == NULL) &&
            (argc >= 4) &&
            (wcsicmp(argv[1], cmdptr->cmd1) == 0) &&
            (wcsicmp(argv[2], cmdptr->cmd2) == 0) &&
            (wcsicmp(argv[3], cmdptr->cmd3) == 0))
            cmdptr3 = cmdptr;
    }

    if (cmdptr3 != NULL)
    {
        return HelpCommand(cmdptr3);
    }
    else if (cmdptr2 != NULL)
    {
        return HelpCommand(cmdptr2);
    }
    else if (cmdptr1 != NULL)
    {
        return HelpCommand(cmdptr1);
    }

    HelpCommandList();

    return TRUE;
}
