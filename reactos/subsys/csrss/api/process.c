/* $Id: process.c,v 1.6 2000/03/22 18:36:00 dwelch Exp $
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

#include "api.h"

/* GLOBALS *******************************************************************/

static ULONG NrProcess;
static PCSRSS_PROCESS_DATA ProcessData[256];

/* FUNCTIONS *****************************************************************/

VOID CsrInitProcessData(VOID)
{
   ULONG i;

   for (i=0; i<256; i++)
     {
	ProcessData[i] = NULL;
     }
   NrProcess = 256;
}

PCSRSS_PROCESS_DATA CsrGetProcessData(ULONG ProcessId)
{
   ULONG i;
   
   for (i=0; i<NrProcess; i++)
     {
	if (ProcessData[i] &&
	    ProcessData[i]->ProcessId == ProcessId)
	  {
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
		  return(NULL);
	       }
	     ProcessData[i]->ProcessId = ProcessId;
	     return(ProcessData[i]);
	  }
     }
   DbgPrint("CSR: CsrGetProcessData() failed\n");
   return(NULL);
}


NTSTATUS CsrCreateProcess (PCSRSS_PROCESS_DATA ProcessData,
			   PCSRSS_CREATE_PROCESS_REQUEST Request,
			   PLPCMESSAGE* LpcReply)
{
   PCSRSS_PROCESS_DATA NewProcessData;
   PCSRSS_API_REPLY Reply;
   
   (*LpcReply) = RtlAllocateHeap(CsrssApiHeap,
				 HEAP_ZERO_MEMORY,
				 sizeof(LPCMESSAGE));
   (*LpcReply)->ActualMessageLength = sizeof(CSRSS_API_REPLY);
   (*LpcReply)->TotalMessageLength = sizeof(LPCMESSAGE);
   Reply = (PCSRSS_API_REPLY)((*LpcReply)->MessageData);
   
   NewProcessData = CsrGetProcessData(Request->NewProcessId);
   
   if (NewProcessData == NULL)
     {
	Reply->Status = STATUS_NO_MEMORY;
	return(STATUS_NO_MEMORY);
     }
   
   if (Request->Flags & DETACHED_PROCESS)
     {
	NewProcessData->Console = NULL;
     }
   else if (Request->Flags & CREATE_NEW_CONSOLE)
     {
	PCSRSS_CONSOLE Console;

	Console = RtlAllocateHeap(CsrssApiHeap,
				  HEAP_ZERO_MEMORY,
				  sizeof(CSRSS_CONSOLE));
	CsrInitConsole(ProcessData,
		       Console);
	NewProcessData->Console = Console;
     }
   else
     {
	NewProcessData->Console = ProcessData->Console;
     }
   
   CsrInsertObject(NewProcessData,
		   &Reply->Data.CreateProcessReply.ConsoleHandle,
		   NewProcessData->Console);
   
   DbgPrint("CSR: ConsoleHandle %x\n",
	    Reply->Data.CreateProcessReply.ConsoleHandle);
   DisplayString(L"CSR: Did CreateProcess successfully\n");
   
   return(STATUS_SUCCESS);
}

NTSTATUS CsrTerminateProcess(PCSRSS_PROCESS_DATA ProcessData,
			     PCSRSS_API_REQUEST LpcMessage,
			     PLPCMESSAGE* LpcReply)
{
      PCSRSS_API_REPLY Reply;
   
   (*LpcReply) = (PLPCMESSAGE)RtlAllocateHeap(CsrssApiHeap,
					      HEAP_ZERO_MEMORY,
					      sizeof(LPCMESSAGE));
   (*LpcReply)->ActualMessageLength = sizeof(CSRSS_API_REPLY);
   (*LpcReply)->TotalMessageLength = sizeof(LPCMESSAGE);
   Reply = (PCSRSS_API_REPLY)((*LpcReply)->MessageData);
   
   Reply->Status = STATUS_NOT_IMPLEMENTED;
   
   return(STATUS_NOT_IMPLEMENTED);
}

NTSTATUS CsrConnectProcess(PCSRSS_PROCESS_DATA ProcessData,
			   PCSRSS_API_REQUEST Request,
			   PLPCMESSAGE* LpcReply)
{
      PCSRSS_API_REPLY Reply;
   
   (*LpcReply) = RtlAllocateHeap(CsrssApiHeap,
				 HEAP_ZERO_MEMORY,
				 sizeof(LPCMESSAGE));
   (*LpcReply)->ActualMessageLength = sizeof(CSRSS_API_REPLY);
   (*LpcReply)->TotalMessageLength = sizeof(LPCMESSAGE);
   Reply = (PCSRSS_API_REPLY)((*LpcReply)->MessageData);
   
   Reply->Status = STATUS_SUCCESS;
   
   return(STATUS_SUCCESS);
}

/* EOF */
