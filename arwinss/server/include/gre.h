#ifndef __WIN32K_GRE_H
#define __WIN32K_GRE_H

/* Math stuff */
#define M_PI_2 1.57079632679489661923

/* arc.c */
BOOLEAN
APIENTRY
GrepArc(PDC  dc,
        int  Left,
        int  Top,
        int  Right,
        int  Bottom,
        int  XRadialStart,
        int  YRadialStart,
        int  XRadialEnd,
        int  YRadialEnd,
        ARCTYPE arctype);

/* bitblt.c */
INT NTAPI DIB_GetDIBWidthBytes(INT width, INT depth);

BOOLEAN NTAPI
GreAlphaBlend(PDC pDest, INT XDest, INT YDest,
              INT WidthDst, INT HeightDst, PDC pSrc,
              INT XSrc, INT YSrc, INT WidthSrc, INT HeightSrc,
              BLENDFUNCTION blendfn);

BOOLEAN NTAPI
GreBitBlt(PDC pDevDst, INT xDst, INT yDst,
          INT width, INT height, PDC pDevSrc,
          INT xSrc, INT ySrc, DWORD rop);

BOOLEAN NTAPI
GrePatBlt(PDC dc, INT XLeft, INT YLeft,
          INT Width, INT Height, DWORD ROP,
          PBRUSH BrushObj);

ULONG NTAPI
GrepBitmapFormat(WORD Bits, DWORD Compression);

BOOLEAN NTAPI
GrepBitBltEx(
    SURFOBJ *psoTrg,
    SURFOBJ *psoSrc,
    SURFOBJ *psoMask,
    CLIPOBJ *pco,
    XLATEOBJ *pxlo,
    RECTL *prclTrg,
    POINTL *pptlSrc,
    POINTL *pptlMask,
    BRUSHOBJ *pbo,
    POINTL *pptlBrush,
    ROP4 rop4,
    BOOL bRemoveMouse);

BOOL NTAPI
GreStretchBltMask(
    PDC DCDest,
    INT  XOriginDest,
    INT  YOriginDest,
    INT  WidthDest,
    INT  HeightDest,
    PDC DCSrc,
    INT  XOriginSrc,
    INT  YOriginSrc,
    INT  WidthSrc,
    INT  HeightSrc,
    DWORD  ROP,
    DWORD  dwBackColor,
    PDC DCMask);

INT
NTAPI
GreSetDIBits(
    PDC   DC,
    HBITMAP  hBitmap,
    UINT  StartScan,
    UINT  ScanLines,
    CONST VOID  *Bits,
    CONST BITMAPINFO  *bmi,
    UINT  ColorUse);

INT
NTAPI
GreGetDIBits(
    PDC   DC,
    HBITMAP  hBitmap,
    UINT  StartScan,
    UINT  ScanLines,
    VOID  *Bits,
    BITMAPINFO  *bmi,
    UINT  ColorUse);

INT
NTAPI
GreSetDIBitsToDevice(
    PDC   DC,
    INT   xDest,
    INT   yDest,
    DWORD cx,
    DWORD cy,
    INT   xSrc,
    INT   ySrc,
    UINT  StartScan,
    UINT  ScanLines,
    CONST VOID  *Bits,
    CONST BITMAPINFO  *bmi,
    UINT  ColorUse);

UINT
NTAPI
GreSetDIBColorTable(
    PDC pDC,
    UINT StartIndex,
    UINT Entries,
    CONST RGBQUAD *Colors);

INT FASTCALL
BitsPerFormat(ULONG Format);

/* device.c */
LONG FASTCALL GreChangeDisplaySettings(PUNICODE_STRING pDeviceName,
                                       LPDEVMODEW DevMode, DWORD dwflags,
                                       PVOID lParam);
NTSTATUS FASTCALL GreEnumDisplaySettings(PUNICODE_STRING pDeviceName, DWORD iModeNum,
                                         LPDEVMODEW pDevMode, DWORD dwFlags);
INT APIENTRY GreGetDeviceCaps(PDC pDC, INT cap);

/* drawing.c */

VOID
APIENTRY
GrepFillArc( PDC dc,
            INT XLeft,
            INT YLeft,
            INT Width,
            INT Height,
            double StartArc,
            double EndArc,
            ARCTYPE arctype,
            PPOINTL BrushOrigin);

