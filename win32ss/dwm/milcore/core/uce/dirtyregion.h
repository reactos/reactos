// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Dirty region management class.
//

//-----------------------------------------------------------------------------
// Meters

MtExtern(CDirtyRegion2);

//-----------------------------------------------------------------------------
// Forward declarations

class CPerformanceCounter;
class CMilVisual;

//-----------------------------------------------------------------------------
// DirtyRegion2 Implementation

void InflateRectF_InPlace(__inout_ecount(1) CMilRectF *pRect);

void InflateRectF_InPlace(__inout_ecount(1) CMilRectF* pRect, float margin);

class CDirtyRegion2
{
public:
    //
    // ctor
    CDirtyRegion2();


    //
    // dtor
    ~CDirtyRegion2();

    // Update Per Frame Statistics

    static void UpdatePerFrameStatistics();

    //
    // Initialize must be called before adding dirty rects. Initialize can also be called to
    // reset the dirty region.
    //
    void Initialize(
            __in_ecount_opt(1) const CMilRectF *prcNewSurfaceBounds,
            float allowedDirtyRegionOverhead
            );

    // 
    // Adds a new dirty rectangle for the specified node to the dirty region.
    //
    //   Return: E_OUTOFMEMORY - if there isn't enough memory to perform the operation.
    //           S_OK otherwise.
    //
    HRESULT Add(__in_ecount(1) const MilRectF *pNewRegion);            

    //
    // Disables the dirty region collection. It basically turns Add into a no-op.
    //
    //    Remark: This operation is counted to allow for nested Disable/Enable
    //            calls.
    //            Must be matched with Enabled.
    //
    void Disable();
    
    //
    // Enables the dirty region collection. See also Disable.
    //
    //    Remark: This operation is ref-counted and the number of calls must
    //            be matched with Disable.
    //
    void Enable();

    //
    // Checks if the dirty region collection is currently disabled.
    //
    bool IsDisabled() const { return (m_ignoreCount != 0); }

#if DBG==1
    //
    // Returns the nesting level of Disabled calls.
    //
    UINT GetEnabledNestingCount() const { return m_ignoreCount; }
#endif

    //
    // Checks if the dirty region is empty.
    //
    bool IsEmpty() const;

    //
    // Returns an array of dirty rectangles describing the dirty region.
    //
    __out_ecount(m_regionCount) const MilRectF* GetUninflatedDirtyRegions();

    //
    // Returns the dirty region count.
    // NOTE: The region count is NOT VALID until GetDirtyRegion is called.
    //
    UINT GetRegionCount() const { return m_regionCount; }

    //
    // Allow external objects to determine what the maximum number
    // of dirty regions that will be returned is.
    //
    static const UINT MaxDirtyRegionCount = 8;
    
private:
    // Disable allocations on heap.
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CDirtyRegion2));
    
    struct CUnionResult
    {
        CUnionResult(float overhead, float area, MilRectF unionRect) 
        { 
            m_overhead = overhead; m_area = area; m_union = unionRect;
        }

        float m_overhead;
        float m_area;
        MilRectF m_union;
    };

    static CUnionResult Union(
        __in_ecount(1) const MilRectF* pR0, 
        __in_ecount(1) const MilRectF* pR1
        );

    void SetOverhead(UINT i, UINT j, float value);
    float GetOverhead(UINT i, UINT j) const;

    void UpdateOverhead(UINT regionIndex);

private:
    CMilRectF m_dirtyRegions[MaxDirtyRegionCount];
    CMilRectF m_resolvedRegions[MaxDirtyRegionCount];
    LIST_ENTRY m_dirtyRegionLists[MaxDirtyRegionCount];
    float m_overhead[MaxDirtyRegionCount+1][MaxDirtyRegionCount];
    CMilRectF m_rcSurfaceBoundsF;
    float c_allowedDirtyRegionOverhead;
    float m_accumulatedOverhead;
    UINT m_ignoreCount;
    UINT m_regionCount;
    bool m_fOptimized;

    //
    // Fallback flag for the extreme case of invalid dirty regions being added
    // If this flag is true, this object contains only one dirty rect and it is
    // set to the size of m_rcSurfaceBoundsF. 
    // Until the next time Initialize() is called, Add() is a no-op.
    //
    bool m_fMaxSurfaceFallback;

private:
    static CPerformanceCounter* g_pAddedRectStatistics;    
};


