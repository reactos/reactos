/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    iostubs.c

Abstract:

    This module contains the stubs for the console I/O API.

Author:

    Therese Stowell (thereses) 14-Nov-1990

Revision History:
    
--*/

#include "precomp.h"
#pragma hdrstop
#pragma hdrstop

#if !defined(BUILD_WOW6432)

BOOL
APIENTRY
GetConsoleInput(
    IN HANDLE hConsoleInput,
    OUT PINPUT_RECORD lpBuffer,
    IN DWORD nLength,
    OUT LPDWORD lpNumberOfEventsRead,
    IN USHORT wFlags,
    IN BOOLEAN Unicode
    )

/*++

Parameters:

    hConsoleInput - Supplies an open handle to CONIN$ that is to be
        read.  The handle must have been created with GENERIC_READ access.

    lpBuffer - Supplies the address of a buffer to receive the data read
        from the input buffer.

    nLength - Supplies the length of lpBuffer in INPUT_RECORDs.

    lpNumberOfEventsRead - Pointer to number of events read.

    wFlags - Flags that control how data is read.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed.
        Extended error status is available using GetLastError.

--*/

{
    PCSR_CAPTURE_HEADER CaptureBuffer;
    CONSOLE_API_MSG m;
    PCONSOLE_GETCONSOLEINPUT_MSG a = &m.u.GetConsoleInput;

    //
    // If it's not a well formed handle, don't bother going to the server.
    //

    if (!CONSOLE_HANDLE(hConsoleInput)) {
        try {
            *lpNumberOfEventsRead = 0;
            SET_LAST_ERROR(ERROR_INVALID_HANDLE);
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            SET_LAST_ERROR(ERROR_INVALID_ACCESS);
        }
        return FALSE;
    }

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->InputHandle = hConsoleInput;
    a->NumRecords = nLength;
    a->Flags = wFlags;
    a->Unicode = Unicode;

    //
    // for speed in the case where we're only reading a few records, we have
    // a buffer in the msg we pass to the server. this means we don't have to
    // allocate a capture buffer.
    //

    if (nLength > INPUT_RECORD_BUFFER_SIZE) {
        CaptureBuffer = CsrAllocateCaptureBuffer( 1,
                                                  nLength * sizeof(INPUT_RECORD)
                                                );
        if (CaptureBuffer == NULL) {
            SET_LAST_ERROR(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
        CsrCaptureMessageBuffer( CaptureBuffer,
                                 NULL,
                                 nLength * sizeof(INPUT_RECORD),
                                 (PVOID *) &a->BufPtr
                               );

    } else {
        a->BufPtr = a->Record;
        CaptureBuffer = NULL;
    }
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepGetConsoleInput
                                            ),
                         sizeof( *a )
                       );

    try {
        if (NT_SUCCESS( m.ReturnValue )) {
            *lpNumberOfEventsRead = a->NumRecords;
            RtlCopyMemory(lpBuffer, a->BufPtr, a->NumRecords * sizeof(INPUT_RECORD));
        }
        else {
            *lpNumberOfEventsRead = 0;
            SET_LAST_NT_ERROR(m.ReturnValue);
        }
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        if (CaptureBuffer != NULL) {
            CsrFreeCaptureBuffer( CaptureBuffer );
        }
        SET_LAST_ERROR(ERROR_INVALID_ACCESS);
        return FALSE;
    }
    if (CaptureBuffer != NULL) {
        CsrFreeCaptureBuffer( CaptureBuffer );
    }

    return NT_SUCCESS(m.ReturnValue);

}

#endif //!defined(BUILD_WOW6432)

#if !defined(BUILD_WOW64)

BOOL
APIENTRY
PeekConsoleInputA(
    HANDLE hConsoleInput,
    PINPUT_RECORD lpBuffer,
    DWORD nLength,
    LPDWORD lpNumberOfEventsRead
    )

{
    return GetConsoleInput(hConsoleInput,
                           lpBuffer,
                           nLength,
                           lpNumberOfEventsRead,
                           CONSOLE_READ_NOREMOVE | CONSOLE_READ_NOWAIT,
                           FALSE);
}

BOOL
APIENTRY
PeekConsoleInputW(
    HANDLE hConsoleInput,
    PINPUT_RECORD lpBuffer,
    DWORD nLength,
    LPDWORD lpNumberOfEventsRead
    )

/*++

Parameters:

    hConsoleInput - Supplies an open handle to CONIN$ that is to be
        read.  The handle must have been created with GENERIC_READ access.

    lpBuffer - Supplies the address of a buffer to receive the data read
        from the input buffer.

    nLength - Supplies the length of lpBuffer in INPUT_RECORDs.

    lpNumberOfEventsRead - Pointer to number of events read.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed.
        Extended error status is available using GetLastError.

--*/

{
    return GetConsoleInput(hConsoleInput,
                           lpBuffer,
                           nLength,
                           lpNumberOfEventsRead,
                           CONSOLE_READ_NOREMOVE | CONSOLE_READ_NOWAIT,
                           TRUE);
}

BOOL
APIENTRY
ReadConsoleInputA(
    HANDLE hConsoleInput,
    PINPUT_RECORD lpBuffer,
    DWORD nLength,
    LPDWORD lpNumberOfEventsRead
    )

{
    return GetConsoleInput(hConsoleInput,
                           lpBuffer,
                           nLength,
                           lpNumberOfEventsRead,
                           0,
                           FALSE);
}

BOOL
APIENTRY
ReadConsoleInputW(
    HANDLE hConsoleInput,
    PINPUT_RECORD lpBuffer,
    DWORD nLength,
    LPDWORD lpNumberOfEventsRead
    )

/*++

Parameters:

    hConsoleInput - Supplies an open handle to CONIN$ that is to be
        read.  The handle must have been created with GENERIC_READ access.

    lpBuffer - Supplies the address of a buffer to receive the data read
        from the input buffer.

    nLength - Supplies the length of lpBuffer in INPUT_RECORDs.

    lpNumberOfEventsRead - Pointer to number of events read.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed.
        Extended error status is available using GetLastError.

--*/

