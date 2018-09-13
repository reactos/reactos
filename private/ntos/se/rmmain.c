/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    rmmain.c

Abstract:

    Security Reference Monitor - Init, Control and State Change

Author:

    Scott Birrell       (ScottBi)       March 12, 1991

Environment:

Revision History:

--*/

#include <nt.h>
#include <ntlsa.h>
#include "sep.h"
#include <zwapi.h>
#include "rmp.h"
#include "adt.h"
#include "adtp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,SepRmInitPhase0)
#pragma alloc_text(INIT,SeRmInitPhase1)
#pragma alloc_text(PAGE,SepRmCommandServerThread)
#pragma alloc_text(PAGE,SepRmCommandServerThreadInit)
#pragma alloc_text(PAGE,SepRmComponentTestCommandWrkr)
#pragma alloc_text(PAGE,SepRmSendCommandToLsaWrkr)
#pragma alloc_text(PAGE,SepRmCallLsa)
#endif

//
// Reference Monitor Command Worker Table
//

//
// Keep this in sync with RM_COMMAND_NUMBER in ntrmlsa.h
//

SEP_RM_COMMAND_WORKER SepRmCommandDispatch[] = {
                          SepRmComponentTestCommandWrkr,
                          SepRmSetAuditEventWrkr,
                          SepRmSendCommandToLsaWrkr,
                          SepRmComponentTestCommandWrkr,
                          SepRmCreateLogonSessionWrkr,
                          SepRmDeleteLogonSessionWrkr
                          };


BOOLEAN
SeRmInitPhase1(
    )

/*++

Routine Description:

    This function is called by Phase 1 System Initialization to initialize
    the Security Reference Monitor.  Note that initialization of the
    Reference Monitor Global State has already been performed in Phase 0
    initialization to allow access validation routines to operate without
    having to check that Reference Monitor Initialization is complete.

    The steps listed below are performed in this routine.  The remainder
    of Reference Monitor initialization requires the LSA subsystem to have run,
    so that initialization is performed in a separate thread (the RM Command
    Server Thread, see below), so that the present thread can create the
    Session Manager which execs the LSA.

    o Create the Reference Monitor Command LPC port.  The LSA subsystem sends
      commands (e.g. turn on auditing) which change the Reference Monitor
      Global State.
    o Create an Event for use in synchronizing with the LSA subsystem.  The
      LSA will signal the event when the portion of LSA initialization upon
      with the Reference Monitor depends is complete.  The Reference Monitor
      uses another LPC port, called the LSA Command Port to send commands
      to the LSA, so the RM must know that this port has been created before
      trying to connect to it.
    o Create the Reference Monitor Command Server Thread.  This thread is
      a permanent thread of the System Init process that fields the Reference
      Monitor State Change commands described above.


Arguments:

    None.

Return Value:

    BOOLEAN - TRUE if Rm Initialization (Phase 1) succeeded, else FALSE

--*/

