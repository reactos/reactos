/* $Id: port.c,v 1.11 1999/12/01 15:08:31 ekohl Exp $
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

//#define NDEBUG
#include <internal/debug.h>


/* TYPES ********************************************************************/

#define EPORT_INACTIVE                (0)
#define EPORT_WAIT_FOR_CONNECT        (1)
#define EPORT_WAIT_FOR_ACCEPT         (2)
#define EPORT_WAIT_FOR_COMPLETE_SRV   (3)
#define EPORT_WAIT_FOR_COMPLETE_CLT   (4)
#define EPORT_CONNECTED               (5)

typedef struct _QUEUED_MESSAGE
{
   LPC_MESSAGE_TYPE Type;
   ULONG Length;
   PVOID Buffer;
   DWORD Flags;
   PEPROCESS Sender;
} QUEUED_MESSAGE, *PQUEUED_MESSAGE;

typedef struct _EPORT
{
   KSPIN_LOCK Lock;
   ULONG State;
   KEVENT Event;
   struct _EPORT* OtherPort;
   ULONG NumberOfQueuedMessages;
   QUEUED_MESSAGE Msg;
   PEPROCESS ConnectingProcess;
   struct _EPORT* ConnectingPort;
} EPORT, *PEPORT;

/* GLOBALS *******************************************************************/

POBJECT_TYPE ExPortType = NULL;

/* FUNCTIONS *****************************************************************/


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
   ExPortType->Delete = NULL;
   ExPortType->Parse = NULL;
   ExPortType->Security = NULL;
   ExPortType->QueryName = NULL;
   ExPortType->OkayToClose = NULL;
   
   return(STATUS_SUCCESS);
}

