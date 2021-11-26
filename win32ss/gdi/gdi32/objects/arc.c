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
    HANDLE_METADC(BOOL,
                  Arc,
                  FALSE,
                  hdc,
                  xLeft,
                  yTop,
                  xRight,
                  yBottom,
                  xStartArc,
                  yStartArc,
                  xEndArc,
                  yEndArc);

    if ( GdiConvertAndCheckDC(hdc) == NULL ) return FALSE;

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
    HANDLE_EMETAFDC(BOOL,
                  AngleArc,
                  FALSE,
                  hdc,
                  x,
                  y,
                  dwRadius,
                  eStartAngle,
                  eSweepAngle);

    if ( GdiConvertAndCheckDC(hdc) == NULL ) return FALSE;

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
    HANDLE_EMETAFDC(BOOL,
                  ArcTo,
                  FALSE,
                  hdc,
                  xLeft,
                  yTop,
                  xRight,
                  yBottom,
                  xRadial1,
                  yRadial1,
                  xRadial2,
                  yRadial2);

    if ( GdiConvertAndCheckDC(hdc) == NULL ) return FALSE;

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
    HANDLE_METADC(BOOL,
                  Chord,
                  FALSE,
                  hdc,
                  xLeft,
                  yTop,
                  xRight,
                  yBottom,
                  xRadial1,
                  yRadial1,
                  xRadial2,
                  yRadial2);

    if ( GdiConvertAndCheckDC(hdc) == NULL ) return FALSE;

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
    HANDLE_METADC(BOOL,
                  Pie,
                  FALSE,
                  hdc,
                  xLeft,
                  yTop,
                  xRight,
                  yBottom,
                  xRadial1,
                  yRadial1,
                  xRadial2,
                  yRadial2);

    if ( GdiConvertAndCheckDC(hdc) == NULL ) return FALSE;

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


