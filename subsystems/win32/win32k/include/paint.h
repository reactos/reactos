#ifndef _WIN32K_PAINT_H
#define _WIN32K_PAINT_H

BOOL APIENTRY FillSolid (SURFOBJ* Surface, RECTL* Dimensions, ULONG iColor);
BOOL APIENTRY FillPolygon ( DC* dc, SURFACE* pSurface, BRUSHOBJ* BrushObj, MIX RopMode, CONST PPOINT Points, INT Count, RECTL BoundRect );
BOOL FASTCALL IntFillPolygon(PDC dc, SURFACE *psurf, BRUSHOBJ *BrushObj,
    CONST PPOINT Points, int Count, RECTL DestRect, POINTL *BrushOrigin);

#endif /* _WIN32K_PAINT_H */
