/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Regnckey.c

Abstract:

    This module contains the Win32 Registry APIs to notify a caller about
    a changed Key value. That is:

        - RegNotifyChangeKey

Author:

    David J. Gilman (davegi) 10-Feb-1992


Notes:


    The RegNotifyChangeKey server creates an event and calls
    NtNotifyChangeKey asynchronously with that event.  It then
    places the event (plus some other client information, such
    as a named pipe and a client event) in a "Notification List"
    and returns to the client.

    A Notification List is a list of events controled by a
    handler thread. The handler thread waits on the events in
    the list. When an event is signaled the handler thread
    identifies the client to which the event belongs, and
    gives the client (via named pipe) the corresponding client
    event.

    Since there is a limit on the number of events on which a
    thread can wait, there may be several Notification Lists.

    Since all the calls to RegNotifyChangeKey in a client
    process use the same named pipe, we maintain only one copy
    of each pipe. Pipe information is maintained in a symbol table
    for fast lookup.



Revision History:

    02-Apr-1992     Ramon J. San Andres (ramonsa)
                    Changed to use RPC.


--*/

#include <rpc.h>
#include "regrpc.h"
#include "localreg.h"
#include <string.h>


#ifndef REMOTE_NOTIFICATION_DISABLED
//
//  Strings used for generating named pipe names
//
#define NAMED_PIPE_HERE     L"\\Device\\NamedPipe\\"
#define NAMED_PIPE_THERE    L"\\DosDevices\\UNC\\"


//
//  Pipe names are maintained in a symbol table. The symbol table has
//  one entry for each different pipe given by a client.  The entry
//  is maintained for as long as there is at least one entry in a
//  Notification List referencing it.
//
typedef struct _PIPE_ENTRY *PPIPE_ENTRY;

typedef struct _PIPE_ENTRY {

    PPIPE_ENTRY             Previous;
    PPIPE_ENTRY             Next;
    UNICODE_STRING          PipeName;
    DWORD                   ReferenceCount;
    PSECURITY_DESCRIPTOR    SecurityDescriptor;

} PIPE_ENTRY;




//
//  The PIPE_SYMBOL_TABLE structure contains the symbol table for
//  all the pipes being used by the clients of
//  RegNotifyChangeKey
//
#define BUCKETS_IN_SYMBOL_TABLE     211

typedef struct _PIPE_SYMBOL_TABLE   *PPIPE_SYMBOL_TABLE;

typedef struct _PIPE_SYMBOL_TABLE {

    PPIPE_ENTRY             Bucket[ BUCKETS_IN_SYMBOL_TABLE ];

} PIPE_SYMBOL_TABLE;




//
//  Information about a pending event is maintained in a
//  NOTIFICATION_ENTRY structure.
//
typedef struct _NOTIFICATION_ENTRY *PNOTIFICATION_ENTRY;

typedef struct _NOTIFICATION_ENTRY {

    DWORD                   ClientEvent;    //  Event in client side
    HANDLE                  hKey;           //  Key handle
    DWORD                   Flags;          //  Misc. flags
    PPIPE_ENTRY             PipeEntry;      //  Pipe Entry

} NOTIFICATION_ENTRY;


//
//  Flag values
//
#define CLIENT_IS_DEAD       0x00000001
#define MUST_NOTIFY          0x00000002
#define NOTIFICATION_FAILED  0x00000004



//
//  The pending events are maintained in notification lists. Each
//  notification list contains:
//
//  Previous        -   Previous in chain
//  Next            -   Next in chain
//  EventsInUse     -   Number of entries being used in this list
//  EventHandle     -   Array of event handles
//  ClientEvent     -   Array of events in client
//  PipeEntry       -   Array of pointers to pipe entries in symbol table
//
//
//  The first event in the EventHandle list is the event used to wake
//  up the thread whenever we add new entries to the list.
//
//  The array entries 0..EventsInUse-1 contain the pending events.
//  New events are always added at position EventsInUse. When removing
//  an event, all the arrays are shifted.
//
//  Whenever EventsInUse == 1, the list is empty of client events and
//  it can be removed (together with its thread).
//
//
//  Notification Lists are kept in a doubly-linked list. A new
//  Notification List is created and added to the chain whenever an
//  event is added and all the existing lists are full.  Notification
//  lists are deleted when the last event in the list is signaled.
//
//
typedef struct _NOTIFICATION_LIST *PNOTIFICATION_LIST;

typedef struct _NOTIFICATION_LIST {

    PNOTIFICATION_LIST      Previous;
    PNOTIFICATION_LIST      Next;
    DWORD                   EventsInUse;
    HANDLE                  HandlerThread;
    CLIENT_ID               HandlerClientId;
    DWORD                   PendingNotifications;
    HANDLE                  EventHandle[ MAXIMUM_WAIT_OBJECTS ];
    NOTIFICATION_ENTRY      Event[ MAXIMUM_WAIT_OBJECTS ];
    DWORD                   TimeOutCount;
    BOOLEAN                 ResetCount;

} NOTIFICATION_LIST;

#define MAX_TIMEOUT_COUNT   128


#if DBG
    #define BIGDBG 0
#else
    #define BIGDBG 0
#endif

#define HASH(a,b)   Hash(a,b)



// *****************************************************************
//
//                    Static Variables
//
// *****************************************************************



//
//  Head of chain of Notification lists
//
PNOTIFICATION_LIST      NotificationListChainHead;

//
//  The critical sesction protects all the global structures.
//
RTL_CRITICAL_SECTION    NotificationCriticalSection;

//
//  Symbol table for named pipes in use.
//
PIPE_SYMBOL_TABLE       PipeSymbolTable;

//
//  Our machine name is used for determining if requests are local
//  or remote.
//
WCHAR                   OurMachineNameBuffer[ MAX_PATH ];
UNICODE_STRING          OurMachineName;

//
//  The I/O Status Block is updated by the NtNotifyChangeKey API
//  upon notification.  We cannot put this structure on the stack
//  because at notification time this stack might belong to someone
//  else. We can use a single variable because we don't care about
//  its contents so it's ok if several people mess with it at the
//  same time.
//
IO_STATUS_BLOCK         IoStatusBlock;





// *****************************************************************
//
//                    Local Prototypes
//
// *****************************************************************



LONG
CreateNotificationList (
    OUT PNOTIFICATION_LIST  *NotificationListUsed
    );

LONG
DeleteNotificationList (
    IN OUT  PNOTIFICATION_LIST  NotificationList
    );

LONG
AddEvent (
    IN  HKEY                     hKey,
    IN  HANDLE                   EventHandle,
    IN  DWORD                    ClientEvent,
    IN  PUNICODE_STRING          PipeName,
    IN  PRPC_SECURITY_ATTRIBUTES pRpcSa OPTIONAL,
    OUT PNOTIFICATION_LIST      *NotificationListUsed
    );

LONG
RemoveEvent (
    IN      HANDLE              EventHandle,
    IN OUT  PNOTIFICATION_LIST  NotificationList
    );

LONG
GetAvailableNotificationList (
    OUT PNOTIFICATION_LIST  *NotificationListUsed
    );

LONG
AddEntryToNotificationList(
    IN OUT  PNOTIFICATION_LIST       NotificationList,
    IN      HKEY                     hKey,
    IN      HANDLE                   EventHandle,
    IN      DWORD                    ClientEvent,
    IN      PUNICODE_STRING          PipeName,
    IN      PRPC_SECURITY_ATTRIBUTES pRpcSa     OPTIONAL
    );

LONG
RemoveEntryFromNotificationList (
    IN OUT  PNOTIFICATION_LIST  NotificationList,
    IN      DWORD               EntryIndex
    );

LONG
CompactNotificationList (
    IN OUT  PNOTIFICATION_LIST  NotificationList
    );

VOID
AddNotificationListToChain(
    IN OUT  PNOTIFICATION_LIST  NotificationList
    );

VOID
RemoveNotificationListFromChain(
    IN OUT  PNOTIFICATION_LIST  NotificationList
    );

LONG
GetFullPipeName(
    IN      PUNICODE_STRING         MachineName,
    IN      PUNICODE_STRING         PipeName,
    IN OUT  PUNICODE_STRING         FullPipeName
    );

LONG
CreatePipeEntry (
    IN  PUNICODE_STRING          PipeName,
    IN  PRPC_SECURITY_ATTRIBUTES pRpcSa     OPTIONAL,
    OUT PPIPE_ENTRY              *PipeEntryUsed
    );

LONG
DeletePipeEntry(
    IN OUT PPIPE_ENTRY  PipeEntry
    );

