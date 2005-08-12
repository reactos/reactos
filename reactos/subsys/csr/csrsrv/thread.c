/* $Id$
 *
 * subsys/csr/csrsrv/thread.c - CSR server - thread management
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

/*=====================================================================
 *	PUBLIC API
 *===================================================================*/

NTSTATUS STDCALL CsrCreateRemoteThread ()
{
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
	
	DPRINT("CSRSRV: %s called\n", __FUNCTION__);
	
	return Status;
}

NTSTATUS STDCALL CsrCreateThread (PCSR_PROCESS pCsrProcess, PCSR_THREAD *ppCsrThread)
{
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
	PCSR_THREAD pCsrThread = NULL;
	PCSR_SESSION pCsrSession = NULL;
	
	DPRINT("CSRSRV: %s called\n", __FUNCTION__);

	if (NULL == pCsrProcess || NULL == ppCsrThread)
	{
		return STATUS_INVALID_PARAMETER;
	}
	pCsrSession = pCsrProcess->CsrSession;
	pCsrThread = RtlAllocateHeap (pCsrSession->Heap,
					HEAP_ZERO_MEMORY,
					sizeof (CSR_THREAD));
	if (NULL == pCsrThread)
	{
		DPRINT1("CSRSRV:%s: out of memory!\n", __FUNCTION__);
		return STATUS_NO_MEMORY;
	}
	pCsrThread->CsrSession = pCsrSession;
	pCsrThread->CsrProcess = pCsrProcess;	
	return Status;
}

NTSTATUS STDCALL CsrDereferenceThread (PCSR_THREAD pCsrThread)
{
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
	
	DPRINT("CSRSRV: %s called\n", __FUNCTION__);
	
	return Status;
}

NTSTATUS STDCALL CsrDestroyThread (PCSR_THREAD pCsrThread)
{
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
	
	DPRINT("CSRSRV: %s called\n", __FUNCTION__);
	
	return Status;
}

NTSTATUS STDCALL CsrLockThreadByClientId ()
{
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
	
	DPRINT("CSRSRV: %s called\n", __FUNCTION__);
	
	return Status;
}

NTSTATUS STDCALL CsrReferenceThread (PCSR_THREAD pCsrThread)
{
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
	
	DPRINT("CSRSRV: %s called\n", __FUNCTION__);
	
	return Status;
}

NTSTATUS STDCALL CsrUnlockThread (PCSR_THREAD pCsrThread)
{
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
	
	DPRINT("CSRSRV: %s called\n", __FUNCTION__);
	
	return Status;
}

/* EOF */
