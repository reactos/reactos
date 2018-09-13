/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Regnckey.c

Abstract:

    This module contains the client side wrappers for the Win32 Registry
    APIs to notify a caller about a changed Key value. That is:

        - RegNotifyChangeKey

Author:

    David J. Gilman (davegi) 10-Feb-1992

Notes:

    The implementation of RegNotifyChangeKeyValue involves >= 4 threads: 2 on
    the client side and >= 2 on the server side.

    Client:

        Thread 1.- The user's thread executing the RegNotifyChangeKeyValue.
                   This threads does:

                   - If thread #2 has not been created yet, it creates a
                     named pipe and thread #2.

                   - Does a synchronous RPC to the server



        Thread 2.- This thread reads events from the named pipe and signals
                   them. The writers to the pipe are the RPC servers which
                   thread 1 has called.





    Server:

        Thread 1.- This thread services the RPC from the client side. It
                   calls the NT notification API and adds the notification
                   handle to a "notification list".

        Thread 2.- This thread waits on part of the "notification list",
                   telling the original client (via named pipe) what events
                   need to be signaled.

        Threads 3... etc. Same as thread 2.





Revision History:

    02-Apr-1992     Ramon J. San Andres (ramonsa)
                    Changed to use RPC.


--*/


#include <rpc.h>
#include "regrpc.h"
#include "client.h"
#include <stdlib.h>

NTSTATUS BaseRegNotifyClassKey(
    IN  HKEY                     hKey,
    IN  HANDLE                   hEvent,
    IN  PIO_STATUS_BLOCK         pLocalIoStatusBlock,
    IN  DWORD                    dwNotifyFilter,
    IN  BOOLEAN                  fWatchSubtree,
    IN  BOOLEAN                  fAsynchronous);

//
// Used by local call to NtNotifyChangeKey.
//

IO_STATUS_BLOCK     LocalIoStatusBlock;


#ifndef REMOTE_NOTIFICATION_DISABLED
//
//  Named pipe full paths.
//
#define NAMED_PIPE_HERE     L"\\Device\\NamedPipe\\"

//
//  Maximum number of times we will retry to create a pipe if there are
//  name conflicts.
//
#define MAX_PIPE_RETRIES    1000



//
//  Local variables.
//

//
//  Critical section to control access to notification structures
//
RTL_CRITICAL_SECTION        NotificationCriticalSection;

//
//  Our machine name
//
UNICODE_STRING              OurMachineName;
WCHAR                       OurMachineNameBuffer[ MAX_PATH ];

//
//  Named pipe used for notification
//
UNICODE_STRING              NotificationPipeName;
WCHAR                       NotificationPipeNameBuffer[ MAX_PATH ];
HANDLE                      NotificationPipeHandle;
RPC_SECURITY_ATTRIBUTES     NotificationPipeSaRpc;

//
//  Security descriptor used in the named pipe
//
SECURITY_DESCRIPTOR         SecurityDescriptor;
PACL                        Acl;
BOOL                        SecurityDescriptorInitialized;

//
//  Notification thread
//
HANDLE                      NotificationThread;
DWORD                       NotificationClientId;


//
//  Local prototypes
//
LONG
CreateNotificationPipe(
    );

VOID
NotificationHandler(
    );
#endif // REMOTE_NOTIFICATION_DISABLED


#ifndef REMOTE_NOTIFICATION_DISABLED

LONG
InitializeNotificationPipeSecurityDescriptor(
    )
/*++

Routine Description:

    Initialize the security descriptor (global variable) to be attached to
    the named pipe.

Arguments:

    None

Return Value:

    LONG - Returns a win32 error code.

--*/

