/*
 * entry.c
 *
 * $Revision: 1.4 $
 * $Author: jfilby $
 * $Date: 2000/04/03 19:55:32 $
 *
 */

#include "gdiinfo.h"
#include "..\vgavideo\vgavideo.h"
#include <internal/debug.h>

#define  DBG_PREFIX  "VGADDI: "

VOID VGADDIAssertMode(IN DHPDEV  DPev, 
                      IN BOOL  Enable); 
VOID VGADDICompletePDEV(IN DHPDEV  PDev,
                        IN HDEV  Dev);
VOID VGADDIDisablePDEV(IN DHPDEV PDev); 
VOID VGADDIDisableSurface(IN DHPDEV PDev); 
DHPDEV VGADDIEnablePDEV(IN DEVMODEW  *DM,
                        IN LPWSTR  LogAddress,
                        IN ULONG  PatternCount,
                        OUT HSURF  *SurfPatterns,
                        IN ULONG  CapsSize,
                        OUT ULONG  *DevCaps,
                        IN ULONG  DevInfoSize,
                        OUT DEVINFO  *DI,
                        IN LPWSTR  DevDataFile,
                        IN LPWSTR  DeviceName,
                        IN HANDLE  Driver);
HSURF VGADDIEnableSurface(IN DHPDEV  PDev);
ULONG VGADDIGetModes(IN HANDLE Driver,
                     IN ULONG DataSize,
                     OUT PDEVMODEW DM);
BOOL VGADDILineTo(SURFOBJ *Surface, CLIPOBJ *Clip, BRUSHOBJ *Brush,
                  LONG x1, LONG y1, LONG x2, LONG y2,
                  RECTL *RectBounds, MIX mix);
BOOL VGADDIPaint(IN SURFOBJ *Surface, IN CLIPOBJ *ClipRegion,
                 IN BRUSHOBJ *Brush,  IN POINTL *BrushOrigin,
                 IN MIX  Mix);
BOOL VGADDIBitBlt(SURFOBJ *Dest, SURFOBJ *Source, SURFOBJ *Mask,
                  CLIPOBJ *Clip, XLATEOBJ *ColorTranslation,
                  RECTL *DestRect, POINTL *SourcePoint, POINTL *MaskPoint,
                  BRUSHOBJ *Brush, POINTL *BrushPoint, ROP4 rop4);

DRVFN FuncList[] =
{
  /*  Required Display driver fuctions  */
  {INDEX_DrvAssertMode, (PFN) VGADDIAssertMode},
  {INDEX_DrvCompletePDEV, (PFN) VGADDICompletePDEV},
  {INDEX_DrvDisablePDEV, (PFN) VGADDIDisablePDEV},
  {INDEX_DrvDisableSurface, (PFN) VGADDIDisableSurface},
  {INDEX_DrvEnablePDEV, (PFN) VGADDIEnablePDEV},
  {INDEX_DrvEnableSurface, (PFN) VGADDIEnableSurface},
  {INDEX_DrvGetModes, (PFN) VGADDIGetModes},
  {INDEX_DrvLineTo, (PFN) VGADDILineTo},
  {INDEX_DrvPaint, (PFN) VGADDIPaint},
  {INDEX_DrvBitBlt, (PFN) VGADDIBitBlt},

#if 0
  /*  Optional Display driver functions  */
  {INDEX_, (PFN) },
  {INDEX_DrvCopyBits, (PFN) VGADDICopyBits},
  {INDEX_DescribePixelFormat, (PFN) VGADDIDescribePixelFormat},
  {INDEX_DrvDitherColor, (PFN) VGADDIDitherColor},
  {INDEX_DrvFillPath, (PFN) VGADDIFillPath},
  {INDEX_DrvGetTrueTypeFile, (PFN) VGADDIGetTrueTypeFile},
  {INDEX_DrvLoadFontFile, (PFN) VGADDILoadFontFile},
  {INDEX_DrvMovePointer, (PFN) VGADDIMovePointer},
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
  {INDEX_DrvSetPointerShape, (PFN) VGADDISetPointerShape},
  {INDEX_DrvStretchBlt, (PFN) VGADDIStretchBlt},
  {INDEX_DrvStrokePath, (PFN) VGADDIStrokePath},
  {INDEX_DrvSwapBuffers, (PFN) VGADDISwapBuffers},
  {INDEX_DrvTextOut, (PFN) VGADDITextOut},
  {INDEX_DrvUnloadFontFile, (PFN) VGADDIUnloadFontFile},
#endif
};


BOOL
DrvEnableDriver(IN ULONG  EngineVersion, 
                IN ULONG  SizeOfDED, 
                OUT PDRVENABLEDATA  DriveEnableData)
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

VOID DrvDisableDriver(VOID)
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

DHPDEV VGADDIEnablePDEV(IN DEVMODEW  *DM,
                        IN LPWSTR  LogAddress,
                        IN ULONG  PatternCount,
                        OUT HSURF  *SurfPatterns,
                        IN ULONG  CapsSize,
                        OUT ULONG  *DevCaps,
                        IN ULONG  DevInfoSize,
                        OUT DEVINFO  *DI,
                        IN LPWSTR  DevDataFile,
                        IN LPWSTR  DeviceName,
                        IN HANDLE  Driver)
{
  PPDEV  PDev;

  PDev = EngAllocMem(FL_ZERO_MEMORY, sizeof(PDEV), ALLOC_TAG);
  if (PDev == NULL)
    {
      EngDebugPrint(DBG_PREFIX, "EngAllocMem failed for PDEV\n", 0);
      
      return  NULL;
    }
  PDev->KMDriver = Driver;
  PDev->xyCursor.x = 320;
  PDev->xyCursor.y = 240;
  PDev->ptlExtent.x = 0;
  PDev->ptlExtent.y = 0;
  PDev->cExtent = 0;
  PDev->flCursor = CURSOR_DOWN;
  // FIXME: fill out DevCaps
  // FIXME: full out DevInfo

  devinfoVGA.hpalDefault = EngCreatePalette(PAL_INDEXED, 16,
                           (PULONG)(VGApalette.PaletteEntry), 0, 0, 0);

  *DI = devinfoVGA;

  return  PDev;
}

