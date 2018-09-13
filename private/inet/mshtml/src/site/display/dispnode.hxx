//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispnode.hxx
//
//  Contents:   CDispNode, base class for nodes in the display tree.
//
//----------------------------------------------------------------------------

#ifndef I_DISPNODE_HXX_
#define I_DISPNODE_HXX_
#pragma INCMSG("--- Beg 'dispnode.hxx'")

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

#ifndef X_REGION_HXX_
#define X_REGION_HXX_
#include "region.hxx"
#endif

#ifndef X_DISPFLAGS_HXX_
#define X_DISPFLAGS_HXX_
#include "dispflags.hxx"
#endif

#ifndef X_DISPEXTRAS_HXX_
#define X_DISPEXTRAS_HXX_
#include "dispextras.hxx"
#endif

#ifndef X_DISPFILTER_HXX_
#define X_DISPFILTER_HXX_
#include "dispfilter.hxx"
#endif


class CDispInteriorNode;
class CDispContext;
class CDispHitContext;
class CDispDrawContext;
class CDispLayerContext;
class CSaveDispContext;
class CRegion;
class CDispClient;

#define DECLARE_DISPNODE_ABSTRACT(NODECLASS, SUPERCLASS) \
    typedef SUPERCLASS super;

#if DBG==1
#define DECLARE_DISPNODE(NODECLASS, SUPERCLASS, NODETYPE) \
    typedef SUPERCLASS super; \
    friend class CDispRoot; \
    public: \
    virtual DISPNODETYPE GetNodeType() const {return NODETYPE;} \
    virtual size_t GetMemorySizeOfThis() const \
        {return sizeof(NODECLASS) + CDispExtras::GetExtrasSize(GetExtras());}
#else
#define DECLARE_DISPNODE(NODECLASS, SUPERCLASS, NODETYPE) \
    typedef SUPERCLASS super; \
    friend class CDispRoot; \
    public: \
    virtual DISPNODETYPE GetNodeType() const {return NODETYPE;}
#endif


// node types
typedef enum tagDISPNODETYPE
{
    DISPNODETYPE_RESERVED           = 0,    // reserved
    DISPNODETYPE_BALANCE            = 1,    // CDispBalanceNode
    DISPNODETYPE_ITEMPLUS           = 2,    // CDispItemPlus
    DISPNODETYPE_GROUP              = 3,    // CDispGroup
    // note: container and scroller types must be last (see CDispNode::IsContainer())
    DISPNODETYPE_CONTAINER          = 4,    // CDispContainer
    DISPNODETYPE_CONTAINERPLUS      = 5,    // CDispContainerPlus
    DISPNODETYPE_ROOT               = 6,    // CDispRoot
    // note: scroller types must be last (see CDispNode::IsScroller())
    DISPNODETYPE_SCROLLER           = 7,    // CDispScroller
    DISPNODETYPE_SCROLLERPLUS       = 8     // CDispScrollerPlus
} DISPNODETYPE;


//+---------------------------------------------------------------------------
//
//  Class:      CDispNode
//
//  Synopsis:   Base class for non-extensible nodes in the display tree.
//
//----------------------------------------------------------------------------

class CDispNode
{
    friend class CDispRoot;
    friend class CDispDrawContext;
    friend class CDispLeafNode;
    friend class CDispInteriorNode;
    friend class CDispContainer;
    friend class CDispContainerPlus;
    friend class CDispScroller;
    friend class CDispScrollerPlus;

protected:
    CDispFlags              _flags;
    CDispInteriorNode*      _pParentNode;
    CDispNode*              _pPreviousSiblingNode;
    CDispNode*              _pNextSiblingNode;
    CRect                   _rcVisBounds;   // union of children's visible bounds
    // 36 bytes (including vtbl pointer)

private:
    // The constructor and destructor of CDispNode are private to restrict
    // subclasses to CDispLeafNode and CDispInteriorNode.  The code assumes that
    // it can dynamically cast to one of these subclasses depending on the
    // result of IsLeafNode()/IsInteriorNode().
                            //CDispNode() {}
                            // BUGBUG (donmarsh) text areas and other specialized
                            // controls are not setting their visibility, so we
                            // have to default it for now
                            CDispNode() {SetFlag(CDispFlags::s_visibleNodeAndBranch);}
                            // _flags set to zero indicating the following defaults:
                            //      illegal layer
                            //      invisible
                            //      unowned
                            //      leaf node
                            //      no background
                            //      no lookaside cache
                            //      left-to-right coordinate system
                            //      affects scroll bounds
                            //      not filtered

#if DBG==1
    virtual                 ~CDispNode();
#else
    virtual                 ~CDispNode() {}
#endif

public:

