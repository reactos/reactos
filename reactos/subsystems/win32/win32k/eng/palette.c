/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           GDI Palette Functions
 * FILE:              subsys/win32k/eng/palette.c
 * PROGRAMER:         Jason Filby
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

//
//
//
VOID
FASTCALL
ColorCorrection(PPALETTE PalGDI, PPALETTEENTRY PaletteEntry, ULONG Colors)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)PalGDI->hPDev;

    if (!ppdev) return;

    if (ppdev->flFlags & PDEV_GAMMARAMP_TABLE)
    {
        INT i;
        PGAMMARAMP GammaRamp = (PGAMMARAMP)ppdev->pvGammaRamp;
        for ( i = 0; i < Colors; i++)
        {
            PaletteEntry[i].peRed   += GammaRamp->Red[i];
            PaletteEntry[i].peGreen += GammaRamp->Green[i];
            PaletteEntry[i].peBlue  += GammaRamp->Blue[i];
        }
    }
    return;
}

/*
 * @implemented
 */
HPALETTE
APIENTRY
EngCreatePalette(
    ULONG Mode,
    ULONG NumColors,
    ULONG *Colors,
    ULONG Red,
    ULONG Green,
    ULONG Blue)
{
    HPALETTE Palette;

    Palette = PALETTE_AllocPalette(Mode, NumColors, Colors, Red, Green, Blue);
    if (Palette != NULL)
    {
        GDIOBJ_SetOwnership(Palette, NULL);
    }

    return Palette;
}

/*
 * @implemented
 */
BOOL
APIENTRY
EngDeletePalette(IN HPALETTE Palette)
{
    GDIOBJ_SetOwnership(Palette, PsGetCurrentProcess());

    return PALETTE_FreePaletteByHandle(Palette);
}

/*
 * @implemented
 */
ULONG
APIENTRY
PALOBJ_cGetColors(PALOBJ *PalObj, ULONG Start, ULONG Colors, ULONG *PaletteEntry)
{
    PALETTE *PalGDI;

    PalGDI = (PALETTE*)PalObj;
   /* PalGDI = (PALETTE*)AccessInternalObjectFromUserObject(PalObj); */

    if (Start >= PalGDI->NumColors)
        return 0;

    Colors = min(Colors, PalGDI->NumColors - Start);

    /* NOTE: PaletteEntry ULONGs are in the same order as PALETTEENTRY. */
    RtlCopyMemory(PaletteEntry, PalGDI->IndexedColors + Start, sizeof(ULONG) * Colors);

    if (PalGDI->Mode & PAL_GAMMACORRECTION)
        ColorCorrection(PalGDI, (PPALETTEENTRY)PaletteEntry, Colors);

    return Colors;
}

/* EOF */
