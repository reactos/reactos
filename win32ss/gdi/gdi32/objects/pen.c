#include <precomp.h>

#define NDEBUG
#include <debug.h>

/*
 * @implemented
 */
HPEN WINAPI
CreatePenIndirect(
    const LOGPEN *lplgpn)
{
    /* Note same behoir as Windows 2000/XP/VISTA, they do not care if  plgpn is NULL´, it will crash */
    return CreatePen(lplgpn->lopnStyle, lplgpn->lopnWidth.x, lplgpn->lopnColor);
}

/*
 * @implemented
 */
HPEN WINAPI
CreatePen(
    int nPenStyle,
    int nWidth,
    COLORREF crColor)
{
/*    HPEN hPen;
    PBRUSH_ATTR Pen_Attr;
*/
    if (nPenStyle < PS_SOLID) nPenStyle = PS_SOLID;
    if (nPenStyle > PS_DASHDOTDOT)
    {
        if (nPenStyle == PS_NULL) return GetStockObject(NULL_PEN);
        if (nPenStyle != PS_INSIDEFRAME) nPenStyle = PS_SOLID;
    }
#if 0
    hPen = hGetPEBHandle(hctPenHandle, nPenStyle);
    if ( nWidth || nPenStyle || !hPen )
    {
       return NtGdiCreatePen(nPenStyle, nWidth, crColor, NULL);
    }

    if ((GdiGetHandleUserData( hPen, GDI_OBJECT_TYPE_PEN, (PVOID) &Pen_Attr)) &&
        ( Pen_Attr != NULL ))
    {
        if ( Pen_Attr->lbColor != crColor)
        {
           Pen_Attr->lbColor = crColor;
           Pen_Attr->AttrFlags |= ATTR_NEW_COLOR;
        }
        return hPen;
    }
    DeleteObject(hPen);
#endif
    return NtGdiCreatePen(nPenStyle, nWidth, crColor, NULL);
}

