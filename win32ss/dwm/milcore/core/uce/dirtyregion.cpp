// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Dirty region management class.
//

#include "precomp.hpp"


// Meters ----------------------------------------------------------------------
MtDefine(CDirtyRegion2, Mem, "DirtyRegion");

// -----------------------------------------------------------------------------
// Helpers
//
// GSchneid: Some helper functions that should really go away when we get one
// _the_ one set of primitive types.

float RectArea(__in_ecount(1) const MilRectF* pR)
{
    return ((pR->right - pR->left) * (pR->bottom - pR->top));
};

//+-----------------------------------------------------------------------------
// Dirty region statistics
//------------------------------------------------------------------------------

CPerformanceCounter g_addedRectStatistics(1000);
CPerformanceCounter* CDirtyRegion2::g_pAddedRectStatistics = &g_addedRectStatistics;

//+-----------------------------------------------------------------------------
// CPerformanceCounter::UpdatePerFrameStatistics
//
// Description:
//     Tracks per frame statistics. 
//------------------------------------------------------------------------------
/*static*/

void
CDirtyRegion2::UpdatePerFrameStatistics()
{
    if (g_pMediaControl)
    {
        CMediaControlFile* pFile = g_pMediaControl->GetDataPtr();
        pFile->DirtyRectAddRate = g_pAddedRectStatistics->GetCurrentRate();
    }
}

//+-----------------------------------------------------------------------------
//
//  Name:
//
//  InflateRectFInPlace
//
//  Description:
//
//  Inflates a rectangle IN PLACE. Used to mark things as dirty on the boundary
//  so that anti aliasing works correctly. Also used to expand for glass blur radius
//
//  How much do we need to inflate:
//
//  '>' is the right edge of the left shape ':' indicates the antialiasing 
//  edge right of the left shape.
//  '<' is the left edge of the right shape. Its anti-aliasing edge is ':' 
//  left of it.
//  | indicate the pixel boundaries.
//  We need to inflate the shapes enough for the intersection tests that iff the
//  shapes rasterized representation influences the color of a pixel the
//  intersection test needs to return TRUE.
//
//  The following example shows that two shapes can influence the same pixel but
//  might not geometrically overlap. 
//
//  >  |:      :|  <     |   
//
//  Assuming that the distance between shape edge and anti-aliasing edge is less
//  then a pixel width then extending by the width of a pixel ensures that the
//  intersection test identifies overlapping when the two shapes influence the same
//  pixel.
//
//  Note that this only works if rectangles are axis aligned. Otherwise the offset
//  needs to be sqrt(2).
//
//  pRect - non empty rectangle 
//  margin - size to inflate, >= 0
//
//------------------------------------------------------------------------------

void InflateRectF_InPlace(__inout_ecount(1) CMilRectF* pRect, float margin)
{
    pRect->Inflate(margin, margin);

    pRect->left = CFloatFPU::FloorF(pRect->left);
    pRect->top = CFloatFPU::FloorF(pRect->top);
    pRect->right = CFloatFPU::CeilingF(pRect->right);
    pRect->bottom = CFloatFPU::CeilingF(pRect->bottom);
}

void InflateRectF_InPlace(__inout_ecount(1) CMilRectF* pRect)
{
    InflateRectF_InPlace(pRect, 1.0f);
}

//+-----------------------------------------------------------------------------
// CDirtyRegion2::ctor
//------------------------------------------------------------------------------

CDirtyRegion2::CDirtyRegion2()
{
    // Initialize linked lists.
    for (UINT i = 0; i < MaxDirtyRegionCount; i++)
    {
        InitializeListHead(&(m_dirtyRegionLists[i]));
    }
    m_fMaxSurfaceFallback = false;
}

//+-----------------------------------------------------------------------------
// CDirtyRegion2::dtor
//------------------------------------------------------------------------------

CDirtyRegion2::~CDirtyRegion2()
{
}

//+-----------------------------------------------------------------------------
// CDirtyRegion2::IsEmpty
//------------------------------------------------------------------------------

bool CDirtyRegion2::IsEmpty() const
{
    bool fIsEmpty = true;

    for (UINT i = 0; i < MaxDirtyRegionCount; i++)
    {
        if (!m_dirtyRegions[i].IsEmpty())
        {
            fIsEmpty = false;
            break;
        }
    }

    return fIsEmpty;
}

//+-----------------------------------------------------------------------------
// CDirtyRegion2::Initialize
//------------------------------------------------------------------------------

