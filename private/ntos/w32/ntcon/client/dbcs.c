/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    dbcs.c

Abstract:

Author:

    KazuM May.11.1992

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#if defined(FE_SB)
#include "conime.h"

#if !defined(BUILD_WOW6432)

BOOL
APIENTRY
GetConsoleNlsMode(
    IN HANDLE hConsoleHandle,
    OUT LPDWORD lpNlsMode
    )

/*++

Parameters:

    hConsoleHandle - Supplies a console input or output handle.

    lpNlsMode - Supplies a pointer to the NLS mode.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.


--*/

{

#if defined(FE_IME)
    CONSOLE_API_MSG m;
    PCONSOLE_NLS_MODE_MSG a = &m.u.GetConsoleNlsMode;
    NTSTATUS Status;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->Handle = hConsoleHandle;
    a->Ready = FALSE;

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

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepGetNlsMode
                                            ),
                         sizeof( *a )
                       );

    if (NT_SUCCESS( m.ReturnValue )) {
	Status = NtWaitForSingleObject(a->hEvent, FALSE, NULL);

        if (a->Ready == FALSE)
        {
            /*
             * If not ready conversion status on this console,
             * then one more try get status.
             */
            CsrClientCallServer( (PCSR_API_MSG)&m,
                                 NULL,
                                 CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                                      ConsolepGetNlsMode
                                                    ),
                                 sizeof( *a )
                               );
            if (! NT_SUCCESS( m.ReturnValue )) {
                SET_LAST_NT_ERROR (m.ReturnValue);
                NtClose(a->hEvent);
                return FALSE;
            }
            else
            {
	        Status = NtWaitForSingleObject(a->hEvent, FALSE, NULL);
            }
        }

	NtClose(a->hEvent);

        try {
            *lpNlsMode = a->NlsMode;
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            SET_LAST_ERROR (ERROR_INVALID_ACCESS);
            return FALSE;
        }
        return TRUE;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        NtClose(a->hEvent);
        return FALSE;
    }
#else
    return FALSE;
#endif

}

BOOL
APIENTRY
SetConsoleNlsMode(
    IN HANDLE hConsoleHandle,
    IN DWORD dwNlsMode
    )

