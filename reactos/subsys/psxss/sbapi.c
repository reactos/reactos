/* $Id: sbapi.c,v 1.1 1999/07/17 23:10:31 ea Exp $
 *
 * sbapi.c - Displatcher for the \SbApiPort
 *
 * ReactOS Operating System
 *
 *
 */
#include <ddk/ntddk.h>
#include <internal/lpc.h>

/* The \SbApi dispatcher: what is this port for? */

LPC_RETURN_CODE
port_dispatcher_sbapi(
	PLPC_REQUEST_REPLY	pLpcRequestReply
	)
{
	return LPC_ERROR_INVALID_FUNCTION;
}


/* EOF */
