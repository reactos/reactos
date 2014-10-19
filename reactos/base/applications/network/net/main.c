/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command
 * FILE:
 * PURPOSE:
 *
 * PROGRAMMERS:     Magnus Olsen (greatlord@reactos.org)
 */

#include "net.h"

#define MAX_BUFFER_SIZE 4096

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
    {L"localgroup", cmdLocalGroup},
    {L"name",       unimplemented},
    {L"pause",      cmdPause},
    {L"print",      unimplemented},
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


VOID
PrintResourceString(
    INT resID,
    ...)
{
    WCHAR szMsgBuf[MAX_BUFFER_SIZE];
    va_list arg_ptr;

    va_start(arg_ptr, resID);
    LoadStringW(GetModuleHandle(NULL), resID, szMsgBuf, MAX_BUFFER_SIZE);
    vwprintf(szMsgBuf, arg_ptr);
    va_end(arg_ptr);
}


int wmain(int argc, WCHAR **argv)
{
    PCOMMAND cmdptr;

    if (argc < 2)
    {
        PrintResourceString(IDS_NET_SYNTAX);
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

    PrintResourceString(IDS_NET_SYNTAX);

    return 1;
}

INT unimplemented(INT argc, WCHAR **argv)
{
    puts("This command is not implemented yet");
    return 1;
}
