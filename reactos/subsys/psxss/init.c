/* $Id: init.c,v 1.1 1999/07/17 23:10:31 ea Exp $
 * 
 * init.c
 *
 * ReactOS Operating System
 *
 */
#include <ddk/ntddk.h>
#include <internal/lpc.h>

struct _SERVER_DIRECTORIES
{
	HANDLE	Root;		/* MS & Interix(tm): \PSXSS\ */
	HANDLE	Session;	/* MS & Interix(tm): \PSXSS\PSXSES\ */
	HANDLE	System;		/* Interix(tm) only: \PSXSS\SYSTEM\ */
};

struct _SERVER_PORT
{
	HANDLE	Port;
	THREAD	Thread;
};

struct _SERVER
{
	struct _SERVER_DIRECTORIES	Directory;
	struct _SERVER_PORT		Api;
	struct _SERVER_PORT		SbApi;
	
};

void Thread_Api(void*);
void Thread_SbApi(void*);

struct _SERVER
Server =
{
	{INVALID_HANDLE_VALUE,INVALID_HANDLE_VALUE},
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


/***********************************************************************
 *	InitializeServer
 *
 * DESCRIPTION
 * 	Initialize the POSIX+ subsystem server process. That is:
 *
 *	1. create the directory object "\PSXSS\";
 * 	2. create the API port "\PSXSS\ApiPort";
 * 	3. create the debug port "\PSXSS\SbApiPort";
 * 	4. create the sessions directory object "\PSXSS\PSXSES\";
 * 	5. create the system directory object "\PSXSS\SYSTEM\";
 * 	6. initialize port threads
 *
 * RETURN VALUE
 * 	TRUE	Initialization succeeded:
 * 	FALSE	Initialization failed.
 *
 * NOTE
 * 	The "\PSXSS\SYSTEM\" directory object does not exist
 * 	in the MS implementation, but appears in WNT's namespace
 * 	when the Interix(tm) subsystem is installed.
 */
BOOL
InitializeServer(void)
{
	NTSTATUS		Status;
	OBJECT_ATTRIBUTES	ObAttributes;


	/*
	 * STEP 1
	 * Create the directory object "\PSXSS\"
	 * 
	 */
	Status = NtCreateDirectoryObject(
			PSXSS_DIRECTORY_NAME_ROOT
			& Server.Directory.Root
			);
	/* 
	 * STEP 2
	 * Create the LPC port "\PSXSS\ApiPort"
	 * 
	 */
	Server.Api.Port =
		NtCreatePort(
			PSXSS_PORT_NAME_APIPORT,
			...
			);
	NtCreateThread(
		& Server.Api.Thread,
		0,			/* desired access */
		& ObAttributes,		/* object attributes */
		NtCurrentProcess(),	/* process' handle */
		0,			/* client id */
		Thread_ApiPort,
		(void*) & Server.Api.Port
		);
	/* 
	 * STEP 3
	 * Create the  LPC port "\PSXSS\SbApiPort"
	 *
	 */
	Server.SbApi.Port =
		NtCreatePort(
			PSXSS_PORT_NAME_SBAPIPORT,
			...
			);
	Status = NtCreateThread(
			& Server.SbApi.Thread,
			Thread_SbApi,
			(void*) & Server.SbApi.Port
			);
	/*
	 * STEP 4
	 * Create the POSIX+ session directory object
	 * "\PSXSS\PSXSES\"
	 *
	 */
	Status = NtCreateDirectoryObject(
			PSXSS_DIRECTORY_NAME_SESSIONS
			& Server.Directory.Sessions
			);
	/*
	 * STEP 5
	 * Create the POSIX+ system directory object
	 * "\PSXSS\SYSTEM\"
	 *
	 */
	Status = NtCreateDirectoryObject(
			PSXSS_DIRECTORY_NAME_SYSTEM
			& Server.Directory.System
			);
	return TRUE;
}


/* EOF */