LONG
AddPipe(
    IN  PUNICODE_STRING          PipeName,
    IN  PRPC_SECURITY_ATTRIBUTES pRpcSa     OPTIONAL,
    OUT PPIPE_ENTRY              *PipeEntryUsed
    );

LONG
RemovePipe(
    IN OUT PPIPE_ENTRY  PipeEntry
    );

LONG
AddPipeEntryToSymbolTable(
    IN OUT  PPIPE_ENTRY PipeEntry
    );

LONG
RemovePipeEntryFromSymbolTable(
    IN OUT  PPIPE_ENTRY PipeEntry
    );

LONG
LookForPipeEntryInSymbolTable(
    IN  PUNICODE_STRING PipeName,
    OUT PPIPE_ENTRY     *PipeEntryUsed
    );

DWORD
Hash(
    IN  PUNICODE_STRING  Symbol,
    IN  DWORD            Buckets
    );

VOID
NotificationHandler(
    IN  PNOTIFICATION_LIST  NotificationList
    );

DWORD
NotificationListMaintenance(
    IN OUT  PNOTIFICATION_LIST  NotificationList
    );

LONG
SendEventToClient(
    IN  DWORD           ClientEvent,
    IN  PPIPE_ENTRY     PipeEntry
    );

#if BIGDBG
VOID
DumpNotificationLists(
    );

VOID
DumpPipeTable(
    );

#endif

#endif // REMOTE_NOTIFICATION_DISABLED



// *****************************************************************
//
//                    BaseRegNotifyChangeKeyValue
//
// *****************************************************************





BOOL
InitializeRegNotifyChangeKeyValue(
    )
/*++

Routine Description:


    Initializes the static data structures used by the
    RegNotifyChangeKeyValue server. Called once at program
    initialization.

Arguments:

    None

Return Value:

    BOOLEAN -   TRUE if successful.


--*/

{
#ifdef REMOTE_NOTIFICATION_DISABLED

    return( TRUE );

#else   // REMOTE_NOTIFICATION_DISABLED

    NTSTATUS        NtStatus;
    DWORD           Bucket;
    DWORD           MachineNameLength;


    NotificationListChainHead = NULL;

    //
    //  Determine our machine name
    //
    MachineNameLength = MAX_PATH;
    if ( !GetComputerNameW( OurMachineNameBuffer, &MachineNameLength ) ) {
        return FALSE;
    }

    OurMachineName.Buffer        = OurMachineNameBuffer;
    OurMachineName.Length        = (USHORT)(MachineNameLength * sizeof(WCHAR));
    OurMachineName.MaximumLength = (USHORT)(MAX_PATH * sizeof(WCHAR));


    //
    //  Initialize Notification critical section
    //
    NtStatus = RtlInitializeCriticalSection(
                    &NotificationCriticalSection
                    );

    if ( !NT_SUCCESS( NtStatus ) ) {
        return FALSE;
    }


    //
    //  Initialize the pipe symbol table
    //
    for ( Bucket = 0; Bucket < BUCKETS_IN_SYMBOL_TABLE; Bucket++ ) {
        PipeSymbolTable.Bucket[Bucket] = NULL;
    }

    return TRUE;
#endif   // REMOTE_NOTIFICATION_DISABLED
}




error_status_t
BaseRegNotifyChangeKeyValue(
    IN  HKEY                     hKey,
    IN  BOOLEAN                  fWatchSubtree,
    IN  DWORD                    dwNotifyFilter,
    IN  DWORD                    hEvent,
    IN  PUNICODE_STRING          MachineName,
    IN  PUNICODE_STRING          PipeName,
    IN  PRPC_SECURITY_ATTRIBUTES pRpcSa OPTIONAL
    )

/*++

Routine Description:

    This API is used to watch a key or sub-tree for changes. It is
    asynchronous. It is possible to filter the criteria by which the
    notification occurs.


Arguments:

    hKey - Supplies a handle to a key that has been previously opened with
        KEY_NOTIFY access.

    fWatchSubtree - Supplies a boolean value that if TRUE causes the
        system to monitor the key and all of its decsendants.  A value of
        FALSE causes the system to monitor only the specified key.

    dwNotifyFilter - Supplies a set of flags that specify the filter
        conditions the system uses to satisfy a change notification.

        REG_NOTIFY_CHANGE_KEYNAME - Any key name changes that occur
            in a key or subtree being watched will satisfy a
            change notification wait.  This includes creations
            and deletions.

        REG_NOTIFY_CHANGE_ATTRIBUTES - Any attribute changes that occur
            in a key or subtree being watched will satisfy a
            change notification.

        REG_NOTIFY_CHANGE_LAST_WRITE - Any last write time changes that
            occur in a key or subtree being watched will satisfy a
            change notification.

        REG_NOTIFY_CHANGE_SECURITY - Any security descriptor changes
            that occur in a key or subtree being watched will
            satisfy a change notification.


    hEvent - Supplies a DWORD which represents an event that will have to
             be communicated to the client (via named pipe) when a key
             has to be notified.


    PipeName - Supplies the name of the pipe used for communicating
            the notification to the client.

    pRpcSa - Supplies the optional security attributes of the named
            pipe.

Return Value:

    LONG -  Returns ERROR_SUCCESS (0); error-code for failure.


--*/

{

#ifdef REMOTE_NOTIFICATION_DISABLED

    return ERROR_INVALID_PARAMETER;

#else   // REMOTE_NOTIFICATION_DISABLED

    NTSTATUS            NtStatus;
    HANDLE              EventHandle;
    PNOTIFICATION_LIST  NotificationList;
    LONG                Error;
    UNICODE_STRING      FullPipeName;


    RPC_IMPERSONATE_CLIENT( NULL );

    //
    //  Enter the critical section
    //
    NtStatus = RtlEnterCriticalSection( &NotificationCriticalSection );
    if ( !NT_SUCCESS( NtStatus ) ) {
        return (error_status_t)RtlNtStatusToDosError( NtStatus );
    }

    try {

#if BIGDBG
        DbgPrint( "WINREG: RegNotify entered\n" );

        //DbgPrint( "WINREG: Notification requested. HKEY 0x%x, Client 0x%x, pipe %wZ\n",
        //           hKey, hEvent, PipeName );
        //DbgPrint( "       Watch subtree: 0x%x, filter 0x%x\n", fWatchSubtree, dwNotifyFilter );

#endif


        //
        //  Subtract the NULL from the Length of all the strings.
        //  This was added by the client so that RPC would transmit
        //  the whole thing.
        //
        if ( MachineName->Length > 0 ) {
            MachineName->Length -= sizeof(UNICODE_NULL );
        }
        if ( PipeName->Length > 0 ) {
            PipeName->Length -= sizeof(UNICODE_NULL );
        }

        //
        //  Construct the full pipe name based on the machine name
        //  and the pipe name given.
        //
        FullPipeName.Buffer = RtlAllocateHeap(
                                RtlProcessHeap( ), 0,
                                MAX_PATH * sizeof(WCHAR)
                                );

        if ( !FullPipeName.Buffer ) {

            Error = ERROR_OUTOFMEMORY;

        } else {


            FullPipeName.Length         = 0;
            FullPipeName.MaximumLength  = MAX_PATH * sizeof(WCHAR);


            Error = GetFullPipeName(
                        MachineName,
                        PipeName,
                        &FullPipeName
                        );

            if ( Error == ERROR_SUCCESS ) {

                //
                //  Create an event on which we will wait for completion of
                //  the API.
                //
                NtStatus = NtCreateEvent(
                                &EventHandle,
                                (ACCESS_MASK)EVENT_ALL_ACCESS,
                                NULL,
                                SynchronizationEvent,
                                FALSE
                                );

                if ( NT_SUCCESS( NtStatus ) ) {

                    //
                    //  Add the event to a Notification List
                    //
                    Error = AddEvent(
                                hKey,
                                EventHandle,
                                hEvent,
                                &FullPipeName,
                                pRpcSa,
                                &NotificationList
                                );

                    if ( Error == ERROR_SUCCESS ) {

                        //
                        //  Call the NT API
                        //
                        NtStatus = NtNotifyChangeKey(
                                        hKey,
                                        EventHandle,
                                        NULL,
                                        NULL,
                                        &IoStatusBlock,
                                        dwNotifyFilter,
                                        ( BOOLEAN ) fWatchSubtree,
                                        NULL,
                                        0,
                                        TRUE
                                        );

                        if ( NT_SUCCESS( NtStatus ) ||
                             (NtStatus == STATUS_PENDING) ) {

                            Error = ERROR_SUCCESS;

                        } else {

                            //
                            //  Could not request notification, remove the
                            //  event from the notification list.
                            //
                            Error = RemoveEvent(
                                        EventHandle,
                                        NotificationList
                                        );

                            ASSERT( Error == ERROR_SUCCESS );

                            Error = RtlNtStatusToDosError( NtStatus );
                        }

                    } else {

                        //
                        //  Could not add the event to any notification
                        //  list.
                        //
                        NtStatus = NtClose( EventHandle );
                        ASSERT( NT_SUCCESS( NtStatus ) );
                    }

                } else {

                    Error = RtlNtStatusToDosError( NtStatus );
                }
            }

            RtlFreeHeap(
                RtlProcessHeap( ), 0,
                FullPipeName.Buffer
                );
        }

    } except ( NtStatus = GetExceptionCode() ) {

#if DBG
        DbgPrint( "WINREG Error: Exception %x in BaseRegNotifyChangeKeyValue\n",
                  NtStatus );
        DbgBreakPoint();
#endif
        Error = RtlNtStatusToDosError( NtStatus );

    }

#if BIGDBG
    DbgPrint( "WINREG: RegNotify left\n" );
#endif

    NtStatus = RtlLeaveCriticalSection( &NotificationCriticalSection );
    ASSERT( NT_SUCCESS( NtStatus ) );

    RPC_REVERT_TO_SELF();
    return (error_status_t)Error;
#endif   // REMOTE_NOTIFICATION_DISABLED
}




