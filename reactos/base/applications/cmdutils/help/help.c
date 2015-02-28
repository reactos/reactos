/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS help utility
 * FILE:            base/applications/cmdutils/help/help.c
 * PURPOSE:         Provide help for command-line utilities
 * PROGRAMMERS:     Lee Schroeder (spaceseel at gmail dot com)
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <wincon.h>
#include <strsafe.h>

#include "help.h"
#include "resource.h"

VOID PrintResourceString(INT resID, ...)
{
    WCHAR bufSrc[RC_STRING_MAX_SIZE];
    WCHAR bufFormatted[RC_STRING_MAX_SIZE];
    CHAR bufFormattedOem[RC_STRING_MAX_SIZE];

    va_list args;
    va_start(args, resID);

    LoadStringW(GetModuleHandleW(NULL), resID, bufSrc, RC_STRING_MAX_SIZE);
    vswprintf(bufFormatted, bufSrc, args);
    CharToOemW(bufFormatted, bufFormattedOem);
    fputs(bufFormattedOem, stdout);

    va_end(args);
}

BOOL IsInternalCommand(LPCWSTR Cmd)
{
    size_t i;
    int res;

    /* Invalid command */
    if (!Cmd) return FALSE;

    for (i = 0; i < ARRAYSIZE(InternalCommands); ++i)
    {
        res = _wcsicmp(InternalCommands[i], Cmd);
        if (res == 0)
        {
            /* This is an internal command */
            return TRUE;
        }
        else if (res > 0)
        {
            /*
             * The internal commands list is sorted in alphabetical order.
             * We can quit the loop immediately since the current internal
             * command is lexically greater than the command to be tested.
             */
            break;
        }
    }

    /* Command not found */
    return FALSE;
}

int wmain(int argc, WCHAR* argv[])
{
    WCHAR CmdLine[CMDLINE_LENGTH];

    /*
     * If the user hasn't asked for specific help,
     * then print out the list of available commands.
     */
    if (argc <= 1)
    {
        PrintResourceString(IDS_HELP1);
        PrintResourceString(IDS_HELP2);
        return 0;
    }

    /*
     * Bad usage (too much options) or we use the /? switch.
     * Display help for the help command.
     */
    if ((argc > 2) || (wcscmp(argv[1], L"/?") == 0))
    {
        PrintResourceString(IDS_USAGE);
        return 0;
    }

    /*
     * If the command is not an internal one,
     * display an information message and exit.
     */
    if (!IsInternalCommand(argv[1]))
    {
        PrintResourceString(IDS_NO_ENTRY, argv[1]);
        return 0;
    }

    /*
     * Run "<command> /?" in the current command processor.
     */
    StringCbPrintfW(CmdLine, sizeof(CmdLine), L"%ls /?", argv[1]);
    
    _flushall();
    return _wsystem(CmdLine);
}

/* EOF */
