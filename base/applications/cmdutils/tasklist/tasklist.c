/*
 * PROJECT:     ReactOS Tasklist Command
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Displays a list of currently running processes on the computer.
 * COPYRIGHT:   Copyright 2020 He Yang (1160386205@qq.com)
 */

#include <stdio.h>
#include <stdlib.h>

#include <windows.h>

#include <conutils.h>

#include "resource.h"

static WCHAR opHelp[] = L"?";
static WCHAR opVerbose[] = L"v";
static PWCHAR opList[] = {opHelp, opVerbose};


#define OP_PARAM_INVALID -1

#define OP_PARAM_HELP 0
#define OP_PARAM_VERBOSE 1

int GetArgumentType(WCHAR* argument)
{
    int i;

    if (argument[0] != L'/' && argument[0] != L'-')
    {
        return OP_PARAM_INVALID;
    }
    argument++;

    for (i = 0; i < _countof(opList); i++)
    {
        if (!wcsicmp(opList[i], argument))
        {
            return i;
        }
    }
    return OP_PARAM_INVALID;
}

BOOL ProcessArguments(int argc, WCHAR *argv[])
{
    int i;
    BOOL bHasHelp = FALSE, bHasVerbose = FALSE;
    for (i = 1; i < argc; i++)
    {
        int Argument = GetArgumentType(argv[i]);

        switch (Argument)
        {
        case OP_PARAM_HELP:
        {
            if (bHasHelp)
            {
                // -? already specified
                ConResMsgPrintf(StdOut, 0, IDS_PARAM_TOO_MUCH, argv[i], 1);
                ConResMsgPrintf(StdOut, 0, IDS_USAGE);
                return FALSE;
            }
            bHasHelp = TRUE;
            break;
        }
        case OP_PARAM_VERBOSE:
        {
            if (bHasVerbose)
            {
                // -V already specified
                ConResMsgPrintf(StdOut, 0, IDS_PARAM_TOO_MUCH, argv[i], 1);
                ConResMsgPrintf(StdOut, 0, IDS_USAGE);
                return FALSE;
            }
            bHasVerbose = TRUE;
            break;
        }
        case OP_PARAM_INVALID:
        default:
        {
            ConResMsgPrintf(StdOut, 0, IDS_INVALID_OPTION);
            ConResMsgPrintf(StdOut, 0, IDS_USAGE);
            return FALSE;
        }
        }
    }


    return TRUE;
}

int wmain(int argc, WCHAR *argv[])
{
    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    if(!ProcessArguments(argc, argv))
    {
        return 1;
    }
    return 0;
}
