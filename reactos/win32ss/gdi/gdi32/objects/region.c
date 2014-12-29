#include <precomp.h>

#define NDEBUG
#include <debug.h>

#define INRECT(r, x, y) \
      ( ( ((r).right >  x)) && \
      ( ((r).left <= x)) && \
      ( ((r).bottom >  y)) && \
      ( ((r).top <= y)) )

#define OVERLAPPING_RGN 0
#define INVERTED_RGN 1
#define SAME_RGN 2
#define DIFF_RGN 3
/*
 From tests, there are four results based on normalized coordinates.
 If the rects are overlapping and normalized, it's OVERLAPPING_RGN.
 If the rects are overlapping in anyway or same in dimension and one is inverted,
 it's INVERTED_RGN.
 If the rects are same in dimension or NULL, it's SAME_RGN.
 If the rects are overlapping and not normalized or displace in different areas,
 it's DIFF_RGN.
 */
INT
FASTCALL
ComplexityFromRects( PRECTL prc1, PRECTL prc2)
{
    if ( prc2->left >= prc1->left )
    {
        if ( ( prc1->right >= prc2->right) &&
                ( prc1->top <= prc2->top ) &&
                ( prc1->bottom >= prc2->bottom ) )
            return SAME_RGN;

        if ( prc2->left > prc1->left )
        {
            if ( ( prc1->left >= prc2->right ) ||
                    ( prc1->right <= prc2->left ) ||
                    ( prc1->top >= prc2->bottom ) ||
                    ( prc1->bottom <= prc2->top ) )
                return DIFF_RGN;
        }
    }

    if ( ( prc2->right < prc1->right ) ||
            ( prc2->top > prc1->top ) ||
            ( prc2->bottom < prc1->bottom ) )
    {
        if ( ( prc1->left >= prc2->right ) ||
                ( prc1->right <= prc2->left ) ||
                ( prc1->top >= prc2->bottom ) ||
                ( prc1->bottom <= prc2->top ) )
            return DIFF_RGN;
    }
    else
    {
        return INVERTED_RGN;
    }
    return OVERLAPPING_RGN;
}

static
VOID
FASTCALL
SortRects(PRECT pRect, INT nCount)
{
    INT i = 0, a = 0, b = 0, c, s;
    RECT sRect;

    if (nCount > 0)
    {
        i = 1; // set index point
        c = nCount; // set inverse count
        do
        {
            s = i; // set sort count
            if ( i < nCount )
            {
                a = i - 1;
                b = i;
                do
                {
                    if(pRect[b].top != pRect[i].bottom) break;
                    if(pRect[b].left < pRect[a].left)
                    {
                        sRect = pRect[a];
                        pRect[a] = pRect[b];
                        pRect[b] = sRect;
                    }
                    ++s;
                    ++b;
                }
                while ( s < nCount );
            }
            ++i;
        }
        while ( c-- != 1 );
    }
}

/*
 * I thought it was okay to have this in DeleteObject but~ Speed. (jt)
 */
BOOL
FASTCALL
DeleteRegion(
    _In_ HRGN hrgn)
{
#if 0
    PRGN_ATTR Rgn_Attr;

    if ((GdiGetHandleUserData(hrgn, GDI_OBJECT_TYPE_REGION, (PVOID) &Rgn_Attr)) &&
            ( Rgn_Attr != NULL ))
    {
        PGDIBSOBJECT pgO;

        pgO = GdiAllocBatchCommand(NULL, GdiBCDelRgn);
        if (pgO)
        {
            pgO->hgdiobj = hrgn;
            return TRUE;
        }
    }
#endif
    return NtGdiDeleteObjectApp(hrgn);
}

INT
FASTCALL
MirrorRgnByWidth(
    _In_ HRGN hrgn,
    _In_ INT Width,
    _In_ HRGN *phrgn)
{
    INT cRgnDSize, Ret = 0;
    PRGNDATA pRgnData;

    cRgnDSize = NtGdiGetRegionData(hrgn, 0, NULL);

    if (cRgnDSize)
    {
        pRgnData = LocalAlloc(LMEM_FIXED, cRgnDSize * sizeof(LONG));
        if (pRgnData)
        {
            if ( GetRegionData(hrgn, cRgnDSize, pRgnData) )
            {
                HRGN hRgnex;
                UINT i;
                INT SaveL = pRgnData->rdh.rcBound.left;
                pRgnData->rdh.rcBound.left = Width - pRgnData->rdh.rcBound.right;
                pRgnData->rdh.rcBound.right = Width - SaveL;
                if (pRgnData->rdh.nCount > 0)
                {
                    PRECT pRect = (PRECT)&pRgnData->Buffer;
                    for (i = 0; i < pRgnData->rdh.nCount; i++)
                    {
                        SaveL = pRect[i].left;
                        pRect[i].left = Width - pRect[i].right;
                        pRect[i].right = Width - SaveL;
                    }
                }
                SortRects((PRECT)&pRgnData->Buffer, pRgnData->rdh.nCount);
                hRgnex = ExtCreateRegion(NULL, cRgnDSize , pRgnData);
                if (hRgnex)
                {
                    if (phrgn) phrgn = (HRGN *)hRgnex;
                    else
                    {
                        CombineRgn(hrgn, hRgnex, 0, RGN_COPY);
                        DeleteObject(hRgnex);
                    }
                    Ret = 1;
                }
            }
            LocalFree(pRgnData);
        }
    }
    return Ret;
}

