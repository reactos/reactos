/* $Id: process.c,v 1.9 2000/05/26 05:40:20 phreak Exp $
 *
 * reactos/subsys/csrss/api/process.c
 *
 * "\windows\ApiPort" port process management functions
 *
 * ReactOS Operating System
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>

#include <csrss/csrss.h>
#include <ntdll/rtl.h>
#include "api.h"

/* GLOBALS *******************************************************************/

static ULONG NrProcess;
static PCSRSS_PROCESS_DATA ProcessData[256];
extern CRITICAL_SECTION ActiveConsoleLock;
CRITICAL_SECTION ProcessDataLock;

/* FUNCTIONS *****************************************************************/

VOID CsrInitProcessData(VOID)
{
   ULONG i;

   for (i=0; i<256; i++)
     {
	ProcessData[i] = NULL;
     }
   NrProcess = 256;
   RtlInitializeCriticalSection( &ProcessDataLock );
}

PCSRSS_PROCESS_DATA CsrGetProcessData(ULONG ProcessId)
{
   ULONG i;

   RtlEnterCriticalSection( &ProcessDataLock );
   for (i=0; i<NrProcess; i++)
     {
	if (ProcessData[i] &&
	    ProcessData[i]->ProcessId == ProcessId)
	  {
	     RtlLeaveCriticalSection( &ProcessDataLock );
	     return(ProcessData[i]);
	  }
     }
   for (i=0; i<NrProcess; i++)
     {
	if (ProcessData[i] == NULL)
	  {	    
	     ProcessData[i] = RtlAllocateHeap(CsrssApiHeap,
					      HEAP_ZERO_MEMORY,
					      sizeof(CSRSS_PROCESS_DATA));
	     if (ProcessData[i] == NULL)
	       {
		  RtlLeaveCriticalSection( &ProcessDataLock );
		  return(NULL);
	       }
	     ProcessData[i]->ProcessId = ProcessId;
	     RtlLeaveCriticalSection( &ProcessDataLock );
	     return(ProcessData[i]);
	  }
     }
//   DbgPrint("CSR: CsrGetProcessData() failed\n");
   RtlLeaveCriticalSection( &ProcessDataLock );
   return(NULL);
}

NTSTATUS CsrFreeProcessData( ULONG Pid )
{
   int i;
   RtlEnterCriticalSection( &ProcessDataLock );
   for( i = 0; i < NrProcess; i++ )
      {
	 if( ProcessData[i] && ProcessData[i]->ProcessId == Pid )
	    {
	       if( ProcessData[i]->HandleTable )
		  {
		     int c;
		     for( c = 0; c < ProcessData[i]->HandleTableSize; c++ )
			if( ProcessData[i]->HandleTable[c] )
			  CsrReleaseObject( ProcessData[i], (HANDLE)((c + 1) << 2) );
		     RtlFreeHeap( CsrssApiHeap, 0, ProcessData[i]->HandleTable );
		  }
	       if( ProcessData[i]->Console )
		  {
		     RtlEnterCriticalSection( &ActiveConsoleLock );
		     if( --ProcessData[i]->Console->ReferenceCount == 0 )
			{
			   RtlLeaveCriticalSection( &ActiveConsoleLock );
			   CsrDeleteConsole( ProcessData[i], ProcessData[i]->Console );
			}
		     RtlLeaveCriticalSection( &ActiveConsoleLock );
		  }
	       RtlFreeHeap( CsrssApiHeap, 0, ProcessData[i] );
	       ProcessData[i] = 0;
	       RtlLeaveCriticalSection( &ProcessDataLock );
	       return STATUS_SUCCESS;
	    }
      }
   RtlLeaveCriticalSection( &ProcessDataLock );
   return STATUS_INVALID_PARAMETER;
}


