/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Change CodePage Command
 * FILE:            base/applications/cmdutils/chcp/chcp.c
 * PURPOSE:         Displays or changes the active console input and output codepages.
 * PROGRAMMERS:     Eric Kohl
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */
/*
 * CHCP.C - chcp internal command.
 *
 * 23-Dec-1998 (Eric Kohl)
 *     Started.
 *
 * 02-Apr-2005 (Magnus Olsen <magnus@greatlord.com>)
 *     Remove all hardcoded strings in En.rc
 */

#include <stdio.h>
#include <stdlib.h>

#include <windef.h>
#include <winbase.h>
#include <wincon.h>

#include <conutils.h>

#include "resource.h"

// INT CommandChcp(LPTSTR cmd, LPTSTR param)
int wmain(int argc, WCHAR* argv[])
{
    UINT uOldCodePage, uNewCodePage;

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    /* Print help */
    if (argc > 1 && wcscmp(argv[1], L"/?") == 0)
    {
        ConResPuts(StdOut, STRING_CHCP_HELP);
        return 0;
    }

    if (argc == 1)
    {
        /* Display the active code page number */
        ConResPrintf(StdOut, STRING_CHCP_ERROR1, GetConsoleOutputCP());
        return 0;
    }

    if (argc > 2)
    {
        /* Too many parameters */
        ConResPrintf(StdErr, STRING_ERROR_INVALID_PARAM_FORMAT, argv[2]);
        return 1;
    }

    uNewCodePage = (UINT)_wtoi(argv[1]);

    if (uNewCodePage == 0)
    {
        ConResPrintf(StdErr, STRING_ERROR_INVALID_PARAM_FORMAT, argv[1]);
        return 1;
    }

    /*
     * Save the original console codepage to be restored in case
     * SetConsoleCP() or SetConsoleOutputCP() fails.
     */
    uOldCodePage = GetConsoleCP();

    /*
     * Try changing the console input codepage. If it works then also change
     * the console output codepage, and refresh our local codepage cache.
     */
    if (SetConsoleCP(uNewCodePage))
    {
        if (SetConsoleOutputCP(uNewCodePage))
        {
            /* Display the active code page number */
            ConResPrintf(StdOut, STRING_CHCP_ERROR1, GetConsoleOutputCP());
            return 0;
        }
        else
        {
            /* Failure, restore the original console codepage */
            SetConsoleCP(uOldCodePage);
        }
    }

    /* An error happened, display an error and bail out */
    ConResPuts(StdErr, STRING_CHCP_ERROR4);
    return 1;
}
