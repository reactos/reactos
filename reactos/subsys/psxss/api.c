/* $Id: api.c,v 1.2 1999/09/07 17:12:39 ea Exp $
 *
 * reactos/subsys/psxss/api.c
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
 * along with this software; see the file COPYING. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.  
 *
 * --------------------------------------------------------------------
 */
#include <ddk/ntddk.h>
#include <psxss/api.h>
#include "api/api.h"

BOOL TerminationRequestPending = FALSE;

LPC_RETURN_CODE
PortRequestDispatcher_PsxApi (
	PLPC_REQUEST_REPLY	pLpcRequestReply
	)
{
	switch (pLpcRequestReply->Function)
	{
		/* PROCESS Management */
		case PSX_SS_API_PROCESS_CREATE:
			return POSIX_PROCESS_Create(pLpcRequestReply);
			
		case PSX_SS_API_PROCESS_TERMINATE:
			return POSIX_PROCESS_Terminate(pLpcRequestReply);
			
		/* THREAD Management */
		case PSX_SS_API_THREAD_CREATE:
			return POSIX_THREAD_Create(pLpcRequestReply);
			
		case PSX_SS_API_THREAD_TERMINATE:
			return POSIX_THREAD_Terminate(pLpcRequestReply);

		/* Subsystem Control */
		case PSX_SS_API_SUBSYSTEM_SHUTDOWN:
			return POSIX_SS_Shutdown(pLpcRequestReply);
	}
	return LPC_ERROR_INVALID_FUNCTION;
}


/* EOF */
