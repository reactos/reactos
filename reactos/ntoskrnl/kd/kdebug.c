/* $Id: kdebug.c,v 1.2 1999/10/21 11:13:38 ekohl Exp $
 *
 * reactos/ntoskrnl/kd/kdebug.c
 *
 */
#include <ddk/ntddk.h>


//BYTE STDCALL KdPortPollByte(VOID);

/* DATA */

BOOLEAN
KdDebuggerEnabled = FALSE;

BOOLEAN
KdDebuggerNotPresent = TRUE;

/* This is exported by hal.dll (Eric Kohl) */
//ULONG
//KdComPortInUse = 0;


/* PRIVATE FUNCTIONS ********************************************************/

VOID
KdInit (VOID)
{

	/* FIXME: parse kernel command line */

	/* initialize debug port */
#ifdef SERIAL_DEBUGGING
	KD_PORT_INFORMATION PortInfo;

	PortInfo.BaseAddress = SERIAL_DEBUG_PORT;
	PortInfo.BaudRate = SERIAL_DEBUG_BAUD_RATE;

	KdPortInitialize (&PortInfo,
	                  0,
	                  0);
#endif
}


/* PUBLIC FUNCTIONS *********************************************************/

/* NTOSKRNL.KdPollBreakIn */
/*
BYTE
STDCALL
KdPollBreakIn (
	VOID
	)
{
	return KdPortPollByte();
}
*/


/* EOF */
