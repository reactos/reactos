extern unsigned char notmask[2];
extern unsigned char altnotmask[2];
extern unsigned char mask1Bpp[8];

VOID    DIB_1BPP_PutPixel(PSURFOBJ SurfObj, LONG x, LONG y, ULONG c);
ULONG   DIB_1BPP_GetPixel(PSURFOBJ SurfObj, LONG x, LONG y);
VOID    DIB_1BPP_HLine(PSURFOBJ SurfObj, LONG x1, LONG x2, LONG y, ULONG c);
VOID    DIB_1BPP_VLine(PSURFOBJ SurfObj, LONG x, LONG y1, LONG y2, ULONG c);
BOOLEAN DIB_1BPP_BitBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                        SURFGDI *DestGDI, SURFGDI *SourceGDI,
                        PRECTL DestRect, POINTL *SourcePoint,
                        XLATEOBJ *ColorTranslation);

VOID    DIB_4BPP_PutPixel(PSURFOBJ SurfObj, LONG x, LONG y, ULONG c);
ULONG   DIB_4BPP_GetPixel(PSURFOBJ SurfObj, LONG x, LONG y);
VOID    DIB_4BPP_HLine(PSURFOBJ SurfObj, LONG x1, LONG x2, LONG y, ULONG c);
VOID    DIB_4BPP_VLine(PSURFOBJ SurfObj, LONG x, LONG y1, LONG y2, ULONG c);
BOOLEAN DIB_4BPP_BitBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                        SURFGDI *DestGDI, SURFGDI *SourceGDI,
                        PRECTL DestRect, POINTL *SourcePoint,
                        XLATEOBJ *ColorTranslation);

VOID    DIB_8BPP_PutPixel(PSURFOBJ SurfObj, LONG x, LONG y, ULONG c);
ULONG   DIB_8BPP_GetPixel(PSURFOBJ SurfObj, LONG x, LONG y);
VOID    DIB_8BPP_HLine(PSURFOBJ SurfObj, LONG x1, LONG x2, LONG y, ULONG c);
VOID    DIB_8BPP_VLine(PSURFOBJ SurfObj, LONG x, LONG y1, LONG y2, ULONG c);
BOOLEAN DIB_8BPP_BitBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                        SURFGDI *DestGDI, SURFGDI *SourceGDI,
                        PRECTL DestRect, POINTL *SourcePoint,
                        XLATEOBJ *ColorTranslation);

VOID    DIB_16BPP_PutPixel(PSURFOBJ SurfObj, LONG x, LONG y, ULONG c);
ULONG   DIB_16BPP_GetPixel(PSURFOBJ SurfObj, LONG x, LONG y);
VOID    DIB_16BPP_HLine(PSURFOBJ SurfObj, LONG x1, LONG x2, LONG y, ULONG c);
VOID    DIB_16BPP_VLine(PSURFOBJ SurfObj, LONG x, LONG y1, LONG y2, ULONG c);
BOOLEAN DIB_16BPP_BitBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                         SURFGDI *DestGDI, SURFGDI *SourceGDI,
                         PRECTL DestRect, POINTL *SourcePoint,
                         XLATEOBJ *ColorTranslation);

VOID    DIB_24BPP_PutPixel(PSURFOBJ SurfObj, LONG x, LONG y, ULONG c);
ULONG   DIB_24BPP_GetPixel(PSURFOBJ SurfObj, LONG x, LONG y);
VOID    DIB_24BPP_HLine(PSURFOBJ SurfObj, LONG x1, LONG x2, LONG y, ULONG c);
VOID    DIB_24BPP_VLine(PSURFOBJ SurfObj, LONG x, LONG y1, LONG y2, ULONG c);
BOOLEAN DIB_24BPP_BitBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                         SURFGDI *DestGDI, SURFGDI *SourceGDI,
                         PRECTL DestRect, POINTL *SourcePoint,
                         XLATEOBJ *ColorTranslation);

VOID    DIB_32BPP_PutPixel(PSURFOBJ SurfObj, LONG x, LONG y, ULONG c);
ULONG   DIB_32BPP_GetPixel(PSURFOBJ SurfObj, LONG x, LONG y);
VOID    DIB_32BPP_HLine(PSURFOBJ SurfObj, LONG x1, LONG x2, LONG y, ULONG c);
VOID    DIB_32BPP_VLine(PSURFOBJ SurfObj, LONG x, LONG y1, LONG y2, ULONG c);
BOOLEAN DIB_32BPP_BitBlt(SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
                         SURFGDI *DestGDI, SURFGDI *SourceGDI,
                         PRECTL DestRect, POINTL *SourcePoint,
                         XLATEOBJ *ColorTranslation);
