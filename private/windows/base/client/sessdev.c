
/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    sessdev.c

Abstract:

    Per Session Dos Device access routines

Author:


Revision History:

--*/

#include "basedll.h"


#define SESSION0_ROOT L"GLOBALROOT"
#define SESSIONX_ROOT L"GLOBALROOT\\Sessions\\"



BOOL
WINAPI
DosPathToSessionPathA(
    IN DWORD   SessionId,
    IN LPCSTR pInPath,
    OUT LPSTR  *ppOutPath
    )

/*++

Routine Description:

    Converts a DOS path relative to the current session to a DOS path
    that allows access to a specific session.

Arguments:

    SessionId - SessionId to access.

    pInPath   - WIN32 DOS path. Could be of the form "C:", "LPT1:",
                "C:\file\path", etc.

    ppOutPath - Output path that accesses the specified session.
                If pIniPath is "C:" and SessionId is 6, the output would be
                "GLOBALROOT\Sessions\6\DosDevices\C:".

Return Value:

    TRUE - Path returned in *ppOutPath in newly allocated memory from
           LocalAlloc.
    FALSE - Call failed. Error code returned via GetLastError()

--*/

{
    BOOL rc;
    DWORD Len;
    PCHAR Buf;
    NTSTATUS Status;
    PWCHAR pOutPath;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;

    // if the input path is null or the pointer is a bad pointer, return
    // an error.

    if( (pInPath == 0) ||
        (IsBadReadPtr( pInPath, sizeof( CHAR ))) ||
        (IsBadWritePtr( ppOutPath, sizeof(LPSTR) )) ) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    try {

        RtlInitAnsiString( &AnsiString, pInPath );
        Status = RtlAnsiStringToUnicodeString( &UnicodeString, &AnsiString, TRUE );

    } except (EXCEPTION_EXECUTE_HANDLER) {

        Status = GetExceptionCode();
    }

    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return FALSE;
    }

    rc = DosPathToSessionPathW(
             SessionId,
             UnicodeString.Buffer,
             &pOutPath
             );

    RtlFreeUnicodeString( &UnicodeString );

    if( !rc ) {
        return( rc );
    }

    RtlInitUnicodeString( &UnicodeString, pOutPath );
    Status = RtlUnicodeStringToAnsiString( &AnsiString, &UnicodeString, TRUE );
    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        LocalFree( pOutPath );
        return FALSE;
    }

    Len = strlen( AnsiString.Buffer ) + 1;
    Buf = LocalAlloc(LMEM_FIXED, Len);

    if( Buf == NULL ) {
        LocalFree( pOutPath );
        RtlFreeAnsiString( &AnsiString );
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    strcpy( Buf, AnsiString.Buffer );

    *ppOutPath = Buf;

    LocalFree( pOutPath );
    RtlFreeAnsiString( &AnsiString );

    return(TRUE);
}


BOOL
WINAPI
DosPathToSessionPathW(
    IN DWORD   SessionId,
    IN LPCWSTR  pInPath,
    OUT LPWSTR  *ppOutPath
    )

/*++

Routine Description:

    Converts a DOS path relative to the current session to a DOS path
    that allows access to a specific session.

Arguments:

    SessionId - SessionId to access.

    pInPath   - WIN32 DOS path. Could be of the form "C:", "LPT1:",
                "C:\file\path", etc.

    ppOutPath - Output path that accesses the specified session.
                If pIniPath is "C:" and SessionId is 6, the output would be
                "GLOBALROOT\Sessions\6\DosDevices\C:".

Return Value:

    TRUE - Path returned in *ppOutPath in newly allocated memory from
           LocalAlloc.
    FALSE - Call failed. Error code returned via GetLastError()

--*/

{
    PWCHAR Buf;
    ULONG  Len;

    //
    // SessionId 0 has no per session object directories.
    //
    if( SessionId == 0 ) {
        Len =  wcslen(SESSION0_ROOT);
    }
    else {
        Len =  wcslen(SESSIONX_ROOT);
        Len += 10;                     // Max DWORD width
    }

    Len += 13;                         // \DosDevices\ ... <NULL>

    // if the input path is null or the pointer is a bad pointer, return
    // an error.

    if( (pInPath == 0) ||
        (IsBadReadPtr( pInPath, sizeof( WCHAR ))) ||
        (IsBadWritePtr( ppOutPath, sizeof(LPWSTR) )) ) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    Len += wcslen(pInPath);

    Buf = LocalAlloc(LMEM_FIXED, Len * sizeof(WCHAR));
    if( Buf == NULL ) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

    try {

        if( SessionId == 0 ) {
            // C: -> GLOBALROOT\DosDevices\C:
            swprintf(
                Buf,
                L"%ws\\DosDevices\\%ws",
                SESSION0_ROOT,
                pInPath
                );
        }
        else {
            // C: -> GLOBALROOT\Sessions\6\DosDevices\C:
            swprintf(
                Buf,
                L"%ws%u\\DosDevices\\%ws",
                SESSIONX_ROOT,
                SessionId,
                pInPath
                );
        }

        *ppOutPath = Buf;

    } except (EXCEPTION_EXECUTE_HANDLER) {

        BaseSetLastNTError(GetExceptionCode());
        return(FALSE);
    }


    return(TRUE);
}


BOOL
WINAPI
ProcessIdToSessionId(
    IN  DWORD  dwProcessId,
    OUT DWORD *pSessionId
    )

/*++

Routine Description:

    Given a ProcessId, return the SessionId.

    This is useful for services that impersonate a caller, and
    redefine a drive letter for the caller. An example is the
    workstation service. Transport specific routines allow the
    ProcessId of the caller to be retrieved.

Arguments:

    Process -  Process identifies process to
                return the SessionId for.

    pSessionId - returned SessionId.

Return Value:

    TRUE - SessionId returned in *pSessionId
    FALSE - Call failed. Error code returned via GetLastError()

--*/

{
    HANDLE Handle;
    NTSTATUS Status;
    CLIENT_ID ClientId;
    OBJECT_ATTRIBUTES Obja;
    PROCESS_SESSION_INFORMATION Info;


    if( IsBadWritePtr( pSessionId, sizeof(DWORD) ) )   {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }


    InitializeObjectAttributes(
        &Obja,
        NULL,
        0,
        NULL,
        NULL
        );

    ClientId.UniqueProcess = (HANDLE) LongToHandle(dwProcessId);
    ClientId.UniqueThread = (HANDLE)NULL;

    Status = NtOpenProcess(
                 &Handle,
                 (ACCESS_MASK)PROCESS_QUERY_INFORMATION,
                 &Obja,
                 &ClientId
                 );

    if( !NT_SUCCESS(Status) ) {
        SetLastError(RtlNtStatusToDosError(Status));
        return(FALSE);
    }

    Status = NtQueryInformationProcess(
                 Handle,
                 ProcessSessionInformation,
                 &Info,
                 sizeof(Info),
                 NULL
                 );

    if( !NT_SUCCESS(Status) ) {
        NtClose( Handle );
        SetLastError(RtlNtStatusToDosError(Status));
        return(FALSE);
    }

    *pSessionId = Info.SessionId;

    NtClose( Handle );

    return(TRUE);
}


