/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    stream.h

Abstract:

        This file implements the NT console direct I/O API

Author:

    KazuM Apr.20.1996

Revision History:

--*/


typedef struct _RAW_READ_DATA {
    PINPUT_INFORMATION InputInfo;
    PCONSOLE_INFORMATION Console;
    ULONG BufferSize;
    PWCHAR BufPtr;
    PCONSOLE_PER_PROCESS_DATA ProcessData;
    HANDLE HandleIndex;
} RAW_READ_DATA, *PRAW_READ_DATA;
