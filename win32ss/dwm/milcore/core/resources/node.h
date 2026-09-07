// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
// File name:
//      node.h
//
// Abstract: 
//      Visual resource.
//
//---------------------------------------------------------------------------

MtExtern(CMilVisual);

//---------------------------------------------------------------------------------
// Forward Declarations
//---------------------------------------------------------------------------------

class CDirtyRegion2;
class CMilScheduleRecord;
class CMilTransformDuce;
class CMilBrushDuce;
class CGuidelineCollection;
class CMilAlphaMaskWrapper;
class CMilVisualCacheSet;

//---------------------------------------------------------------------------------
// class CMilVisual
//---------------------------------------------------------------------------------

// We pack these enums into a DWORD here, so make sure that they're going to fit.
// We use UINTs here to avoid hitting the sign bit even though the enum proper can't be made
// unsigned.
// For flag enums make sure we have just the right number of bits.

#define MIL_EDGE_MODE_BITS 2
#define MIL_CLEARTYPEHINT_BITS 2
#define MIL_COMPOSITING_MODE_BITS 4
#define MIL_BITMAPSCALING_MODE_BITS 2
#define MIL_TEXTRENDERINGMODE_BITS 3
#define MIL_TEXTHINTINGMODE_BITS 2
#define MIL_RENDEROPTIONSFLAGS_BITS 6


C_ASSERT((1 << MIL_EDGE_MODE_BITS) >= MilEdgeMode::Last);
C_ASSERT((1 << MIL_CLEARTYPEHINT_BITS) >= MilClearTypeHint::Last);
C_ASSERT((1 << MIL_COMPOSITING_MODE_BITS) >= MilCompositingMode::Last);
C_ASSERT((1 << MIL_BITMAPSCALING_MODE_BITS) >= MilBitmapScalingMode::Last);
C_ASSERT((1 << MIL_TEXTRENDERINGMODE_BITS) >= MilTextRenderingMode::Last);
C_ASSERT((1 << MIL_TEXTHINTINGMODE_BITS) >= MilTextHintingMode::Last);
C_ASSERT((1 << (MIL_RENDEROPTIONSFLAGS_BITS-1)) + 1 == MilRenderOptionFlags::Last);

typedef struct ScrollableAreaPropertyBagStruct
{
    bool scrollOccurred;
    float oldOffsetX;
    float oldOffsetY;
    CRectF<CoordinateSpace::LocalRendering> clipRect;
} ScrollableAreaPropertyBag;

class CMilVisual : 
    public IGraphNode, 
    public CMilSlaveResource
{
    friend class CResourceFactory;
    friend class CDrawingContext;
    friend class CPreComputeContext;
    friend class CGraphWalker;
    friend class CContentBounder;
    friend class CWindowRenderTarget;
    friend class CMilVisual3D;
    
protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilVisual));

    CMilVisual(__in_ecount(1) CComposition* pComposition)
    {
        m_pComposition = pComposition;
        m_pScheduleRecord = NULL;

#if DBG==1
        m_dwDirtyRegionEnableCount = 0;
#endif
        m_alpha = 1.0;
    }

    virtual ~CMilVisual();

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_VISUAL;
    }

    override BOOL OnChanged(
        CMilSlaveResource *pSender,
        NotificationEventArgs::Flags e
        );

    __out_ecount(1) const CMilRectF &GetBounds() const
    {
        return m_Bounds;
    }
    
    HRESULT SetClip(
        __in_ecount_opt(1) CMilGeometryDuce *pClip
        );
    
    void SetOffset(
        float offsetX,
        float offsetY
        );

    // ------------------------------------------------------------------------
    //
    //   Command handlers
    //
    // ------------------------------------------------------------------------

    static HRESULT ProcessCreate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_VISUAL_CREATE* pCmd
        );

    HRESULT ProcessSetOffset(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_VISUAL_SETOFFSET* pCmd
        );

    HRESULT ProcessSetTransform(
        __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_VISUAL_SETTRANSFORM* pCmd
        );

    HRESULT ProcessSetEffect(
        __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_VISUAL_SETEFFECT* pCmd
        );

    HRESULT ProcessSetCacheMode(
        __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_VISUAL_SETCACHEMODE* pCmd
        );
    
    HRESULT ProcessSetClip(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_VISUAL_SETCLIP* pCmd
        );

    HRESULT ProcessSetScrollableAreaClip(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_VISUAL_SETSCROLLABLEAREACLIP *pCmd
        );

    HRESULT ProcessSetAlpha(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_VISUAL_SETALPHA* pCmd
        );

    HRESULT ProcessSetRenderOptions(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_VISUAL_SETRENDEROPTIONS* pCmd
        );    
    
    virtual HRESULT ProcessSetContent(
        __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_VISUAL_SETCONTENT* pCmd
        );

    HRESULT ProcessSetAlphaMask(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_VISUAL_SETALPHAMASK* pCmd
        );
    virtual HRESULT ProcessRemoveAllChildren(
        __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_VISUAL_REMOVEALLCHILDREN* pCmd
        );

    virtual HRESULT ProcessRemoveChild(
        __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_VISUAL_REMOVECHILD* pCmd
        );

    virtual HRESULT ProcessInsertChildAt(
        __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_VISUAL_INSERTCHILDAT* pCmd
        );

    HRESULT ProcessSetGuidelineCollection(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_VISUAL_SETGUIDELINECOLLECTION* pCmd,
        __in_bcount(cbPayload) LPCVOID pcvPayload,
        UINT cbPayload
        );
  
