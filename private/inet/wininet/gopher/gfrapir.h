/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    gfrapir.c

Abstract:

    Manifests, macros, types, prototypes for gfrapir.c

Author:

    Richard L Firth (rfirth) 14-Oct-1994

Environment:

    Win32 DLL

Revision History:

    14-Oct-1994 rfirth
        Created

--*/

#if defined(__cplusplus)
extern "C" {
#endif

#define IS_GOPHER_SEARCH_SERVER(type) \
    (BOOL)(((type) & (GOPHER_TYPE_CSO | GOPHER_TYPE_INDEX_SERVER)))

//
// prototypes
//

DWORD
wGopherFindFirst(
    IN LPCSTR lpszLocator,
    IN LPCSTR lpszSearchString OPTIONAL,
    OUT LPGOPHER_FIND_DATA lpBuffer OPTIONAL,
    OUT LPHINTERNET lpHandle
    );

DWORD
wGopherFindNext(
    IN HINTERNET hFind,
    OUT LPGOPHER_FIND_DATA lpszBuffer
    );

DWORD
wGopherFindClose(
    IN HINTERNET hFind
    );

DWORD
wGopherOpenFile(
    IN LPCSTR lpszLocator,
    IN LPCSTR lpszView OPTIONAL,
    OUT LPHINTERNET lpHandle
    );

DWORD
wGopherReadFile(
    IN HINTERNET hFile,
    OUT LPBYTE lpBuffer,
    IN DWORD dwBufferLength,
    OUT LPDWORD lpdwBytesReturned
    );

DWORD
wGopherCloseHandle(
    IN HINTERNET hFile
    );

DWORD
wGopherGetAttribute(
    IN LPCSTR lpszLocator,
    IN LPCSTR lpszAttribute,
    OUT LPBYTE lpBuffer,
    IN OUT LPDWORD lpdwBufferLength
    );

#if defined(__cplusplus)
}
#endif
