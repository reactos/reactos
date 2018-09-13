/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    win31io.c

Abstract:

    This file contains Win 3.1 Inter-Operability functions.

Author:

    Steve Wood (stevewo) 21-Feb-1993

Revision History:

--*/

#include "advapi.h"
#include <stdio.h>

#include <winbasep.h>
#include "win31io.h"

#define EVENTLOG_SOURCE "Windows 3.1 Migration"

BOOL
WaitForEventLogToStart( VOID );

#define SIZE_OF_TOKEN_INFORMATION                   \
    sizeof( TOKEN_USER )                            \
    + sizeof( SID )                                 \
    + sizeof( ULONG ) * SID_MAX_SUB_AUTHORITIES

typedef struct _WIN31IO_STATE {
    HANDLE EventLog;
    HANDLE SoftwareRoot;
    HANDLE UserRoot;
    HANDLE Win31IOKey;
    BOOL QueryOnly;
    PWSTR WindowsPath;
    PWSTR FileNamePart;
    PWSTR WorkBuffer;
    ULONG cchWorkBuffer;
    PWIN31IO_STATUS_CALLBACK StatusCallback;
    PVOID CallbackParameter;
    WCHAR szWindowsDirectory[ MAX_PATH ];
    PSID UserSid;
    UCHAR TokenInformation[ SIZE_OF_TOKEN_INFORMATION ];
    WCHAR QueryBuffer[ 64 ];
} WIN31IO_STATE, *PWIN31IO_STATE;


HANDLE
OpenCreateKey(
    IN HANDLE Root,
    IN PWSTR Path,
    IN BOOL WriteAccess
    );

BOOL
SnapShotWin31IniFilesToRegistry(
    IN OUT PWIN31IO_STATE State
    );

BOOL
SnapShotWin31GroupsToRegistry(
    IN OUT PWIN31IO_STATE State
    );

BOOL
SnapShotWin31RegDatToRegistry(
    IN OUT PWIN31IO_STATE State
    );

BOOL
InitializeWin31State(
    IN WIN31IO_EVENT EventType,
    OUT PWIN31IO_STATE State
    );


VOID
TerminateWin31State(
    IN WIN31IO_EVENT EventType,
    IN OUT PWIN31IO_STATE State
    );

BOOL
InitializeWin31State(
    IN WIN31IO_EVENT EventType,
    OUT PWIN31IO_STATE State
    )
{
    NTSTATUS Status;
    ULONG cch;
    HANDLE TokenHandle;
    ULONG ReturnLength;

    memset( State, 0, sizeof( *State ) );
    cch = GetWindowsDirectoryW( State->szWindowsDirectory,
                                sizeof( State->szWindowsDirectory )
                              );
    State->WindowsPath = State->szWindowsDirectory;
    State->FileNamePart = State->WindowsPath + cch;
    *State->FileNamePart++ = OBJ_NAME_PATH_SEPARATOR;

    if (EventType == Win31SystemStartEvent) {
        State->SoftwareRoot = OpenCreateKey( NULL,
                                             L"\\Registry\\Machine\\Software",
                                             FALSE
                                           );
        if (State->SoftwareRoot == NULL) {
            return FALSE;
            }
        }
    else {
        Status = RtlOpenCurrentUser( GENERIC_READ, &State->UserRoot );
        if (!NT_SUCCESS( Status )) {
            BaseSetLastNTError( Status );
            return FALSE;
            }

        if (OpenThreadToken( GetCurrentThread(),
                             TOKEN_READ,
                             TRUE,
                             &TokenHandle
                           ) ||
            OpenProcessToken( GetCurrentProcess(),
                              TOKEN_READ,
                              &TokenHandle
                            )
           ) {
            if (GetTokenInformation( TokenHandle,
                                     TokenUser,
                                     &State->TokenInformation,
                                     sizeof( State->TokenInformation ),
                                     &ReturnLength
                                   )
               ) {
                PTOKEN_USER UserToken = (PTOKEN_USER)&State->TokenInformation;

                State->UserSid = UserToken->User.Sid;
                }

            CloseHandle( TokenHandle );
            }
        }

    State->Win31IOKey = OpenCreateKey( EventType == Win31SystemStartEvent ?
                                            State->SoftwareRoot : State->UserRoot,
                                       L"Windows 3.1 Migration Status",
                                       TRUE
                                     );
    if (State->Win31IOKey == NULL) {
        if (State->SoftwareRoot != NULL) {
            NtClose( State->SoftwareRoot );
            }

        if (State->UserRoot != NULL) {
            NtClose( State->UserRoot );
            }

        if (State->EventLog != NULL) {
            DeregisterEventSource( State->EventLog );
            }

        return FALSE;
        }
    else {
        return TRUE;
        }

    return TRUE;
}


VOID
TerminateWin31State(
    IN WIN31IO_EVENT EventType,
    IN OUT PWIN31IO_STATE State
    )
{
    if (State->Win31IOKey != NULL) {
        NtClose( State->Win31IOKey );
        }

    if (State->SoftwareRoot != NULL) {
        NtClose( State->SoftwareRoot );
        }

    if (State->UserRoot != NULL) {
        NtClose( State->UserRoot );
        }

    if (State->EventLog != NULL && State->EventLog != INVALID_HANDLE_VALUE) {
        DeregisterEventSource( State->EventLog );
        }

    return;
}

#define MAX_EVENT_STRINGS 8

