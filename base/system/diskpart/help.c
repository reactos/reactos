/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/help.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"


/*
 * help_cmdlist():
 * shows all the available commands and basic descriptions for diskpart
 */
VOID help_cmdlist(VOID)
{
    /* Print the header information */
    PrintResourceString(IDS_APP_HEADER, DISKPART_VERSION);

    /* lists all the commands and the basic descriptions */
    PrintResourceString(IDS_HELP_CMD_DESC_ACTIVE);
    PrintResourceString(IDS_HELP_CMD_DESC_ADD);
    PrintResourceString(IDS_HELP_CMD_DESC_ASSIGN);
    PrintResourceString(IDS_HELP_CMD_DESC_ATTACH);
    PrintResourceString(IDS_HELP_CMD_DESC_ATTRIBUTES);
    PrintResourceString(IDS_HELP_CMD_DESC_AUTOMOUNT);
    PrintResourceString(IDS_HELP_CMD_DESC_BREAK);
    PrintResourceString(IDS_HELP_CMD_DESC_CLEAN);
    PrintResourceString(IDS_HELP_CMD_DESC_COMPACT);
    PrintResourceString(IDS_HELP_CMD_DESC_CONVERT);
    PrintResourceString(IDS_HELP_CMD_DESC_CREATE);
    PrintResourceString(IDS_HELP_CMD_DESC_DELETE);
    PrintResourceString(IDS_HELP_CMD_DESC_DETACH);
    PrintResourceString(IDS_HELP_CMD_DESC_DETAIL);
    PrintResourceString(IDS_HELP_CMD_DESC_EXIT);
    PrintResourceString(IDS_HELP_CMD_DESC_EXPAND);
    PrintResourceString(IDS_HELP_CMD_DESC_EXTEND);
    PrintResourceString(IDS_HELP_CMD_DESC_FS);
    PrintResourceString(IDS_HELP_CMD_DESC_FORMAT);
    PrintResourceString(IDS_HELP_CMD_DESC_GPT);
    PrintResourceString(IDS_HELP_CMD_DESC_HELP);
    PrintResourceString(IDS_HELP_CMD_DESC_IMPORT);
    PrintResourceString(IDS_HELP_CMD_DESC_INACTIVE);
    PrintResourceString(IDS_HELP_CMD_DESC_LIST);
    PrintResourceString(IDS_HELP_CMD_DESC_MERGE);
    PrintResourceString(IDS_HELP_CMD_DESC_OFFLINE);
    PrintResourceString(IDS_HELP_CMD_DESC_ONLINE);
    PrintResourceString(IDS_HELP_CMD_DESC_RECOVER);
    PrintResourceString(IDS_HELP_CMD_DESC_REM);
    PrintResourceString(IDS_HELP_CMD_DESC_REMOVE);
    PrintResourceString(IDS_HELP_CMD_DESC_REPAIR);
    PrintResourceString(IDS_HELP_CMD_DESC_RESCAN);
    PrintResourceString(IDS_HELP_CMD_DESC_RETAIN);
    PrintResourceString(IDS_HELP_CMD_DESC_SAN);
    PrintResourceString(IDS_HELP_CMD_DESC_SELECT);
    PrintResourceString(IDS_HELP_CMD_DESC_SETID);
    PrintResourceString(IDS_HELP_CMD_DESC_SHRINK);
    PrintResourceString(IDS_HELP_CMD_DESC_UNIQUEID);
    printf("\n");
}


VOID help_help(INT argc, WCHAR **argv)
{
    PrintResourceString(IDS_HELP_CMD_HELP);
}



/* help_main(char *arg):
 * main entry point for the help command. Gives help to users who needs it.
 */
BOOL help_main(INT argc, WCHAR **argv)
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
        if (_wcsicmp(argv[0], cmdptr->name) == 0 && cmdptr->help != NULL)
        {
            cmdptr->help(argc, argv);
            return TRUE;
        }
    }

    help_cmdlist();

    return TRUE;
}