static NTSTATUS NiInitializePort(PEPORT Port)
{
   memset(Port, 0, sizeof(EPORT));
   KeInitializeSpinLock(&Port->Lock);
   KeInitializeEvent(&Port->Event, NotificationEvent, FALSE);
   Port->State = EPORT_INACTIVE;
   Port->OtherPort = NULL;
   Port->NumberOfQueuedMessages = 0;
   
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL NtCreatePort(PHANDLE PortHandle,
			      ACCESS_MASK DesiredAccess,
			      POBJECT_ATTRIBUTES ObjectAttributes,
			      DWORD a3,
			      DWORD a4)
{
   PEPORT Port;
   NTSTATUS Status;
   
   Port = ObCreateObject(PortHandle,
			 1,      // DesiredAccess
			 ObjectAttributes,
			 ExPortType);
   if (Port == NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   Status = NiInitializePort(Port);
     
   return(Status);
}

NTSTATUS STDCALL NtAcceptConnectPort (IN HANDLE	PortHandle,
				      OUT PHANDLE OurPortHandle,
				      DWORD a2,
				      DWORD	a3,
				      DWORD	a4,
				      DWORD	a5)
{
   NTSTATUS Status;
   PEPORT NamedPort;
   PEPORT OurPort;
   
   Status = ObReferenceObjectByHandle(PortHandle,
				      1,   /* AccessRequired */
				      ExPortType,
				      UserMode,
				      (PVOID*)&NamedPort,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   if (NamedPort->State != EPORT_WAIT_FOR_ACCEPT)
     {
	ObDereferenceObject(NamedPort);
	return(STATUS_INVALID_PARAMETER);
     }
   
   /*
    * Create a port object for our side of the connection
    */
   OurPort = ObCreateObject(OurPortHandle,
			    1,
			    NULL,
			    ExPortType);
   
   /*
    * Connect the two port
    */
   OurPort->OtherPort = NamedPort->ConnectingPort;
   OurPort->OtherPort->OtherPort = OurPort;
   OurPort->State = EPORT_WAIT_FOR_COMPLETE_SRV;
   OurPort->OtherPort->State = EPORT_WAIT_FOR_COMPLETE_CLT;
   
   NamedPort->State = EPORT_INACTIVE;
   NamedPort->ConnectingProcess = NULL;
   NamedPort->ConnectingPort = NULL;
   
   ObDereferenceObject(NamedPort);
    
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL NtCompleteConnectPort (HANDLE PortHandle)
{
   NTSTATUS Status;
   PEPORT OurPort;
   
   Status = ObReferenceObjectByHandle(PortHandle,
				      1,   /* AccessRequired */
				      ExPortType,
				      UserMode,
				      (PVOID*)&OurPort,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   if (OurPort->State != EPORT_WAIT_FOR_COMPLETE_SRV)
     {
	ObDereferenceObject(OurPort);
	return(Status);
     }
   
   OurPort->State = EPORT_CONNECTED;
   OurPort->OtherPort->State = EPORT_CONNECTED;
   
   KeSetEvent(&OurPort->OtherPort->Event, IO_NO_INCREMENT, FALSE);
   
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL NtConnectPort (OUT	PHANDLE			ConnectedPort,
				IN	PUNICODE_STRING		PortName,
				IN	POBJECT_ATTRIBUTES	PortAttributes,
				IN	DWORD			a3,
				IN	DWORD			a4,
				IN	DWORD			a5,
				IN	DWORD			a6,
				IN	ULONG			Flags)
/*
 * FUNCTION: Connect to a named port and wait for the other side to 
 * accept the connection
 */
{
   NTSTATUS Status;
   PEPORT NamedPort;
   PEPORT OurPort;
   HANDLE OurPortHandle;
   
   Status = ObReferenceObjectByName(PortName,
				    0,
				    NULL,
				    1,  /* DesiredAccess */
				    ExPortType,
				    UserMode,
				    NULL,
				    (PVOID*)&NamedPort);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   if (NamedPort->State != EPORT_WAIT_FOR_CONNECT)
     {
	ObDereferenceObject(NamedPort);
	return(STATUS_UNSUCCESSFUL);
     }
   
   /*
    * Create a port to represent our side of the connection
    */
   OurPort = ObCreateObject(&OurPortHandle,
			    1,
			    PortAttributes,
			    ExPortType);
   NiInitializePort(OurPort);
   
   /*
    * 
    */
   NamedPort->ConnectingProcess = PsGetCurrentProcess();
   NamedPort->State = EPORT_WAIT_FOR_ACCEPT;
   NamedPort->ConnectingPort = OurPort;
   
   /*
    * Tell the other side they have a connection
    */
   KeSetEvent(&NamedPort->Event, IO_NO_INCREMENT, FALSE);
   
   /*
    * Wait for them to accept our connection
    */
   KeWaitForSingleObject(&NamedPort->Event,
			 UserRequest,
			 UserMode,
			 FALSE,
			 NULL);
   
   *ConnectedPort = OurPortHandle;
   
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL NtImpersonateClientOfPort (IN	HANDLE		PortHandle,
					    IN	PCLIENT_ID	ClientId)
{
   UNIMPLEMENTED;
}


NTSTATUS STDCALL NtListenPort (IN HANDLE PortHandle,
			       IN DWORD	QueueSize	/* guess */)
/*
 * FUNCTION: Listen on a named port and wait for a connection attempt
 */
{
   NTSTATUS Status;
   PEPORT Port;
   
   DPRINT("NtListenPort(PortHandle %x, QueueSize %d)\n",
	  PortHandle, QueueSize);
   
   Status = ObReferenceObjectByHandle(PortHandle,
				      1,   /* AccessRequired */
				      ExPortType,
				      UserMode,
				      (PVOID*)&Port,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("Failed to reference object (status %x)\n",
	       Status);
	return(Status);
     }
   
   if (Port->State != EPORT_INACTIVE)
     {
	ObDereferenceObject(Port);
	return(STATUS_INVALID_PARAMETER);
     }
   
   Port->State = EPORT_WAIT_FOR_CONNECT;
   Status = KeWaitForSingleObject(&Port->Event,
				  UserRequest,
				  UserMode,
				  FALSE,
				  NULL);
   
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
			      IN PLPC_MESSAGE LpcReply	/* guess */)
{
	UNIMPLEMENTED;
}


NTSTATUS STDCALL NtReplyWaitReceivePort ( IN	HANDLE		PortHandle,
					 IN	PLPC_MESSAGE	LpcReply,	/* guess */
					 OUT	PLPC_MESSAGE	LpcMessage,	/* guess */
					 OUT	PULONG		MessageLength	/* guess */)
{
   UNIMPLEMENTED;
}


NTSTATUS STDCALL NtReplyWaitReplyPort (IN	HANDLE		PortHandle,
				       IN OUT	PLPC_MESSAGE	LpcReply	/* guess */)
{
   UNIMPLEMENTED;
}


NTSTATUS STDCALL NtRequestPort (IN HANDLE PortHandle,
				IN PLPC_MESSAGE	LpcMessage	/* guess */)
{
   return(NtRequestWaitReplyPort(PortHandle, NULL, LpcMessage));
}


NTSTATUS STDCALL NtRequestWaitReplyPort(IN HANDLE PortHandle,
					IN OUT PLPC_MESSAGE LpcReply,	/* guess */
					OUT PLPC_MESSAGE LpcMessage 	/* guess */)
{
   NTSTATUS Status;
   PEPORT Port;
   
   Status = ObReferenceObjectByHandle(PortHandle,
				      1,   /* AccessRequired */
				      ExPortType,
				      UserMode,
				      (PVOID*)&Port,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   if (LpcMessage != NULL)
     {
	/*
	 * Put the message on the other port's queue
	 */
	Port->Msg.Type = LpcMessage->Type;
	Port->Msg.Length = LpcMessage->Length;
	Port->Msg.Buffer = ExAllocatePool(NonPagedPool, Port->Msg.Length);
	memcpy(Port->Msg.Buffer, LpcMessage->Buffer, Port->Msg.Length);
	Port->Msg.Flags = LpcMessage->Flags;
	Port->Msg.Sender = PsGetCurrentProcess();
	Port->NumberOfQueuedMessages++;
   
	/*
	 * Wake up the other side (if it's waiting)
	 */
	KeSetEvent(&Port->OtherPort->Event, IO_NO_INCREMENT, FALSE);
	
     }
   
   /*
    * If we aren't waiting for a reply then return
    */
   if (LpcReply == NULL)
     {
	ObDereferenceObject(Port);
	return(STATUS_SUCCESS);
     }
   
   /*
    * Wait the other side to reply to you
    */
   KeWaitForSingleObject(&Port->Event,
			 UserRequest,
			 UserMode,
			 FALSE,
			 NULL);
   
   /*
    * Copy the received message into the process's address space
    */
   LpcReply->Length = Port->OtherPort->Msg.Length;
   LpcReply->Type = Port->OtherPort->Msg.Type;
   memcpy(LpcReply->Buffer, Port->OtherPort->Msg.Buffer, LpcReply->Length);
   LpcReply->Flags = Port->OtherPort->Msg.Flags;
   
   /*
    * Deallocate the message and remove it from the other side's queue
    */
   ExFreePool(Port->OtherPort->Msg.Buffer);
   Port->OtherPort->NumberOfQueuedMessages--;
   
   return(STATUS_SUCCESS);
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
