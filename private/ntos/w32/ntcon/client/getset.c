/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:
                  
    getset.c      
                  
Abstract:         

    This module contains the stubs for the console get/set API.

Author:

    Therese Stowell (thereses) 14-Nov-1990

Revision History:
    
--*/

#include "precomp.h"
#pragma hdrstop
#pragma hdrstop

#if !defined(BUILD_WOW6432)

BOOL
WINAPI
GetConsoleMode(
    IN HANDLE hConsoleHandle,
    OUT LPDWORD lpMode
    )

/*++

Parameters:

    hConsoleHandle - Supplies a console input or output handle.

    lpMode - Supplies a pointer to a dword in which to store the mode.

        Input Mode Flags:

            ENABLE_LINE_INPUT - line oriented input is on.

            ENABLE_ECHO_INPUT - characters will be written to the screen as they are
                read.

            ENABLE_WINDOW_INPUT - the caller is windows-aware

        Output Mode Flags:

            ENABLE_LINE_OUTPUT - line oriented output is on.

            ENABLE_WRAP_AT_EOL_OUTPUT - the cursor will move to the
                beginning of the next line when the end of the row
                is reached.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.


--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_MODE_MSG a = &m.u.GetConsoleMode;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->Handle = hConsoleHandle;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepGetMode
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        try {
            *lpMode = a->Mode;
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            SET_LAST_ERROR (ERROR_INVALID_ACCESS);
            return FALSE;
        }
        return TRUE;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }

}

DWORD
WINAPI
GetNumberOfConsoleFonts(
    VOID
    )

/*++

Parameters:

    none.

Return Value:

    NON-NULL - Returns the number of fonts available.

    FALSE/NULL - The operation failed.
        Extended error status is available using GetLastError.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_GETNUMBEROFFONTS_MSG a = &m.u.GetNumberOfConsoleFonts;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepGetNumberOfFonts
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        return a->NumberOfFonts;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }

}


BOOL
WINAPI
GetNumberOfConsoleInputEvents(
    IN HANDLE hConsoleInput,
    OUT LPDWORD lpNumberOfEvents
    )

/*++

Parameters:

    hConsoleInput - Supplies an open handle to console input.

    lpNumberOfEvents - Pointer to number of events in input buffer.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed.
        Extended error status is available using GetLastError.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_GETNUMBEROFINPUTEVENTS_MSG a = &m.u.GetNumberOfConsoleInputEvents;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->InputHandle = hConsoleInput;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepGetNumberOfInputEvents
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        try {
            *lpNumberOfEvents = a->ReadyEvents;
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            SET_LAST_ERROR (ERROR_INVALID_ACCESS);
            return FALSE;
        }
        return TRUE;
    }
    else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }

}

COORD
WINAPI
GetLargestConsoleWindowSize(
    IN HANDLE hConsoleOutput
    )

/*++

    Returns largest window possible, given the current font.  The return value
    does not take the screen buffer size into account.

Parameters:

    hConsoleOutput - Supplies an open handle to console output.

Return Value:

    The return value is the maximum window size in rows and columns.  A size
    of zero will be returned if an error occurs.  Extended error information
    can be retrieved by calling the GetLastError function.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_GETLARGESTWINDOWSIZE_MSG a = &m.u.GetLargestConsoleWindowSize;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->OutputHandle = hConsoleOutput;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepGetLargestWindowSize
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        return a->Size;
    } else {
        COORD Dummy;
        Dummy.X = Dummy.Y = 0;
        SET_LAST_NT_ERROR (m.ReturnValue);
        return Dummy;
    }

}


BOOL
WINAPI
GetConsoleScreenBufferInfo(
    IN HANDLE hConsoleOutput,
    OUT PCONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo
    )

/*++

Parameters:

    hConsoleOutput - Supplies an open handle to console output.

    lpConsoleScreenBufferInfo - A pointer to a buffer to receive the
        requested information.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_GETSCREENBUFFERINFO_MSG a = &m.u.GetConsoleScreenBufferInfo;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->OutputHandle = hConsoleOutput;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepGetScreenBufferInfo
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        try {
            lpConsoleScreenBufferInfo->dwSize =              a->Size;
            lpConsoleScreenBufferInfo->dwCursorPosition =    a->CursorPosition;
            lpConsoleScreenBufferInfo->wAttributes =         a->Attributes;
            lpConsoleScreenBufferInfo->srWindow.Left = a->ScrollPosition.X;
            lpConsoleScreenBufferInfo->srWindow.Top = a->ScrollPosition.Y;
            lpConsoleScreenBufferInfo->srWindow.Right = lpConsoleScreenBufferInfo->srWindow.Left + a->CurrentWindowSize.X-1;
            lpConsoleScreenBufferInfo->srWindow.Bottom = lpConsoleScreenBufferInfo->srWindow.Top + a->CurrentWindowSize.Y-1;
            lpConsoleScreenBufferInfo->dwMaximumWindowSize = a->MaximumWindowSize;
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            SET_LAST_ERROR (ERROR_INVALID_ACCESS);
            return FALSE;
        }
        return TRUE;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }

}

BOOL
WINAPI
GetConsoleCursorInfo(
    IN HANDLE hConsoleOutput,
    OUT PCONSOLE_CURSOR_INFO lpConsoleCursorInfo
    )

/*++

Parameters:

    hConsoleOutput - Supplies an open handle to console output.

    lpConsoleCursorInfo - A pointer to a buffer to receive the
        requested information.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_GETCURSORINFO_MSG a = &m.u.GetConsoleCursorInfo;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->OutputHandle = hConsoleOutput;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepGetCursorInfo
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        try {
            lpConsoleCursorInfo->dwSize = a->CursorSize;
            lpConsoleCursorInfo->bVisible = a->Visible;
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            SET_LAST_ERROR (ERROR_INVALID_ACCESS);
            return FALSE;
        }
        return TRUE;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }

}

BOOL
WINAPI
GetNumberOfConsoleMouseButtons(
    OUT LPDWORD lpNumberOfMouseButtons
    )

/*++

Parameters:

    hConsoleInput - Supplies an open handle to console input.

    lpNumberOfMouseButtons - pointer to the number of mouse buttons

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_GETMOUSEINFO_MSG a = &m.u.GetConsoleMouseInfo;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepGetMouseInfo
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        try {
            *lpNumberOfMouseButtons = a->NumButtons;
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            SET_LAST_ERROR (ERROR_INVALID_ACCESS);
            return FALSE;
        }
        return TRUE;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }

}

DWORD
WINAPI
GetConsoleFontInfo(
    IN HANDLE hConsoleOutput,
    IN BOOL bMaximumWindow,
    IN DWORD nLength,
    OUT PCONSOLE_FONT_INFO lpConsoleFontInfo
    )

/*++

Parameters:

    hConsoleOutput - Supplies an open handle to console output.

    bMaximumWindow - TRUE if caller wants available fonts for the maximum
        window size.  FALSE if caller wants available fonts for the current
        window size.

    nLength - Length of buffer in CONSOLE_FONT_INFOs.

    lpConsoleFontInfo - A pointer to a buffer to receive the
        requested information.

Return Value:

    NON-NULL - Returns the number of fonts returned in lpConsoleFontInfo.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_GETFONTINFO_MSG a = &m.u.GetConsoleFontInfo;
    PCSR_CAPTURE_HEADER CaptureBuffer;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->OutputHandle = hConsoleOutput;
    a->MaximumWindow = (BOOLEAN) bMaximumWindow;
    a->NumFonts = nLength;

    CaptureBuffer = CsrAllocateCaptureBuffer( 1,
                                              nLength * sizeof(CONSOLE_FONT_INFO)
                                            );
    if (CaptureBuffer == NULL) {
        SET_LAST_ERROR(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    CsrCaptureMessageBuffer( CaptureBuffer,
                             NULL,
                             nLength * sizeof(CONSOLE_FONT_INFO),
                             (PVOID *) &a->BufPtr
                           );

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepGetFontInfo
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        try {
            RtlCopyMemory( lpConsoleFontInfo, a->BufPtr, a->NumFonts * sizeof(CONSOLE_FONT_INFO));
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            CsrFreeCaptureBuffer( CaptureBuffer );
            SET_LAST_ERROR(ERROR_INVALID_ACCESS);
            return 0;
        }
    }
    else {
        SET_LAST_NT_ERROR (m.ReturnValue);
    }
    CsrFreeCaptureBuffer( CaptureBuffer );
    return a->NumFonts;

}

COORD
WINAPI
GetConsoleFontSize(
    IN HANDLE hConsoleOutput,
    IN DWORD nFont
    )

/*++

Parameters:

    hConsoleOutput - Supplies an open handle to console output.

    nFont - Supplies the index of the font to return the size of.

Return Value:

    The return value is the height and width of each character in the font.
    X field contains width. Y field contains height. Font size
    is expressed in pixels.  If both the x and y sizes are 0, the function
    was unsuccessful.  Extended error information can be retrieved by calling
    the GetLastError function.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_GETFONTSIZE_MSG a = &m.u.GetConsoleFontSize;
    COORD Dummy;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->OutputHandle = hConsoleOutput;
    a->FontIndex = nFont;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepGetFontSize
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        return a->FontSize;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        Dummy.X = Dummy.Y = 0;
        return Dummy;
    }

}

BOOL
WINAPI
GetCurrentConsoleFont(
    IN HANDLE hConsoleOutput,
    IN BOOL bMaximumWindow,
    OUT PCONSOLE_FONT_INFO lpConsoleCurrentFont
    )

/*++

Parameters:

    hConsoleOutput - Supplies an open handle to console output.

    bMaximumWindow - TRUE if caller wants current font for the maximum
        window size.  FALSE if caller wants current font for the current
        window size.

    lpConsoleCurrentFont - A pointer to a buffer to receive the
        requested information.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_GETCURRENTFONT_MSG a = &m.u.GetCurrentConsoleFont;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->OutputHandle = hConsoleOutput;
    a->MaximumWindow = (BOOLEAN) bMaximumWindow;
    a->OutputHandle = hConsoleOutput;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepGetCurrentFont
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        try {
            lpConsoleCurrentFont->dwFontSize = a->FontSize;
            lpConsoleCurrentFont->nFont = a->FontIndex;
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            SET_LAST_ERROR(ERROR_INVALID_ACCESS);
            return FALSE;
        }
        return TRUE;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }

}

BOOL
WINAPI
SetConsoleMode(
    IN HANDLE hConsoleHandle,
    IN DWORD dwMode
    )

/*++

Parameters:

    hConsoleHandle - Supplies a console input or output handle.

    dwMode - Supplies mode.

        Input Mode Flags:

            ENABLE_LINE_INPUT - line oriented input is on.

            ENABLE_ECHO_INPUT - characters will be written to the screen as they are
                read.

            ENABLE_WINDOW_INPUT - the caller is windows-aware

        Output Mode Flags:

            ENABLE_LINE_OUTPUT - line oriented output is on.

            ENABLE_WRAP_AT_EOL_OUTPUT - the cursor will move to the
                beginning of the next line when the end of the row
                is reached.


Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_MODE_MSG a = &m.u.SetConsoleMode;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->Handle = hConsoleHandle;
    a->Mode = dwMode;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepSetMode
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        return TRUE;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }

}

BOOL
WINAPI
GenerateConsoleCtrlEvent(
    IN DWORD dwCtrlEvent,
    IN DWORD dwProcessGroupId
    )

/*++

Parameters:

    dwCtrlEvent - Supplies event(s) to generate.

    dwProcessGroupId - Supplies id of process group to generate
                  event for.  Event will be generated in each
                  process with that id within the console.  If 0,
                  specified event will be generated in all processes
                  within the console.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_CTRLEVENT_MSG a = &m.u.GenerateConsoleCtrlEvent;

    //
    // Check for valid Ctrl Events
    //

    if ((dwCtrlEvent != CTRL_C_EVENT) && (dwCtrlEvent != CTRL_BREAK_EVENT)) {
        SET_LAST_ERROR (ERROR_INVALID_PARAMETER);
        return(FALSE);
    }


    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->CtrlEvent = dwCtrlEvent;
    a->ProcessGroupId = dwProcessGroupId;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepGenerateCtrlEvent
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        return TRUE;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }

}

BOOL
WINAPI
SetConsoleActiveScreenBuffer(
    IN HANDLE hConsoleOutput
    )

/*++

Parameters:

    hConsoleOutput - Supplies an open handle to console output.  The screen
        buffer attached to this handle becomes the displayed screen buffer.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    
    CONSOLE_API_MSG m;
    PCONSOLE_SETACTIVESCREENBUFFER_MSG a = &m.u.SetConsoleActiveScreenBuffer;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->OutputHandle = hConsoleOutput;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepSetActiveScreenBuffer
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        return TRUE;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }

}


BOOL
WINAPI
FlushConsoleInputBuffer(
    IN HANDLE hConsoleInput
    )

/*++

Parameters:

    hConsoleInput - Supplies an open handle to console input.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_FLUSHINPUTBUFFER_MSG a = &m.u.FlushConsoleInputBuffer;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->InputHandle = hConsoleInput;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepFlushInputBuffer
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        return TRUE;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }

}


BOOL
WINAPI
SetConsoleScreenBufferSize(
    IN HANDLE hConsoleOutput,
    IN COORD dwSize
    )

/*++

Parameters:

    hConsoleInput - Supplies an open handle to console input.

    dwSize - New size of screen buffer in rows and columns

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_SETSCREENBUFFERSIZE_MSG a = &m.u.SetConsoleScreenBufferSize;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->OutputHandle = hConsoleOutput;
    a->Size = dwSize;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepSetScreenBufferSize
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        return TRUE;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }


}

BOOL
WINAPI
SetConsoleCursorPosition(
    IN HANDLE hConsoleOutput,
    IN COORD dwCursorPosition
    )

/*++

Parameters:

    hConsoleOutput - Supplies an open handle to console output.

    dwCursorPosition - Position of cursor in screen buffer

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{  

    CONSOLE_API_MSG m;
    PCONSOLE_SETCURSORPOSITION_MSG a = &m.u.SetConsoleCursorPosition;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->OutputHandle = hConsoleOutput;
    a->CursorPosition = dwCursorPosition;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepSetCursorPosition
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        return TRUE;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }


}

BOOL
WINAPI
SetConsoleCursorInfo(
    IN HANDLE hConsoleOutput,
    IN CONST CONSOLE_CURSOR_INFO *lpConsoleCursorInfo
    )

/*++

Parameters:

    hConsoleOutput - Supplies an open handle to console output.

    lpConsoleCursorOrigin - A pointer to a buffer containing the data
        to set.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_SETCURSORINFO_MSG a = &m.u.SetConsoleCursorInfo;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->OutputHandle = hConsoleOutput;
    try {
        a->CursorSize = lpConsoleCursorInfo->dwSize;
        a->Visible = (BOOLEAN) lpConsoleCursorInfo->bVisible;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        SET_LAST_ERROR(ERROR_INVALID_ACCESS);
        return FALSE;
    }
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepSetCursorInfo
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        return TRUE;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }

}

BOOL
WINAPI
SetConsoleWindowInfo(
    IN HANDLE hConsoleOutput,
    IN BOOL bAbsolute,
    IN CONST SMALL_RECT *lpConsoleWindow
    )

/*++

Parameters:

    hConsoleOutput - Supplies an open handle to console output.

    lpConsoleWindow - A pointer to a rectangle containing the new
        dimensions of the console window in screen buffer coordinates.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_SETWINDOWINFO_MSG a = &m.u.SetConsoleWindowInfo;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->OutputHandle = hConsoleOutput;
    a->Absolute = bAbsolute;
    try {
        a->Window = *lpConsoleWindow;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        SET_LAST_ERROR(ERROR_INVALID_ACCESS);
        return FALSE;
    }
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepSetWindowInfo
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        return TRUE;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }

}

BOOL
APIENTRY
ScrollConsoleScreenBufferInternal(
    IN HANDLE hConsoleOutput,
    IN CONST SMALL_RECT *lpScrollRectangle,
    IN CONST SMALL_RECT *lpClipRectangle,
    IN COORD dwDestinationOrigin,
    IN CONST CHAR_INFO *lpFill,
    IN BOOLEAN Unicode
    )

/*++

Parameters:

    hConsoleOutput - Supplies an open handle to console output.

    lpScrollRectangle - Pointer to region within screen buffer to move.

    lpClipRectangle -  Pointer to region within screen buffer that may be
        affected by this scroll.  This pointer may be NULL.

    dwDestinationOrigin - Upper left corner of new location of ScrollRectangle
        contents.

    lpFill - Pointer to structure containing new contents of ScrollRectangle region.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_SCROLLSCREENBUFFER_MSG a = &m.u.ScrollConsoleScreenBuffer;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->OutputHandle = hConsoleOutput;
    a->Unicode = Unicode;
    try {
        a->ScrollRectangle = *lpScrollRectangle;
        if (lpClipRectangle != NULL) {
            a->Clip = TRUE;
            a->ClipRectangle = *lpClipRectangle;
        }
        else {
            a->Clip = FALSE;
        }
        a->Fill = *lpFill;
        a->DestinationOrigin = dwDestinationOrigin;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        SET_LAST_ERROR(ERROR_INVALID_ACCESS);
        return FALSE;
    }
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepScrollScreenBuffer
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        return TRUE;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }

}

#endif //!defined(BUILD_WOW6432)

#if !defined(BUILD_WOW64)

BOOL
APIENTRY
ScrollConsoleScreenBufferA(
    HANDLE hConsoleOutput,
    CONST SMALL_RECT *lpScrollRectangle,
    CONST SMALL_RECT *lpClipRectangle,
    COORD dwDestinationOrigin,
    CONST CHAR_INFO *lpFill
    )
{
    return ScrollConsoleScreenBufferInternal(hConsoleOutput,
                                      lpScrollRectangle,
                                      lpClipRectangle,
                                      dwDestinationOrigin,
                                      lpFill,
                                      FALSE);
}

BOOL
APIENTRY
ScrollConsoleScreenBufferW(
    HANDLE hConsoleOutput,
    CONST SMALL_RECT *lpScrollRectangle,
    CONST SMALL_RECT *lpClipRectangle,
    COORD dwDestinationOrigin,
    CONST CHAR_INFO *lpFill
    )
{
    return ScrollConsoleScreenBufferInternal(hConsoleOutput,
                                      lpScrollRectangle,
                                      lpClipRectangle,
                                      dwDestinationOrigin,
                                      lpFill,
                                      TRUE);
}

#endif //!defined(BUILD_WOW64)

#if !defined(BUILD_WOW6432)

BOOL
WINAPI
SetConsoleTextAttribute(
    IN HANDLE hConsoleOutput,
    IN WORD wAttributes
    )

/*++

Parameters:

    hConsoleOutput - Supplies an open handle to console output.

    wAttributes - Character display attributes.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_SETTEXTATTRIBUTE_MSG a = &m.u.SetConsoleTextAttribute;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->OutputHandle = hConsoleOutput;
    a->Attributes = wAttributes;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepSetTextAttribute
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        return TRUE;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }

}

BOOL
WINAPI
SetConsoleFont(
    IN HANDLE hConsoleOutput,
    IN DWORD nFont
    )

/*++

Parameters:

    hConsoleOutput - Supplies an open handle to console output.

    nFont - Number of font to set as current font

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_SETFONT_MSG a = &m.u.SetConsoleFont;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->OutputHandle = hConsoleOutput;
    a->FontIndex = nFont;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepSetFont
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        return TRUE;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }

}

BOOL
WINAPI
SetConsoleIcon(
    IN HICON hIcon
    )

/*++

Parameters:

    hIcon - Supplies an icon handle.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_SETICON_MSG a = &m.u.SetConsoleIcon;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->hIcon = hIcon;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepSetIcon
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        return TRUE;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }

}

#endif //!defined(BUILD_WOW6432)

#if !defined(BUILD_WOW64)

BOOL
APIENTRY
SetConsoleMaximumWindowSize(
    HANDLE hConsoleOutput,
    COORD dwWindowSize
    )
{
    UNREFERENCED_PARAMETER(hConsoleOutput);
    UNREFERENCED_PARAMETER(dwWindowSize);

    return TRUE;
}

#endif //!defined(BUILD_WOW64)

#if !defined(BUILD_WOW6432)

UINT
WINAPI
GetConsoleCP( VOID )

/**++

Parameters:

    none

Return Value:

    The code page id of the current console.  a null return value
    indicates failure.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_GETCP_MSG a = &m.u.GetConsoleCP;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->Output = FALSE;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepGetCP
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        return a->wCodePageID;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }

}

BOOL
WINAPI
SetConsoleCP(
    IN UINT wCodePageID
    )

/**++

Parameters:

    wCodePageID - the code page is to set for the current console.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.


--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_SETCP_MSG a = &m.u.SetConsoleCP;
    NTSTATUS Status;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->Output = FALSE;
    a->wCodePageID = wCodePageID;
#if defined(FE_SB)
    Status = NtCreateEvent(&(a->hEvent),
                           EVENT_ALL_ACCESS, 
                           NULL,
                           SynchronizationEvent,
                           (BOOLEAN)FALSE
                           );
    if (!NT_SUCCESS(Status)) {
       SET_LAST_NT_ERROR(Status);
       return FALSE;
    }
#endif
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepSetCP
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
#if defined(FE_SB)
        NTSTATUS Status;

        Status = NtWaitForSingleObject(a->hEvent, FALSE, NULL);
        NtClose(a->hEvent);
        if (Status != 0) {
            SET_LAST_NT_ERROR(Status);
            return FALSE;
        }
#endif
        return TRUE;
    } else {
#if defined(FE_SB)
        NtClose(a->hEvent);
#endif
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }

}


UINT
WINAPI
GetConsoleOutputCP( VOID )

/**++

Parameters:

    none

Return Value:

    The code page id of the current console output.  a null return value
    indicates failure.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_GETCP_MSG a = &m.u.GetConsoleCP;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->Output = TRUE;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepGetCP
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        return a->wCodePageID;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }

}

NTSTATUS
APIENTRY
SetConsoleOutputCPInternal(
    IN UINT wCodePageID
    )
{
    CONSOLE_API_MSG m;
    PCONSOLE_SETCP_MSG a = &m.u.SetConsoleCP;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->Output = TRUE;
    a->wCodePageID = wCodePageID;
#if defined(FE_SB)
    a->hEvent = NULL;
#endif

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepSetCP
                                            ),
                         sizeof( *a )
                       );
   
    return m.ReturnValue;
}

#endif //!defined(BUILD_WOW6432)


#if !defined(BUILD_WOW64)

BOOL
WINAPI
SetConsoleOutputCP(
    IN UINT wCodePageID
    )

/**++

Parameters:

    wCodePageID - the code page is to set for the current console output.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.


--*/

