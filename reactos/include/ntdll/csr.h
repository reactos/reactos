/* $Id: csr.h,v 1.3 2000/04/25 23:22:52 ea Exp $
 *
 */

#ifndef __INCLUDE_NTDLL_CSR_H
#define __INCLUDE_NTDLL_CSR_H

#include <csrss/csrss.h>

NTSTATUS STDCALL CsrClientConnectToServer(VOID);
NTSTATUS STDCALL CsrClientCallServer(PCSRSS_API_REQUEST Request,
				     PCSRSS_API_REPLY Reply,
				     ULONG Length,
				     ULONG ReplyLength);
NTSTATUS
STDCALL
CsrSetPriorityClass (
	HANDLE	hProcess,
	DWORD	* PriorityClass
	);

#endif /* __INCLUDE_NTDLL_CSR_H */

/* EOF */
