#ifndef __WIN32K_PAINT_H
#define __WIN32K_PAINT_H

BOOL STDCALL FillSolid (PSURFOBJ Surface, PRECTL Dimensions, ULONG iColor);
BOOL STDCALL FillPolygon_ALTERNATE (PSURFOBJ SurfObj, PBRUSHOBJ BrushObj, MIX RopMode, CONST PPOINT Points, INT Count, RECTL BoundRect);
BOOL STDCALL FillPolygon_WINDING (PSURFOBJ SurfObj, PBRUSHOBJ BrushObj, MIX RopMode, CONST PPOINT Points, INT Count, RECTL BoundRect);

#endif /* __WIN32K_PAINT_H */
