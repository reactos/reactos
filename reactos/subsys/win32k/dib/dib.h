#ifndef _W32K_DIB_DIB_H
#define _W32K_DIB_DIB_H

typedef VOID (*PFN_DIB_PutPixel)(SURFOBJ*,LONG,LONG,ULONG);
typedef ULONG (*PFN_DIB_GetPixel)(SURFOBJ*,LONG,LONG);
typedef VOID (*PFN_DIB_HLine)(SURFOBJ*,LONG,LONG,LONG,ULONG);
typedef VOID (*PFN_DIB_VLine)(SURFOBJ*,LONG,LONG,LONG,ULONG);
typedef BOOLEAN (*PFN_DIB_BitBlt)(SURFOBJ*,SURFOBJ*,RECTL*,POINTL*,BRUSHOBJ*,POINTL,XLATEOBJ*,ULONG);
typedef BOOLEAN (*PFN_DIB_StretchBlt)(SURFOBJ*,SURFOBJ*,RECTL*,RECTL*,POINTL*,POINTL,CLIPOBJ*,XLATEOBJ*,ULONG);
typedef BOOLEAN (*PFN_DIB_TransparentBlt)(SURFOBJ*,SURFOBJ*,RECTL*,POINTL*,XLATEOBJ*,ULONG);

typedef struct
{
  PFN_DIB_PutPixel       DIB_PutPixel;
  PFN_DIB_GetPixel       DIB_GetPixel;
  PFN_DIB_HLine          DIB_HLine;
  PFN_DIB_VLine          DIB_VLine;
  PFN_DIB_BitBlt         DIB_BitBlt;
  PFN_DIB_StretchBlt     DIB_StretchBlt;
  PFN_DIB_TransparentBlt DIB_TransparentBlt;
} DIB_FUNCTIONS;

extern DIB_FUNCTIONS DibFunctionsForBitmapFormat[];

extern unsigned char notmask[2];
extern unsigned char altnotmask[2];
#define MASK1BPP(x) (1<<(7-((x)&7)))
ULONG   DIB_DoRop(ULONG Rop, ULONG Dest, ULONG Source, ULONG Pattern);
ULONG   DIB_GetSource(SURFOBJ* SourceSurf, ULONG sx, ULONG sy, XLATEOBJ* ColorTranslation);
ULONG   DIB_GetSourceIndex(SURFOBJ* SourceSurf, ULONG sx, ULONG sy);

VOID Dummy_PutPixel(SURFOBJ* SurfObj, LONG x, LONG y, ULONG c);
ULONG Dummy_GetPixel(SURFOBJ* SurfObj, LONG x, LONG y);
VOID Dummy_HLine(SURFOBJ* SurfObj, LONG x1, LONG x2, LONG y, ULONG c);
VOID Dummy_VLine(SURFOBJ* SurfObj, LONG x, LONG y1, LONG y2, ULONG c);
BOOLEAN Dummy_BitBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                     RECTL*  DestRect,  POINTL  *SourcePoint,
                     BRUSHOBJ* BrushObj, POINTL BrushOrign,
                     XLATEOBJ *ColorTranslation, ULONG Rop4);
BOOLEAN Dummy_StretchBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                         RECTL*  DestRect,  RECTL  *SourceRect,
                         POINTL* MaskOrigin, POINTL BrushOrign,
                         CLIPOBJ *ClipRegion, XLATEOBJ *ColorTranslation,
                         ULONG Mode);
BOOLEAN Dummy_TransparentBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                             RECTL*  DestRect,  POINTL  *SourcePoint,
                             XLATEOBJ *ColorTranslation, ULONG iTransColor);

VOID    DIB_1BPP_PutPixel(SURFOBJ* SurfObj, LONG x, LONG y, ULONG c);
ULONG   DIB_1BPP_GetPixel(SURFOBJ* SurfObj, LONG x, LONG y);
VOID    DIB_1BPP_HLine(SURFOBJ* SurfObj, LONG x1, LONG x2, LONG y, ULONG c);
VOID    DIB_1BPP_VLine(SURFOBJ* SurfObj, LONG x, LONG y1, LONG y2, ULONG c);
BOOLEAN DIB_1BPP_BitBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                        RECTL* DestRect, POINTL *SourcePoint,
                        BRUSHOBJ* Brush, POINTL BrushOrigin,
                        XLATEOBJ *ColorTranslation, ULONG Rop4);
BOOLEAN DIB_1BPP_StretchBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                            RECTL* DestRect, RECTL *SourceRect,
                            POINTL* MaskOrigin, POINTL BrushOrigin,
                            CLIPOBJ *ClipRegion, XLATEOBJ *ColorTranslation,
                            ULONG Mode);
BOOLEAN DIB_1BPP_TransparentBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                                RECTL*  DestRect,  POINTL  *SourcePoint,
                                XLATEOBJ *ColorTranslation, ULONG iTransColor);

VOID    DIB_4BPP_PutPixel(SURFOBJ* SurfObj, LONG x, LONG y, ULONG c);
ULONG   DIB_4BPP_GetPixel(SURFOBJ* SurfObj, LONG x, LONG y);
VOID    DIB_4BPP_HLine(SURFOBJ* SurfObj, LONG x1, LONG x2, LONG y, ULONG c);
VOID    DIB_4BPP_VLine(SURFOBJ* SurfObj, LONG x, LONG y1, LONG y2, ULONG c);
BOOLEAN DIB_4BPP_BitBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                        RECTL* DestRect, POINTL *SourcePoint,
                        BRUSHOBJ* Brush, POINTL BrushOrigin,
                        XLATEOBJ *ColorTranslation, ULONG Rop4);
