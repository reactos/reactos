//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       disproot.hxx
//
//  Contents:   CDispRoot, an interior node at the root of a display tree.
//
//----------------------------------------------------------------------------

#ifndef I_DISPROOT_HXX_
#define I_DISPROOT_HXX_
#pragma INCMSG("--- Beg 'disproot.hxx'")

#ifndef X_DISPCONTAINER_HXX_
#define X_DISPCONTAINER_HXX_
#include "dispcontainer.hxx"
#endif

#ifndef X_DISPCONTEXT_HXX_
#define X_DISPCONTEXT_HXX_
#include "dispcontext.hxx"
#endif

#ifndef X_DISPSURFACE_HXX_
#define X_DISPSURFACE_HXX_
#include "dispsurface.hxx"
#endif

class CDispItemPlus;
class CDispGroup;

MtExtern(CDispRoot)

//+---------------------------------------------------------------------------
//
//  Class:      CDispRoot
//
//  Synopsis:   A container node at the root of a display tree.
//
//----------------------------------------------------------------------------

class CDispRoot :
    public CDispContainer
{
    DECLARE_DISPNODE(CDispRoot, CDispContainer, DISPNODETYPE_ROOT)

    friend class CDispNode;
    friend class CDispScroller;
    
public:
    // for debugging, we store a pointer to the current URL here
    void*               _debugUrl;
    
protected:
    IDispObserver*      _pDispObserver;
    int                 _cOpen;
    BOOL                _fDrawLock;
    BOOL                _fRecalcLock;
    CDispDrawContext    _drawContext;
    CDispSurface*       _pRenderSurface;
    CDispSurface*       _pOffscreenBuffer;
    CRect               _rcContent;
    BOOL                _fCanScrollDC;
    BOOL                _fCanSmoothScroll;
    BOOL                _fDirectDraw;
    BOOL                _fTexture;
    BOOL                _fAllowOffscreen;
    BOOL                _fWantOffscreen;
    short               _cBufferDepth;
    HPALETTE            _hpal;

    void                    CheckReenter() const
                                    {AssertSz(!_fDrawLock,
                                     "Open/CloseDisplayTree prohibited during Draw()");}
    void                    ReleaseRenderSurface()
                                    {delete _pRenderSurface;
                                     _pRenderSurface = NULL;}
    void                    ReleaseOffscreenBuffer();
                                    
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDispRoot))

                            CDispRoot(
                                IDispObserver* pObserver,
                                CDispClient* pDispClient);
    
    // call Destroy instead of delete
protected:
                            ~CDispRoot();
public:
    // this call destroys the entire tree in an optimal fashion, without
    // executing any destructors.  You must be sure that nothing is still
    // holding a pointer to any node in the tree!
    void                    DestroyTreeWithPrejudice();

    //
    // How we paint and where.  This covers the final destination, buffering and so on.
    //
    void                    SetDestination(HDC hdc, IDirectDrawSurface* pSurface);
   
    // lines = -1 for no buffering, 0 for full size, > 0 for number of lines
    // (in pixels) to buffer
    void                    SetOffscreenBufferInfo(HPALETTE hpal, short bufferDepth, BOOL fDirectDraw, BOOL fTexture, BOOL fWantOffscreen, BOOL fAllowOffscreen);
    BOOL                    SetupOffscreenBuffer(CDispDrawContext *pContext);

    void                    SetRootSize(const SIZE& size, BOOL fInvalidateAll);
    const POINT &           GetRootPosition() const
                                    { return _rcContainer.TopLeft(); }
    void                    GetRootPosition(POINT * ppt) const
                                    {*ppt = _rcContainer.TopLeft();}
    void                    SetRootPosition(const POINT& pt)
                                    {_rcContainer.MoveTo(pt);
                                     _rcVisBounds.MoveTo(pt);}
    
    BOOL                    SetContentOffset(const SIZE& size);
    
    // return a pointer to the root node's display context
    // NOTE: returns a non-AddRef'd pointer
    CDispDrawContext*       GetDrawContext() {return &_drawContext;}

    // DrawRoot contains special optimizations for dealing with scrollbars of
    // its first child
    void                    DrawRoot(
                                CDispDrawContext* pContext,
                                void* pClientData,
                                HRGN hrgnDraw = NULL,
                                const RECT* prcDraw = NULL);

    // DrawNode draws the given node on the given surface without any special
    // optimizations
    void                    DrawNode(
                                CDispNode* pNode,
                                CDispSurface* pSurface,
                                const POINT& ptOrg,
                                HRGN rgnClip,
                                void* pClientData);
    
    // hit testing
    BOOL                    HitTest(
                                CPoint* pptHit,
                                COORDINATE_SYSTEM coordinateSystem,
                                void* pClientData,
                                BOOL fHitContent,
                                long cFuzzyHitTest = 0);
    
    // quickly draw border and background
    void                    EraseBackground(
                                CDispDrawContext* pContext,
                                void* pClientData,
                                HRGN hrgnDraw = NULL,
                                const RECT* prcDraw = NULL);
    
