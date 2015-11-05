/*
 * PROJECT:         ReactOS Framebuffer Display Driver
 * LICENSE:         Microsoft NT4 DDK Sample Code License
 * FILE:            win32ss/drivers/displays/vga_new/palette.c
 * PURPOSE:         Palette Support
 * PROGRAMMERS:     Copyright (c) 1992-1995 Microsoft Corporation
 */

#include "driver.h"

// eVb: 4.1 [VGARISC Change] - Add static 16-color VGA palette from VGA NT4 DDK Sample

/******************************Public*Data*Struct*************************\
* LOGPALETTE
*
* This is the palette for the VGA.
*
\**************************************************************************/

// Little bit of hacking to get this to compile happily.

typedef struct _VGALOGPALETTE
{
    USHORT ident;
    USHORT NumEntries;
    PALETTEENTRY palPalEntry[16];
} VGALOGPALETTE;

const VGALOGPALETTE logPalVGA =
{

0x400,  // driver version
16,     // num entries
{
    { 0,   0,   0,   0 },       // 0
    { 0x80,0,   0,   0 },       // 1
    { 0,   0x80,0,   0 },       // 2
    { 0x80,0x80,0,   0 },       // 3
    { 0,   0,   0x80,0 },       // 4
    { 0x80,0,   0x80,0 },       // 5
    { 0,   0x80,0x80,0 },       // 6
    { 0x80,0x80,0x80,0 },       // 7

    { 0xC0,0xC0,0xC0,0 },       // 8
    { 0xFF,0,   0,   0 },       // 9
    { 0,   0xFF,0,   0 },       // 10
    { 0xFF,0xFF,0,   0 },       // 11
    { 0,   0,   0xFF,0 },       // 12
    { 0xFF,0,   0xFF,0 },       // 13
    { 0,   0xFF,0xFF,0 },       // 14
    { 0xFF,0xFF,0xFF,0 }        // 15
}
};
// eVb: 4.1 [END]

BOOL bInitDefaultPalette(PPDEV ppdev, DEVINFO *pDevInfo);

/******************************Public*Routine******************************\
* bInitPaletteInfo
*
* Initializes the palette information for this PDEV.
*
* Called by DrvEnablePDEV.
*
\**************************************************************************/

BOOL bInitPaletteInfo(PPDEV ppdev, DEVINFO *pDevInfo)
{
    if (!bInitDefaultPalette(ppdev, pDevInfo))
        return(FALSE);

    return(TRUE);
}

/******************************Public*Routine******************************\
* vDisablePalette
*
* Frees resources allocated by bInitPaletteInfo.
*
\**************************************************************************/

VOID vDisablePalette(PPDEV ppdev)
{
// Delete the default palette if we created one.

    if (ppdev->hpalDefault)
    {
        EngDeletePalette(ppdev->hpalDefault);
        ppdev->hpalDefault = (HPALETTE) 0;
    }

// eVb: 4.2 [VGARISC Change] - VGA Palette is static, no need to free
#if 0
    if (ppdev->pPal != (PPALETTEENTRY)NULL)
        EngFreeMem((PVOID)ppdev->pPal);
#endif
// eVb: 4.2 [END]
}

/******************************Public*Routine******************************\
* bInitDefaultPalette
*
* Initializes default palette for PDEV.
*
\**************************************************************************/

BOOL bInitDefaultPalette(PPDEV ppdev, DEVINFO *pDevInfo)
{
// eVb: 4.3 [VGARISC Change] - VGA Palette is static, no need to build
#if 0
    if (ppdev->ulBitCount == 8)
    {
        ULONG ulLoop;
        BYTE jRed,jGre,jBlu;

        //
        // Allocate our palette
        //

        ppdev->pPal = (PPALETTEENTRY)EngAllocMem(0, sizeof(PALETTEENTRY) * 256,
                                                 ALLOC_TAG);

        if ((ppdev->pPal) == NULL) {
            RIP("DISP bInitDefaultPalette() failed EngAllocMem\n");
            return(FALSE);
        }

        //
        // Generate 256 (8*4*4) RGB combinations to fill the palette
        //

        jRed = jGre = jBlu = 0;

        for (ulLoop = 0; ulLoop < 256; ulLoop++)
        {
            ppdev->pPal[ulLoop].peRed   = jRed;
            ppdev->pPal[ulLoop].peGreen = jGre;
            ppdev->pPal[ulLoop].peBlue  = jBlu;
            ppdev->pPal[ulLoop].peFlags = (BYTE)0;

            if (!(jRed += 32))
            if (!(jGre += 32))
            jBlu += 64;
        }

        //
        // Fill in Windows Reserved Colors from the WIN 3.0 DDK
        // The Window Manager reserved the first and last 10 colors for
        // painting windows borders and for non-palette managed applications.
        //

        for (ulLoop = 0; ulLoop < 10; ulLoop++)
        {
            //
            // First 10
            //

            ppdev->pPal[ulLoop] = BASEPALETTE[ulLoop];

            //
            // Last 10
            //

            ppdev->pPal[246 + ulLoop] = BASEPALETTE[ulLoop+10];
        }

#endif
// eVb: 4.3 [END]
        //
        // Create handle for palette.
        //

        ppdev->hpalDefault =
        pDevInfo->hpalDefault = EngCreatePalette(PAL_INDEXED,
// eVb: 4.4 [VGARISC Change] - VGA Palette is 16 colors, not 256, and static
                                                   16,
                                                   (PULONG) &logPalVGA.palPalEntry,
// eVb: 4.4 [END]
                                                   0,0,0);

        if (ppdev->hpalDefault == (HPALETTE) 0)
        {
            RIP("DISP bInitDefaultPalette failed EngCreatePalette\n");
// eVb: 4.5 [VGARISC Change] - VGA Palette is static, no need to free
            //EngFreeMem(ppdev->pPal);
// eVb: 4.5 [END]        
            return(FALSE);
        }

        //
        // Initialize the hardware with the initial palette.
        //

        return(TRUE);

// eVb: 4.6 [VGARISC Change] - VGA Palette is static, no bitfield palette needed
#if 0
    } else {

        ppdev->hpalDefault =
        pDevInfo->hpalDefault = EngCreatePalette(PAL_BITFIELDS,
                                                   0,(PULONG) NULL,
                                                   ppdev->flRed,
                                                   ppdev->flGreen,
                                                   ppdev->flBlue);

        if (ppdev->hpalDefault == (HPALETTE) 0)
        {
            RIP("DISP bInitDefaultPalette failed EngCreatePalette\n");
            return(FALSE);
        }
    }

    return(TRUE);
#endif
// eVb: 4.6 [END]  
}

