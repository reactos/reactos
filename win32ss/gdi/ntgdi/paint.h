#pragma once

BOOL APIENTRY FillSolid (SURFOBJ* Surface, RECTL* Dimensions, ULONG iColor);
BOOL APIENTRY FillPolygon ( DC* dc, SURFACE* pSurface, BRUSHOBJ* BrushObj, MIX RopMode, CONST PPOINT Points, INT Count, RECTL BoundRect );
BOOL FASTCALL IntFillPolygon(PDC dc, SURFACE *psurf, BRUSHOBJ *BrushObj,
    CONST PPOINT Points, int Count, RECTL DestRect, POINTL *BrushOrigin);
BOOL FASTCALL IntPolygon(HDC,POINT *,int);
