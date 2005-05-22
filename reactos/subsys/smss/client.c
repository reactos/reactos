/* $Id$
 *
 * client.c - Session Manager client Management
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
#define NTOS_MODE_USER
#include <ntos.h>
#include "smss.h"
#include <sm/helper.h>

#define NDEBUG
#include <debug.h>

/* Private ADT */


struct _SM_CLIENT_DIRECTORY
{
	RTL_CRITICAL_SECTION  Lock;
	ULONG                 Count;
	PSM_CLIENT_DATA       Client;
	PSM_CLIENT_DATA       CandidateClient;

} SmpClientDirectory;


/**********************************************************************
 *	SmInitializeClientManagement/0
 */
NTSTATUS
SmInitializeClientManagement (VOID)
{
	DPRINT("SM: %s called\n", __FUNCTION__);
	RtlInitializeCriticalSection(& SmpClientDirectory.Lock);
	SmpClientDirectory.Count = 0;
	SmpClientDirectory.Client = NULL;
	SmpClientDirectory.CandidateClient = NULL;
	return STATUS_SUCCESS;

}
/**********************************************************************
 * SmpSetClientInitialized/1
 */
VOID FASTCALL
SmpSetClientInitialized (PSM_CLIENT_DATA Client)
{
	Client->Flags |= SM_CLIENT_FLAG_INITIALIZED;
}
/**********************************************************************
 *	SmpLookupClient/2					PRIVATE
 *
 * DESCRIPTION
 * 	Lookup the subsystem server descriptor given its image ID.
 *
 * ARGUMENTS
 *	SubsystemId: IMAGE_SUBSYSTEM_xxx
 *	Parent: optional: caller provided storage for the
 *		the pointer to the SM_CLIENT_DATA which
 *		Next field contains the value returned by
 *		the function (on success).
 *
 * RETURN VALUES
 *	NULL on error; otherwise a pointer to the SM_CLIENT_DATA
 *	looked up object.
 *
 * WARNING
 * 	SmpClientDirectory.Lock must be held by the caller.
 */
static PSM_CLIENT_DATA FASTCALL
SmpLookupClient (USHORT           SubsystemId,
		 PSM_CLIENT_DATA  * Parent)
{
	PSM_CLIENT_DATA Client = NULL;

	DPRINT("SM: %s(%d) called\n", __FUNCTION__, SubsystemId);
	
	if(NULL != Parent)
	{
		*Parent = NULL;
	}
	if (SmpClientDirectory.Count > 0)
	{
		Client = SmpClientDirectory.Client;
		while (NULL != Client)
		{
			if (SubsystemId == Client->SubsystemId)
			{
				break;
			}
			if(NULL != Parent)
			{
				*Parent = Client;
			}
			Client = Client->Next;
		}
	}
	return Client;
}
/**********************************************************************
 * 	SmBeginClientInitialization/1
 *
 * DESCRIPTION
 * 	Check if the candidate client matches the begin session
 * 	message from the subsystem process.
 *
 * ARGUMENTS
 *	Request: message received by \SmApiPort
 *	ClientData:
 *		
 * RETURN VALUES
 *	NTSTATUS
 */
