/* $Id: process.c,v 1.1 1999/06/08 22:50:59 ea Exp $
 *
 * reactos/subsys/csrss/api/process.c
 *
 * "\windows\ApiPort" port process management functions
 *
 * ReactOS Operating System
 */
#include <internal/lpc.h>

LPC_RETURN_CODE
CSR_CreateProcess (
	PLPC_REQUEST_REPLY	pLpcRequestReply
	)
{
	return LPC_ERROR_CALL_NOT_IMPLEMENTED;
}


LPC_RETURN_CODE
CSR_TerminateProcess(
	PLPC_REQUEST_REPLY	pLpcRequestReply
	)
{
	return LPC_ERROR_CALL_NOT_IMPLEMENTED;
}

/* EOF */
