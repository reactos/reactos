/* $Id: stubs.c,v 1.5 2003/07/10 19:54:13 chorns Exp $
 *
 */
#include <ntos.h>


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
CTEBlock (
	DWORD	Unknown0
	)
{
	return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
VOID
STDCALL
CTEInitEvent (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
}


/*
 * @unimplemented
 */
VOID
STDCALL
CTEInitTimer (
	DWORD	Unknown0
	)
{
}


/*
 * @unimplemented
 */
BOOLEAN
STDCALL
CTEInitialize (
	VOID
	)
{
	/* FIXME: what should it initialize? */
	return TRUE;
}
	

/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
BOOLEAN
STDCALL
CTEScheduleEvent (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	return FALSE;
}


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
ULONG
STDCALL
CTESystemUpTime (
	VOID
	)
{
	return 0;
}

/* EOF */
