#ifndef _WIN32K_INTENG_H
#define _WIN32K_INTENG_H

/* Definitions of IntEngXxx functions */

BOOL STDCALL IntEngLineTo(SURFOBJ *Surface,
                                 CLIPOBJ *Clip,
                                 BRUSHOBJ *Brush,
                                 LONG x1,
                                 LONG y1,
                                 LONG x2,
                                 LONG y2,
                                 RECTL *RectBounds,
                                 MIX mix);
BOOL STDCALL IntEngBitBlt(SURFOBJ *DestObj,
	                  SURFOBJ *SourceObj,
	                  SURFOBJ *Mask,
	                  CLIPOBJ *ClipRegion,
	                  XLATEOBJ *ColorTranslation,
	                  RECTL *DestRect,
	                  POINTL *SourcePoint,
	                  POINTL *MaskOrigin,
	                  BRUSHOBJ *Brush,
	                  POINTL *BrushOrigin,
	                  ROP4 rop4);
BOOL STDCALL
IntEngStretchBlt(SURFOBJ *DestObj,
                 SURFOBJ *SourceObj,
                 SURFOBJ *Mask,
                 CLIPOBJ *ClipRegion,
                 XLATEOBJ *ColorTranslation,
                 RECTL *DestRect,
                 RECTL *SourceRect,
                 POINTL *pMaskOrigin,
                 BRUSHOBJ *Brush,
                 POINTL *BrushOrigin,
                 ULONG Mode);

BOOL STDCALL
IntEngGradientFill(
    SURFOBJ  *psoDest,
    CLIPOBJ  *pco,
    XLATEOBJ  *pxlo,
    TRIVERTEX  *pVertex,
    ULONG  nVertex,
    PVOID  pMesh,
    ULONG  nMesh,
    RECTL  *prclExtents,
    POINTL  *pptlDitherOrg,
    ULONG  ulMode);

XLATEOBJ * STDCALL IntEngCreateXlate(USHORT DestPalType,
                            USHORT SourcePalType,
                            HPALETTE PaletteDest,
                            HPALETTE PaletteSource);

XLATEOBJ * STDCALL IntEngCreateMonoXlate(
   USHORT SourcePalType, HPALETTE PaletteDest, HPALETTE PaletteSource,
   ULONG BackgroundColor);
			
BOOL STDCALL IntEngPolyline(SURFOBJ *DestSurf,
	                           CLIPOBJ *Clip,
	                           BRUSHOBJ *Brush,
	                           CONST LPPOINT  pt,
				   LONG dCount,
	                           MIX mix);
CLIPOBJ* STDCALL IntEngCreateClipRegion(ULONG count,
					 PRECTL pRect,
					 RECTL rcBounds);

BOOL FASTCALL
IntEngTransparentBlt(PSURFOBJ Dest,
                     PSURFOBJ Source,
                     PCLIPOBJ Clip,
                     PXLATEOBJ ColorTranslation,
                     PRECTL DestRect,
                     PRECTL SourceRect,
                     ULONG iTransColor,
                     ULONG Reserved);
#endif /* _WIN32K_INTENG_H */
