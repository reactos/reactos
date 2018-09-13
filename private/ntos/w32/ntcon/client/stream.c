/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    stream.c

Abstract:

    This module contains the stubs for the console stream API.

Author:

    Therese Stowell (thereses) 3-Dec-1990

Revision History:

    
    
--*/

#include "precomp.h"
#pragma hdrstop
#pragma hdrstop

#if !defined(BUILD_WOW64)

HANDLE InputWaitHandle = (HANDLE)-1;

HANDLE
APIENTRY
GetConsoleInputWaitHandle( VOID )
{
    return InputWaitHandle;
}

#endif //!defined(BUILD_WOW64)

#if !defined(BUILD_WOW6432)

HANDLE
APIENTRY
OpenConsoleWInternal(
    IN ULONG HandleType,
    IN ULONG DesiredAccess,
    IN BOOL InheritHandle,
    IN ULONG ShareMode
    )
/*++

Routine Description:

   Marshels parameters for the ConsolepOpenConsole command.

Arguments:

   See the CONSOLE_OPENCONSOLE_MSG structure and OpenConsoleW.

Return Value:

    INVALID_HANDLE_VALUE - An error occured.

--*/
{

    CONSOLE_API_MSG m;
    PCONSOLE_OPENCONSOLE_MSG a = &m.u.OpenConsole;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->HandleType    = HandleType;
    a->DesiredAccess = DesiredAccess;
    a->InheritHandle = InheritHandle;
    a->ShareMode= ShareMode;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepOpenConsole
                                            ),
                         sizeof( *a )
                       );
    if (!NT_SUCCESS( m.ReturnValue)) {
        SET_LAST_NT_ERROR(m.ReturnValue);
        return (HANDLE) INVALID_HANDLE_VALUE;
    }
    else {
        return a->Handle;
    }

}

#endif // !defined(BUILD_WOW6432)

#if !defined(BUILD_WOW64)

HANDLE
APIENTRY
OpenConsoleW(
    IN LPWSTR lpConsoleDevice,
    IN DWORD dwDesiredAccess,
    IN BOOL bInheritHandle,
    IN DWORD dwShareMode
    )

/*++

Parameters:

    lpConsoleDevice - Supplies the console device name to open.  "CONIN$"
        indicates console input.  "CONOUT$" indicates console output.  The
        caller must have appropriate access to the console for this call to
        succeed.

    dwDesiredAccess - Supplies the caller's desired access to the console
        device.

        DesiredAccess Flags:

        GENERIC_READ - Read access to the console device is requested.  This
            allows data to be read from the console device.

        GENERIC_WRITE - Write access to the console device is requested.  This
            allows data to be written to the console device.

    bInheritHandle - Supplies a flag that indicates whether or not the
        returned handle is to be inherited by a new process
        during a CreateProcess.  A value of TRUE indicates that the
        new process will inherit the handle.

    dwShareMode - Supplies a set of flags that indicates how this console
        device is to be shared with other openers of the console device.  A
        value of zero for this parameter indicates no sharing of the console,
        or exclusive access to the console is to occur.

        ShareMode Flags:

        FILE_SHARE_READ - Other open operations may be performed on the
            console device for read access.

        FILE_SHARE_WRITE - Other open operations may be performed on the
            console device for write access.

Return Value:

    Not -1 - Returns an open handle to the specified console device.
        Subsequent access to the file is controlled by the DesiredAccess
        parameter.

    0xffffffff - The operation failed. Extended error status is available
        using GetLastError.
--*/

