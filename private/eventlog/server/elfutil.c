/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ELFUTIL.C

Abstract:

    This file contains all the utility routines for the Eventlog service.

Author:

    Rajen Shah  (rajens)    16-Jul-1991


Revision History:


--*/

//
// INCLUDES
//

#include <eventp.h>
#include <lmalert.h>
#include <string.h>
#include <stdlib.h>



PLOGMODULE
FindModuleStrucFromAtom (
    ATOM Atom
    )

/*++

Routine Description:

    This routine scans the list of module structures and finds the one
    that matches the module atom.

Arguments:

    Atom contains the atom matching the module name.

Return Value:

    A pointer to the log module structure is returned.
    NULL if no matching atom is found.

Note:

--*/
{
    PLOGMODULE      ModuleStruc;

    // Lock the linked list

    RtlEnterCriticalSection ((PRTL_CRITICAL_SECTION)&LogModuleCritSec);

    ModuleStruc = CONTAINING_RECORD (
                        LogModuleHead.Flink,
                        LOGMODULE,
                        ModuleList
                        );

    while ((ModuleStruc->ModuleList.Flink != &LogModuleHead)
            &&
            (ModuleStruc->ModuleAtom != Atom)) {

        ModuleStruc = CONTAINING_RECORD (
                                ModuleStruc->ModuleList.Flink,
                                LOGMODULE,
                                ModuleList
                                );
    }

    // Unlock the linked list

    RtlLeaveCriticalSection ((PRTL_CRITICAL_SECTION)&LogModuleCritSec);

    return (ModuleStruc->ModuleAtom == Atom ? ModuleStruc : NULL);
}



PLOGMODULE
GetModuleStruc (
    PUNICODE_STRING ModuleName
    )

/*++

Routine Description:

    This routine returns a pointer to the log module structure for the
    module specified in ModuleName. If none exists, the default structure
    for application is returned.

Arguments:

    ModuleName contains the name of the module.

Return Value:

    A pointer to the log module structure is returned.

Note:


--*/
{
    NTSTATUS    Status;
    ATOM        ModuleAtom;
    ANSI_STRING ModuleNameA;
    PLOGMODULE  pModule;

    ElfDbgPrint(("[ELF] GetModuleStruc:  "));

    Status = RtlUnicodeStringToAnsiString (
                &ModuleNameA,
                ModuleName,
                TRUE
                );

    if (!NT_SUCCESS(Status)) {

        //
        // Not much else we can do here...
        //
        ElfDbgPrint((" GetModuleStruc:  RtlUnicodeStringToAnsiString FAILED!\n"));
        return (ElfDefaultLogModule);
    }

    //
    // Guarantee that it's NULL terminated
    //

    ModuleNameA.Buffer[ModuleNameA.Length] = '\0';

    ElfDbgPrint((" Module: %Z  ", &ModuleNameA));

    ModuleAtom = FindAtomA( ModuleNameA.Buffer );

    RtlFreeAnsiString (&ModuleNameA);

    if (ModuleAtom == (ATOM)0) {

        ElfDbgPrint((" No atom found. Defaulting to APPLICATION.\n"));
        return (ElfDefaultLogModule);

    }
     
    ElfDbgPrint((" Atom = %d \n", ModuleAtom));

    pModule = FindModuleStrucFromAtom( ModuleAtom );

    return (pModule != NULL ? pModule : ElfDefaultLogModule);
}



VOID
UnlinkContextHandle (
    IELF_HANDLE     LogHandle
    )

/*++

Routine Description:

    This routine unlinks the LogHandle specified from the linked list of
    context handles.
    In order to protect against multiple thread/process access to the
    list at the same time, we use a critical section.

Arguments:

    LogHandle points to a context handle structure.

Return Value:

    NONE

Note:


--*/
{
    // Lock the linked list

    RtlEnterCriticalSection ((PRTL_CRITICAL_SECTION)&LogHandleCritSec);


    // Remove this entry

    RemoveEntryList (&LogHandle->Next);


    // Unlock the linked list

    RtlLeaveCriticalSection ((PRTL_CRITICAL_SECTION)&LogHandleCritSec);
}


VOID
LinkContextHandle (
    IELF_HANDLE    LogHandle
    )

/*++

Routine Description:

    This routine links the LogHandle specified into the linked list of
    context handles.
    In order to protect against multiple thread/process access to the
    list at the same time, we use a critical section.

Arguments:

    LogHandle points to a context handle structure.

Return Value:

    NONE

Note:


--*/
{
    ASSERT(LogHandle->Signature == ELF_CONTEXTHANDLE_SIGN);

    // Lock the linked list

    RtlEnterCriticalSection ((PRTL_CRITICAL_SECTION)&LogHandleCritSec);


    // Place structure at the beginning of the list.

    InsertHeadList (&LogHandleListHead, &LogHandle->Next);


    // Unlock the linked list

    RtlLeaveCriticalSection ((PRTL_CRITICAL_SECTION)&LogHandleCritSec);
}