{
    NTSTATUS Status;
    STRING RmCommandPortName;
    UNICODE_STRING UnicodeRmCommandPortName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    STRING LsaInitEventName;
    UNICODE_STRING UnicodeLsaInitEventName;
    OBJECT_ATTRIBUTES LsaInitEventObjectAttributes;
    SECURITY_DESCRIPTOR LsaInitEventSecurityDescriptor;
    ULONG AclSize;

    PAGED_CODE();

    //
    // Create an LPC port called the Reference Monitor Command Port.
    // This will be used by the LSA to send commands to the Reference
    // Monitor to update its state data.
    //

    RtlInitString( &RmCommandPortName, "\\SeRmCommandPort" );
    Status = RtlAnsiStringToUnicodeString(
                 &UnicodeRmCommandPortName,
                 &RmCommandPortName,
                 TRUE );  ASSERT( NT_SUCCESS(Status) );
    InitializeObjectAttributes(
        &ObjectAttributes,
        &UnicodeRmCommandPortName,
        0,
        NULL,
        NULL
        );

    Status = ZwCreatePort(
                 &SepRmState.RmCommandPortHandle,
                 &ObjectAttributes,
                 sizeof(SEP_RM_CONNECT_INFO),
                 sizeof(RM_COMMAND_MESSAGE),
                 sizeof(RM_COMMAND_MESSAGE) * 32
                 );
    RtlFreeUnicodeString( &UnicodeRmCommandPortName );

    if( !NT_SUCCESS(Status) ) {

        KdPrint(("Security: Rm Create Command Port failed 0x%lx\n", Status));
        return FALSE;
    }

    //
    // Prepare to create an event for synchronizing with the LSA.
    // First, build the Security Descriptor for the Init Event Object
    //

    Status = RtlCreateSecurityDescriptor(
                 &LsaInitEventSecurityDescriptor,
                 SECURITY_DESCRIPTOR_REVISION
                 );

    if (!NT_SUCCESS(Status)) {

        KdPrint(("Security:  Creating Lsa Init Event Desc failed 0x%lx\n",
                  Status));
        return FALSE;
    }

    //
    // Allocate a temporary buffer from the paged pool.  It is a fatal
    // system error if the allocation fails since security cannot be
    // enabled.
    //

    AclSize = sizeof(ACL) +
              sizeof(ACCESS_ALLOWED_ACE) +
              SeLengthSid(SeLocalSystemSid);
    LsaInitEventSecurityDescriptor.Dacl =
        ExAllocatePoolWithTag(PagedPool, AclSize, 'cAeS');

    if (LsaInitEventSecurityDescriptor.Dacl == NULL) {

        KdPrint(("Security LSA:  Insufficient resources to initialize\n"));
        return FALSE;
    }

    //
    // Now create the Discretionary ACL within the Security Descriptor
    //

    Status = RtlCreateAcl(
                 LsaInitEventSecurityDescriptor.Dacl,
                 AclSize,
                 ACL_REVISION2
                 );

    if (!NT_SUCCESS(Status)) {

        KdPrint(("Security:  Creating Lsa Init Event Dacl failed 0x%lx\n",
                  Status));
        return FALSE;
    }

    //
    // Now add an ACE giving GENERIC_ALL access to the User ID
    //

    Status = RtlAddAccessAllowedAce(
                 LsaInitEventSecurityDescriptor.Dacl,
                 ACL_REVISION2,
                 GENERIC_ALL,
                 SeLocalSystemSid
                 );

    if (!NT_SUCCESS(Status)) {

        KdPrint(("Security:  Adding Lsa Init Event ACE failed 0x%lx\n",
                  Status));
        return FALSE;
    }

    //
    // Set up the Object Attributes for the Lsa Initialization Event
    //

    RtlInitString( &LsaInitEventName, "\\SeLsaInitEvent" );
    Status = RtlAnsiStringToUnicodeString(
                 &UnicodeLsaInitEventName,
                 &LsaInitEventName,
                 TRUE );  ASSERT( NT_SUCCESS(Status) );
    InitializeObjectAttributes(
        &LsaInitEventObjectAttributes,
        &UnicodeLsaInitEventName,
        0,
        NULL,
        &LsaInitEventSecurityDescriptor
        );

    //
    // Create an event for use in synchronizing with the LSA.  The LSA will
    // signal this event when LSA initialization has reached the point
    // where the LSA's Reference Monitor Server Port has been created.
    //

    Status = ZwCreateEvent(
                 &(SepRmState.LsaInitEventHandle),
                 EVENT_MODIFY_STATE,
                 &LsaInitEventObjectAttributes,
                 NotificationEvent,
                 FALSE);

    RtlFreeUnicodeString( &UnicodeLsaInitEventName );

    if (!NT_SUCCESS(Status)) {

        KdPrint(("Security: LSA init event creation failed.0x%xl\n",
            Status));
        return FALSE;
    }

    //
    // Deallocate the pool memory used for the Init Event DACL
    //

    ExFreePool( LsaInitEventSecurityDescriptor.Dacl );

    //
    // Create a permanent thread of the Sysinit Process, called the
    // Reference Monitor Server Thread.  This thread is dedicated to
    // receiving Reference Monitor commands and dispatching them.
    //

    Status = PsCreateSystemThread(
                 &SepRmState.SepRmThreadHandle,
                 THREAD_GET_CONTEXT |
                 THREAD_SET_CONTEXT |
                 THREAD_SET_INFORMATION,
                 NULL,
                 NULL,
                 NULL,
                 SepRmCommandServerThread,
                 NULL
                 );

    if (!NT_SUCCESS(Status)) {

        KdPrint(("Security: Rm Server Thread creation failed 0x%lx\n", Status));
        return FALSE;
    }

    //
    // Initialize data from the registry.  This must go here because all other
    // Se initialization takes place before the registry is initialized.
    //

    SepAdtInitializeCrashOnFail();
    SepAdtInitializePrivilegeAuditing();
    SepAdtInitializeAuditingOptions();
    
    //
    // Reference Monitor initialization is successful if we get to here.
    //

    ZwClose( SepRmState.SepRmThreadHandle );
    SepRmState.SepRmThreadHandle = NULL;
    return TRUE;
}


VOID
SepRmCommandServerThread(
    IN PVOID StartContext
)