{
    SID_IDENTIFIER_AUTHORITY WorldSidAuthority = SECURITY_WORLD_SID_AUTHORITY;
    ULONG                    AclLength;
    PSID                     WorldSid;

    NTSTATUS                 NtStatus;

    //
    // Initialize global variables
    //
    SecurityDescriptorInitialized = FALSE;
    Acl = NULL;

    //
    //  Get World SID
    //
    NtStatus = RtlAllocateAndInitializeSid( &WorldSidAuthority,
                                            1,
                                            SECURITY_WORLD_RID,
                                            0, 0, 0, 0, 0, 0, 0,
                                            &WorldSid
                                          );

    ASSERT( NT_SUCCESS( NtStatus ) );
    if ( !NT_SUCCESS( NtStatus )) {
#if DBG
        DbgPrint( "WINREG: Unable to allocate and initialize SID, NtStatus = %x \n", NtStatus );
#endif
        return( RtlNtStatusToDosError( NtStatus ) );
    }


    //
    //  Allocate buffer for ACL.
    //  This buffer should be big enough for the ACL header and for each ACE.
    //  Each ACE needs an ACE header.
    //
    AclLength = sizeof( ACL ) +
                sizeof( ACCESS_ALLOWED_ACE ) +
                GetLengthSid( WorldSid ) +
                sizeof( DWORD );

    Acl = RtlAllocateHeap( RtlProcessHeap(), 0, AclLength );
    ASSERT( Acl != NULL );
    if( Acl == NULL ) {
#if DBG
        DbgPrint( "WINREG: Unable to allocate memory, NtStatus = %x \n", NtStatus );
#endif
        RtlFreeSid( WorldSid );
        return( ERROR_OUTOFMEMORY );
    }

    //
    // Build ACL: World has all access
    //

    NtStatus = RtlCreateAcl( (PACL)Acl,
                             AclLength,
                             ACL_REVISION2
                           );

    ASSERT( NT_SUCCESS( NtStatus ) );
    if ( !NT_SUCCESS( NtStatus )) {
#if DBG
        DbgPrint( "WINREG: Unable to create ACL, NtStatus = %x \n", NtStatus );
#endif
        RtlFreeSid( WorldSid );
        RtlFreeHeap( RtlProcessHeap(), 0, Acl );
        return( RtlNtStatusToDosError( NtStatus ) );
    }

    NtStatus = RtlAddAccessAllowedAce( (PACL)Acl,
                                       ACL_REVISION2,
                                       SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,
                                       WorldSid
                                     );

    RtlFreeSid( WorldSid );
    ASSERT( NT_SUCCESS( NtStatus ) );
    if ( !NT_SUCCESS( NtStatus )) {
#if DBG
        DbgPrint( "WINREG: Unable to add ACE, NtStatus = %x \n", NtStatus );
#endif
        RtlFreeHeap( RtlProcessHeap(), 0, Acl );
        return( RtlNtStatusToDosError( NtStatus ) );
    }

    //
    //  Build security descriptor
    //
    NtStatus = RtlCreateSecurityDescriptor( &SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION );
    ASSERT( NT_SUCCESS( NtStatus ) );
    if ( !NT_SUCCESS( NtStatus )) {
#if DBG
        DbgPrint( "WINREG: Unable to create security descriptor, NtStatus = %x \n", NtStatus );
#endif
        RtlFreeHeap( RtlProcessHeap(), 0, Acl );
        return( RtlNtStatusToDosError( NtStatus ) );
    }

#if DBG
    if( !RtlValidAcl( (PACL )Acl ) ) {
        DbgPrint( "WINREG: Acl is invalid \n" );
        RtlFreeHeap( RtlProcessHeap(), 0, Acl );
        return( ERROR_INVALID_ACL );
    }
#endif

    NtStatus = RtlSetDaclSecurityDescriptor ( &SecurityDescriptor,
                                              TRUE,
                                              (PACL)Acl,
                                              FALSE
                                            );
    ASSERT( NT_SUCCESS( NtStatus ) );
    if ( !NT_SUCCESS( NtStatus )) {
#if DBG
        DbgPrint( "WINREG: Unable to set DACL, NtStatus = %x \n", NtStatus );
#endif
        RtlFreeHeap( RtlProcessHeap(), 0, Acl );
        return( RtlNtStatusToDosError( NtStatus ) );
    }
    SecurityDescriptorInitialized = TRUE;
    return( ERROR_SUCCESS );
}