VOID
ReportWin31IOEvent(
    IN PWIN31IO_STATE State,
    IN WORD EventType,
    IN DWORD EventId,
    IN DWORD SizeOfRawData,
    IN PVOID RawData,
    IN DWORD NumberOfStrings,
    ...
    )
{
    va_list arglist;
    ULONG i;
    PWSTR Strings[ MAX_EVENT_STRINGS ];

    va_start( arglist, NumberOfStrings );

    if (NumberOfStrings > MAX_EVENT_STRINGS) {
        NumberOfStrings = MAX_EVENT_STRINGS;
        }

    for (i=0; i<NumberOfStrings; i++) {
        Strings[ i ] = va_arg( arglist, PWSTR );
        }

    if (State->EventLog == NULL) {
        State->EventLog = RegisterEventSource( NULL, EVENTLOG_SOURCE );
        if (State->EventLog == NULL) {
            if (WaitForEventLogToStart()) {
                State->EventLog = RegisterEventSource( NULL, EVENTLOG_SOURCE );
                }

            if (State->EventLog == NULL) {
                KdPrint(( "WIN31IO: RegisterEventSource( %s ) failed - %u\n", EVENTLOG_SOURCE, GetLastError() ));
                State->EventLog = INVALID_HANDLE_VALUE;
                return;
                }
            }
        }

    if (State->EventLog != INVALID_HANDLE_VALUE) {
        if (!ReportEventW( State->EventLog,
                           EventType,
                           0,            // event category
                           EventId,
                           State->UserSid,
                           (WORD)NumberOfStrings,
                           SizeOfRawData,
                           Strings,
                           RawData
                         )
           ) {
            KdPrint(( "WIN31IO: ReportEvent( %u ) failed - %u\n", EventId, GetLastError() ));
            }
        }
}

int
Win31IOExceptionHandler(
    IN DWORD ExceptionCode,
    IN PEXCEPTION_POINTERS ExceptionInfo,
    IN OUT PWIN31IO_STATE State
    );

int
Win31IOExceptionHandler(
    IN DWORD ExceptionCode,
    IN PEXCEPTION_POINTERS ExceptionInfo,
    IN OUT PWIN31IO_STATE State
    )
{
    KdPrint(( "WIN31IO: Unexpected exception %08x at %08x referencing %08x\n",
              ExceptionInfo->ExceptionRecord->ExceptionCode,
              ExceptionInfo->ExceptionRecord->ExceptionAddress,
              ExceptionInfo->ExceptionRecord->ExceptionInformation[ 1 ]
           ));

    //
    // Unexpected exception.  Log the event with the exception record
    // so we can figure it out later.
    //

    ReportWin31IOEvent( State,
                        EVENTLOG_ERROR_TYPE,
                        WIN31IO_EVENT_EXCEPTION,
                        sizeof( *(ExceptionInfo->ExceptionRecord) ),
                        ExceptionInfo->ExceptionRecord,
                        0
                      );

    return EXCEPTION_EXECUTE_HANDLER;
}


DWORD
WINAPI
QueryWindows31FilesMigration(
    IN WIN31IO_EVENT EventType
    )
{
    DWORD Flags;
    WIN31IO_STATE State;

    if (EventType == Win31LogoffEvent) {
        return 0;
        }

    if (!InitializeWin31State( EventType, &State )) {
        return 0;
        }

    State.QueryOnly = TRUE;
    State.WorkBuffer = State.QueryBuffer;
    State.cchWorkBuffer = sizeof( State.QueryBuffer ) / sizeof( WCHAR );
    Flags = 0;

    try {
        try {
            if (EventType == Win31SystemStartEvent) {
                if (SnapShotWin31IniFilesToRegistry( &State )) {
                    Flags |= WIN31_MIGRATE_INIFILES;
                    }

                if (SnapShotWin31RegDatToRegistry( &State )) {
                    Flags |= WIN31_MIGRATE_REGDAT;
                    }
                }
            else {
                if (SnapShotWin31IniFilesToRegistry( &State )) {
                    Flags |= WIN31_MIGRATE_INIFILES;
                    }

                if (SnapShotWin31GroupsToRegistry( &State )) {
                    Flags |= WIN31_MIGRATE_GROUPS;
                    }
                }
            }
    except( Win31IOExceptionHandler( GetExceptionCode(),
                                     GetExceptionInformation(),
                                     &State
                                   )
          ) {
            BaseSetLastNTError( GetExceptionCode() );
            }
        }
    finally {
        TerminateWin31State( EventType, &State );
        }

    return Flags;
}


