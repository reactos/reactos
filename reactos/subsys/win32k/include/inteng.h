#ifndef _WIN32K_INTENG_H
#define _WIN32K_INTENG_H

#define ROP_NOOP	0x00AA0029

/* Definitions of IntEngXxx functions */

BOOL STDCALL
IntEngLineTo(BITMAPOBJ *Surface,
             CLIPOBJ *Clip,
             BRUSHOBJ *Brush,
             LONG x1,
             LONG y1,
             LONG x2,
             LONG y2,
             RECTL *RectBounds,
             MIX mix);

BOOL STDCALL
IntEngBitBlt(BITMAPOBJ *DestObj,
	     BITMAPOBJ *SourceObj,
	     BITMAPOBJ *Mask,
	     CLIPOBJ *ClipRegion,
	     XLATEOBJ *ColorTranslation,
	     RECTL *DestRect,
	     POINTL *SourcePoint,
	     POINTL *MaskOrigin,
	     BRUSHOBJ *Brush,
	     POINTL *BrushOrigin,
	     ROP4 rop4);

BOOL STDCALL
IntEngStretchBlt(BITMAPOBJ *DestObj,
                 BITMAPOBJ *SourceObj,
                 BITMAPOBJ *Mask,
                 CLIPOBJ *ClipRegion,
                 XLATEOBJ *ColorTranslation,
                 RECTL *DestRect,
                 RECTL *SourceRect,
                 POINTL *pMaskOrigin,
                 BRUSHOBJ *Brush,
                 POINTL *BrushOrigin,
                 ULONG Mode);

BOOL STDCALL
IntEngGradientFill(BITMAPOBJ *psoDest,
                   CLIPOBJ *pco,
                   XLATEOBJ *pxlo,
                   TRIVERTEX *pVertex,
                   ULONG nVertex,
                   PVOID pMesh,
                   ULONG nMesh,
                   RECTL *prclExtents,
                   POINTL *pptlDitherOrg,
                   ULONG ulMode);

XLATEOBJ * STDCALL
IntEngCreateXlate(USHORT DestPalType,
                  USHORT SourcePalType,
                  HPALETTE PaletteDest,
                  HPALETTE PaletteSource);

XLATEOBJ * STDCALL
IntEngCreateMonoXlate(USHORT SourcePalType,
                      HPALETTE PaletteDest,
                      HPALETTE PaletteSource,
                      ULONG BackgroundColor);
			
BOOL STDCALL
IntEngPolyline(BITMAPOBJ *DestSurf,
               CLIPOBJ *Clip,
               BRUSHOBJ *Brush,
               CONST LPPOINT  pt,
               LONG dCount,
               MIX mix);

CLIPOBJ* STDCALL
IntEngCreateClipRegion(ULONG count,
                       PRECTL pRect,
                       PRECTL rcBounds);

BOOL FASTCALL
IntEngTransparentBlt(BITMAPOBJ *Dest,
                     BITMAPOBJ *Source,
                     CLIPOBJ *Clip,
                     XLATEOBJ *ColorTranslation,
                     PRECTL DestRect,
                     PRECTL SourceRect,
                     ULONG iTransColor,
                     ULONG Reserved);

BOOL STDCALL
IntEngPaint(IN BITMAPOBJ *Surface,
            IN CLIPOBJ *ClipRegion,
            IN BRUSHOBJ *Brush,
            IN POINTL *BrushOrigin,
            IN MIX Mix);

#endif /* _WIN32K_INTENG_H */