/*++

Parameters:

    hConsoleHandle - Supplies a console input or output handle.

    dwNlsMode - Supplies NLS mode.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

#if defined(FE_IME)
    CONSOLE_API_MSG m;
    PCONSOLE_NLS_MODE_MSG a = &m.u.SetConsoleNlsMode;
    NTSTATUS Status;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->Handle = hConsoleHandle;
    a->NlsMode = dwNlsMode;

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

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepSetNlsMode
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
	Status = NtWaitForSingleObject(a->hEvent, FALSE, NULL);
        NtClose(a->hEvent);	
	if (Status != 0) {
	    SET_LAST_NT_ERROR(Status);
	    return FALSE;
	}
        return TRUE;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        NtClose(a->hEvent);
        return FALSE;
    }
#else
    return FALSE;
#endif

}

BOOL
APIENTRY
GetConsoleCharType(
    IN HANDLE hConsoleHandle,
    IN COORD coordCheck,
    OUT PDWORD pdwType
    )

/*++

Parameters:

    hConsoleHandle - Supplies a console input or output handle.

    coordCheck - set check position to these coordinates

    pdwType - receive character type

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.


--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_CHAR_TYPE_MSG a = &m.u.GetConsoleCharType;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->Handle = hConsoleHandle;
    a->coordCheck = coordCheck;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepCharType
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        try {
            *pdwType = a->dwType;
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
APIENTRY
SetConsoleLocalEUDC(
    IN HANDLE hConsoleHandle,
    IN WORD   wCodePoint,
    IN COORD  cFontSize,
    IN PCHAR  lpSB
    )

/*++

Parameters:

    hConsoleHandle - Supplies a console input or output handle.

    wCodePoint - Code point of font by Shift JIS code.

    cFontSize - FontSize of Font

    lpSB - Pointer of font bitmap Buffer

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_LOCAL_EUDC_MSG a = &m.u.SetConsoleLocalEUDC;
    PCSR_CAPTURE_HEADER CaptureBuffer;
    ULONG DataLength;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->Handle = hConsoleHandle;
    a->CodePoint = wCodePoint;
    a->FontSize = cFontSize;

    DataLength = ((cFontSize.X + 7) / 8) * cFontSize.Y;

    CaptureBuffer = CsrAllocateCaptureBuffer( 1,
                                              DataLength
                                            );
    if (CaptureBuffer == NULL) {
        SET_LAST_ERROR(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    CsrCaptureMessageBuffer( CaptureBuffer,
                             lpSB,
                             DataLength,
                             (PVOID *) &a->FontFace
                           );

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepSetLocalEUDC
                                            ),
                         sizeof( *a )
                       );
        CsrFreeCaptureBuffer( CaptureBuffer );

    if (NT_SUCCESS( m.ReturnValue )) {
        return TRUE;
    } else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }

}

BOOL
APIENTRY
SetConsoleCursorMode(
    IN HANDLE hConsoleHandle,
    IN BOOL   Blink,
    IN BOOL   DBEnable
    )

/*++

Parameters:

    hConsoleHandle - Supplies a console input or output handle.

    Blink - Blinking enable/disable switch.

    DBEnable - Double Byte width enable/disable switch.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_CURSOR_MODE_MSG a = &m.u.SetConsoleCursorMode;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->Handle = hConsoleHandle;
    a->Blink = Blink;
    a->DBEnable = DBEnable;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepSetCursorMode
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
GetConsoleCursorMode(
    IN HANDLE hConsoleHandle,
    OUT PBOOL  pbBlink,
    OUT PBOOL  pbDBEnable
    )

/*++

Parameters:

    hConsoleHandle - Supplies a console input or output handle.

    Blink - Blinking enable/disable switch.

    DBEnable - Double Byte width enable/disable switch.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{

    CONSOLE_API_MSG m;
    PCONSOLE_CURSOR_MODE_MSG a = &m.u.GetConsoleCursorMode;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->Handle = hConsoleHandle;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepGetCursorMode
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        try {
            *pbBlink = a->Blink;
            *pbDBEnable = a->DBEnable;
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
APIENTRY
RegisterConsoleOS2(
    IN BOOL fOs2Register
    )

/*++

Description:

    This routine registers the OS/2 with the console.

Parameters:

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    CONSOLE_API_MSG m;
    PCONSOLE_REGISTEROS2_MSG a = &m.u.RegisterConsoleOS2;
    NTSTATUS Status;


    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->fOs2Register  = fOs2Register;

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepRegisterOS2
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
SetConsoleOS2OemFormat(
    IN BOOL fOs2OemFormat
    )

/*++

Description:

    This routine sets the OS/2 OEM Format with the console.

Parameters:

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    CONSOLE_API_MSG m;
    PCONSOLE_SETOS2OEMFORMAT_MSG a = &m.u.SetConsoleOS2OemFormat;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->fOs2OemFormat = fOs2OemFormat;
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepSetOS2OemFormat
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

#if defined(FE_IME)

#if !defined(BUILD_WOW6432)

BOOL
APIENTRY
RegisterConsoleIMEInternal(
    IN HWND hWndConsoleIME,
    IN DWORD dwConsoleIMEThreadId,
    IN DWORD DesktopLength,
    IN LPWSTR Desktop,
    OUT DWORD *dwConsoleThreadId
    )
{

   CONSOLE_API_MSG m;
   PCONSOLE_REGISTER_CONSOLEIME_MSG a = &m.u.RegisterConsoleIME;
   PCSR_CAPTURE_HEADER CaptureBuffer = NULL;

   a->ConsoleHandle        = GET_CONSOLE_HANDLE;
   a->hWndConsoleIME       = hWndConsoleIME;
   a->dwConsoleIMEThreadId = dwConsoleIMEThreadId;
   a->DesktopLength        = DesktopLength;

   CaptureBuffer = CsrAllocateCaptureBuffer( 1,
                                             DesktopLength
                                            );
   if (CaptureBuffer == NULL) {
       SET_LAST_ERROR(ERROR_NOT_ENOUGH_MEMORY);
       return FALSE;
   }

   CsrCaptureMessageBuffer( CaptureBuffer,
                            Desktop,
                            a->DesktopLength,
                            (PVOID *) &a->Desktop
                          );

   //
   // Connect to the server process
   //

   CsrClientCallServer( (PCSR_API_MSG)&m,
                        CaptureBuffer,
                        CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                             ConsolepRegisterConsoleIME
                                           ),
                        sizeof( *a )
                      );

//HS Jan.20    if (CaptureBuffer) {
       CsrFreeCaptureBuffer( CaptureBuffer );
//HS Jan.20    }

   if (!NT_SUCCESS( m.ReturnValue)) {
       SET_LAST_NT_ERROR(m.ReturnValue);
       return FALSE;
   }
   else {
       try {
           if (dwConsoleThreadId != NULL)
               *dwConsoleThreadId = a->dwConsoleThreadId;
       } except( EXCEPTION_EXECUTE_HANDLER ) {
           SET_LAST_ERROR (ERROR_INVALID_ACCESS);
           return FALSE;
       }
       return TRUE;
    }

}

#endif //!defined(BUILD_WOW6432)

#if !defined(BUILD_WOW64)

BOOL
APIENTRY
RegisterConsoleIME(
    IN HWND  hWndConsoleIME,
    OUT DWORD *dwConsoleThreadId
    )

/*++

Description:

    This routine register the Console IME on the current desktop.

Parameters:

Return Value:

    TRUE - The operation was successful.

    FALSE - The operation failed.

--*/

