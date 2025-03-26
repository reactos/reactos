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
    WCHAR szFormat[64];
    WCHAR szOutput[256];

    K32LoadStringW(GetModuleHandle(NULL), IDS_HELP_FORMAT_STRING, szFormat, ARRAYSIZE(szFormat));

    /* Print the header information */
    ConResPuts(StdOut, IDS_APP_HEADER);
    ConPuts(StdOut, L"\n");

    /* List all the commands and the basic descriptions */
    for (cmdptr = cmds; cmdptr->cmd1; cmdptr++)
    {
        if ((cmdptr->cmd1 != NULL) &&
            (cmdptr->cmd2 == NULL) &&
            (cmdptr->cmd3 == NULL) &&
            (cmdptr->help != IDS_NONE))
        {
            K32LoadStringW(GetModuleHandle(NULL), cmdptr->help, szOutput, ARRAYSIZE(szOutput));
            ConPrintf(StdOut, szFormat, cmdptr->cmd1, szOutput);
        }
    }

    ConPuts(StdOut, L"\n");
}


BOOL
HelpCommand(
    PCOMMAND pCommand)
{
    PCOMMAND cmdptr;
    BOOL bSubCommands = FALSE;
    WCHAR szFormat[64];
    WCHAR szOutput[256];

    K32LoadStringW(GetModuleHandle(NULL), IDS_HELP_FORMAT_STRING, szFormat, ARRAYSIZE(szFormat));

    ConPuts(StdOut, L"\n");

    /* List all the commands and the basic descriptions */
    for (cmdptr = cmds; cmdptr->cmd1; cmdptr++)
    {
        if (pCommand->cmd1 != NULL && pCommand->cmd2 == NULL && pCommand->cmd3 == NULL)
        {
            if ((cmdptr->cmd1 != NULL && _wcsicmp(pCommand->cmd1, cmdptr->cmd1) == 0) &&
                (cmdptr->cmd2 != NULL) &&
                (cmdptr->cmd3 == NULL) &&
                (cmdptr->help != IDS_NONE))
            {
                K32LoadStringW(GetModuleHandle(NULL), cmdptr->help, szOutput, ARRAYSIZE(szOutput));
                ConPrintf(StdOut, szFormat, cmdptr->cmd2, szOutput);
                bSubCommands = TRUE;
            }
        }
        else if (pCommand->cmd1 != NULL && pCommand->cmd2 != NULL && pCommand->cmd3 == NULL)
        {
            if ((cmdptr->cmd1 != NULL && _wcsicmp(pCommand->cmd1, cmdptr->cmd1) == 0) &&
                (cmdptr->cmd2 != NULL && _wcsicmp(pCommand->cmd2, cmdptr->cmd2) == 0) &&
                (cmdptr->cmd3 != NULL) &&
                (cmdptr->help != IDS_NONE))
            {
                K32LoadStringW(GetModuleHandle(NULL), cmdptr->help, szOutput, ARRAYSIZE(szOutput));
                ConPrintf(StdOut, szFormat, cmdptr->cmd3, szOutput);
                bSubCommands = TRUE;
            }
        }
        else if (pCommand->cmd1 != NULL && pCommand->cmd2 != NULL && pCommand->cmd3 != NULL)
        {
            if ((cmdptr->cmd1 != NULL && _wcsicmp(pCommand->cmd1, cmdptr->cmd1) == 0) &&
                (cmdptr->cmd2 != NULL && _wcsicmp(pCommand->cmd2, cmdptr->cmd2) == 0) &&
                (cmdptr->cmd3 != NULL && _wcsicmp(pCommand->cmd3, cmdptr->cmd3) == 0) &&
                (cmdptr->help_detail != MSG_NONE))
            {
                ConMsgPuts(StdOut,
                           FORMAT_MESSAGE_FROM_HMODULE,
                           NULL,
                           cmdptr->help_detail,
                           LANG_USER_DEFAULT);
                bSubCommands = TRUE;
            }
        }
    }

    if ((bSubCommands == FALSE) && (pCommand->help_detail != MSG_NONE))
    {
        ConMsgPuts(StdOut,
                   FORMAT_MESSAGE_FROM_HMODULE,
                   NULL,
                   pCommand->help_detail,
                   LANG_USER_DEFAULT);
    }

    ConPuts(StdOut, L"\n");

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
            (cmdptr->cmd1 != NULL && _wcsicmp(argv[1], cmdptr->cmd1) == 0))
        {
            cmdptr1 = cmdptr;
        }

        if ((cmdptr2 == NULL) &&
            (argc >= 3) &&
            (cmdptr->cmd1 != NULL && _wcsicmp(argv[1], cmdptr->cmd1) == 0) &&
            (cmdptr->cmd2 != NULL && _wcsicmp(argv[2], cmdptr->cmd2) == 0))
        {
            cmdptr2 = cmdptr;
        }

        if ((cmdptr3 == NULL) &&
            (argc >= 4) &&
            (cmdptr->cmd1 != NULL && _wcsicmp(argv[1], cmdptr->cmd1) == 0) &&
            (cmdptr->cmd2 != NULL && _wcsicmp(argv[2], cmdptr->cmd2) == 0) &&
            (cmdptr->cmd3 != NULL && _wcsicmp(argv[3], cmdptr->cmd3) == 0))
        {
            cmdptr3 = cmdptr;
        }
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