/*++

Routine Description:

    This function is executed indefinitely by a dedicated permanent thread
    of the Sysinit Process, called the Reference Monitor Server Thread.
    This thread updates Reference Monitor Global State Data by dispatching
    commands sent from the LSA through the the Reference Monitor LPC Command
    Port.  The following steps are repeated indefinitely:

    o  Initialize RM Command receive and reply buffer headers
    o  Perform remaining Reference Monitor initialization involving LSA
    o  Wait for RM command sent from LSA, send reply to previous command
       (if any)
    o  Validate command
    o  Dispatch to command worker routine to execute command.

Arguments:

    None.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    PRM_REPLY_MESSAGE Reply;
    RM_COMMAND_MESSAGE CommandMessage;
    RM_REPLY_MESSAGE ReplyMessage;

    PAGED_CODE();

    //
    // Perform the rest of the Reference Monitor initialization, involving
    // synchronization with the LSA or dependency on the LSA having run.
    //

    if (!SepRmCommandServerThreadInit()) {

        KdPrint(("Security: Terminating Rm Command Server Thread\n"));
        return;
    }

    //
    // Initialize LPC port message header type and length fields for the
    // received command message.
    //

    CommandMessage.MessageHeader.u2.ZeroInit = 0;
    CommandMessage.MessageHeader.u1.s1.TotalLength =
        (CSHORT) sizeof(RM_COMMAND_MESSAGE);
    CommandMessage.MessageHeader.u1.s1.DataLength =
    CommandMessage.MessageHeader.u1.s1.TotalLength -
        (CSHORT) sizeof(PORT_MESSAGE);

    //
    // Initialize the LPC port message header type and data sizes for
    // for the reply message.
    //

    ReplyMessage.MessageHeader.u2.ZeroInit = 0;
    ReplyMessage.MessageHeader.u1.s1.TotalLength =
        (CSHORT) sizeof(RM_COMMAND_MESSAGE);
    ReplyMessage.MessageHeader.u1.s1.DataLength =
    ReplyMessage.MessageHeader.u1.s1.TotalLength -
        (CSHORT) sizeof(PORT_MESSAGE);

    //
    // First time through, there is no reply.
    //

    Reply = NULL;

    //
    // Now loop indefinitely, processing incoming Rm commands from the LSA.
    //

    for(;;) {

        //
        // Wait for Command, send reply to previous command (if any)
        //

        Status = ZwReplyWaitReceivePort(
                    SepRmState.RmCommandPortHandle,
                    NULL,
                    (PPORT_MESSAGE) Reply,
                    (PPORT_MESSAGE) &CommandMessage
                    );

        if (!NT_SUCCESS(Status)) {

            //
            // malicious user apps can try to connect to this port.  We will
            // fail later, but if their thread vanishes, we'll get a failure
            // here.  Ignore it:
            //

            if ( Status == STATUS_UNSUCCESSFUL )
            {
                //
                // skip it:
                //

                Reply = NULL ;
                continue;
            }

            KdPrint(("Security: RM message receive from Lsa failed %lx\n",
                Status));

        }

        //
        // Now dispatch to a routine to handle the command.  Allow
        // command errors to occur without bringing system down just now.
        //

        if ( CommandMessage.MessageHeader.u2.s2.Type == LPC_REQUEST ) {
            (*(SepRmCommandDispatch[CommandMessage.CommandNumber]))
                (&CommandMessage, &ReplyMessage);

            //
            // Initialize the client thread info and message id for the
            // reply message.  First time through, the reply message structure
            // is not used.
            //

            ReplyMessage.MessageHeader.ClientId =
                CommandMessage.MessageHeader.ClientId;
            ReplyMessage.MessageHeader.MessageId =
                CommandMessage.MessageHeader.MessageId;

            Reply = &ReplyMessage;

        } else {

            Reply = NULL;
        }
    }  // end_for

    //
    // Make compiler ferme la bouche
    //

    StartContext;
}


BOOLEAN
SepRmCommandServerThreadInit(
    VOID
    )

/*++

Routine Description:

    This function performs initialization of the Reference Monitor Server
    thread.  The following steps are performed.

    o  Wait on the LSA signalling the event.  When the event is signalled,
       the LSA has already created the LSA Command Server LPC Port
    o  Close the LSA Init Event Handle.  The event is not used again.
    o  Listen for the LSA to connect to the Port
    o  Accept the connection.
    o  Connect to the LSA Command Server LPC Port

Arguments:

    None.

Return Value:

--*/

