extern unsigned char notmask[2];
extern unsigned char altnotmask[2];
#define MASK1BPP(x) (1<<(7-((x)&7)))
ULONG   DIB_DoRop(ULONG Rop, ULONG Dest, ULONG Source, ULONG Pattern);
ULONG   DIB_GetSource(SURFOBJ* SourceSurf, SURFGDI* SourceGDI, ULONG sx, ULONG sy, XLATEOBJ* ColorTranslation);
ULONG   DIB_GetSourceIndex(SURFOBJ* SourceSurf, SURFGDI* SourceGDI, ULONG sx, ULONG sy);

VOID    DIB_1BPP_PutPixel(SURFOBJ* SurfObj, LONG x, LONG y, ULONG c);
ULONG   DIB_1BPP_GetPixel(SURFOBJ* SurfObj, LONG x, LONG y);
VOID    DIB_1BPP_HLine(SURFOBJ* SurfObj, LONG x1, LONG x2, LONG y, ULONG c);
VOID    DIB_1BPP_VLine(SURFOBJ* SurfObj, LONG x, LONG y1, LONG y2, ULONG c);
BOOLEAN DIB_1BPP_BitBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                        SURFGDI *DestGDI, SURFGDI *SourceGDI,
                        RECTL* DestRect, POINTL *SourcePoint,
          			    BRUSHOBJ* Brush, POINTL* BrushOrigin,
			            XLATEOBJ *ColorTranslation, ULONG Rop4);
BOOLEAN DIB_1BPP_StretchBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                            SURFGDI *DestGDI, SURFGDI *SourceGDI,
                            RECTL* DestRect, RECTL *SourceRect,
                            POINTL* MaskOrigin, POINTL* BrushOrigin,
			                XLATEOBJ *ColorTranslation, ULONG Mode);
BOOLEAN DIB_1BPP_TransparentBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                                PSURFGDI DestGDI,  PSURFGDI SourceGDI,
                                RECTL*  DestRect,  POINTL  *SourcePoint,
                                XLATEOBJ *ColorTranslation, ULONG iTransColor);

VOID    DIB_4BPP_PutPixel(SURFOBJ* SurfObj, LONG x, LONG y, ULONG c);
ULONG   DIB_4BPP_GetPixel(SURFOBJ* SurfObj, LONG x, LONG y);
VOID    DIB_4BPP_HLine(SURFOBJ* SurfObj, LONG x1, LONG x2, LONG y, ULONG c);
VOID    DIB_4BPP_VLine(SURFOBJ* SurfObj, LONG x, LONG y1, LONG y2, ULONG c);
BOOLEAN DIB_4BPP_BitBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                        SURFGDI *DestGDI, SURFGDI *SourceGDI,
                        RECTL* DestRect, POINTL *SourcePoint,
			            BRUSHOBJ* Brush, POINTL* BrushOrigin,
                        XLATEOBJ *ColorTranslation, ULONG Rop4);
BOOLEAN DIB_4BPP_StretchBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                            SURFGDI *DestGDI, SURFGDI *SourceGDI,
                            RECTL* DestRect, RECTL *SourceRect,
                            POINTL* MaskOrigin, POINTL* BrushOrigin,
			                XLATEOBJ *ColorTranslation, ULONG Mode);
BOOLEAN DIB_4BPP_TransparentBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                                PSURFGDI DestGDI,  PSURFGDI SourceGDI,
                                RECTL*  DestRect,  POINTL  *SourcePoint,
                                XLATEOBJ *ColorTranslation, ULONG iTransColor);

VOID    DIB_8BPP_PutPixel(SURFOBJ* SurfObj, LONG x, LONG y, ULONG c);
ULONG   DIB_8BPP_GetPixel(SURFOBJ* SurfObj, LONG x, LONG y);
VOID    DIB_8BPP_HLine(SURFOBJ* SurfObj, LONG x1, LONG x2, LONG y, ULONG c);
VOID    DIB_8BPP_VLine(SURFOBJ* SurfObj, LONG x, LONG y1, LONG y2, ULONG c);
BOOLEAN DIB_8BPP_BitBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                        SURFGDI *DestGDI, SURFGDI *SourceGDI,
                        RECTL* DestRect, POINTL *SourcePoint,
			            BRUSHOBJ* Brush, POINTL* BrushOrigin,
			            XLATEOBJ *ColorTranslation, ULONG Rop4);
BOOLEAN DIB_8BPP_StretchBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                            SURFGDI *DestGDI, SURFGDI *SourceGDI,
                            RECTL* DestRect, RECTL *SourceRect,
                            POINTL* MaskOrigin, POINTL* BrushOrigin,
			                XLATEOBJ *ColorTranslation, ULONG Mode);
