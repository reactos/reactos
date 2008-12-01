#ifndef _WIN32K_PAINT_H
#define _WIN32K_PAINT_H

BOOL APIENTRY FillSolid (SURFOBJ* Surface, RECTL* Dimensions, ULONG iColor);
BOOL APIENTRY FillPolygon ( DC* dc, BITMAPOBJ* SurfObj, BRUSHOBJ* BrushObj, MIX RopMode, CONST PPOINT Points, INT Count, RECTL BoundRect );

#endif /* _WIN32K_PAINT_H */
