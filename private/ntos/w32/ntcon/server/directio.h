/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    directio.h

Abstract:

        This file implements the NT console direct I/O API

Author:

    KazuM Apr.19.1996

Revision History:

--*/

typedef struct _DIRECT_READ_DATA {
    PINPUT_INFORMATION InputInfo;
    PCONSOLE_INFORMATION Console;
    PCONSOLE_PER_PROCESS_DATA ProcessData;
    HANDLE HandleIndex;
} DIRECT_READ_DATA, *PDIRECT_READ_DATA;

