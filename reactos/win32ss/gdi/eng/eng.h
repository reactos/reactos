#pragma once

VOID
NTAPI
EngAcquireSemaphoreShared(
    IN HSEMAPHORE hsem);

BOOL APIENTRY
IntEngMaskBlt(SURFOBJ *psoDest,
              SURFOBJ *psoMask,
              CLIPOBJ *ClipRegion,
              XLATEOBJ *DestColorTranslation,
              XLATEOBJ *SourceColorTranslation,
              RECTL *DestRect,
              POINTL *pptlMask,
              BRUSHOBJ *pbo,
              POINTL *BrushOrigin);

VOID FASTCALL
IntEngWindowChanged(
        PWND Window,
        FLONG flChanged);

VOID FASTCALL IntGdiAcquireSemaphore ( HSEMAPHORE hsem );
VOID FASTCALL IntGdiReleaseSemaphore ( HSEMAPHORE hsem );
ULONGLONG APIENTRY EngGetTickCount(VOID);

VOID DecompressBitmap(SIZEL Size, BYTE *CompressedBits, BYTE *UncompressedBits, LONG Delta, ULONG iFormat);