{
    NTSTATUS Status;
    UNICODE_STRING LsaCommandPortName;
    PORT_MESSAGE ConnectionRequest;
    SECURITY_QUALITY_OF_SERVICE DynamicQos;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PORT_VIEW ClientView;
    REMOTE_PORT_VIEW LsaClientView;
    BOOLEAN BooleanStatus = TRUE;

    PAGED_CODE();

    //
    // Save a pointer to our process so we can get back into this process
    // to send commands to the LSA (using a handle to an LPC port created
    // below).
    //

    SepRmLsaCallProcess = PsGetCurrentProcess();

    ObReferenceObject(SepRmLsaCallProcess);

    //
    // Wait on the LSA signalling the event.  This means that the LSA
    // has created its command port, not that LSA initialization is
    // complete.
    //

    Status = ZwWaitForSingleObject(
                 SepRmState.LsaInitEventHandle,
                 FALSE,
                 NULL);

    if ( !NT_SUCCESS(Status) ) {

        KdPrint(("Security Rm Init: Waiting for LSA Init Event failed 0x%lx\n", Status));
        goto RmCommandServerThreadInitError;
    }

    //
    // Close the LSA Init Event Handle.  The event is not used again.
    //

    ZwClose(SepRmState.LsaInitEventHandle);

    //
    // Listen for a connection to be made by the LSA to the Reference Monitor
    // Command Port.  This connection will be made by the LSA process.
    //

    ConnectionRequest.u1.s1.TotalLength = sizeof(ConnectionRequest);
    ConnectionRequest.u1.s1.DataLength = (CSHORT)0;
    Status = ZwListenPort(
                 SepRmState.RmCommandPortHandle,
                 &ConnectionRequest
                 );

    if (!NT_SUCCESS(Status)) {

        KdPrint(("Security Rm Init: Listen to Command Port failed 0x%lx\n",
            Status));
        goto RmCommandServerThreadInitError;
    }

    //
    // Obtain a handle to the LSA process for use when auditing.
    //

    InitializeObjectAttributes( &ObjectAttributes, NULL, 0, NULL, NULL );

    Status = ZwOpenProcess(
                 &SepLsaHandle,
                 PROCESS_VM_OPERATION | PROCESS_VM_WRITE,
                 &ObjectAttributes,
                 &ConnectionRequest.ClientId
                 );

    if (!NT_SUCCESS(Status)) {

        KdPrint(("Security Rm Init: Open Listen to Command Port failed 0x%lx\n",
            Status));
        goto RmCommandServerThreadInitError;
    }

    //
    // Accept the connection made by the LSA process.
    //

    LsaClientView.Length = sizeof(LsaClientView);

    Status = ZwAcceptConnectPort(
                 &SepRmState.RmCommandPortHandle,
                 NULL,
                 &ConnectionRequest,
                 TRUE,
                 NULL,
                 &LsaClientView
                 );

    if (!NT_SUCCESS(Status)) {

        KdPrint(("Security Rm Init: Accept Connect to Command Port failed 0x%lx\n",
                Status));

        goto RmCommandServerThreadInitError;
    }

    //
    // Complete the connection.
    //

    Status = ZwCompleteConnectPort(SepRmState.RmCommandPortHandle);

    if (!NT_SUCCESS(Status)) {

        KdPrint(("Security Rm Init: Complete Connect to Command Port failed 0x%lx\n",
                Status));
        goto RmCommandServerThreadInitError;
    }

    //
    // Set up the security quality of service parameters to use over the
    // Lsa Command LPC port.  Use the most efficient (least overhead) - which
    // is dynamic rather than static tracking.
    //

    DynamicQos.ImpersonationLevel = SecurityImpersonation;
    DynamicQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    DynamicQos.EffectiveOnly = TRUE;

    //
    // Create the section to be used as unnamed shared memory for
    // communication between the RM and LSA.
    //

    SepRmState.LsaCommandPortSectionSize.LowPart = PAGE_SIZE;
    SepRmState.LsaCommandPortSectionSize.HighPart = 0;

    Status = ZwCreateSection(
                 &SepRmState.LsaCommandPortSectionHandle,
                 SECTION_ALL_ACCESS,
                 NULL,                           // ObjectAttributes
                 &SepRmState.LsaCommandPortSectionSize,
                 PAGE_READWRITE,
                 SEC_COMMIT,
                 NULL                            // FileHandle
                 );

    if (!NT_SUCCESS(Status)) {

        KdPrint(("Security Rm Init: Create Memory Section for LSA port failed: %X\n", Status));
        goto RmCommandServerThreadInitError;
    }

    //
    // Set up for a call to NtConnectPort and connect to the LSA port.
    // This setup includes a description of the port memory section so that
    // the LPC connection logic can make the section visible to both the
    // client and server processes.
    //

    ClientView.Length = sizeof(ClientView);
    ClientView.SectionHandle = SepRmState.LsaCommandPortSectionHandle;
    ClientView.SectionOffset = 0;
    ClientView.ViewSize = SepRmState.LsaCommandPortSectionSize.LowPart;
    ClientView.ViewBase = 0;
    ClientView.ViewRemoteBase = 0;

    //
    // Set up the security quality of service parameters to use over the
    // port.  Use dynamic tracking so that XACTSRV will impersonate the
    // user that we are impersonating when we call NtRequestWaitReplyPort.
    // If we used static tracking, XACTSRV would impersonate the context
    // when the connection is made.
    //

    DynamicQos.ImpersonationLevel = SecurityImpersonation;
    DynamicQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    DynamicQos.EffectiveOnly = TRUE;

    //
    // Connect to the Lsa Command LPC Port.  This port is used to send
    // commands from the RM to the LSA.
    //

    RtlInitUnicodeString( &LsaCommandPortName, L"\\SeLsaCommandPort" );

    Status = ZwConnectPort(
                 &SepRmState.LsaCommandPortHandle,
                 &LsaCommandPortName,
                 &DynamicQos,
                 &ClientView,
                 NULL,                           // ServerView
                 NULL,                           // MaxMessageLength
                 NULL,                           // ConnectionInformation
                 NULL                            // ConnectionInformationLength
                 );

    if (!NT_SUCCESS(Status)) {

        KdPrint(("Security Rm Init: Connect to LSA Port failed 0x%lx\n", Status));
        goto RmCommandServerThreadInitError;
    }

    //
    // Store information about the section so that we can create pointers
    // meaningful to LSA.
    //

    SepRmState.RmViewPortMemory = ClientView.ViewBase;
    SepRmState.LsaCommandPortMemoryDelta =
        (LONG)((ULONG_PTR)ClientView.ViewRemoteBase - (ULONG_PTR) ClientView.ViewBase );
    SepRmState.LsaViewPortMemory = ClientView.ViewRemoteBase;

/* BugWarning - ScottBi - probably don't need the resource

    //
    // Create the resource serializing access to the port.  This
    // resource prevents the port and the shared memory from being
    // deleted while worker threads are processing requests.
    //

    if ( !SepRmState.LsaCommandPortResourceInitialized ) {

        ExInitializeResource( &SepRmState.LsaCommandPortResource );
        SepRmState.LsaCommandPortResourceInitialized = TRUE;
    }

    SepRmState.LsaCommandPortActive = TRUE;

*/

RmCommandServerThreadInitFinish:

    //
    // Dont need this section handle any more, even if returning
    // success.
    //

    if ( SepRmState.LsaCommandPortSectionHandle != NULL ) {

       NtClose( SepRmState.LsaCommandPortSectionHandle );
       SepRmState.LsaCommandPortSectionHandle = NULL;
    }

    //
    // The Reference Monitor Thread has successfully initialized.
    //

    return BooleanStatus;

RmCommandServerThreadInitError:

    if ( SepRmState.LsaCommandPortHandle != NULL ) {

       NtClose( SepRmState.LsaCommandPortHandle );
       SepRmState.LsaCommandPortHandle = NULL;
    }

    BooleanStatus = FALSE;
    goto RmCommandServerThreadInitFinish;
}



