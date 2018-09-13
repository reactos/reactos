/*++ BUILD Version: 0004    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Windbg.h

Abstract:

    Main header file for the Windbg debugger.

Author:

    Kent Forschmiedt (kentf) 01-Sep-1997

Environment:

    Win32, User Mode

--*/

#if ! defined( _KDOPT_H_ )
#define _KDOPT_H_



LPCSTR
GetPlatformNameFromId(
    MPT mpt
    );

BOOL
GetPlatformIdFromName(
    LPSTR lpszPlatformName,
    MPT * pmpt
    );

MPT
GetPlatformIdFromArrayPos(
    int nArrayPos
    );


#endif // _KDOPT_H_
