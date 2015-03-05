/*
 * PROJECT:         ReactOS Framebuffer Display Driver
 * LICENSE:         Microsoft NT4 DDK Sample Code License
 * FILE:            boot/drivers/video/displays/framebuf/screen.c
 * PURPOSE:         Surface, Screen and PDEV support/initialization
 * PROGRAMMERS:     Copyright (c) 1992-1995 Microsoft Corporation
 *                  ReactOS Portable Systems Group
 */
#include "driver.h"

#define SYSTM_LOGFONT {16,7,0,0,700,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,VARIABLE_PITCH | FF_DONTCARE,L"System"}
#define HELVE_LOGFONT {12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_STROKE_PRECIS,PROOF_QUALITY,VARIABLE_PITCH | FF_DONTCARE,L"MS Sans Serif"}
#define COURI_LOGFONT {12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_STROKE_PRECIS,PROOF_QUALITY,FIXED_PITCH | FF_DONTCARE, L"Courier"}

// This is the basic devinfo for a default driver.  This is used as a base and customized based
// on information passed back from the miniport driver.

const DEVINFO gDevInfoFrameBuffer = {
// eVb: 2.1 [VGARISC CHANGE] - No capabilities
    ( 0
// eVb: 2.1 [END]
// eVb: 2.8 [DDK CHANGE] - No dithering support
// eVb: 2.8 [END]
                   ), /* Graphics capabilities         */
    SYSTM_LOGFONT,    /* Default font description */
    HELVE_LOGFONT,    /* ANSI variable font description   */
    COURI_LOGFONT,    /* ANSI fixed font description          */
    0,                /* Count of device fonts          */
// eVb: 2.2 [VGARISC CHANGE] - DIB format is 4BPP
    BMF_4BPP,         /* Preferred DIB format          */
// eVb: 2.2 [END]
// eVb: 2.9 [DDK CHANGE] - No dithering support
    0,                /* Width of color dither          */
    0,                /* Height of color dither   */
// eVb: 2.9 [END]
    0                 /* Default palette to use for this device */
};

// eVb: 2.3 [VGARISC CHANGE] - Add VGA structures from NT4 DDK Sample VGA driver
/******************************Public*Data*Struct*************************\
* This contains the GDIINFO structure that contains the device capabilities
* which are passed to the NT GDI engine during dhpdevEnablePDEV.
*
\**************************************************************************/

GDIINFO gaulCap = {

     GDI_DRIVER_VERSION,
     DT_RASDISPLAY,         // ulTechnology
     0,                     // ulHorzSize
     0,                     // ulVertSize
     0,                     // ulHorzRes (filled in at initialization)
     0,                     // ulVertRes (filled in at initialization)
     4,                     // cBitsPixel
     1,                     // cPlanes
     16,                    // ulNumColors
     0,                     // flRaster (DDI reserved field)

     0,                     // ulLogPixelsX (filled in at initialization)
     0,                     // ulLogPixelsY (filled in at initialization)

     TC_RA_ABLE | TC_SCROLLBLT,  // flTextCaps

     6,                     // ulDACRed
     6,                     // ulDACGree
     6,                     // ulDACBlue

     0x0024,                // ulAspectX  (one-to-one aspect ratio)
     0x0024,                // ulAspectY
     0x0033,                // ulAspectXY

     1,                     // xStyleStep
     1,                     // yStyleSte;
     3,                     // denStyleStep

     { 0, 0 },              // ptlPhysOffset
     { 0, 0 },              // szlPhysSize

     0,                     // ulNumPalReg (win3.1 16 color drivers say 0 too)

// These fields are for halftone initialization.

     {                                          // ciDevice, ColorInfo
        { 6700, 3300, 0 },                      // Red
        { 2100, 7100, 0 },                      // Green
        { 1400,  800, 0 },                      // Blue
        { 1750, 3950, 0 },                      // Cyan
        { 4050, 2050, 0 },                      // Magenta
        { 4400, 5200, 0 },                      // Yellow
        { 3127, 3290, 0 },                      // AlignmentWhite
        20000,                                  // RedGamma
        20000,                                  // GreenGamma
        20000,                                  // BlueGamma
        0, 0, 0, 0, 0, 0
     },

     0,                      // ulDevicePelsDPI  (filled in at initialization)
     PRIMARY_ORDER_CBA,                         // ulPrimaryOrder
     HT_PATSIZE_4x4_M,                          // ulHTPatternSize
     HT_FORMAT_4BPP_IRGB,                       // ulHTOutputFormat
     HT_FLAG_ADDITIVE_PRIMS,                    // flHTFlags

     0,                                         // ulVRefresh
// eVb: 2.4 [VGARISC DDK CHANGE] - Use 1 bit alignment, not 8
     1,                      // ulBltAlignment (preferred window alignment
// eVb: 2.4 [END]
                             //   for fast-text routines)                            
     0,                                         // ulPanningHorzRes
     0,                                         // ulPanningVertRes
};