INT
WINAPI
MirrorRgnDC(
    _In_ HDC hdc,
    _In_ HRGN hrgn,
    _In_ HRGN *phrn)
{
    if (!GdiIsHandleValid((HGDIOBJ) hdc) ||
        (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC))
        return 0;

    return MirrorRgnByWidth(hrgn, NtGdiGetDeviceWidth(hdc), phrn);
}

/* FUNCTIONS *****************************************************************/

FORCEINLINE
ULONG
IntSetNullRgn(
    _Inout_ PRGN_ATTR prgnattr)
{
    prgnattr->iComplexity = NULLREGION;
    prgnattr->AttrFlags |= ATTR_RGN_DIRTY;
    return NULLREGION;
}

FORCEINLINE
ULONG
IntSetRectRgn(
    _Inout_ PRGN_ATTR prgnattr,
    _In_ INT xLeft,
    _In_ INT yTop,
    _In_ INT xRight,
    _In_ INT yBottom)
{
    ASSERT(xLeft <= xRight);
    ASSERT(yTop <= yBottom);

    if ((xLeft == xRight) || (yTop == yBottom))
        return IntSetNullRgn(prgnattr);

    prgnattr->iComplexity = SIMPLEREGION;
    prgnattr->Rect.left = xLeft;
    prgnattr->Rect.top = yTop;
    prgnattr->Rect.right = xRight;
    prgnattr->Rect.bottom = yBottom;
    prgnattr->AttrFlags |= ATTR_RGN_DIRTY;
    return SIMPLEREGION;
}

/*
 * @implemented
 */
