/* $Id: reboot.c,v 1.1 2004/12/03 20:10:43 gvg Exp $
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
    Ki386DisableInterrupts();


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
    Ki386HaltProcessor();
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