VOID
SepRmComponentTestCommandWrkr(
    IN PRM_COMMAND_MESSAGE CommandMessage,
    OUT PRM_REPLY_MESSAGE ReplyMessage
    )

/*++

Routine Description:

    BUGWARNING - Remove this command when other RM commands are implemented.
             Until then, this command is the only way a CT can verify that
             an RM command with parameters is sent correctly.

    This function processes the Component Test RM command.
    This is a temporary command that can be used to verify that the link
    from RM to LSA is working.  This command verifies that the link
    is working by receiving a ULONG parameter and verifying that it
    has the expected value.

Arguments:

    CommandMessage - Pointer to structure containing RM command message
        information consisting of an LPC PORT_MESSAGE structure followed
        by the command number (RmComponentTestCommand).  This command
        currently has one parameter, a fixed ulong value.

    ReplyMessage - Pointer to structure containing LSA reply message
        information consisting of an LPC PORT_MESSAGE structure followed
        by the command ReturnedStatus field in which a status code from the
        command will be returned.

Return Value:

    VOID

--*/

{
    PAGED_CODE();

    ReplyMessage->ReturnedStatus = STATUS_SUCCESS;

    //
    // Strict check that command is correct.
    //

    ASSERT( CommandMessage->CommandNumber == RmComponentTestCommand );

    KdPrint(("Security: RM Component Test Command Received\n"));

    //
    // Verify that the parameter value passed is as expected.
    //

    if (*((ULONG *) CommandMessage->CommandParams) !=
        RM_CT_COMMAND_PARAM_VALUE ) {

        ReplyMessage->ReturnedStatus = STATUS_INVALID_PARAMETER;
    }

    return;
}



