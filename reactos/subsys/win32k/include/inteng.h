#ifndef __WIN32K_INTENG_H
#define __WIN32K_INTENG_H

/* Definitions of IntEngXxx functions */

extern BOOL STDCALL IntEngLineTo(SURFOBJ *Surface,
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

#endif