BOOL
WINAPI
SynchronizeWindows31FilesAndWindowsNTRegistry(
    IN WIN31IO_EVENT EventType,
    IN DWORD Flags,
    IN PWIN31IO_STATUS_CALLBACK StatusCallback,
    IN PVOID CallbackParameter
    )
{
    BOOL Result;
    VIRTUAL_BUFFER Buffer;
    ULONG cch;
    WIN31IO_STATE State;

    if (Flags == 0 || EventType == Win31LogoffEvent) {
        return TRUE;
        }

    if (!InitializeWin31State( EventType, &State )) {
        return TRUE;
        }
    State.QueryOnly = FALSE;
    State.StatusCallback = StatusCallback;
    State.CallbackParameter = CallbackParameter;

    Result = FALSE;
    try {
        try {
            try {
                cch = ((64 * 1024) / sizeof( WCHAR )) - 1;
                if (!CreateVirtualBuffer( &Buffer, cch * sizeof( WCHAR ), cch * sizeof( WCHAR ) )) {
                    leave;
                    }

                State.WorkBuffer = Buffer.Base;
                State.cchWorkBuffer = cch;

                Result = TRUE;
                if (EventType == Win31SystemStartEvent) {
                    if (Flags & WIN31_MIGRATE_INIFILES) {
                        Result &= SnapShotWin31IniFilesToRegistry( &State );
                        }

                    if (Flags & WIN31_MIGRATE_REGDAT) {
                        Result &= SnapShotWin31RegDatToRegistry( &State );
                        }
                    }
                else {
                    if (Flags & WIN31_MIGRATE_INIFILES) {
                        Result &= SnapShotWin31IniFilesToRegistry( &State );
                        }

                    if (Flags & WIN31_MIGRATE_GROUPS) {
                        Result &= SnapShotWin31GroupsToRegistry( &State );
                        }
                    }
                }
            except( VirtualBufferExceptionHandler( GetExceptionCode(),
                                                   GetExceptionInformation(),
                                                   &Buffer
                                                 )
                  ) {
                if (GetExceptionCode() == STATUS_ACCESS_VIOLATION) {
                    BaseSetLastNTError( STATUS_NO_MEMORY );
                    }
                else {
                    BaseSetLastNTError( GetExceptionCode() );
                    }

                Result = FALSE;
                }
            }
        except( Win31IOExceptionHandler( GetExceptionCode(),
                                         GetExceptionInformation(),
                                         &State
                                       )
              ) {
            BaseSetLastNTError( GetExceptionCode() );
            Result = FALSE;
            }
        }
    finally {
        TerminateWin31State( EventType, &State );
        FreeVirtualBuffer( &Buffer );
        }

    return Result;
}


BOOL
SnapShotWin31IniFileKey(
    IN OUT PWIN31IO_STATE State,
    IN PWSTR TempFileName,
    IN PWSTR FileName,
    IN PWSTR ApplicationName,
    IN PWSTR KeyName
    );

BOOL
SnapShotWin31IniFileSection(
    IN OUT PWIN31IO_STATE State,
    IN PWSTR TempFileName,
    IN PWSTR FileName,
    IN PWSTR ApplicationName
    );

BOOL
SnapShotWin31IniFileSections(
    IN OUT PWIN31IO_STATE State,
    IN PWSTR TempFileName,
    IN PWSTR FileName
    );

