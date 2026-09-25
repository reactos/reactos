// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Abstract:
//      Contains implementation for walking the visual tree for
//      precompute walk. The bounding boxes are updated here and
//      the dirty regions are also collected
//
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//  Node Operation   | NeedsToBe     | NeedsBBoxUpdate | HasNodeThat    | Visit
//                   | AddedToDirty  | (parent chain)  | NeedsToBeAdded | child
//                   | Region        |                 | ToDirtyRegion  |
//                   |               |                 | (parent chain) |
//=============================================================================
//  Set transform    |   Y           |   Y             |   Y(N)
//  -----------------+---------------+-----------------+-----------------------
//  Set opacity      |   Y           |   N             |   Y(N)
//  -----------------+---------------+-----------------+-----------------------
//  Set clip         |   Y           |   Y             |   Y(N)
//  -----------------+---------------+-----------------+-----------------------
//  AttachRenderData |   Y           |   Y             |   Y(N)
//  -----------------+---------------+-----------------+-----------------------
//  FreeRenderData   |   Y           |   Y             |   Y(N)
//  -----------------+---------------+-----------------+-----------------------
//  InsertChild      |   N           |   Y             |   Y
//                   |   Y(child)    |   N             |   Y(N)
//  -----------------+---------------+-----------------+-----------------------
//  InsertChildAt    |   N           |   Y             |   Y
//                   |   Y(child)    |   N             |   Y(N)
//  -----------------+---------------+-----------------+-----------------------
//  ZOrderChild      |   N           |   N             |   Y
//                   |   Y(child)    |   N             |   Y(N)
//  -----------------+---------------+-----------------+-----------------------
//  ReplaceChild     |   Y           |   Y             |   Y(N)
//  -----------------+---------------+-----------------+-----------------------
//  RemoveChild      |   Y           |   Y             |   Y(N)

//----------------------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------------------

class CMilGeometryDuce;
class CMilTransformDuce;
class CMilVisual;

typedef struct ScrollAreaStruct
{
    bool fDoScroll;
    int scrollX;
    int scrollY;
    CRectF<CoordinateSpace::PageInPixels> clipRect;    
    CMILSurfaceRect source;
    CMILSurfaceRect destination;
} ScrollArea;


//----------------------------------------------------------------------------------
// Meters
//----------------------------------------------------------------------------------

MtExtern(CPreComputeContext);

//----------------------------------------------------------------------------------
//  Class: 
//      CPreComputeContext
//
//  Synopsis:
//      Does the precompute walk starting at the specefied root and calculates
//      bounding-boxes for each node and also collects dirty region
//----------------------------------------------------------------------------------

class CPreComputeContext : IGraphIteratorSink
{
#if DBG==1
    // Needed for debug dirty region disabled nesting check. See PreComputeContext::{PreSubgraph, PostSubgraph}
    friend class CMilVisual;
#endif

    // Ctor inaccessible to prevent inheritance.
    // Use the Create method to create a CPreComputeContext.
    CPreComputeContext();

public:
    ~CPreComputeContext();
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CPreComputeContext));

    static HRESULT Create(
        __in_ecount(1) CComposition *pDevice,
        __deref_out_ecount(1) CPreComputeContext** ppPreComputeContext
        );

    // Returns the internal dirty region array. Do not free this memory.
    __out_ecount(GetDirtyRegionCount()) const MilRectF* GetUninflatedDirtyRegions()
    { 
        return m_rootDirtyRegion.GetUninflatedDirtyRegions(); 
    }

    // Returns the internal dirty region array. Do not free this memory.
    UINT GetDirtyRegionCount() const
    { 
        return m_rootDirtyRegion.GetRegionCount(); 
    }

    // PreCompute the specified node. (Make sure to reset the PreComputeContext)
    HRESULT PreCompute(
        __in_ecount(1) CMilVisual *pRoot,
        __in_ecount_opt(1) const CMilRectF *prcSurfaceBounds,
        UINT uNumInvalidTargetRegions,
        __in_ecount_opt(uNumInvalidTargetRegions) MilRectF const *rgInvalidTargetRegions,
        float allowedDirtyRegionOverhead,
        MilBitmapInterpolationMode::Enum defaultInterpolationMode,
        __in_opt ScrollArea *pScrollArea,        
        BOOL fDontComputeDirtyRegions = FALSE
        );

    // IGraphIteratorSink interface ------------------------------------------------
    HRESULT PreSubgraph(
        __out_ecount(1) BOOL *pfVisitChildren
        );

    HRESULT PostSubgraph();