BOOL
CleanDeadClientInfo(
    HKEY    hKey
    )
/*++

Routine Description:

    When a client dies, this function searches the notification lists to
    see if we the client has some pending notifications. We flag the
    entries in the notification lists and signal the events so that
    the notification handler can get rid of these orphans.

Arguments:

    hKey    -   Client's hKey

Return Value:

    BOOL - Returns TRUE unless something REALLY weird happened.


--*/

{
#ifdef REMOTE_NOTIFICATION_DISABLED

    return( TRUE );

#else // REMOTE_NOTIFICATION_DISABLED

    NTSTATUS            NtStatus;
    PNOTIFICATION_LIST  NotificationList;
    PNOTIFICATION_ENTRY Event;
    DWORD               Index;
    BOOL                Ok               = TRUE;
    BOOL                FoundDeadClients;

    //
    //  Enter the critical section
    //
    NtStatus = RtlEnterCriticalSection( &NotificationCriticalSection );
    if ( !NT_SUCCESS( NtStatus ) ) {
        return FALSE;
    }

#if BIGDBG
    DbgPrint( "WINREG: Dead client, hKey 0x%x\n", hKey );
#endif

    try {

        //
        //  Traverse all the lists looking for orphans.
        //
        for ( NotificationList = NotificationListChainHead;
              NotificationList;
              NotificationList = NotificationList->Next ) {

            FoundDeadClients = FALSE;
            Event = NotificationList->Event;

            //
            //  Examine all the entries of the list to see if any
            //  entry is an orphan.
            //
            for ( Index = 1;
                  Index < NotificationList->EventsInUse;
                  Index++ ) {

                //
                //  If this entry is an orphan, flag it as such and
                //  signal the event so that the notification handler
                //  can clean it up.
                //
                if ( Event->hKey == hKey ) {

#if BIGDBG
                    DbgPrint( "WINREG:  Found notification orphan, hKey 0x%x Client 0x%x\n",
                              hKey, Event->ClientEvent );
#endif
                    Event->Flags |= CLIENT_IS_DEAD;

                    FoundDeadClients = TRUE;
                }

                Event++;
            }

            if ( FoundDeadClients ) {
                NtStatus = NtSetEvent( NotificationList->EventHandle[0], NULL );
                ASSERT( NT_SUCCESS( NtStatus ) );
            }
        }

    } except ( NtStatus = GetExceptionCode() ) {

#if DBG
        DbgPrint( "WINREG Error: Exception %x in CleanDeadClientInfo\n",
                  NtStatus );
        DbgBreakPoint();
#endif

        Ok = FALSE;

    }

#if BIGDBG
    DbgPrint( "WINREG: Dead client left\n" );
#endif

    NtStatus = RtlLeaveCriticalSection( &NotificationCriticalSection );
    ASSERT( NT_SUCCESS( NtStatus ) );

    return Ok;
#endif // REMOTE_NOTIFICATION_DISABLED
}


// *****************************************************************
//
//                  Notification List funcions
//
// *****************************************************************



#ifndef REMOTE_NOTIFICATION_DISABLED

LONG
CreateNotificationList (
    OUT PNOTIFICATION_LIST  *NotificationListUsed
    )
/*++

Routine Description:

    Creates a new Notification List and its handler thread.

Arguments:

    NotificationListUsed    -   Supplies pointer to pointer to Notification List

Return Value:

    LONG -  Returns ERROR_SUCCESS (0); error-code for failure.


--*/

{

    PNOTIFICATION_LIST  NotificationList;
    DWORD               Index;
    NTSTATUS            NtStatus;
    LONG                Error;

#if BIGDBG
    DbgPrint( "WINREG: Creating new notification list\n" );
#endif

    //
    //  Allocate memory for the new Notification List
    //
    NotificationList = RtlAllocateHeap(
                            RtlProcessHeap( ), 0,
                            sizeof( NOTIFICATION_LIST )
                            );

    if ( !NotificationList ) {
        return ERROR_OUTOFMEMORY;
    }


    //
    //  Create the "Wake up" event handle, which is used to wake
    //  up the handler thread whenever new events are added to
    //  the notification list.
    //
    NtStatus = NtCreateEvent(
                    &(NotificationList->EventHandle[0] ),
                    (ACCESS_MASK)EVENT_ALL_ACCESS,
                    NULL,
                    SynchronizationEvent,
                    FALSE
                    );

    if ( NT_SUCCESS( NtStatus ) ) {

        //
        //  Mark rest of entries as "available"
        //
        for ( Index = 1; Index < MAXIMUM_WAIT_OBJECTS; Index++ ) {
            NotificationList->EventHandle[Index] = NULL;
        }

        //
        //  Set initial number of EventInUse to 1 (which is the
        //  index to the next available spot in the list).
        //
        NotificationList->EventsInUse = 1;

        //
        //  Set chain links
        //
        NotificationList->Previous  = NULL;
        NotificationList->Next      = NULL;

        NotificationList->PendingNotifications = 0;

        //
        //  Now that everything has been initialized, create the
        //  handler thread for the list.
        //
        NotificationList->HandlerThread =
                                CreateThread(
                                        NULL,
                                        (32 * 1024),
                                        (LPTHREAD_START_ROUTINE)NotificationHandler,
                                        NotificationList,
                                        0,
                                        (LPDWORD)&(NotificationList->HandlerClientId)
                                        );

        if ( NotificationList->HandlerThread != NULL ) {

            *NotificationListUsed = NotificationList;

            Error = ERROR_SUCCESS;

        } else {

            //
            //  Could not create thread, close the event that we just
            //  created.
            //
            Error = GetLastError();

#if DBG
            DbgPrint( "WINREG Error: Cannot create notification thread, error %d\n",
                      Error );
            DbgBreakPoint();
#endif

            NtStatus = NtClose( NotificationList->EventHandle[0] );

            ASSERT( NT_SUCCESS( NtStatus ) );
        }

    } else {

#if DBG
        DbgPrint( "WINREG Error: Cannot create notification event, status 0x%x\n",
                  NtStatus );
        DbgBreakPoint();
#endif

        Error = RtlNtStatusToDosError( NtStatus );
    }

    //
    //  If something went wrong, free up the notification list
    //
    if ( Error != ERROR_SUCCESS ) {
        RtlFreeHeap(
            RtlProcessHeap( ), 0,
            NotificationList
            );
        *NotificationListUsed = NULL;
    }

    return Error;
}




LONG
DeleteNotificationList (
    IN OUT  PNOTIFICATION_LIST  NotificationList
    )
/*++

Routine Description:

    Deletes a Notification List. The handler thread is not terminated
    because it is the handler thread who deletes notification lists,
    commiting suicide afterwards.

Arguments:

    NotificationList    -   Supplies pointer to Notification List

Return Value:

    LONG -  Returns ERROR_SUCCESS (0); error-code for failure.


--*/

