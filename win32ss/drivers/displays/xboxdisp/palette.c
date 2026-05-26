

#include "xboxdisp.h"

static const PALETTEENTRY BASEPALETTE[20] =
{
    { 0x00, 0x00, 0x00, 0x00 }, { 0x80, 0x00, 0x00, 0x00 },
    { 0x00, 0x80, 0x00, 0x00 }, { 0x80, 0x80, 0x00, 0x00 },
    { 0x00, 0x00, 0x80, 0x00 }, { 0x80, 0x00, 0x80, 0x00 },
    { 0x00, 0x80, 0x80, 0x00 }, { 0xC0, 0xC0, 0xC0, 0x00 },
    { 0xC0, 0xDC, 0xC0, 0x00 }, { 0xD4, 0xD0, 0xC8, 0x00 },
    { 0xFF, 0xFB, 0xF0, 0x00 }, { 0x3A, 0x6E, 0xA5, 0x00 },
    { 0x80, 0x80, 0x80, 0x00 }, { 0xFF, 0x00, 0x00, 0x00 },
    { 0x00, 0xFF, 0x00, 0x00 }, { 0xFF, 0xFF, 0x00, 0x00 },
    { 0x00, 0x00, 0xFF, 0x00 }, { 0xFF, 0x00, 0xFF, 0x00 },
    { 0x00, 0xFF, 0xFF, 0x00 }, { 0xFF, 0xFF, 0xFF, 0x00 },
};

BOOL
IntInitDefaultPalette(PPDEV ppdev, PDEVINFO pDevInfo)
{
    ULONG i;
    PPALETTEENTRY p;

    if (ppdev->BitsPerPixel > 8)
    {
        ppdev->DefaultPalette = pDevInfo->hpalDefault =
            EngCreatePalette(PAL_BITFIELDS, 0, NULL,
                             ppdev->RedMask, ppdev->GreenMask, ppdev->BlueMask);
    }
    else
    {
        ppdev->PaletteEntries = EngAllocMem(0, sizeof(PALETTEENTRY) << 8, ALLOC_TAG);
        if (ppdev->PaletteEntries == NULL)
            return FALSE;

        for (i = 256, p = ppdev->PaletteEntries; i != 0; i--, p++)
        {
            p->peRed   = ((i >> 5) & 7) * 255 / 7;
            p->peGreen = ((i >> 3) & 3) * 255 / 3;
            p->peBlue  = (i & 7) * 255 / 7;
            p->peFlags = 0;
        }
        memcpy(ppdev->PaletteEntries,        BASEPALETTE,        10 * sizeof(PALETTEENTRY));
        memcpy(ppdev->PaletteEntries + 246, BASEPALETTE + 10, 10 * sizeof(PALETTEENTRY));

        ppdev->DefaultPalette = pDevInfo->hpalDefault =
            EngCreatePalette(PAL_INDEXED, 256, (PULONG)ppdev->PaletteEntries, 0, 0, 0);
    }
    return ppdev->DefaultPalette != NULL;
}

BOOL APIENTRY
IntSetPalette(IN DHPDEV dhpdev, IN PPALETTEENTRY ppalent,
              IN ULONG iStart, IN ULONG cColors)
{
    PVIDEO_CLUT pClut;
    ULONG ClutSize;

    ClutSize = sizeof(VIDEO_CLUT) + (cColors * sizeof(ULONG));
    pClut = EngAllocMem(0, ClutSize, ALLOC_TAG);
    pClut->FirstEntry = iStart;
    pClut->NumEntries = cColors;
    memcpy(&pClut->LookupTable[0].RgbLong, ppalent, sizeof(ULONG) * cColors);

    if (((PPDEV)dhpdev)->PaletteShift)
    {
        while (cColors--)
        {
            pClut->LookupTable[cColors].RgbArray.Red   >>= ((PPDEV)dhpdev)->PaletteShift;
            pClut->LookupTable[cColors].RgbArray.Green >>= ((PPDEV)dhpdev)->PaletteShift;
            pClut->LookupTable[cColors].RgbArray.Blue  >>= ((PPDEV)dhpdev)->PaletteShift;
            pClut->LookupTable[cColors].RgbArray.Unused = 0;
        }
    }
    else
    {
        while (cColors--)
            pClut->LookupTable[cColors].RgbArray.Unused = 0;
    }

    if (EngDeviceIoControl(((PPDEV)dhpdev)->hDriver, IOCTL_VIDEO_SET_COLOR_REGISTERS,
                           pClut, ClutSize, NULL, 0, &cColors))
    {
        EngFreeMem(pClut);
        return FALSE;
    }
    EngFreeMem(pClut);
    return TRUE;
}

BOOL APIENTRY
DrvSetPalette(IN DHPDEV dhpdev, IN PALOBJ *ppalo, IN FLONG fl,
              IN ULONG iStart, IN ULONG cColors)
{
    PPALETTEENTRY PaletteEntries;
    BOOL bRet;

    if (cColors == 0)
        return FALSE;

    PaletteEntries = EngAllocMem(0, cColors * sizeof(ULONG), ALLOC_TAG);
    if (PaletteEntries == NULL)
        return FALSE;

    if (PALOBJ_cGetColors(ppalo, iStart, cColors, (PULONG)PaletteEntries) != cColors)
    {
        EngFreeMem(PaletteEntries);
        return FALSE;
    }

    bRet = IntSetPalette(dhpdev, PaletteEntries, iStart, cColors);
    EngFreeMem(PaletteEntries);
    return bRet;
}
