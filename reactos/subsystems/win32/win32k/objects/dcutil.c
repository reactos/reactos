
#include <w32k.h>

#define NDEBUG
#include <debug.h>

/*
 * DC device-independent Get/SetXXX functions
 * (RJJ) swiped from WINE
 */

#define DC_GET_VAL( func_type, func_name, dc_field ) \
func_type APIENTRY  func_name( HDC hdc ) \
{                                   \
  func_type  ft;                    \
  PDC  dc = DC_LockDc( hdc );       \
  PDC_ATTR pdcattr;                 \
  if (!dc)                          \
  {                                 \
    SetLastWin32Error(ERROR_INVALID_HANDLE); \
    return 0;                       \
  }                                 \
  pdcattr = dc->pdcattr;           \
  ft = pdcattr->dc_field;           \
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
  PDC_ATTR pdcattr; \
  ASSERT(dc); \
  ASSERT(pt); \
  pdcattr = dc->pdcattr; \
  pt->ax = pdcattr->ret_x; \
  pt->ay = pdcattr->ret_y; \
}

#if 0
BOOL APIENTRY NtGdi##FuncName ( HDC hdc, LP##type pt ) \
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
  _SEH2_TRY \
  { \
    ProbeForWrite(pt, \
                  sizeof( type ), \
                  1); \
    *pt = Safept; \
  } \
  _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) \
  { \
    Status = _SEH2_GetExceptionCode(); \
  } \
  _SEH2_END; \
  if(!NT_SUCCESS(Status)) \
  { \
    SetLastNtError(Status); \
    return FALSE; \
  } \
  return TRUE; \
}
#endif

#define DC_SET_MODE( func_name, dc_field, min_val, max_val ) \
INT APIENTRY  func_name( HDC hdc, INT mode ) \
{                                           \
  INT  prevMode;                            \
  PDC  dc;                                  \
  PDC_ATTR pdcattr;                         \
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
  pdcattr = dc->pdcattr;           \
  prevMode = pdcattr->dc_field;             \
  pdcattr->dc_field = mode;                 \
  DC_UnlockDc ( dc );                    \
  return prevMode;                          \
}


DC_GET_VAL( INT, IntGdiGetMapMode, iMapMode )
DC_GET_VAL( INT, IntGdiGetPolyFillMode, jFillMode )
DC_GET_VAL( COLORREF, IntGdiGetBkColor, crBackgroundClr )
DC_GET_VAL( INT, IntGdiGetBkMode, jBkMode )
DC_GET_VAL( INT, IntGdiGetROP2, jROP2 )
DC_GET_VAL( INT, IntGdiGetStretchBltMode, jStretchBltMode )
DC_GET_VAL( UINT, IntGdiGetTextAlign, lTextAlign )
DC_GET_VAL( COLORREF, IntGdiGetTextColor, crForegroundClr )

DC_GET_VAL_EX( GetViewportOrgEx, ptlViewportOrg.x, ptlViewportOrg.y, POINT, x, y )
DC_GET_VAL_EX( GetWindowExtEx, szlWindowExt.cx, szlWindowExt.cy, SIZE, cx, cy )
DC_GET_VAL_EX( GetWindowOrgEx, ptlWindowOrg.x, ptlWindowOrg.y, POINT, x, y )

DC_SET_MODE( IntGdiSetPolyFillMode, jFillMode, ALTERNATE, WINDING )
DC_SET_MODE( IntGdiSetROP2, jROP2, R2_BLACK, R2_WHITE )
DC_SET_MODE( IntGdiSetStretchBltMode, jStretchBltMode, BLACKONWHITE, HALFTONE )



COLORREF FASTCALL
IntGdiSetBkColor(HDC hDC, COLORREF color)
{
  COLORREF oldColor;
  PDC dc;
  PDC_ATTR pdcattr;
  HBRUSH hBrush;

  if (!(dc = DC_LockDc(hDC)))
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return CLR_INVALID;
  }
  pdcattr = dc->pdcattr;
  oldColor = pdcattr->crBackgroundClr;
  pdcattr->crBackgroundClr = color;
  pdcattr->ulBackgroundClr = (ULONG)color;
  pdcattr->ulDirty_ &= ~(DIRTY_BACKGROUND|DIRTY_LINE|DIRTY_FILL); // Clear Flag if set.
  hBrush = pdcattr->hbrush;
  DC_UnlockDc(dc);
  NtGdiSelectBrush(hDC, hBrush);
  return oldColor;
}

