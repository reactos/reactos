/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    errorlog.c

Abstract:

    This module contains the code for the I/O error log thread.

Author:

    Darryl E. Havens (darrylh) May 3, 1989

Environment:

    Kernel mode, system process thread

Revision History:


--*/

#include "iop.h"
#include "elfkrnl.h"

typedef struct _IOP_ERROR_LOG_CONTEXT {
    KDPC ErrorLogDpc;
    KTIMER ErrorLogTimer;
}IOP_ERROR_LOG_CONTEXT, *PIOP_ERROR_LOG_CONTEXT;

//
// Declare routines local to this module.
//

BOOLEAN
IopErrorLogConnectPort(
    VOID
    );

VOID
IopErrorLogDpc(
    IN struct _KDPC *Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

PLIST_ENTRY
IopErrorLogGetEntry(
    );

VOID
IopErrorLogQueueRequest(
    VOID
    );

VOID
IopErrorLogRequeueEntry(
    IN PLIST_ENTRY ListEntry
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IopErrorLogThread)
#pragma alloc_text(PAGE, IopErrorLogConnectPort)
#pragma alloc_text(PAGE, IopErrorLogQueueRequest)
#endif

//
// Define a global varibles used by the error logging code.
//

WORK_QUEUE_ITEM IopErrorLogWorkItem;
HANDLE ErrorLogPort;
BOOLEAN ErrorLogPortConnected;
BOOLEAN IopErrorLogPortPending;
BOOLEAN IopErrorLogDisabledThisBoot;

//
// Define the amount of space required for the device and driver names.
//

#define IO_ERROR_NAME_LENGTH 100

VOID
IopErrorLogThread(
    IN PVOID StartContext
    )

/*++

Routine Description:

    This is the main loop for the I/O error log thread which executes in the
    system process context.  This routine is started when the system is
    initialized.

Arguments:

    StartContext - Startup context; not used.

Return Value:

    None.

--*/

{
    PERROR_LOG_ENTRY errorLogEntry;
    UNICODE_STRING nameString;
    PLIST_ENTRY listEntry;
    PIO_ERROR_LOG_MESSAGE errorMessage;
    NTSTATUS status;
    PELF_PORT_MSG portMessage;
    PCHAR objectName;
    ULONG messageLength;
    ULONG driverNameLength;
    ULONG deviceNameLength;
    ULONG objectNameLength;
    ULONG remainingLength;
    ULONG stringLength;
    CHAR nameBuffer[IO_ERROR_NAME_LENGTH+sizeof( OBJECT_NAME_INFORMATION )];
    PDRIVER_OBJECT driverObject;
    POBJECT_NAME_INFORMATION nameInformation;
    PIO_ERROR_LOG_PACKET errorData;
    PWSTR string;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( StartContext );

    //
    // Check to see whether a connection has been made to the error log
    // port.  If the port is not connected return.
    //

    if (!IopErrorLogConnectPort()) {

        //
        // The port could not be connected.  A timer was started that will
        // try again later.
        //

        return;
    }

    //
    // Allocate and zero the port message structure, include space for the
    // name of the device and driver.
    //

    messageLength = IO_ERROR_LOG_MESSAGE_LENGTH;
    portMessage = ExAllocatePool(PagedPool, messageLength);

    if (portMessage == NULL) {

        //
        // The message buffer could not be allocated. Request that
        // the error log thread routine be called again later.
        //

        IopErrorLogQueueRequest();
        return;
    }

    RtlZeroMemory( portMessage, sizeof( *portMessage ) );
    portMessage->MessageType = IO_ERROR_LOG;
    errorMessage = &portMessage->u.IoErrorLogMessage;

    nameInformation = (PVOID) &nameBuffer;

    //
    // Now enter the main loop for this thread.  This thread performs the
    // following operations:
    //
    //   1)  If a connection has been made to the error log port, dequeue a
    //       packet from the queue head and attempt to send it to the port.
    //
    //   2)  If the send works, loop sending packets until there are no more
    //       packets;  otherwise, indicate that the connection has been broken,
    //       cleanup, place the packet back onto the head of the queue and
    //       return.
    //
    //   3)  After all the packets are sent clear the pending variable and
    //       return.
    //

    for (;;) {

        //
        // Loop dequeueing  packets from the queue head and attempt to send
        // each to the port.
        //
        // If the send works, continue looping until there are no more packets.
        // Otherwise, indicate that the connection has been broken, cleanup,
        // place the packet back onto the head of the queue, and start from the
        // top of the loop again.
        //

        if (!(listEntry = IopErrorLogGetEntry())) {
            break;
        }

        errorLogEntry = CONTAINING_RECORD( listEntry,
                                           ERROR_LOG_ENTRY,
                                           ListEntry );

        //
        // The size of errorLogEntry is ERROR_LOG_ENTRY +
        // IO_ERROR_LOG_PACKET + (Extra Dump data).  The size of the
        // initial message length should be IO_ERROR_LOG_MESSAGE +
        // (Extra Dump data), since IO_ERROR_LOG_MESSAGE contains an
        // IO_ERROR_LOG_PACKET. Using the above calculations set the
        // message length.
        //

        messageLength = sizeof( IO_ERROR_LOG_MESSAGE ) -
            sizeof( ERROR_LOG_ENTRY ) - sizeof( IO_ERROR_LOG_PACKET ) +
            errorLogEntry->Size;

        errorData = (PIO_ERROR_LOG_PACKET) (errorLogEntry + 1);

        //
        // Copy the error log packet and the extra data to the message.
        //

        RtlMoveMemory( &errorMessage->EntryData,
                       errorData,
                       errorLogEntry->Size - sizeof( ERROR_LOG_ENTRY ) );

        errorMessage->TimeStamp = errorLogEntry->TimeStamp;
        errorMessage->Type = IO_TYPE_ERROR_MESSAGE;

        //
        // Add the driver and device name string.  These strings go
        // before the error log strings.  Just write over the current
        // strings and they will be recopied later.
        //

        if (errorData->NumberOfStrings != 0) {

            //
            // Start the driver and device strings where the current
            // strings start.
            //

            objectName = (PCHAR) (&errorMessage->EntryData) +
                                 errorData->StringOffset;

        } else {

            //
            // Put the driver and device strings at the end of the
            // data.
            //

            objectName = (PCHAR) errorMessage + messageLength;

        }

        //
        // Make sure the driver offset starts on an even bountry.
        //

        objectName = (PCHAR) ((ULONG_PTR) (objectName + sizeof(WCHAR) - 1) &
            ~(ULONG_PTR)(sizeof(WCHAR) - 1));

        errorMessage->DriverNameOffset = (ULONG)(objectName - (PCHAR) errorMessage);

        remainingLength = (ULONG)((PCHAR) portMessage + IO_ERROR_LOG_MESSAGE_LENGTH
                            - objectName);

        //
        // Calculate the length of the driver name and
        // the device name. If the driver object has a name then get
        // it from there; otherwise try to query the device object.
        //

        driverObject = errorLogEntry->DriverObject;
        driverNameLength = 0;

        if (driverObject != NULL) {
            if (driverObject->DriverName.Buffer != NULL) {

                nameString.Buffer = driverObject->DriverName.Buffer;
                driverNameLength = driverObject->DriverName.Length;
            }

            if (driverNameLength == 0) {

                //
                // Try to query the driver object for a name.
                //

                status = ObQueryNameString( driverObject,
                                            nameInformation,
                                            IO_ERROR_NAME_LENGTH + sizeof( OBJECT_NAME_INFORMATION ),
                                            &objectNameLength );

                if (!NT_SUCCESS( status ) || !nameInformation->Name.Length) {

                    //
                    // No driver name was available.
                    //

                    driverNameLength = 0;

                } else {
                    nameString = nameInformation->Name;
                }

            }

        } else {

            //
            // If no driver object, this message must be from the 
            // kernel.   We need to point the eventlog service to
            // an event message file containing ntstatus messages,
            // ie, ntdll, we do this by claiming this event is an
            // application popup.
            //

            nameString.Buffer = L"Application Popup";
            driverNameLength = wcslen(nameString.Buffer) * sizeof(WCHAR);
        }

        if (driverNameLength != 0 ) {

            //
            // Pick out the module name.
            //

            string = nameString.Buffer +
                (driverNameLength / sizeof(WCHAR));

            driverNameLength = sizeof(WCHAR);
            string--;
            while (*string != L'\\' && string != nameString.Buffer) {
                string--;
                driverNameLength += sizeof(WCHAR);
            }

            if (*string == L'\\') {
                string++;
                driverNameLength -= sizeof(WCHAR);
            }

            //
            // Ensure there is enough room for the driver name.
            // Save space for 3 NULLs one for the driver name,
            // one for the device name and one for strings.
            //

            if (driverNameLength > remainingLength - (3 * sizeof(WCHAR))) {
                driverNameLength = remainingLength - (3 * sizeof(WCHAR));
            }

            RtlMoveMemory(
                objectName,
                string,
                driverNameLength
                );

        }

        //
        // Add a null after the driver name even if there is no
        // driver name.
        //

       *((PWSTR) (objectName + driverNameLength)) = L'\0';
       driverNameLength += sizeof(WCHAR);

        //
        // Determine where the next string goes.
        //

        objectName += driverNameLength;
        remainingLength -= driverNameLength;

        errorMessage->EntryData.StringOffset = (USHORT)(objectName - (PCHAR) errorMessage);

        if (errorLogEntry->DeviceObject != NULL) {

            status = ObQueryNameString( errorLogEntry->DeviceObject,
                                        nameInformation,
                                        IO_ERROR_NAME_LENGTH + sizeof( OBJECT_NAME_INFORMATION ) - driverNameLength,
                                        &objectNameLength );

            if (!NT_SUCCESS( status ) || !nameInformation->Name.Length) {

                //
                // No device name was available. Add a Null string.
                //

                nameInformation->Name.Length = 0;
                nameInformation->Name.Buffer = L"\0";

            }

            //
            // No device name was available. Add a Null string.
            // Always add a device name string so that the
            // insertion string counts are correct.
            //

        } else {

                //
                // No device name was available. Add a Null string.
                // Always add a device name string so that the
                // insertion string counts are correct.
                //

                nameInformation->Name.Length = 0;
                nameInformation->Name.Buffer = L"\0";

        }

        deviceNameLength = nameInformation->Name.Length;

        //
        // Ensure there is enough room for the device name.
        // Save space for a NULL.
        //

        if (deviceNameLength > remainingLength - (2 * sizeof(WCHAR))) {

            deviceNameLength = remainingLength - (2 * sizeof(WCHAR));

        }

        RtlMoveMemory( objectName,
                       nameInformation->Name.Buffer,
                       deviceNameLength );

        //
        // Add a null after the device name even if there is no
        // device name.
        //

        *((PWSTR) (objectName + deviceNameLength)) = L'\0';
        deviceNameLength += sizeof(WCHAR);

        //
        // Update the string count for the device object.
        //

        errorMessage->EntryData.NumberOfStrings++;
        objectName += deviceNameLength;
        remainingLength -= deviceNameLength;

        if (errorData->NumberOfStrings) {

            stringLength = errorLogEntry->Size - sizeof( ERROR_LOG_ENTRY ) -
                            errorData->StringOffset;

            //
            // Ensure there is enough room for the strings.
            // Save space for a NULL.
            //

            if (stringLength > remainingLength - sizeof(WCHAR)) {


                messageLength -= stringLength - remainingLength;
                stringLength = remainingLength - sizeof(WCHAR);

            }

            //
            // Copy the strings to the end of the message.
            //

            RtlMoveMemory( objectName,
                           (PCHAR) errorData + errorData->StringOffset,
                           stringLength );

            //
            // Add a null after the strings
            //
            //

           *((PWSTR) (objectName + stringLength)) = L'\0';

        }

        //
        // Update the message length.
        //

        errorMessage->DriverNameLength = (USHORT) driverNameLength;
        messageLength += deviceNameLength + driverNameLength;
        errorMessage->Size = (USHORT) messageLength;

        messageLength += FIELD_OFFSET ( ELF_PORT_MSG, u ) -
            FIELD_OFFSET (ELF_PORT_MSG, MessageType);

        portMessage->PortMessage.u1.s1.TotalLength = (USHORT)
            (sizeof( PORT_MESSAGE ) + messageLength);
        portMessage->PortMessage.u1.s1.DataLength = (USHORT) (messageLength);
        status = NtRequestPort( ErrorLogPort, (PPORT_MESSAGE) portMessage );

        if (!NT_SUCCESS( status )) {

            //
            // The send failed.  Place the packet back onto the head of
            // the error log queue, forget the current connection since
            // it no longer works, and close the handle to the port.
            // Set a timer up for another attempt later.
            // Finally, exit the loop since there is no connection
            // to do any work on.
            //

            NtClose( ErrorLogPort );

            IopErrorLogRequeueEntry( &errorLogEntry->ListEntry );

            IopErrorLogQueueRequest();

            break;

        } else {

            //
            // The send worked fine.  Free the packet and the update
            // the allocation count.
            //

            ExInterlockedAddUlong( &IopErrorLogAllocation,
                                   (ULONG) ( -errorLogEntry->Size ),
                                   &IopErrorLogAllocationLock );

            //
            // Dereference the object pointers now that the name has been
            // captured.
            //


            if (errorLogEntry->DeviceObject != NULL) {
                ObDereferenceObject( errorLogEntry->DeviceObject );
            }

            if (driverObject != NULL) {
                ObDereferenceObject( errorLogEntry->DriverObject );
            }

            ExFreePool( errorLogEntry );

        } // if

    } // for

    //
    // Finally, free the message buffer and return.
    //

    ExFreePool(portMessage);

}

BOOLEAN
IopErrorLogConnectPort(
    VOID
    )
/*++

Routine Description:

    This routine attempts to connect to the error log port.  If the connection
    was made successfully and the port allows suficiently large messages, then
    the ErrorLogPort to the port handle, ErrorLogPortConnected is set to
    TRUE and TRUE is retuned.  Otherwise a timer is started to queue a
    worker thread at a later time, unless there is a pending connection.

Arguments:

    None.

Return Value:

    Returns TRUE if the port was connected.

--*/

{

    UNICODE_STRING errorPortName;
    NTSTATUS status;
    ULONG maxMessageLength;
    SECURITY_QUALITY_OF_SERVICE dynamicQos;

    PAGED_CODE();

    //
    // If the ErrorLogPort is connected then return true.
    //

    if (ErrorLogPortConnected) {

        //
        // The port is connect return.
        //

        return(TRUE);
    }

    //
    // Set up the security quality of service parameters to use over the
    // port.  Use the most efficient (least overhead) - which is dynamic
    // rather than static tracking.
    //

    dynamicQos.ImpersonationLevel = SecurityImpersonation;
    dynamicQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    dynamicQos.EffectiveOnly = TRUE;

    //
    // Generate the string structure for describing the error logger's port.
    //

    RtlInitUnicodeString( &errorPortName, ELF_PORT_NAME_U );

    status = NtConnectPort( &ErrorLogPort,
                            &errorPortName,
                            &dynamicQos,
                            (PPORT_VIEW) NULL,
                            (PREMOTE_PORT_VIEW) NULL,
                            &maxMessageLength,
                            (PVOID) NULL,
                            (PULONG) NULL );

    if (NT_SUCCESS( status )) {
        if (maxMessageLength >= IO_ERROR_LOG_MESSAGE_LENGTH) {
            ErrorLogPortConnected = TRUE;
            return(TRUE);
        } else {
            NtClose(ErrorLogPort);
        }
    }

    //
    // The port was not successfully opened, or its message size was unsuitable
    // for use here.  Queue a later request to run the error log thread.
    //

    IopErrorLogQueueRequest();

    //
    // The port could not be connected at this time return false.
    //

    return(FALSE);
}

VOID
IopErrorLogDpc(
    IN struct _KDPC *Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This routine queues a work request to the worker thread to process logged
    errors. It is called by a timer DPC when the error log port cannot be
    connected.  The DPC structure itself is freed by this routine.

Arguments:

    Dpc - Supplies a pointer to the DPC structure.  This structure is freed by
        this routine.

    DeferredContext - Unused.

    SystemArgument1 - Unused.

    SystemArgument2 - Unused.

Return Value:

    None

--*/

{
    //
    // Free the DPC structure if there is one.
    //

    if (Dpc != NULL) {
        ExFreePool(Dpc);
    }

    ExInitializeWorkItem( &IopErrorLogWorkItem, IopErrorLogThread, NULL );

    ExQueueWorkItem( &IopErrorLogWorkItem, DelayedWorkQueue );
}

PLIST_ENTRY
IopErrorLogGetEntry(
    )

/*++

Routine Description:

    This routine gets the next entry from the head of the error log queue
    and returns it to the caller.

Arguments:

    None.

Return Value:

    The return value is a pointer to the packet removed, or NULL if there were
    no packets on the queue.

--*/

{
    KIRQL irql;
    PLIST_ENTRY listEntry;

    //
    // Remove the next packet from the queue, if there is one.
    //

    ExAcquireSpinLock( &IopErrorLogLock, &irql );
    if (IsListEmpty( &IopErrorLogListHead )) {

        //
        // Indicate no more work will be done in the context of this worker
        // thread and indicate to the caller that no packets were located.
        //

        IopErrorLogPortPending = FALSE;
        listEntry = (PLIST_ENTRY) NULL;
    } else {

        //
        // Remove the next packet from the head of the list.
        //

        listEntry = RemoveHeadList( &IopErrorLogListHead );
    }

    ExReleaseSpinLock( &IopErrorLogLock, irql );
    return listEntry;
}

VOID
IopErrorLogQueueRequest(
    VOID
    )

/*++

Routine Description:

    This routine sets a timer to fire after 30 seconds.  The timer queues a
    DPC which then queues a worker thread request to run the error log thread
    routine.

Arguments:

    None.

Return Value:

    None.

--*/

{
    LARGE_INTEGER interval;
    PIOP_ERROR_LOG_CONTEXT context;

    PAGED_CODE();

    //
    // Allocate a context block which will contain the timer and the DPC.
    //

    context = ExAllocatePool( NonPagedPool, sizeof( IOP_ERROR_LOG_CONTEXT ) );

    if (context == NULL) {

        //
        // The context block could not be allocated. Clear the error log
        // pending bit. If there is another error then a new attempt will
        // be made.  Note the spinlock does not need to be held here since
        // new attempt should be made later not right now, so if another
        // error log packet is currently being queue, it waits with the
        // others.
        //

        IopErrorLogPortPending = FALSE;
        return;
    }

    KeInitializeDpc( &context->ErrorLogDpc,
                     IopErrorLogDpc,
                     NULL );

    KeInitializeTimer( &context->ErrorLogTimer );

    //
    // Delay for 30 seconds and try for the port again.
    //

    interval.QuadPart = - 10 * 1000 * 1000 * 30;

    //
    // Set the timer to fire a DPC in 30 seconds.
    //

    KeSetTimer( &context->ErrorLogTimer, interval, &context->ErrorLogDpc );
}

VOID
IopErrorLogRequeueEntry(
    IN PLIST_ENTRY ListEntry
    )

/*++

Routine Description:

    This routine puts an error packet back at the head of the error log queue
    since it cannot be processed at the moment.

Arguments:

    ListEntry - Supplies a pointer to the packet to be placed back onto the
        error log queue.

Return Value:

    None.

--*/

{
    KIRQL irql;

    //
    // Simply insert the packet back onto the head of the queue, indicate that
    // the error log port is not connected, queue a request to check again
    // soon, and return.
    //

    ExAcquireSpinLock( &IopErrorLogLock, &irql );
    InsertHeadList( &IopErrorLogListHead, ListEntry );
    ErrorLogPortConnected = FALSE;
    ExReleaseSpinLock( &IopErrorLogLock, irql );
}
