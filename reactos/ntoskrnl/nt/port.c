/* $Id: port.c,v 1.16 2000/01/12 19:04:01 ekohl Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/nt/port.c
 * PURPOSE:         Communication mechanism (like Mach?)
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* NOTES ********************************************************************
 * 
 * This is a very rough implementation, not compatible with mach or nt
 * 
 * 
 * 
 * 
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <string.h>
#include <internal/string.h>

#define NDEBUG
#include <internal/debug.h>


/* TYPES ********************************************************************/

#define PORT_ALL_ACCESS               (0x1)

#define EPORT_INACTIVE                (0)
#define EPORT_WAIT_FOR_CONNECT        (1)
#define EPORT_WAIT_FOR_ACCEPT         (2)
#define EPORT_WAIT_FOR_COMPLETE_SRV   (3)
#define EPORT_WAIT_FOR_COMPLETE_CLT   (4)
#define EPORT_CONNECTED               (5)

struct _EPORT;

typedef struct _QUEUEDMESSAGE
{
   struct _EPORT* Sender;
   LIST_ENTRY QueueListEntry;
   LPCMESSAGE Message;
} QUEUEDMESSAGE,  *PQUEUEDMESSAGE;

typedef struct _EPORT
{
   KSPIN_LOCK Lock;
   KEVENT Event;
   
   struct _EPORT* OtherPort;
   
   ULONG QueueLength;
   LIST_ENTRY QueueListHead;
   
   ULONG ConnectQueueLength;
   LIST_ENTRY ConnectQueueListHead;
   
   ULONG MaxDataLength;
   ULONG MaxConnectInfoLength;
} EPORT, *PEPORT;

/* GLOBALS *******************************************************************/

POBJECT_TYPE ExPortType = NULL;
static ULONG EiNextLpcMessageId;

/* FUNCTIONS *****************************************************************/

VOID EiEnqueueMessagePort(PEPORT Port, PQUEUEDMESSAGE Message)
{
   InsertTailList(&Port->QueueListHead, &Message->QueueListEntry);
   Port->QueueLength++;
}

PQUEUEDMESSAGE EiDequeueMessagePort(PEPORT Port)
{
   PQUEUEDMESSAGE Message;
   PLIST_ENTRY entry;
   
   entry = RemoveHeadList(&Port->QueueListHead);
   Message = CONTAINING_RECORD(entry, QUEUEDMESSAGE, QueueListEntry);
   Port->QueueLength--;
   
   return(Message);
}

VOID EiEnqueueConnectMessagePort(PEPORT Port, PQUEUEDMESSAGE Message)
{
   InsertTailList(&Port->ConnectQueueListHead, &Message->QueueListEntry);
   Port->ConnectQueueLength++;
}

PQUEUEDMESSAGE EiDequeueConnectMessagePort(PEPORT Port)
{
   PQUEUEDMESSAGE Message;
   PLIST_ENTRY entry;
   
   entry = RemoveHeadList(&Port->ConnectQueueListHead);
   Message = CONTAINING_RECORD(entry, QUEUEDMESSAGE, QueueListEntry);
   Port->ConnectQueueLength--;
   
   return(Message);
}

NTSTATUS EiReplyOrRequestPort(PEPORT Port, 
			      PLPCMESSAGE LpcReply, 
			      ULONG MessageType,
			      PEPORT Sender)
{
   KIRQL oldIrql;
   PQUEUEDMESSAGE MessageReply;
   
   MessageReply = ExAllocatePool(NonPagedPool, sizeof(QUEUEDMESSAGE));
   MessageReply->Sender = Sender;
   
   if (LpcReply != NULL)
     {
	memcpy(&MessageReply->Message, LpcReply, sizeof(LPCMESSAGE));
     }
   
   MessageReply->Message.ClientProcessId = (DWORD)PsGetCurrentProcessId();
   MessageReply->Message.ClientThreadId = (DWORD)PsGetCurrentThreadId();
   MessageReply->Message.MessageType = MessageType;
   MessageReply->Message.MessageId = InterlockedIncrement(&EiNextLpcMessageId);
   
   KeAcquireSpinLock(&Port->Lock, &oldIrql);
   EiEnqueueMessagePort(Port, MessageReply);
   KeReleaseSpinLock(&Port->Lock, oldIrql);
   
   return(STATUS_SUCCESS);
}


VOID NiDeletePort(PVOID ObjectBody)
{
}

