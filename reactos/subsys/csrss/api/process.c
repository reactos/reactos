/* $Id: process.c,v 1.4 1999/12/30 01:51:42 dwelch Exp $
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

PCSRSS_PROCESS_DATA CsrGetProcessData(ULONG ProcessId)
{
   ULONG i;
   
   for (i=0; i<NrProcess; i++)
     {
	if (ProcessData[i]->ProcessId == ProcessId)
	  {
	     return(ProcessData[i]);
	  }
     }
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


NTSTATUS CsrCreateProcess (PCSRSS_PROCESS_DATA ProcessData,
			   PCSRSS_API_REQUEST LpcMessage)
{
   PCSRSS_CREATE_PROCESS_REQUEST Request;
   PCSRSS_PROCESS_DATA NewProcessData;
   
   Request = (PCSRSS_CREATE_PROCESS_REQUEST)LpcMessage->Data;
   
   NewProcessData = CsrGetProcessData(Request->NewProcessId);
   
   if (NewProcessData == NULL)
     {
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
   
   return(STATUS_NOT_IMPLEMENTED);
}

NTSTATUS CsrTerminateProcess(PCSRSS_PROCESS_DATA ProcessData,
			     PCSRSS_API_REQUEST LpcMessage)
{
   return(STATUS_NOT_IMPLEMENTED);
}

NTSTATUS CsrConnectProcess(PCSRSS_PROCESS_DATA ProcessData,
			   PCSRSS_API_REQUEST Request)
{
   return(STATUS_SUCCESS);
}

/* EOF */