    //
    // public interface
    //

    // call this instead of deleting directly
    void                    Destroy();

    // type checking methods
    virtual DISPNODETYPE    GetNodeType() const = 0;
    BOOL                    IsLeafNode() const
                                    {return _flags.IsLeafNode();}
    BOOL                    IsInteriorNode() const
                                    {return _flags.IsInteriorNode();}
    BOOL                    IsContainer() const
                                    {return GetNodeType()>=DISPNODETYPE_CONTAINER;}
    BOOL                    IsScroller() const
                                    {return GetNodeType()>=DISPNODETYPE_SCROLLER;}
    BOOL                    IsDispRoot() const
                                    {return GetNodeType()==DISPNODETYPE_ROOT;}

    // tree traversal methods
    CDispNode*              GetRootNode() const;
    CDispInteriorNode*      GetParentNode() const;
    CDispNode*              GetPreviousSiblingNode() const;
    CDispNode*              GetPreviousSiblingNode(BOOL fRestrictToLayer) const;
    CDispNode*              GetNextSiblingNode() const;
    CDispNode*              GetNextSiblingNode(BOOL fRestrictToLayer) const;

    BOOL                    TraverseTreeTopToBottom(void* pClientData);

    // tree construction methods
    void                    InsertSiblingNode(
                                CDispNode* pNewSibling,
                                BOOL fBefore = FALSE)
                                    {if (fBefore)
                                        InsertPreviousSiblingNode(pNewSibling);
                                    else InsertNextSiblingNode(pNewSibling);}
    void                    InsertPreviousSiblingNode(CDispNode* pNewSibling);
    void                    InsertNextSiblingNode(CDispNode* pNewSibling);
    void                    ExtractFromTree();
    void                    ReplaceNode(CDispNode* pOldNode, BOOL fKeep = FALSE, BOOL fTakeChildren = TRUE);
    void                    ReplaceParent();
    void                    InsertParent(CDispInteriorNode* pNewParent);

    // node ownership
    BOOL                    IsOwned() const
                                    {return _flags.IsOwned();}
    void                    SetOwned(BOOL fOwned = TRUE)
                                    {_flags.SetOwned(fOwned);}

    // layering
    DISPNODELAYER           GetLayerType() const
                                    {return _flags.GetLayerType();}
    void                    SetLayerType(DISPNODELAYER layerType)
                                    {_flags.SetLayerType(layerType);}
    static BOOL             IsPositionedLayer(DISPNODELAYER layerType)
                                    {Assert(layerType == DISPNODELAYER_NEGATIVEZ ||
                                            layerType == DISPNODELAYER_FLOW ||
                                            layerType == DISPNODELAYER_POSITIVEZ);
                                    return layerType != DISPNODELAYER_FLOW;}
    virtual LONG            GetZOrder() const = 0;
            LONG            CompareZOrder(CDispNode* pDispNode) const
                                    {Assert(pDispNode);
                                    return GetDispClient()->CompareZOrder((CDispNode *)this, pDispNode);}

