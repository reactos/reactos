/* $Id: wapi.c,v 1.4 2000/02/29 23:57:46 ea Exp $
 * 
 * reactos/subsys/csrss/api/wapi.c
 *
 * Initialize the CSRSS subsystem server process.
 *
 * ReactOS Operating System
 *
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <csrss/csrss.h>

#include "api.h"

/* GLOBALS *******************************************************************/

HANDLE CsrssApiHeap;

/* FUNCTIONS *****************************************************************/

static void Thread_Api2(HANDLE ServerPort)
{
   NTSTATUS Status;
   PLPCMESSAGE LpcReply;
   LPCMESSAGE LpcRequest;
   PCSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;
   PCSRSS_PROCESS_DATA ProcessData;
   
   LpcReply = NULL;
   
   for (;;)
     {
	Status = NtReplyWaitReceivePort(ServerPort,
					0,
					LpcReply,
					&LpcRequest);
	if (!NT_SUCCESS(Status))
	  {
	     DisplayString(L"CSR: NtReplyWaitReceivePort failed\n");
	  }
	
	Request = (PCSRSS_API_REQUEST)LpcRequest.MessageData;
	
	ProcessData = CsrGetProcessData(LpcRequest.ClientProcessId);
	
	DisplayString(L"CSR: Received request\n");
	
	switch (Request->Type)
	  {
	   case CSRSS_CREATE_PROCESS:
	     Reply.Status = CsrCreateProcess(ProcessData, 
					     Request);
	     break;
	     
	   case CSRSS_TERMINATE_PROCESS:
	     Reply.Status = CsrTerminateProcess(ProcessData, 
						Request);
	     break;
	     
	   case CSRSS_WRITE_CONSOLE:
	     Reply.Status = CsrWriteConsole(ProcessData, 
					    Request, 
					    &Reply.Count);
	     break;
	     
	   case CSRSS_READ_CONSOLE:
	     Reply.Status = CsrReadConsole(ProcessData, 
					   Request,
					   &Reply.Count);
	     break;
	     
	   case CSRSS_ALLOC_CONSOLE:
	     Reply.Status = CsrAllocConsole(ProcessData, 
					    Request, 
					    &Reply.Handle);
	     break;
	     
	   case CSRSS_FREE_CONSOLE:
	     Reply.Status = CsrFreeConsole(ProcessData, 
					   Request);
	     break;
	     
	   case CSRSS_CONNECT_PROCESS:
	     Reply.Status = CsrConnectProcess(ProcessData, 
					      Request);
	     
	   default:
	     Reply.Status = STATUS_NOT_IMPLEMENTED;
	  }
	
	LpcReply = &LpcRequest;
	RtlCopyMemory(LpcReply->MessageData, &Reply, sizeof(Reply));
     }
}

/**********************************************************************
 * NAME
 *	Thread_Api
 *
 * DESCRIPTION
 * 	Handle connection requests from clients to the port
 * 	"\Windows\ApiPort".
 */
void Thread_Api(PVOID PortHandle)
{
   NTSTATUS Status;
   LPCMESSAGE Request;
   HANDLE ServerPort;
   
   CsrssApiHeap = RtlCreateHeap(HEAP_GROWABLE,
				NULL,
				65536,
				65536,
				NULL,
				NULL);
   if (CsrssApiHeap == NULL)
     {
	PrintString("CSR: Failed to create private heap, aborting\n");
	return;
     }

   CsrInitProcessData();

   for (;;)
     {
	Status = NtListenPort(PortHandle, &Request);
	if (!NT_SUCCESS(Status))
	  {
	     DisplayString(L"CSR: NtListenPort() failed\n");
	     NtTerminateThread(NtCurrentThread(), Status);
	  }
	
	Status = NtAcceptConnectPort(&ServerPort,
				     PortHandle,
				     NULL,
				     1,
				     0,
				     NULL);
	if (!NT_SUCCESS(Status))
	  {
	     DisplayString(L"CSR: NtAcceptConnectPort() failed\n");
	     NtTerminateThread(NtCurrentThread(), Status);
	  }
	
	Status = NtCompleteConnectPort(ServerPort);
	if (!NT_SUCCESS(Status))
	  {
	     DisplayString(L"CSR: NtCompleteConnectPort() failed\n");
	     NtTerminateThread(NtCurrentThread(), Status);
	  }
	
	Status = RtlCreateUserThread(NtCurrentProcess(),
				     NULL,
				     FALSE,
				     0,
				     NULL,
				     NULL,
				     (PTHREAD_START_ROUTINE)Thread_Api2,
				     ServerPort,
				     NULL,
				     NULL);
	if (!NT_SUCCESS(Status))
	  {
	     DisplayString(L"CSR: Unable to create server thread\n");
	     NtClose(ServerPort);
	     NtTerminateThread(NtCurrentThread(), Status);
	  }
     }
}

