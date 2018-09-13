/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    dirlist.h

Abstract:

    Prototypes, etc. for dirlist.c

Author:

    Richard L Firth (rfirth) 31-Jul-1995

Revision History:

    31-Jul-1995 rfirth
        Created

--*/

#if defined(__cplusplus)
extern "C" {
#endif

DWORD
ParseDirList(
    IN LPSTR lpBuffer,
    IN DWORD lpBufferLength,
    IN LPSTR lpszFilespec OPTIONAL,
    IN OUT PLIST_ENTRY lpList
    );

BOOL
IsFilespecWild(
    IN LPCSTR lpszFilespec
    );

PRIVATE
VOID
ClearFindList(
    IN PLIST_ENTRY lpList
    );

#if defined(__cplusplus)
}
#endif
