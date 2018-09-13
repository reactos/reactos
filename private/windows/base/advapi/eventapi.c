/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    EVENTAPI.C

Abstract:

    This module contains the client ends of the EventLog APIs.

Author:

    Rajen Shah  (rajens)    24-Aug-1991


Revision History:


--*/

#include "advapi.h"

static WCHAR wszDosDevices[] = L"\\DosDevices\\";

ULONG
BaseSetLastNTError(
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This API sets the "last error value" and the "last error string"
    based on the value of Status. For status codes that don't have
    a corresponding error string, the string is set to null.

Arguments:

    Status - Supplies the status value to store as the last error value.

Return Value:

    The corresponding Win32 error code that was stored in the
    "last error value" thread variable.

--*/

{
    ULONG dwErrorCode;

    dwErrorCode = RtlNtStatusToDosError( Status );
    SetLastError( dwErrorCode );
    return( dwErrorCode );
}


BOOL
InitAnsiString(
    OUT PANSI_STRING DestinationString,
    IN PCSZ SourceString OPTIONAL
    )

/*++

Routine Description:

    The RtlInitAnsiString function initializes an NT counted string.
    The DestinationString is initialized to point to the SourceString
    and the Length and MaximumLength fields of DestinationString are
    initialized to the length of the SourceString, which is zero if
    SourceString is not specified.

    This is RtlInitAnsiString with a return status that rejects strings
    that are greater than 64K bytes.

Arguments:

    DestinationString - Pointer to the counted string to initialize

    SourceString - Optional pointer to a null terminated string that
        the counted string is to point to.


Return Value:

    None.

--*/

{
    ULONG Length = 0;
    DestinationString->Length = 0;
    DestinationString->Buffer = (PCHAR)SourceString;
    if (ARGUMENT_PRESENT( SourceString )) {
        while (*SourceString++) {
            Length++;
        }

        //
        // Make sure the length won't overflow a USHORT when converted to
        // UNICODE characters
        //

        if (Length * sizeof(WCHAR) > 0xFFFF) {
            return(FALSE);
        }

        DestinationString->Length = (USHORT) Length;
        DestinationString->MaximumLength = (USHORT) (Length + 1);

    }
    else {
        DestinationString->MaximumLength = 0;
        DestinationString->Length = 0;
    }

    return(TRUE);
}


BOOL
InitUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString OPTIONAL
    )

/*++

Routine Description:

    The InitUnicodeString function initializes an NT counted
    unicode string.  The DestinationString is initialized to point to
    the SourceString and the Length and MaximumLength fields of
    DestinationString are initialized to the length of the SourceString,
    which is zero if SourceString is not specified.

    This is RtlInitUnicodeString with a return status that rejects strings
    that are greater than 64K bytes.

Arguments:

    DestinationString - Pointer to the counted string to initialize

    SourceString - Optional pointer to a null terminated unicode string that
        the counted string is to point to.


Return Value:

    None.

--*/

{
    ULONG Length = 0;
    DestinationString->Length = 0;
    DestinationString->Buffer = (PWSTR)SourceString;
    if (ARGUMENT_PRESENT( SourceString )) {
        while (*SourceString++) {
            Length += sizeof(*SourceString);
        }

        //
        // Make sure the length won't overflow a USHORT
        //

        if (Length > 0xFFFF) {
            return(FALSE);
        }

        DestinationString->Length = (USHORT) Length;
        DestinationString->MaximumLength =
            (USHORT) Length + (USHORT) sizeof(UNICODE_NULL);
    }
    else {
        DestinationString->MaximumLength = 0;
        DestinationString->Length = 0;
    }

    return(TRUE);
}

//
// Single version API's (no strings)
//

BOOL
CloseEventLog (
    HANDLE hEventLog
    )

/*++

Routine Description:

  This is the client DLL entry point for the WinCloseEventLog API.
  It closes the RPC binding, and frees any memory allocated for the
  handle.

  NOTE that there is no equivalent call for ANSI.

Arguments:

    LogHandle       - Handle returned from a previous "Open" call.


Return Value:

    Returns TRUE if success, FALSE otherwise.


--*/
{
    NTSTATUS Status;
    BOOL ReturnValue;

    Status = ElfCloseEventLog (hEventLog);

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        ReturnValue = FALSE;
    } else {
        ReturnValue = TRUE;
    }
    return ReturnValue;

}



