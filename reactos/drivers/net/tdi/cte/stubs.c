/* $Id: stubs.c,v 1.3 2002/09/07 15:12:06 chorns Exp $
 *
 */
#include <ddk/ntddk.h>


NTSTATUS
STDCALL
CTEBlock (
	DWORD	Unknown0
	)
{
	return STATUS_NOT_IMPLEMENTED;
}


VOID
STDCALL
CTEInitEvent (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
}


VOID
STDCALL
CTEInitTimer (
	DWORD	Unknown0
	)
{
}


BOOLEAN
STDCALL
CTEInitialize (
	VOID
	)
{
	/* FIXME: what should it initialize? */
	return TRUE;
}
	

NTSTATUS
STDCALL
CTELogEvent (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4,
	DWORD	Unknown5,
	DWORD	Unknown6
	)
{
	/* Probably call
	 * IoAllocateErrorLogEntry and
	 * IoWriteErrorLogEntry
	 */
	return STATUS_NOT_IMPLEMENTED;
}


BOOLEAN
STDCALL
CTEScheduleEvent (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	return FALSE;
}


LONG
STDCALL
CTESignal (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
#if 0
	PKEVENT	kevent = (PKEVENT) Unknown0;
	
	return KeSetEvent (
		kevent,
		0,
		FALSE
		);
#endif
	return 0;
}


BOOLEAN
STDCALL
CTEStartTimer (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3
	)
{
	return FALSE;
}


ULONG
STDCALL
CTESystemUpTime (
	VOID
	)
{
	return 0;
}


/* EOF */