INT FASTCALL
IntGdiSetBkMode(HDC hDC, INT Mode)
{
  COLORREF oldMode;
  PDC dc;
  PDC_ATTR pdcattr;

  if (!(dc = DC_LockDc(hDC)))
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return CLR_INVALID;
  }
  pdcattr = dc->pdcattr;
  oldMode = pdcattr->lBkMode;
  pdcattr->jBkMode = Mode;
  pdcattr->lBkMode = Mode;
  DC_UnlockDc(dc);
  return oldMode;
}

UINT
FASTCALL
IntGdiSetTextAlign(HDC  hDC,
                       UINT  Mode)
{
  UINT prevAlign;
  DC *dc;
  PDC_ATTR pdcattr;

  dc = DC_LockDc(hDC);
  if (!dc)
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return GDI_ERROR;
    }
  pdcattr = dc->pdcattr;
  prevAlign = pdcattr->lTextAlign;
  pdcattr->lTextAlign = Mode;
  DC_UnlockDc( dc );
  return  prevAlign;
}

COLORREF
FASTCALL
IntGdiSetTextColor(HDC hDC,
                 COLORREF color)
{
  COLORREF  oldColor;
  PDC  dc = DC_LockDc(hDC);
  PDC_ATTR pdcattr;
  HBRUSH hBrush;

  if (!dc)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return CLR_INVALID;
  }
  pdcattr = dc->pdcattr;

  oldColor = pdcattr->crForegroundClr;
  pdcattr->crForegroundClr = color;
  hBrush = pdcattr->hbrush;
  pdcattr->ulDirty_ &= ~(DIRTY_TEXT|DIRTY_LINE|DIRTY_FILL);
  DC_UnlockDc( dc );
  NtGdiSelectBrush(hDC, hBrush);
  return  oldColor;
}

VOID
FASTCALL
DCU_SetDcUndeletable(HDC  hDC)
{
  PDC dc = DC_LockDc(hDC);
  if (!dc)
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return;
    }

  dc->fs |= DC_FLAG_PERMANENT;
  DC_UnlockDc( dc );
  return;
}

#if 0
BOOL FASTCALL
IntIsPrimarySurface(SURFOBJ *SurfObj)
{
   if (PrimarySurface.pSurface == NULL)
     {
       return FALSE;
     }
   return SurfObj->hsurf == PrimarySurface.pSurface; // <- FIXME: WTF?
}
#endif

// FIXME: remove me
HDC FASTCALL
DC_GetNextDC (PDC pDC)
{
  return pDC->hdcNext;
}

VOID FASTCALL
DC_SetNextDC (PDC pDC, HDC hNextDC)
{
  pDC->hdcNext = hNextDC;
}


BOOL APIENTRY
NtGdiCancelDC(HDC  hDC)
{
  UNIMPLEMENTED;
  return FALSE;
}




WORD APIENTRY
IntGdiSetHookFlags(HDC hDC, WORD Flags)
{
  WORD wRet;
  DC *dc = DC_LockDc(hDC);

  if (NULL == dc)
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return 0;
    }

  wRet = dc->fs & DC_FLAG_DIRTY_RAO; // Fixme wrong flag!

  /* "Undocumented Windows" info is slightly confusing.
   */

  DPRINT("DC %p, Flags %04x\n", hDC, Flags);

  if (Flags & DCHF_INVALIDATEVISRGN)
    { /* hVisRgn has to be updated */
      dc->fs |= DC_FLAG_DIRTY_RAO;
    }
  else if (Flags & DCHF_VALIDATEVISRGN || 0 == Flags)
    {
      dc->fs &= ~DC_FLAG_DIRTY_RAO;
    }

  DC_UnlockDc(dc);

  return wRet;
}


