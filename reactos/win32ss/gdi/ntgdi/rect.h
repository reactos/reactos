#pragma once

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
    return (x >= prcl->left && x < prcl->right &&
            y >= prcl->top  && y < prcl->bottom);
}

FORCEINLINE
BOOL
RECTL_bIsWellOrdered(const RECTL *prcl)
{
    return ((prcl->left <= prcl->right) &&
            (prcl->top  <= prcl->bottom));
}

BOOL
FASTCALL
RECTL_bUnionRect(RECTL *prclDst, const RECTL *prcl1, const RECTL *prcl2);

BOOL
FASTCALL
RECTL_bIntersectRect(RECTL *prclDst, const RECTL *prcl1, const RECTL *prcl2);

VOID
FASTCALL
RECTL_vMakeWellOrdered(RECTL *prcl);

VOID
FASTCALL
RECTL_vInflateRect(RECTL *rect, INT dx, INT dy);
