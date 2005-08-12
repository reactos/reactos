/* $Id$
 *
 * subsys/csr/csrsrv/session.c - CSR server - session management
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
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.LIB. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.  
 *
 * --------------------------------------------------------------------
 */
#include "srv.h"

//#define NDEBUG
#include <debug.h>

//TODO: when CsrSrvSessionsFlag is FALSE, create just one session and
//TODO: fail for more sessions requests.

/* LOCALS */

struct {
	RTL_CRITICAL_SECTION Lock;
	HANDLE Heap;
	ULONG LastUnusedId;
} Session;



NTSTATUS STDCALL CsrSrvInitializeSession (VOID)
{
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;

	DPRINT("CSRSRV: %s called\n", __FUNCTION__);
	
	Status = RtlInitializeCriticalSection (& Session.Lock);
	if (NT_SUCCESS(Status))
	{
		Session.Heap = RtlCreateHeap (HEAP_GROWABLE,
						NULL,
						65536,
						65536,
						NULL,
						NULL);
		if (NULL == Session.Heap)
		{
			RtlDeleteCriticalSection (& Session.Lock);
			Status = STATUS_NO_MEMORY;
		}
		Session.LastUnusedId = 0;
	}
	return Status;
}

static NTSTATUS STDCALL CsrpCreateSessionDirectories (PCSR_SESSION pCsrSession)
{
	NTSTATUS           Status = STATUS_SUCCESS;
	CHAR               SessionIdBuffer [8];
	ANSI_STRING        SessionIdNameA;
	UNICODE_STRING     SessionIdNameW;
	UNICODE_STRING     SessionDirectoryName;
	OBJECT_ATTRIBUTES  DirectoryAttributes;
	HANDLE             DirectoryHAndle;

	DPRINT("CSRSRV: %s(%08lx) called\n", __FUNCTION__, pCsrSession);

	sprintf (SessionIdBuffer, "\\Sessions\\%ld", pCsrSession->SessionId);
	RtlInitAnsiString (& SessionIdNameA, SessionIdBuffer);
	RtlAnsiStringToUnicodeString (& SessionIdNameW, & SessionIdNameA, TRUE);
	RtlCopyUnicodeString (& SessionDirectoryName, & CsrSrvOption.NameSpace.Root);
	RtlAppendUnicodeStringToString (& SessionDirectoryName, & SessionIdNameW);

	DPRINT("CSRSRV: %s(%08lx): %S\n", __FUNCTION__, pCsrSession,
			SessionDirectoryName.Buffer);

	InitializeObjectAttributes (& DirectoryAttributes,
					& SessionDirectoryName,
					OBJ_OPENIF,
					NULL,
					NULL);
	Status = NtCreateDirectoryObject (& DirectoryHAndle,
					  (DIRECTORY_CREATE_OBJECT|DIRECTORY_CREATE_SUBDIRECTORY),
					  & DirectoryAttributes);
	if (NT_SUCCESS(Status))
	{
		DPRINT1("CSRSRV: session %ld root directory not created (Status=%08lx)\n",
				pCsrSession->SessionId, Status);
	}
	// TODO
	return Status;
}

static NTSTATUS STDCALL CsrpDestroySessionDirectories (PCSR_SESSION pCsrSession)
{
	DPRINT("CSRSRV: %s called\n", __FUNCTION__);
	
	return STATUS_NOT_IMPLEMENTED;
}

/*=====================================================================
 *	PUBLIC API
 *===================================================================*/

NTSTATUS STDCALL CsrDestroySession (PCSR_SESSION pCsrSession)
{
	NTSTATUS Status = STATUS_SUCCESS;

	DPRINT("CSRSRV: %s(%08lx) called\n", __FUNCTION__, pCsrSession);
	
	if (NULL == pCsrSession)
	{
		Status = STATUS_INVALID_PARAMETER;
	} else {
		Status = CsrShutdownProcesses (pCsrSession);
		Status = CsrpDestroySessionDirectories (pCsrSession);
		RtlDestroyHeap (pCsrSession->Heap);
		RtlFreeHeap (Session.Heap, 0, pCsrSession);
	}
	return Status;
}

NTSTATUS STDCALL CsrCreateSession (PCSR_SESSION * ppCsrSession)
{
	NTSTATUS      Status = STATUS_SUCCESS;
	PCSR_SESSION  pCsrSession = NULL;

	DPRINT("CSRSRV: %s called\n", __FUNCTION__);
	
	if (NULL == ppCsrSession)
	{
		Status = STATUS_INVALID_PARAMETER;
	} else {
		RtlEnterCriticalSection (& Session.Lock);
		pCsrSession = RtlAllocateHeap (Session.Heap,
						HEAP_ZERO_MEMORY,
						sizeof (CSR_SESSION));
		if (NULL == pCsrSession)
		{
			Status = STATUS_NO_MEMORY;
		} else {
			pCsrSession->SessionId = Session.LastUnusedId ++;
			Status = CsrpCreateSessionDirectories (pCsrSession);
			if(NT_SUCCESS(Status))
			{
				pCsrSession->Heap = RtlCreateHeap(HEAP_GROWABLE,
								  NULL,
								  65536,
								  65536,
								  NULL,
								  NULL);
				if (NULL == pCsrSession->Heap)
				{
					Status = STATUS_NO_MEMORY;
					CsrpDestroySessionDirectories (pCsrSession);
					-- Session.LastUnusedId;
				}
			}
		}
		RtlLeaveCriticalSection (& Session.Lock);	
	}
	return Status;
}

/* EOF */
