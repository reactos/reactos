#ifndef __WIN32K_COPYBITS_H
#define __WIN32K_COPYBITS_H

BOOLEAN STDCALL CopyBitsCopy(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                     SURFGDI *DestGDI,  SURFGDI *SourceGDI,
                     PRECTL  DestRect,  POINTL  *SourcePoint,
                     ULONG   Delta,     XLATEOBJ *ColorTranslation);

#endif /* __WIN32K_COPYBITS_H */
