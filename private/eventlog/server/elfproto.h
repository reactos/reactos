/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    elfproto.h

Abstract:

    This file contains the prototypes for the Eventlog service.

Author:

    Rajen Shah (rajens) 12-Aug-1991

Revision History:

--*/

#ifndef _ELFPROTO_
#define _ELFPROTO

//
// Other prototypes
//
PVOID
ElfpAllocateBuffer (
    ULONG size
    );

VOID
ElfpFreeBuffer (
    PVOID BufPtr
    );

VOID
ElfPerformRequest( 
    PELF_REQUEST_RECORD Request
    );


PLOGMODULE
GetModuleStruc (
    PUNICODE_STRING ModuleName
    );

PLOGMODULE
FindModuleStrucFromAtom (
    ATOM Atom 
    );

VOID
ElfControlResponse(
    DWORD
    );

VOID
IELF_HANDLE_rundown(
    IELF_HANDLE    ElfHandle
    );

VOID
LinkContextHandle(
    IELF_HANDLE     LogHandle
    );

VOID
UnlinkContextHandle (
    IELF_HANDLE     LogHandle
    );

VOID
LinkLogModule (
    PLOGMODULE   pLogModule,
    ANSI_STRING * pModuleNameA
    );

VOID
UnlinkLogModule (
    PLOGMODULE pLogModule
    );

VOID
LinkLogFile (
    PLOGFILE   pLogFile
    );

VOID
UnlinkLogFile (
    PLOGFILE pLogFile
    );

VOID
GetGlobalResource (
    DWORD Type
    );

VOID
ReleaseGlobalResource(
    VOID
    );

NTSTATUS
SetUpDataStruct (
    PUNICODE_STRING     LogFileName,
    ULONG               MaxFileSize,
    ULONG               Retention,
    ULONG               GuestAccessRestriction,
    PUNICODE_STRING     ModuleName,
    HANDLE              hLogFile,
    ELF_LOG_TYPE        LogType,
    LOGPOPUP            logpLogPopup
    );

NTSTATUS
SetUpModules (
    HANDLE              hLogFile,
    PLOGFILE            pLogFile,
    BOOLEAN             bAllowDupes
    );

BOOL
StartLPCThread (
    VOID
    );

VOID
StopLPCThread (
    VOID
    );

BOOL
ElfStartRegistryMonitor (
    VOID
    );

VOID
StopRegistryMonitor (
    VOID
    );

NTSTATUS
ReadRegistryInfo (
    HANDLE          hLogFiles,
    PUNICODE_STRING SubKeyName,
    PLOG_FILE_INFO  LogFileInfo
    );

NTSTATUS
ElfOpenLogFile (
    PLOGFILE pLogFile,
    ELF_LOG_TYPE LogType
    );

NTSTATUS
ElfpCloseLogFile (
    PLOGFILE    pLogFile,
    DWORD       Flags
    );

BOOL
ValidFilePos (
    PVOID Position,
    PVOID BeginningRecord,
    PVOID EndingRecord,
    PVOID PhysicalEOF,
    PVOID BaseAddress,
    BOOL  fCheckBeginEndRange
    );

VOID
ElfpCleanUp (
    ULONG EventFlags
    );

NTSTATUS
ElfpCopyFile (
    IN HANDLE SourceHandle,
    IN PUNICODE_STRING TargetFileName
    );

VOID
FreeModuleAndLogFileStructs (VOID);

NTSTATUS
ElfpFlushFiles (VOID);


VOID
InvalidateContextHandlesForLogFile (
    PLOGFILE    pLogFile
    );

VOID
FixContextHandlesForRecord (
    DWORD RecordOffset,
    DWORD NewRecordOffset
    );

PLOGFILE
FindLogFileFromName (
    PUNICODE_STRING LogFileName
    );

BOOL
SendAdminAlert (
    ULONG   MessageID,
    ULONG   NumStrings,
    UNICODE_STRING  *pStrings
    );

PVOID
NextRecordPosition (
    ULONG   ReadFlags,
    PVOID   CurrPosition,
    ULONG   CurrRecordLength,
    PVOID   BeginRecord,
    PVOID   EndRecord,
    PVOID   PhysicalEOF,
    PVOID   PhysStart
    );

