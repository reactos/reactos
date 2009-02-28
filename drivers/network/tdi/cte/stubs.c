/* $Id$
 *
 */
#include <ntddk.h>


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
CTEBlock (
	ULONG	Unknown0
	)
{
	return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
VOID
NTAPI
CTEInitEvent (
	ULONG	Unknown0,
	ULONG	Unknown1
	)
{
}


/*
 * @unimplemented
 */
VOID
NTAPI
CTEInitTimer (
	ULONG	Unknown0
	)
{
}


/*
 * @unimplemented
 */
BOOLEAN
NTAPI
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
NTAPI
CTELogEvent (
	ULONG	Unknown0,
	ULONG	Unknown1,
	ULONG	Unknown2,
	ULONG	Unknown3,
	ULONG	Unknown4,
	ULONG	Unknown5,
	ULONG	Unknown6
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
NTAPI
CTEScheduleEvent (
	ULONG	Unknown0,
	ULONG	Unknown1
	)
{
	return FALSE;
}


/*
 * @unimplemented
 */
LONG
NTAPI
CTESignal (
	ULONG	Unknown0,
	ULONG	Unknown1
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
NTAPI
CTEStartTimer (
	ULONG	Unknown0,
	ULONG	Unknown1,
	ULONG	Unknown2,
	ULONG	Unknown3
	)
{
	return FALSE;
}


/*
 * @unimplemented
 */
ULONG
NTAPI
CTESystemUpTime (
	VOID
	)
{
	return 0;
}

/* EOF */