VOID
UnlinkQueuedEvent (
    PELF_QUEUED_EVENT QueuedEvent
    )

/*++

Routine Description:

    This routine unlinks the QueuedEvent specified from the linked list of
    QueuedEvents.
    In order to protect against multiple thread/process access to the
    list at the same time, we use a critical section.

Arguments:

    QueuedEvent - The request to remove from the linked list

Return Value:

    NONE

Note:


--*/
{
    // Lock the linked list

    RtlEnterCriticalSection ((PRTL_CRITICAL_SECTION)&QueuedEventCritSec);


    // Remove this entry

    RemoveEntryList (&QueuedEvent->Next);


    // Unlock the linked list

    RtlLeaveCriticalSection ((PRTL_CRITICAL_SECTION)&QueuedEventCritSec);
}



VOID
LinkQueuedEvent (
    PELF_QUEUED_EVENT QueuedEvent
    )

/*++

Routine Description:

    This routine links the QueuedEvent specified into the linked list of
    QueuedEvents.
    In order to protect against multiple thread/process access to the
    list at the same time, we use a critical section.

Arguments:

    QueuedEvent - The request to add from the linked list

Return Value:

    NONE

Note:


--*/
{
    // Lock the linked list

    RtlEnterCriticalSection ((PRTL_CRITICAL_SECTION)&QueuedEventCritSec);


    // Place structure at the beginning of the list.

    InsertHeadList (&QueuedEventListHead, &QueuedEvent->Next);


    // Unlock the linked list

    RtlLeaveCriticalSection ((PRTL_CRITICAL_SECTION)&QueuedEventCritSec);
}


DWORD
WINAPI
ElfpSendMessage (
    LPVOID UnUsed
    )

/*++

Routine Description:

    This routines just uses MessageBox to pop up a message.

    This is it's own routine so we can spin a thread to do this, in case the
    user doesn't hit the OK button for a while.

Arguments:

    NONE

Return Value:

    NONE

Note:

--*/
{
    PVOID MessageBuffer;
    HANDLE hLibrary;
    LPWSTR * StringPointers;
    DWORD i;
    PELF_QUEUED_EVENT QueuedEvent;
    PELF_QUEUED_EVENT FlushEvent;

    RtlEnterCriticalSection ((PRTL_CRITICAL_SECTION)&QueuedMessageCritSec);

    //
    // First get a handle to the message file used for the message text
    //

    hLibrary = LoadLibraryEx( L"NETMSG.DLL",
                              NULL,
                              LOAD_LIBRARY_AS_DATAFILE );

    //
    // Walk the linked list and process each element
    //

    QueuedEvent = CONTAINING_RECORD (
                        QueuedMessageListHead.Flink,
                        struct _ELF_QUEUED_EVENT,
                        Next
                        );

    while (QueuedEvent->Next.Flink != QueuedMessageListHead.Flink) {

        ASSERT(QueuedEvent->Type == Message);

        // Unlock the linked list

        RtlLeaveCriticalSection ((PRTL_CRITICAL_SECTION)&QueuedMessageCritSec);

        //
        // Build the array of pointers to the insertion strings
        //

        StringPointers = (LPWSTR *) ElfpAllocateBuffer(
            QueuedEvent->Event.Message.NumberOfStrings * sizeof(LPWSTR));

        if (StringPointers && hLibrary) {

            //
            // Build the array of pointers to the insertion string(s)
            //

            if (QueuedEvent->Event.Message.NumberOfStrings) {
                StringPointers[0] = (LPWSTR)
                    ((PBYTE) &QueuedEvent->Event.Message +
                    sizeof(ELF_MESSAGE_RECORD));

                for (i = 1; i < QueuedEvent->Event.Message.NumberOfStrings;
                  i++) {
                    StringPointers[i] = StringPointers[i-1] +
                        wcslen(StringPointers[i-1]) + 1;
                }
            }

            //
            // Call FormatMessage to build the message
            //

            if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                               FORMAT_MESSAGE_ARGUMENT_ARRAY |
                               FORMAT_MESSAGE_FROM_HMODULE,
                               hLibrary,
                               QueuedEvent->Event.Message.MessageId,
                               0, // Language ID defaulted
                               (LPWSTR) &MessageBuffer,
                               0, // Is this ignored if allocate_buffer?
                               (va_list *) StringPointers)) {

                //
                // Now actually display it
                //

                MessageBoxW(NULL, (LPWSTR) MessageBuffer, GlobalMessageBoxTitle,
                    MB_OK | MB_SETFOREGROUND | MB_ICONEXCLAMATION | MB_SERVICE_NOTIFICATION);
            }

            ElfpFreeBuffer(StringPointers);
        }

        RtlEnterCriticalSection ((PRTL_CRITICAL_SECTION)&QueuedMessageCritSec);

        //
        // Move to the next one, saving this one to delete it
        //

        FlushEvent = QueuedEvent;

        QueuedEvent = CONTAINING_RECORD (
                                QueuedEvent->Next.Flink,
                                struct _ELF_QUEUED_EVENT,
                                Next
                                );

        //
        // Now remove this from the queue and free it if we successfully
        // processed it
        //

        RemoveEntryList (&FlushEvent->Next);

    }

    FreeLibrary(hLibrary);

    MBThreadHandle = NULL;

    // Unlock the linked list

    RtlLeaveCriticalSection ((PRTL_CRITICAL_SECTION)&QueuedMessageCritSec);

    return(0);
}


