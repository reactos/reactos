/* $Id: print.c,v 1.1 1999/10/15 15:20:31 ekohl Exp $
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


/* FUNCTIONS ****************************************************************/

ULONG DbgPrint(PCH Format, ...)
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

   HalDisplayString (buffer);

   /*
    * Restore the interrupt flag
    */
   __asm__("push %0\n\tpopf\n\t"
	   :
	   : "m" (eflags));
   return(strlen(buffer));
}

/* EOF */
