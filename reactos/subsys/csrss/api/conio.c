/* $Id: conio.c,v 1.2 1999/12/30 01:51:42 dwelch Exp $
 *
 * reactos/subsys/csrss/api/conio.c
 *
 * Console I/O functions
 *
 * ReactOS Operating System
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>

#include <csrss/csrss.h>
#include "api.h"

/* GLOBALS *******************************************************************/

static HANDLE ConsoleDevice;
static HANDLE KeyboardDevice;

/* FUNCTIONS *****************************************************************/

NTSTATUS CsrAllocConsole(PCSRSS_PROCESS_DATA ProcessData,
			 PCSRSS_API_REQUEST LpcMessage, 
			 PHANDLE ReturnedHandle)
{
   return(STATUS_NOT_IMPLEMENTED);
}

NTSTATUS CsrFreeConsole(PCSRSS_PROCESS_DATA ProcessData,
			PCSRSS_API_REQUEST LpcMessage)
{
   return(STATUS_NOT_IMPLEMENTED);
}

NTSTATUS CsrReadConsole(PCSRSS_PROCESS_DATA ProcessData,
			PCSRSS_API_REQUEST LpcMessage, 
			PULONG CharCount)
{
   return(STATUS_NOT_IMPLEMENTED);
}


NTSTATUS CsrWriteConsole(PCSRSS_PROCESS_DATA ProcessData,
			 PCSRSS_API_REQUEST Message, 
			 PULONG CharCount)
{
   NTSTATUS Status;
   PCSRSS_CONSOLE Console;
   
   if (ProcessData->Console == NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   Console = ProcessData->Console;
   
   Status = NtWaitForSingleObject(Console->LockMutant,
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
   
   if (Console->TopLevel == TRUE)
     {
	Status = NtReleaseMutant(Console->LockMutant, NULL);
	if (!NT_SUCCESS(Status))
	  {
	     return(Status);
	  }
	
	return(Status);
     }
   else
     {
	Status = NtReleaseMutant(Console->LockMutant, NULL);
	if (!NT_SUCCESS(Status))
	  {
	     return(Status);
	  }
	
	return(Status);
     }
}

VOID CsrInitConsole(PCSRSS_PROCESS_DATA ProcessData,
		    PCSRSS_CONSOLE Console)
{
}

/* EOF */
