/*
 * entry.c
 *
 * $Revision: 1.20 $
 * $Author: dwelch $
 * $Date: 2002/09/01 20:39:53 $
 *
 */

#include "gdiinfo.h"
#include "../vgavideo/vgavideo.h"
#include <debug.h>

#define  DBG_PREFIX  "VGADDI: "

static BOOL VGAInitialized = FALSE;

DRVFN FuncList[] =
{
  /*  Required Display driver fuctions  */
  {INDEX_DrvAssertMode, (PFN) DrvAssertMode},
  {INDEX_DrvCompletePDEV, (PFN) DrvCompletePDEV},
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
  /*  Optional Display driver functions  */
  {INDEX_, (PFN) },
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


BOOL STDCALL
DrvEnableDriver(IN ULONG EngineVersion,
		IN ULONG SizeOfDED,
		OUT PDRVENABLEDATA DriveEnableData)
{
  EngDebugPrint("VGADDI", "DrvEnableDriver called...\n", 0);

  vgaPreCalc();

  // FIXME: Use Vidport to map the memory properly
  vidmem = (char *)(0xd0000000 + 0xa0000);

  DriveEnableData->pdrvfn = FuncList;
  DriveEnableData->c = sizeof(FuncList) / sizeof(DRVFN);
  DriveEnableData->iDriverVersion = DDI_DRIVER_VERSION;

  return  TRUE;
}

//    DrvDisableDriver
//  DESCRIPTION:
//    This function is called by the KMGDI at exit.  It should cleanup.
//  ARGUMENTS:
//    NONE
//  RETURNS:
//    NONE

VOID STDCALL
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
//    IN ULONG       CapsSize      the size of the DevCaps object passed in
//    OUT ULONG *    DevCaps       Device Capabilities object
//    IN ULONG       DevInfoSize   the size of the DevInfo object passed in
//    OUT DEVINFO *  DI            Device Info object
//    IN LPWSTR      DevDataFile   ignore
//    IN LPWSTR      DeviceName    Device name
//    IN HANDLE      Driver        handle to KM driver
//  RETURNS:
//    DHPDEV  a handle to a DPev object

DHPDEV STDCALL
DrvEnablePDEV(IN DEVMODEW *DM,
	      IN LPWSTR LogAddress,
	      IN ULONG PatternCount,
	      OUT HSURF *SurfPatterns,
	      IN ULONG CapsSize,
	      OUT ULONG *DevCaps,
	      IN ULONG DevInfoSize,
	      OUT DEVINFO *DI,
	      IN LPWSTR DevDataFile,
	      IN LPWSTR DeviceName,
	      IN HANDLE Driver)
{
  PPDEV  PDev;

  PDev = EngAllocMem(FL_ZERO_MEMORY, sizeof(PDEV), ALLOC_TAG);
  if (PDev == NULL)
    {
      EngDebugPrint(DBG_PREFIX, "EngAllocMem failed for PDEV\n", 0);
      return(NULL);
    }
  PDev->KMDriver = Driver;
  DPRINT( "PDev: %x, Driver: %x\n", PDev, PDev->KMDriver );
  PDev->xyCursor.x = 320;
  PDev->xyCursor.y = 240;
  PDev->ptlExtent.x = 0;
  PDev->ptlExtent.y = 0;
  PDev->cExtent = 0;
  PDev->flCursor = CURSOR_DOWN;
  // FIXME: fill out DevCaps
  // FIXME: full out DevInfo

  devinfoVGA.hpalDefault = EngCreatePalette(PAL_INDEXED, 16, (PULONG *)VGApalette.PaletteEntry, 0, 0, 0);
DPRINT("Palette from Driver: %u\n", devinfoVGA.hpalDefault);
  *DI = devinfoVGA;
DPRINT("Palette from Driver 2: %u and DI is %08x\n", DI->hpalDefault, DI);

  return(PDev);
}


//    DrvCompletePDEV
//  DESCRIPTION
//    Called after initialization of PDEV is complete.  Supplies
//    a reference to the GDI handle for the PDEV.

VOID STDCALL
DrvCompletePDEV(IN DHPDEV PDev,
		IN HDEV Dev)
{
  ((PPDEV) PDev)->GDIDevHandle = Dev; // Handle to the DC
}


BOOL STDCALL
DrvAssertMode(IN DHPDEV DPev,
	      IN BOOL Enable)
{
  PPDEV ppdev = (PPDEV)DPev;
  ULONG returnedDataLength;

  if(Enable==TRUE)
  {
    // Reenable our graphics mode

    if (!InitPointer(ppdev))
    {
      // Failed to set pointer
      return FALSE;
    }

    if (!VGAInitialized)
      {
	if (!InitVGA(ppdev, FALSE))
	  {
	    // Failed to initialize the VGA
	    return FALSE;
	  }
	VGAInitialized = TRUE;
      }
  } else {
    // Go back to last known mode
    DPRINT( "ppdev: %x, KMDriver: %x", ppdev, ppdev->KMDriver );
    if (EngDeviceIoControl(ppdev->KMDriver, IOCTL_VIDEO_RESET_DEVICE, NULL, 0, NULL, 0, &returnedDataLength))
    {
      // Failed to go back to mode
      return FALSE;
    }
    VGAInitialized = FALSE;
  }

}


VOID STDCALL
DrvDisablePDEV(IN DHPDEV PDev)
{
  PPDEV ppdev = (PPDEV)PDev;

  // EngDeletePalette(devinfoVGA.hpalDefault);
  if (ppdev->pjPreallocSSBBuffer != NULL)
  {
    EngFreeMem(ppdev->pjPreallocSSBBuffer);
  }

  if (ppdev->pucDIB4ToVGAConvBuffer != NULL)
  {
    EngFreeMem(ppdev->pucDIB4ToVGAConvBuffer);
  }
  DPRINT( "Freeing PDEV\n" );
  EngFreeMem(PDev);
}


VOID STDCALL
DrvDisableSurface(IN DHPDEV PDev)
{
  PPDEV ppdev = (PPDEV)PDev;
  PDEVSURF pdsurf = ppdev->AssociatedSurf;
  PSAVED_SCREEN_BITS pSSB, pSSBNext;
  CHECKPOINT;
  DPRINT( "KMDriver: %x\n", ppdev->KMDriver );
  //  EngFreeMem(pdsurf->BankSelectInfo);
  CHECKPOINT;
  if (pdsurf->BankInfo != NULL) {
    EngFreeMem(pdsurf->BankInfo);
  }
  CHECKPOINT;
  if (pdsurf->BankInfo2RW != NULL) {
    EngFreeMem(pdsurf->BankInfo2RW);
  }
  CHECKPOINT;
  if (pdsurf->BankBufferPlane0 != NULL) {
    EngFreeMem(pdsurf->BankBufferPlane0);
  }
  CHECKPOINT;
  if (ppdev->pPointerAttributes != NULL) {
    EngFreeMem(ppdev->pPointerAttributes);
  }
  CHECKPOINT;
  // free any pending saved screen bit blocks
  pSSB = pdsurf->ssbList;
  while (pSSB != (PSAVED_SCREEN_BITS) NULL) {

    // Point to the next saved screen bits block
    pSSBNext = (PSAVED_SCREEN_BITS) pSSB->pvNextSSB;

    // Free the current block
    EngFreeMem(pSSB);
    pSSB = pSSBNext;
    }
  EngDeleteSurface((HSURF) ppdev->SurfHandle);
  //  EngFreeMem(pdsurf); // free the surface
}


static VOID
InitSavedBits(PPDEV ppdev)
{
  if (!(ppdev->fl & DRIVER_OFFSCREEN_REFRESHED))
  {
    return;
  }

  // set up rect to right of visible screen
  ppdev->SavedBitsRight.left   = ppdev->sizeSurf.cx;
  ppdev->SavedBitsRight.top    = 0;
  ppdev->SavedBitsRight.right  = ppdev->sizeMem.cx-PLANAR_PELS_PER_CPU_ADDRESS;
  ppdev->SavedBitsRight.bottom = ppdev->sizeSurf.cy;

  if ((ppdev->SavedBitsRight.right <= ppdev->SavedBitsRight.left) ||
      (ppdev->SavedBitsRight.bottom <= ppdev->SavedBitsRight.top))
  {
    ppdev->SavedBitsRight.left   = 0;
    ppdev->SavedBitsRight.top    = 0;
    ppdev->SavedBitsRight.right  = 0;
    ppdev->SavedBitsRight.bottom = 0;
  }

  // set up rect below visible screen
  ppdev->SavedBitsBottom.left   = 0;
  ppdev->SavedBitsBottom.top    = ppdev->sizeSurf.cy;
  ppdev->SavedBitsBottom.right  = ppdev->sizeMem.cx-PLANAR_PELS_PER_CPU_ADDRESS;
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

  return;
}


HSURF STDCALL
DrvEnableSurface(IN DHPDEV PDev)
{
  PPDEV ppdev = (PPDEV)PDev;
  PDEVSURF pdsurf;
  DHSURF dhsurf;
  HSURF hsurf;

  DPRINT1("DrvEnableSurface() called\n");

  // Initialize the VGA
  if (!VGAInitialized)
    {
      if (!InitVGA(ppdev, TRUE))
	{
	  goto error_done;
	}
      VGAInitialized = TRUE;
    }
CHECKPOINT1;

  // dhsurf is of type DEVSURF, which is the drivers specialized surface type
  dhsurf = (DHSURF)EngAllocMem(0, sizeof(DEVSURF), ALLOC_TAG);
  if (dhsurf == (DHSURF) 0)
  {
    goto error_done;
  }

  pdsurf = (PDEVSURF) dhsurf;
  pdsurf->ident         = DEVSURF_IDENT;
  pdsurf->flSurf        = 0;
  pdsurf->Format       = BMF_PHYSDEVICE;
  pdsurf->jReserved1    = 0;
  pdsurf->jReserved2    = 0;
  pdsurf->ppdev         = ppdev;
  pdsurf->sizeSurf.cx   = ppdev->sizeSurf.cx;
  pdsurf->sizeSurf.cy   = ppdev->sizeSurf.cy;
  pdsurf->NextPlane    = 0;
  pdsurf->Scan0       = ppdev->fbScreen;
  pdsurf->BitmapStart = ppdev->fbScreen;
  pdsurf->StartBmp      = ppdev->fbScreen;

/*  pdsurf->Conv          = &ConvertBuffer[0]; */

  if (!InitPointer(ppdev)) {
      DbgPrint("DrvEnablePDEV failed bInitPointer\n");
      goto error_clean;
   }

/* if (!SetUpBanking(pdsurf, ppdev)) {
      DISPDBG((0, "DrvEnablePDEV failed SetUpBanking\n"));
      goto error_clean;
   } BANKING CODE UNIMPLEMENTED */

  if ((hsurf = EngCreateDeviceSurface(dhsurf, ppdev->sizeSurf, BMF_4BPP)) ==
      (HSURF)0)
  {
    // Call to EngCreateDeviceSurface failed
    EngDebugPrint("VGADDI:", "EngCreateDeviceSurface call failed\n", 0);
    goto error_clean;
  }

  InitSavedBits(ppdev);

  if (EngAssociateSurface(hsurf, ppdev->GDIDevHandle, HOOK_BITBLT | HOOK_PAINT | HOOK_LINETO | HOOK_COPYBITS |
    HOOK_TRANSPARENTBLT))
  {
    EngDebugPrint("VGADDI:", "Successfully associated surface\n", 0);
    ppdev->SurfHandle = hsurf;
    ppdev->AssociatedSurf = pdsurf;

    // Set up an empty saved screen block list
    pdsurf->ssbList = NULL;

    return(hsurf);
  }
  DPRINT( "EngAssociateSurface() failed\n" );
  EngDeleteSurface(hsurf);

error_clean:
   EngFreeMem(dhsurf);

error_done:
   return((HSURF)0);
}


ULONG STDCALL
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
  {
    return 0;
  }

  if (DM == NULL)
  {
    OutputSize = NumModes * (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);
  } else {

    OutputSize=0;
    VideoTemp = VideoModeInformation;

    do
    {
      if (VideoTemp->Length != 0)
      {
        if (OutputModes == 0)
        {
          break;
        }

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

        // next DEVMODE entry
        OutputModes--;

        DM = (PDEVMODEW) ( ((ULONG)DM) + sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);

        OutputSize += (sizeof(DEVMODEW) + DRIVER_EXTRA_SIZE);
      }

      VideoTemp = (PVIDEO_MODE_INFORMATION)(((PUCHAR)VideoTemp) + ModeSize);

    } while (--NumModes);
  }
}

/* EOF */
