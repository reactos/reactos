
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    APISTUB.C

Abstract:

    This module contains the client ends of the Elf APIs.


Author:

    Rajen Shah  (rajens)    29-Jul-1991


Revision History:

    29-Jul-1991         RajenS
        Created

    13-Jan-1997         Added extensions for clustering to support replicated
                        eventlogs
--*/
/****
@doc    EXTERNAL INTERFACES EVTLOG
****/

#include <elfclntp.h>
#include <lmerr.h>
#include <stdlib.h>
#include <string.h>

//
// Global data
//
PUNICODE_STRING     pGlobalComputerNameU;
PANSI_STRING        pGlobalComputerNameA;



VOID
w_GetComputerName ( )

/*++

Routine Description:

    This routine gets the name of the computer. It checks the global
    variable to see if the computer name has already been determined.
    If not, it updates that variable with the name.
    It does this for the UNICODE and the ANSI versions.

Arguments:

    NONE

Return Value:

    NONE


--*/
{
    PUNICODE_STRING     pNameU;
    PANSI_STRING        pNameA;
    LPSTR               szName;
    LPWSTR              wszName;
    DWORD               dwStatus;

    //
    // If this fails, we're leaking
    //
    ASSERT(pGlobalComputerNameU == NULL && pGlobalComputerNameA == NULL);

    pNameU = MIDL_user_allocate (sizeof (UNICODE_STRING));
    pNameA = MIDL_user_allocate (sizeof (ANSI_STRING));

    if ((pNameU != NULL) && (pNameA != NULL)) {

        dwStatus = ElfpGetComputerName(&szName, &wszName);

        if (dwStatus == NO_ERROR) {

            //
            // ElfpComputerName has allocated a buffer to contain the
            // ASCII name of the computer. We use that for the ANSI
            // string structure.
            //
            RtlInitAnsiString ( pNameA, szName );
            RtlInitUnicodeString ( pNameU, wszName );

        } else {

            //
            // We could not get the computer name for some reason. Set up
            // the golbal pointer to point to the NULL string.
            //
            RtlInitAnsiString ( pNameA, "\0");
            RtlInitUnicodeString ( pNameU, L"\0");
        }

        pGlobalComputerNameU = pNameU;
        pGlobalComputerNameA = pNameA;
    }
    else {

        //
        // In case one of the two was allocated.
        // 
        MIDL_user_free (pNameU);
        MIDL_user_free (pNameA);
    }
}




PUNICODE_STRING
TmpGetComputerNameW ( )

/*++

Routine Description:

    This routine gets the UNICODE name of the computer. It checks the global
    variable to see if the computer name has already been determined.
    If not, it calls the worker routine to do that.

Arguments:

    NONE

Return Value:

    Returns a pointer to the computer name, or a NULL.


--*/
{
    if (pGlobalComputerNameU == NULL) {
        w_GetComputerName();
    }
    return (pGlobalComputerNameU);
}



PANSI_STRING
TmpGetComputerNameA ( )

/*++

Routine Description:

    This routine gets the ANSI name of the computer. It checks the global
    variable to see if the computer name has already been determined.
    If not, it calls the worker routine to do that.

Arguments:

    NONE

Return Value:

    Returns a pointer to the computer name, or a NULL.


--*/
{

    if (pGlobalComputerNameA == NULL) {
        w_GetComputerName();
    }
    return (pGlobalComputerNameA);
}

//
// These APIs only have one interface, since they don't take or return strings
//

