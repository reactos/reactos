/*
 * PROJECT:         ReactOS VGA display driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            win32ss/drivers/displays/vga/main/enable.c
 * PURPOSE:
 * PROGRAMMERS:
 */

#include <vgaddi.h>

static BOOL VGAInitialized = FALSE;

static DRVFN FuncList[] =
{
    /* Required Display driver fuctions */
    {INDEX_DrvAssertMode, (PFN) DrvAssertMode},
    {INDEX_DrvCompletePDEV, (PFN) DrvCompletePDEV},
    {INDEX_DrvCopyBits, (PFN) DrvCopyBits},
    {INDEX_DrvDisablePDEV, (PFN) DrvDisablePDEV},
    {INDEX_DrvDisableSurface, (PFN) DrvDisableSurface},
    {INDEX_DrvEnablePDEV, (PFN) DrvEnablePDEV},
    {INDEX_DrvEnableSurface, (PFN) DrvEnableSurface},
    {INDEX_DrvGetModes, (PFN) DrvGetModes},
    {INDEX_DrvLineTo, (PFN) DrvLineTo},
    {INDEX_DrvPaint, (PFN) DrvPaint},
    {INDEX_DrvBitBlt, (PFN) DrvBitBlt},
    {INDEX_DrvTransparentBlt, (PFN) DrvTransparentBlt},
    {INDEX_DrvMovePointer, (PFN) DrvMovePointer},
    {INDEX_DrvSetPointerShape, (PFN) DrvSetPointerShape},

#if 0
    /* Optional Display driver functions */
    {INDEX_, },
    {INDEX_DescribePixelFormat, (PFN) VGADDIDescribePixelFormat},
    {INDEX_DrvDitherColor, (PFN) VGADDIDitherColor},
    {INDEX_DrvFillPath, (PFN) VGADDIFillPath},
    {INDEX_DrvGetTrueTypeFile, (PFN) VGADDIGetTrueTypeFile},
    {INDEX_DrvLoadFontFile, (PFN) VGADDILoadFontFile},
    {INDEX_DrvQueryFont, (PFN) VGADDIQueryFont},
    {INDEX_DrvQueryFontCaps, (PFN) VGADDIQueryFontCaps},
    {INDEX_DrvQueryFontData, (PFN) VGADDIQueryFontData},
    {INDEX_DrvQueryFontFile, (PFN) VGADDIQueryFontFile},
    {INDEX_DrvQueryFontTree, (PFN) VGADDIQueryFontTree},
    {INDEX_DrvQueryTrueTypeOutline, (PFN) VGADDIQueryTrueTypeOutline},
    {INDEX_DrvQueryTrueTypeTable, (PFN) VGADDIQueryTrueTypeTable},
    {INDEX_DrvRealizeBrush, (PFN) VGADDIRealizeBrush},
    {INDEX_DrvResetPDEV, (PFN) VGADDIResetPDEV},
    {INDEX_DrvSetPalette, (PFN) VGADDISetPalette},
    {INDEX_DrvSetPixelFormat, (PFN) VGADDISetPixelFormat},
    {INDEX_DrvStretchBlt, (PFN) VGADDIStretchBlt},
    {INDEX_DrvStrokePath, (PFN) VGADDIStrokePath},
    {INDEX_DrvSwapBuffers, (PFN) VGADDISwapBuffers},
    {INDEX_DrvTextOut, (PFN) VGADDITextOut},
    {INDEX_DrvUnloadFontFile, (PFN) VGADDIUnloadFontFile},
#endif
};

