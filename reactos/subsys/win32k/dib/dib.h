static unsigned char notmask[2] = { 0x0f, 0xf0 };
static unsigned char altnotmask[2] = { 0xf0, 0x0f };

typedef VOID  (*PFN_DIB_PutPixel)(PSURFOBJ, LONG, LONG, ULONG);
typedef ULONG (*PFN_DIB_GetPixel)(PSURFOBJ, LONG, LONG);
typedef VOID  (*PFN_DIB_HLine)   (PSURFOBJ, LONG, LONG, LONG, ULONG);
typedef VOID  (*PFN_DIB_VLine)   (PSURFOBJ, LONG, LONG, LONG, ULONG);

PFN_DIB_PutPixel DIB_4BPP_PutPixel(PSURFOBJ SurfObj, LONG x, LONG y, ULONG c);
PFN_DIB_GetPixel DIB_4BPP_GetPixel(PSURFOBJ SurfObj, LONG x, LONG y);
PFN_DIB_HLine DIB_4BPP_HLine(PSURFOBJ SurfObj, LONG x1, LONG x2, LONG y, ULONG c);
PFN_DIB_VLine DIB_4BPP_VLine(PSURFOBJ SurfObj, LONG x, LONG y1, LONG y2, ULONG c);

PFN_DIB_PutPixel DIB_24BPP_PutPixel(PSURFOBJ SurfObj, LONG x, LONG y, ULONG c);
PFN_DIB_GetPixel DIB_24BPP_GetPixel(PSURFOBJ SurfObj, LONG x, LONG y);
PFN_DIB_HLine DIB_24BPP_HLine(PSURFOBJ SurfObj, LONG x1, LONG x2, LONG y, ULONG c);
PFN_DIB_VLine DIB_24BPP_VLine(PSURFOBJ SurfObj, LONG x, LONG y1, LONG y2, ULONG c);
