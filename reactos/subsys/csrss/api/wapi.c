/* $Id: wapi.c,v 1.18 2002/09/07 15:13:08 chorns Exp $
 * 
 * reactos/subsys/csrss/api/wapi.c
 *
 * Initialize the CSRSS subsystem server process.
 *
 * ReactOS Operating System
 *
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#define NTOS_USER_MODE
#include <ntos.h>
#include <csrss/csrss.h>
#include "api.h"

#include <debug.h>

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
   CsrGetConsoleMode,
   CsrSetConsoleMode,
   CsrCreateScreenBuffer,
   CsrSetScreenBuffer,
   CsrSetTitle,
   CsrGetTitle,
   CsrWriteConsoleOutput,
   CsrFlushInputBuffer,
   CsrScrollConsoleScreenBuffer,
   CsrReadConsoleOutputChar,
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
	if ( !NT_SUCCESS( Status ) )
	  {
	     DisplayString(L"CSR: NtReplyWaitReceivePort failed\n");
	  }
	
	if ( LpcRequest.Header.MessageType == LPC_PORT_CLOSED )

	  {
	     CsrFreeProcessData( (ULONG)LpcRequest.Header.ClientId.UniqueProcess );
	     NtClose(ServerPort);
	     NtTerminateThread(NtCurrentThread(), STATUS_SUCCESS);
	     continue;
	  }
	
	Request = (PCSRSS_API_REQUEST)&LpcRequest;
	Reply = (PCSRSS_API_REPLY)&LpcReply;
	
	ProcessData = CsrGetProcessData(
				  (ULONG)LpcRequest.Header.ClientId.UniqueProcess);
	
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
   PCSRSS_PROCESS_DATA ProcessData;
   
   CsrInitProcessData();
   
   for (;;)
     {
       LPC_SECTION_READ LpcRead;

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
				     &LpcRead);
	if (!NT_SUCCESS(Status))
	  {
	     DisplayString(L"CSR: NtAcceptConnectPort() failed\n");
	     NtTerminateThread(NtCurrentThread(), Status);
	  }

	ProcessData = CsrGetProcessData(Request.Header.ClientId.UniqueProcess);
	ProcessData->CsrSectionViewBase = LpcRead.ViewBase;
	ProcessData->CsrSectionViewSize = LpcRead.ViewSize;
	
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

/* EOF */