BOOL
SnapShotWin31IniFilesToRegistry(
    IN OUT PWIN31IO_STATE State
    )
{
    BOOL Result;
    PWSTR s, s1, FileName, ApplicationName, KeyName;
    ULONG n;
    PWSTR CurrentFileName;
    WCHAR Win31IniFileName[ MAX_PATH ];
    WCHAR TempIniFileName[ MAX_PATH ];
    HANDLE FindHandle;
    WIN32_FIND_DATAW FindFileData;
    HANDLE MigrationKey, Key;
    ULONG cchBufferUsed;

    MigrationKey = OpenCreateKey( State->Win31IOKey, L"IniFiles", !State->QueryOnly );
    if (State->QueryOnly) {
        if (MigrationKey != NULL) {
            NtClose( MigrationKey );
            return FALSE;
            }
        }
    else {
        if (MigrationKey == NULL) {
            return FALSE;
            }
        }

    Result = FALSE;
    wcscpy( State->FileNamePart, L"system.ini" );
    FindHandle = FindFirstFileW( State->WindowsPath, &FindFileData );
    if (FindHandle != INVALID_HANDLE_VALUE) {
        FindClose( FindHandle );
	if (FindFileData.nFileSizeLow > 1024) {
            Result = TRUE;
            }
        }

    if (!Result || State->QueryOnly) {
        if (MigrationKey == NULL) {
            MigrationKey = OpenCreateKey( State->Win31IOKey, L"IniFiles", TRUE );
            }

        if (MigrationKey != NULL) {
            NtClose( MigrationKey );
            }

        return Result;
        }

    cchBufferUsed = 0;
    if (State->UserRoot != NULL) {
        Result = QueryWin31IniFilesMappedToRegistry( WIN31_INIFILES_MAPPED_TO_USER,
                                                     State->WorkBuffer,
                                                     State->cchWorkBuffer,
                                                     &cchBufferUsed
                                                   );
        }
    else {
        Result = QueryWin31IniFilesMappedToRegistry( WIN31_INIFILES_MAPPED_TO_SYSTEM,
                                                     State->WorkBuffer,
                                                     State->cchWorkBuffer,
                                                     &cchBufferUsed
                                                   );
        }

    if (Result) {
        s = State->WorkBuffer;
        State->WorkBuffer += cchBufferUsed;
        State->cchWorkBuffer -= cchBufferUsed;
        CurrentFileName = NULL;
        TempIniFileName[ 0 ] = UNICODE_NULL;
        do {
            FileName = (PWSTR)((*s == UNICODE_NULL) ? NULL : s);
            while (*s++) {
                ;
                }
            ApplicationName = (PWSTR)((*s == UNICODE_NULL) ? NULL : s);
            while (*s++) {
                ;
                }
            KeyName = (PWSTR)((*s == UNICODE_NULL) ? NULL : s);
            while (*s++) {
                ;
                }

            if (FileName) {
                if (!CurrentFileName || _wcsicmp( FileName, CurrentFileName )) {
                    if (TempIniFileName[ 0 ] != UNICODE_NULL) {
                        WritePrivateProfileStringW( NULL, NULL, NULL, L"" );
                        if (!DeleteFileW( TempIniFileName )) {
                            KdPrint(( "WIN31IO: DeleteFile( %ws ) - failed (%u)\n",
                                      TempIniFileName,
                                      GetLastError()
                                   ));
                            }

                        TempIniFileName[ 0 ] = UNICODE_NULL;
                        }

                    CurrentFileName = NULL;
                    GetWindowsDirectoryW( Win31IniFileName, sizeof( Win31IniFileName ) );
                    wcscat( Win31IniFileName, L"\\" );
                    wcscat( Win31IniFileName, FileName );
                    wcscpy( TempIniFileName, Win31IniFileName );
                    _wcslwr( TempIniFileName );
                    s1 = wcsstr( TempIniFileName, L".ini" );
                    if (!s1) {
                        s1 = wcschr( TempIniFileName, UNICODE_NULL );
                        if (s1[-1] == L'.') {
                            s1--;
                            }
                        }
                    n = 0;
                    while (n < 1000) {
                        swprintf( s1, L".%03u", n++ );
                        if (CopyFileW( Win31IniFileName, TempIniFileName, TRUE )) {
                            if (State->StatusCallback != NULL) {
                                (State->StatusCallback)( FileName, State->CallbackParameter );
                                }
                            Key = OpenCreateKey( MigrationKey, FileName, TRUE );
                            if (Key != NULL) {
                                NtClose( Key );
                                }
                            CurrentFileName = FileName;
                            break;
                            }
                        else
                        if (GetLastError() != ERROR_FILE_EXISTS) {
                            if (GetLastError() != ERROR_FILE_NOT_FOUND) {
                                KdPrint(("WIN31IO: CopyFile( %ws, %ws ) failed - %u\n",
                                          Win31IniFileName,
                                          TempIniFileName,
                                          GetLastError()
                                       ));
                                }
                            break;
                            }
                        }

                    if (CurrentFileName == NULL) {
                        TempIniFileName[ 0 ] = UNICODE_NULL;
                        CurrentFileName = FileName;
                        }
                    }


                if (TempIniFileName[ 0 ] != UNICODE_NULL) {
                    if (ApplicationName) {
                        if (KeyName) {
                            if (SnapShotWin31IniFileKey( State,
                                                         TempIniFileName,
                                                         FileName,
                                                         ApplicationName,
                                                         KeyName
                                                       )
                               ) {
                                ReportWin31IOEvent( State,
                                                    EVENTLOG_INFORMATION_TYPE,
                                                    WIN31IO_EVENT_MIGRATE_INI_VARIABLE,
                                                    0,
                                                    NULL,
                                                    3,
                                                    KeyName,
                                                    ApplicationName,
                                                    FileName
                                                  );
                                }
                            else {
                                ReportWin31IOEvent( State,
                                                    EVENTLOG_WARNING_TYPE,
                                                    WIN31IO_EVENT_MIGRATE_INI_VARIABLE_FAILED,
                                                    0,
                                                    NULL,
                                                    3,
                                                    KeyName,
                                                    ApplicationName,
                                                    FileName
                                                  );
                                }

                            }
                        else {
                            if (SnapShotWin31IniFileSection( State,
                                                             TempIniFileName,
                                                             FileName,
                                                             ApplicationName
                                                           )
                               ) {
                                ReportWin31IOEvent( State,
                                                    EVENTLOG_INFORMATION_TYPE,
                                                    WIN31IO_EVENT_MIGRATE_INI_SECTION,
                                                    0,
                                                    NULL,
                                                    2,
                                                    ApplicationName,
                                                    FileName
                                                  );
                                }
                            else {
                                ReportWin31IOEvent( State,
                                                    EVENTLOG_WARNING_TYPE,
                                                    WIN31IO_EVENT_MIGRATE_INI_SECTION_FAILED,
                                                    0,
                                                    NULL,
                                                    2,
                                                    ApplicationName,
                                                    FileName
                                                  );
                                }

                            }
                        }
                    else {
                        if (SnapShotWin31IniFileSections( State,
                                                          TempIniFileName,
                                                          FileName
                                                        )
                            ) {
                            ReportWin31IOEvent( State,
                                                EVENTLOG_INFORMATION_TYPE,
                                                WIN31IO_EVENT_MIGRATE_INI_FILE,
                                                0,
                                                NULL,
                                                1,
                                                FileName
                                              );
                            }
                        else {
                            ReportWin31IOEvent( State,
                                                EVENTLOG_WARNING_TYPE,
                                                WIN31IO_EVENT_MIGRATE_INI_FILE_FAILED,
                                                0,
                                                NULL,
                                                1,
                                                FileName
                                              );
                            }
                        }
                    }
                }
            }
        while (*s != UNICODE_NULL);

        State->WorkBuffer -= cchBufferUsed;
        State->cchWorkBuffer += cchBufferUsed;
        }

    if (TempIniFileName[ 0 ] != UNICODE_NULL) {
        WritePrivateProfileStringW( NULL, NULL, NULL, L"" );
        if (!DeleteFileW( TempIniFileName )) {
            KdPrint(( "WIN31IO: DeleteFile( %ws ) - failed (%u)\n",
                      TempIniFileName,
                      GetLastError()
                   ));
            }
        }

    if (MigrationKey != NULL) {
        NtClose( MigrationKey );
        }
    return Result;
}