    // size and position
    virtual void            GetSize(SIZE* psize) const
                                    {Assert(FALSE);
                                    *psize = g_Zero.size;}
    virtual void            SetSize(const SIZE& size, BOOL fInvalidateAll)
                                    {Assert(FALSE);}
    virtual const CPoint&   GetPosition() const
                                    {Assert(FALSE);
                                    return (const CPoint&) g_Zero.pt;}
    virtual void            GetPositionTopRight(CPoint* pptTopRight)
                                    {Assert(FALSE);}
    virtual void            SetPosition(const POINT& ptTopLeft)
                                    {Assert(FALSE);}
    virtual void            SetPositionTopRight(const POINT& ptTopRight)
                                    {Assert(FALSE);}
    virtual void            GetBounds(RECT* prcBounds) const
                                    {*prcBounds = _rcVisBounds;}
    virtual const CRect&    GetBounds() const
                                    {return _rcVisBounds;}
    void                    GetBounds(
                                RECT* prcBounds,
                                COORDINATE_SYSTEM coordinateSystem) const;
    void                    GetClippedBounds(
                                RECT* prcBounds,
                                COORDINATE_SYSTEM coordinateSystem) const;
    virtual void            FlipBounds()
                                    {Assert(FALSE);}
    BOOL                    IsRightToLeft() const
                                    {return IsSet(CDispFlags::s_rightToLeft);}
    BOOL                    AffectsScrollBounds() const
                                    {return !IsSet(CDispFlags::s_noScrollBounds);}
    void                    SetAffectsScrollBounds(BOOL fAffectsScrollBounds)
                                    {SetBoolean(CDispFlags::s_noScrollBounds,
                                                !fAffectsScrollBounds);}

    // visibility and opacity methods
    BOOL                    IsVisible() const
                                    {return _flags.IsSet(CDispFlags::s_visibleNode);}
    void                    SetVisible(BOOL fVisible = TRUE);
    
    BOOL                    IsOpaque() const
                                    {return _flags.IsSet(CDispFlags::s_opaqueNode);}
    void                    SetOpaque(BOOL fOpaque)
                                    {if (!IsFiltered())
                                        UnfilteredSetOpaque(fOpaque);
                                    else
                                        GetFilter()->SetOpaque(fOpaque);}
    void                    UnfilteredSetOpaque(BOOL fOpaque);
    
    BOOL                    IsInView() const
                                    {return _flags.IsSet(CDispFlags::s_inView);}

    BOOL                    GetClipRect(RECT* prcClip) const;
    virtual void            GetClientRect(RECT* prc, CLIENTRECT type) const
                                    {AssertSz(FALSE,
                                    "Unexpected call to CDispNode::GetClientRect");
                                    *prc = g_Zero.rc;}
    void                    GetClippedClientRect(RECT* prc, CLIENTRECT type) const;

    // filtering
    BOOL                    IsFiltered() const
                                    {return IsSet(CDispFlags::s_filtered);}
    void                    SetFiltered(BOOL fFiltered);

    CDispFilter*            GetFilter() const
                                    {Assert(IsFiltered());
                                    CDispClient* pClient = GetDispClient();
                                    Assert(pClient);
                                    CDispFilter* pFilter = pClient->GetFilter();
                                    Assert(pFilter);
                                    return pFilter;}

    //
    // BUGBUG. FATHIT. marka - Fix for Bug 65015 - enabling "Fat" hit testing on tables.
    // Edit team is to provide a better UI-level way of dealing with this problem for post IE5.
    //

    BOOL                    IsFatHitTest() const
                                    {return IsSet(CDispFlags::s_fatHitTest);}

    void                    SetFatHitTest(BOOL fFatHitTest)
                                    { SetBoolean( CDispFlags::s_fatHitTest ,fFatHitTest);}                                        

    // node invalidation methods
    BOOL                    IsInvalidationVisible() const
                                    {return _flags.MaskedEquals(
                                                    CDispFlags::s_invalAndVisible,
                                                    CDispFlags::s_visible);}

    void                    Invalidate(
                                const CRect& rcInvalid,
                                COORDINATE_SYSTEM coordinateSystem,
                                BOOL fSynchronousRedraw = FALSE,
                                BOOL fIgnoreFilter = FALSE)
                                    {if (IsInvalidationVisible())
                                        PrivateInvalidate(rcInvalid, coordinateSystem,
                                                        fSynchronousRedraw, fIgnoreFilter); }

    void                    Invalidate(
                                const CRegion& rgnInvalid,
                                COORDINATE_SYSTEM coordinateSystem,
                                BOOL fSynchronousRedraw = FALSE,
                                BOOL fIgnoreFilter = FALSE)
                                    {if (IsInvalidationVisible())
                                        PrivateInvalidate(rgnInvalid, coordinateSystem,
                                                        fSynchronousRedraw, fIgnoreFilter); }

