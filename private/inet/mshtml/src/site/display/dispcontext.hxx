//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispcontext.hxx
//
//  Contents:   Context object passed throughout display tree.
//
//----------------------------------------------------------------------------

#ifndef I_DISPCONTEXT_HXX_
#define I_DISPCONTEXT_HXX_
#pragma INCMSG("--- Beg 'dispcontext.hxx'")

#ifndef X_REGION_HXX_
#define X_REGION_HXX_
#include "region.hxx"
#endif

#ifndef X_REGIONSTACK_HXX_
#define X_REGIONSTACK_HXX_
#include "regionstack.hxx"
#endif

#ifndef X_DISPFLAGS_HXX_
#define X_DISPFLAGS_HXX_
#include "dispflags.hxx"
#endif

class CDispContextStack;
class CDispSurface;
class CDispExtras;
class CDispInfo;

MtExtern(CDispContext)
MtExtern(CDispHitContext)
MtExtern(CDispDrawContext)
MtExtern(CDispLayerContext)


//+---------------------------------------------------------------------------
//
//  Class:      CDispContext
//
//  Synopsis:   Base class for context objects passed throughout display tree.
//
//----------------------------------------------------------------------------

class CDispContext
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CDispContext))
    
                    CDispContext() {}
                    CDispContext(const CRect& rcClip, const CSize& offset)
                            {_rcClip = rcClip; _offset = offset;}
                    CDispContext(const CDispContext& c)
                            {_rcClip = c._rcClip; _offset = c._offset;}
                    ~CDispContext() {}
    
    CDispContext&   operator= (const CDispContext& c)
                            {_rcClip = c._rcClip; _offset = c._offset;
                            return *this;}
    
    // SetNoClip() sets the clip rect to a really big rect, but not
    // the maximum rect, because otherwise translations may cause it to
    // over/underflow.  (For speed reasons, we aren't using CRect::SafeOffset)
    void            SetNoClip()
                            {static const LONG bigVal = 100000000;
                            _rcClip.SetRect(-bigVal, -bigVal, bigVal, bigVal);}
    
    void            SetToIdentity()
                            {_offset = g_Zero.size; SetNoClip();}
    
    void            SetClipRect(const CRect& rcClip)
                            {_rcClip = rcClip;}
    const CRect&    GetClipRect() const
                            {return _rcClip;}
    
    // transform a point or region or rect to new coordinate system, with
    // clipping for rects and regions
    void            Transform(CPoint* ppt) const
                            {*ppt += _offset;}
    
    void            Transform(CRegion* prgn) const
                            {prgn->Intersect(_rcClip);
                            prgn->Offset(_offset);}

    void            Transform(CRect* prc) const
                            {prc->IntersectRect(_rcClip);
                            prc->OffsetRect(_offset);}

    void            GetTransformedClipRect(CRect* prcClip) const
                            {prcClip->left = _rcClip.left + _offset.cx;
                            prcClip->top = _rcClip.top + _offset.cy;
                            prcClip->right = _rcClip.right + _offset.cx;
                            prcClip->bottom = _rcClip.bottom + _offset.cy;}
    
    // reverse transforms
    void            Untransform(CPoint* ppt) const
                            {*ppt -= _offset;}
    
    void            Untransform(CRect* prc) const
                            {prc->OffsetRect(-_offset);
                            prc->IntersectRect(_rcClip);}

    void            GetUntransformedClipRect(CRect* prcClip) const
                            {*prcClip = _rcClip;}
    const CRect&    GetUntransformedClipRect() const
                            {return _rcClip;}
    
    // to transform from one coordinate system to another, first clip,
    // using _rcClip (which is in source coordinates), then add _offset
    CRect   _rcClip;    // in source coordinates
    CSize   _offset;    // add to produce destination coordinates
};


//+---------------------------------------------------------------------------
//
//  Class:      CDispHitContext
//
//  Synopsis:   Context used for hit testing.
//
//----------------------------------------------------------------------------

class CDispHitContext :
    public CDispContext
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CDispHitContext))
    
    CDispHitContext() {}
    ~CDispHitContext() {}
    
    BOOL            RectIsHit(const CRect& rc) const
                            {return rc.Contains(_ptHitTest);}

    //
    // BUGBUG. FATHIT. marka - Fix for Bug 65015 - enabling "Fat" hit testing on tables.
    // Edit team is to provide a better UI-level way of dealing with this problem for post IE5.
    //
    // BUGBUG - when this change is done - signature of FuzzyRectIsHit *must* be changed
    // to not take an extra param saying whether to do FatHitTesting
    //

    BOOL            FuzzyRectIsHit(const CRect& rc, BOOL fFatHitTest ); 

    //
    // BUGBUG. FATHIT. marka - Fix for Bug 65015 - enabling "Fat" hit testing on tables.
    // Edit team is to provide a better UI-level way of dealing with this problem for post IE5.
    //

    BOOL            FatRectIsHit(const CRect& rc);
                            
    
    // hit testing
    CPoint          _ptHitTest;             // hit test point
    void*           _pClientData;           // client supplied data
    long            _cFuzzyHitTest;         // pixels to extend hit test rects
};


