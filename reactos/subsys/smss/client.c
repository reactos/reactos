/* $Id: smss.c 12852 2005-01-06 13:58:04Z mf $
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
	return STATUS_SUCCESS;

}

/**********************************************************************
 *	SmCompleteClientInitialization/1
 *
 * DESCRIPTION
 * 	Lookup the subsystem server descriptor given the process handle
 * 	of the subsystem server process.
 */
NTSTATUS STDCALL
SmCompleteClientInitialization (HANDLE hProcess)
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
			if (hProcess == Client->ServerProcess)
			{
				Client->Initialized = TRUE;
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
 *	SmpLookupClient/1					PRIVATE
 *
 * DESCRIPTION
 * 	Lookup the subsystem server descriptor given its image ID.
 *
 * SIDE EFFECTS
 * 	SmpClientDirectory.Lock is released only on success.
 */
static PSM_CLIENT_DATA FASTCALL
SmpLookupClientUnsafe (USHORT           SubsystemId,
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

static PSM_CLIENT_DATA STDCALL
SmpLookupClient (USHORT SubsystemId)
{
	PSM_CLIENT_DATA Client = NULL;

	DPRINT("SM: %s called\n", __FUNCTION__);

	RtlEnterCriticalSection (& SmpClientDirectory.Lock);
	Client = SmpLookupClientUnsafe (SubsystemId, NULL);
	if(NULL != Client)
	{
		RtlLeaveCriticalSection (& SmpClientDirectory.Lock);
	}
	/*
	 * Note that we do *not* release SmpClientDirectory.Lock here
	 * because SmpLookupClient is called to FAIL when SubsystemId
	 * is not registered yet.
	 */
	return Client;
}

/**********************************************************************
 *	SmpCreateClient/2
 */
NTSTATUS STDCALL
SmCreateClient(PSM_PORT_MESSAGE Request, PSM_CLIENT_DATA * ClientData)
{
	PSM_CLIENT_DATA pClient = NULL;
	PSM_CONNECT_DATA ConnectData = SmpGetConnectData (Request);
	ULONG SbApiPortNameSize = SM_CONNECT_DATA_SIZE(*Request);

	DPRINT("SM: %s called\n", __FUNCTION__);

	/*
	 * Check if a client for the ID already exist.
	 */
	if (SmpLookupClient(ConnectData->Subsystem))
	{
		DPRINT("SM: %s: attempt to register again subsystem %d.\n",
			__FUNCTION__,
			ConnectData->Subsystem);
		return STATUS_UNSUCCESSFUL;
	}
	DPRINT("SM: %s: registering subsystem %d \n", __FUNCTION__, ConnectData->Subsystem);
	/*
	 * Allocate the storage for client data
	 */
	pClient = RtlAllocateHeap (SmpHeap,
				   HEAP_ZERO_MEMORY,
				   sizeof (SM_CLIENT_DATA));
	if (NULL == pClient)
	{
		DPRINT("SM: %s: out of memory!\n",__FUNCTION__);
		return STATUS_NO_MEMORY;
	}
	/*
	 * Initialize the client data
	 */
	pClient->SubsystemId = ConnectData->Subsystem;
	/* SM auto-initializes; other subsystems are required to call
	 * SM_API_COMPLETE_SESSION via SMDLL. */
	pClient->Initialized = (IMAGE_SUBSYSTEM_NATIVE == pClient->SubsystemId);
	if (SbApiPortNameSize > 0)
	{
		RtlCopyMemory (pClient->SbApiPortName,
			       ConnectData->SbName,
			       SbApiPortNameSize);
	}
	/*
	 * Insert the new descriptor in the
	 * client directory.
	 */
	if (NULL == SmpClientDirectory.Client)
	{
		SmpClientDirectory.Client = pClient;
	} else {
		PSM_CLIENT_DATA pCD = NULL;

		for (pCD=SmpClientDirectory.Client;
			(NULL != pCD->Next);
			pCD = pCD->Next);
		pCD->Next = pClient;
	}
	pClient->Next = NULL;
	++ SmpClientDirectory.Count;
	/*
	 * Note we unlock the client directory here, because
	 * it was locked by SmpLookupClient on failure.
	 */
	RtlLeaveCriticalSection (& SmpClientDirectory.Lock);
	if (ClientData) 
	{
		*ClientData = pClient;
	}
	return STATUS_SUCCESS;
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
	Client = SmpLookupClientUnsafe (SubsystemId, & Parent);
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
		RtlFreeHeap (SmpHeap, 0, Client);
		-- SmpClientDirectory.Count;
	}
	RtlLeaveCriticalSection (& SmpClientDirectory.Lock);
	return Status;
}

/* EOF */
