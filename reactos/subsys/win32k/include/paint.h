#ifndef _WIN32K_PAINT_H
#define _WIN32K_PAINT_H

BOOL INTERNAL_CALL FillSolid (SURFOBJ* Surface, RECTL* Dimensions, ULONG iColor);
BOOL INTERNAL_CALL FillPolygon ( DC* dc, BITMAPOBJ* SurfObj, BRUSHOBJ* BrushObj, MIX RopMode, CONST PPOINT Points, INT Count, RECTL BoundRect );

#endif /* _WIN32K_PAINT_H */