BOOL
DeregisterEventSource (
    HANDLE hEventLog
    )

/*++

Routine Description:

  This is the client DLL entry point for the DeregisterEventSource API.
  It closes the RPC binding, and frees any memory allocated for the
  handle.

  NOTE that there is no equivalent call for ANSI.

Arguments:

    LogHandle       - Handle returned from a previous "Open" call.


Return Value:

    Returns TRUE if success, FALSE otherwise.


--*/
{
    NTSTATUS Status;
    BOOL ReturnValue;

    Status = ElfDeregisterEventSource (hEventLog);

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        ReturnValue = FALSE;
    } else {
        ReturnValue = TRUE;
    }
    return ReturnValue;

}

BOOL
NotifyChangeEventLog(
    HANDLE  hEventLog,
    HANDLE  hEvent
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    NTSTATUS Status;

    Status = ElfChangeNotify(hEventLog,hEvent);

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return(FALSE);
    } else {
        return(TRUE);
    }
}

BOOL
GetNumberOfEventLogRecords (
    HANDLE hEventLog,
    PDWORD NumberOfRecords
    )

/*++

Routine Description:

  This is the client DLL entry point that returns the number of records in
  the eventlog specified by hEventLog.

  NOTE that there is no equivalent call for ANSI.

Arguments:

    LogHandle       - Handle returned from a previous "Open" call.
    NumberOfRecords - Pointer to a DWORD to place the number of records.


Return Value:

    Returns TRUE if success, FALSE otherwise.


--*/
{
    NTSTATUS Status;
    BOOL ReturnValue;

    Status = ElfNumberOfRecords (hEventLog, NumberOfRecords);

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        ReturnValue = FALSE;
    } else {
        ReturnValue = TRUE;
    }
    return ReturnValue;

}



BOOL
GetOldestEventLogRecord (
    HANDLE hEventLog,
    PDWORD OldestRecord
    )

/*++

Routine Description:

  This is the client DLL entry point that returns the record number of the
  oldest record in the eventlog specified by hEventLog.

  NOTE that there is no equivalent call for ANSI.

Arguments:

    LogHandle       - Handle returned from a previous "Open" call.
    OldestRecord    - Pointer to a DWORD to place the record number of the
                      oldest record in the eventlog specified by hEventLog


Return Value:

    Returns TRUE if success, FALSE otherwise.


--*/
{
    NTSTATUS Status;
    BOOL ReturnValue;

    Status = ElfOldestRecord (hEventLog, OldestRecord);

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        ReturnValue = FALSE;
    } else {
        ReturnValue = TRUE;
    }
    return ReturnValue;

}


BOOL
GetEventLogInformation (
    HANDLE    hEventLog,
    DWORD     dwInfoLevel,
    PVOID     lpBuffer,
    DWORD     cbBufSize,
    LPDWORD   pcbBytesNeeded
    )

/*++

Routine Description:

  This is the client DLL entry point that returns information about
  the eventlog specified by hEventLog.

Arguments:

    LogHandle       - Handle returned from a previous "Open" call.
    dwInfoLevel     - Which information to return
    lpBuffer        - Pointer to buffer to hold information
    cbBufSize       - Size of buffer, in bytes
    pcbBytesNeeded  - Number of bytes needed

Return Value:

    Returns TRUE if success, FALSE otherwise.


--*/
{
    NTSTATUS ntStatus;

    ntStatus = ElfGetLogInformation(hEventLog,
                                    dwInfoLevel,
                                    lpBuffer,
                                    cbBufSize,
                                    pcbBytesNeeded);

    if (!NT_SUCCESS(ntStatus)) {
        BaseSetLastNTError(ntStatus);
        return FALSE;
    }

    return TRUE;
}


//
// UNICODE APIs
//

BOOL
ClearEventLogW (
    HANDLE hEventLog,
    LPCWSTR BackupFileName
    )

