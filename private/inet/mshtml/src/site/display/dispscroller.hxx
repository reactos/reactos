//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispscroller.hxx
//
//  Contents:   Simple scrolling container.
//
//----------------------------------------------------------------------------

#ifndef I_DISPSCROLLER_HXX_
#define I_DISPSCROLLER_HXX_
#pragma INCMSG("--- Beg 'dispscroller.hxx'")

#ifndef X_DISPCONTAINER_HXX_
#define X_DISPCONTAINER_HXX_
#include "dispcontainer.hxx"
#endif

MtExtern(CDispScroller)


//+---------------------------------------------------------------------------
//
//  Class:      CDispScroller
//
//  Synopsis:   Simple scrolling container.
//
//----------------------------------------------------------------------------

class CDispScroller :
    public CDispContainer
{
    friend class CDispNode;
    DECLARE_DISPNODE(CDispScroller, CDispContainer, DISPNODETYPE_SCROLLER)

protected:
    CSize       _sizeScrollOffset;
    CSize       _sizeContent;
    CSize       _sizeScrollbars;
    BOOL        _fHasHScrollbar     : 1;
    BOOL        _fHasVScrollbar     : 1;
    BOOL        _fForceHScrollbar   : 1;
    BOOL        _fForceVScrollbar   : 1;
    BOOL        _fInvalidHScrollbar : 1;
    BOOL        _fInvalidVScrollbar : 1;
    // 96 bytes (28 bytes + 68 bytes for CDispContainer super class)
    
    // object can be created only by derived classes, and destructed only from
    // special methods
protected:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDispScroller))
    
                            CDispScroller(CDispClient* pDispClient)
                                : CDispContainer(pDispClient) {}
                            ~CDispScroller() {}
    
public:
    BOOL                    SetScrollOffset(
                                const SIZE& offset,
                                BOOL fScrollBits);
    BOOL                    SetScrollOffsetSmoothly(
                                const SIZE& offset,
                                BOOL fScrollBits,
                                LONG lScrollTime);
    void                    GetScrollOffset(SIZE* pOffset) const
                                    {*pOffset = -_sizeScrollOffset;}
    
                            // BUGBUG (donmarsh) -- ForceScrollOffset is called
                            // when printing in order to set a scroll offset and
                            // skip the adjustment that prevents blank space at
                            // the bottom of the page.  This would be more safely
                            // accomplished with a state bit on CDispScroller
                            // that disables that adjustment.
    void                    ForceScrollOffset(const SIZE& offset)
                                    {_sizeScrollOffset = -(CSize&)offset;
                                    CDispInteriorNode::CalculateInView();}
    
    void                    CopyScrollOffset(CDispScroller* pOldScroller)
                                    {_sizeScrollOffset = pOldScroller->_sizeScrollOffset;}
    
    void                    SetVerticalScrollbarWidth(LONG width, BOOL fForce);
    LONG                    GetVerticalScrollbarWidth() const
                                    {return _sizeScrollbars.cx;}
    void                    SetHorizontalScrollbarHeight(LONG height, BOOL fForce);
    LONG                    GetHorizontalScrollbarHeight() const
                                    {return _sizeScrollbars.cy;}
    
    BOOL                    VerticalScrollbarIsForced() const
                                    {return _fForceVScrollbar;}
    BOOL                    HorizontalScrollbarIsForced() const
                                    {return _fForceHScrollbar;}
    
    BOOL                    VerticalScrollbarIsActive() const;
    BOOL                    HorizontalScrollbarIsActive() const;
    
    void                    GetContentSize(SIZE* psizeContent) const
                                    {*psizeContent = _sizeContent;}
    const CSize&            GetContentSize() const {return _sizeContent;}

    // CDispNode overrides
    virtual void            GetClientRect(RECT* prc, CLIENTRECT type) const;
    virtual void            SetSize(const SIZE& size, BOOL fInvalidateAll);
    virtual CDispScroller * HitScrollInset(CPoint *pptHit, DWORD *pdwScrollDir);
    
protected:
    // CDispNode overrides
    virtual BOOL            PreDraw(CDispDrawContext* pContext);
    virtual void            DrawSelf(
                                CDispDrawContext* pContext,
                                CDispNode* pChild);
    virtual BOOL            HitTestPoint(CDispHitContext* pContext) const;
    virtual void            GetNodeTransform(
                                CSize* pOffset,
                                COORDINATE_SYSTEM source,
                                COORDINATE_SYSTEM destination) const;
    virtual void            GetNodeTransform(
                                CDispContext* pContext,
                                COORDINATE_SYSTEM source,
                                COORDINATE_SYSTEM destination) const;
    virtual void            CalcDispInfo(
                                const CRect& rcClip,
                                CDispInfo* pdi) const;
    virtual BOOL            ScrollRectIntoView(
                                const CRect& rc,
                                CRect::SCROLLPIN spVert,
                                CRect::SCROLLPIN spHorz,
                                COORDINATE_SYSTEM coordSystem,
                                BOOL fScrollBits);
    
    // CDispInteriorNode overrides
    virtual BOOL            ComputeVisibleBounds();
    virtual void            RecalcChildren(
                                BOOL fForceRecalc,
                                BOOL fSuppressInval,
                                CDispDrawContext* pContext);
    
    void                    DrawScrollbars(
                                CDispDrawContext* pContext,
                                DWORD dwFlags = 0);
    
    void                    GetVScrollbarRect(
                                CRect* prcVScrollbar,
                                const CDispInfo& di) const;
    void                    GetHScrollbarRect(
                                CRect* prcHScrollbar,
                                const CDispInfo& di) const;

    // this is a special accelerator for the top-level scroll node, so we
    // don't have to render the top-level border and scroll bars in a banded
    // fashion.
    virtual void            DrawBorderAndScrollbars(
                                CDispDrawContext* pContext,
                                CRect* prcContent);

private:
    BOOL                    CalcScrollbars();
    void                    InvalidateScrollbars();
    void                    InvalidateScrollbars(
                                CRegion* prgnInvalid,
                                CDispInfo* pdi);
    void                    RecalcScroller()
                                    {SetFlag(CDispFlags::s_recalcChildren);
                                    RequestRecalc();}
    
#if DBG==1
protected:
    virtual void            DumpBounds(HANDLE hFile, long level, long childNumber);
#endif
};


#pragma INCMSG("--- End 'dispscroller.hxx'")
#else
#pragma INCMSG("*** Dup 'dispscroller.hxx'")
#endif

