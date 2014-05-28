/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command
 * FILE:
 * PURPOSE:
 *
 * PROGRAMMERS:     Magnus Olsen (greatlord@reactos.org)
 */

#include "net.h"

typedef struct _COMMAND
{
    WCHAR *name;
    INT (*func)(INT, WCHAR**);
//    VOID (*help)(INT, WCHAR**);
} COMMAND, *PCOMMAND;

COMMAND cmds[] =
{
    {L"accounts",   cmdAccounts},
    {L"computer",   unimplemented},
    {L"config",     unimplemented},
    {L"continue",   cmdContinue},
    {L"file",       unimplemented},
    {L"group",      unimplemented},
    {L"help",       cmdHelp},
    {L"helpmsg",    cmdHelpMsg},
    {L"localgroup", unimplemented},
    {L"name",       unimplemented},
    {L"print",      unimplemented},
    {L"pause",      cmdPause},
    {L"send",       unimplemented},
    {L"session",    unimplemented},
    {L"share",      unimplemented},
    {L"start",      cmdStart},
    {L"statistics", unimplemented},
    {L"stop",       cmdStop},
    {L"time",       unimplemented},
    {L"use",        unimplemented},
    {L"user",       unimplemented},
    {L"view",       unimplemented},
    {NULL,          NULL}
};

int wmain(int argc, WCHAR **argv)
{
    PCOMMAND cmdptr;

    if (argc < 2)
    {
        help();
        return 1;
    }

    /* Scan the command table */
    for (cmdptr = cmds; cmdptr->name; cmdptr++)
    {
        if (_wcsicmp(argv[1], cmdptr->name) == 0)
        {
            return cmdptr->func(argc, argv);
        }
    }

    help();

    return 1;
}

INT unimplemented(INT argc, WCHAR **argv)
{
    puts("This command is not implemented yet");
    return 1;
}
