/* $Id: init.c,v 1.3 1999/12/22 14:48:29 dwelch Exp $
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

#if 0
static HANDLE SbApiPortHandle;
#endif


#if 0
/**********************************************************************
 * NAME
 *	Thread_SbApi
 *
 * DESCRIPTION
 * 	Handle connection requests from clients to the port
 * 	"\Windows\SbApiPort".
 */
static
void
Thread_SbApi(void * pPort)
{
	NTSTATUS	Status;
	HANDLE		Port;
	HANDLE		ConnectedPort;

	Port = * (HANDLE*) pPort;
	
	Status = NtListenPort(
			Port,
			CSRSS_SBAPI_PORT_QUEUE_SIZE
			);
	if (!NT_SUCCESS(Status))
	{
		return;
	}
	/*
	 * Wait for a client to connect
	 */
	while (TRUE)
	{
		Status = NtAcceptConnectPort(
				Port,
				& ConnectedPort
				);
		if (NT_SUCCESS(Status))
		{
			if (NT_SUCCESS(NtCompleteConnectPort(ConnectedPort)))
			{
				/* dispatch call */
				continue;
			}
			/* error: Port.CompleteConnect failed */
			continue;
		}
		/* error: Port.AcceptConnect failed */
	}
}
#endif


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
   RtlInitUnicodeString(&PortName, L"\\ApiPort");
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
	DisplayString(L"Unable to create \\ApiPort (Status %x)\n");
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
	DisplayString(L"Unable to create server thread\n");
	NtClose(ApiPortHandle);
	return FALSE;
     }
   
#if 0
   /* NEW NAMED PORT: \SbApiPort */
   Status = NtCreatePort(
			 &Server.SbApi.Port,
			& ObAttributes,
			...
			);
	if (!NT_SUCCESS(Status))
	{
		NtClose(Server.Api.Port);
		return FALSE;
	}
	Status = NtCreateThread(
			& Server.SbApi.Thread,
			Thread_SbApi,
			(void*) & Server.SbApi.Port
			);
	if (!NT_SUCCESS(Status))
	{
		NtClose(Server.Api.Port);
		NtClose(Server.SbApi.Port);
		return FALSE;
	}
#endif
     
   return TRUE;
}


/* EOF */
