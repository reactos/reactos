#ifndef __WIN32K_GRE_H
#define __WIN32K_GRE_H

/* bitblt.c */
INT NTAPI DIB_GetDIBWidthBytes(INT width, INT depth);

BOOLEAN NTAPI
GreBitBlt(PDC pDevDst, INT xDst, INT yDst,
          INT width, INT height, PDC pDevSrc,
          INT xSrc, INT ySrc, DWORD rop);

BOOLEAN NTAPI
GrePatBlt(PDC dc, INT XLeft, INT YLeft,
          INT Width, INT Height, DWORD ROP,
          PBRUSHGDI BrushObj);

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

INT FASTCALL
BitsPerFormat(ULONG Format);

/* font.c */
VOID NTAPI
GreTextOut(PDC pDC, INT x, INT y, UINT flags,
           const RECT *lprect, LPCWSTR wstr, UINT count,
           const INT *lpDx, gsCacheEntryFormat *formatEntry);

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
           INT count);

VOID NTAPI
GrePolyline(PDC pDC,
           const POINT *ptPoints,
           INT count);

BOOLEAN NTAPI
RECTL_bIntersectRect(RECTL* prclDst, const RECTL* prcl1, const RECTL* prcl2);

VOID
FORCEINLINE
RECTL_vSetRect(RECTL *prcl, LONG left, LONG top, LONG right, LONG bottom)
{
    prcl->left = left;
    prcl->top = top;
    prcl->right = right;
    prcl->bottom = bottom;
}

VOID
FORCEINLINE
RECTL_vSetEmptyRect(RECTL *prcl)
{
    prcl->left = 0;
    prcl->top = 0;
    prcl->right = 0;
    prcl->bottom = 0;
}

VOID
FORCEINLINE
RECTL_vOffsetRect(RECTL *prcl, INT cx, INT cy)
{
    prcl->left += cx;
    prcl->right += cx;
    prcl->top += cy;
    prcl->bottom += cy;
}

BOOL
FORCEINLINE
RECTL_bIsEmptyRect(const RECTL *prcl)
{
    return (prcl->left >= prcl->right || prcl->top >= prcl->bottom);
}

BOOL
FORCEINLINE
RECTL_bPointInRect(const RECTL *prcl, INT x, INT y)
{
    return (x >= prcl->left && x <= prcl->right &&
            y >= prcl->top  && y <= prcl->bottom);
}

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


#endif