{
    NTSTATUS    NtStatus;

#if BIGDBG
    DbgPrint( "WINREG: Removing empty notification list\n" );
#endif

    //
    //  The only event in the list must be the "wakeup" event
    //
    ASSERT( NotificationList->EventsInUse == 1 );

    //
    //  Delete the "wake up" event
    //
    NtStatus = NtClose( NotificationList->EventHandle[0] );
    ASSERT( NT_SUCCESS( NtStatus ) );

    //
    //  Free up the heap used by the Notification List
    //
    RtlFreeHeap(
         RtlProcessHeap( ), 0,
         NotificationList
         );

    return ERROR_SUCCESS;
}




LONG
AddEvent (
    IN  HKEY                     hKey,
    IN  HANDLE                   EventHandle,
    IN  DWORD                    ClientEvent,
    IN  PUNICODE_STRING          PipeName,
    IN  PRPC_SECURITY_ATTRIBUTES pRpcSa OPTIONAL,
    OUT PNOTIFICATION_LIST      *NotificationListUsed
    )
/*++

Routine Description:

    Adds an event to the first notification list with an available slot.

    If no notification list has an available slot, a new notification list
    (an its handler thread) is created.


Arguments:

    hKey        -   Supplies registry key handle

    EventHandle -   Supplies an event on which the handler thread of the
                    Notification List has to wait.

    ClientEvent -   Supplies the event which has to be communicated to the
                    client when out EventHandle is signaled. This event is
                    communicated to the client via named pipe.

    PipeNameU   -   Supplies the name of the pipe for communicating with the
                    client.

    pRpcSa      -   Supplies the optional security attributes of the named
                    pipe.

    NotificationListused -   Supplies a pointer where the address of the
                             Notification List in which the event is put
                             is placed.

Return Value:

    LONG -  Returns ERROR_SUCCESS (0); error-code for failure.


--*/

{
    PNOTIFICATION_LIST  NotificationList;
    LONG                Error;
    NTSTATUS            NtStatus;

    ASSERT( EventHandle != NULL );
    ASSERT( PipeName && PipeName->Buffer );
    ASSERT( NotificationListUsed );


    //
    //  Get a Notification List with an available entry.
    //
    Error = GetAvailableNotificationList(
                    &NotificationList
                    );

    if ( Error == ERROR_SUCCESS ) {

        //
        //  Add the entry
        //
        Error = AddEntryToNotificationList(
                        NotificationList,
                        hKey,
                        EventHandle,
                        ClientEvent,
                        PipeName,
                        pRpcSa
                        );

        if ( Error == ERROR_SUCCESS ) {

            //
            //  A new entry has been added, we have to wake up the
            //  handler thread so that it will wait on the newly added
            //  event.
            //
            NtStatus = NtSetEvent( NotificationList->EventHandle[0], NULL );
            ASSERT( NT_SUCCESS( NtStatus ) );

            *NotificationListUsed = NotificationList;

        } else {

#if DBG
            DbgPrint( "WINREG: Could not add notification entry! Error %d\n ", Error);
#endif
        }

    } else {

#if DBG
        DbgPrint( "WINREG: Could not get a notification list! Error %d\n ", Error);
#endif

    }

    return Error;
}




LONG
RemoveEvent (
    IN      HANDLE              EventHandle,
    IN OUT  PNOTIFICATION_LIST  NotificationList
    )

/*++

Routine Description:

    Removes an event from the notification list. The caller must
    make sure that the event handle given does live in the
    Notification List specified.

    This function is called if the notification is aborted for some
    reason (e.g. the NT notification API fails).

Arguments:

    EventHandle         -   Supplies the event to remove.

    NotificationList    -   Supplies the Notification List in which
                            the event lives.

Return Value:

    LONG -  Returns ERROR_SUCCESS (0); error-code for failure.


--*/

{
    LONG        Error;
    DWORD       EntryIndex;

    //
    //  Search for the entry that we have to remove.
    //
    for ( EntryIndex = 1;
          EntryIndex < NotificationList->EventsInUse;
          EntryIndex++ ) {

        if ( EventHandle == NotificationList->EventHandle[ EntryIndex ] ) {
            break;
        }
    }

    ASSERT( EntryIndex < NotificationList->EventsInUse );

    if ( EntryIndex < NotificationList->EventsInUse ) {

        //
        //  Found entry, remove it
        //
        Error = RemoveEntryFromNotificationList(
                    NotificationList,
                    EntryIndex
                    );

        //
        //  Note that we are leaving a hole in the Notification list,
        //  the handler will eventually compact it.
        //

    } else {

        Error = ERROR_ARENA_TRASHED;

    }

    return Error;
}




LONG
GetAvailableNotificationList (
    OUT PNOTIFICATION_LIST  *NotificationListUsed
    )
/*++

Routine Description:

    Gets a Notification List with an available entry.

Arguments:

    NotificationList    -   Supplies pointer to where the Notification
                            List pointer will be placed.

Return Value:

    LONG -  Returns ERROR_SUCCESS (0); error-code for failure.


--*/
{
    LONG                Error = ERROR_SUCCESS;
    PNOTIFICATION_LIST  NotificationList;

    //
    //  Traverse the chain of Notification lists until we find a Notification
    //  list with an available entry.
    //
    for ( NotificationList = NotificationListChainHead;
          NotificationList && NotificationList->EventsInUse >= MAXIMUM_WAIT_OBJECTS;
          NotificationList = NotificationList->Next );


    //
    //  If we did not find a Notification List with an available spot,
    //  create a new Notification List and add it to the chain.
    //
    if ( !NotificationList ) {

        Error = CreateNotificationList( &NotificationList );

        if ( Error == ERROR_SUCCESS ) {

            ASSERT( NotificationList );

            AddNotificationListToChain( NotificationList );
        }
    }

    *NotificationListUsed = NotificationList;

    return Error;
}




LONG
AddEntryToNotificationList(
        IN OUT  PNOTIFICATION_LIST       NotificationList,
        IN      HKEY                     hKey,
        IN      HANDLE                   EventHandle,
        IN      DWORD                    ClientEvent,
        IN      PUNICODE_STRING          PipeName,
        IN      PRPC_SECURITY_ATTRIBUTES pRpcSa     OPTIONAL
        )
/*++

Routine Description:

    Adds an entry to a notification list.

    Calls to this function must be protected by the critical
    section of the Notification List.


Arguments:

    NotificationList    -   Supplies pointer to Notification List

    hKey                -   Supplies registry key handle

    EventHandle         -   Supplies the event handle

    ClientEvent         -   Supplies the client's event

    PipeName            -   Supplies name of pipe.

    pRpcSa              -   Supplies security attributes for the pipe

Return Value:

    LONG -  Returns ERROR_SUCCESS (0); error-code for failure.


--*/
{
    LONG                Error;
    PPIPE_ENTRY         PipeEntry;
    DWORD               Index;
    PNOTIFICATION_ENTRY Event;


    //
    //  Add the pipe information to the pipe symbol table
    //
    Error = AddPipe( PipeName, pRpcSa, &PipeEntry );

    if ( Error == ERROR_SUCCESS ) {

        //
        //  Add the event in the next available spot in the list,
        //  and increment the number of events in use by the
        //  list.
        //
        Index = NotificationList->EventsInUse++;

        Event = &(NotificationList->Event[ Index ]);

        NotificationList->EventHandle[ Index ] = EventHandle;

        Event->ClientEvent = ClientEvent;
        Event->hKey        = hKey;
        Event->Flags       = 0;
        Event->PipeEntry   = PipeEntry;

    } else {

#if BIGDBG
        DbgPrint( "WINREG: Could not create pipe entry for %wZ\n",
                  PipeName );
#endif

    }

    return Error;
}




LONG
RemoveEntryFromNotificationList (
    IN OUT  PNOTIFICATION_LIST  NotificationList,
    IN      DWORD               EntryIndex
    )
/*++

Routine Description:


    Removes an entry from a Notification List. It leaves a hole
    in the list, i.e. the list is not compacted.

Arguments:

    NotificationList    -   Supplies pointer to Notification List.

    EntryIndex          -   Supplies index of entry to remove.

Return Value:

    LONG -  Returns ERROR_SUCCESS (0); error-code for failure.


--*/

