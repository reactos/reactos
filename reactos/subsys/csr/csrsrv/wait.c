/* $Id$
 *
 * subsys/csr/csrsrv/wait.c - CSR server - wait management
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

NTSTATUS STDCALL CsrCreateWait (PCSR_THREAD pCsrThread, PCSR_WAIT * ppCsrWait)
{
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
	
	DPRINT("CSRSRV: %s called\n", __FUNCTION__);
	
	return Status;
}

NTSTATUS STDCALL CsrDereferenceWait (PCSR_WAIT pCsrWait)
{
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
	
	DPRINT("CSRSRV: %s called\n", __FUNCTION__);
	
	return Status;
}

NTSTATUS STDCALL CsrMoveSatisfiedWait (PCSR_WAIT pCsrWait)
{
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
	
	DPRINT("CSRSRV: %s called\n", __FUNCTION__);
	
	return Status;
}

NTSTATUS STDCALL CsrNotifyWait (PCSR_WAIT pCsrWait)
{
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
	
	DPRINT("CSRSRV: %s called\n", __FUNCTION__);
	
	return Status;
}


/* EOF */
