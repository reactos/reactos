/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/hw/omap3-zoom2/hwlcd.c
 * PURPOSE:         LLB LCD Routines for OMAP3 ZOOM2
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include "precomp.h"

PUSHORT LlbHwVideoBuffer;

VOID
NTAPI
LlbHwOmap3LcdInitialize(VOID)
{
    /*
     * N.B. The following initialization sequence took about 12 months to figure
     *      out.
     *      This means if you are glancing at it and have no idea what on Earth
     *      could possibly be going on, this is *normal*.
     *      Just trust that this turns on the LCD.
     *      And be thankful all you ever have to worry about is Java and HTML.
     */

    /* Turn on the functional and interface clocks in the entire PER domain */
    WRITE_REGISTER_ULONG(0x48005000, 0x3ffff);  /* Functional clocks */
    WRITE_REGISTER_ULONG(0x48005010, 0x3ffff);  /* Interface clocks */

    /* Now that GPIO Module 3 is on, send a reset to the LCD panel on GPIO 96 */
    WRITE_REGISTER_ULONG(0x49054034, 0);             /* FIXME: Enable all as output */
    WRITE_REGISTER_ULONG(0x49054094, 0xffffffff);    /* FIXME: Output on all gpios */

    /* Now turn on the functional and interface clocks in the CORE domain */
    WRITE_REGISTER_ULONG(0x48004a00, 0x03fffe29); /* Functional clocks */
    WRITE_REGISTER_ULONG(0x48004a10, 0x3ffffffb); /* Interface clocks */

    /* The HS I2C interface is now on, configure it */
    WRITE_REGISTER_USHORT(0x48070024, 0x0);    /* Disable I2c */
    WRITE_REGISTER_USHORT(0x48070030, 0x17);   /* Configure clock divider */
    WRITE_REGISTER_USHORT(0x48070034, 0xd);    /* Configure clock scaler */
    WRITE_REGISTER_USHORT(0x48070038, 0xf);    /* Configure clock scaler */
    WRITE_REGISTER_USHORT(0x48070020, 0x215);  /* Configure clocks and idle */
    WRITE_REGISTER_USHORT(0x4807000c, 0x636f); /* Select wakeup bits */
    WRITE_REGISTER_USHORT(0x48070014, 0x4343); /* Disable DMA */
    WRITE_REGISTER_USHORT(0x48070024, 0x8000); /* Enable I2C */

    /*
     * Set the VPLL2 to cover all device groups instead of just P3.
     * This essentially enables the VRRTC to power up the LCD panel.
     */
    LlbHwOmap3TwlWrite1(0x4B, 0x8E, 0xE0);

    /* VPLL2 runs at 1.2V by default, so we need to reprogram to 1.8V for DVI */
    LlbHwOmap3TwlWrite1(0x4B, 0x91, 0x05);

    /* Set GPIO pin 7 on the TWL4030 as an output pin */
    LlbHwOmap3TwlWrite1(0x49, 0x9B, 0x80);

    /* Set GPIO pin 7 signal on the TWL4030 ON. This powers the LCD backlight */
    LlbHwOmap3TwlWrite1(0x49, 0xA4, 0x80);

    /* Now go on the McSPI interface and program it on for the channel */
    WRITE_REGISTER_ULONG(0x48098010, 0x15);
    WRITE_REGISTER_ULONG(0x48098020, 0x1);
    WRITE_REGISTER_ULONG(0x48098028, 0x1);
    WRITE_REGISTER_ULONG(0x4809802c, 0x112fdc);

    /* Send the reset signal (R2 = 00h) to the NEC WVGA LCD Panel */
    WRITE_REGISTER_ULONG(0x48098034, 0x1);
    WRITE_REGISTER_ULONG(0x48098038, 0x20100);
    WRITE_REGISTER_ULONG(0x48098034, 0x0);

    /* Turn on the functional and interface clocks in the DSS domain */
    WRITE_REGISTER_ULONG(0x48004e00, 0x5);
    WRITE_REGISTER_ULONG(0x48004e10, 0x1);

    /* Reset the Display Controller (DISPC) */
    WRITE_REGISTER_ULONG(0x48050410, 0x00000005); // DISPC_SYSCONFIG

    /* Set the frame buffer address */
	WRITE_REGISTER_ULONG(0x48050480, 0x800A0000); // DISPC_GFX_BA0

	/* Set resolution and RGB16 color mode */
	WRITE_REGISTER_ULONG(0x4805048c, 0x01df031f); // DISPC_GFX_SIZE
	WRITE_REGISTER_ULONG(0x480504a0, 0x0000000d); // DISPC_GFX_ATTRIBUTES

    /* Set LCD timings (VSync and HSync), pixel clock, and LCD size */
	WRITE_REGISTER_ULONG(0x4805046c, 0x00003000); // DISPC_POL_FREQ
	WRITE_REGISTER_ULONG(0x48050470, 0x00010004); // DISPC_DIVISOR
	WRITE_REGISTER_ULONG(0x48050464, 0x00300500); // DISPC_TIMING_H
	WRITE_REGISTER_ULONG(0x48050468, 0x00400300); // DISPC_TIMING_V
	WRITE_REGISTER_ULONG(0x4805047c, 0x01df031f); // DISPC_SIZE_LCD

    /* Turn the LCD on */
	WRITE_REGISTER_ULONG(0x48050440, 0x00018309); // DISPC_CONTROL
}

ULONG
NTAPI
LlbHwGetScreenWidth(VOID)
{
    return 800;
}

ULONG
NTAPI
LlbHwGetScreenHeight(VOID)
{
    return 480;
}

PVOID
NTAPI
LlbHwGetFrameBuffer(VOID)
{
    return (PVOID)0x800A0000;
}

ULONG
NTAPI
LlbHwVideoCreateColor(IN ULONG Red,
                      IN ULONG Green,
                      IN ULONG Blue)
{
    return (((Red >> 3) << 11)| ((Green >> 2) << 5)| ((Blue >> 3) << 0));
}

/* EOF */
