/* $Id: kdebug.c,v 1.1 1999/08/20 16:28:10 ea Exp $
 *
 * reactos/ntoskrnl/kd/kdebug.c
 *
 */
#include <ntos.h>

BYTE STDCALL KdPortPollByte(VOID);

/* DATA */

BOOLEAN
KdDebuggerEnabled = FALSE;

BOOLEAN
KdDebuggerNotPresent = TRUE;

BOOLEAN
KdComPortInUse = FALSE;


/* NTOSKRNL.KdPollBreakIn */
BYTE
STDCALL
KdPollBreakIn (
	VOID
	)
{
	return KdPortPollByte();
}



/* EOF */
