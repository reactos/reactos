#ifndef _WIN32K_RECT_H
#define _WIN32K_RECT_H

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

BOOL
FASTCALL
RECTL_bUnionRect(RECTL *prclDst, const RECTL *prcl1, const RECTL *prcl2);

BOOL
FASTCALL
RECTL_bIntersectRect(RECTL *prclDst, const RECTL *prcl1, const RECTL *prcl2);




#endif /* _WIN32K_RECT_H */
