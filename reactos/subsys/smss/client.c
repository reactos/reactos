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
#include <sm/api.h>
#include "smss.h"

/* Private ADT */

typedef struct _SM_CLIENT_DATA
{
	USHORT	SubsystemId;
	BOOL	Initialized;
	HANDLE	ServerProcess;
	HANDLE	ApiPort;
	HANDLE	SbApiPort;
	struct _SM_CLIENT_DATA * Next;
	
} SM_CLIENT_DATA, *PSM_CLIENT_DATA;


struct _SM_CLIENT_DIRECTORY
{
	RTL_CRITICAL_SECTION  Lock;
	ULONG                 Count;
	PSM_CLIENT_DATA       Client;

} SmpClientDirectory;

/**********************************************************************
 *	SmpInitializeClientManagement/0
 */
VOID STDCALL
SmpInitializeClientManagement (VOID)
{
	RtlInitializeCriticalSection(& SmpClientDirectory.Lock);
	SmpClientDirectory.Count = 0;
	SmpClientDirectory.Client = NULL;
}

/**********************************************************************
 *	SmpCreateClient/1
 */
NTSTATUS STDCALL
SmpCreateClient(SM_PORT_MESSAGE Request)
{
	PSM_CLIENT_DATA pClient = NULL;

	/*
	 * Allocate the storage for client data
	 */
	pClient = RtlAllocateHeap (SmpHeap,
				   HEAP_ZERO_MEMORY,
				   sizeof (SM_CLIENT_DATA));
	if (NULL == pClient) return STATUS_NO_MEMORY;
	/*
	 * Initialize the client data
	 */
	// TODO
	/*
	 * Insert the new descriptor in the
	 * client directory.
	 */
	RtlEnterCriticalSection (& SmpClientDirectory.Lock);
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
	++ SmpClientDirectory.Count;
	RtlLeaveCriticalSection (& SmpClientDirectory.Lock);
	return STATUS_SUCCESS;
}

/**********************************************************************
 * 	SmpDestroyClient/1
 */
NTSTATUS STDCALL
SmpDestroyClient (ULONG SubsystemId)
{
	RtlEnterCriticalSection (& SmpClientDirectory.Lock);
	/* TODO */
	RtlLeaveCriticalSection (& SmpClientDirectory.Lock);
	return STATUS_SUCCESS;
}

/* EOF */