{
    ULONG HandleType;

    try {
        if (!lstrcmpiW(CONSOLE_INPUT_STRING,lpConsoleDevice)) {
            HandleType = CONSOLE_INPUT_HANDLE;
        }
        else if (!lstrcmpiW(CONSOLE_OUTPUT_STRING,lpConsoleDevice)) {
            HandleType = CONSOLE_OUTPUT_HANDLE;
        }
        else {
            SET_LAST_ERROR(ERROR_INVALID_PARAMETER);
            return (HANDLE) INVALID_HANDLE_VALUE;
        }
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        SET_LAST_ERROR(ERROR_INVALID_ACCESS);
        return (HANDLE) INVALID_HANDLE_VALUE;
    }
    if (dwDesiredAccess & ~VALID_ACCESSES ||
        dwShareMode & ~VALID_SHARE_ACCESSES) {
        SET_LAST_ERROR(ERROR_INVALID_PARAMETER);
        return (HANDLE) INVALID_HANDLE_VALUE;
    }

    return OpenConsoleWInternal(HandleType,
                                dwDesiredAccess,
                                bInheritHandle,
                                dwShareMode
                                );

}

#endif // !defined(BUILD_WOW64)

#if !defined(BUILD_WOW6432)

BOOL
APIENTRY
ReadConsoleInternal(
    IN HANDLE hConsoleInput,
    OUT LPVOID lpBuffer,
    IN DWORD nNumberOfCharsToRead,
    OUT LPDWORD lpNumberOfCharsRead,
    IN OUT LPVOID lpReserved,
    IN BOOLEAN Unicode,
    IN USHORT ExeNameLength,
    IN LPWSTR ExeName
    )
/*++
Parameters:

    hConsoleInput - Supplies an open handle to "CONIN$" open for GENERIC_READ
        or the StdIn handle.

    lpBuffer - Supplies the address of a buffer to receive the data read
        from the console input.

    nNumberOfBytesToRead - Supplies the number of bytes to read from the
        input buffer.

    lpReserved - Ignore unless 4.0 application, in which case it points
        to a CONSOLE_READCONSOLE_CONTROL data structure.  UNICODE only.
        If !Unicode, then call fails if this parameter is non-NULL

    Unicode - TRUE if call from ReadConsoleW, FALSE if ReadConsoleA


Return Value:

    NON-NULL - Returns the number of bytes actually read from the input buffer.

    FALSE/NULL - The operation failed.
        Extended error status is available using GetLastError.
--*/

