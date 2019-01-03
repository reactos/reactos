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
    PWSTR endptr;
    PWSTR pBuffer;
    PWSTR pInserts[10] = {L"***", L"***", L"***", L"***",
                          L"***", L"***", L"***", L"***",
                          L"***", NULL};

    if (argc < 3)
    {
        ConResPuts(StdOut, IDS_GENERIC_SYNTAX);
        PrintNetMessage(MSG_HELPMSG_SYNTAX);
        return 1;
    }

    for (i = 2; i < argc; i++)
    {
        if (_wcsicmp(argv[i], L"/help") == 0)
        {
            ConResPuts(StdOut, IDS_GENERIC_SYNTAX);
            PrintNetMessage(MSG_HELPMSG_SYNTAX);
            PrintNetMessage(MSG_HELPMSG_HELP);
            return 1;
        }
    }

    errNum = wcstol(argv[2], &endptr, 10);
    if (*endptr != 0)
    {
        ConResPuts(StdOut, IDS_GENERIC_SYNTAX);
        PrintNetMessage(MSG_HELPMSG_SYNTAX);
        return 1;
    }

    if (errNum >= MIN_LANMAN_MESSAGE_ID && errNum <= MAX_LANMAN_MESSAGE_ID)
    {
        FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE |
                       FORMAT_MESSAGE_ARGUMENT_ARRAY,
                       hModuleNetMsg,
                       errNum,
                       LANG_USER_DEFAULT,
                       (LPWSTR)&pBuffer,
                       0,
                       (va_list *)pInserts);
        if (pBuffer)
        {
            ConPrintf(StdOut, L"\n%s\n", pBuffer);
            LocalFree(pBuffer);
        }
        else
        {
            PrintErrorMessage(3871);
        }
    }
    else
    {
        /* Retrieve the message string without appending extra newlines */
        FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_ARGUMENT_ARRAY,
                       NULL,
                       errNum,
                       LANG_USER_DEFAULT,
                       (LPWSTR)&pBuffer,
                       0,
                       (va_list *)pInserts);
        if (pBuffer)
        {
            ConPrintf(StdOut, L"\n%s\n", pBuffer);
            LocalFree(pBuffer);
        }
        else
        {
            PrintErrorMessage(3871);
        }
    }

    return 0;
}

/* EOF */