INT
WINAPI
CombineRgn(
    _In_ HRGN hrgnDest,
    _In_ HRGN hrgnSrc1,
    _In_ HRGN hrgnSrc2,
    _In_ INT  iCombineMode)
{
    PRGN_ATTR prngattrDest = NULL;
    PRGN_ATTR prngattrSrc1 = NULL;
    PRGN_ATTR prngattrSrc2 = NULL;
    RECT rcTemp;

    /* Get the region attribute for dest and source 1 */
    prngattrDest = GdiGetRgnAttr(hrgnDest);
    prngattrSrc1 = GdiGetRgnAttr(hrgnSrc1);

    /* If that failed or if the source 1 region is complex, go to win32k */
    if ((prngattrDest == NULL) || (prngattrSrc1 == NULL) ||
        (prngattrSrc1->iComplexity > SIMPLEREGION))
    {
        return NtGdiCombineRgn(hrgnDest, hrgnSrc1, hrgnSrc2, iCombineMode);
    }

    /* Handle RGN_COPY first, it needs only hrgnSrc1 */
    if (iCombineMode == RGN_COPY)
    {
        /* Check if the source region is a NULLREGION */
        if (prngattrSrc1->iComplexity == NULLREGION)
        {
            /* The dest region is a NULLREGION, too */
            return IntSetNullRgn(prngattrDest);
        }

        /* We already know that the source region cannot be complex, so
           create a rect region from the bounds of the source rect */
        return IntSetRectRgn(prngattrDest,
                             prngattrSrc1->Rect.left,
                             prngattrSrc1->Rect.top,
                             prngattrSrc1->Rect.right,
                             prngattrSrc1->Rect.bottom);
    }

    /* For all other operations we need hrgnSrc2 */
    prngattrSrc2 = GdiGetRgnAttr(hrgnSrc2);

    /* If we got no attribute or the region is complex, go to win32k */
    if ((prngattrSrc2 == NULL) || (prngattrSrc2->iComplexity > SIMPLEREGION))
    {
        return NtGdiCombineRgn(hrgnDest, hrgnSrc1, hrgnSrc2, iCombineMode);
    }

    /* Handle RGN_AND */
    if (iCombineMode == RGN_AND)
    {
        /* Check if either of the regions is a NULLREGION */
        if ((prngattrSrc1->iComplexity == NULLREGION) ||
            (prngattrSrc2->iComplexity == NULLREGION))
        {
            /* Result is also a NULLREGION */
            return IntSetNullRgn(prngattrDest);
        }

        /* Get the intersection of the 2 rects */
        if (!IntersectRect(&rcTemp, &prngattrSrc1->Rect, &prngattrSrc2->Rect))
        {
            /* The rects do not intersect, result is a NULLREGION */
            return IntSetNullRgn(prngattrDest);
        }

        /* Use the intersection of the rects */
        return IntSetRectRgn(prngattrDest,
                             rcTemp.left,
                             rcTemp.top,
                             rcTemp.right,
                             rcTemp.bottom);
    }

    /* Handle RGN_DIFF */
    if (iCombineMode == RGN_DIFF)
    {
        /* Check if source 1 is a NULLREGION */
        if (prngattrSrc1->iComplexity == NULLREGION)
        {
            /* The result is a NULLREGION as well */
            return IntSetNullRgn(prngattrDest);
        }

        /* Get the intersection of the 2 rects */
        if ((prngattrSrc2->iComplexity == NULLREGION) ||
            !IntersectRect(&rcTemp, &prngattrSrc1->Rect, &prngattrSrc2->Rect))
        {
            /* The rects do not intersect, dest equals source 1 */
            return IntSetRectRgn(prngattrDest,
                                 prngattrSrc1->Rect.left,
                                 prngattrSrc1->Rect.top,
                                 prngattrSrc1->Rect.right,
                                 prngattrSrc1->Rect.bottom);
        }

        /* We need to check is whether we can subtract the rects. For that
           we call SubtractRect, which will give us the bounding box of the
           subtraction. The function returns FALSE if the resulting rect is
           empty */
        if (!SubtractRect(&rcTemp, &prngattrSrc1->Rect, &rcTemp))
        {
            /* The result is a NULLREGION */
            return IntSetNullRgn(prngattrDest);
        }

        /* Now check if the result of SubtractRect matches the source 1 rect.
           Since we already know that the rects intersect, the result can
           only match the source 1 rect, if it could not be "cut" on either
           side, but the overlapping was on a corner, so the new bounding box
           equals the previous rect */
        if (!EqualRect(&rcTemp, &prngattrSrc1->Rect))
        {
            /* We got a properly subtracted rect, so use it. */
            return IntSetRectRgn(prngattrDest,
                                 rcTemp.left,
                                 rcTemp.top,
                                 rcTemp.right,
                                 rcTemp.bottom);
        }

        /* The result would be a complex region, go to win32k */
        return NtGdiCombineRgn(hrgnDest, hrgnSrc1, hrgnSrc2, iCombineMode);
    }

    /* Handle OR and XOR */
    if ((iCombineMode == RGN_OR) || (iCombineMode == RGN_XOR))
    {
        /* Check if source 1 is a NULLREGION */
        if (prngattrSrc1->iComplexity == NULLREGION)
        {
            /* Check if source 2 is also a NULLREGION */
            if (prngattrSrc2->iComplexity == NULLREGION)
            {
                /* Both are NULLREGIONs, result is also a NULLREGION */
                return IntSetNullRgn(prngattrDest);
            }

            /* The result is equal to source 2 */
            return IntSetRectRgn(prngattrDest,
                                 prngattrSrc2->Rect.left,
                                 prngattrSrc2->Rect.top,
                                 prngattrSrc2->Rect.right,
                                 prngattrSrc2->Rect.bottom );
        }

        /* Check if only source 2 is a NULLREGION */
        if (prngattrSrc2->iComplexity == NULLREGION)
        {
            /* The result is equal to source 1 */
            return IntSetRectRgn(prngattrDest,
                                 prngattrSrc1->Rect.left,
                                 prngattrSrc1->Rect.top,
                                 prngattrSrc1->Rect.right,
                                 prngattrSrc1->Rect.bottom);
        }

        /* Do the rects have the same x extent */
        if ((prngattrSrc1->Rect.left == prngattrSrc2->Rect.left) &&
            (prngattrSrc1->Rect.right == prngattrSrc2->Rect.right))
        {
            /* Do the rects also have the same y extent */
            if ((prngattrSrc1->Rect.top == prngattrSrc2->Rect.top) &&
                (prngattrSrc1->Rect.bottom == prngattrSrc2->Rect.bottom))
            {
                /* Rects are equal, if this is RGN_OR, the result is source 1 */
                if (iCombineMode == RGN_OR)
                {
                    /* The result is equal to source 1 */
                    return IntSetRectRgn(prngattrDest,
                                         prngattrSrc1->Rect.left,
                                         prngattrSrc1->Rect.top,
                                         prngattrSrc1->Rect.right,
                                         prngattrSrc1->Rect.bottom );
                }
                else
                {
                    /* XORing with itself yields an empty region */
                    return IntSetNullRgn(prngattrDest);
                }
            }

            /* Check if the rects are disjoint */
            if ((prngattrSrc2->Rect.bottom < prngattrSrc1->Rect.top) ||
                (prngattrSrc2->Rect.top > prngattrSrc1->Rect.bottom))
            {
                /* The result would be a complex region, go to win32k */
                return NtGdiCombineRgn(hrgnDest, hrgnSrc1, hrgnSrc2, iCombineMode);
            }

            /* Check if this is OR */
            if (iCombineMode == RGN_OR)
            {
                /* Use the maximum extent of both rects combined */
                return IntSetRectRgn(prngattrDest,
                                     prngattrSrc1->Rect.left,
                                     min(prngattrSrc1->Rect.top, prngattrSrc2->Rect.top),
                                     prngattrSrc1->Rect.right,
                                     max(prngattrSrc1->Rect.bottom, prngattrSrc2->Rect.bottom));
            }

            /* Check if the rects are adjacent */
            if (prngattrSrc2->Rect.bottom == prngattrSrc1->Rect.top)
            {
                /* The result is the combined rects */
                return IntSetRectRgn(prngattrDest,
                                     prngattrSrc1->Rect.left,
                                     prngattrSrc2->Rect.top,
                                     prngattrSrc1->Rect.right,
                                     prngattrSrc1->Rect.bottom );
            }
            else if (prngattrSrc2->Rect.top == prngattrSrc1->Rect.bottom)
            {
                /* The result is the combined rects */
                return IntSetRectRgn(prngattrDest,
                                     prngattrSrc1->Rect.left,
                                     prngattrSrc1->Rect.top,
                                     prngattrSrc1->Rect.right,
                                     prngattrSrc2->Rect.bottom );
            }

            /* When we are here, this is RGN_XOR and the rects overlap */
            return NtGdiCombineRgn(hrgnDest, hrgnSrc1, hrgnSrc2, iCombineMode);
        }

        /* Do the rects have the same y extent */
        if ((prngattrSrc1->Rect.top == prngattrSrc2->Rect.top) &&
            (prngattrSrc1->Rect.bottom == prngattrSrc2->Rect.bottom))
        {
            /* Check if the rects are disjoint */
            if ((prngattrSrc2->Rect.right < prngattrSrc1->Rect.left) ||
                (prngattrSrc2->Rect.left > prngattrSrc1->Rect.right))
            {
                /* The result would be a complex region, go to win32k */
                return NtGdiCombineRgn(hrgnDest, hrgnSrc1, hrgnSrc2, iCombineMode);
            }

            /* Check if this is OR */
            if (iCombineMode == RGN_OR)
            {
                /* Use the maximum extent of both rects combined */
                return IntSetRectRgn(prngattrDest,
                                     min(prngattrSrc1->Rect.left, prngattrSrc2->Rect.left),
                                     prngattrSrc1->Rect.top,
                                     max(prngattrSrc1->Rect.right, prngattrSrc2->Rect.right),
                                     prngattrSrc1->Rect.bottom);
            }

            /* Check if the rects are adjacent */
            if (prngattrSrc2->Rect.right == prngattrSrc1->Rect.left)
            {
                /* The result is the combined rects */
                return IntSetRectRgn(prngattrDest,
                                     prngattrSrc2->Rect.left,
                                     prngattrSrc1->Rect.top,
                                     prngattrSrc1->Rect.right,
                                     prngattrSrc1->Rect.bottom );
            }
            else if (prngattrSrc2->Rect.left == prngattrSrc1->Rect.right)
            {
                /* The result is the combined rects */
                return IntSetRectRgn(prngattrDest,
                                     prngattrSrc1->Rect.left,
                                     prngattrSrc1->Rect.top,
                                     prngattrSrc2->Rect.right,
                                     prngattrSrc1->Rect.bottom );
            }

            /* When we are here, this is RGN_XOR and the rects overlap */
            return NtGdiCombineRgn(hrgnDest, hrgnSrc1, hrgnSrc2, iCombineMode);
        }

        /* Last case: RGN_OR and one rect is completely within the other */
        if (iCombineMode == RGN_OR)
        {
            /* Check if rect 1 can contain rect 2 */
            if (prngattrSrc1->Rect.left <= prngattrSrc2->Rect.left)
            {
                /* rect 1 might be the outer one, check of that is true */
                if ((prngattrSrc1->Rect.right >= prngattrSrc2->Rect.right) &&
                    (prngattrSrc1->Rect.top <= prngattrSrc2->Rect.top) &&
                    (prngattrSrc1->Rect.bottom >= prngattrSrc2->Rect.bottom))
                {
                    /* Rect 1 contains rect 2, use it */
                    return IntSetRectRgn(prngattrDest,
                                         prngattrSrc1->Rect.left,
                                         prngattrSrc1->Rect.top,
                                         prngattrSrc1->Rect.right,
                                         prngattrSrc1->Rect.bottom );
                }
            }
            else
            {
                /* rect 2 might be the outer one, check of that is true */
                if ((prngattrSrc2->Rect.right >= prngattrSrc1->Rect.right) &&
                    (prngattrSrc2->Rect.top <= prngattrSrc1->Rect.top) &&
                    (prngattrSrc2->Rect.bottom >= prngattrSrc1->Rect.bottom))
                {
                    /* Rect 2 contains rect 1, use it */
                    return IntSetRectRgn(prngattrDest,
                                         prngattrSrc2->Rect.left,
                                         prngattrSrc2->Rect.top,
                                         prngattrSrc2->Rect.right,
                                         prngattrSrc2->Rect.bottom );
                }
            }
        }

        /* We couldn't handle the operation, go to win32k */
        return NtGdiCombineRgn(hrgnDest, hrgnSrc1, hrgnSrc2, iCombineMode);
    }

    DPRINT1("Invalid iCombineMode %d\n", iCombineMode);
    SetLastError(ERROR_INVALID_PARAMETER);
    return ERROR;
}


