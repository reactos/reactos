/* $Id: init.c,v 1.1 1999/06/08 22:50:59 ea Exp $
 * 
 * reactos/subsys/csrss/init.c
 *
 * Initialize the CSRSS subsystem server process.
 *
 * ReactOS Operating System
 *
 */
#include <ddk/ntddk.h>
#include <internal/lpc.h>

#if 0
struct _SERVER_PORT
{
	HANDLE	Port;
	THREAD	Thread;
};

struct _SERVER_PORTS
{
	struct _SERVER_PORT	Api;
	struct _SERVER_PORT	SbApi;
	
}

void Thread_Api(void*);
void Thread_SbApi(void*);

struct _SERVER_PORTS
Server =
{
	{INVALID_HANDLE_VALUE,Thread_Api},
	{INVALID_HANDLE_VALUE,Thread_SbApi}
};


static
void
Thread_Api(void * pPort)
{
	HANDLE	port;

	port = * (HANDLE*) pPort;
	
	NtListenPort(port);
	while (TRUE)
	{
		NtAcceptConnectPort(
			port
			);
		if (NT_SUCCESS(NtCompleteConnectPort(port)))
		{
			/* dispatch call */
		}
	}
}


static
void
Thread_SbApi(void * pPort)
{
	HANDLE	port;

	port = * (HANDLE*) pPort;
	
	NtListenPort(port);
	while (TRUE)
	{
		NtAcceptConnectPort(
			port
			);
		if (NT_SUCCESS(NtCompleteConnectPort(port)))
		{
			/* dispatch call */
			continue;
		}
		/* error */
	}
}
#endif


BOOL
InitializeServer(void)
{
	OBJECT_ATTRIBUTES	ObAttributes;
	HANDLE			MySelf;
#if 0
	MySelf = NtCurrentProcess();
	/* \ApiPort */
	Server.Api.Port =
		NtCreatePort(
			LPC_CSR_PORT_NAME_APIPORT,
			...
			);
	NtCreateThread(
		& Server.Api.Thread,
		0,			/* desired access */
		& ObAttributes,		/* object attributes */
		MySelf,			/* process' handle */
		0,			/* client id */
		Thread_ApiPort,
		(void*) & Server.Api.Port
		);
	/* \SbApiPort */
	Server.SbApi.Port =
		NtCreatePort(
			LPC_CSR_PORT_NAME_SBAPIPORT,
			...
			);
	NtCreateThread(
		& Server.SbApi.Thread,
		Thread_SbApi,
		(void*) & Server.SbApi.Port
		);
#endif
	return TRUE;
}


/* EOF */