#if DBG==1
    void                    OpenDisplayTree();
    void                    CloseDisplayTree();
#else
    void                    OpenDisplayTree()
                                    {_cOpen++;}
    void                    CloseDisplayTree()
                                    {if (_cOpen == 1) RecalcRoot(); _cOpen--;}
#endif

    BOOL                    DisplayTreeIsOpen() const
                                    {return (_cOpen > 0);}
    
    void                    RecalcRoot();
    
    void                    SetObserver(IDispObserver* pObserver)
                                    {_pDispObserver = pObserver;}
    IDispObserver*          GetObserver()
                                    {return _pDispObserver;}

    void                    SetCanSmoothScroll(BOOL fSmooth = TRUE)
                                    {_fCanSmoothScroll = fSmooth;}
    BOOL                    CanSmoothScroll() const
                                    {return _fCanSmoothScroll;}
    
    void                    SetCanScrollDC(BOOL fCanScrollDC = TRUE)
                                    {_fCanScrollDC = fCanScrollDC;}
    BOOL                    CanScrollDC() const
                                    {return _fCanScrollDC;}
    
    // static creation methods
    static CDispItemPlus*   CreateDispItemPlus(
                                CDispClient* pDispClient,
                                BOOL fHasExtraCookie,
                                BOOL fHasUserClip,
                                BOOL fHasInset,
                                DISPNODEBORDER borderType,
                                BOOL fRightToLeft);
    static CDispGroup*      CreateDispGroup(
                                CDispClient* pDispClient);
    static CDispContainer*  CreateDispContainer(
                                CDispClient* pDispClient,
                                BOOL fHasExtraCookie,
                                BOOL fHasUserClip,
                                BOOL fHasInset,
                                DISPNODEBORDER borderType,
                                BOOL fRightToLeft);
    static CDispContainer*  CreateDispContainer(
                                const CDispItemPlus* pItemPlus);
    static CDispScroller*   CreateDispScroller(
                                CDispClient* pDispClient,
                                BOOL fHasExtraCookie,
                                BOOL fHasUserClip,
                                BOOL fHasInset,
                                DISPNODEBORDER borderType,
                                BOOL fRightToLeft);

    void                    InvalidateRoot(const CRegion& rgn,
                                BOOL fSynchronousRedraw,
                                BOOL fInvalChildWindows);
    void                    InvalidateRoot(const CRect& rc,
                                BOOL fSynchronousRedraw,
                                BOOL fInvalChildWindows);
    
protected:
    // CDispNode overrides
    virtual BOOL            PreDraw(CDispDrawContext* pContext);
    virtual void            DrawSelf(
                                CDispDrawContext* pContext,
                                CDispNode* pChild);
    virtual BOOL            ScrollRectIntoView(
                                const CRect& rc,
                                CRect::SCROLLPIN spVert,
                                CRect::SCROLLPIN spHorz,
                                COORDINATE_SYSTEM coordinateSystem,
                                BOOL fScrollBits);

    // CDispInteriorNode overrides
    virtual void            PushContext(
                                const CDispNode* pChild,
                                CDispContextStack* pContextStack,
                                CDispContext* pContext) const;

    virtual void            RecalcChildren(
                                BOOL fForceRecalc,
                                BOOL fSuppressInval,
                                CDispDrawContext* pContext);
    
    BOOL                    ScrollRect(
                                const CRect& rcScroll,
                                const CSize& scrollDelta,
                                CDispScroller* pScroller,
                                const CRegion& rgnInvalid,
                                BOOL fMayScrollDC);
    
private:
    void                    DrawEntire(CDispDrawContext* pContext);
    void                    DrawBands(
                                CDispDrawContext* pContext,
                                CRegion* prgnRedraw,
                                const CRegionStack& redrawRegionStack);
    void                    DrawBand(CDispDrawContext* pContext);
    BOOL                    SmoothScroll(CDispDrawContext* pContext);
};


#pragma INCMSG("--- End 'disproot.hxx'")
#else
#pragma INCMSG("*** Dup 'disproot.hxx'")
#endif
