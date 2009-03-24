/* 
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Functions for creation and destruction of DCs
 * FILE:              subsystem/win32/win32k/objects/dcstate.c
 * PROGRAMER:         Timo Kreuzer (timo.kreuzer@rectos.org)
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>


VOID
FASTCALL
IntGdiCopyToSaveState(PDC dc, PDC newdc)
{
  PDC_ATTR pdcattr, nDc_Attr;

  pdcattr = dc->pdcattr;
  nDc_Attr = newdc->pdcattr;

  newdc->dclevel.flPath     = dc->dclevel.flPath | DCPATH_SAVESTATE;

  nDc_Attr->dwLayout        = pdcattr->dwLayout;
  nDc_Attr->hpen            = pdcattr->hpen;
  nDc_Attr->hbrush          = pdcattr->hbrush;
  nDc_Attr->hlfntNew        = pdcattr->hlfntNew;
  newdc->rosdc.hBitmap          = dc->rosdc.hBitmap;
  newdc->dclevel.hpal       = dc->dclevel.hpal;
  newdc->rosdc.bitsPerPixel     = dc->rosdc.bitsPerPixel;
  nDc_Attr->jROP2           = pdcattr->jROP2;
  nDc_Attr->jFillMode       = pdcattr->jFillMode;
  nDc_Attr->jStretchBltMode = pdcattr->jStretchBltMode;
  nDc_Attr->lRelAbs         = pdcattr->lRelAbs;
  nDc_Attr->jBkMode         = pdcattr->jBkMode;
  nDc_Attr->lBkMode         = pdcattr->lBkMode;
  nDc_Attr->crBackgroundClr = pdcattr->crBackgroundClr;
  nDc_Attr->crForegroundClr = pdcattr->crForegroundClr;
  nDc_Attr->ulBackgroundClr = pdcattr->ulBackgroundClr;
  nDc_Attr->ulForegroundClr = pdcattr->ulForegroundClr;
  nDc_Attr->ptlBrushOrigin  = pdcattr->ptlBrushOrigin;
  nDc_Attr->lTextAlign      = pdcattr->lTextAlign;
  nDc_Attr->lTextExtra      = pdcattr->lTextExtra;
  nDc_Attr->cBreak          = pdcattr->cBreak;
  nDc_Attr->lBreakExtra     = pdcattr->lBreakExtra;
  nDc_Attr->iMapMode        = pdcattr->iMapMode;
  nDc_Attr->iGraphicsMode   = pdcattr->iGraphicsMode;
#if 0
  /* Apparently, the DC origin is not changed by [GS]etDCState */
  newdc->ptlDCOrig.x           = dc->ptlDCOrig.x;
  newdc->ptlDCOrig.y           = dc->ptlDCOrig.y;
#endif
  nDc_Attr->ptlCurrent      = pdcattr->ptlCurrent;
  nDc_Attr->ptfxCurrent     = pdcattr->ptfxCurrent;
  newdc->dclevel.mxWorldToDevice = dc->dclevel.mxWorldToDevice;
  newdc->dclevel.mxDeviceToWorld = dc->dclevel.mxDeviceToWorld;
  newdc->dclevel.mxWorldToPage   = dc->dclevel.mxWorldToPage;
  nDc_Attr->flXform         = pdcattr->flXform;
  nDc_Attr->ptlWindowOrg    = pdcattr->ptlWindowOrg;
  nDc_Attr->szlWindowExt    = pdcattr->szlWindowExt;
  nDc_Attr->ptlViewportOrg  = pdcattr->ptlViewportOrg;
  nDc_Attr->szlViewportExt  = pdcattr->szlViewportExt;

  newdc->dclevel.lSaveDepth = 0;
  newdc->dctype = dc->dctype;

#if 0
  PATH_InitGdiPath( &newdc->dclevel.hPath );
#endif

  /* Get/SetDCState() don't change hVisRgn field ("Undoc. Windows" p.559). */

  newdc->rosdc.hGCClipRgn = newdc->rosdc.hVisRgn = 0;
  if (dc->rosdc.hClipRgn)
  {
    newdc->rosdc.hClipRgn = NtGdiCreateRectRgn( 0, 0, 0, 0 );
    NtGdiCombineRgn( newdc->rosdc.hClipRgn, dc->rosdc.hClipRgn, 0, RGN_COPY );
  }
}

