/* $Id: api.c,v 1.1 1999/07/17 23:10:30 ea Exp $
 *
 * reactos/subsys/psxss/api.c
 *
 * ReactOS Operating System
 *
 *
 */
#include <ddk/ntddk.h>
#include <internal/lpc.h>
#include "api.h"

BOOL TerminationRequestPending = FALSE;

long
port_dispatcher_api(
	PLPC_REQUEST_REPLY	pLpcRequestReply
	)
{
	switch (pLpcRequestReply->Function)
	{
		case LPC_PSX_API_PROCESS_CREATE:
			return POSIX_PROCESS_Create(pLpcRequestReply);
		case LPC_PSX_API_PROCESS_TERMINATE:
			return POSIX_PROCESS_Terminate(pLpcRequestReply);
	}
	return LPC_ERROR_INVALID_FUNCTION;
}

/* EOF */