void 
CDirtyRegion2::Initialize(
    __in_ecount_opt(1) const CMilRectF* prcNewSurfaceBounds,
    float allowedDirtyRegionOverhead
    )
{
    m_ignoreCount = 0;
    c_allowedDirtyRegionOverhead = allowedDirtyRegionOverhead;
    memset(m_dirtyRegions, 0, sizeof(m_dirtyRegions));
    memset(m_overhead, 0, sizeof(m_overhead));
    m_accumulatedOverhead = 0;
    m_fOptimized = false;
    m_fMaxSurfaceFallback = false;

    //
    // surface bounds kept in floating point to allow for intersection
    // with dirty rects in float space
    //
    m_rcSurfaceBoundsF =
        (prcNewSurfaceBounds) ?
        *prcNewSurfaceBounds :
        m_rcSurfaceBoundsF.sc_rcEmpty;
}

//+-----------------------------------------------------------------------------
// CDirtyRegion2::CUnionResult
//------------------------------------------------------------------------------

CDirtyRegion2::CUnionResult 
CDirtyRegion2::Union(
    __in_ecount(1) const MilRectF* pR0, 
    __in_ecount(1) const MilRectF* pR1
    )
{
    CMilRectF unioned(*pR0);
    unioned.Union(*pR1);

    CMilRectF intersected(*pR0);
    intersected.Intersect(*pR1);

    float areaOfUnion = RectArea(&unioned);

    float overhead = areaOfUnion - (RectArea(pR0) + RectArea(pR1) - RectArea(&intersected));

    // Use 0 as overhead if computed overhead is negative or overhead
    // computation returns a nan.  (If more than one of the previous
    // area computations overflowed then overhead could be not a
    // number.)
    if (!(overhead > 0))
    {
        overhead = 0;
    }

    return CUnionResult(overhead, areaOfUnion, unioned);
}

//+-----------------------------------------------------------------------------
// CDirtyRegion2::SetOverhead
//------------------------------------------------------------------------------

void 
CDirtyRegion2::SetOverhead(UINT i, UINT j, float value)
{
    Assert(i != j);
    if (i > j) 
    { 
        m_overhead[i][j] = value;
    }

    if (i < j) 
    {
        m_overhead[j][i] = value;
    }
}

//+-----------------------------------------------------------------------------
// CDirtyRegion2::GetOverhead
//------------------------------------------------------------------------------

float 
CDirtyRegion2::GetOverhead(UINT i, UINT j) const
{
    Assert(i != j);
    if (i > j) 
    {
        return m_overhead[i][j];
    }

    if (i < j) 
    {
        return m_overhead[j][i];
    }

    Assert(false);
    return FLT_MAX;
}

//+-----------------------------------------------------------------------------
// CDirtyRegion2::Add
//------------------------------------------------------------------------------

