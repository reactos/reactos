/* $Id: lpcstub.c,v 1.1 1999/06/08 22:50:59 ea Exp $
 *
 * lpcstub.c
 *
 * ReactOS Operating System
 *
 *
 */
#include <ddk/ntddk.h>
#include <internal/lpc.h>
#include "api.h"


/* CUI & GUI Win32(tm) API port */

LPC_RETURN_CODE
PortDispatcher_Api(
	PLPC_REQUEST_REPLY	pLpcRequestReply
	)
{
	switch (pLpcRequestReply->Function)
	{
		case CSRSS_API_PROCESS_CREATE:
			return CSRSS_CreateProcess(pLpcRequestReply);
		case CSRSS_API_PROCESS_TERMINATE:
			return CSRSS_TerminateProcess(pLpcRequestReply);
	}
	return LPC_ERROR_INVALID_FUNCTION;
}


/* The \SbApi dispatcher: what is this port for? */

LPC_RETURN_CODE
PortDispatcher_SbApi(
	PLPC_REQUEST_REPLY	pLpcRequestReply
	)
{
	return LPC_ERROR_INVALID_FUNCTION;
}


/* EOF */
