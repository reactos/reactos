/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

HCOLORSPACE hStockColorSpace = NULL;


HCOLORSPACE
FASTCALL
IntGdiCreateColorSpace(
    PLOGCOLORSPACEEXW pLogColorSpace)
{
  PCOLORSPACE pCS;
  HCOLORSPACE hCS;

  pCS = COLORSPACEOBJ_AllocCSWithHandle();
  hCS = pCS->BaseObject.hHmgr;

  pCS->lcsColorSpace = pLogColorSpace->lcsColorSpace;
  pCS->dwFlags = pLogColorSpace->dwFlags;

  COLORSPACEOBJ_UnlockCS(pCS);
  return hCS;
}

BOOL
FASTCALL
IntGdiDeleteColorSpace(
    HCOLORSPACE hColorSpace)
{
  BOOL Ret = FALSE;

  if ( hColorSpace != hStockColorSpace )
  {
     Ret = COLORSPACEOBJ_FreeCSByHandle(hColorSpace);
     if ( !Ret ) SetLastWin32Error(ERROR_INVALID_PARAMETER);
  }
  return Ret;
}

HANDLE
APIENTRY
NtGdiCreateColorSpace(
    IN PLOGCOLORSPACEEXW pLogColorSpace)
{
  LOGCOLORSPACEEXW Safelcs;
  NTSTATUS Status = STATUS_SUCCESS;

  _SEH2_TRY
  {
     ProbeForRead( pLogColorSpace,
                    sizeof(LOGCOLORSPACEEXW),
                    1);
     RtlCopyMemory(&Safelcs, pLogColorSpace, sizeof(LOGCOLORSPACEEXW));
  }
  _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
  {
     Status = _SEH2_GetExceptionCode();
  }
  _SEH2_END;

  if (!NT_SUCCESS(Status))
  {
     SetLastNtError(Status);
     return NULL;
  }
  return IntGdiCreateColorSpace(&Safelcs);
}

BOOL
APIENTRY
NtGdiDeleteColorSpace(
    IN HANDLE hColorSpace)
{
  return IntGdiDeleteColorSpace(hColorSpace);
}

BOOL
FASTCALL
IntGetDeviceGammaRamp(HDEV hPDev, PGAMMARAMP Ramp)
{
  PGDIDEVICE pGDev = (PGDIDEVICE) hPDev;
  int i;

  if (!(pGDev->flFlags & PDEV_DISPLAY )) return FALSE;

  if ((pGDev->DevInfo.iDitherFormat == BMF_8BPP)  ||
      (pGDev->DevInfo.iDitherFormat == BMF_16BPP) ||
      (pGDev->DevInfo.iDitherFormat == BMF_24BPP) ||
      (pGDev->DevInfo.iDitherFormat == BMF_32BPP))
  {
     if (pGDev->flFlags & PDEV_GAMMARAMP_TABLE)
        RtlCopyMemory( Ramp,
                       pGDev->pvGammaRamp,
                       sizeof(GAMMARAMP));
     else
     // Generate the 256-colors array
        for(i=0; i<256; i++ )
        {
          int NewValue = i * 256;

          Ramp->Red[i] = Ramp->Green[i] = Ramp->Blue[i] = ((WORD)NewValue);
        }
     return TRUE;
  }
  else
     return FALSE;
}

BOOL
APIENTRY
NtGdiGetDeviceGammaRamp(HDC  hDC,
                             LPVOID  Ramp)
{
  BOOL Ret;
  PDC dc;
  NTSTATUS Status = STATUS_SUCCESS;
  PGAMMARAMP SafeRamp;

  if (!Ramp) return FALSE;

  dc = DC_LockDc(hDC);
  if (!dc)
  {
     SetLastWin32Error(ERROR_INVALID_HANDLE);
     return FALSE;
  }

  SafeRamp = ExAllocatePool(PagedPool, sizeof(GAMMARAMP));
  if (!SafeRamp)
  {
      DC_UnlockDc(dc);
      SetLastWin32Error(STATUS_NO_MEMORY);
      return FALSE;
  }

  Ret = IntGetDeviceGammaRamp((HDEV)dc->ppdev, SafeRamp);

  if (!Ret) return Ret;

  _SEH2_TRY
  {
     ProbeForWrite( Ramp,
                    sizeof(PVOID),
                    1);
     RtlCopyMemory( Ramp,
                    SafeRamp,
                    sizeof(GAMMARAMP));
  }
  _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
  {
     Status = _SEH2_GetExceptionCode();
  }
  _SEH2_END;

  DC_UnlockDc(dc);
  ExFreePool(SafeRamp);

  if (!NT_SUCCESS(Status))
  {
     SetLastNtError(Status);
     return FALSE;
  }
  return Ret;
}