{
    PCSR_CAPTURE_HEADER CaptureBuffer;
    CONSOLE_API_MSG m;
    PCONSOLE_READCONSOLE_MSG a = &m.u.ReadConsole;
    BOOLEAN Dummy;
    PCONSOLE_READCONSOLE_CONTROL pInputControl;
    NTSTATUS Status;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->InputHandle = hConsoleInput;
    a->ExeNameLength = ExeNameLength;
    RtlCopyMemory(a->Buffer, ExeName, ExeNameLength);
    a->Unicode = Unicode;

    //
    // if ansi, make capture buffer large enough to hold translated
    // string.  this will make server side code much simpler.
    //

    a->CaptureBufferSize = a->NumBytes = nNumberOfCharsToRead * sizeof(WCHAR);
    if (a->CaptureBufferSize > BUFFER_SIZE) {
        CaptureBuffer = CsrAllocateCaptureBuffer( 1,
                                                  a->CaptureBufferSize
                                                );
        if (CaptureBuffer == NULL) {
            SET_LAST_ERROR(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
        CsrCaptureMessageBuffer( CaptureBuffer,
                                 NULL,
                                 a->CaptureBufferSize,
                                 (PVOID *) &a->BufPtr
                               );

    }
    else {
        a->BufPtr = a->Buffer;
        CaptureBuffer = NULL;
    }


    pInputControl = (PCONSOLE_READCONSOLE_CONTROL)lpReserved;
    a->InitialNumBytes = 0;
    a->CtrlWakeupMask = 0;
    a->ControlKeyState = 0;
    Status = STATUS_SUCCESS;
    try {
        if (Unicode &&
            ARGUMENT_PRESENT(lpReserved) &&
            NtCurrentPeb()->ImageSubsystemMajorVersion >= 4 &&
            pInputControl->nLength == sizeof(*pInputControl)
           ) {
            if ((pInputControl->nInitialChars > nNumberOfCharsToRead)) {
                Status = STATUS_INVALID_PARAMETER;
            } else {
                a->InitialNumBytes = pInputControl->nInitialChars * sizeof(WCHAR);
                if (pInputControl->nInitialChars != 0) {
                    RtlCopyMemory( a->BufPtr, lpBuffer, a->InitialNumBytes );
                }
                a->CtrlWakeupMask = pInputControl->dwCtrlWakeupMask;
            }
        } else {
            pInputControl = NULL;
        }
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        Status = GetExceptionCode();
    }

    if (!NT_SUCCESS(Status) && pInputControl != NULL) {
        if (CaptureBuffer != NULL) {
            CsrFreeCaptureBuffer( CaptureBuffer );
        }
        SET_LAST_NT_ERROR(Status);
        return FALSE;
    }

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepReadConsole
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        try {
            *lpNumberOfCharsRead = a->NumBytes;
            if (Unicode) {
                *lpNumberOfCharsRead /= sizeof(WCHAR);
                if (pInputControl != NULL) {
                    pInputControl->dwControlKeyState = a->ControlKeyState;
                }
            }
            RtlCopyMemory( lpBuffer, a->BufPtr, a->NumBytes );
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
    if (!NT_SUCCESS( m.ReturnValue )) {
        SET_LAST_NT_ERROR(m.ReturnValue);
        return FALSE;
    } else if (m.ReturnValue == STATUS_ALERTED) {
        // ctrl-c or ctrl-break
        NtYieldExecution();
        SET_LAST_ERROR(ERROR_OPERATION_ABORTED);
    }
    return TRUE;
}

#endif //!defined(BUILD_WOW6432)

#if !defined(BUILD_WOW64)

BOOL
APIENTRY
ReadConsoleA(
    IN HANDLE hConsoleInput,
    OUT LPVOID lpBuffer,
    IN DWORD nNumberOfCharsToRead,
    OUT LPDWORD lpNumberOfCharsRead,
    IN OUT LPVOID lpReserved
    )
{

    WCHAR ExeName[BUFFER_SIZE/2];
    USHORT ExeNameLength;

    ExeNameLength = GetCurrentExeName(ExeName, sizeof(ExeName));

    return ReadConsoleInternal(hConsoleInput,
                               lpBuffer,
                               nNumberOfCharsToRead,
                               lpNumberOfCharsRead,
                               NULL,
                               FALSE,
                               ExeNameLength,
                               ExeName
                              );
    UNREFERENCED_PARAMETER(lpReserved);
}

BOOL
APIENTRY
ReadConsoleW(
    IN HANDLE hConsoleInput,
    OUT LPVOID lpBuffer,
    IN DWORD nNumberOfCharsToRead,
    OUT LPDWORD lpNumberOfCharsRead,
    IN OUT LPVOID lpReserved
    )
{
    WCHAR ExeName[BUFFER_SIZE/2];
    USHORT ExeNameLength;

    ExeNameLength = GetCurrentExeName(ExeName, sizeof(ExeName));

    return ReadConsoleInternal(hConsoleInput,
                               lpBuffer,
                               nNumberOfCharsToRead,
                               lpNumberOfCharsRead,
                               lpReserved,
                               TRUE,
                               ExeNameLength,
                               ExeName
                              );
    UNREFERENCED_PARAMETER(lpReserved);
}

#endif //!defined(BUILD_WOW64)

#if !defined(BUILD_WOW6432)

BOOL
APIENTRY
WriteConsoleInternal(
    IN HANDLE hConsoleOutput,
    IN CONST VOID *lpBuffer,
    IN DWORD nNumberOfCharsToWrite,
    OUT LPDWORD lpNumberOfCharsWritten,
    IN BOOLEAN Unicode
    )

/*++
Parameters:

    hFile - Supplies an open handle to to "CONOUT$" open for GENERIC_WRITE
        or the StdOut or StdErr handle.

    lpBuffer - Supplies the address of the data that is to be written to
        the console output.

    nNumberOfBytesToWrite - Supplies the number of bytes to write to the
        console output.

Return Value:

    NON-NULL - Returns the number of bytes actually written to the device.

    FALSE/NULL - The operation failed.
        Extended error status is available using GetLastError.
--*/

{

    PCSR_CAPTURE_HEADER CaptureBuffer;
    CONSOLE_API_MSG m;
    PCONSOLE_WRITECONSOLE_MSG a = &m.u.WriteConsole;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->OutputHandle = hConsoleOutput;

    if (Unicode) {
        a->NumBytes = nNumberOfCharsToWrite * sizeof(WCHAR);
    } else {
        a->NumBytes = nNumberOfCharsToWrite;
    }

    a->Unicode = Unicode;
    if (a->NumBytes > BUFFER_SIZE) {
        CaptureBuffer = CsrAllocateCaptureBuffer( 1,
                                                  a->NumBytes
                                                );
        if (CaptureBuffer == NULL) {
            SET_LAST_ERROR(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
        CsrCaptureMessageBuffer( CaptureBuffer,
                                 (PCHAR) lpBuffer,
                                 a->NumBytes,
                                 (PVOID *) &a->BufPtr
                               );
        a->BufferInMessage = FALSE;
    }
    else {
        a->BufPtr = a->Buffer;
        try {
            RtlCopyMemory( a->BufPtr, lpBuffer, a->NumBytes);
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            SET_LAST_ERROR(ERROR_INVALID_ACCESS);
            return FALSE;
        }
        CaptureBuffer = NULL;
        a->BufferInMessage = TRUE;
    }
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepWriteConsole
                                            ),
                         sizeof( *a )
                       );
    if (CaptureBuffer != NULL) {
        CsrFreeCaptureBuffer( CaptureBuffer );
    }
    if (!NT_SUCCESS( m.ReturnValue )) {
        SET_LAST_NT_ERROR(m.ReturnValue);
        return FALSE;
    }
    try {
       *lpNumberOfCharsWritten = a->NumBytes;
       if (Unicode) {
           *lpNumberOfCharsWritten /= sizeof(WCHAR);
       }
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        SET_LAST_ERROR(ERROR_INVALID_ACCESS);
        return FALSE;
    }
    return TRUE;

}

#endif //!defined(BUILD_WOW6432)

#if !defined(BUILD_WOW64)

BOOL
APIENTRY
WriteConsoleA(
    IN HANDLE hConsoleOutput,
    IN CONST VOID *lpBuffer,
    IN DWORD nNumberOfCharsToWrite,
    OUT LPDWORD lpNumberOfCharsWritten,
    IN OUT LPVOID lpReserved
    )
{
    return WriteConsoleInternal(hConsoleOutput,
                                lpBuffer,
                                nNumberOfCharsToWrite,
                                lpNumberOfCharsWritten,
                                FALSE
                               );
    UNREFERENCED_PARAMETER(lpReserved);
}

BOOL
APIENTRY
WriteConsoleW(
    IN HANDLE hConsoleOutput,
    IN CONST VOID *lpBuffer,
    IN DWORD nNumberOfCharsToWrite,
    OUT LPDWORD lpNumberOfCharsWritten,
    IN OUT LPVOID lpReserved
    )
{
    return WriteConsoleInternal(hConsoleOutput,
                                lpBuffer,
                                nNumberOfCharsToWrite,
                                lpNumberOfCharsWritten,
                                TRUE
                               );
    UNREFERENCED_PARAMETER(lpReserved);
}

#endif //!defined(BUILD_WOW64)

#if !defined(BUILD_WOW6432)

BOOL
APIENTRY
CloseConsoleHandle(
    IN HANDLE hConsole
    )

/*++

Parameters:

    hConsole - An open handle to console input or output.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_CLOSEHANDLE_MSG a = &m.u.CloseHandle;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->Handle = hConsole;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepCloseHandle
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

HANDLE
APIENTRY
DuplicateConsoleHandle(
    IN HANDLE hSourceHandle,
    IN DWORD dwDesiredAccess,
    IN BOOL bInheritHandle,
    IN DWORD dwOptions
    )
/*++
Parameters:

    hSourceHandle - An open handle to the console device.

    dwDesiredAccess - The access requested to for the new handle.  This
        access must be equal to or a proper subset of the granted access
        associated with the SourceHandle.  This parameter is ignored if
        the DUPLICATE_SAME_ACCESS option is specified.

    bInheritHandle - Supplies a flag that if TRUE, marks the target
        handle as inheritable.  If this is the case, then the target
        handle will be inherited to new processes each time the target
        process creates a new process using CreateProcess.

    dwOptions - Specifies optional behaviors for the caller.

        Options Flags:

        DUPLICATE_CLOSE_SOURCE - The SourceHandle will be closed by
            this service prior to returning to the caller.  This occurs
            regardless of any error status returned.

        DUPLICATE_SAME_ACCESS - The DesiredAccess parameter is ignored
            and instead the GrantedAccess associated with SourceHandle
            is used as the DesiredAccess when creating the TargetHandle.


Return Value:

    Not -1 - Returns an open handle to the specified console device.
        Subsequent access to the file is controlled by the DesiredAccess
        parameter.

    0xffffffff - The operation failed. Extended error status is available
        using GetLastError.
--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_DUPHANDLE_MSG a = &m.u.DuplicateHandle;

    if (dwOptions & ~VALID_DUP_OPTIONS) {
        SET_LAST_ERROR(ERROR_INVALID_PARAMETER);
        return (HANDLE) INVALID_HANDLE_VALUE;
    }
    if (((dwOptions & DUPLICATE_SAME_ACCESS) == 0) &&
         (dwDesiredAccess & ~VALID_ACCESSES)) {
        SET_LAST_ERROR(ERROR_INVALID_PARAMETER);
        return (HANDLE) INVALID_HANDLE_VALUE;
    }

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->SourceHandle = hSourceHandle;
    a->DesiredAccess = dwDesiredAccess;
    a->InheritHandle = (BOOLEAN) bInheritHandle;
    a->Options = dwOptions;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepDupHandle
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        return a->TargetHandle;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return (HANDLE) INVALID_HANDLE_VALUE;
    }

}


BOOL
APIENTRY
GetConsoleHandleInformation(
    IN HANDLE hObject,
    OUT LPDWORD lpdwFlags
    )

/*++
Parameters:

    hObject - An open handle to console input or output.

    lpdwFlags - Receives flags for console object.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_GETHANDLEINFORMATION_MSG a = &m.u.GetHandleInformation;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->Handle = hObject;

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepGetHandleInformation
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        try {
           *lpdwFlags = a->Flags;
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
APIENTRY
SetConsoleHandleInformation(
    IN HANDLE hObject,
    IN DWORD dwMask,
    IN DWORD dwFlags
    )

/*++
Parameters:

    hObject - An open handle to console input or output.

    dwMask - Flags to change.

    dwFlags - New values for flags.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_SETHANDLEINFORMATION_MSG a = &m.u.SetHandleInformation;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->Handle = hObject;
    a->Mask = dwMask;
    a->Flags = dwFlags;

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepSetHandleInformation
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
VerifyConsoleIoHandle(
    IN HANDLE hIoHandle
    )

/*++
Parameters:

    hIoHandle - Handle to verify

Return Value:

    TRUE - handle is a valid console handle.

    FALSE - handle is not a valid console handle.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_VERIFYIOHANDLE_MSG a = &m.u.VerifyConsoleIoHandle;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->Handle = hIoHandle;

    //
    // If this process doesn't have a console handle, bail immediately.
    //

    if (a->ConsoleHandle == NULL) {
        return FALSE;
    }

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepVerifyIoHandle
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        return a->Valid;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }


}

#endif //!defined(BUILD_WOW6432)
