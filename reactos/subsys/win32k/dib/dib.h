static unsigned char notmask[2] = { 0x0f, 0xf0 };
static unsigned char altnotmask[2] = { 0xf0, 0x0f };

typedef VOID  (*PFN_DIB_PutPixel)(SURFOBJ *, LONG, LONG, ULONG);
typedef ULONG (*PFN_DIB_GetPixel)(SURFOBJ *, LONG, LONG);
typedef VOID  (*PFN_DIB_HLine)   (SURFOBJ *, LONG, LONG, LONG, ULONG);
typedef VOID  (*PFN_DIB_VLine)   (SURFOBJ *, LONG, LONG, LONG, ULONG);

VOID DIB_4BPP_PutPixel(PSURFOBJ SurfObj, LONG x, LONG y, BYTE c);
BYTE DIB_4BPP_GetPixel(PSURFOBJ SurfObj, LONG x, LONG y);
VOID DIB_4BPP_HLine(PSURFOBJ SurfObj, LONG x1, LONG x2, LONG y, BYTE c);
VOID DIB_4BPP_VLine(PSURFOBJ SurfObj, LONG x, LONG y1, LONG y2, BYTE c);

VOID DIB_24BPP_PutPixel(PSURFOBJ SurfObj, LONG x, LONG y, RGBTRIPLE c);
RGBTRIPLE DIB_24BPP_GetPixel(PSURFOBJ SurfObj, LONG x, LONG y);
VOID DIB_24BPP_HLine(PSURFOBJ SurfObj, LONG x1, LONG x2, LONG y, RGBTRIPLE c);
VOID DIB_24BPP_VLine(PSURFOBJ SurfObj, LONG x, LONG y1, LONG y2, RGBTRIPLE c);
