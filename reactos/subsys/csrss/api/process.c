/* $Id: process.c,v 1.2 1999/07/17 23:10:30 ea Exp $
 *
 * reactos/subsys/csrss/api/process.c
 *
 * "\windows\ApiPort" port process management functions
 *
 * ReactOS Operating System
 */
#define PROTO_LPC
#include <ddk/ntddk.h>


DWORD
CSR_CreateProcess (
	PLPC_MESSAGE	pLpcMessage
	)
{
	return LPC_ERROR_CALL_NOT_IMPLEMENTED;
}


DWORD
CSR_TerminateProcess(
	PLPC_MESSAGE	pLpcMessage
	)
{
	return LPC_ERROR_CALL_NOT_IMPLEMENTED;
}

/* EOF */
