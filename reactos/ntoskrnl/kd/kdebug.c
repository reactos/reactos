/* $Id: kdebug.c,v 1.7 2000/03/03 00:46:37 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/kd/kdebug.c
 * PURPOSE:         Kernel debugger
 * PROGRAMMER:      Eric Kohl (ekohl@abo.rhein-zeitung.de)
 * UPDATE HISTORY:
 *                  21/10/99: Created
 */

#include <ddk/ntddk.h>
#include <internal/ntoskrnl.h>
#include <internal/kd.h>


/*
 * Uncomment one of the following symbols to select a debug output style.
 *
 * SCREEN_DEBUGGING:
 *    Debug information is printed on the screen.
 *
 * SERIAL_DEBUGGING:
 *    Debug information is printed to a serial device. Check the port
 *    address, the baud rate and the data format.
 *    Default: COM1 19200 Baud 8N1 (8 data bits, no parity, 1 stop bit)
 *
 * BOCHS_DEBUGGING: (not tested yet)
 *    Debug information is printed to the bochs logging port. Bochs
 *    writes the output to a log file.
 */

#define SCREEN_DEBUGGING	/* debug info is printed on the screen */
//#define SERIAL_DEBUGGING	/* remote debugging */
//#define BOCHS_DEBUGGING	/* debug output using bochs */


#define SERIAL_DEBUG_PORT 1	/* COM 1 */
// #define SERIAL_DEBUG_PORT 2	/* COM 2 */
#define SERIAL_DEBUG_BAUD_RATE 19200


//#define BOCHS_DEBUGGING
#ifdef BOCHS_DEBUGGING
#define BOCHS_LOGGER_PORT (0xe9)
#endif


/* VARIABLES ***************************************************************/

BOOLEAN KdDebuggerEnabled = FALSE;		/* EXPORTED */
BOOLEAN KdDebuggerNotPresent = TRUE;		/* EXPORTED */


/* PRIVATE FUNCTIONS ********************************************************/

VOID
KdInitSystem (VOID)
{

	/* FIXME: parse kernel command line */

	/* initialize debug port */
#ifdef SERIAL_DEBUGGING
	KD_PORT_INFORMATION PortInfo;

	PortInfo.ComPort  = SERIAL_DEBUG_PORT;
	PortInfo.BaudRate = SERIAL_DEBUG_BAUD_RATE;

	KdPortInitialize (&PortInfo,
	                  0,
	                  0);
#endif
}


ULONG
KdpPrintString (PANSI_STRING String)
{
#if defined(SERIAL_DEBUGGING) || defined(BOCHS_DEBUGGING)
   PCH pch = String->Buffer;
#endif

#ifdef SCREEN_DEBUGGING
   HalDisplayString (String->Buffer);
#endif

#ifdef SERIAL_DEBUGGING
   while (*pch != 0)
   {
       if (*pch == '\n')
       {
           KdPortPutByte ('\r');
       }

       KdPortPutByte (*pch);

       pch++;
   }
#endif

#ifdef BOCHS_DEBUGGING
   while (*pch != 0)
   {
       if (*pch == '\n')
       {
           WRITE_PORT_UCHAR((PUCHAR)BOCHS_LOGGER_PORT, '\r');
       }

       WRITE_PORT_UCHAR((PUCHAR)BOCHS_LOGGER_PORT, *pch);

       pch++;
   }
#endif

    return (ULONG)String->Length;
}

/* PUBLIC FUNCTIONS *********************************************************/

/* NTOSKRNL.KdPollBreakIn */

BYTE
STDCALL
KdPollBreakIn (
	VOID
	)
{
	return KdPortPollByte();
}

VOID
STDCALL
KeEnterKernelDebugger (
	VOID
	)
{
	HalDisplayString ("\n\n *** Entered kernel debugger ***\n");

	for (;;)
		__asm__("hlt\n\t");
}

/* EOF */
