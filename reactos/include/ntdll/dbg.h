/* $Id: dbg.h,v 1.1 2000/04/14 01:41:38 ekohl Exp $
 *
 */

#ifndef __INCLUDE_NTDLL_DBG_H
#define __INCLUDE_NTDLL_DBG_H


NTSTATUS
STDCALL
DbgUiContinue (
	PCLIENT_ID	ClientId,
	ULONG		ContinueStatus
	);


#endif /* __INCLUDE_NTDLL_DBG_H */

/* EOF */
