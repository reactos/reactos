/*
 * PROJECT:         ReactOS Session Manager
 * LICENSE:         GPL v2 or later - See COPYING in the top level directory
 * FILE:            base/system/smss/debug.c
 * PURPOSE:         Debug messages switch and router.
 * PROGRAMMERS:     ReactOS Development Team
 */

/* INCLUDES ******************************************************************/
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

