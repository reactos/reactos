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

BOOL CDECL RosDrv_PatBlt( NTDRV_PDEVICE *physDev, INT left, INT top, INT width, INT height, DWORD rop )
{
    POINT pts[2], ptBrush;

    /* Map coordinates */
    pts[0].x = left;
    pts[0].y = top;
    pts[1].x = left + width;
    pts[1].y = top + height;

    LPtoDP(physDev->hdc, pts, 2);
    width = pts[1].x - pts[0].x;
    height = pts[1].y - pts[0].y;
    left = pts[0].x + physDev->dc_rect.left;
    top = pts[0].y + physDev->dc_rect.top;

    /* Update brush origin */
    GetBrushOrgEx(physDev->hdc, &ptBrush);
    ptBrush.x += physDev->dc_rect.left;
    ptBrush.y += physDev->dc_rect.top;
    RosGdiSetBrushOrg(physDev->hKernelDC, ptBrush.x, ptBrush.y);

    return RosGdiPatBlt(physDev->hKernelDC, left, top, width, height, rop);
}

BOOL CDECL RosDrv_BitBlt( NTDRV_PDEVICE *physDevDst, INT xDst, INT yDst,
                    INT width, INT height, NTDRV_PDEVICE *physDevSrc,
                    INT xSrc, INT ySrc, DWORD rop )
{
    BOOL useSrc;
    POINT pts[2], ptBrush;

    useSrc = (((rop >> 2) & 0x330000) != (rop & 0x330000));
    if (!physDevSrc && useSrc) return FALSE;

    /* Forward to PatBlt if src dev is missing */
    if (!physDevSrc)
        return RosDrv_PatBlt(physDevDst, xDst, yDst, width, height, rop);

    /* map source coordinates */
    if (physDevSrc)
    {
        pts[0].x = xSrc;
        pts[0].y = ySrc;
        pts[1].x = xSrc + width;
        pts[1].y = ySrc + height;

        LPtoDP(physDevSrc->hdc, pts, 2);
        width = pts[1].x - pts[0].x;
        height = pts[1].y - pts[0].y;
        xSrc = pts[0].x + physDevSrc->dc_rect.left;
        ySrc = pts[0].y + physDevSrc->dc_rect.top;
    }

    /* map dest coordinates */
    pts[0].x = xDst;
    pts[0].y = yDst;
    LPtoDP(physDevDst->hdc, pts, 1);
    xDst = pts[0].x + physDevDst->dc_rect.left;
    yDst = pts[0].y + physDevDst->dc_rect.top;

    /* Update brush origin */
    GetBrushOrgEx(physDevDst->hdc, &ptBrush);
    ptBrush.x += physDevDst->dc_rect.left;
    ptBrush.y += physDevDst->dc_rect.top;
    RosGdiSetBrushOrg(physDevDst->hKernelDC, ptBrush.x, ptBrush.y);

    //FIXME("xDst %d, yDst %d, widthDst %d, heightDst %d, src x %d y %d\n",
    //    xDst, yDst, width, height, xSrc, ySrc);

    return RosGdiBitBlt(physDevDst->hKernelDC, xDst, yDst, width, height,
        physDevSrc->hKernelDC, xSrc, ySrc, rop);
}

BOOL CDECL RosDrv_StretchBlt( NTDRV_PDEVICE *physDevDst, INT xDst, INT yDst,
                              INT widthDst, INT heightDst,
                              NTDRV_PDEVICE *physDevSrc, INT xSrc, INT ySrc,
                              INT widthSrc, INT heightSrc, DWORD rop )
{
    BOOL useSrc;
    POINT pts[2], ptBrush;

    useSrc = (((rop >> 2) & 0x330000) != (rop & 0x330000));
    if (!physDevSrc && useSrc) return FALSE;

    /* map source coordinates */
    if (physDevSrc)
    {
        pts[0].x = xSrc;
        pts[0].y = ySrc;
        pts[1].x = xSrc + widthSrc;
        pts[1].y = ySrc + heightSrc;

        LPtoDP(physDevSrc->hdc, pts, 2);
        widthSrc = pts[1].x - pts[0].x;
        heightSrc = pts[1].y - pts[0].y;
        xSrc = pts[0].x + physDevSrc->dc_rect.left;
        ySrc = pts[0].y + physDevSrc->dc_rect.top;
    }

    /* map dest coordinates */
    pts[0].x = xDst;
    pts[0].y = yDst;
    pts[1].x = xDst + widthDst;
    pts[1].y = yDst + heightDst;
    LPtoDP(physDevDst->hdc, pts, 2);
    widthDst = pts[1].x - pts[0].x;
    heightDst = pts[1].y - pts[0].y;
    xDst = pts[0].x + physDevDst->dc_rect.left;
    yDst = pts[0].y + physDevDst->dc_rect.top;

    /* Update brush origin */
    GetBrushOrgEx(physDevDst->hdc, &ptBrush);
    ptBrush.x += physDevDst->dc_rect.left;
    ptBrush.y += physDevDst->dc_rect.top;
    RosGdiSetBrushOrg(physDevDst->hKernelDC, ptBrush.x, ptBrush.y);

    return RosGdiStretchBlt(physDevDst->hKernelDC, xDst, yDst, widthDst, heightDst,
        physDevSrc->hKernelDC, xSrc, ySrc, widthSrc, heightSrc, rop);
}

BOOL CDECL RosDrv_AlphaBlend(NTDRV_PDEVICE *physDevDst, INT xDst, INT yDst, INT widthDst, INT heightDst,
                             NTDRV_PDEVICE *physDevSrc, INT xSrc, INT ySrc, INT widthSrc, INT heightSrc,
                             BLENDFUNCTION blendfn)
{
    POINT pts[2], ptBrush;

    /* map source coordinates */
    if (physDevSrc)
    {
        pts[0].x = xSrc;
        pts[0].y = ySrc;
        pts[1].x = xSrc + widthSrc;
        pts[1].y = ySrc + heightSrc;

        LPtoDP(physDevSrc->hdc, pts, 2);
        widthSrc = pts[1].x - pts[0].x;
        heightSrc = pts[1].y - pts[0].y;
        xSrc = pts[0].x + physDevSrc->dc_rect.left;
        ySrc = pts[0].y + physDevSrc->dc_rect.top;
    }

    /* map dest coordinates */
    pts[0].x = xDst;
    pts[0].y = yDst;
    pts[1].x = xDst + widthDst;
    pts[1].y = yDst + heightDst;

    LPtoDP(physDevDst->hdc, pts, 2);
    widthDst = pts[1].x - pts[0].x;
    heightDst = pts[1].y - pts[0].y;
    xDst = pts[0].x + physDevDst->dc_rect.left;
    yDst = pts[0].y + physDevDst->dc_rect.top;

    /* Update brush origin */
    GetBrushOrgEx(physDevDst->hdc, &ptBrush);
    ptBrush.x += physDevDst->dc_rect.left;
    ptBrush.y += physDevDst->dc_rect.top;
    RosGdiSetBrushOrg(physDevDst->hKernelDC, ptBrush.x, ptBrush.y);

    return RosGdiAlphaBlend(physDevDst->hKernelDC, xDst, yDst, widthDst, heightDst,
        physDevSrc->hKernelDC, xSrc, ySrc, widthSrc, heightSrc, blendfn);

}


/* EOF */