    // hit testing
    virtual BOOL            HitTest(
                                CPoint* pptHit,
                                COORDINATE_SYSTEM coordinateSystem,
                                void* pClientData,
                                BOOL fHitContent,
                                long cFuzzyHitTest = 0);
    virtual CDispScroller * HitScrollInset(CPoint *pptHit, DWORD *pdwScrollDir)
                                    {Assert(FALSE); return NULL;};

    // background methods
    BOOL                    HasBackground() const
                                    {return _flags.HasBackground();}
    void                    SetBackground(BOOL fHasBackground = TRUE)
                                    {_flags.SetBackground(fHasBackground);}
    // BUGBUG (donmarsh) - move these methods to CDispScroller, put the HasFixedBackground
    // flag there, and return it in CDispInfo for use by CDispContainer::Draw.
    BOOL                    HasFixedBackground() const
                                    {Assert(IsScroller());
                                    return _flags.HasFixedBackground();}
    void                    SetFixedBackground(BOOL fFixedBackground = TRUE)
                                    {Assert(IsScroller());
                                    _flags.SetFixedBackground(fFixedBackground);}

    // position awareness - does this node need to be told about position
    // changes?
    BOOL                    IsPositionAware() const
                                    {return _flags.IsSet(
                                        CDispFlags::s_positionChange);}
    void                    SetPositionAware(BOOL fPositionAware = TRUE)
                                    {SetBooleanBranchFlag(
                                        CDispFlags::s_positionChange,
                                        fPositionAware);
                                    SetBooleanBranchFlag(
                                        CDispFlags::s_inViewChange,
                                        fPositionAware);
                                    RequestRecalc();}

    // in view awareness - does this node need to be told about in view
    // status changes?
    BOOL                    IsInViewAware() const
                                    {return _flags.IsSet(
                                        CDispFlags::s_inViewChange);}
    void                    SetInViewAware(BOOL fInViewAware = TRUE)
                                    {Assert(fInViewAware ||
                                            !IsSet(CDispFlags::s_positionChange));
                                    SetBooleanBranchFlag(
                                        CDispFlags::s_inViewChange,
                                        fInViewAware);
                                    RequestRecalc();}
    void                    RequestViewChange()
                                    {Assert(IsSet(CDispFlags::s_inViewChange));
                                    SetFlag(CDispFlags::s_positionHasChanged);
                                    RequestRecalc();}

    // insertion awareness - does this node need to be told when inserted
    // into the tree?
    BOOL                    IsInsertionAware() const
                                    {return _flags.IsSet(
                                        CDispFlags::s_needsJustInserted);}
    void                    SetInsertionAware(BOOL fInsertionAware = TRUE)
                                    {SetBooleanBranchFlag(
                                        CDispFlags::s_needsJustInserted,
                                        fInsertionAware);
                                    RequestRecalc();}

    //
    // extras
    //
    BOOL                    HasExtraCookie() const
                                    {return HasExtraCookie(GetExtras());}
    void*                   GetExtraCookie() const
                                    {return GetExtraCookie(GetExtras());}
    void                    SetExtraCookie(void* cookie)
                                    {SetExtraCookie(cookie, GetExtras());}

    BOOL                    HasUserClip() const
                                    {return IsSet(CDispFlags::s_hasUserClip);}
    const RECT&             GetUserClip() const
                                    {return GetUserClip(GetExtras());}
    void                    SetUserClip(const RECT& rcUserClip)
                                    {SetUserClip(rcUserClip, GetExtras());}

    BOOL                    HasInset() const
                                    {return HasInset(GetExtras());}
    const CSize&            GetInset() const
                                    {return GetInset(GetExtras());}
    void                    SetInset(const SIZE& sizeInset)
                                    {SetInset(sizeInset, GetExtras());}