{
    LONG    Error;

    ASSERT( EntryIndex < NotificationList->EventsInUse );
    ASSERT( NotificationList->EventHandle[ EntryIndex ] != NULL );

    //
    //  Remove the entry from the pipe symbol table.
    //
    Error = RemovePipe( NotificationList->Event[ EntryIndex ].PipeEntry );

    if ( Error == ERROR_SUCCESS ) {

        //
        //  We "remove" the entry from the notification list by
        //  invalidating its handle. Note that we don't decrement
        //  the counter of entries in the notification list because
        //  that is used for indexing the next available entry.
        //  The counter will be fixed by the compaction function.
        //
        NotificationList->EventHandle[ EntryIndex ]     = NULL;
        NotificationList->Event[ EntryIndex ].PipeEntry = NULL;
    }

    return Error;
}




LONG
CompactNotificationList (
    IN OUT  PNOTIFICATION_LIST  NotificationList
    )
/*++

Routine Description:


    Compacts (i.e. removes holes from) a Notification List.

Arguments:

    NotificationList    -   Supplies pointer to Notification List.

Return Value:

    LONG -  Returns ERROR_SUCCESS (0); error-code for failure.


--*/

{
    DWORD   ToIndex;
    DWORD   FromIndex;
    DWORD   Index;
    DWORD   EntriesToMove;
    PVOID   Src;
    PVOID   Dst;

#if BIGDBG
    DbgPrint( "    * Compacting notification list\n" );
#endif

    for ( ToIndex = 1; ToIndex < NotificationList->EventsInUse; ToIndex++ ) {

#if BIGDBG
        DbgPrint( "        - %d\n", ToIndex );
#endif
        //
        //  If we find a hole, we compact the arrays i.e. shift them to
        //  remove the hole.
        //
        if ( NotificationList->EventHandle[ ToIndex ] == NULL ) {

            //
            //  Found the beginning of a hole, search for the next
            //  entry in use.
            //
            for ( FromIndex = ToIndex+1;
                  (FromIndex < NotificationList->EventsInUse) &&
                  (NotificationList->EventHandle[ FromIndex ] == NULL );
                  FromIndex++ ) {
            }

            //
            //  If there is something to shift, shift it
            //
            if ( FromIndex < NotificationList->EventsInUse ) {

                EntriesToMove = NotificationList->EventsInUse - FromIndex;

                Src = (PVOID)&(NotificationList->EventHandle[ FromIndex ] );
                Dst = (PVOID)&(NotificationList->EventHandle[ ToIndex ] );

                RtlMoveMemory(
                         Dst,
                         Src,
                         EntriesToMove * sizeof( HANDLE )
                         );

                Src = &(NotificationList->Event[ FromIndex ] );
                Dst = &(NotificationList->Event[ ToIndex ] );

                RtlMoveMemory(
                         Dst,
                         Src,
                         EntriesToMove * sizeof( NOTIFICATION_ENTRY )
                         );

                //
                //  Clear the rest of the entries, just to keep things
                //  clean.
                //
                for ( Index = ToIndex + EntriesToMove;
                      Index < NotificationList->EventsInUse;
                      Index++ ) {

                    NotificationList->EventHandle[ Index ] = NULL;
                }

                NotificationList->EventsInUse -= (FromIndex - ToIndex);


            } else {

                //
                //  Nothing to shift, this will become the
                //  first available entry of the list.
                //
                NotificationList->EventsInUse = ToIndex;
            }
        }
    }

#if BIGDBG
    DbgPrint( "    * Compacted.\n" );
#endif


    return ERROR_SUCCESS;
}





VOID
AddNotificationListToChain(
    IN OUT  PNOTIFICATION_LIST  NotificationList
    )

/*++

Routine Description:

    Adds a Notification list to the chain of Notification Lists.

    The new list is put at the head of the chain.

Arguments:

    NotificationList    -   Supplies the Notification list to add


Return Value:

    LONG -  Returns ERROR_SUCCESS (0); error-code for failure.


--*/

{

    NotificationList->Previous = NULL;
    NotificationList->Next     = NotificationListChainHead;

    if ( NotificationListChainHead ) {
        NotificationListChainHead->Previous = NotificationList;
    }

    NotificationListChainHead = NotificationList;
}




VOID
RemoveNotificationListFromChain(
    IN OUT  PNOTIFICATION_LIST  NotificationList
    )
/*++

Routine Description:

    Removes a Notification list from the chain


Arguments:

    NotificationList    -   Supplies the Notification list to remove


Return Value:

    LONG -  Returns ERROR_SUCCESS (0); error-code for failure.


--*/

{

    if ( NotificationList->Previous ) {
        (NotificationList->Previous)->Next = NotificationList->Next;
    }

    if ( NotificationList->Next ) {
        (NotificationList->Next)->Previous = NotificationList->Previous;
    }


    //
    //  If this is at the head of the chain, Let the next
    //  list be the new head.
    //
    if ( NotificationListChainHead == NotificationList ) {
        NotificationListChainHead = NotificationList->Next;
    }
}



// *****************************************************************
//
//                  Pipe Symbol Table functions
//
// *****************************************************************


LONG
GetFullPipeName (
    IN  PUNICODE_STRING          MachineName,
    IN  PUNICODE_STRING          PipeName,
    OUT PUNICODE_STRING          FullPipeName
    )
/*++

Routine Description:

    Makes a fully qualified pipe name from the supplied machine
    name and pipe name.

Arguments:

    PipeName        -   Supplies the pipe name

    MachineName     -   Supplies the client's machine name

    FullPipeName    -   Supplies the full pipe name

Return Value:

    LONG -  Returns ERROR_SUCCESS (0); error-code for failure.


--*/

{
    LONG            Error = ERROR_SUCCESS;
    NTSTATUS        NtStatus;

    ASSERT( PipeName->Buffer     && MachineName->Buffer &&
            PipeName->Length > 0 && MachineName->Length > 0 );

    if( !PipeName->Buffer     || !MachineName->Buffer ||
        PipeName->Length == 0 || MachineName->Length == 0 ) {
        Error =  ERROR_INVALID_PARAMETER;
    }

    if ( Error == ERROR_SUCCESS ) {

        //
        //  If the client's machine name and our machine name match,
        //  then we form a local named pipe path, otherwise we
        //  form a remote named pipe path.
        //
        if ( RtlEqualUnicodeString(
                        MachineName,
                        &OurMachineName,
                        TRUE
                        ) ) {


            //
            //  Pipe is local
            //
            RtlMoveMemory(
                    FullPipeName->Buffer,
                    NAMED_PIPE_HERE,
                    sizeof( NAMED_PIPE_HERE )
                    );

            FullPipeName->Length = sizeof( NAMED_PIPE_HERE ) - sizeof(UNICODE_NULL);


        } else {

            //
            //  Pipe is remote
            //
            RtlMoveMemory(
                    FullPipeName->Buffer,
                    NAMED_PIPE_THERE,
                    sizeof( NAMED_PIPE_THERE )
                    );

            FullPipeName->Length = sizeof( NAMED_PIPE_THERE ) - sizeof(UNICODE_NULL);

            NtStatus = RtlAppendUnicodeStringToString(
                                FullPipeName,
                                MachineName
                                );

            ASSERT( NT_SUCCESS( NtStatus ) );

            if ( NT_SUCCESS( NtStatus ) ) {

                NtStatus = RtlAppendUnicodeToString(
                                    FullPipeName,
                                    L"\\Pipe\\"
                                    );

                ASSERT( NT_SUCCESS( NtStatus ) );

                if ( !NT_SUCCESS( NtStatus ) ) {
                    Error = RtlNtStatusToDosError( NtStatus );
                }

            } else {

                Error = RtlNtStatusToDosError( NtStatus );
            }
        }

        if ( Error == ERROR_SUCCESS ) {

            NtStatus = RtlAppendUnicodeStringToString(
                                FullPipeName,
                                PipeName
                                );

            ASSERT( NT_SUCCESS( NtStatus ) );

            if ( !NT_SUCCESS( NtStatus ) ) {
                Error = RtlNtStatusToDosError( NtStatus );
            }
        }
    }

    return Error;
}






LONG
CreatePipeEntry (
    IN  PUNICODE_STRING          PipeName,
    IN  PRPC_SECURITY_ATTRIBUTES pRpcSa     OPTIONAL,
    OUT PPIPE_ENTRY              *PipeEntryUsed
    )
/*++

Routine Description:

    Creates a pipe entry

Arguments:

    PipeName    -   Supplies the pipe name

    pRpcSa      -   Supplies the optional security attributes for the pipe

    PipeEntry   -   Supplies pointer to pointer to pipe entry.

Return Value:

    LONG -  Returns ERROR_SUCCESS (0); error-code for failure.


--*/

