/* $Id: dbg.h,v 1.3 2003/03/31 22:30:48 hyperion Exp $
 *
 */

#ifndef __INCLUDE_NTDLL_DBG_H
#define __INCLUDE_NTDLL_DBG_H

NTSTATUS
STDCALL
DbgSsInitialize (
	HANDLE	ReplyPort,
	ULONG	Unknown1,
	ULONG	Unknown2,
	ULONG	Unknown3
	);

NTSTATUS
STDCALL
DbgUiConnectToDbg (
	VOID
	);

NTSTATUS
STDCALL
DbgUiContinue (
	PCLIENT_ID	ClientId,
	ULONG		ContinueStatus
	);

NTSTATUS
STDCALL
DbgUiWaitStateChange (
	ULONG	Unknown1,
	ULONG	Unknown2
	);

NTSTATUS STDCALL DbgUiRemoteBreakin(VOID);

NTSTATUS STDCALL DbgUiIssueRemoteBreakin(HANDLE Process);

#endif /* __INCLUDE_NTDLL_DBG_H */

/* EOF */
