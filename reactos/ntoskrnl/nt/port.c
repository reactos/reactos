/* $Id: port.c,v 1.7 1999/07/17 23:10:28 ea Exp $
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

struct _EPORT;

typedef struct _PORT_MSG
{
   ULONG DataLength;
   PVOID Data;
   LIST_ENTRY ListEntry;
   struct _EPORT* ReplyPort;
   PMDL Mdl;
} EPORT_MSG, *PEPORT_MSG;

typedef struct _EPORT
{
   PEPROCESS Owner;
   LIST_ENTRY MsgQueueHead;
   KEVENT MsgNotify;
   KSPIN_LOCK PortLock;
} EPORT, *PEPORT;

/* GLOBALS *******************************************************************/

POBJECT_TYPE ExPortType = NULL;

/* FUNCTIONS *****************************************************************/
#ifndef PROTO_LPC
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
			      POBJECT_ATTRIBUTES ObjectAttributes)
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
   InitializeListHead(&Port->MsgQueueHead);
   KeInitializeEvent(&Port->MsgNotify, NotificationEvent, FALSE);
   KeInitializeSpinLock(&Port->PortLock);
   Port->Owner = PsGetCurrentProcess();
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL NtAcceptConnectPort(VOID)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtCompleteConnectPort(VOID)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtConnectPort(PHANDLE PortHandle,
			       POBJECT_ATTRIBUTES ObjectAttributes)
{
   NTSTATUS Status;
   PEPORT Port;   

   Status = ObReferenceObjectByName(ObjectAttributes->ObjectName,
				    ObjectAttributes->Attributes,
				    NULL,
				    STANDARD_RIGHTS_REQUIRED,
				    ExPortType,
				    UserMode,
				    NULL,
				    (PVOID*)&Port);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   
   Status = ObCreateHandle(PsGetCurrentProcess(),
			   Port,
			   STANDARD_RIGHTS_REQUIRED,
			   FALSE,
			   PortHandle);
   ObDereferenceObject(Port);
   
   return(STATUS_SUCCESS);

}

