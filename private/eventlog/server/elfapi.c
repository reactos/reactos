/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ELFAPI.C

Abstract:

    This module contains the server end of the Elf APIs.


Author:

    Rajen Shah  (rajens)    29-Jul-1991


Revision History:

    10-Sep-1998         jschwart
        Added ElfrGetLogInformation (GetEventlogInformation) API

    30-Jan-1995         MarkBl
        Backup operators are allowed to open the security log, but only
        to perform backup operations. All other operations are prohibited.

    13-Oct-1993         Danl
        ElfrOpenELA:  Fixed Memory Leak bug where it was not calling
        RtlFreeUnicodeString for pRegModuleNameU and PModuleNameU.

    29-Jul-1991         RajenS
        Created

--*/
/****
@doc    EXTERNAL INTERFACES EVTLOG
****/


//#include <rpcutil.h>

#include <eventp.h>
#include <elfcfg.h>
#include <stdio.h>  // sprintf
#include <stdlib.h>
#include <memory.h>
#include <clussprt.h>

//
// Maximum number of ChangeNotify requests per context handle
//
#define MAX_NOTIFY_REQUESTS     5


//
//  PROTOTYPES
//
NTSTATUS
ElfpOpenELW (
    IN  EVENTLOG_HANDLE_W   UNCServerName,
    IN  PRPC_UNICODE_STRING ModuleName,
    IN  PRPC_UNICODE_STRING RegModuleName,
    IN  ULONG               MajorVersion,
    IN  ULONG               MinorVersion,
    OUT PIELF_HANDLE        LogHandle,
    IN  ULONG               DesiredAccess
    );

NTSTATUS
ElfpOpenELA (
    IN  EVENTLOG_HANDLE_A   UNCServerName,
    IN  PRPC_STRING         ModuleName,
    IN  PRPC_STRING         RegModuleName,
    IN  ULONG               MajorVersion,
    IN  ULONG               MinorVersion,
    OUT PIELF_HANDLE        LogHandle,
    IN  ULONG               DesiredAccess
    );

VOID
FreePUStringArray (
    IN  PUNICODE_STRING  * pUStringArray,
    IN  USHORT             NumStrings
    );

NTSTATUS
VerifyElfHandle(
    IN IELF_HANDLE LogHandle
    );

NTSTATUS
VerifyUnicodeString(
    IN PUNICODE_STRING pUString
    );

NTSTATUS
VerifyAnsiString(
    IN PANSI_STRING pAString
    );


//
// These APIs only have one interface, since they don't take or return strings
//

NTSTATUS
ElfrNumberOfRecords(
    IN  IELF_HANDLE     LogHandle,
    OUT PULONG          NumberOfRecords
    )