/*
 * @implemented
 */
HRGN
WINAPI
CreateEllipticRgnIndirect(
    const RECT *prc
)
{
    /* Notes if prc is NULL it will crash on All Windows NT I checked 2000/XP/VISTA */
    return NtGdiCreateEllipticRgn(prc->left, prc->top, prc->right, prc->bottom);

}

/*
 * @implemented
 */
HRGN
WINAPI
CreatePolygonRgn( const POINT * lppt, int cPoints, int fnPolyFillMode)
{
    return (HRGN) NtGdiPolyPolyDraw( (HDC) fnPolyFillMode, (PPOINT) lppt, (PULONG) &cPoints, 1, GdiPolyPolyRgn);
}

/*
 * @implemented
 */
HRGN
WINAPI
CreatePolyPolygonRgn( const POINT* lppt,
                      const INT* lpPolyCounts,
                      int nCount,
                      int fnPolyFillMode)
{
    return (HRGN) NtGdiPolyPolyDraw(  (HDC) fnPolyFillMode, (PPOINT) lppt, (PULONG) lpPolyCounts, (ULONG) nCount, GdiPolyPolyRgn );
}

/*
 * @implemented
 */
HRGN
WINAPI
CreateRectRgn(int x1, int y1, int x2, int y2)
{
    PRGN_ATTR pRgn_Attr;
    HRGN hrgn;
    int tmp;

/// <-
//// Remove when Brush/Pen/Rgn Attr is ready!
    return NtGdiCreateRectRgn(x1,y1,x2,y2);
////

    /* Normalize points */
    tmp = x1;
    if ( x1 > x2 )
    {
        x1 = x2;
        x2 = tmp;
    }

    tmp = y1;
    if ( y1 > y2 )
    {
        y1 = y2;
        y2 = tmp;
    }
    /* Check outside 24 bit limit for universal set. Chp 9 Areas, pg 560.*/
    if ( x1 < -(1<<27)  ||
            y1 < -(1<<27)  ||
            x2 > (1<<27)-1 ||
            y2 > (1<<27)-1 )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    hrgn = hGetPEBHandle(hctRegionHandle, 0);

    if (!hrgn)
        hrgn = NtGdiCreateRectRgn(0, 0, 1, 1);

    if (!hrgn)
        return hrgn;

    if (!GdiGetHandleUserData((HGDIOBJ) hrgn, GDI_OBJECT_TYPE_REGION, (PVOID) &pRgn_Attr))
    {
        DPRINT1("No Attr for Region handle!!!\n");
        DeleteRegion(hrgn);
        return NULL;
    }

    if (( x1 == x2) || (y1 == y2))
    {
        pRgn_Attr->iComplexity = NULLREGION;
        pRgn_Attr->Rect.left = pRgn_Attr->Rect.top =
                                   pRgn_Attr->Rect.right = pRgn_Attr->Rect.bottom = 0;
    }
    else
    {
        pRgn_Attr->iComplexity = SIMPLEREGION;
        pRgn_Attr->Rect.left   = x1;
        pRgn_Attr->Rect.top    = y1;
        pRgn_Attr->Rect.right  = x2;
        pRgn_Attr->Rect.bottom = y2;
    }

    pRgn_Attr->AttrFlags = (ATTR_RGN_DIRTY|ATTR_RGN_VALID);

    return hrgn;
}