BOOL
SnapShotWin31IniFileKey(
    IN OUT PWIN31IO_STATE State,
    IN PWSTR TempFileName,
    IN PWSTR FileName,
    IN PWSTR ApplicationName,
    IN PWSTR KeyName
    )
{
    ULONG cch;

    cch = GetPrivateProfileStringW( ApplicationName,
                                    KeyName,
                                    NULL,
                                    State->WorkBuffer,
                                    State->cchWorkBuffer,
                                    TempFileName
                                  );
    if (cch != 0) {
        if (WritePrivateProfileStringW( ApplicationName,
                                        KeyName,
                                        State->WorkBuffer,
                                        FileName
                                      )
           ) {
            return TRUE;
            }
        else {
            KdPrint(( "WIN31IO: Copy to %ws [%ws].%ws == %ws (failed - %u)\n", FileName, ApplicationName, KeyName, State->WorkBuffer, GetLastError() ));
            return FALSE;
            }
        }
    else {
        return TRUE;
        }
}

BOOL
SnapShotWin31IniFileSection(
    IN OUT PWIN31IO_STATE State,
    IN PWSTR TempFileName,
    IN PWSTR FileName,
    IN PWSTR ApplicationName
    )
{
    BOOL Result;
    PWSTR KeyName;
    ULONG cch;

    Result = TRUE;
    cch = GetPrivateProfileStringW( ApplicationName,
                                    NULL,
                                    NULL,
                                    State->WorkBuffer,
                                    State->cchWorkBuffer,
                                    TempFileName
                                  );
    if (cch != 0) {
        KeyName = State->WorkBuffer;
        cch += 1;                   // Account for extra null
        State->WorkBuffer += cch;
        State->cchWorkBuffer -= cch;
        while (*KeyName != UNICODE_NULL) {
            Result &= SnapShotWin31IniFileKey( State,
                                               TempFileName,
                                               FileName,
                                               ApplicationName,
                                               KeyName
                                             );

            while (*KeyName++) {
                ;
                }
            }

        State->WorkBuffer -= cch;
        State->cchWorkBuffer += cch;
        }

    return Result;
}

BOOL
SnapShotWin31IniFileSections(
    IN OUT PWIN31IO_STATE State,
    IN PWSTR TempFileName,
    IN PWSTR FileName
    )
{
    BOOL Result;
    PWSTR ApplicationName;
    ULONG cch;

    Result = TRUE;
    cch = GetPrivateProfileStringW( NULL,
                                    NULL,
                                    NULL,
                                    State->WorkBuffer,
                                    State->cchWorkBuffer,
                                    TempFileName
                                  );
    if (cch != 0) {
        ApplicationName = State->WorkBuffer;
        cch += 1;                   // Account for extra null
        State->WorkBuffer += cch;
        State->cchWorkBuffer -= cch;
        while (*ApplicationName != UNICODE_NULL) {
            Result &= SnapShotWin31IniFileSection( State,
                                                   TempFileName,
                                                   FileName,
                                                   ApplicationName
                                                 );

            while (*ApplicationName++) {
                ;
                }
            }

        State->WorkBuffer -= cch;
        State->cchWorkBuffer += cch;
        }

    return Result;
}


BOOL
ConvertWindows31GroupsToRegistry(
    IN OUT PWIN31IO_STATE State,
    IN HANDLE MigrationKey,
    IN HANDLE CommonGroupsKey,
    IN HANDLE PersonalGroupsKey,
    IN PWSTR IniFileName,
    IN PWSTR GroupNames,
    IN ULONG nGroupNames
    );

BOOL
SnapShotWin31GroupsToRegistry(
    IN OUT PWIN31IO_STATE State
    )
{
    BOOL Result;
    PWSTR GroupNames, s;
    DWORD nGroupNames, cchGroupNames;
    HANDLE MigrationKey, PersonalGroupsKey, CommonGroupsKey;

    MigrationKey = OpenCreateKey( State->Win31IOKey, L"Groups", !State->QueryOnly );
    if (State->QueryOnly) {
        if (MigrationKey != NULL) {
            NtClose( MigrationKey );
            return FALSE;
            }
        }
    else {
        if (MigrationKey == NULL) {
            return FALSE;
            }
        }

    Result = FALSE;
    wcscpy( State->FileNamePart, L"progman.ini" );
    if (GetFileAttributesW( State->WindowsPath ) != 0xFFFFFFFF) {
        cchGroupNames = GetPrivateProfileStringW( L"Groups",
                                                  NULL,
                                                  L"",
                                                  State->WorkBuffer,
                                                  State->cchWorkBuffer,
                                                  State->WindowsPath
                                                );
        if (cchGroupNames != 0) {
            Result = TRUE;
            }
        }

    if (!Result || State->QueryOnly) {
        if (MigrationKey == NULL) {
            MigrationKey = OpenCreateKey( State->Win31IOKey, L"Groups", TRUE );
            }

        if (MigrationKey != NULL) {
            NtClose( MigrationKey );
            }

        return Result;
        }

    if (cchGroupNames) {
        PersonalGroupsKey = OpenCreateKey( State->UserRoot,
                                           L"UNICODE Program Groups",
                                           !State->QueryOnly
                                         );
        if (PersonalGroupsKey == NULL) {
            if (MigrationKey != NULL) {
                NtClose( MigrationKey );
                }

            return FALSE;
            }

        CommonGroupsKey = OpenCreateKey( NULL,
                                         L"\\Registry\\Machine\\Software\\Program Groups",
                                         FALSE
                                       );
        GroupNames = s = State->WorkBuffer;
        nGroupNames = 0;
        while (*s) {
            while (*s++) {
                ;
                }

            nGroupNames += 1;
            }
        State->WorkBuffer = s + 1;
        State->cchWorkBuffer -= cchGroupNames + 1;

        ConvertWindows31GroupsToRegistry( State,
                                          MigrationKey,
                                          CommonGroupsKey,
                                          PersonalGroupsKey,
                                          State->WindowsPath,
                                          GroupNames,
                                          nGroupNames
                                        );

        State->WorkBuffer = GroupNames,
        State->cchWorkBuffer += cchGroupNames + 1;

        NtClose( PersonalGroupsKey );
        if (CommonGroupsKey != NULL) {
            NtClose( CommonGroupsKey );
            }
        }

    if (MigrationKey != NULL) {
        NtClose( MigrationKey );
        }

    return TRUE;
}