    BOOL                    HasBorder() const
                                    {return HasBorder(GetExtras());}
    DISPNODEBORDER          GetBorderType() const
                                    {return GetBorderType(GetExtras());}
    void                    GetBorderWidths(RECT* prcBorderWidths) const
                                    {GetBorderWidths(prcBorderWidths, GetExtras());}
    void                    SetBorderWidths(LONG borderWidth)
                                    {SetBorderWidths(borderWidth, GetExtras());}
    void                    SetBorderWidths(const CRect& rcBorderWidths)
                                    {SetBorderWidths(rcBorderWidths, GetExtras());}


    // coordinate system conversion
    void                    GetTransformOffset(
                                CSize* pOffset,
                                COORDINATE_SYSTEM source,
                                COORDINATE_SYSTEM destination) const;
    void                    GetTransformContext(
                                CDispContext* pContext,
                                COORDINATE_SYSTEM source,
                                COORDINATE_SYSTEM destination) const;

    // the following are for convenience.  If you need to transform multiple
    // points or rects, or you want to do clipping, use one of the methods above.
    void                    TransformPoint(
                                CPoint* ppt,
                                COORDINATE_SYSTEM source,
                                COORDINATE_SYSTEM destination) const
                                    {CSize offset;
                                    GetTransformOffset(
                                        &offset, source, destination);
                                    *ppt += offset;}
    void                    TransformRect(
                                CRect* prc,
                                COORDINATE_SYSTEM source,
                                COORDINATE_SYSTEM destination,
                                BOOL fClip = FALSE) const;

    // Scroll the given rect *** IN CONTENT COORDINATES *** into view.
    // NOTE: Do not call ScrollRectIntoView for rectangles associated with
    // positioned (non-flow) content, as optional insets will be added incorrectly
    // for such items.  Use ScrollIntoView instead.
    BOOL                    ScrollRectIntoView(
                                const CRect& rc,
                                CRect::SCROLLPIN spVert,
                                CRect::SCROLLPIN spHorz,
                                BOOL fScrollBits)
                                    {return ScrollRectIntoView(
                                        rc, spVert, spHorz, COORDSYS_CONTENT, fScrollBits);}

    // scroll the given node into view
    virtual BOOL            ScrollIntoView(
                                CRect::SCROLLPIN spVert,
                                CRect::SCROLLPIN spHorz);

    // override in derived classes that have a CDispClient*
    virtual CDispClient*    GetDispClient() const {return NULL;}
                                            
    // special call to draw this node's content for a filter to operate on
    void                    DrawNodeForFilter(
                                CDispDrawContext* pContext,
                                HDC hdc,
                                const CPoint& ptOrg);
    
    //
    // internal interface
    //
protected:

    void                    PrivateInvalidate(
                                const CRect& rcInvalid,
                                COORDINATE_SYSTEM coordinateSystem,
                                BOOL fSynchronousRedraw = FALSE,
                                BOOL fIgnoreFilter = FALSE);

    void                    PrivateInvalidate(
                                const CRegion& rgnInvalid,
                                COORDINATE_SYSTEM coordinateSystem,
                                BOOL fSynchronousRedraw = FALSE,
                                BOOL fIgnoreFilter = FALSE);

    BOOL                    IsBalanceNode() const
                                    {return _flags.IsBalanceNode();}

    virtual BOOL            CalculateInView(
                                CDispContext* pContext,
                                BOOL fPositionChanged,
                                BOOL fNoRedraw)
                                    {Assert(FALSE); return FALSE;}
    virtual BOOL            PreDraw(CDispDrawContext* pContext);
    virtual void            DrawSelf(
                                CDispDrawContext* pContext,
                                CDispNode* pChild = NULL) = 0;
    virtual BOOL            HitTestPoint(CDispHitContext* pContext) const = 0;
    virtual void            GetNodeTransform(
                                CSize* pOffset,
                                COORDINATE_SYSTEM source,
                                COORDINATE_SYSTEM destination) const;
    virtual void            GetNodeTransform(
                                CDispContext* pContext,
                                COORDINATE_SYSTEM source,
                                COORDINATE_SYSTEM destination) const;
    void                    TransformRegion(
                                CRegion* prgn,
                                COORDINATE_SYSTEM source,
                                COORDINATE_SYSTEM destination,
                                BOOL fClip = FALSE) const;
    virtual void            CalcDispInfo(
                                const CRect& rcClip,
                                CDispInfo* pdi) const
                                    {AssertSz(FALSE, "Unexpected call to CalcDispInfo");}
                                    