{

    PPIPE_ENTRY PipeEntry;
    LONG        Error;
    ULONG       LengthSd;

    ASSERT( PipeName && PipeName->Buffer );

    //
    //  Validate the security descriptor if one was provided
    //
    if ( pRpcSa ) {
        if ( !RtlValidSecurityDescriptor(
                pRpcSa->RpcSecurityDescriptor.lpSecurityDescriptor
                ) ) {

            return ERROR_INVALID_PARAMETER;
        }
    }


    //
    //  Allocate space for the Pipe Entry
    //
    PipeEntry = RtlAllocateHeap(
                    RtlProcessHeap( ), 0,
                    sizeof( PIPE_ENTRY )
                    );

    if ( !PipeEntry ) {
        return ERROR_OUTOFMEMORY;
    }


    //
    //  Allocate space for the pipe's name
    //
    PipeEntry->PipeName.Buffer = RtlAllocateHeap(
                                    RtlProcessHeap( ), 0,
                                    PipeName->Length + sizeof( UNICODE_NULL )
                                    );

    PipeEntry->PipeName.MaximumLength = PipeName->Length + (USHORT)sizeof( UNICODE_NULL );

    if ( PipeEntry->PipeName.Buffer ) {

        //
        //  Copy the pipe name
        //
        RtlCopyUnicodeString(
                &(PipeEntry->PipeName),
                PipeName
                );

        PipeEntry->Previous = NULL;
        PipeEntry->Next     = NULL;

        PipeEntry->ReferenceCount = 0;

        //
        //  Allocate space for the security descriptor if one
        //  is provided.
        //
        if ( pRpcSa ) {

            LengthSd = RtlLengthSecurityDescriptor(
                            pRpcSa->RpcSecurityDescriptor.lpSecurityDescriptor
                            );

            PipeEntry->SecurityDescriptor = RtlAllocateHeap(
                                                RtlProcessHeap( ), 0,
                                                LengthSd
                                                );


            if ( PipeEntry->SecurityDescriptor ) {

                //
                //  Copy the security descriptor
                //
                RtlMoveMemory (
                        PipeEntry->SecurityDescriptor,
                        pRpcSa->RpcSecurityDescriptor.lpSecurityDescriptor,
                        LengthSd
                        );

                *PipeEntryUsed = PipeEntry;

                return ERROR_SUCCESS;

            } else {

                Error = ERROR_OUTOFMEMORY;
            }

            RtlFreeHeap(
                 RtlProcessHeap( ), 0,
                 PipeEntry->PipeName.Buffer
                 );

        } else {

            PipeEntry->SecurityDescriptor = NULL;

            *PipeEntryUsed = PipeEntry;

            return ERROR_SUCCESS;
        }

    } else {

        Error = ERROR_OUTOFMEMORY;
    }


    RtlFreeHeap(
         RtlProcessHeap( ), 0,
         PipeEntry
         );

    return Error;
}



LONG
DeletePipeEntry(
    IN OUT PPIPE_ENTRY  PipeEntry
    )

/*++

Routine Description:


    Deletes a pipe entry

Arguments:

    PipeEntry   -   Supplies pointer to pipe entry

Return Value:

    LONG -  Returns ERROR_SUCCESS (0); error-code for failure.


--*/

{

#if BIGDBG
    DbgPrint( "    * In DeletePipeEntry\n" );
#endif

    ASSERT( PipeEntry );
    ASSERT( PipeEntry->PipeName.Buffer );

    if ( PipeEntry->PipeName.Buffer ) {
        RtlFreeHeap(
             RtlProcessHeap( ), 0,
            PipeEntry->PipeName.Buffer
            );
    }

    if ( PipeEntry->SecurityDescriptor != NULL ) {
        RtlFreeHeap(
             RtlProcessHeap( ), 0,
             PipeEntry->SecurityDescriptor
             );
    }

    RtlFreeHeap(
         RtlProcessHeap( ), 0,
         PipeEntry
         );


#if BIGDBG
    DbgPrint( "    * Deleted PipeEntry.\n" );
#endif

    return ERROR_SUCCESS;
}




LONG
AddPipe(
    IN  PUNICODE_STRING          PipeName,
    IN  PRPC_SECURITY_ATTRIBUTES pRpcSa     OPTIONAL,
    OUT PPIPE_ENTRY              *PipeEntryUsed
    )

/*++

Routine Description:

    Adds a new entry to the pipe symbol table

Arguments:

    PipeName    -   Supplies the pipe name

    pRpcSa      -   Supplies the optional security attributes for the pipe

    PipeEntry   -   Supplies pointer to pointer to pipe entry in the
                    symbol table.

Return Value:

    LONG -  Returns ERROR_SUCCESS (0); error-code for failure.


--*/

{
    PPIPE_ENTRY PipeEntry;
    LONG        Error;


    //
    //  Look for the pipe name in the symbol table
    //
    Error  = LookForPipeEntryInSymbolTable( PipeName, &PipeEntry );

    if ( Error == ERROR_SUCCESS ) {

        //
        //  If the pipe is not in the symbol table, add it
        //
        if ( !PipeEntry ) {

            //
            //  Create a new pipe entry
            //
            Error = CreatePipeEntry(
                        PipeName,
                        pRpcSa,
                        &PipeEntry
                        );

            if ( Error == ERROR_SUCCESS ) {

                //
                //  Add the entry to the symbol table
                //
                Error = AddPipeEntryToSymbolTable(
                            PipeEntry
                            );

                if ( Error != ERROR_SUCCESS ) {

                    //
                    //  Could not add pipe entry, delete it.
                    //
                    DeletePipeEntry( PipeEntry );
                    PipeEntry = NULL;
                }
            }
        }

        //
        //  If got a pipe entry, increment its reference count
        //
        if ( PipeEntry ) {

            PipeEntry->ReferenceCount++;
            *PipeEntryUsed = PipeEntry;
        }
    }

#if BIGDBG
        DbgPrint( "Added Pipe %Z:\n", PipeName );
        DumpPipeTable();
#endif

    return Error;
}




LONG
RemovePipe(
    IN OUT PPIPE_ENTRY  PipeEntry
    )

/*++

Routine Description:

    Decrements the reference count of a pipe entry and removes the
    entry if the reference count reaches zero.

Arguments:

    PipeEntry   -   Supplies pointer to pipe entry in the symbol table

Return Value:

    LONG -  Returns ERROR_SUCCESS (0); error-code for failure.


--*/

{

    LONG        Error = ERROR_SUCCESS;
    PPIPE_ENTRY Entry = PipeEntry;

    ASSERT( Entry );
    ASSERT( Entry->ReferenceCount > 0 );

#if BIGDBG
    DbgPrint( "    * In RemovePipe - Ref. count %d\n", Entry->ReferenceCount );
#endif

    //
    //  Decrement the reference count
    //
    Entry->ReferenceCount--;

    //
    //  If the reference count is zero, we can delete the
    //  entry
    //
    if ( Entry->ReferenceCount == 0 ) {

        //
        //  Remove the pipe entry from the symbol table
        //
        Error = RemovePipeEntryFromSymbolTable(
                    Entry
                    );

        if ( Error == ERROR_SUCCESS ) {

            //
            //  Delete the pipe entry
            //
            ASSERT( PipeEntry > (PPIPE_ENTRY)0x100 );
            Error = DeletePipeEntry( Entry );
        }
    }

#if BIGDBG
    DbgPrint( "    * Pipe Removed.\n" );
#endif

    return Error;
}



LONG
AddPipeEntryToSymbolTable(
    IN OUT  PPIPE_ENTRY PipeEntry
    )

/*++

Routine Description:


    Adds a pipe entry to the symbol table at the specified bucket.

    Entries are always added at the head of the chain.

    Calls to this function must be protected by the critical section
    of the pipe symbol table.

Arguments:

    PipeEntry   -   Supplies pointer to pipe entry

Return Value:

    LONG -  Returns ERROR_SUCCESS (0); error-code for failure.


--*/

{
    DWORD   Bucket;

    Bucket = HASH( &(PipeEntry->PipeName), BUCKETS_IN_SYMBOL_TABLE );

    PipeEntry->Previous = NULL;
    PipeEntry->Next     = PipeSymbolTable.Bucket[ Bucket ];

    if ( PipeSymbolTable.Bucket[ Bucket ] ) {
        (PipeSymbolTable.Bucket[ Bucket ])->Previous = PipeEntry;
    }

    PipeSymbolTable.Bucket[ Bucket ] = PipeEntry;

    return ERROR_SUCCESS;
}




LONG
RemovePipeEntryFromSymbolTable(
    IN OUT  PPIPE_ENTRY PipeEntry
    )

