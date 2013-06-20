/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/inc/omap3.h
 * PURPOSE:         LLB Board-Specific Hardware Functions for OMAP3
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

VOID
NTAPI
LlbHwOmap3UartInitialize(
    VOID
);

VOID
NTAPI
LlbHwOmap3LcdInitialize(
    VOID
);

UCHAR
NTAPI
LlbHwOmap3TwlRead1(
    IN UCHAR ChipAddress,
    IN UCHAR RegisterAddress
);

VOID
NTAPI
LlbHwOmap3TwlWrite(
    IN UCHAR ChipAddress,
    IN UCHAR RegisterAddress,
    IN UCHAR Length,
    IN PUCHAR Values
);

VOID
NTAPI
LlbHwOmap3TwlWrite1(
    IN UCHAR ChipAddress,
    IN UCHAR RegisterAddress,
    IN UCHAR Value
);

VOID
NTAPI
LlbHwOmap3SynKpdInitialize(
    VOID
);

/* EOF */