BOOL
ConvertWindows31GroupsToRegistry(
    IN OUT PWIN31IO_STATE State,
    IN HANDLE MigrationKey,
    IN HANDLE CommonGroupsKey,
    IN HANDLE PersonalGroupsKey,
    IN PWSTR IniFileName,
    IN PWSTR GroupNames,
    IN ULONG nGroupNames
    )
{
    BOOL Result;
    NTSTATUS Status;
    PGROUP_DEF16 Group16;
    PGROUP_DEF Group32;
    UNICODE_STRING Group32Name;
    PWSTR Group16PathName;
    PWSTR Group16FileName;
    PWSTR s;
    ANSI_STRING AnsiString;
    ULONG NumberOfPersonalGroupNames;
    ULONG OldNumberOfPersonalGroupNames;
    HANDLE GroupNamesKey, SettingsKey, Key;

    NumberOfPersonalGroupNames = QueryNumberOfPersonalGroupNames( State->UserRoot,
                                                                  &GroupNamesKey,
                                                                  &SettingsKey
                                                                );
    OldNumberOfPersonalGroupNames = NumberOfPersonalGroupNames;

    Result = TRUE;
    while (*GroupNames) {
        //
        // Get the group file (.grp) name
        //
        if (GetPrivateProfileStringW( L"Groups",
                                      GroupNames,
                                      L"",
                                      State->WorkBuffer,
                                      State->cchWorkBuffer,
                                      IniFileName
                                    )
           ) {
            Group16PathName = State->WorkBuffer;
            Group16 = LoadGroup16( Group16PathName );
            if (Group16 != NULL) {
                Group16FileName = Group16PathName + wcslen( Group16PathName );
                while (Group16FileName > Group16PathName) {
                    if (Group16FileName[ -1 ] == OBJ_NAME_PATH_SEPARATOR) {
                        break;
                        }

                    Group16FileName -= 1;
                    }

                //
                // Get the group name.
                //
                if (Group16->pName == 0) {
                    RtlInitUnicodeString( &Group32Name, Group16FileName );
                    Status = STATUS_SUCCESS;
                    }
                else {
                    RtlInitAnsiString( &AnsiString, (PSZ)PTR( Group16, Group16->pName ) );
                    Status = RtlAnsiStringToUnicodeString( &Group32Name, &AnsiString, TRUE );
                    s = Group32Name.Buffer;
                    while (*s) {
                        if (*s == OBJ_NAME_PATH_SEPARATOR) {
                            *s = L'/';
                            }
                        s += 1;
                        }
                    }

                if (NT_SUCCESS( Status )) {
                    if (DoesExistGroup( PersonalGroupsKey, Group32Name.Buffer )) {
                        ReportWin31IOEvent( State,
                                            EVENTLOG_INFORMATION_TYPE,
                                            WIN31IO_EVENT_MIGRATE_GROUP_EXISTS,
                                            0,
                                            NULL,
                                            2,
                                            Group16PathName,
                                            Group32Name.Buffer
                                          );
                        }
                    else
                    if (DoesExistGroup( CommonGroupsKey, Group32Name.Buffer )) {
                        ReportWin31IOEvent( State,
                                            EVENTLOG_INFORMATION_TYPE,
                                            WIN31IO_EVENT_MIGRATE_GROUP_EXISTS,
                                            0,
                                            NULL,
                                            2,
                                            Group16PathName,
                                            Group32Name.Buffer
                                          );
                        }
                    else {
                        // DumpGroup16( State->WorkBuffer, Group16 );
                        Group32 = CreateGroupFromGroup16( AnsiString.Buffer, Group16 );
                        if (Group32 != NULL) {
                            if (Group32 == (PGROUP_DEF)-1) {
                                ReportWin31IOEvent( State,
                                                    EVENTLOG_WARNING_TYPE,
                                                    WIN31IO_EVENT_MIGRATE_GROUP_FAILED4,
                                                    0,
                                                    NULL,
                                                    1,
                                                    Group32Name.Buffer
                                                  );
                                Group32 = NULL;
                                }
                            else {
                                // DumpGroup( Group32Name.Buffer, Group32 );
                                if (!SaveGroup( PersonalGroupsKey,
                                                Group32Name.Buffer,
                                                Group32
                                              )
                                   ) {
                                    ReportWin31IOEvent( State,
                                                        EVENTLOG_WARNING_TYPE,
                                                        WIN31IO_EVENT_MIGRATE_GROUP_FAILED,
                                                        0,
                                                        NULL,
                                                        1,
                                                        Group32Name.Buffer
                                                      );
                                    }
                                else
                                if (!NewPersonalGroupName( GroupNamesKey,
                                                           Group32Name.Buffer,
                                                           NumberOfPersonalGroupNames+1
                                                         )
                                   ) {
                                    DeleteGroup( PersonalGroupsKey, Group32Name.Buffer );
                                    ReportWin31IOEvent( State,
                                                        EVENTLOG_WARNING_TYPE,
                                                        WIN31IO_EVENT_MIGRATE_GROUP_FAILED,
                                                        0,
                                                        NULL,
                                                        1,
                                                        Group32Name.Buffer
                                                      );
                                    }
                                else {
                                    if (State->StatusCallback != NULL) {
                                        (State->StatusCallback)( Group16FileName, State->CallbackParameter );
                                        }
                                    ReportWin31IOEvent( State,
                                                        EVENTLOG_INFORMATION_TYPE,
                                                        WIN31IO_EVENT_MIGRATE_GROUP,
                                                        0,
                                                        NULL,
                                                        1,
                                                        Group32Name.Buffer
                                                      );
                                    NumberOfPersonalGroupNames += 1;
                                    Key = OpenCreateKey( MigrationKey, Group16FileName, TRUE );
                                    if (Key != NULL) {
                                        NtClose( Key );
                                        }
                                    else {
                                        KdPrint(("WIN31IO: (3)Unable to create sub migration key for %ws (%u)\n", State->WorkBuffer, GetLastError() ));
                                        }
                                    }

                                UnloadGroup( Group32 );
                                }
                            }
                        else {
                            WCHAR ErrorCode[ 32 ];

                            _snwprintf( ErrorCode,
                                         sizeof( ErrorCode ) / sizeof( WCHAR ),
                                         L"%u",
                                         GetLastError()
                                       );
                            ReportWin31IOEvent( State,
                                                EVENTLOG_WARNING_TYPE,
                                                WIN31IO_EVENT_MIGRATE_GROUP_FAILED1,
                                                0,
                                                NULL,
                                                2,
                                                State->WorkBuffer,
                                                ErrorCode
                                              );
                            }
                        }

                    RtlFreeUnicodeString( &Group32Name );
                    }

                UnloadGroup16( Group16 );
                }
            else {
                WCHAR ErrorCode[ 32 ];

                _snwprintf( ErrorCode,
                             sizeof( ErrorCode ) / sizeof( WCHAR ),
                             L"%u",
                             GetLastError()
                           );
                ReportWin31IOEvent( State,
                                    EVENTLOG_WARNING_TYPE,
                                    WIN31IO_EVENT_MIGRATE_GROUP_FAILED1,
                                    0,
                                    NULL,
                                    2,
                                    State->WorkBuffer,
                                    ErrorCode
                                  );
                }
            }
        else {
            ReportWin31IOEvent( State,
                                EVENTLOG_WARNING_TYPE,
                                WIN31IO_EVENT_MIGRATE_GROUP_FAILED2,
                                0,
                                NULL,
                                2,
                                IniFileName,
                                GroupNames
                              );
            }

        while (*GroupNames++) {
            ;
            }
        }


    if (OldNumberOfPersonalGroupNames != NumberOfPersonalGroupNames) {
        UNICODE_STRING ValueName;
        ULONG ValueData = TRUE;

        RtlInitUnicodeString( &ValueName, L"InitialArrange" );
        Status = NtSetValueKey( SettingsKey,
                                &ValueName,
                                0,
                                REG_DWORD,
                                &ValueData,
                                sizeof( ValueData )
                              );
#if DBG
        if (!NT_SUCCESS( Status )) {
            KdPrint(( "WIN31IO: Unable to set value of %wZ - Status == %x\n", &ValueName, Status ));
            }
#endif
        }

    NtClose( SettingsKey );
    NtClose( GroupNamesKey );
    return Result;
}