HRESULT
CDirtyRegion2::Add(
    __in_ecount(1) const MilRectF *pNewRegion
    )
{
    HRESULT hr = S_OK;
    
    CMilRectF clippedNewRegion(*pNewRegion);
    
    if (IsDisabled()) 
    {
        goto Cleanup; 
    }

    AssertMsg(!m_fOptimized, "You need to reset the dirty region before you can use it again.");

    //
    // We've already fallen back to setting the whole surface as a dirty region
    // because of invalid dirty rects, so no need to add any new ones
    //
    if (m_fMaxSurfaceFallback)
    {
        goto Cleanup;
    }
    
    //
    // Check if rectangle is well formed before we try to intersect it, 
    // because Intersect will fail for badly formed rects
    //
    if (!clippedNewRegion.IsWellOrdered())
    {
        //
        // If we're here it means that we've been passed an invalid rectangle as a dirty
        // region, containing NAN or a non well ordered rectangle.
        // In this case, make the dirty region the full surface size and warn in the debugger
        // since this could cause a serious perf regression.
        //
        TraceTag((tagMILWarning, "Invalid dirty region received, setting dirty region to surface size."));

        // 
        // Remove all dirty regions from this object, since
        // they're no longer relevant.
        //
        Initialize(&m_rcSurfaceBoundsF, c_allowedDirtyRegionOverhead);

        m_fMaxSurfaceFallback = true;
        m_regionCount = 1;
    }        
    else
    {
        clippedNewRegion.Intersect(m_rcSurfaceBoundsF);

        if (clippedNewRegion.IsEmpty())
        {
           goto Cleanup;
        }  

        // Always keep bounding boxes in device space integer.
        clippedNewRegion.left = CFloatFPU::FloorF(clippedNewRegion.left);
        clippedNewRegion.top = CFloatFPU::FloorF(clippedNewRegion.top);
        clippedNewRegion.right = CFloatFPU::CeilingF(clippedNewRegion.right);
        clippedNewRegion.bottom = CFloatFPU::CeilingF(clippedNewRegion.bottom);

        //
        // Keep dirty rectangle addition statistics.
        if (g_pMediaControl)
        {
            g_pAddedRectStatistics->Inc();
        }       

        // Compute the overhead for the new region combined with all the other existing regions.

        for (UINT n = 0; n < MaxDirtyRegionCount; n++)
        {
            CUnionResult ur = CDirtyRegion2::Union(&(m_dirtyRegions[n]), &clippedNewRegion);
            SetOverhead(MaxDirtyRegionCount, n, ur.m_overhead);
        }

        // Find the pair of dirty regions that if merged create the minimal overhead. A overhead
        // of 0 is perfect in the sense that it can not get better. In that case we break early
        // out of the loop.

        float minimalOverhead = FLT_MAX;
        UINT bestMatch_N = 0;
        UINT bestMatch_K = 0;
        bool fMatchFound = false;

        for (UINT n = MaxDirtyRegionCount; n > 0; n--)
        {
            for (UINT k = 0; k < n; k++)
            {
                float overhead_N_K = GetOverhead(n, k);
                if (minimalOverhead >= overhead_N_K) 
                {
                    minimalOverhead = overhead_N_K;
                    bestMatch_N = n; bestMatch_K = k;
                    fMatchFound = true;

                    if (overhead_N_K < c_allowedDirtyRegionOverhead)
                    {
                        // If the overhead is very small, we bail out early since this
                        // saves us some valuable cycles. Note that "small" means really
                        // nothing here. In fact we don't always know if that number is
                        // actually small. However, it the algorithm stays still correct
                        // in the sense that we render everything that is necessary. It
                        // might just be not optimal.
                        goto LoopExit;
                    }
                }
            }
        }

        if (!fMatchFound)
        {
            //
            // Should never be here. Being here implies that clippedNewRegion is not well formed
            // which is handled in parameter checking at the top of this function
            //
            MilUnexpectedError(E_FAIL, TEXT("Invalid dirty region"));
        }

    LoopExit:

        // There are two major cases now:
        // Case A: (bestMatch_N == MaxDirtyRegionCount)
        //         This means the new dirty region can be combined with an existing one
        //         without significant overhead.

        if (bestMatch_N == MaxDirtyRegionCount)
        {                    
            CUnionResult ur = CDirtyRegion2::Union(&clippedNewRegion, &(m_dirtyRegions[bestMatch_K]));
            MilRectF unioned = ur.m_union;
            if (m_dirtyRegions[bestMatch_K].DoesContain(unioned))
            {
                // Check if newDirtyRegion is enclosed by dirty region bestMatch_K. In this case we are done.
                goto Cleanup;
            }
            else
            {
                m_accumulatedOverhead += ur.m_overhead;
                m_dirtyRegions[bestMatch_K] = unioned;
                UpdateOverhead(bestMatch_K);
            }
        }
        else
            // Case B: (bestMatch_N != MaxDirtyRegionCount)
            //         This means that it is more efficient to merge first region N with 
            //         region K and then store the new region without combinding it with
            //         another one.
            // 
            //         Merged region is stored in slot N. New region is stored in slot K.
        {
            CUnionResult ur = CDirtyRegion2::Union(&(m_dirtyRegions[bestMatch_N]), &(m_dirtyRegions[bestMatch_K]));
            m_accumulatedOverhead += ur.m_overhead;
            Assert((0 < bestMatch_N) && (bestMatch_N <= MaxDirtyRegionCount)); 
            Assert(bestMatch_K < MaxDirtyRegionCount);
            m_dirtyRegions[bestMatch_N] = ur.m_union;
            m_dirtyRegions[bestMatch_K] = clippedNewRegion;
            UpdateOverhead(bestMatch_N);
            UpdateOverhead(bestMatch_K);
        }
    }
    
Cleanup:

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
// CDirtyRegion2::UpdateOverhead
//------------------------------------------------------------------------------

void 
CDirtyRegion2::UpdateOverhead(UINT regionIndex)
{
    const MilRectF* pRegionAtIndex = &(m_dirtyRegions[regionIndex]);
    for (UINT i = 0; i < MaxDirtyRegionCount; i++)
    {
        if (regionIndex != i)
        {
            CUnionResult ur = CDirtyRegion2::Union(&(m_dirtyRegions[i]), pRegionAtIndex); 
            SetOverhead(i, regionIndex, ur.m_overhead);
        }
    }
}

//+--------------------------------------------------------------------------------------
// GetUninflatedDirtyRegions
//
//     Returns a pointer to the internal dirty region rectangle array. Do not free that
//     memory.  Note that the regions have NOT been inflated for anti-aliasing, it is
//     up to the caller to handle that.
//---------------------------------------------------------------------------------------

__out_ecount(m_regionCount) const MilRectF* 
CDirtyRegion2::GetUninflatedDirtyRegions()
{
    C_ASSERT(sizeof(m_dirtyRegions) == sizeof(m_resolvedRegions));

    if (m_fMaxSurfaceFallback)
    {
        return &m_rcSurfaceBoundsF;
    }

    if (!m_fOptimized)
    {
        memset(m_resolvedRegions, 0, sizeof(m_resolvedRegions));

        // Consolidate the dirtyRegions array to minimize looping below
        UINT addedDirtyRegionCount = 0;
        for (UINT i = 0; i < MaxDirtyRegionCount; i++)
        {
            if (!m_dirtyRegions[i].IsEmpty())
            {
                if (i != addedDirtyRegionCount)
                {
                    m_dirtyRegions[addedDirtyRegionCount] = m_dirtyRegions[i];
                    UpdateOverhead(addedDirtyRegionCount);
                }
                addedDirtyRegionCount++;
            }
        }

        // Merge all dirty rects that we can:
        // Because the algorithm for accumulating dirty regions can only combine them once when one is
        // added, a situation can arise where we have two dirty regions in the array that overlap
        // significantly or are contained one inside another.  A full Render walk will occur for both
        // regions and will redraw all their content twice.

        bool couldMerge = true;
        // Loop until no more rects in m_dirtyRegions can be merged.
        // Each time we merge two rects in this final loop it creates the opportunity for the resulting
        // rect to also be merged on a subsequent loop execution.
        // Loop will execute a max of MaxDirtyRegionsCount - 1 times.
        while(couldMerge)
        {
            couldMerge = false;
            // The pair of for loops look at each pair of dirty rects, and see merge them if the overhead
            // is low enough and neither rect is empty.  The array is not consolidated as rects are merged
            // since it would require an UpdateOverhead call on the slot moved - its cheaper to consolidate
            // only once more at the end.
            for (UINT n = 0; n < addedDirtyRegionCount; n++)
            {
                for (UINT k = n + 1; k < addedDirtyRegionCount; k++)
                {
                    if (   !m_dirtyRegions[n].IsEmpty()
                        && !m_dirtyRegions[k].IsEmpty() 
                        && GetOverhead(n, k) < c_allowedDirtyRegionOverhead)
                    {
                        //Merge N and K
                        CUnionResult ur = CDirtyRegion2::Union(&(m_dirtyRegions[n]), &(m_dirtyRegions[k]));
                        
                        //Place merged region in slot N
                        m_dirtyRegions[n] = ur.m_union;

                        //Clear slot k, don't need to update its overhead since it's now empty
                        m_dirtyRegions[k].SetEmpty();
                        UpdateOverhead(n);

                        couldMerge = true;
                    }
                }
            }
        }
        
        // Consolidate and copy into resolvedRegions
        UINT finalRegionCount = 0;
        for (UINT i = 0; i < addedDirtyRegionCount; i++)
        {
            if (!m_dirtyRegions[i].IsEmpty())
            {
                m_resolvedRegions[finalRegionCount] = m_dirtyRegions[i];
                finalRegionCount++;
            }
        }

        m_regionCount = finalRegionCount;

        m_fOptimized = true;
    }
    
    return m_resolvedRegions;
}

//+--------------------------------------------------------------------------------------
// Disable
// Disables the dirty region collection. It basically turns Add into a no-op.
//---------------------------------------------------------------------------------------

void 
CDirtyRegion2::Disable() 
{ 
    ++m_ignoreCount; 
}

//+--------------------------------------------------------------------------------------
// Enable
// Enables the dirty region collection. See also Disable.
//---------------------------------------------------------------------------------------

void 
CDirtyRegion2::Enable() 
{ 
    Assert(m_ignoreCount > 0);
    --m_ignoreCount; 
}