/*++

Routine Description:

  This is the client DLL entry point for the ClearEventLogFile API.
  The call is passed to the eventlog service on the appropriate server
  identified by LogHandle.


Arguments:

    LogHandle       - Handle returned from a previous "Open" call. This is
                      used to identify the module and the server.

    BackupFileName  - Name of the file to back up the current log file.
                      NULL implies not to back up the file.


Return Value:

    TRUE if success, FALSE otherwise.


--*/
{

    UNICODE_STRING Unicode;
    UNICODE_STRING DLUnicode;   // Downlevel NT filename.
    NTSTATUS Status;
    BOOL ReturnValue;

    //
    // Turn the Dos filename into an NT filename
    //

    if (BackupFileName) {
        ReturnValue = RtlDosPathNameToNtPathName_U(BackupFileName, &Unicode, NULL, NULL);
        if (!BackupFileName || !ReturnValue) {
           SetLastError(ERROR_INVALID_PARAMETER);
           return(FALSE);
        }
    }
    else {
        Unicode.Length = 0;
        Unicode.MaximumLength = 0;
        Unicode.Buffer = NULL;
    }

    Status = ElfClearEventLogFileW (hEventLog, &Unicode);

    //
    // With NT 4.0, NT filenames are preceeded with \?? vs. \DosDevices
    // in 3.51. This retry logic exists for 3.51 machines which don't
    // recognize NT 4.0 filenames. The API should have passed Windows
    // filenames vs NT.
    //

    if (Status == STATUS_OBJECT_PATH_NOT_FOUND && BackupFileName != NULL) {
        DLUnicode.MaximumLength = (wcslen(BackupFileName) * sizeof(WCHAR)) +
                                            sizeof(wszDosDevices);

        DLUnicode.Buffer = RtlAllocateHeap(
                                RtlProcessHeap(), 0,
                                DLUnicode.MaximumLength);

        if (DLUnicode.Buffer != NULL) {
            wcscpy(DLUnicode.Buffer, wszDosDevices);
            wcscat(DLUnicode.Buffer, BackupFileName);
            DLUnicode.Length = DLUnicode.MaximumLength - sizeof(UNICODE_NULL);

            Status = ElfClearEventLogFileW (hEventLog, &DLUnicode);
            RtlFreeHeap(RtlProcessHeap(), 0, DLUnicode.Buffer);
        }
        else {
            Status = STATUS_NO_MEMORY;
        }
    }

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        ReturnValue = FALSE;
    } else {
        ReturnValue = TRUE;
    }

    if (Unicode.MaximumLength) {
        RtlFreeHeap(RtlProcessHeap(), 0, Unicode.Buffer);
    }
    return ReturnValue;

}



BOOL
BackupEventLogW (
    HANDLE hEventLog,
    LPCWSTR BackupFileName
    )

/*++

Routine Description:

  This is the client DLL entry point for the BackupEventLogFile API.
  The call is passed to the eventlog service on the appropriate server
  identified by LogHandle.


Arguments:

    LogHandle       - Handle returned from a previous "Open" call. This is
                      used to identify the module and the server.

    BackupFileName  - Name of the file to back up the current log file.


Return Value:

    TRUE if success, FALSE otherwise.


--*/
{

    UNICODE_STRING Unicode;
    UNICODE_STRING DLUnicode;   // Downlevel NT filename.
    NTSTATUS Status;
    BOOL ReturnValue = TRUE;

    //
    // Turn the Dos filename into an NT filename
    //

    if (BackupFileName) {
        ReturnValue = RtlDosPathNameToNtPathName_U(BackupFileName, &Unicode,
            NULL, NULL);
    }

    if (!BackupFileName || !ReturnValue) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    Status = ElfBackupEventLogFileW (hEventLog, &Unicode);

    //
    // With NT 4.0, NT filenames are preceeded with \?? vs. \DosDevices
    // in 3.51. This retry logic exists for 3.51 machines which don't
    // recognize NT 4.0 filenames. The API should have passed Windows
    // filenames vs NT.
    //

    if (Status == STATUS_OBJECT_PATH_NOT_FOUND && BackupFileName != NULL) {
        DLUnicode.MaximumLength = (wcslen(BackupFileName) * sizeof(WCHAR)) +
                                            sizeof(wszDosDevices);

        DLUnicode.Buffer = RtlAllocateHeap(
                                RtlProcessHeap(), 0,
                                DLUnicode.MaximumLength);

        if (DLUnicode.Buffer != NULL) {
            wcscpy(DLUnicode.Buffer, wszDosDevices);
            wcscat(DLUnicode.Buffer, BackupFileName);
            DLUnicode.Length = DLUnicode.MaximumLength - sizeof(UNICODE_NULL);

            Status = ElfBackupEventLogFileW (hEventLog, &DLUnicode);
            RtlFreeHeap(RtlProcessHeap(), 0, DLUnicode.Buffer);
        }
        else {
            Status = STATUS_NO_MEMORY;
        }
    }

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        ReturnValue = FALSE;
    } else {
        ReturnValue = TRUE;
    }

    if (Unicode.MaximumLength) {
        RtlFreeHeap(RtlProcessHeap(), 0, Unicode.Buffer);
    }
    return ReturnValue;

}