BOOL
APIENTRY
NtGdiSetColorSpace(IN HDC hdc,
                   IN HCOLORSPACE hColorSpace)
{
  PDC pDC;
  PDC_ATTR pDc_Attr;
  PCOLORSPACE pCS;

  pDC = DC_LockDc(hdc);
  if (!pDC)
  {
     SetLastWin32Error(ERROR_INVALID_HANDLE);
     return FALSE;
  }
  pDc_Attr = pDC->pDc_Attr;
  if(!pDc_Attr) pDc_Attr = &pDC->Dc_Attr;

  if (pDc_Attr->hColorSpace == hColorSpace)
  {
     DC_UnlockDc(pDC);
     return TRUE; 
  }
  
  pCS = COLORSPACEOBJ_LockCS(hColorSpace);
  if (!pCS)
  {
     SetLastWin32Error(ERROR_INVALID_HANDLE);
     return FALSE;
  }
  
  if (pDC->DcLevel.pColorSpace)
  {
     GDIOBJ_ShareUnlockObjByPtr((POBJ) pDC->DcLevel.pColorSpace);
  }

  pDC->DcLevel.pColorSpace = pCS;
  pDc_Attr->hColorSpace = hColorSpace;

  COLORSPACEOBJ_UnlockCS(pCS);
  DC_UnlockDc(pDC);
  return TRUE;
}

BOOL
FASTCALL
UpdateDeviceGammaRamp( HDEV hPDev )
{
  BOOL Ret = FALSE;
  PPALGDI palGDI;
  PALOBJ *palPtr;
  PGDIDEVICE pGDev = (PGDIDEVICE) hPDev;

  if ((pGDev->DevInfo.iDitherFormat == BMF_8BPP)  ||
      (pGDev->DevInfo.iDitherFormat == BMF_16BPP) ||
      (pGDev->DevInfo.iDitherFormat == BMF_24BPP) ||
      (pGDev->DevInfo.iDitherFormat == BMF_32BPP))
  {
     if (pGDev->DriverFunctions.IcmSetDeviceGammaRamp)
         return pGDev->DriverFunctions.IcmSetDeviceGammaRamp( pGDev->hPDev,
                                                        IGRF_RGB_256WORDS,
                                                       pGDev->pvGammaRamp);

     if ( (pGDev->DevInfo.iDitherFormat != BMF_8BPP) ||
         !(pGDev->GDIInfo.flRaster & RC_PALETTE)) return FALSE;

     if (!(pGDev->flFlags & PDEV_GAMMARAMP_TABLE)) return FALSE;

     palGDI = PALETTE_LockPalette(pGDev->DevInfo.hpalDefault);
     if(!palGDI) return FALSE;
     palPtr = (PALOBJ*) palGDI;

     if (pGDev->flFlags & PDEV_GAMMARAMP_TABLE)
        palGDI->Mode |= PAL_GAMMACORRECTION;
     else
        palGDI->Mode &= ~PAL_GAMMACORRECTION;

     if (!(pGDev->flFlags & PDEV_DRIVER_PUNTED_CALL)) // No punting, we hook
     {
     // BMF_8BPP only!
     // PALOBJ_cGetColors check mode flags and update Gamma Correction.
     // Set the HDEV to pal and go.
        palGDI->hPDev = hPDev;
        Ret = pGDev->DriverFunctions.SetPalette(pGDev->hPDev,
                                                     palPtr,
                                                          0,
                                                          0,
                                          palGDI->NumColors);
     }
     PALETTE_UnlockPalette(palGDI);
     return Ret;
  }
  else
     return FALSE;
}

//
// ICM registry subkey sets internal brightness range, gamma range is 128 or
// 256 when icm is init.
INT IcmGammaRangeSet = 128; // <- make it global