/*
 * @implemented
 */
HRGN
WINAPI
CreateRectRgnIndirect(
    const RECT *prc
)
{
    /* Notes if prc is NULL it will crash on All Windows NT I checked 2000/XP/VISTA */
    return CreateRectRgn(prc->left, prc->top, prc->right, prc->bottom);

}

/*
 * @implemented
 */
INT
WINAPI
ExcludeClipRect(
    _In_ HDC hdc,
    _In_ INT xLeft,
    _In_ INT yTop,
    _In_ INT xRight,
    _In_ INT yBottom)
{
    HANDLE_METADC(INT, ExcludeClipRect, ERROR, hdc, xLeft, yTop, xRight, yBottom);

    return NtGdiExcludeClipRect(hdc, xLeft, yTop, xRight, yBottom);
}

/*
 * @implemented
 */
HRGN
WINAPI
ExtCreateRegion(
    CONST XFORM *	lpXform,
    DWORD		nCount,
    CONST RGNDATA *	lpRgnData
)
{
    if (lpRgnData)
    {
        if ((!lpXform) && (lpRgnData->rdh.nCount == 1))
        {
            PRECT pRect = (PRECT)&lpRgnData->Buffer[0];
            return CreateRectRgn(pRect->left, pRect->top, pRect->right, pRect->bottom);
        }
        return NtGdiExtCreateRegion((LPXFORM) lpXform, nCount,(LPRGNDATA) lpRgnData);
    }
    SetLastError(ERROR_INVALID_PARAMETER);
    return NULL;
}

