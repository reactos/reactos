#ifndef __WIN32K_ENG_H
#define __WIN32K_ENG_H

BOOL EngIntersectRect(PRECTL prcDst, PRECTL prcSrc1, PRECTL prcSrc2);
BOOL EngBitBlt(SURFOBJ *Dest, SURFOBJ *Source,
               SURFOBJ *Mask, CLIPOBJ *ClipRegion,
               XLATEOBJ *ColorTranslation, RECTL *DestRect,
               POINTL *SourcePoint, POINTL *MaskRect,
               BRUSHOBJ *Brush, POINTL *BrushOrigin, ROP4 rop4);

#endif /* __WIN32K_ENG_H */