/*++

Routine Description:


    Removes a pipe entry from the symbol table at the specified bucket

    Calls to this function must be protected by the critical section
    of the pipe symbol table.

Arguments:

    PipeEntry   -   Supplies pointer to pipe entry

Return Value:

    LONG -  Returns ERROR_SUCCESS (0); error-code for failure.


--*/

{
    DWORD   Bucket;

#if BIGDBG
    DbgPrint( "    * In RemovePipeEntryFromSymbolTable\n" );
#endif

    ASSERT( PipeEntry > (PPIPE_ENTRY)0x100 );

    Bucket = HASH( &(PipeEntry->PipeName), BUCKETS_IN_SYMBOL_TABLE );

    ASSERT( PipeEntry > (PPIPE_ENTRY)0x100 );
    ASSERT( Bucket < BUCKETS_IN_SYMBOL_TABLE );

    //
    //  Remove the entry from the chain
    //
    if ( PipeEntry->Previous ) {
        (PipeEntry->Previous)->Next = PipeEntry->Next;
    }

    if ( PipeEntry->Next ) {
        (PipeEntry->Next)->Previous = PipeEntry->Previous;
    }


    //
    //  If this entry is at the head of the chain, Let the next
    //  entry be the new head.
    //
    ASSERT( PipeSymbolTable.Bucket[ Bucket ] != NULL );
    if ( PipeSymbolTable.Bucket[ Bucket ] == PipeEntry ) {
        PipeSymbolTable.Bucket[ Bucket ] = PipeEntry->Next;
    }

    PipeEntry->Next     = NULL;
    PipeEntry->Previous = NULL;

    ASSERT( PipeEntry > (PPIPE_ENTRY)0x100 );

#if BIGDBG
    DbgPrint( "    * Piped entry removed from symbol table.\n" );
#endif

    return ERROR_SUCCESS;
}




LONG
LookForPipeEntryInSymbolTable(
    IN  PUNICODE_STRING PipeName,
    OUT PPIPE_ENTRY     *PipeEntryUsed
    )
/*++

Routine Description:

    Looks for an entry corresponding to the given name in a particular
    bucket of the pipe symbol table.

    Note that this function always returns ERROR_SUCCESS. To find out
    if the pipe is in the chain or not the returned parameter has to
    be checked.

    Calls to this function must be protected by the critical section
    of the pipe symbol table.

Arguments:

    PipeName    -   Supplies the pipe name

    Bucket      -   Supplies the bucket

    PipeEntry   -   Supplies pointer to pointer to pipe entry.

Return Value:

    LONG -  Returns ERROR_SUCCESS (0); error-code for failure.


--*/
{
    PPIPE_ENTRY  PipeEntry;
    DWORD        Bucket;

    Bucket = HASH( PipeName, BUCKETS_IN_SYMBOL_TABLE );

    //
    //  Look for the entry
    //
    for ( PipeEntry = PipeSymbolTable.Bucket[ Bucket ];
          PipeEntry && !RtlEqualUnicodeString( PipeName, &(PipeEntry->PipeName), TRUE);
          PipeEntry = PipeEntry->Next );


    *PipeEntryUsed = PipeEntry;

    return ERROR_SUCCESS;
}




DWORD
Hash(
    IN  PUNICODE_STRING  Symbol,
    IN  DWORD            Buckets
    )
/*++

Routine Description:


    Obtains a hash value for a given symbol

Arguments:


    Symbol      -   Supplies the symbol to hash

    Buckets     -   Supplies the number of buckets in the sybol table.

Return Value:

    LONG -  Returns ERROR_SUCCESS (0); error-code for failure.


--*/
{
    DWORD   n;
    DWORD   HashValue;
    WCHAR   c;
    LPWSTR  s;

#if BIGDBG
    DbgPrint( "    * Hashing\n" );
#endif

    n         = Symbol->Length/sizeof(WCHAR);
    s         = Symbol->Buffer;
    HashValue = 0;

    while (n--) {

        c = *s++;

        HashValue = HashValue + (c << 1) + (c >> 1) + c;
    }

    return HashValue % Buckets;

}







// *****************************************************************
//
//                  Notification List Handler
//
// *****************************************************************



VOID
NotificationHandler(
    IN  PNOTIFICATION_LIST  NotificationList
    )

/*++

Routine Description:

    Handler of a Notification List.

Arguments:


    NotificationList    -   Supplies pointer to the Notification List
                            to handle.

Return Value:

    None

--*/

{

    NTSTATUS        NtStatus;
    DWORD           NumberOfEvents;
    HANDLE          Thread;
    BOOLEAN         KeepOnGoing = TRUE;
    DWORD           Index;
    LARGE_INTEGER   TimeOut;


    ASSERT( NotificationList );

    //
    //  Initially we'll wait on only one event, i.e. the
    //  "wake up" event
    //
    NumberOfEvents = 1;
    NotificationList->TimeOutCount = 0;
    NotificationList->ResetCount = FALSE;

    while ( KeepOnGoing ) {

        TimeOut.QuadPart = Int32x32To64( -10000,
                                         5000*NotificationList->TimeOutCount );

        //
        //  Wait for some event
        //
        NtStatus = NtWaitForMultipleObjects(
                        (CHAR)NumberOfEvents,
                        NotificationList->EventHandle,
                        WaitAny,
                        FALSE,
                        (NotificationList->PendingNotifications > 0) ?
                            &TimeOut : NULL
                        );

        Index = (DWORD)NtStatus;

        if ( (Index < 0) || (Index >= NumberOfEvents) ) {
            Index = 0;
        }

        ASSERT( Index < NumberOfEvents );

        NtStatus = RtlEnterCriticalSection( &NotificationCriticalSection );
        ASSERT( NT_SUCCESS( NtStatus ) );

#if BIGDBG
        DbgPrint( "WINREG: Notification handler signaled, Index %d\n", Index );
#endif

        try {

            //
            //  If an event was triggered, mark it as a pending notification so
            //  that the NotificationListMaintenance function will notify
            //  the client.
            //
            if ( Index > 0 ) {
                NotificationList->PendingNotifications++;
                NotificationList->Event[Index].Flags |= MUST_NOTIFY;
            }

            //
            //  Notify all the clients with pending notifications and
            //  remove entries for dead clients.
            //
            NumberOfEvents = NotificationListMaintenance( NotificationList );

            if( NotificationList->PendingNotifications != 0 ) {
                if( NotificationList->ResetCount ) {
                    NotificationList->TimeOutCount = 1;
                    NotificationList->ResetCount = FALSE;
                } else {
                    if( NotificationList->TimeOutCount == 0 ) {
                        NotificationList->TimeOutCount = 1;
                    } else {
                        if( NotificationList->TimeOutCount != MAX_TIMEOUT_COUNT ) {
                            NotificationList->TimeOutCount =
                                          NotificationList->TimeOutCount << 1;
                        }
                    }
                }
            } else {
                NotificationList->TimeOutCount = 0;
            }

            //
            //  If the list is empty, then try to take it out of the chain, and
            //  if successful, our job is done.
            //
            if ( NumberOfEvents == 1 ) {

#if BIGDBG
                DbgPrint( "    * Removing the notification list!\n" );
#endif
                //
                //  Make sure that the list is empty.
                //
                ASSERT( NotificationList->EventsInUse == 1 );
                if (NotificationList->EventsInUse == 1) {

                    //
                    //  The list is empty, remove the list from the chain
                    //  and delete it.
                    //
                    RemoveNotificationListFromChain( NotificationList );
                    Thread = NotificationList->HandlerThread;
                    DeleteNotificationList( NotificationList );

                    //
                    //  The list is gone, we can die.
                    //
                    KeepOnGoing = FALSE;
                }
            }

        } except ( NtStatus = GetExceptionCode() ) {

#if DBG
            DbgPrint( "WINREG Error: Exception %x in NotificationHandler\n",
                      NtStatus );
            DbgBreakPoint();
#endif

        }


#if BIGDBG
        if ( KeepOnGoing ) {
            DbgPrint( "WINREG: Notification handler waiting...\n" );
        } else {
            DbgPrint( "WINREG: Notification handler dying...\n" );
        }
#endif

        NtStatus = RtlLeaveCriticalSection( &NotificationCriticalSection );
        ASSERT( NT_SUCCESS( NtStatus ) );

    }

    //
    //  The list is gone, and so must we.
    //
    ExitThread( 0 );

    ASSERT( FALSE );

}



DWORD
NotificationListMaintenance(
    IN OUT  PNOTIFICATION_LIST  NotificationList
    )
