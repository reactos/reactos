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
DeleteRegion( HRGN hRgn )
{
#if 0
    PRGN_ATTR Rgn_Attr;

    if ((GdiGetHandleUserData((HGDIOBJ) hRgn, GDI_OBJECT_TYPE_REGION, (PVOID) &Rgn_Attr)) &&
            ( Rgn_Attr != NULL ))
    {
        PGDIBSOBJECT pgO;

        pgO = GdiAllocBatchCommand(NULL, GdiBCDelRgn);
        if (pgO)
        {
            pgO->hgdiobj = (HGDIOBJ)hRgn;
            return TRUE;
        }
    }
#endif
    return NtGdiDeleteObjectApp((HGDIOBJ) hRgn);
}

INT
FASTCALL
MirrorRgnByWidth(HRGN hRgn, INT Width, HRGN *phRgn)
{
    INT cRgnDSize, Ret = 0;
    PRGNDATA pRgnData;

    cRgnDSize = NtGdiGetRegionData(hRgn, 0, NULL);

    if (cRgnDSize)
    {
        pRgnData = LocalAlloc(LMEM_FIXED, cRgnDSize * sizeof(LONG));
        if (pRgnData)
        {
            if ( GetRegionData(hRgn, cRgnDSize, pRgnData) )
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
                    if (phRgn) phRgn = (HRGN *)hRgnex;
                    else
                    {
                        CombineRgn(hRgn, hRgnex, 0, RGN_COPY);
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
MirrorRgnDC(HDC hdc, HRGN hRgn, HRGN *phRgn)
{
    if (!GdiIsHandleValid((HGDIOBJ) hdc) ||
            (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)) return 0;

    return MirrorRgnByWidth(hRgn, NtGdiGetDeviceWidth(hdc), phRgn);
}

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
INT
WINAPI
CombineRgn(HRGN  hDest,
           HRGN  hSrc1,
           HRGN  hSrc2,
           INT  CombineMode)
{
    PRGN_ATTR pRgn_Attr_Dest = NULL;
    PRGN_ATTR pRgn_Attr_Src1 = NULL;
    PRGN_ATTR pRgn_Attr_Src2 = NULL;
    INT Complexity;
    BOOL Ret;

// HACK
    return NtGdiCombineRgn(hDest, hSrc1, hSrc2, CombineMode);

    Ret = GdiGetHandleUserData((HGDIOBJ) hDest, GDI_OBJECT_TYPE_REGION, (PVOID) &pRgn_Attr_Dest);
    Ret = GdiGetHandleUserData((HGDIOBJ) hSrc1, GDI_OBJECT_TYPE_REGION, (PVOID) &pRgn_Attr_Src1);

    if ( !Ret ||
            !pRgn_Attr_Dest ||
            !pRgn_Attr_Src1 ||
            pRgn_Attr_Src1->iComplexity > SIMPLEREGION )
        return NtGdiCombineRgn(hDest, hSrc1, hSrc2, CombineMode);

    /* Handle COPY and use only src1. */
    if ( CombineMode == RGN_COPY )
    {
        switch (pRgn_Attr_Src1->iComplexity)
        {
        case NULLREGION:
            Ret = SetRectRgn( hDest, 0, 0, 0, 0);
            if (Ret)
                return NULLREGION;
            goto ERROR_Exit;

        case SIMPLEREGION:
            Ret = SetRectRgn( hDest,
                              pRgn_Attr_Src1->Rect.left,
                              pRgn_Attr_Src1->Rect.top,
                              pRgn_Attr_Src1->Rect.right,
                              pRgn_Attr_Src1->Rect.bottom );
            if (Ret)
                return SIMPLEREGION;
            goto ERROR_Exit;

        case COMPLEXREGION:
        default:
            return NtGdiCombineRgn(hDest, hSrc1, hSrc2, CombineMode);
        }
    }

    Ret = GdiGetHandleUserData((HGDIOBJ) hSrc2, GDI_OBJECT_TYPE_REGION, (PVOID) &pRgn_Attr_Src2);
    if ( !Ret ||
            !pRgn_Attr_Src2 ||
            pRgn_Attr_Src2->iComplexity > SIMPLEREGION )
        return NtGdiCombineRgn(hDest, hSrc1, hSrc2, CombineMode);

    /* All but AND. */
    if ( CombineMode != RGN_AND)
    {
        if ( CombineMode <= RGN_AND)
        {
            /*
               There might be some type of junk in the call, so go K.
               If this becomes a problem, need to setup parameter check at the top.
             */
            DPRINT1("Might be junk! CombineMode %d\n",CombineMode);
            return NtGdiCombineRgn(hDest, hSrc1, hSrc2, CombineMode);
        }

        if ( CombineMode > RGN_XOR) /* Handle DIFF. */
        {
            if ( CombineMode != RGN_DIFF)
            {
                /* Filter check! Well, must be junk?, so go K. */
                DPRINT1("RGN_COPY was handled! CombineMode %d\n",CombineMode);
                return NtGdiCombineRgn(hDest, hSrc1, hSrc2, CombineMode);
            }
            /* Now handle DIFF. */
            if ( pRgn_Attr_Src1->iComplexity == NULLREGION )
            {
                if (SetRectRgn( hDest, 0, 0, 0, 0))
                    return NULLREGION;
                goto ERROR_Exit;
            }

            if ( pRgn_Attr_Src2->iComplexity != NULLREGION )
            {
                Complexity = ComplexityFromRects( &pRgn_Attr_Src1->Rect, &pRgn_Attr_Src2->Rect);

                if ( Complexity != DIFF_RGN )
                {
                    if ( Complexity != INVERTED_RGN)
                        /* If same or overlapping and norm just go K. */
                        return NtGdiCombineRgn(hDest, hSrc1, hSrc2, CombineMode);

                    if (SetRectRgn( hDest, 0, 0, 0, 0))
                        return NULLREGION;
                    goto ERROR_Exit;
                }
            }
        }
        else /* Handle OR or XOR. */
        {
            if ( pRgn_Attr_Src1->iComplexity == NULLREGION )
            {
                if ( pRgn_Attr_Src2->iComplexity != NULLREGION )
                {
                    /* Src1 null and not NULL, set from src2. */
                    Ret = SetRectRgn( hDest,
                                      pRgn_Attr_Src2->Rect.left,
                                      pRgn_Attr_Src2->Rect.top,
                                      pRgn_Attr_Src2->Rect.right,
                                      pRgn_Attr_Src2->Rect.bottom );
                    if (Ret)
                        return SIMPLEREGION;
                    goto ERROR_Exit;
                }
                /* Both are NULL. */
                if (SetRectRgn( hDest, 0, 0, 0, 0))
                    return NULLREGION;
                goto ERROR_Exit;
            }
            /* Src1 is not NULL. */
            if ( pRgn_Attr_Src2->iComplexity != NULLREGION )
            {
                if ( CombineMode != RGN_OR ) /* Filter XOR, so go K. */
                    return NtGdiCombineRgn(hDest, hSrc1, hSrc2, CombineMode);

                Complexity = ComplexityFromRects( &pRgn_Attr_Src1->Rect, &pRgn_Attr_Src2->Rect);
                /* If inverted use Src2. */
                if ( Complexity == INVERTED_RGN)
                {
                    Ret = SetRectRgn( hDest,
                                      pRgn_Attr_Src2->Rect.left,
                                      pRgn_Attr_Src2->Rect.top,
                                      pRgn_Attr_Src2->Rect.right,
                                      pRgn_Attr_Src2->Rect.bottom );
                    if (Ret)
                        return SIMPLEREGION;
                    goto ERROR_Exit;
                }
                /* Not NULL or overlapping or differentiated, go to K. */
                if ( Complexity != SAME_RGN)
                    return NtGdiCombineRgn(hDest, hSrc1, hSrc2, CombineMode);
                /* If same, just fall through. */
            }
        }
        Ret = SetRectRgn( hDest,
                          pRgn_Attr_Src1->Rect.left,
                          pRgn_Attr_Src1->Rect.top,
                          pRgn_Attr_Src1->Rect.right,
                          pRgn_Attr_Src1->Rect.bottom );
        if (Ret)
            return SIMPLEREGION;
        goto ERROR_Exit;
    }

    /* Handle AND.  */
    if ( pRgn_Attr_Src1->iComplexity != NULLREGION &&
            pRgn_Attr_Src2->iComplexity != NULLREGION )
    {
        Complexity = ComplexityFromRects( &pRgn_Attr_Src1->Rect, &pRgn_Attr_Src2->Rect);

        if ( Complexity == DIFF_RGN ) /* Differentiated in anyway just NULL rgn. */
        {
            if (SetRectRgn( hDest, 0, 0, 0, 0))
                return NULLREGION;
            goto ERROR_Exit;
        }

        if ( Complexity != INVERTED_RGN) /* Not inverted and overlapping. */
        {
            if ( Complexity != SAME_RGN) /* Must be norm and overlapping. */
                return NtGdiCombineRgn(hDest, hSrc1, hSrc2, CombineMode);
            /* Merge from src2.  */
            Ret = SetRectRgn( hDest,
                              pRgn_Attr_Src2->Rect.left,
                              pRgn_Attr_Src2->Rect.top,
                              pRgn_Attr_Src2->Rect.right,
                              pRgn_Attr_Src2->Rect.bottom );
            if (Ret)
                return SIMPLEREGION;
            goto ERROR_Exit;
        }
        /* Inverted so merge from src1. */
        Ret = SetRectRgn( hDest,
                          pRgn_Attr_Src1->Rect.left,
                          pRgn_Attr_Src1->Rect.top,
                          pRgn_Attr_Src1->Rect.right,
                          pRgn_Attr_Src1->Rect.bottom );
        if (Ret)
            return SIMPLEREGION;
        goto ERROR_Exit;
    }

    /* It's all NULL! */
    if (SetRectRgn( hDest, 0, 0, 0, 0))
        return NULLREGION;

ERROR_Exit:
    /* Even on error the flag is set dirty and force server side to redraw. */
    pRgn_Attr_Dest->AttrFlags |= ATTR_RGN_DIRTY;
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
ExcludeClipRect(IN HDC hdc, IN INT xLeft, IN INT yTop, IN INT xRight, IN INT yBottom)
{
#if 0
// Handle something other than a normal dc object.
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_ExcludeClipRect( hdc, xLeft, yTop, xRight, yBottom);
        else
        {
            PLDC pLDC = GdiGetLDC(hdc);
            if ( pLDC )
            {
                if (pLDC->iType != LDC_EMFLDC || EMFDRV_ExcludeClipRect( hdc, xLeft, yTop, xRight, yBottom))
                    return NtGdiExcludeClipRect(hdc, xLeft, yTop, xRight, yBottom);
            }
            else
                SetLastError(ERROR_INVALID_HANDLE);
            return ERROR;
        }
    }
#endif
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
ExtSelectClipRgn( IN HDC hdc, IN HRGN hrgn, IN INT iMode)
{
    INT Ret;
    HRGN NewRgn = NULL;

#if 0
// Handle something other than a normal dc object.
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_ExtSelectClipRgn( hdc, );
        else
        {
            PLDC pLDC = GdiGetLDC(hdc);
            if ( pLDC )
            {
                if (pLDC->iType != LDC_EMFLDC || EMFDRV_ExtSelectClipRgn( hdc, ))
                    return NtGdiExtSelectClipRgn(hdc, );
            }
            else
                SetLastError(ERROR_INVALID_HANDLE);
            return ERROR;
        }
    }
#endif
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
                            Ret = pDc_Attr->VisRectRegion.iComplexity;
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
IntersectClipRect(HDC hdc,
                  int nLeftRect,
                  int nTopRect,
                  int nRightRect,
                  int nBottomRect)
{
#if 0
// Handle something other than a normal dc object.
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_IntersectClipRect( hdc, nLeftRect, nTopRect, nRightRect, nBottomRect);
        else
        {
            PLDC pLDC = GdiGetLDC(hdc);
            if ( pLDC )
            {
                if (pLDC->iType != LDC_EMFLDC || EMFDRV_IntersectClipRect( hdc, nLeftRect, nTopRect, nRightRect, nBottomRect))
                    return NtGdiIntersectClipRect(hdc, nLeftRect, nTopRect, nRightRect, nBottomRect);
            }
            else
                SetLastError(ERROR_INVALID_HANDLE);
            return ERROR;
        }
    }
#endif
    return NtGdiIntersectClipRect(hdc, nLeftRect, nTopRect, nRightRect, nBottomRect);
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
OffsetClipRgn(HDC hdc,
              int nXOffset,
              int nYOffset)
{
#if 0
// Handle something other than a normal dc object.
    if (GDI_HANDLE_GET_TYPE(hdc) != GDI_OBJECT_TYPE_DC)
    {
        if (GDI_HANDLE_GET_TYPE(hdc) == GDI_OBJECT_TYPE_METADC)
            return MFDRV_OffsetClipRgn( hdc, nXOffset, nYOffset );
        else
        {
            PLDC pLDC = GdiGetLDC(hdc);
            if ( !pLDC )
            {
                SetLastError(ERROR_INVALID_HANDLE);
                return ERROR;
            }
            if (pLDC->iType == LDC_EMFLDC && !EMFDRV_OffsetClipRgn( hdc, nXOffset, nYOffset ))
                return ERROR;
            return NtGdiOffsetClipRgn( hdc,  nXOffset,  nYOffset);
        }
    }
#endif
    return NtGdiOffsetClipRgn( hdc,  nXOffset,  nYOffset);
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
int WINAPI
SelectClipRgn(
    HDC     hdc,
    HRGN    hrgn
)
{
    return ExtSelectClipRgn(hdc, hrgn, RGN_COPY);
}

/*
 * @implemented
 */
BOOL
WINAPI
SetRectRgn(HRGN hrgn,
           int nLeftRect,
           int nTopRect,
           int nRightRect,
           int nBottomRect)
{
    PRGN_ATTR Rgn_Attr;

    //if (!GdiGetHandleUserData((HGDIOBJ) hrgn, GDI_OBJECT_TYPE_REGION, (PVOID) &Rgn_Attr))
    return NtGdiSetRectRgn(hrgn, nLeftRect, nTopRect, nRightRect, nBottomRect);

    if ((nLeftRect == nRightRect) || (nTopRect == nBottomRect))
    {
        Rgn_Attr->AttrFlags |= ATTR_RGN_DIRTY;
        Rgn_Attr->iComplexity = NULLREGION;
        Rgn_Attr->Rect.left = Rgn_Attr->Rect.top =
                                  Rgn_Attr->Rect.right = Rgn_Attr->Rect.bottom = 0;
        return TRUE;
    }

    Rgn_Attr->Rect.left   = nLeftRect;
    Rgn_Attr->Rect.top    = nTopRect;
    Rgn_Attr->Rect.right  = nRightRect;
    Rgn_Attr->Rect.bottom = nBottomRect;

    if(nLeftRect > nRightRect)
    {
        Rgn_Attr->Rect.left   = nRightRect;
        Rgn_Attr->Rect.right  = nLeftRect;
    }
    if(nTopRect > nBottomRect)
    {
        Rgn_Attr->Rect.top    = nBottomRect;
        Rgn_Attr->Rect.bottom = nTopRect;
    }

    Rgn_Attr->AttrFlags |= ATTR_RGN_DIRTY ;
    Rgn_Attr->iComplexity = SIMPLEREGION;
    return TRUE;
}

/*
 * @implemented
 */
int
WINAPI
SetMetaRgn( HDC hDC )
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

