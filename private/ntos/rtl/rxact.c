/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    rxact.c

Abstract:

    This Module implements a simple transaction mechanism for registry
    database operations which helps eliminate the possibility of partial
    updates being made.  The cases covered are specifically partial updates
    caused by system crashes or program aborts during multiple-write updates.


    WARNING:  This package does not yet deal with full-disk problems
              automatically.  If a full disk is encountered during
              transaction commit, then manual interaction may be required
              to free enough space to complete the commit.  There is
              no means provided for backing out the commit.

Author:

    Jim Kelly       (JimK)     15-May-1991
    Robert Reichel  (RobertRe) 15-July-1992

Environment:

    Pure Runtime Library Routine

Revision History:



--*/


/*
////////////////////////////////////////////////////////////////////////////

High Level Description:

    The simple transaction mechanism expects the following to be true:

       (1) A single server is responsible for operations on an entire
           sub-tree of the registry database.  For example, the security
           account manager server (SAM) is responsible for everything
           below \REGISTRY\LOCAL_MACHINE\SECURITY\SAM.

       (2) Transactions on the sub-tree are serialized by the server
           responsible for the sub-tree.  That is, the server will not
           start a second user request until all previous user requests
           have been completed.

    The simple transaction mechanism helps eliminate the problem of partial
    updates caused by system crash or program abort during multiple-write
    updates to the registry database.  This is achieved by:

       (1) Keeping all actions in-memory until instructed to commit.  The
           presence of in-memory data structures implicitly indicates
           that a transaction is in progress.

           The initial state is no transaction in progress.

       (2) Providing a service which allows a server to initiate a transaction.
           This allocates in-memory data structures, thereby changing the
           state to transaction in progress.

       (3) Keeping a log of all keys in the sub-tree that are to be
           updated in a single transaction.  Each record in this log
           contains the following information:

                   (a) The name of the sub-key effected

                   (b) The operation to be performed on the sub-key
                       either DELETE or SET_VALUE.  Note that these
                       operations are idempotent and may be applied
                       again in the event that the server aborts during
                       an initial commit.

                   (c) The new value of the sub-key (if applicable)

                   (d) (optionally) The attribute name of the subkey
                       to be operated on.

                    (note that SET_VALUE is used to create new sub-keys
                     as well as updated existing ones).

            The entire list of sub-keys to be modified must be entered
            into this log before ANY of the sub-keys is actually modified.

       (4)  Providing a commit service that applies all changes indicated
            in the change log.  This is done by first writing the contents
            of the in-memory structures to a single key value ("Log") in
            the registry and flushing the data to disk.  The presence of
            the "Log" value and data imply that a commit is in progress.

            All necessary changes are applied, the "Log" value and its
            data are deleted, and in-memory data structres are freed,
            thereby changing the state to no-transaction.


    The package also includes a service which must be called upon server
    startup.  This service checks to make sure the state of the sub-tree
    is NO_TRANSACTION.  If it is not, then one of the actions below is
    performed based upon the current state of the sub-tree:

        COMMITTING - This means the server was previously aborted while
            a transaction was being committed (applied to the registry).
            In this case, the commit is performed again from the beginning
            of the change log.  After the commit is completed, the state
            of the sub-tree is set to NO_TRANSACTION.

////////////////////////////////////////////////////////////////////////////
*/



/*
////////////////////////////////////////////////////////////////////////////

Detailed Description:

    Registry State
    --------------

    The registry state of a subtree is kept in a sub-key of that tree
    named:

            "RXACT"

    The value field of that registry key includes a revision field.


    RXact Context
    -------------

    A call to RtlInitializeRXact will return a pointer to an
    RTL_RXACT_CONTEXT structure.  This structure contains:

    (1) the passed RootRegistryKey (eg, key to "Sam"),

    (2) a handle to the top of the RXact subtree (eg, key to
        "Sam\RXACT"),

    (3) a flag indicating if handles stored in the log are
        valid,

    (4) a pointer to the current RXactLog.

    The subsystem calling RtlInitializeRXact must keep this returned
    pointer and pass it back to RXact in all subsequent calls.


    Operation Log
    -------------

    The operation log of a registry sub-tree transaction is kept as sequence
    of "operation log entries".

    An in-memory log is a block of heap memory allocted by RtlStartRXact.
    It has a header which contains:

    (1) The count of operations in the log.

    (2) The maximum size of the log.

    (3) The amount of the log currently in use.

    The log data itself follows the header directly.


    Operation Log Entries
    ---------------------

    An operation log entry is described by the following structure:

    typedef struct _RXACT_LOG_ENTRY {
        ULONG LogEntrySize;
        RTL_RXACT_OPERATION Operation;
        UNICODE_STRING SubKeyName;       // Self-relativized (Buffer is really offset)
        UNICODE_STRING AttributeName;    // Self-relativized (Buffer is really offset)
        HANDLE KeyHandle;                // optional, not valid if read from disk.
        ULONG NewKeyValueType;
        ULONG NewKeyValueLength;
        PVOID NewKeyValue;               // Contains offset to data from start of log
    } RXACT_LOG_ENTRY, *PRXACT_LOG_ENTRY;

    The log entry contains all of the information passed in during a call
    to RtlAddActionToRXact or RtlAddAttributeActionToRXact.

    The UNICODE_STRING structures contain an offset to the string data
    rather than a pointer.  These offsets are relative to the start of
    the log data, and are adjusted in place as each log entry is commited.

    The KeyHandle is valid if it is not equal to INVALID_HANDLE_VALUE and
    if the HandlesValid flag in the RXactContext structure is TRUE.  This
    is so that we do not attempt to use the handles if the log has been
    read from disk after a reboot.


////////////////////////////////////////////////////////////////////////////
*/


#include "ntrtlp.h"

//
// Cannot include <windows.h> from kernel code
//
#define INVALID_HANDLE_VALUE (HANDLE)-1





///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//    Local Macros & Definitions                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//
// Revision level of a registry transaction .
//