//+---------------------------------------------------------------------------
//
//  Class:      CDispDrawContext
//
//  Synopsis:   Context used for PreDraw and Draw.
//
//----------------------------------------------------------------------------

class CDispDrawContext :
    public CDispContext
{
    friend class CDispNode;
    friend class CDispLeafNode;
    friend class CDispInteriorNode;
    friend class CDispContentNode;
    friend class CDispItemPlus;
    friend class CDispRoot;
    friend class CDispContainer;
    friend class CDispContainerPlus;
    friend class CDispScroller;
    friend class CDispScrollerPlus;
    friend class CDispSurface;
    friend class CDispLayerContext;
    
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDispDrawContext))

                            CDispDrawContext()
                                : _drawSelector(CDispFlags::s_drawSelector)
                                , _fBypassFilter(FALSE) {}
                            ~CDispDrawContext() {}

    
    // client data
    void                    SetClientData(void* pClientData)
                                    {_pClientData = pClientData;}
    void*                   GetClientData() {return _pClientData;}

    CDispSurface*           GetDispSurface();
    void                    SetDispSurface(CDispSurface* pSurface)
                                    {_pDispSurface = pSurface; SetSurfaceRegion(); }

protected:
    CDispContext&           operator= (const CDispContext& c)
                                    {_rcClip = c._rcClip; _offset = c._offset;
                                    return *this;}
    
    // modify the current offset to global coordinates
    void                    AddGlobalOffset(const CSize& offset)
                                    {_offset += offset;}
    void                    SubtractGlobalOffset(const CSize& offset)
                                    {_offset -= offset;}
    const CSize&            GetGlobalOffset() const
                                    {return _offset;}
    void                    SetGlobalOffset(const CSize& offset)
                                    {_offset = offset;}
    
    void                    SetRedrawRegion(CRegion* prgn)
                                {_prgnRedraw = prgn;}
    CRegion*                GetRedrawRegion()
                                {return _prgnRedraw;}
    void                    AddToRedrawRegion(const CRect& rc)
                                {CRect rcGlobal(rc);
                                Transform(&rcGlobal);
                                AddToRedrawRegionGlobal(rcGlobal);}
    void                    AddToRedrawRegionGlobal(const CRect& rcGlobal);
    void                    IntersectRedrawRegion(CRegion* prgn)
                                {Transform(prgn);
                                prgn->Intersect(*_prgnRedraw);
                                prgn->Offset(-_offset);}
    void                    IntersectRedrawRegion(CRect *prc)
                                {Transform(prc);
                                prc->IntersectRect(_prgnRedraw->GetBounds());
                                prc->OffsetRect(-_offset);}
    BOOL                    IntersectsRedrawRegion(const CRect& rc)
                                {CRect rcGlobal(rc);
                                Transform(&rcGlobal);
                                return _prgnRedraw->Intersects(rcGlobal);}
    BOOL                    IsRedrawRegionEmpty() const
                                {return _prgnRedraw->IsEmpty();}
    BOOL                    RectContainsRedrawRegion(const CRect& rc) const
                                {CRect rcRedraw;
                                _prgnRedraw->GetBounds(&rcRedraw);
                                rcRedraw.OffsetRect(-_offset);    // local coords.
                                return rc.Contains(rcRedraw);}
    
    // the following methods deal with the region redraw stack, which is used
    // during PreDraw traversal to save the redraw region before opaque and
    // buffered items are subtracted from it
    void                    SetRedrawRegionStack(CRegionStack* pRegionStack)
                                    {_pRedrawRegionStack = pRegionStack;}
    CRegionStack*           GetRedrawRegionStack() const
                                    {return _pRedrawRegionStack;}
    BOOL                    PushRedrawRegion(
                                const CRegion& rgn,
                                void* key);
    BOOL                    PopRedrawRegionForKey(void* key)
                                    {if (!_pRedrawRegionStack->
                                         PopRegionForKey(key, &_prgnRedraw))
                                        return FALSE;
                                    SetSurfaceRegion();
                                    return TRUE;}
    void*                   PopFirstRedrawRegion()
                                    {void* key = 
                                        _pRedrawRegionStack->PopFirstRegion(
                                            &_prgnRedraw);
                                    SetSurfaceRegion();
                                    return key;}
    void                    RestorePreviousRedrawRegion()
                                    {delete _prgnRedraw;
                                    _prgnRedraw = _pRedrawRegionStack->
                                        RestorePreviousRegion();}
    
    
    // manage context stack for drawing
    void                    SetContextStack(CDispContextStack* pContextStack)
                                    {_pContextStack = pContextStack;}
    CDispContextStack*      GetContextStack() const {return _pContextStack;}
    BOOL                    PopContext(CDispNode* pDispNode);
    void                    FillContextStack(CDispNode* pDispNode);
    void                    SaveContext(
                                const CDispNode *pDispNode,
                                const CDispContext& context);
    
    // internal methods
    HDC                     GetRawDC();

    BOOL            _fBypassFilter;         // Bypass filter for rendering