{
    return GetConsoleInput(hConsoleInput,
                           lpBuffer,
                           nLength,
                           lpNumberOfEventsRead,
                           0,
                           TRUE);
}

BOOL
APIENTRY
ReadConsoleInputExA(
    HANDLE hConsoleInput,
    PINPUT_RECORD lpBuffer,
    DWORD nLength,
    LPDWORD lpNumberOfEventsRead,
    USHORT wFlags
    )

{
    return GetConsoleInput(hConsoleInput,
                           lpBuffer,
                           nLength,
                           lpNumberOfEventsRead,
                           wFlags,
                           FALSE);
}

BOOL
APIENTRY
ReadConsoleInputExW(
    HANDLE hConsoleInput,
    PINPUT_RECORD lpBuffer,
    DWORD nLength,
    LPDWORD lpNumberOfEventsRead,
    USHORT wFlags
    )

/*++

Parameters:

    hConsoleInput - Supplies an open handle to CONIN$ that is to be
        read.  The handle must have been created with GENERIC_READ access.

    lpBuffer - Supplies the address of a buffer to receive the data read
        from the input buffer.

    nLength - Supplies the length of lpBuffer in INPUT_RECORDs.

    lpNumberOfEventsRead - Pointer to number of events read.

    wFlags - Flags that control how data is read.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed.
        Extended error status is available using GetLastError.

--*/

{
    return GetConsoleInput(hConsoleInput,
                           lpBuffer,
                           nLength,
                           lpNumberOfEventsRead,
                           wFlags,
                           TRUE);
}

#endif //!defined(BUILD_WOW64)

#if !defined(BUILD_WOW6432)

BOOL
APIENTRY
WriteConsoleInputInternal(
    IN HANDLE hConsoleInput,
    IN CONST INPUT_RECORD *lpBuffer,
    IN DWORD nLength,
    OUT LPDWORD lpNumberOfEventsWritten,
    IN BOOLEAN Unicode,
    IN BOOLEAN Append
    )

/*++
Parameters:

    hConsoleInput - Supplies an open handle to CONIN$ that is to be
        written.  The handle must have been created with GENERIC_WRITE access.

    lpBuffer - Supplies the address of a buffer containing the input records
        to write to the input buffer.

    nLength - Supplies the length of lpBuffer in INPUT_RECORDs.

    lpNumberOfEventsWritten - Pointer to number of events written.

    Unicode - TRUE if characters are unicode

    Append - TRUE if append to end of input stream.  if FALSE, write to the
    beginning of the input stream (used by VDM).

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed.
        Extended error status is available using GetLastError.

--*/