public:
    // 
    // IGraphNode interface implementation.

    override UINT GetChildrenCount() const { return static_cast<UINT>(m_rgpChildren.GetCount()); }
    override IGraphNode* GetChildAt(UINT index);
    override bool EnterNode();
    override void LeaveNode();
    override bool CanEnterNode() const;

    //+------------------------------------------------------------------------
    //
    //  Member:  
    //      CMilVisual::GetOuterBounds
    //
    //  Synopsis:  
    //      Returns the transformed, offset, & clipped bounds of the Visual's
    //      content union'd with the bounds of all of it's children.
    //
    //  Note:
    //      A Precompute walk must be performed on this Visual before
    //      this method is called.
    //
    //-------------------------------------------------------------------------
    const CMilRectF& GetOuterBounds() const { return m_Bounds; }

    virtual HRESULT GetContentBounds(
        __in_ecount(1) CContentBounder *pContentBounder, 
        __out_ecount(1) CMilRectF *prcBounds
        );

    virtual HRESULT RenderContent(
        __in_ecount(1) CDrawingContext* pCDrawingContext
        );

public:
    HRESULT AddAdditionalDirtyRects(
        __in_ecount(1) MilRectF const *pRegion
        );

    CMilVisual *GetParent();
    
    // 
    // Guidelines helper.
    HRESULT ScheduleRender();

    // Used for device lost cache invalidation.
    void MarkDirtyForPrecompute();

    HRESULT RegisterCache(
        __in_opt CMilBitmapCacheDuce *pCacheMode
        );

    void UnRegisterCache(
        __in_opt CMilBitmapCacheDuce *pCacheMode
        );

    __out_ecount_opt(1) CMilVisualCacheSet* GetCacheSet();
    
protected:
    HRESULT SetContent(
        __in_ecount_opt(1) CMilSlaveResource* pContent
        );

    void SetParent(
        __in_opt CMilVisual *pParentNode
        );

    HRESULT InsertChildAt(
        __in CMilVisual *pNewChild,
        UINT iPosition
        );

    HRESULT RemoveChild(
        __in CMilVisual *pChild
        );

    VOID RemoveAllChildren();

    static void PropagateFlags(
        __in CMilVisual *pNode,
        BOOL fNeedsBoundingBoxUpdate,
        BOOL fDirtyForRender,
        BOOL fAdditionalDirtyRegion = FALSE,
        BOOL fUpdatePreventingScroll = TRUE
        );

    virtual HRESULT CollectAdditionalDirtyRegion(
        __in_ecount(1) CDirtyRegion2 *pDirtyRegion,
        __in_ecount(1) const CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> *pWorldTransform,
        int scrollX,
        int scrollY,
        CRectF<CoordinateSpace::PageInPixels> clipRect,
        __in_ecount_opt(1) const CRectF<CoordinateSpace::PageInPixels> *pWorldClip = NULL);    

    bool CanBeScrolled() const
    {
        // Must have scrollable area properties set, and have no changes other than
        // offset for this pass to allow scrolling
        return (HasScrollableArea() && (m_fHasStateOtherThanOffsetChanged == 0));
    }
    
    CPtrArray<CMilVisual>& GetChildren() { return m_rgpChildren; }

    //
    // Get the alpha mask for this node
    //
    __out_ecount_opt(1) CMilBrushDuce* GetAlphaMask();

    void NotifyVisualTreeListeners();

    // A helper method for PreSubgraph/PostSubgraph
    // Returns TRUE if Effects need to be handled.
    BOOL HasEffects();
    
    static void TransformAndSnapScrollableRect(
        __in CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> *pTransform,
        __in_opt CMilRectF *pClip,
        __in CRectF<CoordinateSpace::LocalRendering> *pRectIn,
        __out CRectF<CoordinateSpace::PageInPixels> *pRectOut
        );

    static HRESULT TransformAndSnapOffset(
        __in CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> *pTransform,
        __inout MilPoint2F *pOffset,
        bool fReturnToLocalSpace        
        );
    