protected:
    
    // redraw region
    CRegion*        _prgnRedraw;            // redraw region (global coords.)
    CRegion         _rgnRedrawClipped;      // redraw region clipped to current band
    CRegionStack*   _pRedrawRegionStack;    // stack of redraw regions
    CDispNode*      _pFirstDrawNode;        // first node to draw
    CDispRoot*      _pRootNode;             // root node for this tree

    // context stack
    CDispContextStack*  _pContextStack;     // saved context information for Draw
                                            
    // display surface
    CDispSurface*   _pDispSurface;          // display surface

    // client data
    void*           _pClientData;           // client data

    // draw selector
    CDispFlags      _drawSelector;          // choose which nodes to draw

private:
    void            SetSurfaceRegion();
};


//+---------------------------------------------------------------------------
//
//  Class:      CDispContextStack
//
//  Synopsis:   Store interesting parts of the display context (such as
//              clipping rects and offsets) for a branch of the display
//              tree, so that we don't have n^2 calculation of this
//              information while drawing.
//
//----------------------------------------------------------------------------

class CDispContextStack
{
public:
                    CDispContextStack() {_stackIndex = _stackMax = 0;}
                    ~CDispContextStack() {}
                            
    void            Init()  {_stackIndex = _stackMax = 0;}
    void            Restore() {_stackIndex = 0;}
    
    BOOL            PopContext(CDispContext* pContext, const CDispNode* pDispNode)
                            {if (_stackIndex == _stackMax)  // run out of stack
                                return FALSE;
#if DBG==1
                            Assert(_stack[_stackIndex]._pDispNode == pDispNode);
#endif
                            *pContext = _stack[_stackIndex++]._context;
                            return TRUE;}
                            
    BOOL            MoreToPop() const {return _stackIndex < _stackMax;}
    BOOL            IsEmpty() const {return _stackMax == 0;}
    
    void            ReserveSlot(const CDispNode* pDispNode)
                            {
#if DBG==1
                            if (_stackIndex < CONTEXTSTACKSIZE)
                                _stack[_stackIndex]._pDispNode = pDispNode;
#endif
                            _stackIndex++;
                            if (_stackMax < CONTEXTSTACKSIZE)
                                _stackMax++;}
    
    void            PushContext(
                        const CDispContext& context,
                        const CDispNode* pDispNode)
                            {if (--_stackIndex >= CONTEXTSTACKSIZE) return;
#if DBG==1
                            Assert(_stackIndex >= 0 && _stackIndex < _stackMax);
                            Assert(_stack[_stackIndex]._pDispNode == pDispNode);
#endif
                            _stack[_stackIndex]._context = context;}
    
    void            SaveContext(
                        const CDispContext& context,
                        const CDispNode* pDispNode)
                            {if (_stackIndex >= CONTEXTSTACKSIZE) return;
#if DBG==1
                            _stack[_stackIndex]._pDispNode = pDispNode;
#endif
                            _stack[_stackIndex++]._context = context;
                            Assert(_stackIndex > _stackMax);
                            _stackMax = _stackIndex;}
    
#if DBG==1
    const CDispNode*    GetTopNode()
                            {return (_stackIndex >= 0 && _stackIndex < _stackMax)
                                ? _stack[_stackIndex]._pDispNode : NULL;}
#endif

private:
    enum            {CONTEXTSTACKSIZE = 32};
    struct stackElement
    {
        CDispContext        _context;
#if DBG==1
        const CDispNode*    _pDispNode;
#endif
    };
               
    int             _stackIndex;
    int             _stackMax;
    stackElement    _stack[CONTEXTSTACKSIZE];
};


