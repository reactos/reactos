#ifndef __WIN32K_PAINT_H
#define __WIN32K_PAINT_H

BOOL STDCALL FillSolid (SURFOBJ* Surface, RECTL* Dimensions, ULONG iColor);
BOOL STDCALL FillPolygon ( DC* dc, SURFOBJ* SurfObj, BRUSHOBJ* BrushObj, MIX RopMode, CONST PPOINT Points, INT Count, RECTL BoundRect );

#endif /* __WIN32K_PAINT_H */