private:
    bool IsAcceleratedScrollEnabled() const { return (m_pScrollAreaParameters != NULL); }
    bool ScrollHasCompleted() const { return m_fScrollHasCompleted; }
    bool ScrollHasBegun() const { return m_fScrollHasBegun; }

    HRESULT AddToDirtyRegion(
        __in_ecount(1) CDirtyRegion2 *pDirtyRegion,
        __in_ecount(1) const CMilRectF *pBoundsLocalSpace
        );

    void TransformBoundsToWorldAndClip(
        __in_ecount(1) const CMilRectF *pBoundsLocalSpace,
        __inout_ecount(1) CRectF<CoordinateSpace::PageInPixels> *pBboxWorld
        );

    static HRESULT ConvertInnerToOuterBounds(
        __in_ecount(1) CMilVisual *pNode
        );

    HRESULT CollectAlphaMaskDirtyRegions(
        __in_ecount(1) CDirtyRegion2 *pDirtyRegion,
        __in_ecount(1) CMilVisual *pNode,
        __in_ecount(1) const CMilRectF *pNodeInnerBounds
        );

    HRESULT PushBoundsAffectingProperties(
        __in CMilVisual *pNode
        );

    void PopBoundsAffectingProperties(
        __in CMilVisual const *pNode
        );

    HRESULT PushCache(
        __in CMilVisual *pNode
        );

    HRESULT PopCache(
        __in CMilVisual *pNode
        );

    HRESULT ScrollableAreaHandling(
        __in CMilVisual *pNode,
        __in CDirtyRegion2 *pDirtyRegion,
        __out bool *pScrollOccurred
        );

    bool ScrollHandlingRequired(
        __in CMilVisual const *pNode
        );

    bool EffectsInParentChain() const 
    {
        return m_effectCount != 0;
    }

    void PushEffect()
    {
        ++m_effectCount;
    }

    void PopEffect()
    {
        --m_effectCount;
        Assert(m_effectCount >= 0);
    }
        
private:
    CDirtyRegion2 m_rootDirtyRegion;
    float m_allowedDirtyRegionOverhead;
    CWatermarkStack<CDirtyRegion2*, 8 /* MinCapacity */, 2 /* GrowFactor */, 8 /* TrimCount */> m_dirtyRegionStack;

    // Future Consideration:   Find accurate coordinate space name
    //  for CPreComputeContext's "world" space.  It is currently annotated as
    //  PageInPixels because one use, when given a device transform in
    //  PreCompute, does work with PageInPixels and aliased dirty rectangles
    //  are used which only works well when there is a notion of pixels. 
    //  CMilVisualBrushDuce uses CPreComputeContext without passing a device
    //  transform.

    CMatrixStack<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> m_transformStack;
    CClipStack<CoordinateSpace::PageInPixels> m_clipStack;
    CGraphIterator *m_pGraphIterator;
    CContentBounder *m_pContentBounder;

    CMilRectF m_surfaceBounds;

    ScrollArea *m_pScrollAreaParameters;

    // True for the rest of the scenegraph traversal after _exiting_ a node for which
    // ScrollHandlingRequired has returned true, and a scroll actually occurred.
    bool  m_fScrollHasCompleted                         : 1;

    // Set in PreCompute as soon as a node is hit for which scrolling is available and will
    // be executed. Is used later to check that other scrolls don't occur also. We allow
    // multiple scroll areas to exist in the tree simultaneously, but only one accelerated
    // scroll per frame
    bool m_fScrollHasBegun                              : 1;
    
    // The clip area for the scroll that occurred. Only valid when m_fScrollHasOccurred is true
    CRectF<CoordinateSpace::PageInPixels>               m_scrolledClipArea;

    // Effect stack count. Keeps track of how many nodes in the parent chain returned true for
    // CMilVisual::HasEffects
    int m_effectCount;

};

