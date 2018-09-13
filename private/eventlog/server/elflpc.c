/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ELFLPC.C

Abstract:

    This file contains the routines that deal with the LPC port in the
    eventlog service.

Author:

    Rajen Shah  (rajens)    10-Jul-1991

Revision History:



--*/

//
// INCLUDES
//

#include <eventp.h>
#include <ntiolog.h>       // For IO_ERROR_LOG_[MESSAGE/PACKET]
#include <ntiologc.h>      // QUOTA error codes
#include <elfkrnl.h>
//#include <elflpc.h>
#include <stdlib.h>
#include <memory.h>
#include <elfextrn.h> // Computername

#include <nt.h>         // DbgPrint prototype
#include <ntrtl.h>      // DbgPrint prototype
#include <ntdef.h>
#include <ntstatus.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <winbase.h>    // LocalAlloc
#include <lmcons.h>
#include <string.h>
#include <lmerr.h>
#include <elfmsg.h>


//
//  Global value for the "system" module
//

PLOGMODULE SystemModule = NULL;

NTSTATUS
SetUpLPCPort ()


/*++

Routine Description:

    This routine sets up the LPC port for the service.


Arguments:



Return Value:



Note:


--*/
{

    NTSTATUS status;
    UNICODE_STRING unicodePortName;
    OBJECT_ATTRIBUTES objectAttributes;
    PORT_MESSAGE connectionRequest;


    ElfDbgPrint (("[ELF] Set up LPC port\n"));

    // Initialize the handles to zero so that we can determine what to do
    // if we need to clean up.

    ElfConnectionPortHandle = NULL;
    ElfCommunicationPortHandle = NULL;

    //
    // Create the LPC port.
    //
    RtlInitUnicodeString( &unicodePortName, ELF_PORT_NAME_U );

    InitializeObjectAttributes(
            &objectAttributes,
            &unicodePortName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

    status = NtCreatePort(
                    &ElfConnectionPortHandle,
                    &objectAttributes,
                    0,
                    ELF_PORT_MAX_MESSAGE_LENGTH,
                    ELF_PORT_MAX_MESSAGE_LENGTH * 32
                    );

    if ( !NT_SUCCESS(status) ) {
        ElfDbgPrintNC(( "[ELF] Port not created\n" ));
    }

    return(status);

}



LPWSTR
ElfpCopyString(
    LPWSTR Destination,
    LPWSTR Source,
    ULONG Length )
/*++

Routine Description:

    Copies a string to the destination.  Correctly NUL terminates
    the string.


Arguments:

    Destination - place where string is to be copied
    
    Source - string that may or may not be NUL terminated
    
    Length - length in bytes of string being copied.  May include NUL

Return Value:

    LPWSTR to first WCHAR past NUL

--*/
{

    //
    //  Copy the data
    //
    
    RtlMoveMemory( Destination, Source, Length );

    //
    //  Make sure it's NULL terminated
    //

    if (Length != 0) {
        Destination += Length/2 - 1;
        if (*Destination != L'\0') {
            Destination++;
            *Destination = L'\0';
        }
    } else {
        *Destination = L'0';
    }

    return Destination + 1;
}


NTSTATUS
ElfProcessIoLPCPacket ( 
    ULONG PacketLength,
    PIO_ERROR_LOG_MESSAGE pIoErrorLogMessage
    )

/*++

Routine Description:

    This routine takes the packet received from the LPC port and processes it.
    The logfile will be system, the module name will be the driver that 
    generated the packet, the SID will always be NULL and
    there will always be one string, which will be the device name.

    It extracts the information from the LPC packet, and then calls the
    common routine to do the work of formatting the data into
    an event record and writing it out to the log file.


Arguments:

    pIoErrorLogMessage - Pointer to the data portion of the packet just
        received through the LPC port.


Return Value:

        Status of this operation.

--*/

{
    NTSTATUS status;
    ELF_REQUEST_RECORD  Request;
    WRITE_PKT WritePkt;

    UNICODE_STRING SystemString;
    ULONG RecordLength;
    PEVENTLOGRECORD EventLogRecord;
    LPWSTR DestinationString, SourceString;
    PBYTE BinaryData;
    ULONG PadSize;
    LARGE_INTEGER Time;
    ULONG TimeWritten;
    PULONG pEndLength;
    ULONG i = 0;
    PWCHAR pwch;
    PWCHAR pwStart;
    PWCHAR pwEnd;
    ULONG StringLength;

    PacketLength = min( pIoErrorLogMessage->Size, PacketLength );

    try {

        //
        // Validate the packet, First make sure there are the correct
        // number of NULL terminated strings, and remember the
        // total number of bytes to copy
        //

        pwStart = pwch = (PWCHAR) ((PBYTE) pIoErrorLogMessage +
                                   pIoErrorLogMessage->EntryData.StringOffset);

        pwEnd = (PWCHAR) ((PBYTE) pIoErrorLogMessage + PacketLength );

        while (pwch < pwEnd &&
            i < pIoErrorLogMessage->EntryData.NumberOfStrings) {
                if (*pwch == L'\0') {
                    i++;
                }
                pwch++;
        }
        StringLength = (ULONG) (pwch - pwStart) * sizeof(WCHAR);

        //
        // Now make sure everything in the packet is true
        //

        if ((i != pIoErrorLogMessage->EntryData.NumberOfStrings)

                ||

            (pIoErrorLogMessage->DriverNameOffset 
             + pIoErrorLogMessage->DriverNameLength >= PacketLength)

                ||


            (pIoErrorLogMessage->EntryData.StringOffset >= PacketLength)

                ||

            (FIELD_OFFSET(IO_ERROR_LOG_MESSAGE, EntryData) 
             + FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData) 
             + (ULONG)pIoErrorLogMessage->EntryData.DumpDataSize >= PacketLength)
            ) {
            
                    //
                    // It's a bad packet, log it and return
                    //

                    ElfDbgPrintNC(("[ELF] Bad packet from LPC port\n"));
                    ElfpCreateElfEvent(EVENT_BadDriverPacket,
                                       EVENTLOG_ERROR_TYPE,
                                       0,                    // EventCategory
                                       0,                    // NumberOfStrings
                                       NULL,                 // Strings
                                       pIoErrorLogMessage,   // Data
                                       PacketLength,         // Datalength
                                       0                     // flags
                                       );
                    return(STATUS_UNSUCCESSFUL);
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {

        //
        // It's a bad packet, log it and return
        //

        ElfDbgPrintNC(("[ELF] Bad packet from LPC port\n"));
        ElfpCreateElfEvent(EVENT_BadDriverPacket,
                           EVENTLOG_ERROR_TYPE,
                           0,                    // EventCategory
                           0,                    // NumberOfStrings
                           NULL,                 // Strings
                           pIoErrorLogMessage,   // Data
                           PacketLength,         // Datalength
                           0                     // flags
                           );
        return(STATUS_UNSUCCESSFUL);

    }

    //
    // We're going to need this everytime, so just get it once
    //

    if (!SystemModule) {

        //
        // Get the system module to log driver events
        //

        RtlInitUnicodeString(&SystemString, ELF_SYSTEM_MODULE_NAME);
        SystemModule = GetModuleStruc (&SystemString);
        ASSERT(SystemModule); // GetModuleStruc never returns NULL

    }

    //
    // The packet should be an IO_ERROR_LOG_MESSAGE
    //

    ASSERT(pIoErrorLogMessage->Type == IO_TYPE_ERROR_MESSAGE);

    //
    // Set up write packet in request packet
    //

    Request.Pkt.WritePkt = &WritePkt;
    Request.Flags = 0;

    //
    // Generate any additional information needed in the record.
    //

    // TIMEWRITTEN
    // We need to generate a time when the log is written. This
    // gets written in the log so that we can use it to test the
    // retention period when wrapping the file.
    //

    NtQuerySystemTime(&Time);
    RtlTimeToSecondsSince1970(
                        &Time,
                        &TimeWritten
                        );

    //
    // Determine how big a buffer is needed for the eventlog record.
    //

    RecordLength = sizeof(EVENTLOGRECORD)
                 + ComputerNameLength             // computername
                 + 2 * sizeof(WCHAR)                    // term's
                 + PacketLength
                 - FIELD_OFFSET(IO_ERROR_LOG_MESSAGE, EntryData)
                 + sizeof(RecordLength);                // final len

    //
    // Determine how many pad bytes are needed to align to a DWORD
    // boundary.
    //

    PadSize = sizeof(ULONG) - (RecordLength % sizeof(ULONG));

    RecordLength += PadSize;    // True size needed

    //
    // Allocate the buffer for the Eventlog record
    //

    EventLogRecord = (PEVENTLOGRECORD) ElfpAllocateBuffer(RecordLength);

    if (EventLogRecord != (PEVENTLOGRECORD) NULL) {

        //
        // Fill up the event record
        //

        EventLogRecord->Length = RecordLength;
        RtlTimeToSecondsSince1970(
                        &pIoErrorLogMessage->TimeStamp,
                        &EventLogRecord->TimeGenerated
                        );
        EventLogRecord->Reserved = ELF_LOG_FILE_SIGNATURE;
        EventLogRecord->TimeWritten = TimeWritten;
        EventLogRecord->EventID = pIoErrorLogMessage->EntryData.ErrorCode;

        // set EventType based on the high order nibble of
        // pIoErrorLogMessage->EntryData.ErrorCode

        if (NT_INFORMATION(pIoErrorLogMessage->EntryData.ErrorCode)) {

                EventLogRecord->EventType =  EVENTLOG_INFORMATION_TYPE;

        }
        else if (NT_WARNING(pIoErrorLogMessage->EntryData.ErrorCode)) {

                EventLogRecord->EventType =  EVENTLOG_WARNING_TYPE;

        }
        else if (NT_ERROR(pIoErrorLogMessage->EntryData.ErrorCode)) {

                EventLogRecord->EventType = EVENTLOG_ERROR_TYPE;

        }
        else {

            //
            // Unknown, set to error
            //

            EventLogRecord->EventType = EVENTLOG_ERROR_TYPE;

        }

        EventLogRecord->NumStrings =
            pIoErrorLogMessage->EntryData.NumberOfStrings;
        EventLogRecord->EventCategory =
            pIoErrorLogMessage->EntryData.EventCategory;
        EventLogRecord->StringOffset = sizeof(EVENTLOGRECORD) +
           pIoErrorLogMessage->DriverNameLength + ComputerNameLength;
        EventLogRecord->DataLength =
            FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData) +
            pIoErrorLogMessage->EntryData.DumpDataSize;
        EventLogRecord->DataOffset = EventLogRecord->StringOffset +
            StringLength;

        //
        // Quota events contain a SID.

        if (pIoErrorLogMessage->EntryData.ErrorCode == IO_FILE_QUOTA_LIMIT ||
            pIoErrorLogMessage->EntryData.ErrorCode == IO_FILE_QUOTA_THRESHOLD) {

            PFILE_QUOTA_INFORMATION pFileQuotaInformation =
                (PFILE_QUOTA_INFORMATION) pIoErrorLogMessage->EntryData.DumpData;

            EventLogRecord->UserSidLength = pFileQuotaInformation->SidLength;
            EventLogRecord->UserSidOffset = EventLogRecord->DataOffset +
                    FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData) +
                    FIELD_OFFSET(FILE_QUOTA_INFORMATION, Sid);

            EventLogRecord->DataLength = EventLogRecord->UserSidOffset -
                EventLogRecord->DataOffset;

        } else {
            EventLogRecord->UserSidLength = 0;
            EventLogRecord->UserSidOffset = 0;
        }

        //
        // Fill in the variable-length fields

        // MODULENAME
        //
        // Use the driver name as the module name, since it's location is
        // described by an offset from the start of the IO_ERROR_LOG_MESSAGE
        // turn it into a pointer
        //

        DestinationString = (LPWSTR)((LPBYTE)EventLogRecord +
           sizeof(EVENTLOGRECORD));
        SourceString = (LPWSTR)((LPBYTE) pIoErrorLogMessage +
            pIoErrorLogMessage->DriverNameOffset);

        DestinationString = ElfpCopyString( DestinationString, 
                                            SourceString,
                                            pIoErrorLogMessage->DriverNameLength );

        //
        // COMPUTERNAME
        //

        DestinationString = ElfpCopyString( DestinationString,
                                            LocalComputerName,
                                            ComputerNameLength );

        //
        // STRING
        //

        DestinationString = ElfpCopyString( DestinationString, pwStart, StringLength);

        //
        // BINARY DATA
        //
        
        BinaryData = (LPBYTE) DestinationString;


        RtlMoveMemory ( BinaryData, 
                        & pIoErrorLogMessage->EntryData,
                        FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData) 
                        + pIoErrorLogMessage->EntryData.DumpDataSize );

        //
        // LENGTH at end of record
        //

        pEndLength = (PULONG)((LPBYTE) EventLogRecord + RecordLength - sizeof(ULONG));
        *pEndLength = RecordLength;

        //
        // Set up request packet.
        // Link event log record into the request structure.
        //

        Request.Module = SystemModule;
        Request.LogFile = Request.Module->LogFile;
        Request.Command = ELF_COMMAND_WRITE;
        Request.Pkt.WritePkt->Buffer = (PVOID)EventLogRecord;
        Request.Pkt.WritePkt->Datasize = RecordLength;

        //
        // Perform the operation
        //

        ElfPerformRequest( &Request );

        //
        // Free up the buffer
        //

        ElfpFreeBuffer(EventLogRecord );

        status = Request.Status;                // Set status of WRITE

    } else {
        status = STATUS_NO_MEMORY;
    }

    return (status);
}