NTSTATUS NiCreatePort(PVOID ObjectBody,
		      PVOID Parent,
		      PWSTR RemainingPath,
		      POBJECT_ATTRIBUTES ObjectAttributes)
{
   NTSTATUS Status;
   
   if (RemainingPath == NULL)
     {
	return(STATUS_SUCCESS);
     }
   
   if (wcschr(RemainingPath+1, '\\') != NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   Status = ObReferenceObjectByPointer(Parent,
				       STANDARD_RIGHTS_REQUIRED,
				       ObDirectoryType,
				       UserMode);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   ObAddEntryDirectory(Parent, ObjectBody, RemainingPath+1);
   ObDereferenceObject(Parent);
   
   return(STATUS_SUCCESS);
}

NTSTATUS NiInitPort(VOID)
{
   ExPortType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));
   
   RtlInitUnicodeString(&ExPortType->TypeName,L"Port");
   
   ExPortType->MaxObjects = ULONG_MAX;
   ExPortType->MaxHandles = ULONG_MAX;
   ExPortType->TotalObjects = 0;
   ExPortType->TotalHandles = 0;
   ExPortType->PagedPoolCharge = 0;
   ExPortType->NonpagedPoolCharge = sizeof(EPORT);
   ExPortType->Dump = NULL;
   ExPortType->Open = NULL;
   ExPortType->Close = NULL;
   ExPortType->Delete = NiDeletePort;
   ExPortType->Parse = NULL;
   ExPortType->Security = NULL;
   ExPortType->QueryName = NULL;
   ExPortType->OkayToClose = NULL;
   ExPortType->Create = NiCreatePort;
   
   EiNextLpcMessageId = 0;
   
   return(STATUS_SUCCESS);
}

