/* $Id: init.c,v 1.2 1999/09/07 17:12:39 ea Exp $
 * 
 * init.c
 *
 * ReactOS Operating System
 *
 * --------------------------------------------------------------------
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.  
 *
 * --------------------------------------------------------------------
 */
#define PROTO_LPC
#include <ddk/ntddk.h>

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
InitializeServer (void)
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
	if (!NT_SUCCESS(Status))
	{
		return FALSE;
	}
	/* 
	 * STEP 2
	 * Create the LPC port "\PSXSS\ApiPort"
	 * 
	 */
	ObAttributes.Name = PSXSS_PORT_NAME_APIPORT;
	Status = NtCreatePort(
			& Server.Api.Port,
			& ObAttributes,
			0,
			0,
			0,
			0
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
	/* 
	 * STEP 3
	 * Create the  LPC port "\PSXSS\SbApiPort"
	 *
	 */
	ObAttributes.Name = PSXSS_PORT_NAME_SBAPIPORT;
	Status = NtCreatePort(
			& Server.SbApi.Port,
			& ObAttributes,
			0,
			0,
			0,
			0
			);
	if (!NT_SUCCESS(Status))
	{
		NtClose(Server.Api.Port);
		NtTerminateThread(/*FIXME*/);
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
		NtTerminateThread(/*FIXME*/);
		NtClose(Server.SbApi.Port);
		return FALSE;
	}
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
	if (!NT_SUCCESS(Status))
	{
		NtClose(Server.Api.Port);
		NtTerminateThread(Server.Api.Thread);
		NtClose(Server.SbApi.Port);
		NtTerminateThread(Server.SbApi.Thread);
		return FALSE;
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