// FIXME: why 2 different functions that do the same?
VOID
FASTCALL
IntGdiCopyFromSaveState(PDC dc, PDC dcs, HDC hDC)
{
  PDC_ATTR pdcattr, sDc_Attr;

  pdcattr = dc->pdcattr;
  sDc_Attr = dcs->pdcattr;

  dc->dclevel.flPath       = dcs->dclevel.flPath & ~DCPATH_SAVESTATE;

  pdcattr->dwLayout        = sDc_Attr->dwLayout;
  pdcattr->jROP2           = sDc_Attr->jROP2;
  pdcattr->jFillMode       = sDc_Attr->jFillMode;
  pdcattr->jStretchBltMode = sDc_Attr->jStretchBltMode;
  pdcattr->lRelAbs         = sDc_Attr->lRelAbs;
  pdcattr->jBkMode         = sDc_Attr->jBkMode;
  pdcattr->crBackgroundClr = sDc_Attr->crBackgroundClr;
  pdcattr->crForegroundClr = sDc_Attr->crForegroundClr;
  pdcattr->lBkMode         = sDc_Attr->lBkMode;
  pdcattr->ulBackgroundClr = sDc_Attr->ulBackgroundClr;
  pdcattr->ulForegroundClr = sDc_Attr->ulForegroundClr;
  pdcattr->ptlBrushOrigin  = sDc_Attr->ptlBrushOrigin;

  pdcattr->lTextAlign      = sDc_Attr->lTextAlign;
  pdcattr->lTextExtra      = sDc_Attr->lTextExtra;
  pdcattr->cBreak          = sDc_Attr->cBreak;
  pdcattr->lBreakExtra     = sDc_Attr->lBreakExtra;
  pdcattr->iMapMode        = sDc_Attr->iMapMode;
  pdcattr->iGraphicsMode   = sDc_Attr->iGraphicsMode;
#if 0
/* Apparently, the DC origin is not changed by [GS]etDCState */
  dc->ptlDCOrig.x             = dcs->ptlDCOrig.x;
  dc->ptlDCOrig.y             = dcs->ptlDCOrig.y;
#endif
  pdcattr->ptlCurrent      = sDc_Attr->ptlCurrent;
  pdcattr->ptfxCurrent     = sDc_Attr->ptfxCurrent;
  dc->dclevel.mxWorldToDevice = dcs->dclevel.mxWorldToDevice;
  dc->dclevel.mxDeviceToWorld = dcs->dclevel.mxDeviceToWorld;
  dc->dclevel.mxWorldToPage   = dcs->dclevel.mxWorldToPage;
  pdcattr->flXform         = sDc_Attr->flXform;
  pdcattr->ptlWindowOrg    = sDc_Attr->ptlWindowOrg;
  pdcattr->szlWindowExt    = sDc_Attr->szlWindowExt;
  pdcattr->ptlViewportOrg  = sDc_Attr->ptlViewportOrg;
  pdcattr->szlViewportExt  = sDc_Attr->szlViewportExt;

  if (dc->dctype != DC_TYPE_MEMORY)
  {
     dc->rosdc.bitsPerPixel = dcs->rosdc.bitsPerPixel;
  }

#if 0
  if (dcs->rosdc.hClipRgn)
  {
    if (!dc->rosdc.hClipRgn)
    {
       dc->rosdc.hClipRgn = NtGdiCreateRectRgn( 0, 0, 0, 0 );
    }
    NtGdiCombineRgn( dc->rosdc.hClipRgn, dcs->rosdc.hClipRgn, 0, RGN_COPY );
  }
  else
  {
    if (dc->rosdc.hClipRgn)
    {
       NtGdiDeleteObject( dc->rosdc.hClipRgn );
    }
    dc->rosdc.hClipRgn = 0;
  }
  {
    int res;
    res = CLIPPING_UpdateGCRegion( dc );
    ASSERT ( res != ERROR );
  }
  DC_UnlockDc ( dc );
#else
  GdiExtSelectClipRgn(dc, dcs->rosdc.hClipRgn, RGN_COPY);
  DC_UnlockDc ( dc );
#endif
  if(!hDC) return; // Not a MemoryDC or SaveLevel DC, return.

  NtGdiSelectBitmap( hDC, dcs->rosdc.hBitmap );
  NtGdiSelectBrush( hDC, sDc_Attr->hbrush );
  NtGdiSelectFont( hDC, sDc_Attr->hlfntNew );
  NtGdiSelectPen( hDC, sDc_Attr->hpen );

  IntGdiSetBkColor( hDC, sDc_Attr->crBackgroundClr);
  IntGdiSetTextColor( hDC, sDc_Attr->crForegroundClr);

  GdiSelectPalette( hDC, dcs->dclevel.hpal, FALSE );

#if 0
  GDISelectPalette16( hDC, dcs->dclevel.hpal, FALSE );
#endif
}

HDC APIENTRY
IntGdiGetDCState(HDC hDC)
{
  PDC pdcNew, pdc;
  HDC hdcNew;

  pdc = DC_LockDc(hDC);
  if (pdc == NULL)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return 0;
  }

  pdcNew = DC_AllocDC(NULL);
  if (pdcNew == NULL)
  {
    DC_UnlockDc(pdc);
    return 0;
  }
  hdcNew = pdcNew->BaseObject.hHmgr;

  pdcNew->dclevel.hdcSave = hdcNew;
  IntGdiCopyToSaveState(pdc, pdcNew);

  DC_UnlockDc(pdcNew);
  DC_UnlockDc(pdc);

  return hdcNew;
}