HANDLE
OpenEventLogW (
    LPCWSTR  UNCServerName,
    LPCWSTR  ModuleName
    )

/*++

Routine Description:

    This is the client DLL entry point for the WinOpenEventLog API.

    It creates an RPC binding for the server specified, and stores that
    and additional data away. It returns a handle to the caller that can
    be used to later on access the handle-specific information.

Arguments:

    UNCServerName     - Server with which to bind for subsequent operations.

    ModuleName        - Supplies the name of the module to associate with
                        this handle.


Return Value:

    Returns a handle that can be used for subsequent Win API calls. If
    the handle is NULL, then an error occurred.


--*/
{

    UNICODE_STRING Unicode;
    UNICODE_STRING UnicodeModuleName;
    HANDLE LogHandle;
    NTSTATUS Status;
    HANDLE ReturnHandle;

    RtlInitUnicodeString(&UnicodeModuleName,ModuleName);
    RtlInitUnicodeString(&Unicode, UNCServerName);

    Status = ElfOpenEventLogW (
                        &Unicode,
                        &UnicodeModuleName,
                        &LogHandle
                        );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        ReturnHandle = (HANDLE)NULL;
    } else {
        ReturnHandle = (HANDLE)LogHandle;
    }

    return ReturnHandle;
}


HANDLE
RegisterEventSourceW (
    LPCWSTR  UNCServerName,
    LPCWSTR  ModuleName
    )

/*++

Routine Description:

    This is the client DLL entry point for the RegisterEventSource API.

    It creates an RPC binding for the server specified, and stores that
    and additional data away. It returns a handle to the caller that can
    be used to later on access the handle-specific information.

Arguments:

    UNCServerName     - Server with which to bind for subsequent operations.

    ModuleName        - Supplies the name of the module to associate with
                        this handle.


Return Value:

    Returns a handle that can be used for subsequent Win API calls. If
    the handle is NULL, then an error occurred.


--*/
{

    UNICODE_STRING Unicode;
    UNICODE_STRING UnicodeModuleName;
    HANDLE LogHandle;
    NTSTATUS Status;
    HANDLE ReturnHandle;

    RtlInitUnicodeString(&UnicodeModuleName,ModuleName);
    RtlInitUnicodeString(&Unicode, UNCServerName);

    Status = ElfRegisterEventSourceW (
                        &Unicode,
                        &UnicodeModuleName,
                        &LogHandle
                        );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        ReturnHandle = (HANDLE)NULL;
    } else {
        ReturnHandle = (HANDLE)LogHandle;
    }

    return ReturnHandle;
}


HANDLE
OpenBackupEventLogW (
    LPCWSTR  UNCServerName,
    LPCWSTR  FileName
    )

