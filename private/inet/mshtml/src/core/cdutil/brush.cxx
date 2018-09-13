//+------------------------------------------------------------------------
//
//  File:       brush.cxx
//
//  Contents:   Brush utilities.
//
//  History:    20-Oct-94   GaryBu  Created
//
//-------------------------------------------------------------------------

#include "headers.hxx"

PerfDbgTag(tagBrushCache,        "BrushCache", "Trace BrushCache")
PerfDbgTag(tagDisableBrushCache, "BrushCache", "Disable Brush Cache")

MtDefine(THREADSTATE_pbcr, THREADSTATE, "THREADSTATE::_pbcr")

#define CBCR_MAX            16

struct BCR
{
    COLORREF    cr;
    int         refs;
    HBRUSH      hbr;
};


//+----------------------------------------------------------------------------
//
//  Function:   InitBrushCache
//
//  Synopsis:   Allocate brush cache array
//
//  Arguments:  pts - THREADSTATE of current thread
//
//  Returns:    S_OK, E_OUTOFMEMORY
//
//-----------------------------------------------------------------------------
HRESULT
InitBrushCache(
    THREADSTATE *   pts)
{
    Assert(pts);

    pts->pbcr = new(Mt(THREADSTATE_pbcr)) BCR[CBCR_MAX];
    if (!pts->pbcr)
        RRETURN(E_OUTOFMEMORY);

    MemSetName((pts->pbcr, "Brush Cache Data"));

    memset(pts->pbcr, 0, sizeof(BCR) * CBCR_MAX);

    pts->ibcrNext = -1;

    RRETURN(S_OK);
}


//+---------------------------------------------------------------------------
//
//  Function:   DeinitBrushCache
//
//  Synopsis:   Releases any brushes still stored in the cache
//
//----------------------------------------------------------------------------
void
DeinitBrushCache(
    THREADSTATE *   pts)
{
    int     c;
    BCR *   pbcr;

    Assert(pts);

    if (!pts->pbcr)
        return;

    for (c = CBCR_MAX, pbcr = pts->pbcr; c > 0; c--, pbcr++)
    {
        Assert(pbcr->refs == 0 && "Unreleased brush in cache at Deinit");
        if (pbcr->hbr)
            DeleteObject(pbcr->hbr);
    }

    delete [] pts->pbcr;
    return;
}


//+---------------------------------------------------------------------------
//
//  Function:   GetCachedBrush
//
//  Synopsis:   Returns a brush handle matching the given OLE Color.
//              A process-wide cache of brushes is maintained.  If this
//              function is called and a matching brush is found in the
//              cache, it is returned.  If no match is found, one of
//              the cached brushes is kicked out and a new one is
//              added.
//
//              ReleaseCachedBrush should be called to release the
//              brush handle when it is no longer needed.
//
//  Arguments:  clr   OLE Color of the brush
//
//  Returns:    Brush handle.
//
//  Notes:      THE ARGUMENT IS A *COLORREF*!! (Don't pass in an OLE_COLOR)
//
//----------------------------------------------------------------------------

HBRUSH
GetCachedBrush(COLORREF cr)
{
    THREADSTATE *   pts;
    int             c;
    BCR *           pbcr;
    HBRUSH          hbr;

#if DBG==1 || defined(PERFTAGS)
    if (IsPerfDbgEnabled(tagDisableBrushCache))
        return(CreateSolidBrush(cr));
#endif

    Assert((cr & 0x80000000) == 0);

    pts = GetThreadState();

    // Look for previously cached brush.

    for (c = CBCR_MAX, pbcr = pts->pbcr; c > 0; c--, pbcr++)
    {
        Assert(pbcr->hbr != NULL || pbcr->cr == 0);

        //  The brush cache initially starts out filled with zeros,
        //    but cr == 0 is a valid color, so we need to check for
        //    both the color matching AND having a valid brush.

        if (pbcr->cr == cr && pbcr->hbr != NULL)
        {
            pbcr->refs++;

            PerfDbgLog3(tagBrushCache, NULL, "GCB: found %x @ %d (%d)\r",
                    cr, pbcr - pts->pbcr, pbcr->refs);

            return pbcr->hbr;
        }
    }

    // Otherwise, create a new brush and add it to the cache.

    hbr = CreateSolidBrush(cr);
    if (!hbr)
    {
        TraceTag((tagError,
                "GCB: could not create brush, falling back to stock brush"));

        // BUGBUG: Do we need to be careful sharing stock objects between threads?
        hbr = (HBRUSH)GetStockObject(BLACK_BRUSH);
    }

    // Handle case where the system returns the same
    // brush handle for different color values.

    for (c = CBCR_MAX, pbcr = pts->pbcr; c > 0; c--, pbcr++)
    {
        if (pbcr->hbr == hbr)
        {
            pbcr->refs++;

            PerfDbgLog3(tagBrushCache, NULL, "GCB: found dup of %x @ %d (%d)\r",
                    cr, pbcr - pts->pbcr, pbcr->refs);

            return hbr;
        }
    }

    pbcr = pts->pbcr + pts->ibcrNext;
    for (c = CBCR_MAX; c > 0; c--)
    {
        pbcr++;
        pts->ibcrNext++;
        if (pts->ibcrNext >= CBCR_MAX)
        {
            pts->ibcrNext = 0;
            pbcr = pts->pbcr;
        }

        Assert(0 <= pts->ibcrNext && pts->ibcrNext < CBCR_MAX);

        if (pbcr->refs == 0)
        {
            if (pbcr->hbr)
                DeleteObject(pbcr->hbr);

            pbcr->hbr = hbr;
            pbcr->cr = cr;
            pbcr->refs = 1;

            PerfDbgLog2(tagBrushCache, NULL, "GCB: new %x @ %d (1)\r", cr, pts->ibcrNext);

            return hbr;
        }

    }

    TraceTag((tagWarning, "GCB: cache full"));

    return hbr;
}


