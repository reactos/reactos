/* $Id: port.c,v 1.9 1999/11/24 11:51:53 dwelch Exp $
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

#include <internal/debug.h>

/* TYPES ********************************************************************/

#define EPORT_WAIT_FOR_CONNECT       (1)
#define EPORT_WAIT_FOR_ACCEPT        (2)
#define EPORT_WAIT_FOR_COMPLETE      (3)
#define EPORT_CONNECTED              (4)

typedef struct _QUEUED_MESSAGE
{
   LPC_MESSAGE_TYPE Type;
   ULONG Length;
   PVOID Buffer;
   DWORD Flags;
   PEPROCESS Sender;
   PMDL BufferMdl;
} QUEUED_MESSAGE, *PQUEUED_MESSAGE;

typedef struct _EPORT
{
   KSPIN_LOCK Lock;
   ULONG State;
   KEVENT Event;
   struct _EPORT* ForeignPort;
   QUEUED_MESSAGE Msg;
} EPORT, *PEPORT;

/* GLOBALS *******************************************************************/

POBJECT_TYPE ExPortType = NULL;

/* FUNCTIONS *****************************************************************/


NTSTATUS NiInitPort(VOID)
{
   ExPortType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));
   
   RtlInitUnicodeString(&ExPortType->TypeName,L"Event");
   
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

NTSTATUS STDCALL NtCreatePort(PHANDLE PortHandle,
			      ACCESS_MASK DesiredAccess,
			      POBJECT_ATTRIBUTES ObjectAttributes,
			      DWORD a3,
			      DWORD a4)
{
   PEPORT Port;
   
   Port = ObCreateObject(PortHandle,
			 DesiredAccess,
			 ObjectAttributes,
			 ExPortType);
   if (Port == NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   KeInitializeSpinLock(&Port->Lock);
   KeInitializeEvent(&Port->Event, NotificationEvent, FALSE);
   Port->State = EPORT_WAIT_FOR_CONNECT;
   Port->ForeignPort = NULL;
     
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL NtAcceptConnectPort (IN	HANDLE	PortHandle,
				      OUT	PHANDLE	ConnectedPort,
				      DWORD	a2,
				      DWORD	a3,
				      DWORD	a4,
				      DWORD	a5)
{
   NTSTATUS Status;
   PEPORT Port;
   
   Status = ObReferenceObjectByHandle(PortHandle,
				      0,   /* AccessRequired */
				      ExPortType,
				      UserMode,
				      (PVOID*)&Port,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   if (Port->State != EPORT_WAIT_FOR_ACCEPT)
     {
	return(STATUS_INVALID_PARAMETER);
     }
   
   Status = ObCreateHandle(PsGetCurrentProcess(),
			   Port->ForeignPort,
			   0,   /* DesiredAccess */
			   FALSE,
			   ConnectedPort);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   KeSetEvent(&Port->ForeignPort->Event, IO_NO_INCREMENT, FALSE);
   
   Port->ForeignPort->State = EPORT_WAIT_FOR_COMPLETE;
   
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL NtCompleteConnectPort (HANDLE PortHandle)
{
   NTSTATUS Status;
   PEPORT Port;
   
   Status = ObReferenceObjectByHandle(PortHandle,
				      0,   /* AccessRequired */
				      ExPortType,
				      UserMode,
				      (PVOID*)&Port,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   Port->ForeignPort->State = EPORT_CONNECTED;
   Port->State = EPORT_CONNECTED;
   
   KeSetEvent(&Port->ForeignPort->Event, IO_NO_INCREMENT, FALSE);
   
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
{
   NTSTATUS Status;
   PEPORT ForeignPort;
   PEPORT Port;
   
   Status = ObReferenceObjectByName(PortName,
				    0,
				    NULL,
				    0,  /* DesiredAccess */
				    ExPortType,
				    UserMode,
				    NULL,
				    (PVOID*)&ForeignPort);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   if (ForeignPort->State != EPORT_WAIT_FOR_CONNECT)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   Port = ObCreateObject(ConnectedPort,
			 0,   /* DesiredAccess */
			 PortAttributes, 
			 ExPortType);
   if (Port == NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   KeInitializeSpinLock(&Port->Lock);
   KeInitializeEvent(&Port->Event, NotificationEvent, FALSE);
   Port->State = EPORT_WAIT_FOR_ACCEPT;
   Port->ForeignPort = ForeignPort;
   
   ForeignPort->State = EPORT_WAIT_FOR_ACCEPT;
   
   KeSetEvent(&ForeignPort->Event, IO_NO_INCREMENT, FALSE);
   
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL NtImpersonateClientOfPort (IN	HANDLE		PortHandle,
					    IN	PCLIENT_ID	ClientId)
{
   UNIMPLEMENTED;
}


NTSTATUS STDCALL NtListenPort (IN HANDLE PortHandle,
			       IN DWORD	QueueSize	/* guess */)
{
   NTSTATUS Status;
   PEPORT Port;
   
   Status = ObReferenceObjectByHandle(PortHandle,
				      0,   /* AccessRequired */
				      ExPortType,
				      UserMode,
				      (PVOID*)&Port,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
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
			      IN PLPC_REPLY LpcReply	/* guess */)
{
	UNIMPLEMENTED;
}


NTSTATUS STDCALL NtReplyWaitReceivePort ( IN	HANDLE		PortHandle,
					 IN	PLPC_REPLY	LpcReply,	/* guess */
					 OUT	PLPC_MESSAGE	LpcMessage,	/* guess */
					 OUT	PULONG		MessageLength	/* guess */)
{
   UNIMPLEMENTED;
}


NTSTATUS STDCALL NtReplyWaitReplyPort (IN	HANDLE		PortHandle,
				       IN OUT	PLPC_REPLY	LpcReply	/* guess */)
{
   UNIMPLEMENTED;
}


NTSTATUS STDCALL NtRequestPort (IN HANDLE PortHandle,
				IN PLPC_MESSAGE	LpcMessage	/* guess */)
{
      NTSTATUS Status;
   PEPORT Port;
   
   Status = ObReferenceObjectByHandle(PortHandle,
				      0,   /* AccessRequired */
				      ExPortType,
				      UserMode,
				      (PVOID*)&Port,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   Port->Msg.Type = LpcMessage->Type;
   Port->Msg.Length = LpcMessage->Length;
   Port->Msg.Buffer = LpcMessage->Buffer;
   Port->Msg.Flags = LpcMessage->Flags;
   Port->Msg.Sender = PsGetCurrentProcess();
   Port->Msg.BufferMdl = MmCreateMdl(NULL, 
				     LpcMessage->Buffer,
				     LpcMessage->Length);
   MmProbeAndLockPages(Port->Msg.BufferMdl,
		       UserMode,
		       IoReadAccess);
   
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL NtRequestWaitReplyPort(IN HANDLE PortHandle,
					IN OUT PLPC_REPLY LpcReply,	/* guess */
					IN TIME* TimeToWait 	/* guess */)
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