NTSTATUS
ElfNumberOfRecords(
    IN      HANDLE      LogHandle,
    OUT     PULONG      NumberOfRecords
    )
{
    NTSTATUS status;

    //
    // Make sure the output pointer is valid
    //

    if (!NumberOfRecords) {
       return(STATUS_INVALID_PARAMETER);
    }

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //
    RpcTryExcept {

        // Call service entry point

        status = ElfrNumberOfRecords (
                        (IELF_HANDLE) LogHandle,
                        NumberOfRecords
                        );

    }
    RpcExcept (1) {
            status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept

    return (status);

}

NTSTATUS
ElfOldestRecord(
    IN      HANDLE      LogHandle,
    OUT     PULONG      OldestRecordNumber
    )
{
    NTSTATUS status;

    //
    //
    // Make sure the output pointer is valid
    //

    if (!OldestRecordNumber) {
       return(STATUS_INVALID_PARAMETER);
    }

    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //
    RpcTryExcept {

        // Call service entry point

        status = ElfrOldestRecord (
                        (IELF_HANDLE) LogHandle,
                        OldestRecordNumber
                        );

    }
    RpcExcept (1) {
            status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept

    return (status);

}


NTSTATUS
ElfChangeNotify(
    IN      HANDLE      LogHandle,
    IN      HANDLE      Event
    )
{

    NTSTATUS status;
    RPC_CLIENT_ID RpcClientId;
    CLIENT_ID ClientId;

    //
    // Map the handles to something that RPC can understand
    //

    ClientId = NtCurrentTeb()->ClientId;
    RpcClientId.UniqueProcess = (ULONG)((ULONG_PTR)ClientId.UniqueProcess);
    RpcClientId.UniqueThread = (ULONG)((ULONG_PTR)ClientId.UniqueThread);

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        // Call service entry point

        status = ElfrChangeNotify (
                        (IELF_HANDLE)((ULONG_PTR)LogHandle),
                        RpcClientId,
                        (DWORD)(ULONG_PTR)Event
                        );

    }
    RpcExcept (1) {
            status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept

    return (status);

}


NTSTATUS
ElfGetLogInformation(
    IN     HANDLE        LogHandle,
    IN     ULONG         InfoLevel,
    OUT    PVOID         lpBuffer,
    IN     ULONG         cbBufSize,
    OUT    PULONG        pcbBytesNeeded
    )
{
    NTSTATUS ntStatus;

    //
    // Make sure the Infolevel is valid
    //

    switch (InfoLevel) {

        case EVENTLOG_FULL_INFO:

            RpcTryExcept {

                // Call service entry point

                ntStatus = ElfrGetLogInformation(
                               (IELF_HANDLE) LogHandle,
                               InfoLevel,
                               lpBuffer,
                               cbBufSize,
                               pcbBytesNeeded);

            }
            RpcExcept (1) {
                ntStatus = I_RpcMapWin32Status(RpcExceptionCode());
            }
            RpcEndExcept

            break;

        default:

            ntStatus = STATUS_INVALID_LEVEL;
            break;
    }

    return ntStatus;
}


//
// UNICODE APIs
//

NTSTATUS
ElfOpenEventLogW (
    IN  PUNICODE_STRING         UNCServerName,
    IN  PUNICODE_STRING         LogName,
    OUT PHANDLE                 LogHandle
    )

/*++

Routine Description:

    This is the client DLL entry point for the ElfOpenEventLog API.

    It creates an RPC binding for the server specified, and stores that
    and additional data away. It returns a handle to the caller that can
    be used to later on access the handle-specific information.

Arguments:

    UNCServerName   - Server with which to bind for subsequent operations.

    LogName         - Supplies the name of the module for the logfile
                      to associate with this handle.

    LogHandle       - Location where log handle is to be returned.


Return Value:

    Returns an NTSTATUS code and, if no error, a handle that can be used
    for subsequent Elf API calls.


--*/
{
    NTSTATUS            status    = STATUS_SUCCESS;
    NTSTATUS            ApiStatus;
    UNICODE_STRING      RegModuleName;
    EVENTLOG_HANDLE_W   ServerNameString;
    BOOLEAN             fWasEnabled = FALSE;
    BOOL                fIsSecurityLog;

    //
    // Make sure input & output pointers are valid
    //

    if (!LogHandle || !LogName || LogName->Length == 0) {
       return(STATUS_INVALID_PARAMETER);
    }

    if ((UNCServerName != NULL) && (UNCServerName->Length != 0)) {
        ServerNameString = UNCServerName->Buffer;
    } else {
        ServerNameString = NULL;
    }

    RtlInitUnicodeString( &RegModuleName, UNICODE_NULL);

    // Call service via RPC. Pass in major and minor version numbers.

    *LogHandle = NULL;          // Must be NULL so RPC fills it in

    fIsSecurityLog = (_wcsicmp(ELF_SECURITY_MODULE_NAME, LogName->Buffer) == 0);

    if (fIsSecurityLog) {

        //
        // Tacitly attempt to enable the SE_SECURITY_PRIVILEGE so we can
        // can check it on the server side.  We ignore the return value
        // because it's possible for this call to fail here but for the
        // user to have this privilege if the log is on a remote server.
        //
        // Note that we make this call on behalf of the client to avoid
        // a regression when we check for the privilege on the server
        // side -- without this call, 3rd party apps that successfully
        // called this API before would fail.  Under normal circumstances,
        // this is not an encouraged practice.
        //

        //
        // BUGBUG -- This should really be done via ImpersonateSelf()
        //           and adjusting the thread token
        //
        ApiStatus = RtlAdjustPrivilege(SE_SECURITY_PRIVILEGE,
                                       TRUE,
                                       FALSE,
                                       &fWasEnabled);
    }

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //
    RpcTryExcept {

        status = ElfrOpenELW(
                    ServerNameString,
                    (PRPC_UNICODE_STRING) LogName,
                    (PRPC_UNICODE_STRING) &RegModuleName,
                    ELF_VERSION_MAJOR,
                    ELF_VERSION_MINOR,
                    (PIELF_HANDLE) LogHandle
                    );

    }
    RpcExcept (1) {

        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept


    if (fIsSecurityLog && NT_SUCCESS(ApiStatus)) {

        //
        // Restore the state
        //

        RtlAdjustPrivilege(SE_SECURITY_PRIVILEGE,
                           fWasEnabled,
                           FALSE,
                           &fWasEnabled);
    }

    return (status);
}


NTSTATUS
ElfRegisterEventSourceW (
    IN  PUNICODE_STRING         UNCServerName,
    IN  PUNICODE_STRING         ModuleName,
    OUT PHANDLE                 LogHandle
    )

/*++

Routine Description:

    This is the client DLL entry point for the ElfRegisterEventSource API.

    It creates an RPC binding for the server specified, and stores that
    and additional data away. It returns a handle to the caller that can
    be used to later on access the handle-specific information.

Arguments:

    UNCServerName   - Server with which to bind for subsequent operations.

    ModuleName      - Supplies the name of the module to associate with
                      this handle.

    LogHandle       - Location where log handle is to be returned.


Return Value:

    Returns an NTSTATUS code and, if no error, a handle that can be used
    for subsequent Elf API calls.


--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    UNICODE_STRING      RegModuleName;
    EVENTLOG_HANDLE_W   ServerNameString;

    //
    // Make sure input & output pointers are valid
    //

    if (!LogHandle || !ModuleName || ModuleName->Length == 0) {
       return(STATUS_INVALID_PARAMETER);
    }

    if ((UNCServerName != NULL) && (UNCServerName->Length != 0)) {
        ServerNameString = UNCServerName->Buffer;
    } else {
        ServerNameString = NULL;
    }

    RtlInitUnicodeString( &RegModuleName, UNICODE_NULL);

    // Call service via RPC. Pass in major and minor version numbers.

    *LogHandle = NULL;          // Must be NULL so RPC fills it in

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //
    RpcTryExcept {

        status = ElfrRegisterEventSourceW(
                    ServerNameString,
                    (PRPC_UNICODE_STRING)ModuleName,
                    (PRPC_UNICODE_STRING)&RegModuleName,
                    ELF_VERSION_MAJOR,
                    ELF_VERSION_MINOR,
                    (PIELF_HANDLE) LogHandle
                    );

    }
    RpcExcept (1) {

        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept


    return (status);
}


NTSTATUS
ElfOpenBackupEventLogW (
    IN  PUNICODE_STRING UNCServerName,
    IN  PUNICODE_STRING BackupFileName,
    OUT PHANDLE LogHandle
    )

/*++

Routine Description:

    This is the client DLL entry point for the ElfOpenBackupEventLog API.

    It creates an RPC binding for the server specified, and stores that
    and additional data away. It returns a handle to the caller that can
    be used to later on access the handle-specific information.

Arguments:

    UNCServerName   - Server with which to bind for subsequent operations.

    BackupFileName  - Supplies the filename of the module to associate with
                      this handle.

    LogHandle       - Location where log handle is to be returned.


Return Value:

    Returns an NTSTATUS code and, if no error, a handle that can be used
    for subsequent Elf API calls.


--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    EVENTLOG_HANDLE_W   ServerNameString;

    //
    // Make sure input & output pointers are valid
    //

    if (!LogHandle || !BackupFileName || BackupFileName->Length == 0) {
       return(STATUS_INVALID_PARAMETER);
    }

    if ((UNCServerName != NULL) && (UNCServerName->Length != 0)) {
        ServerNameString = UNCServerName->Buffer;
    } else {
        ServerNameString = NULL;
    }

    // Call service via RPC. Pass in major and minor version numbers.

    *LogHandle = NULL;          // Must be NULL so RPC fills it in

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        status = ElfrOpenBELW(
                    ServerNameString,
                    (PRPC_UNICODE_STRING)BackupFileName,
                    ELF_VERSION_MAJOR,
                    ELF_VERSION_MINOR,
                    (PIELF_HANDLE) LogHandle
                    );

    }
    RpcExcept (1) {

        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept

    return (status);
}



NTSTATUS
ElfClearEventLogFileW (
    IN      HANDLE          LogHandle,
    IN      PUNICODE_STRING BackupFileName
    )

/*++

Routine Description:

  This is the client DLL entry point for the ElfClearEventLogFile API.
  The call is passed to the eventlog service on the appropriate server
  identified by LogHandle.


Arguments:

    LogHandle       - Handle returned from a previous "Open" call. This is
                      used to identify the module and the server.

    BackupFileName  - Name of the file to back up the current log file.
                      NULL implies not to back up the file.


Return Value:

    Returns an NTSTATUS code.


--*/
{
    NTSTATUS status;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //
    RpcTryExcept {

        // Call service entry point

        status = ElfrClearELFW (
                        (IELF_HANDLE) LogHandle,
                        (PRPC_UNICODE_STRING)BackupFileName
                        );

    }
    RpcExcept (1) {

        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept

    return (status);

}


NTSTATUS
ElfBackupEventLogFileW (
    IN      HANDLE          LogHandle,
    IN      PUNICODE_STRING BackupFileName
    )

/*++

Routine Description:

  This is the client DLL entry point for the ElfBackupEventLogFile API.
  The call is passed to the eventlog service on the appropriate server
  identified by LogHandle.


Arguments:

    LogHandle       - Handle returned from a previous "Open" call. This is
                      used to identify the module and the server.

    BackupFileName  - Name of the file to back up the current log file.


Return Value:

    Returns an NTSTATUS code.


--*/
{
    NTSTATUS status;
    NTSTATUS ApiStatus;
    BOOLEAN  fWasEnabled;

    //
    // Make sure input pointers are valid
    //

    if (!BackupFileName || BackupFileName->Length == 0) {
       return(STATUS_INVALID_PARAMETER);
    }

    //
    // Tacitly attempt to enable the SE_BACKUP_PRIVILEGE so we can
    // can check it on the server side
    //
    // Note that we make this call on behalf of the client to avoid
    // a regression when we check for the privilege on the server
    // side -- without this call, 3rd party apps that successfully
    // called this API before would fail.  Under normal circumstances,
    // this is not an encouraged practice.
    //

    //
    // BUGBUG -- This should really be done via ImpersonateSelf()
    //           and adjusting the thread token
    //
    ApiStatus = RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE,
                                   TRUE,
                                   FALSE,
                                   &fWasEnabled);

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //
    RpcTryExcept {

        // Call service entry point

        status = ElfrBackupELFW (
                        (IELF_HANDLE) LogHandle,
                        (PRPC_UNICODE_STRING)BackupFileName);

    }
    RpcExcept (1) {

        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept

    //
    // Restore the client's privilege state to what it was before
    //

    if (NT_SUCCESS(ApiStatus)) {

        RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE,
                           fWasEnabled,
                           TRUE,
                           &fWasEnabled);
    }

    return (status);
}


NTSTATUS
ElfCloseEventLog (
    IN  HANDLE  LogHandle
    )

/*++

Routine Description:

  This is the client DLL entry point for the ElfCloseEventLog API.
  It closes the RPC binding, and frees any memory allocated for the
  handle.


Arguments:

    LogHandle       - Handle returned from a previous "Open" call.


Return Value:

    Returns an NTSTATUS code.


--*/
{
    NTSTATUS status;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //
    RpcTryExcept {

        // Call server

        status = ElfrCloseEL (
                        (PIELF_HANDLE)  &LogHandle
                        );
    }
    RpcExcept (1) {

        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept

    return (status);

}


NTSTATUS
ElfDeregisterEventSource (
    IN  HANDLE  LogHandle
    )

/*++

Routine Description:

  This is the client DLL entry point for the ElfDeregisterEventSource API.
  It closes the RPC binding, and frees any memory allocated for the
  handle.


Arguments:

    LogHandle       - Handle returned from a previous "Open" call.


Return Value:

    Returns an NTSTATUS code.


--*/
{
    NTSTATUS status;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //
    RpcTryExcept {

        // Call server

        status = ElfrDeregisterEventSource (
                        (PIELF_HANDLE)  &LogHandle
                        );
    }
    RpcExcept (1) {

        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept

    return (status);

}



NTSTATUS
ElfReadEventLogW (
    IN          HANDLE      LogHandle,
    IN          ULONG       ReadFlags,
    IN          ULONG       RecordNumber,
    OUT         PVOID       Buffer,
    IN          ULONG       NumberOfBytesToRead,
    OUT         PULONG      NumberOfBytesRead,
    OUT         PULONG      MinNumberOfBytesNeeded
    )

/*++

Routine Description:

  This is the client DLL entry point for the ElfReadEventLog API.

Arguments:



Return Value:

    Returns an NTSTATUS code.


--*/
{
    NTSTATUS status;
    ULONG    FlagBits;

    //
    // Make sure the output pointers are valid
    //

    if (!Buffer || !NumberOfBytesRead || !MinNumberOfBytesNeeded) {
       return(STATUS_INVALID_PARAMETER);
    }

    //
    // Ensure that the ReadFlags we got are valid.
    // Make sure that one of each type of bit is set.
    //
    FlagBits = ReadFlags & (EVENTLOG_SEQUENTIAL_READ | EVENTLOG_SEEK_READ);

    if ((FlagBits > 2) || (FlagBits == 0)) {
        return(STATUS_INVALID_PARAMETER);
    }

    FlagBits = ReadFlags & (EVENTLOG_FORWARDS_READ | EVENTLOG_BACKWARDS_READ);

    if ((FlagBits > 8) || (FlagBits == 0)) {
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //
    RpcTryExcept {

        // Call service

        status = ElfrReadELW (
                        (IELF_HANDLE) LogHandle,
                        ReadFlags,
                        RecordNumber,
                        NumberOfBytesToRead,
                        Buffer,
                        NumberOfBytesRead,
                        MinNumberOfBytesNeeded
                        );

    }
    RpcExcept (1) {

        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept

    // Return status and bytes read/required.

    return (status);

}



NTSTATUS
ElfReportEventW (
    IN      HANDLE          LogHandle,
    IN      USHORT          EventType,
    IN      USHORT          EventCategory OPTIONAL,
    IN      ULONG           EventID,
    IN      PSID            UserSid,
    IN      USHORT          NumStrings,
    IN      ULONG           DataSize,
    IN      PUNICODE_STRING *Strings,
    IN      PVOID           Data,
    IN      USHORT          Flags,
    IN OUT  PULONG          RecordNumber OPTIONAL,
    IN OUT  PULONG          TimeWritten  OPTIONAL
    )

/*++

Routine Description:

  This is the client DLL entry point for the ElfReportEvent API.

Arguments:


Return Value:

    Returns an NTSTATUS code.

Note:

    The last three parameters (Flags, RecordNumber and TimeWritten) are
    designed to be used by Security Auditing for the implementation of
    paired events (associating a file open event with the subsequent file
    close). This will not be implemented in Product 1, but the API is
    defined to allow easier support of this in a later release.


--*/
{
    NTSTATUS status;
    PUNICODE_STRING pComputerNameU;
    LARGE_INTEGER Time;
    ULONG EventTime;

    //
    // Generate the time of the event. This is done on the client side
    // since that is where the event occurred.
    //
    NtQuerySystemTime(&Time);
    RtlTimeToSecondsSince1970(&Time,
                          &EventTime
                         );

    //
    // Generate the ComputerName of the client.
    // We have to do this in the client side since this call may be
    // remoted to another server and we would not necessarily have
    // the computer name there.
    //
    pComputerNameU = TmpGetComputerNameW();

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //
    RpcTryExcept {

        // Call service

        status = ElfrReportEventW (
                    (IELF_HANDLE)   LogHandle,
                    EventTime,
                    EventType,
                    EventCategory,
                    EventID,
                    NumStrings,
                    DataSize,
                    (PRPC_UNICODE_STRING)pComputerNameU,
                    UserSid,
                    (PRPC_UNICODE_STRING *)Strings,
                    Data,
                    Flags,
                    RecordNumber,
                    TimeWritten
                    );

    }
    RpcExcept (1) {

        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept

    return (status);

}


//
// ANSI APIs
//

NTSTATUS
ElfOpenEventLogA (
    IN  PANSI_STRING    UNCServerName,
    IN  PANSI_STRING    LogName,
    OUT PHANDLE         LogHandle
    )

/*++

Routine Description:

    This is the client DLL entry point for the ElfOpenEventLog API.

    It creates an RPC binding for the server specified, and stores that
    and additional data away. It returns a handle to the caller that can
    be used to later on access the handle-specific information.

Arguments:

    UNCServerName   - Server with which to bind for subsequent operations.

    LogName         - Supplies the name of the module for the logfile to
                      associate with this handle.

    LogHandle       - Location where log handle is to be returned.


Return Value:

    Returns an NTSTATUS code and, if no error, a handle that can be used
    for subsequent Elf API calls.


--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    NTSTATUS            ApiStatus;
    ANSI_STRING         RegModuleName;
    EVENTLOG_HANDLE_A   ServerNameString;
    BOOLEAN             fWasEnabled = FALSE;
    BOOL                fIsSecurityLog;

    //
    // Make sure input & output pointers are valid
    //

    if (!LogHandle || !LogName || LogName->Length == 0) {
       return(STATUS_INVALID_PARAMETER);
    }

    if ((UNCServerName != NULL) && (UNCServerName->Length != 0)) {
        ServerNameString = UNCServerName->Buffer;
    } else {
        ServerNameString = NULL;
    }

    RtlInitAnsiString( &RegModuleName, ELF_APPLICATION_MODULE_NAME_ASCII );

    // Call service via RPC. Pass in major and minor version numbers.

    *LogHandle = NULL;          // Must be NULL so RPC fills it in

    fIsSecurityLog = (_stricmp(ELF_SECURITY_MODULE_NAME_ASCII, LogName->Buffer) == 0);

    if (fIsSecurityLog) {

        //
        // Tacitly attempt to enable the SE_SECURITY_PRIVILEGE so we can
        // can check it on the server side.  We ignore the return value
        // because it's possible for this call to fail here but for the
        // user to have this privilege if the log is on a remote server
        //
        // Note that we make this call on behalf of the client to avoid
        // a regression when we check for the privilege on the server
        // side -- without this call, 3rd party apps that successfully
        // called this API before would fail.  Under normal circumstances,
        // this is not an encouraged practice.
        //

        //
        // BUGBUG -- This should really be done via ImpersonateSelf()
        //           and adjusting the thread token
        //
        ApiStatus = RtlAdjustPrivilege(SE_SECURITY_PRIVILEGE,
                                       TRUE,
                                       FALSE,
                                       &fWasEnabled);
    }

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //
    RpcTryExcept {

         status = ElfrOpenELA (
                    ServerNameString,
                    (PRPC_STRING) LogName,
                    (PRPC_STRING) &RegModuleName,
                    ELF_VERSION_MAJOR,
                    ELF_VERSION_MINOR,
                    (PIELF_HANDLE) LogHandle);

    }
    RpcExcept (1) {

        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept

    if (fIsSecurityLog && NT_SUCCESS(ApiStatus)) {

        //
        // Restore the state
        //

        RtlAdjustPrivilege(SE_SECURITY_PRIVILEGE,
                           fWasEnabled,
                           FALSE,
                           &fWasEnabled);
    }

    return (status);
}


NTSTATUS
ElfRegisterEventSourceA (
    IN  PANSI_STRING    UNCServerName,
    IN  PANSI_STRING    ModuleName,
    OUT PHANDLE         LogHandle
    )

/*++

Routine Description:

    This is the client DLL entry point for the ElfOpenEventLog API.

    It creates an RPC binding for the server specified, and stores that
    and additional data away. It returns a handle to the caller that can
    be used to later on access the handle-specific information.

Arguments:

    UNCServerName   - Server with which to bind for subsequent operations.

    ModuleName      - Supplies the name of the module to associate with
                      this handle.

    LogHandle       - Location where log handle is to be returned.


Return Value:

    Returns an NTSTATUS code and, if no error, a handle that can be used
    for subsequent Elf API calls.


--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    ANSI_STRING         RegModuleName;
    EVENTLOG_HANDLE_A   ServerNameString;

    //
    // Make sure input & output pointers are valid
    //

    if (!LogHandle || !ModuleName || ModuleName->Length == 0) {
       return(STATUS_INVALID_PARAMETER);
    }

    if ((UNCServerName != NULL) && (UNCServerName->Length != 0)) {
        ServerNameString = UNCServerName->Buffer;
    } else {
        ServerNameString = NULL;
    }

    RtlInitAnsiString( &RegModuleName, ELF_APPLICATION_MODULE_NAME_ASCII );

    if ( NT_SUCCESS (status) ) {

        // Call service via RPC. Pass in major and minor version numbers.

        *LogHandle = NULL;          // Must be NULL so RPC fills it in

        //
        // Do the RPC call with an exception handler since RPC will raise an
        // exception if anything fails. It is up to us to figure out what
        // to do once the exception is raised.
        //
        RpcTryExcept {

            status = ElfrRegisterEventSourceA (
                        ServerNameString,
                        (PRPC_STRING)ModuleName,
                        (PRPC_STRING)&RegModuleName,
                        ELF_VERSION_MAJOR,
                        ELF_VERSION_MINOR,
                        (PIELF_HANDLE) LogHandle
                        );

        }
        RpcExcept (1) {

            status = I_RpcMapWin32Status(RpcExceptionCode());
        }
        RpcEndExcept


    }

    return (status);
}



NTSTATUS
ElfOpenBackupEventLogA (
    IN  PANSI_STRING    UNCServerName,
    IN  PANSI_STRING    FileName,
    OUT PHANDLE         LogHandle
    )

/*++

Routine Description:

    This is the client DLL entry point for the ElfOpenBackupEventLog API.

    It creates an RPC binding for the server specified, and stores that
    and additional data away. It returns a handle to the caller that can
    be used to later on access the handle-specific information.

Arguments:

    UNCServerName   - Server with which to bind for subsequent operations.

    FileName        - Supplies the filename of the logfile to associate with
                      this handle.

    LogHandle       - Location where log handle is to be returned.


Return Value:

    Returns an NTSTATUS code and, if no error, a handle that can be used
    for subsequent Elf API calls.


--*/
{
    EVENTLOG_HANDLE_A   ServerNameString;
    NTSTATUS            status;

    //
    // Make sure input & output pointers are valid
    //

    if (!LogHandle || !FileName || FileName->Length == 0) {
       return(STATUS_INVALID_PARAMETER);
    }

    if ((UNCServerName != NULL) && (UNCServerName->Length != 0)) {
        ServerNameString = UNCServerName->Buffer;
    } else {
        ServerNameString = NULL;
    }

    // Call service via RPC. Pass in major and minor version numbers.

    *LogHandle = NULL;          // Must be NULL so RPC fills it in

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        status = ElfrOpenBELA (
                    ServerNameString,
                    (PRPC_STRING)FileName,
                    ELF_VERSION_MAJOR,
                    ELF_VERSION_MINOR,
                    (PIELF_HANDLE) LogHandle
                    );
    }
    RpcExcept (1) {

        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept

    return (status);
}



NTSTATUS
ElfClearEventLogFileA (
    IN      HANDLE          LogHandle,
    IN      PANSI_STRING BackupFileName
    )

/*++

Routine Description:

  This is the client DLL entry point for the ElfClearEventLogFile API.
  The call is passed to the eventlog service on the appropriate server
  identified by LogHandle.


Arguments:

    LogHandle       - Handle returned from a previous "Open" call. This is
                      used to identify the module and the server.

    BackupFileName  - Name of the file to back up the current log file.
                      NULL implies not to back up the file.


Return Value:

    Returns an NTSTATUS code.


--*/
{
    NTSTATUS status;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //
    RpcTryExcept {

        // Call service entry point

        status = ElfrClearELFA (
                        (IELF_HANDLE) LogHandle,
                        (PRPC_STRING)BackupFileName
                        );

    }
    RpcExcept (1) {

        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept

    return (status);

}


NTSTATUS
ElfBackupEventLogFileA (
    IN      HANDLE       LogHandle,
    IN      PANSI_STRING BackupFileName
    )

/*++

Routine Description:

  This is the client DLL entry point for the ElfBackupEventLogFile API.
  The call is passed to the eventlog service on the appropriate server
  identified by LogHandle.


Arguments:

    LogHandle       - Handle returned from a previous "Open" call. This is
                      used to identify the module and the server.

    BackupFileName  - Name of the file to back up the current log file.


Return Value:

    Returns an NTSTATUS code.


--*/
{
    NTSTATUS status;
    NTSTATUS ApiStatus;
    BOOLEAN  fWasEnabled;

    //
    // Make sure input pointers are valid
    //

    if (!BackupFileName || BackupFileName->Length == 0) {
       return(STATUS_INVALID_PARAMETER);
    }

    //
    // Tacitly attempt to enable the SE_BACKUP_PRIVILEGE so we can
    // can check it on the server side
    //
    // Note that we make this call on behalf of the client to avoid
    // a regression when we check for the privilege on the server
    // side -- without this call, 3rd party apps that successfully
    // called this API before would fail.  Under normal circumstances,
    // this is not an encouraged practice.
    //

    //
    // BUGBUG -- This should really be done via ImpersonateSelf()
    //           and adjusting the thread token
    //
    ApiStatus = RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE,
                                   TRUE,
                                   FALSE,
                                   &fWasEnabled);

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //
    RpcTryExcept {

        // Call service entry point

        status = ElfrBackupELFA (
                        (IELF_HANDLE) LogHandle,
                        (PRPC_STRING)BackupFileName
                        );

    }
    RpcExcept (1) {

        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept

    //
    // Restore the client's privilege state to what it was before
    //

    if (NT_SUCCESS(ApiStatus)) {

        RtlAdjustPrivilege(SE_BACKUP_PRIVILEGE,
                           fWasEnabled,
                           TRUE,
                           &fWasEnabled);
    }

    return (status);
}



NTSTATUS
ElfReadEventLogA (
    IN          HANDLE      LogHandle,
    IN          ULONG       ReadFlags,
    IN          ULONG       RecordNumber,
    OUT         PVOID       Buffer,
    IN          ULONG       NumberOfBytesToRead,
    OUT         PULONG      NumberOfBytesRead,
    OUT         PULONG      MinNumberOfBytesNeeded
    )

/*++

Routine Description:

  This is the client DLL entry point for the ElfReadEventLog API.

Arguments:



Return Value:

    Returns an NTSTATUS code.


--*/
{
    NTSTATUS status;
    ULONG    FlagBits;

    //
    // Make sure the output pointers are valid
    //

    if (!Buffer || !NumberOfBytesRead || !MinNumberOfBytesNeeded) {
       return(STATUS_INVALID_PARAMETER);
    }

    //
    // Ensure that the ReadFlags we got are valid.
    // Make sure that one of each type of bit is set.
    //
    FlagBits = ReadFlags & (EVENTLOG_SEQUENTIAL_READ | EVENTLOG_SEEK_READ);

    if (   (FlagBits == (EVENTLOG_SEQUENTIAL_READ | EVENTLOG_SEEK_READ))
        || (FlagBits == 0)) {
        return(STATUS_INVALID_PARAMETER);
    }

    FlagBits = ReadFlags & (EVENTLOG_FORWARDS_READ | EVENTLOG_BACKWARDS_READ);

    if (   (FlagBits == (EVENTLOG_FORWARDS_READ | EVENTLOG_BACKWARDS_READ))
        || (FlagBits == 0)) {
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //
    RpcTryExcept {

        // Call service

        status = ElfrReadELA (
                        (IELF_HANDLE) LogHandle,
                        ReadFlags,
                        RecordNumber,
                        NumberOfBytesToRead,
                        Buffer,
                        NumberOfBytesRead,
                        MinNumberOfBytesNeeded
                        );

    }
    RpcExcept (1) {

        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept

    // Return status and bytes read/required.

    return (status);

}



NTSTATUS
ElfReportEventA (
    IN      HANDLE          LogHandle,
    IN      USHORT          EventType,
    IN      USHORT          EventCategory OPTIONAL,
    IN      ULONG           EventID,
    IN      PSID            UserSid,
    IN      USHORT          NumStrings,
    IN      ULONG           DataSize,
    IN      PANSI_STRING    *Strings,
    IN      PVOID           Data,
    IN      USHORT          Flags,
    IN OUT  PULONG          RecordNumber OPTIONAL,
    IN OUT  PULONG          TimeWritten  OPTIONAL
    )

/*++

Routine Description:

  This is the client DLL entry point for the ElfReportEvent API.

Arguments:


Return Value:

    Returns an NTSTATUS code.

Note:

    The last three parameters (Flags, RecordNumber and TimeWritten) are
    designed to be used by Security Auditing for the implementation of
    paired events (associating a file open event with the subsequent file
    close). This will not be implemented in Product 1, but the API is
    defined to allow easier support of this in a later release.


--*/
{
    NTSTATUS status;
    PANSI_STRING pComputerNameA;
    LARGE_INTEGER Time;
    ULONG EventTime;

    //
    // Generate the time of the event. This is done on the client side
    // since that is where the event occurred.
    //
    NtQuerySystemTime(&Time);
    RtlTimeToSecondsSince1970(&Time,
                          &EventTime
                         );

    //
    // Generate the ComputerName of the client.
    // We have to do this in the client side since this call may be
    // remoted to another server and we would not necessarily have
    // the computer name there.
    //
    pComputerNameA = TmpGetComputerNameA();

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //
    RpcTryExcept {

        // Call service

        status = ElfrReportEventA (
                    (IELF_HANDLE)   LogHandle,
                    EventTime,
                    EventType,
                    EventCategory,
                    EventID,
                    NumStrings,
                    DataSize,
                    (PRPC_STRING)pComputerNameA,
                    UserSid,
                    (PRPC_STRING*)Strings,
                    Data,
                    Flags,
                    RecordNumber,
                    TimeWritten
                    );

    }
    RpcExcept (1) {

        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept

    return (status);

}


/****
@func       NTSTATUS | ElfRegisterClusterSvc|  The cluster service registers
            itself with the event log service at initialization by calling this api.

@parm       IN PUNICODE_STRING | UNCServerName | Inidicates the server on which the
            cluster service will register with the eventlog service.  This must
            be the local node.

@parm       OUT PULONG | pulSize | A pointer to a long that returns the size of the
            packed event information structure that is returned.

@parm       OUT PPACKEDEVENTINFO | *ppPackedEventInfo| A pointer to the packed event information
            structure for propagation is returned via this parameter.

@comm       The elf client validates parameters and called the servier entry point.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref        <f ElfrRegisterClusterSvc>
****/
NTSTATUS
ElfRegisterClusterSvc (
    IN  PUNICODE_STRING     UNCServerName,
    OUT PULONG              pulSize,
    OUT PPACKEDEVENTINFO    *ppPackedEventInfo
    )
{
    EVENTLOG_HANDLE_W   ServerNameString;

    NTSTATUS status;

    if ((UNCServerName != NULL) && (UNCServerName->Length != 0))
    {
        ServerNameString = UNCServerName->Buffer;
    }
    else
    {
        ServerNameString = NULL;
    }

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //
    RpcTryExcept {

        // Call service

        status = ElfrRegisterClusterSvc (ServerNameString, pulSize,
                (PBYTE *)ppPackedEventInfo);

    }
    RpcExcept (1) {

        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept

    return(status);
}

/****
@func       NTSTATUS | ElfDeregisterClusterSvc|  Before stopping the cluster
            service deregisters itself for propagation of events from the
            eventlog service.

@parm       IN PUNICODE_STRING | UNCServerName | Inidicates the server on which the
            cluster service will register with the eventlog service.  This must
            be on the local node.

@comm       The elf client forwards this to the appropriate eventlog server entry point.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref       <f ElfDeregisterClusterSvc> <f ElfrDeregisterClusterSvc>
****/
NTSTATUS
ElfDeregisterClusterSvc(
    IN  PUNICODE_STRING     UNCServerName
    )
{

    NTSTATUS status;
    EVENTLOG_HANDLE_W   ServerNameString;

    if ((UNCServerName != NULL) && (UNCServerName->Length != 0))
    {
        ServerNameString = UNCServerName->Buffer;
    }
    else
    {
        ServerNameString = NULL;
    }

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //
    RpcTryExcept {

        // Call service

        status = ElfrDeregisterClusterSvc (ServerNameString);

    }
    RpcExcept (1) {

        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept
    return(status);

}


/****
@func       NTSTATUS | ElfWriteClusterEvents| The cluster service calls this
            api to log events reported at other nodes of the cluster.

@parm       IN EVENTLOG_HANDLE_W | UNCServerName | Not used.

@parm       IN ULONG | ulSize | The size of the    packed event information structure.

@parm       IN PACKEDEVENTINFO | pPackedEventInfo| A pointer to the packed event information
            structure for propagation.

@comm       The elf client validates the parameters and forwards this to the appropriate
            entry point in the eventlog server.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref
****/
NTSTATUS
ElfWriteClusterEvents(
    IN  PUNICODE_STRING     UNCServerName,
    IN  ULONG               ulSize,
    IN  PPACKEDEVENTINFO    pPackedEventInfo)
{

    NTSTATUS status;
    EVENTLOG_HANDLE_W   ServerNameString;

    //validate input parameters
    if (!pPackedEventInfo || !ulSize || (pPackedEventInfo->ulSize != ulSize))
       return(STATUS_INVALID_PARAMETER);

    if ((UNCServerName != NULL) && (UNCServerName->Length != 0))
    {
        ServerNameString = UNCServerName->Buffer;
    }
    else
    {
        ServerNameString = NULL;
    }

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //
    RpcTryExcept {

        // Call service

        status = ElfrWriteClusterEvents (ServerNameString, ulSize,
            (PBYTE)pPackedEventInfo);

    }
    RpcExcept (1) {

        status = I_RpcMapWin32Status(RpcExceptionCode());
    }
    RpcEndExcept
    return(status);

}