//    DrvEnablePDEV
//  DESCRIPTION
//    Called after initialization of PDEV is complete.  Supplies
//    a reference to the GDI handle for the PDEV.

VOID VGADDICompletePDEV(IN DHPDEV  PDev,
                        IN HDEV  Dev)
{
  ((PPDEV) PDev)->GDIDevHandle = Dev;
}


VOID VGADDIAssertMode(IN DHPDEV  DPev, 
                      IN BOOL  Enable)
{
  PPDEV ppdev = (PPDEV)DPev;
  ULONG returnedDataLength;

  if(Enable==TRUE)
  {
    // Reenable our graphics mode

/*   if (!InitPointer(ppdev))
     {
         // Failed to set pointer
         return FALSE;
     } POINTER CODE CURRENTLY UNIMPLEMENTED... */

     if (!InitVGA(ppdev, FALSE))
     {
        // Failed to initialize the VGA
        return FALSE;
     }

  } else {
    // Go back to last known mode

    if (EngDeviceIoControl(ppdev->KMDriver,
                           IOCTL_VIDEO_RESET_DEVICE,
                           NULL,
                           0,
                           NULL,
                           0,
                           &returnedDataLength))
    {
      // Failed to go back to mode
      return FALSE;
    }

  }
}

VOID VGADDIDisablePDEV(IN DHPDEV PDev)
{
  PPDEV ppdev = (PPDEV)PDev;

  EngDeletePalette(devinfoVGA.hpalDefault);

  if (ppdev->pjPreallocSSBBuffer != NULL)
  {
    EngFreeMem(ppdev->pjPreallocSSBBuffer);
  }

  if (ppdev->pucDIB4ToVGAConvBuffer != NULL)
  {
    EngFreeMem(ppdev->pucDIB4ToVGAConvBuffer);
  }

  EngFreeMem(PDev);
}

VOID VGADDIDisableSurface(IN DHPDEV PDev)
{
  PPDEV ppdev = (PPDEV)PDev;
  PDEVSURF pdsurf = ppdev->AssociatedSurf;
  PSAVED_SCREEN_BITS pSSB, pSSBNext;

  EngFreeMem(pdsurf->BankSelectInfo);

  if (pdsurf->BankInfo != NULL) {
    EngFreeMem(pdsurf->BankInfo);
  }
  if (pdsurf->BankInfo2RW != NULL) {
    EngFreeMem(pdsurf->BankInfo2RW);
  }
  if (pdsurf->BankBufferPlane0 != NULL) {
    EngFreeMem(pdsurf->BankBufferPlane0);
  }
  if (ppdev->PointerAttributes != NULL) {
    EngFreeMem(ppdev->PointerAttributes);
  }

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
  EngFreeMem(pdsurf); // free the surface
}

VOID InitSavedBits(PPDEV ppdev)
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

HSURF VGADDIEnableSurface(IN DHPDEV  PDev)
{
  PPDEV ppdev = (PPDEV)PDev;
  PDEVSURF pdsurf;
  DHSURF dhsurf;
  HSURF hsurf;

  DbgPrint("DrvEnableSurface..\n");

  // Initialize the VGA
  if (!InitVGA(ppdev, TRUE))
  {
    goto error_done;
  }

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

/* if (!bInitPointer(ppdev)) {
      DISPDBG((0, "DrvEnablePDEV failed bInitPointer\n"));
      goto error_clean;
   } POINTER CODE UNIMPLEMENTED */

/* if (!SetUpBanking(pdsurf, ppdev)) {
      DISPDBG((0, "DrvEnablePDEV failed SetUpBanking\n"));
      goto error_clean;
   } BANKING CODE UNIMPLEMENTED */

  DbgPrint("Calling EngCreateDeviceSurface..\n");

  if ((hsurf = EngCreateDeviceSurface(dhsurf, ppdev->sizeSurf, BMF_4BPP)) ==
      (HSURF)0)
  {
    // Call to EngCreateDeviceSurface failed
    EngDebugPrint("VGADDI:", "EngCreateDeviceSurface call failed\n", 0);
    goto error_clean;
  }
DbgPrint("init saved bits.. ");
  InitSavedBits(ppdev);
DbgPrint("successful\n");

  if (EngAssociateSurface(hsurf, ppdev->GDIDevHandle, HOOK_BITBLT | HOOK_PAINT | HOOK_LINETO))
  {
    EngDebugPrint("VGADDI:", "Successfully associated surface\n", 0);
    ppdev->SurfHandle = hsurf;
    ppdev->AssociatedSurf = pdsurf;

    // Set up an empty saved screen block list
    pdsurf->ssbList = NULL;

    return(hsurf);
  }

  EngDeleteSurface(hsurf);

error_clean:
   EngFreeMem(dhsurf);

error_done:
   return((HSURF)0);
}

ULONG VGADDIGetModes(IN HANDLE Driver,
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
