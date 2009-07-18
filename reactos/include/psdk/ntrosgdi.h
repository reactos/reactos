/*
 * NtRosGdi Entrypoints
 */
#ifndef _NTROSGDI_
#define _NTROSGDI_

#ifndef W32KAPI
#define W32KAPI  DECLSPEC_ADDRSAFE
#endif

typedef struct _NTDRV_PDEVICE
{
    HDC hUserDC;
    HDC hKernelDC;
    int cache_index; /* cache of a currently selected font */
} NTDRV_PDEVICE, *PNTDRV_PDEVICE;

typedef struct _ROS_DCINFO
{
    WORD  dwType;
    SIZE  szVportExt;
    POINT ptVportOrg;
    SIZE  szWndExt;
    POINT ptWndOrg;
    XFORM xfWorld2Wnd;
    XFORM xfWnd2Vport;
} ROS_DCINFO, *PROS_DCINFO;

typedef struct
{
    LOGFONTW lf;
    XFORM    xform;
    SIZE     devsize;  /* size in device coords */
    DWORD    hash;
} LFANDSIZE;

typedef enum { AA_None = 0, AA_Grey, AA_RGB, AA_BGR, AA_VRGB, AA_VBGR, AA_MAXVALUE } AA_Type;

typedef struct
{
    int xOff;
    int yOff;
    int width;
    int height;
    int x;
    int y;
} GlyphInfo;

typedef struct
{
    //GlyphSet glyphset;
    //XRenderPictFormat *font_format;
    int nrealized;
    BOOL *realized;
    void **bitmaps;
    GlyphInfo *gis;
} gsCacheEntryFormat;

typedef struct
{
    LFANDSIZE lfsz;
    AA_Type aa_default;
    gsCacheEntryFormat * format[AA_MAXVALUE];
    INT count;
    INT next;
} gsCacheEntry;

/* bitmap.c */

BOOL APIENTRY RosGdiAlphaBlend(HDC devDst, INT xDst, INT yDst, INT widthDst, INT heightDst,
                             HDC devSrc, INT xSrc, INT ySrc, INT widthSrc, INT heightSrc,
                             BLENDFUNCTION blendfn);
BOOL APIENTRY RosGdiBitBlt( HDC physDevDst, INT xDst, INT yDst,
                    INT width, INT height, HDC physDevSrc,
                    INT xSrc, INT ySrc, DWORD rop );
BOOL APIENTRY RosGdiCreateBitmap( HDC physDev, HBITMAP hBitmap, BITMAP *pBitmap, LPVOID bmBits );
HBITMAP APIENTRY RosGdiCreateDIBSection( HDC physDev, HBITMAP hbitmap,
                                       const BITMAPINFO *bmi, UINT usage );
BOOL APIENTRY RosGdiDeleteBitmap( HBITMAP hbitmap );
LONG APIENTRY RosGdiGetBitmapBits( HBITMAP hbitmap, void *buffer, LONG count );
INT APIENTRY RosGdiGetDIBits( HDC physDev, HBITMAP hbitmap, UINT startscan, UINT lines,
                            LPVOID bits, BITMAPINFO *info, UINT coloruse );
COLORREF APIENTRY RosGdiGetPixel( HDC physDev, INT x, INT y );
BOOL APIENTRY RosGdiPatBlt( HDC physDev, INT left, INT top, INT width, INT height, DWORD rop );
LONG APIENTRY RosGdiSetBitmapBits( HBITMAP hbitmap, const void *bits, LONG count );
UINT APIENTRY RosGdiSetDIBColorTable( HDC physDev, UINT start, UINT count, const RGBQUAD *colors );
INT APIENTRY RosGdiSetDIBits( HDC physDev, HBITMAP hbitmap, UINT startscan,
                            UINT lines, LPCVOID bits, const BITMAPINFO *info, UINT coloruse );
INT APIENTRY RosGdiSetDIBitsToDevice( HDC physDev, INT xDest, INT yDest, DWORD cx,
                                    DWORD cy, INT xSrc, INT ySrc,
                                    UINT startscan, UINT lines, LPCVOID bits,
                                    const BITMAPINFO *info, UINT coloruse );
BOOL APIENTRY RosGdiStretchBlt( HDC physDevDst, INT xDst, INT yDst,
                              INT widthDst, INT heightDst,
                              HDC physDevSrc, INT xSrc, INT ySrc,
                              INT widthSrc, INT heightSrc, DWORD rop );

/* dc.c */
BOOL APIENTRY RosGdiCreateDC( PROS_DCINFO dc, HDC *pdev, LPCWSTR driver, LPCWSTR device,
                              LPCWSTR output, const DEVMODEW* initData );
