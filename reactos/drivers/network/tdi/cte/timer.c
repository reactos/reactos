/*
 * PROJECT:         ReactOS TDI driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/network/tdi/cte/timer.c
 * PURPOSE:         CTE timer support
 * PROGRAMMERS:     Oleg Baikalow (obaikalow@gmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntddk.h>

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