NTSTATUS STDCALL
SmBeginClientInitialization (IN  PSM_PORT_MESSAGE Request,
			     OUT PSM_CLIENT_DATA  * ClientData)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PSM_CONNECT_DATA ConnectData = SmpGetConnectData (Request);
	ULONG SbApiPortNameSize = SM_CONNECT_DATA_SIZE(*Request);


	DPRINT("SM: %s called\n", __FUNCTION__);
	
	RtlEnterCriticalSection (& SmpClientDirectory.Lock);
	/*
	 * Is there a subsystem bootstrap in progress?
	 */
	if (SmpClientDirectory.CandidateClient)
	{
		PROCESS_BASIC_INFORMATION pbi;
		
		RtlZeroMemory (& pbi, sizeof pbi);
		Status = NtQueryInformationProcess (Request->Header.ClientId.UniqueProcess,
					    	    ProcessBasicInformation,
						    & pbi,
						    sizeof pbi,
						    NULL);
		if (NT_SUCCESS(Status))
		{
			SmpClientDirectory.CandidateClient->ServerProcessId =
				(ULONG) pbi.UniqueProcessId;
		}
	}
	else
	{
		RtlFreeHeap (SmpHeap, 0, SmpClientDirectory.CandidateClient);
		DPRINT1("SM: %s: subsys booting with no descriptor!\n", __FUNCTION__);
		Status = STATUS_NOT_FOUND;
		RtlLeaveCriticalSection (& SmpClientDirectory.Lock);
		return Status;		
	}
	/*
	 * Check if a client for the ID already exist.
	 */
	if (SmpLookupClient(ConnectData->SubSystemId, NULL))
	{
		DPRINT("SM: %s: attempt to register again subsystem %d.\n",
			__FUNCTION__,
			ConnectData->SubSystemId);
		return STATUS_UNSUCCESSFUL;
	}
	DPRINT("SM: %s: registering subsystem ID=%d \n",
		__FUNCTION__, ConnectData->SubSystemId);

	/*
	 * Initialize the client data
	 */
	SmpClientDirectory.CandidateClient->SubsystemId = ConnectData->SubSystemId;
	/* SM && DBG auto-initializes; other subsystems are required to call
	 * SM_API_COMPLETE_SESSION via SMDLL. */
	if ((IMAGE_SUBSYSTEM_NATIVE == SmpClientDirectory.CandidateClient->SubsystemId) ||
	    (IMAGE_SUBSYSTEM_UNKNOWN == SmpClientDirectory.CandidateClient->SubsystemId))
	{
		SmpSetClientInitialized (SmpClientDirectory.CandidateClient);
	}
	if (SbApiPortNameSize > 0)
	{
		RtlCopyMemory (SmpClientDirectory.CandidateClient->SbApiPortName,
			       ConnectData->SbName,
			       SbApiPortNameSize);
	}
	/*
	 * Insert the new descriptor in the
	 * client directory.
	 */
	if (NULL == SmpClientDirectory.Client)
	{
		SmpClientDirectory.Client = SmpClientDirectory.CandidateClient;
	} else {
		PSM_CLIENT_DATA pCD = NULL;

		for (pCD=SmpClientDirectory.Client;
			(NULL != pCD->Next);
			pCD = pCD->Next);
		pCD->Next = SmpClientDirectory.CandidateClient;
	}
	SmpClientDirectory.CandidateClient->Next = NULL;
	/*
	 * Increment the number of active subsystems.
	 */
	++ SmpClientDirectory.Count;
	/*
	 * Notify to the caller the reference to the client data.
	 */
	if (ClientData) 
	{
		*ClientData = SmpClientDirectory.CandidateClient;
	}
	/*
	 * Free the slot for the candidate subsystem.
	 */
	SmpClientDirectory.CandidateClient = NULL;

	RtlLeaveCriticalSection (& SmpClientDirectory.Lock);
	
	return STATUS_SUCCESS;
}
/**********************************************************************
 *	SmCompleteClientInitialization/1
 *
 * DESCRIPTION
 * 	Lookup the subsystem server descriptor given the process ID
 * 	of the subsystem server process.
 */
NTSTATUS STDCALL
SmCompleteClientInitialization (ULONG ProcessId)
{
	NTSTATUS        Status = STATUS_SUCCESS;
	PSM_CLIENT_DATA Client = NULL;

	DPRINT("SM: %s called\n", __FUNCTION__);

	RtlEnterCriticalSection (& SmpClientDirectory.Lock);
	if (SmpClientDirectory.Count > 0)
	{
		Client = SmpClientDirectory.Client;
		while (NULL != Client)
		{
			if (ProcessId == Client->ServerProcessId)
			{
				SmpSetClientInitialized (Client);
				break;
			}
			Client = Client->Next;
		}
		Status = STATUS_NOT_FOUND;
	}
	RtlLeaveCriticalSection (& SmpClientDirectory.Lock);
	return Status;
}

/**********************************************************************
 *	SmpCreateClient/1
 *
 * DESCRIPTION
 * 	Create a "candidate" client. Client descriptor will enter the
 * 	client directory only at the end of the registration
 * 	procedure. Otherwise, we will kill the associated process.
 *
 * ARGUMENTS
 * 	ProcessHandle: handle of the subsystem server process.
 *
 * RETURN VALUE
 * 	NTSTATUS:
 */
NTSTATUS STDCALL
SmCreateClient (PRTL_PROCESS_INFO ProcessInfo, PWSTR ProgramName)
{
	NTSTATUS Status = STATUS_SUCCESS;

	
	DPRINT("SM: %s(%lx) called\n", __FUNCTION__, ProcessInfo->ProcessHandle);
	RtlEnterCriticalSection (& SmpClientDirectory.Lock);
	/*
	 * Check if the candidate client slot is empty.
	 */
	if (NULL != SmpClientDirectory.CandidateClient)
	{
		DPRINT1("SM: %s: CandidateClient pending!\n", __FUNCTION__);
		RtlLeaveCriticalSection (& SmpClientDirectory.Lock);
		return STATUS_UNSUCCESSFUL;
	}
	/*
	 * Allocate the storage for client data
	 */
	SmpClientDirectory.CandidateClient =
		RtlAllocateHeap (SmpHeap,
				 HEAP_ZERO_MEMORY,
				 sizeof (SM_CLIENT_DATA));
	if (NULL == SmpClientDirectory.CandidateClient)
	{
		DPRINT("SM: %s(%lx): out of memory!\n",
			__FUNCTION__, ProcessInfo->ProcessHandle);
		Status = STATUS_NO_MEMORY;
	}
	else
	{
		/* Initialize the candidate client. */
		RtlInitializeCriticalSection(& SmpClientDirectory.CandidateClient->Lock);
		SmpClientDirectory.CandidateClient->ServerProcess =
			(HANDLE) ProcessInfo->ProcessHandle;
		SmpClientDirectory.CandidateClient->ServerProcessId = 
			(ULONG) ProcessInfo->ClientId.UniqueProcess;
	}
	/*
	 * Copy the program name
	 */
	RtlCopyMemory (SmpClientDirectory.CandidateClient->ProgramName,
		       ProgramName,
		       SM_SB_NAME_MAX_LENGTH);
	RtlLeaveCriticalSection (& SmpClientDirectory.Lock);
	return Status;
}

