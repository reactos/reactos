/* $Id: ipi.c,v 1.2 2004/08/15 16:39:05 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/ipi.c
 * PURPOSE:         IPI Routines (Inter-Processor Interrupts). NT5+
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 11/08/2004
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
STDCALL
BOOLEAN
KiIpiServiceRoutine(
	IN PKTRAP_FRAME   		TrapFrame,
	IN PKEXCEPTION_FRAME  	ExceptionFrame
)
{
	UNIMPLEMENTED;
	return FALSE;
}

/* EOF */