#define RTLP_RXACT_REVISION1          (1l)
#define RTLP_RXACT_CURRENT_REVISION   RTLP_RXACT_REVISION1


#define RTLP_RXACT_KEY_NAME           L"RXACT"

#define RTLP_RXACT_LOG_NAME           L"Log"

#define RTLP_INITIAL_LOG_SIZE         0x4000

//
//  Given a value return its longword aligned equivalent value
//

#define DwordAlign(Value) (                       \
    (ULONG)((((ULONG)(Value)) + 3) & 0xfffffffc)  \
    )

//
// The value field of the RXACT registry key is one of the following data
// structures.
//

//
// The state of a registry sub-tree is one of the following:
//
//         RtlpRXactStateNoTransaction - There is not a transaction in progress.
//
//         RtlpRXactStateCommitting    - The actions of a transaction are being
//                                       applied to the registry database.
//

typedef enum _RTLP_RXACT_STATE {
    RtlpRXactStateNoTransaction = 2,
    RtlpRXactStateCommitting
} RTLP_RXACT_STATE, *PRTLP_RXACT_STATE;


typedef struct _RTLP_RXACT {
    ULONG Revision;
    RTLP_RXACT_STATE State;   // no longer used
    ULONG OperationCount;     // no longer used
} RTLP_RXACT, *PRTLP_RXACT;


typedef struct _RXACT_LOG_ENTRY {
    ULONG LogEntrySize;
    RTL_RXACT_OPERATION Operation;
    UNICODE_STRING SubKeyName;       // Self-relativized (Buffer is really offset)
    UNICODE_STRING AttributeName;    // Self-relativized (Buffer is really offset)
    HANDLE KeyHandle;                // optional, not valid if read from disk.
    ULONG NewKeyValueType;
    ULONG NewKeyValueLength;
    PVOID NewKeyValue;               // Contains offset to data from start of log
} RXACT_LOG_ENTRY, *PRXACT_LOG_ENTRY;




////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                      Prototypes for local procedures                       //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////




NTSTATUS
RXactpCommit(
    IN PRTL_RXACT_CONTEXT RXactContext
    );

NTSTATUS
RXactpOpenTargetKey(
    IN HANDLE RootRegistryKey,
    IN RTL_RXACT_OPERATION Operation,
    IN PUNICODE_STRING SubKeyName,
    OUT PHANDLE TargetKey
    );



VOID
RXactInitializeContext(
    IN PRTL_RXACT_CONTEXT RXactContext,
    IN HANDLE RootRegistryKey,
    IN HANDLE RXactKey
    );


#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(PAGE,RXactpCommit)
#pragma alloc_text(PAGE,RXactpOpenTargetKey)
#pragma alloc_text(PAGE,RXactInitializeContext)
#pragma alloc_text(PAGE,RtlInitializeRXact)
#pragma alloc_text(PAGE,RtlStartRXact)
#pragma alloc_text(PAGE,RtlAbortRXact)
#pragma alloc_text(PAGE,RtlAddAttributeActionToRXact)
#pragma alloc_text(PAGE,RtlAddActionToRXact)
#pragma alloc_text(PAGE,RtlApplyRXact)
#pragma alloc_text(PAGE,RtlApplyRXactNoFlush)
#endif


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//    Exported Procedures   (defined in ntrtl.h)                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


NTSTATUS
RtlInitializeRXact(
    IN HANDLE RootRegistryKey,
    IN BOOLEAN CommitIfNecessary,
    OUT PRTL_RXACT_CONTEXT *RXactContext
    )

/*++

Routine Description:

    This routine should be called by a server exactly once when it starts.
    This routine will check to see that the registry transaction information
    exists for the specified registry sub-tree, and will create it if it
    doesn't exist.

Arguments:

    RootRegistryKey - A handle to the registry key within whose sub-tree
        a transaction is to be initialized.

    CommitIfNecessary - A BOOLEAN value indicating whether or not any
        previously aborted commit discovered should be commited at this
        time.  A value of TRUE indicates the commit should be applied
        if encountered.  A value of FALSE indicates a previously
        aborted COMMIT should not be committed at this time.

    RXactContext - Returns a pointer to an RTL_RXACT_CONTEXT structure
        allocated out of the local heap.  The caller must keep this
        pointer and pass it back in for all future RXact transactions
        for the passed RootRegistryKey.


Return Value:

    STATUS_SUCCESS - Indicates the transaction state already exists for the
        registry sub-tree and is already in the NO_TRANSACTION state.

    STATUS_UNKNOWN_REVISION - Indicates that a transaction state already
        exists for the specified sub-tree, but is a revision level that is
        unknown by this service.

    STATUS_RXACT_STATE_CREATED - This informational level status indicates
        that a specified registry sub-tree transaction state did not yet
        exist and had to be created.

    STATUS_RXACT_COMMIT_NECESSARY - This warning level status indicates that the
        transaction state already exists for the registry sub-tree, but that
        a transaction commit was previously aborted.  The commit has NOT been
        completed.  Another call to this service with a CommitIfNecessary value
        of TRUE may be used to commit the transaction.


    STATUS_RXACT_INVALID_STATE - Indicates that the transaction state
        of the registry sub-tree is incompatible with the requested operation.
        For example, a request to start a new transaction while one is already
        in progress, or a request to apply a transaction when one is not
        currently in progress.

--*/

