/* $Id: ipi.c,v 1.1 2004/08/12 06:04:21 ion Exp $
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

#include <ddk/ntddk.h>
#include <internal/ps.h>

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