//+---------------------------------------------------------------------------
//
//  Class:      CApplyUserClip
//
//  Synopsis:   A stack-based object which incorporates optional user clip for
//              the given display node into the total clip rect in the context,
//              and removes it when destructed.
//
//----------------------------------------------------------------------------

class CApplyUserClip
{
public:
                            CApplyUserClip(CDispNode* pNode, CDispContext* pContext);
                            ~CApplyUserClip()
                                    {if (_fHasUserClip)
                                        _pContext->_rcClip = _rcClipSave;}
    
    
private:
    BOOL            _fHasUserClip;
    CDispContext*   _pContext;
    CRect           _rcClipSave;
};


#ifdef NEVER
//+---------------------------------------------------------------------------
//
//  Class:      CDispLayerContext
//
//  Synopsis:   This object's purpose in life to help us cache state between
//              layer calls.  This lets us factor the layers into their own
//              functions that can be called either by the main Draw function
//              or by a filter/behavior.
//
//              This class also provides nice simple abstractions the filter
//              can use to get or change the destination clip, offset and
//              drawing surface.
//
//              The interface of this class is designed with a COM interface
//              in mind.  When we start exposing this to binary behaviors we
//              want a clean story.
//
//              When receiving one of these objects in a Draw call, the filter or
//              behavior can change the surface, clip and/or offset.  These
//              attributes are totally independent so changing the surface will
//              not affect the clip or the offset.  Similarly, changing the offset
//              will not cause the clip to be offset.
//
//              The typical caller will change none or all three of surface, clip
//              and offset.
//
//              Because some of the methods on this class can change the associated
//              context, the constructor squirrels away the original values and
//              the private method RestoreContextState() puts them all back.  If
//              you need to reset the saved values, call SaveContextState().
//
//----------------------------------------------------------------------------

class CDispLayerContext
{
    friend class CDispNode;
    friend class CDispItemPlus;
    friend class CDispContainer;
    friend class CDispContentNode;
    friend class CDispScroller;
    friend class CDispRoot;
    
public:
    // External Interface
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CDispLayerContext))

    HRESULT GetDC(HDC *phdc);
    HRESULT GetDDSurface(IDirectDrawSurface **ppDDSurface);
    CDispSurface *GetSurface() { return _pDrawContext->GetDispSurface(); }

    // The two following methods are mutually exclusive.
    // Either one replaces the other.
    // In the case of SetDC, the DC is still owned by
    // the caller.  Obviously the DirectDraw case uses
    // ref counting
    //
    HRESULT SetDC(HDC hdc);
    HRESULT SetDDSurface(IDirectDrawSurface *pDDSurface);
    HRESULT GetClip(LPRECT prc); // Caller must provide a region, we stomp on it
    HRESULT SetClip(LPCRECT prc); // Caller retains ownership
    HRESULT GetOffset(POINT *pt);
    HRESULT SetOffset(const POINT *pt);

    // Draw the unfiltered version of the layer.
	void DrawLayer(DISPNODELAYER layer); // default destination
    void DrawLayer(HDC hdc, long x, long y, DISPNODELAYER layer); // different destination

    DISPNODELAYER GetLayer() const {return _layer;}
    
    void SaveContextState();
    void RestoreContextState();

private:
    CDispLayerContext(
        CDispDrawContext *pDrawContext,
        CDispExtras* pExtras,
        CDispInfo* pDispInfo,
        CDispNode *pDispNode,
        CDispNode *pCurrentChildNode = NULL)
    {
        ZeroMemory(this, sizeof(*this));
        _pDrawContext = pDrawContext;
        _pExtras = pExtras;
        _pDispInfo = pDispInfo;
        _pDispNode = pDispNode;
        _pCurrentChildNode = pCurrentChildNode;
    }

    ~CDispLayerContext() {}

    CDispContext _saveContext;
    CDispSurface *_pDispSurfaceSave;
    CRegion *_pregionSave;
	CRegion *_prgnRedrawSave;
    CDispNode *_pCurrentChildNodeSave;

    CDispDrawContext *_pDrawContext;
	CDispExtras *_pExtras;
    CDispInfo *_pDispInfo;
    CDispNode *_pDispNode;
    CDispNode *_pCurrentChildNode;

    DWORD _dwClientLayersMask;
    DWORD _dwFilterLayersMask;
    DISPNODELAYER _layer;
};
#endif

#pragma INCMSG("--- End 'dispcontext.hxx'")
#else
#pragma INCMSG("*** Dup 'dispcontext.hxx'")
#endif
