/* $Id: profile.c,v 1.1 2004/06/23 22:31:51 ion Exp $
 *
 * FILE:            ntoskrnl/ke/profile.c
 * PURPOSE:         Kernel Profiling
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 23/06/04
 */

#include <ddk/ntddk.h>
#include <internal/io.h>
#include <internal/ps.h>
#include <internal/pool.h>

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
