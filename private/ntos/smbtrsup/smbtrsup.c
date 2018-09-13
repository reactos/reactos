/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    smbtrsup.c

Abstract:

    This module contains the code to implement the kernel mode SmbTrace
    component within the LanMan server and redirector.
    The interface between the kernel mode component and the
    server/redirector is found in nt\private\inc\smbtrsup.h
    The interface providing user-level access to SmbTrace is found in
    nt\private\inc\smbtrace.h

Author:

    Peter Gray (w-peterg)   23-March-1992

Revision History:

    Stephan Mueller (t-stephm)   21-July-1992

        Completed, fixed bugs, moved all associated declarations here
        from various places in the server, ported to the redirector
        and converted to a kernel DLL.

--*/

#include <ntsrv.h>
#include <smbtrace.h>     // for names and structs shared with user-mode app

#define _SMBTRSUP_SYS_ 1  // to get correct definitions for exported variables
#include <smbtrsup.h>     // for functions exported to server/redirector

#if DBG
ULONG SmbtrsupDebug = 0;
#define TrPrint(x) if (SmbtrsupDebug) KdPrint(x)
#else
#define TrPrint(x)
#endif

//
// we assume all well-known names are #defined in Unicode, and require
// them to be so: in the SmbTrace application and the smbtrsup.sys package
//
#ifndef UNICODE
#error "UNICODE build required"
#endif


#if DBG
#define PAGED_DBG 1
#endif
#ifdef PAGED_DBG
#undef PAGED_CODE
#define PAGED_CODE() \
    struct { ULONG bogus; } ThisCodeCantBePaged; \
    ThisCodeCantBePaged; \
    if (KeGetCurrentIrql() > APC_LEVEL) { \
        KdPrint(( "SMBTRSUP: Pageable code called at IRQL %d.  File %s, Line %d\n", KeGetCurrentIrql(), __FILE__, __LINE__ )); \
        ASSERT(FALSE); \
        }
#define PAGED_CODE_CHECK() if (ThisCodeCantBePaged) ;
ULONG ThisCodeCantBePaged;
#else
#define PAGED_CODE_CHECK()
#endif


#if PAGED_DBG
#define ACQUIRE_SPIN_LOCK(a, b) {               \
    PAGED_CODE_CHECK();                         \
    KeAcquireSpinLock(a, b);                    \
    }
#define RELEASE_SPIN_LOCK(a, b) {               \
    PAGED_CODE_CHECK();                         \
    KeReleaseSpinLock(a, b);                    \
    }

#else
#define ACQUIRE_SPIN_LOCK(a, b) KeAcquireSpinLock(a, b)
#define RELEASE_SPIN_LOCK(a, b) KeReleaseSpinLock(a, b)
#endif

//
// Increment shared variable in instance data using appropriate interlock
//
#define LOCK_INC_ID(var)                                     \
     ExInterlockedAddUlong( (PULONG)&ID(var),                \
                            1, &ID(var##Interlock) )

//
// Zero shared variable in instance data using appropriate interlock
//
#define LOCK_ZERO_ID(var) {                                  \
     ID(var) = 0;                                            \
     }


//
// The various states SmbTrace can be in.  These states are internal
// only.  The external SmbTraceActive variable contains much less
// detailed information:  it is TRUE when TraceRunning, FALSE in any other
// state.
//
typedef enum _SMBTRACE_STATE {
    TraceStopped,          // not running
    TraceStarting,         // preparing to run
    TraceStartStopFile,    // starting, but want to shut down immediately
                           // because the FileObject closed
    TraceStartStopNull,    // starting, but want to shut down immediately
                           // because a new fsctl came in
    TraceAppWaiting,       // waiting for application to die
    TraceRunning,          // processing SMBs
    TraceStopping          // waiting for smbtrace thread to stop
} SMBTRACE_STATE;


//
// Structure used to hold information regarding an SMB which is put into
// the SmbTrace thread queue.
//
typedef struct _SMBTRACE_QUEUE_ENTRY {
    LIST_ENTRY  ListEntry;      // usual doubly-linked list
    ULONG       SmbLength;      // the length of this SMB
    PVOID       Buffer;         // pointer into SmbTracePortMemoryHeap
                                // or non-paged pool
    PVOID       SmbAddress;     // address of real SMB, if SMB still
                                // available (i.e. if slow mode)
    BOOLEAN     BufferNonPaged; // TRUE if Buffer in non-paged pool, FALSE if
                                // Buffer in SmbTracePortMemoryHeap
                                // Redirector-specific
    PKEVENT     WaitEvent;      // pointer to worker thread event to be
                                // signalled when SMB has been processed
                                // slow mode specific
} SMBTRACE_QUEUE_ENTRY, *PSMBTRACE_QUEUE_ENTRY;


//
// Instance data is specific to the component being traced.  In order
// to unclutter the source code, use the following macro to access
// instance specific data.
// Every exported function either has an explicit parameter (named
// Component) which the caller provides, or is implicitly applicable
// only to one component, and has a local variable named Component
// which is always set to the appropriate value.
//
#define ID(field) (SmbTraceData[Component].field)

//
// Instance data.  The fields which need to be statically initialized
// are declared before those that we don't care to initialize.
//
typedef struct _INSTANCE_DATA {

    //
    // Statically initialized fields.
    //

    //
    // Names for identifying the component being traced in KdPrint messages,
    // and global objects
    //
    PCHAR ComponentName;
    PWSTR SharedMemoryName;
    PWSTR NewSmbEventName;
    PWSTR DoneSmbEventName;

    //
    //  Prevent reinitializing resources if rdr/srv reloaded
    //
    BOOLEAN InstanceInitialized;

    //
    // some tracing parameters, from SmbTrace application
    //
    BOOLEAN SingleSmbMode;
    CLONG   Verbosity;

    //
    // State of the current trace.
    //
    SMBTRACE_STATE TraceState;

    //
    // Pointer to file object of client who started the current trace.
    //
    PFILE_OBJECT StartersFileObject;

    //
    // Fsp process of the component we're tracing in.
    //
    PEPROCESS FspProcess;

    //
    // All subsequent fields are not expliticly statically initiliazed.
    //

    //
    // Current count of number of SMBs lost since last one output.
    // Use an interlock to access, cleared when an SMB is sent to
    // the client successfully.  This lock is used with ExInterlockedXxx
    // routines, so it cannot be treated as a real spin lock (i.e.
    // don't use KeAcquireSpinLock.)
    //
    KSPIN_LOCK SmbsLostInterlock;
    ULONG      SmbsLost;

    //
    // some events, only accessed within the kernel
    //
    KEVENT ActiveEvent;
    KEVENT TerminatedEvent;
    KEVENT TerminationEvent;
    KEVENT AppTerminationEvent;
    KEVENT NeedMemoryEvent;

    //
    // some events, shared with the outside world
    //
    HANDLE NewSmbEvent;
    HANDLE DoneSmbEvent;

    //
    // Handle to the shared memory used for communication between
    // the server/redirector and SmbTrace.
    //
    HANDLE SectionHandle;

    //
    // Pointers to control the shared memory for the SmbTrace application.
    // The port memory heap handle is initialized to NULL to indicate that
    // there is no connection with SmbTrace yet.
    //
    PVOID PortMemoryBase;
    ULONG_PTR PortMemorySize;
    ULONG TableSize;
    PVOID PortMemoryHeap;

    //
    // serialized access to the heap,
    // to allow clean shutdown (StateInterlock)
    //
    KSPIN_LOCK  HeapReferenceCountLock;
    PERESOURCE StateInterlock;
    PERESOURCE HeapInterlock;
    ULONG     HeapReferenceCount;

    //
    // Pointers to the structured data, located in the shared memory.
    //
    PSMBTRACE_TABLE_HEADER  TableHeader;
    PSMBTRACE_TABLE_ENTRY   Table;

    //
    // Fields for the SmbTrace queue.  The server/redirector puts
    // incoming and outgoing SMBs into this queue (when
    // SmbTraceActive[Component] is TRUE and they are processed
    // by the SmbTrace thread.
    //
    LIST_ENTRY Queue;            // The queue itself
    KSPIN_LOCK QueueInterlock;   // Synchronizes access to queue
    KSEMAPHORE QueueSemaphore;   // Counts elements in queue

} INSTANCE_DATA;


#ifdef  ALLOC_DATA_PRAGMA
#pragma data_seg("PAGESMBD")
#endif
//
// Global variables for SmbTrace support
//

INSTANCE_DATA SmbTraceData[] = {

    //
    // Server data
    //

    {
        "Srv",                                 // ComponentName
        SMBTRACE_SRV_SHARED_MEMORY_NAME,       // SharedMemoryName
        SMBTRACE_SRV_NEW_SMB_EVENT_NAME,       // NewSmbEventName
        SMBTRACE_SRV_DONE_SMB_EVENT_NAME,      // DoneSmbEventName

        FALSE,                                 // InstanceInitialized

        FALSE,                                 // SingleSmbMode
        SMBTRACE_VERBOSITY_ERROR,              // Verbosity

        TraceStopped,                          // TraceState

        NULL,                                  // StartersFileObject
        NULL                                   // FspProcess

        // rest of fields expected to get 'all-zeroes'
    },

    //
    // Redirector data
    //

    {
        "Rdr",                                 // ComponentName
        SMBTRACE_LMR_SHARED_MEMORY_NAME,       // SharedMemoryName
        SMBTRACE_LMR_NEW_SMB_EVENT_NAME,       // NewSmbEventName
        SMBTRACE_LMR_DONE_SMB_EVENT_NAME,      // DoneSmbEventName

        FALSE,                                 // InstanceInitialized

        FALSE,                                 // SingleSmbMode
        SMBTRACE_VERBOSITY_ERROR,              // Verbosity

        TraceStopped,                          // TraceState

        NULL,                                  // StartersFileObject
        NULL                                   // FspProcess

        // rest of fields expected to get 'all-zeroes'
    }
};


//
// some state booleans, exported to clients.  For this reason,
// they're stored separately from the rest of the instance data.
// Initially, SmbTrace is neither active nor transitioning.
//
BOOLEAN SmbTraceActive[] = {FALSE, FALSE};
BOOLEAN SmbTraceTransitioning[] = {FALSE, FALSE};

HANDLE
SmbTraceDiscardableCodeHandle = 0;

HANDLE
SmbTraceDiscardableDataHandle = 0;

#ifdef  ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif

//
// Forward declarations of internal routines
//

BOOLEAN
SmbTraceReferenceHeap(
    IN SMBTRACE_COMPONENT Component
    );

VOID
SmbTraceDereferenceHeap(
    IN SMBTRACE_COMPONENT Component
    );

VOID
SmbTraceDisconnect(
    IN SMBTRACE_COMPONENT Component
    );

VOID
SmbTraceEmptyQueue (
    IN SMBTRACE_COMPONENT Component
    );

VOID
SmbTraceThreadEntry(
    IN PVOID Context
    );

NTSTATUS
SmbTraceFreeMemory (
    IN SMBTRACE_COMPONENT Component
    );

VOID
SmbTraceToClient(
    IN PVOID Smb,
    IN CLONG SmbLength,
    IN PVOID SmbAddress,
    IN SMBTRACE_COMPONENT Component
    );

ULONG
SmbTraceMdlLength(
    IN PMDL Mdl
    );

VOID
SmbTraceCopyMdlContiguous(
    OUT PVOID Destination,
    IN  PMDL Mdl,
    IN  ULONG Length
    );

//NTSTATUS
//DriverEntry(
//    IN PDRIVER_OBJECT DriverObject,
//    IN PUNICODE_STRING RegistryPath
//    );

VOID
SmbTraceDeferredDereferenceHeap(
    IN PVOID Context
    );

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, SmbTraceInitialize)
#pragma alloc_text(PAGE, SmbTraceTerminate)
#pragma alloc_text(PAGE, SmbTraceStart)
#pragma alloc_text(PAGE, SmbTraceStop)
#pragma alloc_text(PAGE, SmbTraceCompleteSrv)
#pragma alloc_text(PAGE, SmbTraceDisconnect)
#pragma alloc_text(PAGE, SmbTraceEmptyQueue)
#pragma alloc_text(PAGE, SmbTraceThreadEntry)
#pragma alloc_text(PAGE, SmbTraceFreeMemory)
#pragma alloc_text(PAGE, SmbTraceToClient)
#pragma alloc_text(PAGE, SmbTraceDeferredDereferenceHeap)
#pragma alloc_text(PAGESMBC, SmbTraceCompleteRdr)
#pragma alloc_text(PAGESMBC, SmbTraceReferenceHeap)
#pragma alloc_text(PAGESMBC, SmbTraceDereferenceHeap)
#pragma alloc_text(PAGESMBC, SmbTraceMdlLength)
#pragma alloc_text(PAGESMBC, SmbTraceCopyMdlContiguous)
#endif



