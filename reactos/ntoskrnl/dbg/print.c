/* $Id: print.c,v 1.2 1999/10/16 21:08:23 ekohl Exp $
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
#include <internal/halio.h>

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

#define SERIAL_LINE_CONTROL (SR_LCR_CS8 | SR_LCR_ST1 | SR_LCR_PNO)

#ifdef BOCHS_DEBUGGING
#define BOCHS_LOGGER_PORT (0x3ed)
#endif

#ifdef SERIAL_DEBUGGING
#define   SER_RBR   SERIAL_DEBUG_PORT + 0
#define   SER_THR   SERIAL_DEBUG_PORT + 0
#define   SER_DLL   SERIAL_DEBUG_PORT + 0
#define   SER_IER   SERIAL_DEBUG_PORT + 1
#define   SER_DLM   SERIAL_DEBUG_PORT + 1
#define   SER_IIR   SERIAL_DEBUG_PORT + 2
#define   SER_LCR   SERIAL_DEBUG_PORT + 3
#define     SR_LCR_CS5 0x00
#define     SR_LCR_CS6 0x01
#define     SR_LCR_CS7 0x02
#define     SR_LCR_CS8 0x03
#define     SR_LCR_ST1 0x00
#define     SR_LCR_ST2 0x04
#define     SR_LCR_PNO 0x00
#define     SR_LCR_POD 0x08
#define     SR_LCR_PEV 0x18
#define     SR_LCR_PMK 0x28
#define     SR_LCR_PSP 0x38
#define     SR_LCR_BRK 0x40
#define     SR_LCR_DLAB 0x80
#define   SER_MCR   SERIAL_DEBUG_PORT + 4
#define     SR_MCR_DTR 0x01
#define     SR_MCR_RTS 0x02
#define   SER_LSR   SERIAL_DEBUG_PORT + 5
#define     SR_LSR_TBE 0x20
#define   SER_MSR   SERIAL_DEBUG_PORT + 6
#endif



/* FUNCTIONS ****************************************************************/

#ifdef SERIAL_DEBUGGING
static VOID
DbgDisplaySerialString(PCH String)
{
    PCH pch = String;

    while (*pch != 0)
    {

        while ((inb_p(SER_LSR) & SR_LSR_TBE) == 0)
            ;

        outb_p(SER_THR, *pch);

        if (*pch == '\n')
        {
            while ((inb_p(SER_LSR) & SR_LSR_TBE) == 0)
                ;

            outb_p(SER_THR, '\r');
        }

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
        outb_p(BOCHS_LOGGER_PORT, *pch);

        if (*pch == '\n')
        {
            outb_p(BOCHS_LOGGER_PORT, '\r');
        }

        pch++;
    }
}
#endif /* BOCHS_DEBUGGING */



VOID
DbgInit (VOID)
{
#ifdef SERIAL_DEBUGGING
        /*  turn on DTR and RTS  */
        outb_p(SER_MCR, SR_MCR_DTR | SR_MCR_RTS);
        /*  set baud rate, line control  */
        outb_p(SER_LCR, SERIAL_LINE_CONTROL | SR_LCR_DLAB);
        outb_p(SER_DLL, (115200 / SERIAL_DEBUG_BAUD_RATE) & 0xff);
        outb_p(SER_DLM, ((115200 / SERIAL_DEBUG_BAUD_RATE) >> 8) & 0xff);
        outb_p(SER_LCR, SERIAL_LINE_CONTROL);
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