VOID
LinkQueuedMessage (
    PELF_QUEUED_EVENT QueuedEvent
    )

/*++

Routine Description:

    This routine links the QueuedEvent specified into the linked list of
    QueuedMessages.  If there's not already a messagebox thread running,
    it starts one.
    In order to protect against multiple thread/process access to the
    list at the same time, we use a critical section.

Arguments:

    QueuedEvent - The request to add from the linked list

Return Value:

    NONE

Note:


--*/
{
    DWORD ThreadId;

    // Lock the linked list

    RtlEnterCriticalSection ((PRTL_CRITICAL_SECTION)&QueuedMessageCritSec);


    // Place structure at the end of the list.

    InsertTailList (&QueuedMessageListHead, &QueuedEvent->Next);

    if (!MBThreadHandle) {
        //
        // Since the user can just let this sit on their screen,
        // spin a thread for this
        //

        MBThreadHandle = CreateThread(NULL,               // lpThreadAttributes
                                      4096,               // dwStackSize
                                      ElfpSendMessage,    // lpStartAddress
                                      NULL,               // lpParameter
                                      0L,                 // dwCreationFlags
                                      &ThreadId);         // lpThreadId
    }

    // Unlock the linked list

    RtlLeaveCriticalSection ((PRTL_CRITICAL_SECTION)&QueuedMessageCritSec);
}


VOID
NotifyChange (
    PLOGFILE pLogFile
    )

/*++

Routine Description:

    This routine runs the list of events that are registered with
    ElfChangeNotify to be notified when a log has changed, and pulses
    the event.

    In order to protect against multiple thread/process access to the
    list at the same time, we use an exclusive resource.

Arguments:

    LogHandle points to a context handle structure.

Return Value:

    NONE

Note:

--*/
{

    //
    // How frequently will I try to pulse the events?  How about every
    // 5 seconds
    //

#define MINIMUM_PULSE_TIME 5

    PNOTIFIEE Notifiee;
    LARGE_INTEGER Time;
    ULONG CurrentTime;
    NTSTATUS Status;

    //
    // Get exclusive access to the log file. This will ensure no one
    // else is accessing the file.
    //

    RtlAcquireResourceExclusive (
                    &pLogFile->Resource,
                    TRUE);                  // Wait until available

    //
    // See if we've done this in the last MINIMUM_PULSE_TIME seconds
    //

    Status = NtQuerySystemTime(&Time);

    if (NT_SUCCESS(Status)) {

        RtlTimeToSecondsSince1970(&Time, &CurrentTime);
        CurrentTime = CurrentTime - MINIMUM_PULSE_TIME;

        if (CurrentTime > pLogFile->ulLastPulseTime) {

            //
            // Remember that we pulsed
            //

            pLogFile->ulLastPulseTime = CurrentTime;

            //
            // Walk the linked list and and pulse any events
            //

            Notifiee = CONTAINING_RECORD (
                                pLogFile->Notifiees.Flink,
                                struct _NOTIFIEE,
                                Next
                                );


            while (Notifiee->Next.Flink != pLogFile->Notifiees.Flink) {

                //
                // Pulse each event as we get to it.
                //

                NtPulseEvent(Notifiee->Event,NULL);

                Notifiee = CONTAINING_RECORD (
                                        Notifiee->Next.Flink,
                                        struct _NOTIFIEE,
                                        Next);
            }
        }
    }

    //
    // Free the resource
    //

    RtlReleaseResource ( &pLogFile->Resource );
}


VOID
WriteQueuedEvents (
    )