BOOL
InitializeRegNotifyChangeKeyValue(
    )
/*++

Routine Description:


    Initializes the static data structures used by the
    RegNotifyChangeKeyValue client. Called once at DLL
    initialization.

Arguments:

    None

Return Value:

    BOOLEAN -   TRUE if successful.


--*/

{

    NTSTATUS    NtStatus;


    NtStatus = RtlInitializeCriticalSection(
                    &NotificationCriticalSection
                    );

    if ( NT_SUCCESS( NtStatus ) ) {



        //
        //  Initialize our machine name. Note that the actual
        //  name is only obtained when the notification API
        //  is first invoked.
        //
        OurMachineName.Length        = 0;
        OurMachineName.MaximumLength = MAX_PATH * sizeof(WCHAR);
        OurMachineName.Buffer        = OurMachineNameBuffer;

        //
        //  Initialize named pipe data
        //
        NotificationPipeName.Length         = 0;
        NotificationPipeName.MaximumLength  = MAX_PATH * sizeof(WCHAR);
        NotificationPipeName.Buffer         = NotificationPipeNameBuffer;

        NotificationThread      = NULL;
        NotificationPipeHandle  = NULL;

        NotificationPipeSaRpc.RpcSecurityDescriptor.lpSecurityDescriptor = NULL;

        return TRUE;
    }

    return FALSE;

}



BOOL
CleanupRegNotifyChangeKeyValue(
    )
/*++

Routine Description:


    Performs any cleanup of the static data structures used
    by the RegNotifyChangeKeyValue client. Called once at
    process termination.

Arguments:

    None

Return Value:

    BOOLEAN -   TRUE if successful.


--*/

{

    NTSTATUS    NtStatus;


    //
    //  Terminate notification thread if there is one running
    //
    if ( NotificationThread != NULL ) {

        //
        //  Close the named pipe
        //
        if ( NotificationPipeHandle != NULL ) {

            NtStatus = NtClose( NotificationPipeHandle );

            ASSERT( NT_SUCCESS( NtStatus ) );
        }

        TerminateThread( NotificationThread, 0 );
    }

    //
    //  Delete the notification critical section
    //
    NtStatus = RtlDeleteCriticalSection(
                    &NotificationCriticalSection
                    );

    ASSERT( NT_SUCCESS( NtStatus ) );

    if ( NotificationPipeSaRpc.RpcSecurityDescriptor.lpSecurityDescriptor ) {
        RtlFreeHeap(
            RtlProcessHeap( ), 0,
            NotificationPipeSaRpc.RpcSecurityDescriptor.lpSecurityDescriptor
            );
    }

    return  TRUE;

}
#endif  // REMOTE_NOTIFICATION_DISABLED



LONG
RegNotifyChangeKeyValue(
    HKEY    hKey,
    BOOL    fWatchSubtree,
    DWORD   dwNotifyFilter,
    HANDLE  hEvent,
    BOOL    fAsynchronous
    )

/*++

Routine Description:

    This API is used to watch a key or sub-tree for changes. It can be
    called either synchronously or asynchronously. In the latter case the
    caller must supply an event that is signalled when changes occur. In
    either case it is possible to filter the criteria by which the
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


    hEvent - Supplies an optional event handle. This parameter is ignored
        if fAsynchronus is set to FALSE.

    fAsynchronous - Supplies a flag which if FALSE causes the API to not
        return until something has changed. If TRUE, the API returns
        immediately and changes are reported via the supplied event. It
        is an error for this parameter to be TRUE and hEvent to be NULL.

Return Value:

    LONG -  Returns ERROR_SUCCESS (0); error-code for failure.

Notes:

    If the supplied hKey is closed the event is signalled.
    Therefore it is possible to return from a wait on the event and then
    have subsequent APIs fail.

--*/