/******************************Module*Header*******************************\
* Color tables
\**************************************************************************/

// Values for the internal, EGA-compatible palette.

static WORD PaletteBuffer[] = {

        16, // 16 entries
        0,  // start with first palette register

// On the VGA, the palette contains indices into the array of color DACs.
// Since we can program the DACs as we please, we'll just put all the indices
// down at the beginning of the DAC array (that is, pass pixel values through
// the internal palette unchanged).

        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};


// These are the values for the first 16 DAC registers, the only ones we'll
// work with. These correspond to the RGB colors (6 bits for each primary, with
// the fourth entry unused) for pixel values 0-15.

static BYTE ColorBuffer[] = {

      16, // 16 entries
      0,
      0,
      0,  // start with first palette register
                0x00, 0x00, 0x00, 0x00, // black
                0x2A, 0x00, 0x15, 0x00, // red
                0x00, 0x2A, 0x15, 0x00, // green
                0x2A, 0x2A, 0x15, 0x00, // mustard/brown
                0x00, 0x00, 0x2A, 0x00, // blue
                0x2A, 0x15, 0x2A, 0x00, // magenta
                0x15, 0x2A, 0x2A, 0x00, // ScanLinesan
                0x21, 0x22, 0x23, 0x00, // dark gray   2A
                0x30, 0x31, 0x32, 0x00, // light gray  39
                0x3F, 0x00, 0x00, 0x00, // bright red
                0x00, 0x3F, 0x00, 0x00, // bright green
                0x3F, 0x3F, 0x00, 0x00, // bright yellow
                0x00, 0x00, 0x3F, 0x00, // bright blue
                0x3F, 0x00, 0x3F, 0x00, // bright magenta
                0x00, 0x3F, 0x3F, 0x00, // bright ScanLinesan
                0x3F, 0x3F, 0x3F, 0x00  // bright white
};
// eVb: 2.3 [END]
/******************************Public*Routine******************************\
* bInitSURF
*
* Enables the surface.        Maps the frame buffer into memory.
*
\**************************************************************************/