static NTSTATUS NiInitializePort(PEPORT Port)
{
   memset(Port, 0, sizeof(EPORT));
   KeInitializeSpinLock(&Port->Lock);
   KeInitializeEvent(&Port->Event, SynchronizationEvent, FALSE);
   Port->OtherPort = NULL;
   Port->QueueLength = 0;
   Port->ConnectQueueLength = 0;
   InitializeListHead(&Port->QueueListHead);
   InitializeListHead(&Port->ConnectQueueListHead);
   
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL NtCreatePort(PHANDLE PortHandle,
			      POBJECT_ATTRIBUTES ObjectAttributes,
			      ULONG MaxConnectInfoLength,
			      ULONG MaxDataLength,
			      ULONG Unknown1)
{
   PEPORT Port;
   NTSTATUS Status;
   
   DPRINT("NtCreatePort() Name %x\n", ObjectAttributes->ObjectName->Buffer);
   
   Port = ObCreateObject(PortHandle,
			 PORT_ALL_ACCESS,
			 ObjectAttributes,
			 ExPortType);
   if (Port == NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   Status = NiInitializePort(Port);
   Port->MaxConnectInfoLength = 260;
   Port->MaxDataLength = 328;
   
   return(Status);
}

NTSTATUS STDCALL NtConnectPort (OUT	PHANDLE			ConnectedPort,
				IN	PUNICODE_STRING		PortName,
				IN	PVOID     	        Unknown1,
				IN      PLPCSECTIONINFO SectionInfo,
				IN	PLPCSECTIONMAPINFO MapInfo,
				IN	PVOID Unknown2,
				IN	PVOID ConnectInfo,
				IN	PULONG uConnectInfoLength)
/*
 * FUNCTION: Connect to a named port and wait for the other side to 
 * accept the connection
 */
{
   NTSTATUS Status;
   PEPORT NamedPort;
   PEPORT OurPort;
   HANDLE OurPortHandle;
   LPCMESSAGE Request;
   PQUEUEDMESSAGE Reply;
   ULONG ConnectInfoLength;
   KIRQL oldIrql;
   
   DPRINT("PortName %x\n", PortName);
   DPRINT("NtConnectPort(PortName %S)\n", PortName->Buffer);
   
   /*
    * Copy in user parameters
    */
   memcpy(&ConnectInfoLength, uConnectInfoLength, sizeof(*uConnectInfoLength));
   
   /*
    * Get access to the port
    */
   Status = ObReferenceObjectByName(PortName,
				    0,
				    NULL,
				    PORT_ALL_ACCESS,  /* DesiredAccess */
				    ExPortType,
				    UserMode,
				    NULL,
				    (PVOID*)&NamedPort);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("Failed to reference named port (status %x)\n", Status);
	return(Status);
     }
   
   /*
    * Create a port to represent our side of the connection
    */
   OurPort = ObCreateObject(&OurPortHandle,
			    PORT_ALL_ACCESS,
			    NULL,
			    ExPortType);
   NiInitializePort(OurPort);
   
   /*
    * Create a request message
    */
   DPRINT("Creating request message\n");
   
   Request.ActualMessageLength = ConnectInfoLength;
   Request.TotalMessageLength = sizeof(LPCMESSAGE) + ConnectInfoLength;
   Request.SharedSectionSize = 0;
   if (ConnectInfo != NULL && ConnectInfoLength > 0)
     {
	memcpy(Request.MessageData, ConnectInfo, ConnectInfoLength);
     }
   
   /*
    * Queue the message to the named port
    */
   DPRINT("Queuing message\n");
   
   EiReplyOrRequestPort(NamedPort, &Request, LPC_CONNECTION_REQUEST, OurPort);
   KeSetEvent(&NamedPort->Event, IO_NO_INCREMENT, FALSE);
   
   DPRINT("Waiting for connection completion\n");
   
   /*
    * Wait for them to accept our connection
    */
   KeWaitForSingleObject(&OurPort->Event,
			 UserRequest,
			 UserMode,
			 FALSE,
			 NULL);
   
   DPRINT("Received connection completion\n");
   KeAcquireSpinLock(&OurPort->Lock, &oldIrql);
   Reply = EiDequeueMessagePort(OurPort);
   KeReleaseSpinLock(&OurPort->Lock, oldIrql);
   memcpy(ConnectInfo, Reply->Message.MessageData,
	  Reply->Message.ActualMessageLength);
   *uConnectInfoLength = Reply->Message.ActualMessageLength;
   
   if (Reply->Message.MessageType == LPC_CONNECTION_REFUSED)
     {
	ZwClose(OurPortHandle);
	ExFreePool(Reply);
	return(STATUS_UNSUCCESSFUL);
     }
   
   *ConnectedPort = OurPortHandle;
   ExFreePool(Reply);
   
   DPRINT("Exited successfully\n");
   
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL NtAcceptConnectPort (PHANDLE ServerPortHandle,
				      HANDLE NamedPortHandle,
				      PLPCMESSAGE LpcMessage,
				      ULONG AcceptIt,
				      ULONG Unknown2,
				      PLPCSECTIONMAPINFO MapInfo)
{
   NTSTATUS Status;
   PEPORT NamedPort;
   PEPORT OurPort = NULL;
   PQUEUEDMESSAGE ConnectionRequest;
   KIRQL oldIrql;
   
   Status = ObReferenceObjectByHandle(NamedPortHandle,
				      PORT_ALL_ACCESS,
				      ExPortType,
				      UserMode,
				      (PVOID*)&NamedPort,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   /*
    * Create a port object for our side of the connection
    */
   if (AcceptIt == 1)
     {
	OurPort = ObCreateObject(ServerPortHandle,
				 PORT_ALL_ACCESS,
				 NULL,
				 ExPortType);
	NiInitializePort(OurPort);
     }
   
   /*
    * Dequeue the connection request
    */
   KeAcquireSpinLock(&NamedPort->Lock, &oldIrql);
   ConnectionRequest = EiDequeueConnectMessagePort(NamedPort);
   KeReleaseSpinLock(&NamedPort->Lock, oldIrql);
      
   if (AcceptIt != 1)
     {	
	EiReplyOrRequestPort(ConnectionRequest->Sender, 
			     LpcMessage, 
			     LPC_CONNECTION_REFUSED,
			     NamedPort);
	KeSetEvent(&ConnectionRequest->Sender->Event, IO_NO_INCREMENT, FALSE);
	ExFreePool(ConnectionRequest);
	ObDereferenceObject(NamedPort);
	return(STATUS_SUCCESS);
     }
   
   /*
    * Connect the two ports
    */
   OurPort->OtherPort = ConnectionRequest->Sender;
   OurPort->OtherPort->OtherPort = OurPort;
   EiReplyOrRequestPort(ConnectionRequest->Sender, 
			LpcMessage, 
			LPC_REPLY,
			OurPort);
   ExFreePool(ConnectionRequest);
   
   ObDereferenceObject(NamedPort);
    
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL NtCompleteConnectPort (HANDLE PortHandle)
{
   NTSTATUS Status;
   PEPORT OurPort;
   
   DPRINT("NtCompleteConnectPort(PortHandle %x)\n", PortHandle);
   
   Status = ObReferenceObjectByHandle(PortHandle,
				      PORT_ALL_ACCESS,
				      ExPortType,
				      UserMode,
				      (PVOID*)&OurPort,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   KeSetEvent(&OurPort->OtherPort->Event, IO_NO_INCREMENT, FALSE);
   
   ObDereferenceObject(OurPort);
   
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL NtImpersonateClientOfPort (IN	HANDLE		PortHandle,
					    IN	PLPCMESSAGE ClientMessage)
{
   UNIMPLEMENTED;
}


NTSTATUS STDCALL NtListenPort (IN HANDLE PortHandle,
			       IN PLPCMESSAGE ConnectMsg)
/*
 * FUNCTION: Listen on a named port and wait for a connection attempt
 */
{
   NTSTATUS Status;
   
   for(;;)
     {
	Status = NtReplyWaitReceivePort(PortHandle,
					NULL,
					NULL,
					ConnectMsg);
	DPRINT("Got message (type %x)\n", LPC_CONNECTION_REQUEST);
	if (!NT_SUCCESS(Status) || 
	    ConnectMsg->MessageType == LPC_CONNECTION_REQUEST)
	  {
	     break;
	  }
     }
   return(Status);
}


NTSTATUS STDCALL NtQueryInformationPort (IN HANDLE PortHandle,
					 IN CINT PortInformationClass,	/* guess */
					 OUT PVOID PortInformation,	/* guess */
					 IN ULONG PortInformationLength,	/* guess */
					 OUT PULONG ReturnLength		/* guess */)
{
   UNIMPLEMENTED;
}


NTSTATUS STDCALL NtReplyPort (IN HANDLE PortHandle,
			      IN PLPCMESSAGE LpcReply)
{
   NTSTATUS Status;
   PEPORT Port;
   
   DPRINT("NtReplyPort(PortHandle %x, LpcReply %x)\n", PortHandle, LpcReply);
   
   Status = ObReferenceObjectByHandle(PortHandle,
				      PORT_ALL_ACCESS,   /* AccessRequired */
				      ExPortType,
				      UserMode,
				      (PVOID*)&Port,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("NtReplyPort() = %x\n", Status);
	return(Status);
     }
   
   Status = EiReplyOrRequestPort(Port->OtherPort, 
				 LpcReply, 
				 LPC_REPLY,
				 Port);
   KeSetEvent(&Port->OtherPort->Event, IO_NO_INCREMENT, FALSE);
   
   ObDereferenceObject(Port);
   
   return(Status);
}


NTSTATUS STDCALL NtReplyWaitReceivePort (IN	HANDLE		PortHandle,
					 PVOID Unknown,
					 IN	PLPCMESSAGE	LpcReply,     
					 OUT	PLPCMESSAGE	LpcMessage)
{
   NTSTATUS Status;
   PEPORT Port;
   KIRQL oldIrql;
   PQUEUEDMESSAGE Request;
   
   DPRINT("NtReplyWaitReceivePort(PortHandle %x, LpcReply %x, "
	  "LpcMessage %x)\n", PortHandle, LpcReply, LpcMessage);
   
   Status = ObReferenceObjectByHandle(PortHandle,
				      PORT_ALL_ACCESS,
				      ExPortType,
				      UserMode,
				      (PVOID*)&Port,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("NtReplyWaitReceivePort() = %x\n", Status);
	return(Status);
     }
   
   /*
    * Send the reply
    */
   if (LpcReply != NULL)
     {
	Status = EiReplyOrRequestPort(Port->OtherPort, 
				      LpcReply,
				      LPC_REPLY,
				      Port);
	KeSetEvent(&Port->OtherPort->Event, IO_NO_INCREMENT, FALSE);
	
	if (!NT_SUCCESS(Status))
	  {
	     ObDereferenceObject(Port);
	     return(Status);
	  }
     }
   
   /*
    * Want for a message to be received
    */
   DPRINT("Entering wait for message\n");
   KeWaitForSingleObject(&Port->Event,
			 UserRequest,
			 UserMode,
			 FALSE,
			 NULL);
   DPRINT("Woke from wait for message\n");
   
   /*
    * Dequeue the message
    */
   KeAcquireSpinLock(&Port->Lock, &oldIrql);
   Request = EiDequeueMessagePort(Port);
   memcpy(LpcMessage, &Request->Message, sizeof(*LpcMessage));
   if (Request->Message.MessageType == LPC_CONNECTION_REQUEST)
     {
	EiEnqueueConnectMessagePort(Port, Request);
	KeReleaseSpinLock(&Port->Lock, oldIrql);
     }
   else
     {
	KeReleaseSpinLock(&Port->Lock, oldIrql);
	ExFreePool(Request);
     }
   
   /*
    * 
    */
   ObDereferenceObject(Port);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL NtRequestPort (IN HANDLE PortHandle,
				IN PLPCMESSAGE	LpcMessage	/* guess */)
{
   NTSTATUS Status;
   PEPORT Port;
   
   DPRINT("NtRequestPort(PortHandle %x LpcMessage %x)\n", PortHandle, 
	  LpcMessage);
   
   Status = ObReferenceObjectByHandle(PortHandle,
				      PORT_ALL_ACCESS,
				      ExPortType,
				      UserMode,
				      (PVOID*)&Port,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("NtRequestPort() = %x\n", Status);
	return(Status);
     }

   Status = EiReplyOrRequestPort(Port->OtherPort, 
				 LpcMessage, 
				 LPC_DATAGRAM,
				 Port);
   KeSetEvent(&Port->OtherPort->Event, IO_NO_INCREMENT, FALSE);
   
   ObDereferenceObject(Port);
   return(Status);
}


NTSTATUS STDCALL NtRequestWaitReplyPort(IN HANDLE PortHandle,
					PLPCMESSAGE LpcRequest,    
					PLPCMESSAGE LpcReply)
{
   NTSTATUS Status;
   PEPORT Port;
   PQUEUEDMESSAGE Message;
   KIRQL oldIrql;
   
   DPRINT("NtRequestWaitReplyPort(PortHandle %x, LpcRequest %x, "
	  "LpcReply %x)\n", PortHandle, LpcRequest, LpcReply);
   
   Status = ObReferenceObjectByHandle(PortHandle,
				      PORT_ALL_ACCESS, 
				      ExPortType,
				      UserMode,
				      (PVOID*)&Port,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   
   Status = EiReplyOrRequestPort(Port->OtherPort, 
				 LpcRequest, 
				 LPC_REQUEST,
				 Port);
   KeSetEvent(&Port->OtherPort->Event, IO_NO_INCREMENT, FALSE);
   
   if (!NT_SUCCESS(Status))
     {
	ObDereferenceObject(Port);
	return(Status);
     }
   
   /*
    * Wait for a reply
    */
   KeWaitForSingleObject(&Port->Event,
			 UserRequest,
			 UserMode,
			 FALSE,
			 NULL);
   
   /*
    * Dequeue the reply
    */
   KeAcquireSpinLock(&Port->Lock, &oldIrql);
   Message = EiDequeueMessagePort(Port);
   KeReleaseSpinLock(&Port->Lock, oldIrql);
   memcpy(LpcReply, &Message->Message, sizeof(*LpcReply));
   ExFreePool(Message);
   
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL NtReplyWaitReplyPort(PVOID a, PVOID b)
{
   UNIMPLEMENTED;
}

/**********************************************************************
 * NAME							SYSTEM
 *	NtReadRequestData				NOT EXPORTED
 *
 * DESCRIPTION
 * 	Probably used only for FastLPC, to read data from the
 * 	recipient's address space directly.
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * NOTE
 * 	The number of arguments is the same as in NT's.
 *
 * REVISIONS
 * 
 */
NTSTATUS STDCALL NtReadRequestData (DWORD	a0,
				    DWORD	a1,
				    DWORD	a2,
				    DWORD	a3,
				    DWORD	a4,
				    DWORD	a5)
{
	UNIMPLEMENTED;
}


/**********************************************************************
 * NAME							SYSTEM
 *	NtWriteRequestData				NOT EXPORTED
 *
 * DESCRIPTION
 * 	Probably used only for FastLPC, to write data in the
 * 	recipient's address space directly.
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * NOTE
 * 	The number of arguments is the same as in NT's.
 *
 * REVISIONS
 * 
 */
NTSTATUS STDCALL NtWriteRequestData (DWORD	a0,
				     DWORD	a1,
				     DWORD	a2,
				     DWORD	a3,
				     DWORD	a4,
				     DWORD	a5)
{
   UNIMPLEMENTED;
}
