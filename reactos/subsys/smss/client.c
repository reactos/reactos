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

#if 0
/**********************************************************************
 *	SmpRegisterSmss/0
 */
static NTSTATUS
SmpRegisterSmss(VOID)
{
	NTSTATUS Status = STATUS_SUCCESS;
	UNICODE_STRING SbApiPortName = {0,0,NULL};
	HANDLE hSmApiPort = (HANDLE) 0;

	DPRINT("SM: %s called\n",__FUNCTION__);
	
	Status = SmConnectApiPort(& SbApiPortName,
				  (HANDLE) -1,
				  IMAGE_SUBSYSTEM_NATIVE,
				  & hSmApiPort);
	if(!NT_SUCCESS(Status))
	{
		DPRINT("SM: %s: SMDLL!SmConnectApiPort failed (Status=0x%08lx)\n",__FUNCTION__,Status);
		return Status;
	}
	Status = SmCompleteSession (hSmApiPort, (HANDLE)0, hSmApiPort);
	if(!NT_SUCCESS(Status))
	{
		DPRINT("SM: %s: SMDLL!SmCompleteSession failed (Status=0x%08lx)\n",__FUNCTION__,Status);
		return Status;
	}
	NtClose(hSmApiPort);
	return Status;
}
#endif

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
#if 0
	/* Register IMAGE_SUBSYSTE_NATIVE to be managed by SM */
	return SmpRegisterSmss();
#endif
	return STATUS_SUCCESS;

}

/**********************************************************************
 *	SmpLookupClient/1
 *
 * DESCRIPTION
 * 	Lookup the subsystem server descriptor given its image ID.
 *
 * SIDE EFFECTS
 * 	SmpClientDirectory.Lock is released only on success.
 */
static PSM_CLIENT_DATA STDCALL
SmpLookupClient (USHORT SubsystemId)
{
	PSM_CLIENT_DATA Client = NULL;

	DPRINT("SM: %s called\n", __FUNCTION__);

	RtlEnterCriticalSection (& SmpClientDirectory.Lock);
	if (SmpClientDirectory.Count > 0)
	{
		Client = SmpClientDirectory.Client;
		while (NULL != Client)
		{
			if (SubsystemId == Client->SubsystemId)
			{
				RtlLeaveCriticalSection (& SmpClientDirectory.Lock);
				return Client;
			}
			Client = Client->Next;
		}
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
	PSM_CONNECT_DATA ConnectData = (PSM_CONNECT_DATA) ((PBYTE) Request) + sizeof (LPC_REQUEST);
	ULONG SbApiPortNameSize = SM_CONNECT_DATA_SIZE(*Request);

	DPRINT("SM: %s called\n", __FUNCTION__);

	/*
	 * Check if a client for the ID already exist.
	 */
	if (SmpLookupClient(ConnectData->Subsystem))
	{
		DPRINT("SMSS: %s: attempt to register again subsystem %d.\n",
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
 */
NTSTATUS STDCALL
SmDestroyClient (ULONG SubsystemId)
{
	DPRINT("SM: %s called\n", __FUNCTION__);

	RtlEnterCriticalSection (& SmpClientDirectory.Lock);
	/* TODO */
	RtlLeaveCriticalSection (& SmpClientDirectory.Lock);
	return STATUS_SUCCESS;
}

/* EOF */
