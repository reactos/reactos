/* $Id: reboot.c,v 1.6 2004/03/18 19:58:35 dwelch Exp $
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
#include <hal.h>


static VOID
HalReboot (VOID)
{
    char data;
    extern PVOID HalpZeroPageMapping;

    /* enable warm reboot */
    ((PUCHAR)HalpZeroPageMapping)[0x472] = 0x34;
    ((PUCHAR)HalpZeroPageMapping)[0x473] = 0x12;

    /* disable interrupts */
#if defined(__GNUC__)
    __asm__("cli\n");
#elif defined(_MSC_VER)
    __asm	cli
#else
#error Unknown compiler for inline assembler
#endif


    /* disable periodic interrupt (RTC) */
    WRITE_PORT_UCHAR((PUCHAR)0x70, 0x0b);
    data = READ_PORT_UCHAR((PUCHAR)0x71);
    WRITE_PORT_UCHAR((PUCHAR)0x71, (UCHAR)(data & 0xbf));

    /* */
    WRITE_PORT_UCHAR((PUCHAR)0x70, 0x0a);
    data = READ_PORT_UCHAR((PUCHAR)0x71);
    WRITE_PORT_UCHAR((PUCHAR)0x71, (UCHAR)((data & 0xf0) | 0x06));

    /* */
    WRITE_PORT_UCHAR((PUCHAR)0x70, 0x15);

    /* generate RESET signal via keyboard controller */
    WRITE_PORT_UCHAR((PUCHAR)0x64, 0xfe);

    /* stop the processor */
#if 1
#if defined(__GNUC__)
    __asm__("hlt\n");
#elif defined(_MSC_VER)
    __asm	hlt
#else
#error Unknown compiler for inline assembler
#endif
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
        HalReleaseDisplayOwnership();
        HalReboot ();
    }
}

/* EOF */
