/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command
 * FILE:            base/applications/network/net/cmdHelpMsg.c
 * PURPOSE:
 *
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#include "net.h"

#include <stdlib.h>

INT cmdHelpMsg(INT argc, WCHAR **argv)
{
    INT i;
    LONG errNum;
    LPWSTR endptr;
    // DWORD dwLength = 0;
    LPWSTR lpBuffer;

    if (argc < 3)
    {
        ConResPuts(StdOut, IDS_HELPMSG_SYNTAX);
        return 1;
    }

    for (i = 2; i < argc; i++)
    {
        if (_wcsicmp(argv[i], L"/help") == 0)
        {
            ConResPuts(StdOut, IDS_HELPMSG_HELP);
            return 1;
        }
    }

    errNum = wcstol(argv[2], &endptr, 10);
    if (*endptr != 0)
    {
        ConResPuts(StdOut, IDS_HELPMSG_SYNTAX);
        return 1;
    }

    /* Retrieve the message string without appending extra newlines */
    // dwLength =
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                   FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
                   NULL,
                   errNum,
                   LANG_USER_DEFAULT,
                   (LPWSTR)&lpBuffer,
                   0, NULL);
    if (lpBuffer /* && dwLength */)
    {
        ConPrintf(StdOut, L"\n%s\n", lpBuffer);
        LocalFree(lpBuffer);
    }
    else
    {
        ConPrintf(StdOut, L"Unrecognized error code: %ld\n", errNum);
    }

    return 0;
}

/* EOF */

