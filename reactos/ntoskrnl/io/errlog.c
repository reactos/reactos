/* $Id: errlog.c,v 1.4 2000/03/26 19:38:22 ea Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/errlog.c
 * PURPOSE:         Error logging
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/port.h>

#include <internal/debug.h>

/* TYPES *********************************************************************/

typedef struct _IO_ERROR_LOG_PACKET
{
   UCHAR MajorFunctionCode;
   UCHAR RetryCount;
   USHORT DumpDataSize;
   USHORT NumberOfStrings;
   USHORT StringOffset;
   USHORT EventCategory;
   NTSTATUS ErrorCode;
   ULONG UniqueErrorValue;
   NTSTATUS FinalStatus;
   ULONG SequenceNumber;
   ULONG IoControlCode;
   LARGE_INTEGER DeviceOffset;
   ULONG DumpData[1];
} IO_ERROR_LOG_PACKET, *PIO_ERROR_LOG_PACKET;

typedef struct _ERROR_LOG_MESSAGE
{
   PIO_ERROR_LOG_PACKET Packet;
   LIST_ENTRY ListEntry;
} IO_ERROR_LOG_MESSAGE, *PIO_ERROR_LOG_MESSAGE;

/* GLOBALS *******************************************************************/

static HANDLE ErrorLogPortHandle;
static HANDLE ErrorLogThreadHandle;
static PEPORT ErrorLogPort;

static LIST_ENTRY ErrorLogListHead;
static KSPIN_LOCK ErrorLogListLock;
static KSEMAPHORE ErrorLogSemaphore;

/* FUNCTIONS *****************************************************************/

static VOID IoSendErrorLogEntry(PIO_ERROR_LOG_PACKET Packet)
{
   LPCMESSAGE Message;
   ULONG Size;
   ULONG i;
   
   Size = sizeof(IO_ERROR_LOG_PACKET) +
     (Packet->DumpDataSize * sizeof(UCHAR));
     
   for (i=0; i<((Size % MAX_MESSAGE_DATA) - 1); i++)
     {
	Message.ActualMessageLength = MAX_MESSAGE_DATA;
	Message.TotalMessageLength = sizeof(LPCMESSAGE);
	Message.MessageType = i;
	memcpy(Message.MessageData, (PVOID)Packet, MAX_MESSAGE_DATA);
	LpcRequestPort(ErrorLogPort, &Message);
     }
   Message.ActualMessageLength = MAX_MESSAGE_DATA;
   Message.TotalMessageLength = sizeof(LPCMESSAGE);
   Message.MessageType = i;
   memcpy(Message.MessageData, (PVOID)Packet, Size % MAX_MESSAGE_DATA);
   LpcRequestPort(ErrorLogPort, &Message);
}

NTSTATUS IoErrorLogThreadMain(PVOID Context)
{
   NTSTATUS Status;
   LPCMESSAGE ConnectMsg;
   HANDLE PortHandle;
   PIO_ERROR_LOG_MESSAGE Message;
   KIRQL oldIrql;
   PLIST_ENTRY ListEntry;
   
   for (;;)
     {
	Status = NtListenPort(ErrorLogPortHandle, &ConnectMsg);
	if (!NT_SUCCESS(Status))
	  {
	     return(Status);
	  }
	
	Status = NtAcceptConnectPort(&PortHandle,
				     ErrorLogPortHandle,
				     NULL,
				     1,
				     0,
				     NULL);
	if (!NT_SUCCESS(Status))
	  {
	     return(Status);
	  }
	
	Status = NtCompleteConnectPort(PortHandle);
	if (!NT_SUCCESS(Status))
	  {
	     return(Status);
	  }
	
	Status = ObReferenceObjectByHandle(PortHandle,
					   PORT_ALL_ACCESS,
					   ExPortType,
					   UserMode,
					   (PVOID*)&ErrorLogPort,
					   NULL);
	if (!NT_SUCCESS(Status))
	  {
	     ZwClose(PortHandle);
	     return(Status);
	  }
	
	ZwClose(PortHandle); 
	
	for (;;)
	  {
	
	     KeWaitForSingleObject(&ErrorLogSemaphore,
				   UserRequest,
				   KernelMode,
				   FALSE,
				   NULL);
	     
	     KeAcquireSpinLock(&ErrorLogListLock, &oldIrql);
	     
	     ListEntry = RemoveHeadList(&ErrorLogListHead);

	     KeReleaseSpinLock(&ErrorLogListLock, oldIrql);
	     
	     Message = CONTAINING_RECORD(ListEntry, 
					 IO_ERROR_LOG_MESSAGE, 
					 ListEntry);
	     
	     IoSendErrorLogEntry(Message->Packet);
	
	     ExFreePool(Message->Packet);
	     ExFreePool(Message);
	  }
     }
}

NTSTATUS IoInitErrorLog(VOID)
{
   NTSTATUS Status;
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING PortName;
   CLIENT_ID Cid;
   
   InitializeListHead(&ErrorLogListHead);
   KeInitializeSpinLock(&ErrorLogListLock);
   
   KeInitializeSemaphore(&ErrorLogSemaphore,
			 0,
			 500);
   
   RtlInitUnicodeString(&PortName, L"\\ErrorLogPort");
   InitializeObjectAttributes(&ObjectAttributes,
			      &PortName,
			      0,
			      NULL,
			      NULL);
   Status = NtCreatePort(&ErrorLogPortHandle,
			 &ObjectAttributes,
			 0,
			 0,
			 0);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   Status = PsCreateSystemThread(ErrorLogThreadHandle,
				 0,
				 NULL,
				 NULL,
				 &Cid,
				 IoErrorLogThreadMain,
				 NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   return(STATUS_SUCCESS);
}


PVOID STDCALL IoAllocateErrorLogEntry(PVOID IoObject, UCHAR EntrySize)
{
   UNIMPLEMENTED;
}

VOID STDCALL IoWriteErrorLogEntry(PVOID ElEntry)
{
   KIRQL oldIrql;
   PIO_ERROR_LOG_MESSAGE Message;
   
   Message = ExAllocatePool(NonPagedPool, sizeof(IO_ERROR_LOG_MESSAGE));
   Message->Packet = (PIO_ERROR_LOG_PACKET)ElEntry;
   
   KeAcquireSpinLock(&ErrorLogListLock, &oldIrql);
   
   InsertTailList(&ErrorLogListHead, &Message->ListEntry);
   
   KeReleaseSemaphore(&ErrorLogSemaphore,
		      IO_NO_INCREMENT,
		      1,
		      FALSE);
   
   KeReleaseSpinLock(&ErrorLogListLock, oldIrql);
} 


/* EOF */
