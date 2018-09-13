/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WGDI31.H
 *  WOW32 16-bit Win 3.1 GDI API support
 *
 *  History:
 *  Created 16-Mar-1992 by Chandan S. Chauhan (ChandanC)
--*/

#define PUTSIZE16(vp, lp) {\
    PSIZE16 p16;\
    GETVDMPTR(vp, sizeof(SIZE16), p16);\
    STORESHORT(p16->cx, (lp)->cx);\
    STORESHORT(p16->cy, (lp)->cy);\
    FREEVDMPTR(p16);\
    }

#define PUTRASTERIZERSTATUS16(vp, lp) {\
    PRASTERIZER_STATUS16 p16;\
    GETVDMPTR(vp, sizeof(RASTERIZER_STATUS16), p16);\
    STORESHORT(p16->nSize, (lp)->nSize);\
    STORESHORT(p16->wFlags, (lp)->wFlags);\
    STORESHORT(p16->nLanguageID, (lp)->nLanguageID);\
    FREEVDMPTR(p16);\
    }

#define PUTGLYPHMETRICS16(vp, lp) {\
    PGLYPHMETRICS16 p16;\
    GETVDMPTR(vp, sizeof(GLYPHMETRICS16), p16);\
    STOREWORD(p16->gmBlackBoxX, (lp)->gmBlackBoxX);\
    STOREWORD(p16->gmBlackBoxY, (lp)->gmBlackBoxY );\
    STORESHORT(p16->gmptGlyphOrigin.x, (lp)->gmptGlyphOrigin.x);\
    STORESHORT(p16->gmptGlyphOrigin.y, (lp)->gmptGlyphOrigin.y);\
    STORESHORT(p16->gmCellIncX, (lp)->gmCellIncX);\
    STORESHORT(p16->gmCellIncY, (lp)->gmCellIncY);\
    FREEVDMPTR(p16);\
    }

#define GETMAT2(vp, lp) {\
    PMAT216 p16;\
    GETVDMPTR(vp, sizeof(MAT216), p16);\
    (lp)->eM11.fract = FETCHWORD(p16->eM11.fract);\
    (lp)->eM11.value = FETCHSHORT(p16->eM11.value);\
    (lp)->eM12.fract = FETCHWORD(p16->eM12.fract);\
    (lp)->eM12.value = FETCHSHORT(p16->eM12.value);\
    (lp)->eM21.fract = FETCHWORD(p16->eM21.fract);\
    (lp)->eM21.value = FETCHSHORT(p16->eM21.value);\
    (lp)->eM22.fract = FETCHWORD(p16->eM22.fract);\
    (lp)->eM22.value = FETCHSHORT(p16->eM22.value);\
    FREEVDMPTR(p16);\
    }


ULONG FASTCALL WG32AbortDoc(PVDMFRAME pFrame);
ULONG FASTCALL WG32CreateScalableFontResource(PVDMFRAME pFrame);
ULONG FASTCALL WG32EndDoc(PVDMFRAME pFrame);
ULONG FASTCALL WG32EnumFontFamilies(PVDMFRAME pFrame);
ULONG FASTCALL WG32GetAspectRatioFilterEx(PVDMFRAME pFrame);
ULONG FASTCALL WG32GetBitmapDimensionEx(PVDMFRAME pFrame);
ULONG FASTCALL WG32GetBoundsRect(PVDMFRAME pFrame);
ULONG FASTCALL WG32GetBrushOrgEx(PVDMFRAME pFrame);
ULONG FASTCALL WG32GetCharABCWidths(PVDMFRAME pFrame);
ULONG FASTCALL WG32GetCurrentPositionEx(PVDMFRAME pFrame);
ULONG FASTCALL WG32GetGlyphOutline(PVDMFRAME pFrame);
ULONG FASTCALL WG32GetKerningPairs(PVDMFRAME pFrame);
ULONG FASTCALL WG32GetOutlineTextMetrics(PVDMFRAME pFrame);
ULONG FASTCALL WG32GetTextExtentPoint(PVDMFRAME pFrame);
ULONG FASTCALL WG32GetViewportExtEx(PVDMFRAME pFrame);
ULONG FASTCALL WG32GetViewportOrgEx(PVDMFRAME pFrame);
ULONG FASTCALL WG32GetWindowExtEx(PVDMFRAME pFrame);
ULONG FASTCALL WG32GetWindowOrgEx(PVDMFRAME pFrame);
ULONG FASTCALL WG32MoveToEx(PVDMFRAME pFrame);
ULONG FASTCALL WG32OffsetViewportOrgEx(PVDMFRAME pFrame);
ULONG FASTCALL WG32OffsetWindowOrgEx(PVDMFRAME pFrame);
ULONG FASTCALL WG32ResetDC(PVDMFRAME pFrame);
ULONG FASTCALL WG32ScaleViewportExtEx(PVDMFRAME pFrame);
ULONG FASTCALL WG32ScaleWindowExtEx(PVDMFRAME pFrame);
ULONG FASTCALL WG32SetAbortProc(PVDMFRAME pFrame);
ULONG FASTCALL WG32SetBitmapDimensionEx(PVDMFRAME pFrame);
ULONG FASTCALL WG32SetBoundsRect(PVDMFRAME pFrame);
ULONG FASTCALL WG32SetMetaFileBitsBetter(PVDMFRAME pFrame);
ULONG FASTCALL WG32SetViewportExtEx(PVDMFRAME pFrame);
ULONG FASTCALL WG32SetViewportOrgEx(PVDMFRAME pFrame);
ULONG FASTCALL WG32SetWindowExtEx(PVDMFRAME pFrame);
ULONG FASTCALL WG32SetWindowOrgEx(PVDMFRAME pFrame);
ULONG FASTCALL WG32StartDoc(PVDMFRAME pFrame);
ULONG FASTCALL WG32GetRasterizerCaps(PVDMFRAME pFrame);

VOID  putabcpairs16(VPABC16 vpAbc, UINT c, LPABC lpAbc);

ULONG FASTCALL WG32InquireVisRgn(PVDMFRAME pFrame);
BOOL  InitVisRgn();
ULONG FASTCALL WG32GetClipRgn(PVDMFRAME pFrame);
