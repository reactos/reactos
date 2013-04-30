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

int cmdHelpMsg(int argc, wchar_t *argv[])
{
    wchar_t *endptr;
    LPSTR lpBuffer;
    if(argc<3)
    {
        puts("Usage: NET HELPMSG <Error Code>");
        return 1;
    }
    long errNum=wcstol(argv[2], &endptr, 10);
    if(*endptr != 0)
    {
        puts("Usage: NET HELPMSG <Error Code>");
        return 1;
    }

    /* Unicode printing is not supported in ReactOS yet */
    if(FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                      NULL,
                      errNum,
                      LANG_USER_DEFAULT,
                      (LPSTR)&lpBuffer,
                      0,
                      NULL))
    {
        printf("\n%s\n", lpBuffer);
        LocalFree(lpBuffer);
    }
    else printf("Unrecognized error code: %ld\n", errNum);
    
    return 0;
}

/* EOF */