NTSTATUS
ElfProcessSmLPCPacket (
    ULONG PacketLength,
    PSM_ERROR_LOG_MESSAGE SmErrorLogMessage
    )

/*++

Routine Description:

    This routine takes the packet received from the LPC port and processes it.
    The packet is an SM_ERROR_LOG_MESSAGE.  The logfile will be system, the 
    module name will be SMSS, the SID will always be NULL and
    there will always be one string, which will be the filename

    It extracts the information from the LPC packet, and then calls the
    common routine to do the work of formatting the data into
    an event record and writing it out to the log file.


Arguments:

    SmErrorLogMessage - Pointer to the data portion of the packet just
        received through the LPC port.


Return Value:

        Status of this operation.

--*/

{
    NTSTATUS status;
    ELF_REQUEST_RECORD  Request;
    WRITE_PKT WritePkt;

    UNICODE_STRING SystemString;
    ULONG RecordLength;
    PEVENTLOGRECORD EventLogRecord;
    LPWSTR DestinationString, SourceString;
    PBYTE BinaryData;
    ULONG PadSize;
    LARGE_INTEGER Time;
    ULONG TimeWritten;
    PULONG pEndLength;

    try {

        //
        //  Validate the packet.  
        //

        if ( 
            PacketLength < sizeof( SM_ERROR_LOG_MESSAGE ) 

                ||
            
            // 
            //  Offset begins before header
            //
             
            SmErrorLogMessage->StringOffset < sizeof( *SmErrorLogMessage )
                
                ||

            //
            //  Offset begins after packet
            //

            SmErrorLogMessage->StringOffset >= PacketLength

                ||

            //
            //  Length of string longer than packet
            //

            SmErrorLogMessage->StringLength > PacketLength

                ||

            //
            //  String end after end of packet
            //

            SmErrorLogMessage->StringOffset
            + SmErrorLogMessage->StringLength > PacketLength

             ) {

            RtlRaiseStatus( STATUS_UNSUCCESSFUL );
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {

        //
        // It's a bad packet, log it and return
        //

        ElfDbgPrintNC(( "[ELF] Bad packet from LPC port\n" ));
        ElfDbgPrintNC(( "[ELF] SmErrorLogMessage->StringOffset %x\n", SmErrorLogMessage->StringOffset ));
        ElfDbgPrintNC(( "[ELF] PacketLength %x\n", PacketLength ));
        ElfDbgPrintNC(( "[ELF] SmErrorLogMessage->StringLength %x\n", SmErrorLogMessage->StringLength ));
        ElfpCreateElfEvent( EVENT_BadDriverPacket,
                            EVENTLOG_ERROR_TYPE,
                            0,                    // EventCategory
                            0,                    // NumberOfStrings
                            NULL,                 // Strings
                            SmErrorLogMessage,    // Data
                            PacketLength,           // Datalength
                            0                     // flags
                           );
        return(STATUS_UNSUCCESSFUL);

    }

    //
    // We're going to need this everytime, so just get it once
    //

    if (!SystemModule) {

        //
        // Get the system module to log driver events
        //

        RtlInitUnicodeString( &SystemString, ELF_SYSTEM_MODULE_NAME );
        SystemModule = GetModuleStruc( &SystemString );
        ASSERT( SystemModule != NULL );
    }

    //
    // Set up write packet in request packet
    //

    Request.Pkt.WritePkt = &WritePkt;
    Request.Flags = 0;

    //
    // Generate any additional information needed in the record.
    //

    // TIMEWRITTEN
    // We need to generate a time when the log is written. This
    // gets written in the log so that we can use it to test the
    // retention period when wrapping the file.
    //


    //
    //  Determine how big a buffer is needed for the eventlog record.  
    //  We overestimate string lengths rather than probing for 
    //  terminating NUL's
    //

    RecordLength = sizeof( EVENTLOGRECORD )
                 + sizeof( L"system" )
                 + ComputerNameLength + sizeof( WCHAR )
                 + SmErrorLogMessage->StringLength + sizeof( WCHAR )
                 + sizeof( RecordLength );

    //
    //  Since the RecordLength at the end must be ULONG aligned, we round 
    //  up the total size to be ULONG aligned.
    //

    RecordLength += sizeof( ULONG ) - (RecordLength % sizeof( ULONG ));

    //
    // Allocate the buffer for the Eventlog record
    //

    EventLogRecord = (PEVENTLOGRECORD) ElfpAllocateBuffer(RecordLength);

    if (EventLogRecord == NULL) {
        return STATUS_NO_MEMORY;
    }

    //
    // Fill up the event record
    //

    EventLogRecord->Length = RecordLength;
    EventLogRecord->Reserved = ELF_LOG_FILE_SIGNATURE;
    RtlTimeToSecondsSince1970(
                    &SmErrorLogMessage->TimeStamp,
                    &EventLogRecord->TimeGenerated
                    );
    NtQuerySystemTime( &Time );
    RtlTimeToSecondsSince1970( &Time, &EventLogRecord->TimeWritten );
    EventLogRecord->EventID = SmErrorLogMessage->Status;

    //
    // set EventType based on the high order nibble of
    // the eventID
    //

    if (NT_INFORMATION( EventLogRecord->EventID )) {

            EventLogRecord->EventType =  EVENTLOG_INFORMATION_TYPE;

    } else if (NT_WARNING( EventLogRecord->EventID )) {

            EventLogRecord->EventType =  EVENTLOG_WARNING_TYPE;

    } else if (NT_ERROR( EventLogRecord->EventID )) {

            EventLogRecord->EventType = EVENTLOG_ERROR_TYPE;

    } else {

        //
        // Unknown, set to error
        //

        EventLogRecord->EventType = EVENTLOG_ERROR_TYPE;

    }

    //
    //  There is a single string;  it is the name of the file being 
    //  replaced
    //

    EventLogRecord->NumStrings = 1;
    EventLogRecord->EventCategory =  ELF_CATEGORY_SYSTEM_EVENT;
    
    //  Nothing for ReservedFlags
    //  Nothing for ClosingRecordNumber
    
    EventLogRecord->StringOffset = 
        sizeof(EVENTLOGRECORD) 
        + sizeof( L"system" )
        + ComputerNameLength;
    
    //
    //  No SID's present
    //
    
    EventLogRecord->UserSidLength = 0;
    EventLogRecord->UserSidOffset = 0;
    
    EventLogRecord->DataLength = 0;
    EventLogRecord->DataOffset = 0;

    //
    // Fill in the variable-length fields
    //
    // MODULENAME
    //
    // SMSS
    //

    DestinationString = (LPWSTR)((LPBYTE)EventLogRecord + sizeof( EVENTLOGRECORD ));

    DestinationString = ElfpCopyString( DestinationString, 
                                        L"system", 
                                        sizeof( L"system" ));
    
    //
    // COMPUTERNAME
    //

    DestinationString = ElfpCopyString( DestinationString, 
                                        LocalComputerName, 
                                        ComputerNameLength );

    //
    // STRING. 
    //

    
    SourceString = (LPWSTR)((LPBYTE)SmErrorLogMessage + SmErrorLogMessage->StringOffset);
    KdPrint(( "[ELF] String is '%*ws'\n", SmErrorLogMessage->StringLength, SourceString ));
    
    DestinationString = ElfpCopyString( DestinationString, 
                                        SourceString, 
                                        SmErrorLogMessage->StringLength );

    //
    // LENGTH at end of record
    //

    pEndLength = (PULONG)((LPBYTE) EventLogRecord + RecordLength - sizeof(ULONG));
    *pEndLength = RecordLength;

    //
    // Set up request packet.
    // Link event log record into the request structure.
    //

    Request.Module = SystemModule;
    Request.LogFile = Request.Module->LogFile;
    Request.Command = ELF_COMMAND_WRITE;
    Request.Pkt.WritePkt->Buffer = (PVOID)EventLogRecord;
    Request.Pkt.WritePkt->Datasize = RecordLength;

    //
    // Perform the operation
    //

    ElfPerformRequest( &Request );

    //
    // Free up the buffer
    //

    ElfpFreeBuffer( EventLogRecord );

    return Request.Status;
}



NTSTATUS
ElfProcessLPCCalls (
    )

/*++

Routine Description:

    This routine waits for messages to come through the LPC port to
    the system thread. When one does, it calls the appropriate routine to
    handle the API, then replies to the system thread indicating that the
    call has completed if the message was a request, if it was a datagram,
    it just waits for the next message.

Arguments:


Return Value:

--*/

{
    NTSTATUS status;

    BOOL SendReply = FALSE;

    ELF_REPLY_MESSAGE replyMessage;
    PELF_PORT_MSG receiveMessage;
    PHANDLE PortConnectionHandle;

    //
    // Loop dispatching API requests.
    //

    receiveMessage = ElfpAllocateBuffer(ELF_PORT_MAX_MESSAGE_LENGTH +
        sizeof(PORT_MESSAGE));
    if (!receiveMessage) {
        return(STATUS_NO_MEMORY);
    }

    while ( TRUE ) {

        //
        // On the first call to NtReplyWaitReceivePort, don't send a
        // reply since there's nobody to reply to.  However, on subsequent
        // calls the reply to the message from the prior time if that message
        // wasn't a LPC_DATAGRAM
        //

        status = NtReplyWaitReceivePort(
                     ElfConnectionPortHandle,
                     (PVOID)&PortConnectionHandle,
                     (PPORT_MESSAGE)( SendReply ? &replyMessage : NULL),
                     (PPORT_MESSAGE) receiveMessage
                     );

        if ( !NT_SUCCESS(status) ) {
            ElfDbgPrintNC(( "[ELF] ElfProcessLPCCalls: NtReplyWaitReceivePort failed: %X\n",
                              status ));
            return status;
        }

        ElfDbgPrint(( "[ELF] ElfProcessLPCCalls: received message\n" ));

        //
        // Take the record received and perform the operation.  Strip off
        // the PortMessage and just send the packet
        //



        //
        // Set up the response message to be sent on the next call to
        // NtReplyWaitReceivePort if this wasn't a datagram.
        // 'status' contains the status to return from this call.
        // Only process messages that are LPC_REQUEST or LPC_DATAGRAM
        //

        if (receiveMessage->PortMessage.u2.s2.Type == LPC_REQUEST ||
            receiveMessage->PortMessage.u2.s2.Type == LPC_DATAGRAM) {
            
            ElfDbgPrint(( "[ELF] Type = %x\n", receiveMessage->PortMessage.u2.s2.Type ));
            
            if (receiveMessage->MessageType == IO_ERROR_LOG) {
                ElfDbgPrint(( "[ELF] SM_IO_LOG\n" ));
                status = 
                    ElfProcessIoLPCPacket( 
                        receiveMessage->PortMessage.u1.s1.DataLength, 
                        &receiveMessage->u.IoErrorLogMessage );

            } else if (receiveMessage->MessageType == SM_ERROR_LOG) {

                ElfDbgPrint(( "[ELF] SM_ERROR_LOG\n" ));
                status = 
                    ElfProcessSmLPCPacket(
                        receiveMessage->PortMessage.u1.s1.DataLength, 
                        &receiveMessage->u.SmErrorLogMessage );

            } else {

                ElfDbgPrintNC(( "[ELF] Unknown MessageType %x\n", 
                              receiveMessage->MessageType ));

            }

            if (receiveMessage->PortMessage.u2.s2.Type == LPC_REQUEST) {
                replyMessage.PortMessage.u1.s1.DataLength =
                    sizeof(replyMessage) - sizeof(PORT_MESSAGE);
                replyMessage.PortMessage.u1.s1.TotalLength = sizeof(replyMessage);
                replyMessage.PortMessage.u2.ZeroInit = 0;
                replyMessage.PortMessage.ClientId =
                    receiveMessage->PortMessage.ClientId;
                replyMessage.PortMessage.MessageId =
                   receiveMessage->PortMessage.MessageId;
                replyMessage.Status = status;
    
                SendReply = TRUE;
            } else {
                SendReply = FALSE;
            }
            
        } else if (receiveMessage->PortMessage.u2.s2.Type == LPC_CONNECTION_REQUEST) {
            HANDLE Handle;
            PHANDLE SavedHandle = (PHANDLE) ElfpAllocateBuffer( sizeof( HANDLE ));
            BOOLEAN Accept = TRUE;

            ElfDbgPrint(( "[ELF] ElfProcessLPCCalls: Processing connection request\n" ));
            
            if (SavedHandle == NULL) {
                ElfDbgPrintNC(( "[ELF] ElfProcessLPCCalls: Unable to allocate handle save area\n" ));
                Accept = FALSE;
            }
            
            status = NtAcceptConnectPort( &Handle, 
                                          SavedHandle, 
                                          &receiveMessage->PortMessage,
                                          Accept,
                                          NULL,
                                          NULL );
            if (!Accept) {
                continue;
            }

            if (NT_SUCCESS( status )) {
                *SavedHandle = Handle;
                status = NtCompleteConnectPort( Handle );
                if (!NT_SUCCESS( status )) {
                    ElfDbgPrintNC(( "[ELF] ElfProcessLPCCalls: NtCompleteConnectPort failed %x\n", status ));
                    NtClose( Handle );
                }
            }

            if (!NT_SUCCESS( status )) {
                ElfDbgPrintNC(( "[ELF] ElfProcessLPCCalls: cleaning up failed connect\n" ));
                ElfpFreeBuffer( SavedHandle );                
            }

        } else if (receiveMessage->PortMessage.u2.s2.Type == LPC_PORT_CLOSED) {
            ElfDbgPrint(( "[ELF] ElfProcessLPCCalls: Processing Port closed\n" ));
            NtClose( *PortConnectionHandle );
            ElfpFreeBuffer( PortConnectionHandle );
        } else {
            //
            // We received a message type we didn't expect, probably due to
            // error.  BUGBUG - write an event
            //

            ElfDbgPrintNC(( "[ELF] Unexpected message type received on LPC port\n"));
            ElfDbgPrintNC(( "[ELF] Message type = %x\n", 
                            receiveMessage->PortMessage.u2.s2.Type));

        }
   }

} // ElfProcessLPCCalls



DWORD
MainLPCThread (
        LPVOID      LPCThreadParm
        )

/*++

Routine Description:

    This is the main thread that monitors the LPC port from the I/O system.
    It takes care of creating the LPC port, and waiting for input, which
    it then transforms into the right operation on the event log.


Arguments:

    NONE

Return Value:

    NONE

--*/

{
    NTSTATUS    Status;

    ElfDbgPrint(( "[ELF] Inside LPC thread\n" ));

    Status = SetUpLPCPort();

    if (NT_SUCCESS(Status)) {

        //
        // Loop forever. This thread will be killed when the service terminates.
        //
        while (TRUE) {

            Status = ElfProcessLPCCalls ();

        }

    }
    ElfDbgPrintNC (("[ELF] Error from SetUpLPCPort. Status = %lx\n", Status));

    return (Status);

    UNREFERENCED_PARAMETER ( LPCThreadParm );
}



BOOL
StartLPCThread ()

/*++

Routine Description:

    This routine starts up the thread that monitors the LPC port.

Arguments:

    NONE

Return Value:

    TRUE if thread creation succeeded, FALSE otherwise.

Note:


--*/
{
    DWORD       error;
    DWORD       ThreadId;

    ElfDbgPrint(( "[ELF] Start up the LPC thread\n" ));

    //
    // Start up the actual thread.
    //

    LPCThreadHandle = CreateThread(
                            NULL,               // lpThreadAttributes
                            4096,               // dwStackSize
                            MainLPCThread,      // lpStartAddress
                            NULL,               // lpParameter
                            0L,                 // dwCreationFlags
                            &ThreadId        // lpThreadId
                            );

    if ( LPCThreadHandle == NULL ) {
        error = GetLastError();
        ElfDbgPrintNC(( "[ELF]: LPCThread - CreateThread failed: %ld\n", error ));
        return (FALSE);
    }
    return (TRUE);
}