/*++

Routine Description:

    This routine runs the list of queued events and writes them.

    In order to protect against multiple thread/process access to the
    list at the same time, we use an exclusive resource.

Arguments:

    NONE

Return Value:

    NONE

Note:

--*/
{
    PELF_QUEUED_EVENT QueuedEvent;
    PELF_QUEUED_EVENT FlushEvent;
    BOOLEAN           bFlushEvent;
    LARGE_INTEGER Time;
    ULONG CurrentTime;
    static ULONG LastAlertTried = 0;
    static BOOLEAN LastAlertFailed = FALSE;

    // Lock the linked list, you must get the System Log File Resource
    // first, it is the higher level lock

    RtlAcquireResourceExclusive (
                    &ElfModule->LogFile->Resource,
                    TRUE);                  // Wait until available

    RtlEnterCriticalSection ((PRTL_CRITICAL_SECTION)&QueuedEventCritSec);

    //
    // Walk the linked list and process each element
    //

    QueuedEvent = CONTAINING_RECORD (
                        QueuedEventListHead.Flink,
                        struct _ELF_QUEUED_EVENT,
                        Next);

    while (QueuedEvent->Next.Flink != QueuedEventListHead.Flink) {

        //
        // Default is to flush the event after processing
        //

        bFlushEvent = TRUE;

        //
        // Do the appropriate thing
        //

        if (QueuedEvent->Type == Event) {
            PerformWriteRequest (&QueuedEvent->Event.Request);
        }
        else if (QueuedEvent->Type == Alert) {

            //
            // Don't even try to send failed alerts quicker than once a minute
            //

            NtQuerySystemTime(&Time);
            RtlTimeToSecondsSince1970(&Time, &CurrentTime);

            if (!LastAlertFailed || CurrentTime > LastAlertTried + 60) {

                LastAlertFailed = 
                    
                    !SendAdminAlert(QueuedEvent->Event.Alert.MessageId,
                                    QueuedEvent->Event.Alert.NumberOfStrings,
                                    (PUNICODE_STRING)((PBYTE) QueuedEvent +
                                        FIELD_OFFSET(ELF_QUEUED_EVENT, Event) +
                                        sizeof(ELF_ALERT_RECORD)));

                LastAlertTried = CurrentTime;
            }

            //
            // Only try to write it for 5 minutes, then give up (the
            // alerter service may not be configured to run)
            //

            if (LastAlertFailed
                &&
                QueuedEvent->Event.Alert.TimeOut > CurrentTime) {

                bFlushEvent = FALSE;
            }
        }

        //
        // Move to the next one, saving this one to delete it
        //

        FlushEvent = QueuedEvent;

        QueuedEvent = CONTAINING_RECORD (
                                QueuedEvent->Next.Flink,
                                struct _ELF_QUEUED_EVENT,
                                Next);

        //
        // Now remove this from the queue and free it if we successfully
        // processed it
        //

        if (bFlushEvent) {
            UnlinkQueuedEvent(FlushEvent);
            ElfpFreeBuffer(FlushEvent);
        }
    }

    // Unlock the linked list

    RtlLeaveCriticalSection ((PRTL_CRITICAL_SECTION)&QueuedEventCritSec);
    RtlReleaseResource (&ElfModule->LogFile->Resource);
}


VOID
FlushQueuedEvents (
    VOID
    )

/*++

Routine Description:

    This routine runs the list of queued events and frees them.

    In order to protect against multiple thread/process access to the
    list at the same time, we use an exclusive resource.

Arguments:

    NONE

Return Value:

    NONE

Note:

--*/
{

    PELF_QUEUED_EVENT QueuedEvent;
    PELF_QUEUED_EVENT FlushEvent;

    // Lock the linked list

    RtlEnterCriticalSection ((PRTL_CRITICAL_SECTION)&QueuedEventCritSec);

    //
    // Walk the linked list and and free the memory for any events
    //

    QueuedEvent = CONTAINING_RECORD (
                        QueuedEventListHead.Flink,
                        struct _ELF_QUEUED_EVENT,
                        Next);


    while (QueuedEvent->Next.Flink != QueuedEventListHead.Flink) {

        //
        // Free each event as we get to it.
        //

        FlushEvent = QueuedEvent;

        QueuedEvent = CONTAINING_RECORD (
                                QueuedEvent->Next.Flink,
                                struct _ELF_QUEUED_EVENT,
                                Next
                                );

        ElfpFreeBuffer(FlushEvent);
    }

    // Unlock the linked list

    RtlLeaveCriticalSection ((PRTL_CRITICAL_SECTION)&QueuedEventCritSec);
}


VOID
UnlinkLogModule (
    PLOGMODULE LogModule
    )

