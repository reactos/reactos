#ifndef _WIN32K_INTGDI_H
#define _WIN32K_INTGDI_H

/* Brush functions */

HBRUSH FASTCALL
IntGdiCreateBrushIndirect(PLOGBRUSH lb);

HBRUSH FASTCALL
IntGdiCreateDIBPatternBrush(HGLOBAL  hDIBPacked,
                            UINT  ColorSpec);

HBRUSH FASTCALL
IntGdiCreateDIBPatternBrushPt(CONST VOID  *PackedDIB,
                              UINT  Usage);

BOOL FASTCALL
IntPatBlt(DC *dc,
          INT  XLeft,
          INT  YLeft,
          INT  Width,
          INT  Height,
          DWORD  ROP,
          PBRUSHOBJ BrushObj);

/* Pen functions */

HPEN FASTCALL
IntGdiCreatePenIndirect(PLOGPEN lgpn);


#endif /* _WIN32K_INTGDI_H */

