/*
 * PROJECT:     ReactOS Console Text-Mode Device Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Loading specific fonts into VGA.
 * COPYRIGHT:   Copyright 2008-2019 Aleksey Bragin (aleksey@reactos.org)
 *              Copyright 2008-2019 Colin Finck (mail@colinfinck.de)
 *              Copyright 2008-2019 Christoph von Wittich (christoph_vw@reactos.org)
 */

/* INCLUDES ***************************************************************/

#include "blue.h"

//
// FIXME: For the moment we support only a fixed 256-char 8-bit font.
//

VOID OpenBitPlane(VOID);
VOID CloseBitPlane(VOID);
VOID LoadFont(_In_ PUCHAR Bitplane, _In_ PUCHAR FontBitfield);

/* FUNCTIONS ****************************************************************/

VOID
ScrSetFont(
    _In_ PUCHAR FontBitfield)
{
    PHYSICAL_ADDRESS BaseAddress;
    PUCHAR Bitplane;

    /* open bit plane for font table access */
    OpenBitPlane();

    /* get pointer to video memory */
    BaseAddress.QuadPart = BITPLANE_BASE;
    Bitplane = (PUCHAR)MmMapIoSpace(BaseAddress, 0xFFFF, MmNonCached);

    LoadFont(Bitplane, FontBitfield);

    MmUnmapIoSpace(Bitplane, 0xFFFF);

    /* close bit plane */
    CloseBitPlane();
}

/* PRIVATE FUNCTIONS *********************************************************/

/* Font-load specific funcs */
VOID
OpenBitPlane(VOID)
{
    /* disable interrupts */
    _disable();

    /* sequence reg */
    WRITE_PORT_UCHAR(SEQ_COMMAND, SEQ_RESET); WRITE_PORT_UCHAR(SEQ_DATA, 0x01);
    WRITE_PORT_UCHAR(SEQ_COMMAND, SEQ_ENABLE_WRT_PLANE); WRITE_PORT_UCHAR(SEQ_DATA, 0x04);
    WRITE_PORT_UCHAR(SEQ_COMMAND, SEQ_MEM_MODE); WRITE_PORT_UCHAR(SEQ_DATA, 0x07);
    WRITE_PORT_UCHAR(SEQ_COMMAND, SEQ_RESET); WRITE_PORT_UCHAR(SEQ_DATA, 0x03);

    /* graphic reg */
    WRITE_PORT_UCHAR(GCT_COMMAND, GCT_READ_PLANE); WRITE_PORT_UCHAR(GCT_DATA, 0x02);
    WRITE_PORT_UCHAR(GCT_COMMAND, GCT_RW_MODES); WRITE_PORT_UCHAR(GCT_DATA, 0x00);
    WRITE_PORT_UCHAR(GCT_COMMAND, GCT_GRAPH_MODE); WRITE_PORT_UCHAR(GCT_DATA, 0x00);

    /* enable interrupts */
    _enable();
}

VOID
CloseBitPlane(VOID)
{
    /* disable interrupts */
    _disable();

    /* sequence reg */
    WRITE_PORT_UCHAR(SEQ_COMMAND, SEQ_RESET); WRITE_PORT_UCHAR(SEQ_DATA, 0x01);
    WRITE_PORT_UCHAR(SEQ_COMMAND, SEQ_ENABLE_WRT_PLANE); WRITE_PORT_UCHAR(SEQ_DATA, 0x03);
    WRITE_PORT_UCHAR(SEQ_COMMAND, SEQ_MEM_MODE); WRITE_PORT_UCHAR(SEQ_DATA, 0x03);
    WRITE_PORT_UCHAR(SEQ_COMMAND, SEQ_RESET); WRITE_PORT_UCHAR(SEQ_DATA, 0x03);

    /* graphic reg */
    WRITE_PORT_UCHAR(GCT_COMMAND, GCT_READ_PLANE); WRITE_PORT_UCHAR(GCT_DATA, 0x00);
    WRITE_PORT_UCHAR(GCT_COMMAND, GCT_RW_MODES); WRITE_PORT_UCHAR(GCT_DATA, 0x10);
    WRITE_PORT_UCHAR(GCT_COMMAND, GCT_GRAPH_MODE); WRITE_PORT_UCHAR(GCT_DATA, 0x0e);

    /* enable interrupts */
    _enable();
}

VOID
LoadFont(
    _In_ PUCHAR Bitplane,
    _In_ PUCHAR FontBitfield)
{
    UINT32 i, j;

    for (i = 0; i < 256; i++)
    {
        for (j = 0; j < 8; j++)
        {
            *Bitplane = FontBitfield[i * 8 + j];
            Bitplane++;
        }

        // padding
        for (j = 8; j < 32; j++)
        {
            *Bitplane = 0;
            Bitplane++;
        }
    }
}