//
// Exported routines
//


NTSTATUS
SmbTraceInitialize (
    IN SMBTRACE_COMPONENT Component
    )

/*++

Routine Description:

    This routine initializes the SmbTrace component-specific instance
    globals.  On first-ever invocation, it performs truly global
    initialization.

Arguments:

    Component - Context from which we're called: server or redirector

Return Value:

    NTSTATUS - Indicates failure if unable to allocate resources

--*/

{
    PAGED_CODE();

    if ( ID(InstanceInitialized) == FALSE ) {
        //
        // Component specific initialization -- events and locks.
        //

        KeInitializeEvent( &ID(ActiveEvent), NotificationEvent, FALSE);
        KeInitializeEvent( &ID(TerminatedEvent), NotificationEvent, FALSE);
        KeInitializeEvent( &ID(TerminationEvent), NotificationEvent, FALSE);
        KeInitializeEvent( &ID(AppTerminationEvent), NotificationEvent, FALSE);
        KeInitializeEvent( &ID(NeedMemoryEvent), NotificationEvent, FALSE);

        KeInitializeSpinLock( &ID(SmbsLostInterlock) );
        KeInitializeSpinLock( &ID(HeapReferenceCountLock) );

        ID(StateInterlock) = ExAllocatePoolWithTag(
                                NonPagedPool,
                                sizeof(ERESOURCE),
                                'tbmS'
                                );
        if ( ID(StateInterlock) == NULL ) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        ExInitializeResource( ID(StateInterlock) );

        ID(HeapInterlock) = ExAllocatePoolWithTag(
                                NonPagedPool,
                                sizeof(ERESOURCE),
                                'tbmS'
                                );
        if ( ID(HeapInterlock) == NULL ) {
            ExDeleteResource( ID(StateInterlock) );
            ExFreePool( ID(StateInterlock) );
            ID(StateInterlock) = NULL;
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        ExInitializeResource( ID(HeapInterlock) );

        ID(InstanceInitialized) = TRUE;
    }

    return STATUS_SUCCESS;

} // SmbTraceInitialize


VOID
SmbTraceTerminate (
    IN SMBTRACE_COMPONENT Component
    )

/*++

Routine Description:

    This routine cleans up the SmbTrace component-specific instance
    globals.  It should be called by the component when the component
    is unloaded.

Arguments:

    Component - Context from which we're called: server or redirector

Return Value:

    None

--*/

{
    PAGED_CODE();

    if ( ID(InstanceInitialized) ) {

        ExDeleteResource( ID(StateInterlock) );
        ExFreePool( ID(StateInterlock) );

        ExDeleteResource( ID(HeapInterlock) );
        ExFreePool( ID(HeapInterlock) );

        ID(InstanceInitialized) = FALSE;
    }

    return;

} // SmbTraceTerminate


NTSTATUS
SmbTraceStart (
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength,
    IN OUT PVOID ConfigInOut,
    IN PFILE_OBJECT FileObject,
    IN SMBTRACE_COMPONENT Component
    )

/*++

Routine Description:

    This routine performs all the work necessary to connect the server/
    redirector to SmbTrace.  It creates the section of shared memory to
    be used, then creates the events needed. All these objects are then
    opened by the client (smbtrace) program. This code initializes the
    table, the heap stored in the section and table header.  This routine
    must be called from an Fsp process.

Arguments:

    InputBufferLength - Length of the ConfigInOut packet

    OutputBufferLength - Length expected for the ConfigInOut packet returned

    ConfigInOut - A structure that has configuration information.

    FileObject - FileObject of the process requesting that SmbTrace be started,
                 used to automatically shut down when the app dies.

    Component - Context from which we're called: server or redirector

Return Value:

    NTSTATUS - result of operation.

--*/

// size of our one, particular, ACL
#define ACL_LENGTH  (ULONG)sizeof(ACL) +                 \
                    (ULONG)sizeof(ACCESS_ALLOWED_ACE) +  \
                    sizeof(LUID) +                       \
                    8

{
    NTSTATUS status;
    UNICODE_STRING memoryNameU;

    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    UCHAR Buffer[ACL_LENGTH];
    PACL AdminAcl = (PACL)(&Buffer[0]);
    SECURITY_DESCRIPTOR securityDescriptor;

    UNICODE_STRING eventNameU;
    OBJECT_ATTRIBUTES objectAttributes;
    ULONG i;
    LARGE_INTEGER sectionSize;
    PSMBTRACE_CONFIG_PACKET_REQ  ConfigPacket;
    PSMBTRACE_CONFIG_PACKET_RESP ConfigPacketResp;
    HANDLE threadHandle;

    PAGED_CODE();

    ASSERT( ID(InstanceInitialized) );

    //
    // Validate the buffer lengths passed in.
    //

    if ( ( InputBufferLength  != sizeof( SMBTRACE_CONFIG_PACKET_REQ ) )
      || ( OutputBufferLength != sizeof( SMBTRACE_CONFIG_PACKET_RESP ) )
    ) {

        TrPrint(( "%s!SmbTraceStart: config packet(s) of wrong size!\n",
                  ID(ComponentName) ));

        return STATUS_INFO_LENGTH_MISMATCH;

    }

    ExAcquireResourceExclusive( ID(StateInterlock), TRUE );

    if ( ID(TraceState) != TraceStopped ) {
        ExReleaseResource( ID(StateInterlock) );
        return STATUS_INVALID_DEVICE_STATE;
    }

    ASSERT(!SmbTraceActive[Component]);

    ASSERT (SmbTraceDiscardableDataHandle == NULL);

    ASSERT (SmbTraceDiscardableCodeHandle == NULL);

    SmbTraceDiscardableCodeHandle = MmLockPagableCodeSection(SmbTraceReferenceHeap);

    SmbTraceDiscardableDataHandle = MmLockPagableDataSection(SmbTraceData);

    ID(TraceState) = TraceStarting;

    //
    // Initialize global variables so that we know what to close on errexit
    //

    ID(SectionHandle) = NULL;
    ID(PortMemoryHeap) = NULL;
    ID(NewSmbEvent) = NULL;
    ID(DoneSmbEvent) = NULL;

    //
    // Caution! Both input and output packets are the same, we must
    // read all of the input before we write any output.
    //

    ConfigPacket = (PSMBTRACE_CONFIG_PACKET_REQ) ConfigInOut;
    ConfigPacketResp = (PSMBTRACE_CONFIG_PACKET_RESP) ConfigInOut;

    //
    // Set the mode of operation (read all values).
    //

    ID(SingleSmbMode)  = ConfigPacket->SingleSmbMode;
    ID(Verbosity)      = ConfigPacket->Verbosity;
    ID(PortMemorySize) = ConfigPacket->BufferSize;
    ID(TableSize)      = ConfigPacket->TableSize;

    //
    // Create a security descriptor containing a discretionary Acl
    // allowing administrator access.  This SD will be used to allow
    // Smbtrace access to the shared memory and the notification events.
    //

    // Create Acl allowing administrator access using well-known Sid.

    status = RtlCreateAcl( AdminAcl, ACL_LENGTH, ACL_REVISION2 );
    if ( !NT_SUCCESS(status) ) {
        TrPrint((
            "%s!SmbTraceStart: RtlCreateAcl failed: %X\n",
            ID(ComponentName), status ));
        goto errexit;
    }

    status = RtlAddAccessAllowedAce(
             AdminAcl,
             ACL_REVISION2,
             GENERIC_ALL,
             SeExports->SeAliasAdminsSid
             );
    if ( !NT_SUCCESS(status) ) {
        TrPrint((
            "%s!SmbTraceStart: RtlAddAccessAllowedAce failed: %X\n",
            ID(ComponentName), status ));
        goto errexit;
    }

    // Create SecurityDescriptor containing AdminAcl as a discrectionary ACL.

    RtlCreateSecurityDescriptor(
             &securityDescriptor,
             SECURITY_DESCRIPTOR_REVISION1
             );
    if ( !NT_SUCCESS(status) ) {
        TrPrint((
            "%s!SmbTraceStart: RtlCreateSecurityDescriptor failed: %X\n",
            ID(ComponentName), status ));
        goto errexit;
    }

    status = RtlSetDaclSecurityDescriptor(
             &securityDescriptor,
             TRUE,
             AdminAcl,
             FALSE
             );
    if ( !NT_SUCCESS(status) ) {
        TrPrint((
            "%s!SmbTraceStart: "
            "RtlSetDAclAllowedSecurityDescriptor failed: %X\n",
            ID(ComponentName), status ));
        goto errexit;
    }

    //
    // Create the section to be used for communication between the
    // server/redirector and SmbTrace.
    //

    // Define the object name.

    RtlInitUnicodeString( &memoryNameU, ID(SharedMemoryName) );

    // Define the object information, including security descriptor and name.

    InitializeObjectAttributes(
        &objectAttributes,
        &memoryNameU,
        OBJ_CASE_INSENSITIVE,
        NULL,
        &securityDescriptor
        );

    // Setup the section size.

    sectionSize.QuadPart = ID(PortMemorySize);

    // Create the named section of memory with all of our attributes.

    status = ZwCreateSection(
                &ID(SectionHandle),
                SECTION_MAP_READ | SECTION_MAP_WRITE,
                &objectAttributes,
                &sectionSize,
                PAGE_READWRITE,
                SEC_RESERVE,
                NULL                        // file handle
                );

    if ( !NT_SUCCESS(status) ) {
        TrPrint(( "%s!SmbTraceStart: ZwCreateSection failed: %X\n",
                  ID(ComponentName), status ));
        goto errexit;
    }

    // Now, map it into our address space.

    ID(PortMemoryBase) = NULL;

    status = ZwMapViewOfSection(
                    ID(SectionHandle),
                    NtCurrentProcess(),
                    &ID(PortMemoryBase),
                    0,                        // zero bits (don't care)
                    0,                        // commit size
                    NULL,                     // SectionOffset
                    &ID(PortMemorySize),      // viewSize
                    ViewUnmap,                // inheritDisposition
                    0L,                       // allocation type
                    PAGE_READWRITE            // protection
                    );

    if ( !NT_SUCCESS(status) ) {
        TrPrint(( "%s!SmbTraceStart: NtMapViewOfSection failed: %X\n",
                  ID(ComponentName), status ));
        goto errexit;
    }

    //
    // Set up the shared section memory as a heap.
    //
    // *** Note that the HeapInterlock for the client instance is passed
    //     to the heap manager to be used for serialization of
    //     allocation and deallocation.  It is necessary for the
    //     resource to be allocated FROM NONPAGED POOL externally to the
    //     heap manager, because if we let the heap manager allocate
    //     the resource, if would allocate it from process virtual
    //     memory.
    //

    ID(PortMemoryHeap) = RtlCreateHeap(
                              0,                            // Flags
                              ID(PortMemoryBase),           // HeapBase
                              ID(PortMemorySize),           // ReserveSize
                              PAGE_SIZE,                    // CommitSize
                              ID(HeapInterlock),            // Lock
                              0                             // Reserved
                              );

    //
    // Allocate and initialize the table and its header.
    //

    ID(TableHeader) = RtlAllocateHeap(
                                    ID(PortMemoryHeap), 0,
                                    sizeof( SMBTRACE_TABLE_HEADER )
                                    );

    ID(Table) = RtlAllocateHeap(
                        ID(PortMemoryHeap), 0,
                        sizeof( SMBTRACE_TABLE_ENTRY ) * ID(TableSize)
                        );

    if ( (ID(TableHeader) == NULL) || (ID(Table) == NULL) ) {
        TrPrint((
            "%s!SmbTraceStart: Not enough memory!\n",
            ID(ComponentName) ));

        status = STATUS_NO_MEMORY;

        goto errexit;
    }

    // Initialize the values inside.

    ID(TableHeader)->HighestConsumed = 0;
    ID(TableHeader)->NextFree = 1;
    ID(TableHeader)->ApplicationStop = FALSE;

    for ( i = 0; i < ID(TableSize); i++) {
        ID(Table)[i].BufferOffset = 0L;
        ID(Table)[i].SmbLength = 0L;
    }

    //
    // Create the required event handles.
    //

    // Define the object information.

    RtlInitUnicodeString( &eventNameU, ID(NewSmbEventName) );

    InitializeObjectAttributes(
        &objectAttributes,
        &eventNameU,
        OBJ_CASE_INSENSITIVE,
        NULL,
        &securityDescriptor
        );

    // Open the named object.

    status = ZwCreateEvent(
                &ID(NewSmbEvent),
                EVENT_ALL_ACCESS,
                &objectAttributes,
                NotificationEvent,
                FALSE                        // initial state
                );

    if ( !NT_SUCCESS(status) ) {
        TrPrint(( "%s!SmbTraceStart: ZwCreateEvent (1st) failed: %X\n",
                  ID(ComponentName), status ));

        goto errexit;
    }

    if ( ID(SingleSmbMode) ) {    // this event may not be required.

        // Define the object information.

        RtlInitUnicodeString( &eventNameU, ID(DoneSmbEventName) );

        InitializeObjectAttributes(
            &objectAttributes,
            &eventNameU,
            OBJ_CASE_INSENSITIVE,
            NULL,
            &securityDescriptor
            );

        // Create the named object.

        status = ZwCreateEvent(
                    &ID(DoneSmbEvent),
                    EVENT_ALL_ACCESS,
                    &objectAttributes,
                    NotificationEvent,
                    FALSE                    // initial state
                    );

        if ( !NT_SUCCESS(status) ) {
            TrPrint((
                "%s!SmbTraceStart: NtCreateEvent (2nd) failed: %X\n",
                 ID(ComponentName), status ));
            goto errexit;
        }
        TrPrint(( "%s!SmbTraceStart: DoneSmbEvent handle %x in process %x\n",
                ID(ComponentName), ID(DoneSmbEvent), PsGetCurrentProcess()));

    }

    //
    //  Reset any events that may be in the wrong state from a previous run.
    //

    KeResetEvent(&ID(TerminationEvent));
    KeResetEvent(&ID(TerminatedEvent));

    //
    // Connection was successful, now start the SmbTrace thread.
    //

    //
    // Create the SmbTrace thread and wait for it to finish
    // initializing (at which point SmbTraceActiveEvent is set)
    //

    status = PsCreateSystemThread(
        &threadHandle,
        THREAD_ALL_ACCESS,
        NULL,
        NtCurrentProcess(),
        NULL,
        (PKSTART_ROUTINE) SmbTraceThreadEntry,
        (PVOID)Component
        );

    if ( !NT_SUCCESS(status) ) {

        TrPrint((
            "%s!SmbTraceStart: PsCreateSystemThread failed: %X\n",
            ID(ComponentName), status ));

        goto errexit;
    }

    //
    // Wait until SmbTraceThreadEntry has finished initializing
    //

    (VOID)KeWaitForSingleObject(
            &ID(ActiveEvent),
            UserRequest,
            KernelMode,
            FALSE,
            NULL
            );

    //
    // Close the handle to the process so the object will be
    // destroyed when the thread dies.
    //

    ZwClose( threadHandle );


    //
    // Record who started SmbTrace so we can stop if he dies or otherwise
    // closes this handle to us.
    //

    ID(StartersFileObject) = FileObject;

    //
    // Record caller's process; which is always the appropriate Fsp
    // process.
    //

    ID(FspProcess) = PsGetCurrentProcess();


    //
    // Setup the response packet, since everything worked (write all values).
    //

    ConfigPacketResp->HeaderOffset = (ULONG)
                                ( (ULONG_PTR)ID(TableHeader)
                                - (ULONG_PTR)ID(PortMemoryBase) );

    ConfigPacketResp->TableOffset = (ULONG)
                                ( (ULONG_PTR)ID(Table)
                                - (ULONG_PTR)ID(PortMemoryBase) );

    TrPrint(( "%s!SmbTraceStart: SmbTrace started.\n", ID(ComponentName) ));

    ExReleaseResource( ID(StateInterlock) );

    //
    // if someone wanted it shut down while it was starting, shut it down
    //

    switch ( ID(TraceState) ) {

    case TraceStartStopFile :
        SmbTraceStop( ID(StartersFileObject), Component );
        return STATUS_UNSUCCESSFUL;  // app closed, so we should shut down
        break;

    case TraceStartStopNull :
        SmbTraceStop( NULL, Component );
        return STATUS_UNSUCCESSFUL;  // someone requested a shut down
        break;

    default :
        ID(TraceState) = TraceRunning;
        SmbTraceActive[Component] = TRUE;
        return STATUS_SUCCESS;
    }

errexit:

    SmbTraceDisconnect( Component );

    ID(TraceState) = TraceStopped;

    ExReleaseResource( ID(StateInterlock) );

    //
    // return original failure status code, not success of cleanup
    //

    return status;

} // SmbTraceStart

// constant only of interest while constructing the particular Acl
// in SmbTraceStart
#undef ACL_LENGTH


NTSTATUS
SmbTraceStop(
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN SMBTRACE_COMPONENT Component
    )

/*++

Routine Description:

    This routine stops tracing in the server/redirector.  If no
    FileObject is provided, the SmbTrace application is stopped.
    If a FileObject is provided, SmbTrace is stopped if the
    FileObject refers to the one who started it.

Arguments:

    FileObject - FileObject of a process that terminated.  If it's the process
                 that requested SmbTracing, we shut down automatically.

    Component - Context from which we're called: server or redirector

Return Value:

    NTSTATUS - result of operation.  Possible results are:
        STATUS_SUCCESS - SmbTrace was stopped
        STATUS_UNSUCCESSFUL - SmbTrace was not stopped because the
            provided FileObject did not refer to the SmbTrace starter
            or because SmbTrace was not running.

--*/

{
    PAGED_CODE();

    //
    // If we haven't been initialized, there's nothing to stop.  (And no
    // resource to acquire!)
    //

    if ( !ID(InstanceInitialized) ) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // If it's not the FileObject that started SmbTrace, we don't care.
    // From then on, if ARGUMENT_PRESENT(FileObject) it's the right one.
    //

    if ( ARGUMENT_PRESENT(FileObject) &&
         FileObject != ID(StartersFileObject)
    ) {
       return STATUS_UNSUCCESSFUL;
    }

    ExAcquireResourceExclusive( ID(StateInterlock), TRUE );

    //
    // Depending on the current state of SmbTrace and whether this is
    // a FileObject or unconditional shutdown request, we do different
    // things.  It is always clear at this point, though, that
    // SmbTraceActive should be set to FALSE.
    //

    SmbTraceActive[Component] = FALSE;

    switch ( ID(TraceState) ) {
    case TraceStopped :
    case TraceStopping :
    case TraceStartStopFile :
    case TraceStartStopNull :

        // if we're not running or already in a mode where we know we'll
        // soon be shut down, ignore the request.
        ExReleaseResource( ID(StateInterlock) );
        return STATUS_UNSUCCESSFUL;
        break;

    case TraceStarting :

        // inform starting SmbTrace that it should shut down immediately
        // upon finishing initialization.  It needs to know whether this
        // is a FileObject or unconditional shutdown request.

        ID(TraceState) = ARGUMENT_PRESENT(FileObject)
                       ? TraceStartStopFile
                       : TraceStartStopNull;
        ExReleaseResource( ID(StateInterlock) );
        return STATUS_SUCCESS;
        break;

    case TraceAppWaiting :

        // we're waiting for the application to die already, so ignore
        // new unconditional requests.  But FileObject requests are
        // welcomed.  We cause the SmbTrace thread to kill itself.
        if ( ARGUMENT_PRESENT(FileObject) ) {
            break;  // thread kill code follows switch
        } else {
            ExReleaseResource( ID(StateInterlock) );
            return STATUS_UNSUCCESSFUL;
        }
        break;

    case TraceRunning :

        // if it's a FileObject request, the app is dead, so we cause
        // the SmbTrace thread to kill itself.  Otherwise, we need to
        // signal the app to stop and return.  When the app is gone, we
        // will be called again; this time with a FileObject.

        if ( ARGUMENT_PRESENT(FileObject) ) {
            break;  // thread kill code follows switch
        } else {
            KeSetEvent( &ID(AppTerminationEvent), 2, FALSE );
            ID(TraceState) = TraceAppWaiting;
            ExReleaseResource( ID(StateInterlock) );
            return STATUS_SUCCESS;
        }

        break;

    default :
        ASSERT(!"SmbTraceStop: invalid TraceState");
        break;
    }

    //
    // We reach here from within the switch only in the case where
    // we actually want to kill the SmbTrace thread.  Signal it to
    // wake up, and wait until it terminates.  Signal DoneSmbEvent
    // in case it is currently waiting for the application to signal
    // it in slow mode.
    //

    ID(StartersFileObject) = NULL;

    if ( ID(SingleSmbMode)) {

        BOOLEAN ProcessAttached = FALSE;

        if (PsGetCurrentProcess() != ID(FspProcess)) {
            KeAttachProcess(ID(FspProcess));
            ProcessAttached = TRUE;
        }

        TrPrint(( "%s!SmbTraceStop: Signal DoneSmbEvent, handle %x, process %x.\n",
                    ID(ComponentName), ID(DoneSmbEvent), PsGetCurrentProcess()));
        ZwSetEvent( ID(DoneSmbEvent), NULL );

        if (ProcessAttached) {
            KeDetachProcess();
        }

    }

    TrPrint(( "%s!SmbTraceStop: Signal Termination Event.\n", ID(ComponentName) ));
    ID(TraceState) = TraceStopping;
    KeSetEvent( &ID(TerminationEvent), 2, FALSE );

    ExReleaseResource( ID(StateInterlock) );

    KeWaitForSingleObject(
        &ID(TerminatedEvent),
        UserRequest,
        KernelMode,
        FALSE,
        NULL
        );

    TrPrint(( "%s!SmbTraceStop: Terminated Event is set.\n", ID(ComponentName) ));
    ExAcquireResourceExclusive( ID(StateInterlock), TRUE );

    ID(TraceState) = TraceStopped;

    ExReleaseResource( ID(StateInterlock) );

    TrPrint(( "%s!SmbTraceStop: SmbTrace stopped.\n", ID(ComponentName) ));

    MmUnlockPagableImageSection(SmbTraceDiscardableCodeHandle);

    SmbTraceDiscardableCodeHandle = NULL;

    MmUnlockPagableImageSection(SmbTraceDiscardableDataHandle);

    SmbTraceDiscardableDataHandle = NULL;

    return STATUS_SUCCESS;

} // SmbTraceStop


VOID
SmbTraceCompleteSrv (
    IN PMDL SmbMdl,
    IN PVOID Smb,
    IN CLONG SmbLength
    )

/*++

Routine Description:

    Server version.

    Snapshot an SMB and export it to the SmbTrace application.  How
    this happens is determined by which mode (fast or slow) SmbTracing
    was requested in.  In the server, it is easy to guarantee that when
    tracing, a thread is always executing in the Fsp.

    Fast mode: the SMB is copied into shared memory and an entry for it
    is queued to the server SmbTrace thread, which asynchronously
    passes SMBs to the app.  If there is insufficient memory
    for anything (SMB, queue entry, etc.) the SMB is lost.

    Slow mode: identical to Fast mode except that this thread waits
    until the server SmbTrace thread signals that the app has finished
    processing the SMB.  Because each thread waits until its SMB has
    been completely processed, there is much less chance of running
    out of any resources.

    The SMB is either contained in SmbMdl, or at address Smb with length
    SmbLength.

Arguments:

    SmbMdl - an Mdl containing the SMB.

    Smb - a pointer to the SMB.

    SmbLength - the length of the SMB.

Return Value:

    None

--*/

{
    PSMBTRACE_QUEUE_ENTRY  queueEntry;
    PVOID  buffer;
    SMBTRACE_COMPONENT Component = SMBTRACE_SERVER;
    KEVENT WaitEvent;

    PAGED_CODE();

    //
    // This routine is server specific.
    //

    ASSERT( ID(TraceState) == TraceRunning );
    ASSERT( SmbTraceActive[SMBTRACE_SERVER] );

    //
    // We want either an Mdl, or a pointer and a length, or occasionally,
    // a completely NULL response.
    //

    ASSERT( ( SmbMdl == NULL  &&  Smb != NULL  &&  SmbLength != 0 )
         || ( SmbMdl != NULL  &&  Smb == NULL  &&  SmbLength == 0 )
         || ( SmbMdl == NULL  &&  Smb == NULL  &&  SmbLength == 0 ) );

    //
    // We've taken pains not to be at DPC level and to be in
    // the Fsp context too, for that matter.
    //

    ASSERT( KeGetCurrentIrql() < DISPATCH_LEVEL);
    ASSERT( PsGetCurrentProcess() == ID(FspProcess) );

    //
    // Ensure that SmbTrace really is still active and hence, the
    // shared memory is still around.
    //

    if ( SmbTraceReferenceHeap( Component ) == FALSE ) {
        return;
    }

    //
    // If the SMB is currently in an MDL, we don't yet have the length,
    // which we need, to know how much memory to allocate.
    //

    if ( SmbMdl != NULL ) {
        SmbLength = SmbTraceMdlLength(SmbMdl);
    }

    //
    // If we are in slow mode, then we wait after queuing the SMB
    // to the SmbTrace thread.  If we are set for fast mode we
    // garbage collect in case of no memory.
    //

    if ( ID(SingleSmbMode) ) {
        KeInitializeEvent( &WaitEvent, NotificationEvent, FALSE );
    }

    queueEntry = ExAllocatePoolWithTag( NonPagedPool,
                                        sizeof(SMBTRACE_QUEUE_ENTRY),
                                        'tbmS'
                                        );

    if ( queueEntry == NULL ) {
        // No free memory, this SMB is lost.  Record its loss.
        LOCK_INC_ID(SmbsLost);
        SmbTraceDereferenceHeap( Component );
        return;
    }

    //
    // Allocate the required amount of memory in our heap
    // in the shared memory.
    //

    buffer = RtlAllocateHeap( ID(PortMemoryHeap), 0, SmbLength );

    if ( buffer == NULL ) {
        // No free memory, this SMB is lost.  Record its loss.
        // Very unlikely in slow mode.
        LOCK_INC_ID(SmbsLost);
        ExFreePool( queueEntry );

        if ( !ID(SingleSmbMode) ) {
            //
            // Encourage some garbage collection.
            //
            KeSetEvent( &ID(NeedMemoryEvent), 0, FALSE );
        }

        SmbTraceDereferenceHeap( Component );
        return;
    }

    //
    // Copy the SMB to shared memory pointed to by the queue entry,
    // keeping in mind whether it's in an Mdl or contiguous to begin
    // with, and also preserving the address of the real SMB...
    //

    if ( SmbMdl != NULL ) {
        SmbTraceCopyMdlContiguous( buffer, SmbMdl, SmbLength );
        queueEntry->SmbAddress = SmbMdl;
    } else {
        RtlCopyMemory( buffer, Smb, SmbLength );
        queueEntry->SmbAddress = Smb;
    }

    queueEntry->SmbLength = SmbLength;
    queueEntry->Buffer = buffer;
    queueEntry->BufferNonPaged = FALSE;

    //
    // In slow mode, we want to wait until the SMB has been eaten,
    // in fast mode, we don't want to pass the address of the real
    // SMB along, since the SMB is long gone by the time it gets
    // decoded and printed.
    //

    if ( ID(SingleSmbMode) ) {
        queueEntry->WaitEvent = &WaitEvent;
    } else {
        queueEntry->WaitEvent = NULL;
        queueEntry->SmbAddress = NULL;
    }

    //
    // ...queue the entry to the SmbTrace thread...
    //

    ExInterlockedInsertTailList(
            &ID(Queue),
            &queueEntry->ListEntry,
            &ID(QueueInterlock)
            );

    KeReleaseSemaphore(
            &ID(QueueSemaphore),
            SEMAPHORE_INCREMENT,
            1,
            FALSE
            );

    //
    // ...and wait for the SMB to be eaten, in slow mode.
    //

    if ( ID(SingleSmbMode) ) {
        TrPrint(( "%s!SmbTraceCompleteSrv: Slow mode wait\n", ID(ComponentName) ));
        KeWaitForSingleObject(
            &WaitEvent,
            UserRequest,
            KernelMode,
            FALSE,
            NULL
            );
        TrPrint(( "%s!SmbTraceCompleteSrv: Slow mode wait done\n", ID(ComponentName) ));
    }

    SmbTraceDereferenceHeap( Component );

    return;

} // SmbTraceCompleteSrv


VOID
SmbTraceCompleteRdr (
    IN PMDL SmbMdl,
    IN PVOID Smb,
    IN CLONG SmbLength
    )

/*++

Routine Description:

    Redirector version

    Snapshot an SMB and export it to the SmbTrace application.  How
    this happens is determined by which mode (fast or slow) SmbTracing
    was requested in, and which context (DPC, Fsp or Fsd) the current
    thread is executing in.

    Fast mode: the SMB is copied into shared memory and an entry for it
    is queued to the redirector SmbTrace thread, which asynchronously
    passes SMBs to the app.  (When in DPC, the SMB is copied to non-paged
    pool instead of shared memory, and the SmbTrace thread deals with
    moving it to shared memory later.)  If there is insufficient memory
    for anything (SMB, queue entry, etc.) the SMB is lost.

    Slow mode: identical to Fast mode except that this thread waits
    until the server SmbTrace thread signals that the app has finished
    processing the SMB.  Because each thread waits until its SMB has
    been completely processed, there is much less chance of running
    out of any resources. If at DPC level, we behave exactly as in the
    fast mode case, because it would be a Bad Thing to block this thread
    at DPC level.

    The SMB is either contained in SmbMdl, or at address Smb with length
    SmbLength.

Arguments:

    SmbMdl - an Mdl containing the SMB.

    Smb - a pointer to the SMB.

    SmbLength - the length of the SMB.

Return Value:

    None

--*/

{
    PSMBTRACE_QUEUE_ENTRY  queueEntry;
    PVOID  buffer;
    BOOLEAN ProcessAttached = FALSE;
    BOOLEAN AtDpcLevel;
    SMBTRACE_COMPONENT Component = SMBTRACE_REDIRECTOR;
    KEVENT WaitEvent;

    //
    // This routine is redirector specific.
    //

    ASSERT( ID(TraceState) == TraceRunning );
    ASSERT( SmbTraceActive[SMBTRACE_REDIRECTOR] );

    //
    // We want either an Mdl, or a pointer and a length, or occasionally,
    // a completely NULL response
    //

    ASSERT( ( SmbMdl == NULL  &&  Smb != NULL  &&  SmbLength != 0 )
         || ( SmbMdl != NULL  &&  Smb == NULL  &&  SmbLength == 0 )
         || ( SmbMdl == NULL  &&  Smb == NULL  &&  SmbLength == 0 ) );

    //
    // Ensure that SmbTrace really is still active and hence, the
    // shared memory is still around.
    //

    if ( SmbTraceReferenceHeap( Component ) == FALSE ) {
        return;
    }

    //
    // To avoid multiple system calls, we find out once and for all.
    //

    AtDpcLevel = (BOOLEAN)(KeGetCurrentIrql() >= DISPATCH_LEVEL);

    //
    // If the SMB is currently in an MDL, we don't yet have the length,
    // which we need to know how much memory to allocate.
    //

    if ( SmbMdl != NULL ) {
        SmbLength = SmbTraceMdlLength(SmbMdl);
    }

    //
    // If we are in slow mode, then we wait after queuing the SMB
    // to the SmbTrace thread.  If we are set for fast mode we
    // garbage collect in case of no memory.  If we're at DPC level,
    // we store the SMB in non-paged pool.
    //

    if ( ID(SingleSmbMode) ) {
        KeInitializeEvent( &WaitEvent, NotificationEvent, FALSE );
    }

    //
    // allocate queue entry
    //

    queueEntry = ExAllocatePoolWithTag(
                     NonPagedPool,
                     sizeof(SMBTRACE_QUEUE_ENTRY),
                     'tbmS'
                     );

    if ( queueEntry == NULL ) {
        // No free memory, this SMB is lost.  Record its loss.
        LOCK_INC_ID(SmbsLost);
        SmbTraceDereferenceHeap( Component );
        return;
    }

    //
    // allocate buffer for SMB, in non-paged pool or shared heap as
    // appropriate
    //

    if ( AtDpcLevel ) {

        buffer = ExAllocatePoolWithTag( NonPagedPool, SmbLength, 'tbmS' );
        queueEntry->BufferNonPaged = TRUE;

    } else {

        if ( PsGetCurrentProcess() != ID(FspProcess) ) {
            KeAttachProcess(ID(FspProcess));
            ProcessAttached = TRUE;
        }

        buffer = RtlAllocateHeap( ID(PortMemoryHeap), 0, SmbLength );
        queueEntry->BufferNonPaged = FALSE;

    }

    if ( buffer == NULL ) {

        if ( ProcessAttached ) {
            KeDetachProcess();
        }

        // No free memory, this SMB is lost.  Record its loss.
        LOCK_INC_ID(SmbsLost);

        if (!ID(SingleSmbMode)) {

            //
            // If it was shared memory we ran out of, encourage
            // some garbage collection.
            //
            if ( !queueEntry->BufferNonPaged ) {
                KeSetEvent( &ID(NeedMemoryEvent), 0, FALSE );
            }
        }

        ExFreePool( queueEntry );
        SmbTraceDereferenceHeap( Component );
        return;
    }

    //
    // Copy the SMB to shared or non-paged memory pointed to by the
    // queue entry, keeping in mind whether it's in an Mdl or contiguous
    // to begin with, and also preserving the address of the real SMB...
    //

    if ( SmbMdl != NULL ) {
        SmbTraceCopyMdlContiguous( buffer, SmbMdl, SmbLength );
        queueEntry->SmbAddress = SmbMdl;
    } else {
        RtlCopyMemory( buffer, Smb, SmbLength );
        queueEntry->SmbAddress = Smb;
    }

    if ( ProcessAttached ) {
        KeDetachProcess();
    }

    queueEntry->SmbLength = SmbLength;
    queueEntry->Buffer = buffer;

    //
    // In slow mode, we want to wait until the SMB has been eaten,
    // in fast mode, we don't want to pass the address of the real
    // SMB along, since the SMB is long gone by the time it gets
    // decoded and printed.
    //

    if ( ID(SingleSmbMode) && !AtDpcLevel ) {
        queueEntry->WaitEvent = &WaitEvent;
    } else {
        queueEntry->WaitEvent = NULL;
        queueEntry->SmbAddress = NULL;
    }

    //
    // ...queue the entry to the SmbTrace thread...
    //

    ExInterlockedInsertTailList(
            &ID(Queue),
            &queueEntry->ListEntry,
            &ID(QueueInterlock)
            );

    KeReleaseSemaphore(
            &ID(QueueSemaphore),
            SEMAPHORE_INCREMENT,
            1,
            FALSE
            );

    //
    // ...and wait for the SMB to be eaten, in slow mode.
    //

    if ( ID(SingleSmbMode) && !AtDpcLevel ) {
        TrPrint(( "%s!SmbTraceCompleteRdr: Slow mode wait\n", ID(ComponentName) ));
        KeWaitForSingleObject(
            &WaitEvent,
            UserRequest,
            KernelMode,
            FALSE,
            NULL
            );
        TrPrint(( "%s!SmbTraceCompleteRdr: Slow mode wait done\n", ID(ComponentName) ));
    }

    SmbTraceDereferenceHeap( Component );

    return;

} // SmbTraceCompleteRdr


//
// Internal routines
//


BOOLEAN
SmbTraceReferenceHeap(
    IN SMBTRACE_COMPONENT Component
    )

/*++

Routine Description:

    This routine references the SmbTrace shared memory heap,
    ensuring it isn't disposed of while caller is using it.

Arguments:

    Component - Context from which we're called: server or redirector

Return Value:

    BOOLEAN - TRUE if SmbTrace is still active, and hence
              heap exists and was successfully referenced.
              FALSE otherwise.

--*/

{
    BOOLEAN retval = TRUE;  // assume we'll get it
    KIRQL OldIrql;

    ACQUIRE_SPIN_LOCK( &ID(HeapReferenceCountLock), &OldIrql );

    if ( ID(TraceState) != TraceRunning ) {
        retval = FALSE;
    } else {
        ASSERT( ID(HeapReferenceCount) > 0 );
        ID(HeapReferenceCount)++;
        TrPrint(( "%s!SmbTraceReferenceHeap: Count now %lx\n",
            ID(ComponentName),
            ID(HeapReferenceCount) ));
    }

    RELEASE_SPIN_LOCK( &ID(HeapReferenceCountLock), OldIrql );

    return retval;

} // SmbTraceReferenceHeap

typedef struct _TRACE_DEREFERENCE_ITEM {
    WORK_QUEUE_ITEM WorkItem;
    SMBTRACE_COMPONENT Component;
} TRACE_DEREFERENCE_ITEM, *PTRACE_DEREFERENCE_ITEM;

VOID
SmbTraceDeferredDereferenceHeap(
    IN PVOID Context
    )
/*++

Routine Description:

    If a caller dereferences a heap to 0 from DPC_LEVEL, this routine will
    be called in a system thread to complete the dereference at task time.

Arguments:

    Component - Context from which we're called: server or redirector

Return Value:

    None

--*/

{
    PTRACE_DEREFERENCE_ITEM WorkItem = Context;
    SMBTRACE_COMPONENT Component = WorkItem->Component;

    PAGED_CODE();

    ExFreePool(WorkItem);

    SmbTraceDereferenceHeap(Component);

}


VOID
SmbTraceDereferenceHeap(
    IN SMBTRACE_COMPONENT Component
    )

/*++

Routine Description:

    This routine dereferences the SmbTrace shared memory heap,
    disposing of it when the reference count is zero.

Arguments:

    Component - Context from which we're called: server or redirector

Return Value:

    None

--*/

{
    ULONG oldCount;
    KIRQL OldIrql;

    ACQUIRE_SPIN_LOCK( &ID(HeapReferenceCountLock), &OldIrql );

    if (ID(HeapReferenceCount) > 1) {
        ID(HeapReferenceCount) --;

        TrPrint(( "%s!SmbTraceDereferenceHeap: Count now %lx\n",
            ID(ComponentName),
            ID(HeapReferenceCount) ));

        RELEASE_SPIN_LOCK( &ID(HeapReferenceCountLock), OldIrql );

        return;
    }

    RELEASE_SPIN_LOCK( &ID(HeapReferenceCountLock), OldIrql );

    //
    //  If we are executing at DPC_LEVEL, we cannot dereference the heap
    //  to 0.
    //

    if (KeGetCurrentIrql() >= DISPATCH_LEVEL) {
        PTRACE_DEREFERENCE_ITEM WorkItem;

        WorkItem = ExAllocatePoolWithTag(NonPagedPoolMustSucceed, sizeof(TRACE_DEREFERENCE_ITEM), 'tbmS');

        ExInitializeWorkItem(&WorkItem->WorkItem, SmbTraceDeferredDereferenceHeap, WorkItem);
        WorkItem->Component = Component;

        ExQueueWorkItem(&WorkItem->WorkItem, DelayedWorkQueue);

        return;

    }

    ACQUIRE_SPIN_LOCK( &ID(HeapReferenceCountLock), &OldIrql );

    oldCount = ID(HeapReferenceCount)--;

    TrPrint(( "%s!SmbTraceDereferenceHeap: Count now %lx\n",
        ID(ComponentName),
        ID(HeapReferenceCount) ));

    RELEASE_SPIN_LOCK( &ID(HeapReferenceCountLock), OldIrql );

    if ( oldCount == 1 ) {

        //
        // Free the section, release the handles and such.
        //

        SmbTraceDisconnect( Component );
    }

    return;

} // SmbTraceDereferenceHeap


VOID
SmbTraceDisconnect (
    IN SMBTRACE_COMPONENT Component
    )

/*++

Routine Description:

    This routine reverses all the effects of SmbTraceStart. Mostly,
    it just needs to close certain handles to do this.

Arguments:

    Component - Context from which we're called: server or redirector

Return Value:

    None - always works

--*/

{
    BOOLEAN ProcessAttached = FALSE;

    PAGED_CODE();

    if (PsGetCurrentProcess() != ID(FspProcess)) {
        KeAttachProcess(ID(FspProcess));
        ProcessAttached = TRUE;

    }


    if ( ID(DoneSmbEvent) != NULL ) {
        // Worker thread may be blocked on this, so we set it first
        TrPrint(( "%s!SmbTraceDisconnect: Signal DoneSmbEvent, handle %x, process %x.\n",
                    ID(ComponentName), ID(DoneSmbEvent), PsGetCurrentProcess()));
        ZwSetEvent( ID(DoneSmbEvent), NULL );

        TrPrint(( "%s!SmbTraceDisconnect: Close DoneSmbEvent, handle %x, process %x.\n",
                    ID(ComponentName), ID(DoneSmbEvent), PsGetCurrentProcess()));
        ZwClose( ID(DoneSmbEvent) );
        ID(DoneSmbEvent) = NULL;
    }

    if ( ID(NewSmbEvent) != NULL ) {
        ZwClose( ID(NewSmbEvent) );
        ID(NewSmbEvent) = NULL;
    }

    if ( ID(PortMemoryHeap) != NULL ) {
        RtlDestroyHeap( ID(PortMemoryHeap) );
        ID(PortMemoryHeap) = NULL;
    }

    if ( ID(SectionHandle) != NULL ) {
        ZwClose( ID(SectionHandle) );
        ID(SectionHandle) = NULL;
    }

    if (ProcessAttached) {
        KeDetachProcess();
    }

    return;

} // SmbTraceDisconnect


VOID
SmbTraceEmptyQueue (
    IN SMBTRACE_COMPONENT Component
    )

/*++

Routine Description:

    This routine empties the queue of unprocessed SMBs.

Arguments:

    Component - Context from which we're called: server or redirector

Return Value:

    None - always works

--*/

{
    PLIST_ENTRY            listEntry;
    PSMBTRACE_QUEUE_ENTRY  queueEntry;

    PAGED_CODE();

    while ( ( listEntry = ExInterlockedRemoveHeadList(
                              &ID(Queue),
                              &ID(QueueInterlock)
                              )
            ) != NULL
    ) {
        queueEntry = CONTAINING_RECORD(
                          listEntry,
                          SMBTRACE_QUEUE_ENTRY,
                          ListEntry
                          );

        //
        // If data for this entry is in non-paged pool, free it too.
        // This only ever happens in the redirector.
        //

        if ( queueEntry->BufferNonPaged ) {

            ASSERT( Component == SMBTRACE_REDIRECTOR );

            ExFreePool( queueEntry->Buffer );
        }

        //
        // If a worker thread is waiting on this event, let it go.
        // This only ever happens in slow mode.
        //

        if ( queueEntry->WaitEvent != NULL ) {

            ASSERT( ID(SingleSmbMode) == TRUE );

            KeSetEvent( queueEntry->WaitEvent, 0, FALSE );
        }

        ExFreePool( queueEntry );
    }

    return;

} // SmbTraceEmptyQueue


VOID
SmbTraceThreadEntry (
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is the entry point of the SmbTrace thread for the server/
    redirector.  It is started by SmbTraceStart. This thread loops
    continuously until the client SmbTrace dies or another SmbTrace sends
    an FsCtl to stop the trace.

Arguments:

    Context - pointer to context block containing component from which
              we're called: server or redirector

Return Value:

    None

--*/

// we wait for termination, work-to-do and need-memory events
#define NUMBER_OF_BLOCKING_OBJECTS 4

// keep these definitions in sync

#define INDEX_WAIT_TERMINATIONEVENT     0
#define INDEX_WAIT_APPTERMINATIONEVENT  1
#define INDEX_WAIT_NEEDMEMORYEVENT      2
#define INDEX_WAIT_QUEUESEMAPHORE       3

#define STATUS_WAIT_TERMINATIONEVENT    STATUS_WAIT_0
#define STATUS_WAIT_APPTERMINATIONEVENT STATUS_WAIT_1
#define STATUS_WAIT_NEEDMEMORYEVENT     STATUS_WAIT_2
#define STATUS_WAIT_QUEUESEMAPHORE      STATUS_WAIT_3

{
    NTSTATUS status;
    PLIST_ENTRY listEntry;
    PSMBTRACE_QUEUE_ENTRY    queueEntry;
    PVOID buffer;
    PVOID waitObjects[NUMBER_OF_BLOCKING_OBJECTS];
    SMBTRACE_COMPONENT Component;
    BOOLEAN Looping;

#if NUMBER_OF_BLOCKING_OBJECTS > THREAD_WAIT_OBJECTS
    //
    // If we try to wait on too many objects, we need to allocate
    // our own wait blocks.
    //

    KWAIT_BLOCK waitBlocks[NUMBER_OF_BLOCKING_OBJECTS];
#endif

    PAGED_CODE();

    //
    // Context is really just the component
    //
    Component = (SMBTRACE_COMPONENT)(UINT_PTR)Context;

    //
    // Initialize the queue.
    //

    InitializeListHead(    &ID(Queue) );
    KeInitializeSpinLock(  &ID(QueueInterlock) );
    KeInitializeSemaphore( &ID(QueueSemaphore), 0, 0x7FFFFFFF );

    //
    // Set up the array of objects to wait on.  We wait (in order)
    // for our termination event, the appliction termination event,
    // a no shared memory event or an SMB request to show up in the
    // SmbTrace queue.
    //

    waitObjects[INDEX_WAIT_TERMINATIONEVENT]    = &ID(TerminationEvent);
    waitObjects[INDEX_WAIT_APPTERMINATIONEVENT] = &ID(AppTerminationEvent);
    waitObjects[INDEX_WAIT_NEEDMEMORYEVENT]     = &ID(NeedMemoryEvent);
    waitObjects[INDEX_WAIT_QUEUESEMAPHORE]      = &ID(QueueSemaphore);

    //
    // No SMBs have been lost yet, and this thread is the first user
    // of the shared memory.  It's also a special user in that it gets
    // access before TraceState == TraceRunning, a requirement for all
    // subsequent referencers.
    //

    ID(SmbsLost) = 0L;
    ID(HeapReferenceCount) = 1;

    //
    // Signal to the FSP that we are ready to start capturing SMBs.
    //

    KeSetEvent( &ID(ActiveEvent), 0, FALSE );

    //
    // Main loop, executed until the thread is terminated.
    //

    TrPrint(( "%s!SmbTraceThread: Tracing started.\n", ID(ComponentName) ));

    Looping = TRUE;
    while( Looping ) {

        TrPrint(( "%s!SmbTraceThread: WaitForMultiple.\n", ID(ComponentName) ));
        status = KeWaitForMultipleObjects(
                    NUMBER_OF_BLOCKING_OBJECTS,
                    &waitObjects[0],
                    WaitAny,
                    UserRequest,
                    KernelMode,
                    FALSE,
                    NULL,
#if NUMBER_OF_BLOCKING_OBJECTS > THREAD_WAIT_OBJECTS
                    &waitBlocks[0]
#else
                    NULL
#endif
                    );

        if ( !NT_SUCCESS(status) ) {
            TrPrint((
                "%s!SmbTraceThreadEntry: KeWaitForMultipleObjectsfailed: %X\n",
                ID(ComponentName), status ));
        } else {
            TrPrint((
                "%s!SmbTraceThreadEntry: %lx\n",
                ID(ComponentName), status ));
        }

        switch( status ) {

        case STATUS_WAIT_TERMINATIONEVENT:

            //
            // Stop looping, and then proceed to clean up and die.
            //

            Looping = FALSE;
            break;

        case STATUS_WAIT_APPTERMINATIONEVENT:

            //  Turn off the event so we don't go in a tight loop
            KeResetEvent(&ID(AppTerminationEvent));

            //
            // Inform the app that it is time to die.  The NULL SMB
            // sent here may not be the next to be processed by the
            // app, but the ApplicationStop bit will be detected
            // immediately.
            //

            ID(TableHeader)->ApplicationStop = TRUE;
            SmbTraceToClient( NULL, 0, NULL, Component );

            break;

        case STATUS_WAIT_NEEDMEMORYEVENT:

            //  Turn off the event so we don't go in a loop.
            KeResetEvent(&ID(NeedMemoryEvent));
            //
            // Do a garbage collection, freeing all memory that is
            // allocated in the shared memory but that has been read
            // by the client.
            //

            SmbTraceFreeMemory( Component );

            break;

        case STATUS_WAIT_QUEUESEMAPHORE:

            //
            // If any get through once we've gone into AppWaiting
            // state, don't bother sending them on, they're not
            // going to get processed.
            //

            if ( ID(TraceState) == TraceAppWaiting ) {
                SmbTraceEmptyQueue( Component );
                break;
            }

            //
            // Remove the first element in the our queue.  A
            // work item is represented by our header followed by
            // an SMB. We must free the entry after we are done
            // with it.
            //

            listEntry = ExInterlockedRemoveHeadList(
                            &ID(Queue),
                            &ID(QueueInterlock)
                            );

            if ( listEntry != NULL ) {

                //
                // Get the address of the queue entry.
                //

                queueEntry = CONTAINING_RECORD(
                                  listEntry,
                                  SMBTRACE_QUEUE_ENTRY,
                                  ListEntry
                                  );

                //
                // If the data is in non-paged pool, move it to shared
                // memory and free the non-paged pool before passing
                // the SMB to the client.  Note that in this case,
                // there's no need to signal anyone.  They ain't waiting.
                //

                if ( queueEntry->BufferNonPaged ) {

                    //
                    // Server never uses non-paged pool.
                    //

                    ASSERT( Component != SMBTRACE_SERVER );

                    buffer = RtlAllocateHeap( ID(PortMemoryHeap), 0,
                                              queueEntry->SmbLength );

                    if ( buffer == NULL ) {

                        LOCK_INC_ID(SmbsLost);

                        ExFreePool( queueEntry->Buffer );
                        ExFreePool( queueEntry );

                        break;

                    }

                    RtlCopyMemory( buffer, queueEntry->Buffer,
                                   queueEntry->SmbLength );

                    ExFreePool( queueEntry->Buffer );

                    //
                    // Send it off.  Because the original SMB is long
                    // dead, we don't pass its real address along (not
                    // that we have it, anyway.)
                    //

                    ASSERT( queueEntry->SmbAddress == NULL );

                    SmbTraceToClient(
                            buffer,
                            queueEntry->SmbLength,
                            NULL,
                            Component
                            );

                } else {

                    //
                    // Enter the SMB into the table and send it to the
                    // client. Can block in slow mode.  When it does so, we'll
                    // signal the applicable thread.
                    //

                    SmbTraceToClient(
                            queueEntry->Buffer,
                            queueEntry->SmbLength,
                            queueEntry->SmbAddress,
                            Component
                            );

                    if ( queueEntry->WaitEvent != NULL ) {
                        KeSetEvent( queueEntry->WaitEvent, 0, FALSE );
                    }
                }

                //
                // Now, we must free the queue entry.
                //

                ExFreePool( queueEntry );

            }

            break;

        default:
            break;
        }

    }

    //
    // Clean up!
    //
    TrPrint(( "%s!SmbTraceThread: Tracing clean up.\n", ID(ComponentName) ));

    SmbTraceDereferenceHeap( Component );

    SmbTraceEmptyQueue( Component );

    //
    // Signal to SmbTraceStop that we're dying.
    //

    TrPrint(( "%s!SmbTraceThread: Tracing terminated.\n", ID(ComponentName) ));

    KeSetEvent( &ID(TerminatedEvent), 0, FALSE );

    //
    // Kill this thread.
    //

    status = PsTerminateSystemThread( STATUS_SUCCESS );

    // Shouldn't get here
    TrPrint((
        "%s!SmbTraceThreadEntry: PsTerminateSystemThread() failed: %X\n",
        ID(ComponentName), status ));

} // SmbTraceThreadEntry

// constant only of interest while constructing waitObject arrays
// in SmbTraceThreadEntry
#undef NUMBER_OF_BLOCKING_OBJECTS


NTSTATUS
SmbTraceFreeMemory (
    IN SMBTRACE_COMPONENT Component
    )

/*++

Routine Description:

    This procedure frees any memory that may have been allocated to an
    SMB that the client has already consumed. It does not alter table
    entries, except to record that the memory buffer has been cleared.
    This routinue is not espectally fast, it should not be called often,
    only when needed.

Arguments:

    Component - Context from which we're called: server or redirector

Return Value:

    NTSTATUS - result of operation.

--*/

{
    PVOID    buffer;
    PSMBTRACE_TABLE_ENTRY    tableEntry;
    ULONG    tableIndex;

    PAGED_CODE();

    TrPrint(( "%s!SmbTraceFreeMemory: Called for garbage collection.\n",
              ID(ComponentName) ));

    //
    // No free memory in the heap, perhaps we can free some by freeing
    // memory in old table entries. This is expensive for time.
    //

    tableIndex = ID(TableHeader)->NextFree;

    while( tableIndex != ID(TableHeader)->HighestConsumed ) {

        tableEntry = ID(Table) + tableIndex;

        //
        // Check if this table entry has been used but its memory has not
        // been freed yet. If so, free it.
        //

        if ( tableEntry->BufferOffset != 0L ) {

            buffer = (PVOID)( (ULONG_PTR)tableEntry->BufferOffset
                        + (ULONG_PTR)ID(PortMemoryBase) );

            RtlFreeHeap( ID(PortMemoryHeap), 0, buffer);

            tableEntry->BufferOffset = 0L;
        }


        tableIndex = (tableIndex + 1) % ID(TableSize);
    }

    return( STATUS_SUCCESS );

} // SmbTraceFreeMemory


VOID
SmbTraceToClient(
    IN PVOID Smb,
    IN CLONG SmbLength,
    IN PVOID SmbAddress,
    IN SMBTRACE_COMPONENT Component
    )

/*++

Routine Description:

    Enter an SMB already found in shared memory into the table.  Set
    an event for the client.  If there is no table space, the SMB is
    not saved.  If in slow mode, wait for the client to finish with
    and then free the memory occupied by the SMB.

Arguments:

    Smb - a pointer to the SMB (which is ALREADY in shared memory).
          Can be NULL, indicating no new SMB is to be added, but the
          application is to be signalled anyway.

    SmbLength - the length of the SMB.

    SmbAddress - the address of the real SMB, not in shared memory.

    Component - Context from which we're called: server or redirector

Return Value:

    None

--*/

{
    NTSTATUS status;
    PVOID    buffer;
    PSMBTRACE_TABLE_ENTRY    tableEntry;
    ULONG    tableIndex;

    PAGED_CODE();

    //
    //  Reset DoneSmbEvent so we can determine when the request has been processed
    //

    if ( ID(SingleSmbMode) ) {
        PKEVENT DoneEvent;

        TrPrint(( "%s!SmbTraceToClient: Reset DoneSmbEvent, handle %x, process %x.\n",
                    ID(ComponentName), ID(DoneSmbEvent), PsGetCurrentProcess()));

        status = ObReferenceObjectByHandle( ID(DoneSmbEvent),
                                            EVENT_MODIFY_STATE,
                                            NULL,
                                            KernelMode,
                                            (PVOID *)&DoneEvent,
                                            NULL
                                            );

        ASSERT ( NT_SUCCESS(status) );

        KeResetEvent(DoneEvent);

        ObDereferenceObject(DoneEvent);
    }

    if (Smb != NULL) {

        //
        // See if there is room in the table for a pointer to our SMB.
        //

        if ( ID(TableHeader)->NextFree == ID(TableHeader)->HighestConsumed ) {
            // Tough luck. No memory in the table, this SMB is lost.
            LOCK_INC_ID( SmbsLost );
            RtlFreeHeap( ID(PortMemoryHeap), 0, Smb );
            return;
        }

        tableIndex = ID(TableHeader)->NextFree;

        tableEntry = ID(Table) + tableIndex;

        //
        // Record the number of SMBs that were lost before this one and
        // (maybe) zero the count for the next one.
        //

        tableEntry->NumberMissed = ID(SmbsLost);

        if ( tableEntry->NumberMissed != 0 ) {
            LOCK_ZERO_ID(SmbsLost);
        }

        //
        // Check if this table entry has been used but its memory has not
        // been freed yet. If so, free it.
        //
        if ( tableEntry->BufferOffset != 0L ) {

            buffer = (PVOID)( (ULONG_PTR)tableEntry->BufferOffset
                        + (ULONG_PTR)ID(PortMemoryBase) );

            RtlFreeHeap( ID(PortMemoryHeap), 0, buffer);
            tableEntry->BufferOffset = 0L;
        }

        //
        // Record the location and size of this SMB in the table.
        //

        tableEntry->BufferOffset = (ULONG)((ULONG_PTR)Smb - (ULONG_PTR)ID(PortMemoryBase));
        tableEntry->SmbLength = SmbLength;

        //
        // Record the real address of the actual SMB (i.e. not the shared
        // memory copy) if it's available.
        //

        tableEntry->SmbAddress = SmbAddress;

        //
        // Increment the Next Free counter.
        //

        ID(TableHeader)->NextFree = (tableIndex + 1) % ID(TableSize);

    }


    //
    // Unlock the client so it will process this new SMB.
    //

    TrPrint(( "%s!SmbTraceToClient: Set NewSmbEvent.\n", ID(ComponentName) ));
    status = ZwSetEvent( ID(NewSmbEvent), NULL );

    //
    //  When stopping the trace we set TraceState to TraceStopping and then
    //  DoneSmbEvent. This prevents this routine from blocking indefinitely
    //  because it Resets DoneSmbEvent processes the Smb and then checks TraceState
    //  before blocking.
    //
    if (( ID(SingleSmbMode) ) &&
        ( ID(TraceState) == TraceRunning )) {

        //
        // Wait for the app to acknowledge that the SMB has been
        // processed.
        //

        TrPrint(( "%s!SmbTraceToClient: Waiting for DoneSmbEvent, handle %x, process %x.\n",
                    ID(ComponentName), ID(DoneSmbEvent), PsGetCurrentProcess()));
        status = ZwWaitForSingleObject(
                    ID(DoneSmbEvent),
                    FALSE,
                    NULL
                    );

        TrPrint(( "%s!SmbTraceToClient: DoneSmbEvent is set, handle %x, process %x.\n",
                    ID(ComponentName), ID(DoneSmbEvent), PsGetCurrentProcess()));
        ASSERT( NT_SUCCESS(status) );

        if (Smb != NULL) {

            tableEntry->BufferOffset = 0L;
            RtlFreeHeap( ID(PortMemoryHeap), 0, Smb);
        }

    }

    return;

} // SmbTraceToClient


ULONG
SmbTraceMdlLength(
    IN PMDL Mdl
    )

/*++

Routine Description:

    Determine the total number of bytes of data found in an Mdl.

Arguments:

    Mdl - a pointer to an Mdl whose length is to be calculated

Return Value:

    ULONG - total number of data bytes in Mdl

--*/

{
    ULONG Bytes = 0;

    while (Mdl != NULL) {
        Bytes += MmGetMdlByteCount(Mdl);
        Mdl = Mdl->Next;
    }

    return Bytes;
} // SmbTraceMdlLength


VOID
SmbTraceCopyMdlContiguous(
    OUT PVOID Destination,
    IN  PMDL Mdl,
    IN  ULONG Length
    )

/*++

Routine Description:

    Copy the data stored in Mdl into the contiguous memory at
    Destination.  Length is present to keep the same interface
    as RtlCopyMemory.

Arguments:

    Destination - a pointer to previously allocated memory into which
                  the Mdl is to be copied.

    Mdl - a pointer to an Mdl which is to be copied to Destination

    Length - number of data bytes expected in Mdl

Return Value:

    None

--*/

{
    PCHAR Dest = Destination;

    UNREFERENCED_PARAMETER(Length);

    while (Mdl != NULL) {

        RtlCopyMemory(
            Dest,
            MmGetSystemAddressForMdl(Mdl),
            MmGetMdlByteCount(Mdl)
            );

        Dest += MmGetMdlByteCount(Mdl);
        Mdl = Mdl->Next;
    }

    ASSERT((ULONG)(Dest - (PCHAR)Destination) == Length);

    return;

} // SmbTraceCopyMdlContiguous

