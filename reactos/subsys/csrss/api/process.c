/* $Id: process.c,v 1.3 1999/12/22 14:48:30 dwelch Exp $
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

/* FUNCTIONS *****************************************************************/

NTSTATUS CsrCreateProcess (PCSRSS_PROCESS_DATA ProcessData,
			   PCSRSS_API_REQUEST LpcMessage)
{
   return(STATUS_NOT_IMPLEMENTED);
}

NTSTATUS CsrTerminateProcess(PCSRSS_PROCESS_DATA ProcessData,
			     PCSRSS_API_REQUEST LpcMessage)
{
   return(STATUS_NOT_IMPLEMENTED);
}

NTSTATUS CsrConnectProcess(CSRSS_PROCESS_DATA ProcessData,
			   PCSRSS_API_REQUEST Request)
{
   HANDLE ConsoleHandle;
   
   ConsoleHandle = ((PULONG)Request.MessageData)[0];
   
   ProcessData.Console = CsrReferenceConsoleByHandle(ConsoleHandle);
   
   return(STATUS_SUCCESS);
}

/* EOF */
