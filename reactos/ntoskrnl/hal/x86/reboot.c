/* $Id: reboot.c,v 1.4 2000/03/19 13:34:47 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/reboot.c
 * PURPOSE:         Reboot functions.
 * PROGRAMMER:      Eric Kohl (ekohl@abo.rhein-zeitung.de)
 * UPDATE HISTORY:
 *                  Created 11/10/99
 */


#include <ddk/ntddk.h>
#include <internal/hal.h>
#include <internal/i386/io.h>


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
    outb_p (0x70, 0x0b);
    data = inb_p (0x71);
    outb_p (0x71, data & 0xbf);

    /* */
    outb_p (0x70, 0x0a);
    data = inb_p (0x71);
    outb_p (0x71, (data & 0xf0) | 0x06);

    /* */
    outb_p (0x70, 0x15);

    /* generate RESET signal via keyboard controller */
    outb_p (0x64, 0xfe);

    /* stop the processor */
    __asm__("hlt\n");
}


VOID
STDCALL
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
