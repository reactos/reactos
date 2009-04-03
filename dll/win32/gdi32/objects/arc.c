#include "precomp.h"

BOOL
WINAPI
Arc(
	HDC hDC,
	int nLeftRect,
	int nTopRect,
	int nRightRect,
	int nBottomRect,
	int nXStartArc,
	int nYStartArc,
	int nXEndArc,
	int nYEndArc
)
{
#if 0
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
#endif
	return NtGdiArcInternal(GdiTypeArc,
	                        hDC,
	                        nLeftRect,
	                        nTopRect,
	                        nRightRect,
	                        nBottomRect,
	                        nXStartArc,
	                        nYStartArc,
	                        nXEndArc,
	                        nYEndArc);
}


/*
 * @implemented
 */
BOOL
WINAPI
AngleArc(HDC   hDC,
         int   X,
         int   Y,
         DWORD Radius,
         FLOAT StartAngle,
         FLOAT SweepAngle)
{
  gxf_long worker, worker1;

  worker.f  = StartAngle;
  worker1.f = SweepAngle;

#if 0
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return FALSE; //No meta support for AngleArc
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
        BOOL Ret = EMFDRV_AngleArc( hDC, X, Y, Radius, StartAngle, SweepAngle);
        return Ret;
      }
      return FALSE;
    }
 }
#endif
  return NtGdiAngleArc(hDC, X, Y, Radius, (DWORD)worker.l, (DWORD)worker1.l);
}

BOOL
WINAPI
ArcTo(
	HDC  hDC,
	int  nLeftRect,
	int  nTopRect,
	int  nRightRect,
	int  nBottomRect,
	int  nXRadial1,
	int  nYRadial1,
	int  nXRadial2,
	int  nYRadial2)
{
#if 0
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
#endif
	return NtGdiArcInternal(GdiTypeArcTo,
	                        hDC,
	                        nLeftRect,
	                        nTopRect,
	                        nRightRect,
	                        nBottomRect,
	                        nXRadial1,
	                        nYRadial1,
	                        nXRadial2,
	                        nYRadial2);
}

BOOL
WINAPI
Chord(
	HDC  hDC,
	int  nLeftRect,
	int  nTopRect,
	int  nRightRect,
	int  nBottomRect,
	int  nXRadial1,
	int  nYRadial1,
	int  nXRadial2,
	int  nYRadial2)
{
#if 0
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
#endif
	return NtGdiArcInternal(GdiTypeChord,
	                        hDC,
	                        nLeftRect,
	                        nTopRect,
	                        nRightRect,
	                        nBottomRect,
	                        nXRadial1,
	                        nYRadial1,
	                        nXRadial2,
	                        nYRadial2);
}


/*
 * @unimplemented
 */
BOOL
WINAPI
Pie(
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
#if 0
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
#endif
 return NtGdiArcInternal(GdiTypePie, hDC, a1, a2, a3, a4, a5, a6, a7, a8);
}


