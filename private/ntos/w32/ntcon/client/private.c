/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    private.c

Abstract:

    This file implements private APIs for Hardware Desktop Support.
    
Author:

    Therese Stowell (thereses) 12-13-1991

Revision History:

Notes:
    
--*/

#include "precomp.h"
#pragma hdrstop
#pragma hdrstop

#if !defined(BUILD_WOW64)

typedef HANDLE (*PCONVPALFUNC)(HANDLE);
PCONVPALFUNC pfnGdiConvertPalette;

#endif //!defined(BUILD_WOW64)

#if !defined(BUILD_WOW6432)

BOOL
WINAPI
SetConsoleCursor(
    IN HANDLE hConsoleOutput,
    IN HCURSOR hCursor
    )

/*++

Description:

    Sets the mouse pointer for the specified screen buffer.

Parameters:

    hConsoleOutput - Supplies a console output handle.

    hCursor - win32 cursor handle, should be NULL to set the default
        cursor.

Return value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_SETCURSOR_MSG a = &m.u.SetConsoleCursor;
    
    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->OutputHandle = hConsoleOutput;
    a->CursorHandle = hCursor;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepSetCursor
                                            ),
                         sizeof( *a )
                       );
    if (!NT_SUCCESS( m.ReturnValue)) {
        SET_LAST_NT_ERROR(m.ReturnValue);
        return FALSE;
    }
    else {
        return TRUE;
    }

}

int
WINAPI
ShowConsoleCursor(
    IN HANDLE hConsoleOutput,
    IN BOOL bShow
    )

/*++

Description:

    Sets the mouse pointer visibility counter.  If the counter is less than
    zero, the mouse pointer is not shown.

Parameters:

    hOutput - Supplies a console output handle.

    bShow - if TRUE, the display count is to be increased. if FALSE,
        decreased.

Return value:

    The return value specifies the new display count.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_SHOWCURSOR_MSG a = &m.u.ShowConsoleCursor;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->OutputHandle = hConsoleOutput;
    a->bShow = bShow;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepShowCursor
                                            ),
                         sizeof( *a )
                       );
    return a->DisplayCount;

}

HMENU
APIENTRY
ConsoleMenuControl(
    IN HANDLE hConsoleOutput,
    IN UINT dwCommandIdLow,
    IN UINT dwCommandIdHigh
    )

/*++

Description:

    Sets the command id range for the current screen buffer and returns the
    menu handle.

Parameters:

    hConsoleOutput - Supplies a console output handle.

    dwCommandIdLow - Specifies the lowest command id to store in the input buffer.

    dwCommandIdHigh - Specifies the highest command id to store in the input
        buffer.

Return value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_MENUCONTROL_MSG a = &m.u.ConsoleMenuControl;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->OutputHandle = hConsoleOutput;
    a->CommandIdLow =dwCommandIdLow;
    a->CommandIdHigh = dwCommandIdHigh;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepMenuControl
                                            ),
                         sizeof( *a )
                       );
    return a->hMenu;

}

BOOL
APIENTRY 
SetConsolePaletteInternal(
    IN HANDLE hConsoleOutput,
    IN HPALETTE hPalette,
    IN UINT dwUsage 
    )
{

    CONSOLE_API_MSG m;
    PCONSOLE_SETPALETTE_MSG a = &m.u.SetConsolePalette;
    NTSTATUS Status;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->OutputHandle = hConsoleOutput;
    a->hPalette = hPalette;
    a->dwUsage = dwUsage;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepSetPalette
                                            ),
                         sizeof( *a )
                       );
    if (!NT_SUCCESS( m.ReturnValue)) {
        SET_LAST_NT_ERROR(m.ReturnValue);
        return FALSE;
    }
    else {
        return TRUE;
    }

}

BOOL
APIENTRY
SetConsoleDisplayMode(
    IN HANDLE hConsoleOutput,
    IN DWORD dwFlags,
    OUT PCOORD lpNewScreenBufferDimensions
    )

/*++

Description:

    This routine sets the console display mode for an output buffer.
    This API is only supported on x86 machines.  Frame buffer consoles
    are always windowed.

Parameters:

    hConsoleOutput - Supplies a console output handle.

    dwFlags - Specifies the display mode. Options are:

        CONSOLE_FULLSCREEN_MODE - data is displayed fullscreen

        CONSOLE_WINDOWED_MODE - data is displayed in a window

    lpNewScreenBufferDimensions - On output, contains the new dimensions of
        the screen buffer.  The dimensions are in rows and columns for
        textmode screen buffers.
Return value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_SETDISPLAYMODE_MSG a = &m.u.SetConsoleDisplayMode;
    NTSTATUS Status;

#if !defined(_X86_)
    return FALSE;
#else
    
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

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->OutputHandle = hConsoleOutput;
    a->dwFlags = dwFlags;

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepSetDisplayMode
                                            ),
                         sizeof( *a )
                       );
    if (!NT_SUCCESS( m.ReturnValue)) {
        SET_LAST_NT_ERROR(m.ReturnValue);
        NtClose(a->hEvent);
        return FALSE;

    }
    else {
	     Status = NtWaitForSingleObject(a->hEvent, FALSE, NULL);
        NtClose(a->hEvent);

	 if (Status != 0) {
	    SET_LAST_NT_ERROR(Status);
	    return FALSE;
	 }
        try {
            *lpNewScreenBufferDimensions = a->ScreenBufferDimensions;
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            SET_LAST_ERROR(ERROR_INVALID_ACCESS);
            return FALSE;
        }
        return TRUE;
    }

#endif

}

BOOL
APIENTRY
RegisterConsoleVDM(
    IN DWORD dwRegisterFlags,
    IN HANDLE hStartHardwareEvent,
    IN HANDLE hEndHardwareEvent,
    IN LPWSTR lpStateSectionName,
    IN DWORD dwStateSectionNameLength,
    OUT LPDWORD lpStateLength,
    OUT PVOID *lpState,
    IN LPWSTR lpVDMBufferSectionName,
    IN DWORD dwVDMBufferSectionNameLength,
    IN COORD VDMBufferSize OPTIONAL,
    OUT PVOID *lpVDMBuffer
    )

/*++

Description:

    This routine registers the VDM with the console.

Parameters:

    hStartHardwareEvent - the event the VDM waits on to be
        notified of gaining/losing control of the hardware.

    hEndHardwareEvent - the event the VDM sets when it is done
        saving/restoring the hardware.

Return value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    CONSOLE_API_MSG m;
    PCONSOLE_REGISTERVDM_MSG a = &m.u.RegisterConsoleVDM;
    PCSR_CAPTURE_HEADER CaptureBuffer;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->RegisterFlags = dwRegisterFlags;
    if (dwRegisterFlags) {
        a->StartEvent = hStartHardwareEvent;
        a->EndEvent = hEndHardwareEvent;
        a->VDMBufferSectionNameLength = dwVDMBufferSectionNameLength;
        a->VDMBufferSize = VDMBufferSize;
        a->StateSectionNameLength = dwStateSectionNameLength;
        CaptureBuffer = CsrAllocateCaptureBuffer( 2,
                                                  dwStateSectionNameLength+dwVDMBufferSectionNameLength
                                                );
        if (CaptureBuffer == NULL) {
            SET_LAST_ERROR(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        CsrCaptureMessageBuffer( CaptureBuffer,
                                 lpStateSectionName,
                                 dwStateSectionNameLength,
                                 (PVOID *) &a->StateSectionName
                                   );

        CsrCaptureMessageBuffer( CaptureBuffer,
                                 lpVDMBufferSectionName,
                                 dwVDMBufferSectionNameLength,
                                 (PVOID *) &a->VDMBufferSectionName
                                   );
    } else {
        CaptureBuffer = NULL;
    }
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepRegisterVDM
                                            ),
                         sizeof( *a )
                       );
    if (CaptureBuffer != NULL) {
        CsrFreeCaptureBuffer( CaptureBuffer );
    }
    if (!NT_SUCCESS( m.ReturnValue)) {
        SET_LAST_NT_ERROR(m.ReturnValue);
        return FALSE;
    }
    else {
        if (dwRegisterFlags) {
            try {
                *lpStateLength = a->StateLength;
                *lpState = a->StateBuffer;
                *lpVDMBuffer = a->VDMBuffer;
            } except (EXCEPTION_EXECUTE_HANDLER) {
                SET_LAST_ERROR(ERROR_INVALID_ACCESS);
                return FALSE;
            }
        }
        return TRUE;
    }
}

BOOL
APIENTRY
GetConsoleHardwareState(
    IN HANDLE hConsoleOutput,
    OUT PCOORD lpResolution,
    OUT PCOORD lpFontSize
    )

/*++

Description:

    This routine returns the video resolution and font.

Parameters:

    hConsoleOutput - Supplies a console output handle.

    lpResolution - Pointer to structure to store screen
        resolution in.  Resolution is returned in pixels.

    lpFontSize - Pointer to structure to store font size in.
        Font size is returned in pixels.

Return value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_GETHARDWARESTATE_MSG a = &m.u.GetConsoleHardwareState;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->OutputHandle = hConsoleOutput;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepGetHardwareState
                                            ),
                         sizeof( *a )
                       );
    if (!NT_SUCCESS( m.ReturnValue)) {
        SET_LAST_NT_ERROR(m.ReturnValue);
        return FALSE;
    }
    else {
        try {
            *lpResolution = a->Resolution;
            *lpFontSize = a->FontSize;
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            SET_LAST_ERROR(ERROR_INVALID_ACCESS);
            return FALSE;
        }
        return TRUE;
    }

}

BOOL
APIENTRY
SetConsoleHardwareState(
    IN HANDLE hConsoleOutput,
    IN COORD dwResolution,
    IN COORD dwFontSize
    )

/*++

Description:

    This routine set the video resolution and font.

Parameters:

    hConsoleOutput - Supplies a console output handle.

    dwResolution - Contains screen resolution to set.
        Resolution is returned in pixels.

    dwFontSize - Contains font size to set.
        Font size is returned in pixels.

Return value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_SETHARDWARESTATE_MSG a = &m.u.SetConsoleHardwareState;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->OutputHandle = hConsoleOutput;
    a->Resolution = dwResolution;
    a->FontSize = dwFontSize;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepSetHardwareState
                                            ),
                         sizeof( *a )
                       );
    if (!NT_SUCCESS( m.ReturnValue)) {
        SET_LAST_NT_ERROR(m.ReturnValue);
        return FALSE;
    }
    else {
        return TRUE;
    }

}

BOOL
APIENTRY
GetConsoleDisplayMode(
    OUT LPDWORD lpModeFlags
    )

/*++

Description:

    This routine returns the display mode of the console.

Parameters:

    lpModeFlags - pointer to store display mode in.

Return value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_GETDISPLAYMODE_MSG a = &m.u.GetConsoleDisplayMode;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepGetDisplayMode
                                            ),
                         sizeof( *a )
                       );
    if (!NT_SUCCESS( m.ReturnValue)) {
        SET_LAST_NT_ERROR(m.ReturnValue);
        return FALSE;
    }
    else {
        try {
            *lpModeFlags = a->ModeFlags;
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            SET_LAST_ERROR(ERROR_INVALID_ACCESS);
            return FALSE;
        }
        return TRUE;
    }

}

BOOL
APIENTRY
SetConsoleKeyShortcuts(
    IN BOOL bSet,
    IN BYTE bReserveKeys,
    IN LPAPPKEY lpAppKeys,
    IN DWORD dwNumAppKeys
    )

/*++

Description:

    Only one set of key shortcuts is valid per console.  Calling
    SetConsoleKeyShortcuts(set) overwrites any previous settings.
    SetConsoleKeyShortcuts(!set) removes any shortcuts.

Parameters:

    bSet - if TRUE, set shortcuts.  else remove shortcuts.

    bReserveKeys - byte containing reserve key info.

    lpAppKeys - pointer to application-defined shortcut keys.  can be null.

    dwNumAppKeys - number of app keys contained in lpAppKeys.

Return value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_SETKEYSHORTCUTS_MSG a = &m.u.SetConsoleKeyShortcuts;
    PCSR_CAPTURE_HEADER CaptureBuffer;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->Set = bSet;
    a->ReserveKeys = bReserveKeys;
    if (lpAppKeys == NULL) {
        a->NumAppKeys = 0;
        CaptureBuffer = NULL;
    } else {
        a->NumAppKeys = dwNumAppKeys;
        CaptureBuffer = CsrAllocateCaptureBuffer( 1,
                                                  dwNumAppKeys * sizeof(APPKEY)
                                                );
        if (CaptureBuffer == NULL) {
            SET_LAST_ERROR(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
        CsrCaptureMessageBuffer( CaptureBuffer,
                                 lpAppKeys,
                                 dwNumAppKeys * sizeof(APPKEY),
                                 (PVOID *) &a->AppKeys
                               );
    }

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepSetKeyShortcuts
                                            ),
                         sizeof( *a )
                       );
    if (CaptureBuffer)
        CsrFreeCaptureBuffer( CaptureBuffer );
    if (!NT_SUCCESS( m.ReturnValue)) {
        SET_LAST_NT_ERROR(m.ReturnValue);
        return FALSE;
    }
    else {
        return TRUE;
    }

}

BOOL
APIENTRY
SetConsoleMenuClose(
    IN BOOL bEnable
    )

/*++

Description:

    This routine returns the display mode of the console.

Parameters:

    bEnable - if TRUE, close is enabled in the system menu.

Return value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_SETMENUCLOSE_MSG a = &m.u.SetConsoleMenuClose;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->Enable = bEnable;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepSetMenuClose
                                            ),
                         sizeof( *a )
                       );
    if (!NT_SUCCESS( m.ReturnValue)) {
        SET_LAST_NT_ERROR(m.ReturnValue);
        return FALSE;
    }
    else {
        return TRUE;
    }

}

#endif //!defined(BUILD_WOW6432)

#if !defined(BUILD_WOW64)

BOOL
SetConsolePalette(
    IN HANDLE hConsoleOutput,
    IN HPALETTE hPalette,
    IN UINT dwUsage
    )

/*++

Description:

    Sets the palette for the console screen buffer.

Parameters:

    hOutput - Supplies a console output handle.

    hPalette - Supplies a handle to the palette to set.

    dwUsage - Specifies use of the system palette.

        SYSPAL_NOSTATIC - System palette contains no static colors
                          except black and white.

        SYSPAL_STATIC -   System palette contains static colors
                          which will not change when an application
                          realizes its logical palette.

Return value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    HANDLE hmodGdi;
    UNICODE_STRING ModuleNameString;
    NTSTATUS Status;

    if ( !pfnGdiConvertPalette ) {
        RtlInitUnicodeString( &ModuleNameString, L"gdi32" );
        Status = LdrLoadDll( UNICODE_NULL, NULL, &ModuleNameString, &hmodGdi ); 
        if ( !NT_SUCCESS(Status) ) {
            SET_LAST_NT_ERROR(Status);
            return FALSE;
            }
        pfnGdiConvertPalette = (PCONVPALFUNC)GetProcAddress(hmodGdi, "GdiConvertPalette");
        if (pfnGdiConvertPalette == NULL) {
            SET_LAST_NT_ERROR(STATUS_PROCEDURE_NOT_FOUND);
            return FALSE;
            }

        }

    hPalette = (*pfnGdiConvertPalette)(hPalette);

    return SetConsolePaletteInternal(hConsoleOutput,
                                     hPalette,
                                     dwUsage);
}

BOOL
APIENTRY
WriteConsoleInputVDMA(
    HANDLE hConsoleInput,
    PINPUT_RECORD lpBuffer,
    DWORD nLength,
    LPDWORD lpNumberOfEventsWritten
    )
{
    return WriteConsoleInputInternal(hConsoleInput,lpBuffer,nLength,lpNumberOfEventsWritten,FALSE,FALSE);
}

BOOL
APIENTRY
WriteConsoleInputVDMW(
    HANDLE hConsoleInput,
    PINPUT_RECORD lpBuffer,
    DWORD nLength,
    LPDWORD lpNumberOfEventsWritten
    )
{
    return WriteConsoleInputInternal(hConsoleInput,lpBuffer,nLength,lpNumberOfEventsWritten,TRUE,FALSE);
}

#endif //!defined(BUILD_WOW6432)
