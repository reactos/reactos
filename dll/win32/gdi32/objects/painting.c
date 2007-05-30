#include "precomp.h"

/* the following deal with IEEE single-precision numbers */
#define EXCESS          126L
#define SIGNBIT         0x80000000L
#define SIGN(fp)        ((fp) & SIGNBIT)
#define EXP(fp)         (((fp) >> 23L) & 0xFF)
#define MANT(fp)        ((fp) & 0x7FFFFFL)
#define PACK(s,e,m)     ((s) | ((e) << 23L) | (m))

// Sames as lrintf.
#ifdef __GNUC__
#define FLOAT_TO_INT(in,out)  \
           __asm__ __volatile__ ("fistpl %0" : "=m" (out) : "t" (in) : "st");
#else
#define FLOAT_TO_INT(in,out) \
          __asm fld in \
          __asm fistp out
#endif

LONG
FASTCALL
EFtoF( EFLOAT_S * efp)
{
 long Mant, Exp, Sign = 0;

 if (!efp->lMant) return 0;

 Mant = efp->lMant;
 Exp = efp->lExp;
 Sign = SIGN(Mant);

//// M$ storage emulation
 if( Sign ) Mant = -Mant;
 Mant = ((Mant & 0x3fffffff) >> 7);
 Exp += (EXCESS-1);
////
 Mant = MANT(Mant);
 return PACK(Sign, Exp, Mant);
}

VOID
FASTCALL
FtoEF( EFLOAT_S * efp, FLOATL f)
{
 long Mant, Exp, Sign = 0;
 gxf_long worker;

#ifdef _X86_
 worker.l = f; // It's a float stored in a long.
#else
 worker.f = f;
#endif

 Exp = EXP(worker.l);
 Mant = MANT(worker.l); 
 if (SIGN(worker.l)) Sign = -1;
//// M$ storage emulation
 Mant = ((Mant << 7) | 0x40000000);
 Mant ^= Sign; 
 Mant -= Sign;
 Exp -= (EXCESS-1);
//// 
 efp->lMant = Mant;
 efp->lExp = Exp;
}


VOID FASTCALL
CoordCnvP(MATRIX_S * mx, LPPOINT Point)
{
  FLOAT x, y;
  gxf_long a, b;
  
  x = (FLOAT)Point->x;
  y = (FLOAT)Point->y;

  a.l = EFtoF( &mx->efM11 );
  b.l = EFtoF( &mx->efM21 );
  x = x * a.f + y * b.f + mx->fxDx;

  a.l = EFtoF( &mx->efM12 );
  b.l = EFtoF( &mx->efM22 );
  y = x * a.f + y * b.f + mx->fxDy;

  FLOAT_TO_INT(x, Point->x );
  FLOAT_TO_INT(y, Point->y );
}


BOOL
STDCALL
DPtoLP ( HDC hDC, LPPOINT Points, INT Count )
{
  INT i;
  PDC_ATTR Dc_Attr;
 
  if (!GdiGetHandleUserData((HGDIOBJ) hDC, (PVOID) &Dc_Attr)) return FALSE;

  for ( i = 0; i < Count; i++ )
    CoordCnvP ( &Dc_Attr->mxDevicetoWorld, &Points[i] );
  return TRUE;
}


BOOL
STDCALL
LPtoDP ( HDC hDC, LPPOINT Points, INT Count )
{
  INT i;
  PDC_ATTR Dc_Attr;
 
  if (!GdiGetHandleUserData((HGDIOBJ) hDC, (PVOID) &Dc_Attr)) return FALSE;

  for ( i = 0; i < Count; i++ )
    CoordCnvP ( &Dc_Attr->mxWorldToDevice, &Points[i] );
  return TRUE;
}