BOOL
FASTCALL
IntSetDeviceGammaRamp(HDEV hPDev, PGAMMARAMP Ramp, BOOL Test)
{
  WORD IcmGR, i, R, G, B;
  BOOL Ret = FALSE, TstPeak;
  PGDIDEVICE pGDev = (PGDIDEVICE) hPDev;

  if (!hPDev) return FALSE;

  if (!(pGDev->flFlags & PDEV_DISPLAY )) return FALSE;

  if ((pGDev->DevInfo.iDitherFormat == BMF_8BPP)  ||
      (pGDev->DevInfo.iDitherFormat == BMF_16BPP) ||
      (pGDev->DevInfo.iDitherFormat == BMF_24BPP) ||
      (pGDev->DevInfo.iDitherFormat == BMF_32BPP))
  {
     if (!pGDev->DriverFunctions.IcmSetDeviceGammaRamp)
     {  // No driver support
        if (!(pGDev->DevInfo.flGraphicsCaps2 & GCAPS2_CHANGEGAMMARAMP))
        { // Driver does not support Gamma Ramp, so test to see we
          // have BMF_8BPP only and palette operation support.
           if ((pGDev->DevInfo.iDitherFormat != BMF_8BPP) ||
              !(pGDev->GDIInfo.flRaster & RC_PALETTE))  return FALSE;
        }
     }

     if (pGDev->flFlags & PDEV_GAMMARAMP_TABLE)
        if (RtlCompareMemory( pGDev->pvGammaRamp, Ramp, sizeof(GAMMARAMP)) ==
                                               sizeof(GAMMARAMP)) return TRUE;
     // Verify Ramp is inside range.
     IcmGR = -IcmGammaRangeSet;
     TstPeak = (Test == FALSE);
     for (i = 0; i < 256; i++)
     {
         R = Ramp->Red[i]   / 256;
         G = Ramp->Green[i] / 256;
         B = Ramp->Blue[i]  / 256;
         if ( R >= IcmGR)
         {
            if ( R <= IcmGammaRangeSet + i)
            {
               if ( G >= IcmGR &&
                   (G <= IcmGammaRangeSet + i) &&
                    B >= IcmGR &&
                   (B <= IcmGammaRangeSet + i) ) continue;
            }
         }
         if (Test) return Ret; // Don't set and return.
         // No test override, check max range
         if (TstPeak)
         {
            if ( R != (IcmGR * 256) ||
                 G != (IcmGR * 256) ||
                 B != (IcmGR * 256) ) TstPeak = FALSE; // W/i range.
         }
     }
     // ReactOS allocates a ramp even if it is 8BPP and Palette only.
     // This way we have a record of the change in memory.
     if (!pGDev->pvGammaRamp && !(pGDev->flFlags & PDEV_GAMMARAMP_TABLE))
     {  // If the above is true and we have nothing allocated, create it.
        pGDev->pvGammaRamp = ExAllocatePoolWithTag(PagedPool, sizeof(GAMMARAMP), TAG_GDIICM);
        pGDev->flFlags |= PDEV_GAMMARAMP_TABLE;
     }
     if (pGDev->pvGammaRamp)
        RtlCopyMemory( pGDev->pvGammaRamp, Ramp, sizeof(GAMMARAMP));

     Ret = UpdateDeviceGammaRamp(hPDev);

     return Ret;
  }
  else
     return Ret;
}

BOOL
APIENTRY
NtGdiSetDeviceGammaRamp(HDC  hDC,
                             LPVOID  Ramp)
{
  BOOL Ret;
  PDC dc;
  NTSTATUS Status = STATUS_SUCCESS;
  PGAMMARAMP SafeRamp;
  if (!Ramp) return FALSE;

  dc = DC_LockDc(hDC);
  if (!dc)
  {
     SetLastWin32Error(ERROR_INVALID_HANDLE);
     return FALSE;
  }

  SafeRamp = ExAllocatePool(PagedPool, sizeof(GAMMARAMP));
  if (!SafeRamp)
  {
      DC_UnlockDc(dc);
      SetLastWin32Error(STATUS_NO_MEMORY);
      return FALSE;
  }
  _SEH2_TRY
  {
     ProbeForRead( Ramp,
                   sizeof(PVOID),
                   1);
     RtlCopyMemory( SafeRamp,
                    Ramp,
                    sizeof(GAMMARAMP));
  }
  _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
  {
     Status = _SEH2_GetExceptionCode();
  }
  _SEH2_END;

  if (!NT_SUCCESS(Status))
  {
     DC_UnlockDc(dc);
     ExFreePool(SafeRamp);
     SetLastNtError(Status);
     return FALSE;
  }

  Ret = IntSetDeviceGammaRamp((HDEV)dc->ppdev, SafeRamp, TRUE);
  DC_UnlockDc(dc);
  ExFreePool(SafeRamp);
  return Ret;
}

INT
APIENTRY
NtGdiSetIcmMode(HDC  hDC,
                ULONG nCommand,
                ULONG EnableICM) // ulMode
{
  /* FIXME: this should be coded someday  */
  if (EnableICM == ICM_OFF)
    {
      return  ICM_OFF;
    }
  if (EnableICM == ICM_ON)
    {
      return  0;
    }
  if (EnableICM == ICM_QUERY)
    {
      return  ICM_OFF;
    }

  return  0;
}

/* EOF */
