#ifndef _WIN32K_ENG_H
#define _WIN32K_ENG_H

BOOL APIENTRY  EngIntersectRect (PRECTL prcDst, PRECTL prcSrc1, PRECTL prcSrc2);
VOID FASTCALL EngDeleteXlate (XLATEOBJ *XlateObj);
BOOL APIENTRY
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

VOID FASTCALL IntGdiAcquireSemaphore ( HSEMAPHORE hsem );
VOID FASTCALL IntGdiReleaseSemaphore ( HSEMAPHORE hsem );
ULONGLONG APIENTRY EngGetTickCount(VOID);

#endif /* _WIN32K_ENG_H */
