/* $Id: conio.c,v 1.1 1999/12/22 14:48:30 dwelch Exp $
 *
 * reactos/subsys/csrss/api/conio.c
 *
 * Console I/O functions
 *
 * ReactOS Operating System
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>

#include "csrss.h"
#include "api.h"

/* GLOBALS *******************************************************************/

static HANDLE ConsoleDevice;
static HANDLE KeyboardDevice;

/* FUNCTIONS *****************************************************************/

NTSTATUS CsrWriteConsole(PCSRSS_CONIO_PROCESS ProcessData,
			 PCSRSS_REQUEST Message, 
			 PULONG CharCount)
{
   NTSTATUS Status;
   
   if (ProcessData->Console == NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   Status = NtWaitForSingleObject(ProcessData->Console->LockMutant,
				  TRUE,
				  NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   if (Status == STATUS_ALERTED ||
       Status == STATUS_USER_APC)
     {
	return(Status);
     }
   
   if (TopLevel == TRUE)
     {
	Status = NtReleaseMutant(ProcessData->Console->LockMutant, NULL);
	if (!NT_SUCCESS(Status))
	  {
	     return(Status);
	  }
	
	return(Status);
     }
   else
     {
	Status = NtReleaseMutant(ProcessData->Console->LockMutant, NULL);
	if (!NT_SUCCESS(Status))
	  {
	     return(Status);
	  }
	
	return(Status);
     }
}

PCSRSS_CONIO_PROCESS CsrAllocConioProcess(VOID)
{
   PCSRSS_CONIO_PROCESS ProcessData;
   
   ThreadData = RtlAllocHeap(NULL, HEAP_ZERO_MEMORY, 
			     sizeof(PROCESS_CONIO_THREAD));
   if (ThreadData == NULL)
     {
	return(NULL);
     }
   ProcessData->Console = NULL;
   return(ProcessData);
}

VOID CsrFreeConioProcess(PCSRSS_CONIO_PROCESS ProcessData)
{
   RtlFreeHeap(NULL, 0, ProcessData);
}



/* EOF */
