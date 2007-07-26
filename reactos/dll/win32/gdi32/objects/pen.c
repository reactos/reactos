#include "precomp.h"

#define NDEBUG
#include <debug.h>

/*
 * @implemented
 */
HPEN WINAPI
CreatePenIndirect(
    const LOGPEN *plgpn)
{
    if (plgpn)
    {
        return NtGdiCreatePen(plgpn->lopnStyle,
        /* FIXME: MSDN says in docs for LOGPEN:
           "If the pointer member is NULL, the pen is one pixel 
           wide on raster devices." Do we need to handle this? */
                              plgpn->lopnWidth.x,
                              plgpn->lopnColor,
                              NULL);
    }
    return NULL;
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
    return NtGdiCreatePen(nPenStyle, nWidth, crColor, NULL);
}