VOID
SepRmSendCommandToLsaWrkr(
    IN PRM_COMMAND_MESSAGE CommandMessage,
    OUT PRM_REPLY_MESSAGE ReplyMessage
    )

/*++

Routine Description:

    This function carries out the special Rm Send Command To Lsa Command.  This
    command is used only by the ctlsarm component test which checks that
    LSA to RM and RM to LSA communication via LPC is working.

Arguments:

    CommandMessage - Pointer to structure containing RM command message
        information consisting of an LPC PORT_MESSAGE structure followed
        by the command number (RmDisableAuditCommand), followed by the
        command parameters.  The parameters of this special command consists
        of the Command Number of an LSA command and its parameters (if any).

    ReplyMessage - Pointer to structure containing RM reply message
        information consisting of an LPC PORT_MESSAGE structure followed
        by the command ReturnedStatus field in which a status code from the
        command will be returned.

Return Value:

    VOID

--*/

{
    //
    // Obtain a pointer to the LSA command's params, and the size of the
    // params in bytes.  If there are no params, set the pointer to NULL.
    //

    PVOID LsaCommandParams =
        ((RM_SEND_COMMAND_TO_LSA_PARAMS *)
        (CommandMessage->CommandParams))->LsaCommandParams;
    ULONG LsaCommandParamsLength =
        ((RM_SEND_COMMAND_TO_LSA_PARAMS *)
        (CommandMessage->CommandParams))->LsaCommandParamsLength;

    PAGED_CODE();

    if (LsaCommandParamsLength == 0) {

        LsaCommandParams = NULL;

    }

    //
    // Strict check that command is correct one for this worker.
    //

    ASSERT( CommandMessage->CommandNumber == RmSendCommandToLsaCommand );

    KdPrint(("Security: RM Send Command back to LSA Command Received\n"));

    ReplyMessage->ReturnedStatus = STATUS_SUCCESS;

//    Status = SepRmCallLsa(
//                 ((RM_SEND_COMMAND_TO_LSA_PARAMS *)
//                    (CommandMessage->CommandParams))->LsaCommandNumber,
//                 LsaCommandParams,
//                 LsaCommandParamsLength,
//                 NULL,
//                 0,
//                 NULL,
//                 NULL
//                 );


    return;

}




NTSTATUS
SepRmCallLsa(
    PSEP_WORK_ITEM SepWorkItem
    )