/*++

Routine Description:

    This routine unlinks the LogModule specified from the linked list.
    In order to protect against multiple thread/process access to the
    list at the same time, we use a critical section.

Arguments:

    LogModule points to a context handle structure.

Return Value:

    NONE

Note:


--*/
{
    // Lock the linked list

    RtlEnterCriticalSection ((PRTL_CRITICAL_SECTION)&LogModuleCritSec);


    // Remove this entry

    RemoveEntryList (&LogModule->ModuleList);


    // Unlock the linked list

    RtlLeaveCriticalSection ((PRTL_CRITICAL_SECTION)&LogModuleCritSec);
}



VOID
LinkLogModule (
    PLOGMODULE    LogModule,
    ANSI_STRING * pModuleNameA
    )

/*++

Routine Description:

    This routine links the LogModule specified into the linked list.
    In order to protect against multiple thread/process access to the
    list at the same time, we use a critical section.

Arguments:

    LogModule points to a context handle structure.
    ANSI LogModule name.

Return Value:

    NONE

Note:


--*/
{
    // Lock the linked list

    RtlEnterCriticalSection ((PRTL_CRITICAL_SECTION)&LogModuleCritSec);


    // Add the atom for this module.

    LogModule->ModuleAtom = AddAtomA(pModuleNameA->Buffer);

    // Place structure at the beginning of the list.

    InsertHeadList (&LogModuleHead, &LogModule->ModuleList);


    // Unlock the linked list

    RtlLeaveCriticalSection ((PRTL_CRITICAL_SECTION)&LogModuleCritSec);
}


VOID
UnlinkLogFile (
    PLOGFILE pLogFile
    )

/*++

Routine Description:

    This routine unlinks the LogFile structure specified from the linked
    list of log files;
    In order to protect against multiple thread/process access to the
    list at the same time, we use a critical section.

Arguments:

    pLogFile points to a log file structure.

Return Value:

    NONE

Note:


--*/
{
    // Lock the linked list

    RtlEnterCriticalSection ((PRTL_CRITICAL_SECTION)&LogFileCritSec);


    // Remove this entry

    RemoveEntryList (&pLogFile->FileList);


    // Unlock the linked list

    RtlLeaveCriticalSection ((PRTL_CRITICAL_SECTION)&LogFileCritSec);
}



VOID
LinkLogFile (
    PLOGFILE   pLogFile
    )

/*++

Routine Description:

    This routine links the LogFile specified into the linked list of
    log files.
    In order to protect against multiple thread/process access to the
    list at the same time, we use a critical section.

Arguments:

    pLogFile points to a context handle structure.

Return Value:

    NONE

Note:


--*/
{
    // Lock the linked list

    RtlEnterCriticalSection ((PRTL_CRITICAL_SECTION)&LogFileCritSec);


    // Place structure at the beginning of the list.

    InsertHeadList (&LogFilesHead, &pLogFile->FileList);


    // Unlock the linked list

    RtlLeaveCriticalSection ((PRTL_CRITICAL_SECTION)&LogFileCritSec);
}



VOID
GetGlobalResource (
    DWORD Type
    )

/*++

Routine Description:

    This routine takes the global resource either for shared access or
    exclusive access depending on the value of Type. It waits forever for
    the resource to become available.

Arguments:

    Type is one of ELF_GLOBAL_SHARED or ELF_GLOBAL_EXCLUSIVE.

Return Value:

    NONE

Note:


--*/
{
    BOOL    Acquired;

    if (Type & ELF_GLOBAL_SHARED) {

        Acquired = RtlAcquireResourceShared(
                        &GlobalElfResource,
                        TRUE);                  // Wait forever

    } else {    // Assume EXCLUSIVE

        Acquired = RtlAcquireResourceExclusive(
                        &GlobalElfResource,
                        TRUE);                  // Wait forever
    }
 
    ASSERT (Acquired);      // This must always be TRUE.
}


VOID
ReleaseGlobalResource(
    VOID
    )

/*++

Routine Description:

    This routine releases the global resource.

Arguments:

    NONE

Return Value:

    NONE

Note:


--*/
{
    RtlReleaseResource ( &GlobalElfResource );
}


VOID
InvalidateContextHandlesForLogFile (
    PLOGFILE    pLogFile
    )

