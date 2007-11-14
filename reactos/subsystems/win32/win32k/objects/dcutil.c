
#include <w32k.h>

#define NDEBUG
#include <debug.h>

static
VOID
CopytoUserDcAttr(PDC dc, PDC_ATTR Dc_Attr, FLONG Dirty)
{
      Dc_Attr->hpen              = dc->Dc_Attr.hpen;
      Dc_Attr->hbrush            = dc->Dc_Attr.hbrush;
      Dc_Attr->hColorSpace       = dc->Dc_Attr.hColorSpace;
      Dc_Attr->hlfntNew          = dc->Dc_Attr.hlfntNew;

      Dc_Attr->jROP2             = dc->Dc_Attr.jROP2;
      Dc_Attr->jFillMode         = dc->Dc_Attr.jFillMode;
      Dc_Attr->jStretchBltMode   = dc->Dc_Attr.jStretchBltMode;
      Dc_Attr->lRelAbs           = dc->Dc_Attr.lRelAbs;
      Dc_Attr->jBkMode           = dc->Dc_Attr.jBkMode;

      Dc_Attr->crBackgroundClr   = dc->Dc_Attr.crBackgroundClr;
      Dc_Attr->ulBackgroundClr   = dc->Dc_Attr.ulBackgroundClr;
      Dc_Attr->crForegroundClr   = dc->Dc_Attr.crForegroundClr;
      Dc_Attr->ulForegroundClr   = dc->Dc_Attr.ulForegroundClr;

      Dc_Attr->ulBrushClr        = dc->Dc_Attr.ulBrushClr;
      Dc_Attr->crBrushClr        = dc->Dc_Attr.crBrushClr;

      Dc_Attr->ulPenClr          = dc->Dc_Attr.ulPenClr;
      Dc_Attr->crPenClr          = dc->Dc_Attr.crPenClr;

      Dc_Attr->ptlBrushOrigin    = dc->Dc_Attr.ptlBrushOrigin;

      Dc_Attr->lTextAlign        = dc->Dc_Attr.lTextAlign;
      Dc_Attr->lTextExtra        = dc->Dc_Attr.lTextExtra;
      Dc_Attr->cBreak            = dc->Dc_Attr.cBreak;
      Dc_Attr->lBreakExtra       = dc->Dc_Attr.lBreakExtra;
      Dc_Attr->iMapMode          = dc->Dc_Attr.iMapMode;
      Dc_Attr->iGraphicsMode     = dc->Dc_Attr.iGraphicsMode;

      Dc_Attr->ptlCurrent        = dc->Dc_Attr.ptlCurrent;
      Dc_Attr->ptlWindowOrg      = dc->Dc_Attr.ptlWindowOrg;
      Dc_Attr->szlWindowExt      = dc->Dc_Attr.szlWindowExt;
      Dc_Attr->ptlViewportOrg    = dc->Dc_Attr.ptlViewportOrg;
      Dc_Attr->szlViewportExt    = dc->Dc_Attr.szlViewportExt;

      Dc_Attr->ulDirty_          = dc->Dc_Attr.ulDirty_; //Copy flags! We may have set them.
      
      XForm2MatrixS( &Dc_Attr->mxWorldToDevice, &dc->w.xformWorld2Vport);
      XForm2MatrixS( &Dc_Attr->mxDevicetoWorld, &dc->w.xformVport2World);
      XForm2MatrixS( &Dc_Attr->mxWorldToPage, &dc->w.xformWorld2Wnd);
}


BOOL
FASTCALL
DCU_SyncDcAttrtoUser(PDC dc, FLONG Dirty)
{
  PDC_ATTR Dc_Attr = dc->pDc_Attr;
  if (!Dirty) return FALSE;

  if (Dc_Attr == ((PDC_ATTR)&dc->Dc_Attr)) return TRUE; // No need to copy self.
  
  if (!Dc_Attr) return FALSE;
  else
    CopytoUserDcAttr( dc, Dc_Attr, Dirty);
  return TRUE;
}

BOOL
FASTCALL
DCU_SynchDcAttrtoUser(HDC hDC, FLONG Dirty)
{
  PDC pDC = DC_LockDc ( hDC );
  if (!pDC) return FALSE;
  BOOL Ret = DCU_SyncDcAttrtoUser(pDC, Dirty);
  DC_UnlockDc( pDC );
  return Ret;
}