{

    HANDLE RXactKey;
    LARGE_INTEGER LastWriteTime;
    NTSTATUS Status, TmpStatus;
    OBJECT_ATTRIBUTES RXactAttributes;
    PKEY_VALUE_FULL_INFORMATION FullInformation;
    RTLP_RXACT RXactKeyValue;
    UCHAR BasicInformation[128];      // Should be more than long enough
    ULONG Disposition;
    ULONG KeyValueLength;
    ULONG KeyValueType;
    ULONG ResultLength;
    UNICODE_STRING RXactKeyName;
    UNICODE_STRING ValueName;
    UNICODE_STRING NullName;

    RTL_PAGED_CODE();

    //
    // Initialize some stuff
    //

    KeyValueLength = (ULONG)sizeof( RTLP_RXACT );
    KeyValueType   = 0;         // Not used by RXact

    RtlInitUnicodeString( &NullName, NULL );

    //
    // Create or open the RXACT key.
    //

    RtlInitUnicodeString( &RXactKeyName, RTLP_RXACT_KEY_NAME);

    InitializeObjectAttributes(
        &RXactAttributes,
        &RXactKeyName,
        OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
        RootRegistryKey,
        NULL);

//    Status = RtlpNtCreateKey(
//                 &RXactKey,
//                 (KEY_READ | KEY_WRITE | DELETE),
//                 &RXactAttributes,
//                 0,
//                 NULL,
//                 &Disposition
//                 );

    Status = NtCreateKey( &RXactKey,
                          (KEY_READ | KEY_WRITE | DELETE),
                          &RXactAttributes,
                          0,                          //TitleIndex
                          NULL,                       //Class OPTIONAL,
                          REG_OPTION_NON_VOLATILE,    //CreateOptions,
                          &Disposition
                          );

    if ( !NT_SUCCESS(Status) ) {
        return(Status);
    }

    //
    // Allocate the RXactContext block
    //

    *RXactContext = RtlAllocateHeap( RtlProcessHeap(), 0, sizeof( RTL_RXACT_CONTEXT ));

    if ( *RXactContext == NULL ) {

        //
        // Something prevented value assignment...
        // Get rid of the RXact key and return the error
        //

        TmpStatus = NtDeleteKey( RXactKey );
        ASSERT(NT_SUCCESS(TmpStatus)); //Safe to ignore, notify security group
        TmpStatus = NtClose( RXactKey );
        ASSERT(NT_SUCCESS(TmpStatus)); //Safe to ignore, notify security group

        return( STATUS_NO_MEMORY );
    }

    //
    // Initialize the newly created RXactContext structure.
    //

    RXactInitializeContext( *RXactContext, RootRegistryKey, RXactKey );

    //
    // If we created (as opposed to opened an existing) rxact key,
    // then we need to initialize it.
    //

    if ( Disposition == REG_CREATED_NEW_KEY ) {

        RXactKeyValue.Revision       = RTLP_RXACT_REVISION1;

        Status = NtSetValueKey( RXactKey,
                                &NullName,       // ValueName
                                0,               // TitleIndex
                                KeyValueType,
                                &RXactKeyValue,
                                KeyValueLength
                                );

        if ( !NT_SUCCESS(Status) ) {

            //
            // Something prevented value assignment...
            // Get rid of the RXact key and return the error
            //

            TmpStatus = NtDeleteKey( RXactKey );
            ASSERT(NT_SUCCESS(TmpStatus)); //Safe to ignore, notify security group
            TmpStatus = NtClose( RXactKey );
            ASSERT(NT_SUCCESS(TmpStatus)); //Safe to ignore, notify security group

            RtlFreeHeap( RtlProcessHeap(), 0, *RXactContext );

            return( Status );
        }

        return( STATUS_RXACT_STATE_CREATED );
    }



    //
    // We have opened an existing RXACT key.
    // See if it is a revision level we know about.
    //

    Status = RtlpNtQueryValueKey(
                 RXactKey,              // KeyHandle
                 &KeyValueType,         // KeyValueType
                 &RXactKeyValue,        // KeyValue
                 &KeyValueLength,       // KeyValueLength
                 &LastWriteTime         // LastWriteTime
                 );


    if ( !NT_SUCCESS(Status) ) {

        //
        // Something prevented value query...
        //

        TmpStatus = NtClose( RXactKey );
        ASSERT(NT_SUCCESS(TmpStatus)); //Safe to ignore, notify security group
        RtlFreeHeap( RtlProcessHeap(), 0, *RXactContext );
        return( Status );
    }


    if ( KeyValueLength != (ULONG)sizeof(RTLP_RXACT) ) {
        TmpStatus = NtClose( RXactKey );
        ASSERT(NT_SUCCESS(TmpStatus)); //Safe to ignore, notify security group
        RtlFreeHeap( RtlProcessHeap(), 0, *RXactContext );
        return( STATUS_UNKNOWN_REVISION );
    }

    if (RXactKeyValue.Revision != RTLP_RXACT_REVISION1) {
        TmpStatus = NtClose( RXactKey );
        ASSERT(NT_SUCCESS(TmpStatus)); //Safe to ignore, notify security group
        RtlFreeHeap( RtlProcessHeap(), 0, *RXactContext );
        return( STATUS_UNKNOWN_REVISION );
    }



    //
    // Right revision...
    // See if there is a transaction or commit in progress.  If not,
    // return success
    //

    //
    // If a log file exists, then we are committing.
    //

    RtlInitUnicodeString( &ValueName, RTLP_RXACT_LOG_NAME );

    Status = NtQueryValueKey(
                 RXactKey,
                 &ValueName,
                 KeyValueBasicInformation,
                 &BasicInformation,
                 128,
                 &ResultLength
                 );

    if ( NT_SUCCESS( Status )) {

        //
        // We found a value called 'Log'.  This means that a commit
        // was in progress.
        //

        if ( CommitIfNecessary ) {

            //
            // Query the full value of the log, then call a low level routine
            // to actually perform the commit.
            //

            Status = NtQueryValueKey(
                         RXactKey,
                         &ValueName,
                         KeyValueFullInformation,
                         NULL,
                         0,
                         &ResultLength
                         );

            if ( Status != STATUS_BUFFER_TOO_SMALL ) {
                return( Status );
            }

            FullInformation = RtlAllocateHeap( RtlProcessHeap(), 0, ResultLength );

            if ( FullInformation == NULL ) {
                return( STATUS_NO_MEMORY );
            }


            Status = NtQueryValueKey(
                         RXactKey,
                         &ValueName,
                         KeyValueFullInformation,
                         FullInformation,
                         ResultLength,
                         &ResultLength
                         );

            if ( !NT_SUCCESS( Status )) {

                RtlFreeHeap( RtlProcessHeap(), 0, FullInformation );
                RtlFreeHeap( RtlProcessHeap(), 0, *RXactContext );
                return( Status );
            }

            //
            // The log information is buried in the returned FullInformation
            // buffer.  Dig it out and make the RXactLog in the RXactContext
            // structure point to it.  Then commit.
            //

            (*RXactContext)->RXactLog = (PRTL_RXACT_LOG)((PCHAR)FullInformation + FullInformation->DataOffset);

            //
            // Don't use any handles we may find in the log file
            //

            (*RXactContext)->HandlesValid = FALSE;

            Status = RXactpCommit( *RXactContext );

            if ( !NT_SUCCESS( Status )) {

                RtlFreeHeap( RtlProcessHeap(), 0, FullInformation );
                RtlFreeHeap( RtlProcessHeap(), 0, *RXactContext );
                return( Status );
            }


            //
            // The commit was successful.  Clean up.
            // Delete the log file value and data
            //

            Status = NtDeleteValueKey( RXactKey, &ValueName );

            //
            // This should never fail
            //

            ASSERT( NT_SUCCESS( Status ));

            //
            // Get rid of the in memory data structures.  Abort
            // will free the RXactLog, so put what we want
            // freed in there and it will go away.
            //

            (*RXactContext)->RXactLog = (PRTL_RXACT_LOG)FullInformation;

            Status = RtlAbortRXact( *RXactContext );

            //
            // This should never fail
            //

            ASSERT( NT_SUCCESS( Status ));
            return( Status );
        } else {

            return( STATUS_RXACT_COMMIT_NECESSARY );
        }

    } else {

        //
        // No log, so nothing to do here.
        //

        return( STATUS_SUCCESS );
    }

}



