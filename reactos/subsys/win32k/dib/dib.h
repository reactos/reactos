extern unsigned char notmask[2];
extern unsigned char altnotmask[2];

typedef VOID  (*PFN_DIB_PutPixel)(SURFOBJ*, LONG, LONG, ULONG);
typedef ULONG (*PFN_DIB_GetPixel)(SURFOBJ*, LONG, LONG);
typedef VOID  (*PFN_DIB_HLine)   (SURFOBJ*, LONG, LONG, LONG, ULONG);
typedef VOID  (*PFN_DIB_VLine)   (SURFOBJ*, LONG, LONG, LONG, ULONG);

PFN_DIB_PutPixel DIB_4BPP_PutPixel(SURFOBJ* SurfObj, LONG x, LONG y, ULONG c);
PFN_DIB_GetPixel DIB_4BPP_GetPixel(SURFOBJ* SurfObj, LONG x, LONG y);
PFN_DIB_HLine DIB_4BPP_HLine(SURFOBJ* SurfObj, LONG x1, LONG x2, LONG y, ULONG c);
PFN_DIB_VLine DIB_4BPP_VLine(SURFOBJ* SurfObj, LONG x, LONG y1, LONG y2, ULONG c);
BOOLEAN DIB_To_4BPP_Bitblt(  SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
        SURFGDI *DestGDI,  SURFGDI *SourceGDI,
        PRECTL  DestRect,  POINTL  *SourcePoint,
			     ULONG   Delta,     XLATEOBJ *ColorTranslation);

PFN_DIB_PutPixel DIB_24BPP_PutPixel(SURFOBJ* SurfObj, LONG x, LONG y, ULONG c);
PFN_DIB_GetPixel DIB_24BPP_GetPixel(SURFOBJ* SurfObj, LONG x, LONG y);
PFN_DIB_HLine DIB_24BPP_HLine(SURFOBJ* SurfObj, LONG x1, LONG x2, LONG y, ULONG c);
PFN_DIB_VLine DIB_24BPP_VLine(SURFOBJ* SurfObj, LONG x, LONG y1, LONG y2, ULONG c);
BOOLEAN DIB_To_24BPP_Bitblt(  SURFOBJ *DestSurf, SURFOBJ *SourceSurf,
        SURFGDI *DestGDI,  SURFGDI *SourceGDI,
        RECTL  *DestRect,  POINTL  *SourcePoint,
			      ULONG   Delta,     XLATEOBJ *ColorTranslation);
