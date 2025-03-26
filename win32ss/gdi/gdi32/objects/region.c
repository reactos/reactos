#include <precomp.h>

#define NDEBUG
#include <debug.h>

//
// "Windows Graphics Programming: Win32 GDI and DirectDraw",
//   Chp 9 Areas, Region, Set Operations on Regions, hard copy pg 560.
//   universal set's bounding box to be [-(1 << 27), -(1 << 27), (1 << 27) -1, (1 << 27) -1].
//
#define MIN_COORD (INT_MIN/16) // See also ntgdi/region.c
#define MAX_COORD (INT_MAX/16)

#define INRECT(r, x, y) \
      ( ( ((r).right >  x)) && \
      ( ((r).left <= x)) && \
      ( ((r).bottom >  y)) && \
      ( ((r).top <= y)) )

static
VOID
FASTCALL
SortRects(PRECT pRect, INT nCount)
{
    INT i, a, b, c, s;
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
                a = i - 1; // [0]
                b = i;     // [1]
                do
                {
                    if ( pRect[a].top != pRect[b].top ) break;
                    if ( pRect[a].left > pRect[b].left )
                    {
                        sRect = pRect[a];
                        pRect[a] = pRect[b];
                        pRect[b] = sRect;
                    }
                    ++s;
                    b++;
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
    PRGN_ATTR Rgn_Attr = GdiGetRgnAttr(hrgn);

    if ( Rgn_Attr )
    {
        PGDIBSOBJECT pgO;

        pgO = GdiAllocBatchCommand(NULL, GdiBCDelRgn);
        if (pgO)
        {
            pgO->hgdiobj = hrgn;
            return TRUE;
        }
    }
    return NtGdiDeleteObjectApp(hrgn);
}

INT
FASTCALL
MirrorRgnByWidth(
    _In_ HRGN hrgn,
    _In_ INT Width,
    _Out_opt_ HRGN *phrgn)
{
    INT cRgnDSize, Ret = 0;
    PRGNDATA pRgnData;

    cRgnDSize = NtGdiGetRegionData(hrgn, 0, NULL);

    if (cRgnDSize)
    {
        pRgnData = HeapAlloc(GetProcessHeap(), 0, cRgnDSize * sizeof(LONG));
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
                    if (phrgn)
                    {
                        *phrgn = hRgnex;
                    }
                    else
                    {
                        CombineRgn(hrgn, hRgnex, 0, RGN_COPY);
                        DeleteObject(hRgnex);
                    }
                    Ret = 1;
                }
            }
            HeapFree( GetProcessHeap(), 0, pRgnData);
        }
    }
    return Ret;
}

