#include <precomp.h>

BOOL
WINAPI
Arc(
    _In_ HDC hdc,
    _In_ INT xLeft,
    _In_ INT yTop,
    _In_ INT xRight,
    _In_ INT yBottom,
    _In_ INT xStartArc,
    _In_ INT yStartArc,
    _In_ INT xEndArc,
    _In_ INT yEndArc)
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
                            hdc,
                            xLeft,
                            yTop,
                            xRight,
                            yBottom,
                            xStartArc,
                            yStartArc,
                            xEndArc,
                            yEndArc);
}


/*
 * @implemented
 */
BOOL
WINAPI
AngleArc(
    _In_ HDC hdc,
    _In_ INT x,
    _In_ INT y,
    _In_ DWORD dwRadius,
    _In_ FLOAT eStartAngle,
    _In_ FLOAT eSweepAngle)
{
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
    return NtGdiAngleArc(hdc,
                         x,
                         y,
                         dwRadius,
                         RCAST(DWORD, eStartAngle),
                         RCAST(DWORD, eSweepAngle));
}

BOOL
WINAPI
ArcTo(
    _In_ HDC hdc,
    _In_ INT xLeft,
    _In_ INT yTop,
    _In_ INT xRight,
    _In_ INT yBottom,
    _In_ INT xRadial1,
    _In_ INT yRadial1,
    _In_ INT xRadial2,
    _In_ INT yRadial2)
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
                            hdc,
                            xLeft,
                            yTop,
                            xRight,
                            yBottom,
                            xRadial1,
                            yRadial1,
                            xRadial2,
                            yRadial2);
}

BOOL
WINAPI
Chord(
    _In_ HDC hdc,
    _In_ INT xLeft,
    _In_ INT yTop,
    _In_ INT xRight,
    _In_ INT yBottom,
    _In_ INT xRadial1,
    _In_ INT yRadial1,
    _In_ INT xRadial2,
    _In_ INT yRadial2)
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
                            hdc,
                            xLeft,
                            yTop,
                            xRight,
                            yBottom,
                            xRadial1,
                            yRadial1,
                            xRadial2,
                            yRadial2);
}


/*
 * @unimplemented
 */
BOOL
WINAPI
Pie(
    _In_ HDC hdc,
    _In_ INT xLeft,
    _In_ INT yTop,
    _In_ INT xRight,
    _In_ INT yBottom,
    _In_ INT xRadial1,
    _In_ INT yRadial1,
    _In_ INT xRadial2,
    _In_ INT yRadial2)
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
    return NtGdiArcInternal(GdiTypePie,
                            hdc,
                            xLeft,
                            yTop,
                            xRight,
                            yBottom,
                            xRadial1,
                            yRadial1,
                            xRadial2,
                            yRadial2);
}


