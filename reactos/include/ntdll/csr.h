/* $Id: csr.h,v 1.1 2000/02/27 02:01:24 ekohl Exp $
 *
 */

#ifndef __INCLUDE_NTDLL_CSR_H
#define __INCLUDE_NTDLL_CSR_H


/*
NTSTATUS
STDCALL
CsrClientCallServer (
	ULONG Unknown1,
	ULONG Unknown2,
	ULONG Unknown3,
	ULONG Unknown4
	);
*/

NTSTATUS
STDCALL
CsrClientConnectToServer (
	ULONG Unknown1,
	ULONG Unknown2,
	ULONG Unknown3,
	ULONG Unknown4,
	ULONG Unknown5,
	ULONG Unknown6
	);

#endif /* __INCLUDE_NTDLL_CSR_H */

/* EOF */