/*++

Routine Description:

    This function sends a command to the LSA via the LSA Reference Monitor
    Server Command LPC Port.  If the command has parameters, they will be
    copied directly into a message structure and sent via LPC, therefore,
    the supplied parameters may not contain any absolute pointers.  A caller
    must remove pointers by "marshalling" them into the buffer CommandParams.

    This function will create a queue of requests.  This is in order to allow
    greater throughput for the majority if its callers.  If a thread enters
    this routine and finds the queue empty, it is the responsibility of that
    thread to service all requests that come in while it is working until the
    queue is empty again.  Other threads that enter will simply hook their work
    item onto the queue and exit.


    To implement a new LSA command, do the following:
    ================================================

    (1)  If the command takes no parameters, just call this routine directly
         and provide an LSA worker routine called Lsap<command>Wrkr.  See
         file lsa\server\lsarm.c for examples

    (2)  If the command takes parameters, provide a routine called
         SepRmSend<command>Command that takes the parameters in unmarshalled
         form and calls SepRmCallLsa() with the command id, marshalled
         parameters, length  of marshalled parameters and pointer to
         optional reply message.  The marshalled parameters are free format:
         the only restriction is that there must be no absolute address
         pointers.  These parameters are all placed in the passed LsaWorkItem
         structure.

    (3)  In file private\inc\ntrmlsa.h, append a command name  to the
         enumerated type LSA_COMMAND_NUMBER defined in file
         private\inc\ntrmlsa.h.  Change the #define for LsapMaximumCommand
         to reference the new command.

    (4)  Add the Lsap<command>Wrkr to the command dispatch table structure
         LsapCommandDispatch[] in file lsarm.c.

    (5)  Add function prototypes to lsap.h and sep.h.


Arguments:

    LsaWorkItem - Supplies a pointer to an SE_LSA_WORK_ITEM containing the
        information to be passed to LSA.  This structure will be freed
        asynchronously by some invocation of this routine, not necessarily
        in the current context.

        !THIS PARAMETER MUST BE ALLOCATED OUT OF NONPAGED POOL!

Return Value:

    NTSTATUS - Result Code.  This is either a result code returned from
        trying to send the command/receive the reply, or a status code
        from the command itself.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    LSA_COMMAND_MESSAGE CommandMessage;
    LSA_REPLY_MESSAGE ReplyMessage;
    PSEP_LSA_WORK_ITEM WorkQueueItem;
    ULONG LocalListLength = 0;
    SIZE_T RegionSize;
    PVOID CopiedCommandParams = NULL;
    PVOID LsaViewCopiedCommandParams = NULL;

    PAGED_CODE();

#if 0
  DbgPrint("Entering SepRmCallLsa\n");
#endif

    WorkQueueItem = SepWorkListHead();

    KeAttachProcess( &SepRmLsaCallProcess->Pcb );

    while ( WorkQueueItem ) {

#if 0
      DbgPrint("Got a work item from head of queue, processing\n");
#endif

        //
        // Construct a message for LPC.  First, fill in the message header
        // fields for LPC, specifying the message type and data sizes for
        // the outgoing CommandMessage and the incoming ReplyMessage.
        //

        CommandMessage.MessageHeader.u2.ZeroInit = 0;
        CommandMessage.MessageHeader.u1.s1.TotalLength =
            ((CSHORT) RM_COMMAND_MESSAGE_HEADER_SIZE +
            (CSHORT) WorkQueueItem->CommandParamsLength);
        CommandMessage.MessageHeader.u1.s1.DataLength =
            CommandMessage.MessageHeader.u1.s1.TotalLength -
            (CSHORT) sizeof(PORT_MESSAGE);

        ReplyMessage.MessageHeader.u2.ZeroInit = 0;
        ReplyMessage.MessageHeader.u1.s1.DataLength = (CSHORT) WorkQueueItem->ReplyBufferLength;
        ReplyMessage.MessageHeader.u1.s1.TotalLength =
            ReplyMessage.MessageHeader.u1.s1.DataLength +
            (CSHORT) sizeof(PORT_MESSAGE);

        //
        // Next, fill in the header info needed by the LSA.
        //

        CommandMessage.CommandNumber = WorkQueueItem->CommandNumber;
        ReplyMessage.ReturnedStatus = STATUS_SUCCESS;

        //
        // Set up the Command Parameters either in the LPC Command Message
        // itself, in the preallocated Lsa shared memory block, or in a
        // specially allocated block.  The parameters are either
        // immediate (i.e. in the WorkQueueItem itself, or are in a buffer
        // pointed to by the address in the WorkQueueItem.
        //

        switch (WorkQueueItem->CommandParamsMemoryType) {

        case SepRmImmediateMemory:

            //
            // The Command Parameters are in the CommandParams buffer
            // in the Work Queue Item.  Just copy them to the corresponding
            // buffer in the CommandMessage buffer.
            //

            CommandMessage.CommandParamsMemoryType = SepRmImmediateMemory;

            RtlCopyMemory(
                CommandMessage.CommandParams,
                &WorkQueueItem->CommandParams,
                WorkQueueItem->CommandParamsLength
                );

            break;

        case SepRmPagedPoolMemory:
        case SepRmUnspecifiedMemory:

            //
            // The Command Parameters are contained in paged pool memory.
            // Since this memory is is not accessible by the LSA, we must
            // copy of them either to the LPC Command Message Block, or
            // into LSA shared memory.
            //

            if (WorkQueueItem->CommandParamsLength <= LSA_MAXIMUM_COMMAND_PARAM_SIZE) {

                //
                // Parameters will fit into the LPC Command Message block.
                //

                CopiedCommandParams = CommandMessage.CommandParams;

                RtlCopyMemory(
                    CopiedCommandParams,
                    WorkQueueItem->CommandParams.BaseAddress,
                    WorkQueueItem->CommandParamsLength
                    );

                CommandMessage.CommandParamsMemoryType = SepRmImmediateMemory;

            } else {

                //
                // Parameters too large for LPC Command Message block.
                // If possible, copy them to the preallocated Lsa Shared
                // Memory block.  If they are too large to fit, copy them
                // to an individually allocated chunk of Shared Virtual
                // Memory.
                //

                if (WorkQueueItem->CommandParamsLength <= SEP_RM_LSA_SHARED_MEMORY_SIZE) {

                    RtlCopyMemory(
                        SepRmState.RmViewPortMemory,
                        WorkQueueItem->CommandParams.BaseAddress,
                        WorkQueueItem->CommandParamsLength
                        );

                    LsaViewCopiedCommandParams = SepRmState.LsaViewPortMemory;
                    CommandMessage.CommandParamsMemoryType = SepRmLsaCommandPortSharedMemory;

                } else {

                    Status = SepAdtCopyToLsaSharedMemory(
                                 SepLsaHandle,
                                 WorkQueueItem->CommandParams.BaseAddress,
                                 WorkQueueItem->CommandParamsLength,
                                 &LsaViewCopiedCommandParams
                                 );

                    if (!NT_SUCCESS(Status)) {

                        //
                        // An error occurred, most likely in allocating
                        // shared virtual memory.  For now, just ignore
                        // the error and discard the Audit Record.  Later,
                        // we may consider generating a warning record
                        // indicating some records lost.
                        //

                        break;

                    }

                    CommandMessage.CommandParamsMemoryType = SepRmLsaCustomSharedMemory;
                }

                //
                // Buffer has been successfully copied to a shared Lsa
                // memory buffer.  Place the address of the buffer valid in
                // the LSA's process context in the Command Message.
                //

                *((PVOID *) CommandMessage.CommandParams) =
                    LsaViewCopiedCommandParams;

                CommandMessage.MessageHeader.u1.s1.TotalLength =
                    ((CSHORT) RM_COMMAND_MESSAGE_HEADER_SIZE +
                    (CSHORT) sizeof( LsaViewCopiedCommandParams ));
                CommandMessage.MessageHeader.u1.s1.DataLength =
                    CommandMessage.MessageHeader.u1.s1.TotalLength -
                    (CSHORT) sizeof(PORT_MESSAGE);
            }

            //
            // Free input command params buffer if Paged Pool.
            //

            if (WorkQueueItem->CommandParamsMemoryType == SepRmPagedPoolMemory) {

                ExFreePool( WorkQueueItem->CommandParams.BaseAddress );
            }

            break;

        default:

            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (NT_SUCCESS(Status)) {

            //
            // Send Message to the LSA via the LSA Server Command LPC Port.
            // This must be done in the process in which the handle was created.
            //

            Status = ZwRequestWaitReplyPort(
                         SepRmState.LsaCommandPortHandle,
                         (PPORT_MESSAGE) &CommandMessage,
                         (PPORT_MESSAGE) &ReplyMessage
                         );

            //
            // If the command was successful, copy the data back to the output
            // buffer.
            //

            if (NT_SUCCESS(Status)) {

                //
                // Move output from command (if any) to buffer.  Note that this
                // is done even if the command returns status, because some status
                // values are not errors.
                //

                if (ARGUMENT_PRESENT(WorkQueueItem->ReplyBuffer)) {

                    RtlCopyMemory(
                        WorkQueueItem->ReplyBuffer,
                        ReplyMessage.ReplyBuffer,
                        WorkQueueItem->ReplyBufferLength
                        );
                }

                //
                // Return status from command.
                //

                Status = ReplyMessage.ReturnedStatus;

                if (!NT_SUCCESS(Status)) {
                    KdPrint(("Security: Command sent from RM to LSA returned 0x%lx\n",
                        Status));
                }

            } else {

                KdPrint(("Security: Sending Command RM to LSA failed 0x%lx\n", Status));
            }

            //
            // On return from the LPC call to the LSA, we expect the called
            // LSA worker routine to have copied the Command Parameters
            // buffer (if any).  If a custom shared memory boffer was allocated,
            // free it now.
            //

            if (CommandMessage.CommandParamsMemoryType == SepRmLsaCustomSharedMemory) {

                RegionSize = 0;

                Status = ZwFreeVirtualMemory(
                             SepLsaHandle,
                             (PVOID *) &CommandMessage.CommandParams,
                             &RegionSize,
                             MEM_RELEASE
                             );

                ASSERT(NT_SUCCESS(Status));
            }

        }


        //
        // Clean up.  We must call the cleanup functions on its parameter
        // and then free the used WorkQueueItem itself.
        //

        if ( ARGUMENT_PRESENT( WorkQueueItem->CleanupFunction)) {

            (WorkQueueItem->CleanupFunction)(WorkQueueItem->CleanupParameter);
        }

        //
        // Determine if there is more work to do on this list
        //

        WorkQueueItem = SepDequeueWorkItem();
#if 0
      if ( WorkQueueItem ) {
            DbgPrint("Got another item from list, going back\n");
      } else {
          DbgPrint("List is empty, leaving\n");
      }
#endif


    }

    KeDetachProcess();

    if ( LocalListLength > SepLsaQueueLength ) {
        SepLsaQueueLength = LocalListLength;
    }

    return Status;
}





BOOLEAN
SepRmInitPhase0(
    )

/*++

Routine Description:

    This function performs Reference Monitor Phase 0 initialization.
    This includes initializing the reference monitor database to a state
    which allows access validation routines to operate (always granting
    access) prior to the main init of the Reference Monitor in Phase 1
    initialization, without having to check if the RM is initialized.


Arguments:

    None.

Return Value:

   BOOLEAN - TRUE if successful, else FALSE

--*/

{

    BOOLEAN CompletionStatus;

    PAGED_CODE();

    CompletionStatus = SepRmDbInitialization();

    return CompletionStatus;
}