VOID
RXactInitializeContext(
    IN PRTL_RXACT_CONTEXT RXactContext,
    IN HANDLE RootRegistryKey,
    IN HANDLE RXactKey
    )

/*++

Routine Description:

    Initializes an in-memory RXactContext structure.

Arguments:

    RXactContext - Supplies a pointer to an RXact Context created
        by RtlInitializeRXact.

    RootRegistryKey - Supplies the RootRegistryKey for this component.

    RXactKey - Supplies the {RootRegistryKey}\RXactKey for this component


Return Value:

    None.

--*/

{
    //
    // Initialize the RXactContext for this client
    //

    RXactContext->RootRegistryKey      = RootRegistryKey;
    RXactContext->HandlesValid         = TRUE;
    RXactContext->RXactLog             = NULL;
    RXactContext->RXactKey             = RXactKey;

    return;
}



NTSTATUS
RtlStartRXact(
    IN PRTL_RXACT_CONTEXT RXactContext
    )

/*++

Routine Description:

    This routine is used to start a new transaction in a registry sub-tree.
    Transactions must be serialized by the server so that only one transaction
    is in progress at a time.

Arguments:

    RXactContext - Supplies a pointer to an RTL_RXACT_CONTEXT structure
        that is not currently in use.

Return Value:

    STATUS_SUCCESS - Indicates the transaction was started.

    STATUS_RXACT_INVALID_STATE - Indicates that the transaction state
        of the registry sub-tree is incompatible with the requested operation.
        For example, a request to start a new transaction while one is already
        in progress, or a request to apply a transaction when one is not
        currently in progress.  This may also indicate that there is no
        transaction state at all for the specified registry sub-tree.

--*/
{
    PRTL_RXACT_LOG RXactLogHeader;

    RTL_PAGED_CODE();

    //
    // Allocate in-memory log file and initialize.  This implicitly
    // sets the state to 'transaction in progress'.
    //

    if ( RXactContext->RXactLog != NULL ) {

        //
        // There is already a transaction in progress for this
        // context.  Return an error.
        //

        return( STATUS_RXACT_INVALID_STATE );
    }

    RXactLogHeader = RtlAllocateHeap( RtlProcessHeap(), 0, RTLP_INITIAL_LOG_SIZE );

    if ( RXactLogHeader == NULL ) {
        return( STATUS_NO_MEMORY );
    }

    //
    // Fill in the log header information at the top of the
    // newly allocated buffer.
    //


    RXactLogHeader->OperationCount = 0;
    RXactLogHeader->LogSize        = RTLP_INITIAL_LOG_SIZE;
    RXactLogHeader->LogSizeInUse   = sizeof( RTL_RXACT_LOG );

    RXactContext->RXactLog = RXactLogHeader;

    return( STATUS_SUCCESS );

}


NTSTATUS
RtlAbortRXact(
    IN PRTL_RXACT_CONTEXT RXactContext
    )

/*++

Routine Description:

    This routine is used to abort a transaction in a registry sub-tree.

Arguments:

    RootRegistryKey - A handle to the registry key within whose sub-tree
        the transaction is to be aborted.

Return Value:

    STATUS_SUCCESS - Indicates the transaction was aborted.


    STATUS_UNKNOWN_REVISION - Indicates that a transaction state
        exists for the specified sub-tree, but has a revision level that is
        unknown by this service.


    STATUS_RXACT_INVALID_STATE - Indicates that the transaction state
        of the registry sub-tree is incompatible with the requested operation.
        For example, a request to start a new transaction while one is already
        in progress, or a request to apply a transaction when one is not
        currently in progress.  This may also indicate that there is no
        transaction state at all for the specified registry sub-tree.

--*/

{
    RTL_PAGED_CODE();

    if ( RXactContext->RXactLog == NULL ) {

        //
        // There is no transaction in progress for this
        // context.  Return an error.
        //

        return( STATUS_RXACT_INVALID_STATE );
    }

    (VOID) RtlFreeHeap( RtlProcessHeap(), 0, RXactContext->RXactLog );

    //
    // Reinitialize the RXactContext structure with the same initial data.
    //

    RXactInitializeContext(
        RXactContext,
        RXactContext->RootRegistryKey,
        RXactContext->RXactKey
        );


    return( STATUS_SUCCESS );

}