BOOLEAN
APIENTRY
GrepDrawArc( PDC dc,
            INT XLeft,
            INT YLeft,
            INT Width,
            INT Height,
            double StartArc,
            double EndArc,
            ARCTYPE arctype,
            PBRUSH pbrush,
            PPOINTL BrushOrigin);

BOOLEAN
APIENTRY
GrepDrawEllipse(PDC dc,
                INT XLeft,
                INT YLeft,
                INT Width,
                INT Height,
                PBRUSH pbrush,
                PPOINTL brushOrg);

BOOLEAN
APIENTRY
GrepFillEllipse(PDC dc,
                INT XLeft,
                INT YLeft,
                INT Width,
                INT Height,
                PBRUSH pbrush,
                PPOINTL brushOrg);

/* ellipse.c */
VOID NTAPI
GreEllipse(PDC dc,
           INT Left,
           INT Top,
           INT Right,
           INT Bottom);

/* font.c */
VOID NTAPI
GreTextOut(PDC pDC, INT x, INT y, UINT flags,
           const RECT *lprect, LPCWSTR wstr, UINT count,
           const INT *lpDx, gsCacheEntryFormat *formatEntry,
           AA_Type aa_type);

/* lineto.c */
BOOLEAN NTAPI
GreLineTo(SURFOBJ *psoDest,
          CLIPOBJ *ClipObj,
          BRUSHOBJ *pbo,
          LONG x1,
          LONG y1,
          LONG x2,
          LONG y2,
          RECTL *RectBounds,
          MIX Mix);

/* polyfill.c */
BOOL NTAPI
GrepFillPolygon(
    PDC dc,
    SURFOBJ *psurf,
    BRUSHOBJ *BrushObj,
    CONST POINT *Points,
    int Count,
    PRECTL DestRect,
    POINTL *BrushOrigin);

/* rect.c */
VOID NTAPI
GreRectangle(PDC dc,
             INT LeftRect,
             INT TopRect,
             INT RightRect,
             INT BottomRect);

VOID NTAPI
GrePolygon(PDC pDC,
           const POINT *ptPoints,
           INT count,
           PRECTL pDestRect);

VOID NTAPI
GrePolyline(PDC pDC,
           const POINT *ptPoints,
           INT count);

BOOL NTAPI 
GreFloodFill( PDC dc, POINTL *Pt, COLORREF Color, UINT FillType );

BOOLEAN NTAPI
RECTL_bIntersectRect(RECTL* prclDst, const RECTL* prcl1, const RECTL* prcl2);

FORCEINLINE
VOID
RECTL_vSetRect(RECTL *prcl, LONG left, LONG top, LONG right, LONG bottom)
{
    prcl->left = left;
    prcl->top = top;
    prcl->right = right;
    prcl->bottom = bottom;
}

FORCEINLINE
VOID
RECTL_vSetEmptyRect(RECTL *prcl)
{
    prcl->left = 0;
    prcl->top = 0;
    prcl->right = 0;
    prcl->bottom = 0;
}

FORCEINLINE
VOID
RECTL_vOffsetRect(RECTL *prcl, INT cx, INT cy)
{
    prcl->left += cx;
    prcl->right += cx;
    prcl->top += cy;
    prcl->bottom += cy;
}

FORCEINLINE
BOOL
RECTL_bIsEmptyRect(const RECTL *prcl)
{
    return (prcl->left >= prcl->right || prcl->top >= prcl->bottom);
}

FORCEINLINE
BOOL
RECTL_bPointInRect(const RECTL *prcl, INT x, INT y)
{
    return (x >= prcl->left && x <= prcl->right &&
            y >= prcl->top  && y <= prcl->bottom);
}

/* surfobj.c */
COLORREF
NTAPI
GreGetPixel(
    PDC pDC,
    UINT x,
    UINT y);

VOID
NTAPI
GreSetPixel(
    PDC pDC,
    UINT x,
    UINT y,
    COLORREF crColor);

BOOL
APIENTRY
GreCopyBits(
    SURFOBJ *psoDest,
    SURFOBJ *psoSource,
    CLIPOBJ *Clip,
    XLATEOBJ *ColorTranslation,
    RECTL *DestRect,
    POINTL *SourcePoint);