INT
WINAPI
MirrorRgnDC(
    _In_ HDC hdc,
    _In_ HRGN hrgn,
    _Out_opt_ HRGN *phrn)
{
    if (!GdiValidateHandle((HGDIOBJ) hdc) ||
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
    prgnattr->Rect.left = prgnattr->Rect.top = prgnattr->Rect.right = prgnattr->Rect.bottom = 0;
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
    return (HRGN) NtGdiPolyPolyDraw( (HDC)UlongToHandle(fnPolyFillMode), (PPOINT) lppt, (PULONG) &cPoints, 1, GdiPolyPolyRgn);
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
    return (HRGN) NtGdiPolyPolyDraw(  (HDC)UlongToHandle(fnPolyFillMode), (PPOINT) lppt, (PULONG) lpPolyCounts, (ULONG) nCount, GdiPolyPolyRgn );
}

/*
 * @implemented
 */
HRGN
WINAPI
CreateRectRgn(int x1, int y1, int x2, int y2)
{
    PRGN_ATTR pRgn_Attr;
    HRGN hrgn = NULL;
    int tmp;

    /* Normalize points, REGION_SetRectRgn does this too. */
    if ( x1 > x2 )
    {
        tmp = x1;
        x1 = x2;
        x2 = tmp;
    }

    if ( y1 > y2 )
    {
        tmp = y1;
        y1 = y2;
        y2 = tmp;
    }
    /* Check outside 28 bit limit for universal set bound box. REGION_SetRectRgn doesn't do this! */
    if ( x1 < MIN_COORD ||
         y1 < MIN_COORD ||
         x2 > MAX_COORD ||
         y2 > MAX_COORD  )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    hrgn = hGetPEBHandle(hctRegionHandle, 0);
    if (hrgn)
    {
       DPRINT1("PEB Handle Cache Test return hrgn %p, should be NULL!\n",hrgn);
       hrgn = NULL;
    }

    if (!hrgn)
        hrgn = NtGdiCreateRectRgn(0, 0, 1, 1);

    if (!hrgn)
        return hrgn;

    if (!(pRgn_Attr = GdiGetRgnAttr(hrgn)) )
    {
        DPRINT1("No Attr for Region handle!!!\n");
        DeleteRegion(hrgn);
        return NULL;
    }

    pRgn_Attr->AttrFlags = ATTR_RGN_VALID;

    IntSetRectRgn( pRgn_Attr, x1, y1, x2, y2 );

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
        PDC_ATTR pdcattr;
        PRGN_ATTR pRgn_Attr = NULL;

        /* Get the DC attribute */
        pdcattr = GdiGetDcAttr(hdc);
        if ( pdcattr )
        {
            PGDI_TABLE_ENTRY pEntry = GdiHandleTable + GDI_HANDLE_GET_INDEX(hdc);

            /* hrgn can be NULL unless the RGN_COPY mode is specified. */
            if (hrgn) pRgn_Attr = GdiGetRgnAttr(hrgn);

            if ( !(pdcattr->ulDirty_ & DC_DIBSECTION) &&
                 !(pEntry->Flags & GDI_ENTRY_VALIDATE_VIS) )
            {
                if (!hrgn || (hrgn && pRgn_Attr && pRgn_Attr->iComplexity <= SIMPLEREGION) )
                {
                    PGDIBSEXTSELCLPRGN pgO = GdiAllocBatchCommand(hdc, GdiBCExtSelClipRgn);
                    if (pgO)
                    {
                        pgO->fnMode = iMode;

                        if ( hrgn && pRgn_Attr )
                        {
                            Ret = pRgn_Attr->iComplexity;
                            // Note from ntgdi/dcstate.c : "The VisRectRegion field needs to be set to a valid state."
                            if ( pdcattr->VisRectRegion.Rect.left   >= pRgn_Attr->Rect.right  ||
                                 pdcattr->VisRectRegion.Rect.top    >= pRgn_Attr->Rect.bottom ||
                                 pdcattr->VisRectRegion.Rect.right  <= pRgn_Attr->Rect.left   ||
                                 pdcattr->VisRectRegion.Rect.bottom <= pRgn_Attr->Rect.top )
                                Ret = NULLREGION;

                            // Pass the rect since this region will go away.
                            pgO->rcl = pRgn_Attr->Rect;
                        }
                        else
                        {
                            Ret = pdcattr->VisRectRegion.iComplexity;
                            pgO->fnMode |= GDIBS_NORECT; // Set no hrgn mode.
                        }
                        if ( NewRgn ) DeleteObject(NewRgn);
                        return Ret;
                    }
                }
            }
        }
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
        return -1;
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

    if (!(Rgn_Attr = GdiGetRgnAttr(hrgn)))
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
    INT l;
    RECT Rect;
    GetWindowRect(hwnd, &Rect);
    l = Rect.right - Rect.left;
    Rect.right -= Rect.left;
    return MirrorRgnByWidth(hrgn, l, NULL);
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
    RECTL rc;

    if (!(pRgn_Attr = GdiGetRgnAttr(hrgn)))
        return NtGdiOffsetRgn(hrgn,nXOffset,nYOffset);

    if ( pRgn_Attr->iComplexity == NULLREGION)
        return pRgn_Attr->iComplexity;

    if ( pRgn_Attr->iComplexity != SIMPLEREGION)
        return NtGdiOffsetRgn(hrgn,nXOffset,nYOffset);

    rc = pRgn_Attr->Rect;

    if (rc.left < rc.right)
    {
        if (rc.top < rc.bottom)
        {
            rc.left   += nXOffset;
            rc.top    += nYOffset;
            rc.right  += nXOffset;
            rc.bottom += nYOffset;

            /* Make sure the offset is within the legal range */
            if ( (rc.left   & MIN_COORD && ((rc.left   & MIN_COORD) != MIN_COORD)) ||
                 (rc.top    & MIN_COORD && ((rc.top    & MIN_COORD) != MIN_COORD)) ||
                 (rc.right  & MIN_COORD && ((rc.right  & MIN_COORD) != MIN_COORD)) ||
                 (rc.bottom & MIN_COORD && ((rc.bottom & MIN_COORD) != MIN_COORD))  )
            {
                DPRINT("OffsetRgn ERROR\n");
                return ERROR;
            }

            pRgn_Attr->Rect = rc;
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

    if (!(pRgn_Attr = GdiGetRgnAttr(hrgn)))
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

    if (!(pRgn_Attr = GdiGetRgnAttr(hrgn)))
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

    if ( ( pRgn_Attr->Rect.left   >= rc.right )  ||
         ( pRgn_Attr->Rect.right  <= rc.left )   ||
         ( pRgn_Attr->Rect.top    >= rc.bottom ) ||
         ( pRgn_Attr->Rect.bottom <= rc.top ) )
    {
        return FALSE;
    }

    return TRUE;
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
    if (GDI_HANDLE_GET_TYPE(hDC) != GDILoObjType_LO_DC_TYPE)
    {
        PLDC pLDC = GdiGetLDC(hDC);
        if ( pLDC && GDI_HANDLE_GET_TYPE(hDC) != GDILoObjType_LO_METADC16_TYPE )
        {
            if (pLDC->iType == LDC_EMFLDC && !EMFDC_SetMetaRgn( pLDC ))
            {
                return ERROR;
            }
        }
        else
        {
            SetLastError(ERROR_INVALID_HANDLE);
            return ERROR;
        }
    }
    return NtGdiSetMetaRgn(hDC);
}