BOOL
SnapShotWin31RegDatToRegistry(
    IN OUT PWIN31IO_STATE State
    )
{
    BOOL Result;
    HANDLE MigrationKey;
    PREG_HEADER16 RegDat16;

    MigrationKey = OpenCreateKey( State->Win31IOKey, L"REG.DAT", !State->QueryOnly );
    if (State->QueryOnly) {
        if (MigrationKey != NULL) {
            NtClose( MigrationKey );
            return FALSE;
            }
        }
    else {
        if (MigrationKey == NULL) {
            return FALSE;
            }
        }

    Result = FALSE;
    wcscpy( State->FileNamePart, L"reg.dat" );
    if (GetFileAttributesW( State->WindowsPath ) != 0xFFFFFFFF) {
        Result = TRUE;
        }

    if (!Result || State->QueryOnly) {
        if (MigrationKey == NULL) {
            MigrationKey = OpenCreateKey( State->Win31IOKey, L"REG.DAT", TRUE );
            }

        if (MigrationKey != NULL) {
            NtClose( MigrationKey );
            }

        return Result;
        }

    RegDat16 = LoadRegistry16( State->WindowsPath );
    if (RegDat16 != NULL) {
        if (State->StatusCallback != NULL) {
            (State->StatusCallback)( State->WindowsPath, State->CallbackParameter );
            }

        if (CreateRegistryClassesFromRegistry16( State->SoftwareRoot, RegDat16 )) {
            ReportWin31IOEvent( State,
                                EVENTLOG_INFORMATION_TYPE,
                                WIN31IO_EVENT_MIGRATE_REGDAT,
                                0,
                                NULL,
                                1,
                                State->WindowsPath
                              );
            }
        else {
            ReportWin31IOEvent( State,
                                EVENTLOG_WARNING_TYPE,
                                WIN31IO_EVENT_MIGRATE_REGDAT_FAILED,
                                0,
                                NULL,
                                1,
                                State->WindowsPath
                              );
            }
        UnloadRegistry16( RegDat16 );
        }

    if (MigrationKey != NULL) {
        NtClose( MigrationKey );
        }

    return TRUE;
}


