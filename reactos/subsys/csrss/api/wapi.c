/* $Id: wapi.c,v 1.8 2000/05/26 05:40:20 phreak Exp $
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

typedef NTSTATUS (*CsrFunc)( PCSRSS_PROCESS_DATA, PCSRSS_API_REQUEST, PCSRSS_API_REPLY );

static const CsrFunc CsrFuncs[] = {
   CsrCreateProcess,
   CsrTerminateProcess,
   CsrWriteConsole,
   CsrReadConsole,
   CsrAllocConsole,
   CsrFreeConsole,
   CsrConnectProcess,
   CsrGetScreenBufferInfo,
   CsrSetCursor,
   CsrFillOutputChar,
   CsrReadInputEvent,
   CsrWriteConsoleOutputChar,
   CsrWriteConsoleOutputAttrib,
   CsrFillOutputAttrib,
   CsrGetCursorInfo,
   CsrSetCursorInfo,
   CsrSetTextAttrib,
   0 };

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
	     CsrFreeProcessData( LpcRequest.Header.Cid.UniqueProcess );
	     NtClose(ServerPort);
	     NtTerminateThread(NtCurrentThread(), STATUS_SUCCESS);
	  }
	
	Request = (PCSRSS_API_REQUEST)&LpcRequest;
	Reply = (PCSRSS_API_REPLY)&LpcReply;
	
	ProcessData = CsrGetProcessData(
				  (ULONG)LpcRequest.Header.Cid.UniqueProcess);
	
//	DisplayString(L"CSR: Received request\n");
	if( Request->Type >= (sizeof( CsrFuncs ) / sizeof( CsrFunc )) - 1 )
	    Reply->Status = STATUS_INVALID_SYSTEM_SERVICE;
	else CsrFuncs[ Request->Type ]( ProcessData, Request, Reply );
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
   
   CsrInitProcessData();
   
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