static GDIINFO gaulCap = {
    GDI_DRIVER_VERSION,    // ulVersion
    DT_RASDISPLAY,         // ulTechnology
    0,                     // ulHorzSize
    0,                     // ulVertSize
    0,                     // ulHorzRes (filled in at initialization)
    0,                     // ulVertRes (filled in at initialization)
    4,                     // cBitsPixel
    1,                     // cPlanes
    16,                    // ulNumColors
    0,                     // flRaster (DDI reserved field)

    96,                    // ulLogPixelsX (must be set to 96 according to MSDN)
    96,                    // ulLogPixelsY (must be set to 96 according to MSDN)

    TC_RA_ABLE | TC_SCROLLBLT,  // flTextCaps

    6,                     // ulDACRed
    6,                     // ulDACGreen
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

    {                                         // ciDevice, ColorInfo
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

    0,                                         // ulDevicePelsDPI
    PRIMARY_ORDER_CBA,                         // ulPrimaryOrder
    HT_PATSIZE_4x4_M,                          // ulHTPatternSize
    HT_FORMAT_4BPP_IRGB,                       // ulHTOutputFormat
    HT_FLAG_ADDITIVE_PRIMS,                    // flHTFlags

    0,                                         // ulVRefresh
    8,                                         // ulBltAlignment
    0,                                         // ulPanningHorzRes
    0,                                         // ulPanningVertRes

    0,                                         // xPanningAlignment
    0,                                         // yPanningAlignment
    0,                                         // cxHTPat
    0,                                         // cyHTPat
    NULL,                                      // pHTPatA
    NULL,                                      // pHTPatB
    NULL,                                      // pHTPatC
    0,                                         // flShadeBlend
    0,                                         // ulPhysicalPixelCharacteristics
    0                                          // ulPhysicalPixelGamma
};

// Palette for VGA

typedef struct _VGALOGPALETTE
{
    USHORT ident;
    USHORT NumEntries;
    PALETTEENTRY PaletteEntry[16];
} VGALOGPALETTE;

const VGALOGPALETTE VGApalette =
{

    0x400,  // driver version
    16,     // num entries
    {
        { 0x00, 0x00, 0x00, 0x00 }, // 0
        { 0x80, 0x00, 0x00, 0x00 }, // 1
        { 0x00, 0x80, 0x00, 0x00 }, // 2
        { 0x80, 0x80, 0x00, 0x00 }, // 3
        { 0x00, 0x00, 0x80, 0x00 }, // 4
        { 0x80, 0x00, 0x80, 0x00 }, // 5
        { 0x00, 0x80, 0x80, 0x00 }, // 6
        { 0x80, 0x80, 0x80, 0x00 }, // 7
        { 0xc0, 0xc0, 0xc0, 0x00 }, // 8
        { 0xff, 0x00, 0x00, 0x00 }, // 9
        { 0x00, 0xff, 0x00, 0x00 }, // 10
        { 0xff, 0xff, 0x00, 0x00 }, // 11
        { 0x00, 0x00, 0xff, 0x00 }, // 12
        { 0xff, 0x00, 0xff, 0x00 }, // 13
        { 0x00, 0xff, 0xff, 0x00 }, // 14
        { 0xff, 0xff, 0xff, 0x00 } // 15
    }
};

// Devinfo structure passed back to the engine in DrvEnablePDEV

#define SYSTM_LOGFONT {16,7,0,0,700,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,VARIABLE_PITCH | FF_DONTCARE, L"System"}
#define HELVE_LOGFONT {12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_STROKE_PRECIS,PROOF_QUALITY,VARIABLE_PITCH | FF_DONTCARE, L"MS Sans Serif"}
#define COURI_LOGFONT {12,9,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_STROKE_PRECIS,PROOF_QUALITY,FIXED_PITCH | FF_DONTCARE, L"Courier"}

DEVINFO devinfoVGA =
{
    (GCAPS_OPAQUERECT | GCAPS_HORIZSTRIKE | GCAPS_ALTERNATEFILL | GCAPS_MONO_DITHER | GCAPS_COLOR_DITHER |
     GCAPS_WINDINGFILL | GCAPS_DITHERONREALIZE
    ),       // Graphics capabilities

    SYSTM_LOGFONT,  // Default font description
    HELVE_LOGFONT,  // ANSI variable font description
    COURI_LOGFONT,  // ANSI fixed font description
    0,              // Count of device fonts
    BMF_4BPP,       // preferred DIB format
    8,              // Width of color dither
    8,              // Height of color dither
    NULL,           // Default palette to use for this device
    0               // flGraphicsCaps2
};

BOOL APIENTRY
DrvEnableDriver(IN ULONG EngineVersion,
                IN ULONG SizeOfDED,
                OUT PDRVENABLEDATA DriveEnableData)
{
    DPRINT("DrvEnableDriver called...\n");

    vgaPreCalc();

    VGADDI_InitializeOffScreenMem((SCREEN_X * SCREEN_Y) >> 3, 65536 - ((SCREEN_X * SCREEN_Y) >> 3));

    DriveEnableData->pdrvfn = FuncList;
    DriveEnableData->c = sizeof(FuncList) / sizeof(DRVFN);
    DriveEnableData->iDriverVersion = DDI_DRIVER_VERSION_NT4;

    return  TRUE;
}

//    DrvDisableDriver
//  DESCRIPTION:
//    This function is called by the KMGDI at exit.  It should cleanup.
//  ARGUMENTS:
//    NONE
//  RETURNS:
//    NONE

VOID APIENTRY
DrvDisableDriver(VOID)
{
    return;
}

//  -----------------------------------------------  Driver Implementation


//    DrvEnablePDEV
//  DESCRIPTION:
//    This function is called after DrvEnableDriver to get information
//    about the mode that is to be used.  This function just returns
//    information, and should not yet initialize the mode.
//  ARGUMENTS:
//    IN DEVMODEW *  DM            Describes the mode requested
//    IN LPWSTR      LogAddress
//    IN ULONG       PatternCount  number of patterns expected
//    OUT HSURF *    SurfPatterns  array to contain pattern handles
//    IN ULONG       GDIInfoSize   the size of the GDIInfo object passed in
//    OUT ULONG *    GDIInfo       GDI Info object
//    IN ULONG       DevInfoSize   the size of the DevInfo object passed in
//    OUT ULONG *    DevInfo       Device Info object
//    IN LPWSTR      DevDataFile   ignore
//    IN LPWSTR      DeviceName    Device name
//    IN HANDLE      Driver        handle to KM driver
//  RETURNS:
//    DHPDEV  a handle to a DPev object

DHPDEV APIENTRY
DrvEnablePDEV(IN DEVMODEW *DM,
              IN LPWSTR LogAddress,
              IN ULONG PatternCount,
              OUT HSURF *SurfPatterns,
              IN ULONG GDIInfoSize,
              OUT ULONG *GDIInfo,
              IN ULONG DevInfoSize,
              OUT DEVINFO *DevInfo,
              IN HDEV Dev,
              IN LPWSTR DeviceName,
              IN HANDLE Driver)
{
    PPDEV  PDev;

    PDev = EngAllocMem(FL_ZERO_MEMORY, sizeof(PDEV), ALLOC_TAG);
    if (PDev == NULL)
    {
        DPRINT1("EngAllocMem failed for PDEV\n");
        return NULL;
    }
    PDev->KMDriver = Driver;
    DPRINT( "PDev: %x, Driver: %x\n", PDev, PDev->KMDriver );

    gaulCap.ulHorzRes = SCREEN_X;
    gaulCap.ulVertRes = SCREEN_Y;
    if (sizeof(GDIINFO) < GDIInfoSize)
        GDIInfoSize = sizeof(GDIINFO);
    memcpy(GDIInfo, &gaulCap, GDIInfoSize);
    DM->dmBitsPerPel = gaulCap.cBitsPixel * gaulCap.cPlanes;
    DM->dmPelsWidth = gaulCap.ulHorzRes;
    DM->dmPelsHeight = gaulCap.ulVertRes;

    devinfoVGA.hpalDefault = EngCreatePalette(PAL_INDEXED, 16, (ULONG *) VGApalette.PaletteEntry, 0, 0, 0);
    if (sizeof(DEVINFO) < DevInfoSize)
        DevInfoSize = sizeof(DEVINFO);
    memcpy(DevInfo, &devinfoVGA, DevInfoSize);

    return (DHPDEV) PDev;
}


//    DrvCompletePDEV
//  DESCRIPTION
//    Called after initialization of PDEV is complete.  Supplies
//    a reference to the GDI handle for the PDEV.

VOID APIENTRY
DrvCompletePDEV(IN DHPDEV PDev,
                IN HDEV Dev)
{
    ((PPDEV) PDev)->GDIDevHandle = Dev; // Handle to the DC
}


BOOL APIENTRY
DrvAssertMode(IN DHPDEV DPev,
              IN BOOL Enable)
{
    PPDEV ppdev = (PPDEV)DPev;
    ULONG returnedDataLength;

    if (Enable)
    {
        /* Reenable our graphics mode */
        if (!InitPointer(ppdev))
        {
            /* Failed to set pointer */
            return FALSE;
        }

        if (!VGAInitialized)
        {
            if (!InitVGA(ppdev, FALSE))
            {
                /* Failed to initialize the VGA */
                return FALSE;
            }
            VGAInitialized = TRUE;
      }
    }
    else
    {
        /* Go back to last known mode */
        DPRINT( "ppdev: %x, KMDriver: %x", ppdev, ppdev->KMDriver );
        if (EngDeviceIoControl(ppdev->KMDriver, IOCTL_VIDEO_RESET_DEVICE, NULL, 0, NULL, 0, &returnedDataLength))
        {
            /* Failed to go back to mode */
            return FALSE;
        }
        VGAInitialized = FALSE;
    }
    return TRUE;
}


VOID APIENTRY
DrvDisablePDEV(IN DHPDEV PDev)
{
    PPDEV ppdev = (PPDEV)PDev;

    /*  EngDeletePalette(devinfoVGA.hpalDefault); */
    if (ppdev->pjPreallocSSBBuffer)
        EngFreeMem(ppdev->pjPreallocSSBBuffer);

    if (ppdev->pucDIB4ToVGAConvBuffer)
        EngFreeMem(ppdev->pucDIB4ToVGAConvBuffer);

    DPRINT("Freeing PDEV\n");
    EngFreeMem(PDev);
}


VOID APIENTRY
DrvDisableSurface(IN DHPDEV PDev)
{
    PPDEV ppdev = (PPDEV)PDev;
    PDEVSURF pdsurf = ppdev->AssociatedSurf;

    DPRINT("KMDriver: %x\n", ppdev->KMDriver);
    DeinitVGA(ppdev);
    /* EngFreeMem(pdsurf->BankSelectInfo); */

    if (pdsurf->BankInfo != NULL)
        EngFreeMem(pdsurf->BankInfo);
    if (pdsurf->BankInfo2RW != NULL)
        EngFreeMem(pdsurf->BankInfo2RW);
    if (pdsurf->BankBufferPlane0 != NULL)
        EngFreeMem(pdsurf->BankBufferPlane0);
    if (ppdev->pPointerAttributes != NULL)
        EngFreeMem(ppdev->pPointerAttributes);

    /* free any pending saved screen bit blocks */
#if 0
    pSSB = pdsurf->ssbList;
    while (pSSB != (PSAVED_SCREEN_BITS) NULL)
    {
        /* Point to the next saved screen bits block */
        pSSBNext = (PSAVED_SCREEN_BITS) pSSB->pvNextSSB;

        /* Free the current block */
        EngFreeMem(pSSB);
        pSSB = pSSBNext;
    }
#endif
    EngDeleteSurface((HSURF) ppdev->SurfHandle);
    /* EngFreeMem(pdsurf); */ /* free the surface */
}


static VOID
InitSavedBits(IN PPDEV ppdev)
{
    if (!(ppdev->fl & DRIVER_OFFSCREEN_REFRESHED))
        return;

    /* set up rect to right of visible screen */
    ppdev->SavedBitsRight.left   = ppdev->sizeSurf.cx;
    ppdev->SavedBitsRight.top    = 0;
    ppdev->SavedBitsRight.right  = ppdev->sizeMem.cx - PLANAR_PELS_PER_CPU_ADDRESS;
    ppdev->SavedBitsRight.bottom = ppdev->sizeSurf.cy;

    if ((ppdev->SavedBitsRight.right <= ppdev->SavedBitsRight.left) ||
        (ppdev->SavedBitsRight.bottom <= ppdev->SavedBitsRight.top))
    {
        ppdev->SavedBitsRight.left   = 0;
        ppdev->SavedBitsRight.top    = 0;
        ppdev->SavedBitsRight.right  = 0;
        ppdev->SavedBitsRight.bottom = 0;
    }

    /* set up rect below visible screen */
    ppdev->SavedBitsBottom.left   = 0;
    ppdev->SavedBitsBottom.top    = ppdev->sizeSurf.cy;
    ppdev->SavedBitsBottom.right  = ppdev->sizeMem.cx - PLANAR_PELS_PER_CPU_ADDRESS;
    ppdev->SavedBitsBottom.bottom = ppdev->sizeMem.cy - ppdev->NumScansUsedByPointer;

    if ((ppdev->SavedBitsBottom.right <= ppdev->SavedBitsBottom.left) ||
        (ppdev->SavedBitsBottom.bottom <= ppdev->SavedBitsBottom.top))
    {
        ppdev->SavedBitsBottom.left   = 0;
        ppdev->SavedBitsBottom.top    = 0;
        ppdev->SavedBitsBottom.right  = 0;
        ppdev->SavedBitsBottom.bottom = 0;
    }

    ppdev->BitsSaved = FALSE;
}


HSURF APIENTRY
DrvEnableSurface(IN DHPDEV PDev)
{
    PPDEV ppdev = (PPDEV)PDev;
    PDEVSURF pdsurf;
    DHSURF dhsurf;
    HSURF hsurf;

    DPRINT("DrvEnableSurface() called\n");

    /* Initialize the VGA */
    if (!VGAInitialized)
    {
        if (!InitVGA(ppdev, TRUE))
            goto error_done;
        VGAInitialized = TRUE;
    }

    /* dhsurf is of type DEVSURF, which is the drivers specialized surface type */
    dhsurf = (DHSURF)EngAllocMem(0, sizeof(DEVSURF), ALLOC_TAG);
    if (dhsurf == (DHSURF) 0)
        goto error_done;

    pdsurf = (PDEVSURF) dhsurf;
    pdsurf->ident         = DEVSURF_IDENT;
    pdsurf->flSurf        = 0;
    pdsurf->Format        = BMF_PHYSDEVICE;
    pdsurf->jReserved1    = 0;
    pdsurf->jReserved2    = 0;
    pdsurf->ppdev         = ppdev;
    pdsurf->sizeSurf.cx   = ppdev->sizeSurf.cx;
    pdsurf->sizeSurf.cy   = ppdev->sizeSurf.cy;
    pdsurf->NextPlane     = 0;
    pdsurf->Scan0         = ppdev->fbScreen;
    pdsurf->BitmapStart   = ppdev->fbScreen;
    pdsurf->StartBmp      = ppdev->fbScreen;
    pdsurf->BankInfo      = NULL;
    pdsurf->BankInfo2RW   = NULL;
    pdsurf->BankBufferPlane0 = NULL;

/*    pdsurf->Conv          = &ConvertBuffer[0]; */

    if (!InitPointer(ppdev))
    {
        DPRINT1("DrvEnablePDEV failed bInitPointer\n");
        goto error_clean;
     }

/*    if (!SetUpBanking(pdsurf, ppdev))
    {
        DPRINT1("DrvEnablePDEV failed SetUpBanking\n");
        goto error_clean;
    } BANKING CODE UNIMPLEMENTED */

    if ((hsurf = EngCreateDeviceSurface(dhsurf, ppdev->sizeSurf, BMF_4BPP)) ==
        (HSURF)0)
    {
        /* Call to EngCreateDeviceSurface failed */
        DPRINT("EngCreateDeviceSurface call failed\n");
        goto error_clean;
    }

    InitSavedBits(ppdev);

    if (EngAssociateSurface(hsurf, ppdev->GDIDevHandle, HOOK_BITBLT | HOOK_PAINT | HOOK_LINETO | HOOK_COPYBITS |
        HOOK_TRANSPARENTBLT))
    {
        DPRINT("Successfully associated surface\n");
        ppdev->SurfHandle = hsurf;
        ppdev->AssociatedSurf = pdsurf;

       /* Set up an empty saved screen block list */
       pdsurf->ssbList = NULL;

      return hsurf;
    }
    DPRINT("EngAssociateSurface() failed\n");
    EngDeleteSurface(hsurf);

error_clean:
    EngFreeMem(dhsurf);

error_done:
    return (HSURF)0;
}


ULONG APIENTRY
DrvGetModes(IN HANDLE Driver,
            IN ULONG DataSize,
            OUT PDEVMODEW DM)
{
    DWORD NumModes;
    DWORD ModeSize;
    DWORD OutputSize;
    DWORD OutputModes = DataSize / (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);
    PVIDEO_MODE_INFORMATION VideoModeInformation, VideoTemp;

    NumModes = getAvailableModes(Driver,
                                 (PVIDEO_MODE_INFORMATION *) &VideoModeInformation,
                                 &ModeSize);

    if (NumModes == 0)
        return 0;

    if (DM == NULL)
    {
        OutputSize = NumModes * (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);
    }
    else
    {
        OutputSize=0;
        VideoTemp = VideoModeInformation;

        do
        {
            if (VideoTemp->Length != 0)
            {
                if (OutputModes == 0)
                    break;

                memset(DM, 0, sizeof(DEVMODEW));
                memcpy(DM->dmDeviceName, DLL_NAME, sizeof(DLL_NAME));

                DM->dmSpecVersion      = DM_SPECVERSION;
                DM->dmDriverVersion    = DM_SPECVERSION;
                DM->dmSize             = sizeof(DEVMODEW);
                DM->dmDriverExtra      = DRIVER_EXTRA_SIZE;
                DM->dmBitsPerPel       = VideoTemp->NumberOfPlanes *
                                         VideoTemp->BitsPerPlane;
                DM->dmPelsWidth        = VideoTemp->VisScreenWidth;
                DM->dmPelsHeight       = VideoTemp->VisScreenHeight;
                DM->dmDisplayFrequency = VideoTemp->Frequency;
                DM->dmDisplayFlags     = 0;

                DM->dmFields           = DM_BITSPERPEL       |
                                         DM_PELSWIDTH        |
                                         DM_PELSHEIGHT       |
                                         DM_DISPLAYFREQUENCY |
                                         DM_DISPLAYFLAGS     ;

                /* next DEVMODE entry */
                OutputModes--;

                DM = (PDEVMODEW) ( ((ULONG_PTR)DM) + sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);

                OutputSize += (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);
            }

            VideoTemp = (PVIDEO_MODE_INFORMATION)(((PUCHAR)VideoTemp) + ModeSize);

        } while (--NumModes);
    }
    return OutputSize;
}

ULONG DbgPrint(PCCH Format,...)
{
    va_list ap;
    va_start(ap, Format);
    EngDebugPrint("VGADDI", (PCHAR)Format, ap);
    va_end(ap);
    return 0;
}

/* EOF */