#define SERVICE_TO_WAIT_FOR "EventLog"
#define MAX_TICKS_WAIT   90000   // ms

BOOL
WaitForEventLogToStart( VOID )
{
    BOOL bStarted = FALSE;
    DWORD StartTickCount;
    DWORD dwOldCheckPoint = (DWORD)-1;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;
    SERVICE_STATUS ServiceStatus;

    if ((hScManager = OpenSCManager( NULL,
                                     NULL,
                                     SC_MANAGER_CONNECT
                                   )
        ) == (SC_HANDLE) NULL
       ) {
        KdPrint(("WIN31IO: IsNetworkStarted: OpenSCManager failed, error = %d\n", GetLastError()));
        goto Exit;
    }

    //
    // OpenService
    //

    if ((hService = OpenService( hScManager,
                                 SERVICE_TO_WAIT_FOR,
                                 SERVICE_QUERY_STATUS
                               )
        ) == (SC_HANDLE) NULL
       ) {
        KdPrint(("WIN31IO: IsNetworkStarted: OpenService failed, error = %d\n", GetLastError()));
        goto Exit;
        }

    //
    // Loop until the service starts or we think it never will start
    // or we've exceeded our maximum time delay.
    //

    StartTickCount = GetTickCount();
    while (!bStarted) {

        if ((GetTickCount() - StartTickCount) > MAX_TICKS_WAIT) {
            KdPrint(("WIN31IO: Max wait exceeded waiting for service <%s> to start\n", SERVICE_TO_WAIT_FOR));
            break;
            }

        if (!QueryServiceStatus( hService, &ServiceStatus )) {
            KdPrint(("WIN31IO: IsNetworkStarted: QueryServiceStatus failed, error = %d\n", GetLastError()));
            break;
            }

        if (ServiceStatus.dwCurrentState == SERVICE_STOPPED) {
            KdPrint(("WIN31IO: Service STOPPED"));

            if (ServiceStatus.dwWin32ExitCode == ERROR_SERVICE_NEVER_STARTED) {
                KdPrint(("WIN31IO: Waiting for 3 secs"));
                Sleep(3000);
                }
            else {
                KdPrint(("WIN31IO: Service exit code = %d, returning failure\n", ServiceStatus.dwWin32ExitCode));
                break;
                }
            }
        else
        if ( (ServiceStatus.dwCurrentState == SERVICE_RUNNING) ||
             (ServiceStatus.dwCurrentState == SERVICE_CONTINUE_PENDING) ||
             (ServiceStatus.dwCurrentState == SERVICE_PAUSE_PENDING) ||
             (ServiceStatus.dwCurrentState == SERVICE_PAUSED)
           ) {
            bStarted = TRUE;
            }
        else
        if (ServiceStatus.dwCurrentState == SERVICE_START_PENDING) {

            //
            // Wait to give a chance for the network to start.
            //

            Sleep( ServiceStatus.dwWaitHint );
            }
        else {
            KdPrint(( "WIN31IO: Service in unknown state : %d\n", ServiceStatus.dwCurrentState ));
            }
        }

Exit:
    if (hScManager != NULL) {
        CloseServiceHandle( hScManager );
    }
    if (hService != NULL) {
        CloseServiceHandle( hService );
    }

    return( bStarted );
}


HANDLE
OpenCreateKey(
    IN HANDLE Root,
    IN PWSTR Path,
    IN BOOL WriteAccess
    )
{
    NTSTATUS Status;
    HANDLE Key;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG CreateDisposition;

    RtlInitUnicodeString( &KeyName, Path );
    InitializeObjectAttributes( &ObjectAttributes,
                                &KeyName,
                                OBJ_CASE_INSENSITIVE,
                                Root,
                                NULL
                              );
    Status = NtOpenKey( &Key,
                        WriteAccess ? (STANDARD_RIGHTS_WRITE |
                                          KEY_QUERY_VALUE |
                                          KEY_ENUMERATE_SUB_KEYS |
                                          KEY_SET_VALUE |
                                          KEY_CREATE_SUB_KEY
                                      )
                                    : GENERIC_READ,
                        &ObjectAttributes
                      );
    if (NT_SUCCESS( Status )) {
        return Key;
        }
    else
    if (!WriteAccess) {
        BaseSetLastNTError( Status );
        return NULL;
        }

    Status = NtCreateKey( &Key,
                          STANDARD_RIGHTS_WRITE |
                            KEY_QUERY_VALUE |
                            KEY_ENUMERATE_SUB_KEYS |
                            KEY_SET_VALUE |
                            KEY_CREATE_SUB_KEY,
                          &ObjectAttributes,
                          0,
                          NULL,
                          0,
                          &CreateDisposition
                        );
    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError( Status );
        return NULL;
        }
    else {
        return Key;
        }
}