/*++

Routine Description:

  This is the RPC server entry point for the ElfrCurrentRecord API.

Arguments:

    LogHandle       - The context-handle for this module's call.

    NumberOfRecords - Where to return the total number of records in the
                      log file.

Return Value:

    Returns an NTSTATUS code.


--*/
{

    PLOGMODULE Module;
    NTSTATUS   Status;

    //
    // Check the handle before proceeding.
    //

    Status = VerifyElfHandle(LogHandle);

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // Insure the caller has read access.
    //

    if (!(LogHandle->GrantedAccess & ELF_LOGFILE_READ)) {
        return(STATUS_ACCESS_DENIED);
    }

    //
    // Verify additional arguments.
    //

    if (NumberOfRecords == NULL) {
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // This condition is TRUE iff a backup operator has opened the security
    // log. In this case deny access, since backup operators are allowed
    // only backup operation on the security log.
    //

    if (LogHandle->GrantedAccess & ELF_LOGFILE_BACKUP) {
        return(STATUS_ACCESS_DENIED);
    }

    //
    // If the OldestRecordNumber is 0, that means we have an empty
    // file, else we calculate the difference between the oldest
    // and next record numbers
    //

    Module = FindModuleStrucFromAtom (LogHandle->Atom);

    if (Module != NULL) {
        *NumberOfRecords = Module->LogFile->OldestRecordNumber == 0 ? 0 :
        Module->LogFile->CurrentRecordNumber -
            Module->LogFile->OldestRecordNumber;
    }
    else {
        Status = STATUS_INVALID_HANDLE;
    }

    return (Status);

}


NTSTATUS
ElfrOldestRecord(
    IN  IELF_HANDLE         LogHandle,
    OUT PULONG          OldestRecordNumber
    )
{
    PLOGMODULE Module;
    NTSTATUS   Status;

    //
    // Check the handle before proceeding.
    //

    Status = VerifyElfHandle(LogHandle);

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // Insure the caller has read access.
    //

    if (!(LogHandle->GrantedAccess & ELF_LOGFILE_READ)) {
        return(STATUS_ACCESS_DENIED);
    }

    //
    // Verify additional arguments.
    //

    if (OldestRecordNumber == NULL) {
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // This condition is TRUE iff a backup operator has opened the security
    // log. In this case deny access, since backup operators are allowed
    // only backup operation on the security log.
    //

    if (LogHandle->GrantedAccess & ELF_LOGFILE_BACKUP)
    {
        return(STATUS_ACCESS_DENIED);
    }

    Module = FindModuleStrucFromAtom (LogHandle->Atom);

    if (Module != NULL) {
        *OldestRecordNumber = Module->LogFile->OldestRecordNumber;
    }
    else {
        Status = STATUS_INVALID_HANDLE;
    }

    return (Status);
}


NTSTATUS
ElfrChangeNotify(
    IN  IELF_HANDLE         LogHandle,
    IN  RPC_CLIENT_ID       ClientId,
    IN  ULONG               Event
    )
{
    NTSTATUS Status;
    NTSTATUS RpcStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ProcessHandle = NULL;
    HANDLE EventHandle;
    PLOGMODULE Module;
    PNOTIFIEE Notifiee;

    //
    // Check the handle before proceeding.
    //

    Status = VerifyElfHandle(LogHandle);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Ensure the caller has read access.
    // BUGBUG : Is this necessary?
    //

    if (!(LogHandle->GrantedAccess & ELF_LOGFILE_READ)) {
        return STATUS_ACCESS_DENIED;
    }

    //
    // First make sure that this is a local call and that it is not a
    // handle that was created for a backup log file
    //

    if (LogHandle->Flags & ELF_LOG_HANDLE_REMOTE_HANDLE ||
        LogHandle->Flags & ELF_LOG_HANDLE_BACKUP_LOG) {
            return STATUS_INVALID_HANDLE;
    }

    //
    // This condition is TRUE iff a backup operator has opened the security
    // log. In this case deny access, since backup operators are allowed
    // only backup operation on the security log.
    //

    if (LogHandle->GrantedAccess & ELF_LOGFILE_BACKUP) {
        return STATUS_ACCESS_DENIED;
    }

    //
    // Make sure the client has the right to open this process
    //

    RpcStatus = RpcImpersonateClient(NULL);

    if (RpcStatus != RPC_S_OK) {
        ElfDbgPrint(("RPC IMPERSONATION FAILED %d\n", RpcStatus));
        return RpcStatus;
    }

    //
    // First get a handle to the process using the passed in ClientId. Note
    // that the ClientId is supplied by the client so a rogue client may
    // supply any client ID. However, because we impersonate when opening
    // the process we don't get any additional access the client doesn't have.
    //

    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,                   // UNICODE string
                               0,                      // Attributes
                               NULL,                   // Root directory
                               NULL);                  // Security descriptor

    Status = NtOpenProcess(&ProcessHandle,
                           PROCESS_DUP_HANDLE,
                           &ObjectAttributes,
                           (PCLIENT_ID) &ClientId);

    RpcStatus = RpcRevertToSelf();

    if (RpcStatus != RPC_S_OK) {
        DbgPrint("RPC REVERT TO SELF FAILED %d\n", RpcStatus);
    }

    if (NT_SUCCESS(Status)) {

        //
        // Now dupe the handle they passed in for the event
        //

        Status = NtDuplicateObject(ProcessHandle,
                                   (HANDLE) Event,
                                   NtCurrentProcess(),
                                   &EventHandle,
                                   0,
                                   0,
                                   DUPLICATE_SAME_ACCESS);

         if (NT_SUCCESS(Status)) {

             //
             // Create a new NOTIFIEE control block to link in
             //

             Notifiee = ElfpAllocateBuffer(sizeof(NOTIFIEE));

             if (Notifiee) {

                 //
                 // Fill in the fields
                 //

                 Notifiee->Handle = LogHandle;
                 Notifiee->Event = EventHandle;

                 //
                 // Find the LOGFILE associated with this handle
                 //

                 Module = FindModuleStrucFromAtom (LogHandle->Atom);

                 if (Module != NULL) {

                     //
                     // Get exclusive access to the log file. This will ensure
                     // no one else is accessing the file.
                     //

                     RtlAcquireResourceExclusive (&Module->LogFile->Resource,
                                                  TRUE);   // Wait until available

                     //
                     // Enforce the limit of ChangeNotify requests per context handle
                     //

                     if (LogHandle->dwNotifyRequests == MAX_NOTIFY_REQUESTS) {

                         CloseHandle(EventHandle);
                         ElfpFreeBuffer(Notifiee);
                         Status = STATUS_INSUFFICIENT_RESOURCES;
                     }
                     else {

                         //
                         // Insert the new notifiee into the list and increment this
                         // context handle's ChangeNotify request count
                         //

                         InsertHeadList(&Module->LogFile->Notifiees,
                                        &Notifiee->Next);

                         LogHandle->dwNotifyRequests++;
                     }

                     //
                     // Free the resource
                     //

                     RtlReleaseResource ( &Module->LogFile->Resource );
                 }
                 else {

                     CloseHandle(EventHandle);
                     ElfpFreeBuffer(Notifiee);
                     Status = STATUS_INVALID_HANDLE;
                 }
             }
             else {
                 Status = STATUS_NO_MEMORY;

                 //
                 // Free the duplicated handle
                 //
                 CloseHandle(EventHandle);
             }
         }
    }
    else {
        if (Status == STATUS_INVALID_CID) {
            Status = STATUS_INVALID_HANDLE;
        }
    }

    if (ProcessHandle) {
        NtClose(ProcessHandle);
    }

    return Status;
}


NTSTATUS
ElfrGetLogInformation(
    IN     IELF_HANDLE    LogHandle,
    IN     ULONG          InfoLevel,
    OUT    PBYTE          lpBuffer,
    IN     ULONG          cbBufSize,
    OUT    PULONG         pcbBytesNeeded
    )
/*++

Routine Description:

  This is the RPC server entry point for the ElfrGetLogInformation API.

Arguments:

    LogHandle      - The context-handle for this module's call.
    InfoLevel      - Infolevel that specifies which information the user is requesting
    lpBuffer       - Buffer into which to place the information
    cbBufSize      - Size of lpBuffer, in bytes
    pcbBytesNeeded - Required size of the buffer

Return Value:

    Returns an NTSTATUS code.


--*/
{
    NTSTATUS   ntStatus;
    PLOGMODULE pLogModule;

    //
    // Check the handle before proceeding.
    //

    ntStatus = VerifyElfHandle(LogHandle);

    if (!NT_SUCCESS(ntStatus)) {
        return ntStatus;
    }

    //
    // This condition is TRUE iff a backup operator has opened the security
    // log. In this case deny access, since backup operators are allowed
    // only backup operation on the security log.
    //

    if (LogHandle->GrantedAccess & ELF_LOGFILE_BACKUP)
    {
        return(STATUS_ACCESS_DENIED);
    }

    //
    // Take the appropriate actions based on the Infolevel
    //
    switch (InfoLevel) {
            
        case EVENTLOG_FULL_INFO:

            *pcbBytesNeeded = sizeof(EVENTLOG_FULL_INFORMATION);

            if (cbBufSize < *pcbBytesNeeded) {
                ntStatus = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            //
            // Get the module associated with this log handle
            //
            pLogModule = FindModuleStrucFromAtom(LogHandle->Atom);

            if (pLogModule != NULL) {

                //
                // The caller has the permission for this operation.  Note
                // that an access check is done when opening the log, so
                // there's no need to repeat it here.
                //
                ((LPEVENTLOG_FULL_INFORMATION)lpBuffer)->dwFull =

                    ( pLogModule->LogFile->Flags & ELF_LOGFILE_LOGFULL_WRITTEN ?
                          TRUE :
                          FALSE);
            }
            else {
                ntStatus = STATUS_INVALID_HANDLE;
            }

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
ElfrClearELFW (
    IN  IELF_HANDLE         LogHandle,
    IN  PRPC_UNICODE_STRING BackupFileName
    )

/*++

Routine Description:

  This is the RPC server entry point for the ElfrClearELFW API.

Arguments:

    LogHandle       - The context-handle for this module's call.  This must
                      not have been returned from OpenBackupEventlog, or
                      this call will fail with invalid handle.

    BackupFileName  - Name of the file to back up the current log file.
                      NULL implies not to back up the file.


Return Value:

    Returns an NTSTATUS code.


--*/
{
    NTSTATUS            Status;
    PLOGMODULE          Module;
    ELF_REQUEST_RECORD  Request;
    CLEAR_PKT           ClearPkt;
    DWORD               status = NO_ERROR;

    //
    // Check the handle before proceeding.
    //

    Status = VerifyElfHandle(LogHandle);

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // Insure the caller has clear access.
    //

    if (!(LogHandle->GrantedAccess & ELF_LOGFILE_CLEAR)) {
        return(STATUS_ACCESS_DENIED);
    }

    //
    // Verify additional arguments.
    //

    if (BackupFileName != NULL) {

        Status = VerifyUnicodeString(BackupFileName);

        if (!NT_SUCCESS(Status)) {
            return(Status);
        }
    }

    //
    // Can't clear a backup log
    //

    if (LogHandle->Flags & ELF_LOG_HANDLE_BACKUP_LOG) {
        return(STATUS_INVALID_HANDLE);
    }

    //
    // This condition is TRUE iff a backup operator has opened the security
    // log. In this case deny access, since backup operators are allowed
    // only backup operation on the security log.
    //

    if (LogHandle->GrantedAccess & ELF_LOGFILE_BACKUP)
    {
        return(STATUS_ACCESS_DENIED);
    }

    //
    // Find the matching module structure
    //

    Module = FindModuleStrucFromAtom (LogHandle->Atom);

    Request.Pkt.ClearPkt = &ClearPkt;
    Request.Flags = 0;

    if (Module != NULL) {

        //
        // Verify that the caller has clear access to this logfile
        //

        if (! RtlAreAllAccessesGranted (
                LogHandle->GrantedAccess,
                ELF_LOGFILE_CLEAR)) {

            Status = STATUS_ACCESS_DENIED;
        }

        if (NT_SUCCESS(Status)) {

            //
            // Fill in the request packet
            //

            Request.Module = Module;
            Request.LogFile = Module->LogFile;
            Request.Command = ELF_COMMAND_CLEAR;
            Request.Status = STATUS_SUCCESS;
            Request.Pkt.ClearPkt->BackupFileName =
                                (PUNICODE_STRING)BackupFileName;

            //
            // Call the worker routine to do the operation.
            //

            ElfPerformRequest (&Request);

            //
            // Extract status of operation from the request packet
            //

            Status = Request.Status;

            //
            // If this was the Security Logfile, and the clear was
            // successful, then generate an audit.
            //
            if (NT_SUCCESS(Status) &&
                (_wcsicmp(ELF_SECURITY_MODULE_NAME,
                          Module->LogFile->LogModuleName->Buffer) == 0)) {

                //
                // We just cleared the security log.  Now we want to add
                // a new event to that log to indicate who did it.
                //
                ElfpGenerateLogClearedEvent(LogHandle);
            }
        }
    }
    else {
        Status = STATUS_INVALID_HANDLE;
    }

    return (Status);
}


NTSTATUS
ElfrBackupELFW (
    IN  IELF_HANDLE         LogHandle,
    IN  PRPC_UNICODE_STRING BackupFileName
    )

/*++

Routine Description:

  This is the RPC server entry point for the ElfrBackupELFW API.

Arguments:

    LogHandle       - The context-handle for this module's call.

    BackupFileName  - Name of the file to back up the current log file.


Return Value:

    Returns an NTSTATUS code.


--*/
{
    NTSTATUS            Status;
    PLOGMODULE          Module;
    ELF_REQUEST_RECORD  Request;
    BACKUP_PKT          BackupPkt;

    //
    // Check the handle before proceeding.
    //

    Status = VerifyElfHandle(LogHandle);

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // Ensure the caller has read access.
    //

    if (!(LogHandle->GrantedAccess & ELF_LOGFILE_READ)) {
        return(STATUS_ACCESS_DENIED);
    }

    //
    // Make sure the client has SE_BACKUP_PRIVILEGE enabled.  Note
    // that we attempted to enable this on the client side
    //
    
    if (ElfpTestClientPrivilege(SE_BACKUP_PRIVILEGE, NULL) != STATUS_SUCCESS) {

        ElfDbgPrint(("[ELF] ElfrBackupELFW: Client does not have SE_BACKUP_PRIVILEGE\n"));

        return(STATUS_PRIVILEGE_NOT_HELD);
    }

    //
    // Verify additional arguments.
    //

    Status = VerifyUnicodeString(BackupFileName);

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // A filename must be specified.
    //

    if (BackupFileName->Length == 0) {
        return(STATUS_INVALID_PARAMETER);
    }

    Request.Pkt.BackupPkt = &BackupPkt;
    Request.Flags = 0;

    //
    // Find the matching module structure
    //

    Module = FindModuleStrucFromAtom (LogHandle->Atom);

    if (Module != NULL) {

        //
        // Fill in the request packet

        Request.Module  = Module;
        Request.LogFile = Module->LogFile;
        Request.Command = ELF_COMMAND_BACKUP;
        Request.Status  = STATUS_SUCCESS;
        Request.Pkt.BackupPkt->BackupFileName =
                            (PUNICODE_STRING)BackupFileName;

        //
        // Call the worker routine to do the operation.
        //

        ElfPerformRequest (&Request);

        //
        // Extract status of operation from the request packet
        //

        Status = Request.Status;

    } else {
        Status = STATUS_INVALID_HANDLE;
    }

    return (Status);

}


NTSTATUS
ElfrCloseEL (
    IN OUT  PIELF_HANDLE    LogHandle
    )

/*++

Routine Description:

  This is the RPC server entry point for the ElfrCloseEL API.

Arguments:


Return Value:

    Returns an NTSTATUS code.


--*/
{
    NTSTATUS Status;

    //
    // Check the handle before proceeding.
    //

    if (LogHandle == NULL) {
        return(STATUS_INVALID_PARAMETER);
    }

    Status = VerifyElfHandle(*LogHandle);

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // Call the rundown routine to do all the work
    //

    IELF_HANDLE_rundown(*LogHandle);

    *LogHandle = NULL; // so RPC knows it's closed

    return (STATUS_SUCCESS);
}


NTSTATUS
ElfrDeregisterEventSource (
    IN OUT  PIELF_HANDLE    LogHandle
    )

/*++

Routine Description:

  This is the RPC server entry point for the ElfrDeregisterEventSource API.

Arguments:


Return Value:

    Returns an NTSTATUS code.


--*/
{
    NTSTATUS Status;

    if (LogHandle == NULL) {
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // Check the handle before proceeding.
    //

    Status = VerifyElfHandle(*LogHandle);

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // This condition is TRUE iff a backup operator has opened the security
    // log. In this case deny access, since backup operators are allowed
    // only backup operation on the security log.
    //

    if ((*LogHandle)->GrantedAccess & ELF_LOGFILE_BACKUP)
    {
        return(STATUS_ACCESS_DENIED);
    }

    //
    // Call the rundown routine to do all the work
    //

    IELF_HANDLE_rundown(*LogHandle);

    *LogHandle = NULL; // so RPC knows it's closed

    return (STATUS_SUCCESS);
}




NTSTATUS
ElfrOpenBELW (
    IN  EVENTLOG_HANDLE_W   UNCServerName,
    IN  PRPC_UNICODE_STRING BackupFileName,
    IN  ULONG               MajorVersion,
    IN  ULONG               MinorVersion,
    OUT PIELF_HANDLE        LogHandle
    )

/*++

Routine Description:

  This is the RPC server entry point for the ElfrOpenBELW API.  It creates
  a module structure $BACKUPnnn where nnn is a unique number for every backup
  log that is opened.  It then calls ElfpOpenELW to actually open the file.


Arguments:

    UNCServerName   - Not used.

    BackupFileName      - Name of the backup log file.

    MajorVersion/MinorVersion - The version of the client.


    LogHandle       - Pointer to the place where the pointer to the
                              context handle structure will be placed.

Return Value:

    Returns an NTSTATUS code and, if no error, a "handle".


--*/
{

    NTSTATUS        Status;
    UNICODE_STRING  BackupStringW;
    LPWSTR          BackupModuleName;
    PLOGMODULE      pModule;
    DWORD           dwModuleNumber;

//
// Size of buffer (in bytes) required for a UNICODE string of $BACKUPnnn
//

#define SIZEOF_BACKUP_MODULE_NAME 64

    UNREFERENCED_PARAMETER(UNCServerName);

    //
    // Check arguments.
    //

    Status = VerifyUnicodeString(BackupFileName);

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // A filename must be specified.
    //

    if (BackupFileName->Length == 0) {
        return(STATUS_INVALID_PARAMETER);
    }

    if (LogHandle == NULL) {
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // Create a unique module name by incrementing a global value
    //

    BackupModuleName = ElfpAllocateBuffer(SIZEOF_BACKUP_MODULE_NAME);
    if (BackupModuleName == NULL) {
        return(STATUS_NO_MEMORY);
    }

    //
    // Serialize read, increment of the global backup module number.
    // Note: double-timing the log file list critical section so as to not
    // require another critical section specifically dedicated to this
    // operation.
    //

    RtlEnterCriticalSection ((PRTL_CRITICAL_SECTION)&LogFileCritSec);

    dwModuleNumber = BackupModuleNumber++;

    RtlLeaveCriticalSection ((PRTL_CRITICAL_SECTION)&LogFileCritSec);

    swprintf(BackupModuleName, L"$BACKUP%06d", dwModuleNumber);
    RtlInitUnicodeString(&BackupStringW, BackupModuleName);

    //
    // Call SetupDataStruct to build the module and log data structures
    // and actually open the file.
    //
    // NOTE:  If this call is successful, the Unicode String Buffer for
    //  BackupStringW (otherwise known as BackupModuleName) will be attached
    //  to the LogModule structure, and should not be free'd.
    //
    //

    Status = SetUpDataStruct(
                    BackupFileName,  // Filename
                    0,               // Max size, it will use actual
                    0,               // retention period, not used for bkup
                    ELF_GUEST_ACCESS_UNRESTRICTED,  // restrict guest
                                     // access flag, inapplicable for bkup
                    &BackupStringW,  // Module name
                    NULL,            // Handle to registry, not used
                    ElfBackupLog,    // Log type
                    LOGPOPUP_NEVER_SHOW
                    );

    if (!NT_SUCCESS(Status)) {
        ElfpFreeBuffer(BackupModuleName);
        return(Status);
    }

    //
    // Call ElfOpenELW to actually open the log file and get a handle.
    //
    if (NT_SUCCESS(Status)) {

        Status = ElfpOpenELW(NULL,
                             (PRPC_UNICODE_STRING) & BackupStringW,
                             NULL,
                             MajorVersion,
                             MinorVersion,
                             LogHandle,
                             ELF_LOGFILE_READ);
    }

    if (NT_SUCCESS(Status)) {

        //
        // Mark this as a handle for a backup log, so we can clean up
        // differently when it's closed, as well as disallow clear, backup
        // and write operations.
        //

        (*LogHandle)->Flags |= ELF_LOG_HANDLE_BACKUP_LOG;

    }
    else {
        //
        // If we couldn't open the log file, then we need to tear down
        // the DataStruct we set up with SetUpDataStruct.
        //
        pModule = GetModuleStruc ((PUNICODE_STRING)BackupFileName);

        Status = ElfpCloseLogFile (pModule->LogFile, ELF_LOG_CLOSE_BACKUP);

        UnlinkLogModule(pModule);
        DeleteAtom(pModule->ModuleAtom);

        // NB : DO NOT decrement the logfile reference count - this has
        //      been done in ElfpCloseLogFile.
        if (pModule->LogFile->RefCount == 0) {
            UnlinkLogFile(pModule->LogFile); // Unlink the structure
            RtlDeleteResource ( &pModule->LogFile->Resource );
            RtlDeleteSecurityObject(&pModule->LogFile->Sd);
            ElfpFreeBuffer (pModule->LogFile->LogFileName);
            ElfpFreeBuffer (pModule->LogFile->LogModuleName);
            ElfpFreeBuffer (pModule->LogFile);
        }
        ElfpFreeBuffer(pModule->ModuleName);
        ElfpFreeBuffer(pModule);

    }

    return(Status);
}


NTSTATUS
ElfrRegisterEventSourceW (
    IN  EVENTLOG_HANDLE_W   UNCServerName,
    IN  PRPC_UNICODE_STRING ModuleName,
    IN  PRPC_UNICODE_STRING RegModuleName,
    IN  ULONG               MajorVersion,
    IN  ULONG               MinorVersion,
    OUT PIELF_HANDLE        LogHandle
    )

/*++

Routine Description:

  This is the RPC server entry point for the ElfrRegisterEventSourceW API.
  This routine allocates a structure for the context handle, finds
  the matching module name and fills in the data. It returns the
  pointer to the handle structure.


Arguments:

    UNCServerName   - Not used.

    ModuleName      - Name of the module that is making this call.

    RegModuleName   - Not used.

    MajorVersion/MinorVersion - The version of the client.


    LogHandle       - Pointer to the place where the pointer to the
                      context handle structure will be placed.

Return Value:

    Returns an NTSTATUS code and, if no error, a "handle".

Note:

    For now, just call ElfpOpenELW.


--*/
{
    //
    // All arguments checked in ElfpOpenELW.
    //

    return(ElfpOpenELW(UNCServerName, ModuleName, RegModuleName,
        MajorVersion, MinorVersion, LogHandle, ELF_LOGFILE_WRITE));
}

NTSTATUS
ElfrOpenELW (
    IN  EVENTLOG_HANDLE_W   UNCServerName,
    IN  PRPC_UNICODE_STRING ModuleName,
    IN  PRPC_UNICODE_STRING RegModuleName,
    IN  ULONG               MajorVersion,
    IN  ULONG               MinorVersion,
    OUT PIELF_HANDLE        LogHandle
    )

/*++

Routine Description:

  This is the RPC server entry point for the ElfrOpenELW API.
  This routine allocates a structure for the context handle, finds
  the matching module name and fills in the data. It returns the
  pointer to the handle structure.


Arguments:

    UNCServerName   - Not used.

    ModuleName      - Name of the module that is making this call.

    RegModuleName   - Not used.

    MajorVersion/MinorVersion - The version of the client.


    LogHandle       - Pointer to the place where the pointer to the
                      context handle structure will be placed.

Return Value:

    Returns an NTSTATUS code and, if no error, a "handle".


--*/
{
    //
    // All arguments checked in ElfpOpenELW.
    //

    return( ElfpOpenELW (
            UNCServerName,
            ModuleName,
            RegModuleName,
            MajorVersion,
            MinorVersion,
            LogHandle,
            ELF_LOGFILE_READ));
}

NTSTATUS
ElfpOpenELW (
    IN  EVENTLOG_HANDLE_W   UNCServerName,
    IN  PRPC_UNICODE_STRING ModuleName,
    IN  PRPC_UNICODE_STRING RegModuleName,
    IN  ULONG               MajorVersion,
    IN  ULONG               MinorVersion,
    OUT PIELF_HANDLE        LogHandle,
    IN  ULONG               DesiredAccess
    )

/*++

Routine Description:

  Looks alot like ElfrOpenELW but also gets passed a DesiredAccess.

Arguments:

    UNCServerName   - Not used.

    ModuleName      - Name of the module that is making this call.

    RegModuleName   - Not used.

    MajorVersion/MinorVersion - The version of the client.


    LogHandle       - Pointer to the place where the pointer to the
                      context handle structure will be placed.

    DesiredAccess   - Indicates the access desired for this logfile.

Return Value:

    Returns an NTSTATUS code and, if no error, a "handle".


--*/
{
    NTSTATUS        Status;
    PLOGMODULE      Module;
    IELF_HANDLE     LogIHandle;
    BOOL            ForSecurityLog = FALSE;

    //
    // Check arguments.
    //

    Status = VerifyUnicodeString(ModuleName);

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    if (LogHandle == NULL) {
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // Allocate a new structure for the context handle
    //

    LogIHandle = (IELF_HANDLE) ElfpAllocateBuffer (
                                    sizeof (*LogIHandle)
                                  + ModuleName->Length
                                  + sizeof (WCHAR)
                                  );

    if (LogIHandle) {

        //
        // Find the module structure in order to pull out the Atom.
        //
        // GetModuleStruc *always* succeeds! (returns default if module
        // not found).
        //

        Module = GetModuleStruc ((PUNICODE_STRING)ModuleName);

        //
        // Validate the caller has appropriate access to this logfile.
        // If this is the security log, then check privilege instead.
        //
        if (_wcsicmp(ELF_SECURITY_MODULE_NAME, Module->LogFile->LogModuleName->Buffer) == 0) {
            ForSecurityLog = TRUE;
        }
        Status = ElfpAccessCheckAndAudit(
            L"EventLog",            // SubSystemName
            L"LogFile",             // ObjectTypeName
            Module->ModuleName,     // ObjectName
            LogIHandle,             // Context handle - required?
            Module->LogFile->Sd,    // Security Descriptor
            DesiredAccess,          // Requested Access
            NULL,                   // GENERIC_MAPPING
            ForSecurityLog          // Indicates the check is for security log
            );

        if (NT_SUCCESS(Status)) {

            LogIHandle->Atom = Module->ModuleAtom;

            LogIHandle->NameLength = ModuleName->Length + sizeof(WCHAR);
            RtlMoveMemory( LogIHandle->Name,
                            ModuleName->Buffer,
                            ModuleName->Length
                         );

            LogIHandle->Name[ModuleName->Length / sizeof(WCHAR)] = L'\0';

            LogIHandle->MajorVersion = MajorVersion; // Store the version
            LogIHandle->MinorVersion = MinorVersion; // of the client

            //
            // Initialize seek positions and flags to zero.
            //

            LogIHandle->SeekRecordPos    = 0;
            LogIHandle->SeekBytePos      = 0;
            LogIHandle->Flags            = 0;
            LogIHandle->dwNotifyRequests = 0;

            //
            // Link in this structure to the list of context handles
            //

            LogIHandle->Signature = ELF_CONTEXTHANDLE_SIGN; // DEBUG
            LinkContextHandle (LogIHandle);

            *LogHandle = LogIHandle;                // Set return handle
            Status = STATUS_SUCCESS;                // Set return status
        }
        else {
            ElfpFreeBuffer(LogIHandle);
        }

    } else {

        Status = STATUS_NO_MEMORY;
    }

    return (Status);

    UNREFERENCED_PARAMETER(UNCServerName);
    UNREFERENCED_PARAMETER(RegModuleName);
}



NTSTATUS
w_ElfrReadEL (
    IN      ULONG       Flags,                  // ANSI or UNICODE
    IN      IELF_HANDLE LogHandle,
    IN      ULONG       ReadFlags,
    IN      ULONG       RecordNumber,
    IN      ULONG       NumberOfBytesToRead,
    IN      PBYTE       Buffer,
    OUT     PULONG      NumberOfBytesRead,
    OUT     PULONG      MinNumberOfBytesNeeded
    )

/*++

Routine Description:

  This is the worker for the ElfrReadEL APIs.

Arguments:

   Same as ElfrReadELW API except that Flags contains an indication
   of whether this is ANSI or UNICODE.

Return Value:

    Same as the main API.

NOTES:

    We assume that the client-side has validated the flags to ensure that
    only one type of each bit is set. No checking is done at the server end.


--*/
{
    NTSTATUS            Status;
    PLOGMODULE          Module;
    ELF_REQUEST_RECORD  Request;
    READ_PKT            ReadPkt;

    //
    // Check the handle before proceeding.
    //

    Status = VerifyElfHandle(LogHandle);

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // Insure the caller has read access.
    //

    if (!(LogHandle->GrantedAccess & ELF_LOGFILE_READ)) {
        return(STATUS_ACCESS_DENIED);
    }

    //
    // Verify additional arguments.
    //

    if (Buffer == NULL || !NumberOfBytesRead || !MinNumberOfBytesNeeded) {
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // The ELF_HANDLE_INVALID_FOR_READ flag bit would be set if the
    // file changed underneath this handle.
    //

    if (LogHandle->Flags & ELF_LOG_HANDLE_INVALID_FOR_READ) {
       return(STATUS_EVENTLOG_FILE_CHANGED);
    }

    //
    // This condition is TRUE iff a backup operator has opened the security
    // log. In this case deny access, since backup operators are allowed
    // only backup operation on the security log.
    //

    if (LogHandle->GrantedAccess & ELF_LOGFILE_BACKUP)
    {
        return(STATUS_ACCESS_DENIED);
    }

    Request.Pkt.ReadPkt = &ReadPkt; // Set up read packet in request packet

    //
    // Find the matching module structure
    //
    Module = FindModuleStrucFromAtom (LogHandle->Atom);

    //
    // Only continue if the module was found
    //

    if (Module != NULL) {

        //
        // Fill in the request packet
        //
        Request.Module = Module;
        Request.Flags = 0;
        Request.LogFile = Module->LogFile;
        Request.Command = ELF_COMMAND_READ;
        Request.Status = STATUS_SUCCESS;
        Request.Pkt.ReadPkt->MinimumBytesNeeded = *MinNumberOfBytesNeeded;
        Request.Pkt.ReadPkt->BufferSize = NumberOfBytesToRead;
        Request.Pkt.ReadPkt->Buffer = (PVOID)Buffer;
        Request.Pkt.ReadPkt->ReadFlags = ReadFlags;
        Request.Pkt.ReadPkt->RecordNumber = RecordNumber;
        Request.Pkt.ReadPkt->LastSeekPos = LogHandle->SeekBytePos;
        Request.Pkt.ReadPkt->LastSeekRecord = LogHandle->SeekRecordPos;
        Request.Pkt.ReadPkt->Flags = Flags;     // Indicate UNICODE or ANSI

        //
        // Pass along whether the last read was in a forward or backward
        // direction (affects how we treat being at EOF). Then reset the
        // bit in the handle depending on what this read is.
        //

        if (LogHandle->Flags & ELF_LOG_HANDLE_LAST_READ_FORWARD) {
            Request.Pkt.ReadPkt->Flags |= ELF_LAST_READ_FORWARD;
        }

        if (ReadFlags & EVENTLOG_FORWARDS_READ) {
            LogHandle->Flags |= ELF_LOG_HANDLE_LAST_READ_FORWARD;
        }
        else {
            LogHandle->Flags &= ~(ELF_LOG_HANDLE_LAST_READ_FORWARD);
        }


        //
        // Perform the operation
        //
        ElfPerformRequest( &Request );

        //
        // Update current seek positions
        //
        LogHandle->SeekRecordPos = Request.Pkt.ReadPkt->LastSeekRecord;
        LogHandle->SeekBytePos = Request.Pkt.ReadPkt->LastSeekPos;

        //
        // Set up return values
        //
        *NumberOfBytesRead = Request.Pkt.ReadPkt->BytesRead;
        *MinNumberOfBytesNeeded = Request.Pkt.ReadPkt->MinimumBytesNeeded;

        Status = Request.Status;

    } else {
        Status = STATUS_INVALID_HANDLE;
        //
        // Set the NumberOfBytesNeeded to zero since there are no bytes to
        // transfer.
        //
        *NumberOfBytesRead = 0;
        *MinNumberOfBytesNeeded = 0;
    }

    return (Status);
}


NTSTATUS
ElfrReadELW (
    IN      IELF_HANDLE LogHandle,
    IN      ULONG       ReadFlags,
    IN      ULONG       RecordNumber,
    IN      ULONG       NumberOfBytesToRead,
    IN      PBYTE       Buffer,
    OUT     PULONG      NumberOfBytesRead,
    OUT     PULONG      MinNumberOfBytesNeeded
    )

/*++

Routine Description:

  This is the RPC server entry point for the ElfrReadELW API.

Arguments:



Return Value:

    Returns an NTSTATUS code, NumberOfBytesRead if the read was successful
    and MinNumberOfBytesNeeded if the buffer was not big enough.


--*/
{
    NTSTATUS Status;

    //
    // All arguments checked in w_ElfrReadEL.
    //

    //
    // Call the worker with the UNICODE flag
    //

    return(w_ElfrReadEL (
                        ELF_IREAD_UNICODE,
                        LogHandle,
                        ReadFlags,
                        RecordNumber,
                        NumberOfBytesToRead,
                        Buffer,
                        NumberOfBytesRead,
                        MinNumberOfBytesNeeded
                        ));

}



NTSTATUS
ElfrReportEventW (
    IN      IELF_HANDLE LogHandle,
    IN      ULONG               EventTime,
    IN      USHORT              EventType,
    IN      USHORT              EventCategory OPTIONAL,
    IN      ULONG               EventID,
    IN      USHORT              NumStrings,
    IN      ULONG               DataSize,
    IN      PRPC_UNICODE_STRING ComputerName,
    IN      PRPC_SID            UserSid,
    IN      PRPC_UNICODE_STRING Strings[],
    IN      PBYTE               Data,
    IN      USHORT              Flags,
    IN OUT  PULONG              RecordNumber OPTIONAL,
    IN OUT  PULONG              TimeWritten  OPTIONAL
    )

/*++

Routine Description:

  This is the RPC server entry point for the ElfrReportEventW API.

Arguments:


Return Value:

    Returns an NTSTATUS code.


--*/
{
    NTSTATUS            Status;
    PLOGMODULE          Module;
    ELF_REQUEST_RECORD  Request;
    WRITE_PKT           WritePkt;

    ULONG RecordLength;
    ULONG StringOffset, DataOffset;
    ULONG StringsSize;
    USHORT i;
    PVOID EventBuffer;
    PEVENTLOGRECORD EventLogRecord;
    PWSTR  ReplaceStrings, SrcString;
    PBYTE  BinaryData;
    PUNICODE_STRING  UComputerName;
    PWSTR   UModuleName;
    ULONG   PadSize;
    ULONG   UserSidLength = 0;              // Init to zero
    ULONG   UserSidOffset;
    ULONG   ModuleNameLen, ComputerNameLen; // Length in bytes
    ULONG   zero = 0;                       // For pad bytes
    LARGE_INTEGER    Time;
    ULONG   LogTimeWritten;

    //
    // These are for Security Auditing to use for paired events.  This will
    // not be implemented in Product 1
    //

    UNREFERENCED_PARAMETER(RecordNumber);
    UNREFERENCED_PARAMETER(TimeWritten);

    //
    // Check the handle before proceeding.
    //

    Status = VerifyElfHandle(LogHandle);

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // Insure the caller has write access.
    //

    if (!(LogHandle->GrantedAccess & ELF_LOGFILE_WRITE)) {
        return(STATUS_ACCESS_DENIED);
    }

    //
    // Verify additional arguments.
    //

    Status = VerifyUnicodeString(ComputerName);

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    if (Strings == NULL && NumStrings != 0) {
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // This condition is TRUE iff a backup operator has opened the security
    // log. In this case deny access, since backup operators are allowed
    // only backup operation on the security log.
    //

    if (LogHandle->GrantedAccess & ELF_LOGFILE_BACKUP)
    {
        return(STATUS_ACCESS_DENIED);
    }

    //
    // Make sure the SID passed in is valid
    //

    if (ARGUMENT_PRESENT(UserSid)) {
        if (!IsValidSid(UserSid)) {
            return(STATUS_INVALID_PARAMETER);
        }
    }

    //
    // Verify the string arguments
    //
    for (i = 0; i < NumStrings; i++ ) {

        Status = VerifyUnicodeString(Strings[i]);

        if (!NT_SUCCESS(Status)) {
            return(STATUS_INVALID_PARAMETER);
        }
    }

    //
    // Can't write to a backup log
    //

    if (LogHandle->Flags & ELF_LOG_HANDLE_BACKUP_LOG) {
        return(STATUS_INVALID_HANDLE);
    }

    //
    // Make sure they didn't pass in a null pointer for the data, but tell
    // me there was something there (I still think RPC should protect me from
    // this!)
    //

    if (!Data && DataSize != 0) {
        return(STATUS_INVALID_PARAMETER);
    }


    UComputerName = (PUNICODE_STRING)ComputerName;
    UModuleName = LogHandle->Name;

    Request.Pkt.WritePkt = &WritePkt;   // Set up write packet in request packet
    Request.Flags = 0;

    //
    // Find the matching module structure
    //

    Module = FindModuleStrucFromAtom (LogHandle->Atom);

    if (Module != NULL) {

        //
        // Generate any additional information needed in the record.
        //
        // Info that we have                Info to generate
        // -----------------                ----------------
        //  Modulename                      UserSidLength
        //  EventType                       Length
        //  EventID                         StringOffset
        //  NumStrings                      DataOffset
        //  Strings                         PadBytes
        //  DataLength                      LogTimeWritten
        //  Data
        //  UserSidOffset
        //  UserSid
        //  ComputerName
        //  TimeGenerated
        //

        // LogTimeWritten
        // We need to generate a time when the log is written. This
        // gets written in the log so that we can use it to test the
        // retention period when wrapping the file.
        //

        NtQuerySystemTime(&Time);
        RtlTimeToSecondsSince1970(
                            &Time,
                            &LogTimeWritten
                            );


        //
        // USERSIDLENTGH
        //

        if (UserSid) {
            UserSidLength = RtlLengthSid ((PSID)UserSid);
        }

        //
        // USERSIDOFFSET
        //
        // Extract the lengths from the STRING structure, and take care of
        // the trailing NULLs.
        //

        ModuleNameLen = (wcslen(UModuleName) + 1)
                        * sizeof (WCHAR);
        ComputerNameLen = UComputerName->Length + sizeof(WCHAR);

        UserSidOffset = sizeof(EVENTLOGRECORD)
                      + ModuleNameLen
                      + ComputerNameLen;

        //
        // STRING OFFSET:
        //

        StringOffset = UserSidOffset + UserSidLength;


        //
        // Calculate the length of strings so that we can see how
        // much space is needed for that.
        //

        StringsSize = 0;

        for (i = 0; i < NumStrings; i++) {
                StringsSize += Strings[i]->Length + sizeof(WCHAR);
        }

        //
        // DATA OFFSET:
        //

        DataOffset = StringOffset + StringsSize;

        //
        // Determine how big a buffer is needed for the eventlog record.
        //

        RecordLength = DataOffset
                     + DataSize
                     + sizeof(RecordLength); // Size excluding pad bytes

        //
        // Determine how many pad bytes are needed to align to a DWORD
        // boundary.
        //

        PadSize = sizeof(ULONG) - (RecordLength % sizeof(ULONG));

        RecordLength += PadSize;    // True size needed

        //
        // Allocate the buffer for the Eventlog record
        //

        EventBuffer = ElfpAllocateBuffer(RecordLength);

        if (EventBuffer != NULL) {

            //
            // Fill up the event record
            //

            EventLogRecord = (PEVENTLOGRECORD)EventBuffer;

            EventLogRecord->Length = RecordLength;
            EventLogRecord->TimeGenerated = EventTime;
            EventLogRecord->Reserved  = ELF_LOG_FILE_SIGNATURE;
            EventLogRecord->TimeWritten = LogTimeWritten;
            EventLogRecord->EventID = EventID;
            EventLogRecord->EventType = EventType;
            EventLogRecord->EventCategory = EventCategory;
            EventLogRecord->ReservedFlags = Flags;
            EventLogRecord->ClosingRecordNumber = 0;
            EventLogRecord->NumStrings = NumStrings;
            EventLogRecord->StringOffset = StringOffset;
            EventLogRecord->DataLength = DataSize;
            EventLogRecord->DataOffset = DataOffset;
            EventLogRecord->UserSidLength = UserSidLength;
            EventLogRecord->UserSidOffset = UserSidOffset;

            //
            // Fill in the variable-length fields
            //

            //
            // STRINGS
            //

            ReplaceStrings = (PWSTR) (  (ULONG_PTR)EventLogRecord
                                      + (ULONG)StringOffset
                                     );

            for (i = 0; i < NumStrings; i++) {

                SrcString = (PWSTR)Strings[i]->Buffer;
                RtlMoveMemory(ReplaceStrings, SrcString, Strings[i]->Length);

                ReplaceStrings[Strings[i]->Length / sizeof(WCHAR)] = L'\0';
                ReplaceStrings = (PWSTR)((PBYTE) ReplaceStrings
                        + Strings[i]->Length + sizeof(WCHAR));
            }

            //
            // MODULENAME
            //

            BinaryData = (PBYTE) EventLogRecord + sizeof(EVENTLOGRECORD);
            RtlMoveMemory (BinaryData,
                           UModuleName,
                           ModuleNameLen);

            //
            // COMPUTERNAME
            //

            ReplaceStrings = (LPWSTR) (BinaryData + ModuleNameLen);

            RtlMoveMemory (ReplaceStrings,
                           UComputerName->Buffer,
                           UComputerName->Length);

            ReplaceStrings[UComputerName->Length / sizeof(WCHAR)] = L'\0';

            //
            // USERSID
            //

            BinaryData = (PBYTE) ReplaceStrings + ComputerNameLen;

            ASSERT (BinaryData
                    == ((PBYTE) EventLogRecord) + UserSidOffset);

            RtlMoveMemory (BinaryData,
                           UserSid,
                           UserSidLength);

            //
            // BINARY DATA
            //

            BinaryData = (PBYTE) ((ULONG_PTR)EventLogRecord + DataOffset);
            if (Data) {
                RtlMoveMemory (BinaryData, Data, DataSize);
            }

            //
            // PAD  - Fill with zeros
            //

            BinaryData = (PBYTE) ((ULONG_PTR)BinaryData + DataSize);
            RtlMoveMemory (BinaryData, &zero, PadSize);

            //
            // LENGTH at end of record
            //

            BinaryData = (PBYTE)((ULONG_PTR)BinaryData + PadSize);// Point after pad bytes
            ((PULONG)BinaryData)[0] = RecordLength;

            //
            // Make sure we are in the right place
            //

            ASSERT ((ULONG_PTR)BinaryData
                == (RecordLength + (ULONG_PTR)EventLogRecord) - sizeof(ULONG));

            //
            // Set up request packet.
            // Link event log record into the request structure.
            //

            Request.Module = Module;
            Request.LogFile = Request.Module->LogFile;
            Request.Command = ELF_COMMAND_WRITE;
            Request.Pkt.WritePkt->Buffer = (PVOID)EventBuffer;
            Request.Pkt.WritePkt->Datasize = RecordLength;

            //
            // Perform the operation
            //

            ElfPerformRequest( &Request );

            //acquire the critical section for this global propagation area
            RtlEnterCriticalSection ((PRTL_CRITICAL_SECTION)&gClPropCritSec);

            if (gbClustering && (RecordLength < MAXSIZE_OF_EVENTSTOPROP))
            {


                PEVENTSFORLOGFILE   pEventsForLogFile;
                DWORD               dwError;


                gpClPackedEventInfo->ulSize = (sizeof(PACKEDEVENTINFO)
                    + sizeof(ULONG) //size of offsets, currently 1
                    + sizeof(EVENTSFORLOGFILE) //sizeof eventsforlogfile structure
                    + RecordLength);

                pEventsForLogFile = (PEVENTSFORLOGFILE)((PBYTE)gpClPackedEventInfo +
                    gpClPackedEventInfo->ulOffsets[0]);
                //set the events for log file structure
                pEventsForLogFile->ulNumRecords = 1;
                lstrcpyW(pEventsForLogFile->szLogicalLogFile,
                    Module->LogFile->LogModuleName->Buffer);
                pEventsForLogFile->ulSize = sizeof(EVENTSFORLOGFILE) +
                        RecordLength;
                RtlCopyMemory(pEventsForLogFile->pEventLogRecords, (PVOID)EventBuffer,
                    RecordLength);

                dwError = (*gpfnPropagateEvents)(ghCluster,
                    gpClPackedEventInfo->ulSize, (UCHAR *)gpClPackedEventInfo);

                //advance the session start record number
                //so that the same record doesnt get propagated twice, if the service restarts
                //dont advance the session number if the call doesnt succeed
                //BUGBUG: if the server is multi threaded, then we might lose events
                //in propagation if a latter one gets propagated before one that gets
                //written to the log earlier
                if (dwError == ERROR_SUCCESS)
                    Module->LogFile->SessionStartRecordNumber = Module->LogFile->CurrentRecordNumber;



            }
            RtlLeaveCriticalSection ((PRTL_CRITICAL_SECTION)&gClPropCritSec);

            //
            // Free up the buffer
            //
            ElfpFreeBuffer(EventBuffer);

            Status = Request.Status;                // Set status of WRITE

        } else {
            Status = STATUS_NO_MEMORY;
        }

    } else {
        Status = STATUS_INVALID_HANDLE;
    }

    return (Status);
}


//
// ANSI APIs
//

NTSTATUS
ElfrClearELFA (
    IN  IELF_HANDLE     LogHandle,
    IN  PRPC_STRING     BackupFileName
    )

/*++

Routine Description:

  This is the RPC server entry point for the ElfrClearELFA API.

Arguments:

    LogHandle       - The context-handle for this module's call.

    BackupFileName  - Name of the file to back up the current log file.
                      NULL implies not to back up the file.


Return Value:

    Returns an NTSTATUS code.


--*/
{
    NTSTATUS        Status;
    UNICODE_STRING  BackupFileNameU;

    //
    // Check the handle before proceeding.
    //

    Status = VerifyElfHandle(LogHandle);

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // Insure the caller has clear access.
    //

    if (!(LogHandle->GrantedAccess & ELF_LOGFILE_CLEAR)) {
        return(STATUS_ACCESS_DENIED);
    }

    //
    // Verify additional arguments.
    //

    if (BackupFileName != NULL) {
        Status = VerifyAnsiString((PANSI_STRING)BackupFileName);

        if (!NT_SUCCESS(Status)) {
            return(Status);
        }
    }

    //
    // Convert the BackupFileName to a UNICODE STRING and call the
    // UNICODE API to do the work.
    //
    Status = RtlAnsiStringToUnicodeString (
                    (PUNICODE_STRING)&BackupFileNameU,
                    (PANSI_STRING)BackupFileName,
                    TRUE
                    );

    if (NT_SUCCESS(Status)) {

        Status = ElfrClearELFW (
                LogHandle,
                (PRPC_UNICODE_STRING)&BackupFileNameU
                );

        RtlFreeUnicodeString (&BackupFileNameU);
    }

    return (Status);

}



NTSTATUS
ElfrBackupELFA (
    IN  IELF_HANDLE     LogHandle,
    IN  PRPC_STRING     BackupFileName
    )

/*++

Routine Description:

  This is the RPC server entry point for the ElfrBackupELFA API.

Arguments:

    LogHandle       - The context-handle for this module's call.

    BackupFileName  - Name of the file to back up the current log file.


Return Value:

    Returns an NTSTATUS code.


--*/
{
    NTSTATUS        Status;
    UNICODE_STRING  BackupFileNameU;

    //
    // Check the handle before proceeding.
    //

    Status = VerifyElfHandle(LogHandle);

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // Insure the caller has backup access.
    //

    if (!(LogHandle->GrantedAccess & ELF_LOGFILE_BACKUP)) {
        return(STATUS_ACCESS_DENIED);
    }

    //
    // Verify additional arguments.
    //

    Status = VerifyAnsiString((PANSI_STRING)BackupFileName);

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // Convert the BackupFileName to a UNICODE STRING and call the
    // UNICODE API to do the work.
    //
    Status = RtlAnsiStringToUnicodeString (
                    (PUNICODE_STRING)&BackupFileNameU,
                    (PANSI_STRING)BackupFileName,
                    TRUE
                    );

    if (NT_SUCCESS(Status)) {

        Status = ElfrBackupELFW (
                LogHandle,
                (PRPC_UNICODE_STRING)&BackupFileNameU
                );

        RtlFreeUnicodeString (&BackupFileNameU);
    }

    return (Status);

}


NTSTATUS
ElfrRegisterEventSourceA (
    IN  EVENTLOG_HANDLE_A   UNCServerName,
    IN  PRPC_STRING         ModuleName,
    IN  PRPC_STRING         RegModuleName,
    IN  ULONG               MajorVersion,
    IN  ULONG               MinorVersion,
    OUT PIELF_HANDLE        LogHandle
    )

/*++

Routine Description:

  This is the RPC server entry point for the ElfrRegisterEventSourceA API.
  This routine allocates a structure for the context handle, finds
  the matching module name and fills in the data. It returns the
  pointer to the handle structure.


Arguments:

    UNCServerName   - Not used.

    ModuleName      - Name of the module that is making this call.

    RegModuleName   - Not used.

    MajorVersion/MinorVersion - The version of the client.


    LogHandle       - Pointer to the place where the pointer to the
                      context handle structure will be placed.

Return Value:

    Returns an NTSTATUS code and, if no error, a "handle".

Note:

    For now, just call ElfrOpenELA.


--*/
{

    NTSTATUS Status;
    PLOGMODULE Module;
    UNICODE_STRING ModuleNameU;

    //
    // Check arguments.
    //
    // LogHandle check in ElfpOpenELA.
    //

    Status = VerifyAnsiString((PANSI_STRING)ModuleName);

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    Status = RtlAnsiStringToUnicodeString (
                    (PUNICODE_STRING) &ModuleNameU,
                    (PANSI_STRING) ModuleName,
                    TRUE
                    );

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    Module = GetModuleStruc ((PUNICODE_STRING) & ModuleNameU);

    RtlFreeUnicodeString (& ModuleNameU);

    return(ElfpOpenELA(UNCServerName, ModuleName, RegModuleName,
        MajorVersion, MinorVersion, LogHandle, ELF_LOGFILE_WRITE));

}

NTSTATUS
ElfrOpenELA (
    IN  EVENTLOG_HANDLE_A   UNCServerName,
    IN  PRPC_STRING         ModuleName,
    IN  PRPC_STRING         RegModuleName,
    IN  ULONG               MajorVersion,
    IN  ULONG               MinorVersion,
    OUT PIELF_HANDLE        LogHandle
    )

/*++

Routine Description:

  This is the RPC server entry point for the ElfrOpenEL API.
  This routine allocates a structure for the context handle, finds
  the matching module name and fills in the data. It returns the
  pointer to the handle structure.


Arguments:

    UNCServerName   - Not used.

    ModuleName      - Name of the module that is making this call.

    RegModuleName   - Name of module to use to determine the log file.

    MajorVersion/MinorVersion - The version of the client.


    LogHandle       - Pointer to the place where the pointer to the
                      context handle structure will be placed.

Return Value:

    Returns an NTSTATUS code and, if no error, a "handle".


--*/
{
    //
    // All arguments checked in ElfpOpenELA.
    //

    return (ElfpOpenELA (
                UNCServerName,
                ModuleName,
                RegModuleName,
                MajorVersion,
                MinorVersion,
                LogHandle,
                ELF_LOGFILE_READ));
}

NTSTATUS
ElfpOpenELA (
    IN  EVENTLOG_HANDLE_A   UNCServerName,
    IN  PRPC_STRING         ModuleName,
    IN  PRPC_STRING         RegModuleName,
    IN  ULONG               MajorVersion,
    IN  ULONG               MinorVersion,
    OUT PIELF_HANDLE        LogHandle,
    IN  ULONG               DesiredAccess
    )

/*++

Routine Description:

  Looks alot loke ElfrOpenELA, only this also takes a DesiredAccess parameter.


Arguments:

    UNCServerName   - Not used.

    ModuleName      - Name of the module that is making this call.

    RegModuleName   - Name of module to use to determine the log file.

    MajorVersion/MinorVersion - The version of the client.


    LogHandle       - Pointer to the place where the pointer to the
                      context handle structure will be placed.

Return Value:

    Returns an NTSTATUS code and, if no error, a "handle".

--*/
{
    NTSTATUS       Status;
    UNICODE_STRING ModuleNameU;

    //
    // Check arguments.
    //

    Status = VerifyAnsiString((PANSI_STRING)ModuleName);

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    if (LogHandle == NULL) {
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // Convert the ModuleName and RegModulename to UNICODE STRINGs and call
    // the UNICODE API to do the work.
    //

    Status = RtlAnsiStringToUnicodeString (
                    (PUNICODE_STRING)&ModuleNameU,
                    (PANSI_STRING)ModuleName,
                    TRUE
                    );

    if (NT_SUCCESS(Status)) {

        //
        // We *KNOW* that the UNCServerName is not used
        // by ElfpOpenELW so we save ourselves some work
        // and just pass in a NULL.
        //
        Status = ElfpOpenELW (
                     (EVENTLOG_HANDLE_W) NULL,
                     (PRPC_UNICODE_STRING)&ModuleNameU,
                     NULL,
                     MajorVersion,
                     MinorVersion,
                     LogHandle,
                     DesiredAccess
                     );

        RtlFreeUnicodeString(&ModuleNameU);
    }

    return (Status);
    UNREFERENCED_PARAMETER(UNCServerName);
}





NTSTATUS
ElfrOpenBELA (
    IN  EVENTLOG_HANDLE_A   UNCServerName,
    IN  PRPC_STRING         FileName,
    IN  ULONG               MajorVersion,
    IN  ULONG               MinorVersion,
    OUT PIELF_HANDLE        LogHandle
    )

/*++

Routine Description:

  This is the RPC server entry point for the ElfrOpenBEL API.
  This routine allocates a structure for the context handle, finds
  the matching module name and fills in the data. It returns the
  pointer to the handle structure.


Arguments:

    UNCServerName   - Not used.

    FileName        - Filename of the logfile

    MajorVersion/MinorVersion - The version of the client.

    LogHandle       - Pointer to the place where the pointer to the
                      context handle structure will be placed.

Return Value:

    Returns an NTSTATUS code and, if no error, a "handle".


--*/
{
    NTSTATUS        Status;
    UNICODE_STRING  FileNameU;

    //
    // Check arguments.
    //

    Status = VerifyAnsiString((PANSI_STRING)FileName);

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // A filename must be specified.
    //

    if (FileName->Length == 0) {
        return(STATUS_INVALID_PARAMETER);
    }

    if (LogHandle == NULL) {
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // Convert the FileName to a UNICODE STRINGs and call
    // the UNICODE API to do the work.
    //

    Status = RtlAnsiStringToUnicodeString (
                    (PUNICODE_STRING) &FileNameU,
                    (PANSI_STRING) FileName,
                    TRUE
                    );

    if (NT_SUCCESS(Status)) {

        //
        // We *KNOW* that the UNCServerName is not used
        // by ElfrOpenELW so we save ourselves some work
        // and just pass in a NULL.
        //

        Status = ElfrOpenBELW (
                    (EVENTLOG_HANDLE_W) NULL,
                    (PRPC_UNICODE_STRING)&FileNameU,
                    MajorVersion,
                    MinorVersion,
                    LogHandle
                    );

        RtlFreeUnicodeString(&FileNameU);
    }

    return (Status);
    UNREFERENCED_PARAMETER(UNCServerName);

}



NTSTATUS
ElfrReadELA (
    IN      IELF_HANDLE LogHandle,
    IN      ULONG       ReadFlags,
    IN      ULONG       RecordNumber,
    IN      ULONG       NumberOfBytesToRead,
    IN      PBYTE       Buffer,
    OUT     PULONG      NumberOfBytesRead,
    OUT     PULONG      MinNumberOfBytesNeeded
    )

/*++

Routine Description:

  This is the RPC server entry point for the ElfrReadEL API.

Arguments:



Return Value:

    Returns an NTSTATUS code, NumberOfBytesRead if the read was successful
    and MinNumberOfBytesNeeded if the buffer was not big enough.


--*/
{
    //
    // All arguments checked in w_ElfrReadEL.
    //

    //
    // Call the worker with the UNICODE flag
    //

    return(w_ElfrReadEL (
                        ELF_IREAD_ANSI,
                        LogHandle,
                        ReadFlags,
                        RecordNumber,
                        NumberOfBytesToRead,
                        Buffer,
                        NumberOfBytesRead,
                        MinNumberOfBytesNeeded
                        ));
}




NTSTATUS
ConvertStringArrayToUnicode (
    PUNICODE_STRING *pUStringArray,
    PANSI_STRING    *Strings,
    USHORT          NumStrings
    )
/*++

Routine Description:

  This routine takes an array of STRINGs and generates an array of
  PUNICODE_STRINGs. The destination array has already been allocated
  by the caller, but the structures for the UNICODE_STRINGs will need
  to be allocated by this routine.

Arguments:

    pUStringArray   - Array of PUNICODE_STRINGs.
    Strings         - Array of PANSI_STRINGs.
    NumStrings      - Number of elements in the arrays.

Return Value:

    Returns an NTSTATUS code.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    USHORT   i;

    //
    // For each string passed in, allocate a UNICODE_STRING buffer
    // and set it to the UNICODE equivalent of the string passed in.
    //
    for (i=0; i<NumStrings; i++) {

        if (Strings[i]) {

            Status = VerifyAnsiString(Strings[i]);

            if (!NT_SUCCESS(Status)) {
                break;
            }

            pUStringArray[i] = ElfpAllocateBuffer (sizeof (UNICODE_STRING));

            if (pUStringArray[i]) {

                Status = RtlAnsiStringToUnicodeString (
                                        pUStringArray[i],
                                        (PANSI_STRING)Strings[i],
                                        TRUE
                                        );
            } else {
                Status = STATUS_NO_MEMORY;
            }
        } else {
            pUStringArray[i] = NULL;
        }
        if (!NT_SUCCESS(Status))
            break;                  // Jump out of loop and return status
    }

    //
    // Free any allocations on failure.
    //

    if (!NT_SUCCESS(Status)) {
        FreePUStringArray(pUStringArray, (USHORT)(i + 1));
    }

    return (Status);
}



VOID
FreePUStringArray (
    PUNICODE_STRING  *pUStringArray,
    USHORT          NumStrings
    )
/*++

Routine Description:

  This routine takes the PUNICODE_STRING array that was filled in by
  ConvertStringArrayToUnicode and frees the buffer portion of
  each unicode string and then the UNICODE structure itseld. It handles
  the case where the array may not have been filled completely due
  to insufficient memory.

Arguments:

    pUStringArray   - Array of PUNICODE_STRINGs.
    NumStrings      - Number of elements in the array.

Return Value:

    NONE.

--*/
{
    USHORT      i;

    for (i=0; i<NumStrings; i++) {

        if (pUStringArray[i]) {

            if (pUStringArray[i]->Buffer) {
                RtlFreeUnicodeString (pUStringArray[i]); // Free the string buffer

                ElfpFreeBuffer (pUStringArray[i]);  // Free the structure itself
            }
        }
    }
}



NTSTATUS
ElfrReportEventA (
    IN      IELF_HANDLE         LogHandle,
    IN      ULONG               Time,
    IN      USHORT              EventType,
    IN      USHORT              EventCategory OPTIONAL,
    IN      ULONG               EventID,
    IN      USHORT              NumStrings,
    IN      ULONG               DataSize,
    IN      PRPC_STRING         ComputerName,
    IN      PRPC_SID            UserSid,
    IN      PRPC_STRING         Strings[],
    IN      PBYTE               Data,
    IN      USHORT              Flags,
    IN OUT  PULONG              RecordNumber OPTIONAL,
    IN OUT  PULONG              TimeWritten OPTIONAL
    )

/*++

Routine Description:

  This is the RPC server entry point for the ElfrReportEventA API.

Arguments:


Return Value:

    Returns an NTSTATUS code.


--*/
{
    NTSTATUS            Status;
    UNICODE_STRING      ComputerNameU;
    PUNICODE_STRING     *pUStringArray = NULL;

    //
    // Check the handle before proceeding.
    //

    Status = VerifyElfHandle(LogHandle);

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    //
    // Insure the caller has write access.
    //

    if (!(LogHandle->GrantedAccess & ELF_LOGFILE_WRITE)) {
        return(STATUS_ACCESS_DENIED);
    }

    //
    // Verify additional arguments.
    //

    Status = VerifyAnsiString((PANSI_STRING)ComputerName);

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    if (Strings == NULL && NumStrings != 0) {
        return(STATUS_INVALID_PARAMETER);
    }

    //
    // Convert the ComputerName to a UNICODE STRING and call the
    // UNICODE API.
    //

    Status = RtlAnsiStringToUnicodeString (
                    (PUNICODE_STRING)&ComputerNameU,
                    (PANSI_STRING)ComputerName,
                    TRUE
                    );

    if (NT_SUCCESS(Status)) {

        if (NumStrings) {

            pUStringArray = ElfpAllocateBuffer (
                                NumStrings * sizeof (PUNICODE_STRING)
                                );

            if (pUStringArray) {

                //
                // Convert the array of STRINGs to an array of UNICODE-STRINGs
                // before calling the unicode API.
                // We can just use the array of Strings passed in since we
                // don't need to use it anywhere else.
                //
                Status = ConvertStringArrayToUnicode (
                                    pUStringArray,
                                    (PANSI_STRING *)Strings,
                                    NumStrings
                                    );
            }
            else {
                Status = STATUS_NO_MEMORY;
            }
        }

        if (NT_SUCCESS(Status)) {

            Status = ElfrReportEventW (
                        LogHandle,
                        Time,
                        EventType,
                        EventCategory,
                        EventID,
                        NumStrings,
                        DataSize,
                        (PRPC_UNICODE_STRING)&ComputerNameU,
                        UserSid,
                        (PRPC_UNICODE_STRING*)pUStringArray,
                        Data,
                        Flags,       // Flags       -  paired event
                        RecordNumber,// RecordNumber | support. not in
                        TimeWritten  // TimeWritten _  product 1
                        );

            FreePUStringArray (pUStringArray, NumStrings);

        }

        RtlFreeUnicodeString (&ComputerNameU);
    }

    if (pUStringArray)
        ElfpFreeBuffer (pUStringArray);

    return (Status);
}


NTSTATUS
VerifyElfHandle(
    IN IELF_HANDLE LogHandle
    )

/*++

Routine Description:

    Verify the handle via its DWORD signature.

Arguments:

    LogHandle   - Handle to verify.

Return Value:

    STATUS_SUCCESS          - Presumably valid handle.
    STATUS_INVALID_HANDLE   - Invalid handle.

--*/
{
    NTSTATUS Status;

    if (LogHandle != NULL) {
        try {
            if (LogHandle->Signature == ELF_CONTEXTHANDLE_SIGN) {
                Status = STATUS_SUCCESS;
            } else {
                Status = STATUS_INVALID_HANDLE;
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
            Status = STATUS_INVALID_HANDLE;
        }
    } else {
        Status = STATUS_INVALID_HANDLE;
    }

    return(Status);
}


ULONG
Safewcslen(
    UNALIGNED WCHAR *p,
    LONG            MaxLength
    )
/*++

    Safewcslen - Strlen that won't exceed MaxLength

Routine Description:

    This routine is called to determine the size of a UNICODE_STRING

Arguments:
    p         - The string to count.
    MaxLength - The maximum length to look at.


Return Value:

    Number of bytes in the string (or MaxLength)

--*/
{
    ULONG Count = 0;

    if (p) {
        while (MaxLength > 0 && *p++ != UNICODE_NULL) {
            MaxLength -= sizeof(WCHAR);
            Count     += sizeof(WCHAR);
        }
    }

    return Count;
}


ULONG
Safestrlen(
    UNALIGNED char *p,
    LONG           MaxLength
    )
/*++

    Safestrlen - Strlen that won't exceed MaxLength

Routine Description:

    This routine is called to determine the length of an ANSI_STRING

Arguments:
    p         - The string to count.
    MaxLength - The maximum length to look at.


Return Value:

    Number of chars in the string (or MaxLength)

--*/
{
    ULONG Count = 0;

    if (p) {
        while (MaxLength > 0 && *p++ != '\0') {
            MaxLength--;
            Count++;
        }
    }

    return Count;
}



NTSTATUS
VerifyUnicodeString(
    IN PUNICODE_STRING pUString
    )

/*++

Routine Description:

    Verify the unicode string. The string is invalid if:
        The UNICODE_STRING structure ptr is NULL.
        The MaximumLength field is invalid (too small).
        The Length field is incorrect.

Arguments:

    pUString    - String to verify.

Return Value:

    STATUS_SUCCESS              - Valid string.
    STATUS_INVALID_PARAMETER    - I wonder.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Check validity of structure fields and actual string
    // length vs. length value supplied
    //
    if (!pUString ||
        pUString->MaximumLength < pUString->Length ||
        pUString->Length != Safewcslen(pUString->Buffer,
                                       pUString->MaximumLength)) {

        Status = STATUS_INVALID_PARAMETER;
    }

    return Status;
}



NTSTATUS
VerifyAnsiString(
    IN PANSI_STRING pAString
    )

/*++

Routine Description:

    Verify the ansi string. The string is invalid if:
        The ANSI_STRING structure ptr is NULL.
        The MaximumLength field is invalid (too small).
        The Length field is incorrect.

Arguments:

    pAString    - String to verify.

Return Value:

    STATUS_SUCCESS              - Valid string.
    STATUS_INVALID_PARAMETER    - I wonder.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (!pAString ||
        pAString->MaximumLength < pAString->Length ||
        pAString->Length != Safestrlen(pAString->Buffer,
                                       pAString->MaximumLength)) {

        Status = STATUS_INVALID_PARAMETER;
    }

    return Status;
}



//SS:changes made to enable cluster wide event logging
/****
@func       NTSTATUS | ElfrRegisterClusterSvc|  This is the server entrypoint
            for ElfRegisterClusterSvc.  The cluster service registers
            itself with the event log service to enable propagation of events
            across the cluster.  The binding handle to the cluster service for
            propagation of events is obtained.

@parm       IN  EVENTLOG_HANDLE_W | UNCServerName | This parameter is ignored. It
            is retained for correspondence with other elf apis.

@parm       OUT PULONG | pulSize | A pointer to a long where the size of the
            packed event information structure is returned.

@parm       OUT PBYTE | *ppPackedEventInfo| A pointer to the packed event information
            structure for propagation is returned via this parameter.

@comm       The cluster service propagates events contained in this structure
            and deletes the memory after it has done so.  Once the cluster service has
            registered with the eventlog service, the eventlog service passes up
            logged events to the cluster service for propagation.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref       <f ElfRegisterClusterSvc> <f ElfrDeregisterClusterSvc>
****/
NTSTATUS
ElfrRegisterClusterSvc(
    IN EVENTLOG_HANDLE_W UNCServerName,
    OUT PULONG pulSize,
    OUT PBYTE * ppPackedEventInfo)
{
    ULONG               ulTotalSize = 0;
    ULONG               ulTotalEventsSize=0;
    ULONG               ulNumLogFiles=0;
    PPROPLOGFILEINFO    pPropLogFileInfo=NULL;
    NTSTATUS            Status=STATUS_SUCCESS;
    PPACKEDEVENTINFO    pPackedEventInfo=NULL;
    UINT                i;
    PEVENTSFORLOGFILE   pEventsForLogFile;
    WCHAR               *pBinding = NULL;
    HANDLE              hClusSvcNode = NULL;
    UNICODE_STRING      RootRegistryNode;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    BOOL                bAcquired = FALSE;
    BOOL                bInitedCritSec = FALSE;

    ElfDbgPrint(("[ELF] ElfrRegisterClusterSvc: Entry.\r\n"));

    //initialize
    *pulSize = 0;
    *ppPackedEventInfo = NULL;

    //check to see if the cluster service is installed.
    RtlInitUnicodeString(&RootRegistryNode, REG_CLUSSVC_NODE_PATH);
    InitializeObjectAttributes(&ObjectAttributes,
                               &RootRegistryNode,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                               );
    NtOpenKey(&hClusSvcNode, KEY_READ | KEY_NOTIFY, &ObjectAttributes);

    if (!hClusSvcNode)
    {
        Status = STATUS_ACCESS_DENIED;
        goto FnExit;
    }

    //if the cluster service dies and restarts again in the same session
    //then it will try to register again
    //we dont reinitialize these globals again to prevent leaks
    RtlEnterCriticalSection ((PRTL_CRITICAL_SECTION)&gClPropCritSec);
    if (!gbClustering)
    {
        ElfDbgPrint(("[ELF] ElfrDeregisterClusterSvc: gbClustering is FALSE.\r\n"));

        //load the cluster support dll
        ghClusDll = LoadLibraryW(L"CLUSSPRT.DLL");

        if (!ghClusDll)
        {
            RtlLeaveCriticalSection ((PRTL_CRITICAL_SECTION)&gClPropCritSec);
            Status = GetLastError();
            goto FnExit;
        }

    }
    RtlLeaveCriticalSection ((PRTL_CRITICAL_SECTION)&gClPropCritSec);

    gpfnPropagateEvents = (PROPAGATEEVENTSPROC)
        GetProcAddress((HMODULE)ghClusDll,"PropagateEvents");
    gpfnBindToCluster = (BINDTOCLUSTERPROC)
        GetProcAddress((HMODULE)ghClusDll,"BindToClusterSvc");
    gpfnUnbindFromCluster = (UNBINDFROMCLUSTERPROC)
        GetProcAddress((HMODULE)ghClusDll,"UnbindFromClusterSvc");

    //get the function entry points
    if (!gpfnPropagateEvents || !gpfnBindToCluster || !gpfnUnbindFromCluster )
    {
        Status = STATUS_PROCEDURE_NOT_FOUND;
        goto FnExit;
    }
    //bind to th cluster service
    ghCluster = (*gpfnBindToCluster)(NULL);

    if (!ghCluster)
    {
        Status = GetLastError();
        goto FnExit;
    }

    //since we are going to read the logs, make sure the service is running
    while (  (GetElState() == RUNNING) && (!bAcquired))
    {

        bAcquired = RtlAcquireResourceShared(
                            &GlobalElfResource,
                            FALSE                       // Don't wait
                            );
        if (!bAcquired) {
            ElfDbgPrint(("[ElfrRegisterClusterSvc] Sleep waiting for global resource\n" ));
            Sleep (ELF_GLOBAL_RESOURCE_WAIT);
        }

    }

    // If the resource was not available and the status of the service
    // changed to one of the "non-working" states, then we just return
    // unsuccesful.  Rpc should not allow this to happen.
    //

    if (!bAcquired) {

        ElfDbgPrint(("[ELF] Global resource not acquired.\n" ));
        Status = STATUS_UNSUCCESSFUL;
        goto FnExit;
    }

    //determine the size and acquire read locks on all files
    //FindSizeofEventsSinceStart acquires the per log locks if
    //there are events in at that log to propagate.
    Status = FindSizeofEventsSinceStart (&ulTotalEventsSize, &ulNumLogFiles,
                &pPropLogFileInfo);

    if (!NT_SUCCESS (Status))
        goto FnExit;

    //if there are any events to propagate
    if (ulNumLogFiles && ulTotalEventsSize && pPropLogFileInfo)
    {

        ulTotalSize = sizeof(PACKEDEVENTINFO)         //for header
                    + (sizeof(ULONG) * ulNumLogFiles) //for the offsets
                    + (sizeof(EVENTSFORLOGFILE) * ulNumLogFiles) //info for every log files
                    + ulTotalEventsSize;



        //allocate memory
        *ppPackedEventInfo = (PBYTE)ElfpAllocateBuffer (ulTotalSize);
        if (!(*ppPackedEventInfo))
        {
            //free the read locks acquired in FindSizeofEventsSinceStart
            for (i=0;i<ulNumLogFiles;i++)
                RtlReleaseResource (&(pPropLogFileInfo[i].pLogFile->Resource));
            Status = STATUS_NO_MEMORY;
            goto FnExit;
        }
        pPackedEventInfo = (PPACKEDEVENTINFO)(*ppPackedEventInfo);
        ElfDbgPrint(("[ELF] ElfrRegisterClusterSvc: Allocated %d bytes, pPackedEventInfo=0x%08lx\r\n",
            ulTotalSize,pPackedEventInfo));

        pPackedEventInfo->ulNumEventsForLogFile = ulNumLogFiles;


        for (i=0;i<ulNumLogFiles;i++)
        {

            //set the offsets to the eventlogforfile structures
            pPackedEventInfo->ulOffsets[i] = ((i== 0) ?
                (sizeof(PACKEDEVENTINFO) +
                ulNumLogFiles * sizeof(ULONG)) :
                (pPackedEventInfo->ulOffsets[i-1] +
                (pPropLogFileInfo[i-1].ulTotalEventSize + sizeof(EVENTSFORLOGFILE))));

            ElfDbgPrint(("[ELF] ElfrRegisterClusterSvc: pPackedEventInfo->uloffsets[i]=%d\r\n",
                pPackedEventInfo->ulOffsets[i]));

            pEventsForLogFile = (PEVENTSFORLOGFILE)(
                (PBYTE)pPackedEventInfo + pPackedEventInfo->ulOffsets[i]);

            //set the size of the ith EVENTSFORLOGFILE structure
            pEventsForLogFile->ulSize = sizeof(EVENTSFORLOGFILE) + pPropLogFileInfo[i].ulTotalEventSize;
            //copy the file name or should we get the module name
            lstrcpyW(pEventsForLogFile->szLogicalLogFile,
                pPropLogFileInfo[i].pLogFile->LogModuleName->Buffer);
            //set the number of events
            pEventsForLogFile->ulNumRecords = pPropLogFileInfo[i].ulNumRecords;

            ElfDbgPrint(("[ELF] ElfrRegisterClusterSvc: pEventsForLogFile struct.ulSize=%d,LogicalFile=%ws,ulNumRecords=%d\r\n",
                pEventsForLogFile->ulSize, pEventsForLogFile->szLogicalLogFile,
                pEventsForLogFile->ulNumRecords));

            //get the events
            Status = GetEventsToProp((PEVENTLOGRECORD)((PBYTE)pEventsForLogFile + sizeof(EVENTSFORLOGFILE)),
                pPropLogFileInfo+i);

            //ss::if that fails??
            //set the ulNumRecords to 0, so that on a write this data is discarded.
            if (!NT_SUCCESS (Status))
            {
                pEventsForLogFile->ulNumRecords=0;
                //reset the error, we will go to the next file
                Status = STATUS_SUCCESS;
            }
            //advance the startpointer for all the files
            //if the cluster service dies and is restarted, then these
            //events wont be propagated again
            pPropLogFileInfo[i].pLogFile->SessionStartRecordNumber =
                pPropLogFileInfo[i].pLogFile->CurrentRecordNumber;

            RtlReleaseResource (&(pPropLogFileInfo[i].pLogFile->Resource));

        }
        //set the total size
        pPackedEventInfo->ulSize = pPackedEventInfo->ulOffsets[ulNumLogFiles - 1] +
                pPropLogFileInfo[ulNumLogFiles - 1].ulTotalEventSize +
                sizeof(EVENTSFORLOGFILE);
        *pulSize = pPackedEventInfo->ulSize;

    }

    RtlEnterCriticalSection ((PRTL_CRITICAL_SECTION)&gClPropCritSec);
    //if the cluster service hadnt registered before or glClPackedEventInfo is NULL
    if (!gbClustering || !gpClPackedEventInfo)
    {
        //allocate memory for processing events for propagation
        ulTotalSize = sizeof(PACKEDEVENTINFO)         //for header
                    + (sizeof(ULONG) * 1) //for the offsets
                    + (sizeof(EVENTSFORLOGFILE) * 1) //info for every log files
                    + MAXSIZE_OF_EVENTSTOPROP;

        //allocate memory
        gpClPackedEventInfo = (PPACKEDEVENTINFO)ElfpAllocateBuffer (ulTotalSize);
        if (!(gpClPackedEventInfo))
        {
            Status = STATUS_NO_MEMORY;
            RtlLeaveCriticalSection ((PRTL_CRITICAL_SECTION)&gClPropCritSec);
            goto FnExit;
        }

        //setup the propagatepacketeventinfo
        gpClPackedEventInfo->ulNumEventsForLogFile = 1;
        gpClPackedEventInfo->ulOffsets[0]= sizeof(PACKEDEVENTINFO) +
                sizeof(ULONG);

        

    }
    //set the flag to true so that propagation is now on.
    gbClustering = TRUE;
    RtlLeaveCriticalSection ((PRTL_CRITICAL_SECTION)&gClPropCritSec);

FnExit:

    //if everything went ok,
    if (!NT_SUCCESS (Status))
    {
        if (gpClPackedEventInfo)
        {
            ElfpFreeBuffer(gpClPackedEventInfo);
            gpClPackedEventInfo = NULL;
        }
        if (ghCluster && gpfnUnbindFromCluster)
            (*gpfnUnbindFromCluster)(ghCluster);
        if (ghClusDll)
            FreeLibrary(ghClusDll);


    }
    //free the pPropLogFileInfo stucture
    if (pPropLogFileInfo) ElfpFreeBuffer(pPropLogFileInfo);

    if (bAcquired) ReleaseGlobalResource ();

    ElfDbgPrint(("[ELF] ElfrRegisterClusterSvc: returning Status=%d\r\n",
        Status));
    ElfDbgPrint(("[ELF] ElfrRegisterClusterSvc: *pulSize=%d pPackedEventInfo=0x%08lx\r\n",
        *pulSize, pPackedEventInfo));
    return(Status);
}



/****
@func       NTSTATUS | ElfrDeregisterClusterSvc| This is the server entry point
            for ElfDeregisterClusterSvc().  Before shutdown the cluster
            service deregisters itself for propagation of events from the
            eventlog service.

@comm       Note that events logged after the cluster service goes down
            are not propagated.  Binding handle is freed.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref       <f ElfrRegisterClusterSvc>
****/
NTSTATUS
ElfrDeregisterClusterSvc(
    IN EVENTLOG_HANDLE_W UNCServerName
)
{
    ElfDbgPrint(("[ELF] ElfrDeregisterClusterSvc: Entry.\r\n"));

    RtlEnterCriticalSection ((PRTL_CRITICAL_SECTION)&gClPropCritSec);
    if (gbClustering)
    {
        gbClustering = FALSE;

        if (gpClPackedEventInfo) ElfpFreeBuffer(gpClPackedEventInfo);
        gpClPackedEventInfo = NULL;

        //unload the cluster support dll
        if (ghCluster && gpfnUnbindFromCluster)
        {
            (*gpfnUnbindFromCluster)(ghCluster);
            ghCluster = NULL;
        }
        if (ghClusDll)
        {
            FreeLibrary(ghClusDll);
            ghClusDll = NULL;
        }
    }
    RtlLeaveCriticalSection ((PRTL_CRITICAL_SECTION)&gClPropCritSec);
    
    ElfDbgPrint(("[ELF] ElfrDeregisterClusterSvc: Exit.\r\n"));

    return(STATUS_SUCCESS);
};



/****
@func   NTSTATUS | ElfrWriteClusterEvents| The cluster service calls this
        api to log events reported at other nodes of the cluster in the event log files.

@parm   IN EVENTLOG_HANDLE_W | UNCServerName | Not used.

@parm   IN ULONG | ulSize | The size of the    packed event information structure.

@parm   IN PBYTE | pPackedEventInfo| A pointer to the packed event information
        structure for propagation.

@comm   The pPackedEventInfo is delinearized into eventlogbuffers for different event
        log files and the events are recorded in the appropriate eventlog file.  Multiple
        events per log file are supported.

@rdesc  Returns a result code. ERROR_SUCCESS on success.

@xref
****/
NTSTATUS
ElfrWriteClusterEvents(
    IN EVENTLOG_HANDLE_W UNCServerName,
    IN ULONG ulSize,
    IN BYTE    *pBuffer

)
{
    UINT                i,j;
    PEVENTSFORLOGFILE   pEventsForLogFile;
    UNICODE_STRING      ModuleName;
    PLOGMODULE          pLogModule;
    PEVENTLOGRECORD     pEventLogRecord;
    ELF_REQUEST_RECORD  Request;
    PPACKEDEVENTINFO    pPackedEventInfo;
    NTSTATUS            Status = STATUS_SUCCESS;
    WRITE_PKT           WritePkt;
    HANDLE              hClientToken = NULL;
    BOOL                bCheckMember;

    //impersonate to figure if the caller is in the admin group
    //the cluster service must run in an account that has local admin
    //privileges
    Status = RpcImpersonateClient(NULL);

    if (Status != RPC_S_OK) {
        ElfDbgPrint(("ElfrWriteClusterEvents: RPC IMPERSONATION FAILED %d\n", 
            Status));
        goto FnExit;
    }


    if (!OpenThreadToken(GetCurrentThread(), TOKEN_READ, TRUE, &hClientToken)) {
        Status = GetLastError();
        RpcRevertToSelf();            
        goto FnExit;
    }

    if (!CheckTokenMembership(hClientToken,
                              ElfGlobalData->AliasAdminsSid,
                              &bCheckMember))
    {
        Status = GetLastError();
        ElfDbgPrint(("ElfrWriteClusterEvents : AllocInitializeSid failed, status %d\r\n",
            Status));
        RpcRevertToSelf();            
        goto FnExit;
    }

    if (!bCheckMember)
    {
        Status = STATUS_ACCESS_DENIED;
        RpcRevertToSelf();
        goto FnExit;
    }

    //stop impersonating
    Status = RpcRevertToSelf();
    if (Status != RPC_S_OK) {
        ElfDbgPrint(("ElfrWriteClusterEvents : RPC REVERT TO SELF FAILED %d\n", 
            Status));
        goto FnExit;            
    }

    //we want to put this in a try/except block in any case
    //because we dont want services.exe to fault because of
    //bad data that we didnt verify

    try {

        //pbuffer is validated to be non null by the client
        pPackedEventInfo = (PPACKEDEVENTINFO)pBuffer;

        //validate input parameters and check that clustering is on
        if (!pPackedEventInfo || !ulSize || 
            (((PBYTE)pPackedEventInfo + sizeof(PACKEDEVENTINFO)) >
            (PBYTE)(pBuffer + ulSize)) ||
            (pPackedEventInfo->ulSize != ulSize) || (!gbClustering))
    
            return(STATUS_INVALID_PARAMETER);


        ElfDbgPrint(("[ELF] ElfrWriteClusterEvents: ulSize=%d ulNumEventsForLogFile=%d\r\n",
            ulSize, pPackedEventInfo->ulNumEventsForLogFile));


        //setup the request packet
        Request.Pkt.WritePkt = &WritePkt;   // Set up write packet in request packet
        Request.Flags = 0;

        //for each log
        for (i=0; i< pPackedEventInfo->ulNumEventsForLogFile; i++)
        {
            pEventsForLogFile = (PEVENTSFORLOGFILE)((PBYTE)pPackedEventInfo +
                    pPackedEventInfo->ulOffsets[i]);

            //check the packet integrity - 
            //check whether an overflow occured or our pointer points beyond 
            //
            if (((PBYTE)pEventsForLogFile < pBuffer) ||
                (((PBYTE)pEventsForLogFile + sizeof(EVENTSFORLOGFILE)) > 
                    (PBYTE)(pBuffer + ulSize))
                )
            {
                Status = STATUS_INVALID_PARAMETER;
                goto FnExit;
            }


            ElfDbgPrint(("[ELF] ElfrWriteClusterEvents: szLogicalLogFile=%ws ulNumRecords=%d\r\n",
                pEventsForLogFile->szLogicalLogFile,pEventsForLogFile->ulNumRecords));

            //find the module
            //since we dont trust this string, force null termination
            pEventsForLogFile->szLogicalLogFile[MAXLOGICALLOGNAMESIZE-1] = L'\0';
            RtlInitUnicodeString(&ModuleName, pEventsForLogFile->szLogicalLogFile);

            pLogModule = GetModuleStruc ((PUNICODE_STRING) & ModuleName);
            //for all the find the log file related to the module.
            if (pLogModule)
            {
                ElfDbgPrint(("[ELF] ElfrWriteClusterEvents: processing records for pLogModule=0x%08lx\r\n",
                    pLogModule));

                Request.Module = pLogModule;
                Request.LogFile = Request.Module->LogFile;
                Request.Command = ELF_COMMAND_WRITE;
                pEventLogRecord = (PEVENTLOGRECORD)(pEventsForLogFile->pEventLogRecords);
                for (j=0;(j< pEventsForLogFile->ulNumRecords &&
                    pEventLogRecord->Reserved == ELF_LOG_FILE_SIGNATURE);
                    j++)
                {
                    // check for packet interity
                    // that a bigger enough buffer was provided for the records
                    // also check that no overflow occurs
                    if (((PBYTE)pEventLogRecord + pEventLogRecord->Length) >
                        (PBYTE)(pBuffer + ulSize))
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        goto FnExit;
                    }
                    //for record in there, set the request structure
                    Request.Pkt.WritePkt->Buffer = (PVOID)pEventLogRecord;
                    Request.Pkt.WritePkt->Datasize = pEventLogRecord->Length;

                    //SS: should we get exclusive access to the log
                    //so that the current record number is not incremented
                    //for an event that needs to be propagated
                    //before the session start record number is set here
                    ElfPerformRequest(&Request);
                    //advance the session start record number, so that we dont
                    //propagate an event propagated to us
                    pLogModule->LogFile->SessionStartRecordNumber = pLogModule->LogFile->CurrentRecordNumber;

                    // Extract status of operation from the request packet
                    Status = Request.Status;
                    if (!(NT_SUCCESS(Request.Status)))
                        ElfDbgPrint(("[ELF] ElfrWriteClusterEvents: failed to write record#=%d\r\n",
                            j));

                    pEventLogRecord = (PEVENTLOGRECORD)((PBYTE)pEventLogRecord +
                            pEventLogRecord->Length);


                }
            }
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = STATUS_INVALID_PARAMETER;
    }

FnExit:

    if (hClientToken)
        CloseHandle(hClientToken);
    return(Status);
}

//SS:end of changes made to enable cluster wide event logging

