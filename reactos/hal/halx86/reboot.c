/* $Id: reboot.c,v 1.2 2002/09/07 15:12:10 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/reboot.c
 * PURPOSE:         Reboot functions.
 * PROGRAMMER:      Eric Kohl (ekohl@abo.rhein-zeitung.de)
 * UPDATE HISTORY:
 *                  Created 11/10/99
 */

#include <hal.h>

#define NDEBUG
#include <internal/debug.h>


static VOID
HalReboot (VOID)
{
    char data;
    BYTE *mem;

    /* enable warm reboot */
    mem = (BYTE *)(0xd0000000 + 0x0000);
//    mem = HalMapPhysicalMemory (0, 1);
    mem[0x472] = 0x34;
    mem[0x473] = 0x12;

    /* disable interrupts */
    __asm__("cli\n");

    /* disable periodic interrupt (RTC) */
    WRITE_PORT_UCHAR((PUCHAR)0x70, 0x0b);
    data = READ_PORT_UCHAR((PUCHAR)0x71);
    WRITE_PORT_UCHAR((PUCHAR)0x71, data & 0xbf);

    /* */
    WRITE_PORT_UCHAR((PUCHAR)0x70, 0x0a);
    data = READ_PORT_UCHAR((PUCHAR)0x71);
    WRITE_PORT_UCHAR((PUCHAR)0x71, (data & 0xf0) | 0x06);

    /* */
    WRITE_PORT_UCHAR((PUCHAR)0x70, 0x15);

    /* generate RESET signal via keyboard controller */
    WRITE_PORT_UCHAR((PUCHAR)0x64, 0xfe);

    /* stop the processor */
#if 1
    __asm__("hlt\n");
#else
   for(;;);
#endif   
}


VOID STDCALL
HalReturnToFirmware (
	ULONG	Action
	)
{
    if (Action == FIRMWARE_HALT)
    {
        DbgPrint ("HalReturnToFirmware called!\n");
        DbgBreakPoint ();
    }
    else if (Action == FIRMWARE_REBOOT)
    {
        HalResetDisplay ();
        HalReboot ();
    }
}

/* EOF */
