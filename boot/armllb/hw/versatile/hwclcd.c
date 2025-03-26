/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/hw/versatile/hwclcd.c
 * PURPOSE:         LLB CLCD Routines for Versatile
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include "precomp.h"

#define LCDTIMING0_PPL(x) 		((((x) / 16 - 1) & 0x3f) << 2)
#define LCDTIMING1_LPP(x) 		(((x) & 0x3ff) - 1)
#define LCDCONTROL_LCDPWR		(1 << 11)
#define LCDCONTROL_LCDEN		(1)
#define LCDCONTROL_LCDBPP(x)	(((x) & 7) << 1)
#define LCDCONTROL_LCDTFT		(1 << 5)

#define PL110_LCDTIMING0	(PVOID)0x10120000
#define PL110_LCDTIMING1	(PVOID)0x10120004
#define PL110_LCDTIMING2	(PVOID)0x10120008
#define PL110_LCDUPBASE		(PVOID)0x10120010
#define PL110_LCDLPBASE		(PVOID)0x10120014
#define PL110_LCDCONTROL	(PVOID)0x10120018

PUSHORT LlbHwVideoBuffer;

VOID
NTAPI
LlbHwVersaClcdInitialize(VOID)
{
    /* Set framebuffer address */
    WRITE_REGISTER_ULONG(PL110_LCDUPBASE, (ULONG)LlbHwGetFrameBuffer());
    WRITE_REGISTER_ULONG(PL110_LCDLPBASE, (ULONG)LlbHwGetFrameBuffer());

    /* Initialize timings to 720x400 */
	WRITE_REGISTER_ULONG(PL110_LCDTIMING0, LCDTIMING0_PPL(LlbHwGetScreenWidth()));
	WRITE_REGISTER_ULONG(PL110_LCDTIMING1, LCDTIMING1_LPP(LlbHwGetScreenHeight()));

    /* Enable the TFT/LCD Display */
	WRITE_REGISTER_ULONG(PL110_LCDCONTROL,
                         LCDCONTROL_LCDEN |
                         LCDCONTROL_LCDTFT |
                         LCDCONTROL_LCDPWR |
                         LCDCONTROL_LCDBPP(4));
}

ULONG
NTAPI
LlbHwGetScreenWidth(VOID)
{
    return 720;
}

ULONG
NTAPI
LlbHwGetScreenHeight(VOID)
{
    return 400;
}

PVOID
NTAPI
LlbHwGetFrameBuffer(VOID)
{
    return (PVOID)0x000A0000;
}

ULONG
NTAPI
LlbHwVideoCreateColor(IN ULONG Red,
                      IN ULONG Green,
                      IN ULONG Blue)
{
    return (((Blue >> 3) << 11)| ((Green >> 2) << 5)| ((Red >> 3) << 0));
}

/* EOF */