/******************************Public*Routine******************************\
* bInit256ColorPalette
*
* Initialize the hardware's palette registers.
*
\**************************************************************************/

BOOL bInit256ColorPalette(PPDEV ppdev)
{
    BYTE        ajClutSpace[MAX_CLUT_SIZE];
    PVIDEO_CLUT pScreenClut;
    ULONG       ulReturnedDataLength;
    ULONG       cColors;
    PVIDEO_CLUTDATA pScreenClutData;

    if (ppdev->ulBitCount == 8)
    {
        //
        // Fill in pScreenClut header info:
        //

        pScreenClut             = (PVIDEO_CLUT) ajClutSpace;
        pScreenClut->NumEntries = 256;
        pScreenClut->FirstEntry = 0;

        //
        // Copy colours in:
        //

        cColors = 256;
        pScreenClutData = (PVIDEO_CLUTDATA) (&(pScreenClut->LookupTable[0]));

        while(cColors--)
        {
            pScreenClutData[cColors].Red =    ppdev->pPal[cColors].peRed >>
                                              ppdev->cPaletteShift;
            pScreenClutData[cColors].Green =  ppdev->pPal[cColors].peGreen >>
                                              ppdev->cPaletteShift;
            pScreenClutData[cColors].Blue =   ppdev->pPal[cColors].peBlue >>
                                              ppdev->cPaletteShift;
            pScreenClutData[cColors].Unused = 0;
        }

        //
        // Set palette registers:
        //

        if (EngDeviceIoControl(ppdev->hDriver,
                               IOCTL_VIDEO_SET_COLOR_REGISTERS,
                               pScreenClut,
                               MAX_CLUT_SIZE,
                               NULL,
                               0,
                               &ulReturnedDataLength))
        {
            DISPDBG((0, "Failed bEnablePalette"));
            return(FALSE);
        }
    }

    DISPDBG((5, "Passed bEnablePalette"));

    return(TRUE);
}

/******************************Public*Routine******************************\
* DrvSetPalette
*
* DDI entry point for manipulating the palette.
*
\**************************************************************************/

BOOL DrvSetPalette(
DHPDEV  dhpdev,
PALOBJ* ppalo,
FLONG   fl,
ULONG   iStart,
ULONG   cColors)
{
    BYTE            ajClutSpace[MAX_CLUT_SIZE];
    PVIDEO_CLUT     pScreenClut;
    PVIDEO_CLUTDATA pScreenClutData;
    PDEV*           ppdev;

    UNREFERENCED_PARAMETER(fl);

    ppdev = (PDEV*) dhpdev;

    //
    // Fill in pScreenClut header info:
    //

    pScreenClut             = (PVIDEO_CLUT) ajClutSpace;
    pScreenClut->NumEntries = (USHORT) cColors;
    pScreenClut->FirstEntry = (USHORT) iStart;

    pScreenClutData = (PVIDEO_CLUTDATA) (&(pScreenClut->LookupTable[0]));

    if (cColors != PALOBJ_cGetColors(ppalo, iStart, cColors,
                                     (ULONG*) pScreenClutData))
    {
        DISPDBG((0, "DrvSetPalette failed PALOBJ_cGetColors\n"));
        return (FALSE);
    }

    //
    // Set the high reserved byte in each palette entry to 0.
    // Do the appropriate palette shifting to fit in the DAC.
    //

    if (ppdev->cPaletteShift)
    {
        while(cColors--)
        {
            pScreenClutData[cColors].Red >>= ppdev->cPaletteShift;
            pScreenClutData[cColors].Green >>= ppdev->cPaletteShift;
            pScreenClutData[cColors].Blue >>= ppdev->cPaletteShift;
            pScreenClutData[cColors].Unused = 0;
        }
    }
    else
    {
        while(cColors--)
        {
            pScreenClutData[cColors].Unused = 0;
        }
    }

    //
    // Set palette registers
    //

    if (EngDeviceIoControl(ppdev->hDriver,
                           IOCTL_VIDEO_SET_COLOR_REGISTERS,
                           pScreenClut,
                           MAX_CLUT_SIZE,
                           NULL,
                           0,
                           &cColors))
    {
        DISPDBG((0, "DrvSetPalette failed EngDeviceIoControl\n"));
        return (FALSE);
    }

    return(TRUE);

}