{

    NTSTATUS Status;
    
    Status = SetConsoleOutputCPInternal(wCodePageID);
    
    if(NT_SUCCESS(Status)) {
        SetTEBLangID();
        return TRUE;
    }
    else {
        SET_LAST_NT_ERROR (Status);
        return FALSE;
    }
}

#endif //!defined(BUILD_WOW64)

#if !defined(BUILD_WOW6432)

BOOL
APIENTRY
GetConsoleKeyboardLayoutNameWorker(
    OUT LPSTR pszLayout,
    IN BOOL bAnsi)

/**++

Parameters:

    pszLayout  - address of buffer of least 9 characters
    bAnsi      - TRUE  want ANSI (8-bit) chars
                 FALSE want Unicode (16-bit) chars

Return Value:

    TRUE  - success
    FALSE - failure

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_GETKEYBOARDLAYOUTNAME_MSG a = &m.u.GetKeyboardLayoutName;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->bAnsi = bAnsi;

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepGetKeyboardLayoutName
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        if (bAnsi) {
           strncpy(pszLayout, a->achLayout, 9);
        } else {
           wcsncpy((LPWSTR)pszLayout, a->awchLayout, 9);
        }
        return TRUE;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }

}


#endif //!defined(BUILD_WOW6432)


#if !defined(BUILD_WOW64)

BOOL
GetConsoleKeyboardLayoutNameA(
    LPSTR pszLayout)
{
    return GetConsoleKeyboardLayoutNameWorker(pszLayout, TRUE);
}

BOOL
GetConsoleKeyboardLayoutNameW(
    LPWSTR pwszLayout)
{
    return GetConsoleKeyboardLayoutNameWorker((LPSTR)pwszLayout, FALSE);
}

#endif //!defined(BUILD_WOW64)

#if !defined(BUILD_WOW6432)

HANDLE
APIENTRY
GetConsoleWindow(
    VOID)
{

    CONSOLE_API_MSG m;
    PCONSOLE_GETCONSOLEWINDOW_MSG a = &m.u.GetConsoleWindow;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepGetConsoleWindow
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        return a->hwnd;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return NULL;
    }

}

#endif 