/*++

Routine Description:

    This routine walks through the context handles and marks the ones
    that point to the LogFile passed in as "invalid for read".

Arguments:

    Pointer to log file structure.

Return Value:

    NONE.

Note:


--*/
{
    IELF_HANDLE LogHandle;
    PLOGMODULE  pLogModule;

    //
    // Lock the context handle list
    //
    RtlEnterCriticalSection ((PRTL_CRITICAL_SECTION)&LogHandleCritSec);

    //
    // Walk the linked list and mark any matching context handles as
    // invalid.
    //

    LogHandle = CONTAINING_RECORD (
                        LogHandleListHead.Flink,
                        struct _IELF_HANDLE,
                        Next);


    while (LogHandle->Next.Flink != LogHandleListHead.Flink) {

        pLogModule = FindModuleStrucFromAtom (LogHandle->Atom);

        ASSERT(pLogModule);

        if (pLogFile == pLogModule->LogFile) {
            LogHandle->Flags |= ELF_LOG_HANDLE_INVALID_FOR_READ;
        }

        LogHandle = CONTAINING_RECORD (
                                LogHandle->Next.Flink,
                                struct _IELF_HANDLE,
                                Next);
    }

    //
    // Unlock the context handle list
    //
    RtlLeaveCriticalSection ((PRTL_CRITICAL_SECTION)&LogHandleCritSec);
}


VOID
FixContextHandlesForRecord (
    DWORD RecordOffset,
    DWORD NewRecordOffset
    )

/*++

Routine Description:

    This routine makes sure that the record starting at RecordOffset isn't
    the current record for any open handle.  If it is, the handle is adjusted
    to point to the next record.

Arguments:

    RecordOffset - The byte offset in the log of the record that is about
                   to be overwritten.
    NewStartingRecord - The new location to point the handle to (this is the
                        new first record)

Return Value:

    NONE.

Note:


--*/
{
    IELF_HANDLE LogHandle;

    //
    // Lock the context handle list
    //

    RtlEnterCriticalSection ((PRTL_CRITICAL_SECTION)&LogHandleCritSec);

    //
    // Walk the linked list and fix any matching context handles
    //

    LogHandle = CONTAINING_RECORD (
                        LogHandleListHead.Flink,
                        struct _IELF_HANDLE,
                        Next);


    while (LogHandle->Next.Flink != LogHandleListHead.Flink) {

        if (LogHandle->SeekBytePos == RecordOffset) {
           LogHandle->SeekBytePos = NewRecordOffset;
        }

        LogHandle = CONTAINING_RECORD (
                                LogHandle->Next.Flink,
                                struct _IELF_HANDLE,
                                Next);
    }

    //
    // Unlock the context handle list
    //

    RtlLeaveCriticalSection ((PRTL_CRITICAL_SECTION)&LogHandleCritSec);
}


PLOGFILE
FindLogFileFromName (
    PUNICODE_STRING pFileName
    )

/*++

Routine Description:

    This routine looks at all the log files to find one that matches
    the name passed in.

Arguments:

    Pointer to name of file.

Return Value:

    Matching LOGFILE structure if file in use.

Note:


--*/
{
    PLOGFILE pLogFile;

    // Lock the linked list

    RtlEnterCriticalSection ((PRTL_CRITICAL_SECTION)&LogFileCritSec);

    pLogFile = CONTAINING_RECORD(
                        LogFilesHead.Flink,
                        LOGFILE,
                        FileList);

    while (pLogFile->FileList.Flink != LogFilesHead.Flink) {

        if (wcscmp (pLogFile->LogFileName->Buffer, pFileName->Buffer) == 0)
            break;

        pLogFile = CONTAINING_RECORD(
                        pLogFile->FileList.Flink,
                        LOGFILE,
                        FileList);
    }

    // Unlock the linked list

    RtlLeaveCriticalSection ((PRTL_CRITICAL_SECTION)&LogFileCritSec);

    return (pLogFile->FileList.Flink == LogFilesHead.Flink ? NULL : pLogFile);
}


VOID
ElfpCreateElfEvent(
    IN ULONG  EventId,
    IN USHORT EventType,
    IN USHORT EventCategory,
    IN USHORT NumStrings,
    IN LPWSTR * Strings,
    IN LPVOID Data,
    IN ULONG  DataSize,
    IN USHORT Flags
    )