NTSTATUS CsrCreateProcess (PCSRSS_PROCESS_DATA ProcessData,
			   PCSRSS_API_REQUEST Request,
			   PCSRSS_API_REPLY Reply)
{
   PCSRSS_PROCESS_DATA NewProcessData;
   NTSTATUS Status;
   HANDLE Process;

   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - 
     sizeof(LPC_MESSAGE_HEADER);
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   
   NewProcessData = CsrGetProcessData(Request->Data.CreateProcessRequest.NewProcessId);
   DbgPrint( "CreateProcess\n" );
   if (NewProcessData == NULL)
     {
	Reply->Status = STATUS_NO_MEMORY;
	return(STATUS_NO_MEMORY);
     }
   
   if (Request->Data.CreateProcessRequest.Flags & DETACHED_PROCESS)
     {
	NewProcessData->Console = NULL;
     }
   else if (Request->Data.CreateProcessRequest.Flags & CREATE_NEW_CONSOLE)
     {
	PCSRSS_CONSOLE Console;

	Console = RtlAllocateHeap(CsrssApiHeap,
				  HEAP_ZERO_MEMORY,
				  sizeof(CSRSS_CONSOLE));
	CsrInitConsole(ProcessData,
		       Console);
	NewProcessData->Console = Console;
	Console->ReferenceCount++;
     }
   else
     {
	NewProcessData->Console = ProcessData->Console;
	RtlEnterCriticalSection( &ActiveConsoleLock );
	ProcessData->Console->ReferenceCount++;
	RtlLeaveCriticalSection( &ActiveConsoleLock );
     }
   
   if( NewProcessData->Console )
     {
       CLIENT_ID ClientId;
       CsrInsertObject(NewProcessData,
		       &Reply->Data.CreateProcessReply.ConsoleHandle,
		       (Object_t *)NewProcessData->Console);
       ClientId.UniqueProcess = (HANDLE)NewProcessData->ProcessId;
       Status = NtOpenProcess( &Process, PROCESS_DUP_HANDLE, 0, &ClientId );
       if( !NT_SUCCESS( Status ) )
	 {
	   DbgPrint( "CSR: NtOpenProcess() failed for handle duplication\n" );
	   CsrFreeProcessData( NewProcessData->ProcessId );
	   Reply->Status = Status;
	   return Status;
	 }
       Status = NtDuplicateObject( NtCurrentProcess(), &NewProcessData->Console->ActiveEvent, Process, &NewProcessData->ConsoleEvent, SYNCHRONIZE, FALSE, 0 );
       if( !NT_SUCCESS( Status ) )
	 {
	   DbgPrint( "CSR: NtDuplicateObject() failed: %x\n", Status );
	   NtClose( Process );
	   CsrFreeProcessData( NewProcessData->ProcessId );
	   Reply->Status = Status;
	   return Status;
	 }
       NtClose( Process );
     }
   else Reply->Data.CreateProcessReply.ConsoleHandle = INVALID_HANDLE_VALUE;
//   DisplayString(L"CSR: Did CreateProcess successfully\n");
//   DbgPrint("Reply->Header.MessageSize %d\n", Reply->Header.MessageSize);
   
   return(STATUS_SUCCESS);
}

NTSTATUS CsrTerminateProcess(PCSRSS_PROCESS_DATA ProcessData,
			     PCSRSS_API_REQUEST LpcMessage,
			     PCSRSS_API_REPLY Reply)
{
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY) 
     - sizeof(LPC_MESSAGE_HEADER);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY);
   
   Reply->Status = STATUS_NOT_IMPLEMENTED;
   
   return(STATUS_NOT_IMPLEMENTED);
}

NTSTATUS CsrConnectProcess(PCSRSS_PROCESS_DATA ProcessData,
			   PCSRSS_API_REQUEST Request,
			   PCSRSS_API_REPLY Reply)
{
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - 
     sizeof(LPC_MESSAGE_HEADER);
   
   Reply->Status = STATUS_SUCCESS;
   
   return(STATUS_SUCCESS);
}

/* EOF */
