#pragma once

typedef ULONG HCLIP;

#define ENUM_RECT_LIMIT 32

typedef struct _RECT_ENUM
{
  ULONG c;
  RECTL arcl[ENUM_RECT_LIMIT];
} RECT_ENUM;

typedef struct tagSPAN
{
  LONG Y;
  LONG X;
  ULONG Width;
} SPAN, *PSPAN;

enum _R3_ROPCODES
{
    R3_OPINDEX_NOOP         = 0xAA,
    R3_OPINDEX_BLACKNESS    = 0x00,
    R3_OPINDEX_NOTSRCERASE  = 0x11,
    R3_OPINDEX_NOTSRCCOPY   = 0x33,
    R3_OPINDEX_SRCERASE     = 0x44,
    R3_OPINDEX_DSTINVERT    = 0x55,
    R3_OPINDEX_PATINVERT    = 0x5A,
    R3_OPINDEX_SRCINVERT    = 0x66,
    R3_OPINDEX_SRCAND       = 0x88,
    R3_OPINDEX_MERGEPAINT   = 0xBB,
    R3_OPINDEX_MERGECOPY    = 0xC0,
    R3_OPINDEX_SRCCOPY      = 0xCC,
    R3_OPINDEX_SRCPAINT     = 0xEE,
    R3_OPINDEX_PATCOPY      = 0xF0,
    R3_OPINDEX_PATPAINT     = 0xFB,
    R3_OPINDEX_WHITENESS    = 0xFF
};

#define ROP2_TO_MIX(Rop2) (((Rop2) << 8) | (Rop2))

#define ROP4_FROM_INDEX(index) ((index) | ((index) << 8))

#define ROP4_USES_DEST(Rop4)    ((((Rop4) & 0xAAAA) >> 1) != ((Rop4) & 0x5555))
#define ROP4_USES_SOURCE(Rop4)  ((((Rop4) & 0xCCCC) >> 2) != ((Rop4) & 0x3333))
#define ROP4_USES_PATTERN(Rop4) ((((Rop4) & 0xF0F0) >> 4) != ((Rop4) & 0x0F0F))
#define ROP4_USES_MASK(Rop4)    ((((Rop4) & 0xFF00) >> 8) != ((Rop4) & 0x00ff))

#define IS_VALID_ROP4(rop) (((rop) & 0xFFFF0000) == 0)

#define ROP4_FGND(Rop4)    ((Rop4) & 0x00FF)
#define ROP4_BKGND(Rop4)    (((Rop4) & 0xFF00) >> 8)

#define ROP4_NOOP (R3_OPINDEX_NOOP | (R3_OPINDEX_NOOP << 8))
#define ROP4_MASK (R3_OPINDEX_SRCCOPY | (R3_OPINDEX_NOOP << 8))
#define ROP4_MASKPAINT (R3_OPINDEX_PATCOPY | (R3_OPINDEX_NOOP << 8))

/* Definitions of IntEngXxx functions */

BOOL APIENTRY
IntEngLineTo(SURFOBJ *Surface,
             CLIPOBJ *Clip,
             BRUSHOBJ *Brush,
             LONG x1,
             LONG y1,
             LONG x2,
             LONG y2,
             RECTL *RectBounds,
             MIX mix);

BOOL APIENTRY
IntEngBitBlt(SURFOBJ *DestObj,
               SURFOBJ *SourceObj,
               SURFOBJ *Mask,
               CLIPOBJ *ClipRegion,
               XLATEOBJ *ColorTranslation,
               RECTL *DestRect,
               POINTL *SourcePoint,
               POINTL *MaskOrigin,
               BRUSHOBJ *Brush,
               POINTL *BrushOrigin,
               ROP4 Rop4);

BOOL APIENTRY
IntEngStretchBlt(SURFOBJ *DestObj,
                 SURFOBJ *SourceObj,
                 SURFOBJ *Mask,
                 CLIPOBJ *ClipRegion,
                 XLATEOBJ *ColorTranslation,
                 COLORADJUSTMENT *pca,
                 RECTL *DestRect,
                 RECTL *SourceRect,
                 POINTL *pMaskOrigin,
                 BRUSHOBJ *Brush,
                 POINTL *BrushOrigin,
                 ULONG Mode);

BOOL APIENTRY
IntEngGradientFill(SURFOBJ *psoDest,
                   CLIPOBJ *pco,
                   XLATEOBJ *pxlo,
                   TRIVERTEX *pVertex,
                   ULONG nVertex,
                   PVOID pMesh,
                   ULONG nMesh,
                   RECTL *prclExtents,
                   POINTL *pptlDitherOrg,
                   ULONG ulMode);

BOOL APIENTRY
IntEngPolyline(SURFOBJ *DestSurf,
               CLIPOBJ *Clip,
               BRUSHOBJ *Brush,
               CONST LPPOINT  pt,
               LONG dCount,
               MIX mix);

VOID FASTCALL
IntEngUpdateClipRegion(XCLIPOBJ* Clip,
                       ULONG count,
                       const RECTL* pRect,
                       const RECTL* rcBounds);

VOID FASTCALL
IntEngInitClipObj(XCLIPOBJ *Clip);

VOID FASTCALL
IntEngFreeClipResources(XCLIPOBJ *Clip);


BOOL FASTCALL
IntEngTransparentBlt(SURFOBJ *Dest,
                     SURFOBJ *Source,
                     CLIPOBJ *Clip,
                     XLATEOBJ *ColorTranslation,
                     PRECTL DestRect,
                     PRECTL SourceRect,
                     ULONG iTransColor,
                     ULONG Reserved);

BOOL
APIENTRY
IntEngPaint(
    _In_ SURFOBJ *pso,
    _In_ CLIPOBJ *pco,
    _In_ BRUSHOBJ *pbo,
    _In_ POINTL *pptlBrushOrg,
    _In_ __in_data_source(USER_MODE) MIX mix);

BOOL
APIENTRY
IntEngAlphaBlend(
    _Inout_ SURFOBJ *psoDest,
    _In_ SURFOBJ *psoSource,
    _In_opt_ CLIPOBJ *pco,
    _In_opt_ XLATEOBJ *pxlo,
    _In_ RECTL *prclDest,
    _In_ RECTL *prclSrc,
    _In_ BLENDOBJ *pBlendObj);

BOOL APIENTRY
IntEngCopyBits(SURFOBJ *psoDest,
	    SURFOBJ *psoSource,
	    CLIPOBJ *pco,
	    XLATEOBJ *pxlo,
	    RECTL *prclDest,
	    POINTL *ptlSource);