NTSTATUS
RtlAddAttributeActionToRXact(
    IN PRTL_RXACT_CONTEXT RXactContext,
    IN RTL_RXACT_OPERATION Operation,
    IN PUNICODE_STRING SubKeyName,
    IN HANDLE KeyHandle OPTIONAL,
    IN PUNICODE_STRING AttributeName,
    IN ULONG NewValueType,
    IN PVOID NewValue,
    IN ULONG NewValueLength
    )

/*++

Routine Description:

    This routine is used to add a new action to the transaction operation log.
    Upon commit, these operations are applied in the order they are added
    to the log.

    This routine differs from RtlAddActionToRXact in that it takes an Attribute
    Name parameter, rather than using the default ("NULL") Attribute of the
    specified key.


Arguments:

    RXactContext - Supplies a pointer to the RXactContext structure for this
        subsystem's root registry key.

    Operation - Indicates the type of operation to perform (e.g., delete
        a sub-key or set the value of a sub-key).  Sub-keys may be created
        by setting a value of a previously non-existent sub-key.  This will
        cause all sub-keys between the root and the specified sub-key to
        be created.

    SubKeyName - Specifies the name of the target registry key.  This name
        is relative to the Root of the Registry transaction sub-tree
        and must NOT start with a delimiter character ("\").

    KeyHandle - Optionally supplies a handle to the target key.  If
        not specified, the name passed for SubKeyName will determine
        the target key.

    AttributeName - Supplies the name of the key attribute to be
        modified.

    NewKeyValueType - (Optional) Contains the KeyValueType to assign
        to the target registry key.  This parameter is ignored if the
        Operation is not RtlRXactOperationSetValue.

    NewKeyValue -  (Optional) Points to a buffer containing the value
        to assign to the specified target registry key.  This parameter
        is ignored if the Operation is not RtlRXactOperationSetValue.

    NewKeyValueLength - Indicates the length (number of bytes) of the
        NewKeyValue buffer.  This parameter is ignored if the Operation
        is not RtlRXactOperationSetValue.


Return Value:

    STATUS_SUCCESS - Indicates the request completed successfully..

    STATUS_INVALID_PARAMETER - Indicates that an unknown Operation
        was requested.

    STATUS_NO_MEMORY - Insufficient memeory was available to complete
        this operation.

    STATUS_UNKNOWN_REVISION - Indicates that a transaction state
        exists for the specified sub-tree, but has a revision level that is
        unknown by this service.


--*/

{

    PRTL_RXACT_LOG   NewLog;
    PRXACT_LOG_ENTRY Base;

    ULONG End;
    ULONG LogEntrySize;
    ULONG NewLogSize;

    RTL_PAGED_CODE();

    //
    // Make sure we were passed a legitimate operation.
    //

    if (  (Operation != RtlRXactOperationDelete)  &&
          (Operation != RtlRXactOperationSetValue)   ) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Compute the total size of the new data
    //

    LogEntrySize = sizeof( RXACT_LOG_ENTRY )               +
                   DwordAlign( SubKeyName->Length )        +
                   DwordAlign( AttributeName->Length )     +
                   DwordAlign( NewValueLength );

    LogEntrySize = ALIGN_UP( LogEntrySize, PVOID );

    //
    // Make sure there is enough space in the current
    // log file for this data.  If not, we must create
    // a larger log, copy all the old data, and then
    // append this to the end.
    //

    if ( RXactContext->RXactLog->LogSizeInUse + LogEntrySize >
                                   RXactContext->RXactLog->LogSize ) {

        //
        // We must allocate a bigger log file.
        //

        NewLogSize = RXactContext->RXactLog->LogSize;

        do {

            NewLogSize = NewLogSize * 2;

        } while ( NewLogSize <
            ( RXactContext->RXactLog->LogSizeInUse + LogEntrySize ) );

        NewLog = RtlAllocateHeap( RtlProcessHeap(), 0, NewLogSize );

        if ( NewLog == NULL ) {
            return( STATUS_NO_MEMORY );
        }

        //
        // Copy over previous information
        //

        RtlMoveMemory( NewLog, RXactContext->RXactLog, RXactContext->RXactLog->LogSizeInUse );

        //
        // Free the old log file
        //

        RtlFreeHeap( RtlProcessHeap(), 0, RXactContext->RXactLog );

        //
        // Install the new log file and adjust its size in its header
        //

        RXactContext->RXactLog = NewLog;
        RXactContext->RXactLog->LogSize = NewLogSize;
    }

    //
    // The log file is big enough, append data to
    // the end.
    //

    Base = (PRXACT_LOG_ENTRY)((PCHAR)(RXactContext->RXactLog) +
                             (RXactContext->RXactLog->LogSizeInUse));


    //
    // Append each parameter to the end of the log.  Unicode string data
    // will be appended to the end of the entry.  The Buffer field in the
    // Unicode string structure will contain the offset to the Buffer,
    // relative to the beginning of the log file.
    //

    Base->LogEntrySize      = LogEntrySize;
    Base->Operation         = Operation;
    Base->SubKeyName        = *SubKeyName;
    Base->AttributeName     = *AttributeName;
    Base->NewKeyValueType   = NewValueType;
    Base->NewKeyValueLength = NewValueLength;
    Base->KeyHandle         = KeyHandle;

    //
    // Fill in the variable length data: SubKeyName, AttributeName,
    // and NewKeyValue
    //

    //
    // End is an offset relative to the beginning of the entire log
    // structure.  It is initialized to 'point' to the offset immediately
    // following the structure we just filled in above.
    //

    End = (ULONG)((RXactContext->RXactLog->LogSizeInUse) +
                 sizeof( *Base ));


    //
    // Append SubKeyName information to the log file
    //

    RtlMoveMemory (
        (PCHAR)(RXactContext->RXactLog) + End,
        SubKeyName->Buffer,
        SubKeyName->Length
        );

    Base->SubKeyName.Buffer = (PWSTR)ULongToPtr(End);
    End += DwordAlign( SubKeyName->Length );



    //
    // Append AttributeName information to the log file
    //


    RtlMoveMemory(
        (PCHAR)(RXactContext->RXactLog) + End,
        AttributeName->Buffer,
        AttributeName->Length
        );

    Base->AttributeName.Buffer = (PWSTR)ULongToPtr(End);
    End += DwordAlign( AttributeName->Length );



    //
    // Append NewKeyValue information (if present) to the log file
    //

    if ( Operation == RtlRXactOperationSetValue ) {

        RtlMoveMemory(
            (PCHAR)(RXactContext->RXactLog) + End,
            NewValue,
            NewValueLength
            );

        Base->NewKeyValue = (PVOID)ULongToPtr(End);
        End += DwordAlign( NewValueLength );
    }

    End = ALIGN_UP( End, PVOID );

    RXactContext->RXactLog->LogSizeInUse = End;
    RXactContext->RXactLog->OperationCount++;

    //
    // We're done
    //

    return(STATUS_SUCCESS);
}