BOOL
APIENTRY
NtGdiGetDCDword(
             HDC hDC,
             UINT u,
             DWORD *Result
               )
{
  BOOL Ret = TRUE;
  PDC dc;
  PDC_ATTR pdcattr;

  DWORD SafeResult = 0;
  NTSTATUS Status = STATUS_SUCCESS;

  if(!Result)
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  dc = DC_LockDc(hDC);
  if(!dc)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  pdcattr = dc->pdcattr;

  switch (u)
  {
    case GdiGetJournal:
      break;
    case GdiGetRelAbs:
      SafeResult = pdcattr->lRelAbs;
      break;
    case GdiGetBreakExtra:
      SafeResult = pdcattr->lBreakExtra;
      break;
    case GdiGerCharBreak:
      SafeResult = pdcattr->cBreak;
      break;
    case GdiGetArcDirection:
      if (pdcattr->dwLayout & LAYOUT_RTL)
          SafeResult = AD_CLOCKWISE - ((dc->dclevel.flPath & DCPATH_CLOCKWISE) != 0);
      else
          SafeResult = ((dc->dclevel.flPath & DCPATH_CLOCKWISE) != 0) + AD_COUNTERCLOCKWISE;
      break;
    case GdiGetEMFRestorDc:
      break;
    case GdiGetFontLanguageInfo:
          SafeResult = IntGetFontLanguageInfo(dc);
      break;
    case GdiGetIsMemDc:
          SafeResult = dc->dctype;
      break;
    case GdiGetMapMode:
      SafeResult = pdcattr->iMapMode;
      break;
    case GdiGetTextCharExtra:
      SafeResult = pdcattr->lTextExtra;
      break;
    default:
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      Ret = FALSE;
      break;
  }

  if (Ret)
  {
    _SEH2_TRY
    {
      ProbeForWrite(Result,
                    sizeof(DWORD),
                    1);
      *Result = SafeResult;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
      Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
  }

  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    DC_UnlockDc(dc);
    return FALSE;
  }

  DC_UnlockDc(dc);
  return Ret;
}

BOOL
APIENTRY
NtGdiGetAndSetDCDword(
                  HDC hDC,
                  UINT u,
                  DWORD dwIn,
                  DWORD *Result
                     )
{
  BOOL Ret = TRUE;
  PDC dc;
  PDC_ATTR pdcattr;

  DWORD SafeResult = 0;
  NTSTATUS Status = STATUS_SUCCESS;

  if(!Result)
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  dc = DC_LockDc(hDC);
  if(!dc)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  pdcattr = dc->pdcattr;

  switch (u)
  {
    case GdiGetSetCopyCount:
      SafeResult = dc->ulCopyCount;
      dc->ulCopyCount = dwIn;
      break;
    case GdiGetSetTextAlign:
      SafeResult = pdcattr->lTextAlign;
      pdcattr->lTextAlign = dwIn;
      // pdcattr->flTextAlign = dwIn; // Flags!
      break;
    case GdiGetSetRelAbs:
      SafeResult = pdcattr->lRelAbs;
      pdcattr->lRelAbs = dwIn;
      break;
    case GdiGetSetTextCharExtra:
      SafeResult = pdcattr->lTextExtra;
      pdcattr->lTextExtra = dwIn;
      break;
    case GdiGetSetSelectFont:
      break;
    case GdiGetSetMapperFlagsInternal:
      if (dwIn & ~1)
      {
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         Ret = FALSE;
         break;
      }
      SafeResult = pdcattr->flFontMapper;
      pdcattr->flFontMapper = dwIn;
      break;
    case GdiGetSetMapMode:
      SafeResult = IntGdiSetMapMode( dc, dwIn);
      break;
    case GdiGetSetArcDirection:
      if (dwIn != AD_COUNTERCLOCKWISE && dwIn != AD_CLOCKWISE)
      {
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         Ret = FALSE;
         break;
      }
      if ( pdcattr->dwLayout & LAYOUT_RTL ) // Right to Left
      {
         SafeResult = AD_CLOCKWISE - ((dc->dclevel.flPath & DCPATH_CLOCKWISE) != 0);
         if ( dwIn == AD_CLOCKWISE )
         {
            dc->dclevel.flPath &= ~DCPATH_CLOCKWISE;
            break;
         }
         dc->dclevel.flPath |= DCPATH_CLOCKWISE;
      }
      else // Left to Right
      {
         SafeResult = ((dc->dclevel.flPath & DCPATH_CLOCKWISE) != 0) + AD_COUNTERCLOCKWISE;
         if ( dwIn == AD_COUNTERCLOCKWISE)
         {
            dc->dclevel.flPath &= ~DCPATH_CLOCKWISE;
            break;
         }
         dc->dclevel.flPath |= DCPATH_CLOCKWISE;
      }
      break;
    default:
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      Ret = FALSE;
      break;
  }

  if (Ret)
  {
    _SEH2_TRY
    {
      ProbeForWrite(Result,
                    sizeof(DWORD),
                    1);
      *Result = SafeResult;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
      Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;
  }

  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    DC_UnlockDc(dc);
    return FALSE;
  }

  DC_UnlockDc(dc);
  return Ret;
}
