#ifndef _WIN32K_INTENG_H
#define _WIN32K_INTENG_H

typedef ULONG HCLIP;

#define ENUM_RECT_LIMIT   50

typedef struct _RECT_ENUM
{
  ULONG c;
  RECTL arcl[ENUM_RECT_LIMIT];
} RECT_ENUM;

typedef struct tagSPAN
{
  LONG Y;
  LONG X;
  ULONG Width;
} SPAN, *PSPAN;

#define ROP_NOOP	0x00AA0029

/* Definitions of IntEngXxx functions */

#define IntEngLockProcessDriverObjs(W32Process) \
  ExAcquireFastMutex(&(W32Process)->DriverObjListLock)

#define IntEngUnLockProcessDriverObjs(W32Process) \
  ExReleaseFastMutex(&(W32Process)->DriverObjListLock)

VOID FASTCALL
IntEngCleanupDriverObjs(struct _EPROCESS *Process,
                        PW32PROCESS Win32Process);


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

XLATEOBJ* FASTCALL
IntEngCreateXlate(USHORT DestPalType,
                  USHORT SourcePalType,
                  HPALETTE PaletteDest,
                  HPALETTE PaletteSource);

XLATEOBJ* FASTCALL
IntEngCreateMonoXlate(USHORT SourcePalType,
                      HPALETTE PaletteDest,
                      HPALETTE PaletteSource,
                      ULONG BackgroundColor);

XLATEOBJ* FASTCALL
IntEngCreateSrcMonoXlate(HPALETTE PaletteDest,
                         ULONG ForegroundColor,
                         ULONG BackgroundColor);
			
BOOL STDCALL
IntEngPolyline(BITMAPOBJ *DestSurf,
               CLIPOBJ *Clip,
               BRUSHOBJ *Brush,
               CONST LPPOINT  pt,
               LONG dCount,
               MIX mix);

CLIPOBJ* FASTCALL
IntEngCreateClipRegion(ULONG count,
                       PRECTL pRect,
                       PRECTL rcBounds);

VOID FASTCALL
IntEngDeleteClipRegion(CLIPOBJ *ClipObj);

BOOLEAN FASTCALL
ClipobjToSpans(PSPAN *Spans,
               UINT *Count,
               CLIPOBJ *ClipRegion,
               PRECTL Boundary);

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
