/* $Id: print.c,v 1.4 1999/12/06 05:48:34 phreak Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/dbg/print.c
 * PURPOSE:         Debug output 
 * PROGRAMMER:      Eric Kohl (ekohl@abo.rhein-zeitung.de)
 * UPDATE HISTORY:
 *                  14/10/99: Created
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/hal/ddk.h>
#include <internal/ntoskrnl.h>

#include <string.h>

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

#define SCREEN_DEBUGGING        /* debug info is printed on the screen */
/* #define SERIAL_DEBUGGING */  /* remote debugging */
/* #define BOCHS_DEBUGGING */   /* debug output using bochs */


#define SERIAL_DEBUG_PORT 0x03f8        /* COM 1 */
/* #define SERIAL_DEBUG_PORT 0x02f8 */  /* COM 2 */
#define SERIAL_DEBUG_BAUD_RATE 19200


#ifdef BOCHS_DEBUGGING
#define BOCHS_LOGGER_PORT (0xe9)
#endif


/* FUNCTIONS ****************************************************************/

#ifdef SERIAL_DEBUGGING
static VOID
DbgDisplaySerialString(PCH String)
{
	PCH pch = String;

	while (*pch != 0)
	{
		if (*pch == '\n')
		{
			KdPortPutByte ('\r');
		}

		KdPortPutByte (*pch);

		pch++;
	}
}
#endif /* SERIAL_DEBUGGING */


#ifdef BOCHS_DEBUGGING
static VOID
DbgDisplayBochsString(PCH String)
{
	PCH pch = String;

	while (*pch != 0)
	{
		if (*pch == '\n')
		{
			WRITE_PORT_UCHAR((PUCHAR)BOCHS_LOGGER_PORT, '\r');
		}

		WRITE_PORT_UCHAR((PUCHAR)BOCHS_LOGGER_PORT, *pch);

		pch++;
	}
}
#endif /* BOCHS_DEBUGGING */


VOID
DbgInit (VOID)
{
#ifdef SERIAL_DEBUGGING
	KD_PORT_INFORMATION PortInfo;

	PortInfo.BaseAddress = SERIAL_DEBUG_PORT;
	PortInfo.BaudRate = SERIAL_DEBUG_BAUD_RATE;

	KdPortInitialize (&PortInfo,
	                  0,
	                  0);
#endif
}


ULONG
DbgPrint(PCH Format, ...)
{
   char buffer[256];
   va_list ap;
   unsigned int eflags;

   /*
    * Because this is used by alomost every subsystem including irqs it
    * must be atomic. The following code sequence disables interrupts after
    * saving the previous state of the interrupt flag
    */
   __asm__("pushf\n\tpop %0\n\tcli\n\t"
	   : "=m" (eflags)
	   : /* */);

   /*
    * Process the format string into a fixed length buffer using the
    * standard C RTL function
    */
   va_start(ap,Format);
   vsprintf(buffer,Format,ap);
   va_end(ap);

#ifdef SCREEN_DEBUGGING
   HalDisplayString (buffer);
#endif
#ifdef SERIAL_DEBUGGING
   DbgDisplaySerialString (buffer);
#endif
#ifdef BOCHS_DEBUGGING
   DbgDisplayBochsString (buffer);
#endif

   /*
    * Restore the interrupt flag
    */
   __asm__("push %0\n\tpopf\n\t"
	   :
	   : "m" (eflags));
   return(strlen(buffer));
}

/* EOF */