// Will move to dc.c
HGDIOBJ
STDCALL
GetDCObject( HDC hDC, INT iType)
{

 if((iType == GDI_OBJECT_TYPE_BRUSH) ||
    (iType == GDI_OBJECT_TYPE_EXTPEN)||
    (iType == GDI_OBJECT_TYPE_PEN)   ||
    (iType == GDI_OBJECT_TYPE_COLORSPACE))
 {
   HGDIOBJ hGO;
   PDC_ATTR Dc_Attr;
   
   if (!GdiGetHandleUserData((HGDIOBJ) hDC, (PVOID) &Dc_Attr)) return NULL;

   switch (iType)
   {
     case GDI_OBJECT_TYPE_BRUSH:
          hGO = Dc_Attr->hbrush;  
          break;
     
     case GDI_OBJECT_TYPE_EXTPEN:
     case GDI_OBJECT_TYPE_PEN:
          hGO = Dc_Attr->hpen;
          break;
     
     case GDI_OBJECT_TYPE_COLORSPACE:
          hGO = Dc_Attr->hColorSpace;
          break;
   }   
   return hGO;
 }  
 return NtGdiGetDCObject( hDC, iType );
}


BOOL
STDCALL
LineTo( HDC hDC, INT x, INT y )
{
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_MetaParam2( hDC, META_LINETO, x, y);
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return MFDRV_LineTo( hDC, x, y )
      }
      return FALSE;
    }
 }
 return NtGdiLineTo( hDC, x, y);
}


BOOL
STDCALL
MoveToEx( HDC hDC, INT x, INT y, LPPOINT Point )
{
 PDC_ATTR Dc_Attr;
 
 if (!GdiGetHandleUserData((HGDIOBJ) hDC, (PVOID) &Dc_Attr)) return FALSE;
 
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_MetaParam2( hDC, META_MOVETO, x, y);
    else
    {
      PLDC pLDC = Dc_Attr->pvLDC;
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        if (!EMFDRV_MoveTo( hDC, x, y)) return FALSE;
      }
    }
 }

 if ( Point )
 {
    if ( Dc_Attr->ulDirty_ & DIRTY_PTLCURRENT ) // Double hit!
    {
       Point->x = Dc_Attr->ptfxCurrent.x; // ret prev before change.
       Point->y = Dc_Attr->ptfxCurrent.y;
       DPtoLP ( hDC, Point, 1);          // reconvert back.
    }
    else
    {
       Point->x = Dc_Attr->ptlCurrent.x;
       Point->y = Dc_Attr->ptlCurrent.y;
    }
 }

 Dc_Attr->ptlCurrent.x = x;
 Dc_Attr->ptlCurrent.y = y;
 
 Dc_Attr->ulDirty_ |= ( DIRTY_PTLCURRENT|DIRTY_STYLESTATE); // Set dirty
 return TRUE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
NewArc(
	HDC	hDC,
	int	a1,
	int	a2,
	int	a3,
	int	a4,
	int	a5,
	int	a6,
	int	a7,
	int	a8
	)
{
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
//    Call Wine (rewrite of) MFDRV_MetaParam8
      return MFDRV_MetaParam8( hDC, META_ARC, a1, a2, a3, a4, a5, a6, a7, a8)
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);      
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
//      Call Wine (rewrite of) EMFDRV_ArcChordPie
        BOOL Ret = EMFDRV_ArcChordPie( hDC, a1, a2, a3, a4, a5, a6, a7, a8, EMR_ARC);
        return Ret;
      }
      return FALSE;  
    } 
 }
 return NtGdiArcInternal(GdiTypeArc, hDC, a1, a2, a3, a4, a5, a6, a7, a8);
}


/*
 * @unimplemented
 */
BOOL
STDCALL
NewArcTo(
	HDC	hDC,
	int	a1,
	int	a2,
	int	a3,
	int	a4,
	int	a5,
	int	a6,
	int	a7,
	int	a8
	)
{
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return FALSE; //No meta support for ArcTo
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        BOOL Ret = EMFDRV_ArcChordPie( hDC, a1, a2, a3, a4, a5, a6, a7, a8, EMR_ARCTO);
        return Ret;
      }
      return FALSE;
    }
 }
 return NtGdiArcInternal(GdiTypeArcTo, hDC, a1, a2, a3, a4, a5, a6, a7, a8);
}


/*
 * @unimplemented
 */
