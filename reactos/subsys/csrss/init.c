/* $Id: init.c,v 1.4 1999/12/30 01:51:41 dwelch Exp $
 * 
 * reactos/subsys/csrss/init.c
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

/* GLOBALS ******************************************************************/

/*
 * Server's named ports.
 */
static HANDLE ApiPortHandle;

/**********************************************************************
 * NAME
 * 	InitializeServer
 *
 * DESCRIPTION
 * 	Create a directory object (\windows) and two named LPC ports:
 *
 * 	1. \windows\ApiPort
 * 	2. \windows\SbApiPort
 *
 * RETURN VALUE
 * 	TRUE: Initialization OK; otherwise FALSE.
 */
BOOL InitializeServer(void)
{
   NTSTATUS		Status;
   OBJECT_ATTRIBUTES	ObAttributes;
   UNICODE_STRING PortName;
	
   /* NEW NAMED PORT: \ApiPort */
   RtlInitUnicodeString(&PortName, L"\\Windows\\ApiPort");
   InitializeObjectAttributes(&ObAttributes,
			      &PortName,
			      0,
			      NULL,
			      NULL);
   Status = NtCreatePort(&ApiPortHandle,
			 &ObAttributes,
			 260,
			 328,
			 0);
   if (!NT_SUCCESS(Status))
     {
	PrintString("Unable to create \\ApiPort (Status %x)\n", Status);
	return(FALSE);
     }

   Status = RtlCreateUserThread(NtCurrentProcess(),
				NULL,
				FALSE,
				0,
				NULL,
				NULL,
				(PTHREAD_START_ROUTINE)Thread_Api,
				ApiPortHandle,
				NULL,
				NULL);
   if (!NT_SUCCESS(Status))
     {
	PrintString("Unable to create server thread\n");
	NtClose(ApiPortHandle);
	return FALSE;
     }
   
   return TRUE;
}


/* EOF */