/*
 * @implemented
 */
INT
WINAPI
ExtSelectClipRgn(
    _In_ HDC hdc,
    _In_ HRGN hrgn,
    _In_ INT iMode)
{
    INT Ret;
    HRGN NewRgn = NULL;

    HANDLE_METADC(INT, ExtSelectClipRgn, 0, hdc, hrgn, iMode);

#if 0
    if ( hrgn )
    {
        if ( GetLayout(hdc) & LAYOUT_RTL )
        {
            if ( MirrorRgnDC(hdc, hrgn, &NewRgn) )
            {
                if ( NewRgn ) hrgn = NewRgn;
            }
        }
    }
#endif
    /* Batch handles RGN_COPY only! */
    if (iMode == RGN_COPY)
    {
#if 0
        PDC_ATTR pDc_Attr;
        PRGN_ATTR pRgn_Attr = NULL;

        /* hrgn can be NULL unless the RGN_COPY mode is specified. */
        if (hrgn)
            GdiGetHandleUserData((HGDIOBJ) hrgn, GDI_OBJECT_TYPE_REGION, (PVOID) &pRgn_Attr);

        if ( GdiGetHandleUserData((HGDIOBJ) hdc, GDI_OBJECT_TYPE_DC, (PVOID) &pDc_Attr) &&
                pDc_Attr )
        {
            PGDI_TABLE_ENTRY pEntry = GdiHandleTable + GDI_HANDLE_GET_INDEX(hdc);
            PTEB pTeb = NtCurrentTeb();

            if ( pTeb->Win32ThreadInfo != NULL &&
                    pTeb->GdiTebBatch.HDC == hdc &&
                    !(pDc_Attr->ulDirty_ & DC_DIBSECTION) &&
                    !(pEntry->Flags & GDI_ENTRY_VALIDATE_VIS) )
            {
                if (!hrgn ||
                        (hrgn && pRgn_Attr && pRgn_Attr->iComplexity <= SIMPLEREGION) )
                {
                    if ((pTeb->GdiTebBatch.Offset + sizeof(GDIBSEXTSELCLPRGN)) <= GDIBATCHBUFSIZE)
                    {
                        // FIXME: This is broken, use GdiAllocBatchCommand!
                        PGDIBSEXTSELCLPRGN pgO = (PGDIBSEXTSELCLPRGN)(&pTeb->GdiTebBatch.Buffer[0] +
                                                 pTeb->GdiTebBatch.Offset);
                        pgO->gbHdr.Cmd = GdiBCExtSelClipRgn;
                        pgO->gbHdr.Size = sizeof(GDIBSEXTSELCLPRGN);
                        pgO->fnMode = iMode;

                        if ( hrgn && pRgn_Attr )
                        {
                            Ret = pRgn_Attr->iComplexity;

                            if ( pDc_Attr->VisRectRegion.Rect.left   >= pRgn_Attr->Rect.right  ||
                                    pDc_Attr->VisRectRegion.Rect.top    >= pRgn_Attr->Rect.bottom ||
                                    pDc_Attr->VisRectRegion.Rect.right  <= pRgn_Attr->Rect.left   ||
                                    pDc_Attr->VisRectRegion.Rect.bottom <= pRgn_Attr->Rect.top )
                                Ret = NULLREGION;

                            pgO->left   = pRgn_Attr->Rect.left;
                            pgO->top    = pRgn_Attr->Rect.top;
                            pgO->right  = pRgn_Attr->Rect.right;
                            pgO->bottom = pRgn_Attr->Rect.bottom;
                        }
                        else
                        {
                            Ret = pDc_Attr->VisRectRegion.Flags;
                            pgO->fnMode |= 0x80000000; // Set no hrgn mode.
                        }
                        pTeb->GdiTebBatch.Offset += sizeof(GDIBSEXTSELCLPRGN);
                        pTeb->GdiBatchCount++;
                        if (pTeb->GdiBatchCount >= GDI_BatchLimit) NtGdiFlush();
                        if ( NewRgn ) DeleteObject(NewRgn);
                        return Ret;
                    }
                }
            }
        }
#endif
    }
    Ret = NtGdiExtSelectClipRgn(hdc, hrgn, iMode);

    if ( NewRgn ) DeleteObject(NewRgn);

    return Ret;
}