/*++

Routine Description:

    Performs all the maintenance necessary in the notification list.
    The maintenance consists of:

    - Notifying all clients with pending notifications.

    - Removing entries in the list for dead clients.

    - Compacting the notification list.

Arguments:


    NotificationList    -   Supplies pointer to the Notification List

Return Value:

    DWORD   -   The new number of events in the list

--*/

{

    LONG                Error;
    DWORD               NumberOfEvents;
    DWORD               Index;
    BOOLEAN             Remove;
    PNOTIFICATION_ENTRY Event;
    NTSTATUS            NtStatus;
    PPIPE_ENTRY         PipeEntry;

#if BIGDBG
    DbgPrint( "    * In NotificationListMaintenance\n" );
    DumpNotificationLists();
#endif

    //
    //  Traverse the list notifying clients if necessary and removing
    //  events that are no longer needed, either because they have
    //  already been notified or because the client is dead.
    //
    for (Index = 1; Index < NotificationList->EventsInUse; Index++ ) {

#if BIGDBG
        DbgPrint( "      - %d\n", Index );
#endif
        Remove = FALSE;
        Event  = &(NotificationList->Event[ Index ]);

        if ( Event->Flags & CLIENT_IS_DEAD ) {

            //
            //  No client, must remove the entry.
            //
            Remove = TRUE;

        } else if ( Event->Flags & MUST_NOTIFY ) {

            //
            //  Must notify this client
            //
            Error = SendEventToClient(
                        Event->ClientEvent,
                        Event->PipeEntry
                        );

            if (Error == ERROR_SUCCESS) {
                //
                //  If successfully notified, remove the entry.
                //
                Remove = TRUE;
                Event->Flags &= ~NOTIFICATION_FAILED;
            } else {
                //
                //  If couldn't notify, set ResetCount if the notification
                //  failed for the first time
                //
                if( ( Event->Flags & NOTIFICATION_FAILED ) == 0 ) {
                    NotificationList->ResetCount = TRUE;
                    Event->Flags |= NOTIFICATION_FAILED;
                }
            }
        }

        //
        //  Remove the entry if no longer needed.
        //
        if ( Remove ) {

            //
            //  Remove the pipe entry
            //
            PipeEntry = Event->PipeEntry;
            RemovePipe( PipeEntry );

            Event->PipeEntry = NULL;

            //
            //  Remove the event
            //
#if BIGDBG
            DbgPrint( "        Cleanup\n" );
#endif

            NtStatus = NtClose( NotificationList->EventHandle[ Index ] );
            ASSERT( NT_SUCCESS( NtStatus ) );
            NotificationList->EventHandle[ Index ] = NULL;

            //
            //  If this was a pending notification, decrement the
            //  counter.
            //
            if ( Event->Flags & MUST_NOTIFY ) {
                NotificationList->PendingNotifications--;
            }
        }
    }


    //
    //  Compact the list.
    //
    Error = CompactNotificationList( NotificationList );

    ASSERT( Error == ERROR_SUCCESS );

    //
    //  Get the new number of entries in the list
    //
    NumberOfEvents = NotificationList->EventsInUse;

#if BIGDBG
    DbgPrint( "    * Maintenance Done (%d)\n", NumberOfEvents );
#endif

    return NumberOfEvents;
}





LONG
SendEventToClient(
    IN  DWORD           ClientEvent,
    IN  PPIPE_ENTRY     PipeEntry
    )
/*++

Routine Description:

    Sends an event to the client via the client's named pipe

Arguments:


    PipeEntry       -   Supplies the pipe entry for the client's named
                        pipe.

    ClientEvent     -   Supplies the event that has to be sent to the
                        client.

Return Value:

    LONG -  Returns ERROR_SUCCESS (0); error-code for failure.

--*/

{

    HANDLE              Handle;
    LONG                Error = ERROR_SUCCESS;
    OBJECT_ATTRIBUTES   Obja;
    NTSTATUS            NtStatus;

    ASSERT( PipeEntry != NULL );
    ASSERT( PipeEntry->PipeName.Buffer != NULL );

    //
    // Initialize the Obja structure for the named pipe
    //
    InitializeObjectAttributes(
        &Obja,
        &(PipeEntry->PipeName),
        OBJ_CASE_INSENSITIVE,
        NULL,
        PipeEntry->SecurityDescriptor
        );


    //
    // Open our side of the pipe
    //
    NtStatus = NtOpenFile(
                    &Handle,
                    GENERIC_WRITE | SYNCHRONIZE,
                    &Obja,
                    &IoStatusBlock,
                    FILE_SHARE_WRITE,
                    FILE_NON_DIRECTORY_FILE
                    );


    if ( NT_SUCCESS( NtStatus ) ) {

        //
        //  Write the event
        //
        NtStatus = NtWriteFile(
                        Handle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        &ClientEvent,
                        sizeof(ClientEvent),
                        NULL,
                        NULL
                        );

        if ( NtStatus == STATUS_PENDING ) {
            NtStatus = NtWaitForSingleObject(
                            Handle,
                            FALSE,
                            NULL );
        }

        if ( NT_SUCCESS( NtStatus ) ) {
#if BIGDBG
            DbgPrint( "    --> Client Notified, Event 0x%x\n", ClientEvent );
#endif
            Error = ERROR_SUCCESS;
        } else {
            Error = RtlNtStatusToDosError( NtStatus );
        }


        //
        //  Close our side of the pipe
        //
        NtStatus = NtClose( Handle );
        ASSERT( NT_SUCCESS( NtStatus ) );

    } else {

        //
        //  If we couldn't open the pipe because the pipe does
        //  not exist, there's no point in keep trying.
        //
        if ( NtStatus == STATUS_OBJECT_NAME_NOT_FOUND ) {
            Error = ERROR_SUCCESS;
        } else {
            Error = RtlNtStatusToDosError( NtStatus );
#if DBG
            DbgPrint( "WINREG: Cannot Open pipe %Z, event %x, status %x\n",
                  &(PipeEntry->PipeName), ClientEvent, NtStatus );
#endif
        }
    }

#if DBG
    if (Error != ERROR_SUCCESS ) {
        DbgPrint( "WINREG: Could not notify client, Error %d\n", Error );
    }
#endif

    return Error;
}


#if BIGDBG

// *****************************************************************
//
//                      Debug Stuff
//
// *****************************************************************


VOID
DumpNotificationLists(
    )
/*++

Routine Description:

    Dumps the notification lists

Arguments:


    None

Return Value:

    None

--*/

{
    PNOTIFICATION_LIST  NotificationList;
    PNOTIFICATION_ENTRY Event;
    DWORD               Index;

    DbgPrint( "        Notification list dump: \n\n" );

    for ( NotificationList = NotificationListChainHead;
          NotificationList;
          NotificationList = NotificationList->Next ) {

        DbgPrint( "        Notification List at 0x%x\n", NotificationList );
        DbgPrint( "        Pending notifications: %d\n", NotificationList->PendingNotifications );

        Event = &(NotificationList->Event[1]);

        for ( Index = 1; Index < NotificationList->EventsInUse; Index++ ) {

            DbgPrint( "          Event %d EventHandle 0x%x Client 0x%x",
                            Index,
                            NotificationList->EventHandle[ Index ],
                            Event->ClientEvent );

            if ( Event->Flags & CLIENT_IS_DEAD ) {
                DbgPrint( " (Dead)\n" );
            } else if ( Event->Flags & MUST_NOTIFY ) {
                DbgPrint( " (Notify)\n" );
            } else {
                DbgPrint( "\n" );
            }

            Event++;
        }

        DbgPrint( "\n");
    }

    DbgPrint( "\n");
}


VOID
DumpPipeTable(
    )
/*++

Routine Description:

    Dumps the pipe table

Arguments:


    None

Return Value:

    None

--*/

{
    DWORD       i;
    PPIPE_ENTRY Entry;

    DbgPrint( "\n\n      Pipes:\n\n" );

    for ( i=0; i < BUCKETS_IN_SYMBOL_TABLE; i++ ) {

        Entry = PipeSymbolTable.Bucket[i];
        if ( Entry ) {
            DbgPrint( "        Bucket %d:\n",i );
            while ( Entry ) {
                DbgPrint( "        %Z (%d)\n", &(Entry->PipeName), Entry->ReferenceCount );
                Entry = Entry->Next;
            }
        }
    }

    DbgPrint( "\n" );
}


#endif  // BIGDBG

#endif   // REMOTE_NOTIFICATION_DISABLED