{
    STARTUPINFOW StartupInfo;
    DWORD dwDesktopLength;
    GetStartupInfoW(&StartupInfo);

    if (StartupInfo.lpDesktop != NULL && *StartupInfo.lpDesktop != 0) {
        dwDesktopLength = (USHORT)((wcslen(StartupInfo.lpDesktop)+1)*sizeof(WCHAR));
        dwDesktopLength = (USHORT)(min(dwDesktopLength,MAX_TITLE_LENGTH));
    } else {
        dwDesktopLength = 0;
    }

    return RegisterConsoleIMEInternal(hWndConsoleIME,
                                      GetCurrentThreadId(),
                                      dwDesktopLength,
                                      StartupInfo.lpDesktop,
                                      dwConsoleThreadId);
}

#endif //!defined(BUILD_WOW64)

#if !defined(BUILD_WOW6432)

BOOL
APIENTRY
UnregisterConsoleIMEInternal(
    IN DWORD dwConsoleIMEThtreadId
    )
{
    CONSOLE_API_MSG m;
    PCONSOLE_UNREGISTER_CONSOLEIME_MSG a = &m.u.UnregisterConsoleIME;

    a->ConsoleHandle        = GET_CONSOLE_HANDLE;
    a->dwConsoleIMEThreadId = dwConsoleIMEThtreadId;

    //
    // Connect to the server process
    //

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepUnregisterConsoleIME
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
APIENTRY
UnregisterConsoleIME(
    )

/*++

Description:

    This routine unregister the Console IME on the current desktop.

Parameters:

Return Value:

    TRUE - The operation was successful.

    FALSE - The operation failed.

--*/

{

    return UnregisterConsoleIMEInternal(GetCurrentThreadId());

}

NTSTATUS
MyRegOpenKey(
    IN HANDLE hKey,
    IN LPWSTR lpSubKey,
    OUT PHANDLE phResult
    )
{
    OBJECT_ATTRIBUTES   Obja;
    UNICODE_STRING      SubKey;

    //
    // Convert the subkey to a counted Unicode string.
    //

    RtlInitUnicodeString( &SubKey, lpSubKey );

    //
    // Initialize the OBJECT_ATTRIBUTES structure and open the key.
    //

    InitializeObjectAttributes(
        &Obja,
        &SubKey,
        OBJ_CASE_INSENSITIVE,
        hKey,
        NULL
        );

    return NtOpenKey(
              phResult,
              KEY_READ,
              &Obja
              );
}

NTSTATUS
MyRegQueryValue(
    IN HANDLE hKey,
    IN LPWSTR lpValueName,
    IN DWORD dwValueLength,
    OUT LPBYTE lpData
    )
{
    UNICODE_STRING ValueName;
    ULONG BufferLength;
    ULONG ResultLength;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation;
    NTSTATUS Status;

    //
    // Convert the subkey to a counted Unicode string.
    //

    RtlInitUnicodeString( &ValueName, lpValueName );

    BufferLength = sizeof(KEY_VALUE_FULL_INFORMATION) + dwValueLength + ValueName.Length;;
    KeyValueInformation = LocalAlloc(LPTR,BufferLength);
    if (KeyValueInformation == NULL)
        return STATUS_NO_MEMORY;

    Status = NtQueryValueKey(
                hKey,
                &ValueName,
                KeyValueFullInformation,
                KeyValueInformation,
                BufferLength,
                &ResultLength
                );
    if (NT_SUCCESS(Status)) {
        ASSERT(KeyValueInformation->DataLength <= dwValueLength);
        RtlMoveMemory(lpData,
            (PBYTE)KeyValueInformation + KeyValueInformation->DataOffset,
            KeyValueInformation->DataLength);
        if (KeyValueInformation->Type == REG_SZ) {
            if (KeyValueInformation->DataLength + sizeof(WCHAR) > dwValueLength) {
                KeyValueInformation->DataLength -= sizeof(WCHAR);
            }
            lpData[KeyValueInformation->DataLength++] = 0;
            lpData[KeyValueInformation->DataLength] = 0;
        }
    }
    LocalFree(KeyValueInformation);
    return Status;
}

VOID
GetCommandLineString(
    IN LPWSTR CommandLine,
    IN DWORD  dwSize
    )
{
    NTSTATUS Status;
    HANDLE hkRegistry;
    WCHAR awchBuffer[ 512 ];
    DWORD dwRet;

    dwRet = GetSystemDirectoryW(CommandLine, dwSize);
    if (dwRet)
    {
        CommandLine[dwRet++] = L'\\';
        CommandLine[dwRet]   = L'\0';
        dwSize -= dwRet;

    }
    else
    {
        CommandLine[0] = L'\0';
    }

    Status = MyRegOpenKey(NULL,
                          MACHINE_REGISTRY_CONSOLE,
                          &hkRegistry);
    if (NT_SUCCESS( Status ))
    {
        Status = MyRegQueryValue(hkRegistry,
                                 MACHINE_REGISTRY_CONSOLEIME,
                                 sizeof(awchBuffer), (PBYTE)&awchBuffer);
        if (NT_SUCCESS( Status ))
        {
            dwRet = wcslen(awchBuffer);
            if (dwRet < dwSize)
            {
                wcscat(CommandLine, awchBuffer);
            }
            else
            {
                CommandLine[0] = L'\0';
                goto ErrorExit;
            }
        }
        else
        {
            goto ErrorExit;
        }

        NtClose(hkRegistry);
    }
    else
    {
        goto ErrorExit;
    }

    return;

ErrorExit:
    wcscat(CommandLine, L"conime.exe");
    return;
}


DWORD
ConsoleIMERoutine(
    IN LPVOID lpThreadParameter
    )

/*++

Routine Description:

    This thread is created when the create input thread.
    It invokes the console IME process.

Arguments:

    lpThreadParameter - not use.

Return Value:

    STATUS_SUCCESS - function was successful

--*/

{
    NTSTATUS Status;
    BOOL fRet;
    static BOOL fInConIMERoutine = FALSE;

    DWORD fdwCreate;
    STARTUPINFOW StartupInfo;
    STARTUPINFOW StartupInfoConsole;
    WCHAR CommandLine[MAX_PATH*2];
    PROCESS_INFORMATION ProcessInformation;
    HANDLE hEvent;
    DWORD dwWait;

    Status = STATUS_SUCCESS;

    //
    // Prevent the user from launching multiple applets attached
    // to a single console
    //

    if (fInConIMERoutine) {
        return (ULONG)STATUS_UNSUCCESSFUL;
    }

    fInConIMERoutine = TRUE;

    //
    // Create event
    //
    hEvent = CreateEventW(NULL,                 // Security attributes
                          FALSE,                // Manual reset
                          FALSE,                // Initial state
                          CONSOLEIME_EVENT);    // Event object name
    if (hEvent == NULL)
    {
        goto ErrorExit;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        goto ErrorExit;
    }

    //
    // Get Console IME process name and event name
    //

    GetCommandLineString(CommandLine, sizeof(CommandLine)/sizeof(WCHAR));

    GetStartupInfoW(&StartupInfoConsole);
    RtlZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.wShowWindow = SW_HIDE;
    StartupInfo.dwFlags = STARTF_FORCEONFEEDBACK;
    StartupInfo.lpDesktop = StartupInfoConsole.lpDesktop;

    //
    // create Console IME process
    //

    fdwCreate = NORMAL_PRIORITY_CLASS | CREATE_DEFAULT_ERROR_MODE | CREATE_NEW_PROCESS_GROUP;
    fRet = CreateProcessW(NULL,                // Application name
                          CommandLine,         // Command line
                          NULL,                // process security attributes
                          NULL,                // thread security attributes
                          FALSE,               // inherit handles
                          fdwCreate,           // create flags
                          NULL,                // environment
                          NULL,                // current directory
                          &StartupInfo,        // Start up information
                          &ProcessInformation  // process information
                         );
    if (! fRet)
    {
        Status = GetLastError();
    }
    else
    {
        dwWait = WaitForSingleObject(hEvent, 10 * 1000);    // wait 10 sec for console IME process
        if (dwWait == WAIT_TIMEOUT)
        {
            TerminateProcess(ProcessInformation.hProcess, 0);
        }
        CloseHandle(ProcessInformation.hThread) ;
        CloseHandle(ProcessInformation.hProcess) ;
    }

    CloseHandle(hEvent);

ErrorExit:

    fInConIMERoutine = FALSE;

    return Status;
}

#endif //!defined(BUILD_WOW6432)

#endif // FE_IME


#else // FE_SB

// Followings are stub functions for FE Console Support


#if !defined(BUILD_WOW6432)

BOOL
APIENTRY
GetConsoleNlsMode(
    IN HANDLE hConsoleHandle,
    OUT LPDWORD lpNlsMode
    )

/*++

Parameters:

    hConsoleHandle - Supplies a console input or output handle.

    lpNlsMode - Supplies a pointer to the NLS mode.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.


--*/

{
    return FALSE;
}

BOOL
APIENTRY
SetConsoleNlsMode(
    IN HANDLE hConsoleHandle,
    IN DWORD dwNlsMode
    )

/*++

Parameters:

    hConsoleHandle - Supplies a console input or output handle.

    dwNlsMode - Supplies NLS mode.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    return FALSE;
}

BOOL
APIENTRY
GetConsoleCharType(
    IN HANDLE hConsoleHandle,
    IN COORD coordCheck,
    OUT PDWORD pdwType
    )

/*++

Parameters:

    hConsoleHandle - Supplies a console input or output handle.

    coordCheck - set check position to these coordinates

    pdwType - receive character type

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.


--*/

{
    return FALSE;
}

BOOL
APIENTRY
SetConsoleLocalEUDC(
    IN HANDLE hConsoleHandle,
    IN WORD   wCodePoint,
    IN COORD  cFontSize,
    IN PCHAR  lpSB
    )

/*++

Parameters:

    hConsoleHandle - Supplies a console input or output handle.

    wCodePoint - Code point of font by Shift JIS code.

    cFontSize - FontSize of Font

    lpSB - Pointer of font bitmap Buffer

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    return FALSE;
}

BOOL
APIENTRY
SetConsoleCursorMode(
    IN HANDLE hConsoleHandle,
    IN BOOL   Blink,
    IN BOOL   DBEnable
    )

/*++

Parameters:

    hConsoleHandle - Supplies a console input or output handle.

    Blink - Blinking enable/disable switch.

    DBEnable - Double Byte width enable/disable switch.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    return FALSE;
}

BOOL
APIENTRY
GetConsoleCursorMode(
    IN HANDLE hConsoleHandle,
    OUT PBOOL  pbBlink,
    OUT PBOOL  pbDBEnable
    )

/*++

Parameters:

    hConsoleHandle - Supplies a console input or output handle.

    Blink - Blinking enable/disable switch.

    DBEnable - Double Byte width enable/disable switch.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    return FALSE;
}

BOOL
APIENTRY
RegisterConsoleOS2(
    IN BOOL fOs2Register
    )

/*++

Description:

    This routine registers the OS/2 with the console.

Parameters:

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    return FALSE;
}

BOOL
APIENTRY
SetConsoleOS2OemFormat(
    IN BOOL fOs2OemFormat
    )

/*++

Description:

    This routine sets the OS/2 OEM Format with the console.

Parameters:

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    return FALSE;
}

#endif //!defined(BUILD_WOW6432)

#if defined(FE_IME)

#if !defined(BUILD_WOW64)

BOOL
APIENTRY
RegisterConsoleIME(
    IN HWND  hWndConsoleIME,
    OUT DWORD *dwConsoleThreadId
    )

/*++

Description:

    This routine register the Console IME on the current desktop.

Parameters:

Return Value:

    TRUE - The operation was successful.

    FALSE - The operation failed.

--*/

{

    return FALSE;

}

BOOL
APIENTRY
UnregisterConsoleIME(
    )

/*++

Description:

    This routine unregister the Console IME on the current desktop.

Parameters:

Return Value:

    TRUE - The operation was successful.

    FALSE - The operation failed.

--*/

{

    return FALSE;

}

#endif //!defined(BUILD_WOW64)

#endif // FE_IME

#endif // FE_SB