/*++

Routine Description:

    This is the client DLL entry point for the OpenBackupEventLog API.

    It creates an RPC binding for the server specified, and stores that
    and additional data away. It returns a handle to the caller that can
    be used to later on access the handle-specific information.

Arguments:

    UNCServerName   - Server with which to bind for subsequent operations.

    FileName        - Supplies the filename of the logfile to associate with
                      this handle.


Return Value:

    Returns a handle that can be used for subsequent Win API calls. If
    the handle is NULL, then an error occurred.


--*/
{

    UNICODE_STRING Unicode;
    UNICODE_STRING UnicodeFileName;
    UNICODE_STRING DLUnicode;   // Downlevel NT filename.
    HANDLE LogHandle;
    NTSTATUS Status;
    HANDLE ReturnHandle;

    RtlInitUnicodeString(&Unicode, UNCServerName);

    //
    // Turn the Dos filename into an NT filename
    //

    if (FileName) {
        RtlDosPathNameToNtPathName_U(FileName, &UnicodeFileName, NULL, NULL);
    }
    else {
        RtlInitUnicodeString(&UnicodeFileName, NULL);
    }

    Status = ElfOpenBackupEventLogW (
                        &Unicode,
                        &UnicodeFileName,
                        &LogHandle
                        );

    //
    // With NT 4.0, NT filenames are preceeded with \?? vs. \DosDevices
    // in 3.51. This retry logic exists for 3.51 machines which don't
    // recognize NT 4.0 filenames. The API should have passed Windows
    // filenames vs NT.
    //

    if (Status == STATUS_OBJECT_PATH_NOT_FOUND && FileName != NULL) {
        DLUnicode.MaximumLength = (wcslen(FileName) * sizeof(WCHAR)) +
                                            sizeof(wszDosDevices);

        DLUnicode.Buffer = RtlAllocateHeap(
                                RtlProcessHeap(), 0,
                                DLUnicode.MaximumLength);

        if (DLUnicode.Buffer != NULL) {
            wcscpy(DLUnicode.Buffer, wszDosDevices);
            wcscat(DLUnicode.Buffer, FileName);
            DLUnicode.Length = DLUnicode.MaximumLength - sizeof(UNICODE_NULL);

            Status = ElfOpenBackupEventLogW (
                                &Unicode,
                                &DLUnicode,
                                &LogHandle
                                );
            RtlFreeHeap(RtlProcessHeap(), 0, DLUnicode.Buffer);
        }
        else {
            Status = STATUS_NO_MEMORY;
        }
    }

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        ReturnHandle = (HANDLE)NULL;
    } else {
        ReturnHandle = (HANDLE)LogHandle;
    }

    if (UnicodeFileName.MaximumLength) {
        RtlFreeHeap(RtlProcessHeap(), 0, UnicodeFileName.Buffer);
    }
    return ReturnHandle;
}





BOOL
ReadEventLogW (
    HANDLE      hEventLog,
    DWORD       dwReadFlags,
    DWORD       dwRecordOffset,
    LPVOID      lpBuffer,
    DWORD       nNumberOfBytesToRead,
    DWORD       *pnBytesRead,
    DWORD       *pnMinNumberOfBytesNeeded
    )

/*++

Routine Description:

  This is the client DLL entry point for the WinreadEventLog API.

Arguments:



Return Value:

    Returns count of bytes read. Zero of none read.


--*/
{

    NTSTATUS Status;
    BOOL ReturnValue;

    Status = ElfReadEventLogW (
                        hEventLog,
                        dwReadFlags,
                        dwRecordOffset,
                        lpBuffer,
                        nNumberOfBytesToRead,
                        pnBytesRead,
                        pnMinNumberOfBytesNeeded
                        );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        ReturnValue = FALSE;
    } else {
        ReturnValue = TRUE;
    }
    return ReturnValue;

}



BOOL
ReportEventW (
    HANDLE      hEventLog,
    WORD        wType,
    WORD        wCategory       OPTIONAL,
    DWORD       dwEventID,
    PSID        lpUserSid       OPTIONAL,
    WORD        wNumStrings,
    DWORD       dwDataSize,
    LPCWSTR     *lpStrings      OPTIONAL,
    LPVOID      lpRawData       OPTIONAL
    )