/*
 * @implemented
 */
int
WINAPI
GetClipRgn(
    HDC     hdc,
    HRGN    hrgn
)
{
    INT Ret;

    /* Check if DC handle is valid */
    if (!GdiGetDcAttr(hdc))
    {
        /* Last error code differs from what NtGdiGetRandomRgn returns */
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    Ret = NtGdiGetRandomRgn(hdc, hrgn, CLIPRGN);

//  if (Ret)
//  {
//     if(GetLayout(hdc) & LAYOUT_RTL) MirrorRgnDC(hdc,(HRGN)Ret, NULL);
//  }
    return Ret;
}

/*
 * @implemented
 */
int
WINAPI
GetMetaRgn(HDC hdc,
           HRGN hrgn)
{
    return NtGdiGetRandomRgn(hdc, hrgn, METARGN);
}

/*
 * @implemented
 *
 */
DWORD
WINAPI
GetRegionData(HRGN hrgn,
              DWORD nCount,
              LPRGNDATA lpRgnData)
{
    if (!lpRgnData)
    {
        nCount = 0;
    }

    return NtGdiGetRegionData(hrgn,nCount,lpRgnData);
}

/*
 * @implemented
 *
 */
INT
WINAPI
GetRgnBox(HRGN hrgn,
          LPRECT prcOut)
{
    PRGN_ATTR Rgn_Attr;

    //if (!GdiGetHandleUserData((HGDIOBJ) hrgn, GDI_OBJECT_TYPE_REGION, (PVOID) &Rgn_Attr))
    return NtGdiGetRgnBox(hrgn, prcOut);

    if (Rgn_Attr->iComplexity == NULLREGION)
    {
        prcOut->left   = 0;
        prcOut->top    = 0;
        prcOut->right  = 0;
        prcOut->bottom = 0;
    }
    else
    {
        if (Rgn_Attr->iComplexity != SIMPLEREGION)
            return NtGdiGetRgnBox(hrgn, prcOut);
        /* WARNING! prcOut is never checked newbies! */
        RtlCopyMemory( prcOut, &Rgn_Attr->Rect, sizeof(RECT));
    }
    return Rgn_Attr->iComplexity;
}

/*
 * @implemented
 */
INT
WINAPI
IntersectClipRect(
    _In_ HDC hdc,
    _In_ INT nLeft,
    _In_ INT nTop,
    _In_ INT nRight,
    _In_ INT nBottom)
{
    HANDLE_METADC(INT, IntersectClipRect, ERROR, hdc, nLeft, nTop, nRight, nBottom);
    return NtGdiIntersectClipRect(hdc, nLeft, nTop, nRight, nBottom);
}

/*
 * @implemented
 */
BOOL
WINAPI
MirrorRgn(HWND hwnd, HRGN hrgn)
{
    RECT Rect;
    GetWindowRect(hwnd, &Rect);
    return MirrorRgnByWidth(hrgn, Rect.right - Rect.left, NULL);
}

/*
 * @implemented
 */
INT
WINAPI
OffsetClipRgn(
    HDC hdc,
    INT nXOffset,
    INT nYOffset)
{
    HANDLE_METADC(INT, OffsetClipRgn, ERROR, hdc, nXOffset, nYOffset);
    return NtGdiOffsetClipRgn(hdc, nXOffset, nYOffset);
}

/*
 * @implemented
 *
 */
INT
WINAPI
OffsetRgn( HRGN hrgn,
           int nXOffset,
           int nYOffset)
{
    PRGN_ATTR pRgn_Attr;
    int nLeftRect, nTopRect, nRightRect, nBottomRect;

// HACKFIX
//  if (!GdiGetHandleUserData((HGDIOBJ) hrgn, GDI_OBJECT_TYPE_REGION, (PVOID) &pRgn_Attr))
    return NtGdiOffsetRgn(hrgn,nXOffset,nYOffset);

    if ( pRgn_Attr->iComplexity == NULLREGION)
        return pRgn_Attr->iComplexity;

    if ( pRgn_Attr->iComplexity != SIMPLEREGION)
        return NtGdiOffsetRgn(hrgn,nXOffset,nYOffset);

    nLeftRect   = pRgn_Attr->Rect.left;
    nTopRect    = pRgn_Attr->Rect.top;
    nRightRect  = pRgn_Attr->Rect.right;
    nBottomRect = pRgn_Attr->Rect.bottom;

    if (nLeftRect < nRightRect)
    {
        if (nTopRect < nBottomRect)
        {
            nLeftRect   = nXOffset + nLeftRect;
            nTopRect    = nYOffset + nTopRect;
            nRightRect  = nXOffset + nRightRect;
            nBottomRect = nYOffset + nBottomRect;

            /* Check 28 bit limit. Chp 9 Areas, pg 560. */
            if ( nLeftRect   < -(1<<27)  ||
                    nTopRect    < -(1<<27)  ||
                    nRightRect  > (1<<27)-1 ||
                    nBottomRect > (1<<27)-1  )
            {
                return ERROR;
            }

            pRgn_Attr->Rect.top    = nTopRect;
            pRgn_Attr->Rect.left   = nLeftRect;
            pRgn_Attr->Rect.right  = nRightRect;
            pRgn_Attr->Rect.bottom = nBottomRect;
            pRgn_Attr->AttrFlags |= ATTR_RGN_DIRTY;
        }
    }
    return pRgn_Attr->iComplexity;
}

/*
 * @implemented
 */
BOOL
WINAPI
PtInRegion(IN HRGN hrgn,
           int x,
           int y)
{
    PRGN_ATTR pRgn_Attr;

    // HACKFIX
    //if (!GdiGetHandleUserData((HGDIOBJ) hrgn, GDI_OBJECT_TYPE_REGION, (PVOID) &pRgn_Attr))
    return NtGdiPtInRegion(hrgn,x,y);

    if ( pRgn_Attr->iComplexity == NULLREGION)
        return FALSE;

    if ( pRgn_Attr->iComplexity != SIMPLEREGION)
        return NtGdiPtInRegion(hrgn,x,y);

    return INRECT( pRgn_Attr->Rect, x, y);
}

/*
 * @implemented
 */
BOOL
WINAPI
RectInRegion(HRGN hrgn,
             LPCRECT prcl)
{
    PRGN_ATTR pRgn_Attr;
    RECTL rc;

    // HACKFIX
    //if (!GdiGetHandleUserData((HGDIOBJ) hrgn, GDI_OBJECT_TYPE_REGION, (PVOID) &pRgn_Attr))
    return NtGdiRectInRegion(hrgn, (LPRECT) prcl);

    if ( pRgn_Attr->iComplexity == NULLREGION)
        return FALSE;

    if ( pRgn_Attr->iComplexity != SIMPLEREGION)
        return NtGdiRectInRegion(hrgn, (LPRECT) prcl);

    /* swap the coordinates to make right >= left and bottom >= top */
    /* (region building rectangles are normalized the same way) */
    if ( prcl->top > prcl->bottom)
    {
        rc.top = prcl->bottom;
        rc.bottom = prcl->top;
    }
    else
    {
        rc.top = prcl->top;
        rc.bottom = prcl->bottom;
    }
    if ( prcl->right < prcl->left)
    {
        rc.right = prcl->left;
        rc.left = prcl->right;
    }
    else
    {
        rc.right = prcl->right;
        rc.left = prcl->left;
    }

    if ( ComplexityFromRects( &pRgn_Attr->Rect, &rc) != DIFF_RGN )
        return TRUE;

    return FALSE;
}

/*
 * @implemented
 */
int
WINAPI
SelectClipRgn(
    _In_ HDC hdc,
    _In_ HRGN hrgn)
{
    return ExtSelectClipRgn(hdc, hrgn, RGN_COPY);
}

/*
 * @implemented
 */
BOOL
WINAPI
SetRectRgn(
    _In_ HRGN hrgn,
    _In_ INT xLeft,
    _In_ INT yTop,
    _In_ INT xRight,
    _In_ INT yBottom)
{
    PRGN_ATTR prngattr;

    /* Try to get the region attribute */
    prngattr = GdiGetRgnAttr(hrgn);
    if (prngattr == NULL)
    {
        return NtGdiSetRectRgn(hrgn, xLeft, yTop, xRight, yBottom);
    }

    /* check for NULL region */
    if ((xLeft == xRight) || (yTop == yBottom))
    {
        IntSetNullRgn(prngattr);
        return TRUE;
    }

    if (xLeft > xRight)
    {
        prngattr->Rect.left   = xRight;
        prngattr->Rect.right  = xLeft;
    }
    else
    {
        prngattr->Rect.left   = xLeft;
        prngattr->Rect.right  = xRight;
    }

    if (yTop > yBottom)
    {
        prngattr->Rect.top    = yBottom;
        prngattr->Rect.bottom = yTop;
    }
    else
    {
        prngattr->Rect.top    = yTop;
        prngattr->Rect.bottom = yBottom;
    }

    prngattr->AttrFlags |= ATTR_RGN_DIRTY ;
    prngattr->iComplexity = SIMPLEREGION;

    return TRUE;
}

/*
 * @implemented
 */
int
WINAPI
SetMetaRgn(HDC hDC)
{
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_DC)
        return NtGdiSetMetaRgn(hDC);
#if 0
    PLDC pLDC = GdiGetLDC(hDC);
    if ( pLDC && GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_METADC )
    {
        if (pLDC->iType == LDC_EMFLDC || EMFDRV_SetMetaRgn(hDC))
        {
            return NtGdiSetMetaRgn(hDC);
        }
        else
            SetLastError(ERROR_INVALID_HANDLE);
    }
#endif
    return ERROR;
}