{
    HKEY                        Handle;
    HANDLE                      EventHandle;
    LONG                        Error       = ERROR_SUCCESS;
    NTSTATUS                    NtStatus;
    PRPC_SECURITY_ATTRIBUTES    pRpcSa;
    HKEY                        TempHandle = NULL;

#if DBG
    if ( BreakPointOnEntry ) {
        DbgBreakPoint();
    }
#endif

    //
    // Limit the capabilities associated with HKEY_PERFORMANCE_DATA.
    //

    ConvertKey(&hKey);
    if( hKey == HKEY_PERFORMANCE_DATA ) {
        return ERROR_INVALID_HANDLE;
    }

    //
    // Validate the dependency between fAsynchronus and hEvent.
    //
    if (( fAsynchronous ) && ( ! ARGUMENT_PRESENT( hEvent ))) {
        return ERROR_INVALID_PARAMETER;
    }

    Handle = MapPredefinedHandle( hKey, &TempHandle );
    if ( Handle == NULL ) {
        CLOSE_LOCAL_HANDLE(TempHandle);
        return ERROR_INVALID_HANDLE;
    }

    //
    // Notification is not supported on remote handles.
    //
    if( !IsLocalHandle( Handle ) ) {
        CLOSE_LOCAL_HANDLE(TempHandle);
        return ERROR_INVALID_HANDLE;

    } else {

        //
        // If its a local handle, make an Nt API call and return.
        //

        if (IsSpecialClassesHandle( Handle )) {

            //
            // We call a special function for class keys
            //
            NtStatus = BaseRegNotifyClassKey(
                Handle,
                hEvent,
                &LocalIoStatusBlock,
                dwNotifyFilter,
                ( BOOLEAN ) fWatchSubtree,
                ( BOOLEAN ) fAsynchronous
                );

        } else {
            NtStatus = NtNotifyChangeKey(
                Handle,
                hEvent,
                NULL,
                NULL,
                &LocalIoStatusBlock,
                dwNotifyFilter,
                ( BOOLEAN ) fWatchSubtree,
                NULL,
                0,
                ( BOOLEAN ) fAsynchronous
                );
        }

        if( NT_SUCCESS( NtStatus ) ||
            ( NtStatus == STATUS_PENDING ) ) {
            Error = (error_status_t)ERROR_SUCCESS;
        } else {
            Error = (error_status_t) RtlNtStatusToDosError( NtStatus );
        }

        CLOSE_LOCAL_HANDLE(TempHandle);
        return Error;
    }

#ifndef REMOTE_NOTIFICATION_DISABLED

    // NOTE: THE FOLLOWING CODE IS DISABLED BY THE CHECK FOR
    // IsLocalHandle AT THE BEGINNING OF THE FUNCTION.
    //

    //
    //  If this is an asynchronous call, we use the user-provided
    //  event and will let the user wait on it him/herself.
    //  Otherwise we have to create our own event and wait on
    //  it ourselves.
    //
    //  This is because the server side of the API is always
    //  asynchronous.
    //
    if ( fAsynchronous ) {

        EventHandle = hEvent;

    } else {

        NtStatus = NtCreateEvent(
                        &EventHandle,
                        EVENT_ALL_ACCESS,
                        NULL,
                        NotificationEvent,
                        FALSE
                        );

        if ( !NT_SUCCESS( NtStatus ) ) {
            return RtlNtStatusToDosError( NtStatus );
        }
    }

    //
    //  See if the notification thread is already running
    //  and create it if not.  We have to protect this
    //  with a critical section because there might be
    //  several instances of this API doing this check
    //  at the same time.
    //
    NtStatus = RtlEnterCriticalSection( &NotificationCriticalSection );

    if ( !NT_SUCCESS( NtStatus ) ) {

        Error = RtlNtStatusToDosError( NtStatus );

    } else {

        //
        //  We are now inside the critical section
        //
        if ( NotificationThread == NULL ) {


            //
            //  Create a named pipe for the notification thread
            //  to use.
            //
            Error = CreateNotificationPipe( );

            if ( Error == ERROR_SUCCESS ) {

                //
                //  Create the notification thread
                //
                NotificationThread = CreateThread(
                                        NULL,
                                        (16 * 1024),
                                        (LPTHREAD_START_ROUTINE)NotificationHandler,
                                        NULL,
                                        0,
                                        &NotificationClientId
                                        );

                if ( NotificationThread == NULL ) {
                    //
                    //  Could not create thread, remove the named pipe.
                    //
                    Error = GetLastError();
                    NtClose( NotificationPipeHandle );
                }
            }
        }

        NtStatus = RtlLeaveCriticalSection( &NotificationCriticalSection );

        ASSERT( NT_SUCCESS( NtStatus ) );
    }

    if ( Error == ERROR_SUCCESS ) {

        //
        //  Let the server side do its work. Remember that this call
        //  is always asynchronous.
        //
        if ( NotificationPipeSaRpc.RpcSecurityDescriptor.lpSecurityDescriptor ) {
            pRpcSa = &NotificationPipeSaRpc;
        } else {
            pRpcSa = NULL;
        }

        //NotificationPipeName.Length += sizeof(UNICODE_NULL);
        //OurMachineName.Length       += sizeof(UNICODE_NULL );

        // DbgPrint(" Waiting for notification, handle %x\n", EventHandle );

        Error = (LONG)BaseRegNotifyChangeKeyValue(
                                DereferenceRemoteHandle( Handle ),
                                (BOOLEAN)fWatchSubtree,
                                dwNotifyFilter,
                                (DWORD)EventHandle,
                                &OurMachineName,
                                &NotificationPipeName,
                                pRpcSa
                                );

        //NotificationPipeName.Length -= sizeof(UNICODE_NULL);
        //OurMachineName.Length       -= sizeof(UNICODE_NULL );

    }


    //
    //  If the call went ok. and we are in synchronous mode, we have
    //  to wait on the event.
    //
    if ( (Error == ERROR_SUCCESS) && !fAsynchronous ) {

        NtStatus = NtWaitForSingleObject(
                        EventHandle,
                        FALSE,
                        NULL
                        );


        if ( !NT_SUCCESS( NtStatus ) ) {
            Error = RtlNtStatusToDosError( NtStatus );
        }
    }


    //
    //  If we created an event, we must close it now.
    //
    if ( !fAsynchronous ) {

        NtStatus = NtClose( EventHandle );
        ASSERT( NT_SUCCESS( NtStatus ));
    }

    return Error;
#endif // REMOTE_NOTIFICATION_DISABLED
}