NTSTATUS
RtlAddActionToRXact(
    IN PRTL_RXACT_CONTEXT RXactContext,
    IN RTL_RXACT_OPERATION Operation,
    IN PUNICODE_STRING SubKeyName,
    IN ULONG NewKeyValueType,
    IN PVOID NewKeyValue OPTIONAL,
    IN ULONG NewKeyValueLength
    )

/*++

Routine Description:

    This routine is used to add a new action to the transaction operation log.
    Upon commit, these operations are applied in the order they are added
    to the log.

Arguments:

    RXactContext - Supplies a pointer to the RXactContext structure for this
        subsystem's root registry key.

    Operation - Indicates the type of operation to perform (e.g., delete
        a sub-key or set the value of a sub-key).  Sub-keys may be created
        by setting a value of a previously non-existent sub-key.  This will
        cause all sub-keys between the root and the specified sub-key to
        be created.

    SubKeyName - Specifies the name of the target registry key.  This name
        is relative to the Root of the Registry transaction sub-tree
        and must NOT start with a delimiter character ("\").

    NewKeyValueType - (Optional) Contains the KeyValueType to assign
        to the target registry key.  This parameter is ignored if the
        Operation is not RtlRXactOperationSetValue.

    NewKeyValue -  (Optional) Points to a buffer containing the value
        to assign to the specified target registry key.  This parameter
        is ignored if the Operation is not RtlRXactOperationSetValue.

    NewKeyValueLength - Indicates the length (number of bytes) of the
        NewKeyValue buffer.  This parameter is ignored if the Operation
        is not RtlRXactOperationSetValue.

Return Value:

    STATUS_SUCCESS - Indicates the request completed successfully..


    STATUS_UNKNOWN_REVISION - Indicates that a transaction state
        exists for the specified sub-tree, but has a revision level that is
        unknown by this service.

    Others - Other status values that may be returned from registry key
        services (such as STATUS_ACCESS_DENIED).

--*/
{
    UNICODE_STRING AttributeName;
    NTSTATUS Status;

    RTL_PAGED_CODE();

    RtlInitUnicodeString( &AttributeName, NULL );

    Status = RtlAddAttributeActionToRXact(
                 RXactContext,
                 Operation,
                 SubKeyName,
                 INVALID_HANDLE_VALUE,
                 &AttributeName,
                 NewKeyValueType,
                 NewKeyValue,
                 NewKeyValueLength
                 );

    return( Status );


}



NTSTATUS
RtlApplyRXact(
    IN PRTL_RXACT_CONTEXT RXactContext
    )

/*++

Routine Description:

    This routine is used to apply the changes of a registry sub-tree
    Transaction to that registry sub-tree.  This routine is meant to be
    called for the common case, where the hive is automatically
    lazy-flushed.  That means that this routine must write the change log
    to disk, then flush the hive (to ensure that pieces of changes aren't
    lazy-written to disk before this routine finishes an atomic operation),
    the apply the changes, then delete the change log.

    The actual changes will be lazy-written to disk, but the registry
    guarantees that none or all will make it.  If the machine goes down
    while this routine is executing, the flushed change log guarantees
    that the hive can be put into a consistent state.

Arguments:

    RXactContext - Supplies a pointer to the RXactContext structure for this
        subsystem's root registry key.

Return Value:

    STATUS_SUCCESS - Indicates the transaction was completed.

    STATUS_UNKNOWN_REVISION - Indicates that a transaction state
        exists for the specified sub-tree, but has a revision level that is
        unknown by this service.


    STATUS_RXACT_INVALID_STATE - Indicates that the transaction state
        of the registry sub-tree is incompatible with the requested operation.
        For example, a request to start a new transaction while one is already
        in progress, or a request to apply a transaction when one is not
        currently in progress.  This may also indicate that there is no
        transaction state at all for the specified registry sub-tree.


--*/
{
    NTSTATUS Status;
    UNICODE_STRING LogName;
    HANDLE RXactKey;

    RTL_PAGED_CODE();

    //
    // Commit the contents of the current log to disk
    //

    RXactKey = RXactContext->RXactKey;

    RtlInitUnicodeString( &LogName, RTLP_RXACT_LOG_NAME );

    Status = NtSetValueKey( RXactKey,
                            &LogName,        // ValueName
                            0,               // TitleIndex
                            REG_BINARY,
                            RXactContext->RXactLog,
                            RXactContext->RXactLog->LogSizeInUse
                            );

    if ( !NT_SUCCESS( Status )) {
        return( Status );
    }

    Status = NtFlushKey( RXactKey );

    if ( !NT_SUCCESS( Status )) {

        //
        // If this fails, maintain the in-memory data,
        // but get rid of what we just tried to write
        // to disk.
        //
        // Ignore the error, since we're in a funky
        // state right now.
        //

        (VOID) NtDeleteValueKey( RXactKey, &LogName );

        return( Status );
    }

    //
    // The log is safe, now execute what is in it
    //

    Status = RXactpCommit( RXactContext );

    if ( !NT_SUCCESS( Status )) {

        //
        // As above, try to get rid of what's on
        // disk, leave the in-memory stuff alone,
        // so that the caller may try again.
        //

        (VOID) NtDeleteValueKey( RXactKey, &LogName );

        return( Status );
    }

    //
    // Delete the log file value and data
    //

    Status = NtDeleteValueKey( RXactKey, &LogName );

    //
    // This should never fail
    //

    ASSERT( NT_SUCCESS( Status ));

    //
    // Get rid of the in memory data structures.  Abort
    // does exactly what we want to do.
    //

    Status = RtlAbortRXact( RXactContext );

    //
    // This should never fail
    //

    ASSERT( NT_SUCCESS( Status ));

    return( STATUS_SUCCESS );

}



