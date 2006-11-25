#include "precomp.h"


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
      PDC_ATTR Dc_Attr;
      PLDC pLDC;
      GdiGetHandleUserData((HGDIOBJ) hDC, (PVOID) &Dc_Attr);
      pLDC = Dc_Attr->pvLDC;
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE)
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
      PDC_ATTR Dc_Attr;
      PLDC pLDC;
      GdiGetHandleUserData((HGDIOBJ) hDC, (PVOID) &Dc_Attr);
      pLDC = Dc_Attr->pvLDC;
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE)
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
      PDC_ATTR Dc_Attr;
      PLDC pLDC;
      GdiGetHandleUserData((HGDIOBJ) hDC, (PVOID) &Dc_Attr);
      pLDC = Dc_Attr->pvLDC;
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE)
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
      PDC_ATTR Dc_Attr;
      PLDC pLDC;
      GdiGetHandleUserData((HGDIOBJ) hDC, (PVOID) &Dc_Attr);
      pLDC = Dc_Attr->pvLDC;
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE)
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