    virtual BOOL            ScrollRectIntoView(
                                const CRect& rc,
                                CRect::SCROLLPIN spVert,
                                CRect::SCROLLPIN spHorz,
                                COORDINATE_SYSTEM coordinateSystem,
                                BOOL fScrollBits);
    COORDINATE_SYSTEM       GetContentCoordinateSystem() const
                                    {return (GetLayerType() == DISPNODELAYER_FLOW
                                             ? COORDSYS_CONTENT
                                             : COORDSYS_NONFLOWCONTENT);}
    void                    RequestRecalc();

    CDispNode*              GetNextNodeInZOrder() const;

    static void             SkipLayer(DISPNODELAYER layer, CDispNode** ppNode)
                                    {while (*ppNode != NULL &&
                                            (*ppNode)->GetLayerType() == layer)
                                        *ppNode = (*ppNode)->_pNextSiblingNode;}
    
    void                    Draw(CDispDrawContext* pContext, CDispNode* pChild);
    
    //
    // convenience methods for manipulating node flags
    //

    void                    SetFlag(const CDispFlags& flag)
                                    {_flags.Set(flag);}
    void                    SetFlags(const CDispFlags& flags,
                                      const CDispFlags& mask)
                                    {_flags.SetMasked(flags,mask);}
    void                    ClearFlag(const CDispFlags& flag)
                                    {_flags.Clear(flag);}
    CDispFlags              GetFlag(const CDispFlags& mask) const
                                    {CDispFlags result(_flags);
                                    result.Mask(mask);
                                    return result;}
    const CDispFlags&       GetFlags() const
                                    {return _flags;}

    BOOL                    IsSet(const CDispFlags& flag) const
                                    {return _flags.IsSet(flag);}
    BOOL                    AllSet(const CDispFlags& flags) const
                                    {return _flags.AllSet(flags);}
    BOOL                    Select(const CDispFlags& mask,
                                    const CDispFlags& value) const
                                    {return _flags.MaskedEquals(mask, value);}
    void                    SetBoolean(const CDispFlags& flag, BOOL fOn)
                                    {_flags.SetBoolean(flag, fOn);}

    void                    SetBranchFlag(const CDispFlags& flag);
    void                    ClearBranchFlag(const CDispFlags& flag);
    void                    SetBooleanBranchFlag(const CDispFlags& flag, BOOL fOn)
                                    {if (fOn) SetBranchFlag(flag);
                                    else ClearBranchFlag(flag);}
    void                    ClearFlagToRoot(const CDispFlags& flag);

    //
    // extras support
    // 
    
    virtual CDispExtras*    GetExtras() const {return NULL;}
    
    BOOL                    HasExtraCookie(const CDispExtras* pExtras) const
                                    {return pExtras != NULL &&
                                        pExtras->HasExtraCookie();}
    void*                   GetExtraCookie(const CDispExtras* pExtras) const
                                    {Assert(pExtras != NULL);
                                    return pExtras->GetExtraCookie();}
    void                    SetExtraCookie(void* cookie, CDispExtras* pExtras)
                                    {Assert(pExtras != NULL);
                                    pExtras->SetExtraCookie(cookie);}
    
    const RECT&             GetUserClip(const CDispExtras* pExtras) const
                                    {Assert(pExtras != NULL);
                                    return pExtras->GetUserClip();}
    void                    SetUserClip(const RECT& rcUserClip, CDispExtras* pExtras);
    
    BOOL                    HasInset(const CDispExtras* pExtras) const
                                    {return pExtras != NULL &&
                                        pExtras->HasInset();}
    const CSize&            GetInset(const CDispExtras* pExtras) const
                                    {Assert(pExtras != NULL);
                                    return (const CSize&) pExtras->GetInset();}
    void                    SetInset(const SIZE& sizeInset, CDispExtras* pExtras)
                                    {Assert(pExtras != NULL);
                                    pExtras->SetInset(sizeInset);
                                    RequestRecalc();
                                    SetFlag(CDispFlags::s_invalAndRecalcChildren);}
    