/*++

Routine Description:

    This creates an request packet to write an event on behalf of the event
    log service itself.  It then queues this packet to a linked list for
    writing later.

Arguments:

    The fields to use to create the event record


Return Value:

    None

Note:


--*/
{
    PELF_QUEUED_EVENT QueuedEvent;
    PWRITE_PKT WritePkt;
    PEVENTLOGRECORD EventLogRecord;
    PBYTE BinaryData;
    ULONG RecordLength;
    ULONG StringOffset, DataOffset;
    USHORT StringsSize = 0;
    USHORT i;
    ULONG PadSize;
    ULONG ModuleNameLen; // Length in bytes
    ULONG zero = 0;      // For pad bytes
    LARGE_INTEGER    Time;
    ULONG LogTimeWritten;
    PWSTR ReplaceStrings;

#define ELF_MODULE_NAME L"EventLog"


    // LogTimeWritten
    // We need to generate a time when the log is written. This
    // gets written in the log so that we can use it to test the
    // retention period when wrapping the file.
    //

    NtQuerySystemTime(&Time);
    RtlTimeToSecondsSince1970(
                        &Time,
                        &LogTimeWritten);

    //
    // Figure out how big a buffer to allocate
    //

    ModuleNameLen = (wcslen(ELF_MODULE_NAME) + 1) * sizeof (WCHAR);

    StringOffset = sizeof(EVENTLOGRECORD)
                     + ModuleNameLen
                     + ComputerNameLength;

    //
    // Calculate the length of strings so that we can see how
    // much space is needed for that.
    //

    for (i = 0; i < NumStrings; i++) {
        StringsSize += wcslen(Strings[i]) + 1;
    }

    //
    // DATA OFFSET:
    //

    DataOffset = StringOffset + StringsSize * sizeof(WCHAR);

    //
    // Determine how big a buffer is needed for the queued event record.
    //

    RecordLength = sizeof(ELF_QUEUED_EVENT)
                     + sizeof(WRITE_PKT)
                     + DataOffset
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

    QueuedEvent = (PELF_QUEUED_EVENT) ElfpAllocateBuffer(RecordLength);

    WritePkt = (PWRITE_PKT) (QueuedEvent + 1);

    if (QueuedEvent != NULL) {

        //
        // Fill up the event record
        //


        RecordLength  -= (sizeof(ELF_QUEUED_EVENT) + sizeof(WRITE_PKT));
        EventLogRecord = (PEVENTLOGRECORD) (WritePkt + 1);

        EventLogRecord->Length              = RecordLength;
        EventLogRecord->TimeGenerated       = LogTimeWritten;
        EventLogRecord->Reserved            = ELF_LOG_FILE_SIGNATURE;
        EventLogRecord->TimeWritten         = LogTimeWritten;
        EventLogRecord->EventID             = EventId;
        EventLogRecord->EventType           = EventType;
        EventLogRecord->EventCategory       = EventCategory;
        EventLogRecord->ReservedFlags       = 0;
        EventLogRecord->ClosingRecordNumber = 0;
        EventLogRecord->NumStrings          = NumStrings;
        EventLogRecord->StringOffset        = StringOffset;
        EventLogRecord->DataLength          = DataSize;
        EventLogRecord->DataOffset          = DataOffset;
        EventLogRecord->UserSidLength       = 0;
        EventLogRecord->UserSidOffset       = StringOffset;

        //
        // Fill in the variable-length fields
        //

        //
        // STRINGS
        //

        ReplaceStrings = (PWSTR) ((PBYTE) EventLogRecord
                                  + StringOffset);

        for (i = 0; i < NumStrings; i++) {
            wcscpy (ReplaceStrings, Strings[i]);
            ReplaceStrings += wcslen(Strings[i]) + 1;
        }

        //
        // MODULENAME
        //

        BinaryData = (PBYTE) EventLogRecord + sizeof(EVENTLOGRECORD);
        RtlMoveMemory (BinaryData,
                       ELF_MODULE_NAME,
                       ModuleNameLen);

        //
        // COMPUTERNAME
        //

        BinaryData += ModuleNameLen; // Now point to computername

        RtlMoveMemory (BinaryData,
                       LocalComputerName,
                       ComputerNameLength);

        //
        // BINARY DATA
        //

        BinaryData = (PBYTE) ((PBYTE) EventLogRecord + DataOffset);
        RtlMoveMemory (BinaryData, Data, DataSize);

        //
        // PAD  - Fill with zeros
        //

        BinaryData += DataSize;
        RtlMoveMemory (BinaryData, &zero, PadSize);

        //
        // LENGTH at end of record
        //

        BinaryData += PadSize; // Point after pad bytes
        ((PULONG)BinaryData)[0] = RecordLength;

        //
        // Build the QueuedEvent Packet
        //

        QueuedEvent->Type = Event;

        QueuedEvent->Event.Request.Pkt.WritePkt           = WritePkt;
        QueuedEvent->Event.Request.Module                 = ElfModule;
        QueuedEvent->Event.Request.Flags                  = Flags;
        QueuedEvent->Event.Request.LogFile                = ElfModule->LogFile;
        QueuedEvent->Event.Request.Command                = ELF_COMMAND_WRITE;
        QueuedEvent->Event.Request.Pkt.WritePkt->Buffer   = EventLogRecord;
        QueuedEvent->Event.Request.Pkt.WritePkt->Datasize = RecordLength;

        //
        // Now Queue it on the linked list
        //

        LinkQueuedEvent(QueuedEvent);
    }
}


