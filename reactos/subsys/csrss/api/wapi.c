/* $Id: wapi.c,v 1.5 2000/03/22 18:36:00 dwelch Exp $
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
   PCSRSS_PROCESS_DATA ProcessData;
   PCSRSS_API_REPLY Reply;
   
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
	if (LpcReply != NULL)
	  {
	     RtlFreeHeap(CsrssApiHeap,
			 0,
			 LpcReply);
	  }
	
	Request = (PCSRSS_API_REQUEST)LpcRequest.MessageData;
	LpcReply = NULL;
	Reply = NULL;
	
	ProcessData = CsrGetProcessData(LpcRequest.ClientProcessId);
	
//	DisplayString(L"CSR: Received request\n");
	
	switch (Request->Type)
	  {
	   case CSRSS_CREATE_PROCESS:
	     Status = CsrCreateProcess(ProcessData, 
				       &Request->Data.CreateProcessRequest,
				       &LpcReply);
	     break;
	     
	   case CSRSS_TERMINATE_PROCESS:
	     Status = CsrTerminateProcess(ProcessData, 
					  Request,
					  &LpcReply);
	     break;
	     
	   case CSRSS_WRITE_CONSOLE:
	     Status = CsrWriteConsole(ProcessData, 
				      Request,
				      &LpcReply);
	     break;
	     
	   case CSRSS_READ_CONSOLE:
	     Status = CsrReadConsole(ProcessData, 
				     Request,
				     &LpcReply);
	     break;
	     
	   case CSRSS_ALLOC_CONSOLE:
	     Status = CsrAllocConsole(ProcessData, 
				      Request,
				      &LpcReply);
	     break;
	     
	   case CSRSS_FREE_CONSOLE:
	     Status = CsrFreeConsole(ProcessData, 
				     Request,
				     &LpcReply);
	     break;
	     
	   case CSRSS_CONNECT_PROCESS:
	     Status = CsrConnectProcess(ProcessData, 
					Request,
					&LpcReply);
	     break;
	     
	   default:
	     LpcReply = RtlAllocateHeap(CsrssApiHeap,
					HEAP_ZERO_MEMORY,
					sizeof(LPCMESSAGE));
	     Reply = (PCSRSS_API_REPLY)(LpcReply->MessageData);
	     Reply->Status = STATUS_NOT_IMPLEMENTED;
	  }
	
	Reply = (PCSRSS_API_REPLY)(LpcReply->MessageData);
	if (Reply->Status == STATUS_SUCCESS)
	  {
//	     DisplayString(L"CSR: Returning STATUS_SUCCESS\n");
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
   CsrInitConsoleSupport();
   
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

