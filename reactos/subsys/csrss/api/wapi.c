/* $Id: wapi.c,v 1.6 2000/04/03 21:54:41 dwelch Exp $
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
   LPC_MAX_MESSAGE LpcReply;
   LPC_MAX_MESSAGE LpcRequest;
   PCSRSS_API_REQUEST Request;
   PCSRSS_PROCESS_DATA ProcessData;
   PCSRSS_API_REPLY Reply;
   
   Reply = NULL;
   
   for (;;)
     {
	Status = NtReplyWaitReceivePort(ServerPort,
					0,
					&Reply->Header,
					&LpcRequest.Header);
	if (!NT_SUCCESS(Status))
	  {
	     DisplayString(L"CSR: NtReplyWaitReceivePort failed\n");
	  }
	
	if (LpcRequest.Header.MessageType == LPC_PORT_CLOSED)
	  {
//	     DbgPrint("Client closed port\n");
	     NtClose(ServerPort);
	     NtTerminateThread(NtCurrentThread(), STATUS_SUCCESS);
	  }
	
	Request = (PCSRSS_API_REQUEST)&LpcRequest;
	Reply = (PCSRSS_API_REPLY)&LpcReply;
	
	ProcessData = CsrGetProcessData(
				  (ULONG)LpcRequest.Header.Cid.UniqueProcess);
	
//	DisplayString(L"CSR: Received request\n");
	
	switch (Request->Type)
	  {
	   case CSRSS_CREATE_PROCESS:
	     Status = CsrCreateProcess(ProcessData, 
				       &Request->Data.CreateProcessRequest,
				       Reply);
	     break;
	     
	   case CSRSS_TERMINATE_PROCESS:
	     Status = CsrTerminateProcess(ProcessData, 
					  Request,
					  Reply);
	     break;
	     
	   case CSRSS_WRITE_CONSOLE:
	     Status = CsrWriteConsole(ProcessData, 
				      Request,
				      Reply);
	     break;
	     
	   case CSRSS_READ_CONSOLE:
	     Status = CsrReadConsole(ProcessData, 
				     Request,
				     Reply);
	     break;
	     
	   case CSRSS_ALLOC_CONSOLE:
	     Status = CsrAllocConsole(ProcessData, 
				      Request,
				      Reply);
	     break;
	     
	   case CSRSS_FREE_CONSOLE:
	     Status = CsrFreeConsole(ProcessData, 
				     Request,
				     Reply);
	     break;
	     
	   case CSRSS_CONNECT_PROCESS:
	     Status = CsrConnectProcess(ProcessData, 
					Request,
					Reply);
	     break;
	     
	   default:
	     Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
	       sizeof(LPC_MESSAGE_HEADER);
	     Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
	     Reply->Status = STATUS_NOT_IMPLEMENTED;
	  }
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
   LPC_MAX_MESSAGE Request;
   HANDLE ServerPort;
   HANDLE ServerThread;
   
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
   CsrInitConsoleSupport();
   
   for (;;)
     {
	Status = NtListenPort(PortHandle, &Request.Header);
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
				     &ServerThread,
				     NULL);
	if (!NT_SUCCESS(Status))
	  {
	     DisplayString(L"CSR: Unable to create server thread\n");
	     NtClose(ServerPort);
	     NtTerminateThread(NtCurrentThread(), Status);
	  }
	NtClose(ServerThread);
     }
}