BOOL APIENTRY RosGdiDeleteDC( HDC physDev );
BOOL APIENTRY RosGdiGetDCOrgEx( HDC physDev, LPPOINT lpp );
BOOL APIENTRY RosGdiPaintRgn( HDC physDev, HRGN hrgn );
VOID APIENTRY RosGdiSelectBitmap( HDC physDev, HBITMAP hbitmap );
VOID APIENTRY RosGdiSelectBrush( HDC physDev, LOGBRUSH *pLogBrush );
HFONT APIENTRY RosGdiSelectFont( HDC physDev, HFONT hfont, HANDLE gdiFont );
VOID APIENTRY RosGdiSelectPen( HDC physDev, LOGPEN *pLogPen, EXTLOGPEN *pExtLogPen );
COLORREF APIENTRY RosGdiSetBkColor( HDC physDev, COLORREF color );
COLORREF APIENTRY RosGdiSetDCBrushColor( HDC physDev, COLORREF crColor );
DWORD APIENTRY RosGdiSetDCOrg( HDC physDev, INT x, INT y );
COLORREF APIENTRY RosGdiSetDCPenColor( HDC physDev, COLORREF crColor );
void APIENTRY RosGdiSetDeviceClipping( HDC physDev, UINT count, PRECTL pRects, PRECTL rcBounds );
BOOL APIENTRY RosGdiSetDeviceGammaRamp(HDC physDev, LPVOID ramp);
COLORREF APIENTRY RosGdiSetPixel( HDC physDev, INT x, INT y, COLORREF color );
BOOL APIENTRY RosGdiSetPixelFormat(HDC physDev,
                                   int iPixelFormat,
                                   const PIXELFORMATDESCRIPTOR *ppfd);
COLORREF APIENTRY RosGdiSetTextColor( HDC physDev, COLORREF color );
VOID APIENTRY RosGdiSetDcRect( HDC physDev, RECT *rcDcRect );

/* enum.c */
int APIENTRY RosGdiChoosePixelFormat(HDC physDev,
                                   const PIXELFORMATDESCRIPTOR *ppfd);
int APIENTRY RosGdiDescribePixelFormat(HDC physDev,
			       int iPixelFormat,
			       UINT nBytes,
			       PIXELFORMATDESCRIPTOR *ppfd);
BOOL APIENTRY RosGdiEnumDeviceFonts( HDC physDev, LPLOGFONTW plf,
                                   FONTENUMPROCW proc, LPARAM lp );
BOOL APIENTRY RosGdiGetCharWidth( HDC physDev, UINT firstChar, UINT lastChar,
                                  LPINT buffer );
INT APIENTRY RosGdiGetDeviceCaps( HDC physDev, INT cap );
BOOL APIENTRY RosGdiGetDeviceGammaRamp(HDC physDev, LPVOID ramp);
BOOL APIENTRY RosGdiGetICMProfile( HDC physDev, LPDWORD size, LPWSTR filename );
COLORREF APIENTRY RosGdiGetNearestColor( HDC physDev, COLORREF color );
int APIENTRY RosGdiGetPixelFormat(HDC physDev);
UINT APIENTRY RosGdiGetSystemPaletteEntries( HDC physDev, UINT start, UINT count,
                                     LPPALETTEENTRY entries );
BOOL APIENTRY RosGdiGetTextExtentExPoint( HDC physDev, LPCWSTR str, INT count,
                                        INT maxExt, LPINT lpnFit, LPINT alpDx, LPSIZE size );
BOOL APIENTRY RosGdiGetTextMetrics(HDC physDev, TEXTMETRICW *metrics);

/* misc.c */
BOOL APIENTRY RosGdiArc( HDC physDev, INT left, INT top, INT right, INT bottom,
            INT xstart, INT ystart, INT xend, INT yend );
BOOL APIENTRY RosGdiChord( HDC physDev, INT left, INT top, INT right, INT bottom,
              INT xstart, INT ystart, INT xend, INT yend );
BOOL APIENTRY RosGdiEllipse( HDC physDev, INT left, INT top, INT right, INT bottom );
INT APIENTRY RosGdiExtEscape( HDC physDev, INT escape, INT in_count, LPCVOID in_data,
                            INT out_count, LPVOID out_data );
BOOL APIENTRY RosGdiExtFloodFill( HDC physDev, INT x, INT y, COLORREF color,
                     UINT fillType );
BOOL APIENTRY RosGdiExtTextOut( HDC physDev, INT x, INT y, UINT flags,
                   const RECT *lprect, LPCWSTR wstr, UINT count,
                   const INT *lpDx, gsCacheEntryFormat *formatEntry );
BOOL APIENTRY RosGdiLineTo( HDC physDev, INT x1, INT y1, INT x2, INT y2 );
BOOL APIENTRY RosGdiPie( HDC physDev, INT left, INT top, INT right, INT bottom,
            INT xstart, INT ystart, INT xend, INT yend );
BOOL APIENTRY RosGdiPolyPolygon( HDC physDev, const POINT* pt, const INT* counts, UINT polygons);
BOOL APIENTRY RosGdiPolyPolyline( HDC physDev, const POINT* pt, const DWORD* counts, DWORD polylines );
BOOL APIENTRY RosGdiPolygon( HDC physDev, const POINT* pt, INT count );
BOOL APIENTRY RosGdiPolyline( HDC physDev, const POINT* pt, INT count );
UINT APIENTRY RosGdiRealizeDefaultPalette( HDC physDev );
UINT APIENTRY RosGdiRealizePalette( HDC physDev, HPALETTE hpal, BOOL primary );
BOOL APIENTRY RosGdiRectangle(HDC physDev, PRECT rc);
BOOL APIENTRY RosGdiRoundRect( HDC physDev, INT left, INT top, INT right,
                  INT bottom, INT ell_width, INT ell_height );
BOOL APIENTRY RosGdiSwapBuffers(HDC physDev);
BOOL APIENTRY RosGdiUnrealizePalette( HPALETTE hpal );

#endif
