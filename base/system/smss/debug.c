/* $Id$
 *
 * debug.c - Session Manager debug messages switch and router
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
 * along with this software; see the file COPYING.LIB. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.
 *
 * --------------------------------------------------------------------
 */
#include "smss.h"

#define NDEBUG
#include <debug.h>


/* GLOBALS ***********************************************************/

HANDLE DbgSsApiPort = (HANDLE) 0;
HANDLE DbgUiApiPort = (HANDLE) 0;
HANDLE hSmDbgApiPort = (HANDLE) 0;

/* FUNCTIONS *********************************************************/

static VOID STDCALL
DbgSsApiPortThread (PVOID dummy)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PORT_MESSAGE	Request ;

    RtlZeroMemory(&Request, sizeof(PORT_MESSAGE));
	while (TRUE)
	{
		Status = NtListenPort (DbgSsApiPort, & Request);
		if (!NT_SUCCESS(Status))
		{
			DPRINT1("SM: %s: NtListenPort() failed! (Status==x%08lx)\n", __FUNCTION__, Status);
			break;
		}
		/* TODO */
	}
	NtTerminateThread(NtCurrentThread(),Status);
}

static VOID STDCALL
DbgUiApiPortThread (PVOID dummy)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PORT_MESSAGE	Request;

    RtlZeroMemory(&Request, sizeof(PORT_MESSAGE));
	while (TRUE)
	{
		Status = NtListenPort (DbgUiApiPort, & Request);
		if (!NT_SUCCESS(Status))
		{
			DPRINT1("SM: %s: NtListenPort() failed! (Status==x%08lx)\n", __FUNCTION__, Status);
			break;
		}
		/* TODO */
	}
	NtTerminateThread(NtCurrentThread(),Status);
}

static NTSTATUS STDCALL
SmpCreatePT (IN OUT PHANDLE hPort,
	     IN     LPWSTR  wcPortName,
	     IN     ULONG   ulMaxDataSize,
	     IN     ULONG   ulMaxMessageSize,
	     IN     ULONG   ulPoolCharge OPTIONAL,
	     IN     VOID    (STDCALL * procServingThread)(PVOID) OPTIONAL,
	     IN OUT PHANDLE phServingThread OPTIONAL)
{
	NTSTATUS          Status = STATUS_SUCCESS;
	UNICODE_STRING    PortName = {0};
	OBJECT_ATTRIBUTES ObjectAttributes;
	HANDLE            Thread = (HANDLE) 0;
	CLIENT_ID         Cid = {0, 0};

	RtlInitUnicodeString (& PortName, wcPortName);
	InitializeObjectAttributes (& ObjectAttributes,
				    & PortName,
				    0,
				    NULL,
       				    NULL);
	Status = NtCreatePort (hPort,
			       & ObjectAttributes,
       			       ulMaxDataSize,
       			       ulMaxMessageSize,
       			       ulPoolCharge);
	if(STATUS_SUCCESS != Status)
	{
		return Status;
	}
	/* Create thread for DbgSsApiPort */
	RtlCreateUserThread(NtCurrentProcess(),
			    NULL,
      			    FALSE,
      			    0,
      			    0,
      			    0,
      			    (PTHREAD_START_ROUTINE) procServingThread,
      			    hPort,
      			    & Thread,
      			    & Cid);
	if((HANDLE) 0 == Thread)
	{
		NtClose(*hPort);
		Status = STATUS_UNSUCCESSFUL;
	}
	if(NULL != phServingThread)
	{
		*phServingThread = Thread;
	}
	return Status;
}

NTSTATUS
SmInitializeDbgSs (VOID)
{
	NTSTATUS  Status = STATUS_SUCCESS;
	HANDLE    hDbgSsApiPortThread = (HANDLE) 0;


	DPRINT("SM: %s called\n", __FUNCTION__);

	/* Self register */
	Status = SmRegisterInternalSubsystem (L"Debug",
						(USHORT)-1,
						& hSmDbgApiPort);
	if (!NT_SUCCESS(Status))
	{
		DPRINT1("DBG:%s: SmRegisterInternalSubsystem failed with Status=%08lx\n",
			__FUNCTION__, Status);
		return Status;
	}
	/* Create the \DbgSsApiPort object (LPC) */
	Status = SmpCreatePT(& DbgSsApiPort,
			     SM_DBGSS_PORT_NAME,
			     0, /* MaxDataSize */
			     sizeof(PORT_MESSAGE), /* MaxMessageSize */
			     0, /* PoolCharge */
			     DbgSsApiPortThread,
			     & hDbgSsApiPortThread);
	if(!NT_SUCCESS(Status))
	{
		DPRINT("SM: %s: DBGSS port not created\n",__FUNCTION__);
		return Status;
	}
	/* Create the \DbgUiApiPort object (LPC) */
	Status = SmpCreatePT(& DbgUiApiPort,
			     SM_DBGUI_PORT_NAME,
			     0, /* MaxDataSize */
			     sizeof(PORT_MESSAGE), /* MaxMessageSize */
			     0, /* PoolCharge */
			     DbgUiApiPortThread,
			     NULL);
	if(!NT_SUCCESS(Status))
	{
		DPRINT("SM: %s: DBGUI port not created\n",__FUNCTION__);
		NtClose (hDbgSsApiPortThread);
		NtClose (DbgSsApiPort);
	  	return Status;
	}
	return STATUS_SUCCESS;
}

/* EOF */

