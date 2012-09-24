/*
 * PROJECT:         ReactOS
 * LICENSE:         GNU LGPL by FSF v2.1 or any later
 * FILE:            dll/win32/winent.drv/bitblt.c
 * PURPOSE:         GDI driver stub for ReactOS/Windows
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ***************************************************************/


#include "winent.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(bitblt);

/* FUNCTIONS **************************************************************/

BOOL CDECL RosDrv_PatBlt( PDC_ATTR pdcattr, INT left, INT top, INT width, INT height, DWORD rop )
{
    POINT pts[2], ptBrush;

    /* Map coordinates */
    pts[0].x = left;
    pts[0].y = top;
    pts[1].x = left + width;
    pts[1].y = top + height;

    LPtoDP(pdcattr->hdc, pts, 2);
    width = pts[1].x - pts[0].x;
    height = pts[1].y - pts[0].y;
    left = pts[0].x + pdcattr->dc_rect.left;
    top = pts[0].y + pdcattr->dc_rect.top;

    /* Update brush origin */
    GetBrushOrgEx(pdcattr->hdc, &ptBrush);
    ptBrush.x += pdcattr->dc_rect.left;
    ptBrush.y += pdcattr->dc_rect.top;
    RosGdiSetBrushOrg(pdcattr->hKernelDC, ptBrush.x, ptBrush.y);

    return RosGdiPatBlt(pdcattr->hKernelDC, left, top, width, height, rop);
}

BOOL CDECL RosDrv_BitBlt( PDC_ATTR pdcattrDst, INT xDst, INT yDst,
                    INT width, INT height, PDC_ATTR pdcattrSrc,
                    INT xSrc, INT ySrc, DWORD rop )
{
    BOOL useSrc;
    POINT pts[2], ptBrush;

    useSrc = (((rop >> 2) & 0x330000) != (rop & 0x330000));
    if (!pdcattrSrc && useSrc) return FALSE;

    /* Forward to PatBlt if src dev is missing */
    if (!pdcattrSrc)
        return RosDrv_PatBlt(pdcattrDst, xDst, yDst, width, height, rop);

    /* map source coordinates */
    if (pdcattrSrc)
    {
        pts[0].x = xSrc;
        pts[0].y = ySrc;
        pts[1].x = xSrc + width;
        pts[1].y = ySrc + height;

        LPtoDP(pdcattrSrc->hdc, pts, 2);
        width = pts[1].x - pts[0].x;
        height = pts[1].y - pts[0].y;
        xSrc = pts[0].x + pdcattrSrc->dc_rect.left;
        ySrc = pts[0].y + pdcattrSrc->dc_rect.top;
    }

    /* map dest coordinates */
    pts[0].x = xDst;
    pts[0].y = yDst;
    LPtoDP(pdcattrDst->hdc, pts, 1);
    xDst = pts[0].x + pdcattrDst->dc_rect.left;
    yDst = pts[0].y + pdcattrDst->dc_rect.top;

    /* Update brush origin */
    GetBrushOrgEx(pdcattrDst->hdc, &ptBrush);
    ptBrush.x += pdcattrDst->dc_rect.left;
    ptBrush.y += pdcattrDst->dc_rect.top;
    RosGdiSetBrushOrg(pdcattrDst->hKernelDC, ptBrush.x, ptBrush.y);

    //FIXME("xDst %d, yDst %d, widthDst %d, heightDst %d, src x %d y %d\n",
    //    xDst, yDst, width, height, xSrc, ySrc);

    return RosGdiBitBlt(pdcattrDst->hKernelDC, xDst, yDst, width, height,
        pdcattrSrc->hKernelDC, xSrc, ySrc, rop);
}

