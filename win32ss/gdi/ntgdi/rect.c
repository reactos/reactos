/*
 * PROJECT:         ReactOS win32 kernel mode subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/objects/rect.c
 * PURPOSE:         Rect functions
 * PROGRAMMER:      Timo Kreuzer
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

BOOL
FASTCALL
RECTL_bUnionRect(
    _Out_ RECTL *prclDst,
    _In_ const RECTL *prcl1,
    _In_ const RECTL *prcl2)
{
    if (RECTL_bIsEmptyRect(prcl1))
    {
        if (RECTL_bIsEmptyRect(prcl2))
	    {
	        RECTL_vSetEmptyRect(prclDst);
	        return FALSE;
	    }
        else
	    {
	        *prclDst = *prcl2;
	    }
    }
    else
    {
        if (RECTL_bIsEmptyRect(prcl2))
	    {
	        *prclDst = *prcl1;
	    }
        else
	    {
	        prclDst->left = min(prcl1->left, prcl2->left);
	        prclDst->top = min(prcl1->top, prcl2->top);
	        prclDst->right = max(prcl1->right, prcl2->right);
	        prclDst->bottom = max(prcl1->bottom, prcl2->bottom);
	    }
    }

    return TRUE;
}

BOOL
FASTCALL
RECTL_bIntersectRect(
    _Out_ RECTL* prclDst,
    _In_ const RECTL* prcl1,
    _In_ const RECTL* prcl2)
{
    prclDst->left  = max(prcl1->left, prcl2->left);
    prclDst->right = min(prcl1->right, prcl2->right);

    if (prclDst->left < prclDst->right)
    {
        prclDst->top    = max(prcl1->top, prcl2->top);
        prclDst->bottom = min(prcl1->bottom, prcl2->bottom);

        if (prclDst->top < prclDst->bottom)
        {
            return TRUE;
        }
    }

    RECTL_vSetEmptyRect(prclDst);

    return FALSE;
}

VOID
FASTCALL
RECTL_vMakeWellOrdered(
    _Inout_ RECTL *prcl)
{
    LONG lTmp;
    if (prcl->left > prcl->right)
    {
        lTmp = prcl->left;
        prcl->left = prcl->right;
        prcl->right = lTmp;
    }
    if (prcl->top > prcl->bottom)
    {
        lTmp = prcl->top;
        prcl->top = prcl->bottom;
        prcl->bottom = lTmp;
    }
}

VOID
FASTCALL
RECTL_vInflateRect(
    _Inout_ RECTL *rect,
    _In_ INT dx,
    _In_ INT dy)
{
    rect->left -= dx;
    rect->top -= dy;
    rect->right += dx;
    rect->bottom += dy;
}

/* EOF */