BOOLEAN DIB_4BPP_StretchBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                            RECTL* DestRect, RECTL *SourceRect,
                            POINTL* MaskOrigin, POINTL BrushOrigin,
                            CLIPOBJ *ClipRegion, XLATEOBJ *ColorTranslation,
                            ULONG Mode);
BOOLEAN DIB_4BPP_TransparentBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                                RECTL*  DestRect,  POINTL  *SourcePoint,
                                XLATEOBJ *ColorTranslation, ULONG iTransColor);

VOID    DIB_8BPP_PutPixel(SURFOBJ* SurfObj, LONG x, LONG y, ULONG c);
ULONG   DIB_8BPP_GetPixel(SURFOBJ* SurfObj, LONG x, LONG y);
VOID    DIB_8BPP_HLine(SURFOBJ* SurfObj, LONG x1, LONG x2, LONG y, ULONG c);
VOID    DIB_8BPP_VLine(SURFOBJ* SurfObj, LONG x, LONG y1, LONG y2, ULONG c);
BOOLEAN DIB_8BPP_BitBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                        RECTL* DestRect, POINTL *SourcePoint,
                        BRUSHOBJ* Brush, POINTL BrushOrigin,
                        XLATEOBJ *ColorTranslation, ULONG Rop4);
BOOLEAN DIB_8BPP_StretchBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                            RECTL* DestRect, RECTL *SourceRect,
                            POINTL* MaskOrigin, POINTL BrushOrigin,
                            CLIPOBJ *ClipRegion, XLATEOBJ *ColorTranslation,
                            ULONG Mode);
BOOLEAN DIB_8BPP_TransparentBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                                RECTL*  DestRect,  POINTL  *SourcePoint,
                                XLATEOBJ *ColorTranslation, ULONG iTransColor);

VOID    DIB_16BPP_PutPixel(SURFOBJ* SurfObj, LONG x, LONG y, ULONG c);
ULONG   DIB_16BPP_GetPixel(SURFOBJ* SurfObj, LONG x, LONG y);
VOID    DIB_16BPP_HLine(SURFOBJ* SurfObj, LONG x1, LONG x2, LONG y, ULONG c);
VOID    DIB_16BPP_VLine(SURFOBJ* SurfObj, LONG x, LONG y1, LONG y2, ULONG c);
BOOLEAN DIB_16BPP_BitBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                         RECTL* DestRect, POINTL *SourcePoint,
                         BRUSHOBJ* Brush, POINTL BrushOrigin,
                         XLATEOBJ *ColorTranslation, ULONG Rop4);
BOOLEAN DIB_16BPP_StretchBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                             RECTL* DestRect, RECTL *SourceRect,
                             POINTL* MaskOrigin, POINTL BrushOrigin,
                             CLIPOBJ *ClipRegion, XLATEOBJ *ColorTranslation,
                             ULONG Mode);
BOOLEAN DIB_16BPP_TransparentBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                                 RECTL*  DestRect,  POINTL  *SourcePoint,
                                 XLATEOBJ *ColorTranslation, ULONG iTransColor);

VOID    DIB_24BPP_PutPixel(SURFOBJ* SurfObj, LONG x, LONG y, ULONG c);
ULONG   DIB_24BPP_GetPixel(SURFOBJ* SurfObj, LONG x, LONG y);
VOID    DIB_24BPP_HLine(SURFOBJ* SurfObj, LONG x1, LONG x2, LONG y, ULONG c);
VOID    DIB_24BPP_VLine(SURFOBJ* SurfObj, LONG x, LONG y1, LONG y2, ULONG c);
BOOLEAN DIB_24BPP_BitBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                         RECTL* DestRect, POINTL *SourcePoint,
                         BRUSHOBJ* Brush, POINTL BrushOrigin,
                         XLATEOBJ *ColorTranslation, ULONG Rop4);
BOOLEAN DIB_24BPP_StretchBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                             RECTL* DestRect, RECTL *SourceRect,
                             POINTL* MaskOrigin, POINTL BrushOrigin,
                             CLIPOBJ *ClipRegion, XLATEOBJ *ColorTranslation,
                             ULONG Mode);
BOOLEAN DIB_24BPP_TransparentBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                                 RECTL*  DestRect,  POINTL  *SourcePoint,
                                 XLATEOBJ *ColorTranslation, ULONG iTransColor);

VOID    DIB_32BPP_PutPixel(SURFOBJ* SurfObj, LONG x, LONG y, ULONG c);
ULONG   DIB_32BPP_GetPixel(SURFOBJ* SurfObj, LONG x, LONG y);
VOID    DIB_32BPP_HLine(SURFOBJ* SurfObj, LONG x1, LONG x2, LONG y, ULONG c);
VOID    DIB_32BPP_VLine(SURFOBJ* SurfObj, LONG x, LONG y1, LONG y2, ULONG c);
BOOLEAN DIB_32BPP_BitBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                         RECTL* DestRect, POINTL *SourcePoint,
                         BRUSHOBJ* Brush, POINTL BrushOrigin,
                         XLATEOBJ *ColorTranslation, ULONG Rop4);
BOOLEAN DIB_32BPP_StretchBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                             RECTL* DestRect, RECTL *SourceRect,
                             POINTL* MaskOrigin, POINTL BrushOrigin,
                             CLIPOBJ *ClipRegion, XLATEOBJ *ColorTranslation,
                             ULONG Mode);
BOOLEAN DIB_32BPP_TransparentBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                                 RECTL*  DestRect,  POINTL  *SourcePoint,
                                 XLATEOBJ *ColorTranslation, ULONG iTransColor);

#endif /* _W32K_DIB_DIB_H */

