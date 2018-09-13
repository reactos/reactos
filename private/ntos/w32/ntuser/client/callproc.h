/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    callproc.h

Abstract:

    Private DialogProc call routines
    Copied from ldrthunk.asm

Author:

    Joe Jones (joejo) 11-30-98

Revision History:

--*/

#ifndef _CALLPROC_
#define _CALLPROC_


#if defined(_X86_)
LRESULT
UserCallWinProc(
    WNDPROC proc,
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

/*
 * Bug 246472 - joejo
 * fixup all DDE Callbacks since some apps make their callbacks
 * C-Style instead of PASCAL.
 */
HDDEDATA 
UserCallDDECallback(
    PFNCALLBACK pfnDDECallback,
    UINT wType, 
    UINT wFmt, 
    HCONV hConv,
    HSZ hsz1, 
    HSZ hsz2, 
    HDDEDATA hData, 
    ULONG_PTR dwData1, 
    ULONG_PTR dwData2
    );


#else

#define UserCallWinProc(winproc, hwnd, message, wParam, lParam)    \
    (winproc)(hwnd, message, wParam, lParam)

#define UserCallDDECallback(pfnDDECallback, wType, wFmt, hConv, hsz1, hsz2, hData, dwData1, dwData2) \
    (pfnDDECallback)(wType, wFmt, hConv, hsz1, hsz2, hData, dwData1, dwData2)
    

#endif

#endif /* _CALLPROC_ */