BOOLEAN DIB_8BPP_TransparentBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                                PSURFGDI DestGDI,  PSURFGDI SourceGDI,
                                RECTL*  DestRect,  POINTL  *SourcePoint,
                                XLATEOBJ *ColorTranslation, ULONG iTransColor);

VOID    DIB_16BPP_PutPixel(SURFOBJ* SurfObj, LONG x, LONG y, ULONG c);
ULONG   DIB_16BPP_GetPixel(SURFOBJ* SurfObj, LONG x, LONG y);
VOID    DIB_16BPP_HLine(SURFOBJ* SurfObj, LONG x1, LONG x2, LONG y, ULONG c);
VOID    DIB_16BPP_VLine(SURFOBJ* SurfObj, LONG x, LONG y1, LONG y2, ULONG c);
BOOLEAN DIB_16BPP_BitBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                         SURFGDI *DestGDI, SURFGDI *SourceGDI,
                         RECTL* DestRect, POINTL *SourcePoint,
			             BRUSHOBJ* Brush, POINTL* BrushOrigin,
			             XLATEOBJ *ColorTranslation, ULONG Rop4);
BOOLEAN DIB_16BPP_StretchBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                            SURFGDI *DestGDI, SURFGDI *SourceGDI,
                            RECTL* DestRect, RECTL *SourceRect,
                            POINTL* MaskOrigin, POINTL* BrushOrigin,
			                XLATEOBJ *ColorTranslation, ULONG Mode);
BOOLEAN DIB_16BPP_TransparentBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                                 PSURFGDI DestGDI,  PSURFGDI SourceGDI,
                                 RECTL*  DestRect,  POINTL  *SourcePoint,
                                 XLATEOBJ *ColorTranslation, ULONG iTransColor);

VOID    DIB_24BPP_PutPixel(SURFOBJ* SurfObj, LONG x, LONG y, ULONG c);
ULONG   DIB_24BPP_GetPixel(SURFOBJ* SurfObj, LONG x, LONG y);
VOID    DIB_24BPP_HLine(SURFOBJ* SurfObj, LONG x1, LONG x2, LONG y, ULONG c);
VOID    DIB_24BPP_VLine(SURFOBJ* SurfObj, LONG x, LONG y1, LONG y2, ULONG c);
BOOLEAN DIB_24BPP_BitBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                         SURFGDI *DestGDI, SURFGDI *SourceGDI,
                         RECTL* DestRect, POINTL *SourcePoint,
			             BRUSHOBJ* Brush, POINTL* BrushOrigin,
			             XLATEOBJ *ColorTranslation, ULONG Rop4);
BOOLEAN DIB_24BPP_StretchBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                            SURFGDI *DestGDI, SURFGDI *SourceGDI,
                            RECTL* DestRect, RECTL *SourceRect,
                            POINTL* MaskOrigin, POINTL* BrushOrigin,
			                XLATEOBJ *ColorTranslation, ULONG Mode);
BOOLEAN DIB_24BPP_TransparentBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                                 PSURFGDI DestGDI,  PSURFGDI SourceGDI,
                                 RECTL*  DestRect,  POINTL  *SourcePoint,
                                 XLATEOBJ *ColorTranslation, ULONG iTransColor);

VOID    DIB_32BPP_PutPixel(SURFOBJ* SurfObj, LONG x, LONG y, ULONG c);
ULONG   DIB_32BPP_GetPixel(SURFOBJ* SurfObj, LONG x, LONG y);
VOID    DIB_32BPP_HLine(SURFOBJ* SurfObj, LONG x1, LONG x2, LONG y, ULONG c);
VOID    DIB_32BPP_VLine(SURFOBJ* SurfObj, LONG x, LONG y1, LONG y2, ULONG c);
BOOLEAN DIB_32BPP_BitBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                         SURFGDI *DestGDI, SURFGDI *SourceGDI,
                         RECTL* DestRect, POINTL *SourcePoint,
			             BRUSHOBJ* Brush, POINTL* BrushOrigin,
			             XLATEOBJ *ColorTranslation, ULONG Rop4);
BOOLEAN DIB_32BPP_StretchBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                            SURFGDI *DestGDI, SURFGDI *SourceGDI,
                            RECTL* DestRect, RECTL *SourceRect,
                            POINTL* MaskOrigin, POINTL* BrushOrigin,
			                XLATEOBJ *ColorTranslation, ULONG Mode);			             
BOOLEAN DIB_32BPP_TransparentBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                                 PSURFGDI DestGDI,  PSURFGDI SourceGDI,
                                 RECTL*  DestRect,  POINTL  *SourcePoint,
                                 XLATEOBJ *ColorTranslation, ULONG iTransColor);

