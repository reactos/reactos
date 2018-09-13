//+------------------------------------------------------------------------
//
//  File:       brushbmp.cxx
//
//  Contents:   Bitmap brush utilities.
//
//  History:    6-Nov-95   RodC     Created
//
//-------------------------------------------------------------------------


#include "headers.hxx"


struct BBCR
{
    int         resId;
    HBRUSH      hbr;
};

MtDefine(CAryBBCR, THREADSTATE, "CAryBBCR")
MtDefine(CAryBBCR_pv, THREADSTATE, "CAryBBCR::_pv")

// We have to do this because of the forward declare in thread state.
DECLARE_CDataAry(CAryBBCR, BBCR, Mt(CAryBBCR), Mt(CAryBBCR_pv))

//+----------------------------------------------------------------------------
//
//  Function:   InitBmpBrushCache
//
//  Synopsis:   Allocate bitmap brushes cache array
//
//  Arguments:  pts - THREADSTATE of current thread
//
//  Returns:    S_OK, E_OUTOFMEMORY
//
//-----------------------------------------------------------------------------
HRESULT
InitBmpBrushCache(
    THREADSTATE *   pts)
{
    Assert(pts);

    pts->paryBBCR = new CAryBBCR;
    if (!pts->paryBBCR)
        RRETURN(E_OUTOFMEMORY);

    MemSetName((pts->paryBBCR, "BmpBrushCache ary"));

    RRETURN(S_OK);
}


//+---------------------------------------------------------------------------
//
//  Function:   DeinitBmpBrushCache
//
//  Synopsis:   Releases any bitmap brushes still stored in the cache.
//
//----------------------------------------------------------------------------
void
DeinitBmpBrushCache(
    THREADSTATE *   pts)
{
    Assert(pts);

    if (!pts->paryBBCR)
        return;

    int     c = (*pts->paryBBCR).Size();

    // Loop over all the brushes in the cache and delete them.
    while (c--)
    {
        Assert((*pts->paryBBCR)[c].hbr);
        DeleteObject((*pts->paryBBCR)[c].hbr);
    }

    (*pts->paryBBCR).DeleteAll();

    delete pts->paryBBCR;
}


//+---------------------------------------------------------------------------
//
//  Function:   GetCachedBmpBrush
//
//  Synopsis:   Returns a brush handle for the bitmap matching the resource
//              id. A process-wide cache of brushes is maintained.  If this
//              function is called and a matching brush is found in the
//              cache, it is returned.  If no match is found, a new one is
//              added.
//
//              No release is needed.  There are few enough bitmap brushes
//              that they will all be around until the detach.
//
//----------------------------------------------------------------------------

HBRUSH
GetCachedBmpBrush(int resId)
{
    THREADSTATE *   pts;
    HBITMAP         hbmp = NULL;
    HBRUSH          hbr = NULL;
    BBCR *          pbbcr;
    int             c;

    pts = GetThreadState();

    //
    // Look for previously cached bitmap brush.
    //
    for (c = (*pts->paryBBCR).Size(), pbbcr = (*pts->paryBBCR); c > 0; c--, pbbcr++)
    {
        Assert(pbbcr->hbr && pbbcr->resId);

        // Return brush if found.
        if (pbbcr->resId == resId)
        {
            hbr = pbbcr->hbr;
            goto Cleanup;
        }
    }

    //
    // If we didn't find the brush, we need to cook one up.
    //

    // Make sure we will have room for the brush.
    if ((*pts->paryBBCR).EnsureSize((*pts->paryBBCR).Size() + 1))
        goto Cleanup;

    // Load the bitmap resouce.
    hbmp = LoadBitmap(g_hInstCore, MAKEINTRESOURCE(resId)); // NOTE (lmollico): bitmaps are in mshtml.dll
    if (!hbmp)
        goto Cleanup;

    // Turn the bitmap into a brush.
    hbr = CreatePatternBrush(hbmp);
    if (!hbr)
        goto Cleanup;

    // Store the brush.
    (*pts->paryBBCR)[(*pts->paryBBCR).Size()].resId = resId;
    (*pts->paryBBCR)[(*pts->paryBBCR).Size()].hbr = hbr;
    (*pts->paryBBCR).SetSize((*pts->paryBBCR).Size() + 1);

Cleanup:
    if (hbmp)
        DeleteObject(hbmp);

    return hbr;
}