VOID
NotifyChange (
    PLOGFILE pLogFile
    );

VOID
WriteQueuedEvents (
    VOID
    );

VOID
FlushQueuedEvents (
    VOID
    );

VOID
PerformWriteRequest (
    PELF_REQUEST_RECORD Request
    );

NTSTATUS
ElfpCreateLogFileObject(
    PLOGFILE LogFile,
    DWORD Type,
    ULONG GuestAccessRestriction
    );

VOID
ElfpDeleteLogFileObject(
    PLOGFILE LogFile
    );

VOID
ElfpCloseAudit(
    IN  LPWSTR      SubsystemName,
    IN  IELF_HANDLE ContextHandle
    );

NTSTATUS
ElfpAccessCheckAndAudit(
    IN     LPWSTR SubsystemName,
    IN     LPWSTR ObjectTypeName,
    IN     LPWSTR ObjectName,
    IN OUT IELF_HANDLE ContextHandle,
    IN     PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN     ACCESS_MASK DesiredAccess,
    IN     PGENERIC_MAPPING GenericMapping,
    IN     BOOL ForSecurityLog
    );

NTSTATUS
ElfCreateWellKnownSids(
    VOID
    );

VOID
ElfFreeWellKnownSids(
    VOID
    );

NTSTATUS
ElfCreateAndSetSD(
    IN  PRTL_ACE_DATA AceData,
    IN  ULONG AceCount,
    IN  PSID OwnerSid OPTIONAL,
    IN  PSID GroupSid OPTIONAL,
    OUT PSECURITY_DESCRIPTOR *NewDescriptor
    );

NTSTATUS
ElfCreateUserSecurityObject(
    IN  PRTL_ACE_DATA AceData,
    IN  ULONG AceCount,
    IN  PSID OwnerSid,
    IN  PSID GroupSid,
    IN  BOOLEAN IsDirectoryObject,
    IN  PGENERIC_MAPPING GenericMapping,
    OUT PSECURITY_DESCRIPTOR *NewDescriptor
    );

VOID
ElfpCreateElfEvent(
    IN ULONG EventId,
    IN USHORT EventType,
    IN USHORT EventCategory,
    IN USHORT NumStrings,
    IN LPWSTR * Strings,
    IN LPVOID Data,
    IN ULONG DataSize,
    IN USHORT Flags
    );


VOID
ElfpCreateQueuedAlert(
    DWORD MessageId,
    DWORD NumberOfStrings,
    LPWSTR Strings[]
    );

VOID
ElfpCreateQueuedMessage(
    DWORD MessageId,
    DWORD NumberOfStrings,
    LPWSTR Strings[]
    );

DWORD
ElfStatusUpdate(
    IN DWORD    NewState
    );

DWORD
GetElState (
    VOID
    );

VOID
ElfpGenerateLogClearedEvent(
    IELF_HANDLE     LogHandle
    );

VOID
ElInitStatus(
    VOID
    );

VOID
ElCleanupStatus(
    VOID
    );

DWORD
ElfBeginForcedShutdown(
    IN BOOL     PendingCode,
    IN DWORD    ExitCode,
    IN DWORD    ServiceSpecificCode
    );

NTSTATUS
ElfpTestClientPrivilege(
    IN ULONG  ulPrivilege,
    IN HANDLE hThreadToken     OPTIONAL
    );

//SS: added to extend clustering support
NTSTATUS
FindSizeofEventsSinceStart (
    OUT PULONG               pulTotalEventSize,
    IN PULONG                pulNumLogFiles,
    OUT PPROPLOGFILEINFO     *ppPropLogFileInfo
    );    
    
NTSTATUS
GetEventsToProp(
    IN PEVENTLOGRECORD       pEventLogRecords,
    IN PPROPLOGFILEINFO      pPropLogFileInfo
    );

//SS: end of changes for clustering

VOID
ElfWriteTimeStamp(
    TIMESTAMPEVENT  EventType,
    BOOLEAN         Append
    );

VOID CALLBACK
ElfWriteLastAliveTimeStamp(
    UINT    uID,
    UINT    uMsg,
    DWORD   dwUser,
    DWORD   dw1,
    DWORD   dw2
    );

#endif // ifndef _ELFPROTO_
