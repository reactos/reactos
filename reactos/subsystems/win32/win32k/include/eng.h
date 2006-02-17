#ifndef _WIN32K_ENG_H
#define _WIN32K_ENG_H

BOOL STDCALL  EngIntersectRect (PRECTL prcDst, PRECTL prcSrc1, PRECTL prcSrc2);
VOID FASTCALL EngDeleteXlate (XLATEOBJ *XlateObj);
BOOL STDCALL
IntEngMaskBlt(SURFOBJ *DestObj,
             SURFOBJ *Mask,
             CLIPOBJ *ClipRegion,
             XLATEOBJ *DestColorTranslation,
             XLATEOBJ *SourceColorTranslation,
             RECTL *DestRect,
             POINTL *SourcePoint,
             POINTL *MaskOrigin,
             BRUSHOBJ *Brush,
             POINTL *BrushOrigin);

VOID FASTCALL
IntEngWindowChanged(
        PWINDOW_OBJECT Window,
        FLONG flChanged);

#endif /* _WIN32K_ENG_H */
