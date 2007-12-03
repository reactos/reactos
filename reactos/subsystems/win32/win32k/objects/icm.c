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
/* $Id$ */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

BOOL
STDCALL
NtGdiColorMatchToTarget(HDC  hDC,
                             HDC  hDCTarget,
                             DWORD  Action)
{
  UNIMPLEMENTED;
  return FALSE;
}

HANDLE
APIENTRY
NtGdiCreateColorSpace(
    IN PLOGCOLORSPACEEXW pLogColorSpace)
{
  UNIMPLEMENTED;
  return 0;
}

BOOL
APIENTRY
NtGdiDeleteColorSpace(
    IN HANDLE hColorSpace)
{
  UNIMPLEMENTED;
  return FALSE;
}

INT
STDCALL
NtGdiEnumICMProfiles(HDC    hDC,
                    LPWSTR lpstrBuffer,
                    UINT   cch )
{
  /*
   * FIXME - build list of file names into lpstrBuffer.
   * (MULTI-SZ would probably be best format)
   * return (needed) length of buffer in bytes
   */
  UNIMPLEMENTED;
  return 0;
}

HCOLORSPACE
STDCALL
NtGdiGetColorSpace(HDC  hDC)
{
  /* FIXME: Need to to whatever GetColorSpace actually does */
  return  0;
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
          if (NewValue > 65535) NewValue = 65535;
          
          Ramp->Red[i] = Ramp->Green[i] = Ramp->Blue[i] = ((WORD)NewValue);
        }
     return TRUE;
  }
  else
     return FALSE;
}

BOOL
STDCALL
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

  Ret = IntGetDeviceGammaRamp((HDEV)dc->pPDev, SafeRamp);

  if (!Ret) return Ret;

  _SEH_TRY
  {
     ProbeForWrite( Ramp,
                    sizeof(PVOID),
                    1);
     RtlCopyMemory( Ramp,
                    SafeRamp,
                    sizeof(GAMMARAMP));
  }
  _SEH_HANDLE
  {
     Status = _SEH_GetExceptionCode();
  }
  _SEH_END;

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
STDCALL
NtGdiGetICMProfile(HDC  hDC,
                        LPDWORD  NameSize,
                        LPWSTR  Filename)
{
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
STDCALL
NtGdiGetLogColorSpace(HCOLORSPACE  hColorSpace,
                           LPLOGCOLORSPACEW  Buffer,
                           DWORD  Size)
{
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
STDCALL
NtGdiSetColorSpace(IN HDC hdc,
                   IN HCOLORSPACE hColorSpace)
{
  UNIMPLEMENTED;
  return 0;
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
         return pGDev->DriverFunctions.IcmSetDeviceGammaRamp( pGDev->PDev,
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
        Ret = pGDev->DriverFunctions.SetPalette(pGDev->PDev, 
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

BOOL
FASTCALL
IntSetDeviceGammaRamp(HDEV hPDev, PGAMMARAMP Ramp)
{
  BOOL Ret = FALSE;
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

     if (!pGDev->pvGammaRamp && !(pGDev->flFlags & PDEV_GAMMARAMP_TABLE))
     {  // If the above is true and we have nothing allocated, create it.
        pGDev->pvGammaRamp = ExAllocatePoolWithTag(PagedPool, sizeof(GAMMARAMP), TAG_GDIICM);
        pGDev->flFlags |= PDEV_GAMMARAMP_TABLE;
     }
     //
     // Need to adjust the input Ramp with internal brightness before copy.
     // ICM subkey sets internal brightness, gamma range 128 or 256 during icm init.
     RtlCopyMemory( pGDev->pvGammaRamp, Ramp, sizeof(GAMMARAMP));

     Ret = UpdateDeviceGammaRamp(hPDev);

     return Ret;
  }
  else
     return FALSE;
}

BOOL
STDCALL
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
  _SEH_TRY
  {
     ProbeForRead( Ramp,
                   sizeof(PVOID),
                   1);
     RtlCopyMemory( SafeRamp,
                    Ramp,
                    sizeof(GAMMARAMP));
  }
  _SEH_HANDLE
  {
     Status = _SEH_GetExceptionCode();
  }
  _SEH_END;

  if (!NT_SUCCESS(Status))
  {
     DC_UnlockDc(dc);
     ExFreePool(SafeRamp);
     SetLastNtError(Status);
     return FALSE;
  }

  Ret = IntSetDeviceGammaRamp((HDEV)dc->pPDev, SafeRamp);
  DC_UnlockDc(dc);
  ExFreePool(SafeRamp);
  return Ret;
}

INT
STDCALL
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

BOOL
STDCALL
NtGdiSetICMProfile(HDC  hDC,
                        LPWSTR  Filename)
{
  UNIMPLEMENTED;
  return FALSE;
}

BOOL
STDCALL
NtGdiUpdateICMRegKey(DWORD  Reserved,
                          LPWSTR  CMID,
                          LPWSTR  Filename,
                          UINT  Command)
{
  UNIMPLEMENTED;
  return FALSE;
}

/* EOF */