NTSTATUS
RtlApplyRXactNoFlush(
    IN PRTL_RXACT_CONTEXT RXactContext
    )

/*++

Routine Description:

    This routine is used to apply the changes of a registry sub-tree
    Transaction to that registry sub-tree.  This routine should only be
    called for special hives that do not have automatic lazy-flushing.
    The caller must decide when to flush the hive in order to guarantee
    a consistent hive.

Arguments:

    RXactContext - Supplies a pointer to the RXactContext structure for this
        subsystem's root registry key.

Return Value:

    STATUS_SUCCESS - Indicates the transaction was completed.

    STATUS_UNKNOWN_REVISION - Indicates that a transaction state
        exists for the specified sub-tree, but has a revision level that is
        unknown by this service.


    STATUS_RXACT_INVALID_STATE - Indicates that the transaction state
        of the registry sub-tree is incompatible with the requested operation.
        For example, a request to start a new transaction while one is already
        in progress, or a request to apply a transaction when one is not
        currently in progress.  This may also indicate that there is no
        transaction state at all for the specified registry sub-tree.


--*/
{
    NTSTATUS Status;

    RTL_PAGED_CODE();

    //
    // Execute the contents of the RXACT log.
    //

    Status = RXactpCommit( RXactContext );

    if ( NT_SUCCESS( Status ) ) {

        //
        // Get rid of the in memory data structures.  Abort
        // does exactly what we want to do.
        //

        Status = RtlAbortRXact( RXactContext );

        //
        // This should never fail
        //

        ASSERT( NT_SUCCESS( Status ));
    }

    return( Status );

}



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//    Internal Procedures   (defined in within this file)                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////





NTSTATUS
RXactpCommit(
    IN PRTL_RXACT_CONTEXT RXactContext
    )

/*++

Routine Description:

    This routine commits the operations in the operation log.

    When all changes have been applied, the transaction state
    is changed to NO_TRANSACTION.

Arguments:

    RXactContext - Supplies a pointer to the RXactContext structure for this
        subsystem's root registry key.

Return Value:

    STATUS_SUCCESS - Indicates the transaction was completed.



--*/
{
    BOOLEAN HandlesValid;

    HANDLE TargetKey;
    HANDLE RXactKey;
    HANDLE RootRegistryKey;

    PRTL_RXACT_LOG      RXactLog;
    PRXACT_LOG_ENTRY    RXactLogEntry;
    RTL_RXACT_OPERATION Operation;

    ULONG OperationCount;
    ULONG i;

    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS TmpStatus = STATUS_SUCCESS;
    BOOLEAN CloseTargetKey;

    //
    // Extract information from the RXactContext to simplify
    // the code that follows
    //

    RootRegistryKey = RXactContext->RootRegistryKey;
    RXactKey        = RXactContext->RXactKey;
    RXactLog        = RXactContext->RXactLog;

    OperationCount  = RXactLog->OperationCount;

    HandlesValid    = RXactContext->HandlesValid;


    //
    // Keep a pointer to the beginning of the current log entry.
    //

    RXactLogEntry = (PRXACT_LOG_ENTRY)((PCHAR)RXactLog + sizeof( RTL_RXACT_LOG ));


    //
    // Go through and perform each operation log.  Notice that some operation
    // logs may already have been deleted by a previous commit attempt.
    // So, don't get alarmed if we don't successfully open some operation
    // log entry keys.
    //

    for ( i=0 ; i<OperationCount ; i++ ) {

        //
        // Turn the self-relative offsets in the structure
        // back into real pointers.
        //

        RXactLogEntry->SubKeyName.Buffer = (PWSTR) ((PCHAR)RXactLogEntry->SubKeyName.Buffer +
                                                    (ULONG_PTR)RXactLog);

        RXactLogEntry->AttributeName.Buffer = (PWSTR) ((PCHAR)RXactLogEntry->AttributeName.Buffer +
                                                       (ULONG_PTR)RXactLog);

        RXactLogEntry->NewKeyValue = (PVOID)((PCHAR)RXactLogEntry->NewKeyValue + (ULONG_PTR)RXactLog);

        Operation = RXactLogEntry->Operation;

        //
        // Perform this operation
        //

        switch (Operation) {
            case RtlRXactOperationDelete:

                //
                // Open the target key and delete it.
                // The name is relative to the RootRegistryKey.
                //

                if ( ((RXactLogEntry->KeyHandle == INVALID_HANDLE_VALUE) || !HandlesValid) ) {

                    Status = RXactpOpenTargetKey(
                                 RootRegistryKey,
                                 RtlRXactOperationDelete,
                                 &RXactLogEntry->SubKeyName,
                                 &TargetKey
                                 );

                    if ( !NT_SUCCESS(Status)) {

                        //
                        // We must allow the object not to be found,
                        // because we may be replaying this log after
                        // it had been partially executed.
                        //

                        if ( Status != STATUS_OBJECT_NAME_NOT_FOUND ) {

                            return( Status );

                        } else {

                            break;
                        }
                    }

                    CloseTargetKey = TRUE;

                } else {

                    TargetKey = RXactLogEntry->KeyHandle;
                    CloseTargetKey = FALSE;
                }


                //
                // If this fails, then it is an error
                // because the key should exist at
                // this point.
                //

                Status = NtDeleteKey( TargetKey );


                //
                // Only close the target key if we opened it
                //

                if ( CloseTargetKey ) {

                    TmpStatus = NtClose( TargetKey );

                    //
                    // If we opened this handle, then we should
                    // be able to close it, whether it has been
                    // deleted or not.
                    //

                    ASSERT(NT_SUCCESS(TmpStatus));        // safe to ignore, but curious...
                }


                if (!NT_SUCCESS(Status)) {
                    return(Status);
                }

                break;

            case RtlRXactOperationSetValue:

                //
                // Open the target key.
                // The name is relative to the RootRegistryKey.
                //

                if ( ((RXactLogEntry->KeyHandle == INVALID_HANDLE_VALUE) || !HandlesValid) ) {

                    Status = RXactpOpenTargetKey(
                                 RootRegistryKey,
                                 RtlRXactOperationSetValue,
                                 &RXactLogEntry->SubKeyName,
                                 &TargetKey
                                 );

                    if ( !NT_SUCCESS(Status) ) {
                        return(Status);
                    }

                    CloseTargetKey = TRUE;

                } else {

                    TargetKey = RXactLogEntry->KeyHandle;
                    CloseTargetKey = FALSE;
                }

                //
                // Assign to the target key's new value
                //

                Status = NtSetValueKey( TargetKey,
                                        &RXactLogEntry->AttributeName,
                                        0,               // TitleIndex
                                        RXactLogEntry->NewKeyValueType,
                                        RXactLogEntry->NewKeyValue,
                                        RXactLogEntry->NewKeyValueLength
                                        );

                //
                // Only close the target key if we opened it
                //

                if ( CloseTargetKey ) {

                    TmpStatus = NtClose( TargetKey );
                    ASSERT(NT_SUCCESS(TmpStatus));        // safe to ignore, but curious...

                }

                if ( !NT_SUCCESS(Status) ) {
                    return(Status);
                }

                break;



            default:

                //
                // Unknown operation type.  This should never happen.
                //

                ASSERT( FALSE );

                return(STATUS_INVALID_PARAMETER);

        }

        RXactLogEntry = (PRXACT_LOG_ENTRY)((PCHAR)RXactLogEntry + RXactLogEntry->LogEntrySize);

    }

    //
    // Commit complete
    //

    return( STATUS_SUCCESS );

}




