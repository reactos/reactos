//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       displeaf.hxx
//
//  Contents:   Abstract base class for all leaf nodes in the display tree.
//
//----------------------------------------------------------------------------

#ifndef I_DISPLEAF_HXX_
#define I_DISPLEAF_HXX_
#pragma INCMSG("--- Beg 'displeaf.hxx'")

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_DISPINTERIOR_HXX_
#define X_DISPINTERIOR_HXX_
#include "dispinterior.hxx"
#endif


//+---------------------------------------------------------------------------
//
//  Class:      CDispLeafNode
//
//  Synopsis:   Abstract base class for all leaf nodes in the display tree.
//
//----------------------------------------------------------------------------

class CDispLeafNode :
    public CDispNode
{
    DECLARE_DISPNODE_ABSTRACT(CDispLeafNode, CDispNode)
    
    friend class CDispNode;
    friend class CDispInteriorNode;

protected:
    // no additional data
    // 36 bytes (0 bytes + 36 bytes for CDispNode base class)
    
protected:
    // object can be created only by derived classes, and destructed only from
    // special methods
                            CDispLeafNode()
                                : CDispNode() {}
    virtual                 ~CDispLeafNode() {}
    
    
public:
    // size and position
    virtual void            GetSize(SIZE* psize) const
                                    {_rcVisBounds.GetSize(psize); }
    virtual void            SetSize(const SIZE& size, BOOL fInvalidateAll)
                                    {SetSize(size, g_Zero.rc, fInvalidateAll);}
    virtual const CPoint&   GetPosition() const
                                    {return _rcVisBounds.TopLeft();}
    virtual void            GetPositionTopRight(CPoint* pptTopRight)
                                    {_rcVisBounds.GetTopRight(pptTopRight);}
    virtual void            SetPosition(const POINT& ptTopLeft);
    virtual void            SetPositionTopRight(const POINT& ptTopRight);
    virtual void            FlipBounds()
                                    {_rcVisBounds.MirrorX();}
    
    virtual void*           GetCookie() const {return NULL;}
    
protected:
    // CDispLeafNode virtuals
    virtual void            NotifyInViewChange(
                                CDispContext* pContext,
                                BOOL fResolvedVisible,
                                BOOL fWasResolvedVisible,
                                BOOL fNoRedraw) = 0;
    
    void                    SetSize(
                                const SIZE& size,
                                const RECT& rcBorderWidths,
                                BOOL fInvalidateAll);
};


#pragma INCMSG("--- End 'displeaf.hxx'")
#else
#pragma INCMSG("*** Dup 'displeaf.hxx'")
#endif