/*++

Routine Description:

  This is the client DLL entry point for the ReportEvent API.

Arguments:


Return Value:

    Returns TRUE if success, FALSE otherwise.


--*/
{

    NTSTATUS Status = STATUS_SUCCESS;
    BOOL ReturnValue;
    PUNICODE_STRING  *pUStrings;
    ULONG   i;
    ULONG AllocatedStrings;

    //
    // Convert the array of strings to an array of PUNICODE_STRINGs
    // before calling ElfReportEventW.
    //
    pUStrings = RtlAllocateHeap(
                            RtlProcessHeap(), 0,
                            wNumStrings * sizeof(PUNICODE_STRING)
                            );

    if (pUStrings) {

        //
        // Guard the memory allocation above while we peruse the user's
        // buffer. If not, we'd leak it on an exception.
        //

        try {
            //
            // For each string passed in, allocate a UNICODE_STRING structure
            // and set it to the matching string.
            //
            for (AllocatedStrings = 0; AllocatedStrings < wNumStrings;
              AllocatedStrings++) {
                pUStrings[AllocatedStrings] = RtlAllocateHeap(
                                RtlProcessHeap(), 0,
                                sizeof(UNICODE_STRING)
                                );

                if (pUStrings[AllocatedStrings]) {

                    if (!InitUnicodeString(
                                pUStrings[AllocatedStrings],
                                lpStrings[AllocatedStrings]
                                )) {
                        //
                        // This string was invalid (> 64K bytes) give up
                        // and make sure we only free the ones we've already
                        // allocated (including this last one)
                        //

                        AllocatedStrings++;
                        Status = STATUS_INVALID_PARAMETER;
                        break;
                    }
                }
            }
        }
        except (EXCEPTION_EXECUTE_HANDLER) {
            Status = STATUS_INVALID_PARAMETER;
        }

        if (Status == STATUS_SUCCESS) {
            Status = ElfReportEventW (
                            hEventLog,
                            wType,
                            wCategory,
                            dwEventID,
                            lpUserSid,
                            wNumStrings,
                            dwDataSize,
                            pUStrings,
                            lpRawData,
                            0,            // Flags        -  Paired event
                            NULL,         // RecordNumber  | support.  Not
                            NULL          // TimeWritten  -  in P1
                            );
        }

        //
        // Free the space allocated for the UNICODE strings
        // and then free the space for the array.
        //
        for (i = 0; i < AllocatedStrings; i++) {
            if (pUStrings[i])
                RtlFreeHeap (RtlProcessHeap(), 0, pUStrings[i]);
        }
        RtlFreeHeap (RtlProcessHeap(), 0, pUStrings);

    } else {
        Status = STATUS_NO_MEMORY;
    }

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        ReturnValue = FALSE;
    } else {
        ReturnValue = TRUE;
    }

    return ReturnValue;

}


//
// ANSI APIs
//

BOOL
ClearEventLogA (
    HANDLE  hEventLog,
    LPCSTR  BackupFileName
    )

/*++

Routine Description:

  This is the client DLL entry point for the ClearEventLogFile API.
  The call is passed to the eventlog service on the appropriate server
  identified by LogHandle.


Arguments:

    LogHandle       - Handle returned from a previous "Open" call. This is
                      used to identify the module and the server.

    BackupFileName  - Name of the file to back up the current log file.
                      NULL implies not to back up the file.


Return Value:

    TRUE if success, FALSE otherwise.


--*/
{

    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    BOOL ReturnValue;

    //
    // Turn the backup filename into UNICODE
    //

    if (BackupFileName) {
        RtlInitAnsiString(&AnsiString, BackupFileName);
        Status = RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString,
            TRUE);
        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError(Status);
            return(FALSE);
        }
    }
    else {
        RtlInitUnicodeString(&UnicodeString, NULL);
    }

    ReturnValue = ClearEventLogW (hEventLog, (LPCWSTR)UnicodeString.Buffer);
    RtlFreeUnicodeString(&UnicodeString);
    return(ReturnValue);
}



BOOL
BackupEventLogA (
    HANDLE  hEventLog,
    LPCSTR  BackupFileName
    )

/*++

Routine Description:

  This is the client DLL entry point for the BackupEventLogFile API.
  The call is passed to the eventlog service on the appropriate server
  identified by LogHandle.


Arguments:

    LogHandle       - Handle returned from a previous "Open" call. This is
                      used to identify the module and the server.

    BackupFileName  - Name of the file to back up the current log file.


Return Value:

    TRUE if success, FALSE otherwise.


--*/
{

    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    BOOL ReturnValue;

    //
    // Turn the backup filename into UNICODE
    //

    if (BackupFileName) {
        RtlInitAnsiString(&AnsiString, BackupFileName);
        Status = RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString,
            TRUE);
        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError(Status);
            return(FALSE);
        }
    }
    else {
        RtlInitUnicodeString(&UnicodeString, NULL);
    }

    ReturnValue = BackupEventLogW (hEventLog, (LPCWSTR)UnicodeString.Buffer);
    RtlFreeUnicodeString(&UnicodeString);
    return(ReturnValue);

}


HANDLE
OpenEventLogA (
    LPCSTR   UNCServerName,
    LPCSTR   ModuleName
    )