BOOL
STDCALL
Chord(
	HDC	hDC,
	int	a1,
	int	a2,
	int	a3,
	int	a4,
	int	a5,
	int	a6,
	int	a7,
	int	a8
	)
{
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_MetaParam8( hDC, META_CHORD, a1, a2, a3, a4, a5, a6, a7, a8)
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        BOOL Ret = EMFDRV_ArcChordPie( hDC, a1, a2, a3, a4, a5, a6, a7, a8, EMR_CHORD);
        return Ret;
      }
      return FALSE;
    }
 }
 return NtGdiArcInternal(GdiTypeChord, hDC, a1, a2, a3, a4, a5, a6, a7, a8);
}


/*
 * @unimplemented
 */
BOOL
STDCALL
NewPie(
	HDC	hDC,
	int	a1,
	int	a2,
	int	a3,
	int	a4,
	int	a5,
	int	a6,
	int	a7,
	int	a8
	)
{
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_MetaParam8( hDC, META_PIE, a1, a2, a3, a4, a5, a6, a7, a8)
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        BOOL Ret = EMFDRV_ArcChordPie( hDC, a1, a2, a3, a4, a5, a6, a7, a8, EMR_PIE);
        return Ret;
      }
      return FALSE;
    }
 }
 return NtGdiArcInternal(GdiTypePie, hDC, a1, a2, a3, a4, a5, a6, a7, a8);
}


BOOL
STDCALL
Ellipse(HDC hDC, INT Left, INT Top, INT Right, INT Bottom)
{
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_MetaParam4(hDC, META_ELLIPSE, Left, Top, Right, Bottom );
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_Ellipse( hDC, Left, Top, Right, Bottom );
      }
      return FALSE;
    }
 }
 return NtGdiEllipse( hDC, Left, Top, Right, Bottom);
}


BOOL
STDCALL
Rectangle(HDC, INT Left, INT Top, INT Right, INT Bottom)
{
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_MetaParam4(hDC, META_RECTANGLE, Left, Top, Right, Bottom );
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_Rectangle( hDC, Left, Top, Right, Bottom );
      }
      return FALSE;
    }
 }
 return NtGdiRectangle( hDC, Left, Top, Right, Bottom);
}


BOOL
STDCALL
RoundRect(HDC, INT Left, INT Top, INT Right, INT Bottom, 
                                                INT ell_Width, INT ell_Height)
{
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_MetaParam6( hDC, META_ROUNDRECT, Left, Top, Right, Bottom,
                                                      ell_Width, ell_Height  );
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_RoundRect( hDC, Left, Top, Right, Bottom, 
                                                      ell_Width, ell_Height );
      }
      return FALSE;
    }
 }
 return NtGdiRoundRect( hDc, Left, Top, Right, Bottom, ell_Width, ell_Height);
}


COLORREF
STDCALL
GetPixel( HDC hDC, INT x, INT y )
{
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC) return CLR_INVALID;
 if (!GdiIsHandleValid((HGDIOBJ) hDC)) return CLR_INVALID;
 return NtGdiGetPixel( hDC, x, y);
}


COLORREF
STDCALL
SetPixel( HDC hDC, INT x, INT y, COLORREF Color )
{
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_MetaParam4(hDC, META_SETPIXEL, x, y, HIWORD(Color),
                                                              LOWORD(Color));
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return 0;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_SetPixel( hDC, x, y, Color );
      }
      return 0;
    }
 }
 return NtGdiSetPixel( hDC, x, y, Color);
}


BOOL
STDCALL
SetPixelV( HDC hDC, INT x, INT y, COLORREF Color )
{
   COLORREF Cr = SetPixel( hDC, x, y, Color );
   if (Cr) return TRUE;
   return FALSE;
}


BOOL
STDCALL
FillRgn( HDC hDC, HRGN hRgn, HBRUSH hBrush )
{

 if ( (!hRgn) || (!hBrush) ) return FALSE;
 
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_FillRgn( hDC, hRgn, hBrush);
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_FillRgn(( hDC, hRgn, hBrush);
      }
      return FALSE;
    }
 }
 return NtGdiFillRgn( hDC, hRgn, hBrush);
}


BOOL
STDCALL
FrameRgn( HDC hDC, HRGN hRgn, HBRUSH hBrush, INT nWidth, INT nHeight )
{

 if ( (!hRgn) || (!hBrush) ) return FALSE;
 
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_FrameRgn( hDC, hRgn, hBrush, nWidth, nHeight );
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_FrameRgn( hDC, hRgn, hBrush, nWidth, nHeight );
      }
      return FALSE;
    }
 }
 return NtGdiFrameRgn( hDC, hRgn, hBrush, nWidth, nHeight);
}


