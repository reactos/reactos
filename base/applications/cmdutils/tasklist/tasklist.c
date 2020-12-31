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
static WCHAR opModule[] = L"m";
static WCHAR opFormat[] = L"fo";

static PWCHAR opList[] = {opHelp, opModule, opFormat};


#define OP_PARAM_INVALID -1

#define OP_PARAM_HELP 0
#define OP_PARAM_MODULE 1
#define OP_PARAM_FORMAT 2


static int get_argument_type(WCHAR* argument)
{
    int i;

    if (argument[0] != L'/' && argument[0] != L'-')
    {
        return OP_PARAM_INVALID;
    }
    argument++;

    for (i = 0; i < _countof(opList); i++)
    {
        if (!wcscmp(opList[i], argument))
        {
            return i;
        }
    }
    return OP_PARAM_INVALID;
}

static BOOL process_arguments(int argc, WCHAR *argv[])
{
    int i;
    for (i = 1; i < argc; i++)
    {
        int argument = get_argument_type(argv[i]);

        switch (argument)
        {
        case OP_PARAM_HELP:
        {
            break;
        }
        case OP_PARAM_MODULE:
        {
            break;
        }
        case OP_PARAM_FORMAT:
        {
            break;
        }
        case OP_PARAM_INVALID:
        default:
        {
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

    if(!process_arguments(argc, argv))
    {
        return 1;
    }
    return 0;
}