VOID
APIENTRY
IntGdiSetDCState ( HDC hDC, HDC hDCSave )
{
  PDC  dc, dcs;

  dc = DC_LockDc ( hDC );
  if ( dc )
  {
    dcs = DC_LockDc ( hDCSave );
    if ( dcs )
    {
      if ( dcs->dclevel.flPath & DCPATH_SAVESTATE )
      {
        IntGdiCopyFromSaveState( dc, dcs, dc->dclevel.hdcSave);
      }
      else
      {
        DC_UnlockDc(dc);
      }
      DC_UnlockDc ( dcs );
    }
    else
    {
      DC_UnlockDc ( dc );
      SetLastWin32Error(ERROR_INVALID_HANDLE);
    }
  }
  else
    SetLastWin32Error(ERROR_INVALID_HANDLE);
}



BOOL
APIENTRY
NtGdiResetDC(
    IN HDC hdc,
    IN LPDEVMODEW pdm,
    OUT PBOOL pbBanding,
    IN OPTIONAL VOID *pDriverInfo2,
    OUT VOID *ppUMdhpdev)
{
  UNIMPLEMENTED;
  return 0;
}

BOOL APIENTRY
NtGdiRestoreDC(HDC  hDC, INT  SaveLevel)
{
  PDC dc, dcs;
  BOOL success;

  DPRINT("NtGdiRestoreDC(%lx, %d)\n", hDC, SaveLevel);

  dc = DC_LockDc(hDC);
  if (!dc)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }

  if (SaveLevel < 0)
      SaveLevel = dc->dclevel.lSaveDepth + SaveLevel + 1;

  if(SaveLevel < 0 || dc->dclevel.lSaveDepth<SaveLevel)
  {
    DC_UnlockDc(dc);
    return FALSE;
  }

  success=TRUE;
  while (dc->dclevel.lSaveDepth >= SaveLevel)
  {
     HDC hdcs = dc->hdcNext;

     dcs = DC_LockDc (hdcs);
     if (dcs == NULL)
     {
        DC_UnlockDc(dc);
        return FALSE;
     }

     dc->hdcNext = dcs->hdcNext;
     dcs->hdcNext = 0;

     if (--dc->dclevel.lSaveDepth < SaveLevel)
     {
         DC_UnlockDc( dc );
         DC_UnlockDc( dcs );

         IntGdiSetDCState(hDC, hdcs);

         dc = DC_LockDc(hDC);
         if(!dc)
         {
            return FALSE;
         }
         // Restore Path by removing it, if the Save flag is set.
         // BeginPath will takecare of the rest.
         if ( dc->dclevel.hPath && dc->dclevel.flPath & DCPATH_SAVE)
         {
            PATH_Delete(dc->dclevel.hPath);
            dc->dclevel.hPath = 0;
            dc->dclevel.flPath &= ~DCPATH_SAVE;
         }
       }
       else
       {
         DC_UnlockDc( dcs );
       }
       NtGdiDeleteObjectApp (hdcs);
  }
  DC_UnlockDc( dc );
  return  success;
}


INT APIENTRY
NtGdiSaveDC(HDC  hDC)
{
  HDC  hdcs;
  PDC  dc, dcs;
  INT  ret;

  DPRINT("NtGdiSaveDC(%lx)\n", hDC);

  if (!(hdcs = IntGdiGetDCState(hDC)))
  {
    return 0;
  }

  dcs = DC_LockDc (hdcs);
  if (dcs == NULL)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return 0;
  }
  dc = DC_LockDc (hDC);
  if (dc == NULL)
  {
    DC_UnlockDc(dcs);
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return 0;
  }

  /* 
   * Copy path.
   */
  dcs->dclevel.hPath = dc->dclevel.hPath;
  if (dcs->dclevel.hPath) dcs->dclevel.flPath |= DCPATH_SAVE;

  dcs->hdcNext = dc->hdcNext;
  dc->hdcNext = hdcs;
  ret = ++dc->dclevel.lSaveDepth;
  DC_UnlockDc( dcs );
  DC_UnlockDc( dc );

  return  ret;
}



BOOL FASTCALL
IntGdiCleanDC(HDC hDC)
{
  PDC dc;
  if (!hDC) return FALSE;
  dc = DC_LockDc ( hDC );
  if (!dc) return FALSE;
  // Clean the DC
  if (defaultDCstate) IntGdiCopyFromSaveState(dc, defaultDCstate, hDC );
  return TRUE;
}

