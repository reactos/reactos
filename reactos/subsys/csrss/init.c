/* $Id: init.c,v 1.2 1999/07/17 23:10:30 ea Exp $
 * 
 * reactos/subsys/csrss/init.c
 *
 * Initialize the CSRSS subsystem server process.
 *
 * ReactOS Operating System
 *
 */
#define PROTO_LPC
#include <ddk/ntddk.h>
#include "csrss.h"


void Thread_Api(void*);
void Thread_SbApi(void*);

/*
 * Server's named ports.
 */
struct _SERVER_PORTS
Server =
{
	{INVALID_HANDLE_VALUE,Thread_Api},
	{INVALID_HANDLE_VALUE,Thread_SbApi}
};
/**********************************************************************
 * NAME
 *	Thread_Api
 *
 * DESCRIPTION
 * 	Handle connection requests from clients to the port
 * 	"\Windows\ApiPort".
 */
static
void
Thread_Api(void * pPort)
{
	NTSTATUS	Status;
	HANDLE		Port;
	HANDLE		ConnectedPort;

	Port = * (HANDLE*) pPort;
	
	/*
	 * Make the CSR API port listen.
	 */
	Status = NtListenPort(
			Port,
			CSRSS_API_PORT_QUEUE_SIZE
			);
	if (!NT_SUCCESS(Status))
	{
		/*
		 * FIXME: notify SM we could not 
		 * make the port listen.
		 */
		return;
	}
	/*
	 * Wait for a client to connect
	 */
	while (TRUE)
	{
		/*
		 * Wait for a connection request;
		 * the new connected port's handle
		 * is stored in ConnectedPort.
		 */
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
			/* error */
		}
	}
}


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
BOOL
InitializeServer(void)
{
	NTSTATUS		Status;
	OBJECT_ATTRIBUTES	ObAttributes;

	
	/* NEW NAMED PORT: \ApiPort */
	Status = NtCreatePort(
			& Server.Api.Port,	/* New port's handle */
			& ObAttributes,		/* Port object's attributes */
			...
			);
	if (!NT_SUCCESS(Status))
	{
		return FALSE;
	}
	Status = NtCreateThread(
			& Server.Api.Thread,
			0,			/* desired access */
			& ObAttributes,		/* object attributes */
			NtCurrentProcess(),	/* process' handle */
			0,			/* client id */
			Thread_ApiPort,
			(void*) & Server.Api.Port
			);
	if (!NT_SUCCESS(Status))
	{
		NtClose(Server.Api.Port);
		return FALSE;
	}
	/* NEW NAMED PORT: \SbApiPort */
	Status = NtCreatePort(
			& Server.SbApi.Port,
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
	return TRUE;
}


/* EOF */
