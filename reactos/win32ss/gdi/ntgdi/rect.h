#pragma once

FORCEINLINE
VOID
RECTL_vSetRect(
    _Out_ RECTL *prcl,
    _In_ LONG left,
    _In_ LONG top,
    _In_ LONG right,
    _In_ LONG bottom)
{
    prcl->left = left;
    prcl->top = top;
    prcl->right = right;
    prcl->bottom = bottom;
}

FORCEINLINE
VOID
RECTL_vSetEmptyRect(
    _Out_ RECTL *prcl)
{
    prcl->left = 0;
    prcl->top = 0;
    prcl->right = 0;
    prcl->bottom = 0;
}

FORCEINLINE
VOID
RECTL_vOffsetRect(
    _Inout_ RECTL *prcl,
    _In_ INT cx,
    _In_ INT cy)
{
    prcl->left += cx;
    prcl->right += cx;
    prcl->top += cy;
    prcl->bottom += cy;
}

FORCEINLINE
BOOL
RECTL_bIsEmptyRect(
    _In_ const RECTL *prcl)
{
    return (prcl->left >= prcl->right || prcl->top >= prcl->bottom);
}

FORCEINLINE
BOOL
RECTL_bPointInRect(
    _In_ const RECTL *prcl,
    _In_ INT x,
    _In_ INT y)
{
    return (x >= prcl->left && x < prcl->right &&
            y >= prcl->top  && y < prcl->bottom);
}

FORCEINLINE
BOOL
RECTL_bIsWellOrdered(
    _In_ const RECTL *prcl)
{
    return ((prcl->left <= prcl->right) &&
            (prcl->top  <= prcl->bottom));
}

BOOL
FASTCALL
RECTL_bUnionRect(
    _Out_ RECTL *prclDst,
    _In_ const RECTL *prcl1,
    _In_ const RECTL *prcl2);

BOOL
FASTCALL
RECTL_bIntersectRect(
    _Out_ RECTL* prclDst,
    _In_ const RECTL* prcl1,
    _In_ const RECTL* prcl2);

VOID
FASTCALL
RECTL_vMakeWellOrdered(
    _Inout_ RECTL *prcl);

VOID
FASTCALL
RECTL_vInflateRect(
    _Inout_ RECTL *rect,
    _In_ INT dx,
    _In_ INT dy);