//+---------------------------------------------------------------------------
//
//  Function:   ReleaseCachedBrush
//
//  Synopsis:   Releases a brush handle returned by GetCachedBrush.
//
//  Arguments:  hbr  Brush handle to release
//
//----------------------------------------------------------------------------

void
ReleaseCachedBrush(HBRUSH hbr)
{
    THREADSTATE *   pts;
    int             c;
    BCR *           pbcr;

    if (!hbr)
        return;

    pts = GetThreadState();

    for (c = CBCR_MAX, pbcr = pts->pbcr; c > 0; c--, pbcr++)
    {
        if (pbcr->hbr == hbr)
        {
            Assert(pbcr->refs > 0);
            pbcr->refs--;

            PerfDbgLog3(tagBrushCache, NULL, "GCB: released %x @ %d (%d)\r",
                pbcr->cr, pbcr - pts->pbcr, pbcr->refs);

            return;
        }
    }

    TraceTag((tagWarning, "RCB: Brush not found in cache, deleting anyway."));

    DeleteObject((HGDIOBJ)hbr);
}

//+---------------------------------------------------------------------------
//
//  Function:   SelectCachedBrush
//
//  Synopsis:   Helper function to optimally select a cached brush into a DC
//
//  Arguments:  hdc         Device context
//              crNew       color of the brush
//              phbrNew     new brush
//              phbrOld     original brush from the hdc (init to NULL)
//              pcrNow      current cached brush color in the hdc (init to COLORREF_NONE)
//
//----------------------------------------------------------------------------

void SelectCachedBrush(HDC hdc, COLORREF crNew, HBRUSH * phbrNew, HBRUSH * phbrOld, COLORREF * pcrNow)
{
    HBRUSH hbrNew, hbrOld;

    *phbrNew = *phbrOld;
    
    if (*pcrNow != crNew)
    {
        *phbrNew = hbrNew = GetCachedBrush(crNew);

        if (hbrNew)
        {
            hbrOld = (HBRUSH)SelectObject(hdc, hbrNew);

            if (hbrOld)
            {
                if (*phbrOld == NULL)
                    *phbrOld = hbrOld;
                else
                    ReleaseCachedBrush(hbrOld);

                *pcrNow = crNew;
            }
            else
            {
                ReleaseCachedBrush(hbrNew);
            }
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   PatBltBrush
//
//  Synopsis:   Helper function for calling PatBlt with a given COLORREF
//
//  Arguments:  hdc         Device context
//              x,y         Point where painting starts
//              xWid,yHei   Size of paint rectangle
//              dwRop       Raster op
//              cr          COLORREF of color to use
//
//----------------------------------------------------------------------------

void PatBltBrush(HDC hdc, LONG x, LONG y, LONG xWid, LONG yHei,
    DWORD dwRop, COLORREF cr)
{
    HBRUSH hbr = GetCachedBrush(cr);
    HBRUSH hbrOld = (HBRUSH)SelectObject(hdc, hbr);
    PatBlt(hdc, x, y, xWid, yHei, dwRop);
    SelectObject(hdc, hbrOld);
    ReleaseCachedBrush(hbr);
}

void PatBltBrush(HDC hdc, RECT * prc, DWORD dwRop, COLORREF cr)
{
    PatBltBrush(hdc, prc->left, prc->top, prc->right - prc->left,
        prc->bottom - prc->top, dwRop, cr);
}
