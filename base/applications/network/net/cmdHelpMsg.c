/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command
 * FILE:            cmdHelpmsg.c
 * PURPOSE:
 *
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#include "net.h"
#include "stdlib.h"

INT cmdHelpMsg(INT argc, WCHAR **argv)
{
    LPWSTR endptr;
    LPWSTR lpBuffer;
    LONG errNum;

    if (argc < 3)
    {
        puts("Usage: NET HELPMSG <Error Code>");
        return 1;
    }

    errNum = wcstol(argv[2], &endptr, 10);
    if (*endptr != 0)
    {
        puts("Usage: NET HELPMSG <Error Code>");
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
        printf("\n%S\n", lpBuffer);
        LocalFree(lpBuffer);
    }
    else
    {
        printf("Unrecognized error code: %ld\n", errNum);
    }

    return 0;
}

/* EOF */

