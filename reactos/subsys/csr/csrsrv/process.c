/* $Id$
 *
 * subsys/csr/csrsrv/process.c - CSR server - process management
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

/* LOCALS */

struct {
	RTL_CRITICAL_SECTION Lock;
} Process;



NTSTATUS STDCALL CsrSrvInitializeProcess (VOID)
{
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;

	DPRINT("CSRSRV: %s called\n", __FUNCTION__);
	
	Status = RtlInitializeCriticalSection (& Process.Lock);
	if(NT_SUCCESS(Status))
	{
		// more process management initialization
	}
	return Status;
}

/*=====================================================================
 * 	PUBLIC API
 *===================================================================*/

NTSTATUS STDCALL CsrCreateProcess (PCSR_SESSION pCsrSession, PCSR_PROCESS * ppCsrProcess)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PCSR_PROCESS pCsrProcess = NULL;
	
	DPRINT("CSRSRV: %s called\n", __FUNCTION__);

	pCsrProcess = RtlAllocateHeap (pCsrSession->Heap,
					HEAP_ZERO_MEMORY,
					sizeof (CSR_PROCESS));
	if (NULL == pCsrProcess)
	{
		Status = STATUS_NO_MEMORY;
	} else {
		pCsrProcess->CsrSession = pCsrSession;
		if (NULL != ppCsrProcess)
		{
			*ppCsrProcess = pCsrProcess;
		}
	}
	return Status;
}

NTSTATUS STDCALL CsrDereferenceProcess (PCSR_PROCESS pCsrProcess)
{
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
	
	DPRINT("CSRSRV: %s called\n", __FUNCTION__);
	return Status;
}

NTSTATUS STDCALL CsrDestroyProcess (PCSR_PROCESS pCsrProcess)
{
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
	
	DPRINT("CSRSRV: %s called\n", __FUNCTION__);
	return Status;
}

NTSTATUS STDCALL CsrGetProcessLuid (PCSR_PROCESS pCsrProcess, PLUID pLuid)
{
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
	
	DPRINT("CSRSRV: %s called\n", __FUNCTION__);
	return Status;
}

NTSTATUS STDCALL CsrLockProcessByClientId ()
{
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
	
	DPRINT("CSRSRV: %s called\n", __FUNCTION__);
	return Status;
}

NTSTATUS STDCALL CsrShutdownProcesses (PCSR_SESSION pCsrSession OPTIONAL)
{
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
	
	DPRINT("CSRSRV: %s called\n", __FUNCTION__);

	if (NULL == pCsrSession)
	{
		// TODO: shutdown every session
	} else {
		// TODO: shutdown every process in pCsrSession
	}
	return Status;
}

NTSTATUS STDCALL CsrUnlockProcess (PCSR_PROCESS pCsrProcess)
{
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
	
	DPRINT("CSRSRV: %s called\n", __FUNCTION__);
	return Status;
}

/* EOF */
