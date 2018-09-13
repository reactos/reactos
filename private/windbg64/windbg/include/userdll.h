/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    UserDll.h

Abstract:

    Header file for User Dll management

Author:

    Ramon J. San Andres (ramonsa)  26-October-1992

Environment:

    Win32, User Mode

--*/


#define MAX_SYM_SEARCH_PATHS (MAX_PATH * 20)


//
// Helper functions
//
BOOL
TruncatePathsToMaxPath(
    PCSTR pszOriginal,
    PSTR & ppszTruncated
    );

BOOL
IsLenOfIndivPathValid(
    PCSTR pszOriginal,
    BOOL bWarn
    );

//
// User DLL functions
//
VOID
ModListSetDefaultShe (
    SHE     SheDefault
    );

BOOL
ModListGetDefaultShe (
    CHAR    *Name,
    SHE     *She
    );

VOID
ModListSetSearchPath(
    CHAR    *Path
    );

VOID
ModListAddSearchPath(
    CHAR    *Path
    );

DWORD
ModListGetSearchPath(
    CHAR    *Buffer,
    DWORD   Size
    );

VOID
ModListInit(
    VOID
    );

VOID
ModListAdd(
    CHAR    *Name,
    SHE      SheDefault
    );

VOID
ModListModLoad(
    CHAR    *Name,
    SHE     SheCurrent
    );

VOID
ModListModUnload(
    CHAR    *Name
    );

PVOID
ModListGetNext(
    PVOID   Previous,
    CHAR   *Name,
    SHE    *SheDefault
    );
