/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    tmslot.c

Abstract:

    Test program to Win32 mailslot API calls

Author:

    Manny Weiser (mannyw) 5-Mar-1991

Revision History:

--*/

#include "stdio.h"
#include "windows.h"

#define BUFFER_SIZE   100

char Buffer[BUFFER_SIZE];

DWORD
main(
    int argc,
    char *argv[],
    char *envp[]
    )

{
    BOOL success;
    HANDLE handle;
    LPSTR mailslotName =  "\\\\.\\mailslot\\asdf";

    DWORD maxMessageSize, nextSize, messageCount, readTimeout;
    DWORD bytesRead;

    handle = CreateMailslot( mailslotName,
                             100,
                             MAILSLOT_WAIT_FOREVER,
                             NULL );

    if (handle == (HANDLE)-1) {
        printf ("Failed to open mailslot ""%s""\n", mailslotName);
        return 1;
    }

    printf ("Successfully opened the mailslot.\n");

    success = SetMailslotInfo( handle,
                               atoi( argv[1] ) );
    if (!success) {
        printf ("Failed to set information for mailslot\n");
        return 1;
    }

    printf ("Set mailslot timeout to %d\n", atoi(argv[1]) );

    success = GetMailslotInfo( handle,
                               &maxMessageSize,
                               &nextSize,
                               &messageCount,
                               &readTimeout );

    if (!success) {
        printf ("Failed to get information for mailslot\n");
        return 1;
    }

    printf ("Max message size  = %d\n", maxMessageSize );
    printf ("Next message size = %d\n", nextSize );
    printf ("Message count     = %d\n", messageCount );
    printf ("Read timeout      = %u\n", readTimeout );

    success = ReadFile( handle,
                        Buffer,
                        BUFFER_SIZE,
                        &bytesRead,
                        NULL );

    if (!success) {
        printf ("Failed to read mailslot\n");
        return 1;
    } else {
        printf ("Successfully read %d bytes '%s'\n", bytesRead, Buffer );
    }

}
