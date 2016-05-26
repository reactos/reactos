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
    LPWSTR endptr;
    LPWSTR lpBuffer;
    LONG errNum;
    INT i;

    if (argc < 3)
    {
        PrintResourceString(IDS_HELPMSG_SYNTAX);
        return 1;
    }

    for (i = 2; i < argc; i++)
    {
        if (_wcsicmp(argv[i], L"/help") == 0)
        {
            PrintResourceString(IDS_HELPMSG_HELP);
            return 1;
        }
    }

    errNum = wcstol(argv[2], &endptr, 10);
    if (*endptr != 0)
    {
        PrintResourceString(IDS_HELPMSG_SYNTAX);
        return 1;
    }

    /* Unicode printing is not supported in ReactOS yet */
    if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                       NULL,
                       errNum,
                       LANG_USER_DEFAULT,
                       (LPWSTR)&lpBuffer,
                       0,
                       NULL))
    {
        PrintToConsole(L"\n%s\n", lpBuffer);
        LocalFree(lpBuffer);
    }
    else
    {
        PrintToConsole(L"Unrecognized error code: %ld\n", errNum);
    }

    return 0;
}

/* EOF */

