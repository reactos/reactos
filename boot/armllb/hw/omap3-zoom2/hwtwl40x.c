/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/hw/omap3-zoom2/hwtwl40x.c
 * PURPOSE:         LLB Synpatics Keypad Support for OMAP3 ZOOM 2
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include "precomp.h"

/* FUNCTIONS ******************************************************************/

UCHAR
NTAPI
LlbHwOmap3TwlRead1(IN UCHAR ChipAddress,
                   IN UCHAR RegisterAddress)
{
    volatile int i = 1000;

    /* Select the register */
    LlbHwOmap3TwlWrite(ChipAddress, RegisterAddress, 0, NULL);

    /* Now read it */
    WRITE_REGISTER_USHORT(0x48070024, 0x8401);
    for (i = 1000; i > 0; i--);
    return READ_REGISTER_USHORT(0x4807001c);
}

VOID
NTAPI
LlbHwOmap3TwlWrite(IN UCHAR ChipAddress,
                   IN UCHAR RegisterAddress,
                   IN UCHAR Length,
                   IN PUCHAR Values)
{
    volatile int i = 1000;
    ULONG j;

    /* Select chip address */
    WRITE_REGISTER_USHORT(0x4807002c, ChipAddress);
    WRITE_REGISTER_USHORT(0x48070018, Length + 1);

    /* Enable master transmit mode */
    WRITE_REGISTER_USHORT(0x48070024, 0x8601);
    WRITE_REGISTER_USHORT(0x4807001c, RegisterAddress);

    /* Loop each byte */
    for (j = 0; j < Length; j++)
    {
        /* Write the data */
        WRITE_REGISTER_USHORT(0x4807001c, Values[j]);
    }

    /* Issue stop command */
    WRITE_REGISTER_USHORT(0x48070024, 0x8602);
    for (i = 1000; i > 0; i--);
}

VOID
NTAPI
LlbHwOmap3TwlWrite1(IN UCHAR ChipAddress,
                    IN UCHAR RegisterAddress,
                    IN UCHAR Value)
{
    /* Do the actual write */
    LlbHwOmap3TwlWrite(ChipAddress, RegisterAddress, 1, &Value);
}

/* EOF */