BOOL CDECL RosDrv_StretchBlt( PDC_ATTR pdcattrDst, INT xDst, INT yDst,
                              INT widthDst, INT heightDst,
                              PDC_ATTR pdcattrSrc, INT xSrc, INT ySrc,
                              INT widthSrc, INT heightSrc, DWORD rop )
{
    BOOL useSrc;
    POINT pts[2], ptBrush;

    useSrc = (((rop >> 2) & 0x330000) != (rop & 0x330000));
    if (!pdcattrSrc && useSrc) return FALSE;

    /* map source coordinates */
    if (pdcattrSrc)
    {
        pts[0].x = xSrc;
        pts[0].y = ySrc;
        pts[1].x = xSrc + widthSrc;
        pts[1].y = ySrc + heightSrc;

        LPtoDP(pdcattrSrc->hdc, pts, 2);
        widthSrc = pts[1].x - pts[0].x;
        heightSrc = pts[1].y - pts[0].y;
        xSrc = pts[0].x + pdcattrSrc->dc_rect.left;
        ySrc = pts[0].y + pdcattrSrc->dc_rect.top;
    }

    /* map dest coordinates */
    pts[0].x = xDst;
    pts[0].y = yDst;
    pts[1].x = xDst + widthDst;
    pts[1].y = yDst + heightDst;
    LPtoDP(pdcattrDst->hdc, pts, 2);
    widthDst = pts[1].x - pts[0].x;
    heightDst = pts[1].y - pts[0].y;
    xDst = pts[0].x + pdcattrDst->dc_rect.left;
    yDst = pts[0].y + pdcattrDst->dc_rect.top;

    /* Update brush origin */
    GetBrushOrgEx(pdcattrDst->hdc, &ptBrush);
    ptBrush.x += pdcattrDst->dc_rect.left;
    ptBrush.y += pdcattrDst->dc_rect.top;
    RosGdiSetBrushOrg(pdcattrDst->hKernelDC, ptBrush.x, ptBrush.y);

    return RosGdiStretchBlt(pdcattrDst->hKernelDC, xDst, yDst, widthDst, heightDst,
        pdcattrSrc->hKernelDC, xSrc, ySrc, widthSrc, heightSrc, rop);
}

BOOL CDECL RosDrv_AlphaBlend(PDC_ATTR pdcattrDst, INT xDst, INT yDst, INT widthDst, INT heightDst,
                             PDC_ATTR pdcattrSrc, INT xSrc, INT ySrc, INT widthSrc, INT heightSrc,
                             BLENDFUNCTION blendfn)
{
    POINT pts[2], ptBrush;

    /* map source coordinates */
    if (pdcattrSrc)
    {
        pts[0].x = xSrc;
        pts[0].y = ySrc;
        pts[1].x = xSrc + widthSrc;
        pts[1].y = ySrc + heightSrc;

        LPtoDP(pdcattrSrc->hdc, pts, 2);
        widthSrc = pts[1].x - pts[0].x;
        heightSrc = pts[1].y - pts[0].y;
        xSrc = pts[0].x + pdcattrSrc->dc_rect.left;
        ySrc = pts[0].y + pdcattrSrc->dc_rect.top;
    }

    /* map dest coordinates */
    pts[0].x = xDst;
    pts[0].y = yDst;
    pts[1].x = xDst + widthDst;
    pts[1].y = yDst + heightDst;

    LPtoDP(pdcattrDst->hdc, pts, 2);
    widthDst = pts[1].x - pts[0].x;
    heightDst = pts[1].y - pts[0].y;
    xDst = pts[0].x + pdcattrDst->dc_rect.left;
    yDst = pts[0].y + pdcattrDst->dc_rect.top;

    /* Update brush origin */
    GetBrushOrgEx(pdcattrDst->hdc, &ptBrush);
    ptBrush.x += pdcattrDst->dc_rect.left;
    ptBrush.y += pdcattrDst->dc_rect.top;
    RosGdiSetBrushOrg(pdcattrDst->hKernelDC, ptBrush.x, ptBrush.y);

    return RosGdiAlphaBlend(pdcattrDst->hKernelDC, xDst, yDst, widthDst, heightDst,
        pdcattrSrc->hKernelDC, xSrc, ySrc, widthSrc, heightSrc, blendfn);

}


/* EOF */