/*++

Routine Description:

    This is the client DLL entry point for the WinOpenEventLog API.

    It creates an RPC binding for the server specified, and stores that
    and additional data away. It returns a handle to the caller that can
    be used to later on access the handle-specific information.

Arguments:

    UNCServerName     - Server with which to bind for subsequent operations.

    ModuleName        - Supplies the name of the module to associate with
                        this handle.


Return Value:

    Returns a handle that can be used for subsequent Win API calls. If
    the handle is NULL, then an error occurred.


--*/
{

    ANSI_STRING AnsiString;
    ANSI_STRING AnsiModuleName;
    NTSTATUS Status;
    HANDLE LogHandle;
    HANDLE ReturnHandle;

    RtlInitAnsiString(&AnsiModuleName,ModuleName);
    RtlInitAnsiString(&AnsiString, UNCServerName);

    Status = ElfOpenEventLogA (
                        &AnsiString,
                        &AnsiModuleName,
                        &LogHandle
                        );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        ReturnHandle = (HANDLE)NULL;
    } else {
        ReturnHandle = (HANDLE)LogHandle;
    }

    return ReturnHandle;
}


HANDLE
RegisterEventSourceA (
    LPCSTR   UNCServerName,
    LPCSTR   ModuleName
    )

/*++

Routine Description:

    This is the client DLL entry point for the RegisterEventSource API.

    It creates an RPC binding for the server specified, and stores that
    and additional data away. It returns a handle to the caller that can
    be used to later on access the handle-specific information.

Arguments:

    UNCServerName     - Server with which to bind for subsequent operations.

    ModuleName        - Supplies the name of the module to associate with
                        this handle.


Return Value:

    Returns a handle that can be used for subsequent Win API calls. If
    the handle is NULL, then an error occurred.


--*/
{

    ANSI_STRING AnsiString;
    ANSI_STRING AnsiModuleName;
    NTSTATUS Status;
    HANDLE LogHandle;
    HANDLE ReturnHandle;

    RtlInitAnsiString(&AnsiModuleName,ModuleName);
    RtlInitAnsiString(&AnsiString, UNCServerName);

    Status = ElfRegisterEventSourceA (
                        &AnsiString,
                        &AnsiModuleName,
                        &LogHandle
                        );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        ReturnHandle = (HANDLE)NULL;
    } else {
        ReturnHandle = (HANDLE)LogHandle;
    }

    return ReturnHandle;
}


HANDLE
OpenBackupEventLogA (
    LPCSTR   UNCServerName,
    LPCSTR   FileName
    )

/*++

Routine Description:

    This is the client DLL entry point for the OpenBackupEventLog API.

    It creates an RPC binding for the server specified, and stores that
    and additional data away. It returns a handle to the caller that can
    be used to later on access the handle-specific information.

Arguments:

    UNCServerName   - Server with which to bind for subsequent operations.

    FileName        - Supplies the filename of the logfile to associate with
                      this handle.


Return Value:

    Returns a handle that can be used for subsequent Win API calls. If
    the handle is NULL, then an error occurred.


--*/
{

    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeServerName;
    UNICODE_STRING UnicodeFileName;
    NTSTATUS Status;
    HANDLE ReturnHandle;

    //
    // Turn the servername into UNICODE
    //

    if (UNCServerName) {
        RtlInitAnsiString(&AnsiString, UNCServerName);
        Status = RtlAnsiStringToUnicodeString(&UnicodeServerName, &AnsiString,
            TRUE);
        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError(Status);
            return(NULL);
        }
    }
    else {
        RtlInitUnicodeString(&UnicodeServerName, NULL);
    }

    //
    // Turn the filename into UNICODE
    //

    if (FileName) {
        RtlInitAnsiString(&AnsiString, FileName);
        Status = RtlAnsiStringToUnicodeString(&UnicodeFileName, &AnsiString,
            TRUE);
        if ( !NT_SUCCESS(Status) ) {
            RtlFreeUnicodeString(&UnicodeServerName);
            BaseSetLastNTError(Status);
            return(NULL);
        }
    }
    else {
        RtlInitUnicodeString(&UnicodeFileName, NULL);
    }

    ReturnHandle = OpenBackupEventLogW ((LPCWSTR)UnicodeServerName.Buffer,
        (LPCWSTR)UnicodeFileName.Buffer);
    RtlFreeUnicodeString(&UnicodeServerName);
    RtlFreeUnicodeString(&UnicodeFileName);
    return(ReturnHandle);

}