NTSTATUS
RXactpOpenTargetKey(
    IN HANDLE RootRegistryKey,
    IN RTL_RXACT_OPERATION Operation,
    IN PUNICODE_STRING SubKeyName,
    OUT PHANDLE TargetKey
    )

/*++

Routine Description:

    This routine opens the target registry key of an operation.

Arguments:

    RootRegistryKey - A handle to the registry key within whose sub-tree
        a transaction is to be initialized.

    Operation - Indicates what operation is to be performed on the target.
        This will effect how the target is opened.

    OperationNameKey - A handle to the operation log sub-key
        containing the name of the target registry key.

    TargetKey - Receives a handle to the target registry key.

Return Value:

    STATUS_SUCCESS - Indicates the operation log entry was opened.

    STATUS_NO_MEMORY - Ran out of heap.


--*/
{

    NTSTATUS Status;
    OBJECT_ATTRIBUTES TargetKeyAttributes;
    ACCESS_MASK DesiredAccess;
    ULONG Disposition;


    if (Operation == RtlRXactOperationDelete) {

        DesiredAccess = DELETE;

        InitializeObjectAttributes(
            &TargetKeyAttributes,
            SubKeyName,
            OBJ_CASE_INSENSITIVE,
            RootRegistryKey,
            NULL);

//        Status = RtlpNtOpenKey(
//                     TargetKey,
//                     DesiredAccess,
//                     &TargetKeyAttributes,
//                     0);

        Status = NtOpenKey( TargetKey,
                            DesiredAccess,
                            &TargetKeyAttributes
                            );


    } else if (Operation == RtlRXactOperationSetValue) {

        DesiredAccess = KEY_WRITE;

        InitializeObjectAttributes(
            &TargetKeyAttributes,
            SubKeyName,
            OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
            RootRegistryKey,
            NULL);

        Status = NtCreateKey(
                     TargetKey,
                     DesiredAccess,
                     &TargetKeyAttributes,
                     0,
                     NULL,
                     REG_OPTION_NON_VOLATILE,
                     &Disposition
                     );

    } else {
        return STATUS_INVALID_PARAMETER;
    }



    return( Status );

}



//NTSTATUS
//RXactpAssignTargetValue(
//    IN PVOID NewKeyValue,
//    IN ULONG NewKeyValueLength,
//    IN ULONG NewKeyValueType,
//    IN HANDLE TargetKey,
//    IN PUNICODE_STRING AttributeName
//    );


//NTSTATUS
//RXactpAssignTargetValue(
//    IN PVOID NewKeyValue,
//    IN ULONG NewKeyValueLength,
//    IN ULONG NewKeyValueType,
//    IN HANDLE TargetKey,
//    IN PUNICODE_STRING AttributeName
//    )
//
///*++
//
//Routine Description:
//
//    This routine copies the value of an operation log entry to its
//    corresponding target key.  The target key must already be open.
//
//Arguments:
//
//    NewKeyValue - The new value for the key being modified.
//
//    NewKeyValueLength - The size in bytes of the new value information.
//
//    NewKeyValueType - The type of the data for the new key.
//
//    TargetKey - A handle to the target registry key.
//
//    AttributeName - Supplies the name of the key attribute being edited.
//
//Return Value:
//
//    STATUS_SUCCESS - Indicates the value was successfully applied to
//        the target registry key.
//
//    STATUS_NO_MEMORY - ran out of heap.
//
//
//--*/
//{
//    NTSTATUS Status;
//
//    //
//    // Now apply the value to the target key
//    //
//    // Even if there is no key value, we need to do the assign so that
//    // the key value type is assigned.
//    //
//
//    Status = NtSetValueKey( TargetKey,
//                            AttributeName,
//                            0,               // TitleIndex
//                            NewKeyValueType,
//                            NewKeyValue,
//                            NewKeyValueLength
//                            );
//
//
//    return( Status );
//}
