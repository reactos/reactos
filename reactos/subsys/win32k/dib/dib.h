static unsigned char notmask[2] = { 0x0f, 0xf0 };
static unsigned char altnotmask[2] = { 0xf0, 0x0f };
static unsigned char mask1Bpp[8] = { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };

typedef VOID  (*PFN_DIB_PutPixel)(PSURFOBJ, LONG, LONG, ULONG);
typedef ULONG (*PFN_DIB_GetPixel)(PSURFOBJ, LONG, LONG);
typedef VOID  (*PFN_DIB_HLine)   (PSURFOBJ, LONG, LONG, LONG, ULONG);
typedef VOID  (*PFN_DIB_VLine)   (PSURFOBJ, LONG, LONG, LONG, ULONG);

PFN_DIB_PutPixel DIB_1BPP_PutPixel(PSURFOBJ SurfObj, LONG x, LONG y, ULONG c);
PFN_DIB_GetPixel DIB_1BPP_GetPixel(PSURFOBJ SurfObj, LONG x, LONG y);
PFN_DIB_HLine DIB_1BPP_HLine(PSURFOBJ SurfObj, LONG x1, LONG x2, LONG y, ULONG c);
PFN_DIB_VLine DIB_1BPP_VLine(PSURFOBJ SurfObj, LONG x, LONG y1, LONG y2, ULONG c);
BOOLEAN DIB_To_1BPP_Bitblt(  SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
        SURFGDI *DestGDI,  SURFGDI *SourceGDI,
        PRECTL  DestRect,  POINTL  *SourcePoint,
			     LONG   Delta,     XLATEOBJ *ColorTranslation);

PFN_DIB_PutPixel DIB_4BPP_PutPixel(PSURFOBJ SurfObj, LONG x, LONG y, ULONG c);
PFN_DIB_GetPixel DIB_4BPP_GetPixel(PSURFOBJ SurfObj, LONG x, LONG y);
PFN_DIB_HLine DIB_4BPP_HLine(PSURFOBJ SurfObj, LONG x1, LONG x2, LONG y, ULONG c);
PFN_DIB_VLine DIB_4BPP_VLine(PSURFOBJ SurfObj, LONG x, LONG y1, LONG y2, ULONG c);
BOOLEAN DIB_To_4BPP_Bitblt(  SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
        SURFGDI *DestGDI,  SURFGDI *SourceGDI,
        PRECTL  DestRect,  POINTL  *SourcePoint,
			     LONG   Delta,     XLATEOBJ *ColorTranslation);

VOID DIB_16BPP_PutPixel(PSURFOBJ SurfObj, LONG x, LONG y, ULONG c);
ULONG DIB_16BPP_GetPixel(PSURFOBJ SurfObj, LONG x, LONG y);
VOID DIB_16BPP_HLine(PSURFOBJ SurfObj, LONG x1, LONG x2, LONG y, ULONG c);
VOID DIB_16BPP_VLine(PSURFOBJ SurfObj, LONG x, LONG y1, LONG y2, ULONG c);
BOOL DIB_To_16BPP_Bitblt(  SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
        SURFGDI *DestGDI,  SURFGDI *SourceGDI,
        PRECTL  DestRect,  POINTL  *SourcePoint,
			     LONG   Delta,     XLATEOBJ *ColorTranslation);

PFN_DIB_PutPixel DIB_24BPP_PutPixel(PSURFOBJ SurfObj, LONG x, LONG y, ULONG c);
PFN_DIB_GetPixel DIB_24BPP_GetPixel(PSURFOBJ SurfObj, LONG x, LONG y);
PFN_DIB_HLine DIB_24BPP_HLine(PSURFOBJ SurfObj, LONG x1, LONG x2, LONG y, ULONG c);
PFN_DIB_VLine DIB_24BPP_VLine(PSURFOBJ SurfObj, LONG x, LONG y1, LONG y2, ULONG c);
BOOLEAN DIB_To_24BPP_Bitblt(  SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
        SURFGDI *DestGDI,  SURFGDI *SourceGDI,
        PRECTL  DestRect,  POINTL  *SourcePoint,
			     LONG   Delta,     XLATEOBJ *ColorTranslation);
