/* $Id: csr.h,v 1.2 2000/03/22 18:35:47 dwelch Exp $
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

#endif /* __INCLUDE_NTDLL_CSR_H */

/* EOF */
