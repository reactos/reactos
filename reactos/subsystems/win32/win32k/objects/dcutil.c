
#include <w32k.h>

#define NDEBUG
#include <debug.h>

/*
 * DC device-independent Get/SetXXX functions
 * (RJJ) swiped from WINE
 */

#define DC_GET_VAL( func_type, func_name, dc_field ) \
func_type STDCALL  func_name( HDC hdc ) \
{                                   \
  func_type  ft;                    \
  PDC  dc = DC_LockDc( hdc );       \
  PDC_ATTR Dc_Attr;                 \
  if (!dc)                          \
  {                                 \
    SetLastWin32Error(ERROR_INVALID_HANDLE); \
    return 0;                       \
  }                                 \
  Dc_Attr = dc->pDc_Attr;           \
  if (!Dc_Attr) Dc_Attr = &dc->Dc_Attr; \
  ft = Dc_Attr->dc_field;           \
  DC_UnlockDc(dc);                  \
  return ft;                        \
}

/* DC_GET_VAL_EX is used to define functions returning a POINT or a SIZE. It is
 * important that the function has the right signature, for the implementation
 * we can do whatever we want.
 */
#define DC_GET_VAL_EX( FuncName, ret_x, ret_y, type, ax, ay ) \
VOID FASTCALL Int##FuncName ( PDC dc, LP##type pt) \
{ \
  PDC_ATTR Dc_Attr; \
  ASSERT(dc); \
  ASSERT(pt); \
  Dc_Attr = dc->pDc_Attr; \
  if (!Dc_Attr) Dc_Attr = &dc->Dc_Attr; \
  pt->ax = Dc_Attr->ret_x; \
  pt->ay = Dc_Attr->ret_y; \
} \
BOOL STDCALL NtGdi##FuncName ( HDC hdc, LP##type pt ) \
{ \
  NTSTATUS Status = STATUS_SUCCESS; \
  type Safept; \
  PDC dc; \
  if(!pt) \
  { \
    SetLastWin32Error(ERROR_INVALID_PARAMETER); \
    return FALSE; \
  } \
  if(!(dc = DC_LockDc(hdc))) \
  { \
    SetLastWin32Error(ERROR_INVALID_HANDLE); \
    return FALSE; \
  } \
  Int##FuncName( dc, &Safept); \
  DC_UnlockDc(dc); \
  _SEH_TRY \
  { \
    ProbeForWrite(pt, \
                  sizeof( type ), \
                  1); \
    *pt = Safept; \
  } \
  _SEH_HANDLE \
  { \
    Status = _SEH_GetExceptionCode(); \
  } \
  _SEH_END; \
  if(!NT_SUCCESS(Status)) \
  { \
    SetLastNtError(Status); \
    return FALSE; \
  } \
  return TRUE; \
}

#define DC_SET_MODE( func_name, dc_field, min_val, max_val ) \
INT STDCALL  func_name( HDC hdc, INT mode ) \
{                                           \
  INT  prevMode;                            \
  PDC  dc;                                  \
  PDC_ATTR Dc_Attr;                         \
  if ((mode < min_val) || (mode > max_val)) \
  { \
    SetLastWin32Error(ERROR_INVALID_PARAMETER); \
    return 0;                               \
  } \
  dc = DC_LockDc ( hdc );              \
  if ( !dc )                                \
  { \
    SetLastWin32Error(ERROR_INVALID_HANDLE); \
    return 0;                               \
  } \
  Dc_Attr = dc->pDc_Attr;           \
  if (!Dc_Attr) Dc_Attr = &dc->Dc_Attr; \
  prevMode = Dc_Attr->dc_field;             \
  Dc_Attr->dc_field = mode;                 \
  DC_UnlockDc ( dc );                    \
  return prevMode;                          \
}


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

      MmCopyToCaller(Dc_Attr, &dc->Dc_Attr, sizeof(DC_ATTR));

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


DC_GET_VAL( INT, NtGdiGetMapMode, iMapMode )
DC_GET_VAL( INT, NtGdiGetPolyFillMode, jFillMode )
DC_GET_VAL( COLORREF, NtGdiGetBkColor, crBackgroundClr )
DC_GET_VAL( INT, NtGdiGetBkMode, jBkMode )
DC_GET_VAL( INT, NtGdiGetROP2, jROP2 )
DC_GET_VAL( INT, NtGdiGetStretchBltMode, jStretchBltMode )
DC_GET_VAL( UINT, NtGdiGetTextAlign, lTextAlign )
DC_GET_VAL( COLORREF, NtGdiGetTextColor, crForegroundClr )

DC_GET_VAL_EX( GetViewportExtEx, szlViewportExt.cx, szlViewportExt.cy, SIZE, cx, cy )
DC_GET_VAL_EX( GetViewportOrgEx, ptlViewportOrg.x, ptlViewportOrg.y, POINT, x, y )
DC_GET_VAL_EX( GetWindowExtEx, szlWindowExt.cx, szlWindowExt.cy, SIZE, cx, cy )
DC_GET_VAL_EX( GetWindowOrgEx, ptlWindowOrg.x, ptlWindowOrg.y, POINT, x, y )
DC_GET_VAL_EX ( GetCurrentPositionEx, ptlCurrent.x, ptlCurrent.y, POINT, x, y )

DC_SET_MODE( NtGdiSetBkMode, jBkMode, TRANSPARENT, OPAQUE )
DC_SET_MODE( NtGdiSetPolyFillMode, jFillMode, ALTERNATE, WINDING )
DC_SET_MODE( NtGdiSetROP2, jROP2, R2_BLACK, R2_WHITE )
DC_SET_MODE( NtGdiSetStretchBltMode, jStretchBltMode, BLACKONWHITE, HALFTONE )



COLORREF STDCALL
NtGdiSetBkColor(HDC hDC, COLORREF color)
{
  COLORREF oldColor;
  PDC dc;
  PDC_ATTR Dc_Attr;
  HBRUSH hBrush;

  if (!(dc = DC_LockDc(hDC)))
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return CLR_INVALID;
  }
  Dc_Attr = dc->pDc_Attr;
  if (!Dc_Attr) Dc_Attr = &dc->Dc_Attr;
  oldColor = Dc_Attr->crBackgroundClr;
  Dc_Attr->crBackgroundClr = color;
  hBrush = Dc_Attr->hbrush;
  DC_UnlockDc(dc);
  NtGdiSelectObject(hDC, hBrush);
  return oldColor;
}

