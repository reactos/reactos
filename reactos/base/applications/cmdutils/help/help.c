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

#include "help.h"
#include "resource.h"

BOOL IsConsoleHandle(HANDLE hHandle)
{
    DWORD dwMode;

    /* Check whether the handle may be that of a console... */
    if ((GetFileType(hHandle) & ~FILE_TYPE_REMOTE) != FILE_TYPE_CHAR)
        return FALSE;

    /*
     * It may be. Perform another test... The idea comes from the
     * MSDN description of the WriteConsole API:
     *
     * "WriteConsole fails if it is used with a standard handle
     *  that is redirected to a file. If an application processes
     *  multilingual output that can be redirected, determine whether
     *  the output handle is a console handle (one method is to call
     *  the GetConsoleMode function and check whether it succeeds).
     *  If the handle is a console handle, call WriteConsole. If the
     *  handle is not a console handle, the output is redirected and
     *  you should call WriteFile to perform the I/O."
     */
    return GetConsoleMode(hHandle, &dwMode);
}

VOID PrintResourceString(INT resID, ...)
{
    HANDLE OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    WCHAR tmpBuffer[RC_STRING_MAX_SIZE];
    va_list arg_ptr;

    va_start(arg_ptr, resID);
    LoadStringW(GetModuleHandleW(NULL), resID, tmpBuffer, RC_STRING_MAX_SIZE);

    // FIXME: Optimize by using Win32 console functions.
    if (IsConsoleHandle(OutputHandle))
    {
        _vcwprintf(tmpBuffer, arg_ptr);
    }
    else
    {
        vwprintf(tmpBuffer, arg_ptr);
    }

    va_end(arg_ptr);
}

BOOL IsInternalCommand(LPCWSTR Cmd)
{
    size_t i;
    int res;

    /* Invalid command */
    if (!Cmd) return FALSE;

    for (i = 0; i < sizeof(InternalCommands)/sizeof(InternalCommands[0]); ++i)
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
    wcsncpy(CmdLine, argv[1], CMDLINE_LENGTH - wcslen(CmdLine));
    wcsncat(CmdLine, L" /?" , CMDLINE_LENGTH - wcslen(CmdLine));

    _flushall();
    return _wsystem(CmdLine);
}

/* EOF */