VOID
ElfpCreateQueuedAlert(
    DWORD MessageId,
    DWORD NumberOfStrings,
    LPWSTR Strings[]
    )
{
    DWORD i;
    DWORD RecordLength;
    PELF_QUEUED_EVENT QueuedEvent;
    PUNICODE_STRING UnicodeStrings;
    LPWSTR pString;
    PBYTE ptr;
    LARGE_INTEGER Time;
    ULONG CurrentTime;

    //
    // Turn the input strings into UNICODE_STRINGS and figure out how
    // big to make the buffer to allocate
    //

    RecordLength   = sizeof(UNICODE_STRING) * NumberOfStrings;
    UnicodeStrings = ElfpAllocateBuffer(RecordLength);

    if (!UnicodeStrings) {
        return;
    }

    RecordLength += FIELD_OFFSET(ELF_QUEUED_EVENT, Event) + 
                      sizeof(ELF_ALERT_RECORD);

    for (i = 0; i < NumberOfStrings; i++) {

        RtlInitUnicodeString(&UnicodeStrings[i], Strings[i]);
        RecordLength += UnicodeStrings[i].MaximumLength;
    }

    //
    // Now allocate what will be the real queued event
    //

    QueuedEvent = ElfpAllocateBuffer(RecordLength);

    if (!QueuedEvent) {

        ElfpFreeBuffer(UnicodeStrings);
        return;
    }

    QueuedEvent->Type = Alert;

    QueuedEvent->Event.Alert.MessageId       = MessageId;
    QueuedEvent->Event.Alert.NumberOfStrings = NumberOfStrings;

    //
    // If we can't send the alert in 5 minutes, give up
    //

    NtQuerySystemTime(&Time);
    RtlTimeToSecondsSince1970(&Time, &CurrentTime);
    QueuedEvent->Event.Alert.TimeOut = CurrentTime + 300;

    //
    // Move the array of UNICODE_STRINGS into the queued event and
    // point UnicodeStrings at it.  Then fix up the Buffer pointers.
    //

    ptr = (PBYTE) QueuedEvent + FIELD_OFFSET(ELF_QUEUED_EVENT, Event) +
              sizeof(ELF_ALERT_RECORD);

    RtlCopyMemory(ptr,
                  UnicodeStrings,
                  sizeof(UNICODE_STRING) * NumberOfStrings);

    ElfpFreeBuffer(UnicodeStrings);
    UnicodeStrings = (PUNICODE_STRING) ptr;

    pString = (LPWSTR) (ptr + sizeof(UNICODE_STRING) * NumberOfStrings);

    for (i = 0; i < NumberOfStrings; i++) {

        RtlMoveMemory(pString, UnicodeStrings[i].Buffer,
            UnicodeStrings[i].MaximumLength);
        UnicodeStrings[i].Buffer = pString;
        pString =
            (LPWSTR) ((PBYTE) pString + UnicodeStrings[i].MaximumLength);
    }

    LinkQueuedEvent(QueuedEvent);
}


VOID
ElfpCreateQueuedMessage(
    DWORD MessageId,
    DWORD NumberOfStrings,
    LPWSTR Strings[]
    )
{
    DWORD i;
    DWORD RecordLength = 0;
    PELF_QUEUED_EVENT QueuedEvent;
    LPWSTR pString;

    //
    // Figure out how big to make the buffer to allocate
    //

    RecordLength = sizeof(ELF_QUEUED_EVENT);

    for (i = 0; i < NumberOfStrings; i++) {

        RecordLength += (wcslen(Strings[i]) + 1) * sizeof(WCHAR);
    }

    //
    // Now allocate what will be the real queued event
    //

    QueuedEvent = ElfpAllocateBuffer(RecordLength);

    if (!QueuedEvent) {
        return;
    }

    QueuedEvent->Type = Message;

    QueuedEvent->Event.Message.MessageId       = MessageId;
    QueuedEvent->Event.Message.NumberOfStrings = NumberOfStrings;

    //
    // Move the array of UNICODE strings into the queued event
    //

    pString = (LPWSTR) ((PBYTE) QueuedEvent +
                  FIELD_OFFSET(ELF_QUEUED_EVENT, Event) +
                  sizeof(ELF_MESSAGE_RECORD));

    for (i = 0; i < NumberOfStrings; i++) {
       wcscpy(pString, Strings[i]);
       pString += wcslen(Strings[i]) + 1;
    }

    LinkQueuedMessage(QueuedEvent);
}