#include "precomp.h"

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
    /* FIXME Some part need be done in user mode */
    if (nPenStyle > PS_DASHDOTDOT)
    {
       if (nPenStyle == PS_NULL) return GetStockObject(NULL_PEN);
       if (nPenStyle != PS_INSIDEFRAME) nPenStyle = PS_SOLID;
    }
    return NtGdiCreatePen(nPenStyle, nWidth, crColor, NULL);
}

