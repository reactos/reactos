/* $Id$
 *
 * FILE:            ntoskrnl/ke/profile.c
 * PURPOSE:         Kernel Profiling
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 23/06/04
 */

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/*
 * @unimplemented
 */
STDCALL
VOID
KeProfileInterrupt(
    PKTRAP_FRAME TrapFrame
)
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
STDCALL
VOID
KeProfileInterruptWithSource(
	IN PKTRAP_FRAME   		TrapFrame,
	IN KPROFILE_SOURCE		Source
)
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
STDCALL
VOID
KeSetProfileIrql(
    IN KIRQL ProfileIrql
)
{
	UNIMPLEMENTED;
}