NTSTATUS STDCALL NtListenPort(HANDLE PortHandle,
			      PLARGE_INTEGER Timeout,
			      PPORT_MSG_DATA Msg)
{
   NTSTATUS Status;
   PEPORT Port;
   KIRQL oldIrql;
   PEPORT_MSG KMsg;
   PVOID Data;
   
   Status = ObReferenceObjectByHandle(PortHandle,
				      STANDARD_RIGHTS_REQUIRED,
				      ExPortType,
				      UserMode,
				      (PVOID*)&Port,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   Status = KeWaitForSingleObject(&Port->MsgNotify, 0,
				  KernelMode, FALSE, Timeout);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   KeAcquireSpinLock(&Port->PortLock, &oldIrql);
   KMsg = CONTAINING_RECORD(RemoveHeadList(&Port->MsgQueueHead),
			    EPORT_MSG,
			    ListEntry);
   KeReleaseSpinLock(&Port->PortLock, oldIrql);
   
   if (KMsg->DataLength > Msg->DataLength)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   Msg->DataLength = KMsg->DataLength;
   Data = MmGetSystemAddressForMdl(KMsg->Mdl);
   memcpy(Msg->Data, Data, KMsg->DataLength);
   MmUnmapLockedPages(Data, KMsg->Mdl);
   Status = ObCreateHandle(PsGetCurrentProcess(),
			   KMsg->ReplyPort,
			   STANDARD_RIGHTS_REQUIRED,
			   FALSE,
			   &Msg->ReplyPort);
   ObDereferenceObject(PortHandle);
   return(Status);
}

NTSTATUS STDCALL NtReplyPort(VOID)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtReplyWaitReceivePort(VOID)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtReplyWaitReplyPort(VOID)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtRequestPort(HANDLE PortHandle,
			       ULONG DataLength,
			       PVOID Data,
			       ULONG Options,
			       PHANDLE ReplyPortHandle)
/*
 * FUNCTION: Send a request to a port
 * ARGUMENTS:
 *         PortHandle = Handle to the destination port
 *         DataLength = Length of the data to send
 *         Data = Data to send
 *         Option = Send options
 *         ReplyPortHandle = Optional port for reply
 */
{
   PEPORT Port;
   NTSTATUS Status;
   KIRQL oldIrql;
   PEPORT_MSG Msg;
   
   Status = ObReferenceObjectByHandle(PortHandle,
				      STANDARD_RIGHTS_REQUIRED,
				      ExPortType,
				      UserMode,
				      (PVOID*)&Port,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   Msg = ExAllocatePool(NonPagedPool, sizeof(EPORT_MSG));
   Msg->DataLength = DataLength;
   Msg->Mdl = MmCreateMdl(NULL,
			  Data,
			  Msg->DataLength);
   MmProbeAndLockPages(Msg->Mdl,
		       UserMode,
		       IoReadAccess);
   
   if (ReplyPortHandle != NULL)
     {
	NtCreatePort(ReplyPortHandle,
		     STANDARD_RIGHTS_REQUIRED,
		     NULL);
	Status = ObReferenceObjectByHandle(*ReplyPortHandle,
					   STANDARD_RIGHTS_REQUIRED,
					   ExPortType,
					   UserMode,
					   (PVOID*)&Msg->ReplyPort,
					   NULL);
	if (!NT_SUCCESS(Status))
	  {
	     ExFreePool(Msg);
	     return(Status);
	  }
     }
   
   KeAcquireSpinLock(&Port->PortLock, &oldIrql);
   InsertHeadList(&Port->MsgQueueHead, &Msg->ListEntry);
   KeReleaseSpinLock(&Port->PortLock, oldIrql);
   KeSetEvent(&Port->MsgNotify, 0, FALSE);
   
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL NtRequestWaitReplyPort(VOID)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtQueryInformationPort(VOID)
{
   UNIMPLEMENTED;
}

NTSTATUS
STDCALL
NtReadRequestData(VOID)
{
   UNIMPLEMENTED;
}

NTSTATUS
STDCALL
NtWriteRequestData(VOID)
{
   UNIMPLEMENTED;
}
#else
NTSTATUS
STDCALL
NtAcceptConnectPort ( /* @24 */
	IN	HANDLE	PortHandle,
	OUT	PHANDLE	ConnectedPort,
	DWORD	a2,
	DWORD	a3,
	DWORD	a4,
	DWORD	a5
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtCompleteConnectPort ( /* @4 */
	HANDLE	PortHandle
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtConnectPort ( /* @32 */
	OUT	PHANDLE			ConnectedPort,
	IN	PUNICODE_STRING		PortName,
	IN	POBJECT_ATTRIBUTES	PortAttributes,
	IN	DWORD			a3,
	IN	DWORD			a4,
	IN	DWORD			a5,
	IN	DWORD			a6,
	IN	ULONG			Flags
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtCreatePort ( /* @20 */
	OUT	PHANDLE			PortHandle,
	IN	ACCESS_MASK		DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes	OPTIONAL,  
	IN	DWORD			a3,	/* unknown */
	IN	DWORD			a4	/* unknown */
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtImpersonateClientOfPort ( /* @8 */
	IN	HANDLE		PortHandle,
	IN	PCLIENT_ID	ClientId
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtListenPort ( /* @8 */
	IN	HANDLE	PortHAndle,
	IN	DWORD	QueueSize	/* guess */
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtQueryInformationPort ( /* @20 */
	IN	HANDLE	PortHandle,
	IN	CINT	PortInformationClass,	/* guess */
	OUT	PVOID	PortInformation,	/* guess */
	IN	ULONG	PortInformationLength,	/* guess */
	OUT	PULONG	ReturnLength		/* guess */
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtReplyPort ( /* @8 */
	IN	HANDLE		PortHandle,
	IN	PLPC_REPLY	LpcReply	/* guess */
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtReplyWaitReceivePort ( /* @16 */
	IN	HANDLE		PortHandle,
	IN	PLPC_REPLY	LpcReply,	/* guess */
	OUT	PLPC_MESSAGE	LpcMessage,	/* guess */
	OUT	PULONG		MessageLength	/* guess */
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtReplyWaitReplyPort ( /* @8 */
	IN	HANDLE		PortHandle,
	IN OUT	PLPC_REPLY	LpcReply	/* guess */
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtRequestPort ( /* @8 */
	IN	HANDLE		PortHandle,
	IN 	PLPC_MESSAGE	LpcMessage	/* guess */
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtRequestWaitReplyPort ( /* @12 */
	IN	HANDLE		PortHandle,
	IN OUT	PLPC_REPLY	LpcReply,	/* guess */
	IN	TIME		* TimeToWait 	/* guess */
	)
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
NTSTATUS
STDCALL
NtReadRequestData ( /* @24 */
	DWORD	a0,
	DWORD	a1,
	DWORD	a2,
	DWORD	a3,
	DWORD	a4,
	DWORD	a5
	)
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
NTSTATUS
STDCALL
NtWriteRequestData ( /* @24 */
	DWORD	a0,
	DWORD	a1,
	DWORD	a2,
	DWORD	a3,
	DWORD	a4,
	DWORD	a5
	)
{
	UNIMPLEMENTED;
}
#endif /* ndef PROTO_LPC */

/* EOF */