/* Private Eng functions */
BOOL APIENTRY
EngpStretchBlt(SURFOBJ *psoDest,
               SURFOBJ *psoSource,
               SURFOBJ *MaskSurf,
               CLIPOBJ *ClipRegion,
               XLATEOBJ *ColorTranslation,
               RECTL *DestRect,
               RECTL *SourceRect,
               POINTL *pMaskOrigin,
               BRUSHOBJ *pbo,
               POINTL *BrushOrigin,
               ROP4 ROP);

/* IntEng Enter/Leave support */
typedef struct INTENG_ENTER_LEAVE_TAG
  {
  /* Contents is private to EngEnter/EngLeave */
  SURFOBJ *DestObj;
  SURFOBJ *OutputObj;
  HBITMAP OutputBitmap;
  CLIPOBJ *TrivialClipObj;
  RECTL DestRect;
  BOOL ReadOnly;
  } INTENG_ENTER_LEAVE, *PINTENG_ENTER_LEAVE;

BOOL APIENTRY IntEngEnter(PINTENG_ENTER_LEAVE EnterLeave,
                                SURFOBJ *DestObj,
                                RECTL *DestRect,
                                BOOL ReadOnly,
                                POINTL *Translate,
                                SURFOBJ **OutputObj);

BOOL APIENTRY IntEngLeave(PINTENG_ENTER_LEAVE EnterLeave);

void NTAPI
NWtoSE(SURFOBJ* OutputObj, CLIPOBJ* Clip,
       BRUSHOBJ* pbo, LONG x, LONG y, LONG deltax, LONG deltay,
       POINTL* Translate);

void NTAPI
SWtoNE(SURFOBJ* OutputObj, CLIPOBJ* Clip,
       BRUSHOBJ* pbo, LONG x, LONG y, LONG deltax, LONG deltay,
       POINTL* Translate);

void NTAPI
NEtoSW(SURFOBJ* OutputObj, CLIPOBJ* Clip,
       BRUSHOBJ* pbo, LONG x, LONG y, LONG deltax, LONG deltay,
       POINTL* Translate);

void NTAPI
SEtoNW(SURFOBJ* OutputObj, CLIPOBJ* Clip,
       BRUSHOBJ* pbo, LONG x, LONG y, LONG deltax, LONG deltay,
       POINTL* Translate);

/* Mouse pointer */

VOID
NTAPI
GreMovePointer(
    HDC hdc,
    LONG x,
    LONG y);

ULONG
NTAPI
GreSetPointerShape(
    HDC hdc,
    HBITMAP hbmMask,
    HBITMAP hbmColor,
    LONG xHot,
    LONG yHot,
    LONG x,
    LONG y);

INT INTERNAL_CALL
MouseSafetyOnDrawStart(
    SURFOBJ *pso,
    LONG HazardX1,
    LONG HazardY1,
    LONG HazardX2,
    LONG HazardY2);

INT INTERNAL_CALL
MouseSafetyOnDrawEnd(
    SURFOBJ *pso);

RGBQUAD *
FASTCALL
DIB_MapPaletteColors(PDC dc, CONST BITMAPINFO* lpbmi);

HBRUSH
APIENTRY
GreCreateDIBBrush(
    CONST BITMAPINFO *BitmapInfo,
    UINT ColorSpec,
    UINT BitmapInfoSize,
    CONST VOID *PackedDIB);

HBRUSH
APIENTRY
GreCreateHatchBrush(
    INT Style,
    COLORREF Color);

HBRUSH
APIENTRY
GreCreatePatternBrush(
    HBITMAP hBitmap);

HBRUSH
APIENTRY
GreCreateSolidBrush(
    COLORREF Color);

HBRUSH
APIENTRY
GreCreateNullBrush(VOID);

HBRUSH APIENTRY GreSelectBrush( HDC hdc, HBRUSH hbrush );

UINT APIENTRY
GreGetSystemPaletteEntries(HDC  hDC,
                           UINT  StartIndex,
                           UINT  Entries,
                           LPPALETTEENTRY  pe);

HPALETTE
FASTCALL
BuildDIBPalette(CONST BITMAPINFO *bmi, PINT paletteType);

/* Test functions */
VOID NTAPI GrePerformTests();

#endif