/**********************************************************************
 * 	SmpDestroyClient/1
 *
 * 	1. close any handle
 * 	2. kill client process
 * 	3. release resources
 */
NTSTATUS STDCALL
SmDestroyClient (ULONG SubsystemId)
{
	NTSTATUS         Status = STATUS_SUCCESS;
	PSM_CLIENT_DATA  Parent = NULL;
	PSM_CLIENT_DATA  Client = NULL;

	DPRINT("SM: %s called\n", __FUNCTION__);

	RtlEnterCriticalSection (& SmpClientDirectory.Lock);
	Client = SmpLookupClient (SubsystemId, & Parent);
	if(NULL == Client)
	{
		DPRINT1("SM: %s: del req for non existent subsystem (id=%d)\n",
			__FUNCTION__, SubsystemId);
		Status = STATUS_NOT_FOUND;
	}
	else
	{
		/* 1st in the list? */
		if(NULL == Parent)
		{
			SmpClientDirectory.Client = Client->Next;
		}
		else
		{
			if(NULL != Parent)
			{
				Parent->Next = Client->Next;
			} else {
				DPRINT1("SM: %s: n-th has no parent!\n", __FUNCTION__);
				Status = STATUS_UNSUCCESSFUL; /* FIXME */
			}
		}
		/* TODO: send shutdown or kill */
		NtTerminateProcess (Client->ServerProcess, 0); //FIXME
		RtlFreeHeap (SmpHeap, 0, Client);
		-- SmpClientDirectory.Count;
	}
	RtlLeaveCriticalSection (& SmpClientDirectory.Lock);
	return Status;
}

/* === Utilities for SmQryInfo === */

/**********************************************************************
 * SmGetClientBasicInformation/1
 */
NTSTATUS FASTCALL
SmGetClientBasicInformation (PSM_BASIC_INFORMATION i)
{
	INT              Index = 0;
	PSM_CLIENT_DATA  ClientData = NULL;

	DPRINT("SM: %s called\n", __FUNCTION__);

	RtlEnterCriticalSection (& SmpClientDirectory.Lock);

	i->SubSystemCount = SmpClientDirectory.Count;
	i->Unused = 0;
	
	if (SmpClientDirectory.Count > 0)
	{
		ClientData = SmpClientDirectory.Client;
		while ((NULL != ClientData) && (Index < SM_QRYINFO_MAX_SS_COUNT))
		{
			i->SubSystem[Index].Id        = ClientData->SubsystemId;
			i->SubSystem[Index].Flags     = ClientData->Flags;
			i->SubSystem[Index].ProcessId = ClientData->ServerProcessId;
			ClientData = ClientData->Next;
		}
	}

	RtlLeaveCriticalSection (& SmpClientDirectory.Lock);
	return STATUS_SUCCESS;
}

/**********************************************************************
 * SmGetSubSystemInformation/1
 */
NTSTATUS FASTCALL
SmGetSubSystemInformation (PSM_SUBSYSTEM_INFORMATION i)
{
	NTSTATUS         Status = STATUS_SUCCESS;
	PSM_CLIENT_DATA  ClientData = NULL;
	
	DPRINT("SM: %s called\n", __FUNCTION__);

	RtlEnterCriticalSection (& SmpClientDirectory.Lock);
	ClientData = SmpLookupClient (i->SubSystemId, NULL);
	if (NULL == ClientData)
	{
		Status = STATUS_NOT_FOUND;
	}
	else
	{
		i->Flags     = ClientData->Flags;
		i->ProcessId = ClientData->ServerProcessId;
		RtlCopyMemory (i->NameSpaceRootNode,
				ClientData->SbApiPortName,
				(SM_QRYINFO_MAX_ROOT_NODE * sizeof(i->NameSpaceRootNode[0])));
	}
	RtlLeaveCriticalSection (& SmpClientDirectory.Lock);
	return Status;
}

/* EOF */