BOOL bInitSURF(PPDEV ppdev, BOOL bFirst)
{
    DWORD returnedDataLength;
    DWORD MaxWidth, MaxHeight;
    VIDEO_MEMORY videoMemory;
    VIDEO_MEMORY_INFORMATION videoMemoryInformation;
// eVb: 2.1 [DDK Change] - Support new VGA Miniport behavior w.r.t updated framebuffer remapping
    ULONG RemappingNeeded = 0;  
// eVb: 2.1 [END]
    //
    // Set the current mode into the hardware.
    //

    if (EngDeviceIoControl(ppdev->hDriver,
                           IOCTL_VIDEO_SET_CURRENT_MODE,
                           &(ppdev->ulMode),
                           sizeof(ULONG),
// eVb: 2.2 [DDK Change] - Support new VGA Miniport behavior w.r.t updated framebuffer remapping
                           &RemappingNeeded,
                           sizeof(ULONG),
// eVb: 2.2 [END]
                           &returnedDataLength))
    {
        RIP("DISP bInitSURF failed IOCTL_SET_MODE\n");
        return(FALSE);
    }

    //
    // If this is the first time we enable the surface we need to map in the
    // memory also.
    //
// eVb: 2.3 [DDK Change] - Support new VGA Miniport behavior w.r.t updated framebuffer remapping
    if (bFirst || RemappingNeeded)
    {
// eVb: 2.3 [END]
        videoMemory.RequestedVirtualAddress = NULL;

        if (EngDeviceIoControl(ppdev->hDriver,
                               IOCTL_VIDEO_MAP_VIDEO_MEMORY,
                               &videoMemory,
                               sizeof(VIDEO_MEMORY),
                               &videoMemoryInformation,
                               sizeof(VIDEO_MEMORY_INFORMATION),
                               &returnedDataLength))
        {
            RIP("DISP bInitSURF failed IOCTL_VIDEO_MAP\n");
            return(FALSE);
        }

        ppdev->pjScreen = (PBYTE)(videoMemoryInformation.FrameBufferBase);

        if (videoMemoryInformation.FrameBufferBase !=
            videoMemoryInformation.VideoRamBase)
        {
            RIP("VideoRamBase does not correspond to FrameBufferBase\n");
        }
// eVb: 2.4 [DDK Change] - Make sure frame buffer mapping worked
        //
        // Make sure we can access this video memory
        //

        *(PULONG)(ppdev->pjScreen) = 0xaa55aa55;

        if (*(PULONG)(ppdev->pjScreen) != 0xaa55aa55) {

            DISPDBG((1, "Frame buffer memory is not accessible.\n"));
            return(FALSE);
        }
// eVb: 2.4 [END]
        ppdev->cScreenSize = videoMemoryInformation.VideoRamLength;

        //
        // Initialize the head of the offscreen list to NULL.
        //

        ppdev->pOffscreenList = NULL;

        // It's a hardware pointer; set up pointer attributes.

        MaxHeight = ppdev->PointerCapabilities.MaxHeight;

        // Allocate space for two DIBs (data/mask) for the pointer. If this
        // device supports a color Pointer, we will allocate a larger bitmap.
        // If this is a color bitmap we allocate for the largest possible
        // bitmap because we have no idea of what the pixel depth might be.

        // Width rounded up to nearest byte multiple

        if (!(ppdev->PointerCapabilities.Flags & VIDEO_MODE_COLOR_POINTER))
        {
            MaxWidth = (ppdev->PointerCapabilities.MaxWidth + 7) / 8;
        }
        else
        {
            MaxWidth = ppdev->PointerCapabilities.MaxWidth * sizeof(DWORD);
        }

        ppdev->cjPointerAttributes =
                sizeof(VIDEO_POINTER_ATTRIBUTES) +
                ((sizeof(UCHAR) * MaxWidth * MaxHeight) * 2);

        ppdev->pPointerAttributes = (PVIDEO_POINTER_ATTRIBUTES)
                EngAllocMem(0, ppdev->cjPointerAttributes, ALLOC_TAG);

        if (ppdev->pPointerAttributes == NULL) {

            DISPDBG((0, "bInitPointer EngAllocMem failed\n"));
            return(FALSE);
        }

        ppdev->pPointerAttributes->Flags = ppdev->PointerCapabilities.Flags;
        ppdev->pPointerAttributes->WidthInBytes = MaxWidth;
        ppdev->pPointerAttributes->Width = ppdev->PointerCapabilities.MaxWidth;
        ppdev->pPointerAttributes->Height = MaxHeight;
        ppdev->pPointerAttributes->Column = 0;
        ppdev->pPointerAttributes->Row = 0;
        ppdev->pPointerAttributes->Enable = 0;
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* vDisableSURF
*
* Disable the surface. Un-Maps the frame in memory.
*
\**************************************************************************/

VOID vDisableSURF(PPDEV ppdev)
{
    DWORD returnedDataLength;
    VIDEO_MEMORY videoMemory;

    videoMemory.RequestedVirtualAddress = (PVOID) ppdev->pjScreen;

    if (EngDeviceIoControl(ppdev->hDriver,
                           IOCTL_VIDEO_UNMAP_VIDEO_MEMORY,
                           &videoMemory,
                           sizeof(VIDEO_MEMORY),
                           NULL,
                           0,
                           &returnedDataLength))
    {
        RIP("DISP vDisableSURF failed IOCTL_VIDEO_UNMAP\n");
    }
}


/******************************Public*Routine******************************\
* bInitPDEV
*
* Determine the mode we should be in based on the DEVMODE passed in.
* Query mini-port to get information needed to fill in the DevInfo and the
* GdiInfo .
*
\**************************************************************************/

BOOL bInitPDEV(
PPDEV ppdev,
DEVMODEW *pDevMode,
GDIINFO *pGdiInfo,
DEVINFO *pDevInfo)
{
    ULONG cModes;
    PVIDEO_MODE_INFORMATION pVideoBuffer, pVideoModeSelected, pVideoTemp;
    VIDEO_COLOR_CAPABILITIES colorCapabilities;
    ULONG ulTemp;
    BOOL bSelectDefault;
    ULONG cbModeSize;

    //
    // calls the miniport to get mode information.
    //

    cModes = getAvailableModes(ppdev->hDriver, &pVideoBuffer, &cbModeSize);

    if (cModes == 0)
    {
        return(FALSE);
    }

    //
    // Now see if the requested mode has a match in that table.
    //

    pVideoModeSelected = NULL;
    pVideoTemp = pVideoBuffer;

    if ((pDevMode->dmPelsWidth        == 0) &&
        (pDevMode->dmPelsHeight       == 0) &&
        (pDevMode->dmBitsPerPel       == 0) &&
        (pDevMode->dmDisplayFrequency == 0))
    {
        DISPDBG((2, "Default mode requested"));
        bSelectDefault = TRUE;
    }
    else
    {
// eVb: 2.5 [DDK Change] - Add missing newlines to debug output
        DISPDBG((2, "Requested mode...\n"));
        DISPDBG((2, "   Screen width  -- %li\n", pDevMode->dmPelsWidth));
        DISPDBG((2, "   Screen height -- %li\n", pDevMode->dmPelsHeight));
        DISPDBG((2, "   Bits per pel  -- %li\n", pDevMode->dmBitsPerPel));
        DISPDBG((2, "   Frequency     -- %li\n", pDevMode->dmDisplayFrequency));
// eVb: 2.5 [END]
        bSelectDefault = FALSE;
    }

    while (cModes--)
    {
        if (pVideoTemp->Length != 0)
        {
            if (bSelectDefault ||
                ((pVideoTemp->VisScreenWidth  == pDevMode->dmPelsWidth) &&
                 (pVideoTemp->VisScreenHeight == pDevMode->dmPelsHeight) &&
                 (pVideoTemp->BitsPerPlane *
                  pVideoTemp->NumberOfPlanes  == pDevMode->dmBitsPerPel) &&
                 (pVideoTemp->Frequency  == pDevMode->dmDisplayFrequency)))
            {
                pVideoModeSelected = pVideoTemp;
                DISPDBG((3, "Found a match\n")) ;
                break;
            }
        }

        pVideoTemp = (PVIDEO_MODE_INFORMATION)
            (((PUCHAR)pVideoTemp) + cbModeSize);
    }

    //
    // If no mode has been found, return an error
    //

    if (pVideoModeSelected == NULL)
    {
        EngFreeMem(pVideoBuffer);
        DISPDBG((0,"DISP bInitPDEV failed - no valid modes\n"));
        return(FALSE);
    }

    //
    // Fill in the GDIINFO data structure with the information returned from
    // the kernel driver.
    //

    ppdev->ulMode = pVideoModeSelected->ModeIndex;
    ppdev->cxScreen = pVideoModeSelected->VisScreenWidth;
    ppdev->cyScreen = pVideoModeSelected->VisScreenHeight;
    ppdev->lDeltaScreen = pVideoModeSelected->ScreenStride;
// eVb: 2.8 [VGARISC CHANGE] - Extra fields not required on VGA, start with defaults
#if 0
    ppdev->ulBitCount = pVideoModeSelected->BitsPerPlane *
                        pVideoModeSelected->NumberOfPlanes;
    ppdev->flRed = pVideoModeSelected->RedMask;
    ppdev->flGreen = pVideoModeSelected->GreenMask;
    ppdev->flBlue = pVideoModeSelected->BlueMask;
#else
    *pGdiInfo = gaulCap;
#endif
// eVb: 2.8 [END]


    pGdiInfo->ulVersion    = GDI_DRIVER_VERSION;
    pGdiInfo->ulTechnology = DT_RASDISPLAY;
    pGdiInfo->ulHorzSize   = pVideoModeSelected->XMillimeter;
    pGdiInfo->ulVertSize   = pVideoModeSelected->YMillimeter;

    pGdiInfo->ulHorzRes        = ppdev->cxScreen;
    pGdiInfo->ulVertRes        = ppdev->cyScreen;
    pGdiInfo->ulPanningHorzRes = ppdev->cxScreen;
    pGdiInfo->ulPanningVertRes = ppdev->cyScreen;
    pGdiInfo->cBitsPixel       = pVideoModeSelected->BitsPerPlane;
    pGdiInfo->cPlanes          = pVideoModeSelected->NumberOfPlanes;
    pGdiInfo->ulVRefresh       = pVideoModeSelected->Frequency;
    pGdiInfo->ulBltAlignment   = 1;     // We don't have accelerated screen-
                                        //   to-screen blts, and any
                                        //   window alignment is okay

    pGdiInfo->ulLogPixelsX = pDevMode->dmLogPixels;
    pGdiInfo->ulLogPixelsY = pDevMode->dmLogPixels;

// eVb: 2.9 [VGARISC CHANGE] - Extra fields not required on VGA
#if 0
#ifdef MIPS
    if (ppdev->ulBitCount == 8)
        pGdiInfo->flTextCaps = (TC_RA_ABLE | TC_SCROLLBLT);
    else
#endif
    pGdiInfo->flTextCaps = TC_RA_ABLE;

    pGdiInfo->flRaster = 0;           // flRaster is reserved by DDI
#endif
// eVb: 2.9 [END]

    pGdiInfo->ulDACRed   = pVideoModeSelected->NumberRedBits;
    pGdiInfo->ulDACGreen = pVideoModeSelected->NumberGreenBits;
    pGdiInfo->ulDACBlue  = pVideoModeSelected->NumberBlueBits;

// eVb: 2.7 [VGARISC CHANGE] - Extra fields not required on VGA
#if 0
    pGdiInfo->ulAspectX    = 0x24;    // One-to-one aspect ratio
    pGdiInfo->ulAspectY    = 0x24;
    pGdiInfo->ulAspectXY   = 0x33;

    pGdiInfo->xStyleStep   = 1;       // A style unit is 3 pels
    pGdiInfo->yStyleStep   = 1;
    pGdiInfo->denStyleStep = 3;

    pGdiInfo->ptlPhysOffset.x = 0;
    pGdiInfo->ptlPhysOffset.y = 0;
    pGdiInfo->szlPhysSize.cx  = 0;
    pGdiInfo->szlPhysSize.cy  = 0;

    // RGB and CMY color info.

    //
    // try to get it from the miniport.
    // if the miniport doesn ot support this feature, use defaults.
    //

    if (EngDeviceIoControl(ppdev->hDriver,
                           IOCTL_VIDEO_QUERY_COLOR_CAPABILITIES,
                           NULL,
                           0,
                           &colorCapabilities,
                           sizeof(VIDEO_COLOR_CAPABILITIES),
                           &ulTemp))
    {

        DISPDBG((2, "getcolorCapabilities failed \n"));

        pGdiInfo->ciDevice.Red.x = 6700;
        pGdiInfo->ciDevice.Red.y = 3300;
        pGdiInfo->ciDevice.Red.Y = 0;
        pGdiInfo->ciDevice.Green.x = 2100;
        pGdiInfo->ciDevice.Green.y = 7100;
        pGdiInfo->ciDevice.Green.Y = 0;
        pGdiInfo->ciDevice.Blue.x = 1400;
        pGdiInfo->ciDevice.Blue.y = 800;
        pGdiInfo->ciDevice.Blue.Y = 0;
        pGdiInfo->ciDevice.AlignmentWhite.x = 3127;
        pGdiInfo->ciDevice.AlignmentWhite.y = 3290;
        pGdiInfo->ciDevice.AlignmentWhite.Y = 0;

        pGdiInfo->ciDevice.RedGamma = 20000;
        pGdiInfo->ciDevice.GreenGamma = 20000;
        pGdiInfo->ciDevice.BlueGamma = 20000;

    }
    else
    {
        pGdiInfo->ciDevice.Red.x = colorCapabilities.RedChromaticity_x;
        pGdiInfo->ciDevice.Red.y = colorCapabilities.RedChromaticity_y;
        pGdiInfo->ciDevice.Red.Y = 0;
        pGdiInfo->ciDevice.Green.x = colorCapabilities.GreenChromaticity_x;
        pGdiInfo->ciDevice.Green.y = colorCapabilities.GreenChromaticity_y;
        pGdiInfo->ciDevice.Green.Y = 0;
        pGdiInfo->ciDevice.Blue.x = colorCapabilities.BlueChromaticity_x;
        pGdiInfo->ciDevice.Blue.y = colorCapabilities.BlueChromaticity_y;
        pGdiInfo->ciDevice.Blue.Y = 0;
        pGdiInfo->ciDevice.AlignmentWhite.x = colorCapabilities.WhiteChromaticity_x;
        pGdiInfo->ciDevice.AlignmentWhite.y = colorCapabilities.WhiteChromaticity_y;
        pGdiInfo->ciDevice.AlignmentWhite.Y = colorCapabilities.WhiteChromaticity_Y;

        // if we have a color device store the three color gamma values,
        // otherwise store the unique gamma value in all three.

        if (colorCapabilities.AttributeFlags & VIDEO_DEVICE_COLOR)
        {
            pGdiInfo->ciDevice.RedGamma = colorCapabilities.RedGamma;
            pGdiInfo->ciDevice.GreenGamma = colorCapabilities.GreenGamma;
            pGdiInfo->ciDevice.BlueGamma = colorCapabilities.BlueGamma;
        }
        else
        {
            pGdiInfo->ciDevice.RedGamma = colorCapabilities.WhiteGamma;
            pGdiInfo->ciDevice.GreenGamma = colorCapabilities.WhiteGamma;
            pGdiInfo->ciDevice.BlueGamma = colorCapabilities.WhiteGamma;
        }

    };

    pGdiInfo->ciDevice.Cyan.x = 0;
    pGdiInfo->ciDevice.Cyan.y = 0;
    pGdiInfo->ciDevice.Cyan.Y = 0;
    pGdiInfo->ciDevice.Magenta.x = 0;
    pGdiInfo->ciDevice.Magenta.y = 0;
    pGdiInfo->ciDevice.Magenta.Y = 0;
    pGdiInfo->ciDevice.Yellow.x = 0;
    pGdiInfo->ciDevice.Yellow.y = 0;
    pGdiInfo->ciDevice.Yellow.Y = 0;

    // No dye correction for raster displays.

    pGdiInfo->ciDevice.MagentaInCyanDye = 0;
    pGdiInfo->ciDevice.YellowInCyanDye = 0;
    pGdiInfo->ciDevice.CyanInMagentaDye = 0;
    pGdiInfo->ciDevice.YellowInMagentaDye = 0;
    pGdiInfo->ciDevice.CyanInYellowDye = 0;
    pGdiInfo->ciDevice.MagentaInYellowDye = 0;

    pGdiInfo->ulDevicePelsDPI = 0;   // For printers only
    pGdiInfo->ulPrimaryOrder = PRIMARY_ORDER_CBA;

    // BUGBUG this should be modified to take into account the size
    // of the display and the resolution.

    pGdiInfo->ulHTPatternSize = HT_PATSIZE_4x4_M;

    pGdiInfo->flHTFlags = HT_FLAG_ADDITIVE_PRIMS;
// eVb: 2.7 [END]
#endif

    // Fill in the basic devinfo structure

    *pDevInfo = gDevInfoFrameBuffer;

// eVb: 2.6 [VGARISC CHANGE] - Use defaults in gaulCap for GDI Info
#if 0
    // Fill in the rest of the devinfo and GdiInfo structures.

    if (ppdev->ulBitCount == 8)
    {
        // It is Palette Managed.

        pGdiInfo->ulNumColors = 20;
        pGdiInfo->ulNumPalReg = 1 << ppdev->ulBitCount;
// eVb: 2.7 [DDK CHANGE] - No dithering support
        pDevInfo->flGraphicsCaps |= GCAPS_PALMANAGED;
// eVb: 2.7 [END]
        pGdiInfo->ulHTOutputFormat = HT_FORMAT_8BPP;
        pDevInfo->iDitherFormat = BMF_8BPP;

        // Assuming palette is orthogonal - all colors are same size.

        ppdev->cPaletteShift   = 8 - pGdiInfo->ulDACRed;
    }
    else
    {
        pGdiInfo->ulNumColors = (ULONG) (-1);
        pGdiInfo->ulNumPalReg = 0;

        if (ppdev->ulBitCount == 16)
        {
            pGdiInfo->ulHTOutputFormat = HT_FORMAT_16BPP;
            pDevInfo->iDitherFormat = BMF_16BPP;
        }
        else if (ppdev->ulBitCount == 24)
        {
            pGdiInfo->ulHTOutputFormat = HT_FORMAT_24BPP;
            pDevInfo->iDitherFormat = BMF_24BPP;
        }
        else
        {
            pGdiInfo->ulHTOutputFormat = HT_FORMAT_32BPP;
            pDevInfo->iDitherFormat = BMF_32BPP;
        }
    }
#endif
// eVb: 2.6 [END]

    EngFreeMem(pVideoBuffer);

    return(TRUE);
}


/******************************Public*Routine******************************\
* getAvailableModes
*
* Calls the miniport to get the list of modes supported by the kernel driver,
* and returns the list of modes supported by the diplay driver among those
*
* returns the number of entries in the videomode buffer.
* 0 means no modes are supported by the miniport or that an error occured.
*
* NOTE: the buffer must be freed up by the caller.
*
\**************************************************************************/

DWORD getAvailableModes(
HANDLE hDriver,
PVIDEO_MODE_INFORMATION *modeInformation,
DWORD *cbModeSize)
{
    ULONG ulTemp;
    VIDEO_NUM_MODES modes;
    PVIDEO_MODE_INFORMATION pVideoTemp;

    //
    // Get the number of modes supported by the mini-port
    //

    if (EngDeviceIoControl(hDriver,
                           IOCTL_VIDEO_QUERY_NUM_AVAIL_MODES,
                           NULL,
                           0,
                           &modes,
                           sizeof(VIDEO_NUM_MODES),
                           &ulTemp))
    {
        DISPDBG((0, "getAvailableModes failed VIDEO_QUERY_NUM_AVAIL_MODES\n"));
        return(0);
    }

    *cbModeSize = modes.ModeInformationLength;

    //
    // Allocate the buffer for the mini-port to write the modes in.
    //

    *modeInformation = (PVIDEO_MODE_INFORMATION)
                        EngAllocMem(0, modes.NumModes *
                                    modes.ModeInformationLength, ALLOC_TAG);

    if (*modeInformation == (PVIDEO_MODE_INFORMATION) NULL)
    {
        DISPDBG((0, "getAvailableModes failed EngAllocMem\n"));

        return 0;
    }

    //
    // Ask the mini-port to fill in the available modes.
    //

    if (EngDeviceIoControl(hDriver,
                           IOCTL_VIDEO_QUERY_AVAIL_MODES,
                           NULL,
                           0,
                           *modeInformation,
                           modes.NumModes * modes.ModeInformationLength,
                           &ulTemp))
    {

        DISPDBG((0, "getAvailableModes failed VIDEO_QUERY_AVAIL_MODES\n"));

        EngFreeMem(*modeInformation);
        *modeInformation = (PVIDEO_MODE_INFORMATION) NULL;

        return(0);
    }

    //
    // Now see which of these modes are supported by the display driver.
    // As an internal mechanism, set the length to 0 for the modes we
    // DO NOT support.
    //

    ulTemp = modes.NumModes;
    pVideoTemp = *modeInformation;

// eVb: 2.5 [VGARISC CHANGE] - Add correct mode checks for VGA
    //
    // Mode is rejected if it is not 4 planes, or not graphics, or is not
    // one of 1 bits per pel.
    //

    while (ulTemp--)
    {
        if ((pVideoTemp->NumberOfPlanes != 4 ) ||
            !(pVideoTemp->AttributeFlags & VIDEO_MODE_GRAPHICS) ||
            ((pVideoTemp->BitsPerPlane != 1) ||
             (pVideoTemp->VisScreenWidth > 800)))
// eVb: 2.5 [END]
        {
            pVideoTemp->Length = 0;
        }

        pVideoTemp = (PVIDEO_MODE_INFORMATION)
            (((PUCHAR)pVideoTemp) + modes.ModeInformationLength);
    }

    return modes.NumModes;

}