private:  
    bool HasScrollableArea() const
    {
        return (m_pScrollBag != NULL);
    }
    
    CComposition * m_pComposition;
    CMilScheduleRecord *m_pScheduleRecord;
    
    CMilRectF m_Bounds;

    // m_fNeedsBoundingBoxUpdate -- indicates that the bounding box of this node has
    // changed. There are several reasons for this A) a property of the node itself
    // changed or B) a node in its sub-graph changed.
    DWORD m_fNeedsBoundingBoxUpdate     :  1;

    // m_fIsDirtyForRenderInSubgraph -- indicates that a node in the sub-tree of this
    // node needs to be added to the dirty region.
    DWORD m_fIsDirtyForRenderInSubgraph :  1;

    // m_fIsDirtyForRender -- marks a node that needs to be rerendered. In other
    // words his dirty region must be added to the DirtyRegion collection.
    DWORD m_fIsDirtyForRender           :  1;

    // m_fSkipNodeRender -- this flag is used during the render walk to indicate if the
    // node should be skippped because it is clipped out.
    DWORD m_fSkipNodeRender             :  1;

    // Indicates that we are skipping rendering this node's properties, content, and
    // children as input to an Effect, either because the Effect doesn't use the
    // input or we have that input cached already.
    DWORD m_fUseCacheAsEffectInput    :  1;
    
    // Indicates that this node has partial dirty information. If this flag is true
    // additional dirty region is collected by calling the CollectAdditionalDirtyRegion
    // virtual method.
    DWORD m_fHasAdditionalDirtyRegion   :  1;

    // Indicates that the contents of this Visual (Drawing or RenderData) have changed.
    DWORD m_fHasContentChanged          :  1;

    // Set of flags indicating rendering options to set when traversing this node.
    DWORD m_renderOptionsFlags          : MIL_RENDEROPTIONSFLAGS_BITS;

    // This edge mode determines whether this Visual's content and children will be rendered aliased.
    // NB: We use two bits to ensure that this isn't compared with sign extend.
    // If we add states to MilEdgeMode::Enum we will need to use more bits for this field.
    UINT m_edgeMode                     : MIL_EDGE_MODE_BITS;

    UINT m_bitmapScalingMode            : MIL_BITMAPSCALING_MODE_BITS;

    UINT m_clearTypeHint                : MIL_CLEARTYPEHINT_BITS;

    UINT m_textRenderingMode            : MIL_TEXTRENDERINGMODE_BITS;
    UINT m_textHintingMode              : MIL_TEXTHINTINGMODE_BITS;

    // If m_renderOptionsFlags contains MilRenderOptionFlags::CompositingMode then this field
    // contains the compositing mode to use for this subtree
    UINT m_compositingMode : MIL_COMPOSITING_MODE_BITS;
    
    // Used by the scroll optimization to mark the root of a subtree that has its "old"
    // bounding box intersect with the scroll area. This means the subtree of this
    // node do not have to have to check their bounding boxes against the scrollable area.
    // In the case of bounding box update due to new child content, the new bounding box
    // will be added anyway through the regular path.
    // See comment on CPreComputeContext::ScrollableAreaHandling()
    DWORD m_fHasBoundingBoxAdded        : 1;

    // Used by the scroll optimization to determine whether any changes have occurred on this
    // node other than the offset change and other changes which still allow scrolling optimization
    // (eg if children are being added or removed).
    // If, for example, the offset and the transform of this
    // node were both changed, we could not perform the scroll optimization.
    // The flag gets set by default in PropagateFlags, as all interesting changes to the node will propagate
    // flags. When the offset is changed, it specifically tells PropagateFlags not to set this flag.
    // Thus by the time we are finished processing batches and are in the precompute walk, if this flag
    // is 0 we can be sure that only the offset changed on the node (or nothing changed at all).
    // See comment on CPreComputeContext::ScrollableAreaHandling()
    DWORD m_fHasStateOtherThanOffsetChanged : 1;

    // Used only during the precompute walk, so that PostSubgraph may recognize this node as one that
    // was treated specially in PreSubgraph, and take appropriate action
    // See comment on CPreComputeContext::ScrollableAreaHandling()
    DWORD m_fNodeWasScrolled : 1;

    // Indicates that the additional dirty rects array (m_rgpAdditionalDirtyRects) has exceeded
    // the size specified in c_maxAdditionalDirtyRects, and that all the entries have been unioned
    // into the first entry in the array, and all further entries should do likewise.
    DWORD m_fAdditionalDirtyRectsExceeded : 1;

    CMilVisualCacheSet *m_pCaches;
    CMilTransformDuce *m_pTransform;
    CMilEffectDuce *m_pEffect;
    CMilGeometryDuce *m_pClip;
    CGuidelineCollection *m_pGuidelineCollection;
    DOUBLE m_alpha;

    // Wrapper which contains the alpha mask and bounds
    CMilAlphaMaskWrapper *m_pAlphaMaskWrapper;
    CMilSlaveResource *m_pContent;  // Can be either Drawing or RenderData
    CMilVisual* m_pParent;
    CPtrArray<CMilVisual> m_rgpChildren;
    float m_offsetX;
    float m_offsetY;
    // This value must be > 1. See CMilVisual::AddAdditionalDirtyRects for explanation
    static const int c_maxAdditionalDirtyRects = 2;
    DynArray<CRectF<CoordinateSpace::LocalRendering>>* m_rgpAdditionalDirtyRects;

    // Property bag for information related to accelerated scrolling. Will only be non null if
    // a user has set ScrollableAreaClip on the associated Visual
    // See comment on CPreComputeContext::ScrollableAreaHandling()   
    ScrollableAreaPropertyBag *m_pScrollBag;
    
#if DBG==1
    UINT m_dwDirtyRegionEnableCount;
#endif

};