    BOOL                    HasBorder(const CDispExtras* pExtras) const
                                    {return pExtras != NULL &&
                                        pExtras->GetBorderType() != DISPNODEBORDER_NONE;}
    DISPNODEBORDER          GetBorderType(const CDispExtras* pExtras) const
                                    {return (pExtras == NULL) ? DISPNODEBORDER_NONE
                                        : pExtras->GetBorderType();}
    void                    GetBorderWidths(RECT* prcBorderWidths, const CDispExtras* pExtras) const
                                    {if (pExtras == NULL) *prcBorderWidths = g_Zero.rc;
                                    else pExtras->GetBorderWidths((CRect*) prcBorderWidths);}
    void                    SetBorderWidths(LONG borderWidth, CDispExtras* pExtras)
                                    {Assert(pExtras != NULL);
                                    pExtras->SetBorderWidths(borderWidth);
                                    RequestRecalc();
                                    SetFlag(CDispFlags::s_invalAndRecalcChildren);}
    void                    SetBorderWidths(const CRect& rcBorderWidths, CDispExtras* pExtras)
                                    {Assert(pExtras != NULL);
                                    pExtras->SetBorderWidths(rcBorderWidths);
                                    RequestRecalc();
                                    SetFlag(CDispFlags::s_invalAndRecalcChildren);}
    
    
    // called after insertion or removal of nodes
    void                    Recalc(
                                BOOL fForceRecalc,
                                BOOL fSuppressInval,
                                CDispDrawContext* pContext);

    BOOL                    MustInvalidate() const;
    BOOL                    SubtractOpaqueSiblings(CRegion* pRegion);

    void                    InvalidateEdges(
                                const CRect& rcOld,
                                const CRect& rcNew,
                                const CRect& rcBorderWidths,
                                BOOL fRightToLeft);

    static void             ClipToParentCoords(
                                CRect* prcDst,
                                const CRect& rcSrc,
                                const CSize& sizeParent)
                                    {prcDst->left = max(0L, rcSrc.left);
                                    prcDst->top = max(0L, rcSrc.top);
                                    prcDst->right =
                                        min(sizeParent.cx, rcSrc.right);
                                    prcDst->bottom =
                                        min(sizeParent.cy, rcSrc.bottom);}

#if DBG==1
public:
    void                    DumpDisplayTree();
    void                    DumpDisplayNode();
    static void             DumpStart(HANDLE hFile);
    static void             DumpEnd(HANDLE hFile);
    virtual void            Dump(HANDLE hFile, long level, long maxLevel, long childNumber);
    virtual void            VerifyTreeCorrectness() const;
protected:
    virtual void            DumpClassName(HANDLE hFile, long level, long childNumber);
    virtual void            DumpInfo(HANDLE hFile, long level, long childNumber);
    void                    DumpRect(HANDLE hFile, const RECT& rc);
    void                    DumpRect(HANDLE hFile, const RECT& rc, const CDispFlags& flags);
    virtual void            DumpBounds(HANDLE hFile, long level, long childNumber);
    virtual void            DumpCoord(HANDLE hFile, TCHAR* pszLabel, LONG coord, BOOL fHighlight);
    void                    DumpEndLine(HANDLE hFile);
    virtual void            DumpFlags(HANDLE hFile, long level, long childNumber);
    virtual void            DumpChildren(HANDLE hFile, long level, long maxLevel, long childNumber);
    virtual size_t          GetMemorySizeOfThis() const = 0;
    virtual size_t          GetMemorySize() const
                                    {return GetMemorySizeOfThis();}
    void                    VerifyRecalc() const;
#endif
};

#if DBG==1
extern void WriteHelp(HANDLE hFile, TCHAR *format, ...);
extern void WriteString(HANDLE hFile, TCHAR *pszStr);
#endif

#pragma INCMSG("--- End 'dispnode.hxx'")
#else
#pragma INCMSG("*** Dup 'dispnode.hxx'")
#endif