{
    PCSR_CAPTURE_HEADER CaptureBuffer;
    CONSOLE_API_MSG m;
    PCONSOLE_WRITECONSOLEINPUT_MSG a = &m.u.WriteConsoleInput;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->InputHandle = hConsoleInput;
    a->NumRecords = nLength;
    a->Unicode = Unicode;
    a->Append = Append;

    //
    // for speed in the case where we're only writing a few records, we have
    // a buffer in the msg we pass to the server. this means we don't have to
    // allocate a capture buffer.
    //

    if (nLength > INPUT_RECORD_BUFFER_SIZE) {
        CaptureBuffer = CsrAllocateCaptureBuffer( 1,
                                                  nLength * sizeof(INPUT_RECORD)
                                                );
        if (CaptureBuffer == NULL) {
            SET_LAST_ERROR(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
        CsrCaptureMessageBuffer( CaptureBuffer,
                                 (PCHAR) lpBuffer,
                                 nLength * sizeof(INPUT_RECORD),
                                 (PVOID *) &a->BufPtr
                               );

    } else {
        a->BufPtr = a->Record;
        CaptureBuffer = NULL;
        try {
            RtlCopyMemory(a->BufPtr, lpBuffer, nLength * sizeof(INPUT_RECORD));
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            SET_LAST_ERROR(ERROR_INVALID_ACCESS);
            return FALSE;
        }
    }
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepWriteConsoleInput
                                            ),
                         sizeof( *a )
                       );
    if (CaptureBuffer != NULL) {
        CsrFreeCaptureBuffer( CaptureBuffer );
    }
    try {
        *lpNumberOfEventsWritten = a->NumRecords;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        SET_LAST_ERROR(ERROR_INVALID_ACCESS);
        return FALSE;
    }
    if (!NT_SUCCESS(m.ReturnValue)) {
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
APIENTRY
WriteConsoleInputA(
    HANDLE hConsoleInput,
    CONST INPUT_RECORD *lpBuffer,
    DWORD nLength,
    LPDWORD lpNumberOfEventsWritten
    )
{
    return WriteConsoleInputInternal(hConsoleInput,lpBuffer,nLength,lpNumberOfEventsWritten,FALSE,TRUE);
}

BOOL
APIENTRY
WriteConsoleInputW(
    HANDLE hConsoleInput,
    CONST INPUT_RECORD *lpBuffer,
    DWORD nLength,
    LPDWORD lpNumberOfEventsWritten
    )
{
    return WriteConsoleInputInternal(hConsoleInput,lpBuffer,nLength,lpNumberOfEventsWritten,TRUE,TRUE);
}


#endif //!defined(BUILD_WOW64)

#if !defined(BUILD_WOW6432)

VOID
CopyRectangle(
    IN CONST CHAR_INFO *Source,
    IN COORD SourceSize,
    IN PSMALL_RECT SourceRect,
    OUT PCHAR_INFO Target,
    IN COORD TargetSize,
    IN COORD TargetPoint
    )

/*++

Routine Description:

    This routine copies a rectangular region, doing any necessary clipping.

Arguments:

    Source - pointer to source buffer

    SourceSize - dimensions of source buffer

    SourceRect - rectangle in source buffer to copy

    Target - pointer to target buffer

    TargetSize - dimensions of target buffer

    TargetPoint - upper left coordinates of target rectangle

Return Value:


--*/

{

#define SCREEN_BUFFER_POINTER(BASE,X,Y,XSIZE,CELLSIZE) ((ULONG_PTR)BASE + ((XSIZE * (Y)) + (X)) * (ULONG)CELLSIZE)

    CONST CHAR_INFO *SourcePtr;
    PCHAR_INFO TargetPtr;
    SHORT i,j;
    SHORT XSize,YSize;
    BOOLEAN WholeSource,WholeTarget;

    XSize = (SHORT)CONSOLE_RECT_SIZE_X(SourceRect);
    YSize = (SHORT)CONSOLE_RECT_SIZE_Y(SourceRect);

    // do clipping.  we only clip for target, not source.

    if (XSize > (SHORT)(TargetSize.X - TargetPoint.X + 1)) {
        XSize = (SHORT)(TargetSize.X - TargetPoint.X + 1);
    }
    if (YSize > (SHORT)(TargetSize.Y - TargetPoint.Y + 1)) {
        YSize = (SHORT)(TargetSize.Y - TargetPoint.Y + 1);
    }

    WholeSource = WholeTarget = FALSE;
    if (XSize == SourceSize.X) {
        ASSERT (SourceRect->Left == 0);
        if (SourceRect->Top == 0) {
            SourcePtr = Source;
        }
        else {
            SourcePtr = (PCHAR_INFO) SCREEN_BUFFER_POINTER(Source,
                                                           SourceRect->Left,
                                                           SourceRect->Top,
                                                           SourceSize.X,
                                                           sizeof(CHAR_INFO));
        }
        WholeSource = TRUE;
    }
    if (XSize == TargetSize.X) {
        ASSERT (TargetPoint.X == 0);
        if (TargetPoint.Y == 0) {
            TargetPtr = Target;
        }
        else {
            TargetPtr = (PCHAR_INFO) SCREEN_BUFFER_POINTER(Target,
                                                           TargetPoint.X,
                                                           TargetPoint.Y,
                                                           TargetSize.X,
                                                           sizeof(CHAR_INFO));
        }
        WholeTarget = TRUE;
    }
    if (WholeSource && WholeTarget) {
        memmove(TargetPtr,SourcePtr,XSize*YSize*sizeof(CHAR_INFO));
        return;
    }

    for (i=0;i<YSize;i++) {
        if (!WholeTarget) {
            TargetPtr = (PCHAR_INFO) SCREEN_BUFFER_POINTER(Target,
                                                           TargetPoint.X,
                                                           TargetPoint.Y+i,
                                                           TargetSize.X,
                                                           sizeof(CHAR_INFO));
        }
        if (!WholeSource) {
            SourcePtr = (PCHAR_INFO) SCREEN_BUFFER_POINTER(Source,
                                                           SourceRect->Left,
                                                           SourceRect->Top+i,
                                                           SourceSize.X,
                                                           sizeof(CHAR_INFO));
        }
        for (j=0;j<XSize;j++,SourcePtr++,TargetPtr++) {
            *TargetPtr = *SourcePtr;
        }
    }
}

BOOL
APIENTRY
ReadConsoleOutputInternal(
    IN HANDLE hConsoleOutput,
    OUT PCHAR_INFO lpBuffer,
    IN COORD dwBufferSize,
    IN COORD dwBufferCoord,
    IN OUT PSMALL_RECT lpReadRegion,
    IN BOOLEAN Unicode
    )

/*++
Parameters:

    hConsoleOutput - Supplies an open handle to the screen buffer (CONOUT$)
        that is to be read.  The handle must have been created with
        GENERIC_READ access.

    lpBuffer - Supplies the address of a buffer to receive the data read
        from the screen buffer.  This pointer is treated as the origin of
        a two dimensional array of size dwBufferSize.

    dwBufferSize - size of lpBuffer

    dwBufferCoord - coordinates of upper left point in buffer to receive
        read data.

    lpReadRegion - Supplies on input the address of a structure indicating the
        rectangle within the screen buffer to read from.  The fields in
        the structure are column and row coordinates.  On output, the fields
        contain the actual region read.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.
--*/

{
    PCSR_CAPTURE_HEADER CaptureBuffer;
    CONSOLE_API_MSG m;
    PCONSOLE_READCONSOLEOUTPUT_MSG a = &m.u.ReadConsoleOutput;
    ULONG NumChars;
    COORD SourceSize;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->OutputHandle = hConsoleOutput;
    a->Unicode = Unicode;

    //
    // clip region to read based on caller's buffer size
    //

    SourceSize.X = (SHORT)CONSOLE_RECT_SIZE_X(lpReadRegion);
    if (SourceSize.X > dwBufferSize.X-dwBufferCoord.X)
        SourceSize.X = dwBufferSize.X-dwBufferCoord.X;
    SourceSize.Y = (SHORT)CONSOLE_RECT_SIZE_Y(lpReadRegion);
    if (SourceSize.Y > dwBufferSize.Y-dwBufferCoord.Y)
        SourceSize.Y = dwBufferSize.Y-dwBufferCoord.Y;

    a->CharRegion.Left = lpReadRegion->Left;
    a->CharRegion.Right = (SHORT)(lpReadRegion->Left + SourceSize.X - 1);
    a->CharRegion.Top  = lpReadRegion->Top;
    a->CharRegion.Bottom = (SHORT)(lpReadRegion->Top + SourceSize.Y - 1);

    //
    // for speed in the case where we're only reading one character, we have
    // a buffer in the msg we pass to the server. this means we don't have to
    // allocate a capture buffer.
    //

    NumChars = SourceSize.X * SourceSize.Y;
    if (NumChars > 1) {
        CaptureBuffer = CsrAllocateCaptureBuffer( 1,
                                                  NumChars * sizeof(CHAR_INFO)
                                                );
        if (CaptureBuffer == NULL) {
            SET_LAST_ERROR(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
        CsrCaptureMessageBuffer( CaptureBuffer,
                                 NULL,
                                 NumChars * sizeof(CHAR_INFO),
                                 (PVOID *) &a->BufPtr
                               );
    }
    else {
        CaptureBuffer = NULL;
        a->BufPtr = &a->Char;
    }
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepReadConsoleOutput
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        try {
            SMALL_RECT SourceRect;

            SourceRect.Left = a->CharRegion.Left - lpReadRegion->Left;
            SourceRect.Top = a->CharRegion.Top - lpReadRegion->Top;
            SourceRect.Right = SourceRect.Left +
                    (a->CharRegion.Right - a->CharRegion.Left);
            SourceRect.Bottom =  SourceRect.Top +
                    (a->CharRegion.Bottom - a->CharRegion.Top);
            dwBufferCoord.X += SourceRect.Left;
            dwBufferCoord.Y += SourceRect.Top;
            CopyRectangle(a->BufPtr,
                          SourceSize,
                          &SourceRect,
                          lpBuffer,
                          dwBufferSize,
                          dwBufferCoord
                         );
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            if (CaptureBuffer != NULL) {
                CsrFreeCaptureBuffer( CaptureBuffer );
            }
            SET_LAST_ERROR(ERROR_INVALID_ACCESS);
            return FALSE;
        }
    }
    if (CaptureBuffer != NULL) {
        CsrFreeCaptureBuffer( CaptureBuffer );
    }
    *lpReadRegion = a->CharRegion;
    if (!NT_SUCCESS(m.ReturnValue)) {
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
APIENTRY
ReadConsoleOutputW(
    HANDLE hConsoleOutput,
    PCHAR_INFO lpBuffer,
    COORD dwBufferSize,
    COORD dwBufferCoord,
    PSMALL_RECT lpReadRegion
    )
{
    return ReadConsoleOutputInternal(hConsoleOutput,
                                     lpBuffer,
                                     dwBufferSize,
                                     dwBufferCoord,
                                     lpReadRegion,
                                     TRUE
                                    );
}

BOOL
APIENTRY
ReadConsoleOutputA(
    HANDLE hConsoleOutput,
    PCHAR_INFO lpBuffer,
    COORD dwBufferSize,
    COORD dwBufferCoord,
    PSMALL_RECT lpReadRegion
    )
{
    return ReadConsoleOutputInternal(hConsoleOutput,
                                     lpBuffer,
                                     dwBufferSize,
                                     dwBufferCoord,
                                     lpReadRegion,
                                     FALSE
                                    );
}

#endif //!defined(BUILD_WOW64)

#if !defined(BUILD_WOW6432)

BOOL
APIENTRY
WriteConsoleOutputInternal(
    IN HANDLE hConsoleOutput,
    IN CONST CHAR_INFO *lpBuffer,
    IN COORD dwBufferSize,
    IN COORD dwBufferCoord,
    IN PSMALL_RECT lpWriteRegion,
    IN BOOLEAN Unicode
    )

/*++
Parameters:

    hConsoleOutput - Supplies an open handle to the screen buffer (CONOUT$)
        that is to be written.  The handle must have been created with
        GENERIC_WRITE access.

    lpBuffer - Supplies the address of a buffer containing the data to write
        to the screen buffer.  This buffer is treated as a two dimensional
        array.

    dwBufferSize - size of lpBuffer

    dwBufferCoord - coordinates of upper left point in buffer to write data
        from.

    lpWriteRegion - Supplies on input the address of a structure indicating the
        rectangle within the screen buffer to write to.  The fields in
        the structure are column and row coordinates.  On output, the fields
        contain the actual region written.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.
--*/

{

    PCSR_CAPTURE_HEADER CaptureBuffer;
    CONSOLE_API_MSG m;
    PCONSOLE_WRITECONSOLEOUTPUT_MSG a = &m.u.WriteConsoleOutput;
    ULONG NumChars;
    COORD SourceSize;
    COORD TargetPoint;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->OutputHandle = hConsoleOutput;
    a->Unicode = Unicode;

    //
    // clip region to write based on caller's buffer size
    //

    SourceSize.X = (SHORT)CONSOLE_RECT_SIZE_X(lpWriteRegion);
    if (SourceSize.X > dwBufferSize.X-dwBufferCoord.X)
        SourceSize.X = dwBufferSize.X-dwBufferCoord.X;
    SourceSize.Y = (SHORT)CONSOLE_RECT_SIZE_Y(lpWriteRegion);
    if (SourceSize.Y > dwBufferSize.Y-dwBufferCoord.Y)
        SourceSize.Y = dwBufferSize.Y-dwBufferCoord.Y;

    if (SourceSize.X <= 0 ||
        SourceSize.Y <= 0) {
        SET_LAST_ERROR(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    a->CharRegion.Left = lpWriteRegion->Left;
    a->CharRegion.Right = (SHORT)(lpWriteRegion->Left + SourceSize.X - 1);
    a->CharRegion.Top  = lpWriteRegion->Top;
    a->CharRegion.Bottom = (SHORT)(lpWriteRegion->Top + SourceSize.Y - 1);

    //
    // for speed in the case where we're only writing one character, we have
    // a buffer in the msg we pass to the server. this means we don't have to
    // allocate a capture buffer.
    //

    NumChars = SourceSize.X * SourceSize.Y;
    if (NumChars > 1) {
        CaptureBuffer = CsrAllocateCaptureBuffer( 1,
                                                  NumChars * sizeof(CHAR_INFO)
                                                );
        if (CaptureBuffer == NULL) {
            a->ReadVM=TRUE;
            a->BufPtr = RtlAllocateHeap( RtlProcessHeap(), 0, NumChars * sizeof(CHAR_INFO));
            if (a->BufPtr == NULL) {
                SET_LAST_ERROR(ERROR_NOT_ENOUGH_MEMORY);
                return FALSE;
            }
        } else {
            a->ReadVM=FALSE;
            CsrCaptureMessageBuffer( CaptureBuffer,
                                     NULL,
                                     NumChars * sizeof(CHAR_INFO),
                                     (PVOID *) &a->BufPtr
                                   );
        }
    }
    else {
        a->ReadVM=FALSE;
        CaptureBuffer = NULL;
        a->BufPtr = &a->Char;
    }
    try {
        SMALL_RECT SourceRect;

        SourceRect.Left = dwBufferCoord.X;
        SourceRect.Top = dwBufferCoord.Y;
        SourceRect.Right = (SHORT)(dwBufferCoord.X+SourceSize.X-1);
        SourceRect.Bottom = (SHORT)(dwBufferCoord.Y+SourceSize.Y-1);
        TargetPoint.X = 0;
        TargetPoint.Y = 0;
        CopyRectangle(lpBuffer,
                      dwBufferSize,
                      &SourceRect,
                      a->BufPtr,
                      SourceSize,
                      TargetPoint
                     );
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        if (CaptureBuffer != NULL) {
            CsrFreeCaptureBuffer( CaptureBuffer );
        } else if (a->ReadVM) {
            // a->BufPtr was allocated with RtlAllocateHeap.
            RtlFreeHeap( RtlProcessHeap(), 0, a->BufPtr);  
        }
        SET_LAST_ERROR(ERROR_INVALID_ACCESS);
        return FALSE;
    }
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepWriteConsoleOutput
                                            ),
                         sizeof( *a )
                       );
    if (CaptureBuffer != NULL) {
        CsrFreeCaptureBuffer( CaptureBuffer );
    } else if (a->ReadVM) {
        // a->BufPtr was allocated with RtlAllocateHeap.
        RtlFreeHeap(RtlProcessHeap(),0,a->BufPtr);       
    }
    *lpWriteRegion = a->CharRegion;
    if (!NT_SUCCESS(m.ReturnValue)) {
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
APIENTRY
WriteConsoleOutputW(
    HANDLE hConsoleOutput,
    CONST CHAR_INFO *lpBuffer,
    COORD dwBufferSize,
    COORD dwBufferCoord,
    PSMALL_RECT lpWriteRegion
    )
{
    return WriteConsoleOutputInternal(hConsoleOutput,
                                      lpBuffer,
                                      dwBufferSize,
                                      dwBufferCoord,
                                      lpWriteRegion,
                                      TRUE
                                      );
}

BOOL
APIENTRY
WriteConsoleOutputA(
    HANDLE hConsoleOutput,
    CONST CHAR_INFO *lpBuffer,
    COORD dwBufferSize,
    COORD dwBufferCoord,
    PSMALL_RECT lpWriteRegion
    )
{
    return WriteConsoleOutputInternal(hConsoleOutput,
                                      lpBuffer,
                                      dwBufferSize,
                                      dwBufferCoord,
                                      lpWriteRegion,
                                      FALSE
                                      );
}

#endif //defined(BUILD_WOW64)

#if !defined(BUILD_WOW6432)

BOOL
APIENTRY
ReadConsoleOutputString(
    IN HANDLE hConsoleOutput,
    OUT LPVOID lpString,
    IN DWORD nLength,
    IN DWORD nSize,
    IN DWORD fFlags,
    IN COORD dwReadCoord,
    OUT LPDWORD lpNumberOfElementsRead
    )

/*++

Parameters:

    hConsoleOutput - Supplies an open handle to the screen buffer (CONOUT$)
        that is to be read.  The handle must have been created with
        GENERIC_READ access.

    lpString - Supplies the address of a buffer to receive the character
        or attribute string read from the screen buffer.

    nLength - Size of lpCharacter buffer in elements.

    nSize - Size of element to read.

    fFlags - flag indicating what type of string to copy

    dwReadCoord - Screen buffer coordinates of string to read.
        read data.

    lpNumberOfElementsRead - Pointer to number of events read.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed.
        Extended error status is available using GetLastError.

--*/

{
    PCSR_CAPTURE_HEADER CaptureBuffer;
    CONSOLE_API_MSG m;
    PCONSOLE_READCONSOLEOUTPUTSTRING_MSG a = &m.u.ReadConsoleOutputString;
    ULONG DataLength;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->OutputHandle = hConsoleOutput;
    a->NumRecords = nLength;
    a->StringType = fFlags;
    a->ReadCoord = dwReadCoord;

    DataLength = nLength*nSize;

    //
    // for speed in the case where the string is small, we have a buffer
    // in the msg we pass to the server. this means we don't have to
    // allocate a capture buffer.
    //

    if (DataLength > sizeof(a->String)) {
        CaptureBuffer = CsrAllocateCaptureBuffer( 1,
                                                  DataLength
                                                );
        if (CaptureBuffer == NULL) {
            SET_LAST_ERROR(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
        CsrCaptureMessageBuffer( CaptureBuffer,
                                 NULL,
                                 DataLength,
                                 (PVOID *) &a->BufPtr
                               );

    }
    else {
        a->BufPtr = a->String;
        CaptureBuffer = NULL;
    }
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepReadConsoleOutputString
                                            ),
                         sizeof( *a )
                       );

    try {
       *lpNumberOfElementsRead = a->NumRecords;
        if (NT_SUCCESS( m.ReturnValue )) {
            RtlCopyMemory(lpString, a->BufPtr, a->NumRecords * nSize);
        }
        else {
            SET_LAST_NT_ERROR(m.ReturnValue);
        }
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        if (CaptureBuffer != NULL) {
            CsrFreeCaptureBuffer( CaptureBuffer );
        }
        SET_LAST_ERROR(ERROR_INVALID_ACCESS);
        return FALSE;
    }
    if (CaptureBuffer != NULL) {
        CsrFreeCaptureBuffer( CaptureBuffer );
    }
    return NT_SUCCESS(m.ReturnValue);
}

#endif //!defined(BUILD_WOW6432)

#if !defined(BUILD_WOW64)

BOOL
APIENTRY
ReadConsoleOutputCharacterA(
    HANDLE hConsoleOutput,
    LPSTR lpCharacter,
    DWORD nLength,
    COORD dwReadCoord,
    LPDWORD lpNumberOfCharsRead
    )

/*++

Parameters:

    hConsoleOutput - Supplies an open handle to the screen buffer (CONOUT$)
        that is to be read.  The handle must have been created with
        GENERIC_READ access.

    lpCharacter - Supplies the address of a buffer to receive the character
        string read from the screen buffer.

    nLength - Size of lpCharacter buffer in characters.

    dwReadCoord - Screen buffer coordinates of string to read.
        read data.

    lpNumberOfCharsRead - Pointer to number of chars read.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed.
        Extended error status is available using GetLastError.

--*/

{
    return ReadConsoleOutputString(hConsoleOutput,
                                   lpCharacter,
                                   nLength,
                                   sizeof(CHAR),
                                   CONSOLE_ASCII,
                                   dwReadCoord,
                                   lpNumberOfCharsRead
                                  );
}


BOOL
APIENTRY
ReadConsoleOutputCharacterW(
    HANDLE hConsoleOutput,
    LPWSTR lpCharacter,
    DWORD nLength,
    COORD dwReadCoord,
    LPDWORD lpNumberOfCharsRead
    )

{
    return ReadConsoleOutputString(hConsoleOutput,
                                   lpCharacter,
                                   nLength,
                                   sizeof(WCHAR),
                                   CONSOLE_REAL_UNICODE,
                                   dwReadCoord,
                                   lpNumberOfCharsRead
                                  );
}


BOOL
APIENTRY
ReadConsoleOutputAttribute(
    HANDLE hConsoleOutput,
    LPWORD lpAttribute,
    DWORD nLength,
    COORD dwReadCoord,
    LPDWORD lpNumberOfAttrsRead
    )

/*++

Parameters:

    hConsoleOutput - Supplies an open handle to the screen buffer (CONOUT$)
        that is to be read.  The handle must have been created with
        GENERIC_READ access.

    lpAttribute - Supplies the address of a buffer to receive the attribute
        string read from the screen buffer.

    nLength - Size of lpAttribute buffer in bytes.

    dwReadCoord - Screen buffer coordinates of string to read.
        read data.

    lpNumberOfAttrsRead - Pointer to number of attrs read.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed.
        Extended error status is available using GetLastError.

--*/

{
    return ReadConsoleOutputString(hConsoleOutput,
                                   lpAttribute,
                                   nLength,
                                   sizeof(WORD),
                                   CONSOLE_ATTRIBUTE,
                                   dwReadCoord,
                                   lpNumberOfAttrsRead
                                  );
}

#endif //!defined(BUILD_WOW64)

#if !defined(BUILD_WOW6432)

BOOL
APIENTRY
WriteConsoleOutputString(
    IN HANDLE hConsoleOutput,
    IN CONST VOID *lpString,
    IN DWORD nLength,
    IN DWORD nSize,
    IN DWORD fFlags,
    IN COORD dwWriteCoord,
    OUT LPDWORD lpNumberOfElementsWritten
    )

/*++

Parameters:

    hConsoleOutput - Supplies an open handle to the screen buffer (CONOUT$)
        that is to be written.  The handle must have been created with
        GENERIC_WRITE access.

    lpString - Supplies the address of a buffer containing the character
        or attribute string to write to the screen buffer.

    nLength - Length of string to write, in elements.

    nSize - Size of element to read.

    fFlags - flag indicating what type of string to copy

    dwWriteCoord - Screen buffer coordinates to write the string to.

    lpNumberOfElementsWritten - Pointer to number of elements written.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed.
        Extended error status is available using GetLastError.

--*/

{

    PCSR_CAPTURE_HEADER CaptureBuffer;
    CONSOLE_API_MSG m;
    PCONSOLE_WRITECONSOLEOUTPUTSTRING_MSG a = &m.u.WriteConsoleOutputString;
    ULONG DataLength;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->OutputHandle = hConsoleOutput;
    a->NumRecords = nLength;
    a->StringType = fFlags;
    a->WriteCoord = dwWriteCoord;

    //
    // for speed in the case where the string is small, we have a buffer
    // in the msg we pass to the server. this means we don't have to
    // allocate a capture buffer.
    //

    DataLength = nLength*nSize;
    if (DataLength > sizeof(a->String)) {
        CaptureBuffer = CsrAllocateCaptureBuffer( 1,
                                                  DataLength
                                                );
        if (CaptureBuffer == NULL) {
            SET_LAST_ERROR(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
        CsrCaptureMessageBuffer( CaptureBuffer,
                                 (PCHAR) lpString,
                                 DataLength,
                                 (PVOID *) &a->BufPtr
                               );
    }
    else {
        a->BufPtr = a->String;
        CaptureBuffer = NULL;

        try {
            RtlCopyMemory(a->BufPtr, lpString, DataLength);
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            SET_LAST_ERROR(ERROR_INVALID_ACCESS);
            return FALSE;
        }
    }
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepWriteConsoleOutputString
                                            ),
                         sizeof( *a )
                       );
    if (CaptureBuffer != NULL) {
        CsrFreeCaptureBuffer( CaptureBuffer );
    }
    try {
        *lpNumberOfElementsWritten = a->NumRecords;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        SET_LAST_ERROR(ERROR_INVALID_ACCESS);
        return FALSE;
    }
    if (!NT_SUCCESS(m.ReturnValue)) {
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
APIENTRY
WriteConsoleOutputCharacterA(
    HANDLE hConsoleOutput,
    LPCSTR lpCharacter,
    DWORD nLength,
    COORD dwWriteCoord,
    LPDWORD lpNumberOfCharsWritten
    )

/*++

Parameters:

    hConsoleOutput - Supplies an open handle to the screen buffer (CONOUT$)
        that is to be written.  The handle must have been created with
        GENERIC_WRITE access.

    lpCharacter - Supplies the address of a buffer containing the character
        string to write to the screen buffer.

    nLength - Length of string to write, in characters.

    dwWriteCoord - Screen buffer coordinates to write the string to.

    lpNumberOfCharsWritten - Pointer to number of chars written.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed.
        Extended error status is available using GetLastError.

--*/

{
    return WriteConsoleOutputString(hConsoleOutput,
                                    lpCharacter,
                                    nLength,
                                    sizeof(CHAR),
                                    CONSOLE_ASCII,
                                    dwWriteCoord,
                                    lpNumberOfCharsWritten
                                   );
}

BOOL
APIENTRY
WriteConsoleOutputCharacterW(
    HANDLE hConsoleOutput,
    LPCWSTR lpCharacter,
    DWORD nLength,
    COORD dwWriteCoord,
    LPDWORD lpNumberOfCharsWritten
    )

{
    return WriteConsoleOutputString(hConsoleOutput,
                                    lpCharacter,
                                    nLength,
                                    sizeof(WCHAR),
                                    CONSOLE_REAL_UNICODE,
                                    dwWriteCoord,
                                    lpNumberOfCharsWritten
                                   );
}

BOOL
APIENTRY
WriteConsoleOutputAttribute(
    HANDLE hConsoleOutput,
    CONST WORD *lpAttribute,
    DWORD nLength,
    COORD dwWriteCoord,
    LPDWORD lpNumberOfAttrsWritten
    )

/*++

Parameters:

    hConsoleOutput - Supplies an open handle to the screen buffer (CONOUT$)
        that is to be written.  The handle must have been created with
        GENERIC_WRITE access.

    lpAttribute - Supplies the address of a buffer containing the attribute
        string to write to the screen buffer.

    nLength - Length of string to write.

    dwWriteCoord - Screen buffer coordinates to write the string to.

    lpNumberOfAttrsWritten - Pointer to number of attrs written.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed.
        Extended error status is available using GetLastError.

--*/

{
    return WriteConsoleOutputString(hConsoleOutput,
                                    lpAttribute,
                                    nLength,
                                    sizeof(WORD),
                                    CONSOLE_ATTRIBUTE,
                                    dwWriteCoord,
                                    lpNumberOfAttrsWritten
                                   );
}

#endif //!defined(BUILD_WOW64)

#if !defined(BUILD_WOW6432)

BOOL
APIENTRY
FillConsoleOutput(
    IN HANDLE hConsoleOutput,
    IN WORD   Element,
    IN DWORD  nLength,
    IN DWORD  fFlags,
    IN COORD  dwWriteCoord,
    OUT LPDWORD lpNumberOfElementsWritten
    )

/*++

Parameters:

    hConsoleOutput - Supplies an open handle to the screen buffer (CONOUT$)
        that is to be written.  The handle must have been created with
        GENERIC_WRITE access.

    Element - The attribute or character to write.

    nLength - Number of times to write the element.

    fFlags - flag indicating what type of element to write.

    dwWriteCoord - Screen buffer coordinates to write the element to.

    lpNumberOfElementsWritten - Pointer to number of elements written.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed.
        Extended error status is available using GetLastError.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_FILLCONSOLEOUTPUT_MSG a = &m.u.FillConsoleOutput;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->OutputHandle = hConsoleOutput;
    a->Length = nLength;
    a->ElementType = fFlags;
    a->Element = Element;
    a->WriteCoord = dwWriteCoord;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepFillConsoleOutput
                                            ),
                         sizeof( *a )
                       );
    try {
        *lpNumberOfElementsWritten = a->Length;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        SET_LAST_ERROR(ERROR_INVALID_ACCESS);
        return FALSE;
    }
    if (!NT_SUCCESS(m.ReturnValue)) {
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
APIENTRY
FillConsoleOutputCharacterA(
    HANDLE hConsoleOutput,
    CHAR   cCharacter,
    DWORD  nLength,
    COORD  dwWriteCoord,
    LPDWORD lpNumberOfCharsWritten
    )

/*++

Parameters:

    hConsoleOutput - Supplies an open handle to the screen buffer (CONOUT$)
        that is to be written.  The handle must have been created with
        GENERIC_WRITE access.

    cCharacter - Supplies the ASCII character to write to the screen buffer.

    nLength - Number of times to write the character.

    dwWriteCoord - Screen buffer coordinates to begin writing the character
        to.

    lpNumberOfCharsWritten - Pointer to number of chars written.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed.
        Extended error status is available using GetLastError.

--*/

{
    return FillConsoleOutput(hConsoleOutput,
                             (USHORT) cCharacter,
                             nLength,
                             CONSOLE_ASCII,
                             dwWriteCoord,
                             lpNumberOfCharsWritten
                            );
}

BOOL
APIENTRY
FillConsoleOutputCharacterW(
    HANDLE hConsoleOutput,
    WCHAR   cCharacter,
    DWORD  nLength,
    COORD  dwWriteCoord,
    LPDWORD lpNumberOfCharsWritten
    )

/*++

Parameters:

    hConsoleOutput - Supplies an open handle to the screen buffer (CONOUT$)
        that is to be written.  The handle must have been created with
        GENERIC_WRITE access.

    cCharacter - Supplies the ASCII character to write to the screen buffer.

    nLength - Number of times to write the character.

    dwWriteCoord - Screen buffer coordinates to begin writing the character
        to.

    lpNumberOfCharsWritten - Pointer to number of chars written.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed.
        Extended error status is available using GetLastError.

--*/

{
    return FillConsoleOutput(hConsoleOutput,
                             (USHORT) cCharacter,
                             nLength,
                             CONSOLE_REAL_UNICODE,
                             dwWriteCoord,
                             lpNumberOfCharsWritten
                            );
}

BOOL
APIENTRY
FillConsoleOutputAttribute(
    HANDLE hConsoleOutput,
    WORD   wAttribute,
    DWORD  nLength,
    COORD  dwWriteCoord,
    LPDWORD lpNumberOfAttrsWritten
    )

/*++

Parameters:

    hConsoleOutput - Supplies an open handle to the screen buffer (CONOUT$)
        that is to be written.  The handle must have been created with
        GENERIC_WRITE access.

    wAttribute - Supplies the attribute to write to the screen buffer.

    nLength - Number of times to write the attribute.

    dwWriteCoord - Screen buffer coordinates to begin writing the attribute
        to.

    lpNumberOfAttrsWritten - Pointer to number of attrs written.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed.
        Extended error status is available using GetLastError.

--*/

{
    return FillConsoleOutput(hConsoleOutput,
                             wAttribute,
                             nLength,
                             CONSOLE_ATTRIBUTE,
                             dwWriteCoord,
                             lpNumberOfAttrsWritten
                            );
}

#endif //!defined(BUILD_WOW64)

#if !defined(BUILD_WOW6432)

HANDLE
WINAPI
CreateConsoleScreenBuffer(
    IN DWORD dwDesiredAccess,
    IN DWORD dwShareMode,
    IN CONST SECURITY_ATTRIBUTES *lpSecurityAttributes,
    IN DWORD dwFlags,
    IN PVOID lpScreenBufferData OPTIONAL
    )
{
    CONSOLE_API_MSG m;
    PCONSOLE_CREATESCREENBUFFER_MSG a = &m.u.CreateConsoleScreenBuffer;
    PCONSOLE_GRAPHICS_BUFFER_INFO GraphicsBufferInfo;
    PCSR_CAPTURE_HEADER CaptureBuffer=NULL;

    if (dwDesiredAccess & ~VALID_ACCESSES ||
        dwShareMode & ~VALID_SHARE_ACCESSES ||
        (dwFlags != CONSOLE_TEXTMODE_BUFFER &&
         dwFlags != CONSOLE_GRAPHICS_BUFFER)) {
        SET_LAST_ERROR(ERROR_INVALID_PARAMETER);
        return (HANDLE) INVALID_HANDLE_VALUE;
    }
    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->DesiredAccess = dwDesiredAccess;
    if (ARGUMENT_PRESENT(lpSecurityAttributes)) {
        a->InheritHandle = lpSecurityAttributes->bInheritHandle;
    }
    else {
        a->InheritHandle = FALSE;
    }
    a->ShareMode = dwShareMode;
    a->Flags = dwFlags;
    if (dwFlags == CONSOLE_GRAPHICS_BUFFER) {
        if (a->InheritHandle || lpScreenBufferData == NULL) {
            SET_LAST_ERROR(ERROR_INVALID_PARAMETER);
            return (HANDLE) INVALID_HANDLE_VALUE;
        }
        GraphicsBufferInfo = lpScreenBufferData;
        try {
            a->GraphicsBufferInfo = *GraphicsBufferInfo;
            CaptureBuffer = CsrAllocateCaptureBuffer( 1,
                                                      a->GraphicsBufferInfo.dwBitMapInfoLength
                                                    );
            if (CaptureBuffer == NULL) {
                SET_LAST_ERROR(ERROR_NOT_ENOUGH_MEMORY);
                return (HANDLE) INVALID_HANDLE_VALUE;
            }
            CsrCaptureMessageBuffer( CaptureBuffer,
                                     (PCHAR) GraphicsBufferInfo->lpBitMapInfo,
                                     a->GraphicsBufferInfo.dwBitMapInfoLength,
                                     (PVOID *) &a->GraphicsBufferInfo.lpBitMapInfo
                                   );

        } except( EXCEPTION_EXECUTE_HANDLER ) {
            SET_LAST_ERROR(ERROR_INVALID_ACCESS);
            return (HANDLE) INVALID_HANDLE_VALUE;
        }
    }
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepCreateScreenBuffer
                                            ),
                         sizeof( *a )
                       );
    if (CaptureBuffer != NULL) {
        CsrFreeCaptureBuffer( CaptureBuffer );
    }
    if (!NT_SUCCESS( m.ReturnValue)) {
        SET_LAST_NT_ERROR(m.ReturnValue);
        return (HANDLE) INVALID_HANDLE_VALUE;
    }
    else {
        if (dwFlags == CONSOLE_GRAPHICS_BUFFER) {
            try {
                GraphicsBufferInfo->hMutex = a->hMutex;
                GraphicsBufferInfo->lpBitMap = a->lpBitmap;
            } except( EXCEPTION_EXECUTE_HANDLER ) {
                SET_LAST_ERROR(ERROR_INVALID_ACCESS);
                return (HANDLE) INVALID_HANDLE_VALUE;
            }
        }
        return a->Handle;
    }
}

BOOL
WINAPI
InvalidateConsoleDIBits(
    IN HANDLE hConsoleOutput,
    IN PSMALL_RECT lpRect
    )

/*++

Parameters:

    hConsoleHandle - Supplies a console input or output handle.

    lpRect - the region that needs to be updated to the screen.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.


--*/

{
    CONSOLE_API_MSG m;
    PCONSOLE_INVALIDATERECT_MSG a = &m.u.InvalidateConsoleBitmapRect;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->OutputHandle = hConsoleOutput;
    try {
        a->Rect = *lpRect;
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        SET_LAST_ERROR(ERROR_INVALID_ACCESS);
        return ERROR_INVALID_ACCESS;
    }
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepInvalidateBitmapRect
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
