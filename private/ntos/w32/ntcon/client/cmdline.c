/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    cmdline.c

Abstract:

        This file implements command line editing and aliasing.

Author:

    Therese Stowell (thereses) 22-Mar-1991

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#pragma hdrstop

#define CONSOLE_NAME_PATH_SEPARATOR ((WCHAR)L'\\')
#define CONSOLE_NAME_PATH_TERMINATOR ((WCHAR)L'\0')

#if !defined(BUILD_WOW64)

#define FNAME_LENGTH 256
BOOLEAN ExeNameInitialized;
RTL_CRITICAL_SECTION ExeNameCriticalSection;
WCHAR ExeNameBuffer[FNAME_LENGTH];
USHORT ExeNameLength;       // in chars, not bytes
WCHAR StartDirBuffer[MAX_PATH+1];
USHORT StartDirLength;      // in chars, not bytes

VOID
InitExeName( VOID )
{

    PPEB Peb;
    PLDR_DATA_TABLE_ENTRY ImageEntry;

    if (ExeNameInitialized) {
        return;
    }

    RtlInitializeCriticalSection(&ExeNameCriticalSection);
    ExeNameInitialized = TRUE;
    Peb = NtCurrentPeb();
    ImageEntry = (PLDR_DATA_TABLE_ENTRY)Peb->Ldr->InLoadOrderModuleList.Flink;
    ImageEntry = CONTAINING_RECORD(ImageEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
    ExeNameLength = ImageEntry->BaseDllName.Length/sizeof(WCHAR);
    RtlCopyMemory(ExeNameBuffer,
           ImageEntry->BaseDllName.Buffer,
           ImageEntry->BaseDllName.Length
          );
    ExeNameBuffer[ExeNameLength] = CONSOLE_NAME_PATH_TERMINATOR;
    StartDirLength = Peb->ProcessParameters->CurrentDirectory.DosPath.Length/sizeof(WCHAR);
    RtlCopyMemory(StartDirBuffer,
           Peb->ProcessParameters->CurrentDirectory.DosPath.Buffer,
           Peb->ProcessParameters->CurrentDirectory.DosPath.Length
          );
    StartDirBuffer[StartDirLength] = CONSOLE_NAME_PATH_TERMINATOR;
}


USHORT
GetCurrentExeName(
    OUT LPWSTR Buffer,
    IN ULONG BufferLength
    )
{
    if (!ExeNameInitialized) {
        Buffer[0] = UNICODE_NULL;
        return 0;
    }

    RtlEnterCriticalSection(&ExeNameCriticalSection);
    if (BufferLength > ExeNameLength * sizeof( WCHAR )) {
        BufferLength = ExeNameLength * sizeof( WCHAR );
    }
    RtlCopyMemory(Buffer,ExeNameBuffer,BufferLength);
    RtlLeaveCriticalSection(&ExeNameCriticalSection);
    return (USHORT)BufferLength;
}

#endif //!defined(BUILD_WOW64)

#if !defined(BUILD_WOW6432)

USHORT
GetExeName(
    IN OUT LPVOID *ExeName,
    IN BOOLEAN UnicodeApi,
    OUT PBOOLEAN UnicodeExe
    )
{

    *UnicodeExe = TRUE;
    if (*ExeName == NULL) {
        // Should not get here
        return 0;
    } else if (!UnicodeApi) {
        *UnicodeExe = FALSE;
    }

    if (ExeName == NULL) {
       return 0;
    }
    
    try {
       if (*UnicodeExe) 
           return (USHORT)wcslen(*ExeName)*sizeof(WCHAR);  // returns bytes, not chars
       else
           return (USHORT)strlen(*ExeName);  // returns bytes, not chars
    }
    except(EXCEPTION_EXECUTE_HANDLER) {
       return 0;
    }
}

BOOL
APIENTRY
AddConsoleAliasInternal(
    IN LPVOID Source,
    IN UINT   SourceLength,
    IN LPVOID Target,
    IN UINT   TargetLength,
    IN LPVOID ExeName,
    IN BOOLEAN Unicode
    )

/*++

Parameters:

    Source - String to substitute alias string for, in input stream.

    Target - String to substitute for source.  NULL to delete an existing
        alias.

    ExeName - Exe to set alias in.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    PCSR_CAPTURE_HEADER CaptureBuffer;
    CONSOLE_API_MSG m;
    ULONG CapturedMsgPointers;
    PCONSOLE_ADDALIAS_MSG a = &m.u.AddConsoleAliasW;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->SourceLength = (USHORT)SourceLength;
    a->Unicode = Unicode;
    CapturedMsgPointers = 2;

    a->ExeLength = GetExeName(&ExeName,Unicode,&a->UnicodeExe);

    if (Target != NULL) {
        a->TargetLength = (USHORT)TargetLength;
        CapturedMsgPointers += 1;
    }
    else {
        a->Target = NULL;
        a->TargetLength = 0;
    }

    CaptureBuffer = CsrAllocateCaptureBuffer( CapturedMsgPointers,
                                              a->SourceLength + a->TargetLength + a->ExeLength
                                            );
    if (CaptureBuffer == NULL) {
        SET_LAST_ERROR(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    CsrCaptureMessageBuffer( CaptureBuffer,
                             (PCHAR) Source,
                             a->SourceLength,
                             (PVOID *) &a->Source
                           );

    CsrCaptureMessageBuffer( CaptureBuffer,
                             (PCHAR) ExeName,
                             a->ExeLength,
                             (PVOID *) &a->Exe
                           );

    if (Target != NULL) {
        CsrCaptureMessageBuffer( CaptureBuffer,
                                 (PCHAR) Target,
                                 a->TargetLength,
                                 (PVOID *) &a->Target
                               );
    }

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepAddAlias
                                            ),
                         sizeof( *a )
                       );

    CsrFreeCaptureBuffer( CaptureBuffer );

    if (!NT_SUCCESS( m.ReturnValue )) {
        SET_LAST_NT_ERROR(m.ReturnValue);
        return FALSE;
    }
    return TRUE;

}

#endif //!defined(BUILD_WOW6432)

#if !defined(BUILD_WOW64)

BOOL
APIENTRY
AddConsoleAliasW(
    IN LPWSTR Source,
    IN LPWSTR Target,
    IN LPWSTR ExeName
    )
{
    USHORT TargetLength;

    if (Target != NULL) {
        TargetLength = (USHORT) (lstrlenW(Target)*sizeof(WCHAR));
    } else {
        TargetLength = 0;
    }
    return AddConsoleAliasInternal(Source,
                                   lstrlenW(Source)*sizeof(WCHAR),
                                   Target,
                                   TargetLength,
                                   ExeName,
                                   TRUE
                                  );
}

BOOL
APIENTRY
AddConsoleAliasA(
    IN LPTSTR Source,
    IN LPTSTR Target,
    IN LPTSTR ExeName
    )
{
    USHORT TargetLength;

    if (Target != NULL) {
        TargetLength = (USHORT) lstrlenA(Target);
    } else {
        TargetLength = 0;
    }
    return AddConsoleAliasInternal(Source,
                                   lstrlenA(Source),
                                   Target,
                                   TargetLength,
                                   ExeName,
                                   FALSE
                                  );
}

#endif //!defined(BUILD_WOW64)

#if !defined(BUILD_WOW6432)

DWORD
APIENTRY
GetConsoleAliasInternal(
    IN LPVOID Source,
    IN UINT   SourceLength,
    OUT LPVOID TargetBuffer,
    IN UINT   TargetBufferLength,
    IN LPVOID ExeName,
    IN BOOLEAN Unicode
    )

/*++

Parameters:

    Source - Name of alias to query.  NULL means query all alias names.

    Target - Where to store value of alias.  If Source is NULL, then
        value is one or more null terminated strings terminated by an extra
        null byte.  Each null terminated string is the name of an alias.

Return Value:

    Non-zero - The operation was successful, and the return value is the
        number of characters stored in the Target buffer.

    Zero - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    PCSR_CAPTURE_HEADER CaptureBuffer;
    CONSOLE_API_MSG m;
    ULONG CapturedMsgPointers;
    PCONSOLE_GETALIAS_MSG a = &m.u.GetConsoleAliasW;

    if (Source == NULL) {
        SET_LAST_ERROR(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->Unicode = Unicode;

    CapturedMsgPointers = 3;

    a->ExeLength = GetExeName(&ExeName,Unicode,&a->UnicodeExe);

    a->SourceLength = (USHORT) SourceLength;
    a->TargetLength = (USHORT) TargetBufferLength;

    CaptureBuffer = CsrAllocateCaptureBuffer( CapturedMsgPointers,
                                              a->SourceLength + a->TargetLength + a->ExeLength
                                            );
    if (CaptureBuffer == NULL) {
        SET_LAST_ERROR(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    CsrCaptureMessageBuffer( CaptureBuffer,
                             (PCHAR) ExeName,
                             a->ExeLength,
                             (PVOID *) &a->Exe
                           );

    CsrCaptureMessageBuffer( CaptureBuffer,
                             (PCHAR) Source,
                             a->SourceLength,
                             (PVOID *) &a->Source
                           );

    CsrCaptureMessageBuffer( CaptureBuffer,
                             (PCHAR) NULL,
                             a->TargetLength,
                             (PVOID *) &a->Target
                           );

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepGetAlias
                                            ),
                         sizeof( *a )
                       );

    if (NT_SUCCESS( m.ReturnValue )) {
        RtlCopyMemory( TargetBuffer, a->Target, a->TargetLength );
    }
    else {
        SET_LAST_NT_ERROR(m.ReturnValue);
        if (m.ReturnValue != STATUS_BUFFER_TOO_SMALL) {
            a->TargetLength = 0;
        }
    }

    CsrFreeCaptureBuffer( CaptureBuffer );

    return a->TargetLength;

}

#endif !defined(BUILD_WOW6432)

#if !defined(BUILD_WOW64)

DWORD
APIENTRY
GetConsoleAliasW(
    IN LPWSTR Source,
    OUT LPWSTR TargetBuffer,
    IN DWORD TargetBufferLength,
    IN LPWSTR ExeName
    )
{
    return GetConsoleAliasInternal(Source,
                                   lstrlenW(Source)*sizeof(WCHAR),
                                   TargetBuffer,
                                   TargetBufferLength,
                                   ExeName,
                                   TRUE
                                  );
}

DWORD
APIENTRY
GetConsoleAliasA(
    IN LPTSTR Source,
    OUT LPTSTR TargetBuffer,
    IN DWORD TargetBufferLength,
    IN LPTSTR ExeName
    )
{
    return GetConsoleAliasInternal(Source,
                                   lstrlenA(Source),
                                   TargetBuffer,
                                   TargetBufferLength,
                                   ExeName,
                                   FALSE
                                  );
}

#endif //!defined(BUILD_WOW64)

#if !defined(BUILD_WOW6432)

DWORD
APIENTRY
GetConsoleAliasesLengthInternal(
    IN LPVOID ExeName,
    IN BOOLEAN Unicode
    )
{
    CONSOLE_API_MSG m;
    PCONSOLE_GETALIASESLENGTH_MSG a = &m.u.GetConsoleAliasesLengthW;
    PCSR_CAPTURE_HEADER CaptureBuffer;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->ExeLength = GetExeName(&ExeName,Unicode,&a->UnicodeExe);
    a->Unicode = Unicode;

    CaptureBuffer = CsrAllocateCaptureBuffer( 1,
                                              a->ExeLength
                                            );
    if (CaptureBuffer == NULL) {
        SET_LAST_ERROR(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    CsrCaptureMessageBuffer( CaptureBuffer,
                             (PCHAR) ExeName,
                             a->ExeLength,
                             (PVOID *) &a->Exe
                           );
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepGetAliasesLength
                                            ),
                         sizeof( *a )
                       );
    CsrFreeCaptureBuffer( CaptureBuffer );
    if (NT_SUCCESS( m.ReturnValue )) {
        return a->AliasesLength;
    } else {
        SET_LAST_NT_ERROR(m.ReturnValue);
        return 0;
    }

}

#endif //!defined(BUILD_WOW6432)

#if !defined(BUILD_WOW64)

DWORD
APIENTRY
GetConsoleAliasesLengthW(
    IN LPWSTR ExeName
    )
{
    return GetConsoleAliasesLengthInternal(ExeName,
                                           TRUE
                                          );
}

DWORD
APIENTRY
GetConsoleAliasesLengthA(
    IN LPTSTR ExeName
    )
{
    return GetConsoleAliasesLengthInternal(ExeName,
                                           FALSE
                                          );
}

#endif //!defined(BUILD_WOW64)

#if !defined(BUILD_WOW6432)

DWORD
APIENTRY
GetConsoleAliasExesLengthInternal(
    IN BOOLEAN Unicode
    )
{
    CONSOLE_API_MSG m;
    PCONSOLE_GETALIASEXESLENGTH_MSG a = &m.u.GetConsoleAliasExesLengthW;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->Unicode = Unicode;

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepGetAliasExesLength
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        return a->AliasExesLength;
    } else {
        SET_LAST_NT_ERROR(m.ReturnValue);
        return 0;
    }

}

#endif //!defined(BUILD_WOW6432)

#if !defined(BUILD_WOW64)

DWORD
APIENTRY
GetConsoleAliasExesLengthW( VOID )
{
    return GetConsoleAliasExesLengthInternal( TRUE );
}

DWORD
APIENTRY
GetConsoleAliasExesLengthA( VOID )
{
    return GetConsoleAliasExesLengthInternal( FALSE );
}

#endif //!defined(BUILD_WOW64)

#if !defined(BUILD_WOW6432)

DWORD
APIENTRY
GetConsoleAliasesInternal(
    OUT LPVOID AliasBuffer,
    IN DWORD AliasBufferLength,
    IN LPVOID ExeName,
    IN BOOLEAN Unicode
    )
{
    CONSOLE_API_MSG m;
    PCONSOLE_GETALIASES_MSG a = &m.u.GetConsoleAliasesW;
    PCSR_CAPTURE_HEADER CaptureBuffer;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->ExeLength = GetExeName(&ExeName,Unicode,&a->UnicodeExe);
    a->Unicode = Unicode;
    a->AliasesBufferLength = AliasBufferLength;

    CaptureBuffer = CsrAllocateCaptureBuffer( 2,
                                              a->ExeLength + AliasBufferLength
                                            );
    if (CaptureBuffer == NULL) {
        SET_LAST_ERROR(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    CsrCaptureMessageBuffer( CaptureBuffer,
                             (PCHAR) ExeName,
                             a->ExeLength,
                             (PVOID *) &a->Exe
                           );
    CsrCaptureMessageBuffer( CaptureBuffer,
                             (PCHAR) NULL,
                             a->AliasesBufferLength,
                             (PVOID *) &a->AliasesBuffer
                           );
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepGetAliases
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        try {
            RtlCopyMemory( AliasBuffer, a->AliasesBuffer, a->AliasesBufferLength );
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            CsrFreeCaptureBuffer( CaptureBuffer );
            SET_LAST_ERROR(ERROR_INVALID_ACCESS);
            return 0;
        }
        CsrFreeCaptureBuffer( CaptureBuffer );
        return a->AliasesBufferLength;
    } else {
        CsrFreeCaptureBuffer( CaptureBuffer );
        SET_LAST_NT_ERROR(m.ReturnValue);
        return 0;
    }

}

#endif //!defined(BUILD_WOW6432)

#if !defined(BUILD_WOW64)

DWORD
GetConsoleAliasesW(
    OUT LPWSTR AliasBuffer,
    IN DWORD AliasBufferLength,
    IN LPWSTR ExeName
    )
{
    return GetConsoleAliasesInternal(AliasBuffer,
                                     AliasBufferLength,
                                     ExeName,
                                     TRUE
                                    );
}

DWORD
GetConsoleAliasesA(
    OUT LPTSTR AliasBuffer,
    IN DWORD AliasBufferLength,
    IN LPTSTR ExeName
    )
{
    return GetConsoleAliasesInternal(AliasBuffer,
                                     AliasBufferLength,
                                     ExeName,
                                     FALSE
                                    );
}

#endif //!defined(BUILD_WOW64)

#if !defined(BUILD_WOW6432)

DWORD
APIENTRY
GetConsoleAliasExesInternal(
    OUT LPVOID ExeNameBuffer,
    IN DWORD ExeNameBufferLength,
    IN BOOLEAN Unicode
    )
{
    CONSOLE_API_MSG m;
    PCONSOLE_GETALIASEXES_MSG a = &m.u.GetConsoleAliasExesW;
    PCSR_CAPTURE_HEADER CaptureBuffer;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->AliasExesBufferLength = ExeNameBufferLength;
    a->Unicode = Unicode;
    CaptureBuffer = CsrAllocateCaptureBuffer( 1,
                                              ExeNameBufferLength
                                            );
    if (CaptureBuffer == NULL) {
        SET_LAST_ERROR(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    CsrCaptureMessageBuffer( CaptureBuffer,
                             (PCHAR) NULL,
                             a->AliasExesBufferLength,
                             (PVOID *) &a->AliasExesBuffer
                           );

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepGetAliasExes
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        try {
            RtlCopyMemory( ExeNameBuffer, a->AliasExesBuffer, a->AliasExesBufferLength );
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            CsrFreeCaptureBuffer( CaptureBuffer );
            SET_LAST_ERROR(ERROR_INVALID_ACCESS);
            return 0;
        }
        CsrFreeCaptureBuffer( CaptureBuffer );
        return a->AliasExesBufferLength;
    } else {
        CsrFreeCaptureBuffer( CaptureBuffer );
        SET_LAST_NT_ERROR(m.ReturnValue);
        return 0;
    }

}

#endif //!defined(BUILD_WOW6432)

#if !defined(BUILD_WOW64)

DWORD
GetConsoleAliasExesW(
    OUT LPWSTR ExeNameBuffer,
    IN DWORD ExeNameBufferLength
    )
{
    return GetConsoleAliasExesInternal(ExeNameBuffer,
                                       ExeNameBufferLength,
                                       TRUE
                                      );
}

DWORD
GetConsoleAliasExesA(
    OUT LPTSTR ExeNameBuffer,
    IN DWORD ExeNameBufferLength
    )
{
    return GetConsoleAliasExesInternal(ExeNameBuffer,
                                       ExeNameBufferLength,
                                       FALSE
                                      );
}

#endif //!defined(BUILD_WOW64)

#if !defined(BUILD_WOW6432)

VOID
APIENTRY
ExpungeConsoleCommandHistoryInternal(
    IN LPVOID ExeName,
    IN BOOLEAN Unicode
    )
{
    CONSOLE_API_MSG m;
    PCONSOLE_EXPUNGECOMMANDHISTORY_MSG a = &m.u.ExpungeConsoleCommandHistoryW;
    PCSR_CAPTURE_HEADER CaptureBuffer;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->ExeLength = GetExeName(&ExeName,Unicode,&a->UnicodeExe);
    a->Unicode = Unicode;

    CaptureBuffer = CsrAllocateCaptureBuffer( 1,
                                              a->ExeLength
                                            );
    if (CaptureBuffer == NULL) {
        SET_LAST_ERROR(ERROR_NOT_ENOUGH_MEMORY);
        return;
    }

    CsrCaptureMessageBuffer( CaptureBuffer,
                             (PCHAR) ExeName,
                             a->ExeLength,
                             (PVOID *) &a->Exe
                           );

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepExpungeCommandHistory
                                            ),
                         sizeof( *a )
                       );
    CsrFreeCaptureBuffer( CaptureBuffer );

}


#endif //!defined(BUILD_WOW6432)

#if !defined(BUILD_WOW64)

VOID
ExpungeConsoleCommandHistoryW(
    IN LPWSTR ExeName
    )
{
    ExpungeConsoleCommandHistoryInternal(ExeName,TRUE);
}

VOID
ExpungeConsoleCommandHistoryA(
    IN LPTSTR ExeName
    )
{
    ExpungeConsoleCommandHistoryInternal(ExeName,FALSE);
}

#endif //!defined(BUILD_WOW64)

#if !defined(BUILD_WOW6432)

BOOL
APIENTRY
SetConsoleNumberOfCommandsInternal(
    IN DWORD Number,
    IN LPVOID ExeName,
    IN BOOLEAN Unicode
    )
{
    CONSOLE_API_MSG m;
    PCONSOLE_SETNUMBEROFCOMMANDS_MSG a = &m.u.SetConsoleNumberOfCommandsW;
    PCSR_CAPTURE_HEADER CaptureBuffer;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->NumCommands = Number;
    a->ExeLength = GetExeName(&ExeName,Unicode,&a->UnicodeExe);
    a->Unicode = Unicode;
    CaptureBuffer = CsrAllocateCaptureBuffer( 1,
                                              a->ExeLength
                                            );
    if (CaptureBuffer == NULL) {
        SET_LAST_ERROR(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    CsrCaptureMessageBuffer( CaptureBuffer,
                             (PCHAR) ExeName,
                             a->ExeLength,
                             (PVOID *) &a->Exe
                           );

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepSetNumberOfCommands
                                            ),
                         sizeof( *a )
                       );
    CsrFreeCaptureBuffer( CaptureBuffer );
    if (NT_SUCCESS( m.ReturnValue )) {
        return TRUE;
    } else {
        SET_LAST_NT_ERROR(m.ReturnValue);
        return FALSE;
    }

}

#endif //!defined(BUILD_WOW6432)

#if !defined(BUILD_WOW64)

BOOL
SetConsoleNumberOfCommandsW(
    IN DWORD Number,
    IN LPWSTR ExeName
    )
{
    return SetConsoleNumberOfCommandsInternal(Number,
                                              ExeName,
                                              TRUE
                                             );
}

BOOL
SetConsoleNumberOfCommandsA(
    IN DWORD Number,
    IN LPTSTR ExeName
    )
{
    return SetConsoleNumberOfCommandsInternal(Number,
                                              ExeName,
                                              FALSE
                                             );
}

#endif //!defined(BUILD_WOW64)

#if !defined(BUILD_WOW6432)

DWORD
APIENTRY
GetConsoleCommandHistoryLengthInternal(
    IN LPVOID ExeName,
    IN BOOLEAN Unicode
    )
{    
    CONSOLE_API_MSG m;
    PCONSOLE_GETCOMMANDHISTORYLENGTH_MSG a = &m.u.GetConsoleCommandHistoryLengthW;
    PCSR_CAPTURE_HEADER CaptureBuffer;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->ExeLength = GetExeName(&ExeName,Unicode,&a->UnicodeExe);
    a->Unicode = Unicode;
    CaptureBuffer = CsrAllocateCaptureBuffer( 1,
                                              a->ExeLength
                                            );
    if (CaptureBuffer == NULL) {
        SET_LAST_ERROR(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    CsrCaptureMessageBuffer( CaptureBuffer,
                             (PCHAR) ExeName,
                             a->ExeLength,
                             (PVOID *) &a->Exe
                           );

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepGetCommandHistoryLength
                                            ),
                         sizeof( *a )
                       );
    CsrFreeCaptureBuffer( CaptureBuffer );
    return a->CommandHistoryLength;

}

#endif //!defined(BUILD_WOW6432)

#if !defined(BUILD_WOW64)

DWORD
GetConsoleCommandHistoryLengthW(
    IN LPWSTR ExeName
    )
{
    return GetConsoleCommandHistoryLengthInternal(ExeName,
                                                  TRUE
                                                 );
}

DWORD
GetConsoleCommandHistoryLengthA(
    IN LPTSTR ExeName
    )
{
    return GetConsoleCommandHistoryLengthInternal(ExeName,
                                                  FALSE
                                                 );
}

#endif //!defined(BUILD_WOW64)

#if !defined(BUILD_WOW6432)

DWORD
APIENTRY
GetConsoleCommandHistoryInternal(
    OUT LPVOID Commands,
    IN DWORD CommandBufferLength,
    IN LPVOID ExeName,
    IN BOOLEAN Unicode
    )
{
    CONSOLE_API_MSG m;
    PCONSOLE_GETCOMMANDHISTORY_MSG a = &m.u.GetConsoleCommandHistoryW;
    PCSR_CAPTURE_HEADER CaptureBuffer;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->CommandBufferLength = CommandBufferLength;
    a->ExeLength = GetExeName(&ExeName,Unicode,&a->UnicodeExe);
    a->Unicode = Unicode;
    CaptureBuffer = CsrAllocateCaptureBuffer( 2,
                                              CommandBufferLength + a->ExeLength
                                            );
    if (CaptureBuffer == NULL) {
        SET_LAST_ERROR(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }


    CsrCaptureMessageBuffer( CaptureBuffer,
                             (PCHAR) ExeName,
                             a->ExeLength,
                             (PVOID *) &a->Exe
                           );

    CsrCaptureMessageBuffer( CaptureBuffer,
                             (PCHAR) NULL,
                             a->CommandBufferLength,
                             (PVOID *) &a->CommandBuffer
                           );
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepGetCommandHistory
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        try {
            RtlCopyMemory( Commands, a->CommandBuffer, a->CommandBufferLength );
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            CsrFreeCaptureBuffer( CaptureBuffer );
            SET_LAST_ERROR(ERROR_INVALID_ACCESS);
            return 0;
        }
        CsrFreeCaptureBuffer( CaptureBuffer );
        return a->CommandBufferLength;
    } else {
        CsrFreeCaptureBuffer( CaptureBuffer );
        SET_LAST_NT_ERROR(m.ReturnValue);
        return 0;
    }

}

#endif //!defined(BUILD_WOW6432)

#if !defined(BUILD_WOW64)

DWORD
GetConsoleCommandHistoryW(
    OUT LPWSTR Commands,
    IN DWORD CommandBufferLength,
    IN LPWSTR ExeName
    )
{
    return GetConsoleCommandHistoryInternal(Commands,
                                            CommandBufferLength,
                                            ExeName,
                                            TRUE
                                           );
}

DWORD
GetConsoleCommandHistoryA(
    OUT LPTSTR Commands,
    IN DWORD CommandBufferLength,
    IN LPTSTR ExeName
    )
{
    return GetConsoleCommandHistoryInternal(Commands,
                                            CommandBufferLength,
                                            ExeName,
                                            FALSE
                                           );
}

#endif //!defined(BUILD_WOW64)

#if !defined(BUILD_WOW6432)

BOOL
APIENTRY
SetConsoleCommandHistoryMode(
    IN DWORD Flags
    )
{
    CONSOLE_API_MSG m;
    PCONSOLE_SETCOMMANDHISTORYMODE_MSG a = &m.u.SetConsoleCommandHistoryMode;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->Flags = Flags;

    CsrClientCallServer( (PCSR_API_MSG)&m,
                         NULL,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepSetCommandHistoryMode
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        return TRUE;
    } else {
        SET_LAST_NT_ERROR(m.ReturnValue);
        return FALSE;
    }
}

DWORD
APIENTRY
GetConsoleTitleInternal(
    OUT PVOID lpConsoleTitle,
    IN DWORD nSize,
    IN BOOLEAN Unicode
    )
{
    CONSOLE_API_MSG m;
    PCONSOLE_GETTITLE_MSG a = &m.u.GetConsoleTitle;
    PCSR_CAPTURE_HEADER CaptureBuffer;

    if (nSize == 0) {
        return 0;
    }
    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->TitleLength = nSize;
    a->Unicode = Unicode;
    CaptureBuffer = CsrAllocateCaptureBuffer( 1,
                                              a->TitleLength
                                            );
    if (CaptureBuffer == NULL) {
        return 0;
    }
    CsrCaptureMessageBuffer( CaptureBuffer,
                             NULL,
                             a->TitleLength,
                             (PVOID *) &a->Title
                           );
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepGetTitle
                                            ),
                         sizeof( *a )
                       );
    if (NT_SUCCESS( m.ReturnValue )) {
        try {
            RtlCopyMemory( lpConsoleTitle, a->Title, a->TitleLength);
        } except( EXCEPTION_EXECUTE_HANDLER ) {
            CsrFreeCaptureBuffer( CaptureBuffer );
            SET_LAST_ERROR(ERROR_INVALID_ACCESS);
            return 0;
        }
    }
    else {
        CsrFreeCaptureBuffer( CaptureBuffer );
        SET_LAST_NT_ERROR (m.ReturnValue);
        return 0;
    }
    CsrFreeCaptureBuffer( CaptureBuffer );
    return a->TitleLength;

}

#endif //!defined(BUILD_WOW6432)

#if !defined(BUILD_WOW64)

DWORD
APIENTRY
GetConsoleTitleA(
    LPSTR lpConsoleTitle,
    DWORD nSize
    )
{
    DWORD NumBytes;
    NumBytes = GetConsoleTitleInternal(lpConsoleTitle,
                                   nSize-sizeof(CHAR), // allow room for null
                                   FALSE
                                  );
    if (NumBytes) {
        lpConsoleTitle[NumBytes] = '\0';   // NULL terminate
    }
    return NumBytes;
}

DWORD
APIENTRY
GetConsoleTitleW(
    LPWSTR lpConsoleTitle,
    DWORD nSize
    )
{
    DWORD NumBytes;
    NumBytes = GetConsoleTitleInternal(lpConsoleTitle,
                                   (nSize-1)*sizeof(WCHAR), // allow room for null
                                   TRUE
                                  );
    if (NumBytes) {
        NumBytes /= sizeof(WCHAR);
        lpConsoleTitle[NumBytes] = CONSOLE_NAME_PATH_TERMINATOR;   // NULL terminate
    }
    return NumBytes;
}

#endif //!defined(BUILD_WOW64)

#if !defined(BUILD_WOW6432)

BOOL
APIENTRY
SetConsoleTitleInternal(
    IN CONST VOID *lpConsoleTitle,
    IN DWORD TitleLength,  // in bytes
    IN BOOLEAN Unicode
    )
{
    CONSOLE_API_MSG m;
    PCONSOLE_SETTITLE_MSG a = &m.u.SetConsoleTitle;
    PCSR_CAPTURE_HEADER CaptureBuffer;

    a->ConsoleHandle = GET_CONSOLE_HANDLE;
    a->TitleLength = TitleLength;
    a->Unicode = Unicode;
    CaptureBuffer = CsrAllocateCaptureBuffer( 1,
                                              a->TitleLength
                                            );
    if (CaptureBuffer == NULL) {
        return FALSE;
    }
    CsrCaptureMessageBuffer( CaptureBuffer,
                             (PCHAR) lpConsoleTitle,
                             a->TitleLength,
                             (PVOID *) &a->Title
                           );
    CsrClientCallServer( (PCSR_API_MSG)&m,
                         CaptureBuffer,
                         CSR_MAKE_API_NUMBER( CONSRV_SERVERDLL_INDEX,
                                              ConsolepSetTitle
                                            ),
                         sizeof( *a )
                       );

    CsrFreeCaptureBuffer( CaptureBuffer );

    if (NT_SUCCESS( m.ReturnValue )) {
        return TRUE;
    }
    else {
        SET_LAST_NT_ERROR (m.ReturnValue);
        return FALSE;
    }

}

#endif //!defined(BUILD_WOW6432)

#if !defined(BUILD_WOW64)

BOOL
APIENTRY
SetConsoleTitleA(
    LPCSTR lpConsoleTitle
    )
{
    DWORD TitleLength;
    try {
        TitleLength = strlen(lpConsoleTitle);
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        SET_LAST_ERROR(ERROR_INVALID_ACCESS);
        return FALSE;
    }
    return SetConsoleTitleInternal(lpConsoleTitle,
                                   TitleLength,
                                   FALSE);
}

BOOL
APIENTRY
SetConsoleTitleW(
    LPCWSTR lpConsoleTitle
    )
{
    DWORD TitleLength;
    try {
        TitleLength = wcslen(lpConsoleTitle)*sizeof(WCHAR);
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        SET_LAST_ERROR(ERROR_INVALID_ACCESS);
        return FALSE;
    }
    return SetConsoleTitleInternal(lpConsoleTitle,
                                   TitleLength,
                                   TRUE);
}


DWORD
GetConsoleInputExeNameA(
    IN DWORD nBufferLength,
    OUT LPSTR lpBuffer
    )
{
    PUNICODE_STRING Unicode;
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;
    WCHAR TempExeNameBuffer[FNAME_LENGTH];
    DWORD n;

    n = GetConsoleInputExeNameW(FNAME_LENGTH, TempExeNameBuffer);
    if (n != 0 && n < FNAME_LENGTH) {
        RtlInitUnicodeString(&UnicodeString, TempExeNameBuffer);
        AnsiString.Buffer = lpBuffer;
        AnsiString.Length = 0;
        AnsiString.MaximumLength = (USHORT)nBufferLength;
        Status = RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, FALSE);
        if (!NT_SUCCESS(Status)) {
            if (Status == STATUS_BUFFER_OVERFLOW) {
                SET_LAST_ERROR(ERROR_BUFFER_OVERFLOW);
                return n+1;
            } else {
                SET_LAST_ERROR(ERROR_INVALID_PARAMETER);
            }
        }
    }
    return n;
}

DWORD
GetConsoleInputExeNameW(
    IN DWORD nBufferLength,
    OUT LPWSTR lpBuffer
    )
{
    DWORD n;

    n = ExeNameLength;
    if (n >= nBufferLength) {
        SET_LAST_ERROR(ERROR_BUFFER_OVERFLOW);
        return n+1;
    }

    try {
        RtlEnterCriticalSection(&ExeNameCriticalSection);
        RtlCopyMemory(lpBuffer, ExeNameBuffer, n*sizeof(WCHAR));
        lpBuffer[n] = UNICODE_NULL;
    } finally {
        RtlLeaveCriticalSection(&ExeNameCriticalSection);
    }

    return TRUE;
}

BOOL
SetConsoleInputExeNameA(
    IN LPSTR lpExeName
    )
{
    PUNICODE_STRING Unicode;
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    Unicode = &NtCurrentTeb()->StaticUnicodeString;
    RtlInitAnsiString(&AnsiString,lpExeName);
    Status = RtlAnsiStringToUnicodeString(Unicode,&AnsiString,FALSE);
    if (!NT_SUCCESS(Status)) {
        if (Status == STATUS_BUFFER_OVERFLOW) {
            SET_LAST_ERROR(ERROR_FILENAME_EXCED_RANGE);
        } else {
            SET_LAST_ERROR(ERROR_INVALID_PARAMETER);
        }
        return FALSE;
    }

    return SetConsoleInputExeNameW(Unicode->Buffer);
}

BOOL
SetConsoleInputExeNameW(
    IN LPWSTR lpExeName
    )
{
    DWORD n;

    n = lstrlenW(lpExeName);
    if (n == 0 || n >= FNAME_LENGTH) {
        SET_LAST_ERROR(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    RtlEnterCriticalSection(&ExeNameCriticalSection);
    try {
        RtlCopyMemory(ExeNameBuffer,lpExeName,n*sizeof(WCHAR));
        ExeNameLength = (USHORT)n;
    } finally {
        RtlLeaveCriticalSection(&ExeNameCriticalSection);
    }

    return TRUE;
}

#endif //!defined(BUILD_WOW64)