#ifndef REMOTE_NOTIFICATION_DISABLED


LONG
CreateNotificationPipe(
    )
/*++

Routine Description:


    Creates the notification named pipe and sets the appropriate
    global variables.

    Note that the NotificationPipeName set by this function is
    server-relative, so that no conversion is required on the
    server side.


Arguments:

    None

Return Value:

    Error code.


--*/
{

    UNICODE_STRING      PipeName;
    WCHAR               PipeNameBuffer[ MAX_PATH ];
    USHORT              OrgSize;
    DWORD               Sequence;
    NTSTATUS            NtStatus;
    LARGE_INTEGER       Timeout;
    OBJECT_ATTRIBUTES   Obja;
    IO_STATUS_BLOCK     IoStatusBlock;
    DWORD               MachineNameLength;
    LONG                WinStatus;

    //
    //  Get our machine name
    //
    MachineNameLength = MAX_PATH;
    if ( !GetComputerNameW( OurMachineNameBuffer, &MachineNameLength ) ) {
        return GetLastError();
    }

    OurMachineName.Buffer        = OurMachineNameBuffer;
    OurMachineName.Length        = (USHORT)(MachineNameLength * sizeof(WCHAR));
    OurMachineName.MaximumLength = (USHORT)(MAX_PATH * sizeof(WCHAR));

    //
    //  Get the "here" name
    //
    RtlMoveMemory(
            PipeNameBuffer,
            NAMED_PIPE_HERE,
            sizeof( NAMED_PIPE_HERE)
            );


    PipeName.MaximumLength  = MAX_PATH * sizeof(WCHAR);
    PipeName.Buffer         = PipeNameBuffer;

    //
    //  Remember the size of the base portion of the pipe name, so
    //  we can patch it later when we attempt to create the full
    //  name.
    //
    OrgSize = (USHORT)(sizeof(NAMED_PIPE_HERE) - sizeof(UNICODE_NULL));

    //
    //  Create the named pipe, if the name is already being used,
    //  keep trying with different names.
    //
    Sequence = 0;

    Timeout.QuadPart = Int32x32To64( -10 * 1000, 50 );

    //
    //  Initialize the security descriptor that will be set in the named pipe
    //
    WinStatus = InitializeNotificationPipeSecurityDescriptor();
    if( WinStatus != ERROR_SUCCESS ) {
        return( WinStatus );
    }

    do {

        //
        //  Get a semi-unique name
        //
        if ( !MakeSemiUniqueName( &NotificationPipeName, Sequence++ ) ) {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        //
        //  Patch the full pipe name, in case this is not our first
        //  try.
        //
        PipeName.Buffer[OrgSize/sizeof(WCHAR)] = UNICODE_NULL;
        PipeName.Length          = OrgSize;

        //
        //  Now get the full path of the pipe name
        //
        NtStatus = RtlAppendUnicodeStringToString(
                            &PipeName,
                            &NotificationPipeName
                            );


        ASSERT( NT_SUCCESS( NtStatus ) );

        if ( !NT_SUCCESS( NtStatus ) ) {
            break;
        }


        InitializeObjectAttributes(
                    &Obja,
                    &PipeName,
                    OBJ_CASE_INSENSITIVE,
                    NULL,
                    NULL
                    );


        if( SecurityDescriptorInitialized ) {
            Obja.SecurityDescriptor = &SecurityDescriptor;
        }

        NtStatus = NtCreateNamedPipeFile (
                        &NotificationPipeHandle,
                        SYNCHRONIZE | GENERIC_READ | FILE_WRITE_ATTRIBUTES,
                        &Obja,
                        &IoStatusBlock,
                        FILE_SHARE_WRITE | FILE_SHARE_READ,
                        FILE_CREATE,
                        FILE_SYNCHRONOUS_IO_NONALERT,
                        FILE_PIPE_MESSAGE_TYPE,
                        FILE_PIPE_MESSAGE_MODE,
                        FILE_PIPE_QUEUE_OPERATION,
                        1,
                        0,
                        0,
                        &Timeout
                        );

    } while ( (NtStatus == STATUS_OBJECT_NAME_EXISTS) &&
              (Sequence <= MAX_PIPE_RETRIES )
            );

    //
    // At this point we don't need the security descriptor anymore.
    // Free the memory allocated for the ACL
    //
    if( SecurityDescriptorInitialized ) {
        RtlFreeHeap( RtlProcessHeap( ), 0, Acl );
        Acl = NULL;
        SecurityDescriptorInitialized = FALSE;
    }

    if ( !NT_SUCCESS( NtStatus ) ) {
        return RtlNtStatusToDosError( NtStatus );
    }

    NotificationPipeName.Length += sizeof(UNICODE_NULL);
    OurMachineName.Length       += sizeof(UNICODE_NULL );

    return ERROR_SUCCESS;
}





VOID
NotificationHandler(
    )

/*++

Routine Description:


    This function is the entry point of the notification thread.
    The notification thread is created the first time that
    the RegNotifyChangeKeyValue API is called by the process,
    and keeps on running until the process terminates.

    This function creates a named pipe whose name is given by
    RegNotifyChangeKeyValue to all its servers.  The servers
    then use the pipe to indicate that a particular event has
    to be signaled.
                                                                        117
    Note that this single thread is in charge of signaling the
    events for all the RegNotifyChangeKeyValue invocations of
    the process.  However no state has to be maintained by this
    thread because all the state information is provided by the
    server through the named pipe.


Arguments:

    None

Return Value:

    None


--*/

{
    NTSTATUS        NtStatus;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE          EventHandle;


    ASSERT( NotificationPipeHandle != NULL );

    while ( TRUE ) {

        //
        //  Wait for a connection
        //
        NtStatus = NtFsControlFile(
                        NotificationPipeHandle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        FSCTL_PIPE_LISTEN,
                        NULL,
                        0,
                        NULL,
                        0
                        );

        if ( NtStatus == STATUS_PENDING ) {

            NtStatus = NtWaitForSingleObject(
                            NotificationPipeHandle,
                            FALSE,
                            NULL
                            );
        }

        if ( NT_SUCCESS( NtStatus ) ||
             ( NtStatus == STATUS_PIPE_CONNECTED ) ) {

            //
            //  Read an event handle from the pipe
            //
            NtStatus = NtReadFile(
                            NotificationPipeHandle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            ( PVOID )&EventHandle,
                            sizeof( HANDLE ),
                            NULL,
                            NULL
                            );

            if ( NtStatus == STATUS_PENDING ) {

                NtStatus = NtWaitForSingleObject(
                                NotificationPipeHandle,
                                FALSE,
                                NULL
                                );
            }


            //
            //  Signal the Event.
            //
            if ( NT_SUCCESS( NtStatus ) ) {

                ASSERT( IoStatusBlock.Information == sizeof( HANDLE ) );

                //
                //  Signal the event
                //
                //DbgPrint(" WINREG: Signaling handle %x\n", EventHandle );
                NtStatus = NtSetEvent( EventHandle, NULL );

#if DBG
                if ( !NT_SUCCESS( NtStatus ) ) {
                    DbgPrint( "WINREG: Cannot signal notification event 0x%x, status %x\n",
                                EventHandle, NtStatus );
                }
#endif
                ASSERT( NT_SUCCESS( NtStatus ) );

            } else if ( NtStatus != STATUS_PIPE_BROKEN ) {
#if DBG
                DbgPrint( "WINREG  (Notification handler) error reading pipe\n" );
                DbgPrint( "         status 0x%x\n", NtStatus );
#endif
                ASSERT( NT_SUCCESS( NtStatus ) );
            }

        } else if ( NtStatus != STATUS_PIPE_BROKEN &&
                    NtStatus != STATUS_PIPE_CLOSING) {
#if DBG
            DbgPrint( "WINREG (Notification): FsControlFile (Connect) status 0x%x\n",
                      NtStatus );
#endif
        }

        if ( NT_SUCCESS( NtStatus )             ||
             NtStatus == STATUS_PIPE_BROKEN     ||
             NtStatus == STATUS_PIPE_CLOSING    ||
             NtStatus == STATUS_PIPE_LISTENING  ||
             NtStatus == STATUS_PIPE_BUSY ) {

            //
            //  Disconnect
            //
            NtStatus = NtFsControlFile(
                                NotificationPipeHandle,
                                NULL,
                                NULL,
                                NULL,
                                &IoStatusBlock,
                                FSCTL_PIPE_DISCONNECT,
                                NULL,
                                0,
                                NULL,
                                0
                                );

            if ( NtStatus == STATUS_PENDING) {

                NtStatus = NtWaitForSingleObject(
                                NotificationPipeHandle,
                                FALSE,
                                NULL
                                );
            }

#if DBG
            if ( !NT_SUCCESS( NtStatus ) ) {
                DbgPrint( "WINREG (Notification): FsControlFile (Disconnect) status 0x%x\n",
                          NtStatus );
            }
#endif
            ASSERT( NT_SUCCESS( NtStatus ) );

        }
    }
}

#endif //  REMOTE_NOTIFICATION_DISABLED