BOOL
ReadEventLogA (
    HANDLE      hEventLog,
    DWORD       dwReadFlags,
    DWORD       dwRecordOffset,
    LPVOID      lpBuffer,
    DWORD       nNumberOfBytesToRead,
    DWORD       *pnBytesRead,
    DWORD       *pnMinNumberOfBytesNeeded
    )

/*++

Routine Description:

  This is the client DLL entry point for the WinreadEventLog API.

Arguments:



Return Value:

    Returns count of bytes read. Zero of none read.


--*/
{

    NTSTATUS Status;
    BOOL ReturnValue;

    Status = ElfReadEventLogA (
                        hEventLog,
                        dwReadFlags,
                        dwRecordOffset,
                        lpBuffer,
                        nNumberOfBytesToRead,
                        pnBytesRead,
                        pnMinNumberOfBytesNeeded
                        );

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        ReturnValue = FALSE;
    } else {
        ReturnValue = TRUE;
    }
    return ReturnValue;

}



BOOL
ReportEventA (
    HANDLE      hEventLog,
    WORD        wType,
    WORD        wCategory       OPTIONAL,
    DWORD       dwEventID,
    PSID        lpUserSid       OPTIONAL,
    WORD        wNumStrings,
    DWORD       dwDataSize,
    LPCSTR      *lpStrings      OPTIONAL,
    LPVOID      lpRawData       OPTIONAL
    )

/*++

Routine Description:

  This is the client DLL entry point for the ReportEvent API.

Arguments:


Return Value:

    Returns TRUE if success, FALSE otherwise.


--*/
{

    NTSTATUS Status = STATUS_SUCCESS;
    BOOL ReturnValue;
    PANSI_STRING *pAStrings;
    ULONG       i;
    ULONG AllocatedStrings;

    //
    // Convert the array of strings to an array of PANSI_STRINGs
    // before calling ElfReportEventW.
    //
    pAStrings = RtlAllocateHeap(
                            RtlProcessHeap(), 0,
                            wNumStrings * sizeof(PANSI_STRING)
                            );

    if (pAStrings) {

        //
        // Guard the memory allocation above while we peruse the user's
        // buffer. If not, we'd leak it on an exception.
        //

        try {
            //
            // For each string passed in, allocate an ANSI_STRING structure
            // and fill it in with the string.
            //
            for (AllocatedStrings = 0; AllocatedStrings < wNumStrings;
              AllocatedStrings++) {
                pAStrings[AllocatedStrings] = RtlAllocateHeap(
                                        RtlProcessHeap(), 0,
                                        sizeof(ANSI_STRING)
                                        );

                if (pAStrings[AllocatedStrings]) {

                    if (!InitAnsiString(
                                pAStrings[AllocatedStrings],
                                lpStrings[AllocatedStrings]
                                )) {
                        //
                        // This string was invalid (> 32K chars) give up
                        // and make sure we only free the ones we've already
                        // allocated (including this last one)
                        //

                        AllocatedStrings++;
                        Status = STATUS_INVALID_PARAMETER;
                        break;
                    }
                }
            }
        }
        except (EXCEPTION_EXECUTE_HANDLER) {
            Status = STATUS_INVALID_PARAMETER;
        }

        if (Status == STATUS_SUCCESS) {
            Status = ElfReportEventA (
                            hEventLog,
                            wType,
                            wCategory,
                            dwEventID,
                            lpUserSid,
                            wNumStrings,
                            dwDataSize,
                            pAStrings,
                            lpRawData,
                            0,            // Flags        -  Paired event
                            NULL,         // RecordNumber  | support.  Not
                            NULL          // TimeWritten  -  in P1
                            );
        }

        //
        // Free all the memory that was allocated
        //
        for (i = 0; i < AllocatedStrings; i++) {
            if (pAStrings[i])
                RtlFreeHeap (RtlProcessHeap(), 0, pAStrings[i]);
        }
        RtlFreeHeap (RtlProcessHeap(), 0, pAStrings);

    } else {
        Status = STATUS_NO_MEMORY;
    }

    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        ReturnValue = FALSE;
    } else {
        ReturnValue = TRUE;
    }

    return ReturnValue;

}

