/* $Id: process.c,v 1.18 2002/09/07 15:13:08 chorns Exp $
 *
 * reactos/subsys/csrss/api/process.c
 *
 * "\windows\ApiPort" port process management functions
 *
 * ReactOS Operating System
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#define NTOS_USER_MODE
#include <ntos.h>
#include <csrss/csrss.h>
#include "api.h"

#define LOCK   RtlEnterCriticalSection(&ProcessDataLock)
#define UNLOCK RtlLeaveCriticalSection(&ProcessDataLock)

/* GLOBALS *******************************************************************/

static ULONG NrProcess;
static PCSRSS_PROCESS_DATA ProcessData[256];
extern RTL_CRITICAL_SECTION ActiveConsoleLock;
RTL_CRITICAL_SECTION ProcessDataLock;

/* FUNCTIONS *****************************************************************/

VOID STDCALL CsrInitProcessData(VOID)
{
/*   ULONG i;

   for (i=0; i<256; i++)
     {
	ProcessData[i] = NULL;
     }
*/
   RtlZeroMemory (ProcessData, sizeof ProcessData);
   NrProcess = sizeof ProcessData / sizeof ProcessData[0];
   RtlInitializeCriticalSection( &ProcessDataLock );
}

PCSRSS_PROCESS_DATA STDCALL CsrGetProcessData(ULONG ProcessId)
{
   ULONG i;

   LOCK;
   for (i=0; i<NrProcess; i++)
     {
	if (ProcessData[i] &&
	    ProcessData[i]->ProcessId == ProcessId)
	  {
	     UNLOCK;
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
		  UNLOCK;
		  return(NULL);
	       }
	     ProcessData[i]->ProcessId = ProcessId;
	     UNLOCK;
	     return(ProcessData[i]);
	  }
     }
//   DbgPrint("CSR: CsrGetProcessData() failed\n");
   UNLOCK;
   return(NULL);
}

NTSTATUS STDCALL CsrFreeProcessData(ULONG Pid)
{
   int i;
   LOCK;
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
		     if( InterlockedDecrement( &(ProcessData[i]->Console->Header.ReferenceCount) ) == 0 )
			CsrDeleteConsole( ProcessData[i]->Console );
		  }
	       RtlFreeHeap( CsrssApiHeap, 0, ProcessData[i] );
	       ProcessData[i] = 0;
	       UNLOCK;
	       return STATUS_SUCCESS;
	    }
      }
   UNLOCK;
   return STATUS_INVALID_PARAMETER;
}


/**********************************************************************
 *	CSRSS API
 *********************************************************************/

CSR_API(CsrCreateProcess)
{
   PCSRSS_PROCESS_DATA NewProcessData;
   NTSTATUS Status;
   HANDLE Process;

   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - 
     sizeof(LPC_MESSAGE);
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   
   NewProcessData = CsrGetProcessData(Request->Data.CreateProcessRequest.NewProcessId);
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
	Status = CsrInitConsole(Console);
	if( !NT_SUCCESS( Status ) )
	  {
	    CsrFreeProcessData( NewProcessData->ProcessId );
	    Reply->Status = Status;
	    return Status;
	  }
	NewProcessData->Console = Console;
	Console->Header.ReferenceCount++;
     }
   else
     {
	NewProcessData->Console = ProcessData->Console;
	InterlockedIncrement( &(ProcessData->Console->Header.ReferenceCount) );
     }

   if( NewProcessData->Console )
     {
       CLIENT_ID ClientId;
       CsrInsertObject(NewProcessData,
		       &Reply->Data.CreateProcessReply.InputHandle,
		       (Object_t *)NewProcessData->Console);
       RtlEnterCriticalSection( &ActiveConsoleLock );
       CsrInsertObject( NewProcessData,
          &Reply->Data.CreateProcessReply.OutputHandle,
          &(NewProcessData->Console->ActiveBuffer->Header) );

       RtlLeaveCriticalSection( &ActiveConsoleLock );
       ClientId.UniqueProcess = (HANDLE)NewProcessData->ProcessId;
       Status = NtOpenProcess( &Process, PROCESS_DUP_HANDLE, 0, &ClientId );
       if( !NT_SUCCESS( Status ) )
	 {
	   DbgPrint( "CSR: NtOpenProcess() failed for handle duplication\n" );
	   InterlockedDecrement( &(NewProcessData->Console->Header.ReferenceCount) );
	   CsrFreeProcessData( NewProcessData->ProcessId );
	   Reply->Status = Status;
	   return Status;
	 }
       Status = NtDuplicateObject( NtCurrentProcess(), NewProcessData->Console->ActiveEvent, Process, &NewProcessData->ConsoleEvent, SYNCHRONIZE, FALSE, 0 );
       if( !NT_SUCCESS( Status ) )
	 {
	   DbgPrint( "CSR: NtDuplicateObject() failed: %x\n", Status );
	   NtClose( Process );
	   InterlockedDecrement( &(NewProcessData->Console->Header.ReferenceCount) );
	   CsrFreeProcessData( NewProcessData->ProcessId );
	   Reply->Status = Status;
	   return Status;
	 }
       NtClose( Process );
     }
   else Reply->Data.CreateProcessReply.OutputHandle = Reply->Data.CreateProcessReply.InputHandle = INVALID_HANDLE_VALUE;
   
   return(STATUS_SUCCESS);
}

CSR_API(CsrTerminateProcess)
{
   NTSTATUS Status;

   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY) 
      - sizeof(LPC_MESSAGE);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY);

   Status = CsrFreeProcessData(ProcessData->ProcessId);

   Reply->Status = Status;
   return Status;
}

CSR_API(CsrConnectProcess)
{
   Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
   Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) - 
     sizeof(LPC_MESSAGE);
   
   Reply->Status = STATUS_SUCCESS;
   
   return(STATUS_SUCCESS);
}

/* EOF */