BOOL
STDCALL
InvertRgn( HDC hDC, HRGN hRgn )
{

 if ( !hRgn ) return FALSE;
 
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_InvertRgn( hDC, HRGN hRgn ); // Use this instead of MFDRV_MetaParam.
    else
    {  
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_PaintInvertRgn( hDC, hRgn, EMR_INVERTRGN );
      }
      return FALSE;
    }
 }
 return NtGdiInvertRgn( hDC, hRgn);
}

BOOL
STDCALL
PaintRgn( HDC hDC, HRGN hRgn )
{
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_PaintRgn( hDC, HRGN hRgn ); // Use this instead of MFDRV_MetaParam.
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_PaintInvertRgn( hDC, hRgn, EMR_PAINTRGN );
      }
      return FALSE;
    }
 }
 // Could just use Dc_Attr->hbrush
 HBRUSH hbrush = (HBRUSH) GetDCObject( hDC, GDI_OBJECT_TYPE_BRUSH);
 
 return NtGdiFillRgn( hDC, hRgn, hBrush);
}


BOOL
STDCALL
PolyBezier(HDC hDC ,const POINT* Point, DWORD cPoints)
{
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
 /*
  * Since MetaFiles don't record Beziers and they don't even record
  * approximations to them using lines.
  */
      return FALSE;
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return FALSE; // Not supported yet.
      }
      return FALSE;
    }
 }
 return NtGdiPolyPolyDraw( hDC , Point, &cPoints, 1, GdiPolyBezier );
}
 

BOOL
STDCALL
PolyBezierTo(HDC hDC, const POINT* Point ,DWORD cPoints)
{
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return FALSE;
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return FALSE; // Not supported yet.
      }
      return FALSE;
    }
 }
 return NtGdiPolyPolyDraw( hDC , Point, &cPoints, 1, GdiPolyBezierTo );
}


BOOL
STDCALL
PolyDraw(HDC hDC, const POINT* Point, const BYTE *lpbTypes, int cCount )
{
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return FALSE;
    else
    { 
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return FALSE; // Not supported yet.
      }
      return FALSE;
    }
 }
 return NtGdiPolyDraw( hDC , Point, lpbTypes, cCount );
}


BOOL
STDCALL
Polygon(HDC hDC, const POINT *Point, int Count)
{
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_Polygon( hDC, Point, Count );
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_Polygon( hDC, Point, Count );
      }
      return FALSE;
    }
 }
 return NtGdiPolyPolyDraw( hDC , Point, &Count, 1, GdiPolygon );
}


BOOL
STDCALL
Polyline(HDC hDC, const POINT *Point, int Count)
{
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_Polyline( hDC, Point, Count );
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_Polyline( hDC, Point, Count );
      }
      return FALSE;
    }
 }
 return NtGdiPolyPolyDraw( hDC , Point, &Count, 1, GdiPolyPolyLine );
}


BOOL
STDCALL
PolylineTo(HDC hDC, const POINT* Point, DWORD Count)
{
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return FALSE;
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return FALSE; // Not supported yet.
      }
      return FALSE;
    }
 }
 return NtGdiPolyPolyDraw( hDC , Point, &Count, 1, GdiPolyLineTo );

}


BOOL
STDCALL
PolyPolygon(HDC hDC, const POINT* Point, const INT* Count, int Polys)
{
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_PolyPolygon( hDC, Point, Count, Polys);
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_PolyPolygon( hDC, Point, Count, Polys );
      }
      return FALSE;
    }
 }
 return NtGdiPolyPolyDraw( hDC , Point, Count, Polys, GdiPolygon );

}


BOOL
STDCALL
PolyPolyline(HDC hDC, const POINT* Point, const DWORD* Counts, DWORD Polys)
{
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return FALSE;
    else
    {
      PLDC pLDC = GdiGetLDC(hDC);
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        return EMFDRV_PolyPolyline(hDC, Point, Counts, Polys);
      }
      return FALSE;
    }
 }
 return NtGdiPolyPolyDraw( hDC , Point, Count, Polys, GdiPolyPolyLine );
}

