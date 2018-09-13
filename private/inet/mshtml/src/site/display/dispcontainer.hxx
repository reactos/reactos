//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispcontainer.hxx
//
//  Contents:   Basic container node which introduces a new coordinate system
//              and clipping.
//
//----------------------------------------------------------------------------

#ifndef I_DISPCONTAINER_HXX_
#define I_DISPCONTAINER_HXX_
#pragma INCMSG("--- Beg 'dispcontainer.hxx'")

#ifndef X_DISPCONTENT_HXX_
#define X_DISPCONTENT_HXX_
#include "dispcontent.hxx"
#endif

MtExtern(CDispContainer)


//+---------------------------------------------------------------------------
//
//  Class:      CDispContainer
//
//  Synopsis:   Basic container node which introduces a new coordinate system
//              and clipping.
//
//----------------------------------------------------------------------------

class CDispContainer :
    public CDispContentNode
{
    friend class CDispNode;
    DECLARE_DISPNODE(CDispContainer, CDispContentNode, DISPNODETYPE_CONTAINER)

protected:
    CRect       _rcContainer;
    // 68 bytes (16 bytes + 52 bytes for CDispContentNode super class)
    
    // object can be created only by CDispRoot, and destructed only from
    // special methods
protected:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDispContainer))

                            CDispContainer(CDispClient* pDispClient)
                                : CDispContentNode(pDispClient) {}
                            CDispContainer(const CDispItemPlus* pItemPlus);
                            ~CDispContainer() {}

public:
    // size and position
    virtual void            GetSize(SIZE* psize) const
                                    {_rcContainer.GetSize(psize);}
    virtual void            SetSize(const SIZE& size, BOOL fInvalidateAll)
                                    {SetSize(size, g_Zero.rc, fInvalidateAll);}
    virtual const CPoint&   GetPosition() const
                                    {return _rcContainer.TopLeft();}
    virtual void            GetPositionTopRight(CPoint* pptTopRight)
                                    {_rcContainer.GetTopRight(pptTopRight);}
    virtual void            SetPosition(const POINT& ptTopLeft);
    virtual void            SetPositionTopRight(const POINT& ptTopRight);
    virtual void            GetBounds(RECT* prcBounds) const
                                    {*prcBounds = _rcContainer;}
    virtual const CRect&    GetBounds() const
                                    {return _rcContainer;}
    virtual void            FlipBounds()
                                    {_rcContainer.MirrorX();}
    void                    SetRightToLeft(BOOL fRightToLeft)
                                    {SetBoolean(CDispFlags::s_rightToLeft,fRightToLeft);}
    
    // CDispNode overrides
    virtual void            GetClientRect(RECT* prc, CLIENTRECT type) const;
    virtual BOOL            ScrollIntoView(
                                CRect::SCROLLPIN spVert,
                                CRect::SCROLLPIN spHorz)
                                    {return _pParentNode != NULL &&
                                        _pParentNode->ScrollRectIntoView(
                                        _rcContainer, spVert, spHorz,
                                        GetContentCoordinateSystem(), TRUE);}
    virtual CDispScroller * HitScrollInset(CPoint *pptHit, DWORD *pdwScrollDir);
   
protected:
    void                    SetSize(
                                const SIZE& size,
                                const RECT& rcBorderWidths,
                                BOOL fInvalidateAll);
    
    // this is a special accelerator for the top-level node, so we
    // don't have to render the top-level border and scroll bars in a banded
    // fashion.
    virtual void            DrawBorderAndScrollbars(
                                CDispDrawContext* pContext,
                                CRect* prcContent);
    
    // CDispNode overrides
    virtual BOOL            PreDraw(CDispDrawContext* pContext);
    virtual void            DrawSelf(CDispDrawContext* pContext, CDispNode* pChild);
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
    
    // CDispInteriorNode overrides
    virtual void            PushContext(
                                const CDispNode* pChild,
                                CDispContextStack* pContextStack,
                                CDispContext* pContext) const;
    virtual BOOL            ComputeVisibleBounds();
    virtual BOOL            SubtractOpaqueChildren(
                                CRegion* prgn,
                                CDispContext* pContext);
    virtual BOOL            CalculateInView(
                                CDispContext* pContext,
                                BOOL fPositionChanged,
                                BOOL fNoRedraw);
    virtual void            RecalcChildren(
                                BOOL fForceRecalc,
                                BOOL fSuppressInval,
                                CDispDrawContext* pContext);
    
private:
    void                    DrawBorder(CDispDrawContext* pContext, CDispInfo* pDI);
    void                    DrawBackground(CDispDrawContext* pContext, CDispInfo* pDI);
    void                    DrawChildren(
                                DISPNODELAYER layer,
                                const CSize& offset,
                                const CRect& rcClip,
                                CDispDrawContext* pContext,
                                CDispNode** ppChildNode);
    void                    DrawClientLayer(
                                CDispDrawContext* pContext,
                                const CDispInfo& di,
                                DWORD dwClientLayer);
    
#if DBG==1
protected:
    virtual void            DumpBounds(HANDLE hFile, long level, long childNumber);
#endif
};


#pragma INCMSG("--- End 'dispcontainer.hxx'")
#else
#pragma INCMSG("*** Dup 'dispcontainer.hxx'")
#endif

